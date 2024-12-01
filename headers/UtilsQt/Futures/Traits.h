/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <type_traits>


template<typename T>
class QFuture;

namespace UtilsQt {

template<typename>
struct IsQFuture : std::false_type {};

template<typename T>
struct IsQFuture<QFuture<T>> : std::true_type {};

} // namespace UtilsQt
