#pragma once
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

#include "fwd.hpp"

#include <cassert>
#include <cstddef>

#include "oops.hpp"


// half the important stuff is now in fwd.hpp !!!
namespace tmwa
{
template<class I, I be, I en>
struct ExclusiveIndexing
{
    using index_type = I;
    constexpr static size_t index_to_offset(index_type idx)
    { return static_cast<size_t>(idx) - static_cast<size_t>(be); }
    constexpr static index_type offset_to_index(size_t off)
    { return static_cast<I>(off + static_cast<size_t>(be)); }
    constexpr static size_t alloc_size = index_to_offset(en) - index_to_offset(be);
};

template<class I, I lo, I hi>
struct InclusiveIndexing
{
    using index_type = I;
    constexpr static size_t index_to_offset(index_type idx)
    { return static_cast<size_t>(idx) - static_cast<size_t>(lo); }
    constexpr static index_type offset_to_index(size_t off)
    { return static_cast<I>(off + static_cast<size_t>(lo)); }
    constexpr static size_t alloc_size = index_to_offset(hi) - index_to_offset(lo) + 1;
};

template<class E, E n>
struct EnumIndexing : ExclusiveIndexing<E, static_cast<E>(0), n>
{
};

template<class I, size_t limit>
struct InventoryIndexing
{
    using index_type = I;
    constexpr static size_t index_to_offset(index_type idx)
    { return idx.get0(); }
    constexpr static index_type offset_to_index(size_t off)
    { return I::from(off); }
    constexpr static size_t alloc_size = limit;
};

template<class T, class I>
struct GenericArray
{
    T data[I::alloc_size];
public:
    T *begin()
    { return data + 0; }
    T *end()
    { return data + I::alloc_size; }
    const T *begin() const
    { return data + 0; }
    const T *end() const
    { return data + I::alloc_size; }
    size_t size() const
    { return I::alloc_size; }

    T& operator [](typename I::index_type i_)
    {
        size_t i = I::index_to_offset(i_);
        ALLEGE ("index in bounds", i < size());
        return data[i];
    }
    const T& operator [](typename I::index_type i_) const
    {
        size_t i = I::index_to_offset(i_);
        ALLEGE ("index in bounds", i < size());
        return data[i];
    }

    friend bool operator == (GenericArray& lhs, GenericArray& rhs)
    {
        for (size_t i = 0; i < I::alloc_size; ++i)
        {
            if (lhs.data[i] != rhs.data[i])
                return false;
        }
        return true;
    }
    friend bool operator != (GenericArray& lhs, GenericArray& rhs)
    {
        return !(lhs == rhs);
    }
};
} // namespace tmwa
