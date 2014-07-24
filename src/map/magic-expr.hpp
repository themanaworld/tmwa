#pragma once
//    magic-expr.hpp - Pure functions for the old magic backend.
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

#include "../generic/fwd.hpp"

#include "../range/fwd.hpp"

#include "../strings/zstring.hpp"
#include "../strings/literal.hpp"

#include "../mmo/fwd.hpp"

#include "magic-interpreter.t.hpp"


namespace tmwa
{
namespace magic
{
/*
 * Argument types:
 *  i : int
 *  d : dir
 *  s : string
 *  e : entity
 *  l : location
 *  a : area
 *  S : spell
 *  I : invocation
 *  . : any, except for fail/undef
 *  _ : any, including fail, but not undef
 */
struct fun_t
{
    LString name;
    LString signature;
    char ret_ty;
    int (*fun)(dumb_ptr<env_t> env, val_t *result, Slice<val_t> arga);
};

/**
 * Retrieves a function by name
 * @param name The name to look up
 * @return A function of that name, or nullptr.
 */
fun_t *magic_get_fun(ZString name);

/**
 * Evaluates an expression and stores the result in `dest'
 */
void magic_eval(dumb_ptr<env_t> env, val_t *dest, dumb_ptr<expr_t> expr);

/**
 * Evaluates an expression and coerces the result into an integer
 */
int magic_eval_int(dumb_ptr<env_t> env, dumb_ptr<expr_t> expr);

/**
 * Evaluates an expression and coerces the result into a string
 */
AString magic_eval_str(dumb_ptr<env_t> env, dumb_ptr<expr_t> expr);

void magic_clear_var(val_t *v);

void magic_copy_var(val_t *dest, const val_t *src);

void magic_random_location(location_t *dest, dumb_ptr<area_t> area);

// ret -1: not a string, ret 1: no such item, ret 0: OK
int magic_find_item(Slice<val_t> args, int index, Item *item, int *stackable);

#define GET_ARG_ITEM(index, dest, stackable)                    \
     switch (magic_find_item(args, index, &dest, &stackable))   \
    {                                                           \
        case -1: return 1;                                      \
        case 1: return 0;                                       \
        default: break;                                         \
    }

int magic_location_in_area(map_local *m, int x, int y, dumb_ptr<area_t> area);

/* Helper definitions for dealing with functions and operations */

int magic_signature_check(ZString opname, ZString funname, ZString signature,
        Slice<val_t> args, int line, int column);

void magic_area_rect(map_local **m, int *x, int *y, int *width, int *height,
        area_t& area);
} // namespace magic
} // namespace tmwa
