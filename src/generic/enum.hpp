#pragma once
//    enum.hpp - Safe building blocks for enumerated types.
//
//    Copyright © 2012-2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "fwd.hpp"

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <type_traits>

#include "../compat/iter.hpp"

#include "array.hpp"


namespace tmwa
{
// part moved to fwd.hpp

template<class T, class E, E max>
class eptr
{
    constexpr static
    size_t size()
    {
        return static_cast<size_t>(max);
    }

    T *_data;
public:
    eptr(std::nullptr_t=nullptr)
    : _data(nullptr)
    {}

    eptr(earray<T, E, max>& arr)
    : _data(arr.data)
    {}

    T& operator [](E v) const
    {
        auto i = static_cast<size_t>(v);
        assert (i < size());
        return _data[i];
    }

    explicit operator bool()
    {
        return _data;
    }

    bool operator not()
    {
        return not _data;
    }
};

using std::underlying_type;

template<class E, bool=std::is_enum<E>::value>
struct remove_enum
{
    typedef E type;
};
template<class E>
struct remove_enum<E, true>
{
    typedef typename underlying_type<E>::type type;
};


// This really should just go in a namespace
// that's how I use it anyway ...
#define ENUM_BITWISE_OPERATORS(E)       \
inline                                  \
E operator & (E l, E r)                 \
{                                       \
    typedef underlying_type<E>::type U; \
    return E(U(l) & U(r));              \
}                                       \
inline                                  \
E operator | (E l, E r)                 \
{                                       \
    typedef underlying_type<E>::type U; \
    return E(U(l) | U(r));              \
}                                       \
inline                                  \
E operator ^ (E l, E r)                 \
{                                       \
    typedef underlying_type<E>::type U; \
    return E(U(l) ^ U(r));              \
}                                       \
inline                                  \
E& operator &= (E& l, E r)              \
{                                       \
    return l = l & r;                   \
}                                       \
inline                                  \
E& operator |= (E& l, E r)              \
{                                       \
    return l = l | r;                   \
}                                       \
inline                                  \
E& operator ^= (E& l, E r)              \
{                                       \
    return l = l ^ r;                   \
}                                       \
inline                                  \
E operator ~ (E r)                      \
{                                       \
    return E(-1) ^ r;                   \
}

template<class E>
class EnumMath
{
    typedef typename underlying_type<E>::type U;
public:
    static
    E inced(E v)
    {
        return static_cast<E>(static_cast<U>(v) + 1);
    }
};

template<class E>
IteratorPair<ValueIterator<E, EnumMath<E>>> erange(E b, E e)
{
    return {b, e};
}
} // namespace tmwa
