#include "result.hpp"
//    result_test.cpp - Testsuite for possibly failing return values
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

//#include "../poison.hpp"


namespace tmwa
{
TEST(Result, inspect)
{
    struct Foo
    {
        int val;

        Foo(int v) : val(v) {}
        Foo(Foo&&) = default;
        Foo(const Foo&) = delete;
        Foo& operator = (Foo&&) = default;
        Foo& operator = (const Foo&) = delete;

        bool operator == (const Foo& other) const
        {
            return this->val == other.val;
        }
    };

    Result<Foo> foo = Ok(Foo(1));
    EXPECT_TRUE(foo.is_ok());
    EXPECT_FALSE(foo.is_err());
    EXPECT_EQ(foo.get_success(), Some(Foo(1)));
    EXPECT_EQ(foo.get_failure(), ""_s);
    foo = Err("oops"_s);
    EXPECT_FALSE(foo.is_ok());
    EXPECT_TRUE(foo.is_err());
    EXPECT_EQ(foo.get_success(), None<Foo>());
    EXPECT_EQ(foo.get_failure(), "oops"_s);
}

static
Result<int> try_you(bool b)
{
    return b ? Ok(0) : Err("die"_s);
}

static
Result<int> try_me(bool b)
{
    return Ok(TRY(try_you(b)) + 1);
}

TEST(Result, try)
{
    Result<int> t = try_me(true);
    EXPECT_EQ(t.get_success(), Some(1));
    Result<int> f = try_me(false);
    EXPECT_EQ(f.get_failure(), "die"_s);
}
} // namespace tmwa
