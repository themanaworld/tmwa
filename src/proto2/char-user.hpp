#ifndef TMWA_PROTO2_CHAR_USER_HPP
#define TMWA_PROTO2_CHAR_USER_HPP
//    char-user.hpp - TMWA network protocol: char/user
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

// this is only needed for the payload packet right now, and that needs to die
# pragma pack(push, 1)

template<>
struct Packet_Fixed<0x0061>
{
    static const uint16_t PACKET_ID = 0x0061;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountPass old_pass = {};
    AccountPass new_pass = {};
};

template<>
struct Packet_Fixed<0x0062>
{
    static const uint16_t PACKET_ID = 0x0062;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t status = {};
};

template<>
struct Packet_Fixed<0x0065>
{
    static const uint16_t PACKET_ID = 0x0065;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t login_id1 = {};
    uint32_t login_id2 = {};
    uint16_t packet_tmw_version = {};
    SEX sex = {};
};

template<>
struct Packet_Fixed<0x0066>
{
    static const uint16_t PACKET_ID = 0x0066;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t code = {};
};

template<>
struct Packet_Fixed<0x0067>
{
    static const uint16_t PACKET_ID = 0x0067;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharName char_name = {};
    Stats6 stats = {};
    uint8_t slot = {};
    uint16_t hair_color = {};
    uint16_t hair_style = {};
};

template<>
struct Packet_Fixed<0x0068>
{
    static const uint16_t PACKET_ID = 0x0068;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharId char_id = {};
    AccountEmail email = {};
};

template<>
struct Packet_Head<0x006b>
{
    static const uint16_t PACKET_ID = 0x006b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    VString<19> unused = {};
};
template<>
struct Packet_Repeat<0x006b>
{
    static const uint16_t PACKET_ID = 0x006b;

    CharSelect char_select = {};
};

template<>
struct Packet_Fixed<0x006c>
{
    static const uint16_t PACKET_ID = 0x006c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t code = {};
};

template<>
struct Packet_Fixed<0x006d>
{
    static const uint16_t PACKET_ID = 0x006d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharSelect char_select = {};
};

template<>
struct Packet_Fixed<0x006e>
{
    static const uint16_t PACKET_ID = 0x006e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t code = {};
};

