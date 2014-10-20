#include "extract.hpp"
//    extract_test.cpp - Testsuite for a simple, hierarchical, tokenizer
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

#include "../strings/xstring.hpp"

#include "mmo.hpp"

#include "../poison.hpp"


namespace tmwa
{
TEST(extract, record_int)
{
    int x, y, z;
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(" "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(""_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;

    EXPECT_FALSE(extract("1 2 3 4 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(" "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(""_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;

    EXPECT_FALSE(extract("1 2 3 4 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(" "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(""_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
}

TEST(extract, record_str)
{
    XString x, y, z;
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1 2 3 4 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1 2 3 4"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 3 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 3"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1 2"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1 "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1"_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract(" "_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(""_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract(""_s, record<' '>(&x, &y, &z)));
    EXPECT_EQ(""_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;

    EXPECT_FALSE(extract("1 2 3 4 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1 2 3 4"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 3 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 3"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1"_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract(" "_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(""_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract(""_s, record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(""_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;

    EXPECT_FALSE(extract("1 2 3 4 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_FALSE(extract("1 2 3 4"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 3 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 3"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ("3"_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 2"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ("2"_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1 "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract("1"_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1"_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract(" "_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(""_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
    EXPECT_TRUE(extract(""_s, record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(""_s, x);
    EXPECT_EQ(""_s, y);
    EXPECT_EQ(""_s, z);
    x = y = z = ""_s;
}

TEST(extract, mapname)
{
    MapName map;
    EXPECT_TRUE(extract("abc"_s, &map));
    EXPECT_EQ(map, "abc"_s);
    EXPECT_TRUE(extract("abc.gat"_s, &map));
    EXPECT_EQ(map, "abc"_s);
    EXPECT_TRUE(extract("abcdefghijklmno"_s, &map));
    EXPECT_EQ(map, "abcdefghijklmno"_s);
    EXPECT_TRUE(extract("abcdefghijklmno.gat"_s, &map));
    EXPECT_EQ(map, "abcdefghijklmno"_s);
}

TEST(extract, chrono)
{
    std::chrono::nanoseconds ns;
    std::chrono::microseconds us;
    std::chrono::milliseconds ms;
    std::chrono::seconds s;
    std::chrono::minutes min;
    std::chrono::hours h;
    std::chrono::duration<int, std::ratio<60*60*24>> d;

    EXPECT_TRUE(extract("1"_s, &ns));
    EXPECT_EQ(ns, 1_ns);
    EXPECT_TRUE(extract("3ns"_s, &ns));
    EXPECT_EQ(ns, 3_ns);
    EXPECT_TRUE(extract("4us"_s, &ns));
    EXPECT_EQ(ns, 4_us);
    EXPECT_TRUE(extract("5ms"_s, &ns));
    EXPECT_EQ(ns, 5_ms);
    EXPECT_TRUE(extract("6s"_s, &ns));
    EXPECT_EQ(ns, 6_s);
    EXPECT_TRUE(extract("7min"_s, &ns));
    EXPECT_EQ(ns, 7_min);
    EXPECT_TRUE(extract("8h"_s, &ns));
    EXPECT_EQ(ns, 8_h);
    EXPECT_TRUE(extract("9d"_s, &ns));
    EXPECT_EQ(ns, 9_d);

    EXPECT_TRUE(extract("1"_s, &us));
    EXPECT_EQ(us, 1_us);
    EXPECT_TRUE(extract("4us"_s, &us));
    EXPECT_EQ(us, 4_us);
    EXPECT_TRUE(extract("5ms"_s, &us));
    EXPECT_EQ(us, 5_ms);
    EXPECT_TRUE(extract("6s"_s, &us));
    EXPECT_EQ(us, 6_s);
    EXPECT_TRUE(extract("7min"_s, &us));
    EXPECT_EQ(us, 7_min);
    EXPECT_TRUE(extract("8h"_s, &us));
    EXPECT_EQ(us, 8_h);
    EXPECT_TRUE(extract("9d"_s, &us));
    EXPECT_EQ(us, 9_d);

    EXPECT_TRUE(extract("1"_s, &ms));
    EXPECT_EQ(ms, 1_ms);
    EXPECT_TRUE(extract("5ms"_s, &ms));
    EXPECT_EQ(ms, 5_ms);
    EXPECT_TRUE(extract("6s"_s, &ms));
    EXPECT_EQ(ms, 6_s);
    EXPECT_TRUE(extract("7min"_s, &ms));
    EXPECT_EQ(ms, 7_min);
    EXPECT_TRUE(extract("8h"_s, &ms));
    EXPECT_EQ(ms, 8_h);
    EXPECT_TRUE(extract("9d"_s, &ms));
    EXPECT_EQ(ms, 9_d);

    EXPECT_TRUE(extract("1"_s, &s));
    EXPECT_EQ(s, 1_s);
    EXPECT_TRUE(extract("6s"_s, &s));
    EXPECT_EQ(s, 6_s);
    EXPECT_TRUE(extract("7min"_s, &s));
    EXPECT_EQ(s, 7_min);
    EXPECT_TRUE(extract("8h"_s, &s));
    EXPECT_EQ(s, 8_h);
    EXPECT_TRUE(extract("9d"_s, &s));
    EXPECT_EQ(s, 9_d);

    EXPECT_TRUE(extract("1"_s, &min));
    EXPECT_EQ(min, 1_min);
    EXPECT_TRUE(extract("7min"_s, &min));
    EXPECT_EQ(min, 7_min);
    EXPECT_TRUE(extract("8h"_s, &min));
    EXPECT_EQ(min, 8_h);
    EXPECT_TRUE(extract("9d"_s, &min));
    EXPECT_EQ(min, 9_d);

    EXPECT_TRUE(extract("1"_s, &h));
    EXPECT_EQ(h, 1_h);
    EXPECT_TRUE(extract("8h"_s, &h));
    EXPECT_EQ(h, 8_h);
    EXPECT_TRUE(extract("9d"_s, &h));
    EXPECT_EQ(h, 9_d);

    EXPECT_TRUE(extract("1"_s, &d));
    EXPECT_EQ(d, 1_d);
    EXPECT_TRUE(extract("9d"_s, &d));
    EXPECT_EQ(d, 9_d);
}
} // namespace tmwa
