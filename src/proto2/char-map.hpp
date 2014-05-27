#ifndef TMWA_PROTO2_CHAR_MAP_HPP
#define TMWA_PROTO2_CHAR_MAP_HPP
//    char-map.hpp - TMWA network protocol: char/map
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
#pragma pack(push, 1)

template<>
struct Packet_Fixed<0x2af7>
{
    static const uint16_t PACKET_ID = 0x2af7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x2af8>
{
    static const uint16_t PACKET_ID = 0x2af8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountName account_name = {};
    AccountPass account_pass = {};
    uint32_t unused = {};
    IP4Address ip = {};
    uint16_t port = {};
};

template<>
struct Packet_Fixed<0x2af9>
{
    static const uint16_t PACKET_ID = 0x2af9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t code = {};
};

template<>
struct Packet_Head<0x2afa>
{
    static const uint16_t PACKET_ID = 0x2afa;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x2afa>
{
    static const uint16_t PACKET_ID = 0x2afa;

    MapName map_name = {};
};

template<>
struct Packet_Fixed<0x2afa>
{
    static const uint16_t PACKET_ID = 0x2afa;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    ItemNameId source_item_id = {};
    ItemNameId dest_item_id = {};
};

template<>
struct Packet_Fixed<0x2afb>
{
    static const uint16_t PACKET_ID = 0x2afb;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t unknown = {};
    CharName whisper_name = {};
};

template<>
struct Packet_Fixed<0x2afc>
{
    static const uint16_t PACKET_ID = 0x2afc;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    CharId char_id = {};
    uint32_t login_id1 = {};
    uint32_t login_id2 = {};
    IP4Address ip = {};
};

template<>
struct Packet_Payload<0x2afd>
{
    static const uint16_t PACKET_ID = 0x2afd;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
    uint32_t login_id2 = {};
    TimeT connect_until = {};
    uint16_t packet_tmw_version = {};
    CharKey char_key = {};
    CharData char_data = {};
};

template<>
struct Packet_Fixed<0x2afe>
{
    static const uint16_t PACKET_ID = 0x2afe;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Head<0x2aff>
{
    static const uint16_t PACKET_ID = 0x2aff;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    uint16_t users = {};
};
template<>
struct Packet_Repeat<0x2aff>
{
    static const uint16_t PACKET_ID = 0x2aff;

    CharId char_id = {};
};

template<>
struct Packet_Fixed<0x2b00>
{
    static const uint16_t PACKET_ID = 0x2b00;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint32_t users = {};
};

template<>
struct Packet_Payload<0x2b01>
{
    static const uint16_t PACKET_ID = 0x2b01;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
    CharId char_id = {};
    CharKey char_key = {};
    CharData char_data = {};
};

template<>
struct Packet_Fixed<0x2b02>
{
    static const uint16_t PACKET_ID = 0x2b02;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t login_id1 = {};
    uint32_t login_id2 = {};
    IP4Address ip = {};
};

template<>
struct Packet_Fixed<0x2b03>
{
    static const uint16_t PACKET_ID = 0x2b03;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t unknown = {};
};

template<>
struct Packet_Head<0x2b04>
{
    static const uint16_t PACKET_ID = 0x2b04;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    IP4Address ip = {};
    uint16_t port = {};
};
template<>
struct Packet_Repeat<0x2b04>
{
    static const uint16_t PACKET_ID = 0x2b04;