template<>
struct Packet_Fixed<0x006f>
{
    static const uint16_t PACKET_ID = 0x006f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x0070>
{
    static const uint16_t PACKET_ID = 0x0070;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t code = {};
};

template<>
struct Packet_Fixed<0x0071>
{
    static const uint16_t PACKET_ID = 0x0071;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharId char_id = {};
    MapName map_name = {};
    IP4Address ip = {};
    uint16_t port = {};
};


template<>
struct NetPacket_Fixed<0x0061>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountPass)> old_pass;
    NetString<sizeof(AccountPass)> new_pass;
};
static_assert(offsetof(NetPacket_Fixed<0x0061>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0061>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0061>, old_pass) == 2, "offsetof(NetPacket_Fixed<0x0061>, old_pass) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0061>, new_pass) == 26, "offsetof(NetPacket_Fixed<0x0061>, new_pass) == 26");
static_assert(sizeof(NetPacket_Fixed<0x0061>) == 50, "sizeof(NetPacket_Fixed<0x0061>) == 50");
static_assert(alignof(NetPacket_Fixed<0x0061>) == 1, "alignof(NetPacket_Fixed<0x0061>) == 1");

template<>
struct NetPacket_Fixed<0x0062>
{
    Little16 magic_packet_id;
    Byte status;
};
static_assert(offsetof(NetPacket_Fixed<0x0062>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0062>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0062>, status) == 2, "offsetof(NetPacket_Fixed<0x0062>, status) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0062>) == 3, "sizeof(NetPacket_Fixed<0x0062>) == 3");
static_assert(alignof(NetPacket_Fixed<0x0062>) == 1, "alignof(NetPacket_Fixed<0x0062>) == 1");

template<>
struct NetPacket_Fixed<0x0065>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 login_id1;
    Little32 login_id2;
    Little16 packet_tmw_version;
    Byte sex;
};
static_assert(offsetof(NetPacket_Fixed<0x0065>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0065>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0065>, account_id) == 2, "offsetof(NetPacket_Fixed<0x0065>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0065>, login_id1) == 6, "offsetof(NetPacket_Fixed<0x0065>, login_id1) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0065>, login_id2) == 10, "offsetof(NetPacket_Fixed<0x0065>, login_id2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x0065>, packet_tmw_version) == 14, "offsetof(NetPacket_Fixed<0x0065>, packet_tmw_version) == 14");
static_assert(offsetof(NetPacket_Fixed<0x0065>, sex) == 16, "offsetof(NetPacket_Fixed<0x0065>, sex) == 16");
static_assert(sizeof(NetPacket_Fixed<0x0065>) == 17, "sizeof(NetPacket_Fixed<0x0065>) == 17");
static_assert(alignof(NetPacket_Fixed<0x0065>) == 1, "alignof(NetPacket_Fixed<0x0065>) == 1");

template<>
struct NetPacket_Fixed<0x0066>
{
    Little16 magic_packet_id;
    Byte code;
};
static_assert(offsetof(NetPacket_Fixed<0x0066>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0066>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0066>, code) == 2, "offsetof(NetPacket_Fixed<0x0066>, code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0066>) == 3, "sizeof(NetPacket_Fixed<0x0066>) == 3");
static_assert(alignof(NetPacket_Fixed<0x0066>) == 1, "alignof(NetPacket_Fixed<0x0066>) == 1");

template<>
struct NetPacket_Fixed<0x0067>
{
    Little16 magic_packet_id;
    NetString<sizeof(CharName)> char_name;
    NetStats6 stats;
    Byte slot;
    Little16 hair_color;
    Little16 hair_style;
};
static_assert(offsetof(NetPacket_Fixed<0x0067>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0067>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0067>, char_name) == 2, "offsetof(NetPacket_Fixed<0x0067>, char_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0067>, stats) == 26, "offsetof(NetPacket_Fixed<0x0067>, stats) == 26");
static_assert(offsetof(NetPacket_Fixed<0x0067>, slot) == 32, "offsetof(NetPacket_Fixed<0x0067>, slot) == 32");
static_assert(offsetof(NetPacket_Fixed<0x0067>, hair_color) == 33, "offsetof(NetPacket_Fixed<0x0067>, hair_color) == 33");
static_assert(offsetof(NetPacket_Fixed<0x0067>, hair_style) == 35, "offsetof(NetPacket_Fixed<0x0067>, hair_style) == 35");
static_assert(sizeof(NetPacket_Fixed<0x0067>) == 37, "sizeof(NetPacket_Fixed<0x0067>) == 37");
static_assert(alignof(NetPacket_Fixed<0x0067>) == 1, "alignof(NetPacket_Fixed<0x0067>) == 1");

template<>
struct NetPacket_Fixed<0x0068>
{
    Little16 magic_packet_id;
    Little32 char_id;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetPacket_Fixed<0x0068>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0068>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0068>, char_id) == 2, "offsetof(NetPacket_Fixed<0x0068>, char_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0068>, email) == 6, "offsetof(NetPacket_Fixed<0x0068>, email) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0068>) == 46, "sizeof(NetPacket_Fixed<0x0068>) == 46");
static_assert(alignof(NetPacket_Fixed<0x0068>) == 1, "alignof(NetPacket_Fixed<0x0068>) == 1");

template<>
struct NetPacket_Head<0x006b>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(VString<19>)> unused;
};
static_assert(offsetof(NetPacket_Head<0x006b>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x006b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x006b>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x006b>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x006b>, unused) == 4, "offsetof(NetPacket_Head<0x006b>, unused) == 4");
static_assert(sizeof(NetPacket_Head<0x006b>) == 24, "sizeof(NetPacket_Head<0x006b>) == 24");
static_assert(alignof(NetPacket_Head<0x006b>) == 1, "alignof(NetPacket_Head<0x006b>) == 1");
template<>
struct NetPacket_Repeat<0x006b>
{
    NetCharSelect char_select;
};
static_assert(offsetof(NetPacket_Repeat<0x006b>, char_select) == 0, "offsetof(NetPacket_Repeat<0x006b>, char_select) == 0");
static_assert(sizeof(NetPacket_Repeat<0x006b>) == 106, "sizeof(NetPacket_Repeat<0x006b>) == 106");
static_assert(alignof(NetPacket_Repeat<0x006b>) == 1, "alignof(NetPacket_Repeat<0x006b>) == 1");

template<>
struct NetPacket_Fixed<0x006c>
{
    Little16 magic_packet_id;
    Byte code;
};
static_assert(offsetof(NetPacket_Fixed<0x006c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x006c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x006c>, code) == 2, "offsetof(NetPacket_Fixed<0x006c>, code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x006c>) == 3, "sizeof(NetPacket_Fixed<0x006c>) == 3");
static_assert(alignof(NetPacket_Fixed<0x006c>) == 1, "alignof(NetPacket_Fixed<0x006c>) == 1");

template<>
struct NetPacket_Fixed<0x006d>
{
    Little16 magic_packet_id;
    NetCharSelect char_select;
};
static_assert(offsetof(NetPacket_Fixed<0x006d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x006d>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x006d>, char_select) == 2, "offsetof(NetPacket_Fixed<0x006d>, char_select) == 2");
static_assert(sizeof(NetPacket_Fixed<0x006d>) == 108, "sizeof(NetPacket_Fixed<0x006d>) == 108");
static_assert(alignof(NetPacket_Fixed<0x006d>) == 1, "alignof(NetPacket_Fixed<0x006d>) == 1");

template<>
struct NetPacket_Fixed<0x006e>
{
    Little16 magic_packet_id;
    Byte code;
};
static_assert(offsetof(NetPacket_Fixed<0x006e>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x006e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x006e>, code) == 2, "offsetof(NetPacket_Fixed<0x006e>, code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x006e>) == 3, "sizeof(NetPacket_Fixed<0x006e>) == 3");
static_assert(alignof(NetPacket_Fixed<0x006e>) == 1, "alignof(NetPacket_Fixed<0x006e>) == 1");

template<>
struct NetPacket_Fixed<0x006f>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x006f>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x006f>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x006f>) == 2, "sizeof(NetPacket_Fixed<0x006f>) == 2");
static_assert(alignof(NetPacket_Fixed<0x006f>) == 1, "alignof(NetPacket_Fixed<0x006f>) == 1");

