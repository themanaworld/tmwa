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

#include "all.hpp"

#include "../poison.hpp"


namespace tmwa
{
TEST(StringTests, traits2)
{
    ZString print_non = "\t\e"_s;
    ZString print_mix = "n\t"_s;
    RString print_all = "n "_s;
    EXPECT_FALSE(print_non.has_print());
    EXPECT_TRUE(print_mix.has_print());
    EXPECT_TRUE(print_all.has_print());
    EXPECT_FALSE(print_non.is_print());
    EXPECT_FALSE(print_mix.is_print());
    EXPECT_TRUE(print_all.is_print());
    EXPECT_EQ("__"_s, print_non.to_print());
    EXPECT_EQ("n_"_s, print_mix.to_print());
    EXPECT_EQ("n "_s, print_all.to_print());
    EXPECT_EQ(print_all.begin(), print_all.to_print().begin());

    ZString graph_non = " \e"_s;
    ZString graph_mix = "n "_s;
    RString graph_all = "n."_s;
    EXPECT_FALSE(graph_non.has_graph());
    EXPECT_TRUE(graph_mix.has_graph());
    EXPECT_TRUE(graph_all.has_graph());
    EXPECT_FALSE(graph_non.is_graph());
    EXPECT_FALSE(graph_mix.is_graph());
    EXPECT_TRUE(graph_all.is_graph());

    ZString lower_non = "0A"_s;
    ZString lower_mix = "Oa"_s;
    RString lower_all = "oa"_s;
    EXPECT_FALSE(lower_non.has_lower());
    EXPECT_TRUE(lower_mix.has_lower());
    EXPECT_TRUE(lower_all.has_lower());
    EXPECT_FALSE(lower_non.is_lower());
    EXPECT_FALSE(lower_mix.is_lower());
    EXPECT_TRUE(lower_all.is_lower());
    EXPECT_EQ("0a"_s, lower_non.to_lower());
    EXPECT_EQ("oa"_s, lower_mix.to_lower());
    EXPECT_EQ("oa"_s, lower_all.to_lower());
    EXPECT_EQ(lower_all.begin(), lower_all.to_lower().begin());

    ZString upper_non = "0a"_s;
    ZString upper_mix = "oA"_s;
    RString upper_all = "OA"_s;
    EXPECT_FALSE(upper_non.has_upper());
    EXPECT_TRUE(upper_mix.has_upper());
    EXPECT_TRUE(upper_all.has_upper());
    EXPECT_FALSE(upper_non.is_upper());
    EXPECT_FALSE(upper_mix.is_upper());
    EXPECT_TRUE(upper_all.is_upper());
    EXPECT_EQ("0A"_s, upper_non.to_upper());
    EXPECT_EQ("OA"_s, upper_mix.to_upper());
    EXPECT_EQ("OA"_s, upper_all.to_upper());
    EXPECT_EQ(upper_all.begin(), upper_all.to_upper().begin());

    ZString alpha_non = " 0"_s;
    ZString alpha_mix = "n "_s;
    RString alpha_all = "nA"_s;
    EXPECT_FALSE(alpha_non.has_alpha());
    EXPECT_TRUE(alpha_mix.has_alpha());
    EXPECT_TRUE(alpha_all.has_alpha());
    EXPECT_FALSE(alpha_non.is_alpha());
    EXPECT_FALSE(alpha_mix.is_alpha());
    EXPECT_TRUE(alpha_all.is_alpha());

    ZString digit2_non = "a9"_s;
    ZString digit2_mix = "20"_s;
    RString digit2_all = "01"_s;
    EXPECT_FALSE(digit2_non.has_digit2());
    EXPECT_TRUE(digit2_mix.has_digit2());
    EXPECT_TRUE(digit2_all.has_digit2());
    EXPECT_FALSE(digit2_non.is_digit2());
    EXPECT_FALSE(digit2_mix.is_digit2());
    EXPECT_TRUE(digit2_all.is_digit2());

    ZString digit8_non = "a9"_s;
    ZString digit8_mix = "80"_s;
    RString digit8_all = "37"_s;
    EXPECT_FALSE(digit8_non.has_digit8());
    EXPECT_TRUE(digit8_mix.has_digit8());
    EXPECT_TRUE(digit8_all.has_digit8());
    EXPECT_FALSE(digit8_non.is_digit8());
    EXPECT_FALSE(digit8_mix.is_digit8());
    EXPECT_TRUE(digit8_all.is_digit8());

    ZString digit10_non = "az"_s;
    ZString digit10_mix = "a9"_s;
    RString digit10_all = "42"_s;
    EXPECT_FALSE(digit10_non.has_digit10());
    EXPECT_TRUE(digit10_mix.has_digit10());
    EXPECT_TRUE(digit10_all.has_digit10());
    EXPECT_FALSE(digit10_non.is_digit10());
    EXPECT_FALSE(digit10_mix.is_digit10());
    EXPECT_TRUE(digit10_all.is_digit10());

    ZString digit16_non = "gz"_s;
    ZString digit16_mix = "ao"_s;
    RString digit16_all = "be"_s;
    EXPECT_FALSE(digit16_non.has_digit16());
    EXPECT_TRUE(digit16_mix.has_digit16());
    EXPECT_TRUE(digit16_all.has_digit16());
    EXPECT_FALSE(digit16_non.is_digit16());
    EXPECT_FALSE(digit16_mix.is_digit16());
    EXPECT_TRUE(digit16_all.is_digit16());

    ZString alnum_non = " ."_s;
    ZString alnum_mix = "n "_s;
    RString alnum_all = "n0"_s;
    EXPECT_FALSE(alnum_non.has_alnum());
    EXPECT_TRUE(alnum_mix.has_alnum());
    EXPECT_TRUE(alnum_all.has_alnum());
    EXPECT_FALSE(alnum_non.is_alnum());
    EXPECT_FALSE(alnum_mix.is_alnum());
    EXPECT_TRUE(alnum_all.is_alnum());
}

TEST(StringTests, rempty)
{
    LString empty_text = ""_s;
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
    LString short_text = "0123456789"_s;
    EXPECT_EQ(&*short_text.begin(), &*RString(short_text).begin());
    EXPECT_EQ(&*short_text.begin(), &*AString(short_text).begin());
    RString r = VString<255>(short_text);
    EXPECT_EQ(r.size(), 10);
    AString a = VString<255>(short_text);
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
    LString long_text =
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345 100"
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345 200"
        "01234567890123456789012345678901234567890123456789"
        "0123456789012345678901234567890123456789012345 300"_s;
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
} // namespace tmwa
