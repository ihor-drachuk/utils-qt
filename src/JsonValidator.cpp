/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <UtilsQt/JsonValidator.h>

#include <QRegularExpression>
#include <QJsonDocument>
#include <QMap>
#include <QByteArray>
#include <QDebug>
#include <cassert>

#include <UtilsQt/qvariant_traits.h>

namespace UtilsQt {
namespace JsonValidator {

namespace Internal {

bool MinMaxValidatorInt::check(const QJsonValue& v) const {
    if (!v.isDouble())
        return false;

    bool ok;
    const auto value = v.toVariant().toLongLong(&ok);

    if (!ok)
        return false;

    if (min && value < *min)
        return false;

    if (max && value > *max)
        return false;

    return true;
}

bool MinMaxValidatorDouble::check(const QJsonValue& v) const {
    if (!v.isDouble())
        return false;

    const auto value = v.toDouble();

    if (min && qFuzzyCompare(value, *min))
        return true;

    if (max && qFuzzyCompare(value, *max))
        return true;

    if (min && value < *min)
        return false;

    if (max && value > *max)
        return false;

    return true;
}

namespace {

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

} // namespace

bool Object::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    if (!value.isObject()) {
        logger.notifyError(path, "Object expected, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(ctx, logger, path, value);
}

bool Array::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
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

bool Field::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
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

bool Or::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    ErrorInfo proxyLogger;

    size_t matchingItemsCnt = 0;

    for (const auto& x : nestedValidators()) {
        proxyLogger.clear();
        auto ok = x->check(ctx, proxyLogger, path, value);
        if (ok) {
            if (!m_exclusive) return true;
            matchingItemsCnt++;
        }
    }

    // We get here only if 'exclusive' logic enabled or
    // there is no matching items...

    // Handle 'exclusive' logic
    if (m_exclusive) {
        if (matchingItemsCnt == 1) {
            return true;

        } else if (matchingItemsCnt > 1) {
            logger.notifyError(proxyLogger.getErrorPath(), "Exclusive OR-condition expected, but several items are matching!");
            return false;
        }

        // If matchingItemsCnt == 0, then fallthrough...
    }

    // We get here only in cases, when there is no matching items
    // (independently of exclusive mode state)

    assert(proxyLogger.hasError());
    logger.notifyError(proxyLogger.getErrorPath(), proxyLogger.getErrorDescription());

    return false;
}

bool String::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
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

        bool hexOk = false;

        if (str.left(2) == "0x") {
            str = str.mid(2);

            if (str.size() % 2 == 0) // Size should be even when '0x' prefix is present
                str.toLongLong(&hexOk, 16);
        }

        if (!hexOk) {
            logger.notifyError(path, "Expected HEX-number string, but it isn't");
            return false;
        }
    }

    if (m_base64) {
        const auto strBase64 = value.toString().toLatin1();

        if (!strBase64.isEmpty()) {
            const auto buffer = QByteArray::fromBase64(strBase64, QByteArray::Base64Option::AbortOnBase64DecodingErrors);

            if (buffer.isEmpty()) {
                logger.notifyError(path, "Expected Base64-encoded string, but it isn't");
                return false;
            }
        }
    }

    if (m_ipv4) {
        const static QRegularExpression re(R"(^((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])$)");
        if (!re.match(value.toString()).hasMatch()) {
            logger.notifyError(path, "Expected IPv4 address, but it doesn't match the pattern");
            return false;
        }
    }

    return checkNested(ctx, logger, path, value);
}

bool Bool::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    auto ok = value.isBool();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"Boolean\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    return checkNested(ctx, logger, path, value);
}

bool Number::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    auto ok = value.isDouble();

    if (!ok) {
        logger.notifyError(path, "Expected value of \"Number\" type, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    if (m_isIntegerExpected) {
        bool ok;
        const auto v1 = value.toDouble();
        const auto v2 = value.toVariant().toLongLong(&ok);

        if (!ok) {
            logger.notifyError(path, QStringLiteral("Expected integer value, but it is not (%1)").arg(value.toString()));
            return false;
        }

        if (!qFuzzyCompare(v1, static_cast<double>(v2))) {
            logger.notifyError(path, QStringLiteral("Expected integer value, but it is not (%1)").arg(v1));
            return false;
        }
    }

    if (!m_validator->check(value)) {
        logger.notifyError(path,
                           QStringLiteral("Range validation failed. Min: %1, max: %2, actual: %3")
                             .arg(m_validator->getMin())
                             .arg(m_validator->getMax())
                             .arg(value.toDouble())
                           );
        return false;
    }

    return checkNested(ctx, logger, path, value);
}

bool Exclude::check(ContextData& /*ctx*/, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    for (const auto& x : m_values) {
        if (value == x) {
            logger.notifyError(path, "Value shouldn't be equal to: " + valueToString(value));
            return false;
        }
    }

    return true;
}

bool Include::check(ContextData& /*ctx*/, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    for (const auto& x : m_values) {
        if (value == x)
            return true;
    }

    logger.notifyError(path, "This value doesn't match to 'include' filter: " + valueToString(value));
    return false;
}

bool And::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    return checkNested(ctx, logger, path, value);
}

