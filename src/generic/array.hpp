#ifndef TMWA_GENERIC_ARRAY_HPP
#define TMWA_GENERIC_ARRAY_HPP
//    array.hpp - A simple bounds-checked array.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include "fwd.hpp"

# include <cassert>
# include <cstddef>

template<class T, size_t n>
struct Array
{
    T data[n];
public:
    T& operator [](size_t i) { assert (i < n); return data[i]; }
    const T& operator [](size_t i) const { assert (i < n); return data[i]; }

    T *begin() { return data + 0; }
    T *end() { return data + n; }
    const T *begin() const { return data + 0; }
    const T *end() const { return data + n; }
};

#endif // TMWA_GENERIC_ARRAY_HPP
