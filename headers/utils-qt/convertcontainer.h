#pragma once

#include <QVector>
#include <QList>
#include <QSet>
#include <type_traits>

namespace UtilsQt {

template<typename T = void,
         typename Container,
         typename TD = typename Container::value_type,
         typename R = std::conditional_t<std::is_same_v<T, void>, TD, T>>
QSet<R> toSet(const Container& container)
{
    return QSet<R>(container.cbegin(), container.cend());
}

template<typename T = void,
         typename Container,
         typename TD = typename Container::value_type,
         typename R = std::conditional_t<std::is_same_v<T, void>, TD, T>>
QVector<R> toVector(const Container& container)
{
    return QVector<R>(container.cbegin(), container.cend());
}

template<typename T = void,
         typename Container,
         typename TD = typename Container::value_type,
         typename R = std::conditional_t<std::is_same_v<T, void>, TD, T>>
QList<R> toList(const Container& container)
{
    return QList<R>(container.cbegin(), container.cend());
}

} // namespace UtilsQt
