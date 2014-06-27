#include "ip.hpp"
//    ip_test.cpp - Testsuite for implementation of IP address functions.
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

#include <gtest/gtest.h>

#include "../strings/vstring.hpp"
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"

#include "../poison.hpp"


namespace tmwa
{
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

#define CB(X) (std::integral_constant<bool, (X)>::value)
TEST(ip4addr, cmp)
{
    constexpr static
    IP4Address a = IP4_LOCALHOST;
    constexpr static
    IP4Address b = IP4_BROADCAST;

    EXPECT_FALSE(CB(a < a));
    EXPECT_TRUE (CB(a < b));
    EXPECT_FALSE(CB(b < a));
    EXPECT_FALSE(CB(b < b));

    EXPECT_FALSE(CB(a > a));
    EXPECT_FALSE(CB(a > b));
    EXPECT_TRUE (CB(b > a));
    EXPECT_FALSE(CB(b > b));

    EXPECT_TRUE (CB(a <= a));
    EXPECT_TRUE (CB(a <= b));
    EXPECT_FALSE(CB(b <= a));
    EXPECT_TRUE (CB(b <= b));

    EXPECT_TRUE (CB(a >= a));
    EXPECT_FALSE(CB(a >= b));
    EXPECT_TRUE (CB(b >= a));
    EXPECT_TRUE (CB(b >= b));

    EXPECT_TRUE (CB(a == a));
    EXPECT_FALSE(CB(a == b));
    EXPECT_FALSE(CB(b == a));
    EXPECT_TRUE (CB(b == b));

    EXPECT_FALSE(CB(a != a));
    EXPECT_TRUE (CB(a != b));
    EXPECT_TRUE (CB(b != a));
    EXPECT_FALSE(CB(b != b));
}

TEST(ip4addr, str)
{
    IP4Address a;
    EXPECT_EQ("0.0.0.0"_s, STRNPRINTF(17, "%s"_fmt, a));
    EXPECT_EQ("127.0.0.1"_s, STRNPRINTF(17, "%s"_fmt, IP4_LOCALHOST));
    EXPECT_EQ("255.255.255.255"_s, STRNPRINTF(17, "%s"_fmt, IP4_BROADCAST));
}

TEST(ip4addr, extract)
{
    IP4Address a;
    EXPECT_TRUE(extract("0.0.0.0"_s, &a));
    EXPECT_EQ("0.0.0.0"_s, STRNPRINTF(16, "%s"_fmt, a));
    EXPECT_TRUE(extract("127.0.0.1"_s, &a));
    EXPECT_EQ("127.0.0.1"_s, STRNPRINTF(16, "%s"_fmt, a));
    EXPECT_TRUE(extract("255.255.255.255"_s, &a));
    EXPECT_EQ("255.255.255.255"_s, STRNPRINTF(16, "%s"_fmt, a));
    EXPECT_TRUE(extract("1.2.3.4"_s, &a));
    EXPECT_EQ("1.2.3.4"_s, STRNPRINTF(16, "%s"_fmt, a));

    EXPECT_FALSE(extract("1.2.3.4.5"_s, &a));
    EXPECT_FALSE(extract("1.2.3.4."_s, &a));
    EXPECT_FALSE(extract("1.2.3."_s, &a));
    EXPECT_FALSE(extract("1.2.3"_s, &a));
    EXPECT_FALSE(extract("1.2."_s, &a));
    EXPECT_FALSE(extract("1.2"_s, &a));
    EXPECT_FALSE(extract("1."_s, &a));
    EXPECT_FALSE(extract("1"_s, &a));
    EXPECT_FALSE(extract(""_s, &a));
}


TEST(ip4mask, body)
{
    IP4Mask m;
    EXPECT_EQ(IP4Address(), m.addr());
    EXPECT_EQ(IP4Address(), m.mask());
    m = IP4Mask(IP4_LOCALHOST, IP4_BROADCAST);
    EXPECT_EQ(IP4_LOCALHOST, m.addr());
    EXPECT_EQ(IP4_BROADCAST, m.mask());
}

TEST(ip4mask, str)
{
    IP4Mask m;
    EXPECT_EQ("0.0.0.0/0.0.0.0"_s, STRNPRINTF(33, "%s"_fmt, m));
    m = IP4Mask(IP4_LOCALHOST, IP4_BROADCAST);
    EXPECT_EQ("127.0.0.1/255.255.255.255"_s, STRNPRINTF(33, "%s"_fmt, m));
}

TEST(ip4mask, extract)
{
    IP4Mask m;
    EXPECT_FALSE(extract("9.8.7.6/33"_s, &m));
    EXPECT_FALSE(extract("9.8.7.6.5"_s, &m));
    EXPECT_FALSE(extract("9.8.7.6/"_s, &m));
    EXPECT_FALSE(extract("9.8.7"_s, &m));
    EXPECT_FALSE(extract("9.8"_s, &m));
    EXPECT_FALSE(extract("9"_s, &m));

    EXPECT_TRUE(extract("127.0.0.1"_s, &m));
    EXPECT_EQ("127.0.0.1/255.255.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("127.0.0.1."_s, &m));
    EXPECT_EQ("127.0.0.1/255.255.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("127.0.0."_s, &m));
    EXPECT_EQ("127.0.0.0/255.255.255.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("127.0."_s, &m));
    EXPECT_EQ("127.0.0.0/255.255.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("127."_s, &m));
    EXPECT_EQ("127.0.0.0/255.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));

    EXPECT_TRUE(extract("1.2.3.4/255.255.255.255"_s, &m));
    EXPECT_EQ("1.2.3.4/255.255.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.2.3.0/255.255.255.0"_s, &m));
    EXPECT_EQ("1.2.3.0/255.255.255.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.2.0.4/255.255.0.255"_s, &m));
    EXPECT_EQ("1.2.0.4/255.255.0.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.2.0.0/255.255.0.0"_s, &m));
    EXPECT_EQ("1.2.0.0/255.255.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.0.3.4/255.0.255.255"_s, &m));
    EXPECT_EQ("1.0.3.4/255.0.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.0.3.0/255.0.255.0"_s, &m));
    EXPECT_EQ("1.0.3.0/255.0.255.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.0.0.4/255.0.0.255"_s, &m));
    EXPECT_EQ("1.0.0.4/255.0.0.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("1.0.0.0/255.0.0.0"_s, &m));
    EXPECT_EQ("1.0.0.0/255.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.2.3.4/0.255.255.255"_s, &m));
    EXPECT_EQ("0.2.3.4/0.255.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.2.3.0/0.255.255.0"_s, &m));
    EXPECT_EQ("0.2.3.0/0.255.255.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.2.0.4/0.255.0.255"_s, &m));
    EXPECT_EQ("0.2.0.4/0.255.0.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.2.0.0/0.255.0.0"_s, &m));
    EXPECT_EQ("0.2.0.0/0.255.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.3.4/0.0.255.255"_s, &m));
    EXPECT_EQ("0.0.3.4/0.0.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.3.0/0.0.255.0"_s, &m));
    EXPECT_EQ("0.0.3.0/0.0.255.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.4/0.0.0.255"_s, &m));
    EXPECT_EQ("0.0.0.4/0.0.0.255"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/0.0.0.0"_s, &m));
    EXPECT_EQ("0.0.0.0/0.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));

    // please don't do this
    EXPECT_TRUE(extract("120.248.200.217/89.57.126.5"_s, &m));
    EXPECT_EQ("88.56.72.1/89.57.126.5"_s, STRNPRINTF(32, "%s"_fmt, m));

    EXPECT_TRUE(extract("0.0.0.0/32"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.255"_s, STRNPRINTF(32, "%s"_fmt, m));

    EXPECT_TRUE(extract("0.0.0.0/31"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.254"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/30"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.252"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/29"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.248"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/28"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.240"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/27"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.224"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/26"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.192"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/25"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.128"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/24"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.255.0"_s, STRNPRINTF(32, "%s"_fmt, m));

    EXPECT_TRUE(extract("0.0.0.0/23"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.254.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/22"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.252.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/21"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.248.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/20"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.240.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/19"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.224.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/18"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.192.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/17"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.128.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/16"_s, &m));
    EXPECT_EQ("0.0.0.0/255.255.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));

    EXPECT_TRUE(extract("0.0.0.0/15"_s, &m));
    EXPECT_EQ("0.0.0.0/255.254.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/14"_s, &m));
    EXPECT_EQ("0.0.0.0/255.252.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/13"_s, &m));
    EXPECT_EQ("0.0.0.0/255.248.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/12"_s, &m));
    EXPECT_EQ("0.0.0.0/255.240.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/11"_s, &m));
    EXPECT_EQ("0.0.0.0/255.224.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/10"_s, &m));
    EXPECT_EQ("0.0.0.0/255.192.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/9"_s, &m));
    EXPECT_EQ("0.0.0.0/255.128.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/8"_s, &m));
    EXPECT_EQ("0.0.0.0/255.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));

    EXPECT_TRUE(extract("0.0.0.0/7"_s, &m));
    EXPECT_EQ("0.0.0.0/254.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/6"_s, &m));
    EXPECT_EQ("0.0.0.0/252.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/5"_s, &m));
    EXPECT_EQ("0.0.0.0/248.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/4"_s, &m));
    EXPECT_EQ("0.0.0.0/240.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/3"_s, &m));
    EXPECT_EQ("0.0.0.0/224.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/2"_s, &m));
    EXPECT_EQ("0.0.0.0/192.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/1"_s, &m));
    EXPECT_EQ("0.0.0.0/128.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
    EXPECT_TRUE(extract("0.0.0.0/0"_s, &m));
    EXPECT_EQ("0.0.0.0/0.0.0.0"_s, STRNPRINTF(32, "%s"_fmt, m));
}

