#ifndef TMWA_MMO_IP_HPP
#define TMWA_MMO_IP_HPP
//    ip.hpp - classes to deal with IP addresses.
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

# include <netinet/in.h>

# include <cstddef>
# include <cstdint>

# include "../strings/fwd.hpp"

// TODO - in the long run ports belong here also
// and of course, IPv6 stuff.
// But what about unix socket addresses?

/// Helper function
template<class T, size_t n>
constexpr
bool _ce_a_lt(T (&a)[n], T (&b)[n], size_t i=0)
{
    return (i != n
            && (a[i] < b[i]
                || (a[i] == b[i]
                    && _ce_a_lt(a, b, i + 1))));
}

/// A 32-bit Ipv4 address. Does not include a port.
/// Guaranteed to be laid out like the network wants.
class IP4Address
{
    uint8_t _addr[4];
public:
    constexpr
    IP4Address()
    : _addr{}
    {}
    constexpr explicit
    IP4Address(const uint8_t (&a)[4])
    : _addr{a[0], a[1], a[2], a[3]}
    {}
    explicit
    IP4Address(struct in_addr addr)
    {
        static_assert(sizeof(addr) == sizeof(_addr), "4 bytes");
        *this = IP4Address(reinterpret_cast<const uint8_t (&)[4]>(addr));
    }
    explicit
    operator struct in_addr() const
    {
        return reinterpret_cast<const struct in_addr&>(_addr);
    }

    constexpr friend
    IP4Address operator & (IP4Address l, IP4Address r)
    {
        return IP4Address({
                static_cast<uint8_t>(l._addr[0] & r._addr[0]),
                static_cast<uint8_t>(l._addr[1] & r._addr[1]),
                static_cast<uint8_t>(l._addr[2] & r._addr[2]),
                static_cast<uint8_t>(l._addr[3] & r._addr[3]),
        });
    }

    IP4Address& operator &= (IP4Address m)
    { return *this = *this & m; }

    const uint8_t *bytes() const
    { return _addr; }

    constexpr friend
    bool operator < (IP4Address l, IP4Address r)
    {
        return _ce_a_lt(l._addr, r._addr);
    }

    constexpr friend
    bool operator > (IP4Address l, IP4Address r)
    {
        return _ce_a_lt(r._addr, l._addr);
    }

    constexpr friend
    bool operator >= (IP4Address l, IP4Address r)
    {
        return !_ce_a_lt(l._addr, r._addr);
    }

    constexpr friend
    bool operator <= (IP4Address l, IP4Address r)
    {
        return !_ce_a_lt(r._addr, l._addr);
    }

    constexpr friend
    bool operator == (IP4Address l, IP4Address r)
    {
        return !(l < r || r < l);
    }

    constexpr friend
    bool operator != (IP4Address l, IP4Address r)
    {
        return (l < r || r < l);
    }
};

class IP4Mask
{
    IP4Address _addr, _mask;
public:
    constexpr
    IP4Mask() : _addr(), _mask()
    {}
    constexpr
    IP4Mask(IP4Address a, IP4Address m) : _addr(a & m), _mask(m)
    {}

    constexpr
    IP4Address addr() const
    { return _addr; }
    constexpr
    IP4Address mask() const
    { return _mask; }

    constexpr
    bool covers(IP4Address a) const
    {
        return (a & _mask) == _addr;
    }
};


constexpr
IP4Address IP4_LOCALHOST({127, 0, 0, 1});
constexpr
IP4Address IP4_BROADCAST({255, 255, 255, 255});


VString<15> convert_for_printf(IP4Address a);
VString<31> convert_for_printf(IP4Mask m);

bool extract(XString str, IP4Address *iv);

bool extract(XString str, IP4Mask *iv);

#endif // TMWA_MMO_IP_HPP
