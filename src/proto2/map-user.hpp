#ifndef TMWA_PROTO2_MAP_USER_HPP
#define TMWA_PROTO2_MAP_USER_HPP
//    map-user.hpp - TMWA network protocol: map/user
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
struct Packet_Fixed<0x0072>
{
    static const uint16_t PACKET_ID = 0x0072;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    CharId char_id = {};
    uint32_t login_id1 = {};
    uint32_t client_tick = {};
    SEX sex = {};
};

template<>
struct Packet_Fixed<0x0073>
{
    static const uint16_t PACKET_ID = 0x0073;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    tick_t tick = {};
    Position1 pos = {};
    uint8_t five1 = {};
    uint8_t five2 = {};
};

template<>
struct Packet_Fixed<0x0078>
{
    static const uint16_t PACKET_ID = 0x0078;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    interval_t speed = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    Species species = {};
    uint16_t unused_hair_style = {};
    uint16_t unused_weapon = {};
    uint16_t unused_head_bottom_or_species_again = {};
    uint16_t unused_shield_or_part_of_guild_emblem = {};
    uint16_t unused_head_top_or_unused_part_of_guild_emblem = {};
    uint16_t unused_head_mid_or_part_of_guild_id = {};
    uint16_t unused_hair_color_or_part_of_guild_id = {};
    uint16_t unused_clothes_color = {};
    uint16_t unused_1 = {};
    uint16_t unused_2 = {};
    Position1 unused_pos_again = {};
    uint8_t unused_4b = {};
    uint16_t unused_5 = {};
    uint16_t unused_zero_1 = {};
    uint8_t unused_zero_2 = {};
    uint8_t unused_sex = {};
    Position1 pos = {};
    uint8_t five1 = {};
    uint8_t five2 = {};
    uint8_t zero = {};
    uint16_t level = {};
};

template<>
struct Packet_Fixed<0x007b>
{
    static const uint16_t PACKET_ID = 0x007b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    interval_t speed = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    Species mob_class = {};
    uint16_t unused_hair_style = {};
    uint16_t unused_weapon = {};
    uint16_t unused_head_bottom = {};
    tick_t tick_and_maybe_part_of_guild_emblem = {};
    uint16_t unused_shield_or_maybe_part_of_guild_emblem = {};
    uint16_t unused_head_top_or_maybe_part_of_guild_id = {};
    uint16_t unused_head_mid_or_maybe_part_of_guild_id = {};
    uint16_t unused_hair_color = {};
    uint16_t unused_clothes_color = {};
    uint16_t unused_1 = {};
    uint16_t unused_2 = {};
    uint16_t unused_3 = {};
    uint16_t unused_4 = {};
    uint16_t unused_5 = {};
    uint16_t unused_zero_1 = {};
    uint8_t unused_zero_2 = {};
    uint8_t unused_sex = {};
    Position2 pos2 = {};
    uint8_t zero = {};
    uint8_t five1 = {};
    uint8_t five2 = {};
    uint16_t level = {};
};

template<>
struct Packet_Fixed<0x007c>
{
    static const uint16_t PACKET_ID = 0x007c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    interval_t speed = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    uint16_t unknown_1 = {};
    uint16_t unknown_2 = {};
    uint16_t unknown_3 = {};
    Species species = {};
    uint16_t unknown_4 = {};
    uint16_t unknown_5 = {};
    uint16_t unknown_6 = {};
    uint16_t unknown_7 = {};
    uint16_t unknown_8 = {};
    uint16_t unknown_9 = {};
    uint16_t unknown_10 = {};
    Position1 pos = {};
    uint16_t unknown_11 = {};
};

template<>
struct Packet_Fixed<0x007d>
{
    static const uint16_t PACKET_ID = 0x007d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x007e>
{
    static const uint16_t PACKET_ID = 0x007e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint32_t client_tick = {};
};

template<>
struct Packet_Fixed<0x007f>
{
    static const uint16_t PACKET_ID = 0x007f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    tick_t tick = {};
};

template<>
struct Packet_Fixed<0x0080>
{
    static const uint16_t PACKET_ID = 0x0080;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    BeingRemoveWhy type = {};
};

template<>
struct Packet_Fixed<0x0085>
{
    static const uint16_t PACKET_ID = 0x0085;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    Position1 pos = {};
};

template<>
struct Packet_Fixed<0x0087>
{
    static const uint16_t PACKET_ID = 0x0087;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    tick_t tick = {};
    Position2 pos2 = {};
    uint8_t zero = {};
};

template<>
struct Packet_Fixed<0x0088>
{
    static const uint16_t PACKET_ID = 0x0088;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint16_t x = {};
    uint16_t y = {};
};

template<>
struct Packet_Fixed<0x0089>
{
    static const uint16_t PACKET_ID = 0x0089;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId target_id = {};
    DamageType action = {};
};

template<>
struct Packet_Fixed<0x008a>
{
    static const uint16_t PACKET_ID = 0x008a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId src_id = {};
    BlockId dst_id = {};
    tick_t tick = {};
    interval_t sdelay = {};
    interval_t ddelay = {};
    uint16_t damage = {};
    uint16_t div = {};
    DamageType damage_type = {};
    uint16_t damage2 = {};
};

template<>
struct Packet_Head<0x008c>
{
    static const uint16_t PACKET_ID = 0x008c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x008c>
{
    static const uint16_t PACKET_ID = 0x008c;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x008d>
{
    static const uint16_t PACKET_ID = 0x008d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    BlockId block_id = {};
};
template<>
struct Packet_Repeat<0x008d>
{
    static const uint16_t PACKET_ID = 0x008d;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x008e>
{
    static const uint16_t PACKET_ID = 0x008e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x008e>
{
    static const uint16_t PACKET_ID = 0x008e;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x0090>
{
    static const uint16_t PACKET_ID = 0x0090;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint8_t unused = {};
};

template<>
struct Packet_Fixed<0x0091>
{
    static const uint16_t PACKET_ID = 0x0091;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    MapName map_name = {};
    uint16_t x = {};
    uint16_t y = {};
};

template<>
struct Packet_Fixed<0x0092>
{
    static const uint16_t PACKET_ID = 0x0092;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    MapName map_name = {};
    uint16_t x = {};
    uint16_t y = {};
    IP4Address ip = {};
    uint16_t port = {};
};

template<>
struct Packet_Fixed<0x0094>
{
    static const uint16_t PACKET_ID = 0x0094;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x0095>
{
    static const uint16_t PACKET_ID = 0x0095;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    CharName char_name = {};
};

template<>
struct Packet_Head<0x0096>
{
    static const uint16_t PACKET_ID = 0x0096;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    CharName target_name = {};
};
template<>
struct Packet_Repeat<0x0096>
{
    static const uint16_t PACKET_ID = 0x0096;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x0097>
{
    static const uint16_t PACKET_ID = 0x0097;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    CharName char_name = {};
};
template<>
struct Packet_Repeat<0x0097>
{
    static const uint16_t PACKET_ID = 0x0097;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x0098>
{
    static const uint16_t PACKET_ID = 0x0098;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t flag = {};
};

template<>
struct Packet_Head<0x009a>
{
    static const uint16_t PACKET_ID = 0x009a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x009a>
{
    static const uint16_t PACKET_ID = 0x009a;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x009b>
{
    static const uint16_t PACKET_ID = 0x009b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t unused = {};
    uint8_t client_dir = {};
};

template<>
struct Packet_Fixed<0x009c>
{
    static const uint16_t PACKET_ID = 0x009c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint16_t zero = {};
    uint8_t client_dir = {};
};

template<>
struct Packet_Fixed<0x009d>
{
    static const uint16_t PACKET_ID = 0x009d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    ItemNameId name_id = {};
    uint8_t identify = {};
    uint16_t x = {};
    uint16_t y = {};
    uint16_t amount = {};
    uint8_t subx = {};
    uint8_t suby = {};
};

template<>
struct Packet_Fixed<0x009e>
{
    static const uint16_t PACKET_ID = 0x009e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    ItemNameId name_id = {};
    uint8_t identify = {};
    uint16_t x = {};
    uint16_t y = {};
    uint8_t subx = {};
    uint8_t suby = {};
    uint16_t amount = {};
};

template<>
struct Packet_Fixed<0x009f>
{
    static const uint16_t PACKET_ID = 0x009f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId object_id = {};
};

template<>
struct Packet_Fixed<0x00a0>
{
    static const uint16_t PACKET_ID = 0x00a0;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint16_t amount = {};
    ItemNameId name_id = {};
    uint8_t identify = {};
    uint8_t broken_or_attribute = {};
    uint8_t refine = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
    EPOS epos = {};
    ItemType item_type = {};
    PickupFail pickup_fail = {};
};

template<>
struct Packet_Fixed<0x00a1>
{
    static const uint16_t PACKET_ID = 0x00a1;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x00a2>
{
    static const uint16_t PACKET_ID = 0x00a2;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint16_t amount = {};
};

template<>
struct Packet_Head<0x00a4>
{
    static const uint16_t PACKET_ID = 0x00a4;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x00a4>
{
    static const uint16_t PACKET_ID = 0x00a4;

    uint16_t ioff2 = {};
    ItemNameId name_id = {};
    ItemType item_type = {};
    uint8_t identify = {};
    EPOS epos_pc = {};
    EPOS epos_inv = {};
    uint8_t broken_or_attribute = {};
    uint8_t refine = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
};

template<>
struct Packet_Head<0x00a6>
{
    static const uint16_t PACKET_ID = 0x00a6;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x00a6>
{
    static const uint16_t PACKET_ID = 0x00a6;

    uint16_t soff1 = {};
    ItemNameId name_id = {};
    ItemType item_type = {};
    uint8_t identify = {};
    EPOS epos_id = {};
    EPOS epos_stor = {};
    uint8_t broken_or_attribute = {};
    uint8_t refine = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
};

template<>
struct Packet_Fixed<0x00a7>
{
    static const uint16_t PACKET_ID = 0x00a7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint32_t unused_id = {};
};

template<>
struct Packet_Fixed<0x00a8>
{
    static const uint16_t PACKET_ID = 0x00a8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint16_t amount = {};
    uint8_t ok = {};
};

template<>
struct Packet_Fixed<0x00a9>
{
    static const uint16_t PACKET_ID = 0x00a9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    EPOS epos_ignored = {};
};

template<>
struct Packet_Fixed<0x00aa>
{
    static const uint16_t PACKET_ID = 0x00aa;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    EPOS epos = {};
    uint8_t ok = {};
};

template<>
struct Packet_Fixed<0x00ab>
{
    static const uint16_t PACKET_ID = 0x00ab;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
};

template<>
struct Packet_Fixed<0x00ac>
{
    static const uint16_t PACKET_ID = 0x00ac;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    EPOS epos = {};
    uint8_t ok = {};
};

template<>
struct Packet_Fixed<0x00af>
{
    static const uint16_t PACKET_ID = 0x00af;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint16_t amount = {};
};

template<>
struct Packet_Fixed<0x00b0>
{
    static const uint16_t PACKET_ID = 0x00b0;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SP sp_type = {};
    uint32_t value = {};
};

template<>
struct Packet_Fixed<0x00b1>
{
    static const uint16_t PACKET_ID = 0x00b1;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SP sp_type = {};
    uint32_t value = {};
};

template<>
struct Packet_Fixed<0x00b2>
{
    static const uint16_t PACKET_ID = 0x00b2;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t flag = {};
};

template<>
struct Packet_Fixed<0x00b3>
{
    static const uint16_t PACKET_ID = 0x00b3;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t one = {};
};

template<>
struct Packet_Head<0x00b4>
{
    static const uint16_t PACKET_ID = 0x00b4;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    BlockId block_id = {};
};
template<>
struct Packet_Repeat<0x00b4>
{
    static const uint16_t PACKET_ID = 0x00b4;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x00b5>
{
    static const uint16_t PACKET_ID = 0x00b5;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x00b6>
{
    static const uint16_t PACKET_ID = 0x00b6;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Head<0x00b7>
{
    static const uint16_t PACKET_ID = 0x00b7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    BlockId block_id = {};
};
template<>
struct Packet_Repeat<0x00b7>
{
    static const uint16_t PACKET_ID = 0x00b7;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x00b8>
{
    static const uint16_t PACKET_ID = 0x00b8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId npc_id = {};
    uint8_t menu_entry = {};
};

template<>
struct Packet_Fixed<0x00b9>
{
    static const uint16_t PACKET_ID = 0x00b9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId npc_id = {};
};

template<>
struct Packet_Fixed<0x00bb>
{
    static const uint16_t PACKET_ID = 0x00bb;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SP asp = {};
    uint8_t unused = {};
};

template<>
struct Packet_Fixed<0x00bc>
{
    static const uint16_t PACKET_ID = 0x00bc;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SP sp_type = {};
    uint8_t ok = {};
    uint8_t val = {};
};

template<>
struct Packet_Fixed<0x00bd>
{
    static const uint16_t PACKET_ID = 0x00bd;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t status_point = {};
    uint8_t str_attr = {};
    uint8_t str_upd = {};
    uint8_t agi_attr = {};
    uint8_t agi_upd = {};
    uint8_t vit_attr = {};
    uint8_t vit_upd = {};
    uint8_t int_attr = {};
    uint8_t int_upd = {};
    uint8_t dex_attr = {};
    uint8_t dex_upd = {};
    uint8_t luk_attr = {};
    uint8_t luk_upd = {};
    uint16_t atk_sum = {};
    uint16_t watk2 = {};
    uint16_t matk1 = {};
    uint16_t matk2 = {};
    uint16_t def = {};
    uint16_t def2 = {};
    uint16_t mdef = {};
    uint16_t mdef2 = {};
    uint16_t hit = {};
    uint16_t flee = {};
    uint16_t flee2 = {};
    uint16_t critical = {};
    uint16_t karma = {};
    uint16_t manner = {};
};

template<>
struct Packet_Fixed<0x00be>
{
    static const uint16_t PACKET_ID = 0x00be;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SP sp_type = {};
    uint8_t value = {};
};

template<>
struct Packet_Fixed<0x00bf>
{
    static const uint16_t PACKET_ID = 0x00bf;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t emote = {};
};

template<>
struct Packet_Fixed<0x00c0>
{
    static const uint16_t PACKET_ID = 0x00c0;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint8_t type = {};
};

template<>
struct Packet_Fixed<0x00c1>
{
    static const uint16_t PACKET_ID = 0x00c1;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00c2>
{
    static const uint16_t PACKET_ID = 0x00c2;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint32_t users = {};
};

template<>
struct Packet_Fixed<0x00c4>
{
    static const uint16_t PACKET_ID = 0x00c4;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x00c5>
{
    static const uint16_t PACKET_ID = 0x00c5;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint8_t type = {};
};

template<>
struct Packet_Head<0x00c6>
{
    static const uint16_t PACKET_ID = 0x00c6;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x00c6>
{
    static const uint16_t PACKET_ID = 0x00c6;

    uint32_t base_price = {};
    uint32_t actual_price = {};
    ItemType type = {};
    ItemNameId name_id = {};
};

template<>
struct Packet_Head<0x00c7>
{
    static const uint16_t PACKET_ID = 0x00c7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x00c7>
{
    static const uint16_t PACKET_ID = 0x00c7;

    uint16_t ioff2 = {};
    uint32_t base_price = {};
    uint32_t actual_price = {};
};

template<>
struct Packet_Head<0x00c8>
{
    static const uint16_t PACKET_ID = 0x00c8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x00c8>
{
    static const uint16_t PACKET_ID = 0x00c8;

    uint16_t count = {};
    ItemNameId name_id = {};
};

template<>
struct Packet_Head<0x00c9>
{
    static const uint16_t PACKET_ID = 0x00c9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x00c9>
{
    static const uint16_t PACKET_ID = 0x00c9;

    uint16_t ioff2 = {};
    uint16_t count = {};
};

template<>
struct Packet_Fixed<0x00ca>
{
    static const uint16_t PACKET_ID = 0x00ca;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t fail = {};
};

template<>
struct Packet_Fixed<0x00cb>
{
    static const uint16_t PACKET_ID = 0x00cb;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t fail = {};
};

template<>
struct Packet_Fixed<0x00cd>
{
    static const uint16_t PACKET_ID = 0x00cd;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x00e4>
{
    static const uint16_t PACKET_ID = 0x00e4;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x00e5>
{
    static const uint16_t PACKET_ID = 0x00e5;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharName char_name = {};
};

template<>
struct Packet_Fixed<0x00e6>
{
    static const uint16_t PACKET_ID = 0x00e6;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t type = {};
};

template<>
struct Packet_Fixed<0x00e7>
{
    static const uint16_t PACKET_ID = 0x00e7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t type = {};
};

template<>
struct Packet_Fixed<0x00e8>
{
    static const uint16_t PACKET_ID = 0x00e8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t zeny_or_ioff2 = {};
    uint32_t amount = {};
};

template<>
struct Packet_Fixed<0x00e9>
{
    static const uint16_t PACKET_ID = 0x00e9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint32_t amount = {};
    ItemNameId name_id = {};
    uint8_t identify = {};
    uint8_t broken_or_attribute = {};
    uint8_t refine = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
};

template<>
struct Packet_Fixed<0x00eb>
{
    static const uint16_t PACKET_ID = 0x00eb;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00ec>
{
    static const uint16_t PACKET_ID = 0x00ec;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t fail = {};
};

template<>
struct Packet_Fixed<0x00ed>
{
    static const uint16_t PACKET_ID = 0x00ed;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00ee>
{
    static const uint16_t PACKET_ID = 0x00ee;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00ef>
{
    static const uint16_t PACKET_ID = 0x00ef;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00f0>
{
    static const uint16_t PACKET_ID = 0x00f0;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t fail = {};
};

template<>
struct Packet_Fixed<0x00f2>
{
    static const uint16_t PACKET_ID = 0x00f2;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t current_slots = {};
    uint16_t max_slots = {};
};

template<>
struct Packet_Fixed<0x00f3>
{
    static const uint16_t PACKET_ID = 0x00f3;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint32_t amount = {};
};

template<>
struct Packet_Fixed<0x00f4>
{
    static const uint16_t PACKET_ID = 0x00f4;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t soff1 = {};
    uint32_t amount = {};
    ItemNameId name_id = {};
    uint8_t identify = {};
    uint8_t broken_or_attribute = {};
    uint8_t refine = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
};

template<>
struct Packet_Fixed<0x00f5>
{
    static const uint16_t PACKET_ID = 0x00f5;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t soff1 = {};
    uint32_t amount = {};
};

template<>
struct Packet_Fixed<0x00f6>
{
    static const uint16_t PACKET_ID = 0x00f6;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t soff1 = {};
    uint32_t amount = {};
};

template<>
struct Packet_Fixed<0x00f7>
{
    static const uint16_t PACKET_ID = 0x00f7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00f8>
{
    static const uint16_t PACKET_ID = 0x00f8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x00f9>
{
    static const uint16_t PACKET_ID = 0x00f9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    PartyName party_name = {};
};

template<>
struct Packet_Fixed<0x00fa>
{
    static const uint16_t PACKET_ID = 0x00fa;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint8_t flag = {};
};

template<>
struct Packet_Head<0x00fb>
{
    static const uint16_t PACKET_ID = 0x00fb;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    PartyName party_name = {};
};
template<>
struct Packet_Repeat<0x00fb>
{
    static const uint16_t PACKET_ID = 0x00fb;

    AccountId account_id = {};
    CharName char_name = {};
    MapName map_name = {};
    uint8_t leader = {};
    uint8_t online = {};
};

template<>
struct Packet_Fixed<0x00fc>
{
    static const uint16_t PACKET_ID = 0x00fc;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
};

template<>
struct Packet_Fixed<0x00fd>
{
    static const uint16_t PACKET_ID = 0x00fd;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    CharName char_name = {};
    uint8_t flag = {};
};

template<>
struct Packet_Fixed<0x00fe>
{
    static const uint16_t PACKET_ID = 0x00fe;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    PartyName party_name = {};
};

template<>
struct Packet_Fixed<0x00ff>
{
    static const uint16_t PACKET_ID = 0x00ff;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint32_t flag = {};
};

template<>
struct Packet_Fixed<0x0100>
{
    static const uint16_t PACKET_ID = 0x0100;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x0101>
{
    static const uint16_t PACKET_ID = 0x0101;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t exp = {};
    uint16_t item = {};
};

template<>
struct Packet_Fixed<0x0102>
{
    static const uint16_t PACKET_ID = 0x0102;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t exp = {};
    uint16_t item = {};
};

template<>
struct Packet_Fixed<0x0103>
{
    static const uint16_t PACKET_ID = 0x0103;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    CharName unused_char_name = {};
};

template<>
struct Packet_Fixed<0x0105>
{
    static const uint16_t PACKET_ID = 0x0105;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    CharName char_name = {};
    uint8_t flag = {};
};

template<>
struct Packet_Fixed<0x0106>
{
    static const uint16_t PACKET_ID = 0x0106;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint16_t hp = {};
    uint16_t max_hp = {};
};

template<>
struct Packet_Fixed<0x0107>
{
    static const uint16_t PACKET_ID = 0x0107;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    AccountId account_id = {};
    uint16_t x = {};
    uint16_t y = {};
};

template<>
struct Packet_Head<0x0108>
{
    static const uint16_t PACKET_ID = 0x0108;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x0108>
{
    static const uint16_t PACKET_ID = 0x0108;

    uint8_t c = {};
};

template<>
struct Packet_Head<0x0109>
{
    static const uint16_t PACKET_ID = 0x0109;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    AccountId account_id = {};
};
template<>
struct Packet_Repeat<0x0109>
{
    static const uint16_t PACKET_ID = 0x0109;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x010c>
{
    static const uint16_t PACKET_ID = 0x010c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x010e>
{
    static const uint16_t PACKET_ID = 0x010e;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SkillID skill_id = {};
    uint16_t level = {};
    uint16_t sp = {};
    uint16_t range = {};
    uint8_t can_raise = {};
};

template<>
struct Packet_Head<0x010f>
{
    static const uint16_t PACKET_ID = 0x010f;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x010f>
{
    static const uint16_t PACKET_ID = 0x010f;

    SkillInfo info = {};
};

template<>
struct Packet_Fixed<0x0110>
{
    static const uint16_t PACKET_ID = 0x0110;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SkillID skill_id = {};
    uint16_t btype = {};
    uint16_t zero1 = {};
    uint8_t zero2 = {};
    uint8_t type = {};
};

template<>
struct Packet_Fixed<0x0112>
{
    static const uint16_t PACKET_ID = 0x0112;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SkillID skill_id = {};
};

template<>
struct Packet_Fixed<0x0118>
{
    static const uint16_t PACKET_ID = 0x0118;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x0119>
{
    static const uint16_t PACKET_ID = 0x0119;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    uint8_t zero = {};
};

template<>
struct Packet_Fixed<0x0139>
{
    static const uint16_t PACKET_ID = 0x0139;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint16_t bl_x = {};
    uint16_t bl_y = {};
    uint16_t sd_x = {};
    uint16_t sd_y = {};
    uint16_t range = {};
};

template<>
struct Packet_Fixed<0x013a>
{
    static const uint16_t PACKET_ID = 0x013a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t attack_range = {};
};

template<>
struct Packet_Fixed<0x013b>
{
    static const uint16_t PACKET_ID = 0x013b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t type = {};
};

template<>
struct Packet_Fixed<0x013c>
{
    static const uint16_t PACKET_ID = 0x013c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
};

template<>
struct Packet_Fixed<0x0141>
{
    static const uint16_t PACKET_ID = 0x0141;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SP sp_type = {};
    uint16_t zero = {};
    uint32_t value_status = {};
    uint32_t value_b_e = {};
};

template<>
struct Packet_Fixed<0x0142>
{
    static const uint16_t PACKET_ID = 0x0142;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x0143>
{
    static const uint16_t PACKET_ID = 0x0143;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint32_t input_int_value = {};
};

template<>
struct Packet_Fixed<0x0146>
{
    static const uint16_t PACKET_ID = 0x0146;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Fixed<0x0147>
{
    static const uint16_t PACKET_ID = 0x0147;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SkillInfo info = {};
};

template<>
struct Packet_Fixed<0x0148>
{
    static const uint16_t PACKET_ID = 0x0148;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint16_t type = {};
};

template<>
struct Packet_Fixed<0x014d>
{
    static const uint16_t PACKET_ID = 0x014d;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
};

template<>
struct Packet_Fixed<0x018a>
{
    static const uint16_t PACKET_ID = 0x018a;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t unused = {};
};

template<>
struct Packet_Fixed<0x018b>
{
    static const uint16_t PACKET_ID = 0x018b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t okay = {};
};

template<>
struct Packet_Fixed<0x0195>
{
    static const uint16_t PACKET_ID = 0x0195;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    PartyName party_name = {};
    VString<23> guild_name = {};
    VString<23> guild_pos = {};
    VString<23> guild_pos_again = {};
};

template<>
struct Packet_Fixed<0x0196>
{
    static const uint16_t PACKET_ID = 0x0196;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    StatusChange sc_type = {};
    BlockId block_id = {};
    uint8_t flag = {};
};

template<>
struct Packet_Fixed<0x019b>
{
    static const uint16_t PACKET_ID = 0x019b;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    uint32_t type = {};
};

template<>
struct Packet_Fixed<0x01b1>
{
    static const uint16_t PACKET_ID = 0x01b1;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    uint16_t amount = {};
    uint8_t fail = {};
};

template<>
struct Packet_Fixed<0x01c8>
{
    static const uint16_t PACKET_ID = 0x01c8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    uint16_t ioff2 = {};
    ItemNameId name_id = {};
    BlockId block_id = {};
    uint16_t amount = {};
    uint8_t ok = {};
};

template<>
struct Packet_Fixed<0x01d4>
{
    static const uint16_t PACKET_ID = 0x01d4;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
};

template<>
struct Packet_Head<0x01d5>
{
    static const uint16_t PACKET_ID = 0x01d5;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
    BlockId block_id = {};
};
template<>
struct Packet_Repeat<0x01d5>
{
    static const uint16_t PACKET_ID = 0x01d5;

    uint8_t c = {};
};

template<>
struct Packet_Fixed<0x01d7>
{
    static const uint16_t PACKET_ID = 0x01d7;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    LOOK look_type = {};
    uint16_t weapon_or_name_id_or_value = {};
    ItemNameId shield = {};
};

template<>
struct Packet_Fixed<0x01d8>
{
    static const uint16_t PACKET_ID = 0x01d8;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    interval_t speed = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    Species species = {};
    uint16_t hair_style = {};
    ItemNameId weapon = {};
    ItemNameId shield = {};
    ItemNameId head_bottom = {};
    ItemNameId head_top = {};
    ItemNameId head_mid = {};
    uint16_t hair_color = {};
    uint16_t clothes_color = {};
    DIR head_dir = {};
    uint8_t unused2 = {};
    uint32_t guild_id = {};
    uint16_t guild_emblem_id = {};
    uint16_t manner = {};
    Opt3 opt3 = {};
    uint8_t karma = {};
    SEX sex = {};
    Position1 pos = {};
    uint16_t gm_bits = {};
    uint8_t dead_sit = {};
    uint16_t unused = {};
};

template<>
struct Packet_Fixed<0x01d9>
{
    static const uint16_t PACKET_ID = 0x01d9;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    interval_t speed = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    Species species = {};
    uint16_t hair_style = {};
    ItemNameId weapon = {};
    ItemNameId shield = {};
    ItemNameId head_bottom = {};
    ItemNameId head_top = {};
    ItemNameId head_mid = {};
    uint16_t hair_color = {};
    uint16_t clothes_color = {};
    DIR head_dir = {};
    uint8_t unused2 = {};
    uint32_t guild_id = {};
    uint16_t guild_emblem_id = {};
    uint16_t manner = {};
    Opt3 opt3 = {};
    uint8_t karma = {};
    SEX sex = {};
    Position1 pos = {};
    uint16_t gm_bits = {};
    uint16_t unused = {};
};

template<>
struct Packet_Fixed<0x01da>
{
    static const uint16_t PACKET_ID = 0x01da;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    interval_t speed = {};
    Opt1 opt1 = {};
    Opt2 opt2 = {};
    Option option = {};
    Species species = {};
    uint16_t hair_style = {};
    ItemNameId weapon = {};
    ItemNameId shield = {};
    ItemNameId head_bottom = {};
    tick_t tick = {};
    ItemNameId head_top = {};
    ItemNameId head_mid = {};
    uint16_t hair_color = {};
    uint16_t clothes_color = {};
    DIR head_dir = {};
    uint8_t unused2 = {};
    uint32_t guild_id = {};
    uint16_t guild_emblem_id = {};
    uint16_t manner = {};
    Opt3 opt3 = {};
    uint8_t karma = {};
    SEX sex = {};
    Position2 pos2 = {};
    uint16_t gm_bits = {};
    uint8_t five = {};
    uint16_t unused = {};
};

template<>
struct Packet_Fixed<0x01de>
{
    static const uint16_t PACKET_ID = 0x01de;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    SkillID skill_id = {};
    BlockId src_id = {};
    BlockId dst_id = {};
    tick_t tick = {};
    interval_t sdelay = {};
    interval_t ddelay = {};
    uint32_t damage = {};
    uint16_t skill_level = {};
    uint16_t div = {};
    uint8_t type_or_hit = {};
};

template<>
struct Packet_Head<0x01ee>
{
    static const uint16_t PACKET_ID = 0x01ee;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x01ee>
{
    static const uint16_t PACKET_ID = 0x01ee;

    uint16_t ioff2 = {};
    ItemNameId name_id = {};
    ItemType item_type = {};
    uint8_t identify = {};
    uint16_t amount = {};
    EPOS epos = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
};

template<>
struct Packet_Head<0x01f0>
{
    static const uint16_t PACKET_ID = 0x01f0;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    // TODO remove this
    uint16_t magic_packet_length = {};
};
template<>
struct Packet_Repeat<0x01f0>
{
    static const uint16_t PACKET_ID = 0x01f0;

    uint16_t soff1 = {};
    ItemNameId name_id = {};
    ItemType item_type = {};
    uint8_t identify = {};
    uint16_t amount = {};
    EPOS epos_zero = {};
    uint16_t card0 = {};
    uint16_t card1 = {};
    uint16_t card2 = {};
    uint16_t card3 = {};
};

template<>
struct Packet_Fixed<0x020c>
{
    static const uint16_t PACKET_ID = 0x020c;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId block_id = {};
    IP4Address ip = {};
};

template<>
struct Packet_Fixed<0x0212>
{
    static const uint16_t PACKET_ID = 0x0212;

    // TODO remove this
    uint16_t magic_packet_id = PACKET_ID;
    BlockId npc_id = {};
    uint16_t command = {};
    BlockId id = {};
    uint16_t x = {};
    uint16_t y = {};
};


template<>
struct NetPacket_Fixed<0x0072>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 char_id;
    Little32 login_id1;
    Little32 client_tick;
    Byte sex;
};
static_assert(offsetof(NetPacket_Fixed<0x0072>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0072>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0072>, account_id) == 2, "offsetof(NetPacket_Fixed<0x0072>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0072>, char_id) == 6, "offsetof(NetPacket_Fixed<0x0072>, char_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0072>, login_id1) == 10, "offsetof(NetPacket_Fixed<0x0072>, login_id1) == 10");
static_assert(offsetof(NetPacket_Fixed<0x0072>, client_tick) == 14, "offsetof(NetPacket_Fixed<0x0072>, client_tick) == 14");
static_assert(offsetof(NetPacket_Fixed<0x0072>, sex) == 18, "offsetof(NetPacket_Fixed<0x0072>, sex) == 18");
static_assert(sizeof(NetPacket_Fixed<0x0072>) == 19, "sizeof(NetPacket_Fixed<0x0072>) == 19");
static_assert(alignof(NetPacket_Fixed<0x0072>) == 1, "alignof(NetPacket_Fixed<0x0072>) == 1");

template<>
struct NetPacket_Fixed<0x0073>
{
    Little16 magic_packet_id;
    Little32 tick;
    NetPosition1 pos;
    Byte five1;
    Byte five2;
};
static_assert(offsetof(NetPacket_Fixed<0x0073>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0073>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0073>, tick) == 2, "offsetof(NetPacket_Fixed<0x0073>, tick) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0073>, pos) == 6, "offsetof(NetPacket_Fixed<0x0073>, pos) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0073>, five1) == 9, "offsetof(NetPacket_Fixed<0x0073>, five1) == 9");
static_assert(offsetof(NetPacket_Fixed<0x0073>, five2) == 10, "offsetof(NetPacket_Fixed<0x0073>, five2) == 10");
static_assert(sizeof(NetPacket_Fixed<0x0073>) == 11, "sizeof(NetPacket_Fixed<0x0073>) == 11");
static_assert(alignof(NetPacket_Fixed<0x0073>) == 1, "alignof(NetPacket_Fixed<0x0073>) == 1");

template<>
struct NetPacket_Fixed<0x0078>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 speed;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Little16 species;
    Little16 unused_hair_style;
    Little16 unused_weapon;
    Little16 unused_head_bottom_or_species_again;
    Little16 unused_shield_or_part_of_guild_emblem;
    Little16 unused_head_top_or_unused_part_of_guild_emblem;
    Little16 unused_head_mid_or_part_of_guild_id;
    Little16 unused_hair_color_or_part_of_guild_id;
    Little16 unused_clothes_color;
    Little16 unused_1;
    Little16 unused_2;
    NetPosition1 unused_pos_again;
    Byte unused_4b;
    Little16 unused_5;
    Little16 unused_zero_1;
    Byte unused_zero_2;
    Byte unused_sex;
    NetPosition1 pos;
    Byte five1;
    Byte five2;
    Byte zero;
    Little16 level;
};
static_assert(offsetof(NetPacket_Fixed<0x0078>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0078>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0078>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0078>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0078>, speed) == 6, "offsetof(NetPacket_Fixed<0x0078>, speed) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0078>, opt1) == 8, "offsetof(NetPacket_Fixed<0x0078>, opt1) == 8");
static_assert(offsetof(NetPacket_Fixed<0x0078>, opt2) == 10, "offsetof(NetPacket_Fixed<0x0078>, opt2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x0078>, option) == 12, "offsetof(NetPacket_Fixed<0x0078>, option) == 12");
static_assert(offsetof(NetPacket_Fixed<0x0078>, species) == 14, "offsetof(NetPacket_Fixed<0x0078>, species) == 14");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_hair_style) == 16, "offsetof(NetPacket_Fixed<0x0078>, unused_hair_style) == 16");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_weapon) == 18, "offsetof(NetPacket_Fixed<0x0078>, unused_weapon) == 18");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_head_bottom_or_species_again) == 20, "offsetof(NetPacket_Fixed<0x0078>, unused_head_bottom_or_species_again) == 20");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_shield_or_part_of_guild_emblem) == 22, "offsetof(NetPacket_Fixed<0x0078>, unused_shield_or_part_of_guild_emblem) == 22");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_head_top_or_unused_part_of_guild_emblem) == 24, "offsetof(NetPacket_Fixed<0x0078>, unused_head_top_or_unused_part_of_guild_emblem) == 24");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_head_mid_or_part_of_guild_id) == 26, "offsetof(NetPacket_Fixed<0x0078>, unused_head_mid_or_part_of_guild_id) == 26");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_hair_color_or_part_of_guild_id) == 28, "offsetof(NetPacket_Fixed<0x0078>, unused_hair_color_or_part_of_guild_id) == 28");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_clothes_color) == 30, "offsetof(NetPacket_Fixed<0x0078>, unused_clothes_color) == 30");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_1) == 32, "offsetof(NetPacket_Fixed<0x0078>, unused_1) == 32");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_2) == 34, "offsetof(NetPacket_Fixed<0x0078>, unused_2) == 34");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_pos_again) == 36, "offsetof(NetPacket_Fixed<0x0078>, unused_pos_again) == 36");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_4b) == 39, "offsetof(NetPacket_Fixed<0x0078>, unused_4b) == 39");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_5) == 40, "offsetof(NetPacket_Fixed<0x0078>, unused_5) == 40");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_zero_1) == 42, "offsetof(NetPacket_Fixed<0x0078>, unused_zero_1) == 42");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_zero_2) == 44, "offsetof(NetPacket_Fixed<0x0078>, unused_zero_2) == 44");
static_assert(offsetof(NetPacket_Fixed<0x0078>, unused_sex) == 45, "offsetof(NetPacket_Fixed<0x0078>, unused_sex) == 45");
static_assert(offsetof(NetPacket_Fixed<0x0078>, pos) == 46, "offsetof(NetPacket_Fixed<0x0078>, pos) == 46");
static_assert(offsetof(NetPacket_Fixed<0x0078>, five1) == 49, "offsetof(NetPacket_Fixed<0x0078>, five1) == 49");
static_assert(offsetof(NetPacket_Fixed<0x0078>, five2) == 50, "offsetof(NetPacket_Fixed<0x0078>, five2) == 50");
static_assert(offsetof(NetPacket_Fixed<0x0078>, zero) == 51, "offsetof(NetPacket_Fixed<0x0078>, zero) == 51");
static_assert(offsetof(NetPacket_Fixed<0x0078>, level) == 52, "offsetof(NetPacket_Fixed<0x0078>, level) == 52");
static_assert(sizeof(NetPacket_Fixed<0x0078>) == 54, "sizeof(NetPacket_Fixed<0x0078>) == 54");
static_assert(alignof(NetPacket_Fixed<0x0078>) == 1, "alignof(NetPacket_Fixed<0x0078>) == 1");

template<>
struct NetPacket_Fixed<0x007b>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 speed;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Little16 mob_class;
    Little16 unused_hair_style;
    Little16 unused_weapon;
    Little16 unused_head_bottom;
    Little32 tick_and_maybe_part_of_guild_emblem;
    Little16 unused_shield_or_maybe_part_of_guild_emblem;
    Little16 unused_head_top_or_maybe_part_of_guild_id;
    Little16 unused_head_mid_or_maybe_part_of_guild_id;
    Little16 unused_hair_color;
    Little16 unused_clothes_color;
    Little16 unused_1;
    Little16 unused_2;
    Little16 unused_3;
    Little16 unused_4;
    Little16 unused_5;
    Little16 unused_zero_1;
    Byte unused_zero_2;
    Byte unused_sex;
    NetPosition2 pos2;
    Byte zero;
    Byte five1;
    Byte five2;
    Little16 level;
};
static_assert(offsetof(NetPacket_Fixed<0x007b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x007b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x007b>, block_id) == 2, "offsetof(NetPacket_Fixed<0x007b>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x007b>, speed) == 6, "offsetof(NetPacket_Fixed<0x007b>, speed) == 6");
static_assert(offsetof(NetPacket_Fixed<0x007b>, opt1) == 8, "offsetof(NetPacket_Fixed<0x007b>, opt1) == 8");
static_assert(offsetof(NetPacket_Fixed<0x007b>, opt2) == 10, "offsetof(NetPacket_Fixed<0x007b>, opt2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x007b>, option) == 12, "offsetof(NetPacket_Fixed<0x007b>, option) == 12");
static_assert(offsetof(NetPacket_Fixed<0x007b>, mob_class) == 14, "offsetof(NetPacket_Fixed<0x007b>, mob_class) == 14");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_hair_style) == 16, "offsetof(NetPacket_Fixed<0x007b>, unused_hair_style) == 16");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_weapon) == 18, "offsetof(NetPacket_Fixed<0x007b>, unused_weapon) == 18");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_head_bottom) == 20, "offsetof(NetPacket_Fixed<0x007b>, unused_head_bottom) == 20");
static_assert(offsetof(NetPacket_Fixed<0x007b>, tick_and_maybe_part_of_guild_emblem) == 22, "offsetof(NetPacket_Fixed<0x007b>, tick_and_maybe_part_of_guild_emblem) == 22");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_shield_or_maybe_part_of_guild_emblem) == 26, "offsetof(NetPacket_Fixed<0x007b>, unused_shield_or_maybe_part_of_guild_emblem) == 26");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_head_top_or_maybe_part_of_guild_id) == 28, "offsetof(NetPacket_Fixed<0x007b>, unused_head_top_or_maybe_part_of_guild_id) == 28");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_head_mid_or_maybe_part_of_guild_id) == 30, "offsetof(NetPacket_Fixed<0x007b>, unused_head_mid_or_maybe_part_of_guild_id) == 30");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_hair_color) == 32, "offsetof(NetPacket_Fixed<0x007b>, unused_hair_color) == 32");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_clothes_color) == 34, "offsetof(NetPacket_Fixed<0x007b>, unused_clothes_color) == 34");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_1) == 36, "offsetof(NetPacket_Fixed<0x007b>, unused_1) == 36");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_2) == 38, "offsetof(NetPacket_Fixed<0x007b>, unused_2) == 38");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_3) == 40, "offsetof(NetPacket_Fixed<0x007b>, unused_3) == 40");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_4) == 42, "offsetof(NetPacket_Fixed<0x007b>, unused_4) == 42");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_5) == 44, "offsetof(NetPacket_Fixed<0x007b>, unused_5) == 44");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_zero_1) == 46, "offsetof(NetPacket_Fixed<0x007b>, unused_zero_1) == 46");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_zero_2) == 48, "offsetof(NetPacket_Fixed<0x007b>, unused_zero_2) == 48");
static_assert(offsetof(NetPacket_Fixed<0x007b>, unused_sex) == 49, "offsetof(NetPacket_Fixed<0x007b>, unused_sex) == 49");
static_assert(offsetof(NetPacket_Fixed<0x007b>, pos2) == 50, "offsetof(NetPacket_Fixed<0x007b>, pos2) == 50");
static_assert(offsetof(NetPacket_Fixed<0x007b>, zero) == 55, "offsetof(NetPacket_Fixed<0x007b>, zero) == 55");
static_assert(offsetof(NetPacket_Fixed<0x007b>, five1) == 56, "offsetof(NetPacket_Fixed<0x007b>, five1) == 56");
static_assert(offsetof(NetPacket_Fixed<0x007b>, five2) == 57, "offsetof(NetPacket_Fixed<0x007b>, five2) == 57");
static_assert(offsetof(NetPacket_Fixed<0x007b>, level) == 58, "offsetof(NetPacket_Fixed<0x007b>, level) == 58");
static_assert(sizeof(NetPacket_Fixed<0x007b>) == 60, "sizeof(NetPacket_Fixed<0x007b>) == 60");
static_assert(alignof(NetPacket_Fixed<0x007b>) == 1, "alignof(NetPacket_Fixed<0x007b>) == 1");

