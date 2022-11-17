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
    STR            = 13,
    AGI            = 14,
    VIT            = 15,
    INT            = 16,
    DEX            = 17,
    LUK            = 18,
    RANGE2         = 19,
    RANGE3         = 20,
    SCALE          = 21,
    RACE           = 22,
    ELEMENT        = 23,
    ELEMENT_LVL    = 24,
    MODE           = 25,
    SPEED          = 26,
    ADELAY         = 27,
    AMOTION        = 28,
    DMOTION        = 29,
    MUTATION_NUM   = 30,
    MUTATION_POWER = 31,
    DROPID1        = 32,
    DROPNAME1      = 33,
    DROPPERCENT1   = 34,
    DROPID2        = 35,
    DROPNAME2      = 36,
    DROPPERCENT2   = 37,
    DROPID3        = 38,
    DROPNAME3      = 39,
    DROPPERCENT3   = 40,
    DROPID4        = 41,
    DROPNAME4      = 42,
    DROPPERCENT4   = 43,
    DROPID5        = 44,
    DROPNAME5      = 45,
    DROPPERCENT5   = 46,
    DROPID6        = 47,
    DROPNAME6      = 48,
    DROPPERCENT6   = 49,
    DROPID7        = 50,
    DROPNAME7      = 51,
    DROPPERCENT7   = 52,
    DROPID8        = 53,
    DROPNAME8      = 54,
    DROPPERCENT8   = 55,
};

enum class MobInfo_DropArrays : uint8_t
{
    IDS      =  0,
    NAMES    =  1,
    PERCENTS =  2,
};
} // namespace map
} // namespace tmwa
