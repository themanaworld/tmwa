#pragma once
//    enums.hpp - Common enumerated types
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
enum class SkillID : uint16_t;
constexpr SkillID MAX_SKILL = SkillID(474); // not 450
constexpr SkillID get_enum_min_value(SkillID) { return SkillID(); }
constexpr SkillID get_enum_max_value(SkillID) { return MAX_SKILL; }

namespace e
{
enum class EPOS : uint16_t
{
    ZERO    = 0x0000,

    LEGS    = 0x0001,
    WEAPON  = 0x0002,
    GLOVES  = 0x0004,
    CAPE    = 0x0008,
    MISC1   = 0x0010,
    SHIELD  = 0x0020,
    SHOES   = 0x0040,
    MISC2   = 0x0080,
    HAT     = 0x0100,
    TORSO   = 0x0200,

    ARROW   = 0x8000,
};
ENUM_BITWISE_OPERATORS(EPOS)

constexpr EPOS get_enum_min_value(EPOS) { return EPOS(0x0000); }
constexpr EPOS get_enum_max_value(EPOS) { return EPOS(0xffff); }
}
using e::EPOS;

namespace e
{
enum class SkillFlags : uint16_t;
}
using e::SkillFlags;

// Opt0 and Opt1..3 in map.hpp
namespace e
{
enum class Opt0 : uint16_t;
constexpr Opt0 get_enum_min_value(Opt0) { return Opt0(0x0000); }
constexpr Opt0 get_enum_max_value(Opt0) { return Opt0(0xffff); }
}
using e::Opt0;

enum class ATTR
{
    STR = 0,
    AGI = 1,
    VIT = 2,
    INT = 3,
    DEX = 4,
    LUK = 5,

    COUNT = 6,
};

constexpr ATTR ATTRs[6] =
{
    ATTR::STR,
    ATTR::AGI,
    ATTR::VIT,
    ATTR::INT,
    ATTR::DEX,
    ATTR::LUK,
};

enum class ItemLook : uint16_t
{
    NONE = 0,
    BLADE = 1, // or some other common weapons
    _2,
    SETZER_AND_SCYTHE = 3,
    _6,
    STAFF = 10,
    BOW = 11,
    _13 = 13,
    _14 = 14,
    _16 = 16,
    SINGLE_HANDED_COUNT = 17,

    DUAL_BLADE = 0x11,
    DUAL_2 = 0x12,
    DUAL_6 = 0x13,
    DUAL_12 = 0x14,
    DUAL_16 = 0x15,
    DUAL_26 = 0x16,
};

enum class SEX : uint8_t
{
    FEMALE = 0,
    MALE = 1,
    // For items. This is also used as error, sometime.
    NEUTRAL = 2,
};
inline
char sex_to_char(SEX sex)
{
    switch (sex)
    {
    case SEX::FEMALE: return 'F';
    case SEX::MALE: return 'M';
    default: return '\0';
    }
}
inline
SEX sex_from_char(char c)
{
    switch (c)
    {
    case 'F': return SEX::FEMALE;
    case 'M': return SEX::MALE;
    default: return SEX::NEUTRAL;
    }
}

inline
bool native_to_network(char *network, SEX native)
{
    *network = sex_to_char(native);
    return true;
}
inline
bool network_to_native(SEX *native, char network)
{
    *native = sex_from_char(network);
    return true;
}
} // namespace tmwa
