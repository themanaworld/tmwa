#pragma once
//    slice.hpp - a borrowed array
//
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <cstddef>

#include <type_traits>
#include <vector>


namespace tmwa
{
template<class T>
class Slice
{
    T *_begin;
    T *_end;
public:
    class iterator;

    Slice(std::nullptr_t);
    Slice(T *b, T *e);
    Slice(T *b, size_t l);
    template<class U, typename=typename std::enable_if<sizeof(T) == sizeof(U) && std::is_base_of<T, U>::value>::type>
    Slice(Slice<U> o);
    template<size_t n, typename=typename std::enable_if<sizeof(T) != 1>::type>
    Slice(T (&arr)[n]);
    // TODO: come up with something else once using ranges (wrap all containers?)
    Slice(std::vector<T>& vec);

    iterator begin() const;
    iterator end() const;
    T *data() const;
    size_t size() const;
    operator bool() const;
    bool operator not() const;
    T& front() const;
    T& back() const;
    T& pop_front();
    T& pop_back();
    __attribute__((deprecated("use iterators instead")))
    T& operator[](size_t o);

    Slice slice_t(size_t o) const;
    Slice slice_h(size_t o) const;
    Slice rslice_t(size_t no) const;
    Slice rslice_h(size_t no) const;
    Slice islice_t(iterator it) const;
    Slice islice_h(iterator it) const;
    Slice lslice(size_t o, size_t l) const;
    Slice pslice(size_t b, size_t e) const;
    Slice islice(iterator b, iterator e) const;
};
} // namespace tmwa

#include "slice.tcc"
