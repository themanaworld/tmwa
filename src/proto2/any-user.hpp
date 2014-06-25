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
struct Packet_Fixed<0x0081>
{
    static const uint16_t PACKET_ID = 0x0081;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t error_code = {};
};

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
struct Packet_Payload<0x8000>
{
    static const uint16_t PACKET_ID = 0x8000;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};


template<>
struct NetPacket_Fixed<0x0081>
{
    Little16 magic_packet_id;
    Byte error_code;
};
static_assert(offsetof(NetPacket_Fixed<0x0081>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0081>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0081>, error_code) == 2, "offsetof(NetPacket_Fixed<0x0081>, error_code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0081>) == 3, "sizeof(NetPacket_Fixed<0x0081>) == 3");
static_assert(alignof(NetPacket_Fixed<0x0081>) == 1, "alignof(NetPacket_Fixed<0x0081>) == 1");

template<>
struct NetPacket_Fixed<0x7530>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7530>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7530>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7530>) == 2, "sizeof(NetPacket_Fixed<0x7530>) == 2");
static_assert(alignof(NetPacket_Fixed<0x7530>) == 1, "alignof(NetPacket_Fixed<0x7530>) == 1");

template<>
struct NetPacket_Fixed<0x7531>
{
    Little16 magic_packet_id;
    NetVersion version;
};
static_assert(offsetof(NetPacket_Fixed<0x7531>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7531>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7531>, version) == 2, "offsetof(NetPacket_Fixed<0x7531>, version) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7531>) == 10, "sizeof(NetPacket_Fixed<0x7531>) == 10");
static_assert(alignof(NetPacket_Fixed<0x7531>) == 1, "alignof(NetPacket_Fixed<0x7531>) == 1");

template<>
struct NetPacket_Fixed<0x7532>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7532>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7532>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7532>) == 2, "sizeof(NetPacket_Fixed<0x7532>) == 2");
static_assert(alignof(NetPacket_Fixed<0x7532>) == 1, "alignof(NetPacket_Fixed<0x7532>) == 1");

template<>
struct NetPacket_Payload<0x8000>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Payload<0x8000>, magic_packet_id) == 0, "offsetof(NetPacket_Payload<0x8000>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Payload<0x8000>, magic_packet_length) == 2, "offsetof(NetPacket_Payload<0x8000>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Payload<0x8000>) == 4, "sizeof(NetPacket_Payload<0x8000>) == 4");
static_assert(alignof(NetPacket_Payload<0x8000>) == 1, "alignof(NetPacket_Payload<0x8000>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0081> *network, Packet_Fixed<0x0081> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->error_code, native.error_code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0081> *native, NetPacket_Fixed<0x0081> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->error_code, network.error_code);
    return rv;
}

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

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Payload<0x8000> *network, Packet_Payload<0x8000> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Payload<0x8000> *native, NetPacket_Payload<0x8000> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}


#endif // TMWA_PROTO2_ANY_USER_HPP
