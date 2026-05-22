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
} // namespace map
} // namespace tmwa
