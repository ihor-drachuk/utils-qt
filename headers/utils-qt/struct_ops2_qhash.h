#pragma once
#include <QtCore>
#include <QHash>

/*
    Example
  -----------

  struct My_Struct
  {
      int a_;
      double b_;

      auto tie() const { return std::tie(a_, b_); }
  };

  STRUCT_QHASH(My_Struct);
*/

namespace struct_ops_internal {

inline uint combineHash(uint hash, uint seed)
{
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2)) ;
}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp) - 1, uint>::type
hash(const std::tuple<Tp...>& t, uint seed)
{
    return combineHash(qHash(std::get<I>(t)), seed);
}

template<std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp) - 1, uint>::type
hash(const std::tuple<Tp...>& t, uint seed)
{
    return combineHash(qHash(std::get<I>(t)), hash<I+1>(t, seed));
}

inline uint hash(const std::tuple<>&, uint seed)
{
    return combineHash(0, seed);
}

template<typename... Tp>
inline uint hashTuple(const std::tuple<Tp...>& value, uint seed)
{
    return hash(value, seed);
}

} // namespace struct_ops_internal


#define STRUCT_QHASH(STRUCT) \
    inline uint qHash(const STRUCT& s, uint seed = 0) \
    { \
        return struct_ops_internal::hashTuple(s.tie(), seed); \
    }
