#pragma once
//    time_t.hpp - time_t with a reliable representation
//
//    Copyright © 2013-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <ctime>

#include "../ints/little.hpp"

#include "operators.hpp"


namespace tmwa
{
// Exists in place of time_t, to give it a predictable printf-format.
// (on x86 and amd64, time_t == long, but not on x32)
static_assert(sizeof(long long) >= sizeof(time_t), "long long >= time_t");
struct TimeT : Comparable
{
    long long value;

    // conversion
    TimeT(time_t t=0) : value(t) {}
    TimeT(struct tm t) : value(timegm(&t)) {}
    operator time_t() const { return value; }
    operator struct tm() const { time_t v = value; return *gmtime(&v); }

    explicit operator bool() const { return value; }
    bool operator !() const { return !value; }

    // prevent surprises
    template<class T>
    TimeT(T) = delete;
    template<class T>
    operator T() const = delete;

    static
    TimeT now()
    {
        // poisoned, but this is still in header-land
        return time(nullptr);
    }

    bool error() const
    {
        return value == -1;
    }
    bool okay() const
    {
        return !error();
    }
};

inline
long long convert_for_printf(TimeT t)
{
    return t.value;
}

// 2038 problem
inline __attribute__((warn_unused_result))
bool native_to_network(Little32 *net, TimeT nat)
{
    time_t tmp = nat;
    return native_to_network(net, static_cast<uint32_t>(tmp));
}

inline __attribute__((warn_unused_result))
bool network_to_native(TimeT *nat, Little32 net)
{
    uint32_t tmp;
    bool rv = network_to_native(&tmp, net);
    *nat = static_cast<time_t>(tmp);
    return rv;
}

inline __attribute__((warn_unused_result))
bool native_to_network(Little64 *net, TimeT nat)
{
    time_t tmp = nat;
    return native_to_network(net, static_cast<uint64_t>(tmp));
}

inline __attribute__((warn_unused_result))
bool network_to_native(TimeT *nat, Little64 net)
{
    uint64_t tmp;
    bool rv = network_to_native(&tmp, net);
    *nat = static_cast<time_t>(tmp);
    return rv;
}
} // namespace tmwa
