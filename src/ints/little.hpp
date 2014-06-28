#pragma once
//    little.hpp - integers of known endianness
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

#include <endian.h>

#include <cstdint>


namespace tmwa
{
// We implement our own actual swapping, because glibc emits assembly
// instead of letting the *compiler* do what it does best.
#if __BYTE_ORDER != __BIG_ENDIAN && __BYTE_ORDER != __LITTLE_ENDIAN
# error "broken endians"
#endif

namespace ints
{
    // gcc doesn't always provide a builtin for this,
    // but it *does* optimize hand-written ones
    constexpr
    uint16_t bswap16(uint16_t v)
    {
        return v >> 8 | v << 8;
    }

    // TODO hoist this to byte.hpp and also implement big.hpp
    struct Byte
    {
        uint8_t value;
    };

    struct Little16
    {
        uint8_t data[2];
    };

    struct Little32
    {
        uint8_t data[4];
    };

    struct Little64
    {
        uint8_t data[8];
    };

    inline __attribute__((warn_unused_result))
    bool native_to_network(Byte *net, uint8_t nat)
    {
        net->value = nat;
        return true;
    }
    inline __attribute__((warn_unused_result))
    bool native_to_network(Little16 *net, uint16_t nat)
    {
        if (__BYTE_ORDER == __BIG_ENDIAN)
            nat = bswap16(nat);
        __builtin_memcpy(net, &nat, 2);
        return true;
    }
    inline __attribute__((warn_unused_result))
    bool native_to_network(Little32 *net, uint32_t nat)
    {
        if (__BYTE_ORDER == __BIG_ENDIAN)
            nat = __builtin_bswap32(nat);
        __builtin_memcpy(net, &nat, 4);
        return true;
    }
    inline __attribute__((warn_unused_result))
    bool native_to_network(Little64 *net, uint64_t nat)
    {
        if (__BYTE_ORDER == __BIG_ENDIAN)
            nat = __builtin_bswap64(nat);
        __builtin_memcpy(net, &nat, 8);
        return true;
    }

    inline __attribute__((warn_unused_result))
    bool network_to_native(uint8_t *nat, Byte net)
    {
        *nat = net.value;
        return true;
    }
    inline __attribute__((warn_unused_result))
    bool network_to_native(uint16_t *nat, Little16 net)
    {
        uint16_t tmp;
        __builtin_memcpy(&tmp, &net, 2);
        if (__BYTE_ORDER == __BIG_ENDIAN)
            tmp = bswap16(tmp);
        *nat = tmp;
        return true;
    }
    inline __attribute__((warn_unused_result))
    bool network_to_native(uint32_t *nat, Little32 net)
    {
        uint32_t tmp;
        __builtin_memcpy(&tmp, &net, 4);
        if (__BYTE_ORDER == __BIG_ENDIAN)
            tmp = __builtin_bswap32(tmp);
        *nat = tmp;
        return true;
    }
    inline __attribute__((warn_unused_result))
    bool network_to_native(uint64_t *nat, Little64 net)
    {
        uint64_t tmp;
        __builtin_memcpy(&tmp, &net, 8);
        if (__BYTE_ORDER == __BIG_ENDIAN)
            tmp = __builtin_bswap64(tmp);
        *nat = tmp;
        return true;
    }
} // namespace ints

using ints::Byte;
using ints::Little16;
using ints::Little32;
using ints::Little64;
} // namespace tmwa
