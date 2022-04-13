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

bool Object::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    if (!value.isObject()) {
        logger.notifyError(path, "Object expected, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(ctx, logger, path, value);
}

bool Array::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    if (!value.isArray()) {
        logger.notifyError(path, "Array expected, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    auto array = value.toArray();

    for (int i = 0; i < array.size(); i++) {
        const auto nestedValue = array.at(i);
        const auto newPath = path + QString("[%1]").arg(i);
        const auto iResult = checkNested(ctx, logger, newPath, nestedValue);

        if (!iResult)
            return false;
    }

    return true;
}

bool Field::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    assert(value.isObject());
    auto obj = value.toObject();

    if (obj.contains(m_key)) {
        const auto nestedValue = obj.value(m_key);
        const auto newPath = path + "/" + m_key;

        return checkNested(ctx, logger, newPath, nestedValue);
    } else {
        if (m_optional) { // It's OK.
            return true;
        } else {
            logger.notifyError(path, "Expected field \"" + m_key + "\", but it's missing!");
            return false;
        }
    }
}

bool Or::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    OrProxyLogger proxyLogger;

    for (const auto& x : nestedValidators()) {
        proxyLogger.reset();
        auto ok = x->check(ctx, proxyLogger, path, value);
        if (ok) return true;
    }

    assert(proxyLogger.hasNotifiedError());
    logger.notifyError(proxyLogger.lastPath(), proxyLogger.lastError());

    return false;
}

bool String::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
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

    if (m_hex) {
        auto str = value.toString();

        bool ok = false;

        if (str.left(2) == "0x") {
            str = str.mid(2);
            str.toLongLong(&ok, 16);
        }

        if (!ok) {
            logger.notifyError(path, "Expected HEX-number string, but isn't");
            return false;
        }
    }

    return checkNested(ctx, logger, path, value);
}

bool Bool::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    auto ok = value.isBool();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"Boolean\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(ctx, logger, path, value);
}

bool Number::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    auto ok = value.isDouble();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"Number\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(ctx, logger, path, value);
}

bool Exclude::check(ContextData& /*ctx*/, Logger& logger, const QString& path, const QJsonValue& value)
{
    for (const auto& x : m_values) {
        if (value == x) {
            logger.notifyError(path, "Value shouldn't be equal to: " + valueToString(value));
            return false;
        }
    }

    return true;
}

bool Include::check(ContextData& /*ctx*/, Logger& logger, const QString& path, const QJsonValue& value)
{
    for (const auto& x : m_values) {
        if (value == x)
            return true;
    }

    logger.notifyError(path, "This value doesn't match to 'include' filter: " + valueToString(value));
    return false;
}

bool And::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    return checkNested(ctx, logger, path, value);
}

bool RootValidator::check(Logger& logger, const QJsonValue& value)
{
    ContextData ctx;
    return check(ctx, logger, "", value);
}

bool RootValidator::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    return checkNested(ctx, logger, path, value);
}

bool CtxWriteArrayLength::check(ContextData& ctx, Logger& /*logger*/, const QString& /*path*/, const QJsonValue& value)
{
    assert(value.isArray());
    ctx[m_ctxField] = value.toArray().size();
    return true;
}

bool CtxCheckArrayLength::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    assert(value.isArray());
    assert(ctx.contains(m_ctxField));

    auto arrSize = value.toArray().size();
    auto expSize = ctx.value(m_ctxField).toInt();
    if (arrSize != expSize) {
        logger.notifyError(path, QString("Expected %1 items in array, but there %3 %2")
                           .arg(expSize)
                           .arg(arrSize)
                           .arg(arrSize >= 2 ? "are" : "is"));
        return false;
    }

    return true;
}

bool CtxAppendToList::check(ContextData& ctx, Logger& /*logger*/, const QString& /*path*/, const QJsonValue& value)
{
    assert(!ctx.contains(m_ctxField) || ctx.value(m_ctxField).type() == QVariant::Type::List);

    if (ctx.contains(m_ctxField)) {
        auto list = ctx.value(m_ctxField).toList();
        list.append(value);
        ctx[m_ctxField] = list;
    } else {
        ctx[m_ctxField] = QVariantList{value};
    }

    return true;
}

bool CtxCheckInList::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    assert(ctx.contains(m_ctxField) && ctx.value(m_ctxField).type() == QVariant::Type::List);

    auto list = ctx.value(m_ctxField).toList();

    if (!list.contains(value)) {
        logger.notifyError(path, QString("This value failed in-list check: %1").arg(valueToString(value)));
        return false;
    }

    return true;
}

bool CtxCheckNotInList::check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    assert(!ctx.contains(m_ctxField) || ctx.value(m_ctxField).type() == QVariant::Type::List);

    auto list = ctx.value(m_ctxField).toList();

    if (list.contains(value)) {
        logger.notifyError(path, QString("This value failed not-in-list check: %1").arg(valueToString(value)));
        return false;
    }

    return true;
}

bool CtxClearRecord::check(ContextData& ctx, Logger& /*logger*/, const QString& /*path*/, const QJsonValue& /*value*/)
{
    ctx.remove(m_ctxField);
    return true;
}

} // namespace Internal


void Logger::notifyError(const QString& path, const QString& error)
{
    qCritical().noquote() << "Validation failed at path: \"" + (path.isEmpty() ? "/" : path) + "\"";
    qCritical().noquote() << "Error: " << error;
    setNotifiedFlag();
}

bool Validator::checkNested(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value)
{
    for (const auto& x : m_validators) {
        auto ok = x->check(ctx, logger, path, value);

        if (!ok) {
            assert(logger.hasNotifiedError());
            return false;
        }
    }

    return true;
}

ValidatorPtr CtxWriteArrayLength(const QString& ctxField) { return std::make_shared<Internal::CtxWriteArrayLength>(ctxField); }
ValidatorPtr CtxCheckArrayLength(const QString& ctxField) { return std::make_shared<Internal::CtxCheckArrayLength>(ctxField); }
ValidatorPtr CtxAppendToList(const QString& ctxField) { return std::make_shared<Internal::CtxAppendToList>(ctxField); }
ValidatorPtr CtxCheckInList(const QString& ctxField) { return std::make_shared<Internal::CtxCheckInList>(ctxField); }
ValidatorPtr CtxCheckNotInList(const QString& ctxField) { return std::make_shared<Internal::CtxCheckNotInList>(ctxField); }
ValidatorPtr CtxClearRecord(const QString& ctxField) { return std::make_shared<Internal::CtxClearRecord>(ctxField); }

} // namespace JsonValidator
} // namespace UtilsQt
