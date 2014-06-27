#ifndef TMWA_INTS_WRAP_HPP
#define TMWA_INTS_WRAP_HPP
//    wrap.hpp - basic integer wrapper classes
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

# include <cstdint>

# include <type_traits>


namespace tmwa
{
namespace ints
{
    namespace wrapped
    {
        template<class R>
        struct Wrapped
        {
            typedef R wrapped_type;
            R _value;
        protected:
            constexpr
            Wrapped(uint32_t v=0)
            : _value(v)
            {}
        public:
            explicit
            operator bool () const { return _value; }
            bool operator !() const { return !_value; }
        };

        template<class W, typename=typename W::wrapped_type>
        bool operator == (W l, W r)
        {
            return l._value == r._value;
        }
        template<class W, typename=typename W::wrapped_type>
        bool operator != (W l, W r)
        {
            return l._value != r._value;
        }
        template<class W, typename=typename W::wrapped_type>
        bool operator < (W l, W r)
        {
            return l._value < r._value;
        }

        template<class T>
        constexpr
        typename T::wrapped_type unwrap(typename std::enable_if<true, T>::type w)
        {
            return w._value;
        }
        template<class T>
        constexpr
        T wrap(typename T::wrapped_type v)
        {
            struct Sub : T
            {
                constexpr
                Sub(typename T::wrapped_type v2)
                : T(v2)
                {}
            };
            return Sub(v);
        }

        template<class W>
        constexpr
        W next(W w)
        {
            return wrap<W>(unwrap<W>(w) + 1);
        }
        template<class W>
        constexpr
        W prev(W w)
        {
            return wrap<W>(unwrap<W>(w) - 1);
        }

        template<class R>
        R convert_for_printf(Wrapped<R> w)
        {
            return w._value;
        }
    } // namespace wrapped
} // namespace ints

using ints::wrapped::Wrapped;
using ints::wrapped::unwrap;
using ints::wrapped::wrap;
} // namespace tmwa

#endif // TMWA_INTS_WRAP_HPP
