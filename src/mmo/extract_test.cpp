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

#include "../poison.hpp"

TEST(extract, record_int)
{
    int x, y, z;
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1", record<' '>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(" ", record<' '>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("", record<' '>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;

    EXPECT_FALSE(extract("1 2 3 4 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(" ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;

    EXPECT_FALSE(extract("1 2 3 4 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("1 2 3 4", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 3", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(3, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 2", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(2, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_TRUE(extract("1", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(1, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract(" ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
    EXPECT_FALSE(extract("", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    EXPECT_EQ(0, z);
    x = y = z = 0;
}

TEST(extract, record_str)
{
    XString x, y, z;
    x = y = z = "";
    EXPECT_FALSE(extract("1 2 3 4 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1 2 3 4", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 3 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 3", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1 2", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1 ", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1", record<' '>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract(" ", record<' '>(&x, &y, &z)));
    EXPECT_EQ("", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract("", record<' '>(&x, &y, &z)));
    EXPECT_EQ("", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";

    EXPECT_FALSE(extract("1 2 3 4 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1 2 3 4", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 3 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 3", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract(" ", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_FALSE(extract("", record<' ', 2>(&x, &y, &z)));
    EXPECT_EQ("", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";

    EXPECT_FALSE(extract("1 2 3 4 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_FALSE(extract("1 2 3 4", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 3 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 3", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("3", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 2", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("2", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1 ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract("1", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("1", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract(" ", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
    EXPECT_TRUE(extract("", record<' ', 1>(&x, &y, &z)));
    EXPECT_EQ("", x);
    EXPECT_EQ("", y);
    EXPECT_EQ("", z);
    x = y = z = "";
}

TEST(extract, mapname)
{
    MapName map;
    EXPECT_TRUE(extract("abc", &map));
    EXPECT_EQ(map, "abc");
    EXPECT_TRUE(extract("abc.gat", &map));
    EXPECT_EQ(map, "abc");
    EXPECT_TRUE(extract("abcdefghijklmno", &map));
    EXPECT_EQ(map, "abcdefghijklmno");
    EXPECT_TRUE(extract("abcdefghijklmno.gat", &map));
    EXPECT_EQ(map, "abcdefghijklmno");
}
