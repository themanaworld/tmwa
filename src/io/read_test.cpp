#include "read.hpp"
//    io/read_test.cpp - Testsuite for input from files
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

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/literal.hpp"

#include "../tests/fdhack.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
io::FD string_pipe(ZString sz)
{
    io::FD rfd, wfd;
    if (-1 == io::FD::pipe(rfd, wfd))
        return io::FD();
    if (sz.size() != wfd.write(sz.c_str(), sz.size()))
    {
        rfd.close();
        wfd.close();
        return io::FD();
    }
    wfd.close();
    return rfd;
}

TEST(io, read1)
{
    QuietFd q;
    io::ReadFile rf(string_pipe("Hello"_s));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello"_s);
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read2)
{
    io::ReadFile rf(string_pipe("Hello\n"_s));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello"_s);
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read3)
{
    QuietFd q;
    io::ReadFile rf(string_pipe("Hello\r"_s));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello"_s);
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read4)
{
    QuietFd q;
    io::ReadFile rf(string_pipe("Hello\r\n"_s));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello"_s);
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read5)
{
    QuietFd q;
    io::ReadFile rf(string_pipe("Hello\n\r"_s));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello"_s);
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_FALSE(hi);
    EXPECT_FALSE(rf.getline(hi));
}
} // namespace tmwa
