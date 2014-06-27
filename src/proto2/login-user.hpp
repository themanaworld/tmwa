#ifndef TMWA_PROTO2_LOGIN_USER_HPP
#define TMWA_PROTO2_LOGIN_USER_HPP
//    login-user.hpp - TMWA network protocol: login/user
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

namespace tmwa
{
// This is a public protocol, and changes require client cooperation

template<>
struct Packet_Head<0x0063>
{
    static const uint16_t PACKET_ID = 0x0063;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x0063>
{
    static const uint16_t PACKET_ID = 0x0063;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x0064>
{
    static const uint16_t PACKET_ID = 0x0064;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint32_t unknown = {};
    AccountName account_name = {};
    AccountPass account_pass = {};
    VERSION_2 version_2_flags = {};
};

template<>
struct Packet_Head<0x0069>
{
    static const uint16_t PACKET_ID = 0x0069;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    uint32_t login_id1 = {};
    AccountId account_id = {};
    uint32_t login_id2 = {};
    uint32_t unused = {};
    timestamp_milliseconds_buffer last_login_string = {};
    uint16_t unused2 = {};
    SEX sex = {};
};
template<>
struct Packet_Repeat<0x0069>
{
    static const uint16_t PACKET_ID = 0x0069;

    IP4Address ip = {};
    uint16_t port = {};
    ServerName server_name = {};
    uint16_t users = {};
    uint16_t maintenance = {};
    uint16_t is_new = {};
};

template<>
struct Packet_Fixed<0x006a>
{
    static const uint16_t PACKET_ID = 0x006a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t error_code = {};
    timestamp_seconds_buffer error_message = {};
};


template<>
struct NetPacket_Head<0x0063>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x0063>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x0063>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x0063>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x0063>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x0063>) == 4, "sizeof(NetPacket_Head<0x0063>) == 4");
static_assert(alignof(NetPacket_Head<0x0063>) == 1, "alignof(NetPacket_Head<0x0063>) == 1");
template<>
struct NetPacket_Repeat<0x0063>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x0063>, c) == 0, "offsetof(NetPacket_Repeat<0x0063>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x0063>) == 1, "sizeof(NetPacket_Repeat<0x0063>) == 1");
static_assert(alignof(NetPacket_Repeat<0x0063>) == 1, "alignof(NetPacket_Repeat<0x0063>) == 1");

template<>
struct NetPacket_Fixed<0x0064>
{
    Little16 magic_packet_id;
    Little32 unknown;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> account_pass;
    Byte version_2_flags;
};
static_assert(offsetof(NetPacket_Fixed<0x0064>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0064>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0064>, unknown) == 2, "offsetof(NetPacket_Fixed<0x0064>, unknown) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0064>, account_name) == 6, "offsetof(NetPacket_Fixed<0x0064>, account_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0064>, account_pass) == 30, "offsetof(NetPacket_Fixed<0x0064>, account_pass) == 30");
static_assert(offsetof(NetPacket_Fixed<0x0064>, version_2_flags) == 54, "offsetof(NetPacket_Fixed<0x0064>, version_2_flags) == 54");
static_assert(sizeof(NetPacket_Fixed<0x0064>) == 55, "sizeof(NetPacket_Fixed<0x0064>) == 55");
static_assert(alignof(NetPacket_Fixed<0x0064>) == 1, "alignof(NetPacket_Fixed<0x0064>) == 1");

