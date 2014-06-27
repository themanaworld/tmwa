#include "array.hpp"
//    array_test.cpp - Testsuite for a simple bounds-checked array.
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

TEST(Array, simple)
{
    GenericArray<int, SimpleIndexing<3>> a;
    try
    {
        a[0];
        a[1];
        a[2];
        SUCCEED();
    }
    catch (const AssertionError&)
    {
        FAIL();
    }
    try
    {
        a[3];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
}

TEST(Array, inclusive1)
{
    GenericArray<int, InclusiveIndexing<int, 1, 3>> a;
    try
    {
        a[0];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
    try
    {
        a[1];
        a[2];
        a[3];
        SUCCEED();
    }
    catch (const AssertionError&)
    {
        FAIL();
    }
    try
    {
        a[4];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
}

TEST(Array, negative)
{
    GenericArray<int, InclusiveIndexing<int, -1, 1>> a;
    try
    {
        a[-2];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
    try
    {
        a[-1];
        a[0];
        a[1];
        SUCCEED();
    }
    catch (const AssertionError&)
    {
        FAIL();
    }
    try
    {
        a[2];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
}

TEST(Array, enum)
{
    enum class Blah
    {
        FOO,
        BAR,
        BAZ,
        COUNT,
    };

    GenericArray<int, EnumIndexing<Blah>> a;
    try
    {
        a[static_cast<Blah>(-1)];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
    try
    {
        a[Blah::FOO];
        a[Blah::BAR];
        a[Blah::BAZ];
        SUCCEED();
    }
    catch (const AssertionError&)
    {
        FAIL();
    }
    try
    {
        a[Blah::COUNT];
        FAIL();
    }
    catch (const AssertionError&)
    {
        SUCCEED();
    }
}
} // namespace tmwa
