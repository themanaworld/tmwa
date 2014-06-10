#ifndef TMWA_PROTO2_LOGIN_ADMIN_HPP
#define TMWA_PROTO2_LOGIN_ADMIN_HPP
//    login-admin.hpp - TMWA network protocol: login/admin
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

// This is an internal protocol, and can be changed without notice

// this is only needed for the payload packet right now, and that needs to die
# pragma pack(push, 1)

template<>
struct Packet_Head<0x2726>
{
    static const uint16_t PACKET_ID = 0x2726;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t unused = {};
    // TODO remove this
    uint32_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x2726>
{
    static const uint16_t PACKET_ID = 0x2726;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x7918>
{
    static const uint16_t PACKET_ID = 0x7918;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t encryption_zero = {};
    AccountPass account_pass = {};
};

template<>
struct Packet_Fixed<0x7919>
{
    static const uint16_t PACKET_ID = 0x7919;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t error = {};
};

template<>
struct Packet_Fixed<0x7920>
{
    static const uint16_t PACKET_ID = 0x7920;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId start_account_id = {};
    AccountId end_account_id = {};
};

template<>
struct Packet_Head<0x7921>
{
    static const uint16_t PACKET_ID = 0x7921;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x7921>
{
    static const uint16_t PACKET_ID = 0x7921;

    AccountId account_id = {};
    GmLevel gm_level = {};
    AccountName account_name = {};
    SEX sex = {};
    uint32_t login_count = {};
    uint32_t status = {};
};

template<>
struct Packet_Fixed<0x7924>
{
    static const uint16_t PACKET_ID = 0x7924;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    ItemNameId source_item_id = {};
    ItemNameId dest_item_id = {};
};

template<>
struct Packet_Fixed<0x7925>
{
    static const uint16_t PACKET_ID = 0x7925;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x7930>
{
    static const uint16_t PACKET_ID = 0x7930;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass password = {};
    SEX sex = {};
    AccountEmail email = {};
};

template<>
struct Packet_Fixed<0x7931>
{
    static const uint16_t PACKET_ID = 0x7931;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7932>
{
    static const uint16_t PACKET_ID = 0x7932;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7933>
{
    static const uint16_t PACKET_ID = 0x7933;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7934>
{
    static const uint16_t PACKET_ID = 0x7934;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass password = {};
};

template<>
struct Packet_Fixed<0x7935>
{
    static const uint16_t PACKET_ID = 0x7935;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7936>
{
    static const uint16_t PACKET_ID = 0x7936;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    uint32_t status = {};
    timestamp_seconds_buffer error_message = {};
};

template<>
struct Packet_Fixed<0x7937>
{
    static const uint16_t PACKET_ID = 0x7937;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    uint32_t status = {};
};

template<>
struct Packet_Fixed<0x7938>
{
    static const uint16_t PACKET_ID = 0x7938;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Head<0x7939>
{
    static const uint16_t PACKET_ID = 0x7939;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x7939>
{
    static const uint16_t PACKET_ID = 0x7939;

    IP4Address ip = {};
    uint16_t port = {};
    ServerName name = {};
    uint16_t users = {};
    uint16_t maintenance = {};
    uint16_t is_new = {};
};

template<>
struct Packet_Fixed<0x793a>
{
    static const uint16_t PACKET_ID = 0x793a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass password = {};
};

template<>
struct Packet_Fixed<0x793b>
{
    static const uint16_t PACKET_ID = 0x793b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x793c>
{
    static const uint16_t PACKET_ID = 0x793c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    SEX sex = {};
};

template<>
struct Packet_Fixed<0x793d>
{
    static const uint16_t PACKET_ID = 0x793d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x793e>
{
    static const uint16_t PACKET_ID = 0x793e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    GmLevel gm_level = {};
};

template<>
struct Packet_Fixed<0x793f>
{
    static const uint16_t PACKET_ID = 0x793f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7940>
{
    static const uint16_t PACKET_ID = 0x7940;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountEmail email = {};
};

template<>
struct Packet_Fixed<0x7941>
{
    static const uint16_t PACKET_ID = 0x7941;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Head<0x7942>
{
    static const uint16_t PACKET_ID = 0x7942;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x7942>
{
    static const uint16_t PACKET_ID = 0x7942;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x7943>
{
    static const uint16_t PACKET_ID = 0x7943;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7944>
{
    static const uint16_t PACKET_ID = 0x7944;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7945>
{
    static const uint16_t PACKET_ID = 0x7945;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7946>
{
    static const uint16_t PACKET_ID = 0x7946;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x7947>
{
    static const uint16_t PACKET_ID = 0x7947;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};

template<>
struct Packet_Fixed<0x7948>
{
    static const uint16_t PACKET_ID = 0x7948;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    TimeT valid_until = {};
};

template<>
struct Packet_Fixed<0x7949>
{
    static const uint16_t PACKET_ID = 0x7949;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT valid_until = {};
};

template<>
struct Packet_Fixed<0x794a>
{
    static const uint16_t PACKET_ID = 0x794a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    TimeT ban_until = {};
};

template<>
struct Packet_Fixed<0x794b>
{
    static const uint16_t PACKET_ID = 0x794b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT ban_until = {};
};

template<>
struct Packet_Fixed<0x794c>
{
    static const uint16_t PACKET_ID = 0x794c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    HumanTimeDiff ban_add = {};
};

template<>
struct Packet_Fixed<0x794d>
{
    static const uint16_t PACKET_ID = 0x794d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT ban_until = {};
};

template<>
struct Packet_Head<0x794e>
{
    static const uint16_t PACKET_ID = 0x794e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t unused = {};
    // TODO remove this
    uint32_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x794e>
{
    static const uint16_t PACKET_ID = 0x794e;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x794f>
{
    static const uint16_t PACKET_ID = 0x794f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t error = {};
};

template<>
struct Packet_Fixed<0x7950>
{
    static const uint16_t PACKET_ID = 0x7950;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    HumanTimeDiff valid_add = {};
};

template<>
struct Packet_Fixed<0x7951>
{
    static const uint16_t PACKET_ID = 0x7951;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT valid_until = {};
};

template<>
struct Packet_Fixed<0x7952>
{
    static const uint16_t PACKET_ID = 0x7952;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
};

template<>
struct Packet_Head<0x7953>
{
    static const uint16_t PACKET_ID = 0x7953;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    GmLevel gm_level = {};
    AccountName account_name = {};
    SEX sex = {};
    uint32_t login_count = {};
    uint32_t state = {};
    timestamp_seconds_buffer error_message = {};
    timestamp_milliseconds_buffer last_login_string = {};
    VString<15> ip_string = {};
    AccountEmail email = {};
    TimeT connect_until = {};
    TimeT ban_until = {};
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x7953>
{
    static const uint16_t PACKET_ID = 0x7953;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x7954>
{
    static const uint16_t PACKET_ID = 0x7954;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x7955>
{
    static const uint16_t PACKET_ID = 0x7955;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};


template<>
struct NetPacket_Head<0x2726>
{
    Little16 magic_packet_id;
    Little16 unused;
    SkewedLength<Little32, 8> magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x2726>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2726>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2726>, unused) == 2, "offsetof(NetPacket_Head<0x2726>, unused) == 2");
static_assert(offsetof(NetPacket_Head<0x2726>, magic_packet_length) == 4, "offsetof(NetPacket_Head<0x2726>, magic_packet_length) == 4");
static_assert(sizeof(NetPacket_Head<0x2726>) == 8, "sizeof(NetPacket_Head<0x2726>) == 8");
static_assert(alignof(NetPacket_Head<0x2726>) == 1, "alignof(NetPacket_Head<0x2726>) == 1");
template<>
struct NetPacket_Repeat<0x2726>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x2726>, c) == 0, "offsetof(NetPacket_Repeat<0x2726>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x2726>) == 1, "sizeof(NetPacket_Repeat<0x2726>) == 1");
static_assert(alignof(NetPacket_Repeat<0x2726>) == 1, "alignof(NetPacket_Repeat<0x2726>) == 1");

template<>
struct NetPacket_Fixed<0x7918>
{
    Little16 magic_packet_id;
    Little16 encryption_zero;
    NetString<sizeof(AccountPass)> account_pass;
};
static_assert(offsetof(NetPacket_Fixed<0x7918>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7918>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7918>, encryption_zero) == 2, "offsetof(NetPacket_Fixed<0x7918>, encryption_zero) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7918>, account_pass) == 4, "offsetof(NetPacket_Fixed<0x7918>, account_pass) == 4");
static_assert(sizeof(NetPacket_Fixed<0x7918>) == 28, "sizeof(NetPacket_Fixed<0x7918>) == 28");
static_assert(alignof(NetPacket_Fixed<0x7918>) == 1, "alignof(NetPacket_Fixed<0x7918>) == 1");

template<>
struct NetPacket_Fixed<0x7919>
{
    Little16 magic_packet_id;
    Byte error;
};
static_assert(offsetof(NetPacket_Fixed<0x7919>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7919>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7919>, error) == 2, "offsetof(NetPacket_Fixed<0x7919>, error) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7919>) == 3, "sizeof(NetPacket_Fixed<0x7919>) == 3");
static_assert(alignof(NetPacket_Fixed<0x7919>) == 1, "alignof(NetPacket_Fixed<0x7919>) == 1");

template<>
struct NetPacket_Fixed<0x7920>
{
    Little16 magic_packet_id;
    Little32 start_account_id;
    Little32 end_account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7920>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7920>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7920>, start_account_id) == 2, "offsetof(NetPacket_Fixed<0x7920>, start_account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7920>, end_account_id) == 6, "offsetof(NetPacket_Fixed<0x7920>, end_account_id) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7920>) == 10, "sizeof(NetPacket_Fixed<0x7920>) == 10");
static_assert(alignof(NetPacket_Fixed<0x7920>) == 1, "alignof(NetPacket_Fixed<0x7920>) == 1");

template<>
struct NetPacket_Head<0x7921>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x7921>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x7921>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x7921>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x7921>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x7921>) == 4, "sizeof(NetPacket_Head<0x7921>) == 4");
static_assert(alignof(NetPacket_Head<0x7921>) == 1, "alignof(NetPacket_Head<0x7921>) == 1");
template<>
struct NetPacket_Repeat<0x7921>
{
    Little32 account_id;
    Byte gm_level;
    NetString<sizeof(AccountName)> account_name;
    Byte sex;
    Little32 login_count;
    Little32 status;
};
static_assert(offsetof(NetPacket_Repeat<0x7921>, account_id) == 0, "offsetof(NetPacket_Repeat<0x7921>, account_id) == 0");
static_assert(offsetof(NetPacket_Repeat<0x7921>, gm_level) == 4, "offsetof(NetPacket_Repeat<0x7921>, gm_level) == 4");
static_assert(offsetof(NetPacket_Repeat<0x7921>, account_name) == 5, "offsetof(NetPacket_Repeat<0x7921>, account_name) == 5");
static_assert(offsetof(NetPacket_Repeat<0x7921>, sex) == 29, "offsetof(NetPacket_Repeat<0x7921>, sex) == 29");
static_assert(offsetof(NetPacket_Repeat<0x7921>, login_count) == 30, "offsetof(NetPacket_Repeat<0x7921>, login_count) == 30");
static_assert(offsetof(NetPacket_Repeat<0x7921>, status) == 34, "offsetof(NetPacket_Repeat<0x7921>, status) == 34");
static_assert(sizeof(NetPacket_Repeat<0x7921>) == 38, "sizeof(NetPacket_Repeat<0x7921>) == 38");
static_assert(alignof(NetPacket_Repeat<0x7921>) == 1, "alignof(NetPacket_Repeat<0x7921>) == 1");

template<>
struct NetPacket_Fixed<0x7924>
{
    Little16 magic_packet_id;
    Little32 source_item_id;
    Little32 dest_item_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7924>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7924>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7924>, source_item_id) == 2, "offsetof(NetPacket_Fixed<0x7924>, source_item_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7924>, dest_item_id) == 6, "offsetof(NetPacket_Fixed<0x7924>, dest_item_id) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7924>) == 10, "sizeof(NetPacket_Fixed<0x7924>) == 10");
static_assert(alignof(NetPacket_Fixed<0x7924>) == 1, "alignof(NetPacket_Fixed<0x7924>) == 1");

template<>
struct NetPacket_Fixed<0x7925>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7925>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7925>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7925>) == 2, "sizeof(NetPacket_Fixed<0x7925>) == 2");
static_assert(alignof(NetPacket_Fixed<0x7925>) == 1, "alignof(NetPacket_Fixed<0x7925>) == 1");

template<>
struct NetPacket_Fixed<0x7930>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> password;
    char sex;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetPacket_Fixed<0x7930>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7930>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7930>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7930>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7930>, password) == 26, "offsetof(NetPacket_Fixed<0x7930>, password) == 26");
static_assert(offsetof(NetPacket_Fixed<0x7930>, sex) == 50, "offsetof(NetPacket_Fixed<0x7930>, sex) == 50");
static_assert(offsetof(NetPacket_Fixed<0x7930>, email) == 51, "offsetof(NetPacket_Fixed<0x7930>, email) == 51");
static_assert(sizeof(NetPacket_Fixed<0x7930>) == 91, "sizeof(NetPacket_Fixed<0x7930>) == 91");
static_assert(alignof(NetPacket_Fixed<0x7930>) == 1, "alignof(NetPacket_Fixed<0x7930>) == 1");

template<>
struct NetPacket_Fixed<0x7931>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7931>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7931>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7931>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7931>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7931>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7931>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7931>) == 30, "sizeof(NetPacket_Fixed<0x7931>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7931>) == 1, "alignof(NetPacket_Fixed<0x7931>) == 1");

template<>
struct NetPacket_Fixed<0x7932>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7932>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7932>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7932>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7932>, account_name) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7932>) == 26, "sizeof(NetPacket_Fixed<0x7932>) == 26");
static_assert(alignof(NetPacket_Fixed<0x7932>) == 1, "alignof(NetPacket_Fixed<0x7932>) == 1");

template<>
struct NetPacket_Fixed<0x7933>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7933>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7933>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7933>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7933>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7933>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7933>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7933>) == 30, "sizeof(NetPacket_Fixed<0x7933>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7933>) == 1, "alignof(NetPacket_Fixed<0x7933>) == 1");

template<>
struct NetPacket_Fixed<0x7934>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> password;
};
static_assert(offsetof(NetPacket_Fixed<0x7934>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7934>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7934>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7934>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7934>, password) == 26, "offsetof(NetPacket_Fixed<0x7934>, password) == 26");
static_assert(sizeof(NetPacket_Fixed<0x7934>) == 50, "sizeof(NetPacket_Fixed<0x7934>) == 50");
static_assert(alignof(NetPacket_Fixed<0x7934>) == 1, "alignof(NetPacket_Fixed<0x7934>) == 1");

template<>
struct NetPacket_Fixed<0x7935>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7935>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7935>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7935>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7935>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7935>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7935>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7935>) == 30, "sizeof(NetPacket_Fixed<0x7935>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7935>) == 1, "alignof(NetPacket_Fixed<0x7935>) == 1");

template<>
struct NetPacket_Fixed<0x7936>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 status;
    NetString<sizeof(timestamp_seconds_buffer)> error_message;
};
static_assert(offsetof(NetPacket_Fixed<0x7936>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7936>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7936>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7936>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7936>, status) == 26, "offsetof(NetPacket_Fixed<0x7936>, status) == 26");
static_assert(offsetof(NetPacket_Fixed<0x7936>, error_message) == 30, "offsetof(NetPacket_Fixed<0x7936>, error_message) == 30");
static_assert(sizeof(NetPacket_Fixed<0x7936>) == 50, "sizeof(NetPacket_Fixed<0x7936>) == 50");
static_assert(alignof(NetPacket_Fixed<0x7936>) == 1, "alignof(NetPacket_Fixed<0x7936>) == 1");

template<>
struct NetPacket_Fixed<0x7937>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 status;
};
static_assert(offsetof(NetPacket_Fixed<0x7937>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7937>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7937>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7937>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7937>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7937>, account_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x7937>, status) == 30, "offsetof(NetPacket_Fixed<0x7937>, status) == 30");
static_assert(sizeof(NetPacket_Fixed<0x7937>) == 34, "sizeof(NetPacket_Fixed<0x7937>) == 34");
static_assert(alignof(NetPacket_Fixed<0x7937>) == 1, "alignof(NetPacket_Fixed<0x7937>) == 1");

template<>
struct NetPacket_Fixed<0x7938>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7938>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7938>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7938>) == 2, "sizeof(NetPacket_Fixed<0x7938>) == 2");
static_assert(alignof(NetPacket_Fixed<0x7938>) == 1, "alignof(NetPacket_Fixed<0x7938>) == 1");

template<>
struct NetPacket_Head<0x7939>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x7939>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x7939>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x7939>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x7939>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x7939>) == 4, "sizeof(NetPacket_Head<0x7939>) == 4");
static_assert(alignof(NetPacket_Head<0x7939>) == 1, "alignof(NetPacket_Head<0x7939>) == 1");
template<>
struct NetPacket_Repeat<0x7939>
{
    IP4Address ip;
    Little16 port;
    NetString<sizeof(ServerName)> name;
    Little16 users;
    Little16 maintenance;
    Little16 is_new;
};
static_assert(offsetof(NetPacket_Repeat<0x7939>, ip) == 0, "offsetof(NetPacket_Repeat<0x7939>, ip) == 0");
static_assert(offsetof(NetPacket_Repeat<0x7939>, port) == 4, "offsetof(NetPacket_Repeat<0x7939>, port) == 4");
static_assert(offsetof(NetPacket_Repeat<0x7939>, name) == 6, "offsetof(NetPacket_Repeat<0x7939>, name) == 6");
static_assert(offsetof(NetPacket_Repeat<0x7939>, users) == 26, "offsetof(NetPacket_Repeat<0x7939>, users) == 26");
static_assert(offsetof(NetPacket_Repeat<0x7939>, maintenance) == 28, "offsetof(NetPacket_Repeat<0x7939>, maintenance) == 28");
static_assert(offsetof(NetPacket_Repeat<0x7939>, is_new) == 30, "offsetof(NetPacket_Repeat<0x7939>, is_new) == 30");
static_assert(sizeof(NetPacket_Repeat<0x7939>) == 32, "sizeof(NetPacket_Repeat<0x7939>) == 32");
static_assert(alignof(NetPacket_Repeat<0x7939>) == 1, "alignof(NetPacket_Repeat<0x7939>) == 1");

template<>
struct NetPacket_Fixed<0x793a>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> password;
};
static_assert(offsetof(NetPacket_Fixed<0x793a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x793a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x793a>, account_name) == 2, "offsetof(NetPacket_Fixed<0x793a>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x793a>, password) == 26, "offsetof(NetPacket_Fixed<0x793a>, password) == 26");
static_assert(sizeof(NetPacket_Fixed<0x793a>) == 50, "sizeof(NetPacket_Fixed<0x793a>) == 50");
static_assert(alignof(NetPacket_Fixed<0x793a>) == 1, "alignof(NetPacket_Fixed<0x793a>) == 1");

template<>
struct NetPacket_Fixed<0x793b>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x793b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x793b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x793b>, account_id) == 2, "offsetof(NetPacket_Fixed<0x793b>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x793b>, account_name) == 6, "offsetof(NetPacket_Fixed<0x793b>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x793b>) == 30, "sizeof(NetPacket_Fixed<0x793b>) == 30");
static_assert(alignof(NetPacket_Fixed<0x793b>) == 1, "alignof(NetPacket_Fixed<0x793b>) == 1");

