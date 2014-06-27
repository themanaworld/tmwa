#ifndef TMWA_INTS_CMP_HPP
#define TMWA_INTS_CMP_HPP
//    cmp.hpp - comparison related operations
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

# include <limits>


namespace tmwa
{
namespace ints
{
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wtype-limits"
    template<class T, class U>
    T saturate(const U& v)
    {
        typedef std::numeric_limits<T> Tlim;
        typedef std::numeric_limits<U> Ulim;

        if (Tlim::is_signed == Ulim::is_signed)
        {
            if (v > Tlim::max())
                return Tlim::max();
            if (v < Tlim::min())
                return Tlim::min();
            return v;
        }
        else if (Ulim::is_signed)
        {
            // from signed to unsigned

            // Not like v < Tlim::min(), even though Tlim::min() == 0
            if (v < 0)
                return 0;
            if (v > Tlim::max())
                return Tlim::max();
            return v;
        }
        else // Tlim::is_signed
        {
            // from unsigned to signed
            if (v > Tlim::max())
                return Tlim::max();
            return v;
        }
    }
# pragma GCC diagnostic pop
} // namespace ints

using ints::saturate;
} // namespace tmwa

#endif // TMWA_INTS_CMP_HPP
