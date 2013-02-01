#ifndef CONST_ARRAY_HPP
#define CONST_ARRAY_HPP
//    const_array.hpp - just a pointer-to-const and a length
//
//    Copyright © 2011-2012 Ben Longbons <b.r.longbons@gmail.com>
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

#include "sanity.hpp"

#include <cstring>

#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#ifdef WORKAROUND_GCC46_COMPILER
// constexpr is buggy with templates in this version
# define constexpr /* nothing */
#endif

template<class T>
class const_array
{
    const T *d;
    size_t n;
public:
    typedef const T *iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    constexpr
    const_array(std::nullptr_t)
    : d(nullptr), n(0)
    {}

    constexpr
    const_array(const T *p, size_t z)
    : d(p), n(z)
    {}

    constexpr
    const_array(const T *b, const T *e)
    : d(b), n(e - b)
    {}

    const_array(std::initializer_list<T> list)
    : d(list.begin()), n(list.size())
    {}

    // Implicit conversion from std::vector
    const_array(const std::vector<T>& v)
    : d(v.data()), n(v.size())
    {}

    // but disallow conversion from a temporary
    const_array(std::vector<T>&&) = delete;

    // All methods are non-const to "encourage" you
    // to always pass a const_array by value.
    // After all, "const const_array" looks funny.
    constexpr
    const T *data() { return d; }
    constexpr
    size_t size() { return n; }
    constexpr
    bool empty() { return not n; }
    constexpr explicit
    operator bool() { return n; }

    constexpr
    std::pair<const_array, const_array> cut(size_t o)
    {
        return {const_array(d, o), const_array(d + o, n - o)};
    }

    constexpr
    const_array first(size_t o)
    {
        return cut(o).first;
    }

    constexpr
    const_array last(size_t l)
    {
        return cut(size() - l).second;
    }

    constexpr
    const_array after(size_t o)
    {
        return cut(o).second;
    }

    constexpr
    iterator begin() { return d; }
    constexpr
    iterator end() { return d + n; }
    constexpr
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    constexpr
    reverse_iterator rend() { return reverse_iterator(begin()); }

    constexpr
    const T& front() { return *begin(); }
    constexpr
    const T& back() { return *rbegin(); }
};

// subclass just provides a simpler name and some conversions
class const_string : public const_array<char>
{
public:
    // Implicit conversion from C string.
    constexpr
    const_string(const char *z)
    : const_array<char>(z, z ? strlen(z) : 0)
    {}

    // Same as parent constructor.
    constexpr
    const_string(const char *s, size_t l)
    : const_array<char>(s, l)
    {}

    // Same as parent constructor.
    constexpr
    const_string(const char *b, const char *e)
    : const_array<char>(b, e)
    {}

    // Same as parent constructor.
    const_string(const std::vector<char> s)
    : const_array<char>(s)
    {}

    // Implicit conversion from C++ string.
    const_string(const std::string& s)
    : const_array<char>(s.data(), s.size())
    {}

    // but disallow converion from a temporary.
    const_string(std::string&&) = delete;

    // allow being sloppy
    constexpr
    const_string(const_array<char> a)
    : const_array<char>(a)
    {}
};
#ifdef WORKAROUND_GCC46_COMPILER
# undef constexpr
#endif

inline
std::ostream& operator << (std::ostream& o, const_string s)
{
    return o.write(s.data(), s.size());
}

#endif // CONST_ARRAY_HPP
