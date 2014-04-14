#include "base.hpp"
//    strings/base_test.cpp - Testsuite for CRTP base for string implementations.
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

#include "vstring.hpp"
#include "xstring.hpp"
#include "rstring.hpp"

#include "../poison.hpp"

using namespace strings;

struct _test : VString<1> {};
struct _test2 : VString<1> {};

static_assert(string_comparison_allowed<_test, _test>::value, "tt");
static_assert(string_comparison_allowed<VString<1>, VString<1>>::value, "vv");
static_assert(!string_comparison_allowed<_test, XString>::value, "tx");
static_assert(!string_comparison_allowed<_test, VString<1>>::value, "tv");
static_assert(!string_comparison_allowed<_test, _test2>::value, "t2");
static_assert(string_comparison_allowed<VString<1>, XString>::value, "vx");
static_assert(string_comparison_allowed<XString, XString>::value, "xx");
static_assert(string_comparison_allowed<XString, RString>::value, "xf");

TEST(strings, contains)
{
    XString hi = "Hello"_s;
    EXPECT_TRUE(hi.contains_any("Hi"_s));
    EXPECT_FALSE(hi.contains_any("hi"_s));
}
