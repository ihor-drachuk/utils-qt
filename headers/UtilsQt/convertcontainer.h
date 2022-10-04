#pragma once

#include <QVector>
#include <QList>
#include <QSet>
#include <type_traits>

namespace UtilsQt {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) // Qt 5.14 (^) and upper

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

#else // Before Qt 5.14 (v)

template<typename T = void,
         typename Container,
         typename TD = typename Container::value_type,
         typename R = std::conditional_t<std::is_same_v<T, void>, TD, T>>
QSet<R> toSet(const Container& container)
{
    QSet<R> result;
    for (const auto& x : container)
        result.insert(x);
    return result;
}

template<typename T = void,
         typename Container,
         typename TD = typename Container::value_type,
         typename R = std::conditional_t<std::is_same_v<T, void>, TD, T>>
QVector<R> toVector(const Container& container)
{
    QVector<R> result;
    for (const auto& x : container)
        result.append(x);
    return result;
}

template<typename T = void,
         typename Container,
         typename TD = typename Container::value_type,
         typename R = std::conditional_t<std::is_same_v<T, void>, TD, T>>
QList<R> toList(const Container& container)
{
    QList<R> result;
    for (const auto& x : container)
        result.append(x);
    return result;
}

#endif // End QT_VERSION

} // namespace UtilsQt