    MapName map_name = {};
};

template<>
struct Packet_Fixed<0x2b05>
{
    static const uint16_t PACKET_ID = 0x2b05;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t login_id1 = {};
    uint32_t login_id2 = {};
    CharId char_id = {};
    MapName map_name = {};
    uint16_t x = {};
    uint16_t y = {};
    IP4Address map_ip = {};
    uint16_t map_port = {};
    SEX sex = {};
    IP4Address client_ip = {};
};

template<>
struct Packet_Fixed<0x2b06>
{
    static const uint16_t PACKET_ID = 0x2b06;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t error = {};
    uint32_t unknown = {};
    CharId char_id = {};
    MapName map_name = {};
    uint16_t x = {};
    uint16_t y = {};
    IP4Address map_ip = {};
    uint16_t map_port = {};
};

template<>
struct Packet_Head<0x2b0a>
{
    static const uint16_t PACKET_ID = 0x2b0a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x2b0a>
{
    static const uint16_t PACKET_ID = 0x2b0a;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x2b0b>
{
    static const uint16_t PACKET_ID = 0x2b0b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    GmLevel gm_level = {};
};

template<>
struct Packet_Fixed<0x2b0c>
{
    static const uint16_t PACKET_ID = 0x2b0c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    AccountEmail old_email = {};
    AccountEmail new_email = {};
};

template<>
struct Packet_Fixed<0x2b0d>
{
    static const uint16_t PACKET_ID = 0x2b0d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    SEX sex = {};
};

template<>
struct Packet_Fixed<0x2b0e>
{
    static const uint16_t PACKET_ID = 0x2b0e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    CharName char_name = {};
    uint16_t operation = {};
    HumanTimeDiff ban_add = {};
};

template<>
struct Packet_Fixed<0x2b0f>
{
    static const uint16_t PACKET_ID = 0x2b0f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    CharName char_name = {};
    uint16_t operation = {};
    uint16_t error = {};
};

template<>
struct Packet_Head<0x2b10>
{
    static const uint16_t PACKET_ID = 0x2b10;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x2b10>
{
    static const uint16_t PACKET_ID = 0x2b10;

    VarName name = {};
    uint32_t value = {};
};

template<>
struct Packet_Head<0x2b11>
{
    static const uint16_t PACKET_ID = 0x2b11;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x2b11>
{
    static const uint16_t PACKET_ID = 0x2b11;

    VarName name = {};
    uint32_t value = {};
};

template<>
struct Packet_Fixed<0x2b12>
{
    static const uint16_t PACKET_ID = 0x2b12;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharId char_id = {};
    CharId partner_id = {};
};

template<>
struct Packet_Fixed<0x2b13>
{
    static const uint16_t PACKET_ID = 0x2b13;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x2b14>
{
    static const uint16_t PACKET_ID = 0x2b14;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t ban_not_status = {};
    TimeT status_or_ban_until = {};
};

template<>
struct Packet_Head<0x2b15>
{
    static const uint16_t PACKET_ID = 0x2b15;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x2b15>
{
    static const uint16_t PACKET_ID = 0x2b15;

    AccountId account_id = {};
    GmLevel gm_level = {};
};

template<>
struct Packet_Fixed<0x2b16>
{
    static const uint16_t PACKET_ID = 0x2b16;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharId char_id = {};
};

template<>
struct Packet_Head<0x3000>
{
    static const uint16_t PACKET_ID = 0x3000;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x3000>
{
    static const uint16_t PACKET_ID = 0x3000;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x3001>
{
    static const uint16_t PACKET_ID = 0x3001;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    CharName from_char_name = {};
    CharName to_char_name = {};
};
template<>
struct Packet_Repeat<0x3001>
{
    static const uint16_t PACKET_ID = 0x3001;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x3002>
{
    static const uint16_t PACKET_ID = 0x3002;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharId char_id = {};
    uint8_t flag = {};
};

template<>
struct Packet_Head<0x3003>
{
    static const uint16_t PACKET_ID = 0x3003;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    CharName char_name = {};
    GmLevel min_gm_level = {};
};
template<>
struct Packet_Repeat<0x3003>
{
    static const uint16_t PACKET_ID = 0x3003;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x3004>
{
    static const uint16_t PACKET_ID = 0x3004;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x3004>
{
    static const uint16_t PACKET_ID = 0x3004;

    VarName name = {};
    uint32_t value = {};
};

template<>
struct Packet_Fixed<0x3005>
{
    static const uint16_t PACKET_ID = 0x3005;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x3010>
{
    static const uint16_t PACKET_ID = 0x3010;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Payload<0x3011>
{
    static const uint16_t PACKET_ID = 0x3011;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
    Storage storage = {};
};

template<>
struct Packet_Fixed<0x3020>
{
    static const uint16_t PACKET_ID = 0x3020;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    PartyName party_name = {};
    CharName char_name = {};
    MapName map_name = {};
    uint16_t level = {};
};

template<>
struct Packet_Fixed<0x3021>
{
    static const uint16_t PACKET_ID = 0x3021;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
};

template<>
struct Packet_Fixed<0x3022>
{
    static const uint16_t PACKET_ID = 0x3022;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    CharName char_name = {};
    MapName map_name = {};
    uint16_t level = {};
};

template<>
struct Packet_Fixed<0x3023>
{
    static const uint16_t PACKET_ID = 0x3023;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    uint16_t exp = {};
    uint16_t item = {};
};

template<>
struct Packet_Fixed<0x3024>
{
    static const uint16_t PACKET_ID = 0x3024;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x3025>
{
    static const uint16_t PACKET_ID = 0x3025;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    MapName map_name = {};
    uint8_t online = {};
    uint16_t level = {};
};

template<>
struct Packet_Fixed<0x3026>
{
    static const uint16_t PACKET_ID = 0x3026;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
};

template<>
struct Packet_Head<0x3027>
{
    static const uint16_t PACKET_ID = 0x3027;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    PartyId party_id = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x3027>
{
    static const uint16_t PACKET_ID = 0x3027;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x3028>
{
    static const uint16_t PACKET_ID = 0x3028;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    CharName char_name = {};
};

template<>
struct Packet_Head<0x3800>
{
    static const uint16_t PACKET_ID = 0x3800;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x3800>
{
    static const uint16_t PACKET_ID = 0x3800;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x3801>
{
    static const uint16_t PACKET_ID = 0x3801;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    CharId whisper_id = {};
    CharName src_char_name = {};
    CharName dst_char_name = {};
};
template<>
struct Packet_Repeat<0x3801>
{
    static const uint16_t PACKET_ID = 0x3801;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x3802>
{
    static const uint16_t PACKET_ID = 0x3802;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharName sender_char_name = {};
    uint8_t flag = {};
};

template<>
struct Packet_Head<0x3803>
{
    static const uint16_t PACKET_ID = 0x3803;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    CharName char_name = {};
    GmLevel min_gm_level = {};
};
template<>
struct Packet_Repeat<0x3803>
{
    static const uint16_t PACKET_ID = 0x3803;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x3804>
{
    static const uint16_t PACKET_ID = 0x3804;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x3804>
{
    static const uint16_t PACKET_ID = 0x3804;

    VarName name = {};
    uint32_t value = {};
};

template<>
struct Packet_Payload<0x3810>
{
    static const uint16_t PACKET_ID = 0x3810;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
    Storage storage = {};
};

template<>
struct Packet_Fixed<0x3811>
{
    static const uint16_t PACKET_ID = 0x3811;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t unknown = {};
};

template<>
struct Packet_Fixed<0x3820>
{
    static const uint16_t PACKET_ID = 0x3820;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint8_t error = {};
    PartyId party_id = {};
    PartyName party_name = {};
};

template<>
struct Packet_Head<0x3821>
{
    static const uint16_t PACKET_ID = 0x3821;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    PartyId party_id = {};
};
template<>
struct Packet_Option<0x3821>
{
    static const uint16_t PACKET_ID = 0x3821;

    PartyMost party_most = {};
};

template<>
struct Packet_Fixed<0x3822>
{
    static const uint16_t PACKET_ID = 0x3822;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    uint8_t flag = {};
};

template<>
struct Packet_Fixed<0x3823>
{
    static const uint16_t PACKET_ID = 0x3823;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    uint16_t exp = {};
    uint16_t item = {};
    uint8_t flag = {};
};

template<>
struct Packet_Fixed<0x3824>
{
    static const uint16_t PACKET_ID = 0x3824;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    CharName char_name = {};
};

template<>
struct Packet_Fixed<0x3825>
{
    static const uint16_t PACKET_ID = 0x3825;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    AccountId account_id = {};
    MapName map_name = {};
    uint8_t online = {};
    uint16_t level = {};
};

template<>
struct Packet_Fixed<0x3826>
{
    static const uint16_t PACKET_ID = 0x3826;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyId party_id = {};
    uint8_t flag = {};
};

template<>
struct Packet_Head<0x3827>
{
    static const uint16_t PACKET_ID = 0x3827;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    PartyId party_id = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x3827>
{
    static const uint16_t PACKET_ID = 0x3827;

    uint8_t c = {};
};


template<>
struct NetPacket_Fixed<0x2af7>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2af7>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2af7>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x2af7>) == 2, "sizeof(NetPacket_Fixed<0x2af7>) == 2");

template<>
struct NetPacket_Fixed<0x2af8>
{
    Little16 magic_packet_id;
    NetString<sizeof(AccountName)> account_name;
    NetString<sizeof(AccountPass)> account_pass;
    Little32 unused;
    IP4Address ip;
    Little16 port;
};
static_assert(offsetof(NetPacket_Fixed<0x2af8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2af8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2af8>, account_name) == 2, "offsetof(NetPacket_Fixed<0x2af8>, account_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2af8>, account_pass) == 26, "offsetof(NetPacket_Fixed<0x2af8>, account_pass) == 26");
static_assert(offsetof(NetPacket_Fixed<0x2af8>, unused) == 50, "offsetof(NetPacket_Fixed<0x2af8>, unused) == 50");
static_assert(offsetof(NetPacket_Fixed<0x2af8>, ip) == 54, "offsetof(NetPacket_Fixed<0x2af8>, ip) == 54");
static_assert(offsetof(NetPacket_Fixed<0x2af8>, port) == 58, "offsetof(NetPacket_Fixed<0x2af8>, port) == 58");
static_assert(sizeof(NetPacket_Fixed<0x2af8>) == 60, "sizeof(NetPacket_Fixed<0x2af8>) == 60");

template<>
struct NetPacket_Fixed<0x2af9>
{
    Little16 magic_packet_id;
    Byte code;
};
static_assert(offsetof(NetPacket_Fixed<0x2af9>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2af9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2af9>, code) == 2, "offsetof(NetPacket_Fixed<0x2af9>, code) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2af9>) == 3, "sizeof(NetPacket_Fixed<0x2af9>) == 3");

template<>
struct NetPacket_Head<0x2afa>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x2afa>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2afa>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2afa>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2afa>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x2afa>) == 4, "sizeof(NetPacket_Head<0x2afa>) == 4");
template<>
struct NetPacket_Repeat<0x2afa>
{
    NetString<sizeof(MapName)> map_name;
};
static_assert(offsetof(NetPacket_Repeat<0x2afa>, map_name) == 0, "offsetof(NetPacket_Repeat<0x2afa>, map_name) == 0");
static_assert(sizeof(NetPacket_Repeat<0x2afa>) == 16, "sizeof(NetPacket_Repeat<0x2afa>) == 16");

template<>
struct NetPacket_Fixed<0x2afa>
{
    Little16 magic_packet_id;
    Little32 source_item_id;
    Little32 dest_item_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2afa>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2afa>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2afa>, source_item_id) == 2, "offsetof(NetPacket_Fixed<0x2afa>, source_item_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2afa>, dest_item_id) == 6, "offsetof(NetPacket_Fixed<0x2afa>, dest_item_id) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2afa>) == 10, "sizeof(NetPacket_Fixed<0x2afa>) == 10");

template<>
struct NetPacket_Fixed<0x2afb>
{
    Little16 magic_packet_id;
    Byte unknown;
    NetString<sizeof(CharName)> whisper_name;
};
static_assert(offsetof(NetPacket_Fixed<0x2afb>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2afb>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2afb>, unknown) == 2, "offsetof(NetPacket_Fixed<0x2afb>, unknown) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2afb>, whisper_name) == 3, "offsetof(NetPacket_Fixed<0x2afb>, whisper_name) == 3");
static_assert(sizeof(NetPacket_Fixed<0x2afb>) == 27, "sizeof(NetPacket_Fixed<0x2afb>) == 27");

template<>
struct NetPacket_Fixed<0x2afc>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 char_id;
    Little32 login_id1;
    Little32 login_id2;
    IP4Address ip;
};
static_assert(offsetof(NetPacket_Fixed<0x2afc>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2afc>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2afc>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2afc>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2afc>, char_id) == 6, "offsetof(NetPacket_Fixed<0x2afc>, char_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2afc>, login_id1) == 10, "offsetof(NetPacket_Fixed<0x2afc>, login_id1) == 10");
static_assert(offsetof(NetPacket_Fixed<0x2afc>, login_id2) == 14, "offsetof(NetPacket_Fixed<0x2afc>, login_id2) == 14");
static_assert(offsetof(NetPacket_Fixed<0x2afc>, ip) == 18, "offsetof(NetPacket_Fixed<0x2afc>, ip) == 18");
static_assert(sizeof(NetPacket_Fixed<0x2afc>) == 22, "sizeof(NetPacket_Fixed<0x2afc>) == 22");

template<>
struct NetPacket_Payload<0x2afd>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
    Little32 login_id2;
    Little32 connect_until;
    Little16 packet_tmw_version;
    CharKey char_key;
    CharData char_data;
};
static_assert(offsetof(NetPacket_Payload<0x2afd>, magic_packet_id) == 0, "offsetof(NetPacket_Payload<0x2afd>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Payload<0x2afd>, magic_packet_length) == 2, "offsetof(NetPacket_Payload<0x2afd>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Payload<0x2afd>, account_id) == 4, "offsetof(NetPacket_Payload<0x2afd>, account_id) == 4");
static_assert(offsetof(NetPacket_Payload<0x2afd>, login_id2) == 8, "offsetof(NetPacket_Payload<0x2afd>, login_id2) == 8");
static_assert(offsetof(NetPacket_Payload<0x2afd>, connect_until) == 12, "offsetof(NetPacket_Payload<0x2afd>, connect_until) == 12");
static_assert(offsetof(NetPacket_Payload<0x2afd>, packet_tmw_version) == 16, "offsetof(NetPacket_Payload<0x2afd>, packet_tmw_version) == 16");
static_assert(offsetof(NetPacket_Payload<0x2afd>, char_key) == 18, "offsetof(NetPacket_Payload<0x2afd>, char_key) == 18");

template<>
struct NetPacket_Fixed<0x2afe>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2afe>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2afe>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2afe>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2afe>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2afe>) == 6, "sizeof(NetPacket_Fixed<0x2afe>) == 6");

template<>
struct NetPacket_Head<0x2aff>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little16 users;
};
static_assert(offsetof(NetPacket_Head<0x2aff>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2aff>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2aff>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2aff>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2aff>, users) == 4, "offsetof(NetPacket_Head<0x2aff>, users) == 4");
static_assert(sizeof(NetPacket_Head<0x2aff>) == 6, "sizeof(NetPacket_Head<0x2aff>) == 6");
template<>
struct NetPacket_Repeat<0x2aff>
{
    Little32 char_id;
};
static_assert(offsetof(NetPacket_Repeat<0x2aff>, char_id) == 0, "offsetof(NetPacket_Repeat<0x2aff>, char_id) == 0");
static_assert(sizeof(NetPacket_Repeat<0x2aff>) == 4, "sizeof(NetPacket_Repeat<0x2aff>) == 4");

template<>
struct NetPacket_Fixed<0x2b00>
{
    Little16 magic_packet_id;
    Little32 users;
};
static_assert(offsetof(NetPacket_Fixed<0x2b00>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b00>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b00>, users) == 2, "offsetof(NetPacket_Fixed<0x2b00>, users) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2b00>) == 6, "sizeof(NetPacket_Fixed<0x2b00>) == 6");

template<>
struct NetPacket_Payload<0x2b01>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
    Little32 char_id;
    CharKey char_key;
    CharData char_data;
};
static_assert(offsetof(NetPacket_Payload<0x2b01>, magic_packet_id) == 0, "offsetof(NetPacket_Payload<0x2b01>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Payload<0x2b01>, magic_packet_length) == 2, "offsetof(NetPacket_Payload<0x2b01>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Payload<0x2b01>, account_id) == 4, "offsetof(NetPacket_Payload<0x2b01>, account_id) == 4");
static_assert(offsetof(NetPacket_Payload<0x2b01>, char_id) == 8, "offsetof(NetPacket_Payload<0x2b01>, char_id) == 8");
static_assert(offsetof(NetPacket_Payload<0x2b01>, char_key) == 12, "offsetof(NetPacket_Payload<0x2b01>, char_key) == 12");

template<>
struct NetPacket_Fixed<0x2b02>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 login_id1;
    Little32 login_id2;
    IP4Address ip;
};
static_assert(offsetof(NetPacket_Fixed<0x2b02>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b02>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b02>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b02>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b02>, login_id1) == 6, "offsetof(NetPacket_Fixed<0x2b02>, login_id1) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b02>, login_id2) == 10, "offsetof(NetPacket_Fixed<0x2b02>, login_id2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x2b02>, ip) == 14, "offsetof(NetPacket_Fixed<0x2b02>, ip) == 14");
static_assert(sizeof(NetPacket_Fixed<0x2b02>) == 18, "sizeof(NetPacket_Fixed<0x2b02>) == 18");

template<>
struct NetPacket_Fixed<0x2b03>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte unknown;
};
static_assert(offsetof(NetPacket_Fixed<0x2b03>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b03>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b03>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b03>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b03>, unknown) == 6, "offsetof(NetPacket_Fixed<0x2b03>, unknown) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2b03>) == 7, "sizeof(NetPacket_Fixed<0x2b03>) == 7");

template<>
struct NetPacket_Head<0x2b04>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    IP4Address ip;
    Little16 port;
};
static_assert(offsetof(NetPacket_Head<0x2b04>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2b04>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2b04>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2b04>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2b04>, ip) == 4, "offsetof(NetPacket_Head<0x2b04>, ip) == 4");
static_assert(offsetof(NetPacket_Head<0x2b04>, port) == 8, "offsetof(NetPacket_Head<0x2b04>, port) == 8");
static_assert(sizeof(NetPacket_Head<0x2b04>) == 10, "sizeof(NetPacket_Head<0x2b04>) == 10");
template<>
struct NetPacket_Repeat<0x2b04>
{
    NetString<sizeof(MapName)> map_name;
};
static_assert(offsetof(NetPacket_Repeat<0x2b04>, map_name) == 0, "offsetof(NetPacket_Repeat<0x2b04>, map_name) == 0");
static_assert(sizeof(NetPacket_Repeat<0x2b04>) == 16, "sizeof(NetPacket_Repeat<0x2b04>) == 16");

template<>
struct NetPacket_Fixed<0x2b05>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 login_id1;
    Little32 login_id2;
    Little32 char_id;
    NetString<sizeof(MapName)> map_name;
    Little16 x;
    Little16 y;
    IP4Address map_ip;
    Little16 map_port;
    Byte sex;
    IP4Address client_ip;
};
static_assert(offsetof(NetPacket_Fixed<0x2b05>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b05>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b05>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, login_id1) == 6, "offsetof(NetPacket_Fixed<0x2b05>, login_id1) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, login_id2) == 10, "offsetof(NetPacket_Fixed<0x2b05>, login_id2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, char_id) == 14, "offsetof(NetPacket_Fixed<0x2b05>, char_id) == 14");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, map_name) == 18, "offsetof(NetPacket_Fixed<0x2b05>, map_name) == 18");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, x) == 34, "offsetof(NetPacket_Fixed<0x2b05>, x) == 34");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, y) == 36, "offsetof(NetPacket_Fixed<0x2b05>, y) == 36");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, map_ip) == 38, "offsetof(NetPacket_Fixed<0x2b05>, map_ip) == 38");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, map_port) == 42, "offsetof(NetPacket_Fixed<0x2b05>, map_port) == 42");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, sex) == 44, "offsetof(NetPacket_Fixed<0x2b05>, sex) == 44");
static_assert(offsetof(NetPacket_Fixed<0x2b05>, client_ip) == 45, "offsetof(NetPacket_Fixed<0x2b05>, client_ip) == 45");
static_assert(sizeof(NetPacket_Fixed<0x2b05>) == 49, "sizeof(NetPacket_Fixed<0x2b05>) == 49");

template<>
struct NetPacket_Fixed<0x2b06>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 error;
    Little32 unknown;
    Little32 char_id;
    NetString<sizeof(MapName)> map_name;
    Little16 x;
    Little16 y;
    IP4Address map_ip;
    Little16 map_port;
};
static_assert(offsetof(NetPacket_Fixed<0x2b06>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b06>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b06>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, error) == 6, "offsetof(NetPacket_Fixed<0x2b06>, error) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, unknown) == 10, "offsetof(NetPacket_Fixed<0x2b06>, unknown) == 10");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, char_id) == 14, "offsetof(NetPacket_Fixed<0x2b06>, char_id) == 14");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, map_name) == 18, "offsetof(NetPacket_Fixed<0x2b06>, map_name) == 18");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, x) == 34, "offsetof(NetPacket_Fixed<0x2b06>, x) == 34");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, y) == 36, "offsetof(NetPacket_Fixed<0x2b06>, y) == 36");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, map_ip) == 38, "offsetof(NetPacket_Fixed<0x2b06>, map_ip) == 38");
static_assert(offsetof(NetPacket_Fixed<0x2b06>, map_port) == 42, "offsetof(NetPacket_Fixed<0x2b06>, map_port) == 42");
static_assert(sizeof(NetPacket_Fixed<0x2b06>) == 44, "sizeof(NetPacket_Fixed<0x2b06>) == 44");

template<>
struct NetPacket_Head<0x2b0a>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x2b0a>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2b0a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2b0a>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2b0a>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2b0a>, account_id) == 4, "offsetof(NetPacket_Head<0x2b0a>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x2b0a>) == 8, "sizeof(NetPacket_Head<0x2b0a>) == 8");
template<>
struct NetPacket_Repeat<0x2b0a>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x2b0a>, c) == 0, "offsetof(NetPacket_Repeat<0x2b0a>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x2b0a>) == 1, "sizeof(NetPacket_Repeat<0x2b0a>) == 1");

template<>
struct NetPacket_Fixed<0x2b0b>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 gm_level;
};
static_assert(offsetof(NetPacket_Fixed<0x2b0b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b0b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b0b>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b0b>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b0b>, gm_level) == 6, "offsetof(NetPacket_Fixed<0x2b0b>, gm_level) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2b0b>) == 10, "sizeof(NetPacket_Fixed<0x2b0b>) == 10");

template<>
struct NetPacket_Fixed<0x2b0c>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(AccountEmail)> old_email;
    NetString<sizeof(AccountEmail)> new_email;
};
static_assert(offsetof(NetPacket_Fixed<0x2b0c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b0c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b0c>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b0c>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b0c>, old_email) == 6, "offsetof(NetPacket_Fixed<0x2b0c>, old_email) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b0c>, new_email) == 46, "offsetof(NetPacket_Fixed<0x2b0c>, new_email) == 46");
static_assert(sizeof(NetPacket_Fixed<0x2b0c>) == 86, "sizeof(NetPacket_Fixed<0x2b0c>) == 86");

template<>
struct NetPacket_Fixed<0x2b0d>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte sex;
};
static_assert(offsetof(NetPacket_Fixed<0x2b0d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b0d>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b0d>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b0d>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b0d>, sex) == 6, "offsetof(NetPacket_Fixed<0x2b0d>, sex) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2b0d>) == 7, "sizeof(NetPacket_Fixed<0x2b0d>) == 7");

template<>
struct NetPacket_Fixed<0x2b0e>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
    Little16 operation;
    NetHumanTimeDiff ban_add;
};
static_assert(offsetof(NetPacket_Fixed<0x2b0e>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b0e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b0e>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b0e>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b0e>, char_name) == 6, "offsetof(NetPacket_Fixed<0x2b0e>, char_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b0e>, operation) == 30, "offsetof(NetPacket_Fixed<0x2b0e>, operation) == 30");
static_assert(offsetof(NetPacket_Fixed<0x2b0e>, ban_add) == 32, "offsetof(NetPacket_Fixed<0x2b0e>, ban_add) == 32");
static_assert(sizeof(NetPacket_Fixed<0x2b0e>) == 44, "sizeof(NetPacket_Fixed<0x2b0e>) == 44");

template<>
struct NetPacket_Fixed<0x2b0f>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
    Little16 operation;
    Little16 error;
};
static_assert(offsetof(NetPacket_Fixed<0x2b0f>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b0f>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b0f>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b0f>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b0f>, char_name) == 6, "offsetof(NetPacket_Fixed<0x2b0f>, char_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b0f>, operation) == 30, "offsetof(NetPacket_Fixed<0x2b0f>, operation) == 30");
static_assert(offsetof(NetPacket_Fixed<0x2b0f>, error) == 32, "offsetof(NetPacket_Fixed<0x2b0f>, error) == 32");
static_assert(sizeof(NetPacket_Fixed<0x2b0f>) == 34, "sizeof(NetPacket_Fixed<0x2b0f>) == 34");

template<>
struct NetPacket_Head<0x2b10>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x2b10>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2b10>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2b10>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2b10>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2b10>, account_id) == 4, "offsetof(NetPacket_Head<0x2b10>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x2b10>) == 8, "sizeof(NetPacket_Head<0x2b10>) == 8");
template<>
struct NetPacket_Repeat<0x2b10>
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetPacket_Repeat<0x2b10>, name) == 0, "offsetof(NetPacket_Repeat<0x2b10>, name) == 0");
static_assert(offsetof(NetPacket_Repeat<0x2b10>, value) == 32, "offsetof(NetPacket_Repeat<0x2b10>, value) == 32");
static_assert(sizeof(NetPacket_Repeat<0x2b10>) == 36, "sizeof(NetPacket_Repeat<0x2b10>) == 36");

