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

struct SPacket_0x2726_Head
{
    using NetType = NetSPacket_0x2726_Head;
    static const uint16_t PACKET_ID = 0x2726;

    uint16_t magic_packet_id = PACKET_ID;
    uint16_t unused = {};
    uint32_t magic_packet_length = {};
};
struct SPacket_0x2726_Repeat
{
    using NetType = NetSPacket_0x2726_Repeat;
    static const uint16_t PACKET_ID = 0x2726;

    uint8_t c = {};
};
struct RPacket_0x7920_Fixed
{
    using NetType = NetRPacket_0x7920_Fixed;
    static const uint16_t PACKET_ID = 0x7920;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId start_account_id = {};
    AccountId end_account_id = {};
};
struct SPacket_0x7921_Head
{
    using NetType = NetSPacket_0x7921_Head;
    static const uint16_t PACKET_ID = 0x7921;

    uint16_t magic_packet_id = PACKET_ID;
    uint16_t magic_packet_length = {};
};
struct SPacket_0x7921_Repeat
{
    using NetType = NetSPacket_0x7921_Repeat;
    static const uint16_t PACKET_ID = 0x7921;

    AccountId account_id = {};
    GmLevel gm_level = {};
    AccountName account_name = {};
    SEX sex = {};
    uint32_t login_count = {};
    uint32_t status = {};
};
struct RPacket_0x7924_Fixed
{
    using NetType = NetRPacket_0x7924_Fixed;
    static const uint16_t PACKET_ID = 0x7924;

    uint16_t magic_packet_id = PACKET_ID;
    ItemNameId source_item_id = {};
    ItemNameId dest_item_id = {};
};
struct SPacket_0x7925_Fixed
{
    using NetType = NetSPacket_0x7925_Fixed;
    static const uint16_t PACKET_ID = 0x7925;

    uint16_t magic_packet_id = PACKET_ID;
};
struct RPacket_0x7930_Fixed
{
    using NetType = NetRPacket_0x7930_Fixed;
    static const uint16_t PACKET_ID = 0x7930;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass password = {};
    SEX sex = {};
    AccountEmail email = {};
};
struct SPacket_0x7931_Fixed
{
    using NetType = NetSPacket_0x7931_Fixed;
    static const uint16_t PACKET_ID = 0x7931;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7932_Fixed
{
    using NetType = NetRPacket_0x7932_Fixed;
    static const uint16_t PACKET_ID = 0x7932;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
};
struct SPacket_0x7933_Fixed
{
    using NetType = NetSPacket_0x7933_Fixed;
    static const uint16_t PACKET_ID = 0x7933;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7934_Fixed
{
    using NetType = NetRPacket_0x7934_Fixed;
    static const uint16_t PACKET_ID = 0x7934;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass password = {};
};
struct SPacket_0x7935_Fixed
{
    using NetType = NetSPacket_0x7935_Fixed;
    static const uint16_t PACKET_ID = 0x7935;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7936_Fixed
{
    using NetType = NetRPacket_0x7936_Fixed;
    static const uint16_t PACKET_ID = 0x7936;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    uint32_t status = {};
    timestamp_seconds_buffer error_message = {};
};
struct SPacket_0x7937_Fixed
{
    using NetType = NetSPacket_0x7937_Fixed;
    static const uint16_t PACKET_ID = 0x7937;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    uint32_t status = {};
};
struct RPacket_0x7938_Fixed
{
    using NetType = NetRPacket_0x7938_Fixed;
    static const uint16_t PACKET_ID = 0x7938;

    uint16_t magic_packet_id = PACKET_ID;
};
struct SPacket_0x7939_Head
{
    using NetType = NetSPacket_0x7939_Head;
    static const uint16_t PACKET_ID = 0x7939;

    uint16_t magic_packet_id = PACKET_ID;
    uint16_t magic_packet_length = {};
};
struct SPacket_0x7939_Repeat
{
    using NetType = NetSPacket_0x7939_Repeat;
    static const uint16_t PACKET_ID = 0x7939;

    IP4Address ip = {};
    uint16_t port = {};
    ServerName name = {};
    uint16_t users = {};
    uint16_t maintenance = {};
    uint16_t is_new = {};
};
struct RPacket_0x793a_Fixed
{
    using NetType = NetRPacket_0x793a_Fixed;
    static const uint16_t PACKET_ID = 0x793a;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass password = {};
};
struct SPacket_0x793b_Fixed
{
    using NetType = NetSPacket_0x793b_Fixed;
    static const uint16_t PACKET_ID = 0x793b;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x793c_Fixed
{
    using NetType = NetRPacket_0x793c_Fixed;
    static const uint16_t PACKET_ID = 0x793c;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    SEX sex = {};
};
struct SPacket_0x793d_Fixed
{
    using NetType = NetSPacket_0x793d_Fixed;
    static const uint16_t PACKET_ID = 0x793d;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x793e_Fixed
{
    using NetType = NetRPacket_0x793e_Fixed;
    static const uint16_t PACKET_ID = 0x793e;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    GmLevel gm_level = {};
};
struct SPacket_0x793f_Fixed
{
    using NetType = NetSPacket_0x793f_Fixed;
    static const uint16_t PACKET_ID = 0x793f;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7940_Fixed
{
    using NetType = NetRPacket_0x7940_Fixed;
    static const uint16_t PACKET_ID = 0x7940;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountEmail email = {};
};
struct SPacket_0x7941_Fixed
{
    using NetType = NetSPacket_0x7941_Fixed;
    static const uint16_t PACKET_ID = 0x7941;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7942_Head
{
    using NetType = NetRPacket_0x7942_Head;
    static const uint16_t PACKET_ID = 0x7942;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    uint16_t magic_packet_length = {};
};
struct RPacket_0x7942_Repeat
{
    using NetType = NetRPacket_0x7942_Repeat;
    static const uint16_t PACKET_ID = 0x7942;

