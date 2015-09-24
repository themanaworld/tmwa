#pragma once
//    consts.hpp - Huge mess of constants.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "fwd.hpp"

#include "../net/timer.t.hpp"

#include "ids.hpp"
#include "strs.hpp"


namespace tmwa
{
constexpr int FIFOSIZE_SERVERLINK = 256 * 1024;

constexpr int MAX_MAP_PER_SERVER = 512;
constexpr int MAX_INVENTORY = 100;
constexpr int MAX_AMOUNT = 30000;
constexpr int MAX_ZENY = 1000000000;     // 1G zeny
constexpr int TRADE_MAX = 10;

constexpr int GLOBAL_REG_NUM = 96;
constexpr size_t ACCOUNT_REG_NUM = 16;
constexpr size_t ACCOUNT_REG2_NUM = 16;
constexpr interval_t DEFAULT_WALK_SPEED = 150_ms;
constexpr interval_t MIN_WALK_SPEED = interval_t::zero();
constexpr interval_t MAX_WALK_SPEED = 1_s;
constexpr int MAX_STORAGE = 300;
constexpr int MAX_PARTY = 12;
constexpr int MAX_GUILD = 60;

#define MIN_HAIR_STYLE battle_config.min_hair_style
#define MAX_HAIR_STYLE battle_config.max_hair_style
#define MIN_HAIR_COLOR battle_config.min_hair_color
#define MAX_HAIR_COLOR battle_config.max_hair_color
#define MIN_CLOTH_COLOR battle_config.min_cloth_color
#define MAX_CLOTH_COLOR battle_config.max_cloth_color

namespace map
{
    struct map_session_data;
}

// WTF is this doing here?

struct PartyMember
{
    AccountId account_id;
    CharName name;
    MapName map;
    int leader, online, lv;
    map::map_session_data *sd;
};

struct GuildMember
{
    AccountId account_id;
    CharName name;
    int position, online, lv;
    map::map_session_data *sd;
};

} // namespace tmwa