template<>
struct NetPacket_Head<0x2b11>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x2b11>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2b11>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2b11>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2b11>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x2b11>, account_id) == 4, "offsetof(NetPacket_Head<0x2b11>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x2b11>) == 8, "sizeof(NetPacket_Head<0x2b11>) == 8");
template<>
struct NetPacket_Repeat<0x2b11>
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetPacket_Repeat<0x2b11>, name) == 0, "offsetof(NetPacket_Repeat<0x2b11>, name) == 0");
static_assert(offsetof(NetPacket_Repeat<0x2b11>, value) == 32, "offsetof(NetPacket_Repeat<0x2b11>, value) == 32");
static_assert(sizeof(NetPacket_Repeat<0x2b11>) == 36, "sizeof(NetPacket_Repeat<0x2b11>) == 36");

template<>
struct NetPacket_Fixed<0x2b12>
{
    Little16 magic_packet_id;
    Little32 char_id;
    Little32 partner_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2b12>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b12>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b12>, char_id) == 2, "offsetof(NetPacket_Fixed<0x2b12>, char_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b12>, partner_id) == 6, "offsetof(NetPacket_Fixed<0x2b12>, partner_id) == 6");
static_assert(sizeof(NetPacket_Fixed<0x2b12>) == 10, "sizeof(NetPacket_Fixed<0x2b12>) == 10");

