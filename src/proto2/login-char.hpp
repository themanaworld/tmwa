#ifndef TMWA_PROTO2_LOGIN_CHAR_HPP
#define TMWA_PROTO2_LOGIN_CHAR_HPP
//    login-char.hpp - TMWA network protocol: login/char
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
// This is an internal protocol, and can be changed without notice

template<>
struct Packet_Fixed<0x2709>
{
    static const uint16_t PACKET_ID = 0x2709;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x2710>
{
    static const uint16_t PACKET_ID = 0x2710;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass account_pass = {};
    uint32_t unknown = {};
    IP4Address ip = {};
    uint16_t port = {};
    ServerName server_name = {};
    uint16_t unknown2 = {};
    uint16_t maintenance = {};
    uint16_t is_new = {};
};

template<>
struct Packet_Fixed<0x2711>
{
    static const uint16_t PACKET_ID = 0x2711;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t code = {};
};

template<>
struct Packet_Fixed<0x2712>
{
    static const uint16_t PACKET_ID = 0x2712;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t login_id1 = {};
    uint32_t login_id2 = {};
    SEX sex = {};
    IP4Address ip = {};
};

template<>
struct Packet_Fixed<0x2713>
{
    static const uint16_t PACKET_ID = 0x2713;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t invalid = {};
    AccountEmail email = {};
    TimeT connect_until = {};
};

template<>
struct Packet_Fixed<0x2714>
{
    static const uint16_t PACKET_ID = 0x2714;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint32_t users = {};
};

template<>
struct Packet_Fixed<0x2715>
{
    static const uint16_t PACKET_ID = 0x2715;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountEmail email = {};
};

template<>
struct Packet_Fixed<0x2716>
{
    static const uint16_t PACKET_ID = 0x2716;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x2717>
{
    static const uint16_t PACKET_ID = 0x2717;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountEmail email = {};
    TimeT connect_until = {};
};

template<>
struct Packet_Head<0x2720>
{
    static const uint16_t PACKET_ID = 0x2720;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x2720>
{
    static const uint16_t PACKET_ID = 0x2720;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x2721>
{
    static const uint16_t PACKET_ID = 0x2721;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    GmLevel gm_level = {};
};

template<>
struct Packet_Fixed<0x2722>
{
    static const uint16_t PACKET_ID = 0x2722;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountEmail old_email = {};
    AccountEmail new_email = {};
};

template<>
struct Packet_Fixed<0x2723>
{
    static const uint16_t PACKET_ID = 0x2723;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    SEX sex = {};
};

template<>
struct Packet_Fixed<0x2724>
{
    static const uint16_t PACKET_ID = 0x2724;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t status = {};
};

template<>
struct Packet_Fixed<0x2725>
{
    static const uint16_t PACKET_ID = 0x2725;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    HumanTimeDiff ban_add = {};
};

template<>
struct Packet_Fixed<0x2727>
{
    static const uint16_t PACKET_ID = 0x2727;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Head<0x2728>
{
    static const uint16_t PACKET_ID = 0x2728;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x2728>
{
    static const uint16_t PACKET_ID = 0x2728;

    VarName name = {};
    uint32_t value = {};
};

template<>
struct Packet_Head<0x2729>
{
    static const uint16_t PACKET_ID = 0x2729;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x2729>
{
    static const uint16_t PACKET_ID = 0x2729;

    VarName name = {};
    uint32_t value = {};
};

template<>
struct Packet_Fixed<0x272a>
{
    static const uint16_t PACKET_ID = 0x272a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x2730>
{
    static const uint16_t PACKET_ID = 0x2730;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x2731>
{
    static const uint16_t PACKET_ID = 0x2731;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t ban_not_status = {};
    TimeT status_or_ban_until = {};
};

template<>
struct Packet_Head<0x2732>
{
    static const uint16_t PACKET_ID = 0x2732;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x2732>
{
    static const uint16_t PACKET_ID = 0x2732;

    AccountId account_id = {};
    GmLevel gm_level = {};
};

template<>
struct Packet_Fixed<0x2740>
{
    static const uint16_t PACKET_ID = 0x2740;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountPass old_pass = {};
    AccountPass new_pass = {};
};

template<>
struct Packet_Fixed<0x2741>
{
    static const uint16_t PACKET_ID = 0x2741;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t status = {};
};


template<>
struct NetPacket_Fixed<0x2709>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2709>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2709>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x2709>) == 2, "sizeof(NetPacket_Fixed<0x2709>) == 2");
static_assert(alignof(NetPacket_Fixed<0x2709>) == 1, "alignof(NetPacket_Fixed<0x2709>) == 1");

template<>
struct NetPacket_Fixed<0x2710>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> account_pass;
    Little32 unknown;
    IP4Address ip;
    Little16 port;
    NetString<sizeof(ServerName)> server_name;
    Little16 unknown2;
    Little16 maintenance;
    Little16 is_new;
};
static_assert(offsetof(NetPacket_Fixed<0x2710>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2710>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2710>, account_name) == 2, "offsetof(NetPacket_Fixed<0x2710>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2710>, account_pass) == 26, "offsetof(NetPacket_Fixed<0x2710>, account_pass) == 26");
static_assert(offsetof(NetPacket_Fixed<0x2710>, unknown) == 50, "offsetof(NetPacket_Fixed<0x2710>, unknown) == 50");
static_assert(offsetof(NetPacket_Fixed<0x2710>, ip) == 54, "offsetof(NetPacket_Fixed<0x2710>, ip) == 54");
static_assert(offsetof(NetPacket_Fixed<0x2710>, port) == 58, "offsetof(NetPacket_Fixed<0x2710>, port) == 58");
static_assert(offsetof(NetPacket_Fixed<0x2710>, server_name) == 60, "offsetof(NetPacket_Fixed<0x2710>, server_name) == 60");
static_assert(offsetof(NetPacket_Fixed<0x2710>, unknown2) == 80, "offsetof(NetPacket_Fixed<0x2710>, unknown2) == 80");
static_assert(offsetof(NetPacket_Fixed<0x2710>, maintenance) == 82, "offsetof(NetPacket_Fixed<0x2710>, maintenance) == 82");
static_assert(offsetof(NetPacket_Fixed<0x2710>, is_new) == 84, "offsetof(NetPacket_Fixed<0x2710>, is_new) == 84");
static_assert(sizeof(NetPacket_Fixed<0x2710>) == 86, "sizeof(NetPacket_Fixed<0x2710>) == 86");
static_assert(alignof(NetPacket_Fixed<0x2710>) == 1, "alignof(NetPacket_Fixed<0x2710>) == 1");

template<>
struct NetPacket_Fixed<0x2711>
{
    Little16 magic_packet_id;
    Byte code;
};
static_assert(offsetof(NetPacket_Fixed<0x2711>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2711>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2711>, code) == 2, "offsetof(NetPacket_Fixed<0x2711>, code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2711>) == 3, "sizeof(NetPacket_Fixed<0x2711>) == 3");
static_assert(alignof(NetPacket_Fixed<0x2711>) == 1, "alignof(NetPacket_Fixed<0x2711>) == 1");

template<>
struct NetPacket_Fixed<0x2712>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 login_id1;
    Little32 login_id2;
    Byte sex;
    IP4Address ip;
};
static_assert(offsetof(NetPacket_Fixed<0x2712>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2712>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2712>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2712>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2712>, login_id1) == 6, "offsetof(NetPacket_Fixed<0x2712>, login_id1) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2712>, login_id2) == 10, "offsetof(NetPacket_Fixed<0x2712>, login_id2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x2712>, sex) == 14, "offsetof(NetPacket_Fixed<0x2712>, sex) == 14");
static_assert(offsetof(NetPacket_Fixed<0x2712>, ip) == 15, "offsetof(NetPacket_Fixed<0x2712>, ip) == 15");
static_assert(sizeof(NetPacket_Fixed<0x2712>) == 19, "sizeof(NetPacket_Fixed<0x2712>) == 19");
static_assert(alignof(NetPacket_Fixed<0x2712>) == 1, "alignof(NetPacket_Fixed<0x2712>) == 1");

template<>
struct NetPacket_Fixed<0x2713>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte invalid;
    NetString<sizeof(AccountEmail)> email;
    Little32 connect_until;
};
static_assert(offsetof(NetPacket_Fixed<0x2713>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2713>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2713>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2713>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2713>, invalid) == 6, "offsetof(NetPacket_Fixed<0x2713>, invalid) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2713>, email) == 7, "offsetof(NetPacket_Fixed<0x2713>, email) == 7");
static_assert(offsetof(NetPacket_Fixed<0x2713>, connect_until) == 47, "offsetof(NetPacket_Fixed<0x2713>, connect_until) == 47");
static_assert(sizeof(NetPacket_Fixed<0x2713>) == 51, "sizeof(NetPacket_Fixed<0x2713>) == 51");
static_assert(alignof(NetPacket_Fixed<0x2713>) == 1, "alignof(NetPacket_Fixed<0x2713>) == 1");

template<>
struct NetPacket_Fixed<0x2714>
{
    Little16 magic_packet_id;
    Little32 users;
};
static_assert(offsetof(NetPacket_Fixed<0x2714>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2714>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2714>, users) == 2, "offsetof(NetPacket_Fixed<0x2714>, users) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2714>) == 6, "sizeof(NetPacket_Fixed<0x2714>) == 6");
static_assert(alignof(NetPacket_Fixed<0x2714>) == 1, "alignof(NetPacket_Fixed<0x2714>) == 1");

template<>
struct NetPacket_Fixed<0x2715>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetPacket_Fixed<0x2715>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2715>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2715>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2715>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2715>, email) == 6, "offsetof(NetPacket_Fixed<0x2715>, email) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2715>) == 46, "sizeof(NetPacket_Fixed<0x2715>) == 46");
static_assert(alignof(NetPacket_Fixed<0x2715>) == 1, "alignof(NetPacket_Fixed<0x2715>) == 1");

template<>
struct NetPacket_Fixed<0x2716>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2716>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2716>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2716>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2716>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2716>) == 6, "sizeof(NetPacket_Fixed<0x2716>) == 6");
static_assert(alignof(NetPacket_Fixed<0x2716>) == 1, "alignof(NetPacket_Fixed<0x2716>) == 1");

template<>
struct NetPacket_Fixed<0x2717>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> email;
    Little32 connect_until;
};
static_assert(offsetof(NetPacket_Fixed<0x2717>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2717>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2717>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2717>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2717>, email) == 6, "offsetof(NetPacket_Fixed<0x2717>, email) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2717>, connect_until) == 46, "offsetof(NetPacket_Fixed<0x2717>, connect_until) == 46");
static_assert(sizeof(NetPacket_Fixed<0x2717>) == 50, "sizeof(NetPacket_Fixed<0x2717>) == 50");
static_assert(alignof(NetPacket_Fixed<0x2717>) == 1, "alignof(NetPacket_Fixed<0x2717>) == 1");

template<>
struct NetPacket_Head<0x2720>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x2720>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2720>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2720>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2720>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2720>, account_id) == 4, "offsetof(NetPacket_Head<0x2720>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x2720>) == 8, "sizeof(NetPacket_Head<0x2720>) == 8");
static_assert(alignof(NetPacket_Head<0x2720>) == 1, "alignof(NetPacket_Head<0x2720>) == 1");
template<>
struct NetPacket_Repeat<0x2720>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x2720>, c) == 0, "offsetof(NetPacket_Repeat<0x2720>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x2720>) == 1, "sizeof(NetPacket_Repeat<0x2720>) == 1");
static_assert(alignof(NetPacket_Repeat<0x2720>) == 1, "alignof(NetPacket_Repeat<0x2720>) == 1");

template<>
struct NetPacket_Fixed<0x2721>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 gm_level;
};
static_assert(offsetof(NetPacket_Fixed<0x2721>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2721>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2721>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2721>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2721>, gm_level) == 6, "offsetof(NetPacket_Fixed<0x2721>, gm_level) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2721>) == 10, "sizeof(NetPacket_Fixed<0x2721>) == 10");
static_assert(alignof(NetPacket_Fixed<0x2721>) == 1, "alignof(NetPacket_Fixed<0x2721>) == 1");

template<>
struct NetPacket_Fixed<0x2722>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> old_email;
    NetString<sizeof(AccountEmail)> new_email;
};
static_assert(offsetof(NetPacket_Fixed<0x2722>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2722>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2722>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2722>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2722>, old_email) == 6, "offsetof(NetPacket_Fixed<0x2722>, old_email) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2722>, new_email) == 46, "offsetof(NetPacket_Fixed<0x2722>, new_email) == 46");
static_assert(sizeof(NetPacket_Fixed<0x2722>) == 86, "sizeof(NetPacket_Fixed<0x2722>) == 86");
static_assert(alignof(NetPacket_Fixed<0x2722>) == 1, "alignof(NetPacket_Fixed<0x2722>) == 1");

template<>
struct NetPacket_Fixed<0x2723>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte sex;
};
static_assert(offsetof(NetPacket_Fixed<0x2723>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2723>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2723>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2723>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2723>, sex) == 6, "offsetof(NetPacket_Fixed<0x2723>, sex) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2723>) == 7, "sizeof(NetPacket_Fixed<0x2723>) == 7");
static_assert(alignof(NetPacket_Fixed<0x2723>) == 1, "alignof(NetPacket_Fixed<0x2723>) == 1");

template<>
struct NetPacket_Fixed<0x2724>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 status;
};
static_assert(offsetof(NetPacket_Fixed<0x2724>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2724>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2724>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2724>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2724>, status) == 6, "offsetof(NetPacket_Fixed<0x2724>, status) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2724>) == 10, "sizeof(NetPacket_Fixed<0x2724>) == 10");
static_assert(alignof(NetPacket_Fixed<0x2724>) == 1, "alignof(NetPacket_Fixed<0x2724>) == 1");

template<>
struct NetPacket_Fixed<0x2725>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetHumanTimeDiff ban_add;
};
static_assert(offsetof(NetPacket_Fixed<0x2725>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2725>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2725>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2725>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2725>, ban_add) == 6, "offsetof(NetPacket_Fixed<0x2725>, ban_add) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2725>) == 18, "sizeof(NetPacket_Fixed<0x2725>) == 18");
static_assert(alignof(NetPacket_Fixed<0x2725>) == 1, "alignof(NetPacket_Fixed<0x2725>) == 1");

template<>
struct NetPacket_Fixed<0x2727>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2727>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2727>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2727>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2727>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2727>) == 6, "sizeof(NetPacket_Fixed<0x2727>) == 6");
static_assert(alignof(NetPacket_Fixed<0x2727>) == 1, "alignof(NetPacket_Fixed<0x2727>) == 1");

template<>
struct NetPacket_Head<0x2728>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x2728>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2728>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2728>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2728>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2728>, account_id) == 4, "offsetof(NetPacket_Head<0x2728>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x2728>) == 8, "sizeof(NetPacket_Head<0x2728>) == 8");
static_assert(alignof(NetPacket_Head<0x2728>) == 1, "alignof(NetPacket_Head<0x2728>) == 1");
template<>
struct NetPacket_Repeat<0x2728>
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetPacket_Repeat<0x2728>, name) == 0, "offsetof(NetPacket_Repeat<0x2728>, name) == 0");
static_assert(offsetof(NetPacket_Repeat<0x2728>, value) == 32, "offsetof(NetPacket_Repeat<0x2728>, value) == 32");
static_assert(sizeof(NetPacket_Repeat<0x2728>) == 36, "sizeof(NetPacket_Repeat<0x2728>) == 36");
static_assert(alignof(NetPacket_Repeat<0x2728>) == 1, "alignof(NetPacket_Repeat<0x2728>) == 1");

