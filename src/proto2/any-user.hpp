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

struct RPacket_0x7530_Fixed
{
    using NetType = NetRPacket_0x7530_Fixed;
    static const uint16_t PACKET_ID = 0x7530;

    uint16_t magic_packet_id = PACKET_ID;
};
struct SPacket_0x7531_Fixed
{
    using NetType = NetSPacket_0x7531_Fixed;
    static const uint16_t PACKET_ID = 0x7531;

    uint16_t magic_packet_id = PACKET_ID;
    Version version = {};
};
struct RPacket_0x7532_Fixed
{
    using NetType = NetRPacket_0x7532_Fixed;
    static const uint16_t PACKET_ID = 0x7532;

    uint16_t magic_packet_id = PACKET_ID;
};

struct NetRPacket_0x7530_Fixed
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetRPacket_0x7530_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7530_Fixed, magic_packet_id) == 0");
static_assert(sizeof(NetRPacket_0x7530_Fixed) == 2, "sizeof(NetRPacket_0x7530_Fixed) == 2");
struct NetSPacket_0x7531_Fixed
{
    Little16 magic_packet_id;
    NetVersion version;
};
static_assert(offsetof(NetSPacket_0x7531_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7531_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7531_Fixed, version) == 2, "offsetof(NetSPacket_0x7531_Fixed, version) == 2");
static_assert(sizeof(NetSPacket_0x7531_Fixed) == 10, "sizeof(NetSPacket_0x7531_Fixed) == 10");
struct NetRPacket_0x7532_Fixed
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetRPacket_0x7532_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7532_Fixed, magic_packet_id) == 0");
static_assert(sizeof(NetRPacket_0x7532_Fixed) == 2, "sizeof(NetRPacket_0x7532_Fixed) == 2");

inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7530_Fixed *network, RPacket_0x7530_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7530_Fixed *native, NetRPacket_0x7530_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7531_Fixed *network, SPacket_0x7531_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->version, native.version);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7531_Fixed *native, NetSPacket_0x7531_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->version, network.version);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7532_Fixed *network, RPacket_0x7532_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7532_Fixed *native, NetRPacket_0x7532_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

#endif // TMWA_PROTO2_ANY_USER_HPP