template<>
struct NetPacket_Fixed<0x2b13>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2b13>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b13>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b13>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b13>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2b13>) == 6, "sizeof(NetPacket_Fixed<0x2b13>) == 6");

template<>
struct NetPacket_Fixed<0x2b14>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte ban_not_status;
    Little32 status_or_ban_until;
};
static_assert(offsetof(NetPacket_Fixed<0x2b14>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b14>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b14>, account_id) == 2, "offsetof(NetPacket_Fixed<0x2b14>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x2b14>, ban_not_status) == 6, "offsetof(NetPacket_Fixed<0x2b14>, ban_not_status) == 6");
static_assert(offsetof(NetPacket_Fixed<0x2b14>, status_or_ban_until) == 7, "offsetof(NetPacket_Fixed<0x2b14>, status_or_ban_until) == 7");
static_assert(sizeof(NetPacket_Fixed<0x2b14>) == 11, "sizeof(NetPacket_Fixed<0x2b14>) == 11");

template<>
struct NetPacket_Head<0x2b15>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x2b15>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x2b15>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x2b15>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x2b15>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x2b15>) == 4, "sizeof(NetPacket_Head<0x2b15>) == 4");
template<>
struct NetPacket_Repeat<0x2b15>
{
    Little32 account_id;
    Byte gm_level;
};
static_assert(offsetof(NetPacket_Repeat<0x2b15>, account_id) == 0, "offsetof(NetPacket_Repeat<0x2b15>, account_id) == 0");
static_assert(offsetof(NetPacket_Repeat<0x2b15>, gm_level) == 4, "offsetof(NetPacket_Repeat<0x2b15>, gm_level) == 4");
static_assert(sizeof(NetPacket_Repeat<0x2b15>) == 5, "sizeof(NetPacket_Repeat<0x2b15>) == 5");

template<>
struct NetPacket_Fixed<0x2b16>
{
    Little16 magic_packet_id;
    Little32 char_id;
};
static_assert(offsetof(NetPacket_Fixed<0x2b16>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x2b16>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x2b16>, char_id) == 2, "offsetof(NetPacket_Fixed<0x2b16>, char_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x2b16>) == 6, "sizeof(NetPacket_Fixed<0x2b16>) == 6");

template<>
struct NetPacket_Head<0x3000>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x3000>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3000>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3000>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3000>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x3000>) == 4, "sizeof(NetPacket_Head<0x3000>) == 4");
template<>
struct NetPacket_Repeat<0x3000>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3000>, c) == 0, "offsetof(NetPacket_Repeat<0x3000>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3000>) == 1, "sizeof(NetPacket_Repeat<0x3000>) == 1");

