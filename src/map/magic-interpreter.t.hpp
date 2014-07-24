#pragma once
//    magic-interpreter.t.hpp - Old magic.
//
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

#include "../generic/enum.hpp"


namespace tmwa
{
namespace magic
{
enum class SPELLARG : uint8_t
{
    NONE,
    PC,
    STRING,
};

enum class FOREACH_FILTER : uint8_t
{
    MOB,
    PC,
    ENTITY,
    TARGET,
    SPELL,
    NPC,
};

namespace e
{
enum class SPELL_FLAG : uint8_t
{
    ZERO        = 0,

    // spell associated not with caster but with place
    LOCAL       = 1 << 0,
    // spell invocation never uttered
    SILENT      = 1 << 1,
    // `magic word' only:  don't require spellcasting ability
    NONMAGIC    = 1 << 2,
};
ENUM_BITWISE_OPERATORS(SPELL_FLAG)
}
using e::SPELL_FLAG;

namespace e
{
enum class INVOCATION_FLAG : uint8_t
{
    ZERO        = 0,

    // Bound directly to the caster (i.e., ignore its location)
    BOUND       = 1 << 0,
    // Used `abort' to terminate
    ABORTED     = 1 << 1,
    // On magical attacks: if we run out of steam, stop attacking altogether
    STOPATTACK  = 1 << 2,
};
ENUM_BITWISE_OPERATORS(INVOCATION_FLAG)
}
using e::INVOCATION_FLAG;
} // namespace magic
} // namespace tmwa
