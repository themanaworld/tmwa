#ifndef TMWA_GENERIC_OOPS_HPP
#define TMWA_GENERIC_OOPS_HPP
//    oops.hpp - Stuff that shouldn't happen.
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

# include "fwd.hpp"

# include <cstddef>

# include <stdexcept>

# include "oops.hpp"


class AssertionError : public std::runtime_error
{
    const char *_what;
public:
    AssertionError(const char *desc, const char *expr,
            const char *file, size_t line, const char *function);
};

# define ALLEGE(desc, expr) \
    if (expr) {} \
    else throw AssertionError(desc, #expr, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#endif // TMWA_GENERIC_OOPS_HPP
