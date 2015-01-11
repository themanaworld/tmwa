#pragma once
//    magic-expr-eval.hpp - Utilities for evaluating magic.
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

#include "../strings/zstring.hpp"

#include "magic-interpreter.t.hpp"


namespace tmwa
{
namespace map
{
namespace magic
{
// TODO soon kill this unlike I killed VAR
#define ARGINT(x) args[x].get_if<ValInt>()->v_int
#define ARGDIR(x) args[x].get_if<ValDir>()->v_dir
#define ARGSTR(x) ZString(args[x].get_if<ValString>()->v_string)
#define ARGENTITY(x) args[x].get_if<ValEntityPtr>()->v_entity
#define ARGLOCATION(x) args[x].get_if<ValLocation>()->v_location
#define ARGAREA(x) args[x].get_if<ValArea>()->v_area
#define ARGSPELL(x) args[x].get_if<ValSpell>()->v_spell
#define ARGINVOCATION(x) args[x].get_if<ValInvocationPtr>()->v_invocation

#define ENTITY_TYPE(x) ARGENTITY(x)->bl_type

#define ARGPC(x)  (ARGENTITY(x)->is_player())
#define ARGNPC(x)  (ARGENTITY(x)->is_npc())
#define ARGMOB(x)  (ARGENTITY(x)->is_mob())

#define ARG_MAY_BE_AREA(x) (args[x].is<ValArea>() || args[x].is<ValArea>())
} // namespace magic
} // namespace map
} // namespace tmwa
