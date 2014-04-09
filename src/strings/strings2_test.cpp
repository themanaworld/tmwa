#include "all.hpp"
//    strings2_test.cpp - Testsuite part 2 for strings.
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

#include "../poison.hpp"

TEST(StringTests, traits2)
{
    ZString print_non = "\t\e";
    ZString print_mix = "n\t";
    RString print_all = "n ";
    EXPECT_FALSE(print_non.has_print());
    EXPECT_TRUE(print_mix.has_print());
    EXPECT_TRUE(print_all.has_print());
    EXPECT_FALSE(print_non.is_print());
    EXPECT_FALSE(print_mix.is_print());
    EXPECT_TRUE(print_all.is_print());
    EXPECT_EQ("__", print_non.to_print());
    EXPECT_EQ("n_", print_mix.to_print());
    EXPECT_EQ("n ", print_all.to_print());
    EXPECT_EQ(print_all.begin(), print_all.to_print().begin());

    ZString graph_non = " \e";
    ZString graph_mix = "n ";
    RString graph_all = "n.";
    EXPECT_FALSE(graph_non.has_graph());
    EXPECT_TRUE(graph_mix.has_graph());
    EXPECT_TRUE(graph_all.has_graph());
    EXPECT_FALSE(graph_non.is_graph());
    EXPECT_FALSE(graph_mix.is_graph());
    EXPECT_TRUE(graph_all.is_graph());

    ZString lower_non = "0A";
    ZString lower_mix = "Oa";
    RString lower_all = "oa";
    EXPECT_FALSE(lower_non.has_lower());
    EXPECT_TRUE(lower_mix.has_lower());
    EXPECT_TRUE(lower_all.has_lower());
    EXPECT_FALSE(lower_non.is_lower());
    EXPECT_FALSE(lower_mix.is_lower());
    EXPECT_TRUE(lower_all.is_lower());
    EXPECT_EQ("0a", lower_non.to_lower());
    EXPECT_EQ("oa", lower_mix.to_lower());
    EXPECT_EQ("oa", lower_all.to_lower());
    EXPECT_EQ(lower_all.begin(), lower_all.to_lower().begin());

    ZString upper_non = "0a";
    ZString upper_mix = "oA";
    RString upper_all = "OA";
    EXPECT_FALSE(upper_non.has_upper());
    EXPECT_TRUE(upper_mix.has_upper());
    EXPECT_TRUE(upper_all.has_upper());
    EXPECT_FALSE(upper_non.is_upper());
    EXPECT_FALSE(upper_mix.is_upper());
    EXPECT_TRUE(upper_all.is_upper());
    EXPECT_EQ("0A", upper_non.to_upper());
    EXPECT_EQ("OA", upper_mix.to_upper());
    EXPECT_EQ("OA", upper_all.to_upper());
    EXPECT_EQ(upper_all.begin(), upper_all.to_upper().begin());

    ZString alpha_non = " 0";
    ZString alpha_mix = "n ";
    RString alpha_all = "nA";
    EXPECT_FALSE(alpha_non.has_alpha());
    EXPECT_TRUE(alpha_mix.has_alpha());
    EXPECT_TRUE(alpha_all.has_alpha());
    EXPECT_FALSE(alpha_non.is_alpha());
    EXPECT_FALSE(alpha_mix.is_alpha());
    EXPECT_TRUE(alpha_all.is_alpha());

    ZString digit2_non = "a9";
    ZString digit2_mix = "20";
    RString digit2_all = "01";
    EXPECT_FALSE(digit2_non.has_digit2());
    EXPECT_TRUE(digit2_mix.has_digit2());
    EXPECT_TRUE(digit2_all.has_digit2());
    EXPECT_FALSE(digit2_non.is_digit2());
    EXPECT_FALSE(digit2_mix.is_digit2());
    EXPECT_TRUE(digit2_all.is_digit2());

    ZString digit8_non = "a9";
    ZString digit8_mix = "80";
    RString digit8_all = "37";
    EXPECT_FALSE(digit8_non.has_digit8());
    EXPECT_TRUE(digit8_mix.has_digit8());
    EXPECT_TRUE(digit8_all.has_digit8());
    EXPECT_FALSE(digit8_non.is_digit8());
    EXPECT_FALSE(digit8_mix.is_digit8());
    EXPECT_TRUE(digit8_all.is_digit8());

    ZString digit10_non = "az";
    ZString digit10_mix = "a9";
    RString digit10_all = "42";
    EXPECT_FALSE(digit10_non.has_digit10());
    EXPECT_TRUE(digit10_mix.has_digit10());
    EXPECT_TRUE(digit10_all.has_digit10());
    EXPECT_FALSE(digit10_non.is_digit10());
    EXPECT_FALSE(digit10_mix.is_digit10());
    EXPECT_TRUE(digit10_all.is_digit10());

    ZString digit16_non = "gz";
    ZString digit16_mix = "ao";
    RString digit16_all = "be";
    EXPECT_FALSE(digit16_non.has_digit16());
    EXPECT_TRUE(digit16_mix.has_digit16());
    EXPECT_TRUE(digit16_all.has_digit16());
    EXPECT_FALSE(digit16_non.is_digit16());
    EXPECT_FALSE(digit16_mix.is_digit16());
    EXPECT_TRUE(digit16_all.is_digit16());

    ZString alnum_non = " .";
    ZString alnum_mix = "n ";
    RString alnum_all = "n0";
    EXPECT_FALSE(alnum_non.has_alnum());
    EXPECT_TRUE(alnum_mix.has_alnum());
    EXPECT_TRUE(alnum_all.has_alnum());
    EXPECT_FALSE(alnum_non.is_alnum());
    EXPECT_FALSE(alnum_mix.is_alnum());
    EXPECT_TRUE(alnum_all.is_alnum());
}