template<>
struct NetPacket_Fixed<0x793c>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    char sex;
};
static_assert(offsetof(NetPacket_Fixed<0x793c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x793c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x793c>, account_name) == 2, "offsetof(NetPacket_Fixed<0x793c>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x793c>, sex) == 26, "offsetof(NetPacket_Fixed<0x793c>, sex) == 26");
static_assert(sizeof(NetPacket_Fixed<0x793c>) == 27, "sizeof(NetPacket_Fixed<0x793c>) == 27");
static_assert(alignof(NetPacket_Fixed<0x793c>) == 1, "alignof(NetPacket_Fixed<0x793c>) == 1");

template<>
struct NetPacket_Fixed<0x793d>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x793d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x793d>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x793d>, account_id) == 2, "offsetof(NetPacket_Fixed<0x793d>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x793d>, account_name) == 6, "offsetof(NetPacket_Fixed<0x793d>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x793d>) == 30, "sizeof(NetPacket_Fixed<0x793d>) == 30");
static_assert(alignof(NetPacket_Fixed<0x793d>) == 1, "alignof(NetPacket_Fixed<0x793d>) == 1");

template<>
struct NetPacket_Fixed<0x793e>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Byte gm_level;
};
static_assert(offsetof(NetPacket_Fixed<0x793e>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x793e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x793e>, account_name) == 2, "offsetof(NetPacket_Fixed<0x793e>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x793e>, gm_level) == 26, "offsetof(NetPacket_Fixed<0x793e>, gm_level) == 26");
static_assert(sizeof(NetPacket_Fixed<0x793e>) == 27, "sizeof(NetPacket_Fixed<0x793e>) == 27");
static_assert(alignof(NetPacket_Fixed<0x793e>) == 1, "alignof(NetPacket_Fixed<0x793e>) == 1");

template<>
struct NetPacket_Fixed<0x793f>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x793f>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x793f>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x793f>, account_id) == 2, "offsetof(NetPacket_Fixed<0x793f>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x793f>, account_name) == 6, "offsetof(NetPacket_Fixed<0x793f>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x793f>) == 30, "sizeof(NetPacket_Fixed<0x793f>) == 30");
static_assert(alignof(NetPacket_Fixed<0x793f>) == 1, "alignof(NetPacket_Fixed<0x793f>) == 1");

