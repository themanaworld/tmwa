#pragma once
//    matrix.hpp - A 2D array.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../compat/memory.hpp"


namespace tmwa
{
template<class T>
class Matrix
{
    std::unique_ptr<T[]> _data;
    size_t _xs, _ys;
public:
    Matrix()
    : _data()
    , _xs()
    , _ys()
    {}
    Matrix(size_t x, size_t y)
    : _data(make_unique<T[]>(x * y))
    , _xs(x)
    , _ys(y)
    {}
    // no copy-ctor or copy-assign

    void reset(size_t x, size_t y)
    {
        *this = Matrix(x, y);
    }
    void clear()
    {
        *this = Matrix();
    }

    T& ref(size_t x, size_t y)
    {
        assert (x < _xs);
        assert (y < _ys);
        return _data[x + y * _xs];
    }
    const T& ref(size_t x, size_t y) const
    {
        assert (x < _xs);
        assert (y < _ys);
        return _data[x + y * _xs];
    }

    size_t xs() const
    {
        return _xs;
    }
    size_t ys() const
    {
        return _ys;
    }
};
} // namespace tmwa
