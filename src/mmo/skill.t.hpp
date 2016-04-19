#pragma once
//    skill.t.hpp - Old-style skills.
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

#include "../generic/enum.hpp"


namespace tmwa
{
// TODO remove most of these as their corresponding SkillIDs get deleted.
enum class StatusChange : uint16_t
{
    // indices into (map_session_data).status_change
    SC_SENDMAX          = 256,

    // sometimes means "none", sometimes not
    NEGATIVE1           = 0xffff,

    // these ones are used by clif_status_change,
    // e.g. by the magic system
    ZERO                = 0,
    CLIF_OPTION_SC_INVISIBILITY = 0x1000,

    // the rest are the normal effects
    SC_SLOWPOISON       = 14,   // item script

    SC_WEIGHT50         = 35,   // ? sort of used
    SC_WEIGHT90         = 36,   // definitely used
    SC_SPEEDPOTION0     = 37,   // item script

    SC_HEALING          = 70,   // item script

    SC_POISON           = 132,  // bad; actually used

    SC_ATKPOT           = 185,  // item script
    SC_MATKPOT          = 186,  // unused, but kept for parallel

// Added for Fate's spells
    SC_HIDE             = 194,  // Hide from `detect' magic (PCs only)
    SC_SHEARED          = 194,  // Has been sheared (mobs only)
    SC_HALT_REGENERATE  = 195,  // Suspend regeneration
    SC_FLYING_BACKPACK  = 196,  // Flying backpack
    SC_MBARRIER         = 197,  // Magical barrier, magic resistance (val1 : power (%))
    SC_HASTE            = 198,  // `Haste' spell (val1 : power)
    SC_PHYS_SHIELD      = 199,  // `Protect' spell, reduce damage (val1: power)
    MAX_STATUSCHANGE    = 200,
};

enum class SkillID : uint16_t
{
    // TODO: Remove these!
    NEGATIVE            = 0xffff,
    ZERO                = 0x0000,
    // this is probably the remains of the 'basic' skill,
    // which has since been partially split into emote, trade, and party,
    // but the confusion is caused by the fact that it also covered attacks.
    ONE                 = 0x0001,

    // Basic skills.
    // These should probably be made unconditional.
    NV_EMOTE            = 1,    //
    NV_TRADE            = 2,    //
    NV_PARTY            = 3,    //

    AC_OWL              = 45,   // Mallard's Eye

    NPC_SELFDESTRUCTION = 175,  //

    NPC_POISON          = 178,  //

    NPC_SUMMONSLAVE     = 198,  //
    NPC_EMOTION         = 199,  //

    TMW_SKILLPOOL       = 339,  // skill pool size

    // magic skills
    TMW_MAGIC           = 340,  //
    TMW_MAGIC_LIFE      = 341,  //
    TMW_MAGIC_WAR       = 342,  //
    TMW_MAGIC_TRANSMUTE = 343,  //
    TMW_MAGIC_NATURE    = 344,  //
    TMW_MAGIC_ETHER     = 345,  //
    TMW_MAGIC_DARK      = 346,  //
    TMW_MAGIC_LIGHT     = 347,  //

    // focusable skills
    TMW_BRAWLING        = 350,  //
    TMW_LUCKY_COUNTER   = 351,  //
    TMW_SPEED           = 352,  //
    TMW_RESIST_POISON   = 353,  //
    TMW_ASTRAL_SOUL     = 354,  //
    TMW_RAGING          = 355,  //

    // Note: this value is also hard-coded in common/mmo.hpp
    MAX_SKILL_DB        = 474, // not 450
};

namespace e
{
enum class SkillFlags : uint16_t
{
    ZERO            = 0x00,
    // is a pool skill
    POOL_FLAG       = 0x01,
    // is an active pool skill
    POOL_ACTIVE     = 0x02,
    // pool skill has been activated (used for clif)
    POOL_ACTIVATED  = 0x04,
};
ENUM_BITWISE_OPERATORS(SkillFlags)
}
using e::SkillFlags;
} // namespace tmwa
