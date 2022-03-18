#include <utils-qt/jsonvalidator.h>

#include <QJsonDocument>
#include <QMap>
#include <QDebug>
#include <cassert>

namespace UtilsQt {
namespace JsonValidator {

namespace Internal {

class OrProxyLogger : public Logger
{
public:
    void notifyError(const QString& path, const QString& error) override {
        m_lastPath = path;
        m_lastError = error;
    }

    bool hasNotifiedError() const override { return !m_lastPath.isEmpty() || !m_lastError.isEmpty(); }
    void reset() { m_lastPath.clear(); m_lastError.clear(); }

    const QString& lastPath() const { return m_lastPath; }
    const QString& lastError() const { return m_lastError; }

private:
    QString m_lastPath;
    QString m_lastError;
};


const QMap<QJsonValue::Type, QString> jsonTypeToString = {
    {QJsonValue::Type::Null, "Null"},
    {QJsonValue::Type::Bool, "Boolean"},
    {QJsonValue::Type::Double, "Number"},
    {QJsonValue::Type::String, "String"},
    {QJsonValue::Type::Array, "Array"},
    {QJsonValue::Type::Object, "Object"},
    {QJsonValue::Type::Undefined, "Undefined"}
};

QString valueToString(const QJsonValue& value)
{
    QJsonDocument doc;

    if (value.isArray()) {
        doc.setArray(value.toArray());
    } else if (value.isObject()) {
        doc.setObject(value.toObject());
    } else {
        QJsonObject obj;
        obj["value"] = value;
        doc.setObject(obj);
    }

    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

bool Object::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    if (!value.isObject()) {
        logger.notifyError(path, "Object expected, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(logger, path, value);
}

bool Array::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    if (!value.isArray()) {
        logger.notifyError(path, "Array expected, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    auto array = value.toArray();

    for (int i = 0; i < array.size(); i++) {
        const auto nestedValue = array.at(i);
        const auto newPath = path + QString("[%1]").arg(i);
        const auto iResult = checkNested(logger, newPath, nestedValue);

        if (!iResult)
            return false;
    }

    return true;
}

bool Field::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    assert(value.isObject());
    auto obj = value.toObject();

    if (obj.contains(m_key)) {
        const auto nestedValue = obj.value(m_key);
        const auto newPath = path + "/" + m_key;

        return checkNested(logger, newPath, nestedValue);
    } else {
        if (m_optional) { // It's OK.
            return true;
        } else {
            logger.notifyError(path, "Expected field \"" + m_key + "\", but it's missing!");
            return false;
        }
    }
}

bool Or::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    OrProxyLogger proxyLogger;

    for (const auto& x : nestedValidators()) {
        proxyLogger.reset();
        auto ok = x->check(proxyLogger, path, value);
        if (ok) return true;
    }

    assert(proxyLogger.hasNotifiedError());
    logger.notifyError(proxyLogger.lastPath(), proxyLogger.lastError());

    return false;
}

bool String::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    auto ok = value.isString();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"String\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    if (m_nonEmpty && value.toString().isEmpty()) {
        logger.notifyError(path, "Expected non-empty string, but it's empty");
        return false;
    }

    return checkNested(logger, path, value);
}

bool Bool::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    auto ok = value.isBool();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"Boolean\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(logger, path, value);
}

bool Number::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    auto ok = value.isDouble();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"Number\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(logger, path, value);
}

bool Exclude::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    for (const auto& x : m_values) {
        if (value == x) {
            logger.notifyError(path, "Value shouldn't be equal to: " + valueToString(value));
            return false;
        }
    }

    return true;
}

bool Include::check(Logger& logger, const QString& path, const QJsonValue& value)
{
    for (const auto& x : m_values) {
        if (value == x)
            return true;
    }

    logger.notifyError(path, "This value doesn't match to 'include' filter: " + valueToString(value));
    return false;
}

} // namespace Internal


void Logger::notifyError(const QString& path, const QString& error)
{
    qCritical().noquote() << "Parse failed at path: \"" + (path.isEmpty() ? "/" : path) + "\"";
    qCritical().noquote() << "Error: " << error;
    setNotifiedFlag();
}

bool Validator::checkNested(Logger& logger, const QString& path, const QJsonValue& value)
{
    for (const auto& x : m_validators) {
        auto ok = x->check(logger, path, value);

        if (!ok) {
            assert(logger.hasNotifiedError());
            return false;
        }
    }

    return true;
}

} // namespace JsonValidator
} // namespace UtilsQt
