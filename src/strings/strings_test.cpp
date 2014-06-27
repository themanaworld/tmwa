//    strings_test.cpp - Testsuite part 1 for strings.
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

#include <algorithm>

#include <gtest/gtest.h>

#include "all.hpp"

#include "../poison.hpp"


namespace tmwa
{
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

template<typename T>
class StringTest : public ::testing::Test
{
};
TYPED_TEST_CASE_P(StringTest);

TYPED_TEST_P(StringTest, basic)
{
    TypeParam hi("Hello"_s);
    EXPECT_EQ(5, hi.size());
    EXPECT_EQ(hi, hi);
    LString hi2 = "Hello\0random garbage"_s;
    EXPECT_EQ(hi, hi2);
    TypeParam hi0;
    EXPECT_EQ(0, hi0.size());

    __attribute__((unused))
    const RString *base = hi.base();
}

TYPED_TEST_P(StringTest, order)
{
    TypeParam a;
    TypeParam b("Hello"_s);
    TypeParam c("Hello,"_s);
    TypeParam d("World!"_s);

    // not using EXPECT_LT, etc. for better visibility

    EXPECT_FALSE(a < a);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(a < d);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b < b);
    EXPECT_TRUE(b < c);
    EXPECT_TRUE(b < d);
    EXPECT_FALSE(c < a);
    EXPECT_FALSE(c < b);
    EXPECT_FALSE(c < c);
    EXPECT_TRUE(c < d);
    EXPECT_FALSE(d < a);
    EXPECT_FALSE(d < b);
    EXPECT_FALSE(d < c);
    EXPECT_FALSE(d < d);

    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a <= d);
    EXPECT_FALSE(b <= a);
    EXPECT_TRUE(b <= b);
    EXPECT_TRUE(b <= c);
    EXPECT_TRUE(b <= d);
    EXPECT_FALSE(c <= a);
    EXPECT_FALSE(c <= b);
    EXPECT_TRUE(c <= c);
    EXPECT_TRUE(c <= d);
    EXPECT_FALSE(d <= a);
    EXPECT_FALSE(d <= b);
    EXPECT_FALSE(d <= c);
    EXPECT_TRUE(d <= d);

    EXPECT_TRUE(a >= a);
    EXPECT_FALSE(a >= b);
    EXPECT_FALSE(a >= c);
    EXPECT_FALSE(a >= d);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b >= b);
    EXPECT_FALSE(b >= c);
    EXPECT_FALSE(b >= d);
    EXPECT_TRUE(c >= a);
    EXPECT_TRUE(c >= b);
    EXPECT_TRUE(c >= c);
    EXPECT_FALSE(c >= d);
    EXPECT_TRUE(d >= a);
    EXPECT_TRUE(d >= b);
    EXPECT_TRUE(d >= c);
    EXPECT_TRUE(d >= d);

    EXPECT_FALSE(a > a);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a > c);
    EXPECT_FALSE(a > d);
    EXPECT_TRUE(b > a);
    EXPECT_FALSE(b > b);
    EXPECT_FALSE(b > c);
    EXPECT_FALSE(b > d);
    EXPECT_TRUE(c > a);
    EXPECT_TRUE(c > b);
    EXPECT_FALSE(c > c);
    EXPECT_FALSE(c > d);
    EXPECT_TRUE(d > a);
    EXPECT_TRUE(d > b);
    EXPECT_TRUE(d > c);
    EXPECT_FALSE(d > d);

    EXPECT_TRUE(a == a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a == d);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b == b);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(b == d);
    EXPECT_FALSE(c == a);
    EXPECT_FALSE(c == b);
    EXPECT_TRUE(c == c);
    EXPECT_FALSE(c == d);
    EXPECT_FALSE(d == a);
    EXPECT_FALSE(d == b);
    EXPECT_FALSE(d == c);
    EXPECT_TRUE(d == d);

    EXPECT_FALSE(a != a);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(a != d);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b != b);
    EXPECT_TRUE(b != c);
    EXPECT_TRUE(b != d);
    EXPECT_TRUE(c != a);
    EXPECT_TRUE(c != b);
    EXPECT_FALSE(c != c);
    EXPECT_TRUE(c != d);
    EXPECT_TRUE(d != a);
    EXPECT_TRUE(d != b);
    EXPECT_TRUE(d != c);
    EXPECT_FALSE(d != d);
}

TYPED_TEST_P(StringTest, iterators)
{
    TypeParam hi("Hello"_s);
    EXPECT_EQ(hi.begin(), hi.begin());
    EXPECT_NE(hi.begin(), hi.end());
    EXPECT_EQ(5, std::distance(hi.begin(), hi.end()));
    const char *hi2 = "Hello";
    EXPECT_TRUE(std::equal(hi.begin(), hi.end(), hi2));
}