template<>
struct NetPacket_Fixed<0x7940>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetPacket_Fixed<0x7940>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7940>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7940>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7940>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7940>, email) == 26, "offsetof(NetPacket_Fixed<0x7940>, email) == 26");
static_assert(sizeof(NetPacket_Fixed<0x7940>) == 66, "sizeof(NetPacket_Fixed<0x7940>) == 66");
static_assert(alignof(NetPacket_Fixed<0x7940>) == 1, "alignof(NetPacket_Fixed<0x7940>) == 1");

template<>
struct NetPacket_Fixed<0x7941>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7941>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7941>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7941>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7941>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7941>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7941>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7941>) == 30, "sizeof(NetPacket_Fixed<0x7941>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7941>) == 1, "alignof(NetPacket_Fixed<0x7941>) == 1");

template<>
struct NetPacket_Head<0x7942>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    SkewedLength<Little16, 28> magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x7942>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x7942>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x7942>, account_name) == 2, "offsetof(NetPacket_Head<0x7942>, account_name) == 2");
static_assert(offsetof(NetPacket_Head<0x7942>, magic_packet_length) == 26, "offsetof(NetPacket_Head<0x7942>, magic_packet_length) == 26");
static_assert(sizeof(NetPacket_Head<0x7942>) == 28, "sizeof(NetPacket_Head<0x7942>) == 28");
static_assert(alignof(NetPacket_Head<0x7942>) == 1, "alignof(NetPacket_Head<0x7942>) == 1");
template<>
struct NetPacket_Repeat<0x7942>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x7942>, c) == 0, "offsetof(NetPacket_Repeat<0x7942>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x7942>) == 1, "sizeof(NetPacket_Repeat<0x7942>) == 1");
static_assert(alignof(NetPacket_Repeat<0x7942>) == 1, "alignof(NetPacket_Repeat<0x7942>) == 1");

