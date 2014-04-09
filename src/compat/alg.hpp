#ifndef TMWA_COMPAT_ALG_HPP
#define TMWA_COMPAT_ALG_HPP
//    alg.hpp - Silly math stuff.
//
//    Copyright © 2012 Ben Longbons <b.r.longbons@gmail.com>
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

# include "../sanity.hpp"

# include <type_traits>


template<class A, class B>
typename std::common_type<A, B>::type min(A a, B b)
{
    return a < b ? a : b;
}

template<class A, class B>
typename std::common_type<A, B>::type max(A a, B b)
{
    return b < a ? a : b;
}

#endif // TMWA_COMPAT_ALG_HPP
