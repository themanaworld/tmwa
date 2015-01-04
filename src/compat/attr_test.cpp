#include "attr.hpp"
//    attr_test.cpp - Tests for attributes
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
TEST(attr, withvar)
{
    int x = 41;
    for (int i = 0; i < 2; ++i)
    {
        WITH_VAR_NOLOOP(auto, y, x + 1)
        {
            x = y;
        }
    }
    EXPECT_EQ(x, 43);
    for (int i = 0; i < 2; ++i)
    {
        if (i == 1)
            WITH_VAR_INLOOP(auto, y, i)
            {
                x = y;
                break;
            }
    }
    EXPECT_EQ(x, 1);
}
} // namespace tmwa
