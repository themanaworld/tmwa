#include "oops.hpp"
//    oops.cpp - Stuff that shouldn't happen.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <cstdlib>
#include <cstring>
#include <cstdio>

//#include "../poison.hpp"


static
std::string do_asprintf(const char *desc, const char *expr,
        const char *file, size_t line, const char *function)
{
    char *what = nullptr;
    int len = asprintf(&what, "%s:%zu: error: in '%s', incorrectly alleged that '%s' (%s)",
            file, line, function, desc, expr);
    if (len == -1)
        abort();
    std::string out = what;
    free(what);
    return out;
}

AssertionError::AssertionError(const char *desc, const char *expr,
        const char *file, size_t line, const char *function)
: std::runtime_error(do_asprintf(desc, expr, file, line, function))
{}
