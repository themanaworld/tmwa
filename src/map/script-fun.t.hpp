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

// Identifies one piece of data on a unit, for use with the
// getunitdata and setunitdata script builtins.
//
// Not every key applies to every unit type. Reading an inapplicable
// key returns 0 (or an empty string for UDT_NAME / UDT_MAP); writing
// one is silently ignored.
//
// Each entry notes how the same attribute can also be reached through
// other script facilities, or is marked NEW / "NEW for mob" when
// getunitdata / setunitdata is the only access.
//
// "PARAM" means the SP-based parameter system: pc_readparam /
// pc_setparam, reachable from scripts as PARAM variables and through
// the get() and set(<param>, <value>, <gid>) builtins. get() / set()
// were extended so PARAM reaches mobs too, for the stats that map to
// a real mob_data field. Derived values (e.g. a pc's computed atk)
// can still be read through PARAM but not written.
enum class UnitData : uint8_t
{
    // read-only; also PARAM SP::BL_TYPE
    TYPE            =  0, // BL type (1=PC, 2=NPC, 3=MOB)
    // pc and mob: PARAM SP::BASELEVEL
    LEVEL           =  1, // mob, pc
    // pc and mob: PARAM SP::HP; pc also builtin heal
    HP              =  2, // mob, pc
    // pc and mob: PARAM SP::MAXHP
    MAX_HP          =  3, // mob, pc
    // pc: PARAM SP::SP, builtin heal
    SP              =  4, // pc
    // pc: PARAM SP::MAXSP
    MAX_SP          =  5, // pc
    // pc and mob: PARAM SP::STR
    STR             =  6, // mob, pc
    // pc and mob: PARAM SP::AGI
    AGI             =  7, // mob, pc
    // pc and mob: PARAM SP::VIT
    VIT             =  8, // mob, pc
    // pc and mob: PARAM SP::INT
    INT             =  9, // mob, pc
    // pc and mob: PARAM SP::DEX
    DEX             = 10, // mob, pc
    // pc and mob: PARAM SP::LUK
    LUK             = 11, // mob, pc
    // PARAM SP::ATK1: read for pc and mob, write for mob (pc atk is derived)
    ATK_MIN         = 12, // mob ATK1; pc watk
    // PARAM SP::ATK2: read for pc and mob, write for mob (pc atk is derived)
    ATK_MAX         = 13, // mob ATK2; pc watk2
    // NEW for mob (no live-unit script access; MobInfo only reads the db)
    ADELAY          = 14, // mob attack delay (ms)
    // PARAM SP::DEF1: read for pc and mob, write for mob (pc def is derived)
    DEF             = 15, // mob, pc
    // PARAM SP::MDEF1: read for pc and mob, write for mob (pc mdef is derived)
    MDEF            = 16, // mob, pc
    // pc, mob and npc: PARAM SP::SPEED (read and write)
    SPEED           = 17, // mob/pc/npc movement (ms per cell)
    // NEW (SP::CRITICAL is the crit rate, not critical_def)
    CRITICAL_DEF    = 18, // mob, pc
    // NEW for mob (MobInfo only reads the db, not a live mob)
    XP_BONUS        = 19, // mob (1024 = 100%)
    // NEW for mob (MobInfo only reads the db, not a live mob)
    MODE            = 20, // mob mode flags
    // NEW for mob (MobInfo SCALE only reads the db, not a live mob)
    SIZE            = 21, // mob
    // pc and npc: PARAM SP::SEX
    SEX             = 22, // pc, npc
    // pc and npc: PARAM SP::CLASS; npc sprite also builtin fakenpcname. mob: NEW for mob
    CLASS           = 23, // sprite class (mob_class / npc_class / pc species)
    // pc: builtins getlook / setlook (LOOK::HAIR)
    HAIR_STYLE      = 24, // pc
    // pc: builtins getlook / setlook (LOOK::HAIR_COLOR)
    HAIR_COLOR      = 25, // pc
    // pc: builtins getlook / setlook (LOOK::CLOTHES_COLOR)
    CLOTHES_COLOR   = 26, // pc
    // pc: builtins getlook / setlook (LOOK::WEAPON)
    WEAPON          = 27, // pc weapon look
    // pc: builtins getlook / setlook (LOOK::SHIELD)
    SHIELD          = 28, // pc
    // pc: builtins getlook / setlook (LOOK::HEAD_TOP)
    HEAD_TOP        = 29, // pc
    // pc: builtins getlook / setlook (LOOK::HEAD_MID)
    HEAD_MID        = 30, // pc
    // pc: builtins getlook / setlook (LOOK::HEAD_BOTTOM)
    HEAD_BOTTOM     = 31, // pc
    // pc: read via builtin getdir (attached player only). npc: write via builtin
    // setnpcdirection. read-by-gid, pc write, and mob: NEW
    LOOK_DIR        = 32, // mob/pc/npc facing direction
    // read via builtins getx (pc, self), getnpcx (npc), or PARAM SP::POS_X
    // (any unit). write NEW
    X               = 33, // current x; setting teleports on the same map
    // read via builtins gety (pc, self), getnpcy (npc), or PARAM SP::POS_Y
    // (any unit). write NEW
    Y               = 34, // current y; setting teleports on the same map
    // pc: read via builtin getmap, warp via builtin warp. mob: NEW for mob
    MAP             = 35, // map name (string); setting teleports to current x,y
    // NEW for mob (read-only)
    TARGET_ID       = 36, // mob target (read-only)
    // NEW for mob
    MASTER_ID       = 37, // mob master (e.g. for summons)
    // pc: PARAM SP::BASEEXP
    BASE_EXP        = 38, // pc
    // pc: PARAM SP::JOBEXP
    JOB_EXP         = 39, // pc
    // pc: PARAM SP::JOBLEVEL
    JOB_LEVEL       = 40, // pc
    // pc: PARAM SP::SKILLPOINT
    SKILL_POINT     = 41, // pc
    // pc: PARAM SP::STATUSPOINT
    STATUS_POINT    = 42, // pc
    // pc: PARAM SP::ZENY
    ZENY            = 43, // pc
    // NEW (SP::KARMA is disabled in the enum)
    KARMA           = 44, // pc
    // NEW
    MANNER          = 45, // pc
    // read-only; read via builtin getcharid(1)
    PARTY_ID        = 46, // pc (read-only)
    // NEW (builtins getopt2 / setopt2 cover Opt2, a different field)
    OPTION          = 47, // pc/mob/npc Opt0 status flags
    // pc: read via builtin strcharinfo(0). npc: read via builtin strnpcinfo,
    // write via builtin fakenpcname. mob: NEW for mob
    NAME            = 48, // string: pc char name, npc name, or mob name
};
} // namespace map
} // namespace tmwa
