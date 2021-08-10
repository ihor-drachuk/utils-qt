#pragma once
#include <type_traits>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <utils-cpp/checkmethod.h>
#include <utils-cpp/struct_ops.h>

CREATE_CHECK_METHOD(tie);
CREATE_CHECK_METHOD(tieMod);
CREATE_CHECK_METHOD(names);

/*
    Example
  -----------
  struct My_Struct
  {
      int a_;
      double b_;

      auto tie() const { return std::tie(a_, b_); }
      auto tieMod() { return std::tie(a_, b_); }
      static auto names() { return std::make_tuple("a_", "b_"); }
  };

  STRUCT_QVARIANT_LIST(My_Struct);
  STRUCT_QVARIANT_MAP(My_Struct);
*/

namespace struct_ops_internal {

// ---- qvariantFromValue ----

template<typename T,
         typename std::enable_if<CheckMethod::tie_v<T>, void>::type* = nullptr>
QVariant qvariantFromValue_List(const T& value)
{
    return toQVariantList(value);
}

template<typename T,
         typename std::enable_if<!CheckMethod::tie_v<T>, void>::type* = nullptr>
QVariant qvariantFromValue_List(const T& value)
{
    return QVariant::fromValue(value);
}

template<typename T,
         typename std::enable_if<CheckMethod::tie_v<T> && CheckMethod::names_v<T>, void>::type* = nullptr>
QVariant qvariantFromValue_Map(const T& value)
{
    return toQVariantMap(value);
}

template<typename T,
         typename std::enable_if<!(CheckMethod::tie_v<T> && CheckMethod::names_v<T>), void>::type* = nullptr>
QVariant qvariantFromValue_Map(const T& value)
{
    return QVariant::fromValue(value);
}

// ---- makeQVariantList ----

template<std::size_t I = 0, typename... Tp,
         typename std::enable_if<I == sizeof...(Tp) - 1, void>::type* = nullptr>
inline void makeQVariantListImpl(const std::tuple<Tp...>& t, QVariantList& result)
{
    auto value = qvariantFromValue_List(std::get<I>(t));
    result.append(value);
}

template<std::size_t I = 0, typename... Tp,
         typename std::enable_if<I < sizeof...(Tp) - 1, void>::type* = nullptr>
inline void makeQVariantListImpl(const std::tuple<Tp...>& t, QVariantList& result)
{
    auto value = qvariantFromValue_List(std::get<I>(t));
    result.append(value);
    makeQVariantListImpl<I+1>(t, result);
}

inline void makeQVariantListImpl(const std::tuple<>&, QVariantList&)
{
}

template<typename... Tp>
inline QVariantList makeQVariantList(const std::tuple<Tp...>& value)
{
    QVariantList result;
    makeQVariantListImpl(value, result);
    return result;
}


// ---- makeQVariantMap ----

template<std::size_t I = 0, typename... Tp, typename... Tp2,
         typename std::enable_if<I == sizeof...(Tp) - 1, void>::type* = nullptr>
inline void makeQVariantMapImpl(const std::tuple<Tp...>& t, const std::tuple<Tp2...>& names, QVariantMap& result)
{
    auto value = qvariantFromValue_Map(std::get<I>(t));
    auto name = std::get<I>(names);
    result.insert(name, value);
}

template<std::size_t I = 0, typename... Tp, typename... Tp2,
         typename std::enable_if<I < sizeof...(Tp) - 1, void>::type* = nullptr>
inline void makeQVariantMapImpl(const std::tuple<Tp...>& t, const std::tuple<Tp2...>& names, QVariantMap& result)
{
    auto value = qvariantFromValue_Map(std::get<I>(t));
    auto name = std::get<I>(names);
    result.insert(name, value);
    makeQVariantMapImpl<I+1>(t, names, result);
}

inline void makeQVariantMapImpl(const std::tuple<>&, QVariantMap&)
{
}

template<typename... Tp, typename... Tp2>
inline QVariantMap makeQVariantMap(const std::tuple<Tp...>& values, const std::tuple<Tp2...>& names)
{
    static_assert(sizeof...(Tp) == sizeof...(Tp2));
    QVariantMap result;
    makeQVariantMapImpl(values, names, result);
    return result;
}


// ---- qvariantToValue ----

template<typename T,
         typename std::enable_if<CheckMethod::tieMod_v<T>, void>::type* = nullptr>
void qvariantToValue_List(const QVariant& varValue, T& value)
{
    fromQVariantList(varValue, value);
}

template<typename T,
         typename std::enable_if<!CheckMethod::tieMod_v<T>, void>::type* = nullptr>
void qvariantToValue_List(const QVariant& varValue, T& value)
{
    if (!varValue.canConvert<T>())
        throw std::runtime_error("Can't convert from QVariant -> T. canConvert<T> failed");

    value = varValue.value<T>();
}

// ---- structFromQVariantList ----

template<std::size_t I = 0, typename... Tp,
         typename std::enable_if<I == sizeof...(Tp) - 1, void>::type* = nullptr>
inline void structFromQVariantListImpl(const QVariantList& varValue, std::tuple<Tp...>& values)
{
    qvariantToValue_List(varValue.at(I), std::get<I>(values));
}

template<std::size_t I = 0, typename... Tp,
         typename std::enable_if<I < sizeof...(Tp) - 1, void>::type* = nullptr>
inline void structFromQVariantListImpl(const QVariantList& varValue, std::tuple<Tp...>& values)
{
    qvariantToValue_List(varValue.at(I), std::get<I>(values));
    structFromQVariantListImpl<I+1>(varValue, values);
}

inline void structFromQVariantListImpl(const QVariantList&, std::tuple<>&)
{
}

template<typename... Tp>
inline void structFromQVariantList(const QVariant& value, std::tuple<Tp...>& values)
{
    if (value.type() != QVariant::Type::List)
        throw std::runtime_error("Can't convert from QVariant -> T. Not a list");

    auto list = value.toList();

    if (list.size() != sizeof...(Tp))
         throw std::runtime_error("Can't convert from QVariant -> T. Wrong list's size");

    structFromQVariantListImpl(list, values);
}


// ---- structFromQVariantMap ----

template<typename T,
         typename std::enable_if<CheckMethod::tieMod_v<T>, void>::type* = nullptr>
void qvariantToValue_Map(const QVariant& varValue, T& value)
{
    fromQVariantMap(varValue, value);
}

template<typename T,
         typename std::enable_if<!CheckMethod::tieMod_v<T>, void>::type* = nullptr>
void qvariantToValue_Map(const QVariant& varValue, T& value)
{
    if (!varValue.canConvert<T>())
        throw std::runtime_error("Can't convert from QVariant -> T. canConvert<T> failed");

    value = varValue.value<T>();
}

template<std::size_t I = 0, typename... Tp, typename... Tp2,
         typename std::enable_if<I == sizeof...(Tp) - 1, void>::type* = nullptr>
inline void structFromQVariantMapImpl(const QVariantMap& varValue, std::tuple<Tp...>& values, const std::tuple<Tp2...>& names)
{
    if (!varValue.contains(std::get<I>(names)))
        throw std::runtime_error("Can't convert from QVariant -> T. Field name not found in map");

    qvariantToValue_Map(varValue.value(std::get<I>(names)), std::get<I>(values));
}

template<std::size_t I = 0, typename... Tp, typename... Tp2,
         typename std::enable_if<I < sizeof...(Tp) - 1, void>::type* = nullptr>
inline void structFromQVariantMapImpl(const QVariantMap& varValue, std::tuple<Tp...>& values, const std::tuple<Tp2...>& names)
{
    if (!varValue.contains(std::get<I>(names)))
        throw std::runtime_error("Can't convert from QVariant -> T. Field name not found in map");

    qvariantToValue_Map(varValue.value(std::get<I>(names)), std::get<I>(values));
    structFromQVariantMapImpl<I+1>(varValue, values, names);
}

inline void structFromQVariantMapImpl(const QVariantMap&, std::tuple<>&, const std::tuple<>&)
{
}

template<typename... Tp, typename... Tp2>
inline void structFromQVariantMap(const QVariant& value, std::tuple<Tp...>& values, const std::tuple<Tp2...>& names)
{
    static_assert(sizeof...(Tp) == sizeof...(Tp2));

    if (value.type() != QVariant::Type::Map)
        throw std::runtime_error("Can't convert from QVariant -> T. Not a map");

    auto map = value.toMap();

    if (map.size() != sizeof...(Tp))
        throw std::runtime_error("Can't convert from QVariant -> T. Wrong map's size");

    structFromQVariantMapImpl(map, values, names);
}

} // namespace struct_ops_internal


