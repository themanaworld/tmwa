#pragma once
//    proto2/net-skewed-length.hpp - Deprecated logic for skewed-size packets.
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

#include <cstddef>


namespace tmwa
{
template<class T, size_t N>
struct SkewedLength
{
    T data;
};
template<class T, size_t N, class U>
bool native_to_network(SkewedLength<T, N> *network, U native)
{
    native -= N;
    return native_to_network(&network->data, native);
}
template<class T, size_t N, class U>
bool network_to_native(U *native, SkewedLength<T, N> network)
{
    bool rv = network_to_native(native, network.data);
    *native += N;
    return rv;
}
} // namespace tmwa
