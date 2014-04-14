#ifndef TMWA_GENERIC_ENUM_HPP
#define TMWA_GENERIC_ENUM_HPP
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

# include "../sanity.hpp"

# include <cassert>

# include <algorithm>
# include <type_traits>

# include "../compat/iter.hpp"

template<class T, class E, E max>
struct earray
{
    constexpr static
    size_t size()
    {
        return static_cast<size_t>(max);
    }

    // no ctor/dtor and one public member variable for easy initialization
    T _data[size()];

    T& operator[](E v)
    {
        auto i = static_cast<size_t>(v);
        assert (i < size());
        return _data[i];
    }

    const T& operator[](E v) const
    {
        auto i = static_cast<size_t>(v);
        assert (i < size());
        return _data[i];
    }

    T *begin()
    {
        return _data;
    }

    T *end()
    {
        return _data + size();
    }

    const T *begin() const
    {
        return _data;
    }

    const T *end() const
    {
        return _data + size();
    }

    friend bool operator == (const earray& l, const earray& r)
    {
        return std::equal(l.begin(), l.end(), r.begin());
    }

    friend bool operator != (const earray& l, const earray& r)
    {
        return !(l == r);
    }
};

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
    : _data(arr._data)
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

// std::underlying_type isn't supported until gcc 4.7
// this is a poor man's emulation
template<class E>
struct underlying_type
{
    static_assert(std::is_enum<E>::value, "Only enums have underlying type!");
    typedef typename std::conditional<
        std::is_signed<E>::value,
        typename std::make_signed<E>::type,
        typename std::make_unsigned<E>::type
    >::type type;
};

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
# define ENUM_BITWISE_OPERATORS(E)      \
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
        return E(U(v) + 1);
    }
};

template<class E>
IteratorPair<ValueIterator<E, EnumMath<E>>> erange(E b, E e)
{
    return {b, e};
}

#endif // TMWA_GENERIC_ENUM_HPP
