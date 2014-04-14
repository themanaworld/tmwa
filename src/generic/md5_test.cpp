#include "md5.hpp"
//    md5_test.cpp - Testsuite for fundamental MD5 operations.
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

#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../poison.hpp"

// This should be made part of the main API,
// but is not yet to keep the diff small.
// Edit: hack to fix the new strict comparison.
static
VString<32> MD5(XString in)
{
    md5_string out;
    MD5_to_str(MD5_from_string(in), out);
    return out;
}

TEST(md5calc, rfc1321)
{
    EXPECT_EQ("d41d8cd98f00b204e9800998ecf8427e"_s, MD5(""_s));
    EXPECT_EQ("0cc175b9c0f1b6a831c399e269772661"_s, MD5("a"_s));
    EXPECT_EQ("900150983cd24fb0d6963f7d28e17f72"_s, MD5("abc"_s));
    EXPECT_EQ("f96b697d7cb7938d525a2f31aaf161d0"_s, MD5("message digest"_s));
    EXPECT_EQ("c3fcd3d76192e4007dfb496cca67e13b"_s, MD5("abcdefghijklmnopqrstuvwxyz"_s));
    EXPECT_EQ("d174ab98d277d9f5a5611c2c9f419d9f"_s, MD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"_s));
    EXPECT_EQ("57edf4a22be3c955ac49da2e2107b67a"_s, MD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890"_s));
}
