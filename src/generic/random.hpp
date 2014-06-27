#ifndef TMWA_GENERIC_RANDOM_HPP
#define TMWA_GENERIC_RANDOM_HPP
//    random.hpp - Random number generation.
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

# include "random.t.hpp"

# include <random>


namespace tmwa
{
// This is not namespace random since that collides with a C function,
// but this can be revisited when everything goes into namespace tmwa.
namespace random_
{
    /// Get a random number from 0 .. 2**32 - 1
    extern std::mt19937 generate;

    /// Get a random number from 0 .. bound - 1
    inline
    int to(int bound)
    {
        std::uniform_int_distribution<int> dist(0, bound - 1);
        return dist(generate);
    }

    /// Get a random number from low .. high (inclusive!)
    inline
    int in(int low, int high)
    {
        std::uniform_int_distribution<int> dist(low, high);
        return dist(generate);
    }

    inline
    bool coin()
    {
        // sigh, can't specify <bool> directly ...
        std::uniform_int_distribution<int> dist(false, true);
        return dist(generate);
    }

    inline
    bool chance(Fraction f)
    {
        if (f.num <= 0)
            return false;
        if (f.num >= f.den)
            return true;
        return random_::to(f.den) < f.num;
    }

    // C is usually one of:
    //  std::vector<T>
    //  std::initializer_list<T>
    //  std::array<T, n>
    template<class C>
    auto choice(C&& c) -> decltype(*c.begin())
    {
        return *(c.begin() + random_::to(c.size()));
    }

    // allow bare braces
    template<class T>
    T choice(std::initializer_list<T>&& il)
    {
        return random_::choice(il);
    }
} // namespace random_
} // namespace tmwa

#endif // TMWA_GENERIC_RANDOM_HPP
