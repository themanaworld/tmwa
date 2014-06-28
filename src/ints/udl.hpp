#pragma once
//    udl.hpp - user-defined literals for integers.
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

#include "fwd.hpp"

#include <cstdint>

#include <type_traits>


namespace tmwa
{
namespace ints
{
    namespace
    {
        typedef unsigned long long ullong;
        template<char C>
        struct CharParser
        {
            constexpr static
            bool is_dec = '0' <= C && C <= '9';
            constexpr static
            bool is_upper = 'A' <= C && C <= 'F';
            constexpr static
            bool is_lower = 'a' <= C && C <= 'f';

            static_assert(is_dec || is_upper || is_lower, "char base");

            constexpr static
            ullong value = is_upper ? (C - 'A' + 10) : is_lower ? (C - 'a' + 10) : C - '0';
        };

        template<ullong base, ullong accum, char... C>
        struct BaseParser;
        template<ullong base, ullong accum, char F, char... R>
        struct BaseParser<base, accum, F, R...>
        {
            constexpr static
            ullong mulled = accum * base;
            constexpr static
            ullong add = CharParser<F>::value;
            static_assert(add < base, "parse base");
            constexpr static
            ullong added = mulled + add;
            static_assert(added > accum || accum == 0, "parse overflow");

            constexpr static
            ullong value = BaseParser<base, added, R...>::value;
        };
        template<ullong base, ullong accum>
        struct BaseParser<base, accum>
        {
            constexpr static
            ullong value = accum;
        };

        template<char... C>
        struct IntParser
        {
            constexpr static
            ullong value = BaseParser<10, 0, C...>::value;
        };

        template<char... C>
        struct IntParser<'0', C...>
        {
            constexpr static
            ullong value = BaseParser<8, 0, C...>::value;
        };

        template<char... C>
        struct IntParser<'0', 'x', C...>
        {
            constexpr static
            ullong value = BaseParser<16, 0, C...>::value;
        };

        template<char... C>
        struct IntParser<'0', 'X', C...>
        {
            constexpr static
            ullong value = BaseParser<16, 0, C...>::value;
        };

        template<char... C>
        struct IntParser<'0', 'b', C...>
        {
            constexpr static
            ullong value = BaseParser<2, 0, C...>::value;
        };

        template<char... C>
        struct IntParser<'0', 'B', C...>
        {
            constexpr static
            ullong value = BaseParser<2, 0, C...>::value;
        };

        template<bool S, ullong V>
        struct SignedMagnitudeConstant
        {
            static constexpr
            bool sign = S;
            static constexpr
            ullong magnitude = V;

            template<class T>
            constexpr
            operator T() const
            {
                typedef typename std::make_unsigned<T>::type U;
                // boo, body of constexpr function can't use variables
#define is_signed bool(T(-1) < T(0))
                static_assert(is_signed >= (sign && magnitude), "signed");
#define max ullong(ullong(U(-1) >> is_signed))
                static_assert(magnitude <= max || (sign && magnitude == max + 1), "magna");
#undef is_signed
#undef max
                return sign ? T(ullong(-magnitude)) : T(magnitude);
            }
        };

        template<bool S1, ullong V1, bool S2, ullong V2>
        constexpr
        bool operator == (SignedMagnitudeConstant<S1, V1>, SignedMagnitudeConstant<S2, V2>)
        {
            return V1 == V2 && (S1 == S2 || !V1);
        }
        template<bool S1, ullong V1, bool S2, ullong V2>
        constexpr
        bool operator != (SignedMagnitudeConstant<S1, V1> lhs, SignedMagnitudeConstant<S2, V2> rhs)
        {
            return !(lhs == rhs);
        }
        template<bool S, ullong V>
        constexpr
        SignedMagnitudeConstant<!S, V> operator -(SignedMagnitudeConstant<S, V>)
        {
            return {};
        }

        struct pint8 { int8_t value; int8_t operator +() { return value; } };
        struct pint16 { int16_t value; int16_t operator +() { return value; } };
        struct pint32 { int32_t value; int32_t operator +() { return value; } };
        struct pint64 { int64_t value; int64_t operator +() { return value; } };
        struct nint8 { int8_t value; int8_t operator -() { return value; } };
        struct nint16 { int16_t value; int16_t operator -() { return value; } };
        struct nint32 { int32_t value; int32_t operator -() { return value; } };
        struct nint64 { int64_t value; int64_t operator -() { return value; } };

        template<char... C>
        constexpr
        SignedMagnitudeConstant<false, IntParser<C...>::value> operator "" _const()
        { return {}; }

        template<char... C>
        constexpr
        uint8_t operator "" _u8 () { return operator "" _const<C...>(); }
        template<char... C>
        constexpr
        uint16_t operator "" _u16 () { return operator "" _const<C...>(); }
        template<char... C>
        constexpr
        uint32_t operator "" _u32 () { return operator "" _const<C...>(); }
        template<char... C>
        constexpr
        uint64_t operator "" _u64 () { return operator "" _const<C...>(); }
        template<char... C>
        constexpr
        pint8 operator "" _p8 () { return pint8{operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        pint16 operator "" _p16 () { return pint16{operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        pint32 operator "" _p32 () { return pint32{operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        pint64 operator "" _p64 () { return pint64{operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        nint8 operator "" _n8 () { return nint8{-operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        nint16 operator "" _n16 () { return nint16{-operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        nint32 operator "" _n32 () { return nint32{-operator "" _const<C...>()}; }
        template<char... C>
        constexpr
        nint64 operator "" _n64 () { return nint64{-operator "" _const<C...>()}; }
    } // anonymous namespace
} // namespace ints

using ints::operator "" _const;

using ints::operator "" _u8;
using ints::operator "" _u16;
using ints::operator "" _u32;
using ints::operator "" _u64;
using ints::operator "" _p8;
using ints::operator "" _p16;
using ints::operator "" _p32;
using ints::operator "" _p64;
using ints::operator "" _n8;
using ints::operator "" _n16;
using ints::operator "" _n32;
using ints::operator "" _n64;
} // namespace tmwa