TEST(ip4mask, cover)
{
    IP4Address a;
    IP4Address b = IP4_BROADCAST;
    IP4Address l = IP4_LOCALHOST;
    IP4Address h({127, 255, 255, 255});
    IP4Address p24l({10, 0, 0, 0});
    IP4Address p24h({10, 255, 255, 255});
    IP4Address p20l({172, 16, 0, 0});
    IP4Address p20h({172, 31, 255, 255});
    IP4Address p16l({192, 168, 0, 0});
    IP4Address p16h({192, 168, 255, 255});
    IP4Mask m;
    EXPECT_TRUE(m.covers(a));
    EXPECT_TRUE(m.covers(b));
    EXPECT_TRUE(m.covers(l));
    EXPECT_TRUE(m.covers(h));
    EXPECT_TRUE(m.covers(p24l));
    EXPECT_TRUE(m.covers(p24h));
    EXPECT_TRUE(m.covers(p20l));
    EXPECT_TRUE(m.covers(p20h));
    EXPECT_TRUE(m.covers(p16l));
    EXPECT_TRUE(m.covers(p16h));
    m = IP4Mask(l, a);
    EXPECT_TRUE(m.covers(a));
    EXPECT_TRUE(m.covers(b));
    EXPECT_TRUE(m.covers(l));
    EXPECT_TRUE(m.covers(h));
    EXPECT_TRUE(m.covers(p24l));
    EXPECT_TRUE(m.covers(p24h));
    EXPECT_TRUE(m.covers(p20l));
    EXPECT_TRUE(m.covers(p20h));
    EXPECT_TRUE(m.covers(p16l));
    EXPECT_TRUE(m.covers(p16h));
    m = IP4Mask(l, b);
    EXPECT_FALSE(m.covers(a));
    EXPECT_FALSE(m.covers(b));
    EXPECT_TRUE(m.covers(l));
    EXPECT_FALSE(m.covers(h));
    EXPECT_FALSE(m.covers(p24l));
    EXPECT_FALSE(m.covers(p24h));
    EXPECT_FALSE(m.covers(p20l));
    EXPECT_FALSE(m.covers(p20h));
    EXPECT_FALSE(m.covers(p16l));
    EXPECT_FALSE(m.covers(p16h));

    // but the really useful ones are with partial masks
    m = IP4Mask(IP4Address({10, 0, 0, 0}), IP4Address({255, 0, 0, 0}));
    EXPECT_FALSE(m.covers(a));
    EXPECT_FALSE(m.covers(b));
    EXPECT_FALSE(m.covers(l));
    EXPECT_FALSE(m.covers(h));
    EXPECT_TRUE(m.covers(p24l));
    EXPECT_TRUE(m.covers(p24h));
    EXPECT_FALSE(m.covers(p20l));
    EXPECT_FALSE(m.covers(p20h));
    EXPECT_FALSE(m.covers(p16l));
    EXPECT_FALSE(m.covers(p16h));
    EXPECT_FALSE(m.covers(IP4Address({9, 255, 255, 255})));
    EXPECT_FALSE(m.covers(IP4Address({11, 0, 0, 0})));
    m = IP4Mask(IP4Address({127, 0, 0, 0}), IP4Address({255, 0, 0, 0}));
    EXPECT_FALSE(m.covers(a));
    EXPECT_FALSE(m.covers(b));
    EXPECT_TRUE(m.covers(l));
    EXPECT_TRUE(m.covers(h));
    EXPECT_FALSE(m.covers(p24l));
    EXPECT_FALSE(m.covers(p24h));
    EXPECT_FALSE(m.covers(p20l));
    EXPECT_FALSE(m.covers(p20h));
    EXPECT_FALSE(m.covers(p16l));
    EXPECT_FALSE(m.covers(p16h));
    EXPECT_FALSE(m.covers(IP4Address({126, 255, 255, 255})));
    EXPECT_FALSE(m.covers(IP4Address({128, 0, 0, 0})));
    m = IP4Mask(IP4Address({172, 16, 0, 0}), IP4Address({255, 240, 0, 0}));
    EXPECT_FALSE(m.covers(a));
    EXPECT_FALSE(m.covers(b));
    EXPECT_FALSE(m.covers(l));
    EXPECT_FALSE(m.covers(h));
    EXPECT_FALSE(m.covers(p24l));
    EXPECT_FALSE(m.covers(p24h));
    EXPECT_TRUE(m.covers(p20l));
    EXPECT_TRUE(m.covers(p20h));
    EXPECT_FALSE(m.covers(p16l));
    EXPECT_FALSE(m.covers(p16h));
    EXPECT_FALSE(m.covers(IP4Address({172, 15, 255, 255})));
    EXPECT_FALSE(m.covers(IP4Address({172, 32, 0, 0})));
    m = IP4Mask(IP4Address({192, 168, 0, 0}), IP4Address({255, 255, 0, 0}));
    EXPECT_FALSE(m.covers(a));
    EXPECT_FALSE(m.covers(b));
    EXPECT_FALSE(m.covers(l));
    EXPECT_FALSE(m.covers(h));
    EXPECT_FALSE(m.covers(p24l));
    EXPECT_FALSE(m.covers(p24h));
    EXPECT_FALSE(m.covers(p20l));
    EXPECT_FALSE(m.covers(p20h));
    EXPECT_TRUE(m.covers(p16l));
    EXPECT_TRUE(m.covers(p16h));
    EXPECT_FALSE(m.covers(IP4Address({192, 167, 255, 255})));
    EXPECT_FALSE(m.covers(IP4Address({192, 169, 0, 0})));

    // OTOH this is crazy
    EXPECT_TRUE(extract("120.248.200.217/89.57.126.5"_s, &m));
    EXPECT_TRUE(m.covers(IP4Address({120, 248, 200, 217})));
    EXPECT_TRUE(m.covers(IP4Address({88, 56, 72, 1})));
    EXPECT_FALSE(m.covers(IP4Address({88, 56, 72, 0})));
    EXPECT_FALSE(m.covers(IP4Address({88, 56, 72, 255})));
}
} // namespace tmwa