template<>
struct NetPacket_Head<0x2729>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x2729>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2729>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2729>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2729>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2729>, account_id) == 4, "offsetof(NetPacket_Head<0x2729>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x2729>) == 8, "sizeof(NetPacket_Head<0x2729>) == 8");
static_assert(alignof(NetPacket_Head<0x2729>) == 1, "alignof(NetPacket_Head<0x2729>) == 1");
template<>
struct NetPacket_Repeat<0x2729>
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetPacket_Repeat<0x2729>, name) == 0, "offsetof(NetPacket_Repeat<0x2729>, name) == 0");
static_assert(offsetof(NetPacket_Repeat<0x2729>, value) == 32, "offsetof(NetPacket_Repeat<0x2729>, value) == 32");
static_assert(sizeof(NetPacket_Repeat<0x2729>) == 36, "sizeof(NetPacket_Repeat<0x2729>) == 36");
static_assert(alignof(NetPacket_Repeat<0x2729>) == 1, "alignof(NetPacket_Repeat<0x2729>) == 1");

template<>
struct NetPacket_Fixed<0x272a>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x272a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x272a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x272a>, account_id) == 2, "offsetof(NetPacket_Fixed<0x272a>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x272a>) == 6, "sizeof(NetPacket_Fixed<0x272a>) == 6");
static_assert(alignof(NetPacket_Fixed<0x272a>) == 1, "alignof(NetPacket_Fixed<0x272a>) == 1");

