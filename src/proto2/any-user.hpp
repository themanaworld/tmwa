#ifndef TMWA_PROTO2_ANY_USER_HPP
#define TMWA_PROTO2_ANY_USER_HPP
//    any-user.hpp - TMWA network protocol: any/user
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

// This is a generated file, edit tools/protocol.py instead

# include "fwd.hpp"

# include "types.hpp"

// This is a public protocol, and changes require client cooperation

template<>
struct Packet_Fixed<0x7530>
{
    static const uint16_t PACKET_ID = 0x7530;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};
template<>
struct Packet_Fixed<0x7531>
{
    static const uint16_t PACKET_ID = 0x7531;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    Version version = {};
};
template<>
struct Packet_Fixed<0x7532>
{
    static const uint16_t PACKET_ID = 0x7532;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct NetPacket_Fixed<0x7530>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7530>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7530>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7530>) == 2, "sizeof(NetPacket_Fixed<0x7530>) == 2");
template<>
struct NetPacket_Fixed<0x7531>
{
    Little16 magic_packet_id;
    NetVersion version;
};
static_assert(offsetof(NetPacket_Fixed<0x7531>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7531>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7531>, version) == 2, "offsetof(NetPacket_Fixed<0x7531>, version) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7531>) == 10, "sizeof(NetPacket_Fixed<0x7531>) == 10");
template<>
struct NetPacket_Fixed<0x7532>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7532>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7532>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7532>) == 2, "sizeof(NetPacket_Fixed<0x7532>) == 2");

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7530> *network, Packet_Fixed<0x7530> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7530> *native, NetPacket_Fixed<0x7530> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7531> *network, Packet_Fixed<0x7531> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->version, native.version);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7531> *native, NetPacket_Fixed<0x7531> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->version, network.version);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7532> *network, Packet_Fixed<0x7532> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7532> *native, NetPacket_Fixed<0x7532> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

#endif // TMWA_PROTO2_ANY_USER_HPP