template<>
struct NetPacket_Fixed<0x7943>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7943>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7943>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7943>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7943>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7943>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7943>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7943>) == 30, "sizeof(NetPacket_Fixed<0x7943>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7943>) == 1, "alignof(NetPacket_Fixed<0x7943>) == 1");

template<>
struct NetPacket_Fixed<0x7944>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7944>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7944>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7944>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7944>, account_name) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7944>) == 26, "sizeof(NetPacket_Fixed<0x7944>) == 26");
static_assert(alignof(NetPacket_Fixed<0x7944>) == 1, "alignof(NetPacket_Fixed<0x7944>) == 1");

template<>
struct NetPacket_Fixed<0x7945>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7945>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7945>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7945>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7945>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7945>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7945>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7945>) == 30, "sizeof(NetPacket_Fixed<0x7945>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7945>) == 1, "alignof(NetPacket_Fixed<0x7945>) == 1");

template<>
struct NetPacket_Fixed<0x7946>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7946>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7946>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7946>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7946>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7946>) == 6, "sizeof(NetPacket_Fixed<0x7946>) == 6");
static_assert(alignof(NetPacket_Fixed<0x7946>) == 1, "alignof(NetPacket_Fixed<0x7946>) == 1");

template<>
struct NetPacket_Fixed<0x7947>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7947>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7947>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7947>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7947>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7947>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7947>, account_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x7947>) == 30, "sizeof(NetPacket_Fixed<0x7947>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7947>) == 1, "alignof(NetPacket_Fixed<0x7947>) == 1");

