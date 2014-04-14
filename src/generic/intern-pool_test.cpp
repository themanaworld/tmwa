#include "intern-pool.hpp"
//    intern-pool.hpp - Testsuite for cached integer/string lookups.
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

#include "../strings/base.hpp"

#include "../poison.hpp"

TEST(InternPool, whydoesthisalwaysneedasecondname)
{
    InternPool p;
    EXPECT_EQ(0, p.size());
    EXPECT_EQ(0, p.intern("hello"_s));
    EXPECT_EQ(0, p.intern("hello"_s));
    EXPECT_EQ(1, p.size());
    EXPECT_EQ(1, p.intern("world"_s));
    EXPECT_EQ(0, p.intern("hello"_s));
    EXPECT_EQ(1, p.intern("world"_s));
    EXPECT_EQ(2, p.size());
    EXPECT_EQ("hello"_s, p.outtern(0));
    EXPECT_EQ("world"_s, p.outtern(1));
}
