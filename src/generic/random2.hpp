#ifndef TMWA_GENERIC_RANDOM2_HPP
#define TMWA_GENERIC_RANDOM2_HPP
//    random2.hpp - Random number generation.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
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

# include "random.hpp"

# include <algorithm>

# include "../compat/iter.hpp"

namespace random_
{
    namespace detail
    {
        struct RandomIterator
        {
            int bound;
            int current;
            bool frist;

            void operator ++()
            {
                frist = false;
                // TODO: reimplement in terms of LFSRs, so that certain
                // blockage patterns don't favor adjacent cells.
                current = current + 1;
                if (current == bound)
                    current = 0;
            }
            int operator *()
            {
                return current;
            }
        };

        inline
        bool operator == (RandomIterator l, RandomIterator r)
        {
            return l.current == r.current && l.frist == r.frist;
        }

        inline
        bool operator != (RandomIterator l, RandomIterator r)
        {
            return !(l == r);
        }
    }

    /// Yield every cell from 0 .. bound - 1 in some order.
    /// The starting position is random, but not the order itself.
    ///
    /// Expected usage:
    ///     for (int i : random_::iterator(vec.size()))
    ///         if (vec[i].okay())
    ///             return frob(vec[i]);
    inline
    IteratorPair<detail::RandomIterator> iterator(int bound)
    {
        int current = random_::to(bound);
        return
        {
            detail::RandomIterator{bound, current, true},
            detail::RandomIterator{bound, current, false}
        };
    }

    /// similar to std::random_shuffle(c.begin(), c.end()), but guaranteed
    /// to use a good source of randomness.
    template<class C>
    void shuffle(C&& c)
    {
        std::random_shuffle(c.begin(), c.end(), random_::to);
    }
} // namespace random_

#endif // TMWA_GENERIC_RANDOM2_HPP
