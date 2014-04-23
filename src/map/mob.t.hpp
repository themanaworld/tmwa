#ifndef TMWA_MAP_MOB_T_HPP
#define TMWA_MAP_MOB_T_HPP
//    mob.t.hpp - Really scary code.
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

# include <cstdint>

enum class MobSkillTarget
{
    MST_TARGET = 0,
    MST_SELF,
};

/// Used as a condition when trying to apply the chosen mob skill.
enum class MobSkillCondition : uint16_t
{
    // used as something that never compares true
    NEVER_EQUAL = 0xfffe,
    ANY = 0xffff,

    MSC_ALWAYS = 0x0000,
    MSC_MYHPLTMAXRATE = 0x0001,

    MSC_NOTINTOWN = 0x0032,

    MSC_SLAVELT = 0x0110,
    MSC_SLAVELE = 0x0111,
};

/// Used as a filter when trying to choose a mob skill to use.
enum class MobSkillState : uint8_t
{
    ANY = 0xff,

    MSS_IDLE = 0,
    MSS_WALK,
    MSS_ATTACK,
    MSS_DEAD,
    MSS_LOOT,
    MSS_CHASE,
};

#endif // TMWA_MAP_MOB_T_HPP
