#ifndef TMWA_PROTO2_MAP_USER_HPP
#define TMWA_PROTO2_MAP_USER_HPP
//    map-user.hpp - TMWA network protocol: map/user
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

struct SPacket0x0212_Fixed
{
    uint16_t packet_id;
    BlockId npc_id;
    uint16_t command;
    BlockId id;
    uint16_t x;
    uint16_t y;
};
struct NetSPacket0x0212_Fixed
{
    Little16 packet_id;
    Little32 npc_id;
    Little16 command;
    Little32 id;
    Little16 x;
    Little16 y;
};
static_assert(offsetof(NetSPacket0x0212_Fixed, packet_id) == 0, "offsetof(NetSPacket0x0212_Fixed, packet_id) == 0");
static_assert(offsetof(NetSPacket0x0212_Fixed, npc_id) == 2, "offsetof(NetSPacket0x0212_Fixed, npc_id) == 2");
static_assert(offsetof(NetSPacket0x0212_Fixed, command) == 6, "offsetof(NetSPacket0x0212_Fixed, command) == 6");
static_assert(offsetof(NetSPacket0x0212_Fixed, id) == 8, "offsetof(NetSPacket0x0212_Fixed, id) == 8");
static_assert(offsetof(NetSPacket0x0212_Fixed, x) == 12, "offsetof(NetSPacket0x0212_Fixed, x) == 12");
static_assert(offsetof(NetSPacket0x0212_Fixed, y) == 14, "offsetof(NetSPacket0x0212_Fixed, y) == 14");
static_assert(sizeof(NetSPacket0x0212_Fixed) == 16, "sizeof(NetSPacket0x0212_Fixed) == 16");
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket0x0212_Fixed *network, SPacket0x0212_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->packet_id, native.packet_id);
    rv &= native_to_network(&network->npc_id, native.npc_id);
    rv &= native_to_network(&network->command, native.command);
    rv &= native_to_network(&network->id, native.id);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket0x0212_Fixed *native, NetSPacket0x0212_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->packet_id, network.packet_id);
    rv &= network_to_native(&native->npc_id, network.npc_id);
    rv &= network_to_native(&native->command, network.command);
    rv &= network_to_native(&native->id, network.id);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    return rv;
}


#endif // TMWA_PROTO2_MAP_USER_HPP