template<>
struct NetPacket_Head<0x3001>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(CharName)> from_char_name;
    NetString<sizeof(CharName)> to_char_name;
};
static_assert(offsetof(NetPacket_Head<0x3001>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3001>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3001>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3001>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3001>, from_char_name) == 4, "offsetof(NetPacket_Head<0x3001>, from_char_name) == 4");
static_assert(offsetof(NetPacket_Head<0x3001>, to_char_name) == 28, "offsetof(NetPacket_Head<0x3001>, to_char_name) == 28");
static_assert(sizeof(NetPacket_Head<0x3001>) == 52, "sizeof(NetPacket_Head<0x3001>) == 52");
template<>
struct NetPacket_Repeat<0x3001>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3001>, c) == 0, "offsetof(NetPacket_Repeat<0x3001>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3001>) == 1, "sizeof(NetPacket_Repeat<0x3001>) == 1");

template<>
struct NetPacket_Fixed<0x3002>
{
    Little16 magic_packet_id;
    Little32 char_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x3002>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3002>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3002>, char_id) == 2, "offsetof(NetPacket_Fixed<0x3002>, char_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3002>, flag) == 6, "offsetof(NetPacket_Fixed<0x3002>, flag) == 6");
static_assert(sizeof(NetPacket_Fixed<0x3002>) == 7, "sizeof(NetPacket_Fixed<0x3002>) == 7");

template<>
struct NetPacket_Head<0x3003>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(CharName)> char_name;
    Little16 min_gm_level;
};
static_assert(offsetof(NetPacket_Head<0x3003>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3003>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3003>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3003>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3003>, char_name) == 4, "offsetof(NetPacket_Head<0x3003>, char_name) == 4");
static_assert(offsetof(NetPacket_Head<0x3003>, min_gm_level) == 28, "offsetof(NetPacket_Head<0x3003>, min_gm_level) == 28");
static_assert(sizeof(NetPacket_Head<0x3003>) == 30, "sizeof(NetPacket_Head<0x3003>) == 30");
template<>
struct NetPacket_Repeat<0x3003>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3003>, c) == 0, "offsetof(NetPacket_Repeat<0x3003>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3003>) == 1, "sizeof(NetPacket_Repeat<0x3003>) == 1");

template<>
struct NetPacket_Head<0x3004>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x3004>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3004>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3004>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3004>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3004>, account_id) == 4, "offsetof(NetPacket_Head<0x3004>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x3004>) == 8, "sizeof(NetPacket_Head<0x3004>) == 8");
template<>
struct NetPacket_Repeat<0x3004>
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetPacket_Repeat<0x3004>, name) == 0, "offsetof(NetPacket_Repeat<0x3004>, name) == 0");
static_assert(offsetof(NetPacket_Repeat<0x3004>, value) == 32, "offsetof(NetPacket_Repeat<0x3004>, value) == 32");
static_assert(sizeof(NetPacket_Repeat<0x3004>) == 36, "sizeof(NetPacket_Repeat<0x3004>) == 36");

template<>
struct NetPacket_Fixed<0x3005>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x3005>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3005>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3005>, account_id) == 2, "offsetof(NetPacket_Fixed<0x3005>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x3005>) == 6, "sizeof(NetPacket_Fixed<0x3005>) == 6");

template<>
struct NetPacket_Fixed<0x3010>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x3010>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3010>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3010>, account_id) == 2, "offsetof(NetPacket_Fixed<0x3010>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x3010>) == 6, "sizeof(NetPacket_Fixed<0x3010>) == 6");

template<>
struct NetPacket_Payload<0x3011>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
    Storage storage;
};
static_assert(offsetof(NetPacket_Payload<0x3011>, magic_packet_id) == 0, "offsetof(NetPacket_Payload<0x3011>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Payload<0x3011>, magic_packet_length) == 2, "offsetof(NetPacket_Payload<0x3011>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Payload<0x3011>, account_id) == 4, "offsetof(NetPacket_Payload<0x3011>, account_id) == 4");
static_assert(offsetof(NetPacket_Payload<0x3011>, storage) == 8, "offsetof(NetPacket_Payload<0x3011>, storage) == 8");

template<>
struct NetPacket_Fixed<0x3020>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(PartyName)> party_name;
    NetString<sizeof(CharName)> char_name;
    NetString<sizeof(MapName)> map_name;
    Little16 level;
};
static_assert(offsetof(NetPacket_Fixed<0x3020>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3020>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3020>, account_id) == 2, "offsetof(NetPacket_Fixed<0x3020>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3020>, party_name) == 6, "offsetof(NetPacket_Fixed<0x3020>, party_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3020>, char_name) == 30, "offsetof(NetPacket_Fixed<0x3020>, char_name) == 30");
static_assert(offsetof(NetPacket_Fixed<0x3020>, map_name) == 54, "offsetof(NetPacket_Fixed<0x3020>, map_name) == 54");
static_assert(offsetof(NetPacket_Fixed<0x3020>, level) == 70, "offsetof(NetPacket_Fixed<0x3020>, level) == 70");
static_assert(sizeof(NetPacket_Fixed<0x3020>) == 72, "sizeof(NetPacket_Fixed<0x3020>) == 72");

template<>
struct NetPacket_Fixed<0x3021>
{
    Little16 magic_packet_id;
    Little32 party_id;
};
static_assert(offsetof(NetPacket_Fixed<0x3021>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3021>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3021>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3021>, party_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x3021>) == 6, "sizeof(NetPacket_Fixed<0x3021>) == 6");

template<>
struct NetPacket_Fixed<0x3022>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
    NetString<sizeof(MapName)> map_name;
    Little16 level;
};
static_assert(offsetof(NetPacket_Fixed<0x3022>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3022>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3022>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3022>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3022>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3022>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3022>, char_name) == 10, "offsetof(NetPacket_Fixed<0x3022>, char_name) == 10");
static_assert(offsetof(NetPacket_Fixed<0x3022>, map_name) == 34, "offsetof(NetPacket_Fixed<0x3022>, map_name) == 34");
static_assert(offsetof(NetPacket_Fixed<0x3022>, level) == 50, "offsetof(NetPacket_Fixed<0x3022>, level) == 50");
static_assert(sizeof(NetPacket_Fixed<0x3022>) == 52, "sizeof(NetPacket_Fixed<0x3022>) == 52");

template<>
struct NetPacket_Fixed<0x3023>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    Little16 exp;
    Little16 item;
};
static_assert(offsetof(NetPacket_Fixed<0x3023>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3023>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3023>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3023>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3023>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3023>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3023>, exp) == 10, "offsetof(NetPacket_Fixed<0x3023>, exp) == 10");
static_assert(offsetof(NetPacket_Fixed<0x3023>, item) == 12, "offsetof(NetPacket_Fixed<0x3023>, item) == 12");
static_assert(sizeof(NetPacket_Fixed<0x3023>) == 14, "sizeof(NetPacket_Fixed<0x3023>) == 14");

template<>
struct NetPacket_Fixed<0x3024>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x3024>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3024>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3024>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3024>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3024>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3024>, account_id) == 6");
static_assert(sizeof(NetPacket_Fixed<0x3024>) == 10, "sizeof(NetPacket_Fixed<0x3024>) == 10");

template<>
struct NetPacket_Fixed<0x3025>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    NetString<sizeof(MapName)> map_name;
    Byte online;
    Little16 level;
};
static_assert(offsetof(NetPacket_Fixed<0x3025>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3025>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3025>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3025>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3025>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3025>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3025>, map_name) == 10, "offsetof(NetPacket_Fixed<0x3025>, map_name) == 10");
static_assert(offsetof(NetPacket_Fixed<0x3025>, online) == 26, "offsetof(NetPacket_Fixed<0x3025>, online) == 26");
static_assert(offsetof(NetPacket_Fixed<0x3025>, level) == 27, "offsetof(NetPacket_Fixed<0x3025>, level) == 27");
static_assert(sizeof(NetPacket_Fixed<0x3025>) == 29, "sizeof(NetPacket_Fixed<0x3025>) == 29");

template<>
struct NetPacket_Fixed<0x3026>
{
    Little16 magic_packet_id;
    Little32 party_id;
};
static_assert(offsetof(NetPacket_Fixed<0x3026>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3026>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3026>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3026>, party_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x3026>) == 6, "sizeof(NetPacket_Fixed<0x3026>) == 6");

template<>
struct NetPacket_Head<0x3027>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 party_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x3027>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3027>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3027>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3027>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3027>, party_id) == 4, "offsetof(NetPacket_Head<0x3027>, party_id) == 4");
static_assert(offsetof(NetPacket_Head<0x3027>, account_id) == 8, "offsetof(NetPacket_Head<0x3027>, account_id) == 8");
static_assert(sizeof(NetPacket_Head<0x3027>) == 12, "sizeof(NetPacket_Head<0x3027>) == 12");
template<>
struct NetPacket_Repeat<0x3027>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3027>, c) == 0, "offsetof(NetPacket_Repeat<0x3027>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3027>) == 1, "sizeof(NetPacket_Repeat<0x3027>) == 1");