bool RootValidator::check(ErrorInfo& logger, const QJsonValue& value) const
{
    ContextData ctx;
    return check(ctx, logger, "", value);
}

bool RootValidator::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    return checkNested(ctx, logger, path, value);
}

bool CtxWriteArrayLength::check(ContextData& ctx, ErrorInfo& /*logger*/, const QString& /*path*/, const QJsonValue& value) const
{
    assert(value.isArray());
    ctx[m_ctxField] = value.toArray().size();
    return true;
}

bool CtxCheckArrayLength::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
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

bool CtxAppendToList::check(ContextData& ctx, ErrorInfo& /*logger*/, const QString& /*path*/, const QJsonValue& value) const
{
    assert(!ctx.contains(m_ctxField) || QVariantTraits::isList(ctx.value(m_ctxField)));

    if (ctx.contains(m_ctxField)) {
        auto list = ctx.value(m_ctxField).toList();
        list.append(value);
        ctx[m_ctxField] = list;
    } else {
        ctx[m_ctxField] = QVariantList{value};
    }

    return true;
}

bool CtxCheckInList::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    assert(ctx.contains(m_ctxField) && QVariantTraits::isList(ctx.value(m_ctxField)));

    auto list = ctx.value(m_ctxField).toList();

    if (!list.contains(value)) {
        logger.notifyError(path, QString("This value failed in-list check: %1").arg(valueToString(value)));
        return false;
    }

    return true;
}

bool CtxCheckNotInList::check(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    assert(!ctx.contains(m_ctxField) || QVariantTraits::isList(ctx.value(m_ctxField)));

    auto list = ctx.value(m_ctxField).toList();

    if (list.contains(value)) {
        logger.notifyError(path, QString("This value failed not-in-list check: %1").arg(valueToString(value)));
        return false;
    }

    return true;
}

bool CtxClearRecord::check(ContextData& ctx, ErrorInfo& /*logger*/, const QString& /*path*/, const QJsonValue& /*value*/) const
{
    ctx.remove(m_ctxField);
    return true;
}

bool ArrayLength::check(ContextData& /*ctx*/, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    if (!value.isArray()) {
        logger.notifyError(path, "Array expected, but it's of type \"" + jsonTypeToString.value(value.type()) + "\"");
        return false;
    }

    const size_t arraySize = static_cast<size_t>(value.toArray().size());
    if (m_min && arraySize < *m_min) {
        logger.notifyError(path, QString("Array length is too short: %1, but should be at least %2").arg(arraySize).arg(*m_min));
        return false;
    }

    if (m_max && arraySize > *m_max) {
        logger.notifyError(path, QString("Array length is too long: %1, but should be at most %2").arg(arraySize).arg(*m_max));
        return false;
    }

    if (m_strictLen && arraySize != *m_strictLen) {
        logger.notifyError(path, QString("Array length is %1, but should be exactly %2").arg(arraySize).arg(*m_strictLen));
        return false;
    }

    return true;
}

bool CustomValidator::check(ContextData& /*ctx*/, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    if (!m_validator(value)) {
        logger.notifyError(path, "Custom validation failed");
        return false;
    }

    return true;
}

} // namespace Internal

void ErrorInfo::notifyError(const QString& path, const QString& error)
{
    m_hasError = true;
    m_path = path.isEmpty() ? "/" : path;
    m_error = error;
    notifyErrorImpl(path, error);
}

void ErrorInfo::clear()
{
    m_hasError = false;
    m_path.clear();
    m_error.clear();
}

QString ErrorInfo::toString() const
{
    return QString("Error at '%1': %2").arg(m_path, m_error);
}

void LoggedErrorInfo::notifyErrorImpl(const QString& path, const QString& error)
{
    qCritical().noquote() << "Validation failed at path: \"" + (path.isEmpty() ? "/" : path) + "\"";
    qCritical().noquote() << "Error: " << error;
}

bool Validator::checkNested(ContextData& ctx, ErrorInfo& logger, const QString& path, const QJsonValue& value) const
{
    for (const auto& x : m_validators) {
        auto ok = x->check(ctx, logger, path, value);

        if (!ok) {
            assert(logger.hasError());
            return false;
        }
    }

    return true;
}

ValidatorCPtr CtxWriteArrayLength(const QString& ctxField) { return std::make_shared<Internal::CtxWriteArrayLength>(ctxField); }
ValidatorCPtr CtxCheckArrayLength(const QString& ctxField) { return std::make_shared<Internal::CtxCheckArrayLength>(ctxField); }
ValidatorCPtr CtxAppendToList(const QString& ctxField) { return std::make_shared<Internal::CtxAppendToList>(ctxField); }
ValidatorCPtr CtxCheckInList(const QString& ctxField) { return std::make_shared<Internal::CtxCheckInList>(ctxField); }
ValidatorCPtr CtxCheckNotInList(const QString& ctxField) { return std::make_shared<Internal::CtxCheckNotInList>(ctxField); }
ValidatorCPtr CtxClearRecord(const QString& ctxField) { return std::make_shared<Internal::CtxClearRecord>(ctxField); }

} // namespace JsonValidator
} // namespace UtilsQt
