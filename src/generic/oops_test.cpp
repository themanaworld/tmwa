#include "oops.hpp"
//    oops_test.cpp - Testsuite for stuff that shouldn't happen.
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

#include "../poison.hpp"


namespace tmwa
{
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

TEST(oops, okay)
{
    try
    {
        ALLEGE ("the sky is gray", true);
        SUCCEED();
    }
    catch (const AssertionError& e)
    {
        FAIL();
    }
}

TEST(oops, uhoh)
{
    try
    {
        ALLEGE ("the sky is falling", 1 == 0);
        FAIL();
    }
    catch (const AssertionError& e)
    {
        ASSERT_STREQ(strstr(e.what(), "src/generic/"),
                "src/generic/oops_test.cpp:47: error: in 'virtual void tmwa::oops_uhoh_Test::TestBody()', incorrectly alleged that 'the sky is falling' (1 == 0)");
    }
}
} // namespace tmwa