#define STRUCT_QVARIANT_LIST(STRUCT) \
    inline QVariant toQVariantList(const STRUCT& s) \
    { \
        return struct_ops_internal::makeQVariantList(s.tie()); \
    } \
    \
    inline void fromQVariantList(const QVariant& value, STRUCT& result) \
    { \
        struct_ops_internal::structFromQVariantList(value, result.tieMod()); \
    } \
    \
    template<typename T = STRUCT, \
             typename std::enable_if<std::is_default_constructible<T>::value>::type* = nullptr> \
    [[nodiscard]] T fromQVariantList(const QVariant& value) \
    { \
        T item; \
        fromQVariantList(value, item); \
        return item; \
    } \


#define STRUCT_QVARIANT_MAP(STRUCT) \
    inline QVariant toQVariantMap(const STRUCT& s) \
    { \
        return struct_ops_internal::makeQVariantMap(s.tie(), s.names()); \
    } \
    \
    inline void fromQVariantMap(const QVariant& value, STRUCT& result) \
    { \
        struct_ops_internal::structFromQVariantMap(value, result.tieMod(), result.names()); \
    } \
    \
    template<typename T = STRUCT, \
             typename std::enable_if<std::is_default_constructible<T>::value>::type* = nullptr> \
    [[nodiscard]] T fromQVariantMap(const QVariant& value) \
    { \
        T item; \
        fromQVariantMap(value, item); \
        return item; \
    } \