template<>
struct NetPacket_Head<0x0069>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 login_id1;
    Little32 account_id;
    Little32 login_id2;
    Little32 unused;
    NetString<sizeof(timestamp_milliseconds_buffer)> last_login_string;
    Little16 unused2;
    Byte sex;
};
static_assert(offsetof(NetPacket_Head<0x0069>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x0069>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x0069>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x0069>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x0069>, login_id1) == 4, "offsetof(NetPacket_Head<0x0069>, login_id1) == 4");
static_assert(offsetof(NetPacket_Head<0x0069>, account_id) == 8, "offsetof(NetPacket_Head<0x0069>, account_id) == 8");
static_assert(offsetof(NetPacket_Head<0x0069>, login_id2) == 12, "offsetof(NetPacket_Head<0x0069>, login_id2) == 12");
static_assert(offsetof(NetPacket_Head<0x0069>, unused) == 16, "offsetof(NetPacket_Head<0x0069>, unused) == 16");
static_assert(offsetof(NetPacket_Head<0x0069>, last_login_string) == 20, "offsetof(NetPacket_Head<0x0069>, last_login_string) == 20");
static_assert(offsetof(NetPacket_Head<0x0069>, unused2) == 44, "offsetof(NetPacket_Head<0x0069>, unused2) == 44");
static_assert(offsetof(NetPacket_Head<0x0069>, sex) == 46, "offsetof(NetPacket_Head<0x0069>, sex) == 46");
static_assert(sizeof(NetPacket_Head<0x0069>) == 47, "sizeof(NetPacket_Head<0x0069>) == 47");
static_assert(alignof(NetPacket_Head<0x0069>) == 1, "alignof(NetPacket_Head<0x0069>) == 1");
template<>
struct NetPacket_Repeat<0x0069>
{
    IP4Address ip;
    Little16 port;
    NetString<sizeof(ServerName)> server_name;
    Little16 users;
    Little16 maintenance;
    Little16 is_new;
};
static_assert(offsetof(NetPacket_Repeat<0x0069>, ip) == 0, "offsetof(NetPacket_Repeat<0x0069>, ip) == 0");
static_assert(offsetof(NetPacket_Repeat<0x0069>, port) == 4, "offsetof(NetPacket_Repeat<0x0069>, port) == 4");
static_assert(offsetof(NetPacket_Repeat<0x0069>, server_name) == 6, "offsetof(NetPacket_Repeat<0x0069>, server_name) == 6");
static_assert(offsetof(NetPacket_Repeat<0x0069>, users) == 26, "offsetof(NetPacket_Repeat<0x0069>, users) == 26");
static_assert(offsetof(NetPacket_Repeat<0x0069>, maintenance) == 28, "offsetof(NetPacket_Repeat<0x0069>, maintenance) == 28");
static_assert(offsetof(NetPacket_Repeat<0x0069>, is_new) == 30, "offsetof(NetPacket_Repeat<0x0069>, is_new) == 30");
static_assert(sizeof(NetPacket_Repeat<0x0069>) == 32, "sizeof(NetPacket_Repeat<0x0069>) == 32");
static_assert(alignof(NetPacket_Repeat<0x0069>) == 1, "alignof(NetPacket_Repeat<0x0069>) == 1");

template<>
struct NetPacket_Fixed<0x006a>
{
    Little16 magic_packet_id;
    Byte error_code;
    NetString<sizeof(timestamp_seconds_buffer)> error_message;
};
static_assert(offsetof(NetPacket_Fixed<0x006a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x006a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x006a>, error_code) == 2, "offsetof(NetPacket_Fixed<0x006a>, error_code) == 2");
static_assert(offsetof(NetPacket_Fixed<0x006a>, error_message) == 3, "offsetof(NetPacket_Fixed<0x006a>, error_message) == 3");
static_assert(sizeof(NetPacket_Fixed<0x006a>) == 23, "sizeof(NetPacket_Fixed<0x006a>) == 23");
static_assert(alignof(NetPacket_Fixed<0x006a>) == 1, "alignof(NetPacket_Fixed<0x006a>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x0063> *network, Packet_Head<0x0063> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x0063> *native, NetPacket_Head<0x0063> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x0063> *network, Packet_Repeat<0x0063> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x0063> *native, NetPacket_Repeat<0x0063> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0064> *network, Packet_Fixed<0x0064> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unknown, native.unknown);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->account_pass, native.account_pass);
    rv &= native_to_network(&network->version_2_flags, native.version_2_flags);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0064> *native, NetPacket_Fixed<0x0064> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unknown, network.unknown);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->account_pass, network.account_pass);
    rv &= network_to_native(&native->version_2_flags, network.version_2_flags);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x0069> *network, Packet_Head<0x0069> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->last_login_string, native.last_login_string);
    rv &= native_to_network(&network->unused2, native.unused2);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x0069> *native, NetPacket_Head<0x0069> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->last_login_string, network.last_login_string);
    rv &= network_to_native(&native->unused2, network.unused2);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x0069> *network, Packet_Repeat<0x0069> native)
{
    bool rv = true;
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    rv &= native_to_network(&network->server_name, native.server_name);
    rv &= native_to_network(&network->users, native.users);
    rv &= native_to_network(&network->maintenance, native.maintenance);
    rv &= native_to_network(&network->is_new, native.is_new);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x0069> *native, NetPacket_Repeat<0x0069> network)
{
    bool rv = true;
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    rv &= network_to_native(&native->server_name, network.server_name);
    rv &= network_to_native(&native->users, network.users);
    rv &= network_to_native(&native->maintenance, network.maintenance);
    rv &= network_to_native(&native->is_new, network.is_new);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x006a> *network, Packet_Fixed<0x006a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->error_code, native.error_code);
    rv &= native_to_network(&network->error_message, native.error_message);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x006a> *native, NetPacket_Fixed<0x006a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->error_code, network.error_code);
    rv &= network_to_native(&native->error_message, network.error_message);
    return rv;
}

} // namespace tmwa

#endif // TMWA_PROTO2_LOGIN_USER_HPP