TEST(StringTests, rempty)
{
    const char empty_text[] = "";
    RString r = empty_text;
    EXPECT_EQ(r.size(), 0);
    AString a = empty_text;
    EXPECT_EQ(r, a);
    AString r2 = r, r3;
    RString a2 = a, a3;
    XString r1 = r2;
    XString a1 = a2;
    r3 = r1;
    a3 = a1;
    EXPECT_EQ(r, r1);
    EXPECT_EQ(a, a1);
    EXPECT_EQ(r, r2);
    EXPECT_EQ(a, a2);
    EXPECT_EQ(r, r3);
    EXPECT_EQ(a, a3);
    EXPECT_EQ(&*r.begin(), &*r1.begin());
    EXPECT_EQ(&*a.begin(), &*a1.begin());
    EXPECT_EQ(&*r.begin(), &*r2.begin());
    EXPECT_EQ(&*a.begin(), &*a2.begin());
    EXPECT_EQ(&*r.begin(), &*r3.begin());
    EXPECT_EQ(&*a.begin(), &*a3.begin());
}
TEST(StringTests, rshort)
{
    const char short_text[] = "0123456789";
    RString r = short_text;
    EXPECT_EQ(r.size(), 10);
    AString a = short_text;
    EXPECT_EQ(r, a);
    AString r2 = r, r3;
    RString a2 = a, a3;
    XString r1 = r2;
    XString a1 = a2;
    r3 = r1;
    a3 = a1;
    EXPECT_EQ(r, r1);
    EXPECT_EQ(a, a1);
    EXPECT_EQ(r, r2);
    EXPECT_EQ(a, a2);
    EXPECT_EQ(r, r3);
    EXPECT_EQ(a, a3);
    EXPECT_EQ(&*r.begin(), &*r1.begin());
    EXPECT_NE(&*a.begin(), &*a1.begin());
    EXPECT_EQ(&*r.begin(), &*r2.begin());
    EXPECT_NE(&*a.begin(), &*a2.begin());
    EXPECT_EQ(&*r.begin(), &*r3.begin());
    EXPECT_NE(&*a.begin(), &*a3.begin());
}

TEST(StringTests, rlong)
{
    const char long_text[] =
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345 100"
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345 200"
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345 300";
    RString r = long_text;
    EXPECT_EQ(r.size(), 300);
    AString a = long_text;
    EXPECT_EQ(r, a);
    AString r2 = r, r3;
    RString a2 = a, a3;
    XString r1 = r2;
    XString a1 = a2;
    r3 = r1;
    a3 = a1;
    EXPECT_EQ(r, r1);
    EXPECT_EQ(a, a1);
    EXPECT_EQ(r, r2);
    EXPECT_EQ(a, a2);
    EXPECT_EQ(r, r3);
    EXPECT_EQ(a, a3);
    EXPECT_EQ(&*r.begin(), &*r1.begin());
    EXPECT_EQ(&*a.begin(), &*a1.begin());
    EXPECT_EQ(&*r.begin(), &*r2.begin());
    EXPECT_EQ(&*a.begin(), &*a2.begin());
    EXPECT_EQ(&*r.begin(), &*r3.begin());
    EXPECT_EQ(&*a.begin(), &*a3.begin());
}