template<>
struct NetPacket_Fixed<0x2730>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2730>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2730>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2730>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2730>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2730>) == 6, "sizeof(NetPacket_Fixed<0x2730>) == 6");
static_assert(alignof(NetPacket_Fixed<0x2730>) == 1, "alignof(NetPacket_Fixed<0x2730>) == 1");

template<>
struct NetPacket_Fixed<0x2731>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte ban_not_status;
    Little32 status_or_ban_until;
};
static_assert(offsetof(NetPacket_Fixed<0x2731>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2731>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2731>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2731>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2731>, ban_not_status) == 6, "offsetof(NetPacket_Fixed<0x2731>, ban_not_status) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2731>, status_or_ban_until) == 7, "offsetof(NetPacket_Fixed<0x2731>, status_or_ban_until) == 7");
static_assert(sizeof(NetPacket_Fixed<0x2731>) == 11, "sizeof(NetPacket_Fixed<0x2731>) == 11");
static_assert(alignof(NetPacket_Fixed<0x2731>) == 1, "alignof(NetPacket_Fixed<0x2731>) == 1");

template<>
struct NetPacket_Head<0x2732>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x2732>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2732>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2732>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2732>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x2732>) == 4, "sizeof(NetPacket_Head<0x2732>) == 4");
static_assert(alignof(NetPacket_Head<0x2732>) == 1, "alignof(NetPacket_Head<0x2732>) == 1");
template<>
struct NetPacket_Repeat<0x2732>
{
    Little32 account_id;
    Byte gm_level;
};
static_assert(offsetof(NetPacket_Repeat<0x2732>, account_id) == 0, "offsetof(NetPacket_Repeat<0x2732>, account_id) == 0");
static_assert(offsetof(NetPacket_Repeat<0x2732>, gm_level) == 4, "offsetof(NetPacket_Repeat<0x2732>, gm_level) == 4");
static_assert(sizeof(NetPacket_Repeat<0x2732>) == 5, "sizeof(NetPacket_Repeat<0x2732>) == 5");
static_assert(alignof(NetPacket_Repeat<0x2732>) == 1, "alignof(NetPacket_Repeat<0x2732>) == 1");

