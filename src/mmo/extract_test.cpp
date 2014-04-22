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