template<>
struct NetPacket_Fixed<0x007c>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 speed;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Little16 unknown_1;
    Little16 unknown_2;
    Little16 unknown_3;
    Little16 species;
    Little16 unknown_4;
    Little16 unknown_5;
    Little16 unknown_6;
    Little16 unknown_7;
    Little16 unknown_8;
    Little16 unknown_9;
    Little16 unknown_10;
    NetPosition1 pos;
    Little16 unknown_11;
};
static_assert(offsetof(NetPacket_Fixed<0x007c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x007c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x007c>, block_id) == 2, "offsetof(NetPacket_Fixed<0x007c>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x007c>, speed) == 6, "offsetof(NetPacket_Fixed<0x007c>, speed) == 6");
static_assert(offsetof(NetPacket_Fixed<0x007c>, opt1) == 8, "offsetof(NetPacket_Fixed<0x007c>, opt1) == 8");
static_assert(offsetof(NetPacket_Fixed<0x007c>, opt2) == 10, "offsetof(NetPacket_Fixed<0x007c>, opt2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x007c>, option) == 12, "offsetof(NetPacket_Fixed<0x007c>, option) == 12");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_1) == 14, "offsetof(NetPacket_Fixed<0x007c>, unknown_1) == 14");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_2) == 16, "offsetof(NetPacket_Fixed<0x007c>, unknown_2) == 16");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_3) == 18, "offsetof(NetPacket_Fixed<0x007c>, unknown_3) == 18");
static_assert(offsetof(NetPacket_Fixed<0x007c>, species) == 20, "offsetof(NetPacket_Fixed<0x007c>, species) == 20");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_4) == 22, "offsetof(NetPacket_Fixed<0x007c>, unknown_4) == 22");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_5) == 24, "offsetof(NetPacket_Fixed<0x007c>, unknown_5) == 24");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_6) == 26, "offsetof(NetPacket_Fixed<0x007c>, unknown_6) == 26");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_7) == 28, "offsetof(NetPacket_Fixed<0x007c>, unknown_7) == 28");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_8) == 30, "offsetof(NetPacket_Fixed<0x007c>, unknown_8) == 30");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_9) == 32, "offsetof(NetPacket_Fixed<0x007c>, unknown_9) == 32");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_10) == 34, "offsetof(NetPacket_Fixed<0x007c>, unknown_10) == 34");
static_assert(offsetof(NetPacket_Fixed<0x007c>, pos) == 36, "offsetof(NetPacket_Fixed<0x007c>, pos) == 36");
static_assert(offsetof(NetPacket_Fixed<0x007c>, unknown_11) == 39, "offsetof(NetPacket_Fixed<0x007c>, unknown_11) == 39");
static_assert(sizeof(NetPacket_Fixed<0x007c>) == 41, "sizeof(NetPacket_Fixed<0x007c>) == 41");
static_assert(alignof(NetPacket_Fixed<0x007c>) == 1, "alignof(NetPacket_Fixed<0x007c>) == 1");

template<>
struct NetPacket_Fixed<0x007d>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x007d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x007d>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x007d>) == 2, "sizeof(NetPacket_Fixed<0x007d>) == 2");
static_assert(alignof(NetPacket_Fixed<0x007d>) == 1, "alignof(NetPacket_Fixed<0x007d>) == 1");

template<>
struct NetPacket_Fixed<0x007e>
{
    Little16 magic_packet_id;
    Little32 client_tick;
};
static_assert(offsetof(NetPacket_Fixed<0x007e>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x007e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x007e>, client_tick) == 2, "offsetof(NetPacket_Fixed<0x007e>, client_tick) == 2");
static_assert(sizeof(NetPacket_Fixed<0x007e>) == 6, "sizeof(NetPacket_Fixed<0x007e>) == 6");
static_assert(alignof(NetPacket_Fixed<0x007e>) == 1, "alignof(NetPacket_Fixed<0x007e>) == 1");

template<>
struct NetPacket_Fixed<0x007f>
{
    Little16 magic_packet_id;
    Little32 tick;
};
static_assert(offsetof(NetPacket_Fixed<0x007f>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x007f>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x007f>, tick) == 2, "offsetof(NetPacket_Fixed<0x007f>, tick) == 2");
static_assert(sizeof(NetPacket_Fixed<0x007f>) == 6, "sizeof(NetPacket_Fixed<0x007f>) == 6");
static_assert(alignof(NetPacket_Fixed<0x007f>) == 1, "alignof(NetPacket_Fixed<0x007f>) == 1");

template<>
struct NetPacket_Fixed<0x0080>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Byte type;
};
static_assert(offsetof(NetPacket_Fixed<0x0080>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0080>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0080>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0080>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0080>, type) == 6, "offsetof(NetPacket_Fixed<0x0080>, type) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0080>) == 7, "sizeof(NetPacket_Fixed<0x0080>) == 7");
static_assert(alignof(NetPacket_Fixed<0x0080>) == 1, "alignof(NetPacket_Fixed<0x0080>) == 1");

template<>
struct NetPacket_Fixed<0x0085>
{
    Little16 magic_packet_id;
    NetPosition1 pos;
};
static_assert(offsetof(NetPacket_Fixed<0x0085>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0085>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0085>, pos) == 2, "offsetof(NetPacket_Fixed<0x0085>, pos) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0085>) == 5, "sizeof(NetPacket_Fixed<0x0085>) == 5");
static_assert(alignof(NetPacket_Fixed<0x0085>) == 1, "alignof(NetPacket_Fixed<0x0085>) == 1");

template<>
struct NetPacket_Fixed<0x0087>
{
    Little16 magic_packet_id;
    Little32 tick;
    NetPosition2 pos2;
    Byte zero;
};
static_assert(offsetof(NetPacket_Fixed<0x0087>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0087>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0087>, tick) == 2, "offsetof(NetPacket_Fixed<0x0087>, tick) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0087>, pos2) == 6, "offsetof(NetPacket_Fixed<0x0087>, pos2) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0087>, zero) == 11, "offsetof(NetPacket_Fixed<0x0087>, zero) == 11");
static_assert(sizeof(NetPacket_Fixed<0x0087>) == 12, "sizeof(NetPacket_Fixed<0x0087>) == 12");
static_assert(alignof(NetPacket_Fixed<0x0087>) == 1, "alignof(NetPacket_Fixed<0x0087>) == 1");

template<>
struct NetPacket_Fixed<0x0088>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 x;
    Little16 y;
};
static_assert(offsetof(NetPacket_Fixed<0x0088>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0088>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0088>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0088>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0088>, x) == 6, "offsetof(NetPacket_Fixed<0x0088>, x) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0088>, y) == 8, "offsetof(NetPacket_Fixed<0x0088>, y) == 8");
static_assert(sizeof(NetPacket_Fixed<0x0088>) == 10, "sizeof(NetPacket_Fixed<0x0088>) == 10");
static_assert(alignof(NetPacket_Fixed<0x0088>) == 1, "alignof(NetPacket_Fixed<0x0088>) == 1");

template<>
struct NetPacket_Fixed<0x0089>
{
    Little16 magic_packet_id;
    Little32 target_id;
    Byte action;
};
static_assert(offsetof(NetPacket_Fixed<0x0089>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0089>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0089>, target_id) == 2, "offsetof(NetPacket_Fixed<0x0089>, target_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0089>, action) == 6, "offsetof(NetPacket_Fixed<0x0089>, action) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0089>) == 7, "sizeof(NetPacket_Fixed<0x0089>) == 7");
static_assert(alignof(NetPacket_Fixed<0x0089>) == 1, "alignof(NetPacket_Fixed<0x0089>) == 1");

template<>
struct NetPacket_Fixed<0x008a>
{
    Little16 magic_packet_id;
    Little32 src_id;
    Little32 dst_id;
    Little32 tick;
    Little32 sdelay;
    Little32 ddelay;
    Little16 damage;
    Little16 div;
    Byte damage_type;
    Little16 damage2;
};
static_assert(offsetof(NetPacket_Fixed<0x008a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x008a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x008a>, src_id) == 2, "offsetof(NetPacket_Fixed<0x008a>, src_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x008a>, dst_id) == 6, "offsetof(NetPacket_Fixed<0x008a>, dst_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x008a>, tick) == 10, "offsetof(NetPacket_Fixed<0x008a>, tick) == 10");
static_assert(offsetof(NetPacket_Fixed<0x008a>, sdelay) == 14, "offsetof(NetPacket_Fixed<0x008a>, sdelay) == 14");
static_assert(offsetof(NetPacket_Fixed<0x008a>, ddelay) == 18, "offsetof(NetPacket_Fixed<0x008a>, ddelay) == 18");
static_assert(offsetof(NetPacket_Fixed<0x008a>, damage) == 22, "offsetof(NetPacket_Fixed<0x008a>, damage) == 22");
static_assert(offsetof(NetPacket_Fixed<0x008a>, div) == 24, "offsetof(NetPacket_Fixed<0x008a>, div) == 24");
static_assert(offsetof(NetPacket_Fixed<0x008a>, damage_type) == 26, "offsetof(NetPacket_Fixed<0x008a>, damage_type) == 26");
static_assert(offsetof(NetPacket_Fixed<0x008a>, damage2) == 27, "offsetof(NetPacket_Fixed<0x008a>, damage2) == 27");
static_assert(sizeof(NetPacket_Fixed<0x008a>) == 29, "sizeof(NetPacket_Fixed<0x008a>) == 29");
static_assert(alignof(NetPacket_Fixed<0x008a>) == 1, "alignof(NetPacket_Fixed<0x008a>) == 1");

template<>
struct NetPacket_Head<0x008c>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x008c>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x008c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x008c>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x008c>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x008c>) == 4, "sizeof(NetPacket_Head<0x008c>) == 4");
static_assert(alignof(NetPacket_Head<0x008c>) == 1, "alignof(NetPacket_Head<0x008c>) == 1");
template<>
struct NetPacket_Repeat<0x008c>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x008c>, c) == 0, "offsetof(NetPacket_Repeat<0x008c>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x008c>) == 1, "sizeof(NetPacket_Repeat<0x008c>) == 1");
static_assert(alignof(NetPacket_Repeat<0x008c>) == 1, "alignof(NetPacket_Repeat<0x008c>) == 1");

template<>
struct NetPacket_Head<0x008d>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Head<0x008d>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x008d>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x008d>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x008d>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x008d>, block_id) == 4, "offsetof(NetPacket_Head<0x008d>, block_id) == 4");
static_assert(sizeof(NetPacket_Head<0x008d>) == 8, "sizeof(NetPacket_Head<0x008d>) == 8");
static_assert(alignof(NetPacket_Head<0x008d>) == 1, "alignof(NetPacket_Head<0x008d>) == 1");
template<>
struct NetPacket_Repeat<0x008d>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x008d>, c) == 0, "offsetof(NetPacket_Repeat<0x008d>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x008d>) == 1, "sizeof(NetPacket_Repeat<0x008d>) == 1");
static_assert(alignof(NetPacket_Repeat<0x008d>) == 1, "alignof(NetPacket_Repeat<0x008d>) == 1");

template<>
struct NetPacket_Head<0x008e>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x008e>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x008e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x008e>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x008e>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x008e>) == 4, "sizeof(NetPacket_Head<0x008e>) == 4");
static_assert(alignof(NetPacket_Head<0x008e>) == 1, "alignof(NetPacket_Head<0x008e>) == 1");
template<>
struct NetPacket_Repeat<0x008e>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x008e>, c) == 0, "offsetof(NetPacket_Repeat<0x008e>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x008e>) == 1, "sizeof(NetPacket_Repeat<0x008e>) == 1");
static_assert(alignof(NetPacket_Repeat<0x008e>) == 1, "alignof(NetPacket_Repeat<0x008e>) == 1");

