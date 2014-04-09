#include "nullpo.hpp"
//    nullpo.cpp - Non-fatal pointer assertions.
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

#include <cstdio>

#include "../poison.hpp"

/// Actual output function
static
void nullpo_info(const char *file, int line, const char *func)
{
    if (!file)
        file = "??";
    if (!func || !*func)
        func = "unknown";

    fprintf(stderr, "%s:%d: in func `%s': NULL pointer\n",
            file, line, func);
}

bool nullpo_chk(const char *file, int line, const char *func,
                 const void *target)
{
    if (target)
        return 0;

    nullpo_info(file, line, func);
    return 1;
}
