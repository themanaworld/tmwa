#ifndef TMWA_PROTO2_ANY_USER_HPP
#define TMWA_PROTO2_ANY_USER_HPP
//    any-user.hpp - TMWA network protocol: any/user
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

// This is a generated file, edit tools/protocol.py instead

# include "fwd.hpp"

# include "types.hpp"

// This is a public protocol, and changes require client cooperation

struct RPacket0x7530_Fixed
{
    uint16_t packet_id;
};
struct NetRPacket0x7530_Fixed
{
    Little16 packet_id;
};
static_assert(offsetof(NetRPacket0x7530_Fixed, packet_id) == 0, "offsetof(NetRPacket0x7530_Fixed, packet_id) == 0");
static_assert(sizeof(NetRPacket0x7530_Fixed) == 2, "sizeof(NetRPacket0x7530_Fixed) == 2");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x7530_Fixed *network, RPacket0x7530_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x7530_Fixed *native, NetRPacket0x7530_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    return rv;
}

struct SPacket0x7531_Fixed
{
    uint16_t packet_id;
    Version version;
};
struct NetSPacket0x7531_Fixed
{
    Little16 packet_id;
    NetVersion version;
};
static_assert(offsetof(NetSPacket0x7531_Fixed, packet_id) == 0, "offsetof(NetSPacket0x7531_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x7531_Fixed, version) == 2, "offsetof(NetSPacket0x7531_Fixed, version) == 2");
static_assert(sizeof(NetSPacket0x7531_Fixed) == 10, "sizeof(NetSPacket0x7531_Fixed) == 10");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x7531_Fixed *network, SPacket0x7531_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->version, native.version);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x7531_Fixed *native, NetSPacket0x7531_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->version, network.version);
    return rv;
}

struct RPacket0x7532_Fixed
{
    uint16_t packet_id;
};
struct NetRPacket0x7532_Fixed
{
    Little16 packet_id;
};
static_assert(offsetof(NetRPacket0x7532_Fixed, packet_id) == 0, "offsetof(NetRPacket0x7532_Fixed, packet_id) == 0");
static_assert(sizeof(NetRPacket0x7532_Fixed) == 2, "sizeof(NetRPacket0x7532_Fixed) == 2");
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket0x7532_Fixed *network, RPacket0x7532_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket0x7532_Fixed *native, NetRPacket0x7532_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    return rv;
}


#endif // TMWA_PROTO2_ANY_USER_HPP