template<>
struct NetPacket_Fixed<0x3028>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
};
static_assert(offsetof(NetPacket_Fixed<0x3028>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3028>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3028>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3028>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3028>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3028>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3028>, char_name) == 10, "offsetof(NetPacket_Fixed<0x3028>, char_name) == 10");
static_assert(sizeof(NetPacket_Fixed<0x3028>) == 34, "sizeof(NetPacket_Fixed<0x3028>) == 34");

template<>
struct NetPacket_Head<0x3800>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x3800>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3800>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3800>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3800>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x3800>) == 4, "sizeof(NetPacket_Head<0x3800>) == 4");
template<>
struct NetPacket_Repeat<0x3800>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3800>, c) == 0, "offsetof(NetPacket_Repeat<0x3800>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3800>) == 1, "sizeof(NetPacket_Repeat<0x3800>) == 1");

template<>
struct NetPacket_Head<0x3801>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 whisper_id;
    NetString<sizeof(CharName)> src_char_name;
    NetString<sizeof(CharName)> dst_char_name;
};
static_assert(offsetof(NetPacket_Head<0x3801>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3801>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3801>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3801>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3801>, whisper_id) == 4, "offsetof(NetPacket_Head<0x3801>, whisper_id) == 4");
static_assert(offsetof(NetPacket_Head<0x3801>, src_char_name) == 8, "offsetof(NetPacket_Head<0x3801>, src_char_name) == 8");
static_assert(offsetof(NetPacket_Head<0x3801>, dst_char_name) == 32, "offsetof(NetPacket_Head<0x3801>, dst_char_name) == 32");
static_assert(sizeof(NetPacket_Head<0x3801>) == 56, "sizeof(NetPacket_Head<0x3801>) == 56");
template<>
struct NetPacket_Repeat<0x3801>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3801>, c) == 0, "offsetof(NetPacket_Repeat<0x3801>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3801>) == 1, "sizeof(NetPacket_Repeat<0x3801>) == 1");

template<>
struct NetPacket_Fixed<0x3802>
{
    Little16 magic_packet_id;
    NetString<sizeof(CharName)> sender_char_name;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x3802>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3802>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3802>, sender_char_name) == 2, "offsetof(NetPacket_Fixed<0x3802>, sender_char_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3802>, flag) == 26, "offsetof(NetPacket_Fixed<0x3802>, flag) == 26");
static_assert(sizeof(NetPacket_Fixed<0x3802>) == 27, "sizeof(NetPacket_Fixed<0x3802>) == 27");

template<>
struct NetPacket_Head<0x3803>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(CharName)> char_name;
    Little16 min_gm_level;
};
static_assert(offsetof(NetPacket_Head<0x3803>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3803>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3803>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3803>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3803>, char_name) == 4, "offsetof(NetPacket_Head<0x3803>, char_name) == 4");
static_assert(offsetof(NetPacket_Head<0x3803>, min_gm_level) == 28, "offsetof(NetPacket_Head<0x3803>, min_gm_level) == 28");
static_assert(sizeof(NetPacket_Head<0x3803>) == 30, "sizeof(NetPacket_Head<0x3803>) == 30");
template<>
struct NetPacket_Repeat<0x3803>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3803>, c) == 0, "offsetof(NetPacket_Repeat<0x3803>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3803>) == 1, "sizeof(NetPacket_Repeat<0x3803>) == 1");

template<>
struct NetPacket_Head<0x3804>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x3804>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3804>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3804>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3804>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3804>, account_id) == 4, "offsetof(NetPacket_Head<0x3804>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x3804>) == 8, "sizeof(NetPacket_Head<0x3804>) == 8");
template<>
struct NetPacket_Repeat<0x3804>
{
    NetString<sizeof(VarName)> name;
    Little32 value;
};
static_assert(offsetof(NetPacket_Repeat<0x3804>, name) == 0, "offsetof(NetPacket_Repeat<0x3804>, name) == 0");
static_assert(offsetof(NetPacket_Repeat<0x3804>, value) == 32, "offsetof(NetPacket_Repeat<0x3804>, value) == 32");
static_assert(sizeof(NetPacket_Repeat<0x3804>) == 36, "sizeof(NetPacket_Repeat<0x3804>) == 36");

template<>
struct NetPacket_Payload<0x3810>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
    Storage storage;
};
static_assert(offsetof(NetPacket_Payload<0x3810>, magic_packet_id) == 0, "offsetof(NetPacket_Payload<0x3810>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Payload<0x3810>, magic_packet_length) == 2, "offsetof(NetPacket_Payload<0x3810>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Payload<0x3810>, account_id) == 4, "offsetof(NetPacket_Payload<0x3810>, account_id) == 4");
static_assert(offsetof(NetPacket_Payload<0x3810>, storage) == 8, "offsetof(NetPacket_Payload<0x3810>, storage) == 8");

template<>
struct NetPacket_Fixed<0x3811>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte unknown;
};
static_assert(offsetof(NetPacket_Fixed<0x3811>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3811>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3811>, account_id) == 2, "offsetof(NetPacket_Fixed<0x3811>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3811>, unknown) == 6, "offsetof(NetPacket_Fixed<0x3811>, unknown) == 6");
static_assert(sizeof(NetPacket_Fixed<0x3811>) == 7, "sizeof(NetPacket_Fixed<0x3811>) == 7");

template<>
struct NetPacket_Fixed<0x3820>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Byte error;
    Little32 party_id;
    NetString<sizeof(PartyName)> party_name;
};
static_assert(offsetof(NetPacket_Fixed<0x3820>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3820>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3820>, account_id) == 2, "offsetof(NetPacket_Fixed<0x3820>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3820>, error) == 6, "offsetof(NetPacket_Fixed<0x3820>, error) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3820>, party_id) == 7, "offsetof(NetPacket_Fixed<0x3820>, party_id) == 7");
static_assert(offsetof(NetPacket_Fixed<0x3820>, party_name) == 11, "offsetof(NetPacket_Fixed<0x3820>, party_name) == 11");
static_assert(sizeof(NetPacket_Fixed<0x3820>) == 35, "sizeof(NetPacket_Fixed<0x3820>) == 35");

template<>
struct NetPacket_Head<0x3821>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 party_id;
};
static_assert(offsetof(NetPacket_Head<0x3821>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3821>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3821>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3821>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3821>, party_id) == 4, "offsetof(NetPacket_Head<0x3821>, party_id) == 4");
static_assert(sizeof(NetPacket_Head<0x3821>) == 8, "sizeof(NetPacket_Head<0x3821>) == 8");
template<>
struct NetPacket_Option<0x3821>
{
    PartyMost party_most;
};
static_assert(offsetof(NetPacket_Option<0x3821>, party_most) == 0, "offsetof(NetPacket_Option<0x3821>, party_most) == 0");

template<>
struct NetPacket_Fixed<0x3822>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x3822>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3822>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3822>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3822>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3822>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3822>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3822>, flag) == 10, "offsetof(NetPacket_Fixed<0x3822>, flag) == 10");
static_assert(sizeof(NetPacket_Fixed<0x3822>) == 11, "sizeof(NetPacket_Fixed<0x3822>) == 11");

template<>
struct NetPacket_Fixed<0x3823>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    Little16 exp;
    Little16 item;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x3823>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3823>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3823>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3823>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3823>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3823>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3823>, exp) == 10, "offsetof(NetPacket_Fixed<0x3823>, exp) == 10");
static_assert(offsetof(NetPacket_Fixed<0x3823>, item) == 12, "offsetof(NetPacket_Fixed<0x3823>, item) == 12");
static_assert(offsetof(NetPacket_Fixed<0x3823>, flag) == 14, "offsetof(NetPacket_Fixed<0x3823>, flag) == 14");
static_assert(sizeof(NetPacket_Fixed<0x3823>) == 15, "sizeof(NetPacket_Fixed<0x3823>) == 15");

template<>
struct NetPacket_Fixed<0x3824>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
};
static_assert(offsetof(NetPacket_Fixed<0x3824>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3824>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3824>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3824>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3824>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3824>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3824>, char_name) == 10, "offsetof(NetPacket_Fixed<0x3824>, char_name) == 10");
static_assert(sizeof(NetPacket_Fixed<0x3824>) == 34, "sizeof(NetPacket_Fixed<0x3824>) == 34");

