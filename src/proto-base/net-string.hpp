#pragma once
//    proto2/net-string.hpp - Special logic for fixed-size strings in packets.
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

#include "../strings/vstring.hpp"

#include "../mmo/strs.hpp"


namespace tmwa
{
template<size_t N>
struct NetString
{
    char data[N];
};
template<size_t N>
bool native_to_network(NetString<N> *network, VString<N-1> native)
{
    // basically WBUF_STRING
    char *const begin = network->data;
    char *const end = begin + N;
    char *const mid = std::copy(native.begin(), native.end(), begin);
    std::fill(mid, end, '\0');
    return true;
}
template<size_t N>
bool network_to_native(VString<N-1> *native, NetString<N> network)
{
    // basically RBUF_STRING
    const char *const begin = network.data;
    const char *const end = begin + N;
    const char *const mid = std::find(begin, end, '\0');
    *native = XString(begin, mid, nullptr);
    return true;
}

inline
bool native_to_network(NetString<24> *network, CharName native)
{
    VString<23> tmp = native.to__actual();
    bool rv = native_to_network(network, tmp);
    return rv;
}
inline
bool network_to_native(CharName *native, NetString<24> network)
{
    VString<23> tmp;
    bool rv = network_to_native(&tmp, network);
    *native = stringish<CharName>(tmp);
    return rv;
}

inline
bool native_to_network(NetString<16> *network, MapName native)
{
    XString tmp = native;
    bool rv = native_to_network(network, VString<15>(tmp));
    return rv;
}
inline
bool network_to_native(MapName *native, NetString<16> network)
{
    VString<15> tmp;
    bool rv = network_to_native(&tmp, network);
    *native = stringish<MapName>(tmp);
    return rv;
}
} // namespace tmwa