TYPED_TEST_P(StringTest, xslice)
{
    TypeParam hi("Hello, World!"_s);
    EXPECT_EQ(" World!"_s, hi.xslice_t(6));
    EXPECT_EQ("Hello,"_s, hi.xslice_h(6));
    EXPECT_EQ("World!"_s, hi.xrslice_t(6));
    EXPECT_EQ("Hello, "_s, hi.xrslice_h(6));
    typename TypeParam::iterator it = std::find(hi.begin(), hi.end(), ' ');
    EXPECT_EQ(" World!"_s, hi.xislice_t(it));
    EXPECT_EQ("Hello,"_s, hi.xislice_h(it));
    EXPECT_EQ("World"_s, hi.xlslice(7, 5));
    EXPECT_EQ("World"_s, hi.xpslice(7, 12));
    EXPECT_EQ("World"_s, hi.xislice(hi.begin() + 7, hi.begin() + 12));
    EXPECT_TRUE(hi.startswith("Hello"_s));
    EXPECT_TRUE(hi.endswith("World!"_s));
}

TYPED_TEST_P(StringTest, convert)
{
    constexpr bool is_zstring = std::is_same<TypeParam, ZString>::value;
    typedef typename std::conditional<is_zstring, TString, SString>::type Sstring;
    typedef typename std::conditional<is_zstring, ZString, XString>::type Xstring;
    RString r = "r"_s;
    AString a = "a"_s;
    TString t = "t"_s;
    Sstring s = "s"_s;
    ZString z = "z"_s;
    Xstring x = "x"_s;
    VString<255> v = "v"_s;
    LString l = "l"_s;
    VString<5> hi = "hello"_s;

    TypeParam r2 = r;
    TypeParam a2 = a;
    TypeParam t2 = t;
    TypeParam s2 = s;
    TypeParam z2 = z;
    TypeParam x2 = x;
    TypeParam v2 = v;
    TypeParam l2 = l;
    TypeParam hi2 = hi;

    EXPECT_EQ(r, r2);
    EXPECT_EQ(a, a2);
    EXPECT_EQ(t, t2);
    EXPECT_EQ(s, s2);
    EXPECT_EQ(z, z2);
    EXPECT_EQ(x, x2);
    EXPECT_EQ(v, v2);
    EXPECT_EQ(l, l2);
    EXPECT_EQ(hi, hi2);

    TypeParam r3, a3, t3, s3, z3, x3, v3, l3, hi3;
    r3 = r;
    a3 = a;
    t3 = t;
    s3 = s;
    z3 = z;
    x3 = x;
    v3 = v;
    l3 = l;
    hi3 = hi;

    EXPECT_EQ(r, r3);
    EXPECT_EQ(a, a3);
    EXPECT_EQ(t, t3);
    EXPECT_EQ(s, s3);
    EXPECT_EQ(z, z3);
    EXPECT_EQ(x, x3);
    EXPECT_EQ(v, v3);
    EXPECT_EQ(l, l3);
    EXPECT_EQ(hi, hi3);

    TypeParam r4(r);
    TypeParam a4(a);
    TypeParam t4(t);
    TypeParam s4(s);
    TypeParam z4(z);
    TypeParam x4(x);
    TypeParam v4(v);
    TypeParam l4(l);
    TypeParam hi4(hi);

    EXPECT_EQ(r, r4);
    EXPECT_EQ(a, a4);
    EXPECT_EQ(t, t4);
    EXPECT_EQ(s, s4);
    EXPECT_EQ(z, z4);
    EXPECT_EQ(x, x4);
    EXPECT_EQ(v, v4);
    EXPECT_EQ(l, l4);
    EXPECT_EQ(hi, hi4);
}

REGISTER_TYPED_TEST_CASE_P(StringTest,
        basic, order, iterators, xslice, convert);

typedef ::testing::Types<
    RString, AString, TString, SString, ZString, XString, VString<255>
> MostStringTypes;
INSTANTIATE_TYPED_TEST_CASE_P(StringStuff, StringTest, MostStringTypes);

TEST(VStringTest, basic)
{
    VString<5> hi = "Hello"_s;
    EXPECT_EQ(5, hi.size());
    EXPECT_EQ(hi, hi);
    // truncation
    VString<5> hi2(strings::really_construct_from_a_pointer, "Hello, world!");
    EXPECT_EQ(5, hi2.size());
    EXPECT_EQ(hi, hi2);
    // short
    hi = "hi"_s;
    EXPECT_EQ(2, hi.size());
    VString<5> hi0;
    EXPECT_EQ(0, hi0.size());
}

template<typename T>
class NulStringTest : public ::testing::Test
{
};
TYPED_TEST_CASE_P(NulStringTest);

TYPED_TEST_P(NulStringTest, basic)
{
    TypeParam hi("hello"_s);
    EXPECT_EQ(hi.size(), strlen(hi.c_str()));
    EXPECT_STREQ("hello", hi.c_str());
}

REGISTER_TYPED_TEST_CASE_P(NulStringTest,
        basic);

typedef ::testing::Types<
    RString, AString, TString, ZString, VString<255>
> NulStringTypes;
INSTANTIATE_TYPED_TEST_CASE_P(NulStringStuff, NulStringTest, NulStringTypes);
} // namespace tmwa
