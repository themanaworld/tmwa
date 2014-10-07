#pragma once
//    script-parse-internal.hpp - EAthena script frontend, engine, and library.
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

#include "script-parse.hpp"
#include "fwd.hpp"

#include "../strings/rstring.hpp"


namespace tmwa
{
enum class StringCode : uint8_t
{
    NOP, POS, INT, PARAM, FUNC,
    VARIABLE,
};
enum class ByteCode : uint8_t
{
    // types and specials
    // Note that 'INT' is synthetic, and does not occur in the data stream
    NOP, POS, INT, PARAM, FUNC, STR, ARG,
    VARIABLE, EOL,

    // unary and binary operators
    LOR, LAND, LE, LT, GE, GT, EQ, NE,
    XOR, OR, AND, ADD, SUB, MUL, DIV, MOD,
    NEG, LNOT, NOT, R_SHIFT, L_SHIFT,

    // additions
    // needed because FUNC is used for the actual call
    FUNC_REF,
};

struct str_data_t
{
    StringCode type;
    RString strs;
    int backpatch;
    int label_;
    int val;
};

extern
Map<RString, str_data_t> str_datam;
extern
InternPool variable_names;

str_data_t *search_strp(XString p);
str_data_t *add_strp(XString p);
} // namespace tmwa