template<>
struct NetPacket_Fixed<0x0070>
{
    Little16 magic_packet_id;
    Byte code;
};
static_assert(offsetof(NetPacket_Fixed<0x0070>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0070>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0070>, code) == 2, "offsetof(NetPacket_Fixed<0x0070>, code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0070>) == 3, "sizeof(NetPacket_Fixed<0x0070>) == 3");
static_assert(alignof(NetPacket_Fixed<0x0070>) == 1, "alignof(NetPacket_Fixed<0x0070>) == 1");

template<>
struct NetPacket_Fixed<0x0071>
{
    Little16 magic_packet_id;
    Little32 char_id;
    NetString<sizeof(MapName)> map_name;
    IP4Address ip;
    Little16 port;
};
static_assert(offsetof(NetPacket_Fixed<0x0071>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0071>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0071>, char_id) == 2, "offsetof(NetPacket_Fixed<0x0071>, char_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0071>, map_name) == 6, "offsetof(NetPacket_Fixed<0x0071>, map_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0071>, ip) == 22, "offsetof(NetPacket_Fixed<0x0071>, ip) == 22");
static_assert(offsetof(NetPacket_Fixed<0x0071>, port) == 26, "offsetof(NetPacket_Fixed<0x0071>, port) == 26");
static_assert(sizeof(NetPacket_Fixed<0x0071>) == 28, "sizeof(NetPacket_Fixed<0x0071>) == 28");
static_assert(alignof(NetPacket_Fixed<0x0071>) == 1, "alignof(NetPacket_Fixed<0x0071>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0061> *network, Packet_Fixed<0x0061> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->old_pass, native.old_pass);
    rv &= native_to_network(&network->new_pass, native.new_pass);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0061> *native, NetPacket_Fixed<0x0061> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->old_pass, network.old_pass);
    rv &= network_to_native(&native->new_pass, network.new_pass);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0062> *network, Packet_Fixed<0x0062> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0062> *native, NetPacket_Fixed<0x0062> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0065> *network, Packet_Fixed<0x0065> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->packet_tmw_version, native.packet_tmw_version);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0065> *native, NetPacket_Fixed<0x0065> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->packet_tmw_version, network.packet_tmw_version);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0066> *network, Packet_Fixed<0x0066> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->code, native.code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0066> *native, NetPacket_Fixed<0x0066> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->code, network.code);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0067> *network, Packet_Fixed<0x0067> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->stats, native.stats);
    rv &= native_to_network(&network->slot, native.slot);
    rv &= native_to_network(&network->hair_color, native.hair_color);
    rv &= native_to_network(&network->hair_style, native.hair_style);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0067> *native, NetPacket_Fixed<0x0067> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->stats, network.stats);
    rv &= network_to_native(&native->slot, network.slot);
    rv &= network_to_native(&native->hair_color, network.hair_color);
    rv &= network_to_native(&native->hair_style, network.hair_style);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0068> *network, Packet_Fixed<0x0068> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->email, native.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0068> *native, NetPacket_Fixed<0x0068> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->email, network.email);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x006b> *network, Packet_Head<0x006b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x006b> *native, NetPacket_Head<0x006b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x006b> *network, Packet_Repeat<0x006b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->char_select, native.char_select);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x006b> *native, NetPacket_Repeat<0x006b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->char_select, network.char_select);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x006c> *network, Packet_Fixed<0x006c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->code, native.code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x006c> *native, NetPacket_Fixed<0x006c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->code, network.code);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x006d> *network, Packet_Fixed<0x006d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_select, native.char_select);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x006d> *native, NetPacket_Fixed<0x006d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_select, network.char_select);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x006e> *network, Packet_Fixed<0x006e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->code, native.code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x006e> *native, NetPacket_Fixed<0x006e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->code, network.code);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x006f> *network, Packet_Fixed<0x006f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x006f> *native, NetPacket_Fixed<0x006f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0070> *network, Packet_Fixed<0x0070> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->code, native.code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0070> *native, NetPacket_Fixed<0x0070> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->code, network.code);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0071> *network, Packet_Fixed<0x0071> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0071> *native, NetPacket_Fixed<0x0071> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    return rv;
}


# pragma pack(pop)

#endif // TMWA_PROTO2_CHAR_USER_HPP
