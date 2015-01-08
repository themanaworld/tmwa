#include "ip.hpp"
//    ip.cpp - Implementation of IP address functions.
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

#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"

#include "../poison.hpp"


namespace tmwa
{
bool impl_extract(XString str, IP4Address *rv)
{
    if (str.endswith('.'))
        return false;
    uint8_t buf[4];
    if (extract(str, record<'.'>(&buf[0], &buf[1], &buf[2], &buf[3])))
    {
        *rv = IP4Address(buf);
        return true;
    }
    return false;
}

bool impl_extract(XString str, IP4Mask *rv)
{
    IP4Address a, m;
    unsigned b;
    XString l, r;
    if (str.endswith('/'))
        return false;
    if (extract(str, record<'/'>(&l, &r)))
    {
        // a.b.c.d/e.f.g.h or a.b.c.d/n
        if (!extract(l, &a))
            return false;
        if (extract(r, &m))
        {
            *rv = IP4Mask(a, m);
            return true;
        }
        if (!extract(r, &b) || b > 32)
            return false;
    }
    else
    {
        // a.b.c.d or a.b.c.d. or a.b.c. or a.b. or a.
        if (extract(str, &a))
        {
            *rv = IP4Mask(a, IP4_BROADCAST);
            return true;
        }
        if (!str.endswith('.'))
            return false;
        uint8_t d[4] {};
        if (extract(str, record<'.'>(&d[0], &d[1], &d[2], &d[3])))
            b = 32;
        else if (extract(str, record<'.'>(&d[0], &d[1], &d[2])))
            b = 24;
        else if (extract(str, record<'.'>(&d[0], &d[1])))
            b = 16;
        else if (extract(str, record<'.'>(&d[0])))
            b = 8;
        else
            return false;
        a = IP4Address(d);
    }
    // a is set; need to construct m from b
    if (b == 0)
        m = IP4Address();
    else if (b == 32)
        m = IP4_BROADCAST;
    else
    {
        uint32_t s = -1;
        s <<= (32 - b);
        m = IP4Address({
                static_cast<uint8_t>(s >> 24),
                static_cast<uint8_t>(s >> 16),
                static_cast<uint8_t>(s >> 8),
                static_cast<uint8_t>(s >> 0),
        });
    }
    *rv = IP4Mask(a, m);
    return true;
}

bool impl_extract(XString str, std::vector<IP4Mask> *iv)
{
    if (str == "all"_s)
    {
        iv->clear();
        iv->push_back(IP4Mask());
        return true;
    }
    if (str == "clear"_s)
    {
        iv->clear();
        return true;
    }
    // don't add if already 'all'
    if (iv->size() == 1 && iv->front().mask() == IP4Address())
    {
        return true;
    }
    IP4Mask mask;
    if (extract(str, &mask))
    {
        iv->push_back(mask);
        return true;
    }
    return false;
}

VString<15> convert_for_printf(IP4Address a_)
{
    const uint8_t *a = a_.bytes();
    return STRNPRINTF(16, "%hhu.%hhu.%hhu.%hhu"_fmt, a[0], a[1], a[2], a[3]);
}

VString<31> convert_for_printf(IP4Mask a)
{
    return STRNPRINTF(32, "%s/%s"_fmt,
            a.addr(), a.mask());
}
} // namespace tmwa