template<>
struct NetPacket_Fixed<0x2740>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountPass)> old_pass;
    NetString<sizeof(AccountPass)> new_pass;
};
static_assert(offsetof(NetPacket_Fixed<0x2740>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2740>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2740>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2740>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2740>, old_pass) == 6, "offsetof(NetPacket_Fixed<0x2740>, old_pass) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2740>, new_pass) == 30, "offsetof(NetPacket_Fixed<0x2740>, new_pass) == 30");
static_assert(sizeof(NetPacket_Fixed<0x2740>) == 54, "sizeof(NetPacket_Fixed<0x2740>) == 54");
static_assert(alignof(NetPacket_Fixed<0x2740>) == 1, "alignof(NetPacket_Fixed<0x2740>) == 1");

template<>
struct NetPacket_Fixed<0x2741>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte status;
};
static_assert(offsetof(NetPacket_Fixed<0x2741>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2741>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2741>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2741>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2741>, status) == 6, "offsetof(NetPacket_Fixed<0x2741>, status) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2741>) == 7, "sizeof(NetPacket_Fixed<0x2741>) == 7");
static_assert(alignof(NetPacket_Fixed<0x2741>) == 1, "alignof(NetPacket_Fixed<0x2741>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2709> *network, Packet_Fixed<0x2709> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2709> *native, NetPacket_Fixed<0x2709> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2710> *network, Packet_Fixed<0x2710> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->account_pass, native.account_pass);
    rv &= native_to_network(&network->unknown, native.unknown);
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    rv &= native_to_network(&network->server_name, native.server_name);
    rv &= native_to_network(&network->unknown2, native.unknown2);
    rv &= native_to_network(&network->maintenance, native.maintenance);
    rv &= native_to_network(&network->is_new, native.is_new);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2710> *native, NetPacket_Fixed<0x2710> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->account_pass, network.account_pass);
    rv &= network_to_native(&native->unknown, network.unknown);
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    rv &= network_to_native(&native->server_name, network.server_name);
    rv &= network_to_native(&native->unknown2, network.unknown2);
    rv &= network_to_native(&native->maintenance, network.maintenance);
    rv &= network_to_native(&native->is_new, network.is_new);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2711> *network, Packet_Fixed<0x2711> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->code, native.code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2711> *native, NetPacket_Fixed<0x2711> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->code, network.code);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2712> *network, Packet_Fixed<0x2712> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->ip, native.ip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2712> *native, NetPacket_Fixed<0x2712> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->ip, network.ip);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2713> *network, Packet_Fixed<0x2713> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->invalid, native.invalid);
    rv &= native_to_network(&network->email, native.email);
    rv &= native_to_network(&network->connect_until, native.connect_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2713> *native, NetPacket_Fixed<0x2713> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->invalid, network.invalid);
    rv &= network_to_native(&native->email, network.email);
    rv &= network_to_native(&native->connect_until, network.connect_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2714> *network, Packet_Fixed<0x2714> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->users, native.users);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2714> *native, NetPacket_Fixed<0x2714> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->users, network.users);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2715> *network, Packet_Fixed<0x2715> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->email, native.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2715> *native, NetPacket_Fixed<0x2715> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->email, network.email);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2716> *network, Packet_Fixed<0x2716> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2716> *native, NetPacket_Fixed<0x2716> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2717> *network, Packet_Fixed<0x2717> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->email, native.email);
    rv &= native_to_network(&network->connect_until, native.connect_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2717> *native, NetPacket_Fixed<0x2717> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->email, network.email);
    rv &= network_to_native(&native->connect_until, network.connect_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2720> *network, Packet_Head<0x2720> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2720> *native, NetPacket_Head<0x2720> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2720> *network, Packet_Repeat<0x2720> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2720> *native, NetPacket_Repeat<0x2720> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2721> *network, Packet_Fixed<0x2721> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2721> *native, NetPacket_Fixed<0x2721> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2722> *network, Packet_Fixed<0x2722> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->old_email, native.old_email);
    rv &= native_to_network(&network->new_email, native.new_email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2722> *native, NetPacket_Fixed<0x2722> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->old_email, network.old_email);
    rv &= network_to_native(&native->new_email, network.new_email);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2723> *network, Packet_Fixed<0x2723> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2723> *native, NetPacket_Fixed<0x2723> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2724> *network, Packet_Fixed<0x2724> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2724> *native, NetPacket_Fixed<0x2724> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2725> *network, Packet_Fixed<0x2725> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->ban_add, native.ban_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2725> *native, NetPacket_Fixed<0x2725> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->ban_add, network.ban_add);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2727> *network, Packet_Fixed<0x2727> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2727> *native, NetPacket_Fixed<0x2727> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2728> *network, Packet_Head<0x2728> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2728> *native, NetPacket_Head<0x2728> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2728> *network, Packet_Repeat<0x2728> native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2728> *native, NetPacket_Repeat<0x2728> network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2729> *network, Packet_Head<0x2729> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2729> *native, NetPacket_Head<0x2729> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2729> *network, Packet_Repeat<0x2729> native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2729> *native, NetPacket_Repeat<0x2729> network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x272a> *network, Packet_Fixed<0x272a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x272a> *native, NetPacket_Fixed<0x272a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2730> *network, Packet_Fixed<0x2730> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2730> *native, NetPacket_Fixed<0x2730> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2731> *network, Packet_Fixed<0x2731> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->ban_not_status, native.ban_not_status);
    rv &= native_to_network(&network->status_or_ban_until, native.status_or_ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2731> *native, NetPacket_Fixed<0x2731> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->ban_not_status, network.ban_not_status);
    rv &= network_to_native(&native->status_or_ban_until, network.status_or_ban_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2732> *network, Packet_Head<0x2732> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2732> *native, NetPacket_Head<0x2732> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2732> *network, Packet_Repeat<0x2732> native)
{
    bool rv = true;
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2732> *native, NetPacket_Repeat<0x2732> network)
{
    bool rv = true;
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2740> *network, Packet_Fixed<0x2740> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->old_pass, native.old_pass);
    rv &= native_to_network(&network->new_pass, native.new_pass);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2740> *native, NetPacket_Fixed<0x2740> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->old_pass, network.old_pass);
    rv &= network_to_native(&native->new_pass, network.new_pass);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2741> *network, Packet_Fixed<0x2741> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2741> *native, NetPacket_Fixed<0x2741> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}

} // namespace tmwa

#endif // TMWA_PROTO2_LOGIN_CHAR_HPP
