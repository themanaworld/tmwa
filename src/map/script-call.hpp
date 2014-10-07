#pragma once
//    script-call.hpp - EAthena script frontend, engine, and library.
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

#include "script-call.t.hpp"

#include "fwd.hpp"

#include "../range/fwd.hpp"

#include "../generic/fwd.hpp"

#include "../mmo/fwd.hpp"


namespace tmwa
{
enum class ByteCode : uint8_t;

// implemented in script-parse.cpp because reasons
struct ScriptPointer
{
    const ScriptBuffer *code;
    size_t pos;

    ScriptPointer()
    : code()
    , pos()
    {}

    ScriptPointer(const ScriptBuffer *c, size_t p)
    : code(c)
    , pos(p)
    {}

    ByteCode peek() const;
    ByteCode pop();
    ZString pops();
};

int run_script_l(ScriptPointer, BlockId, BlockId, Slice<argrec_t> args);
int run_script(ScriptPointer, BlockId, BlockId);

void set_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e, int val);
void set_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e, XString val);

int get_script_var_i(dumb_ptr<map_session_data> sd, VarName var, int e);
ZString get_script_var_s(dumb_ptr<map_session_data> sd, VarName var, int e);
} // namespace tmwa
