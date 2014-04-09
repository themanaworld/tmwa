#include "human_time_diff.hpp"
//    human_time_diff.hpp - Testwuite for broken deltas
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

// a sequence of [-+]?[0-9]+([ay]|m|[jd]|h|mn|s)

TEST(humantimediff, single)
{
    HumanTimeDiff diff;

    EXPECT_TRUE(extract("42y", &diff));
    EXPECT_EQ(42, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42m", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(42, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42d", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(42, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42h", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(42, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42mn", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(42, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42s", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(42, diff.second);

    EXPECT_TRUE(extract("+42y", &diff));
    EXPECT_EQ(42, diff.year);
    EXPECT_TRUE(extract("-42y", &diff));
    EXPECT_EQ(-42, diff.year);
    EXPECT_FALSE(extract("++42y", &diff));
    EXPECT_FALSE(extract("+-42y", &diff));
    EXPECT_FALSE(extract("-+42y", &diff));
    EXPECT_FALSE(extract("--42y", &diff));
    EXPECT_FALSE(extract("4+2y", &diff));
    EXPECT_FALSE(extract("42z", &diff));
}

TEST(humantimediff, multiple)
{
    HumanTimeDiff diff;

    EXPECT_TRUE(extract("42y23m-2d", &diff));
    EXPECT_EQ(42, diff.year);
    EXPECT_EQ(23, diff.month);
    EXPECT_EQ(-2, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);
    EXPECT_FALSE(extract("1y2y", &diff));
}