template<>
struct NetPacket_Fixed<0x7948>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 valid_until;
};
static_assert(offsetof(NetPacket_Fixed<0x7948>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7948>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7948>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7948>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7948>, valid_until) == 26, "offsetof(NetPacket_Fixed<0x7948>, valid_until) == 26");
static_assert(sizeof(NetPacket_Fixed<0x7948>) == 30, "sizeof(NetPacket_Fixed<0x7948>) == 30");
static_assert(alignof(NetPacket_Fixed<0x7948>) == 1, "alignof(NetPacket_Fixed<0x7948>) == 1");

template<>
struct NetPacket_Fixed<0x7949>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 valid_until;
};
static_assert(offsetof(NetPacket_Fixed<0x7949>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7949>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7949>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7949>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7949>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7949>, account_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x7949>, valid_until) == 30, "offsetof(NetPacket_Fixed<0x7949>, valid_until) == 30");
static_assert(sizeof(NetPacket_Fixed<0x7949>) == 34, "sizeof(NetPacket_Fixed<0x7949>) == 34");
static_assert(alignof(NetPacket_Fixed<0x7949>) == 1, "alignof(NetPacket_Fixed<0x7949>) == 1");

template<>
struct NetPacket_Fixed<0x794a>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 ban_until;
};
static_assert(offsetof(NetPacket_Fixed<0x794a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x794a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x794a>, account_name) == 2, "offsetof(NetPacket_Fixed<0x794a>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x794a>, ban_until) == 26, "offsetof(NetPacket_Fixed<0x794a>, ban_until) == 26");
static_assert(sizeof(NetPacket_Fixed<0x794a>) == 30, "sizeof(NetPacket_Fixed<0x794a>) == 30");
static_assert(alignof(NetPacket_Fixed<0x794a>) == 1, "alignof(NetPacket_Fixed<0x794a>) == 1");

template<>
struct NetPacket_Fixed<0x794b>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 ban_until;
};
static_assert(offsetof(NetPacket_Fixed<0x794b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x794b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x794b>, account_id) == 2, "offsetof(NetPacket_Fixed<0x794b>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x794b>, account_name) == 6, "offsetof(NetPacket_Fixed<0x794b>, account_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x794b>, ban_until) == 30, "offsetof(NetPacket_Fixed<0x794b>, ban_until) == 30");
static_assert(sizeof(NetPacket_Fixed<0x794b>) == 34, "sizeof(NetPacket_Fixed<0x794b>) == 34");
static_assert(alignof(NetPacket_Fixed<0x794b>) == 1, "alignof(NetPacket_Fixed<0x794b>) == 1");

