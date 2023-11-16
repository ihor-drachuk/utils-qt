/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <optional>
#include <QVariant>

namespace UtilsQt {
namespace QVariantConv {

namespace Internal {

//template<typename T>
//void test(T value) {
//    (void)value;
//}

template<int value0, int... values>
struct Matcher
{
    static bool match(const QVariant& value) {
        return (value.type() == value0) || Matcher<values...>::match(value);
    }
};

template<int value0>
struct Matcher<value0>
{
    static bool match(const QVariant& value) { /*test(value.type());*/ return value.type() == value0; }
};

template<typename T> struct TypeTools { };
template<> struct TypeTools<int>                : Matcher<QVariant::Type::Int>        { static auto convert(const QVariant& variant, bool& ok) { return variant.toInt(&ok); } };
template<> struct TypeTools<unsigned int>       : Matcher<QVariant::Type::UInt>       { static auto convert(const QVariant& variant, bool& ok) { return variant.toUInt(&ok); } };
template<> struct TypeTools<long long>          : Matcher<QVariant::Type::LongLong>   { static auto convert(const QVariant& variant, bool& ok) { return variant.toLongLong(&ok); } };
template<> struct TypeTools<unsigned long long> : Matcher<QVariant::Type::ULongLong>  { static auto convert(const QVariant& variant, bool& ok) { return variant.toULongLong(&ok); } };
template<> struct TypeTools<double>             : Matcher<QVariant::Type::Double>     { static auto convert(const QVariant& variant, bool& ok) { return variant.toDouble(&ok); } };
template<> struct TypeTools<float>              : Matcher<QMetaType::Type::Float>     { static auto convert(const QVariant& variant, bool& ok) { return variant.toFloat(&ok); } };
template<> struct TypeTools<bool>               : Matcher<QVariant::Type::Bool>       { static auto convert(const QVariant& variant, bool& ok) { ok = true; return variant.toBool(); } };
template<> struct TypeTools<char>               : Matcher<QVariant::Type::Int>        { static auto convert(const QVariant& variant, bool& ok) { auto rsl = variant.toInt(&ok); ok &= (rsl >= 0 && rsl <= 255); return rsl; } };
template<> struct TypeTools<QChar>              : Matcher<QVariant::Type::Char>       { static auto convert(const QVariant& variant, bool& ok) { ok = true; return variant.toChar(); } };
template<> struct TypeTools<QString>            : Matcher<QVariant::Type::String>     { static auto convert(const QVariant& variant, bool& ok) { ok = true; return variant.toString(); } };

} // namespace Internal

enum Checks
{
    NoCheck = 0,

    Check_NonNull_Valid = 1,
    Check_Type = 2,
    Check_ConvResult = 4,

    CheckAll = Check_NonNull_Valid |
               Check_Type |
               Check_ConvResult
};

template<typename T,
         typename std::enable_if<!std::is_enum<T>::value>::type* = nullptr>
bool load(const QVariant& src, T& dst, Checks checks = CheckAll)
{
    if (checks & Check_NonNull_Valid)
        if (src.isNull() || !src.isValid())
            return false;

    if (checks & Check_Type)
        if (!Internal::TypeTools<T>::match(src))
            return false;

    bool ok = false;
    T result = Internal::TypeTools<T>::convert(src, ok);

    if ((checks & Check_ConvResult) == 0)
        ok = true;

    if (ok)
        dst = result;

    return ok;
}

template<typename T,
         typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
bool load(const QVariant& src, T& dst, Checks checks = CheckAll)
{
    int value;
    auto ok = load(src, value, checks);

    if (ok)
        dst = static_cast<T>(value);

    return ok;
}

template<typename T>
std::optional<T> load(const QVariant& src, Checks checks = CheckAll)
{
    T result;
    bool ok = load(src, result, checks);
    return ok ? std::optional<T>(result) : std::nullopt;
}

} // namespace QVariantConv
} // namespace UtilsQt

inline UtilsQt::QVariantConv::Checks operator | (UtilsQt::QVariantConv::Checks a, UtilsQt::QVariantConv::Checks b)
{
    return (UtilsQt::QVariantConv::Checks)((int)a | (int)b);
}

inline UtilsQt::QVariantConv::Checks& operator |= (UtilsQt::QVariantConv::Checks& a, UtilsQt::QVariantConv::Checks b)
{
    (int&)a |= (int)b;
    return a;
}

inline UtilsQt::QVariantConv::Checks operator & (UtilsQt::QVariantConv::Checks a, UtilsQt::QVariantConv::Checks b)
{
    return (UtilsQt::QVariantConv::Checks)((int)a & (int)b);
}

inline UtilsQt::QVariantConv::Checks& operator &= (UtilsQt::QVariantConv::Checks& a, UtilsQt::QVariantConv::Checks b)
{
    (int&)a &= (int)b;
    return a;
}

inline UtilsQt::QVariantConv::Checks operator ~ (UtilsQt::QVariantConv::Checks a)
{
    return (UtilsQt::QVariantConv::Checks)(~(int)a & (int)(UtilsQt::QVariantConv::Checks::CheckAll));
}