template<>
struct NetPacket_Fixed<0x0090>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Byte unused;
};
static_assert(offsetof(NetPacket_Fixed<0x0090>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0090>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0090>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0090>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0090>, unused) == 6, "offsetof(NetPacket_Fixed<0x0090>, unused) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0090>) == 7, "sizeof(NetPacket_Fixed<0x0090>) == 7");
static_assert(alignof(NetPacket_Fixed<0x0090>) == 1, "alignof(NetPacket_Fixed<0x0090>) == 1");

template<>
struct NetPacket_Fixed<0x0091>
{
    Little16 magic_packet_id;
    NetString<sizeof(MapName)> map_name;
    Little16 x;
    Little16 y;
};
static_assert(offsetof(NetPacket_Fixed<0x0091>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0091>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0091>, map_name) == 2, "offsetof(NetPacket_Fixed<0x0091>, map_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0091>, x) == 18, "offsetof(NetPacket_Fixed<0x0091>, x) == 18");
static_assert(offsetof(NetPacket_Fixed<0x0091>, y) == 20, "offsetof(NetPacket_Fixed<0x0091>, y) == 20");
static_assert(sizeof(NetPacket_Fixed<0x0091>) == 22, "sizeof(NetPacket_Fixed<0x0091>) == 22");
static_assert(alignof(NetPacket_Fixed<0x0091>) == 1, "alignof(NetPacket_Fixed<0x0091>) == 1");

template<>
struct NetPacket_Fixed<0x0092>
{
    Little16 magic_packet_id;
    NetString<sizeof(MapName)> map_name;
    Little16 x;
    Little16 y;
    IP4Address ip;
    Little16 port;
};
static_assert(offsetof(NetPacket_Fixed<0x0092>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0092>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0092>, map_name) == 2, "offsetof(NetPacket_Fixed<0x0092>, map_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0092>, x) == 18, "offsetof(NetPacket_Fixed<0x0092>, x) == 18");
static_assert(offsetof(NetPacket_Fixed<0x0092>, y) == 20, "offsetof(NetPacket_Fixed<0x0092>, y) == 20");
static_assert(offsetof(NetPacket_Fixed<0x0092>, ip) == 22, "offsetof(NetPacket_Fixed<0x0092>, ip) == 22");
static_assert(offsetof(NetPacket_Fixed<0x0092>, port) == 26, "offsetof(NetPacket_Fixed<0x0092>, port) == 26");
static_assert(sizeof(NetPacket_Fixed<0x0092>) == 28, "sizeof(NetPacket_Fixed<0x0092>) == 28");
static_assert(alignof(NetPacket_Fixed<0x0092>) == 1, "alignof(NetPacket_Fixed<0x0092>) == 1");

template<>
struct NetPacket_Fixed<0x0094>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x0094>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0094>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0094>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0094>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0094>) == 6, "sizeof(NetPacket_Fixed<0x0094>) == 6");
static_assert(alignof(NetPacket_Fixed<0x0094>) == 1, "alignof(NetPacket_Fixed<0x0094>) == 1");

template<>
struct NetPacket_Fixed<0x0095>
{
    Little16 magic_packet_id;
    Little32 block_id;
    NetString<sizeof(CharName)> char_name;
};
static_assert(offsetof(NetPacket_Fixed<0x0095>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0095>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0095>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0095>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0095>, char_name) == 6, "offsetof(NetPacket_Fixed<0x0095>, char_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0095>) == 30, "sizeof(NetPacket_Fixed<0x0095>) == 30");
static_assert(alignof(NetPacket_Fixed<0x0095>) == 1, "alignof(NetPacket_Fixed<0x0095>) == 1");

template<>
struct NetPacket_Head<0x0096>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(CharName)> target_name;
};
static_assert(offsetof(NetPacket_Head<0x0096>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x0096>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x0096>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x0096>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x0096>, target_name) == 4, "offsetof(NetPacket_Head<0x0096>, target_name) == 4");
static_assert(sizeof(NetPacket_Head<0x0096>) == 28, "sizeof(NetPacket_Head<0x0096>) == 28");
static_assert(alignof(NetPacket_Head<0x0096>) == 1, "alignof(NetPacket_Head<0x0096>) == 1");
template<>
struct NetPacket_Repeat<0x0096>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x0096>, c) == 0, "offsetof(NetPacket_Repeat<0x0096>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x0096>) == 1, "sizeof(NetPacket_Repeat<0x0096>) == 1");
static_assert(alignof(NetPacket_Repeat<0x0096>) == 1, "alignof(NetPacket_Repeat<0x0096>) == 1");

template<>
struct NetPacket_Head<0x0097>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(CharName)> char_name;
};
static_assert(offsetof(NetPacket_Head<0x0097>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x0097>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x0097>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x0097>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x0097>, char_name) == 4, "offsetof(NetPacket_Head<0x0097>, char_name) == 4");
static_assert(sizeof(NetPacket_Head<0x0097>) == 28, "sizeof(NetPacket_Head<0x0097>) == 28");
static_assert(alignof(NetPacket_Head<0x0097>) == 1, "alignof(NetPacket_Head<0x0097>) == 1");
template<>
struct NetPacket_Repeat<0x0097>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x0097>, c) == 0, "offsetof(NetPacket_Repeat<0x0097>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x0097>) == 1, "sizeof(NetPacket_Repeat<0x0097>) == 1");
static_assert(alignof(NetPacket_Repeat<0x0097>) == 1, "alignof(NetPacket_Repeat<0x0097>) == 1");

template<>
struct NetPacket_Fixed<0x0098>
{
    Little16 magic_packet_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x0098>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0098>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0098>, flag) == 2, "offsetof(NetPacket_Fixed<0x0098>, flag) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0098>) == 3, "sizeof(NetPacket_Fixed<0x0098>) == 3");
static_assert(alignof(NetPacket_Fixed<0x0098>) == 1, "alignof(NetPacket_Fixed<0x0098>) == 1");

template<>
struct NetPacket_Head<0x009a>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x009a>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x009a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x009a>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x009a>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x009a>) == 4, "sizeof(NetPacket_Head<0x009a>) == 4");
static_assert(alignof(NetPacket_Head<0x009a>) == 1, "alignof(NetPacket_Head<0x009a>) == 1");
template<>
struct NetPacket_Repeat<0x009a>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x009a>, c) == 0, "offsetof(NetPacket_Repeat<0x009a>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x009a>) == 1, "sizeof(NetPacket_Repeat<0x009a>) == 1");
static_assert(alignof(NetPacket_Repeat<0x009a>) == 1, "alignof(NetPacket_Repeat<0x009a>) == 1");

template<>
struct NetPacket_Fixed<0x009b>
{
    Little16 magic_packet_id;
    Little16 unused;
    Byte client_dir;
};
static_assert(offsetof(NetPacket_Fixed<0x009b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x009b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x009b>, unused) == 2, "offsetof(NetPacket_Fixed<0x009b>, unused) == 2");
static_assert(offsetof(NetPacket_Fixed<0x009b>, client_dir) == 4, "offsetof(NetPacket_Fixed<0x009b>, client_dir) == 4");
static_assert(sizeof(NetPacket_Fixed<0x009b>) == 5, "sizeof(NetPacket_Fixed<0x009b>) == 5");
static_assert(alignof(NetPacket_Fixed<0x009b>) == 1, "alignof(NetPacket_Fixed<0x009b>) == 1");

template<>
struct NetPacket_Fixed<0x009c>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 zero;
    Byte client_dir;
};
static_assert(offsetof(NetPacket_Fixed<0x009c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x009c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x009c>, block_id) == 2, "offsetof(NetPacket_Fixed<0x009c>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x009c>, zero) == 6, "offsetof(NetPacket_Fixed<0x009c>, zero) == 6");
static_assert(offsetof(NetPacket_Fixed<0x009c>, client_dir) == 8, "offsetof(NetPacket_Fixed<0x009c>, client_dir) == 8");
static_assert(sizeof(NetPacket_Fixed<0x009c>) == 9, "sizeof(NetPacket_Fixed<0x009c>) == 9");
static_assert(alignof(NetPacket_Fixed<0x009c>) == 1, "alignof(NetPacket_Fixed<0x009c>) == 1");

template<>
struct NetPacket_Fixed<0x009d>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 name_id;
    Byte identify;
    Little16 x;
    Little16 y;
    Little16 amount;
    Byte subx;
    Byte suby;
};
static_assert(offsetof(NetPacket_Fixed<0x009d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x009d>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x009d>, block_id) == 2, "offsetof(NetPacket_Fixed<0x009d>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x009d>, name_id) == 6, "offsetof(NetPacket_Fixed<0x009d>, name_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x009d>, identify) == 8, "offsetof(NetPacket_Fixed<0x009d>, identify) == 8");
static_assert(offsetof(NetPacket_Fixed<0x009d>, x) == 9, "offsetof(NetPacket_Fixed<0x009d>, x) == 9");
static_assert(offsetof(NetPacket_Fixed<0x009d>, y) == 11, "offsetof(NetPacket_Fixed<0x009d>, y) == 11");
static_assert(offsetof(NetPacket_Fixed<0x009d>, amount) == 13, "offsetof(NetPacket_Fixed<0x009d>, amount) == 13");
static_assert(offsetof(NetPacket_Fixed<0x009d>, subx) == 15, "offsetof(NetPacket_Fixed<0x009d>, subx) == 15");
static_assert(offsetof(NetPacket_Fixed<0x009d>, suby) == 16, "offsetof(NetPacket_Fixed<0x009d>, suby) == 16");
static_assert(sizeof(NetPacket_Fixed<0x009d>) == 17, "sizeof(NetPacket_Fixed<0x009d>) == 17");
static_assert(alignof(NetPacket_Fixed<0x009d>) == 1, "alignof(NetPacket_Fixed<0x009d>) == 1");

template<>
struct NetPacket_Fixed<0x009e>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 name_id;
    Byte identify;
    Little16 x;
    Little16 y;
    Byte subx;
    Byte suby;
    Little16 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x009e>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x009e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x009e>, block_id) == 2, "offsetof(NetPacket_Fixed<0x009e>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x009e>, name_id) == 6, "offsetof(NetPacket_Fixed<0x009e>, name_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x009e>, identify) == 8, "offsetof(NetPacket_Fixed<0x009e>, identify) == 8");
static_assert(offsetof(NetPacket_Fixed<0x009e>, x) == 9, "offsetof(NetPacket_Fixed<0x009e>, x) == 9");
static_assert(offsetof(NetPacket_Fixed<0x009e>, y) == 11, "offsetof(NetPacket_Fixed<0x009e>, y) == 11");
static_assert(offsetof(NetPacket_Fixed<0x009e>, subx) == 13, "offsetof(NetPacket_Fixed<0x009e>, subx) == 13");
static_assert(offsetof(NetPacket_Fixed<0x009e>, suby) == 14, "offsetof(NetPacket_Fixed<0x009e>, suby) == 14");
static_assert(offsetof(NetPacket_Fixed<0x009e>, amount) == 15, "offsetof(NetPacket_Fixed<0x009e>, amount) == 15");
static_assert(sizeof(NetPacket_Fixed<0x009e>) == 17, "sizeof(NetPacket_Fixed<0x009e>) == 17");
static_assert(alignof(NetPacket_Fixed<0x009e>) == 1, "alignof(NetPacket_Fixed<0x009e>) == 1");

template<>
struct NetPacket_Fixed<0x009f>
{
    Little16 magic_packet_id;
    Little32 object_id;
};
static_assert(offsetof(NetPacket_Fixed<0x009f>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x009f>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x009f>, object_id) == 2, "offsetof(NetPacket_Fixed<0x009f>, object_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x009f>) == 6, "sizeof(NetPacket_Fixed<0x009f>) == 6");
static_assert(alignof(NetPacket_Fixed<0x009f>) == 1, "alignof(NetPacket_Fixed<0x009f>) == 1");

template<>
struct NetPacket_Fixed<0x00a0>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 amount;
    Little16 name_id;
    Byte identify;
    Byte broken_or_attribute;
    Byte refine;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
    Little16 epos;
    Byte item_type;
    Byte pickup_fail;
};
static_assert(offsetof(NetPacket_Fixed<0x00a0>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00a0>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00a0>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, amount) == 4, "offsetof(NetPacket_Fixed<0x00a0>, amount) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, name_id) == 6, "offsetof(NetPacket_Fixed<0x00a0>, name_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, identify) == 8, "offsetof(NetPacket_Fixed<0x00a0>, identify) == 8");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, broken_or_attribute) == 9, "offsetof(NetPacket_Fixed<0x00a0>, broken_or_attribute) == 9");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, refine) == 10, "offsetof(NetPacket_Fixed<0x00a0>, refine) == 10");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, card0) == 11, "offsetof(NetPacket_Fixed<0x00a0>, card0) == 11");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, card1) == 13, "offsetof(NetPacket_Fixed<0x00a0>, card1) == 13");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, card2) == 15, "offsetof(NetPacket_Fixed<0x00a0>, card2) == 15");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, card3) == 17, "offsetof(NetPacket_Fixed<0x00a0>, card3) == 17");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, epos) == 19, "offsetof(NetPacket_Fixed<0x00a0>, epos) == 19");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, item_type) == 21, "offsetof(NetPacket_Fixed<0x00a0>, item_type) == 21");
static_assert(offsetof(NetPacket_Fixed<0x00a0>, pickup_fail) == 22, "offsetof(NetPacket_Fixed<0x00a0>, pickup_fail) == 22");
static_assert(sizeof(NetPacket_Fixed<0x00a0>) == 23, "sizeof(NetPacket_Fixed<0x00a0>) == 23");
static_assert(alignof(NetPacket_Fixed<0x00a0>) == 1, "alignof(NetPacket_Fixed<0x00a0>) == 1");

template<>
struct NetPacket_Fixed<0x00a1>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00a1>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00a1>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00a1>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00a1>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00a1>) == 6, "sizeof(NetPacket_Fixed<0x00a1>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00a1>) == 1, "alignof(NetPacket_Fixed<0x00a1>) == 1");

template<>
struct NetPacket_Fixed<0x00a2>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x00a2>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00a2>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00a2>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00a2>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00a2>, amount) == 4, "offsetof(NetPacket_Fixed<0x00a2>, amount) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00a2>) == 6, "sizeof(NetPacket_Fixed<0x00a2>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00a2>) == 1, "alignof(NetPacket_Fixed<0x00a2>) == 1");

template<>
struct NetPacket_Head<0x00a4>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x00a4>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00a4>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00a4>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00a4>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x00a4>) == 4, "sizeof(NetPacket_Head<0x00a4>) == 4");
static_assert(alignof(NetPacket_Head<0x00a4>) == 1, "alignof(NetPacket_Head<0x00a4>) == 1");
template<>
struct NetPacket_Repeat<0x00a4>
{
    Little16 ioff2;
    Little16 name_id;
    Byte item_type;
    Byte identify;
    Little16 epos_pc;
    Little16 epos_inv;
    Byte broken_or_attribute;
    Byte refine;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
};
static_assert(offsetof(NetPacket_Repeat<0x00a4>, ioff2) == 0, "offsetof(NetPacket_Repeat<0x00a4>, ioff2) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, name_id) == 2, "offsetof(NetPacket_Repeat<0x00a4>, name_id) == 2");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, item_type) == 4, "offsetof(NetPacket_Repeat<0x00a4>, item_type) == 4");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, identify) == 5, "offsetof(NetPacket_Repeat<0x00a4>, identify) == 5");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, epos_pc) == 6, "offsetof(NetPacket_Repeat<0x00a4>, epos_pc) == 6");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, epos_inv) == 8, "offsetof(NetPacket_Repeat<0x00a4>, epos_inv) == 8");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, broken_or_attribute) == 10, "offsetof(NetPacket_Repeat<0x00a4>, broken_or_attribute) == 10");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, refine) == 11, "offsetof(NetPacket_Repeat<0x00a4>, refine) == 11");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, card0) == 12, "offsetof(NetPacket_Repeat<0x00a4>, card0) == 12");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, card1) == 14, "offsetof(NetPacket_Repeat<0x00a4>, card1) == 14");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, card2) == 16, "offsetof(NetPacket_Repeat<0x00a4>, card2) == 16");
static_assert(offsetof(NetPacket_Repeat<0x00a4>, card3) == 18, "offsetof(NetPacket_Repeat<0x00a4>, card3) == 18");
static_assert(sizeof(NetPacket_Repeat<0x00a4>) == 20, "sizeof(NetPacket_Repeat<0x00a4>) == 20");
static_assert(alignof(NetPacket_Repeat<0x00a4>) == 1, "alignof(NetPacket_Repeat<0x00a4>) == 1");

template<>
struct NetPacket_Head<0x00a6>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x00a6>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00a6>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00a6>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00a6>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x00a6>) == 4, "sizeof(NetPacket_Head<0x00a6>) == 4");
static_assert(alignof(NetPacket_Head<0x00a6>) == 1, "alignof(NetPacket_Head<0x00a6>) == 1");
template<>
struct NetPacket_Repeat<0x00a6>
{
    Little16 soff1;
    Little16 name_id;
    Byte item_type;
    Byte identify;
    Little16 epos_id;
    Little16 epos_stor;
    Byte broken_or_attribute;
    Byte refine;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
};
static_assert(offsetof(NetPacket_Repeat<0x00a6>, soff1) == 0, "offsetof(NetPacket_Repeat<0x00a6>, soff1) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, name_id) == 2, "offsetof(NetPacket_Repeat<0x00a6>, name_id) == 2");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, item_type) == 4, "offsetof(NetPacket_Repeat<0x00a6>, item_type) == 4");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, identify) == 5, "offsetof(NetPacket_Repeat<0x00a6>, identify) == 5");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, epos_id) == 6, "offsetof(NetPacket_Repeat<0x00a6>, epos_id) == 6");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, epos_stor) == 8, "offsetof(NetPacket_Repeat<0x00a6>, epos_stor) == 8");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, broken_or_attribute) == 10, "offsetof(NetPacket_Repeat<0x00a6>, broken_or_attribute) == 10");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, refine) == 11, "offsetof(NetPacket_Repeat<0x00a6>, refine) == 11");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, card0) == 12, "offsetof(NetPacket_Repeat<0x00a6>, card0) == 12");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, card1) == 14, "offsetof(NetPacket_Repeat<0x00a6>, card1) == 14");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, card2) == 16, "offsetof(NetPacket_Repeat<0x00a6>, card2) == 16");
static_assert(offsetof(NetPacket_Repeat<0x00a6>, card3) == 18, "offsetof(NetPacket_Repeat<0x00a6>, card3) == 18");
static_assert(sizeof(NetPacket_Repeat<0x00a6>) == 20, "sizeof(NetPacket_Repeat<0x00a6>) == 20");
static_assert(alignof(NetPacket_Repeat<0x00a6>) == 1, "alignof(NetPacket_Repeat<0x00a6>) == 1");

template<>
struct NetPacket_Fixed<0x00a7>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little32 unused_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00a7>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00a7>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00a7>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00a7>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00a7>, unused_id) == 4, "offsetof(NetPacket_Fixed<0x00a7>, unused_id) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00a7>) == 8, "sizeof(NetPacket_Fixed<0x00a7>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00a7>) == 1, "alignof(NetPacket_Fixed<0x00a7>) == 1");

template<>
struct NetPacket_Fixed<0x00a8>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 amount;
    Byte ok;
};
static_assert(offsetof(NetPacket_Fixed<0x00a8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00a8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00a8>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00a8>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00a8>, amount) == 4, "offsetof(NetPacket_Fixed<0x00a8>, amount) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00a8>, ok) == 6, "offsetof(NetPacket_Fixed<0x00a8>, ok) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00a8>) == 7, "sizeof(NetPacket_Fixed<0x00a8>) == 7");
static_assert(alignof(NetPacket_Fixed<0x00a8>) == 1, "alignof(NetPacket_Fixed<0x00a8>) == 1");

template<>
struct NetPacket_Fixed<0x00a9>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 epos_ignored;
};
static_assert(offsetof(NetPacket_Fixed<0x00a9>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00a9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00a9>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00a9>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00a9>, epos_ignored) == 4, "offsetof(NetPacket_Fixed<0x00a9>, epos_ignored) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00a9>) == 6, "sizeof(NetPacket_Fixed<0x00a9>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00a9>) == 1, "alignof(NetPacket_Fixed<0x00a9>) == 1");

template<>
struct NetPacket_Fixed<0x00aa>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 epos;
    Byte ok;
};
static_assert(offsetof(NetPacket_Fixed<0x00aa>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00aa>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00aa>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00aa>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00aa>, epos) == 4, "offsetof(NetPacket_Fixed<0x00aa>, epos) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00aa>, ok) == 6, "offsetof(NetPacket_Fixed<0x00aa>, ok) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00aa>) == 7, "sizeof(NetPacket_Fixed<0x00aa>) == 7");
static_assert(alignof(NetPacket_Fixed<0x00aa>) == 1, "alignof(NetPacket_Fixed<0x00aa>) == 1");

template<>
struct NetPacket_Fixed<0x00ab>
{
    Little16 magic_packet_id;
    Little16 ioff2;
};
static_assert(offsetof(NetPacket_Fixed<0x00ab>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ab>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00ab>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00ab>, ioff2) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00ab>) == 4, "sizeof(NetPacket_Fixed<0x00ab>) == 4");
static_assert(alignof(NetPacket_Fixed<0x00ab>) == 1, "alignof(NetPacket_Fixed<0x00ab>) == 1");

template<>
struct NetPacket_Fixed<0x00ac>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 epos;
    Byte ok;
};
static_assert(offsetof(NetPacket_Fixed<0x00ac>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ac>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00ac>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00ac>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00ac>, epos) == 4, "offsetof(NetPacket_Fixed<0x00ac>, epos) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00ac>, ok) == 6, "offsetof(NetPacket_Fixed<0x00ac>, ok) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00ac>) == 7, "sizeof(NetPacket_Fixed<0x00ac>) == 7");
static_assert(alignof(NetPacket_Fixed<0x00ac>) == 1, "alignof(NetPacket_Fixed<0x00ac>) == 1");

template<>
struct NetPacket_Fixed<0x00af>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x00af>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00af>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00af>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00af>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00af>, amount) == 4, "offsetof(NetPacket_Fixed<0x00af>, amount) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00af>) == 6, "sizeof(NetPacket_Fixed<0x00af>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00af>) == 1, "alignof(NetPacket_Fixed<0x00af>) == 1");

template<>
struct NetPacket_Fixed<0x00b0>
{
    Little16 magic_packet_id;
    Little16 sp_type;
    Little32 value;
};
static_assert(offsetof(NetPacket_Fixed<0x00b0>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b0>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b0>, sp_type) == 2, "offsetof(NetPacket_Fixed<0x00b0>, sp_type) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00b0>, value) == 4, "offsetof(NetPacket_Fixed<0x00b0>, value) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00b0>) == 8, "sizeof(NetPacket_Fixed<0x00b0>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00b0>) == 1, "alignof(NetPacket_Fixed<0x00b0>) == 1");

template<>
struct NetPacket_Fixed<0x00b1>
{
    Little16 magic_packet_id;
    Little16 sp_type;
    Little32 value;
};
static_assert(offsetof(NetPacket_Fixed<0x00b1>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b1>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b1>, sp_type) == 2, "offsetof(NetPacket_Fixed<0x00b1>, sp_type) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00b1>, value) == 4, "offsetof(NetPacket_Fixed<0x00b1>, value) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00b1>) == 8, "sizeof(NetPacket_Fixed<0x00b1>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00b1>) == 1, "alignof(NetPacket_Fixed<0x00b1>) == 1");

template<>
struct NetPacket_Fixed<0x00b2>
{
    Little16 magic_packet_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x00b2>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b2>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b2>, flag) == 2, "offsetof(NetPacket_Fixed<0x00b2>, flag) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00b2>) == 3, "sizeof(NetPacket_Fixed<0x00b2>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00b2>) == 1, "alignof(NetPacket_Fixed<0x00b2>) == 1");

template<>
struct NetPacket_Fixed<0x00b3>
{
    Little16 magic_packet_id;
    Byte one;
};
static_assert(offsetof(NetPacket_Fixed<0x00b3>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b3>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b3>, one) == 2, "offsetof(NetPacket_Fixed<0x00b3>, one) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00b3>) == 3, "sizeof(NetPacket_Fixed<0x00b3>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00b3>) == 1, "alignof(NetPacket_Fixed<0x00b3>) == 1");

template<>
struct NetPacket_Head<0x00b4>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Head<0x00b4>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00b4>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00b4>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00b4>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x00b4>, block_id) == 4, "offsetof(NetPacket_Head<0x00b4>, block_id) == 4");
static_assert(sizeof(NetPacket_Head<0x00b4>) == 8, "sizeof(NetPacket_Head<0x00b4>) == 8");
static_assert(alignof(NetPacket_Head<0x00b4>) == 1, "alignof(NetPacket_Head<0x00b4>) == 1");
template<>
struct NetPacket_Repeat<0x00b4>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x00b4>, c) == 0, "offsetof(NetPacket_Repeat<0x00b4>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x00b4>) == 1, "sizeof(NetPacket_Repeat<0x00b4>) == 1");
static_assert(alignof(NetPacket_Repeat<0x00b4>) == 1, "alignof(NetPacket_Repeat<0x00b4>) == 1");

template<>
struct NetPacket_Fixed<0x00b5>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00b5>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b5>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b5>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00b5>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00b5>) == 6, "sizeof(NetPacket_Fixed<0x00b5>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00b5>) == 1, "alignof(NetPacket_Fixed<0x00b5>) == 1");

template<>
struct NetPacket_Fixed<0x00b6>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00b6>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b6>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b6>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00b6>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00b6>) == 6, "sizeof(NetPacket_Fixed<0x00b6>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00b6>) == 1, "alignof(NetPacket_Fixed<0x00b6>) == 1");

template<>
struct NetPacket_Head<0x00b7>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Head<0x00b7>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00b7>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00b7>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00b7>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x00b7>, block_id) == 4, "offsetof(NetPacket_Head<0x00b7>, block_id) == 4");
static_assert(sizeof(NetPacket_Head<0x00b7>) == 8, "sizeof(NetPacket_Head<0x00b7>) == 8");
static_assert(alignof(NetPacket_Head<0x00b7>) == 1, "alignof(NetPacket_Head<0x00b7>) == 1");
template<>
struct NetPacket_Repeat<0x00b7>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x00b7>, c) == 0, "offsetof(NetPacket_Repeat<0x00b7>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x00b7>) == 1, "sizeof(NetPacket_Repeat<0x00b7>) == 1");
static_assert(alignof(NetPacket_Repeat<0x00b7>) == 1, "alignof(NetPacket_Repeat<0x00b7>) == 1");

