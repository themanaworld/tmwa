#pragma once
//    attr.hpp - Attributes.
//
//    Copyright © 2013-2014 Ben Longbons <b.r.longbons@gmail.com>
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


namespace tmwa
{
#ifdef __clang__
# define FALLTHROUGH [[clang::fallthrough]]
#else
# define FALLTHROUGH /* fallthrough */
#endif

#define JOIN(a, b) a##b

//  first loop:
//      declare flag 'guard' (initially true)
//      declare flag 'broken' (initially false)
//      condition is 'guard' must be true, which is the case only for the first iteration
//      post checks 'broken' and if set, break the loop
//  second loop:
//      declare public 'var' variable
//      condition is that 'guard' must be true
//      post sets 'guard' to false to make this loop run only once
//  third loop:
//      enable broken flag; it will remain set if 'break' is in the loop
//      condition is that 'broken' must be true
//      post sets 'broken' to false, which then fails the condition
//      if user has a 'break' inside, then 'broken' will be true
//      in either case, go back to the second loop's post
#define WITH_VAR_INLOOP(ty, var, expr)                                                                                          \
    for (bool JOIN(var, _guard) = true, JOIN(var, _broken) = false; JOIN(var, _guard); ({if (JOIN(var, _broken)) { break; } })) \
        for (ty var = expr; JOIN(var, _guard); JOIN(var, _guard) = false)                                                       \
            for (JOIN(var, _broken) = true; JOIN(var, _broken); JOIN(var, _broken) = false)
#define WITH_VAR_NOLOOP(ty, var, expr)                                                                                          \
    for (bool JOIN(var, _guard) = true, JOIN(var, _broken) = false; JOIN(var, _guard); ({if (JOIN(var, _broken)) {abort();} })) \
        for (ty var = expr; JOIN(var, _guard); JOIN(var, _guard) = false)                                                       \
            for (JOIN(var, _broken) = true; JOIN(var, _broken); JOIN(var, _broken) = false)
} // namespace tmwa