template<>
struct NetPacket_Fixed<0x3825>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Little32 account_id;
    NetString<sizeof(MapName)> map_name;
    Byte online;
    Little16 level;
};
static_assert(offsetof(NetPacket_Fixed<0x3825>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3825>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3825>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3825>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3825>, account_id) == 6, "offsetof(NetPacket_Fixed<0x3825>, account_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x3825>, map_name) == 10, "offsetof(NetPacket_Fixed<0x3825>, map_name) == 10");
static_assert(offsetof(NetPacket_Fixed<0x3825>, online) == 26, "offsetof(NetPacket_Fixed<0x3825>, online) == 26");
static_assert(offsetof(NetPacket_Fixed<0x3825>, level) == 27, "offsetof(NetPacket_Fixed<0x3825>, level) == 27");
static_assert(sizeof(NetPacket_Fixed<0x3825>) == 29, "sizeof(NetPacket_Fixed<0x3825>) == 29");

template<>
struct NetPacket_Fixed<0x3826>
{
    Little16 magic_packet_id;
    Little32 party_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x3826>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x3826>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x3826>, party_id) == 2, "offsetof(NetPacket_Fixed<0x3826>, party_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x3826>, flag) == 6, "offsetof(NetPacket_Fixed<0x3826>, flag) == 6");
static_assert(sizeof(NetPacket_Fixed<0x3826>) == 7, "sizeof(NetPacket_Fixed<0x3826>) == 7");

template<>
struct NetPacket_Head<0x3827>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 party_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x3827>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x3827>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x3827>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x3827>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x3827>, party_id) == 4, "offsetof(NetPacket_Head<0x3827>, party_id) == 4");
static_assert(offsetof(NetPacket_Head<0x3827>, account_id) == 8, "offsetof(NetPacket_Head<0x3827>, account_id) == 8");
static_assert(sizeof(NetPacket_Head<0x3827>) == 12, "sizeof(NetPacket_Head<0x3827>) == 12");
template<>
struct NetPacket_Repeat<0x3827>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x3827>, c) == 0, "offsetof(NetPacket_Repeat<0x3827>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x3827>) == 1, "sizeof(NetPacket_Repeat<0x3827>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2af7> *network, Packet_Fixed<0x2af7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2af7> *native, NetPacket_Fixed<0x2af7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2af8> *network, Packet_Fixed<0x2af8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_name, native.account_name);
    rv &= native_to_network(&network->account_pass, native.account_pass);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2af8> *native, NetPacket_Fixed<0x2af8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_name, network.account_name);
    rv &= network_to_native(&native->account_pass, network.account_pass);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2af9> *network, Packet_Fixed<0x2af9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->code, native.code);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2af9> *native, NetPacket_Fixed<0x2af9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->code, network.code);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2afa> *network, Packet_Head<0x2afa> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2afa> *native, NetPacket_Head<0x2afa> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2afa> *network, Packet_Repeat<0x2afa> native)
{
    bool rv = true;
    rv &= native_to_network(&network->map_name, native.map_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2afa> *native, NetPacket_Repeat<0x2afa> network)
{
    bool rv = true;
    rv &= network_to_native(&native->map_name, network.map_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2afa> *network, Packet_Fixed<0x2afa> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->source_item_id, native.source_item_id);
    rv &= native_to_network(&network->dest_item_id, native.dest_item_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2afa> *native, NetPacket_Fixed<0x2afa> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->source_item_id, network.source_item_id);
    rv &= network_to_native(&native->dest_item_id, network.dest_item_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2afb> *network, Packet_Fixed<0x2afb> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unknown, native.unknown);
    rv &= native_to_network(&network->whisper_name, native.whisper_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2afb> *native, NetPacket_Fixed<0x2afb> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unknown, network.unknown);
    rv &= network_to_native(&native->whisper_name, network.whisper_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2afc> *network, Packet_Fixed<0x2afc> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->ip, native.ip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2afc> *native, NetPacket_Fixed<0x2afc> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->ip, network.ip);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Payload<0x2afd> *network, Packet_Payload<0x2afd> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->connect_until, native.connect_until);
    rv &= native_to_network(&network->packet_tmw_version, native.packet_tmw_version);
    rv &= native_to_network(&network->char_key, native.char_key);
    rv &= native_to_network(&network->char_data, native.char_data);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Payload<0x2afd> *native, NetPacket_Payload<0x2afd> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->connect_until, network.connect_until);
    rv &= network_to_native(&native->packet_tmw_version, network.packet_tmw_version);
    rv &= network_to_native(&native->char_key, network.char_key);
    rv &= network_to_native(&native->char_data, network.char_data);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2afe> *network, Packet_Fixed<0x2afe> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2afe> *native, NetPacket_Fixed<0x2afe> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2aff> *network, Packet_Head<0x2aff> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->users, native.users);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2aff> *native, NetPacket_Head<0x2aff> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->users, network.users);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2aff> *network, Packet_Repeat<0x2aff> native)
{
    bool rv = true;
    rv &= native_to_network(&network->char_id, native.char_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2aff> *native, NetPacket_Repeat<0x2aff> network)
{
    bool rv = true;
    rv &= network_to_native(&native->char_id, network.char_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b00> *network, Packet_Fixed<0x2b00> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->users, native.users);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b00> *native, NetPacket_Fixed<0x2b00> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->users, network.users);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Payload<0x2b01> *network, Packet_Payload<0x2b01> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->char_key, native.char_key);
    rv &= native_to_network(&network->char_data, native.char_data);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Payload<0x2b01> *native, NetPacket_Payload<0x2b01> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->char_key, network.char_key);
    rv &= network_to_native(&native->char_data, network.char_data);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b02> *network, Packet_Fixed<0x2b02> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->ip, native.ip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b02> *native, NetPacket_Fixed<0x2b02> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->ip, network.ip);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b03> *network, Packet_Fixed<0x2b03> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->unknown, native.unknown);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b03> *native, NetPacket_Fixed<0x2b03> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->unknown, network.unknown);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2b04> *network, Packet_Head<0x2b04> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2b04> *native, NetPacket_Head<0x2b04> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2b04> *network, Packet_Repeat<0x2b04> native)
{
    bool rv = true;
    rv &= native_to_network(&network->map_name, native.map_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2b04> *native, NetPacket_Repeat<0x2b04> network)
{
    bool rv = true;
    rv &= network_to_native(&native->map_name, network.map_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b05> *network, Packet_Fixed<0x2b05> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->login_id2, native.login_id2);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    rv &= native_to_network(&network->map_ip, native.map_ip);
    rv &= native_to_network(&network->map_port, native.map_port);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->client_ip, native.client_ip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b05> *native, NetPacket_Fixed<0x2b05> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->login_id2, network.login_id2);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    rv &= network_to_native(&native->map_ip, network.map_ip);
    rv &= network_to_native(&native->map_port, network.map_port);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->client_ip, network.client_ip);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b06> *network, Packet_Fixed<0x2b06> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->error, native.error);
    rv &= native_to_network(&network->unknown, native.unknown);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    rv &= native_to_network(&network->map_ip, native.map_ip);
    rv &= native_to_network(&network->map_port, native.map_port);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b06> *native, NetPacket_Fixed<0x2b06> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->error, network.error);
    rv &= network_to_native(&native->unknown, network.unknown);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    rv &= network_to_native(&native->map_ip, network.map_ip);
    rv &= network_to_native(&native->map_port, network.map_port);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2b0a> *network, Packet_Head<0x2b0a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2b0a> *native, NetPacket_Head<0x2b0a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2b0a> *network, Packet_Repeat<0x2b0a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2b0a> *native, NetPacket_Repeat<0x2b0a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b0b> *network, Packet_Fixed<0x2b0b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b0b> *native, NetPacket_Fixed<0x2b0b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b0c> *network, Packet_Fixed<0x2b0c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->old_email, native.old_email);
    rv &= native_to_network(&network->new_email, native.new_email);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b0c> *native, NetPacket_Fixed<0x2b0c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->old_email, network.old_email);
    rv &= network_to_native(&native->new_email, network.new_email);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b0d> *network, Packet_Fixed<0x2b0d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b0d> *native, NetPacket_Fixed<0x2b0d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b0e> *network, Packet_Fixed<0x2b0e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->operation, native.operation);
    rv &= native_to_network(&network->ban_add, native.ban_add);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b0e> *native, NetPacket_Fixed<0x2b0e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->operation, network.operation);
    rv &= network_to_native(&native->ban_add, network.ban_add);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b0f> *network, Packet_Fixed<0x2b0f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->operation, native.operation);
    rv &= native_to_network(&network->error, native.error);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b0f> *native, NetPacket_Fixed<0x2b0f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->operation, network.operation);
    rv &= network_to_native(&native->error, network.error);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2b10> *network, Packet_Head<0x2b10> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2b10> *native, NetPacket_Head<0x2b10> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2b10> *network, Packet_Repeat<0x2b10> native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2b10> *native, NetPacket_Repeat<0x2b10> network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2b11> *network, Packet_Head<0x2b11> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2b11> *native, NetPacket_Head<0x2b11> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2b11> *network, Packet_Repeat<0x2b11> native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2b11> *native, NetPacket_Repeat<0x2b11> network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b12> *network, Packet_Fixed<0x2b12> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->partner_id, native.partner_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b12> *native, NetPacket_Fixed<0x2b12> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->partner_id, network.partner_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b13> *network, Packet_Fixed<0x2b13> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b13> *native, NetPacket_Fixed<0x2b13> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b14> *network, Packet_Fixed<0x2b14> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->ban_not_status, native.ban_not_status);
    rv &= native_to_network(&network->status_or_ban_until, native.status_or_ban_until);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b14> *native, NetPacket_Fixed<0x2b14> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->ban_not_status, network.ban_not_status);
    rv &= network_to_native(&native->status_or_ban_until, network.status_or_ban_until);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x2b15> *network, Packet_Head<0x2b15> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x2b15> *native, NetPacket_Head<0x2b15> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x2b15> *network, Packet_Repeat<0x2b15> native)
{
    bool rv = true;
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->gm_level, native.gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x2b15> *native, NetPacket_Repeat<0x2b15> network)
{
    bool rv = true;
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->gm_level, network.gm_level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x2b16> *network, Packet_Fixed<0x2b16> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x2b16> *native, NetPacket_Fixed<0x2b16> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3000> *network, Packet_Head<0x3000> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3000> *native, NetPacket_Head<0x3000> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3000> *network, Packet_Repeat<0x3000> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3000> *native, NetPacket_Repeat<0x3000> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3001> *network, Packet_Head<0x3001> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->from_char_name, native.from_char_name);
    rv &= native_to_network(&network->to_char_name, native.to_char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3001> *native, NetPacket_Head<0x3001> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->from_char_name, network.from_char_name);
    rv &= network_to_native(&native->to_char_name, network.to_char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3001> *network, Packet_Repeat<0x3001> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3001> *native, NetPacket_Repeat<0x3001> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3002> *network, Packet_Fixed<0x3002> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3002> *native, NetPacket_Fixed<0x3002> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3003> *network, Packet_Head<0x3003> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->min_gm_level, native.min_gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3003> *native, NetPacket_Head<0x3003> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->min_gm_level, network.min_gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3003> *network, Packet_Repeat<0x3003> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3003> *native, NetPacket_Repeat<0x3003> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3004> *network, Packet_Head<0x3004> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3004> *native, NetPacket_Head<0x3004> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3004> *network, Packet_Repeat<0x3004> native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3004> *native, NetPacket_Repeat<0x3004> network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3005> *network, Packet_Fixed<0x3005> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3005> *native, NetPacket_Fixed<0x3005> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3010> *network, Packet_Fixed<0x3010> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3010> *native, NetPacket_Fixed<0x3010> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Payload<0x3011> *network, Packet_Payload<0x3011> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->storage, native.storage);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Payload<0x3011> *native, NetPacket_Payload<0x3011> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->storage, network.storage);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3020> *network, Packet_Fixed<0x3020> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->party_name, native.party_name);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->level, native.level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3020> *native, NetPacket_Fixed<0x3020> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->party_name, network.party_name);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->level, network.level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3021> *network, Packet_Fixed<0x3021> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3021> *native, NetPacket_Fixed<0x3021> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3022> *network, Packet_Fixed<0x3022> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->level, native.level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3022> *native, NetPacket_Fixed<0x3022> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->level, network.level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3023> *network, Packet_Fixed<0x3023> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->exp, native.exp);
    rv &= native_to_network(&network->item, native.item);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3023> *native, NetPacket_Fixed<0x3023> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->exp, network.exp);
    rv &= network_to_native(&native->item, network.item);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3024> *network, Packet_Fixed<0x3024> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3024> *native, NetPacket_Fixed<0x3024> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3025> *network, Packet_Fixed<0x3025> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->online, native.online);
    rv &= native_to_network(&network->level, native.level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3025> *native, NetPacket_Fixed<0x3025> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->online, network.online);
    rv &= network_to_native(&native->level, network.level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3026> *network, Packet_Fixed<0x3026> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3026> *native, NetPacket_Fixed<0x3026> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3027> *network, Packet_Head<0x3027> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3027> *native, NetPacket_Head<0x3027> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3027> *network, Packet_Repeat<0x3027> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3027> *native, NetPacket_Repeat<0x3027> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3028> *network, Packet_Fixed<0x3028> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3028> *native, NetPacket_Fixed<0x3028> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3800> *network, Packet_Head<0x3800> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3800> *native, NetPacket_Head<0x3800> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3800> *network, Packet_Repeat<0x3800> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3800> *native, NetPacket_Repeat<0x3800> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3801> *network, Packet_Head<0x3801> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->whisper_id, native.whisper_id);
    rv &= native_to_network(&network->src_char_name, native.src_char_name);
    rv &= native_to_network(&network->dst_char_name, native.dst_char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3801> *native, NetPacket_Head<0x3801> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->whisper_id, network.whisper_id);
    rv &= network_to_native(&native->src_char_name, network.src_char_name);
    rv &= network_to_native(&native->dst_char_name, network.dst_char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3801> *network, Packet_Repeat<0x3801> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3801> *native, NetPacket_Repeat<0x3801> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3802> *network, Packet_Fixed<0x3802> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sender_char_name, native.sender_char_name);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3802> *native, NetPacket_Fixed<0x3802> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sender_char_name, network.sender_char_name);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3803> *network, Packet_Head<0x3803> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->min_gm_level, native.min_gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3803> *native, NetPacket_Head<0x3803> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->min_gm_level, network.min_gm_level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3803> *network, Packet_Repeat<0x3803> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3803> *native, NetPacket_Repeat<0x3803> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3804> *network, Packet_Head<0x3804> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3804> *native, NetPacket_Head<0x3804> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3804> *network, Packet_Repeat<0x3804> native)
{
    bool rv = true;
    rv &= native_to_network(&network->name, native.name);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3804> *native, NetPacket_Repeat<0x3804> network)
{
    bool rv = true;
    rv &= network_to_native(&native->name, network.name);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Payload<0x3810> *network, Packet_Payload<0x3810> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->storage, native.storage);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Payload<0x3810> *native, NetPacket_Payload<0x3810> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->storage, network.storage);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3811> *network, Packet_Fixed<0x3811> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->unknown, native.unknown);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3811> *native, NetPacket_Fixed<0x3811> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->unknown, network.unknown);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3820> *network, Packet_Fixed<0x3820> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->error, native.error);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->party_name, native.party_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3820> *native, NetPacket_Fixed<0x3820> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->error, network.error);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->party_name, network.party_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3821> *network, Packet_Head<0x3821> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->party_id, native.party_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3821> *native, NetPacket_Head<0x3821> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->party_id, network.party_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Option<0x3821> *network, Packet_Option<0x3821> native)
{
    bool rv = true;
    rv &= native_to_network(&network->party_most, native.party_most);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Option<0x3821> *native, NetPacket_Option<0x3821> network)
{
    bool rv = true;
    rv &= network_to_native(&native->party_most, network.party_most);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3822> *network, Packet_Fixed<0x3822> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3822> *native, NetPacket_Fixed<0x3822> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3823> *network, Packet_Fixed<0x3823> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->exp, native.exp);
    rv &= native_to_network(&network->item, native.item);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3823> *native, NetPacket_Fixed<0x3823> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->exp, network.exp);
    rv &= network_to_native(&native->item, network.item);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3824> *network, Packet_Fixed<0x3824> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3824> *native, NetPacket_Fixed<0x3824> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3825> *network, Packet_Fixed<0x3825> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->online, native.online);
    rv &= native_to_network(&network->level, native.level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3825> *native, NetPacket_Fixed<0x3825> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->online, network.online);
    rv &= network_to_native(&native->level, network.level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x3826> *network, Packet_Fixed<0x3826> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x3826> *native, NetPacket_Fixed<0x3826> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x3827> *network, Packet_Head<0x3827> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->party_id, native.party_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x3827> *native, NetPacket_Head<0x3827> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->party_id, network.party_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x3827> *network, Packet_Repeat<0x3827> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x3827> *native, NetPacket_Repeat<0x3827> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}


#pragma pack(pop)

#endif // TMWA_PROTO2_CHAR_MAP_HPP
