#ifndef TMWA_GENERIC_CONST_ARRAY_HPP
#define TMWA_GENERIC_CONST_ARRAY_HPP
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

# include "../sanity.hpp"

# include <cstring>

# include <iterator>
# include <ostream>
# include <vector>

# ifdef WORKAROUND_GCC46_COMPILER
// constexpr is buggy with templates in this version
// Is this still needed now that const_string is removed?
#  define constexpr /* nothing */
# endif

// TODO see if I ever actually use this, and not the subclass
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

    // Oops. see src/warnings.hpp
    constexpr
    const T *data() const { return d; }
    constexpr
    size_t size() const { return n; }
    constexpr
    bool empty() const { return not n; }
    constexpr explicit
    operator bool() const { return n; }

    constexpr
    std::pair<const_array, const_array> cut(size_t o) const
    {
        return {const_array(d, o), const_array(d + o, n - o)};
    }

    constexpr
    const_array first(size_t o) const
    {
        return cut(o).first;
    }

    constexpr
    const_array last(size_t l) const
    {
        return cut(size() - l).second;
    }

    constexpr
    const_array after(size_t o) const
    {
        return cut(o).second;
    }

    constexpr
    iterator begin() const { return d; }
    constexpr
    iterator end() const { return d + n; }
    constexpr
    reverse_iterator rbegin() const { return reverse_iterator(end()); }
    constexpr
    reverse_iterator rend() const { return reverse_iterator(begin()); }

    constexpr
    const T& front() const { return *begin(); }
    constexpr
    const T& back() const { return *rbegin(); }

    // This probably shouldn't be used, but I'm adding it for porting.
    T& operator[](size_t i)
    {
        return const_cast<T&>(d[i]);
    }
};

# ifdef WORKAROUND_GCC46_COMPILER
#  undef constexpr
# endif

#endif // TMWA_GENERIC_CONST_ARRAY_HPP
