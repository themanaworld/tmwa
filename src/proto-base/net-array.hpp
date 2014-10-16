#pragma once
//    proto2/net-array.hpp - Special logic for fixed-size arrays in packets.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "fwd.hpp"

#include "../generic/array.hpp"


namespace tmwa
{
template<class T, size_t N>
struct NetArray
{
    T data[N];
};
template<class T, class U, class I>
bool native_to_network(NetArray<T, I::alloc_size> *network, GenericArray<U, I> native)
{
    for (size_t i = 0; i < I::alloc_size; ++i)
    {
        if (!native_to_network(&(*network).data[i], native[I::offset_to_index(i)]))
            return false;
    }
    return true;
}
template<class T, class U, class I>
bool network_to_native(GenericArray<U, I> *native, NetArray<T, I::alloc_size> network)
{
    for (size_t i = 0; i < I::alloc_size; ++i)
    {
        if (!network_to_native(&(*native)[I::offset_to_index(i)], network.data[i]))
            return false;
    }
    return true;
}
} // namespace tmwa