template<>
struct NetPacket_Fixed<0x794c>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetHumanTimeDiff ban_add;
};
static_assert(offsetof(NetPacket_Fixed<0x794c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x794c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x794c>, account_name) == 2, "offsetof(NetPacket_Fixed<0x794c>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x794c>, ban_add) == 26, "offsetof(NetPacket_Fixed<0x794c>, ban_add) == 26");
static_assert(sizeof(NetPacket_Fixed<0x794c>) == 38, "sizeof(NetPacket_Fixed<0x794c>) == 38");
static_assert(alignof(NetPacket_Fixed<0x794c>) == 1, "alignof(NetPacket_Fixed<0x794c>) == 1");

template<>
struct NetPacket_Fixed<0x794d>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 ban_until;
};
static_assert(offsetof(NetPacket_Fixed<0x794d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x794d>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x794d>, account_id) == 2, "offsetof(NetPacket_Fixed<0x794d>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x794d>, account_name) == 6, "offsetof(NetPacket_Fixed<0x794d>, account_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x794d>, ban_until) == 30, "offsetof(NetPacket_Fixed<0x794d>, ban_until) == 30");
static_assert(sizeof(NetPacket_Fixed<0x794d>) == 34, "sizeof(NetPacket_Fixed<0x794d>) == 34");
static_assert(alignof(NetPacket_Fixed<0x794d>) == 1, "alignof(NetPacket_Fixed<0x794d>) == 1");

template<>
struct NetPacket_Head<0x794e>
{
    Little16 magic_packet_id;
    Little16 unused;
    SkewedLength<Little32, 8> magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x794e>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x794e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x794e>, unused) == 2, "offsetof(NetPacket_Head<0x794e>, unused) == 2");
static_assert(offsetof(NetPacket_Head<0x794e>, magic_packet_length) == 4, "offsetof(NetPacket_Head<0x794e>, magic_packet_length) == 4");
static_assert(sizeof(NetPacket_Head<0x794e>) == 8, "sizeof(NetPacket_Head<0x794e>) == 8");
static_assert(alignof(NetPacket_Head<0x794e>) == 1, "alignof(NetPacket_Head<0x794e>) == 1");
template<>
struct NetPacket_Repeat<0x794e>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x794e>, c) == 0, "offsetof(NetPacket_Repeat<0x794e>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x794e>) == 1, "sizeof(NetPacket_Repeat<0x794e>) == 1");
static_assert(alignof(NetPacket_Repeat<0x794e>) == 1, "alignof(NetPacket_Repeat<0x794e>) == 1");

template<>
struct NetPacket_Fixed<0x794f>
{
    Little16 magic_packet_id;
    Little16 error;
};
static_assert(offsetof(NetPacket_Fixed<0x794f>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x794f>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x794f>, error) == 2, "offsetof(NetPacket_Fixed<0x794f>, error) == 2");
static_assert(sizeof(NetPacket_Fixed<0x794f>) == 4, "sizeof(NetPacket_Fixed<0x794f>) == 4");
static_assert(alignof(NetPacket_Fixed<0x794f>) == 1, "alignof(NetPacket_Fixed<0x794f>) == 1");

template<>
struct NetPacket_Fixed<0x7950>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetHumanTimeDiff valid_add;
};
static_assert(offsetof(NetPacket_Fixed<0x7950>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7950>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7950>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7950>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7950>, valid_add) == 26, "offsetof(NetPacket_Fixed<0x7950>, valid_add) == 26");
static_assert(sizeof(NetPacket_Fixed<0x7950>) == 38, "sizeof(NetPacket_Fixed<0x7950>) == 38");
static_assert(alignof(NetPacket_Fixed<0x7950>) == 1, "alignof(NetPacket_Fixed<0x7950>) == 1");

template<>
struct NetPacket_Fixed<0x7951>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 valid_until;
};
static_assert(offsetof(NetPacket_Fixed<0x7951>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7951>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7951>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7951>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x7951>, account_name) == 6, "offsetof(NetPacket_Fixed<0x7951>, account_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x7951>, valid_until) == 30, "offsetof(NetPacket_Fixed<0x7951>, valid_until) == 30");
static_assert(sizeof(NetPacket_Fixed<0x7951>) == 34, "sizeof(NetPacket_Fixed<0x7951>) == 34");
static_assert(alignof(NetPacket_Fixed<0x7951>) == 1, "alignof(NetPacket_Fixed<0x7951>) == 1");

template<>
struct NetPacket_Fixed<0x7952>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetPacket_Fixed<0x7952>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7952>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7952>, account_name) == 2, "offsetof(NetPacket_Fixed<0x7952>, account_name) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7952>) == 26, "sizeof(NetPacket_Fixed<0x7952>) == 26");
static_assert(alignof(NetPacket_Fixed<0x7952>) == 1, "alignof(NetPacket_Fixed<0x7952>) == 1");

template<>
struct NetPacket_Head<0x7953>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte gm_level;
    NetString<sizeof(AccountName)> account_name;
    Byte sex;
    Little32 login_count;
    Little32 state;
    NetString<sizeof(timestamp_seconds_buffer)> error_message;
    NetString<sizeof(timestamp_milliseconds_buffer)> last_login_string;
    NetString<sizeof(VString<15>)> ip_string;
    NetString<sizeof(AccountEmail)> email;
    Little32 connect_until;
    Little32 ban_until;
    SkewedLength<Little16, 150> magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x7953>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x7953>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x7953>, account_id) == 2, "offsetof(NetPacket_Head<0x7953>, account_id) == 2");
