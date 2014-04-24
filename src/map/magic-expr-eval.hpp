#ifndef TMWA_MAP_MAGIC_EXPR_EVAL_HPP
#define TMWA_MAP_MAGIC_EXPR_EVAL_HPP
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

# include "fwd.hpp"

# include "../range/slice.hpp"

# include "../strings/fwd.hpp"
# include "../strings/zstring.hpp"

# include "magic-interpreter.t.hpp"

/* Helper definitions for dealing with functions and operations */

int magic_signature_check(ZString opname, ZString funname, ZString signature,
        Slice<val_t> args, int line, int column);

void magic_area_rect(map_local **m, int *x, int *y, int *width, int *height,
        area_t& area);

// TODO kill this like I killed VAR
# define ARGINT(x) args[x].v.v_int
# define ARGDIR(x) args[x].v.v_dir
# define ARGSTR(x) ZString(args[x].v.v_string)
# define ARGENTITY(x) args[x].v.v_entity
# define ARGLOCATION(x) args[x].v.v_location
# define ARGAREA(x) args[x].v.v_area
# define ARGSPELL(x) args[x].v.v_spell
# define ARGINVOCATION(x) args[x].v.v_invocation

# define RESULTINT result->v.v_int
# define RESULTDIR result->v.v_dir
# define RESULTSTR result->v.v_string
# define RESULTENTITY result->v.v_entity
# define RESULTLOCATION result->v.v_location
# define RESULTAREA result->v.v_area
# define RESULTSPELL result->v.v_spell
# define RESULTINVOCATION result->v.v_invocation

# define ARG_TYPE(x) args[x].ty
# define ENTITY_TYPE(x) ARGENTITY(x)->bl_type

# define ARGPC(x)  (ARGENTITY(x)->is_player())
# define ARGNPC(x)  (ARGENTITY(x)->is_npc())
# define ARGMOB(x)  (ARGENTITY(x)->is_mob())

# define ARG_MAY_BE_AREA(x) (ARG_TYPE(x) == TYPE::AREA || ARG_TYPE(x) == TYPE::LOCATION)

#endif // TMWA_MAP_MAGIC_EXPR_EVAL_HPP