template<>
struct NetPacket_Fixed<0x00b8>
{
    Little16 magic_packet_id;
    Little32 npc_id;
    Byte menu_entry;
};
static_assert(offsetof(NetPacket_Fixed<0x00b8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b8>, npc_id) == 2, "offsetof(NetPacket_Fixed<0x00b8>, npc_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00b8>, menu_entry) == 6, "offsetof(NetPacket_Fixed<0x00b8>, menu_entry) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00b8>) == 7, "sizeof(NetPacket_Fixed<0x00b8>) == 7");
static_assert(alignof(NetPacket_Fixed<0x00b8>) == 1, "alignof(NetPacket_Fixed<0x00b8>) == 1");

template<>
struct NetPacket_Fixed<0x00b9>
{
    Little16 magic_packet_id;
    Little32 npc_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00b9>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00b9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00b9>, npc_id) == 2, "offsetof(NetPacket_Fixed<0x00b9>, npc_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00b9>) == 6, "sizeof(NetPacket_Fixed<0x00b9>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00b9>) == 1, "alignof(NetPacket_Fixed<0x00b9>) == 1");

template<>
struct NetPacket_Fixed<0x00bb>
{
    Little16 magic_packet_id;
    Little16 asp;
    Byte unused;
};
static_assert(offsetof(NetPacket_Fixed<0x00bb>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00bb>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00bb>, asp) == 2, "offsetof(NetPacket_Fixed<0x00bb>, asp) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00bb>, unused) == 4, "offsetof(NetPacket_Fixed<0x00bb>, unused) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00bb>) == 5, "sizeof(NetPacket_Fixed<0x00bb>) == 5");
static_assert(alignof(NetPacket_Fixed<0x00bb>) == 1, "alignof(NetPacket_Fixed<0x00bb>) == 1");

template<>
struct NetPacket_Fixed<0x00bc>
{
    Little16 magic_packet_id;
    Little16 sp_type;
    Byte ok;
    Byte val;
};
static_assert(offsetof(NetPacket_Fixed<0x00bc>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00bc>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00bc>, sp_type) == 2, "offsetof(NetPacket_Fixed<0x00bc>, sp_type) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00bc>, ok) == 4, "offsetof(NetPacket_Fixed<0x00bc>, ok) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00bc>, val) == 5, "offsetof(NetPacket_Fixed<0x00bc>, val) == 5");
static_assert(sizeof(NetPacket_Fixed<0x00bc>) == 6, "sizeof(NetPacket_Fixed<0x00bc>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00bc>) == 1, "alignof(NetPacket_Fixed<0x00bc>) == 1");

template<>
struct NetPacket_Fixed<0x00bd>
{
    Little16 magic_packet_id;
    Little16 status_point;
    Byte str_attr;
    Byte str_upd;
    Byte agi_attr;
    Byte agi_upd;
    Byte vit_attr;
    Byte vit_upd;
    Byte int_attr;
    Byte int_upd;
    Byte dex_attr;
    Byte dex_upd;
    Byte luk_attr;
    Byte luk_upd;
    Little16 atk_sum;
    Little16 watk2;
    Little16 matk1;
    Little16 matk2;
    Little16 def;
    Little16 def2;
    Little16 mdef;
    Little16 mdef2;
    Little16 hit;
    Little16 flee;
    Little16 flee2;
    Little16 critical;
    Little16 karma;
    Little16 manner;
};
static_assert(offsetof(NetPacket_Fixed<0x00bd>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00bd>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, status_point) == 2, "offsetof(NetPacket_Fixed<0x00bd>, status_point) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, str_attr) == 4, "offsetof(NetPacket_Fixed<0x00bd>, str_attr) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, str_upd) == 5, "offsetof(NetPacket_Fixed<0x00bd>, str_upd) == 5");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, agi_attr) == 6, "offsetof(NetPacket_Fixed<0x00bd>, agi_attr) == 6");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, agi_upd) == 7, "offsetof(NetPacket_Fixed<0x00bd>, agi_upd) == 7");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, vit_attr) == 8, "offsetof(NetPacket_Fixed<0x00bd>, vit_attr) == 8");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, vit_upd) == 9, "offsetof(NetPacket_Fixed<0x00bd>, vit_upd) == 9");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, int_attr) == 10, "offsetof(NetPacket_Fixed<0x00bd>, int_attr) == 10");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, int_upd) == 11, "offsetof(NetPacket_Fixed<0x00bd>, int_upd) == 11");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, dex_attr) == 12, "offsetof(NetPacket_Fixed<0x00bd>, dex_attr) == 12");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, dex_upd) == 13, "offsetof(NetPacket_Fixed<0x00bd>, dex_upd) == 13");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, luk_attr) == 14, "offsetof(NetPacket_Fixed<0x00bd>, luk_attr) == 14");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, luk_upd) == 15, "offsetof(NetPacket_Fixed<0x00bd>, luk_upd) == 15");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, atk_sum) == 16, "offsetof(NetPacket_Fixed<0x00bd>, atk_sum) == 16");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, watk2) == 18, "offsetof(NetPacket_Fixed<0x00bd>, watk2) == 18");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, matk1) == 20, "offsetof(NetPacket_Fixed<0x00bd>, matk1) == 20");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, matk2) == 22, "offsetof(NetPacket_Fixed<0x00bd>, matk2) == 22");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, def) == 24, "offsetof(NetPacket_Fixed<0x00bd>, def) == 24");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, def2) == 26, "offsetof(NetPacket_Fixed<0x00bd>, def2) == 26");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, mdef) == 28, "offsetof(NetPacket_Fixed<0x00bd>, mdef) == 28");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, mdef2) == 30, "offsetof(NetPacket_Fixed<0x00bd>, mdef2) == 30");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, hit) == 32, "offsetof(NetPacket_Fixed<0x00bd>, hit) == 32");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, flee) == 34, "offsetof(NetPacket_Fixed<0x00bd>, flee) == 34");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, flee2) == 36, "offsetof(NetPacket_Fixed<0x00bd>, flee2) == 36");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, critical) == 38, "offsetof(NetPacket_Fixed<0x00bd>, critical) == 38");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, karma) == 40, "offsetof(NetPacket_Fixed<0x00bd>, karma) == 40");
static_assert(offsetof(NetPacket_Fixed<0x00bd>, manner) == 42, "offsetof(NetPacket_Fixed<0x00bd>, manner) == 42");
static_assert(sizeof(NetPacket_Fixed<0x00bd>) == 44, "sizeof(NetPacket_Fixed<0x00bd>) == 44");
static_assert(alignof(NetPacket_Fixed<0x00bd>) == 1, "alignof(NetPacket_Fixed<0x00bd>) == 1");

template<>
struct NetPacket_Fixed<0x00be>
{
    Little16 magic_packet_id;
    Little16 sp_type;
    Byte value;
};
static_assert(offsetof(NetPacket_Fixed<0x00be>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00be>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00be>, sp_type) == 2, "offsetof(NetPacket_Fixed<0x00be>, sp_type) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00be>, value) == 4, "offsetof(NetPacket_Fixed<0x00be>, value) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00be>) == 5, "sizeof(NetPacket_Fixed<0x00be>) == 5");
static_assert(alignof(NetPacket_Fixed<0x00be>) == 1, "alignof(NetPacket_Fixed<0x00be>) == 1");

template<>
struct NetPacket_Fixed<0x00bf>
{
    Little16 magic_packet_id;
    Byte emote;
};
static_assert(offsetof(NetPacket_Fixed<0x00bf>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00bf>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00bf>, emote) == 2, "offsetof(NetPacket_Fixed<0x00bf>, emote) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00bf>) == 3, "sizeof(NetPacket_Fixed<0x00bf>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00bf>) == 1, "alignof(NetPacket_Fixed<0x00bf>) == 1");

template<>
struct NetPacket_Fixed<0x00c0>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Byte type;
};
static_assert(offsetof(NetPacket_Fixed<0x00c0>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00c0>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00c0>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00c0>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00c0>, type) == 6, "offsetof(NetPacket_Fixed<0x00c0>, type) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00c0>) == 7, "sizeof(NetPacket_Fixed<0x00c0>) == 7");
static_assert(alignof(NetPacket_Fixed<0x00c0>) == 1, "alignof(NetPacket_Fixed<0x00c0>) == 1");

template<>
struct NetPacket_Fixed<0x00c1>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00c1>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00c1>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00c1>) == 2, "sizeof(NetPacket_Fixed<0x00c1>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00c1>) == 1, "alignof(NetPacket_Fixed<0x00c1>) == 1");

template<>
struct NetPacket_Fixed<0x00c2>
{
    Little16 magic_packet_id;
    Little32 users;
};
static_assert(offsetof(NetPacket_Fixed<0x00c2>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00c2>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00c2>, users) == 2, "offsetof(NetPacket_Fixed<0x00c2>, users) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00c2>) == 6, "sizeof(NetPacket_Fixed<0x00c2>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00c2>) == 1, "alignof(NetPacket_Fixed<0x00c2>) == 1");

template<>
struct NetPacket_Fixed<0x00c4>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00c4>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00c4>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00c4>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00c4>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00c4>) == 6, "sizeof(NetPacket_Fixed<0x00c4>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00c4>) == 1, "alignof(NetPacket_Fixed<0x00c4>) == 1");

template<>
struct NetPacket_Fixed<0x00c5>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Byte type;
};
static_assert(offsetof(NetPacket_Fixed<0x00c5>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00c5>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00c5>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00c5>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00c5>, type) == 6, "offsetof(NetPacket_Fixed<0x00c5>, type) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00c5>) == 7, "sizeof(NetPacket_Fixed<0x00c5>) == 7");
static_assert(alignof(NetPacket_Fixed<0x00c5>) == 1, "alignof(NetPacket_Fixed<0x00c5>) == 1");

template<>
struct NetPacket_Head<0x00c6>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x00c6>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00c6>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00c6>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00c6>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x00c6>) == 4, "sizeof(NetPacket_Head<0x00c6>) == 4");
static_assert(alignof(NetPacket_Head<0x00c6>) == 1, "alignof(NetPacket_Head<0x00c6>) == 1");
template<>
struct NetPacket_Repeat<0x00c6>
{
    Little32 base_price;
    Little32 actual_price;
    Byte type;
    Little16 name_id;
};
static_assert(offsetof(NetPacket_Repeat<0x00c6>, base_price) == 0, "offsetof(NetPacket_Repeat<0x00c6>, base_price) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00c6>, actual_price) == 4, "offsetof(NetPacket_Repeat<0x00c6>, actual_price) == 4");
static_assert(offsetof(NetPacket_Repeat<0x00c6>, type) == 8, "offsetof(NetPacket_Repeat<0x00c6>, type) == 8");
static_assert(offsetof(NetPacket_Repeat<0x00c6>, name_id) == 9, "offsetof(NetPacket_Repeat<0x00c6>, name_id) == 9");
static_assert(sizeof(NetPacket_Repeat<0x00c6>) == 11, "sizeof(NetPacket_Repeat<0x00c6>) == 11");
static_assert(alignof(NetPacket_Repeat<0x00c6>) == 1, "alignof(NetPacket_Repeat<0x00c6>) == 1");

template<>
struct NetPacket_Head<0x00c7>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x00c7>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00c7>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00c7>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00c7>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x00c7>) == 4, "sizeof(NetPacket_Head<0x00c7>) == 4");
static_assert(alignof(NetPacket_Head<0x00c7>) == 1, "alignof(NetPacket_Head<0x00c7>) == 1");
template<>
struct NetPacket_Repeat<0x00c7>
{
    Little16 ioff2;
    Little32 base_price;
    Little32 actual_price;
};
static_assert(offsetof(NetPacket_Repeat<0x00c7>, ioff2) == 0, "offsetof(NetPacket_Repeat<0x00c7>, ioff2) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00c7>, base_price) == 2, "offsetof(NetPacket_Repeat<0x00c7>, base_price) == 2");
static_assert(offsetof(NetPacket_Repeat<0x00c7>, actual_price) == 6, "offsetof(NetPacket_Repeat<0x00c7>, actual_price) == 6");
static_assert(sizeof(NetPacket_Repeat<0x00c7>) == 10, "sizeof(NetPacket_Repeat<0x00c7>) == 10");
static_assert(alignof(NetPacket_Repeat<0x00c7>) == 1, "alignof(NetPacket_Repeat<0x00c7>) == 1");

template<>
struct NetPacket_Head<0x00c8>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x00c8>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00c8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00c8>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00c8>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x00c8>) == 4, "sizeof(NetPacket_Head<0x00c8>) == 4");
static_assert(alignof(NetPacket_Head<0x00c8>) == 1, "alignof(NetPacket_Head<0x00c8>) == 1");
template<>
struct NetPacket_Repeat<0x00c8>
{
    Little16 count;
    Little16 name_id;
};
static_assert(offsetof(NetPacket_Repeat<0x00c8>, count) == 0, "offsetof(NetPacket_Repeat<0x00c8>, count) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00c8>, name_id) == 2, "offsetof(NetPacket_Repeat<0x00c8>, name_id) == 2");
static_assert(sizeof(NetPacket_Repeat<0x00c8>) == 4, "sizeof(NetPacket_Repeat<0x00c8>) == 4");
static_assert(alignof(NetPacket_Repeat<0x00c8>) == 1, "alignof(NetPacket_Repeat<0x00c8>) == 1");

template<>
struct NetPacket_Head<0x00c9>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x00c9>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00c9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00c9>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00c9>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x00c9>) == 4, "sizeof(NetPacket_Head<0x00c9>) == 4");
static_assert(alignof(NetPacket_Head<0x00c9>) == 1, "alignof(NetPacket_Head<0x00c9>) == 1");
template<>
struct NetPacket_Repeat<0x00c9>
{
    Little16 ioff2;
    Little16 count;
};
static_assert(offsetof(NetPacket_Repeat<0x00c9>, ioff2) == 0, "offsetof(NetPacket_Repeat<0x00c9>, ioff2) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00c9>, count) == 2, "offsetof(NetPacket_Repeat<0x00c9>, count) == 2");
static_assert(sizeof(NetPacket_Repeat<0x00c9>) == 4, "sizeof(NetPacket_Repeat<0x00c9>) == 4");
static_assert(alignof(NetPacket_Repeat<0x00c9>) == 1, "alignof(NetPacket_Repeat<0x00c9>) == 1");

template<>
struct NetPacket_Fixed<0x00ca>
{
    Little16 magic_packet_id;
    Byte fail;
};
static_assert(offsetof(NetPacket_Fixed<0x00ca>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ca>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00ca>, fail) == 2, "offsetof(NetPacket_Fixed<0x00ca>, fail) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00ca>) == 3, "sizeof(NetPacket_Fixed<0x00ca>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00ca>) == 1, "alignof(NetPacket_Fixed<0x00ca>) == 1");

template<>
struct NetPacket_Fixed<0x00cb>
{
    Little16 magic_packet_id;
    Byte fail;
};
static_assert(offsetof(NetPacket_Fixed<0x00cb>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00cb>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00cb>, fail) == 2, "offsetof(NetPacket_Fixed<0x00cb>, fail) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00cb>) == 3, "sizeof(NetPacket_Fixed<0x00cb>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00cb>) == 1, "alignof(NetPacket_Fixed<0x00cb>) == 1");

template<>
struct NetPacket_Fixed<0x00cd>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00cd>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00cd>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00cd>, account_id) == 2, "offsetof(NetPacket_Fixed<0x00cd>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00cd>) == 6, "sizeof(NetPacket_Fixed<0x00cd>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00cd>) == 1, "alignof(NetPacket_Fixed<0x00cd>) == 1");

template<>
struct NetPacket_Fixed<0x00e4>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00e4>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00e4>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00e4>, block_id) == 2, "offsetof(NetPacket_Fixed<0x00e4>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00e4>) == 6, "sizeof(NetPacket_Fixed<0x00e4>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00e4>) == 1, "alignof(NetPacket_Fixed<0x00e4>) == 1");

template<>
struct NetPacket_Fixed<0x00e5>
{
    Little16 magic_packet_id;
    NetString<sizeof(CharName)> char_name;
};
static_assert(offsetof(NetPacket_Fixed<0x00e5>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00e5>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00e5>, char_name) == 2, "offsetof(NetPacket_Fixed<0x00e5>, char_name) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00e5>) == 26, "sizeof(NetPacket_Fixed<0x00e5>) == 26");
static_assert(alignof(NetPacket_Fixed<0x00e5>) == 1, "alignof(NetPacket_Fixed<0x00e5>) == 1");

template<>
struct NetPacket_Fixed<0x00e6>
{
    Little16 magic_packet_id;
    Byte type;
};
static_assert(offsetof(NetPacket_Fixed<0x00e6>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00e6>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00e6>, type) == 2, "offsetof(NetPacket_Fixed<0x00e6>, type) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00e6>) == 3, "sizeof(NetPacket_Fixed<0x00e6>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00e6>) == 1, "alignof(NetPacket_Fixed<0x00e6>) == 1");

template<>
struct NetPacket_Fixed<0x00e7>
{
    Little16 magic_packet_id;
    Byte type;
};
static_assert(offsetof(NetPacket_Fixed<0x00e7>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00e7>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00e7>, type) == 2, "offsetof(NetPacket_Fixed<0x00e7>, type) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00e7>) == 3, "sizeof(NetPacket_Fixed<0x00e7>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00e7>) == 1, "alignof(NetPacket_Fixed<0x00e7>) == 1");

template<>
struct NetPacket_Fixed<0x00e8>
{
    Little16 magic_packet_id;
    Little16 zeny_or_ioff2;
    Little32 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x00e8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00e8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00e8>, zeny_or_ioff2) == 2, "offsetof(NetPacket_Fixed<0x00e8>, zeny_or_ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00e8>, amount) == 4, "offsetof(NetPacket_Fixed<0x00e8>, amount) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00e8>) == 8, "sizeof(NetPacket_Fixed<0x00e8>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00e8>) == 1, "alignof(NetPacket_Fixed<0x00e8>) == 1");

template<>
struct NetPacket_Fixed<0x00e9>
{
    Little16 magic_packet_id;
    Little32 amount;
    Little16 name_id;
    Byte identify;
    Byte broken_or_attribute;
    Byte refine;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
};
static_assert(offsetof(NetPacket_Fixed<0x00e9>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00e9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, amount) == 2, "offsetof(NetPacket_Fixed<0x00e9>, amount) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, name_id) == 6, "offsetof(NetPacket_Fixed<0x00e9>, name_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, identify) == 8, "offsetof(NetPacket_Fixed<0x00e9>, identify) == 8");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, broken_or_attribute) == 9, "offsetof(NetPacket_Fixed<0x00e9>, broken_or_attribute) == 9");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, refine) == 10, "offsetof(NetPacket_Fixed<0x00e9>, refine) == 10");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, card0) == 11, "offsetof(NetPacket_Fixed<0x00e9>, card0) == 11");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, card1) == 13, "offsetof(NetPacket_Fixed<0x00e9>, card1) == 13");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, card2) == 15, "offsetof(NetPacket_Fixed<0x00e9>, card2) == 15");
static_assert(offsetof(NetPacket_Fixed<0x00e9>, card3) == 17, "offsetof(NetPacket_Fixed<0x00e9>, card3) == 17");
static_assert(sizeof(NetPacket_Fixed<0x00e9>) == 19, "sizeof(NetPacket_Fixed<0x00e9>) == 19");
static_assert(alignof(NetPacket_Fixed<0x00e9>) == 1, "alignof(NetPacket_Fixed<0x00e9>) == 1");

template<>
struct NetPacket_Fixed<0x00eb>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00eb>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00eb>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00eb>) == 2, "sizeof(NetPacket_Fixed<0x00eb>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00eb>) == 1, "alignof(NetPacket_Fixed<0x00eb>) == 1");

template<>
struct NetPacket_Fixed<0x00ec>
{
    Little16 magic_packet_id;
    Byte fail;
};
static_assert(offsetof(NetPacket_Fixed<0x00ec>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ec>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00ec>, fail) == 2, "offsetof(NetPacket_Fixed<0x00ec>, fail) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00ec>) == 3, "sizeof(NetPacket_Fixed<0x00ec>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00ec>) == 1, "alignof(NetPacket_Fixed<0x00ec>) == 1");

template<>
struct NetPacket_Fixed<0x00ed>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00ed>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ed>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00ed>) == 2, "sizeof(NetPacket_Fixed<0x00ed>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00ed>) == 1, "alignof(NetPacket_Fixed<0x00ed>) == 1");

template<>
struct NetPacket_Fixed<0x00ee>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00ee>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ee>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00ee>) == 2, "sizeof(NetPacket_Fixed<0x00ee>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00ee>) == 1, "alignof(NetPacket_Fixed<0x00ee>) == 1");

template<>
struct NetPacket_Fixed<0x00ef>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00ef>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ef>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00ef>) == 2, "sizeof(NetPacket_Fixed<0x00ef>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00ef>) == 1, "alignof(NetPacket_Fixed<0x00ef>) == 1");

template<>
struct NetPacket_Fixed<0x00f0>
{
    Little16 magic_packet_id;
    Byte fail;
};
static_assert(offsetof(NetPacket_Fixed<0x00f0>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f0>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f0>, fail) == 2, "offsetof(NetPacket_Fixed<0x00f0>, fail) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00f0>) == 3, "sizeof(NetPacket_Fixed<0x00f0>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00f0>) == 1, "alignof(NetPacket_Fixed<0x00f0>) == 1");

template<>
struct NetPacket_Fixed<0x00f2>
{
    Little16 magic_packet_id;
    Little16 current_slots;
    Little16 max_slots;
};
static_assert(offsetof(NetPacket_Fixed<0x00f2>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f2>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f2>, current_slots) == 2, "offsetof(NetPacket_Fixed<0x00f2>, current_slots) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00f2>, max_slots) == 4, "offsetof(NetPacket_Fixed<0x00f2>, max_slots) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00f2>) == 6, "sizeof(NetPacket_Fixed<0x00f2>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00f2>) == 1, "alignof(NetPacket_Fixed<0x00f2>) == 1");

template<>
struct NetPacket_Fixed<0x00f3>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little32 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x00f3>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f3>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f3>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x00f3>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00f3>, amount) == 4, "offsetof(NetPacket_Fixed<0x00f3>, amount) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00f3>) == 8, "sizeof(NetPacket_Fixed<0x00f3>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00f3>) == 1, "alignof(NetPacket_Fixed<0x00f3>) == 1");

template<>
struct NetPacket_Fixed<0x00f4>
{
    Little16 magic_packet_id;
    Little16 soff1;
    Little32 amount;
    Little16 name_id;
    Byte identify;
    Byte broken_or_attribute;
    Byte refine;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
};
static_assert(offsetof(NetPacket_Fixed<0x00f4>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f4>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, soff1) == 2, "offsetof(NetPacket_Fixed<0x00f4>, soff1) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, amount) == 4, "offsetof(NetPacket_Fixed<0x00f4>, amount) == 4");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, name_id) == 8, "offsetof(NetPacket_Fixed<0x00f4>, name_id) == 8");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, identify) == 10, "offsetof(NetPacket_Fixed<0x00f4>, identify) == 10");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, broken_or_attribute) == 11, "offsetof(NetPacket_Fixed<0x00f4>, broken_or_attribute) == 11");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, refine) == 12, "offsetof(NetPacket_Fixed<0x00f4>, refine) == 12");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, card0) == 13, "offsetof(NetPacket_Fixed<0x00f4>, card0) == 13");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, card1) == 15, "offsetof(NetPacket_Fixed<0x00f4>, card1) == 15");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, card2) == 17, "offsetof(NetPacket_Fixed<0x00f4>, card2) == 17");
static_assert(offsetof(NetPacket_Fixed<0x00f4>, card3) == 19, "offsetof(NetPacket_Fixed<0x00f4>, card3) == 19");
static_assert(sizeof(NetPacket_Fixed<0x00f4>) == 21, "sizeof(NetPacket_Fixed<0x00f4>) == 21");
static_assert(alignof(NetPacket_Fixed<0x00f4>) == 1, "alignof(NetPacket_Fixed<0x00f4>) == 1");

template<>
struct NetPacket_Fixed<0x00f5>
{
    Little16 magic_packet_id;
    Little16 soff1;
    Little32 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x00f5>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f5>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f5>, soff1) == 2, "offsetof(NetPacket_Fixed<0x00f5>, soff1) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00f5>, amount) == 4, "offsetof(NetPacket_Fixed<0x00f5>, amount) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00f5>) == 8, "sizeof(NetPacket_Fixed<0x00f5>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00f5>) == 1, "alignof(NetPacket_Fixed<0x00f5>) == 1");

template<>
struct NetPacket_Fixed<0x00f6>
{
    Little16 magic_packet_id;
    Little16 soff1;
    Little32 amount;
};
static_assert(offsetof(NetPacket_Fixed<0x00f6>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f6>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f6>, soff1) == 2, "offsetof(NetPacket_Fixed<0x00f6>, soff1) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00f6>, amount) == 4, "offsetof(NetPacket_Fixed<0x00f6>, amount) == 4");
static_assert(sizeof(NetPacket_Fixed<0x00f6>) == 8, "sizeof(NetPacket_Fixed<0x00f6>) == 8");
static_assert(alignof(NetPacket_Fixed<0x00f6>) == 1, "alignof(NetPacket_Fixed<0x00f6>) == 1");

template<>
struct NetPacket_Fixed<0x00f7>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00f7>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f7>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00f7>) == 2, "sizeof(NetPacket_Fixed<0x00f7>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00f7>) == 1, "alignof(NetPacket_Fixed<0x00f7>) == 1");

template<>
struct NetPacket_Fixed<0x00f8>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00f8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f8>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x00f8>) == 2, "sizeof(NetPacket_Fixed<0x00f8>) == 2");
static_assert(alignof(NetPacket_Fixed<0x00f8>) == 1, "alignof(NetPacket_Fixed<0x00f8>) == 1");

