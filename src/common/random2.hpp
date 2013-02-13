#ifndef RANDOM2_HPP
#define RANDOM2_HPP

# include "random.hpp"
# include "utils2.hpp"

# include <algorithm>

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

#endif // RANDOM2_HPP
