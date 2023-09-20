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
using ValidatorCPtr = std::shared_ptr<const Validator>;

class Validator
{
public:
    DEFAULT_COPY_MOVE(Validator);
    // ^ Notice! Only pointers are copied!
    //   But it's ok, they're pointers-to-constant, no modifications expected.

    Validator(const std::vector<ValidatorCPtr>& validators = {})
        : m_validators(validators)
    { }

    virtual ~Validator() = default;
    virtual bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const = 0;

protected:
    bool checkNested(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const;
    const std::vector<ValidatorCPtr>& nestedValidators() const { return m_validators; }

private:
    std::vector<ValidatorCPtr> m_validators;
};

namespace Internal {

enum class _Exclusive { Exclusive };
enum class _Optional { Optional };
enum class _NonEmpty { NonEmpty };
enum class _Hex { Hex };

template<typename... Ts>
std::vector<ValidatorCPtr> convertValidators(const Ts&... validators)
{
    return variadic_to_container<std::vector, ValidatorCPtr>(validators...);
}

class RootValidator : public Validator
{
public:
    RootValidator(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(Logger& logger, const QJsonValue& value) const;
    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;
};

using RootValidatorCPtr = std::shared_ptr<RootValidator>;

class Object : public Validator
{
public:
    Object(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;
};

class Array : public Validator
{
public:
    Array(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;
};

class Field : public Validator
{
public:
    Field(const QString& key, _Optional, const std::vector<ValidatorCPtr>& validators = {})
        : Validator(validators),
          m_optional(true),
          m_key(key)
    {}

    Field(const QString& key, const std::vector<ValidatorCPtr>& validators = {})
        : Validator(validators),
          m_optional(false),
          m_key(key)
    {}

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    bool m_optional;
    QString m_key;
};

class Or : public Validator
{
public:
    Or(_Exclusive, const std::vector<ValidatorCPtr>& validators)
        : Validator(validators),
          m_exclusive(true)
    { }

    Or(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    bool m_exclusive{false};
};

class And : public Validator {
public:
    And(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;
};

class String : public Validator
{
public:
    String(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    String(_NonEmpty, const std::vector<ValidatorCPtr>& validators)
        : Validator(validators),
          m_nonEmpty(true)
    { }

    String(_Hex, const std::vector<ValidatorCPtr>& validators)
        : Validator(validators),
          m_nonEmpty(true),
          m_hex(true)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    bool m_nonEmpty { false };
    bool m_hex { false };
};

class Bool : public Validator
{
public:
    Bool(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;
};

class Number : public Validator
{
public:
    Number(const std::vector<ValidatorCPtr>& validators)
        : Validator(validators)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;
};

class Exclude : public Validator
{
public:
    using Values = std::vector<QJsonValue>;

    Exclude(const Values& values)
        : m_values(values)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

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

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    Values m_values;
};

class CtxWriteArrayLength : public Validator
{
public:
    CtxWriteArrayLength(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    QString m_ctxField;
};

class CtxCheckArrayLength : public Validator
{
public:
    CtxCheckArrayLength(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    QString m_ctxField;
};

class CtxAppendToList : public Validator
{
public:
    CtxAppendToList(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    QString m_ctxField;
};

class CtxCheckInList : public Validator
{
public:
    CtxCheckInList(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    QString m_ctxField;
};

class CtxCheckNotInList : public Validator
{
public:
    CtxCheckNotInList(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    QString m_ctxField;
};

class CtxClearRecord : public Validator
{
public:
    CtxClearRecord(const QString& ctxField)
        : m_ctxField(ctxField)
    { }

    bool check(ContextData& ctx, Logger& logger, const QString& path, const QJsonValue& value) const override;

private:
    QString m_ctxField;
};

} // namespace Internal

constexpr auto Exclusive = Internal::_Exclusive::Exclusive;
constexpr auto Optional = Internal::_Optional::Optional;
constexpr auto NonEmpty = Internal::_NonEmpty::NonEmpty;
constexpr auto Hex = Internal::_Hex::Hex;
using RootValidatorCPtr = Internal::RootValidatorCPtr;

template<typename... Ts>
RootValidatorCPtr RootValidator(const Ts&... validators)
{
    return std::make_shared<Internal::RootValidator>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Object(const Ts&... validators)
{
    return std::make_shared<Internal::Object>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Array(const Ts&... validators)
{
    return std::make_shared<Internal::Array>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Field(const QString& key, Internal::_Optional opt, const Ts&... validators)
{
    return std::make_shared<Internal::Field>(key, opt, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Field(const QString& key, const Ts&... validators)
{
    return std::make_shared<Internal::Field>(key, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Or(Internal::_Exclusive exclusive, const Ts&... validators)
{
    return std::make_shared<Internal::Or>(exclusive, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Or(const Ts&... validators)
{
    return std::make_shared<Internal::Or>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr And(const Ts&... validators)
{
    return std::make_shared<Internal::And>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr String(const Ts&... validators)
{
    return std::make_shared<Internal::String>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr String(Internal::_NonEmpty ne, const Ts&... validators)
{
    return std::make_shared<Internal::String>(ne, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr String(Internal::_Hex hex, const Ts&... validators)
{
    return std::make_shared<Internal::String>(hex, Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Bool(const Ts&... validators)
{
    return std::make_shared<Internal::Bool>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Number(const Ts&... validators)
{
    return std::make_shared<Internal::Number>(Internal::convertValidators(validators...));
}

template<typename... Ts>
ValidatorCPtr Exclude(const Ts&... values)
{
    return std::make_shared<Internal::Exclude>(variadic_to_container<std::vector, QJsonValue>(values...));
}

template<typename... Ts>
ValidatorCPtr Include(const Ts&... values)
{
    return std::make_shared<Internal::Include>(variadic_to_container<std::vector, QJsonValue>(values...));
}

ValidatorCPtr CtxWriteArrayLength(const QString& ctxField);
ValidatorCPtr CtxCheckArrayLength(const QString& ctxField);
ValidatorCPtr CtxAppendToList(const QString& ctxField);
ValidatorCPtr CtxCheckInList(const QString& ctxField);
ValidatorCPtr CtxCheckNotInList(const QString& ctxField);
ValidatorCPtr CtxClearRecord(const QString& ctxField);

} // namespace JsonValidator
} // namespace UtilsQt
