#pragma once

#include <QString>
#include <QVariantMap>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include <vector>
#include <memory>

#include <utils-cpp/copy_move.h>
#include <utils-cpp/variadic_tools.h>

namespace UtilsQt {
namespace JsonValidator {

class Logger
{
public:
    DEFAULT_COPY_MOVE(Logger);

    Logger() = default;
    virtual ~Logger() = default;
    virtual void notifyError(const QString& path, const QString& error);

    virtual bool hasNotifiedError() const { return m_notifiedError; }

protected:
    void setNotifiedFlag() { m_notifiedError = true; }

private:
    mutable bool m_notifiedError { false };
};

class NullLogger : public Logger
{
public:
    void notifyError(const QString& /*path*/, const QString& /*error*/) override { setNotifiedFlag(); };
};

using ContextData = QVariantMap;

class Validator;
using ValidatorPtr = std::shared_ptr<Validator>;

class Validator
{
public:
    DEFAULT_COPY_MOVE(Validator);

    Validator(const std::vector<ValidatorPtr>& validators = {})
        : m_validators(validators)
    { }

    virtual ~Validator() = default;
    virtual bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) = 0;

protected:
    bool checkNested(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value);
    const std::vector<ValidatorPtr>& nestedValidators() const { return m_validators; }

private:
    std::vector<ValidatorPtr> m_validators;
};

using ValidatorPtr = std::shared_ptr<Validator>;

namespace Internal {

enum class _Optional { Optional };
enum class _NonEmpty { NonEmpty };
enum class _Hex { Hex };

template<typename... Ts>
std::vector<ValidatorPtr> convertValidators(const Ts&... validators)
{
    return variadic_to_container<std::vector, ValidatorPtr>(validators...);
}

class RootValidator : public Validator
{
public:
    RootValidator(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(Logger& logger, const QJsonValue& value);
    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

using RootValidatorPtr = std::shared_ptr<RootValidator>;

class Object : public Validator
{
public:
    Object(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

class Array : public Validator
{
public:
    Array(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

class Field : public Validator
{
public:
    Field(const QString& key, _Optional, const std::vector<ValidatorPtr>& validators = {})
        : Validator(validators),
          m_optional(true),
          m_key(key)
    {}

    Field(const QString& key, const std::vector<ValidatorPtr>& validators = {})
        : Validator(validators),
          m_optional(false),
          m_key(key)
    {}

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    bool m_optional;
    QString m_key;
};

class Or : public Validator
{
public:
    Or(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

class And : public Validator {
public:
    And(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

class String : public Validator
{
public:
    String(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    String(_NonEmpty, const std::vector<ValidatorPtr>& validators)
        : Validator(validators),
          m_nonEmpty(true)
    { }

    String(_Hex, const std::vector<ValidatorPtr>& validators)
        : Validator(validators),
          m_nonEmpty(true),
          m_hex(true)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    bool m_nonEmpty { false };
    bool m_hex { false };
};

class Bool : public Validator
{
public:
    Bool(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

class Number : public Validator
{
public:
    Number(const std::vector<ValidatorPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;
};

class Exclude : public Validator
{
public:
    using Values = std::vector<QJsonValue>;

    Exclude(const Values& values)
        : m_values(values)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    Values m_values;
};

class Include : public Validator
{
public:
    using Values = std::vector<QJsonValue>;

    Include(const Values& values)
        : m_values(values)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    Values m_values;
};

class CtxWriteArrayLength : public Validator
{
public:
    CtxWriteArrayLength(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    QString m_ctxField;
};

class CtxCheckArrayLength : public Validator
{
public:
    CtxCheckArrayLength(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    QString m_ctxField;
};

class CtxAppendToList : public Validator
{
public:
    CtxAppendToList(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    QString m_ctxField;
};

class CtxCheckInList : public Validator
{
public:
    CtxCheckInList(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    QString m_ctxField;
};

class CtxCheckNotInList : public Validator
{
public:
    CtxCheckNotInList(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    QString m_ctxField;
};

class CtxClearRecord : public Validator
{
public:
    CtxClearRecord(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) override;

private:
    QString m_ctxField;
};

} // namespace Internal

constexpr auto Optional = Internal::_Optional::Optional;
constexpr auto NonEmpty = Internal::_NonEmpty::NonEmpty;
constexpr auto Hex = Internal::_Hex::Hex;
using RootValidatorPtr = Internal::RootValidatorPtr;

template<typename... Ts>
RootValidatorPtr RootValidator(const Ts&... validators)
{
    return std::make_shared<Internal::RootValidator>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Object(const Ts&... validators)
{
    return std::make_shared<Internal::Object>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Array(const Ts&... validators)
{
    return std::make_shared<Internal::Array>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Field(const QString& key, Internal::_Optional opt, const Ts&... validators)
{
    return std::make_shared<Internal::Field>(key, opt, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Field(const QString& key, const Ts&... validators)
{
    return std::make_shared<Internal::Field>(key, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Or(const Ts&... validators)
{
    return std::make_shared<Internal::Or>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr And(const Ts&... validators)
{
    return std::make_shared<Internal::And>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr String(const Ts&... validators)
{
    return std::make_shared<Internal::String>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr String(Internal::_NonEmpty ne, const Ts&... validators)
{
    return std::make_shared<Internal::String>(ne, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr String(Internal::_Hex hex, const Ts&... validators)
{
    return std::make_shared<Internal::String>(hex, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Bool(const Ts&... validators)
{
    return std::make_shared<Internal::Bool>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Number(const Ts&... validators)
{
    return std::make_shared<Internal::Number>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorPtr Exclude(const Ts&... values)
{
    return std::make_shared<Internal::Exclude>(variadic_to_container<std::vector, QJsonValue>(values...));
}

template<typename... Ts>
ValidatorPtr Include(const Ts&... values)
{
    return std::make_shared<Internal::Include>(variadic_to_container<std::vector, QJsonValue>(values...));
}

ValidatorPtr CtxWriteArrayLength(const QString& ctxField);
ValidatorPtr CtxCheckArrayLength(const QString& ctxField);
ValidatorPtr CtxAppendToList(const QString& ctxField);
ValidatorPtr CtxCheckInList(const QString& ctxField);
ValidatorPtr CtxCheckNotInList(const QString& ctxField);
ValidatorPtr CtxClearRecord(const QString& ctxField);

} // namespace JsonValidator
} // namespace UtilsQt
