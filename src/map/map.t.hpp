#pragma once
//    map.t.hpp - Core of the map server.
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

#include <cstdint>

#include "../strings/vstring.hpp"

#include "../generic/enum.hpp"

#include "../mmo/ids.hpp"
#include "../high/mmo.hpp"


namespace tmwa
{
namespace map
{
enum class BL : uint8_t
{
    NUL,
    PC,
    NPC,
    MOB,
    ITEM,
};
enum class NpcSubtype : uint8_t
{
    WARP,
    SHOP,
    SCRIPT,

    COUNT,
};

enum class mob_stat
{
    LV,
    MAX_HP,
    STR,
    AGI,
    VIT,
    INT,
    DEX,
    LUK,
    // low and high attacks
    ATK1,
    ATK2,
    // attack delay
    ADELAY,
    DEF,
    MDEF,
    SPEED,
    // These must come last:
    // [Fate] Encoded as base to 1024: 1024 means 100%
    XP_BONUS,
    LAST,
};

enum class MS : uint8_t
{
    IDLE,
    WALK,
    ATTACK,
    DEAD,
};

enum class ATK
{
    ZERO = 0,

    LUCKY,
    FLEE,
    DEF,
};


enum class EQUIP
{
    NONE    = -1,
    MISC2   = 0,
    CAPE    = 1,
    SHOES   = 2,
    GLOVES  = 3,
    LEGS    = 4, // also called "head bottom"
    TORSO   = 5, // also called "head middle"
    HAT     = 6, // also called "head top"
    MISC1   = 7,
    SHIELD  = 8,
    WEAPON  = 9,
    ARROW   = 10,
    COUNT   = 11,
};

constexpr
EQUIP EQUIPs[] =
{
    EQUIP::MISC2,
    EQUIP::CAPE,
    EQUIP::SHOES,
    EQUIP::GLOVES,
    EQUIP::LEGS,
    EQUIP::TORSO,
    EQUIP::HAT,
    EQUIP::MISC1,
    EQUIP::SHIELD,
    EQUIP::WEAPON,
    EQUIP::ARROW,
};

constexpr
EQUIP EQUIPs_noarrow[] =
{
    EQUIP::MISC2,
    EQUIP::CAPE,
    EQUIP::SHOES,
    EQUIP::GLOVES,
    EQUIP::LEGS,
    EQUIP::TORSO,
    EQUIP::HAT,
    EQUIP::MISC1,
    EQUIP::SHIELD,
    EQUIP::WEAPON,
};

namespace e
{
enum class MobMode : uint16_t
{
    ZERO                        = 0x0000,

    CAN_MOVE                    = 0x0001,
    LOOTER                      = 0x0002,
    AGGRESSIVE                  = 0x0004,
    ASSIST                      = 0x0008,

    CAST_SENSOR                 = 0x0010,
    BOSS                        = 0x0020,
    // sometimes also called "robust"
    PLANT                       = 0x0040,
    CAN_ATTACK                  = 0x0080,

    DETECTOR                    = 0x0100,
    CHANGE_TARGET               = 0x0200,

    war                         = CAN_MOVE | AGGRESSIVE | CAN_ATTACK,

    SUMMONED                    = 0x1000,
    TURNS_AGAINST_BAD_MASTER    = 0x2000,

    // mob mode flags that Fate actually understood
    SENSIBLE_MASK               = 0xf000,
};

ENUM_BITWISE_OPERATORS(MobMode)
}
using e::MobMode;

namespace e
{
enum class MapCell : uint8_t
{
    // the usual thing
    UNWALKABLE  = 0x01,
    // not in tmwa data
    _range      = 0x04,
};
ENUM_BITWISE_OPERATORS(MapCell)
}
using e::MapCell;

inline
BlockId account_to_block(AccountId a) { return wrap<BlockId>(unwrap<AccountId>(a)); }
inline
AccountId block_to_account(BlockId b) { return wrap<AccountId>(unwrap<BlockId>(b)); }
} // namespace map
} // namespace tmwa