template<>
struct NetPacket_Fixed<0x00f9>
{
    Little16 magic_packet_id;
    NetString<sizeof(PartyName)> party_name;
};
static_assert(offsetof(NetPacket_Fixed<0x00f9>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00f9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00f9>, party_name) == 2, "offsetof(NetPacket_Fixed<0x00f9>, party_name) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00f9>) == 26, "sizeof(NetPacket_Fixed<0x00f9>) == 26");
static_assert(alignof(NetPacket_Fixed<0x00f9>) == 1, "alignof(NetPacket_Fixed<0x00f9>) == 1");

template<>
struct NetPacket_Fixed<0x00fa>
{
    Little16 magic_packet_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x00fa>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00fa>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00fa>, flag) == 2, "offsetof(NetPacket_Fixed<0x00fa>, flag) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00fa>) == 3, "sizeof(NetPacket_Fixed<0x00fa>) == 3");
static_assert(alignof(NetPacket_Fixed<0x00fa>) == 1, "alignof(NetPacket_Fixed<0x00fa>) == 1");

template<>
struct NetPacket_Head<0x00fb>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    NetString<sizeof(PartyName)> party_name;
};
static_assert(offsetof(NetPacket_Head<0x00fb>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x00fb>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x00fb>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x00fb>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x00fb>, party_name) == 4, "offsetof(NetPacket_Head<0x00fb>, party_name) == 4");
static_assert(sizeof(NetPacket_Head<0x00fb>) == 28, "sizeof(NetPacket_Head<0x00fb>) == 28");
static_assert(alignof(NetPacket_Head<0x00fb>) == 1, "alignof(NetPacket_Head<0x00fb>) == 1");
template<>
struct NetPacket_Repeat<0x00fb>
{
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
    NetString<sizeof(MapName)> map_name;
    Byte leader;
    Byte online;
};
static_assert(offsetof(NetPacket_Repeat<0x00fb>, account_id) == 0, "offsetof(NetPacket_Repeat<0x00fb>, account_id) == 0");
static_assert(offsetof(NetPacket_Repeat<0x00fb>, char_name) == 4, "offsetof(NetPacket_Repeat<0x00fb>, char_name) == 4");
static_assert(offsetof(NetPacket_Repeat<0x00fb>, map_name) == 28, "offsetof(NetPacket_Repeat<0x00fb>, map_name) == 28");
static_assert(offsetof(NetPacket_Repeat<0x00fb>, leader) == 44, "offsetof(NetPacket_Repeat<0x00fb>, leader) == 44");
static_assert(offsetof(NetPacket_Repeat<0x00fb>, online) == 45, "offsetof(NetPacket_Repeat<0x00fb>, online) == 45");
static_assert(sizeof(NetPacket_Repeat<0x00fb>) == 46, "sizeof(NetPacket_Repeat<0x00fb>) == 46");
static_assert(alignof(NetPacket_Repeat<0x00fb>) == 1, "alignof(NetPacket_Repeat<0x00fb>) == 1");

template<>
struct NetPacket_Fixed<0x00fc>
{
    Little16 magic_packet_id;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Fixed<0x00fc>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00fc>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00fc>, account_id) == 2, "offsetof(NetPacket_Fixed<0x00fc>, account_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x00fc>) == 6, "sizeof(NetPacket_Fixed<0x00fc>) == 6");
static_assert(alignof(NetPacket_Fixed<0x00fc>) == 1, "alignof(NetPacket_Fixed<0x00fc>) == 1");

template<>
struct NetPacket_Fixed<0x00fd>
{
    Little16 magic_packet_id;
    NetString<sizeof(CharName)> char_name;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x00fd>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00fd>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00fd>, char_name) == 2, "offsetof(NetPacket_Fixed<0x00fd>, char_name) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00fd>, flag) == 26, "offsetof(NetPacket_Fixed<0x00fd>, flag) == 26");
static_assert(sizeof(NetPacket_Fixed<0x00fd>) == 27, "sizeof(NetPacket_Fixed<0x00fd>) == 27");
static_assert(alignof(NetPacket_Fixed<0x00fd>) == 1, "alignof(NetPacket_Fixed<0x00fd>) == 1");

template<>
struct NetPacket_Fixed<0x00fe>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(PartyName)> party_name;
};
static_assert(offsetof(NetPacket_Fixed<0x00fe>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00fe>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00fe>, account_id) == 2, "offsetof(NetPacket_Fixed<0x00fe>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00fe>, party_name) == 6, "offsetof(NetPacket_Fixed<0x00fe>, party_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00fe>) == 30, "sizeof(NetPacket_Fixed<0x00fe>) == 30");
static_assert(alignof(NetPacket_Fixed<0x00fe>) == 1, "alignof(NetPacket_Fixed<0x00fe>) == 1");

template<>
struct NetPacket_Fixed<0x00ff>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little32 flag;
};
static_assert(offsetof(NetPacket_Fixed<0x00ff>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x00ff>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x00ff>, account_id) == 2, "offsetof(NetPacket_Fixed<0x00ff>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x00ff>, flag) == 6, "offsetof(NetPacket_Fixed<0x00ff>, flag) == 6");
static_assert(sizeof(NetPacket_Fixed<0x00ff>) == 10, "sizeof(NetPacket_Fixed<0x00ff>) == 10");
static_assert(alignof(NetPacket_Fixed<0x00ff>) == 1, "alignof(NetPacket_Fixed<0x00ff>) == 1");

template<>
struct NetPacket_Fixed<0x0100>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x0100>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0100>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x0100>) == 2, "sizeof(NetPacket_Fixed<0x0100>) == 2");
static_assert(alignof(NetPacket_Fixed<0x0100>) == 1, "alignof(NetPacket_Fixed<0x0100>) == 1");

template<>
struct NetPacket_Fixed<0x0101>
{
    Little16 magic_packet_id;
    Little16 exp;
    Little16 item;
};
static_assert(offsetof(NetPacket_Fixed<0x0101>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0101>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0101>, exp) == 2, "offsetof(NetPacket_Fixed<0x0101>, exp) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0101>, item) == 4, "offsetof(NetPacket_Fixed<0x0101>, item) == 4");
static_assert(sizeof(NetPacket_Fixed<0x0101>) == 6, "sizeof(NetPacket_Fixed<0x0101>) == 6");
static_assert(alignof(NetPacket_Fixed<0x0101>) == 1, "alignof(NetPacket_Fixed<0x0101>) == 1");

template<>
struct NetPacket_Fixed<0x0102>
{
    Little16 magic_packet_id;
    Little16 exp;
    Little16 item;
};
static_assert(offsetof(NetPacket_Fixed<0x0102>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0102>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0102>, exp) == 2, "offsetof(NetPacket_Fixed<0x0102>, exp) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0102>, item) == 4, "offsetof(NetPacket_Fixed<0x0102>, item) == 4");
static_assert(sizeof(NetPacket_Fixed<0x0102>) == 6, "sizeof(NetPacket_Fixed<0x0102>) == 6");
static_assert(alignof(NetPacket_Fixed<0x0102>) == 1, "alignof(NetPacket_Fixed<0x0102>) == 1");

template<>
struct NetPacket_Fixed<0x0103>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(CharName)> unused_char_name;
};
static_assert(offsetof(NetPacket_Fixed<0x0103>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0103>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0103>, account_id) == 2, "offsetof(NetPacket_Fixed<0x0103>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0103>, unused_char_name) == 6, "offsetof(NetPacket_Fixed<0x0103>, unused_char_name) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0103>) == 30, "sizeof(NetPacket_Fixed<0x0103>) == 30");
static_assert(alignof(NetPacket_Fixed<0x0103>) == 1, "alignof(NetPacket_Fixed<0x0103>) == 1");

template<>
struct NetPacket_Fixed<0x0105>
{
    Little16 magic_packet_id;
    Little32 account_id;
    NetString<sizeof(CharName)> char_name;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x0105>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0105>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0105>, account_id) == 2, "offsetof(NetPacket_Fixed<0x0105>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0105>, char_name) == 6, "offsetof(NetPacket_Fixed<0x0105>, char_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0105>, flag) == 30, "offsetof(NetPacket_Fixed<0x0105>, flag) == 30");
static_assert(sizeof(NetPacket_Fixed<0x0105>) == 31, "sizeof(NetPacket_Fixed<0x0105>) == 31");
static_assert(alignof(NetPacket_Fixed<0x0105>) == 1, "alignof(NetPacket_Fixed<0x0105>) == 1");

template<>
struct NetPacket_Fixed<0x0106>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little16 hp;
    Little16 max_hp;
};
static_assert(offsetof(NetPacket_Fixed<0x0106>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0106>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0106>, account_id) == 2, "offsetof(NetPacket_Fixed<0x0106>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0106>, hp) == 6, "offsetof(NetPacket_Fixed<0x0106>, hp) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0106>, max_hp) == 8, "offsetof(NetPacket_Fixed<0x0106>, max_hp) == 8");
static_assert(sizeof(NetPacket_Fixed<0x0106>) == 10, "sizeof(NetPacket_Fixed<0x0106>) == 10");
static_assert(alignof(NetPacket_Fixed<0x0106>) == 1, "alignof(NetPacket_Fixed<0x0106>) == 1");

template<>
struct NetPacket_Fixed<0x0107>
{
    Little16 magic_packet_id;
    Little32 account_id;
    Little16 x;
    Little16 y;
};
static_assert(offsetof(NetPacket_Fixed<0x0107>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0107>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0107>, account_id) == 2, "offsetof(NetPacket_Fixed<0x0107>, account_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0107>, x) == 6, "offsetof(NetPacket_Fixed<0x0107>, x) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0107>, y) == 8, "offsetof(NetPacket_Fixed<0x0107>, y) == 8");
static_assert(sizeof(NetPacket_Fixed<0x0107>) == 10, "sizeof(NetPacket_Fixed<0x0107>) == 10");
static_assert(alignof(NetPacket_Fixed<0x0107>) == 1, "alignof(NetPacket_Fixed<0x0107>) == 1");

template<>
struct NetPacket_Head<0x0108>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x0108>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x0108>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x0108>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x0108>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x0108>) == 4, "sizeof(NetPacket_Head<0x0108>) == 4");
static_assert(alignof(NetPacket_Head<0x0108>) == 1, "alignof(NetPacket_Head<0x0108>) == 1");
template<>
struct NetPacket_Repeat<0x0108>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x0108>, c) == 0, "offsetof(NetPacket_Repeat<0x0108>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x0108>) == 1, "sizeof(NetPacket_Repeat<0x0108>) == 1");
static_assert(alignof(NetPacket_Repeat<0x0108>) == 1, "alignof(NetPacket_Repeat<0x0108>) == 1");

template<>
struct NetPacket_Head<0x0109>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 account_id;
};
static_assert(offsetof(NetPacket_Head<0x0109>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x0109>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x0109>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x0109>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x0109>, account_id) == 4, "offsetof(NetPacket_Head<0x0109>, account_id) == 4");
static_assert(sizeof(NetPacket_Head<0x0109>) == 8, "sizeof(NetPacket_Head<0x0109>) == 8");
static_assert(alignof(NetPacket_Head<0x0109>) == 1, "alignof(NetPacket_Head<0x0109>) == 1");
template<>
struct NetPacket_Repeat<0x0109>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x0109>, c) == 0, "offsetof(NetPacket_Repeat<0x0109>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x0109>) == 1, "sizeof(NetPacket_Repeat<0x0109>) == 1");
static_assert(alignof(NetPacket_Repeat<0x0109>) == 1, "alignof(NetPacket_Repeat<0x0109>) == 1");

template<>
struct NetPacket_Fixed<0x010c>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x010c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x010c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x010c>, block_id) == 2, "offsetof(NetPacket_Fixed<0x010c>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x010c>) == 6, "sizeof(NetPacket_Fixed<0x010c>) == 6");
static_assert(alignof(NetPacket_Fixed<0x010c>) == 1, "alignof(NetPacket_Fixed<0x010c>) == 1");

template<>
struct NetPacket_Fixed<0x010e>
{
    Little16 magic_packet_id;
    Little16 skill_id;
    Little16 level;
    Little16 sp;
    Little16 range;
    Byte can_raise;
};
static_assert(offsetof(NetPacket_Fixed<0x010e>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x010e>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x010e>, skill_id) == 2, "offsetof(NetPacket_Fixed<0x010e>, skill_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x010e>, level) == 4, "offsetof(NetPacket_Fixed<0x010e>, level) == 4");
static_assert(offsetof(NetPacket_Fixed<0x010e>, sp) == 6, "offsetof(NetPacket_Fixed<0x010e>, sp) == 6");
static_assert(offsetof(NetPacket_Fixed<0x010e>, range) == 8, "offsetof(NetPacket_Fixed<0x010e>, range) == 8");
static_assert(offsetof(NetPacket_Fixed<0x010e>, can_raise) == 10, "offsetof(NetPacket_Fixed<0x010e>, can_raise) == 10");
static_assert(sizeof(NetPacket_Fixed<0x010e>) == 11, "sizeof(NetPacket_Fixed<0x010e>) == 11");
static_assert(alignof(NetPacket_Fixed<0x010e>) == 1, "alignof(NetPacket_Fixed<0x010e>) == 1");

template<>
struct NetPacket_Head<0x010f>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x010f>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x010f>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x010f>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x010f>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x010f>) == 4, "sizeof(NetPacket_Head<0x010f>) == 4");
static_assert(alignof(NetPacket_Head<0x010f>) == 1, "alignof(NetPacket_Head<0x010f>) == 1");
template<>
struct NetPacket_Repeat<0x010f>
{
    NetSkillInfo info;
};
static_assert(offsetof(NetPacket_Repeat<0x010f>, info) == 0, "offsetof(NetPacket_Repeat<0x010f>, info) == 0");
static_assert(sizeof(NetPacket_Repeat<0x010f>) == 37, "sizeof(NetPacket_Repeat<0x010f>) == 37");
static_assert(alignof(NetPacket_Repeat<0x010f>) == 1, "alignof(NetPacket_Repeat<0x010f>) == 1");

template<>
struct NetPacket_Fixed<0x0110>
{
    Little16 magic_packet_id;
    Little16 skill_id;
    Little16 btype;
    Little16 zero1;
    Byte zero2;
    Byte type;
};
static_assert(offsetof(NetPacket_Fixed<0x0110>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0110>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0110>, skill_id) == 2, "offsetof(NetPacket_Fixed<0x0110>, skill_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0110>, btype) == 4, "offsetof(NetPacket_Fixed<0x0110>, btype) == 4");
static_assert(offsetof(NetPacket_Fixed<0x0110>, zero1) == 6, "offsetof(NetPacket_Fixed<0x0110>, zero1) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0110>, zero2) == 8, "offsetof(NetPacket_Fixed<0x0110>, zero2) == 8");
static_assert(offsetof(NetPacket_Fixed<0x0110>, type) == 9, "offsetof(NetPacket_Fixed<0x0110>, type) == 9");
static_assert(sizeof(NetPacket_Fixed<0x0110>) == 10, "sizeof(NetPacket_Fixed<0x0110>) == 10");
static_assert(alignof(NetPacket_Fixed<0x0110>) == 1, "alignof(NetPacket_Fixed<0x0110>) == 1");

template<>
struct NetPacket_Fixed<0x0112>
{
    Little16 magic_packet_id;
    Little16 skill_id;
};
static_assert(offsetof(NetPacket_Fixed<0x0112>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0112>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0112>, skill_id) == 2, "offsetof(NetPacket_Fixed<0x0112>, skill_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0112>) == 4, "sizeof(NetPacket_Fixed<0x0112>) == 4");
static_assert(alignof(NetPacket_Fixed<0x0112>) == 1, "alignof(NetPacket_Fixed<0x0112>) == 1");

template<>
struct NetPacket_Fixed<0x0118>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x0118>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0118>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x0118>) == 2, "sizeof(NetPacket_Fixed<0x0118>) == 2");
static_assert(alignof(NetPacket_Fixed<0x0118>) == 1, "alignof(NetPacket_Fixed<0x0118>) == 1");

template<>
struct NetPacket_Fixed<0x0119>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Byte zero;
};
static_assert(offsetof(NetPacket_Fixed<0x0119>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0119>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0119>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0119>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0119>, opt1) == 6, "offsetof(NetPacket_Fixed<0x0119>, opt1) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0119>, opt2) == 8, "offsetof(NetPacket_Fixed<0x0119>, opt2) == 8");
static_assert(offsetof(NetPacket_Fixed<0x0119>, option) == 10, "offsetof(NetPacket_Fixed<0x0119>, option) == 10");
static_assert(offsetof(NetPacket_Fixed<0x0119>, zero) == 12, "offsetof(NetPacket_Fixed<0x0119>, zero) == 12");
static_assert(sizeof(NetPacket_Fixed<0x0119>) == 13, "sizeof(NetPacket_Fixed<0x0119>) == 13");
static_assert(alignof(NetPacket_Fixed<0x0119>) == 1, "alignof(NetPacket_Fixed<0x0119>) == 1");

template<>
struct NetPacket_Fixed<0x0139>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 bl_x;
    Little16 bl_y;
    Little16 sd_x;
    Little16 sd_y;
    Little16 range;
};
static_assert(offsetof(NetPacket_Fixed<0x0139>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0139>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0139>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0139>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0139>, bl_x) == 6, "offsetof(NetPacket_Fixed<0x0139>, bl_x) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0139>, bl_y) == 8, "offsetof(NetPacket_Fixed<0x0139>, bl_y) == 8");
static_assert(offsetof(NetPacket_Fixed<0x0139>, sd_x) == 10, "offsetof(NetPacket_Fixed<0x0139>, sd_x) == 10");
static_assert(offsetof(NetPacket_Fixed<0x0139>, sd_y) == 12, "offsetof(NetPacket_Fixed<0x0139>, sd_y) == 12");
static_assert(offsetof(NetPacket_Fixed<0x0139>, range) == 14, "offsetof(NetPacket_Fixed<0x0139>, range) == 14");
static_assert(sizeof(NetPacket_Fixed<0x0139>) == 16, "sizeof(NetPacket_Fixed<0x0139>) == 16");
static_assert(alignof(NetPacket_Fixed<0x0139>) == 1, "alignof(NetPacket_Fixed<0x0139>) == 1");

template<>
struct NetPacket_Fixed<0x013a>
{
    Little16 magic_packet_id;
    Little16 attack_range;
};
static_assert(offsetof(NetPacket_Fixed<0x013a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x013a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x013a>, attack_range) == 2, "offsetof(NetPacket_Fixed<0x013a>, attack_range) == 2");
static_assert(sizeof(NetPacket_Fixed<0x013a>) == 4, "sizeof(NetPacket_Fixed<0x013a>) == 4");
static_assert(alignof(NetPacket_Fixed<0x013a>) == 1, "alignof(NetPacket_Fixed<0x013a>) == 1");

template<>
struct NetPacket_Fixed<0x013b>
{
    Little16 magic_packet_id;
    Little16 type;
};
static_assert(offsetof(NetPacket_Fixed<0x013b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x013b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x013b>, type) == 2, "offsetof(NetPacket_Fixed<0x013b>, type) == 2");
static_assert(sizeof(NetPacket_Fixed<0x013b>) == 4, "sizeof(NetPacket_Fixed<0x013b>) == 4");
static_assert(alignof(NetPacket_Fixed<0x013b>) == 1, "alignof(NetPacket_Fixed<0x013b>) == 1");

template<>
struct NetPacket_Fixed<0x013c>
{
    Little16 magic_packet_id;
    Little16 ioff2;
};
static_assert(offsetof(NetPacket_Fixed<0x013c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x013c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x013c>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x013c>, ioff2) == 2");
static_assert(sizeof(NetPacket_Fixed<0x013c>) == 4, "sizeof(NetPacket_Fixed<0x013c>) == 4");
static_assert(alignof(NetPacket_Fixed<0x013c>) == 1, "alignof(NetPacket_Fixed<0x013c>) == 1");

template<>
struct NetPacket_Fixed<0x0141>
{
    Little16 magic_packet_id;
    Little16 sp_type;
    Little16 zero;
    Little32 value_status;
    Little32 value_b_e;
};
static_assert(offsetof(NetPacket_Fixed<0x0141>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0141>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0141>, sp_type) == 2, "offsetof(NetPacket_Fixed<0x0141>, sp_type) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0141>, zero) == 4, "offsetof(NetPacket_Fixed<0x0141>, zero) == 4");
static_assert(offsetof(NetPacket_Fixed<0x0141>, value_status) == 6, "offsetof(NetPacket_Fixed<0x0141>, value_status) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0141>, value_b_e) == 10, "offsetof(NetPacket_Fixed<0x0141>, value_b_e) == 10");
static_assert(sizeof(NetPacket_Fixed<0x0141>) == 14, "sizeof(NetPacket_Fixed<0x0141>) == 14");
static_assert(alignof(NetPacket_Fixed<0x0141>) == 1, "alignof(NetPacket_Fixed<0x0141>) == 1");

template<>
struct NetPacket_Fixed<0x0142>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x0142>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0142>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0142>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0142>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0142>) == 6, "sizeof(NetPacket_Fixed<0x0142>) == 6");
static_assert(alignof(NetPacket_Fixed<0x0142>) == 1, "alignof(NetPacket_Fixed<0x0142>) == 1");

template<>
struct NetPacket_Fixed<0x0143>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little32 input_int_value;
};
static_assert(offsetof(NetPacket_Fixed<0x0143>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0143>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0143>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0143>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0143>, input_int_value) == 6, "offsetof(NetPacket_Fixed<0x0143>, input_int_value) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0143>) == 10, "sizeof(NetPacket_Fixed<0x0143>) == 10");
static_assert(alignof(NetPacket_Fixed<0x0143>) == 1, "alignof(NetPacket_Fixed<0x0143>) == 1");

template<>
struct NetPacket_Fixed<0x0146>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x0146>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0146>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0146>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0146>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0146>) == 6, "sizeof(NetPacket_Fixed<0x0146>) == 6");
static_assert(alignof(NetPacket_Fixed<0x0146>) == 1, "alignof(NetPacket_Fixed<0x0146>) == 1");

template<>
struct NetPacket_Fixed<0x0147>
{
    Little16 magic_packet_id;
    NetSkillInfo info;
};
static_assert(offsetof(NetPacket_Fixed<0x0147>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0147>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0147>, info) == 2, "offsetof(NetPacket_Fixed<0x0147>, info) == 2");
static_assert(sizeof(NetPacket_Fixed<0x0147>) == 39, "sizeof(NetPacket_Fixed<0x0147>) == 39");
static_assert(alignof(NetPacket_Fixed<0x0147>) == 1, "alignof(NetPacket_Fixed<0x0147>) == 1");

template<>
struct NetPacket_Fixed<0x0148>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 type;
};
static_assert(offsetof(NetPacket_Fixed<0x0148>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0148>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0148>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0148>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0148>, type) == 6, "offsetof(NetPacket_Fixed<0x0148>, type) == 6");
static_assert(sizeof(NetPacket_Fixed<0x0148>) == 8, "sizeof(NetPacket_Fixed<0x0148>) == 8");
static_assert(alignof(NetPacket_Fixed<0x0148>) == 1, "alignof(NetPacket_Fixed<0x0148>) == 1");

template<>
struct NetPacket_Fixed<0x014d>
{
    Little16 magic_packet_id;
};
static_assert(offsetof(NetPacket_Fixed<0x014d>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x014d>, magic_packet_id) == 0");
static_assert(sizeof(NetPacket_Fixed<0x014d>) == 2, "sizeof(NetPacket_Fixed<0x014d>) == 2");
static_assert(alignof(NetPacket_Fixed<0x014d>) == 1, "alignof(NetPacket_Fixed<0x014d>) == 1");

template<>
struct NetPacket_Fixed<0x018a>
{
    Little16 magic_packet_id;
    Little16 unused;
};
static_assert(offsetof(NetPacket_Fixed<0x018a>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x018a>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x018a>, unused) == 2, "offsetof(NetPacket_Fixed<0x018a>, unused) == 2");
static_assert(sizeof(NetPacket_Fixed<0x018a>) == 4, "sizeof(NetPacket_Fixed<0x018a>) == 4");
static_assert(alignof(NetPacket_Fixed<0x018a>) == 1, "alignof(NetPacket_Fixed<0x018a>) == 1");

template<>
struct NetPacket_Fixed<0x018b>
{
    Little16 magic_packet_id;
    Little16 okay;
};
static_assert(offsetof(NetPacket_Fixed<0x018b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x018b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x018b>, okay) == 2, "offsetof(NetPacket_Fixed<0x018b>, okay) == 2");
static_assert(sizeof(NetPacket_Fixed<0x018b>) == 4, "sizeof(NetPacket_Fixed<0x018b>) == 4");
static_assert(alignof(NetPacket_Fixed<0x018b>) == 1, "alignof(NetPacket_Fixed<0x018b>) == 1");

template<>
struct NetPacket_Fixed<0x0195>
{
    Little16 magic_packet_id;
    Little32 block_id;
    NetString<sizeof(PartyName)> party_name;
    NetString<sizeof(VString<23>)> guild_name;
    NetString<sizeof(VString<23>)> guild_pos;
    NetString<sizeof(VString<23>)> guild_pos_again;
};
static_assert(offsetof(NetPacket_Fixed<0x0195>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0195>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0195>, block_id) == 2, "offsetof(NetPacket_Fixed<0x0195>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0195>, party_name) == 6, "offsetof(NetPacket_Fixed<0x0195>, party_name) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0195>, guild_name) == 30, "offsetof(NetPacket_Fixed<0x0195>, guild_name) == 30");
static_assert(offsetof(NetPacket_Fixed<0x0195>, guild_pos) == 54, "offsetof(NetPacket_Fixed<0x0195>, guild_pos) == 54");
static_assert(offsetof(NetPacket_Fixed<0x0195>, guild_pos_again) == 78, "offsetof(NetPacket_Fixed<0x0195>, guild_pos_again) == 78");
static_assert(sizeof(NetPacket_Fixed<0x0195>) == 102, "sizeof(NetPacket_Fixed<0x0195>) == 102");
static_assert(alignof(NetPacket_Fixed<0x0195>) == 1, "alignof(NetPacket_Fixed<0x0195>) == 1");

template<>
struct NetPacket_Fixed<0x0196>
{
    Little16 magic_packet_id;
    Little16 sc_type;
    Little32 block_id;
    Byte flag;
};
static_assert(offsetof(NetPacket_Fixed<0x0196>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0196>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0196>, sc_type) == 2, "offsetof(NetPacket_Fixed<0x0196>, sc_type) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0196>, block_id) == 4, "offsetof(NetPacket_Fixed<0x0196>, block_id) == 4");
static_assert(offsetof(NetPacket_Fixed<0x0196>, flag) == 8, "offsetof(NetPacket_Fixed<0x0196>, flag) == 8");
static_assert(sizeof(NetPacket_Fixed<0x0196>) == 9, "sizeof(NetPacket_Fixed<0x0196>) == 9");
static_assert(alignof(NetPacket_Fixed<0x0196>) == 1, "alignof(NetPacket_Fixed<0x0196>) == 1");

template<>
struct NetPacket_Fixed<0x019b>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little32 type;
};
static_assert(offsetof(NetPacket_Fixed<0x019b>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x019b>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x019b>, block_id) == 2, "offsetof(NetPacket_Fixed<0x019b>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x019b>, type) == 6, "offsetof(NetPacket_Fixed<0x019b>, type) == 6");
static_assert(sizeof(NetPacket_Fixed<0x019b>) == 10, "sizeof(NetPacket_Fixed<0x019b>) == 10");
static_assert(alignof(NetPacket_Fixed<0x019b>) == 1, "alignof(NetPacket_Fixed<0x019b>) == 1");

template<>
struct NetPacket_Fixed<0x01b1>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 amount;
    Byte fail;
};
static_assert(offsetof(NetPacket_Fixed<0x01b1>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01b1>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01b1>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x01b1>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01b1>, amount) == 4, "offsetof(NetPacket_Fixed<0x01b1>, amount) == 4");
static_assert(offsetof(NetPacket_Fixed<0x01b1>, fail) == 6, "offsetof(NetPacket_Fixed<0x01b1>, fail) == 6");
static_assert(sizeof(NetPacket_Fixed<0x01b1>) == 7, "sizeof(NetPacket_Fixed<0x01b1>) == 7");
static_assert(alignof(NetPacket_Fixed<0x01b1>) == 1, "alignof(NetPacket_Fixed<0x01b1>) == 1");

