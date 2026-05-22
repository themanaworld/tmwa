#pragma once
//    script-fun.t.hpp - EAthena script frontend, engine, and library.
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

namespace tmwa
{
namespace map
{
enum class MobInfo : uint8_t
{
    ID             =  0,
    ENG_NAME       =  1,
    JAP_NAME       =  2,
    LVL            =  3,
    HP             =  4,
    SP             =  5,
    BASE_EXP       =  6,
    JOB_EXP        =  7,
    RANGE1         =  8,
    ATK1           =  9,
    ATK2           = 10,
    DEF            = 11,
    MDEF           = 12,
    CRITICAL_DEF   = 13,
    STR            = 14,
    AGI            = 15,
    VIT            = 16,
    INT            = 17,
    DEX            = 18,
    LUK            = 19,
    RANGE2         = 20,
    RANGE3         = 21,
    SCALE          = 22,
    RACE           = 23,
    ELEMENT        = 24,
    ELEMENT_LVL    = 25,
    MODE           = 26,
    SPEED          = 27,
    ADELAY         = 28,
    AMOTION        = 29,
    DMOTION        = 30,
    MUTATION_NUM   = 31,
    MUTATION_POWER = 32,
    DROPID0        = 33,
    DROPNAME0      = 34,
    DROPPERCENT0   = 35,
    DROPID1        = 36,
    DROPNAME1      = 37,
    DROPPERCENT1   = 38,
    DROPID2        = 39,
    DROPNAME2      = 40,
    DROPPERCENT2   = 41,
    DROPID3        = 42,
    DROPNAME3      = 43,
    DROPPERCENT3   = 44,
    DROPID4        = 45,
    DROPNAME4      = 46,
    DROPPERCENT4   = 47,
    DROPID5        = 48,
    DROPNAME5      = 49,
    DROPPERCENT5   = 50,
    DROPID6        = 51,
    DROPNAME6      = 52,
    DROPPERCENT6   = 53,
    DROPID7        = 54,
    DROPNAME7      = 55,
    DROPPERCENT7   = 56,
    DROPID8        = 57,
    DROPNAME8      = 58,
    DROPPERCENT8   = 59,
    DROPID9        = 60,
    DROPNAME9      = 61,
    DROPPERCENT9   = 62,
};

enum class MobInfo_DropArrays : uint8_t
{
    IDS      =  0,
    NAMES    =  1,
    PERCENTS =  2,
};

// Identifies one piece of data on a unit, for the getunitdata and
// setunitdata script builtins.
//
// Constant names and numeric values match Hercules' UDT_* enum so that
// a script using a shared key ports between TMWA and Hercules unchanged.
// Only the keys TMWA can back with a real field are declared; the gaps
// in the numbering are Hercules keys TMWA does not support (homunculus,
// pet, mercenary and elemental fields, stats TMWA derives rather than
// stores, and UDT_MAPIDXY, which Hercules itself deprecated in favour
// of unitwarp). The three values from 100 up are TMWA extensions with
// no Hercules equivalent.
//
// Applicability is per unit type; reading an inapplicable key yields 0
// (-1 for an invalid gid or key), writing one fails (0).
enum class UnitData : uint16_t
{
    TYPE            =   0, // BL type, read-only (1=PC, 2=NPC, 3=MOB)
    LEVEL           =   2, // pc, mob
    HP              =   3, // pc, mob
    MAX_HP          =   4, // pc, mob
    SP              =   5, // pc
    MAX_SP          =   6, // pc
    SPEED           =  11, // pc, npc, mob
    MODE            =  12, // mob
    SEX             =  15, // pc, npc
    CLASS           =  16, // sprite class; pc, npc, mob
    HAIR_STYLE      =  17, // pc
    HAIR_COLOR      =  18, // pc
    HEAD_BOTTOM     =  19, // pc
    HEAD_MIDDLE     =  20, // pc
    HEAD_TOP        =  21, // pc
    CLOTHES_COLOR   =  22, // pc
    SHIELD          =  23, // pc
    WEAPON          =  24, // pc
    LOOK_DIR        =  25, // facing direction; pc, npc, mob
    STR             =  27, // pc, mob
    AGI             =  28, // pc, mob
    VIT             =  29, // pc, mob
    INT             =  30, // pc, mob
    DEX             =  31, // pc, mob
    LUK             =  32, // pc, mob
    ATK_MIN         =  34, // pc, mob
    ATK_MAX         =  35, // pc, mob
    DEF             =  38, // pc, mob
    MDEF            =  39, // pc, mob
    ADELAY          =  48, // mob attack delay (ms)
    STATUS_POINT    =  54, // pc
    // TMWA extensions (no Hercules UDT_* equivalent)
    XP_BONUS        = 100, // mob xp bonus (1024 = 100%)
    CRITICAL_DEF    = 101, // mob critical-hit defense
    TARGET_ID       = 102, // mob current attack target gid, read-only
};
} // namespace map
} // namespace tmwa