    uint8_t c = {};
};
struct SPacket_0x7943_Fixed
{
    using NetType = NetSPacket_0x7943_Fixed;
    static const uint16_t PACKET_ID = 0x7943;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7944_Fixed
{
    using NetType = NetRPacket_0x7944_Fixed;
    static const uint16_t PACKET_ID = 0x7944;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
};
struct SPacket_0x7945_Fixed
{
    using NetType = NetSPacket_0x7945_Fixed;
    static const uint16_t PACKET_ID = 0x7945;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7946_Fixed
{
    using NetType = NetRPacket_0x7946_Fixed;
    static const uint16_t PACKET_ID = 0x7946;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};
struct SPacket_0x7947_Fixed
{
    using NetType = NetSPacket_0x7947_Fixed;
    static const uint16_t PACKET_ID = 0x7947;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
};
struct RPacket_0x7948_Fixed
{
    using NetType = NetRPacket_0x7948_Fixed;
    static const uint16_t PACKET_ID = 0x7948;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    TimeT valid_until = {};
};
struct SPacket_0x7949_Fixed
{
    using NetType = NetSPacket_0x7949_Fixed;
    static const uint16_t PACKET_ID = 0x7949;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT valid_until = {};
};
struct RPacket_0x794a_Fixed
{
    using NetType = NetRPacket_0x794a_Fixed;
    static const uint16_t PACKET_ID = 0x794a;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    TimeT ban_until = {};
};
struct SPacket_0x794b_Fixed
{
    using NetType = NetSPacket_0x794b_Fixed;
    static const uint16_t PACKET_ID = 0x794b;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT ban_until = {};
};
struct RPacket_0x794c_Fixed
{
    using NetType = NetRPacket_0x794c_Fixed;
    static const uint16_t PACKET_ID = 0x794c;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    HumanTimeDiff ban_add = {};
};
struct SPacket_0x794d_Fixed
{
    using NetType = NetSPacket_0x794d_Fixed;
    static const uint16_t PACKET_ID = 0x794d;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT ban_until = {};
};
struct RPacket_0x794e_Head
{
    using NetType = NetRPacket_0x794e_Head;
    static const uint16_t PACKET_ID = 0x794e;

    uint16_t magic_packet_id = PACKET_ID;
    uint16_t unused = {};
    uint32_t magic_packet_length = {};
};
struct RPacket_0x794e_Repeat
{
    using NetType = NetRPacket_0x794e_Repeat;
    static const uint16_t PACKET_ID = 0x794e;

    uint8_t c = {};
};
struct SPacket_0x794f_Fixed
{
    using NetType = NetSPacket_0x794f_Fixed;
    static const uint16_t PACKET_ID = 0x794f;

    uint16_t magic_packet_id = PACKET_ID;
    uint16_t error = {};
};
struct RPacket_0x7950_Fixed
{
    using NetType = NetRPacket_0x7950_Fixed;
    static const uint16_t PACKET_ID = 0x7950;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    HumanTimeDiff valid_add = {};
};
struct SPacket_0x7951_Fixed
{
    using NetType = NetSPacket_0x7951_Fixed;
    static const uint16_t PACKET_ID = 0x7951;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountName account_name = {};
    TimeT valid_until = {};
};
struct RPacket_0x7952_Fixed
{
    using NetType = NetRPacket_0x7952_Fixed;
    static const uint16_t PACKET_ID = 0x7952;

    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
};
struct SPacket_0x7953_Head
{
    using NetType = NetSPacket_0x7953_Head;
    static const uint16_t PACKET_ID = 0x7953;

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
    uint16_t magic_packet_length = {};
};
struct SPacket_0x7953_Repeat
{
    using NetType = NetSPacket_0x7953_Repeat;
    static const uint16_t PACKET_ID = 0x7953;

    uint8_t c = {};
};
struct RPacket_0x7954_Fixed
{
    using NetType = NetRPacket_0x7954_Fixed;
    static const uint16_t PACKET_ID = 0x7954;

    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};
struct RPacket_0x7955_Fixed
{
    using NetType = NetRPacket_0x7955_Fixed;
    static const uint16_t PACKET_ID = 0x7955;