template<>
struct NetPacket_Fixed<0x01c8>
{
    Little16 magic_packet_id;
    Little16 ioff2;
    Little16 name_id;
    Little32 block_id;
    Little16 amount;
    Byte ok;
};
static_assert(offsetof(NetPacket_Fixed<0x01c8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01c8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01c8>, ioff2) == 2, "offsetof(NetPacket_Fixed<0x01c8>, ioff2) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01c8>, name_id) == 4, "offsetof(NetPacket_Fixed<0x01c8>, name_id) == 4");
static_assert(offsetof(NetPacket_Fixed<0x01c8>, block_id) == 6, "offsetof(NetPacket_Fixed<0x01c8>, block_id) == 6");
static_assert(offsetof(NetPacket_Fixed<0x01c8>, amount) == 10, "offsetof(NetPacket_Fixed<0x01c8>, amount) == 10");
static_assert(offsetof(NetPacket_Fixed<0x01c8>, ok) == 12, "offsetof(NetPacket_Fixed<0x01c8>, ok) == 12");
static_assert(sizeof(NetPacket_Fixed<0x01c8>) == 13, "sizeof(NetPacket_Fixed<0x01c8>) == 13");
static_assert(alignof(NetPacket_Fixed<0x01c8>) == 1, "alignof(NetPacket_Fixed<0x01c8>) == 1");

template<>
struct NetPacket_Fixed<0x01d4>
{
    Little16 magic_packet_id;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Fixed<0x01d4>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01d4>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01d4>, block_id) == 2, "offsetof(NetPacket_Fixed<0x01d4>, block_id) == 2");
static_assert(sizeof(NetPacket_Fixed<0x01d4>) == 6, "sizeof(NetPacket_Fixed<0x01d4>) == 6");
static_assert(alignof(NetPacket_Fixed<0x01d4>) == 1, "alignof(NetPacket_Fixed<0x01d4>) == 1");

template<>
struct NetPacket_Head<0x01d5>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
    Little32 block_id;
};
static_assert(offsetof(NetPacket_Head<0x01d5>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x01d5>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x01d5>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x01d5>, magic_packet_length) == 2");
static_assert(offsetof(NetPacket_Head<0x01d5>, block_id) == 4, "offsetof(NetPacket_Head<0x01d5>, block_id) == 4");
static_assert(sizeof(NetPacket_Head<0x01d5>) == 8, "sizeof(NetPacket_Head<0x01d5>) == 8");
static_assert(alignof(NetPacket_Head<0x01d5>) == 1, "alignof(NetPacket_Head<0x01d5>) == 1");
template<>
struct NetPacket_Repeat<0x01d5>
{
    Byte c;
};
static_assert(offsetof(NetPacket_Repeat<0x01d5>, c) == 0, "offsetof(NetPacket_Repeat<0x01d5>, c) == 0");
static_assert(sizeof(NetPacket_Repeat<0x01d5>) == 1, "sizeof(NetPacket_Repeat<0x01d5>) == 1");
static_assert(alignof(NetPacket_Repeat<0x01d5>) == 1, "alignof(NetPacket_Repeat<0x01d5>) == 1");

template<>
struct NetPacket_Fixed<0x01d7>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Byte look_type;
    Little16 weapon_or_name_id_or_value;
    Little16 shield;
};
static_assert(offsetof(NetPacket_Fixed<0x01d7>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01d7>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01d7>, block_id) == 2, "offsetof(NetPacket_Fixed<0x01d7>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01d7>, look_type) == 6, "offsetof(NetPacket_Fixed<0x01d7>, look_type) == 6");
static_assert(offsetof(NetPacket_Fixed<0x01d7>, weapon_or_name_id_or_value) == 7, "offsetof(NetPacket_Fixed<0x01d7>, weapon_or_name_id_or_value) == 7");
static_assert(offsetof(NetPacket_Fixed<0x01d7>, shield) == 9, "offsetof(NetPacket_Fixed<0x01d7>, shield) == 9");
static_assert(sizeof(NetPacket_Fixed<0x01d7>) == 11, "sizeof(NetPacket_Fixed<0x01d7>) == 11");
static_assert(alignof(NetPacket_Fixed<0x01d7>) == 1, "alignof(NetPacket_Fixed<0x01d7>) == 1");

template<>
struct NetPacket_Fixed<0x01d8>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 speed;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Little16 species;
    Little16 hair_style;
    Little16 weapon;
    Little16 shield;
    Little16 head_bottom;
    Little16 head_top;
    Little16 head_mid;
    Little16 hair_color;
    Little16 clothes_color;
    Byte head_dir;
    Byte unused2;
    Little32 guild_id;
    Little16 guild_emblem_id;
    Little16 manner;
    Little16 opt3;
    Byte karma;
    Byte sex;
    NetPosition1 pos;
    Little16 gm_bits;
    Byte dead_sit;
    Little16 unused;
};
static_assert(offsetof(NetPacket_Fixed<0x01d8>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01d8>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, block_id) == 2, "offsetof(NetPacket_Fixed<0x01d8>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, speed) == 6, "offsetof(NetPacket_Fixed<0x01d8>, speed) == 6");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, opt1) == 8, "offsetof(NetPacket_Fixed<0x01d8>, opt1) == 8");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, opt2) == 10, "offsetof(NetPacket_Fixed<0x01d8>, opt2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, option) == 12, "offsetof(NetPacket_Fixed<0x01d8>, option) == 12");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, species) == 14, "offsetof(NetPacket_Fixed<0x01d8>, species) == 14");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, hair_style) == 16, "offsetof(NetPacket_Fixed<0x01d8>, hair_style) == 16");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, weapon) == 18, "offsetof(NetPacket_Fixed<0x01d8>, weapon) == 18");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, shield) == 20, "offsetof(NetPacket_Fixed<0x01d8>, shield) == 20");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, head_bottom) == 22, "offsetof(NetPacket_Fixed<0x01d8>, head_bottom) == 22");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, head_top) == 24, "offsetof(NetPacket_Fixed<0x01d8>, head_top) == 24");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, head_mid) == 26, "offsetof(NetPacket_Fixed<0x01d8>, head_mid) == 26");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, hair_color) == 28, "offsetof(NetPacket_Fixed<0x01d8>, hair_color) == 28");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, clothes_color) == 30, "offsetof(NetPacket_Fixed<0x01d8>, clothes_color) == 30");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, head_dir) == 32, "offsetof(NetPacket_Fixed<0x01d8>, head_dir) == 32");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, unused2) == 33, "offsetof(NetPacket_Fixed<0x01d8>, unused2) == 33");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, guild_id) == 34, "offsetof(NetPacket_Fixed<0x01d8>, guild_id) == 34");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, guild_emblem_id) == 38, "offsetof(NetPacket_Fixed<0x01d8>, guild_emblem_id) == 38");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, manner) == 40, "offsetof(NetPacket_Fixed<0x01d8>, manner) == 40");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, opt3) == 42, "offsetof(NetPacket_Fixed<0x01d8>, opt3) == 42");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, karma) == 44, "offsetof(NetPacket_Fixed<0x01d8>, karma) == 44");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, sex) == 45, "offsetof(NetPacket_Fixed<0x01d8>, sex) == 45");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, pos) == 46, "offsetof(NetPacket_Fixed<0x01d8>, pos) == 46");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, gm_bits) == 49, "offsetof(NetPacket_Fixed<0x01d8>, gm_bits) == 49");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, dead_sit) == 51, "offsetof(NetPacket_Fixed<0x01d8>, dead_sit) == 51");
static_assert(offsetof(NetPacket_Fixed<0x01d8>, unused) == 52, "offsetof(NetPacket_Fixed<0x01d8>, unused) == 52");
static_assert(sizeof(NetPacket_Fixed<0x01d8>) == 54, "sizeof(NetPacket_Fixed<0x01d8>) == 54");
static_assert(alignof(NetPacket_Fixed<0x01d8>) == 1, "alignof(NetPacket_Fixed<0x01d8>) == 1");

template<>
struct NetPacket_Fixed<0x01d9>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 speed;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Little16 species;
    Little16 hair_style;
    Little16 weapon;
    Little16 shield;
    Little16 head_bottom;
    Little16 head_top;
    Little16 head_mid;
    Little16 hair_color;
    Little16 clothes_color;
    Byte head_dir;
    Byte unused2;
    Little32 guild_id;
    Little16 guild_emblem_id;
    Little16 manner;
    Little16 opt3;
    Byte karma;
    Byte sex;
    NetPosition1 pos;
    Little16 gm_bits;
    Little16 unused;
};
static_assert(offsetof(NetPacket_Fixed<0x01d9>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01d9>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, block_id) == 2, "offsetof(NetPacket_Fixed<0x01d9>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, speed) == 6, "offsetof(NetPacket_Fixed<0x01d9>, speed) == 6");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, opt1) == 8, "offsetof(NetPacket_Fixed<0x01d9>, opt1) == 8");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, opt2) == 10, "offsetof(NetPacket_Fixed<0x01d9>, opt2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, option) == 12, "offsetof(NetPacket_Fixed<0x01d9>, option) == 12");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, species) == 14, "offsetof(NetPacket_Fixed<0x01d9>, species) == 14");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, hair_style) == 16, "offsetof(NetPacket_Fixed<0x01d9>, hair_style) == 16");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, weapon) == 18, "offsetof(NetPacket_Fixed<0x01d9>, weapon) == 18");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, shield) == 20, "offsetof(NetPacket_Fixed<0x01d9>, shield) == 20");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, head_bottom) == 22, "offsetof(NetPacket_Fixed<0x01d9>, head_bottom) == 22");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, head_top) == 24, "offsetof(NetPacket_Fixed<0x01d9>, head_top) == 24");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, head_mid) == 26, "offsetof(NetPacket_Fixed<0x01d9>, head_mid) == 26");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, hair_color) == 28, "offsetof(NetPacket_Fixed<0x01d9>, hair_color) == 28");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, clothes_color) == 30, "offsetof(NetPacket_Fixed<0x01d9>, clothes_color) == 30");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, head_dir) == 32, "offsetof(NetPacket_Fixed<0x01d9>, head_dir) == 32");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, unused2) == 33, "offsetof(NetPacket_Fixed<0x01d9>, unused2) == 33");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, guild_id) == 34, "offsetof(NetPacket_Fixed<0x01d9>, guild_id) == 34");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, guild_emblem_id) == 38, "offsetof(NetPacket_Fixed<0x01d9>, guild_emblem_id) == 38");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, manner) == 40, "offsetof(NetPacket_Fixed<0x01d9>, manner) == 40");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, opt3) == 42, "offsetof(NetPacket_Fixed<0x01d9>, opt3) == 42");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, karma) == 44, "offsetof(NetPacket_Fixed<0x01d9>, karma) == 44");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, sex) == 45, "offsetof(NetPacket_Fixed<0x01d9>, sex) == 45");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, pos) == 46, "offsetof(NetPacket_Fixed<0x01d9>, pos) == 46");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, gm_bits) == 49, "offsetof(NetPacket_Fixed<0x01d9>, gm_bits) == 49");
static_assert(offsetof(NetPacket_Fixed<0x01d9>, unused) == 51, "offsetof(NetPacket_Fixed<0x01d9>, unused) == 51");
static_assert(sizeof(NetPacket_Fixed<0x01d9>) == 53, "sizeof(NetPacket_Fixed<0x01d9>) == 53");
static_assert(alignof(NetPacket_Fixed<0x01d9>) == 1, "alignof(NetPacket_Fixed<0x01d9>) == 1");

template<>
struct NetPacket_Fixed<0x01da>
{
    Little16 magic_packet_id;
    Little32 block_id;
    Little16 speed;
    Little16 opt1;
    Little16 opt2;
    Little16 option;
    Little16 species;
    Little16 hair_style;
    Little16 weapon;
    Little16 shield;
    Little16 head_bottom;
    Little32 tick;
    Little16 head_top;
    Little16 head_mid;
    Little16 hair_color;
    Little16 clothes_color;
    Byte head_dir;
    Byte unused2;
    Little32 guild_id;
    Little16 guild_emblem_id;
    Little16 manner;
    Little16 opt3;
    Byte karma;
    Byte sex;
    NetPosition2 pos2;
    Little16 gm_bits;
    Byte five;
    Little16 unused;
};
static_assert(offsetof(NetPacket_Fixed<0x01da>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01da>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01da>, block_id) == 2, "offsetof(NetPacket_Fixed<0x01da>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01da>, speed) == 6, "offsetof(NetPacket_Fixed<0x01da>, speed) == 6");
static_assert(offsetof(NetPacket_Fixed<0x01da>, opt1) == 8, "offsetof(NetPacket_Fixed<0x01da>, opt1) == 8");
static_assert(offsetof(NetPacket_Fixed<0x01da>, opt2) == 10, "offsetof(NetPacket_Fixed<0x01da>, opt2) == 10");
static_assert(offsetof(NetPacket_Fixed<0x01da>, option) == 12, "offsetof(NetPacket_Fixed<0x01da>, option) == 12");
static_assert(offsetof(NetPacket_Fixed<0x01da>, species) == 14, "offsetof(NetPacket_Fixed<0x01da>, species) == 14");
static_assert(offsetof(NetPacket_Fixed<0x01da>, hair_style) == 16, "offsetof(NetPacket_Fixed<0x01da>, hair_style) == 16");
static_assert(offsetof(NetPacket_Fixed<0x01da>, weapon) == 18, "offsetof(NetPacket_Fixed<0x01da>, weapon) == 18");
static_assert(offsetof(NetPacket_Fixed<0x01da>, shield) == 20, "offsetof(NetPacket_Fixed<0x01da>, shield) == 20");
static_assert(offsetof(NetPacket_Fixed<0x01da>, head_bottom) == 22, "offsetof(NetPacket_Fixed<0x01da>, head_bottom) == 22");
static_assert(offsetof(NetPacket_Fixed<0x01da>, tick) == 24, "offsetof(NetPacket_Fixed<0x01da>, tick) == 24");
static_assert(offsetof(NetPacket_Fixed<0x01da>, head_top) == 28, "offsetof(NetPacket_Fixed<0x01da>, head_top) == 28");
static_assert(offsetof(NetPacket_Fixed<0x01da>, head_mid) == 30, "offsetof(NetPacket_Fixed<0x01da>, head_mid) == 30");
static_assert(offsetof(NetPacket_Fixed<0x01da>, hair_color) == 32, "offsetof(NetPacket_Fixed<0x01da>, hair_color) == 32");
static_assert(offsetof(NetPacket_Fixed<0x01da>, clothes_color) == 34, "offsetof(NetPacket_Fixed<0x01da>, clothes_color) == 34");
static_assert(offsetof(NetPacket_Fixed<0x01da>, head_dir) == 36, "offsetof(NetPacket_Fixed<0x01da>, head_dir) == 36");
static_assert(offsetof(NetPacket_Fixed<0x01da>, unused2) == 37, "offsetof(NetPacket_Fixed<0x01da>, unused2) == 37");
static_assert(offsetof(NetPacket_Fixed<0x01da>, guild_id) == 38, "offsetof(NetPacket_Fixed<0x01da>, guild_id) == 38");
static_assert(offsetof(NetPacket_Fixed<0x01da>, guild_emblem_id) == 42, "offsetof(NetPacket_Fixed<0x01da>, guild_emblem_id) == 42");
static_assert(offsetof(NetPacket_Fixed<0x01da>, manner) == 44, "offsetof(NetPacket_Fixed<0x01da>, manner) == 44");
static_assert(offsetof(NetPacket_Fixed<0x01da>, opt3) == 46, "offsetof(NetPacket_Fixed<0x01da>, opt3) == 46");
static_assert(offsetof(NetPacket_Fixed<0x01da>, karma) == 48, "offsetof(NetPacket_Fixed<0x01da>, karma) == 48");
static_assert(offsetof(NetPacket_Fixed<0x01da>, sex) == 49, "offsetof(NetPacket_Fixed<0x01da>, sex) == 49");
static_assert(offsetof(NetPacket_Fixed<0x01da>, pos2) == 50, "offsetof(NetPacket_Fixed<0x01da>, pos2) == 50");
static_assert(offsetof(NetPacket_Fixed<0x01da>, gm_bits) == 55, "offsetof(NetPacket_Fixed<0x01da>, gm_bits) == 55");
static_assert(offsetof(NetPacket_Fixed<0x01da>, five) == 57, "offsetof(NetPacket_Fixed<0x01da>, five) == 57");
static_assert(offsetof(NetPacket_Fixed<0x01da>, unused) == 58, "offsetof(NetPacket_Fixed<0x01da>, unused) == 58");
static_assert(sizeof(NetPacket_Fixed<0x01da>) == 60, "sizeof(NetPacket_Fixed<0x01da>) == 60");
static_assert(alignof(NetPacket_Fixed<0x01da>) == 1, "alignof(NetPacket_Fixed<0x01da>) == 1");

template<>
struct NetPacket_Fixed<0x01de>
{
    Little16 magic_packet_id;
    Little16 skill_id;
    Little32 src_id;
    Little32 dst_id;
    Little32 tick;
    Little32 sdelay;
    Little32 ddelay;
    Little32 damage;
    Little16 skill_level;
    Little16 div;
    Byte type_or_hit;
};
static_assert(offsetof(NetPacket_Fixed<0x01de>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x01de>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x01de>, skill_id) == 2, "offsetof(NetPacket_Fixed<0x01de>, skill_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x01de>, src_id) == 4, "offsetof(NetPacket_Fixed<0x01de>, src_id) == 4");
static_assert(offsetof(NetPacket_Fixed<0x01de>, dst_id) == 8, "offsetof(NetPacket_Fixed<0x01de>, dst_id) == 8");
static_assert(offsetof(NetPacket_Fixed<0x01de>, tick) == 12, "offsetof(NetPacket_Fixed<0x01de>, tick) == 12");
static_assert(offsetof(NetPacket_Fixed<0x01de>, sdelay) == 16, "offsetof(NetPacket_Fixed<0x01de>, sdelay) == 16");
static_assert(offsetof(NetPacket_Fixed<0x01de>, ddelay) == 20, "offsetof(NetPacket_Fixed<0x01de>, ddelay) == 20");
static_assert(offsetof(NetPacket_Fixed<0x01de>, damage) == 24, "offsetof(NetPacket_Fixed<0x01de>, damage) == 24");
static_assert(offsetof(NetPacket_Fixed<0x01de>, skill_level) == 28, "offsetof(NetPacket_Fixed<0x01de>, skill_level) == 28");
static_assert(offsetof(NetPacket_Fixed<0x01de>, div) == 30, "offsetof(NetPacket_Fixed<0x01de>, div) == 30");
static_assert(offsetof(NetPacket_Fixed<0x01de>, type_or_hit) == 32, "offsetof(NetPacket_Fixed<0x01de>, type_or_hit) == 32");
static_assert(sizeof(NetPacket_Fixed<0x01de>) == 33, "sizeof(NetPacket_Fixed<0x01de>) == 33");
static_assert(alignof(NetPacket_Fixed<0x01de>) == 1, "alignof(NetPacket_Fixed<0x01de>) == 1");

template<>
struct NetPacket_Head<0x01ee>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x01ee>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x01ee>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x01ee>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x01ee>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x01ee>) == 4, "sizeof(NetPacket_Head<0x01ee>) == 4");
static_assert(alignof(NetPacket_Head<0x01ee>) == 1, "alignof(NetPacket_Head<0x01ee>) == 1");
template<>
struct NetPacket_Repeat<0x01ee>
{
    Little16 ioff2;
    Little16 name_id;
    Byte item_type;
    Byte identify;
    Little16 amount;
    Little16 epos;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
};
static_assert(offsetof(NetPacket_Repeat<0x01ee>, ioff2) == 0, "offsetof(NetPacket_Repeat<0x01ee>, ioff2) == 0");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, name_id) == 2, "offsetof(NetPacket_Repeat<0x01ee>, name_id) == 2");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, item_type) == 4, "offsetof(NetPacket_Repeat<0x01ee>, item_type) == 4");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, identify) == 5, "offsetof(NetPacket_Repeat<0x01ee>, identify) == 5");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, amount) == 6, "offsetof(NetPacket_Repeat<0x01ee>, amount) == 6");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, epos) == 8, "offsetof(NetPacket_Repeat<0x01ee>, epos) == 8");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, card0) == 10, "offsetof(NetPacket_Repeat<0x01ee>, card0) == 10");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, card1) == 12, "offsetof(NetPacket_Repeat<0x01ee>, card1) == 12");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, card2) == 14, "offsetof(NetPacket_Repeat<0x01ee>, card2) == 14");
static_assert(offsetof(NetPacket_Repeat<0x01ee>, card3) == 16, "offsetof(NetPacket_Repeat<0x01ee>, card3) == 16");
static_assert(sizeof(NetPacket_Repeat<0x01ee>) == 18, "sizeof(NetPacket_Repeat<0x01ee>) == 18");
static_assert(alignof(NetPacket_Repeat<0x01ee>) == 1, "alignof(NetPacket_Repeat<0x01ee>) == 1");

template<>
struct NetPacket_Head<0x01f0>
{
    Little16 magic_packet_id;
    Little16 magic_packet_length;
};
static_assert(offsetof(NetPacket_Head<0x01f0>, magic_packet_id) == 0, "offsetof(NetPacket_Head<0x01f0>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Head<0x01f0>, magic_packet_length) == 2, "offsetof(NetPacket_Head<0x01f0>, magic_packet_length) == 2");
static_assert(sizeof(NetPacket_Head<0x01f0>) == 4, "sizeof(NetPacket_Head<0x01f0>) == 4");
static_assert(alignof(NetPacket_Head<0x01f0>) == 1, "alignof(NetPacket_Head<0x01f0>) == 1");
template<>
struct NetPacket_Repeat<0x01f0>
{
    Little16 soff1;
    Little16 name_id;
    Byte item_type;
    Byte identify;
    Little16 amount;
    Little16 epos_zero;
    Little16 card0;
    Little16 card1;
    Little16 card2;
    Little16 card3;
};
static_assert(offsetof(NetPacket_Repeat<0x01f0>, soff1) == 0, "offsetof(NetPacket_Repeat<0x01f0>, soff1) == 0");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, name_id) == 2, "offsetof(NetPacket_Repeat<0x01f0>, name_id) == 2");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, item_type) == 4, "offsetof(NetPacket_Repeat<0x01f0>, item_type) == 4");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, identify) == 5, "offsetof(NetPacket_Repeat<0x01f0>, identify) == 5");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, amount) == 6, "offsetof(NetPacket_Repeat<0x01f0>, amount) == 6");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, epos_zero) == 8, "offsetof(NetPacket_Repeat<0x01f0>, epos_zero) == 8");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, card0) == 10, "offsetof(NetPacket_Repeat<0x01f0>, card0) == 10");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, card1) == 12, "offsetof(NetPacket_Repeat<0x01f0>, card1) == 12");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, card2) == 14, "offsetof(NetPacket_Repeat<0x01f0>, card2) == 14");
static_assert(offsetof(NetPacket_Repeat<0x01f0>, card3) == 16, "offsetof(NetPacket_Repeat<0x01f0>, card3) == 16");
static_assert(sizeof(NetPacket_Repeat<0x01f0>) == 18, "sizeof(NetPacket_Repeat<0x01f0>) == 18");
static_assert(alignof(NetPacket_Repeat<0x01f0>) == 1, "alignof(NetPacket_Repeat<0x01f0>) == 1");