static_assert(offsetof(NetPacket_Head<0x7953>, gm_level) == 6, "offsetof(NetPacket_Head<0x7953>, gm_level) == 6");
static_assert(offsetof(NetPacket_Head<0x7953>, account_name) == 7, "offsetof(NetPacket_Head<0x7953>, account_name) == 7");
static_assert(offsetof(NetPacket_Head<0x7953>, sex) == 31, "offsetof(NetPacket_Head<0x7953>, sex) == 31");
static_assert(offsetof(NetPacket_Head<0x7953>, login_count) == 32, "offsetof(NetPacket_Head<0x7953>, login_count) == 32");
static_assert(offsetof(NetPacket_Head<0x7953>, state) == 36, "offsetof(NetPacket_Head<0x7953>, state) == 36");
static_assert(offsetof(NetPacket_Head<0x7953>, error_message) == 40, "offsetof(NetPacket_Head<0x7953>, error_message) == 40");
static_assert(offsetof(NetPacket_Head<0x7953>, last_login_string) == 60, "offsetof(NetPacket_Head<0x7953>, last_login_string) == 60");
static_assert(offsetof(NetPacket_Head<0x7953>, ip_string) == 84, "offsetof(NetPacket_Head<0x7953>, ip_string) == 84");
static_assert(offsetof(NetPacket_Head<0x7953>, email) == 100, "offsetof(NetPacket_Head<0x7953>, email) == 100");
static_assert(offsetof(NetPacket_Head<0x7953>, connect_until) == 140, "offsetof(NetPacket_Head<0x7953>, connect_until) == 140");
static_assert(offsetof(NetPacket_Head<0x7953>, ban_until) == 144, "offsetof(NetPacket_Head<0x7953>, ban_until) == 144");
static_assert(offsetof(NetPacket_Head<0x7953>, magic_packet_length) == 148, "offsetof(NetPacket_Head<0x7953>, magic_packet_length) == 148");
static_assert(sizeof(NetPacket_Head<0x7953>) == 150, "sizeof(NetPacket_Head<0x7953>) == 150");
static_assert(alignof(NetPacket_Head<0x7953>) == 1, "alignof(NetPacket_Head<0x7953>) == 1");
template<>
struct NetPacket_Repeat<0x7953>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x7953>, c) == 0, "offsetof(NetPacket_Repeat<0x7953>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x7953>) == 1, "sizeof(NetPacket_Repeat<0x7953>) == 1");
static_assert(alignof(NetPacket_Repeat<0x7953>) == 1, "alignof(NetPacket_Repeat<0x7953>) == 1");

template<>
struct NetPacket_Fixed<0x7954>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7954>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7954>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x7954>, account_id) == 2, "offsetof(NetPacket_Fixed<0x7954>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x7954>) == 6, "sizeof(NetPacket_Fixed<0x7954>) == 6");
static_assert(alignof(NetPacket_Fixed<0x7954>) == 1, "alignof(NetPacket_Fixed<0x7954>) == 1");

