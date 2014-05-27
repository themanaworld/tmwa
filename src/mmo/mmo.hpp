#ifndef TMWA_MMO_MMO_HPP
#define TMWA_MMO_MMO_HPP
//    mmo.hpp - Huge mess of structures and constants.
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

# include "fwd.hpp"

# include <algorithm>

# include "../compat/memory.hpp"

# include "../generic/array.hpp"

# include "../net/timer.t.hpp"

# include "enums.hpp"
# include "ids.hpp"
# include "strs.hpp"

constexpr int FIFOSIZE_SERVERLINK = 256 * 1024;

constexpr int MAX_MAP_PER_SERVER = 512;
constexpr int MAX_INVENTORY = 100;
constexpr int MAX_AMOUNT = 30000;
constexpr int MAX_ZENY = 1000000000;     // 1G zeny
constexpr int TRADE_MAX = 10;

constexpr int GLOBAL_REG_NUM = 96;
constexpr size_t ACCOUNT_REG_NUM = 16;
constexpr size_t ACCOUNT_REG2_NUM = 16;
constexpr interval_t DEFAULT_WALK_SPEED = std::chrono::milliseconds(150);
constexpr interval_t MIN_WALK_SPEED = interval_t::zero();
constexpr interval_t MAX_WALK_SPEED = std::chrono::seconds(1);
constexpr int MAX_STORAGE = 300;
constexpr int MAX_PARTY = 12;

# define MIN_HAIR_STYLE battle_config.min_hair_style
# define MAX_HAIR_STYLE battle_config.max_hair_style
# define MIN_HAIR_COLOR battle_config.min_hair_color
# define MAX_HAIR_COLOR battle_config.max_hair_color
# define MIN_CLOTH_COLOR battle_config.min_cloth_color
# define MAX_CLOTH_COLOR battle_config.max_cloth_color

struct item
{
    ItemNameId nameid;
    short amount;
    EPOS equip;
};

struct point
{
    MapName map_;
    short x, y;
};

struct skill_value
{
    unsigned short lv;
    SkillFlags flags;

    friend bool operator == (const skill_value& l, const skill_value& r)
    {
        return l.lv == r.lv && l.flags == r.flags;
    }
    friend bool operator != (const skill_value& l, const skill_value& r)
    {
        return !(l == r);
    }
};

struct global_reg
{
    VarName str;
    int value;
};

struct CharKey
{
    CharName name;
    AccountId account_id;
    CharId char_id;
    unsigned char char_num;
};

struct CharData
{
    CharId partner_id;

    int base_exp, job_exp, zeny;

    Species species;
    short status_point, skill_point;
    int hp, max_hp, sp, max_sp;
    Option option;
    short karma, manner;
    short hair, hair_color, clothes_color;
    PartyId party_id;

    ItemLook weapon;
    ItemNameId shield;
    ItemNameId head_top, head_mid, head_bottom;

    unsigned char base_level, job_level;
    earray<short, ATTR, ATTR::COUNT> attrs;
    SEX sex;

    unsigned long mapip;
    unsigned int mapport;

    struct point last_point, save_point;
    Array<struct item, MAX_INVENTORY> inventory;
    earray<skill_value, SkillID, MAX_SKILL> skill;
    int global_reg_num;
    Array<struct global_reg, GLOBAL_REG_NUM> global_reg;
    int account_reg_num;
    Array<struct global_reg, ACCOUNT_REG_NUM> account_reg;
    int account_reg2_num;
    Array<struct global_reg, ACCOUNT_REG2_NUM> account_reg2;
};

struct CharPair
{
    CharKey key;
    std::unique_ptr<CharData> data;

    CharPair()
    : key{}, data(make_unique<CharData>())
    {}
};

struct Storage
{
    bool dirty;
    AccountId account_id;
    short storage_status;
    short storage_amount;
    Array<struct item, MAX_STORAGE> storage_;
};

struct GM_Account
{
    AccountId account_id;
    GmLevel level;
};

struct party_member
{
    AccountId account_id;
    CharName name;
    MapName map;
    int leader, online, lv;
    struct map_session_data *sd;
};

struct PartyMost
{
    PartyName name;
    int exp;
    int item;
    Array<struct party_member, MAX_PARTY> member;
};

struct PartyPair
{
    PartyId party_id = {};
    PartyMost *party_most = {};

    explicit
    operator bool() const { return party_most; }
    bool operator !() const { return !party_most; }
    PartyMost *operator->() const { return party_most; }
};

#endif // TMWA_MMO_MMO_HPP
