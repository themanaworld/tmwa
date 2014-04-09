#include "slice.hpp"
//    slice_test.cpp - Testsuite for a borrowed array
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

TEST(slice, slice)
{
    int init[] = {1, 2, 3, 4, 5};

    Slice<int> slice(std::begin(init), std::end(init));
    EXPECT_EQ(slice.data(), init);
    EXPECT_EQ(slice.size(), 5);

    Slice<int> head = slice.slice_h(2);
    Slice<int> tail = slice.slice_t(2);
    EXPECT_EQ(head.size(), 2);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(head.front(), 1);
    EXPECT_EQ(head.back(), 2);
    EXPECT_EQ(tail.front(), 3);
    EXPECT_EQ(tail.back(), 5);

    head = slice.rslice_h(3);
    tail = slice.rslice_t(3);
    EXPECT_EQ(head.size(), 2);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(head.front(), 1);
    EXPECT_EQ(head.back(), 2);
    EXPECT_EQ(tail.front(), 3);
    EXPECT_EQ(tail.back(), 5);

    head = slice.islice_h(slice.begin() + 2);
    tail = slice.islice_t(slice.end() - 3);
    EXPECT_EQ(head.size(), 2);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(head.front(), 1);
    EXPECT_EQ(head.back(), 2);
    EXPECT_EQ(tail.front(), 3);
    EXPECT_EQ(tail.back(), 5);

    tail = slice.lslice(1, 3);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(tail.front(), 2);
    EXPECT_EQ(tail.back(), 4);

    tail = slice.pslice(1, 4);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(tail.front(), 2);
    EXPECT_EQ(tail.back(), 4);

    tail = slice.islice(slice.begin() + 1, slice.end() - 1);
    EXPECT_EQ(tail.size(), 3);
    EXPECT_EQ(tail.front(), 2);
    EXPECT_EQ(tail.back(), 4);

    head = slice;
    while (head)
    {
        size_t headsize = head.size();
        EXPECT_EQ(head.back(), headsize);
        EXPECT_EQ(head.pop_back(), headsize);
    }

    tail = slice;
    while (!!tail)
    {
        size_t tailsize = tail.size();
        EXPECT_EQ(tail.front(), 6 - tailsize);
        EXPECT_EQ(tail.pop_front(), 6 - tailsize);
    }
}

TEST(slice, cast)
{
    struct Foo
    {
        int x;
    };
    struct Bar : Foo
    {
    };

    Bar bars[2] = {Bar(), Bar()};

    Slice<Bar> slice(bars, 2);
    Slice<Foo> foos(slice);

    EXPECT_EQ(foos.size(), slice.size());
    EXPECT_EQ(&foos.end()[-1], &slice.end()[-1]);
}
