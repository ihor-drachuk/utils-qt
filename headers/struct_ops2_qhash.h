#pragma once
#include "tuple_for_each.h"
#include <QtCore>
#include <QHash>


namespace struct_ops_internal {

inline uint combineHash(uint hash, uint seed)
{
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2)) ;
}

template<typename StructTuple>
inline uint hashTuple(const StructTuple& tuple)
{
    auto result = for_each_par<uint>(tuple, [](int, const auto& v, const std::optional<uint>& param) -> std::optional<uint> {
        uint hash1 = *param;
        uint hash2 = qHash(v);
        return combineHash(hash1, hash2);
    }, 0, false);

    return *result;
}

} // namespace struct_ops_internal


#define TIED_QHASH(STRUCT) \
    inline uint qHash(const STRUCT& s) \
    { \
        return struct_ops_internal::hashTuple(s.tie()); \
    }

//    Example
//  -----------
//
//  struct My_Struct
//  {
//      int a_;
//      double b_;
//      auto tie() const { return std::tie(a_, b_); }
//  };
//
//  TIED_QHASH(My_Struct)
