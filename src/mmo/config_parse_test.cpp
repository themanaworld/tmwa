#include "config_parse.hpp"
//    config_parse_test.cpp - Testsuite for config parsers
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../strings/literal.hpp"
#include "../strings/rstring.hpp"

#include "../io/span.hpp"

#include "../poison.hpp"


namespace tmwa
{
#define EXPECT_SPAN(span, bl,bc, el,ec)     \
    ({                                      \
        EXPECT_EQ((span).begin.line, bl);   \
        EXPECT_EQ((span).begin.column, bc); \
        EXPECT_EQ((span).end.line, el);     \
        EXPECT_EQ((span).end.column, ec);   \
    })
TEST(configparse, keyvalue)
{
    //              0        1         2         3
    //              123456789012345678901234567890
    RString data = "  key     :      value        "_s;

    io::Spanned<ZString> input, value;
    io::Spanned<XString> key;
    input.data = data;
    input.span.begin.text = data;
    input.span.begin.filename = "<config parse key/value test>"_s;
    input.span.begin.line = 1;
    input.span.begin.column = 1;
    input.span.end = input.span.begin;
    input.span.end.column = data.size();
    EXPECT_EQ(data.size(), 30);
    ASSERT_TRUE(config_split(input, &key, &value));
    EXPECT_SPAN(key.span, 1,3, 1,5);
    EXPECT_SPAN(value.span, 1,18, 1,30);
}
} // namespace tmwa