    uint16_t magic_packet_id = PACKET_ID;
};

struct NetSPacket_0x2726_Head
{
    Little16 magic_packet_id;
    Little16 unused;
    SkewedLength<Little32, 8> magic_packet_length;
};
static_assert(offsetof(NetSPacket_0x2726_Head, magic_packet_id) == 0, "offsetof(NetSPacket_0x2726_Head, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x2726_Head, unused) == 2, "offsetof(NetSPacket_0x2726_Head, unused) == 2");
static_assert(offsetof(NetSPacket_0x2726_Head, magic_packet_length) == 4, "offsetof(NetSPacket_0x2726_Head, magic_packet_length) == 4");
static_assert(sizeof(NetSPacket_0x2726_Head) == 8, "sizeof(NetSPacket_0x2726_Head) == 8");
struct NetSPacket_0x2726_Repeat
{
    Byte c;
};
static_assert(offsetof(NetSPacket_0x2726_Repeat, c) == 0, "offsetof(NetSPacket_0x2726_Repeat, c) == 0");
static_assert(sizeof(NetSPacket_0x2726_Repeat) == 1, "sizeof(NetSPacket_0x2726_Repeat) == 1");
struct NetRPacket_0x7920_Fixed
{
    Little16 magic_packet_id;
    Little32 start_account_id;
    Little32 end_account_id;
};
static_assert(offsetof(NetRPacket_0x7920_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7920_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7920_Fixed, start_account_id) == 2, "offsetof(NetRPacket_0x7920_Fixed, start_account_id) == 2");
static_assert(offsetof(NetRPacket_0x7920_Fixed, end_account_id) == 6, "offsetof(NetRPacket_0x7920_Fixed, end_account_id) == 6");
static_assert(sizeof(NetRPacket_0x7920_Fixed) == 10, "sizeof(NetRPacket_0x7920_Fixed) == 10");
struct NetSPacket_0x7921_Head
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetSPacket_0x7921_Head, magic_packet_id) == 0, "offsetof(NetSPacket_0x7921_Head, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7921_Head, magic_packet_length) == 2, "offsetof(NetSPacket_0x7921_Head, magic_packet_length) == 2");
static_assert(sizeof(NetSPacket_0x7921_Head) == 4, "sizeof(NetSPacket_0x7921_Head) == 4");
struct NetSPacket_0x7921_Repeat
{
    Little32 account_id;
    Byte gm_level;
    NetString<sizeof(AccountName)> account_name;
    Byte sex;
    Little32 login_count;
    Little32 status;
};
static_assert(offsetof(NetSPacket_0x7921_Repeat, account_id) == 0, "offsetof(NetSPacket_0x7921_Repeat, account_id) == 0");
static_assert(offsetof(NetSPacket_0x7921_Repeat, gm_level) == 4, "offsetof(NetSPacket_0x7921_Repeat, gm_level) == 4");
static_assert(offsetof(NetSPacket_0x7921_Repeat, account_name) == 5, "offsetof(NetSPacket_0x7921_Repeat, account_name) == 5");
static_assert(offsetof(NetSPacket_0x7921_Repeat, sex) == 29, "offsetof(NetSPacket_0x7921_Repeat, sex) == 29");
static_assert(offsetof(NetSPacket_0x7921_Repeat, login_count) == 30, "offsetof(NetSPacket_0x7921_Repeat, login_count) == 30");
static_assert(offsetof(NetSPacket_0x7921_Repeat, status) == 34, "offsetof(NetSPacket_0x7921_Repeat, status) == 34");
static_assert(sizeof(NetSPacket_0x7921_Repeat) == 38, "sizeof(NetSPacket_0x7921_Repeat) == 38");
struct NetRPacket_0x7924_Fixed
{
    Little16 magic_packet_id;
    Little32 source_item_id;
    Little32 dest_item_id;
};
static_assert(offsetof(NetRPacket_0x7924_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7924_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7924_Fixed, source_item_id) == 2, "offsetof(NetRPacket_0x7924_Fixed, source_item_id) == 2");
static_assert(offsetof(NetRPacket_0x7924_Fixed, dest_item_id) == 6, "offsetof(NetRPacket_0x7924_Fixed, dest_item_id) == 6");
static_assert(sizeof(NetRPacket_0x7924_Fixed) == 10, "sizeof(NetRPacket_0x7924_Fixed) == 10");
struct NetSPacket_0x7925_Fixed
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetSPacket_0x7925_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7925_Fixed, magic_packet_id) == 0");
static_assert(sizeof(NetSPacket_0x7925_Fixed) == 2, "sizeof(NetSPacket_0x7925_Fixed) == 2");
struct NetRPacket_0x7930_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> password;
    char sex;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetRPacket_0x7930_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7930_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7930_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7930_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7930_Fixed, password) == 26, "offsetof(NetRPacket_0x7930_Fixed, password) == 26");
static_assert(offsetof(NetRPacket_0x7930_Fixed, sex) == 50, "offsetof(NetRPacket_0x7930_Fixed, sex) == 50");
static_assert(offsetof(NetRPacket_0x7930_Fixed, email) == 51, "offsetof(NetRPacket_0x7930_Fixed, email) == 51");
static_assert(sizeof(NetRPacket_0x7930_Fixed) == 91, "sizeof(NetRPacket_0x7930_Fixed) == 91");
struct NetSPacket_0x7931_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7931_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7931_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7931_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7931_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7931_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7931_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7931_Fixed) == 30, "sizeof(NetSPacket_0x7931_Fixed) == 30");
struct NetRPacket_0x7932_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetRPacket_0x7932_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7932_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7932_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7932_Fixed, account_name) == 2");
static_assert(sizeof(NetRPacket_0x7932_Fixed) == 26, "sizeof(NetRPacket_0x7932_Fixed) == 26");
struct NetSPacket_0x7933_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7933_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7933_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7933_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7933_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7933_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7933_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7933_Fixed) == 30, "sizeof(NetSPacket_0x7933_Fixed) == 30");
struct NetRPacket_0x7934_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> password;
};
static_assert(offsetof(NetRPacket_0x7934_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7934_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7934_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7934_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7934_Fixed, password) == 26, "offsetof(NetRPacket_0x7934_Fixed, password) == 26");
static_assert(sizeof(NetRPacket_0x7934_Fixed) == 50, "sizeof(NetRPacket_0x7934_Fixed) == 50");
struct NetSPacket_0x7935_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7935_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7935_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7935_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7935_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7935_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7935_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7935_Fixed) == 30, "sizeof(NetSPacket_0x7935_Fixed) == 30");
struct NetRPacket_0x7936_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 status;
    NetString<sizeof(timestamp_seconds_buffer)> error_message;
};
static_assert(offsetof(NetRPacket_0x7936_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7936_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7936_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7936_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7936_Fixed, status) == 26, "offsetof(NetRPacket_0x7936_Fixed, status) == 26");
static_assert(offsetof(NetRPacket_0x7936_Fixed, error_message) == 30, "offsetof(NetRPacket_0x7936_Fixed, error_message) == 30");
static_assert(sizeof(NetRPacket_0x7936_Fixed) == 50, "sizeof(NetRPacket_0x7936_Fixed) == 50");
struct NetSPacket_0x7937_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 status;
};
static_assert(offsetof(NetSPacket_0x7937_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7937_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7937_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7937_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7937_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7937_Fixed, account_name) == 6");
static_assert(offsetof(NetSPacket_0x7937_Fixed, status) == 30, "offsetof(NetSPacket_0x7937_Fixed, status) == 30");
static_assert(sizeof(NetSPacket_0x7937_Fixed) == 34, "sizeof(NetSPacket_0x7937_Fixed) == 34");
struct NetRPacket_0x7938_Fixed
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetRPacket_0x7938_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7938_Fixed, magic_packet_id) == 0");
static_assert(sizeof(NetRPacket_0x7938_Fixed) == 2, "sizeof(NetRPacket_0x7938_Fixed) == 2");
struct NetSPacket_0x7939_Head
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetSPacket_0x7939_Head, magic_packet_id) == 0, "offsetof(NetSPacket_0x7939_Head, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7939_Head, magic_packet_length) == 2, "offsetof(NetSPacket_0x7939_Head, magic_packet_length) == 2");
static_assert(sizeof(NetSPacket_0x7939_Head) == 4, "sizeof(NetSPacket_0x7939_Head) == 4");
struct NetSPacket_0x7939_Repeat
{
    IP4Address ip;
    Little16 port;
    NetString<sizeof(ServerName)> name;
    Little16 users;
    Little16 maintenance;
    Little16 is_new;
};
static_assert(offsetof(NetSPacket_0x7939_Repeat, ip) == 0, "offsetof(NetSPacket_0x7939_Repeat, ip) == 0");
static_assert(offsetof(NetSPacket_0x7939_Repeat, port) == 4, "offsetof(NetSPacket_0x7939_Repeat, port) == 4");
static_assert(offsetof(NetSPacket_0x7939_Repeat, name) == 6, "offsetof(NetSPacket_0x7939_Repeat, name) == 6");
static_assert(offsetof(NetSPacket_0x7939_Repeat, users) == 26, "offsetof(NetSPacket_0x7939_Repeat, users) == 26");
static_assert(offsetof(NetSPacket_0x7939_Repeat, maintenance) == 28, "offsetof(NetSPacket_0x7939_Repeat, maintenance) == 28");
static_assert(offsetof(NetSPacket_0x7939_Repeat, is_new) == 30, "offsetof(NetSPacket_0x7939_Repeat, is_new) == 30");
static_assert(sizeof(NetSPacket_0x7939_Repeat) == 32, "sizeof(NetSPacket_0x7939_Repeat) == 32");
struct NetRPacket_0x793a_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> password;
};
static_assert(offsetof(NetRPacket_0x793a_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x793a_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x793a_Fixed, account_name) == 2, "offsetof(NetRPacket_0x793a_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x793a_Fixed, password) == 26, "offsetof(NetRPacket_0x793a_Fixed, password) == 26");
static_assert(sizeof(NetRPacket_0x793a_Fixed) == 50, "sizeof(NetRPacket_0x793a_Fixed) == 50");
struct NetSPacket_0x793b_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x793b_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x793b_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x793b_Fixed, account_id) == 2, "offsetof(NetSPacket_0x793b_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x793b_Fixed, account_name) == 6, "offsetof(NetSPacket_0x793b_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x793b_Fixed) == 30, "sizeof(NetSPacket_0x793b_Fixed) == 30");
struct NetRPacket_0x793c_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    char sex;
};
static_assert(offsetof(NetRPacket_0x793c_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x793c_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x793c_Fixed, account_name) == 2, "offsetof(NetRPacket_0x793c_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x793c_Fixed, sex) == 26, "offsetof(NetRPacket_0x793c_Fixed, sex) == 26");
static_assert(sizeof(NetRPacket_0x793c_Fixed) == 27, "sizeof(NetRPacket_0x793c_Fixed) == 27");
struct NetSPacket_0x793d_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x793d_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x793d_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x793d_Fixed, account_id) == 2, "offsetof(NetSPacket_0x793d_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x793d_Fixed, account_name) == 6, "offsetof(NetSPacket_0x793d_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x793d_Fixed) == 30, "sizeof(NetSPacket_0x793d_Fixed) == 30");
struct NetRPacket_0x793e_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Byte gm_level;
};
static_assert(offsetof(NetRPacket_0x793e_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x793e_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x793e_Fixed, account_name) == 2, "offsetof(NetRPacket_0x793e_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x793e_Fixed, gm_level) == 26, "offsetof(NetRPacket_0x793e_Fixed, gm_level) == 26");
static_assert(sizeof(NetRPacket_0x793e_Fixed) == 27, "sizeof(NetRPacket_0x793e_Fixed) == 27");
struct NetSPacket_0x793f_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x793f_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x793f_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x793f_Fixed, account_id) == 2, "offsetof(NetSPacket_0x793f_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x793f_Fixed, account_name) == 6, "offsetof(NetSPacket_0x793f_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x793f_Fixed) == 30, "sizeof(NetSPacket_0x793f_Fixed) == 30");
struct NetRPacket_0x7940_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountEmail)> email;
};
static_assert(offsetof(NetRPacket_0x7940_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7940_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7940_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7940_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7940_Fixed, email) == 26, "offsetof(NetRPacket_0x7940_Fixed, email) == 26");
static_assert(sizeof(NetRPacket_0x7940_Fixed) == 66, "sizeof(NetRPacket_0x7940_Fixed) == 66");
struct NetSPacket_0x7941_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7941_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7941_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7941_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7941_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7941_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7941_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7941_Fixed) == 30, "sizeof(NetSPacket_0x7941_Fixed) == 30");
struct NetRPacket_0x7942_Head
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    SkewedLength<Little16, 28> magic_packet_length;
};
static_assert(offsetof(NetRPacket_0x7942_Head, magic_packet_id) == 0, "offsetof(NetRPacket_0x7942_Head, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7942_Head, account_name) == 2, "offsetof(NetRPacket_0x7942_Head, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7942_Head, magic_packet_length) == 26, "offsetof(NetRPacket_0x7942_Head, magic_packet_length) == 26");
static_assert(sizeof(NetRPacket_0x7942_Head) == 28, "sizeof(NetRPacket_0x7942_Head) == 28");
struct NetRPacket_0x7942_Repeat
{
    Byte c;
};
static_assert(offsetof(NetRPacket_0x7942_Repeat, c) == 0, "offsetof(NetRPacket_0x7942_Repeat, c) == 0");
static_assert(sizeof(NetRPacket_0x7942_Repeat) == 1, "sizeof(NetRPacket_0x7942_Repeat) == 1");
struct NetSPacket_0x7943_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7943_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7943_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7943_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7943_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7943_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7943_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7943_Fixed) == 30, "sizeof(NetSPacket_0x7943_Fixed) == 30");
struct NetRPacket_0x7944_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetRPacket_0x7944_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7944_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7944_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7944_Fixed, account_name) == 2");
static_assert(sizeof(NetRPacket_0x7944_Fixed) == 26, "sizeof(NetRPacket_0x7944_Fixed) == 26");
struct NetSPacket_0x7945_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7945_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7945_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7945_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7945_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7945_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7945_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7945_Fixed) == 30, "sizeof(NetSPacket_0x7945_Fixed) == 30");
struct NetRPacket_0x7946_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket_0x7946_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7946_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7946_Fixed, account_id) == 2, "offsetof(NetRPacket_0x7946_Fixed, account_id) == 2");
static_assert(sizeof(NetRPacket_0x7946_Fixed) == 6, "sizeof(NetRPacket_0x7946_Fixed) == 6");
struct NetSPacket_0x7947_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetSPacket_0x7947_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7947_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7947_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7947_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7947_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7947_Fixed, account_name) == 6");
static_assert(sizeof(NetSPacket_0x7947_Fixed) == 30, "sizeof(NetSPacket_0x7947_Fixed) == 30");
struct NetRPacket_0x7948_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 valid_until;
};
static_assert(offsetof(NetRPacket_0x7948_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7948_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7948_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7948_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7948_Fixed, valid_until) == 26, "offsetof(NetRPacket_0x7948_Fixed, valid_until) == 26");
static_assert(sizeof(NetRPacket_0x7948_Fixed) == 30, "sizeof(NetRPacket_0x7948_Fixed) == 30");
struct NetSPacket_0x7949_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 valid_until;
};
static_assert(offsetof(NetSPacket_0x7949_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7949_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7949_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7949_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7949_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7949_Fixed, account_name) == 6");
static_assert(offsetof(NetSPacket_0x7949_Fixed, valid_until) == 30, "offsetof(NetSPacket_0x7949_Fixed, valid_until) == 30");
static_assert(sizeof(NetSPacket_0x7949_Fixed) == 34, "sizeof(NetSPacket_0x7949_Fixed) == 34");
struct NetRPacket_0x794a_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 ban_until;
};
static_assert(offsetof(NetRPacket_0x794a_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x794a_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x794a_Fixed, account_name) == 2, "offsetof(NetRPacket_0x794a_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x794a_Fixed, ban_until) == 26, "offsetof(NetRPacket_0x794a_Fixed, ban_until) == 26");
static_assert(sizeof(NetRPacket_0x794a_Fixed) == 30, "sizeof(NetRPacket_0x794a_Fixed) == 30");
struct NetSPacket_0x794b_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 ban_until;
};
static_assert(offsetof(NetSPacket_0x794b_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x794b_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x794b_Fixed, account_id) == 2, "offsetof(NetSPacket_0x794b_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x794b_Fixed, account_name) == 6, "offsetof(NetSPacket_0x794b_Fixed, account_name) == 6");
static_assert(offsetof(NetSPacket_0x794b_Fixed, ban_until) == 30, "offsetof(NetSPacket_0x794b_Fixed, ban_until) == 30");
static_assert(sizeof(NetSPacket_0x794b_Fixed) == 34, "sizeof(NetSPacket_0x794b_Fixed) == 34");
struct NetRPacket_0x794c_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetHumanTimeDiff ban_add;
};
static_assert(offsetof(NetRPacket_0x794c_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x794c_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x794c_Fixed, account_name) == 2, "offsetof(NetRPacket_0x794c_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x794c_Fixed, ban_add) == 26, "offsetof(NetRPacket_0x794c_Fixed, ban_add) == 26");
static_assert(sizeof(NetRPacket_0x794c_Fixed) == 38, "sizeof(NetRPacket_0x794c_Fixed) == 38");
struct NetSPacket_0x794d_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 ban_until;
};
static_assert(offsetof(NetSPacket_0x794d_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x794d_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x794d_Fixed, account_id) == 2, "offsetof(NetSPacket_0x794d_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x794d_Fixed, account_name) == 6, "offsetof(NetSPacket_0x794d_Fixed, account_name) == 6");
static_assert(offsetof(NetSPacket_0x794d_Fixed, ban_until) == 30, "offsetof(NetSPacket_0x794d_Fixed, ban_until) == 30");
static_assert(sizeof(NetSPacket_0x794d_Fixed) == 34, "sizeof(NetSPacket_0x794d_Fixed) == 34");
struct NetRPacket_0x794e_Head
{
    Little16 magic_packet_id;
    Little16 unused;
    SkewedLength<Little32, 8> magic_packet_length;
};
static_assert(offsetof(NetRPacket_0x794e_Head, magic_packet_id) == 0, "offsetof(NetRPacket_0x794e_Head, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x794e_Head, unused) == 2, "offsetof(NetRPacket_0x794e_Head, unused) == 2");
static_assert(offsetof(NetRPacket_0x794e_Head, magic_packet_length) == 4, "offsetof(NetRPacket_0x794e_Head, magic_packet_length) == 4");
static_assert(sizeof(NetRPacket_0x794e_Head) == 8, "sizeof(NetRPacket_0x794e_Head) == 8");
struct NetRPacket_0x794e_Repeat
{
    Byte c;
};
static_assert(offsetof(NetRPacket_0x794e_Repeat, c) == 0, "offsetof(NetRPacket_0x794e_Repeat, c) == 0");
static_assert(sizeof(NetRPacket_0x794e_Repeat) == 1, "sizeof(NetRPacket_0x794e_Repeat) == 1");
struct NetSPacket_0x794f_Fixed
{
    Little16 magic_packet_id;
    Little16 error;
};
static_assert(offsetof(NetSPacket_0x794f_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x794f_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x794f_Fixed, error) == 2, "offsetof(NetSPacket_0x794f_Fixed, error) == 2");
static_assert(sizeof(NetSPacket_0x794f_Fixed) == 4, "sizeof(NetSPacket_0x794f_Fixed) == 4");
struct NetRPacket_0x7950_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetHumanTimeDiff valid_add;
};
static_assert(offsetof(NetRPacket_0x7950_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7950_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7950_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7950_Fixed, account_name) == 2");
static_assert(offsetof(NetRPacket_0x7950_Fixed, valid_add) == 26, "offsetof(NetRPacket_0x7950_Fixed, valid_add) == 26");
static_assert(sizeof(NetRPacket_0x7950_Fixed) == 38, "sizeof(NetRPacket_0x7950_Fixed) == 38");
struct NetSPacket_0x7951_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountName)> account_name;
    Little32 valid_until;
};
static_assert(offsetof(NetSPacket_0x7951_Fixed, magic_packet_id) == 0, "offsetof(NetSPacket_0x7951_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7951_Fixed, account_id) == 2, "offsetof(NetSPacket_0x7951_Fixed, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7951_Fixed, account_name) == 6, "offsetof(NetSPacket_0x7951_Fixed, account_name) == 6");
static_assert(offsetof(NetSPacket_0x7951_Fixed, valid_until) == 30, "offsetof(NetSPacket_0x7951_Fixed, valid_until) == 30");
static_assert(sizeof(NetSPacket_0x7951_Fixed) == 34, "sizeof(NetSPacket_0x7951_Fixed) == 34");
struct NetRPacket_0x7952_Fixed
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
};
static_assert(offsetof(NetRPacket_0x7952_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7952_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7952_Fixed, account_name) == 2, "offsetof(NetRPacket_0x7952_Fixed, account_name) == 2");
static_assert(sizeof(NetRPacket_0x7952_Fixed) == 26, "sizeof(NetRPacket_0x7952_Fixed) == 26");
struct NetSPacket_0x7953_Head
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
static_assert(offsetof(NetSPacket_0x7953_Head, magic_packet_id) == 0, "offsetof(NetSPacket_0x7953_Head, magic_packet_id) == 0");
static_assert(offsetof(NetSPacket_0x7953_Head, account_id) == 2, "offsetof(NetSPacket_0x7953_Head, account_id) == 2");
static_assert(offsetof(NetSPacket_0x7953_Head, gm_level) == 6, "offsetof(NetSPacket_0x7953_Head, gm_level) == 6");
static_assert(offsetof(NetSPacket_0x7953_Head, account_name) == 7, "offsetof(NetSPacket_0x7953_Head, account_name) == 7");
static_assert(offsetof(NetSPacket_0x7953_Head, sex) == 31, "offsetof(NetSPacket_0x7953_Head, sex) == 31");
static_assert(offsetof(NetSPacket_0x7953_Head, login_count) == 32, "offsetof(NetSPacket_0x7953_Head, login_count) == 32");
static_assert(offsetof(NetSPacket_0x7953_Head, state) == 36, "offsetof(NetSPacket_0x7953_Head, state) == 36");
static_assert(offsetof(NetSPacket_0x7953_Head, error_message) == 40, "offsetof(NetSPacket_0x7953_Head, error_message) == 40");
static_assert(offsetof(NetSPacket_0x7953_Head, last_login_string) == 60, "offsetof(NetSPacket_0x7953_Head, last_login_string) == 60");
static_assert(offsetof(NetSPacket_0x7953_Head, ip_string) == 84, "offsetof(NetSPacket_0x7953_Head, ip_string) == 84");
static_assert(offsetof(NetSPacket_0x7953_Head, email) == 100, "offsetof(NetSPacket_0x7953_Head, email) == 100");
static_assert(offsetof(NetSPacket_0x7953_Head, connect_until) == 140, "offsetof(NetSPacket_0x7953_Head, connect_until) == 140");
static_assert(offsetof(NetSPacket_0x7953_Head, ban_until) == 144, "offsetof(NetSPacket_0x7953_Head, ban_until) == 144");
static_assert(offsetof(NetSPacket_0x7953_Head, magic_packet_length) == 148, "offsetof(NetSPacket_0x7953_Head, magic_packet_length) == 148");
static_assert(sizeof(NetSPacket_0x7953_Head) == 150, "sizeof(NetSPacket_0x7953_Head) == 150");
struct NetSPacket_0x7953_Repeat
{
    Byte c;
};
static_assert(offsetof(NetSPacket_0x7953_Repeat, c) == 0, "offsetof(NetSPacket_0x7953_Repeat, c) == 0");
static_assert(sizeof(NetSPacket_0x7953_Repeat) == 1, "sizeof(NetSPacket_0x7953_Repeat) == 1");
struct NetRPacket_0x7954_Fixed
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetRPacket_0x7954_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7954_Fixed, magic_packet_id) == 0");
static_assert(offsetof(NetRPacket_0x7954_Fixed, account_id) == 2, "offsetof(NetRPacket_0x7954_Fixed, account_id) == 2");
static_assert(sizeof(NetRPacket_0x7954_Fixed) == 6, "sizeof(NetRPacket_0x7954_Fixed) == 6");
struct NetRPacket_0x7955_Fixed
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetRPacket_0x7955_Fixed, magic_packet_id) == 0, "offsetof(NetRPacket_0x7955_Fixed, magic_packet_id) == 0");
static_assert(sizeof(NetRPacket_0x7955_Fixed) == 2, "sizeof(NetRPacket_0x7955_Fixed) == 2");

inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x2726_Head *network, SPacket_0x2726_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x2726_Head *native, NetSPacket_0x2726_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x2726_Repeat *network, SPacket_0x2726_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x2726_Repeat *native, NetSPacket_0x2726_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7920_Fixed *network, RPacket_0x7920_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->start_account_id, native.start_account_id);
    rv &= native_to_network(&network->end_account_id, native.end_account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7920_Fixed *native, NetRPacket_0x7920_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->start_account_id, network.start_account_id);
    rv &= network_to_native(&native->end_account_id, network.end_account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7921_Head *network, SPacket_0x7921_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7921_Head *native, NetSPacket_0x7921_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7921_Repeat *network, SPacket_0x7921_Repeat native)
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
bool network_to_native(SPacket_0x7921_Repeat *native, NetSPacket_0x7921_Repeat network)
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
bool native_to_network(NetRPacket_0x7924_Fixed *network, RPacket_0x7924_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->source_item_id, native.source_item_id);
    rv &= native_to_network(&network->dest_item_id, native.dest_item_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7924_Fixed *native, NetRPacket_0x7924_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->source_item_id, network.source_item_id);
    rv &= network_to_native(&native->dest_item_id, network.dest_item_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7925_Fixed *network, SPacket_0x7925_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7925_Fixed *native, NetSPacket_0x7925_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7930_Fixed *network, RPacket_0x7930_Fixed native)
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
bool network_to_native(RPacket_0x7930_Fixed *native, NetRPacket_0x7930_Fixed network)
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
bool native_to_network(NetSPacket_0x7931_Fixed *network, SPacket_0x7931_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7931_Fixed *native, NetSPacket_0x7931_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7932_Fixed *network, RPacket_0x7932_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7932_Fixed *native, NetRPacket_0x7932_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7933_Fixed *network, SPacket_0x7933_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7933_Fixed *native, NetSPacket_0x7933_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7934_Fixed *network, RPacket_0x7934_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->password, native.password);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7934_Fixed *native, NetRPacket_0x7934_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->password, network.password);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7935_Fixed *network, SPacket_0x7935_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7935_Fixed *native, NetSPacket_0x7935_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7936_Fixed *network, RPacket_0x7936_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->status, native.status);
    rv &= native_to_network(&network->error_message, native.error_message);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7936_Fixed *native, NetRPacket_0x7936_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->status, network.status);
    rv &= network_to_native(&native->error_message, network.error_message);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7937_Fixed *network, SPacket_0x7937_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->status, native.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7937_Fixed *native, NetSPacket_0x7937_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->status, network.status);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7938_Fixed *network, RPacket_0x7938_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7938_Fixed *native, NetRPacket_0x7938_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7939_Head *network, SPacket_0x7939_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7939_Head *native, NetSPacket_0x7939_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7939_Repeat *network, SPacket_0x7939_Repeat native)
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
bool network_to_native(SPacket_0x7939_Repeat *native, NetSPacket_0x7939_Repeat network)
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
bool native_to_network(NetRPacket_0x793a_Fixed *network, RPacket_0x793a_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->password, native.password);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x793a_Fixed *native, NetRPacket_0x793a_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->password, network.password);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x793b_Fixed *network, SPacket_0x793b_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x793b_Fixed *native, NetSPacket_0x793b_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x793c_Fixed *network, RPacket_0x793c_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x793c_Fixed *native, NetRPacket_0x793c_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x793d_Fixed *network, SPacket_0x793d_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x793d_Fixed *native, NetSPacket_0x793d_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x793e_Fixed *network, RPacket_0x793e_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x793e_Fixed *native, NetRPacket_0x793e_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x793f_Fixed *network, SPacket_0x793f_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x793f_Fixed *native, NetSPacket_0x793f_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7940_Fixed *network, RPacket_0x7940_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->email, native.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7940_Fixed *native, NetRPacket_0x7940_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->email, network.email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7941_Fixed *network, SPacket_0x7941_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7941_Fixed *native, NetSPacket_0x7941_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7942_Head *network, RPacket_0x7942_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7942_Head *native, NetRPacket_0x7942_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7942_Repeat *network, RPacket_0x7942_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7942_Repeat *native, NetRPacket_0x7942_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7943_Fixed *network, SPacket_0x7943_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7943_Fixed *native, NetSPacket_0x7943_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7944_Fixed *network, RPacket_0x7944_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7944_Fixed *native, NetRPacket_0x7944_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7945_Fixed *network, SPacket_0x7945_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7945_Fixed *native, NetSPacket_0x7945_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7946_Fixed *network, RPacket_0x7946_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7946_Fixed *native, NetRPacket_0x7946_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7947_Fixed *network, SPacket_0x7947_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7947_Fixed *native, NetSPacket_0x7947_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7948_Fixed *network, RPacket_0x7948_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_until, native.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7948_Fixed *native, NetRPacket_0x7948_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_until, network.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7949_Fixed *network, SPacket_0x7949_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_until, native.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7949_Fixed *native, NetSPacket_0x7949_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_until, network.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x794a_Fixed *network, RPacket_0x794a_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x794a_Fixed *native, NetRPacket_0x794a_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x794b_Fixed *network, SPacket_0x794b_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x794b_Fixed *native, NetSPacket_0x794b_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x794c_Fixed *network, RPacket_0x794c_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_add, native.ban_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x794c_Fixed *native, NetRPacket_0x794c_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_add, network.ban_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x794d_Fixed *network, SPacket_0x794d_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->ban_until, native.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x794d_Fixed *native, NetSPacket_0x794d_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->ban_until, network.ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x794e_Head *network, RPacket_0x794e_Head native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x794e_Head *native, NetRPacket_0x794e_Head network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x794e_Repeat *network, RPacket_0x794e_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x794e_Repeat *native, NetRPacket_0x794e_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x794f_Fixed *network, SPacket_0x794f_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->error, native.error);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x794f_Fixed *native, NetSPacket_0x794f_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->error, network.error);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7950_Fixed *network, RPacket_0x7950_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_add, native.valid_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7950_Fixed *native, NetRPacket_0x7950_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_add, network.valid_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7951_Fixed *network, SPacket_0x7951_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->valid_until, native.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7951_Fixed *native, NetSPacket_0x7951_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->valid_until, network.valid_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7952_Fixed *network, RPacket_0x7952_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7952_Fixed *native, NetRPacket_0x7952_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetSPacket_0x7953_Head *network, SPacket_0x7953_Head native)
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
bool network_to_native(SPacket_0x7953_Head *native, NetSPacket_0x7953_Head network)
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
bool native_to_network(NetSPacket_0x7953_Repeat *network, SPacket_0x7953_Repeat native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(SPacket_0x7953_Repeat *native, NetSPacket_0x7953_Repeat network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7954_Fixed *network, RPacket_0x7954_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7954_Fixed *native, NetRPacket_0x7954_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetRPacket_0x7955_Fixed *network, RPacket_0x7955_Fixed native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(RPacket_0x7955_Fixed *native, NetRPacket_0x7955_Fixed network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

#endif // TMWA_PROTO2_LOGIN_ADMIN_HPP