template<>
struct NetPacket_Fixed<0x020c>
{
    Little16 magic_packet_id;
    Little32 block_id;
    IP4Address ip;
};
static_assert(offsetof(NetPacket_Fixed<0x020c>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x020c>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x020c>, block_id) == 2, "offsetof(NetPacket_Fixed<0x020c>, block_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x020c>, ip) == 6, "offsetof(NetPacket_Fixed<0x020c>, ip) == 6");
static_assert(sizeof(NetPacket_Fixed<0x020c>) == 10, "sizeof(NetPacket_Fixed<0x020c>) == 10");
static_assert(alignof(NetPacket_Fixed<0x020c>) == 1, "alignof(NetPacket_Fixed<0x020c>) == 1");

template<>
struct NetPacket_Fixed<0x0212>
{
    Little16 magic_packet_id;
    Little32 npc_id;
    Little16 command;
    Little32 id;
    Little16 x;
    Little16 y;
};
static_assert(offsetof(NetPacket_Fixed<0x0212>, magic_packet_id) == 0, "offsetof(NetPacket_Fixed<0x0212>, magic_packet_id) == 0");
static_assert(offsetof(NetPacket_Fixed<0x0212>, npc_id) == 2, "offsetof(NetPacket_Fixed<0x0212>, npc_id) == 2");
static_assert(offsetof(NetPacket_Fixed<0x0212>, command) == 6, "offsetof(NetPacket_Fixed<0x0212>, command) == 6");
static_assert(offsetof(NetPacket_Fixed<0x0212>, id) == 8, "offsetof(NetPacket_Fixed<0x0212>, id) == 8");
static_assert(offsetof(NetPacket_Fixed<0x0212>, x) == 12, "offsetof(NetPacket_Fixed<0x0212>, x) == 12");
static_assert(offsetof(NetPacket_Fixed<0x0212>, y) == 14, "offsetof(NetPacket_Fixed<0x0212>, y) == 14");
static_assert(sizeof(NetPacket_Fixed<0x0212>) == 16, "sizeof(NetPacket_Fixed<0x0212>) == 16");
static_assert(alignof(NetPacket_Fixed<0x0212>) == 1, "alignof(NetPacket_Fixed<0x0212>) == 1");


inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0072> *network, Packet_Fixed<0x0072> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_id, native.char_id);
    rv &= native_to_network(&network->login_id1, native.login_id1);
    rv &= native_to_network(&network->client_tick, native.client_tick);
    rv &= native_to_network(&network->sex, native.sex);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0072> *native, NetPacket_Fixed<0x0072> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_id, network.char_id);
    rv &= network_to_native(&native->login_id1, network.login_id1);
    rv &= network_to_native(&native->client_tick, network.client_tick);
    rv &= network_to_native(&native->sex, network.sex);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0073> *network, Packet_Fixed<0x0073> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->tick, native.tick);
    rv &= native_to_network(&network->pos, native.pos);
    rv &= native_to_network(&network->five1, native.five1);
    rv &= native_to_network(&network->five2, native.five2);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0073> *native, NetPacket_Fixed<0x0073> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->tick, network.tick);
    rv &= network_to_native(&native->pos, network.pos);
    rv &= network_to_native(&native->five1, network.five1);
    rv &= network_to_native(&native->five2, network.five2);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0078> *network, Packet_Fixed<0x0078> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->unused_hair_style, native.unused_hair_style);
    rv &= native_to_network(&network->unused_weapon, native.unused_weapon);
    rv &= native_to_network(&network->unused_head_bottom_or_species_again, native.unused_head_bottom_or_species_again);
    rv &= native_to_network(&network->unused_shield_or_part_of_guild_emblem, native.unused_shield_or_part_of_guild_emblem);
    rv &= native_to_network(&network->unused_head_top_or_unused_part_of_guild_emblem, native.unused_head_top_or_unused_part_of_guild_emblem);
    rv &= native_to_network(&network->unused_head_mid_or_part_of_guild_id, native.unused_head_mid_or_part_of_guild_id);
    rv &= native_to_network(&network->unused_hair_color_or_part_of_guild_id, native.unused_hair_color_or_part_of_guild_id);
    rv &= native_to_network(&network->unused_clothes_color, native.unused_clothes_color);
    rv &= native_to_network(&network->unused_1, native.unused_1);
    rv &= native_to_network(&network->unused_2, native.unused_2);
    rv &= native_to_network(&network->unused_pos_again, native.unused_pos_again);
    rv &= native_to_network(&network->unused_4b, native.unused_4b);
    rv &= native_to_network(&network->unused_5, native.unused_5);
    rv &= native_to_network(&network->unused_zero_1, native.unused_zero_1);
    rv &= native_to_network(&network->unused_zero_2, native.unused_zero_2);
    rv &= native_to_network(&network->unused_sex, native.unused_sex);
    rv &= native_to_network(&network->pos, native.pos);
    rv &= native_to_network(&network->five1, native.five1);
    rv &= native_to_network(&network->five2, native.five2);
    rv &= native_to_network(&network->zero, native.zero);
    rv &= native_to_network(&network->level, native.level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0078> *native, NetPacket_Fixed<0x0078> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->unused_hair_style, network.unused_hair_style);
    rv &= network_to_native(&native->unused_weapon, network.unused_weapon);
    rv &= network_to_native(&native->unused_head_bottom_or_species_again, network.unused_head_bottom_or_species_again);
    rv &= network_to_native(&native->unused_shield_or_part_of_guild_emblem, network.unused_shield_or_part_of_guild_emblem);
    rv &= network_to_native(&native->unused_head_top_or_unused_part_of_guild_emblem, network.unused_head_top_or_unused_part_of_guild_emblem);
    rv &= network_to_native(&native->unused_head_mid_or_part_of_guild_id, network.unused_head_mid_or_part_of_guild_id);
    rv &= network_to_native(&native->unused_hair_color_or_part_of_guild_id, network.unused_hair_color_or_part_of_guild_id);
    rv &= network_to_native(&native->unused_clothes_color, network.unused_clothes_color);
    rv &= network_to_native(&native->unused_1, network.unused_1);
    rv &= network_to_native(&native->unused_2, network.unused_2);
    rv &= network_to_native(&native->unused_pos_again, network.unused_pos_again);
    rv &= network_to_native(&native->unused_4b, network.unused_4b);
    rv &= network_to_native(&native->unused_5, network.unused_5);
    rv &= network_to_native(&native->unused_zero_1, network.unused_zero_1);
    rv &= network_to_native(&native->unused_zero_2, network.unused_zero_2);
    rv &= network_to_native(&native->unused_sex, network.unused_sex);
    rv &= network_to_native(&native->pos, network.pos);
    rv &= network_to_native(&native->five1, network.five1);
    rv &= network_to_native(&native->five2, network.five2);
    rv &= network_to_native(&native->zero, network.zero);
    rv &= network_to_native(&native->level, network.level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x007b> *network, Packet_Fixed<0x007b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->mob_class, native.mob_class);
    rv &= native_to_network(&network->unused_hair_style, native.unused_hair_style);
    rv &= native_to_network(&network->unused_weapon, native.unused_weapon);
    rv &= native_to_network(&network->unused_head_bottom, native.unused_head_bottom);
    rv &= native_to_network(&network->tick_and_maybe_part_of_guild_emblem, native.tick_and_maybe_part_of_guild_emblem);
    rv &= native_to_network(&network->unused_shield_or_maybe_part_of_guild_emblem, native.unused_shield_or_maybe_part_of_guild_emblem);
    rv &= native_to_network(&network->unused_head_top_or_maybe_part_of_guild_id, native.unused_head_top_or_maybe_part_of_guild_id);
    rv &= native_to_network(&network->unused_head_mid_or_maybe_part_of_guild_id, native.unused_head_mid_or_maybe_part_of_guild_id);
    rv &= native_to_network(&network->unused_hair_color, native.unused_hair_color);
    rv &= native_to_network(&network->unused_clothes_color, native.unused_clothes_color);
    rv &= native_to_network(&network->unused_1, native.unused_1);
    rv &= native_to_network(&network->unused_2, native.unused_2);
    rv &= native_to_network(&network->unused_3, native.unused_3);
    rv &= native_to_network(&network->unused_4, native.unused_4);
    rv &= native_to_network(&network->unused_5, native.unused_5);
    rv &= native_to_network(&network->unused_zero_1, native.unused_zero_1);
    rv &= native_to_network(&network->unused_zero_2, native.unused_zero_2);
    rv &= native_to_network(&network->unused_sex, native.unused_sex);
    rv &= native_to_network(&network->pos2, native.pos2);
    rv &= native_to_network(&network->zero, native.zero);
    rv &= native_to_network(&network->five1, native.five1);
    rv &= native_to_network(&network->five2, native.five2);
    rv &= native_to_network(&network->level, native.level);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x007b> *native, NetPacket_Fixed<0x007b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->mob_class, network.mob_class);
    rv &= network_to_native(&native->unused_hair_style, network.unused_hair_style);
    rv &= network_to_native(&native->unused_weapon, network.unused_weapon);
    rv &= network_to_native(&native->unused_head_bottom, network.unused_head_bottom);
    rv &= network_to_native(&native->tick_and_maybe_part_of_guild_emblem, network.tick_and_maybe_part_of_guild_emblem);
    rv &= network_to_native(&native->unused_shield_or_maybe_part_of_guild_emblem, network.unused_shield_or_maybe_part_of_guild_emblem);
    rv &= network_to_native(&native->unused_head_top_or_maybe_part_of_guild_id, network.unused_head_top_or_maybe_part_of_guild_id);
    rv &= network_to_native(&native->unused_head_mid_or_maybe_part_of_guild_id, network.unused_head_mid_or_maybe_part_of_guild_id);
    rv &= network_to_native(&native->unused_hair_color, network.unused_hair_color);
    rv &= network_to_native(&native->unused_clothes_color, network.unused_clothes_color);
    rv &= network_to_native(&native->unused_1, network.unused_1);
    rv &= network_to_native(&native->unused_2, network.unused_2);
    rv &= network_to_native(&native->unused_3, network.unused_3);
    rv &= network_to_native(&native->unused_4, network.unused_4);
    rv &= network_to_native(&native->unused_5, network.unused_5);
    rv &= network_to_native(&native->unused_zero_1, network.unused_zero_1);
    rv &= network_to_native(&native->unused_zero_2, network.unused_zero_2);
    rv &= network_to_native(&native->unused_sex, network.unused_sex);
    rv &= network_to_native(&native->pos2, network.pos2);
    rv &= network_to_native(&native->zero, network.zero);
    rv &= network_to_native(&native->five1, network.five1);
    rv &= network_to_native(&native->five2, network.five2);
    rv &= network_to_native(&native->level, network.level);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x007c> *network, Packet_Fixed<0x007c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->unknown_1, native.unknown_1);
    rv &= native_to_network(&network->unknown_2, native.unknown_2);
    rv &= native_to_network(&network->unknown_3, native.unknown_3);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->unknown_4, native.unknown_4);
    rv &= native_to_network(&network->unknown_5, native.unknown_5);
    rv &= native_to_network(&network->unknown_6, native.unknown_6);
    rv &= native_to_network(&network->unknown_7, native.unknown_7);
    rv &= native_to_network(&network->unknown_8, native.unknown_8);
    rv &= native_to_network(&network->unknown_9, native.unknown_9);
    rv &= native_to_network(&network->unknown_10, native.unknown_10);
    rv &= native_to_network(&network->pos, native.pos);
    rv &= native_to_network(&network->unknown_11, native.unknown_11);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x007c> *native, NetPacket_Fixed<0x007c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->unknown_1, network.unknown_1);
    rv &= network_to_native(&native->unknown_2, network.unknown_2);
    rv &= network_to_native(&native->unknown_3, network.unknown_3);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->unknown_4, network.unknown_4);
    rv &= network_to_native(&native->unknown_5, network.unknown_5);
    rv &= network_to_native(&native->unknown_6, network.unknown_6);
    rv &= network_to_native(&native->unknown_7, network.unknown_7);
    rv &= network_to_native(&native->unknown_8, network.unknown_8);
    rv &= network_to_native(&native->unknown_9, network.unknown_9);
    rv &= network_to_native(&native->unknown_10, network.unknown_10);
    rv &= network_to_native(&native->pos, network.pos);
    rv &= network_to_native(&native->unknown_11, network.unknown_11);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x007d> *network, Packet_Fixed<0x007d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x007d> *native, NetPacket_Fixed<0x007d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x007e> *network, Packet_Fixed<0x007e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->client_tick, native.client_tick);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x007e> *native, NetPacket_Fixed<0x007e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->client_tick, network.client_tick);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x007f> *network, Packet_Fixed<0x007f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->tick, native.tick);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x007f> *native, NetPacket_Fixed<0x007f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->tick, network.tick);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0080> *network, Packet_Fixed<0x0080> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0080> *native, NetPacket_Fixed<0x0080> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0085> *network, Packet_Fixed<0x0085> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->pos, native.pos);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0085> *native, NetPacket_Fixed<0x0085> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->pos, network.pos);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0087> *network, Packet_Fixed<0x0087> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->tick, native.tick);
    rv &= native_to_network(&network->pos2, native.pos2);
    rv &= native_to_network(&network->zero, native.zero);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0087> *native, NetPacket_Fixed<0x0087> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->tick, network.tick);
    rv &= network_to_native(&native->pos2, network.pos2);
    rv &= network_to_native(&native->zero, network.zero);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0088> *network, Packet_Fixed<0x0088> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0088> *native, NetPacket_Fixed<0x0088> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0089> *network, Packet_Fixed<0x0089> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->target_id, native.target_id);
    rv &= native_to_network(&network->action, native.action);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0089> *native, NetPacket_Fixed<0x0089> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->target_id, network.target_id);
    rv &= network_to_native(&native->action, network.action);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x008a> *network, Packet_Fixed<0x008a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->src_id, native.src_id);
    rv &= native_to_network(&network->dst_id, native.dst_id);
    rv &= native_to_network(&network->tick, native.tick);
    rv &= native_to_network(&network->sdelay, native.sdelay);
    rv &= native_to_network(&network->ddelay, native.ddelay);
    rv &= native_to_network(&network->damage, native.damage);
    rv &= native_to_network(&network->div, native.div);
    rv &= native_to_network(&network->damage_type, native.damage_type);
    rv &= native_to_network(&network->damage2, native.damage2);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x008a> *native, NetPacket_Fixed<0x008a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->src_id, network.src_id);
    rv &= network_to_native(&native->dst_id, network.dst_id);
    rv &= network_to_native(&native->tick, network.tick);
    rv &= network_to_native(&native->sdelay, network.sdelay);
    rv &= network_to_native(&native->ddelay, network.ddelay);
    rv &= network_to_native(&native->damage, network.damage);
    rv &= network_to_native(&native->div, network.div);
    rv &= network_to_native(&native->damage_type, network.damage_type);
    rv &= network_to_native(&native->damage2, network.damage2);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x008c> *network, Packet_Head<0x008c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x008c> *native, NetPacket_Head<0x008c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x008c> *network, Packet_Repeat<0x008c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x008c> *native, NetPacket_Repeat<0x008c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x008d> *network, Packet_Head<0x008d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x008d> *native, NetPacket_Head<0x008d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x008d> *network, Packet_Repeat<0x008d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x008d> *native, NetPacket_Repeat<0x008d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x008e> *network, Packet_Head<0x008e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x008e> *native, NetPacket_Head<0x008e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x008e> *network, Packet_Repeat<0x008e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x008e> *native, NetPacket_Repeat<0x008e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0090> *network, Packet_Fixed<0x0090> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0090> *native, NetPacket_Fixed<0x0090> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0091> *network, Packet_Fixed<0x0091> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0091> *native, NetPacket_Fixed<0x0091> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0092> *network, Packet_Fixed<0x0092> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    rv &= native_to_network(&network->ip, native.ip);
    rv &= native_to_network(&network->port, native.port);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0092> *native, NetPacket_Fixed<0x0092> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    rv &= network_to_native(&native->ip, network.ip);
    rv &= network_to_native(&native->port, network.port);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0094> *network, Packet_Fixed<0x0094> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0094> *native, NetPacket_Fixed<0x0094> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0095> *network, Packet_Fixed<0x0095> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0095> *native, NetPacket_Fixed<0x0095> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x0096> *network, Packet_Head<0x0096> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->target_name, native.target_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x0096> *native, NetPacket_Head<0x0096> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->target_name, network.target_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x0096> *network, Packet_Repeat<0x0096> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x0096> *native, NetPacket_Repeat<0x0096> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x0097> *network, Packet_Head<0x0097> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->char_name, native.char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x0097> *native, NetPacket_Head<0x0097> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->char_name, network.char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x0097> *network, Packet_Repeat<0x0097> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x0097> *native, NetPacket_Repeat<0x0097> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0098> *network, Packet_Fixed<0x0098> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0098> *native, NetPacket_Fixed<0x0098> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x009a> *network, Packet_Head<0x009a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x009a> *native, NetPacket_Head<0x009a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x009a> *network, Packet_Repeat<0x009a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x009a> *native, NetPacket_Repeat<0x009a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x009b> *network, Packet_Fixed<0x009b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unused, native.unused);
    rv &= native_to_network(&network->client_dir, native.client_dir);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x009b> *native, NetPacket_Fixed<0x009b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unused, network.unused);
    rv &= network_to_native(&native->client_dir, network.client_dir);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x009c> *network, Packet_Fixed<0x009c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->zero, native.zero);
    rv &= native_to_network(&network->client_dir, native.client_dir);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x009c> *native, NetPacket_Fixed<0x009c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->zero, network.zero);
    rv &= network_to_native(&native->client_dir, network.client_dir);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x009d> *network, Packet_Fixed<0x009d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->subx, native.subx);
    rv &= native_to_network(&network->suby, native.suby);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x009d> *native, NetPacket_Fixed<0x009d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->subx, network.subx);
    rv &= network_to_native(&native->suby, network.suby);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x009e> *network, Packet_Fixed<0x009e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    rv &= native_to_network(&network->subx, native.subx);
    rv &= native_to_network(&network->suby, native.suby);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x009e> *native, NetPacket_Fixed<0x009e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    rv &= network_to_native(&native->subx, network.subx);
    rv &= network_to_native(&native->suby, network.suby);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x009f> *network, Packet_Fixed<0x009f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->object_id, native.object_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x009f> *native, NetPacket_Fixed<0x009f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->object_id, network.object_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00a0> *network, Packet_Fixed<0x00a0> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->broken_or_attribute, native.broken_or_attribute);
    rv &= native_to_network(&network->refine, native.refine);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    rv &= native_to_network(&network->epos, native.epos);
    rv &= native_to_network(&network->item_type, native.item_type);
    rv &= native_to_network(&network->pickup_fail, native.pickup_fail);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00a0> *native, NetPacket_Fixed<0x00a0> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->broken_or_attribute, network.broken_or_attribute);
    rv &= network_to_native(&native->refine, network.refine);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    rv &= network_to_native(&native->epos, network.epos);
    rv &= network_to_native(&native->item_type, network.item_type);
    rv &= network_to_native(&native->pickup_fail, network.pickup_fail);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00a1> *network, Packet_Fixed<0x00a1> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00a1> *native, NetPacket_Fixed<0x00a1> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00a2> *network, Packet_Fixed<0x00a2> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00a2> *native, NetPacket_Fixed<0x00a2> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00a4> *network, Packet_Head<0x00a4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00a4> *native, NetPacket_Head<0x00a4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00a4> *network, Packet_Repeat<0x00a4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->item_type, native.item_type);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->epos_pc, native.epos_pc);
    rv &= native_to_network(&network->epos_inv, native.epos_inv);
    rv &= native_to_network(&network->broken_or_attribute, native.broken_or_attribute);
    rv &= native_to_network(&network->refine, native.refine);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00a4> *native, NetPacket_Repeat<0x00a4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->item_type, network.item_type);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->epos_pc, network.epos_pc);
    rv &= network_to_native(&native->epos_inv, network.epos_inv);
    rv &= network_to_native(&native->broken_or_attribute, network.broken_or_attribute);
    rv &= network_to_native(&native->refine, network.refine);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00a6> *network, Packet_Head<0x00a6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00a6> *native, NetPacket_Head<0x00a6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00a6> *network, Packet_Repeat<0x00a6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->soff1, native.soff1);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->item_type, native.item_type);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->epos_id, native.epos_id);
    rv &= native_to_network(&network->epos_stor, native.epos_stor);
    rv &= native_to_network(&network->broken_or_attribute, native.broken_or_attribute);
    rv &= native_to_network(&network->refine, native.refine);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00a6> *native, NetPacket_Repeat<0x00a6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->soff1, network.soff1);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->item_type, network.item_type);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->epos_id, network.epos_id);
    rv &= network_to_native(&native->epos_stor, network.epos_stor);
    rv &= network_to_native(&native->broken_or_attribute, network.broken_or_attribute);
    rv &= network_to_native(&native->refine, network.refine);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00a7> *network, Packet_Fixed<0x00a7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->unused_id, native.unused_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00a7> *native, NetPacket_Fixed<0x00a7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->unused_id, network.unused_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00a8> *network, Packet_Fixed<0x00a8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->ok, native.ok);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00a8> *native, NetPacket_Fixed<0x00a8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->ok, network.ok);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00a9> *network, Packet_Fixed<0x00a9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->epos_ignored, native.epos_ignored);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00a9> *native, NetPacket_Fixed<0x00a9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->epos_ignored, network.epos_ignored);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00aa> *network, Packet_Fixed<0x00aa> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->epos, native.epos);
    rv &= native_to_network(&network->ok, native.ok);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00aa> *native, NetPacket_Fixed<0x00aa> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->epos, network.epos);
    rv &= network_to_native(&native->ok, network.ok);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ab> *network, Packet_Fixed<0x00ab> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ab> *native, NetPacket_Fixed<0x00ab> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ac> *network, Packet_Fixed<0x00ac> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->epos, native.epos);
    rv &= native_to_network(&network->ok, native.ok);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ac> *native, NetPacket_Fixed<0x00ac> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->epos, network.epos);
    rv &= network_to_native(&native->ok, network.ok);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00af> *network, Packet_Fixed<0x00af> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00af> *native, NetPacket_Fixed<0x00af> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b0> *network, Packet_Fixed<0x00b0> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sp_type, native.sp_type);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b0> *native, NetPacket_Fixed<0x00b0> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sp_type, network.sp_type);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b1> *network, Packet_Fixed<0x00b1> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sp_type, native.sp_type);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b1> *native, NetPacket_Fixed<0x00b1> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sp_type, network.sp_type);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b2> *network, Packet_Fixed<0x00b2> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b2> *native, NetPacket_Fixed<0x00b2> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b3> *network, Packet_Fixed<0x00b3> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->one, native.one);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b3> *native, NetPacket_Fixed<0x00b3> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->one, network.one);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00b4> *network, Packet_Head<0x00b4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00b4> *native, NetPacket_Head<0x00b4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00b4> *network, Packet_Repeat<0x00b4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00b4> *native, NetPacket_Repeat<0x00b4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b5> *network, Packet_Fixed<0x00b5> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b5> *native, NetPacket_Fixed<0x00b5> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b6> *network, Packet_Fixed<0x00b6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b6> *native, NetPacket_Fixed<0x00b6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00b7> *network, Packet_Head<0x00b7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00b7> *native, NetPacket_Head<0x00b7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00b7> *network, Packet_Repeat<0x00b7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00b7> *native, NetPacket_Repeat<0x00b7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b8> *network, Packet_Fixed<0x00b8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->npc_id, native.npc_id);
    rv &= native_to_network(&network->menu_entry, native.menu_entry);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b8> *native, NetPacket_Fixed<0x00b8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->npc_id, network.npc_id);
    rv &= network_to_native(&native->menu_entry, network.menu_entry);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00b9> *network, Packet_Fixed<0x00b9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->npc_id, native.npc_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00b9> *native, NetPacket_Fixed<0x00b9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->npc_id, network.npc_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00bb> *network, Packet_Fixed<0x00bb> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->asp, native.asp);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00bb> *native, NetPacket_Fixed<0x00bb> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->asp, network.asp);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00bc> *network, Packet_Fixed<0x00bc> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sp_type, native.sp_type);
    rv &= native_to_network(&network->ok, native.ok);
    rv &= native_to_network(&network->val, native.val);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00bc> *native, NetPacket_Fixed<0x00bc> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sp_type, network.sp_type);
    rv &= network_to_native(&native->ok, network.ok);
    rv &= network_to_native(&native->val, network.val);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00bd> *network, Packet_Fixed<0x00bd> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->status_point, native.status_point);
    rv &= native_to_network(&network->str_attr, native.str_attr);
    rv &= native_to_network(&network->str_upd, native.str_upd);
    rv &= native_to_network(&network->agi_attr, native.agi_attr);
    rv &= native_to_network(&network->agi_upd, native.agi_upd);
    rv &= native_to_network(&network->vit_attr, native.vit_attr);
    rv &= native_to_network(&network->vit_upd, native.vit_upd);
    rv &= native_to_network(&network->int_attr, native.int_attr);
    rv &= native_to_network(&network->int_upd, native.int_upd);
    rv &= native_to_network(&network->dex_attr, native.dex_attr);
    rv &= native_to_network(&network->dex_upd, native.dex_upd);
    rv &= native_to_network(&network->luk_attr, native.luk_attr);
    rv &= native_to_network(&network->luk_upd, native.luk_upd);
    rv &= native_to_network(&network->atk_sum, native.atk_sum);
    rv &= native_to_network(&network->watk2, native.watk2);
    rv &= native_to_network(&network->matk1, native.matk1);
    rv &= native_to_network(&network->matk2, native.matk2);
    rv &= native_to_network(&network->def, native.def);
    rv &= native_to_network(&network->def2, native.def2);
    rv &= native_to_network(&network->mdef, native.mdef);
    rv &= native_to_network(&network->mdef2, native.mdef2);
    rv &= native_to_network(&network->hit, native.hit);
    rv &= native_to_network(&network->flee, native.flee);
    rv &= native_to_network(&network->flee2, native.flee2);
    rv &= native_to_network(&network->critical, native.critical);
    rv &= native_to_network(&network->karma, native.karma);
    rv &= native_to_network(&network->manner, native.manner);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00bd> *native, NetPacket_Fixed<0x00bd> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->status_point, network.status_point);
    rv &= network_to_native(&native->str_attr, network.str_attr);
    rv &= network_to_native(&native->str_upd, network.str_upd);
    rv &= network_to_native(&native->agi_attr, network.agi_attr);
    rv &= network_to_native(&native->agi_upd, network.agi_upd);
    rv &= network_to_native(&native->vit_attr, network.vit_attr);
    rv &= network_to_native(&native->vit_upd, network.vit_upd);
    rv &= network_to_native(&native->int_attr, network.int_attr);
    rv &= network_to_native(&native->int_upd, network.int_upd);
    rv &= network_to_native(&native->dex_attr, network.dex_attr);
    rv &= network_to_native(&native->dex_upd, network.dex_upd);
    rv &= network_to_native(&native->luk_attr, network.luk_attr);
    rv &= network_to_native(&native->luk_upd, network.luk_upd);
    rv &= network_to_native(&native->atk_sum, network.atk_sum);
    rv &= network_to_native(&native->watk2, network.watk2);
    rv &= network_to_native(&native->matk1, network.matk1);
    rv &= network_to_native(&native->matk2, network.matk2);
    rv &= network_to_native(&native->def, network.def);
    rv &= network_to_native(&native->def2, network.def2);
    rv &= network_to_native(&native->mdef, network.mdef);
    rv &= network_to_native(&native->mdef2, network.mdef2);
    rv &= network_to_native(&native->hit, network.hit);
    rv &= network_to_native(&native->flee, network.flee);
    rv &= network_to_native(&native->flee2, network.flee2);
    rv &= network_to_native(&native->critical, network.critical);
    rv &= network_to_native(&native->karma, network.karma);
    rv &= network_to_native(&native->manner, network.manner);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00be> *network, Packet_Fixed<0x00be> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sp_type, native.sp_type);
    rv &= native_to_network(&network->value, native.value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00be> *native, NetPacket_Fixed<0x00be> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sp_type, network.sp_type);
    rv &= network_to_native(&native->value, network.value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00bf> *network, Packet_Fixed<0x00bf> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->emote, native.emote);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00bf> *native, NetPacket_Fixed<0x00bf> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->emote, network.emote);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00c0> *network, Packet_Fixed<0x00c0> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00c0> *native, NetPacket_Fixed<0x00c0> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00c1> *network, Packet_Fixed<0x00c1> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00c1> *native, NetPacket_Fixed<0x00c1> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00c2> *network, Packet_Fixed<0x00c2> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->users, native.users);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00c2> *native, NetPacket_Fixed<0x00c2> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->users, network.users);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00c4> *network, Packet_Fixed<0x00c4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00c4> *native, NetPacket_Fixed<0x00c4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00c5> *network, Packet_Fixed<0x00c5> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00c5> *native, NetPacket_Fixed<0x00c5> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00c6> *network, Packet_Head<0x00c6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00c6> *native, NetPacket_Head<0x00c6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00c6> *network, Packet_Repeat<0x00c6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->base_price, native.base_price);
    rv &= native_to_network(&network->actual_price, native.actual_price);
    rv &= native_to_network(&network->type, native.type);
    rv &= native_to_network(&network->name_id, native.name_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00c6> *native, NetPacket_Repeat<0x00c6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->base_price, network.base_price);
    rv &= network_to_native(&native->actual_price, network.actual_price);
    rv &= network_to_native(&native->type, network.type);
    rv &= network_to_native(&native->name_id, network.name_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00c7> *network, Packet_Head<0x00c7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00c7> *native, NetPacket_Head<0x00c7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00c7> *network, Packet_Repeat<0x00c7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->base_price, native.base_price);
    rv &= native_to_network(&network->actual_price, native.actual_price);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00c7> *native, NetPacket_Repeat<0x00c7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->base_price, network.base_price);
    rv &= network_to_native(&native->actual_price, network.actual_price);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00c8> *network, Packet_Head<0x00c8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00c8> *native, NetPacket_Head<0x00c8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00c8> *network, Packet_Repeat<0x00c8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->count, native.count);
    rv &= native_to_network(&network->name_id, native.name_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00c8> *native, NetPacket_Repeat<0x00c8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->count, network.count);
    rv &= network_to_native(&native->name_id, network.name_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00c9> *network, Packet_Head<0x00c9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00c9> *native, NetPacket_Head<0x00c9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00c9> *network, Packet_Repeat<0x00c9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->count, native.count);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00c9> *native, NetPacket_Repeat<0x00c9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->count, network.count);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ca> *network, Packet_Fixed<0x00ca> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->fail, native.fail);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ca> *native, NetPacket_Fixed<0x00ca> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->fail, network.fail);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00cb> *network, Packet_Fixed<0x00cb> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->fail, native.fail);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00cb> *native, NetPacket_Fixed<0x00cb> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->fail, network.fail);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00cd> *network, Packet_Fixed<0x00cd> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00cd> *native, NetPacket_Fixed<0x00cd> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00e4> *network, Packet_Fixed<0x00e4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00e4> *native, NetPacket_Fixed<0x00e4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00e5> *network, Packet_Fixed<0x00e5> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00e5> *native, NetPacket_Fixed<0x00e5> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00e6> *network, Packet_Fixed<0x00e6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00e6> *native, NetPacket_Fixed<0x00e6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00e7> *network, Packet_Fixed<0x00e7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00e7> *native, NetPacket_Fixed<0x00e7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00e8> *network, Packet_Fixed<0x00e8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->zeny_or_ioff2, native.zeny_or_ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00e8> *native, NetPacket_Fixed<0x00e8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->zeny_or_ioff2, network.zeny_or_ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00e9> *network, Packet_Fixed<0x00e9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->broken_or_attribute, native.broken_or_attribute);
    rv &= native_to_network(&network->refine, native.refine);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00e9> *native, NetPacket_Fixed<0x00e9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->broken_or_attribute, network.broken_or_attribute);
    rv &= network_to_native(&native->refine, network.refine);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00eb> *network, Packet_Fixed<0x00eb> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00eb> *native, NetPacket_Fixed<0x00eb> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ec> *network, Packet_Fixed<0x00ec> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->fail, native.fail);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ec> *native, NetPacket_Fixed<0x00ec> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->fail, network.fail);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ed> *network, Packet_Fixed<0x00ed> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ed> *native, NetPacket_Fixed<0x00ed> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ee> *network, Packet_Fixed<0x00ee> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ee> *native, NetPacket_Fixed<0x00ee> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ef> *network, Packet_Fixed<0x00ef> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ef> *native, NetPacket_Fixed<0x00ef> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f0> *network, Packet_Fixed<0x00f0> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->fail, native.fail);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f0> *native, NetPacket_Fixed<0x00f0> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->fail, network.fail);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f2> *network, Packet_Fixed<0x00f2> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->current_slots, native.current_slots);
    rv &= native_to_network(&network->max_slots, native.max_slots);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f2> *native, NetPacket_Fixed<0x00f2> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->current_slots, network.current_slots);
    rv &= network_to_native(&native->max_slots, network.max_slots);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f3> *network, Packet_Fixed<0x00f3> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f3> *native, NetPacket_Fixed<0x00f3> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f4> *network, Packet_Fixed<0x00f4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->soff1, native.soff1);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->broken_or_attribute, native.broken_or_attribute);
    rv &= native_to_network(&network->refine, native.refine);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f4> *native, NetPacket_Fixed<0x00f4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->soff1, network.soff1);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->broken_or_attribute, network.broken_or_attribute);
    rv &= network_to_native(&native->refine, network.refine);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f5> *network, Packet_Fixed<0x00f5> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->soff1, native.soff1);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f5> *native, NetPacket_Fixed<0x00f5> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->soff1, network.soff1);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f6> *network, Packet_Fixed<0x00f6> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->soff1, native.soff1);
    rv &= native_to_network(&network->amount, native.amount);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f6> *native, NetPacket_Fixed<0x00f6> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->soff1, network.soff1);
    rv &= network_to_native(&native->amount, network.amount);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f7> *network, Packet_Fixed<0x00f7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f7> *native, NetPacket_Fixed<0x00f7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f8> *network, Packet_Fixed<0x00f8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f8> *native, NetPacket_Fixed<0x00f8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00f9> *network, Packet_Fixed<0x00f9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->party_name, native.party_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00f9> *native, NetPacket_Fixed<0x00f9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->party_name, network.party_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00fa> *network, Packet_Fixed<0x00fa> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00fa> *native, NetPacket_Fixed<0x00fa> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x00fb> *network, Packet_Head<0x00fb> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->party_name, native.party_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x00fb> *native, NetPacket_Head<0x00fb> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->party_name, network.party_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x00fb> *network, Packet_Repeat<0x00fb> native)
{
    bool rv = true;
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->map_name, native.map_name);
    rv &= native_to_network(&network->leader, native.leader);
    rv &= native_to_network(&network->online, native.online);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x00fb> *native, NetPacket_Repeat<0x00fb> network)
{
    bool rv = true;
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->map_name, network.map_name);
    rv &= network_to_native(&native->leader, network.leader);
    rv &= network_to_native(&native->online, network.online);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00fc> *network, Packet_Fixed<0x00fc> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00fc> *native, NetPacket_Fixed<0x00fc> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00fd> *network, Packet_Fixed<0x00fd> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00fd> *native, NetPacket_Fixed<0x00fd> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00fe> *network, Packet_Fixed<0x00fe> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->party_name, native.party_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00fe> *native, NetPacket_Fixed<0x00fe> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->party_name, network.party_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x00ff> *network, Packet_Fixed<0x00ff> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x00ff> *native, NetPacket_Fixed<0x00ff> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0100> *network, Packet_Fixed<0x0100> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0100> *native, NetPacket_Fixed<0x0100> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0101> *network, Packet_Fixed<0x0101> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->exp, native.exp);
    rv &= native_to_network(&network->item, native.item);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0101> *native, NetPacket_Fixed<0x0101> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->exp, network.exp);
    rv &= network_to_native(&native->item, network.item);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0102> *network, Packet_Fixed<0x0102> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->exp, native.exp);
    rv &= native_to_network(&network->item, native.item);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0102> *native, NetPacket_Fixed<0x0102> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->exp, network.exp);
    rv &= network_to_native(&native->item, network.item);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0103> *network, Packet_Fixed<0x0103> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->unused_char_name, native.unused_char_name);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0103> *native, NetPacket_Fixed<0x0103> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->unused_char_name, network.unused_char_name);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0105> *network, Packet_Fixed<0x0105> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->char_name, native.char_name);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0105> *native, NetPacket_Fixed<0x0105> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->char_name, network.char_name);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0106> *network, Packet_Fixed<0x0106> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->hp, native.hp);
    rv &= native_to_network(&network->max_hp, native.max_hp);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0106> *native, NetPacket_Fixed<0x0106> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->hp, network.hp);
    rv &= network_to_native(&native->max_hp, network.max_hp);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0107> *network, Packet_Fixed<0x0107> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->account_id, native.account_id);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0107> *native, NetPacket_Fixed<0x0107> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->account_id, network.account_id);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x0108> *network, Packet_Head<0x0108> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x0108> *native, NetPacket_Head<0x0108> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x0108> *network, Packet_Repeat<0x0108> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x0108> *native, NetPacket_Repeat<0x0108> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x0109> *network, Packet_Head<0x0109> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->account_id, native.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x0109> *native, NetPacket_Head<0x0109> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->account_id, network.account_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x0109> *network, Packet_Repeat<0x0109> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x0109> *native, NetPacket_Repeat<0x0109> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x010c> *network, Packet_Fixed<0x010c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x010c> *native, NetPacket_Fixed<0x010c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x010e> *network, Packet_Fixed<0x010e> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->skill_id, native.skill_id);
    rv &= native_to_network(&network->level, native.level);
    rv &= native_to_network(&network->sp, native.sp);
    rv &= native_to_network(&network->range, native.range);
    rv &= native_to_network(&network->can_raise, native.can_raise);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x010e> *native, NetPacket_Fixed<0x010e> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->skill_id, network.skill_id);
    rv &= network_to_native(&native->level, network.level);
    rv &= network_to_native(&native->sp, network.sp);
    rv &= network_to_native(&native->range, network.range);
    rv &= network_to_native(&native->can_raise, network.can_raise);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x010f> *network, Packet_Head<0x010f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x010f> *native, NetPacket_Head<0x010f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x010f> *network, Packet_Repeat<0x010f> native)
{
    bool rv = true;
    rv &= native_to_network(&network->info, native.info);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x010f> *native, NetPacket_Repeat<0x010f> network)
{
    bool rv = true;
    rv &= network_to_native(&native->info, network.info);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0110> *network, Packet_Fixed<0x0110> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->skill_id, native.skill_id);
    rv &= native_to_network(&network->btype, native.btype);
    rv &= native_to_network(&network->zero1, native.zero1);
    rv &= native_to_network(&network->zero2, native.zero2);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0110> *native, NetPacket_Fixed<0x0110> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->skill_id, network.skill_id);
    rv &= network_to_native(&native->btype, network.btype);
    rv &= network_to_native(&native->zero1, network.zero1);
    rv &= network_to_native(&native->zero2, network.zero2);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0112> *network, Packet_Fixed<0x0112> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->skill_id, native.skill_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0112> *native, NetPacket_Fixed<0x0112> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->skill_id, network.skill_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0118> *network, Packet_Fixed<0x0118> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0118> *native, NetPacket_Fixed<0x0118> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0119> *network, Packet_Fixed<0x0119> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->zero, native.zero);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0119> *native, NetPacket_Fixed<0x0119> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->zero, network.zero);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0139> *network, Packet_Fixed<0x0139> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->bl_x, native.bl_x);
    rv &= native_to_network(&network->bl_y, native.bl_y);
    rv &= native_to_network(&network->sd_x, native.sd_x);
    rv &= native_to_network(&network->sd_y, native.sd_y);
    rv &= native_to_network(&network->range, native.range);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0139> *native, NetPacket_Fixed<0x0139> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->bl_x, network.bl_x);
    rv &= network_to_native(&native->bl_y, network.bl_y);
    rv &= network_to_native(&native->sd_x, network.sd_x);
    rv &= network_to_native(&native->sd_y, network.sd_y);
    rv &= network_to_native(&native->range, network.range);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x013a> *network, Packet_Fixed<0x013a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->attack_range, native.attack_range);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x013a> *native, NetPacket_Fixed<0x013a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->attack_range, network.attack_range);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x013b> *network, Packet_Fixed<0x013b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x013b> *native, NetPacket_Fixed<0x013b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x013c> *network, Packet_Fixed<0x013c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x013c> *native, NetPacket_Fixed<0x013c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0141> *network, Packet_Fixed<0x0141> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sp_type, native.sp_type);
    rv &= native_to_network(&network->zero, native.zero);
    rv &= native_to_network(&network->value_status, native.value_status);
    rv &= native_to_network(&network->value_b_e, native.value_b_e);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0141> *native, NetPacket_Fixed<0x0141> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sp_type, network.sp_type);
    rv &= network_to_native(&native->zero, network.zero);
    rv &= network_to_native(&native->value_status, network.value_status);
    rv &= network_to_native(&native->value_b_e, network.value_b_e);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0142> *network, Packet_Fixed<0x0142> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0142> *native, NetPacket_Fixed<0x0142> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0143> *network, Packet_Fixed<0x0143> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->input_int_value, native.input_int_value);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0143> *native, NetPacket_Fixed<0x0143> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->input_int_value, network.input_int_value);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0146> *network, Packet_Fixed<0x0146> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0146> *native, NetPacket_Fixed<0x0146> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0147> *network, Packet_Fixed<0x0147> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->info, native.info);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0147> *native, NetPacket_Fixed<0x0147> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->info, network.info);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0148> *network, Packet_Fixed<0x0148> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0148> *native, NetPacket_Fixed<0x0148> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x014d> *network, Packet_Fixed<0x014d> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x014d> *native, NetPacket_Fixed<0x014d> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x018a> *network, Packet_Fixed<0x018a> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x018a> *native, NetPacket_Fixed<0x018a> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x018b> *network, Packet_Fixed<0x018b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->okay, native.okay);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x018b> *native, NetPacket_Fixed<0x018b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->okay, network.okay);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0195> *network, Packet_Fixed<0x0195> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->party_name, native.party_name);
    rv &= native_to_network(&network->guild_name, native.guild_name);
    rv &= native_to_network(&network->guild_pos, native.guild_pos);
    rv &= native_to_network(&network->guild_pos_again, native.guild_pos_again);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0195> *native, NetPacket_Fixed<0x0195> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->party_name, network.party_name);
    rv &= network_to_native(&native->guild_name, network.guild_name);
    rv &= network_to_native(&native->guild_pos, network.guild_pos);
    rv &= network_to_native(&native->guild_pos_again, network.guild_pos_again);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0196> *network, Packet_Fixed<0x0196> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->sc_type, native.sc_type);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->flag, native.flag);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0196> *native, NetPacket_Fixed<0x0196> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->sc_type, network.sc_type);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->flag, network.flag);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x019b> *network, Packet_Fixed<0x019b> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->type, native.type);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x019b> *native, NetPacket_Fixed<0x019b> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->type, network.type);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01b1> *network, Packet_Fixed<0x01b1> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->fail, native.fail);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01b1> *native, NetPacket_Fixed<0x01b1> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->fail, network.fail);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01c8> *network, Packet_Fixed<0x01c8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->ok, native.ok);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01c8> *native, NetPacket_Fixed<0x01c8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->ok, network.ok);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01d4> *network, Packet_Fixed<0x01d4> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01d4> *native, NetPacket_Fixed<0x01d4> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x01d5> *network, Packet_Head<0x01d5> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    rv &= native_to_network(&network->block_id, native.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x01d5> *native, NetPacket_Head<0x01d5> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    rv &= network_to_native(&native->block_id, network.block_id);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x01d5> *network, Packet_Repeat<0x01d5> native)
{
    bool rv = true;
    rv &= native_to_network(&network->c, native.c);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x01d5> *native, NetPacket_Repeat<0x01d5> network)
{
    bool rv = true;
    rv &= network_to_native(&native->c, network.c);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01d7> *network, Packet_Fixed<0x01d7> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->look_type, native.look_type);
    rv &= native_to_network(&network->weapon_or_name_id_or_value, native.weapon_or_name_id_or_value);
    rv &= native_to_network(&network->shield, native.shield);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01d7> *native, NetPacket_Fixed<0x01d7> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->look_type, network.look_type);
    rv &= network_to_native(&native->weapon_or_name_id_or_value, network.weapon_or_name_id_or_value);
    rv &= network_to_native(&native->shield, network.shield);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01d8> *network, Packet_Fixed<0x01d8> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->hair_style, native.hair_style);
    rv &= native_to_network(&network->weapon, native.weapon);
    rv &= native_to_network(&network->shield, native.shield);
    rv &= native_to_network(&network->head_bottom, native.head_bottom);
    rv &= native_to_network(&network->head_top, native.head_top);
    rv &= native_to_network(&network->head_mid, native.head_mid);
    rv &= native_to_network(&network->hair_color, native.hair_color);
    rv &= native_to_network(&network->clothes_color, native.clothes_color);
    rv &= native_to_network(&network->head_dir, native.head_dir);
    rv &= native_to_network(&network->unused2, native.unused2);
    rv &= native_to_network(&network->guild_id, native.guild_id);
    rv &= native_to_network(&network->guild_emblem_id, native.guild_emblem_id);
    rv &= native_to_network(&network->manner, native.manner);
    rv &= native_to_network(&network->opt3, native.opt3);
    rv &= native_to_network(&network->karma, native.karma);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->pos, native.pos);
    rv &= native_to_network(&network->gm_bits, native.gm_bits);
    rv &= native_to_network(&network->dead_sit, native.dead_sit);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01d8> *native, NetPacket_Fixed<0x01d8> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->hair_style, network.hair_style);
    rv &= network_to_native(&native->weapon, network.weapon);
    rv &= network_to_native(&native->shield, network.shield);
    rv &= network_to_native(&native->head_bottom, network.head_bottom);
    rv &= network_to_native(&native->head_top, network.head_top);
    rv &= network_to_native(&native->head_mid, network.head_mid);
    rv &= network_to_native(&native->hair_color, network.hair_color);
    rv &= network_to_native(&native->clothes_color, network.clothes_color);
    rv &= network_to_native(&native->head_dir, network.head_dir);
    rv &= network_to_native(&native->unused2, network.unused2);
    rv &= network_to_native(&native->guild_id, network.guild_id);
    rv &= network_to_native(&native->guild_emblem_id, network.guild_emblem_id);
    rv &= network_to_native(&native->manner, network.manner);
    rv &= network_to_native(&native->opt3, network.opt3);
    rv &= network_to_native(&native->karma, network.karma);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->pos, network.pos);
    rv &= network_to_native(&native->gm_bits, network.gm_bits);
    rv &= network_to_native(&native->dead_sit, network.dead_sit);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01d9> *network, Packet_Fixed<0x01d9> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->hair_style, native.hair_style);
    rv &= native_to_network(&network->weapon, native.weapon);
    rv &= native_to_network(&network->shield, native.shield);
    rv &= native_to_network(&network->head_bottom, native.head_bottom);
    rv &= native_to_network(&network->head_top, native.head_top);
    rv &= native_to_network(&network->head_mid, native.head_mid);
    rv &= native_to_network(&network->hair_color, native.hair_color);
    rv &= native_to_network(&network->clothes_color, native.clothes_color);
    rv &= native_to_network(&network->head_dir, native.head_dir);
    rv &= native_to_network(&network->unused2, native.unused2);
    rv &= native_to_network(&network->guild_id, native.guild_id);
    rv &= native_to_network(&network->guild_emblem_id, native.guild_emblem_id);
    rv &= native_to_network(&network->manner, native.manner);
    rv &= native_to_network(&network->opt3, native.opt3);
    rv &= native_to_network(&network->karma, native.karma);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->pos, native.pos);
    rv &= native_to_network(&network->gm_bits, native.gm_bits);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01d9> *native, NetPacket_Fixed<0x01d9> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->hair_style, network.hair_style);
    rv &= network_to_native(&native->weapon, network.weapon);
    rv &= network_to_native(&native->shield, network.shield);
    rv &= network_to_native(&native->head_bottom, network.head_bottom);
    rv &= network_to_native(&native->head_top, network.head_top);
    rv &= network_to_native(&native->head_mid, network.head_mid);
    rv &= network_to_native(&native->hair_color, network.hair_color);
    rv &= network_to_native(&native->clothes_color, network.clothes_color);
    rv &= network_to_native(&native->head_dir, network.head_dir);
    rv &= network_to_native(&native->unused2, network.unused2);
    rv &= network_to_native(&native->guild_id, network.guild_id);
    rv &= network_to_native(&native->guild_emblem_id, network.guild_emblem_id);
    rv &= network_to_native(&native->manner, network.manner);
    rv &= network_to_native(&native->opt3, network.opt3);
    rv &= network_to_native(&native->karma, network.karma);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->pos, network.pos);
    rv &= network_to_native(&native->gm_bits, network.gm_bits);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01da> *network, Packet_Fixed<0x01da> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->speed, native.speed);
    rv &= native_to_network(&network->opt1, native.opt1);
    rv &= native_to_network(&network->opt2, native.opt2);
    rv &= native_to_network(&network->option, native.option);
    rv &= native_to_network(&network->species, native.species);
    rv &= native_to_network(&network->hair_style, native.hair_style);
    rv &= native_to_network(&network->weapon, native.weapon);
    rv &= native_to_network(&network->shield, native.shield);
    rv &= native_to_network(&network->head_bottom, native.head_bottom);
    rv &= native_to_network(&network->tick, native.tick);
    rv &= native_to_network(&network->head_top, native.head_top);
    rv &= native_to_network(&network->head_mid, native.head_mid);
    rv &= native_to_network(&network->hair_color, native.hair_color);
    rv &= native_to_network(&network->clothes_color, native.clothes_color);
    rv &= native_to_network(&network->head_dir, native.head_dir);
    rv &= native_to_network(&network->unused2, native.unused2);
    rv &= native_to_network(&network->guild_id, native.guild_id);
    rv &= native_to_network(&network->guild_emblem_id, native.guild_emblem_id);
    rv &= native_to_network(&network->manner, native.manner);
    rv &= native_to_network(&network->opt3, native.opt3);
    rv &= native_to_network(&network->karma, native.karma);
    rv &= native_to_network(&network->sex, native.sex);
    rv &= native_to_network(&network->pos2, native.pos2);
    rv &= native_to_network(&network->gm_bits, native.gm_bits);
    rv &= native_to_network(&network->five, native.five);
    rv &= native_to_network(&network->unused, native.unused);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01da> *native, NetPacket_Fixed<0x01da> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->speed, network.speed);
    rv &= network_to_native(&native->opt1, network.opt1);
    rv &= network_to_native(&native->opt2, network.opt2);
    rv &= network_to_native(&native->option, network.option);
    rv &= network_to_native(&native->species, network.species);
    rv &= network_to_native(&native->hair_style, network.hair_style);
    rv &= network_to_native(&native->weapon, network.weapon);
    rv &= network_to_native(&native->shield, network.shield);
    rv &= network_to_native(&native->head_bottom, network.head_bottom);
    rv &= network_to_native(&native->tick, network.tick);
    rv &= network_to_native(&native->head_top, network.head_top);
    rv &= network_to_native(&native->head_mid, network.head_mid);
    rv &= network_to_native(&native->hair_color, network.hair_color);
    rv &= network_to_native(&native->clothes_color, network.clothes_color);
    rv &= network_to_native(&native->head_dir, network.head_dir);
    rv &= network_to_native(&native->unused2, network.unused2);
    rv &= network_to_native(&native->guild_id, network.guild_id);
    rv &= network_to_native(&native->guild_emblem_id, network.guild_emblem_id);
    rv &= network_to_native(&native->manner, network.manner);
    rv &= network_to_native(&native->opt3, network.opt3);
    rv &= network_to_native(&native->karma, network.karma);
    rv &= network_to_native(&native->sex, network.sex);
    rv &= network_to_native(&native->pos2, network.pos2);
    rv &= network_to_native(&native->gm_bits, network.gm_bits);
    rv &= network_to_native(&native->five, network.five);
    rv &= network_to_native(&native->unused, network.unused);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x01de> *network, Packet_Fixed<0x01de> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->skill_id, native.skill_id);
    rv &= native_to_network(&network->src_id, native.src_id);
    rv &= native_to_network(&network->dst_id, native.dst_id);
    rv &= native_to_network(&network->tick, native.tick);
    rv &= native_to_network(&network->sdelay, native.sdelay);
    rv &= native_to_network(&network->ddelay, native.ddelay);
    rv &= native_to_network(&network->damage, native.damage);
    rv &= native_to_network(&network->skill_level, native.skill_level);
    rv &= native_to_network(&network->div, native.div);
    rv &= native_to_network(&network->type_or_hit, native.type_or_hit);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x01de> *native, NetPacket_Fixed<0x01de> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->skill_id, network.skill_id);
    rv &= network_to_native(&native->src_id, network.src_id);
    rv &= network_to_native(&native->dst_id, network.dst_id);
    rv &= network_to_native(&native->tick, network.tick);
    rv &= network_to_native(&native->sdelay, network.sdelay);
    rv &= network_to_native(&native->ddelay, network.ddelay);
    rv &= network_to_native(&native->damage, network.damage);
    rv &= network_to_native(&native->skill_level, network.skill_level);
    rv &= network_to_native(&native->div, network.div);
    rv &= network_to_native(&native->type_or_hit, network.type_or_hit);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x01ee> *network, Packet_Head<0x01ee> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x01ee> *native, NetPacket_Head<0x01ee> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x01ee> *network, Packet_Repeat<0x01ee> native)
{
    bool rv = true;
    rv &= native_to_network(&network->ioff2, native.ioff2);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->item_type, native.item_type);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->epos, native.epos);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x01ee> *native, NetPacket_Repeat<0x01ee> network)
{
    bool rv = true;
    rv &= network_to_native(&native->ioff2, network.ioff2);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->item_type, network.item_type);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->epos, network.epos);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Head<0x01f0> *network, Packet_Head<0x01f0> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->magic_packet_length, native.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Head<0x01f0> *native, NetPacket_Head<0x01f0> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->magic_packet_length, network.magic_packet_length);
    return rv;
}
inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Repeat<0x01f0> *network, Packet_Repeat<0x01f0> native)
{
    bool rv = true;
    rv &= native_to_network(&network->soff1, native.soff1);
    rv &= native_to_network(&network->name_id, native.name_id);
    rv &= native_to_network(&network->item_type, native.item_type);
    rv &= native_to_network(&network->identify, native.identify);
    rv &= native_to_network(&network->amount, native.amount);
    rv &= native_to_network(&network->epos_zero, native.epos_zero);
    rv &= native_to_network(&network->card0, native.card0);
    rv &= native_to_network(&network->card1, native.card1);
    rv &= native_to_network(&network->card2, native.card2);
    rv &= native_to_network(&network->card3, native.card3);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Repeat<0x01f0> *native, NetPacket_Repeat<0x01f0> network)
{
    bool rv = true;
    rv &= network_to_native(&native->soff1, network.soff1);
    rv &= network_to_native(&native->name_id, network.name_id);
    rv &= network_to_native(&native->item_type, network.item_type);
    rv &= network_to_native(&native->identify, network.identify);
    rv &= network_to_native(&native->amount, network.amount);
    rv &= network_to_native(&native->epos_zero, network.epos_zero);
    rv &= network_to_native(&native->card0, network.card0);
    rv &= network_to_native(&native->card1, network.card1);
    rv &= network_to_native(&native->card2, network.card2);
    rv &= network_to_native(&native->card3, network.card3);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x020c> *network, Packet_Fixed<0x020c> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->block_id, native.block_id);
    rv &= native_to_network(&network->ip, native.ip);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x020c> *native, NetPacket_Fixed<0x020c> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->block_id, network.block_id);
    rv &= network_to_native(&native->ip, network.ip);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(NetPacket_Fixed<0x0212> *network, Packet_Fixed<0x0212> native)
{
    bool rv = true;
    rv &= native_to_network(&network->magic_packet_id, native.magic_packet_id);
    rv &= native_to_network(&network->npc_id, native.npc_id);
    rv &= native_to_network(&network->command, native.command);
    rv &= native_to_network(&network->id, native.id);
    rv &= native_to_network(&network->x, native.x);
    rv &= native_to_network(&network->y, native.y);
    return rv;
}
inline __attribute__((warn_unused_result))
bool network_to_native(Packet_Fixed<0x0212> *native, NetPacket_Fixed<0x0212> network)
{
    bool rv = true;
    rv &= network_to_native(&native->magic_packet_id, network.magic_packet_id);
    rv &= network_to_native(&native->npc_id, network.npc_id);
    rv &= network_to_native(&native->command, network.command);
    rv &= network_to_native(&native->id, network.id);
    rv &= network_to_native(&native->x, network.x);
    rv &= network_to_native(&native->y, network.y);
    return rv;
}


# pragma pack(pop)

#endif // TMWA_PROTO2_MAP_USER_HPP
