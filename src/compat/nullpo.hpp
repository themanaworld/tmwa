#ifndef TMWA_COMPAT_NULLPO_HPP
#define TMWA_COMPAT_NULLPO_HPP
//    nullpo.hpp - Non-fatal pointer assertions.
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

/// Uncomment this to live dangerously
/// (really exist to detect mostly-unused variables)
//# define BUG_FREE

/// All functions print to standard error (was: standard output)
/// nullpo_ret(cond) - return 0 if given pointer is NULL
/// nullpo_retv(cond) - just return (function returns void)
/// nullpo_retr(rv, cond) - return given value instead

# ifndef BUG_FREE
#  define nullpo_retr(ret, t)                                   \
    if (nullpo_chk(__FILE__, __LINE__, __PRETTY_FUNCTION__, t)) \
        return ret;
# else // BUG_FREE
#  define nullpo_retr(ret, t) /*t*/
# endif // BUG_FREE

# define nullpo_ret(t) nullpo_retr(0, t)
# define nullpo_retv(t) nullpo_retr(, t)

# include "fwd.hpp"

/// Used by macros in this header
bool nullpo_chk(const char *file, int line, const char *func,
        const void *target);

template<class T>
bool nullpo_chk(const char *file, int line, const char *func, T target)
{
    return nullpo_chk(file, line, func, target.operator->());
}
template<class T>
bool nullpo_chk(const char *file, int line, const char *func, T *target)
{
    return nullpo_chk(file, line, func, static_cast<const void *>(target));
}

#endif // TMWA_COMPAT_NULLPO_HPP
