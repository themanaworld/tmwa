#ifndef TMWA_MAP_MAGIC_INTERPRETER_T_HPP
#define TMWA_MAP_MAGIC_INTERPRETER_T_HPP
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

# include "fwd.hpp"

# include "../generic/enum.hpp"

enum class SPELLARG : uint8_t
{
    NONE,
    PC,
    STRING,
};

enum class TYPE : uint8_t
{
    UNDEF,
    INT,
    DIR,
    STRING,
    ENTITY,
    LOCATION,
    AREA,
    SPELL,
    INVOCATION,
    FAIL = 127,

    NEGATIVE_1 = 255,
};

enum class AREA : uint8_t
{
    LOCATION,
    UNION,
    RECT,
    BAR,
};

enum class EXPR : uint8_t
{
    VAL,
    LOCATION,
    AREA,
    FUNAPP,
    ID,
    SPELLFIELD,
};

// temporary rename to avoid collision with enum value
// in magic-interpreter-parser
enum class EFFECT : uint8_t
{
    SKIP,
    ABORT,
    ASSIGN,
    FOREACH,
    FOR,
    IF,
    SLEEP,
    SCRIPT,
    BREAK,
    OP,
    END,
    CALL,
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

enum class SPELLGUARD : uint8_t
{
    CONDITION,
    COMPONENTS,
    CATALYSTS,
    CHOICE,
    MANA,
    CASTTIME,
    EFFECT,
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

enum class CONT_STACK : uint8_t
{
    FOREACH,
    FOR,
    PROC,
};

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

#endif // TMWA_MAP_MAGIC_INTERPRETER_T_HPP