template<>
struct NetPacket_Fixed<0x7955>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x7955>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x7955>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x7955>) == 2, "sizeof(NetPacket_Fixed<0x7955>) == 2");
static_assert(alignof(NetPacket_Fixed<0x7955>) == 1, "alignof(NetPacket_Fixed<0x7955>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2726> *network, Packet_Head<0x2726> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2726> *native, NetPacket_Head<0x2726> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2726> *network, Packet_Repeat<0x2726> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2726> *native, NetPacket_Repeat<0x2726> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7918> *network, Packet_Fixed<0x7918> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->encryption_zero, native.encryption_zero);
    rv &= native_to_network(&network->account_pass, native.account_pass);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7918> *native, NetPacket_Fixed<0x7918> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->encryption_zero, network.encryption_zero);
    rv &= network_to_native(&native->account_pass, network.account_pass);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7919> *network, Packet_Fixed<0x7919> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->error, native.error);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7919> *native, NetPacket_Fixed<0x7919> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->error, network.error);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7920> *network, Packet_Fixed<0x7920> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->start_account_id, native.start_account_id);
    rv &= native_to_network(&network->end_account_id, native.end_account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7920> *native, NetPacket_Fixed<0x7920> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->start_account_id, network.start_account_id);
    rv &= network_to_native(&native->end_account_id, network.end_account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x7921> *network, Packet_Head<0x7921> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x7921> *native, NetPacket_Head<0x7921> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x7921> *network, Packet_Repeat<0x7921> native)
{
    bool rv = true;
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->login_count, native.login_count);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x7921> *native, NetPacket_Repeat<0x7921> network)
{
    bool rv = true;
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->login_count, network.login_count);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7924> *network, Packet_Fixed<0x7924> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->source_item_id, native.source_item_id);
    rv &= native_to_network(&network->dest_item_id, native.dest_item_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7924> *native, NetPacket_Fixed<0x7924> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->source_item_id, network.source_item_id);
    rv &= network_to_native(&native->dest_item_id, network.dest_item_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7925> *network, Packet_Fixed<0x7925> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7925> *native, NetPacket_Fixed<0x7925> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7930> *network, Packet_Fixed<0x7930> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->password, native.password);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->email, native.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7930> *native, NetPacket_Fixed<0x7930> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->password, network.password);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->email, network.email);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7931> *network, Packet_Fixed<0x7931> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7931> *native, NetPacket_Fixed<0x7931> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7932> *network, Packet_Fixed<0x7932> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7932> *native, NetPacket_Fixed<0x7932> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7933> *network, Packet_Fixed<0x7933> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7933> *native, NetPacket_Fixed<0x7933> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7934> *network, Packet_Fixed<0x7934> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->password, native.password);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7934> *native, NetPacket_Fixed<0x7934> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->password, network.password);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7935> *network, Packet_Fixed<0x7935> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7935> *native, NetPacket_Fixed<0x7935> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7936> *network, Packet_Fixed<0x7936> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->status, native.status);
    rv &= native_to_network(&network->error_message, native.error_message);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7936> *native, NetPacket_Fixed<0x7936> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->status, network.status);
    rv &= network_to_native(&native->error_message, network.error_message);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7937> *network, Packet_Fixed<0x7937> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7937> *native, NetPacket_Fixed<0x7937> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7938> *network, Packet_Fixed<0x7938> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7938> *native, NetPacket_Fixed<0x7938> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x7939> *network, Packet_Head<0x7939> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x7939> *native, NetPacket_Head<0x7939> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x7939> *network, Packet_Repeat<0x7939> native)
{
    bool rv = true;
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->users, native.users);
    rv &= native_to_network(&network->maintenance, native.maintenance);
    rv &= native_to_network(&network->is_new, native.is_new);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x7939> *native, NetPacket_Repeat<0x7939> network)
{
    bool rv = true;
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->users, network.users);
    rv &= network_to_native(&native->maintenance, network.maintenance);
    rv &= network_to_native(&native->is_new, network.is_new);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x793a> *network, Packet_Fixed<0x793a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->password, native.password);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x793a> *native, NetPacket_Fixed<0x793a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->password, network.password);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x793b> *network, Packet_Fixed<0x793b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x793b> *native, NetPacket_Fixed<0x793b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x793c> *network, Packet_Fixed<0x793c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x793c> *native, NetPacket_Fixed<0x793c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x793d> *network, Packet_Fixed<0x793d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x793d> *native, NetPacket_Fixed<0x793d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x793e> *network, Packet_Fixed<0x793e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x793e> *native, NetPacket_Fixed<0x793e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x793f> *network, Packet_Fixed<0x793f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x793f> *native, NetPacket_Fixed<0x793f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7940> *network, Packet_Fixed<0x7940> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->email, native.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7940> *native, NetPacket_Fixed<0x7940> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->email, network.email);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7941> *network, Packet_Fixed<0x7941> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7941> *native, NetPacket_Fixed<0x7941> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x7942> *network, Packet_Head<0x7942> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x7942> *native, NetPacket_Head<0x7942> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x7942> *network, Packet_Repeat<0x7942> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x7942> *native, NetPacket_Repeat<0x7942> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7943> *network, Packet_Fixed<0x7943> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7943> *native, NetPacket_Fixed<0x7943> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7944> *network, Packet_Fixed<0x7944> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7944> *native, NetPacket_Fixed<0x7944> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7945> *network, Packet_Fixed<0x7945> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7945> *native, NetPacket_Fixed<0x7945> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7946> *network, Packet_Fixed<0x7946> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7946> *native, NetPacket_Fixed<0x7946> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7947> *network, Packet_Fixed<0x7947> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7947> *native, NetPacket_Fixed<0x7947> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7948> *network, Packet_Fixed<0x7948> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_until, native.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7948> *native, NetPacket_Fixed<0x7948> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_until, network.valid_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7949> *network, Packet_Fixed<0x7949> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_until, native.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7949> *native, NetPacket_Fixed<0x7949> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_until, network.valid_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x794a> *network, Packet_Fixed<0x794a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x794a> *native, NetPacket_Fixed<0x794a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x794b> *network, Packet_Fixed<0x794b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x794b> *native, NetPacket_Fixed<0x794b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x794c> *network, Packet_Fixed<0x794c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_add, native.ban_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x794c> *native, NetPacket_Fixed<0x794c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_add, network.ban_add);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x794d> *network, Packet_Fixed<0x794d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x794d> *native, NetPacket_Fixed<0x794d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x794e> *network, Packet_Head<0x794e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x794e> *native, NetPacket_Head<0x794e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x794e> *network, Packet_Repeat<0x794e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x794e> *native, NetPacket_Repeat<0x794e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x794f> *network, Packet_Fixed<0x794f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->error, native.error);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x794f> *native, NetPacket_Fixed<0x794f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->error, network.error);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7950> *network, Packet_Fixed<0x7950> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_add, native.valid_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7950> *native, NetPacket_Fixed<0x7950> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_add, network.valid_add);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7951> *network, Packet_Fixed<0x7951> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_until, native.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7951> *native, NetPacket_Fixed<0x7951> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_until, network.valid_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7952> *network, Packet_Fixed<0x7952> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7952> *native, NetPacket_Fixed<0x7952> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x7953> *network, Packet_Head<0x7953> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->login_count, native.login_count);
    rv &= native_to_network(&network->state, native.state);
    rv &= native_to_network(&network->error_message, native.error_message);
    rv &= native_to_network(&network->last_login_string, native.last_login_string);
    rv &= native_to_network(&network->ip_string, native.ip_string);
    rv &= native_to_network(&network->email, native.email);
    rv &= native_to_network(&network->connect_until, native.connect_until);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x7953> *native, NetPacket_Head<0x7953> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->login_count, network.login_count);
    rv &= network_to_native(&native->state, network.state);
    rv &= network_to_native(&native->error_message, network.error_message);
    rv &= network_to_native(&native->last_login_string, network.last_login_string);
    rv &= network_to_native(&native->ip_string, network.ip_string);
    rv &= network_to_native(&native->email, network.email);
    rv &= network_to_native(&native->connect_until, network.connect_until);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x7953> *network, Packet_Repeat<0x7953> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x7953> *native, NetPacket_Repeat<0x7953> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7954> *network, Packet_Fixed<0x7954> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7954> *native, NetPacket_Fixed<0x7954> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x7955> *network, Packet_Fixed<0x7955> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x7955> *native, NetPacket_Fixed<0x7955> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}


# pragma pack(pop)

#endif // TMWA_PROTO2_LOGIN_ADMIN_HPP
