#include "udl.hpp"
//    udl_test.cpp - Testsuite for a user-defined integer suffixes
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

#include "../compat/cast.hpp"

#include "../poison.hpp"


#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
TEST(ints, smc)
{
    {
        ints::SignedMagnitudeConstant<false, 0> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7e> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7e> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7f> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7f> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x80> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x80> i;
        (void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xfe> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xfe> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xff> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        (void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xff> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false,0x100> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true, 0x100> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7ffe> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7ffe> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7fff> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7fff> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x8000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x8000> i;
        //(void)static_cast<int8_t>(i);
        (void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xfffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xfffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        (void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false,0x10000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true, 0x10000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7ffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7ffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7fffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7fffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x80000000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x80000000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        (void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xfffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xfffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xffffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        (void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xffffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false,0x100000000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        //(void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true, 0x100000000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7ffffffffffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        //(void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7ffffffffffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x7fffffffffffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        //(void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x7fffffffffffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0x8000000000000000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        //(void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        //(void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0x8000000000000000> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        (void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xfffffffffffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        //(void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        //(void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xfffffffffffffffe> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        //(void)static_cast<int64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<false, 0xffffffffffffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        //(void)static_cast<int64_t>(i);
        //(void)static_cast<uint8_t>(i);
        //(void)static_cast<uint16_t>(i);
        //(void)static_cast<uint32_t>(i);
        (void)static_cast<uint64_t>(i);
    }
    {
        ints::SignedMagnitudeConstant<true,  0xffffffffffffffff> i;
        //(void)static_cast<int8_t>(i);
        //(void)static_cast<int16_t>(i);
        //(void)static_cast<int32_t>(i);
        //(void)static_cast<int64_t>(i);
    }
    {
        //ints::SignedMagnitudeConstant<false,0x10000000000000000> i;
    }
    {
        //ints::SignedMagnitudeConstant<true, 0x10000000000000000> i;
    }
}
#pragma GCC diagnostic pop

TEST(ints, constant)
{
    // gtest is funny with conversions
    assert(0_const == (ints::SignedMagnitudeConstant<false, 0>{}));
    assert(1_const == (ints::SignedMagnitudeConstant<false, 1>{}));
    assert(1_const == (ints::SignedMagnitudeConstant<false, 1>{}));
}

TEST(ints, udl8)
{
    EXPECT_EQ(0b00000000_u8, maybe_cast<uint8_t>(0b00000000U));
    EXPECT_EQ(0b00000001_u8, maybe_cast<uint8_t>(0b00000001U));
    EXPECT_EQ(0b11111110_u8, maybe_cast<uint8_t>(0b11111110U));
    EXPECT_EQ(0b11111111_u8, maybe_cast<uint8_t>(0b11111111U));
    EXPECT_EQ(-0b10000000_n8, maybe_cast<int8_t>(-0b10000000));
    EXPECT_EQ(-0b01111111_n8, maybe_cast<int8_t>(-0b01111111));
    EXPECT_EQ(-0b00000001_n8, maybe_cast<int8_t>(-0b00000001));
    EXPECT_EQ(+0b00000000_p8, maybe_cast<int8_t>(0b00000000));
    EXPECT_EQ(+0b00000001_p8, maybe_cast<int8_t>(0b00000001));
    EXPECT_EQ(+0b01111110_p8, maybe_cast<int8_t>(0b01111110));
    EXPECT_EQ(+0b01111111_p8, maybe_cast<int8_t>(0b01111111));

    EXPECT_EQ(0B00000000_u8, maybe_cast<uint8_t>(0B00000000U));
    EXPECT_EQ(0B00000001_u8, maybe_cast<uint8_t>(0B00000001U));
    EXPECT_EQ(0B11111110_u8, maybe_cast<uint8_t>(0B11111110U));
    EXPECT_EQ(0B11111111_u8, maybe_cast<uint8_t>(0B11111111U));
    EXPECT_EQ(-0B10000000_n8, maybe_cast<int8_t>(-0B10000000));
    EXPECT_EQ(-0B01111111_n8, maybe_cast<int8_t>(-0B01111111));
    EXPECT_EQ(-0B00000001_n8, maybe_cast<int8_t>(-0B00000001));
    EXPECT_EQ(+0B00000000_p8, maybe_cast<int8_t>(0B00000000));
    EXPECT_EQ(+0B00000001_p8, maybe_cast<int8_t>(0B00000001));
    EXPECT_EQ(+0B01111110_p8, maybe_cast<int8_t>(0B01111110));
    EXPECT_EQ(+0B01111111_p8, maybe_cast<int8_t>(0B01111111));

    EXPECT_EQ(0000_u8, maybe_cast<uint8_t>(0000U));
    EXPECT_EQ(0001_u8, maybe_cast<uint8_t>(0001U));
    EXPECT_EQ(0376_u8, maybe_cast<uint8_t>(0376U));
    EXPECT_EQ(0377_u8, maybe_cast<uint8_t>(0377U));
    EXPECT_EQ(-0200_n8, maybe_cast<int8_t>(-0200));
    EXPECT_EQ(-0177_n8, maybe_cast<int8_t>(-0177));
    EXPECT_EQ(-0001_n8, maybe_cast<int8_t>(-0001));
    EXPECT_EQ(+0000_p8, maybe_cast<int8_t>(0000));
    EXPECT_EQ(+0001_p8, maybe_cast<int8_t>(0001));
    EXPECT_EQ(+0176_p8, maybe_cast<int8_t>(0176));
    EXPECT_EQ(+0177_p8, maybe_cast<int8_t>(0177));

    EXPECT_EQ(0_u8, maybe_cast<uint8_t>(0U));
    EXPECT_EQ(1_u8, maybe_cast<uint8_t>(1U));
    EXPECT_EQ(254_u8, maybe_cast<uint8_t>(254U));
    EXPECT_EQ(255_u8, maybe_cast<uint8_t>(255U));
    EXPECT_EQ(-128_n8, maybe_cast<int8_t>(-128));
    EXPECT_EQ(-127_n8, maybe_cast<int8_t>(-127));
    EXPECT_EQ(-1_n8, maybe_cast<int8_t>(-1));
    EXPECT_EQ(+0_p8, maybe_cast<int8_t>(0));
    EXPECT_EQ(+1_p8, maybe_cast<int8_t>(1));
    EXPECT_EQ(+126_p8, maybe_cast<int8_t>(126));
    EXPECT_EQ(+127_p8, maybe_cast<int8_t>(127));

    EXPECT_EQ(0x00_u8, maybe_cast<uint8_t>(0x00U));
    EXPECT_EQ(0x01_u8, maybe_cast<uint8_t>(0x01U));
    EXPECT_EQ(0xfe_u8, maybe_cast<uint8_t>(0xfeU));
    EXPECT_EQ(0xff_u8, maybe_cast<uint8_t>(0xffU));
    EXPECT_EQ(-0x80_n8, maybe_cast<int8_t>(-0x80));
    EXPECT_EQ(-0x7f_n8, maybe_cast<int8_t>(-0x7f));
    EXPECT_EQ(-0x01_n8, maybe_cast<int8_t>(-0x01));
    EXPECT_EQ(+0x00_p8, maybe_cast<int8_t>(0x00));
    EXPECT_EQ(+0x01_p8, maybe_cast<int8_t>(0x01));
    EXPECT_EQ(+0x7e_p8, maybe_cast<int8_t>(0x7e));
    EXPECT_EQ(+0x7f_p8, maybe_cast<int8_t>(0x7f));

    EXPECT_EQ(0X00_u8, maybe_cast<uint8_t>(0X00U));
    EXPECT_EQ(0X01_u8, maybe_cast<uint8_t>(0X01U));
    EXPECT_EQ(0XFE_u8, maybe_cast<uint8_t>(0XFEU));
    EXPECT_EQ(0XFF_u8, maybe_cast<uint8_t>(0XFFU));
    EXPECT_EQ(-0X80_n8, maybe_cast<int8_t>(-0X80));
    EXPECT_EQ(-0X7F_n8, maybe_cast<int8_t>(-0X7F));
    EXPECT_EQ(-0X01_n8, maybe_cast<int8_t>(-0X01));
    EXPECT_EQ(+0X00_p8, maybe_cast<int8_t>(0X00));
    EXPECT_EQ(+0X01_p8, maybe_cast<int8_t>(0X01));
    EXPECT_EQ(+0X7E_p8, maybe_cast<int8_t>(0X7E));
    EXPECT_EQ(+0X7F_p8, maybe_cast<int8_t>(0X7F));
}

TEST(ints, udl16)
{
    EXPECT_EQ(0b0000000000000000_u16, maybe_cast<uint16_t>(0b0000000000000000U));
    EXPECT_EQ(0b0000000000000001_u16, maybe_cast<uint16_t>(0b0000000000000001U));
    EXPECT_EQ(0b1111111111111110_u16, maybe_cast<uint16_t>(0b1111111111111110U));
    EXPECT_EQ(0b1111111111111111_u16, maybe_cast<uint16_t>(0b1111111111111111U));
    EXPECT_EQ(-0b1000000000000000_n16, maybe_cast<int16_t>(-0b1000000000000000));
    EXPECT_EQ(-0b0111111111111111_n16, maybe_cast<int16_t>(-0b0111111111111111));
    EXPECT_EQ(-0b0000000000000001_n16, maybe_cast<int16_t>(-0b0000000000000001));
    EXPECT_EQ(+0b0000000000000000_p16, maybe_cast<int16_t>(0b0000000000000000));
    EXPECT_EQ(+0b0000000000000001_p16, maybe_cast<int16_t>(0b0000000000000001));
    EXPECT_EQ(+0b0111111111111110_p16, maybe_cast<int16_t>(0b0111111111111110));
    EXPECT_EQ(+0b0111111111111111_p16, maybe_cast<int16_t>(0b0111111111111111));

    EXPECT_EQ(0B0000000000000000_u16, maybe_cast<uint16_t>(0B0000000000000000U));
    EXPECT_EQ(0B0000000000000001_u16, maybe_cast<uint16_t>(0B0000000000000001U));
    EXPECT_EQ(0B1111111111111110_u16, maybe_cast<uint16_t>(0B1111111111111110U));
    EXPECT_EQ(0B1111111111111111_u16, maybe_cast<uint16_t>(0B1111111111111111U));
    EXPECT_EQ(-0B1000000000000000_n16, maybe_cast<int16_t>(-0B1000000000000000));
    EXPECT_EQ(-0B0111111111111111_n16, maybe_cast<int16_t>(-0B0111111111111111));
    EXPECT_EQ(-0B0000000000000001_n16, maybe_cast<int16_t>(-0B0000000000000001));
    EXPECT_EQ(+0B0000000000000000_p16, maybe_cast<int16_t>(0B0000000000000000));
    EXPECT_EQ(+0B0000000000000001_p16, maybe_cast<int16_t>(0B0000000000000001));
    EXPECT_EQ(+0B0111111111111110_p16, maybe_cast<int16_t>(0B0111111111111110));
    EXPECT_EQ(+0B0111111111111111_p16, maybe_cast<int16_t>(0B0111111111111111));

    EXPECT_EQ(0000000_u16, maybe_cast<uint16_t>(0000000U));
    EXPECT_EQ(0000001_u16, maybe_cast<uint16_t>(0000001U));
    EXPECT_EQ(0177776_u16, maybe_cast<uint16_t>(0177776U));
    EXPECT_EQ(0177777_u16, maybe_cast<uint16_t>(0177777U));
    EXPECT_EQ(-0100000_n16, maybe_cast<int16_t>(-0100000));
    EXPECT_EQ(-0077777_n16, maybe_cast<int16_t>(-0077777));
    EXPECT_EQ(-0000001_n16, maybe_cast<int16_t>(-0000001));
    EXPECT_EQ(+000000_p16, maybe_cast<int16_t>(000000));
    EXPECT_EQ(+000001_p16, maybe_cast<int16_t>(000001));
    EXPECT_EQ(+077776_p16, maybe_cast<int16_t>(077776));
    EXPECT_EQ(+077777_p16, maybe_cast<int16_t>(077777));

    EXPECT_EQ(0_u16, maybe_cast<uint16_t>(0U));
    EXPECT_EQ(1_u16, maybe_cast<uint16_t>(1U));
    EXPECT_EQ(65534_u16, maybe_cast<uint16_t>(65534U));
    EXPECT_EQ(65535_u16, maybe_cast<uint16_t>(65535U));
    EXPECT_EQ(-32768_n16, maybe_cast<int16_t>(-32768));
    EXPECT_EQ(-32767_n16, maybe_cast<int16_t>(-32767));
    EXPECT_EQ(-1_n16, maybe_cast<int16_t>(-1));
    EXPECT_EQ(+0_p16, maybe_cast<int16_t>(0));
    EXPECT_EQ(+1_p16, maybe_cast<int16_t>(1));
    EXPECT_EQ(+32766_p16, maybe_cast<int16_t>(32766));
    EXPECT_EQ(+32767_p16, maybe_cast<int16_t>(32767));

    EXPECT_EQ(0x0000_u16, maybe_cast<uint16_t>(0x0000U));
    EXPECT_EQ(0x0001_u16, maybe_cast<uint16_t>(0x0001U));
    EXPECT_EQ(0xfffe_u16, maybe_cast<uint16_t>(0xfffeU));
    EXPECT_EQ(0xffff_u16, maybe_cast<uint16_t>(0xffffU));
    EXPECT_EQ(-0x8000_n16, maybe_cast<int16_t>(-0x8000));
    EXPECT_EQ(-0x7fff_n16, maybe_cast<int16_t>(-0x7fff));
    EXPECT_EQ(-0x0001_n16, maybe_cast<int16_t>(-0x0001));
    EXPECT_EQ(+0x0000_p16, maybe_cast<int16_t>(0x0000));
    EXPECT_EQ(+0x0001_p16, maybe_cast<int16_t>(0x0001));
    EXPECT_EQ(+0x7ffe_p16, maybe_cast<int16_t>(0x7ffe));
    EXPECT_EQ(+0x7fff_p16, maybe_cast<int16_t>(0x7fff));

    EXPECT_EQ(0X0000_u16, maybe_cast<uint16_t>(0X0000U));
    EXPECT_EQ(0X0001_u16, maybe_cast<uint16_t>(0X0001U));
    EXPECT_EQ(0XFFFE_u16, maybe_cast<uint16_t>(0XFFFEU));
    EXPECT_EQ(0XFFFF_u16, maybe_cast<uint16_t>(0XFFFFU));
    EXPECT_EQ(-0X8000_n16, maybe_cast<int16_t>(-0X8000));
    EXPECT_EQ(-0X7FFF_n16, maybe_cast<int16_t>(-0X7FFF));
    EXPECT_EQ(-0X0001_n16, maybe_cast<int16_t>(-0X0001));
    EXPECT_EQ(+0X0000_p16, maybe_cast<int16_t>(0X0000));
    EXPECT_EQ(+0X0001_p16, maybe_cast<int16_t>(0X0001));
    EXPECT_EQ(+0X7FFE_p16, maybe_cast<int16_t>(0X7FFE));
    EXPECT_EQ(+0X7FFF_p16, maybe_cast<int16_t>(0X7FFF));
}

TEST(ints, udl32)
{
    EXPECT_EQ(0b00000000000000000000000000000000_u32, maybe_cast<uint32_t>(0b00000000000000000000000000000000U));
    EXPECT_EQ(0b00000000000000000000000000000001_u32, maybe_cast<uint32_t>(0b00000000000000000000000000000001U));
    EXPECT_EQ(0b11111111111111111111111111111110_u32, maybe_cast<uint32_t>(0b11111111111111111111111111111110U));
    EXPECT_EQ(0b11111111111111111111111111111111_u32, maybe_cast<uint32_t>(0b11111111111111111111111111111111U));
    EXPECT_EQ(-0b10000000000000000000000000000000_n32, maybe_cast<int32_t>(-0b10000000000000000000000000000000));
    EXPECT_EQ(-0b01111111111111111111111111111111_n32, maybe_cast<int32_t>(-0b01111111111111111111111111111111));
    EXPECT_EQ(-0b00000000000000000000000000000001_n32, maybe_cast<int32_t>(-0b00000000000000000000000000000001));
    EXPECT_EQ(+0b00000000000000000000000000000000_p32, maybe_cast<int32_t>(0b00000000000000000000000000000000));
    EXPECT_EQ(+0b00000000000000000000000000000001_p32, maybe_cast<int32_t>(0b00000000000000000000000000000001));
    EXPECT_EQ(+0b01111111111111111111111111111110_p32, maybe_cast<int32_t>(0b01111111111111111111111111111110));
    EXPECT_EQ(+0b01111111111111111111111111111111_p32, maybe_cast<int32_t>(0b01111111111111111111111111111111));

    EXPECT_EQ(0B00000000000000000000000000000000_u32, maybe_cast<uint32_t>(0B00000000000000000000000000000000U));
    EXPECT_EQ(0B00000000000000000000000000000001_u32, maybe_cast<uint32_t>(0B00000000000000000000000000000001U));
    EXPECT_EQ(0B11111111111111111111111111111110_u32, maybe_cast<uint32_t>(0B11111111111111111111111111111110U));
    EXPECT_EQ(0B11111111111111111111111111111111_u32, maybe_cast<uint32_t>(0B11111111111111111111111111111111U));
    EXPECT_EQ(-0B10000000000000000000000000000000_n32, maybe_cast<int32_t>(-0B10000000000000000000000000000000));
    EXPECT_EQ(-0B01111111111111111111111111111111_n32, maybe_cast<int32_t>(-0B01111111111111111111111111111111));
    EXPECT_EQ(-0B00000000000000000000000000000001_n32, maybe_cast<int32_t>(-0B00000000000000000000000000000001));
    EXPECT_EQ(+0B00000000000000000000000000000000_p32, maybe_cast<int32_t>(0B00000000000000000000000000000000));
    EXPECT_EQ(+0B00000000000000000000000000000001_p32, maybe_cast<int32_t>(0B00000000000000000000000000000001));
    EXPECT_EQ(+0B01111111111111111111111111111110_p32, maybe_cast<int32_t>(0B01111111111111111111111111111110));
    EXPECT_EQ(+0B01111111111111111111111111111111_p32, maybe_cast<int32_t>(0B01111111111111111111111111111111));

    EXPECT_EQ(000000000000_u32, maybe_cast<uint32_t>(000000000000U));
    EXPECT_EQ(000000000001_u32, maybe_cast<uint32_t>(000000000001U));
    EXPECT_EQ(037777777776_u32, maybe_cast<uint32_t>(037777777776U));
    EXPECT_EQ(037777777777_u32, maybe_cast<uint32_t>(037777777777U));
    EXPECT_EQ(-020000000000_n32, maybe_cast<int32_t>(-020000000000));
    EXPECT_EQ(-017777777777_n32, maybe_cast<int32_t>(-017777777777));
    EXPECT_EQ(-000000000001_n32, maybe_cast<int32_t>(-000000000001));
    EXPECT_EQ(+000000000000_p32, maybe_cast<int32_t>(000000000000));
    EXPECT_EQ(+000000000001_p32, maybe_cast<int32_t>(000000000001));
    EXPECT_EQ(+017777777776_p32, maybe_cast<int32_t>(017777777776));
    EXPECT_EQ(+017777777777_p32, maybe_cast<int32_t>(017777777777));

    EXPECT_EQ(0_u32, maybe_cast<uint32_t>(0U));
    EXPECT_EQ(1_u32, maybe_cast<uint32_t>(1U));
    EXPECT_EQ(4294967294_u32, maybe_cast<uint32_t>(4294967294U));
    EXPECT_EQ(4294967295_u32, maybe_cast<uint32_t>(4294967295U));
    EXPECT_EQ(-2147483648_n32, maybe_cast<int32_t>(-2147483648));
    EXPECT_EQ(-2147483647_n32, maybe_cast<int32_t>(-2147483647));
    EXPECT_EQ(-1_n32, maybe_cast<int32_t>(-1));
    EXPECT_EQ(+0_p32, maybe_cast<int32_t>(0));
    EXPECT_EQ(+1_p32, maybe_cast<int32_t>(1));
    EXPECT_EQ(+2147483646_p32, maybe_cast<int32_t>(2147483646));
    EXPECT_EQ(+2147483647_p32, maybe_cast<int32_t>(2147483647));

    EXPECT_EQ(0x00000000_u32, maybe_cast<uint32_t>(0x00000000U));
    EXPECT_EQ(0x00000001_u32, maybe_cast<uint32_t>(0x00000001U));
    EXPECT_EQ(0xfffffffe_u32, maybe_cast<uint32_t>(0xfffffffeU));
    EXPECT_EQ(0xffffffff_u32, maybe_cast<uint32_t>(0xffffffffU));
    EXPECT_EQ(-0x80000000_n32, maybe_cast<int32_t>(-0x80000000));
    EXPECT_EQ(-0x7fffffff_n32, maybe_cast<int32_t>(-0x7fffffff));
    EXPECT_EQ(-0x00000001_n32, maybe_cast<int32_t>(-0x00000001));
    EXPECT_EQ(+0x00000000_p32, maybe_cast<int32_t>(0x00000000));
    EXPECT_EQ(+0x00000001_p32, maybe_cast<int32_t>(0x00000001));
    EXPECT_EQ(+0x7ffffffe_p32, maybe_cast<int32_t>(0x7ffffffe));
    EXPECT_EQ(+0x7fffffff_p32, maybe_cast<int32_t>(0x7fffffff));

    EXPECT_EQ(0X00000000_u32, maybe_cast<uint32_t>(0X00000000U));
    EXPECT_EQ(0X00000001_u32, maybe_cast<uint32_t>(0X00000001U));
    EXPECT_EQ(0XFFFFFFFE_u32, maybe_cast<uint32_t>(0XFFFFFFFEU));
    EXPECT_EQ(0XFFFFFFFF_u32, maybe_cast<uint32_t>(0XFFFFFFFFU));
    EXPECT_EQ(-0X80000000_n32, maybe_cast<int32_t>(-0X80000000));
    EXPECT_EQ(-0X7FFFFFFF_n32, maybe_cast<int32_t>(-0X7FFFFFFF));
    EXPECT_EQ(-0X00000001_n32, maybe_cast<int32_t>(-0X00000001));
    EXPECT_EQ(+0X00000000_p32, maybe_cast<int32_t>(0X00000000));
    EXPECT_EQ(+0X00000001_p32, maybe_cast<int32_t>(0X00000001));
    EXPECT_EQ(+0X7FFFFFFE_p32, maybe_cast<int32_t>(0X7FFFFFFE));
    EXPECT_EQ(+0X7FFFFFFF_p32, maybe_cast<int32_t>(0X7FFFFFFF));
}

TEST(ints, udl64)
{
    EXPECT_EQ(0b0000000000000000000000000000000000000000000000000000000000000000_u64, maybe_cast<uint64_t>(0b0000000000000000000000000000000000000000000000000000000000000000U));
    EXPECT_EQ(0b0000000000000000000000000000000000000000000000000000000000000001_u64, maybe_cast<uint64_t>(0b0000000000000000000000000000000000000000000000000000000000000001U));
    EXPECT_EQ(0b1111111111111111111111111111111111111111111111111111111111111110_u64, maybe_cast<uint64_t>(0b1111111111111111111111111111111111111111111111111111111111111110U));
    EXPECT_EQ(0b1111111111111111111111111111111111111111111111111111111111111111_u64, maybe_cast<uint64_t>(0b1111111111111111111111111111111111111111111111111111111111111111U));
    EXPECT_EQ(-0b1000000000000000000000000000000000000000000000000000000000000000_n64, maybe_cast<int64_t>(-0b1000000000000000000000000000000000000000000000000000000000000000));
    EXPECT_EQ(-0b0111111111111111111111111111111111111111111111111111111111111111_n64, maybe_cast<int64_t>(-0b0111111111111111111111111111111111111111111111111111111111111111));
    EXPECT_EQ(-0b0000000000000000000000000000000000000000000000000000000000000001_n64, maybe_cast<int64_t>(-0b0000000000000000000000000000000000000000000000000000000000000001));
    EXPECT_EQ(+0b0000000000000000000000000000000000000000000000000000000000000000_p64, maybe_cast<int64_t>(0b0000000000000000000000000000000000000000000000000000000000000000));
    EXPECT_EQ(+0b0000000000000000000000000000000000000000000000000000000000000001_p64, maybe_cast<int64_t>(0b0000000000000000000000000000000000000000000000000000000000000001));
    EXPECT_EQ(+0b0111111111111111111111111111111111111111111111111111111111111110_p64, maybe_cast<int64_t>(0b0111111111111111111111111111111111111111111111111111111111111110));
    EXPECT_EQ(+0b0111111111111111111111111111111111111111111111111111111111111111_p64, maybe_cast<int64_t>(0b0111111111111111111111111111111111111111111111111111111111111111));

    EXPECT_EQ(0B0000000000000000000000000000000000000000000000000000000000000000_u64, maybe_cast<uint64_t>(0B0000000000000000000000000000000000000000000000000000000000000000U));
    EXPECT_EQ(0B0000000000000000000000000000000000000000000000000000000000000001_u64, maybe_cast<uint64_t>(0B0000000000000000000000000000000000000000000000000000000000000001U));
    EXPECT_EQ(0B1111111111111111111111111111111111111111111111111111111111111110_u64, maybe_cast<uint64_t>(0B1111111111111111111111111111111111111111111111111111111111111110U));
    EXPECT_EQ(0B1111111111111111111111111111111111111111111111111111111111111111_u64, maybe_cast<uint64_t>(0B1111111111111111111111111111111111111111111111111111111111111111U));
    EXPECT_EQ(-0B1000000000000000000000000000000000000000000000000000000000000000_n64, maybe_cast<int64_t>(-0B1000000000000000000000000000000000000000000000000000000000000000));
    EXPECT_EQ(-0B0111111111111111111111111111111111111111111111111111111111111111_n64, maybe_cast<int64_t>(-0B0111111111111111111111111111111111111111111111111111111111111111));
    EXPECT_EQ(-0B0000000000000000000000000000000000000000000000000000000000000001_n64, maybe_cast<int64_t>(-0B0000000000000000000000000000000000000000000000000000000000000001));
    EXPECT_EQ(+0B0000000000000000000000000000000000000000000000000000000000000000_p64, maybe_cast<int64_t>(0B0000000000000000000000000000000000000000000000000000000000000000));
    EXPECT_EQ(+0B0000000000000000000000000000000000000000000000000000000000000001_p64, maybe_cast<int64_t>(0B0000000000000000000000000000000000000000000000000000000000000001));
    EXPECT_EQ(+0B0111111111111111111111111111111111111111111111111111111111111110_p64, maybe_cast<int64_t>(0B0111111111111111111111111111111111111111111111111111111111111110));
    EXPECT_EQ(+0B0111111111111111111111111111111111111111111111111111111111111111_p64, maybe_cast<int64_t>(0B0111111111111111111111111111111111111111111111111111111111111111));

    EXPECT_EQ(00000000000000000000000_u64, maybe_cast<uint64_t>(00000000000000000000000U));
    EXPECT_EQ(00000000000000000000001_u64, maybe_cast<uint64_t>(00000000000000000000001U));
    EXPECT_EQ(01777777777777777777776_u64, maybe_cast<uint64_t>(01777777777777777777776U));
    EXPECT_EQ(01777777777777777777777_u64, maybe_cast<uint64_t>(01777777777777777777777U));
    EXPECT_EQ(-01000000000000000000000_n64, maybe_cast<int64_t>(-01000000000000000000000));
    EXPECT_EQ(-00777777777777777777777_n64, maybe_cast<int64_t>(-00777777777777777777777));
    EXPECT_EQ(-00000000000000000000001_n64, maybe_cast<int64_t>(-000000000000000000000001));
    EXPECT_EQ(+0000000000000000000000_p64, maybe_cast<int64_t>(0000000000000000000000));
    EXPECT_EQ(+0000000000000000000001_p64, maybe_cast<int64_t>(0000000000000000000001));
    EXPECT_EQ(+0777777777777777777776_p64, maybe_cast<int64_t>(0777777777777777777776));
    EXPECT_EQ(+0777777777777777777777_p64, maybe_cast<int64_t>(0777777777777777777777));

    EXPECT_EQ(0_u64, maybe_cast<uint64_t>(0U));
    EXPECT_EQ(1_u64, maybe_cast<uint64_t>(1U));
    EXPECT_EQ(18446744073709551614_u64, maybe_cast<uint64_t>(18446744073709551614U));
    EXPECT_EQ(18446744073709551615_u64, maybe_cast<uint64_t>(18446744073709551615U));
    EXPECT_EQ(-9223372036854775808_n64, maybe_cast<int64_t>(-9223372036854775808U));
    EXPECT_EQ(-9223372036854775807_n64, maybe_cast<int64_t>(-9223372036854775807));
    EXPECT_EQ(-1_n64, maybe_cast<int64_t>(-1));
    EXPECT_EQ(+0_p64, maybe_cast<int64_t>(0));
    EXPECT_EQ(+1_p64, maybe_cast<int64_t>(1));
    EXPECT_EQ(+9223372036854775806_p64, maybe_cast<int64_t>(9223372036854775806));
    EXPECT_EQ(+9223372036854775807_p64, maybe_cast<int64_t>(9223372036854775807));

    EXPECT_EQ(0x0000000000000000_u64, maybe_cast<uint64_t>(0x0000000000000000U));
    EXPECT_EQ(0x0000000000000001_u64, maybe_cast<uint64_t>(0x0000000000000001U));
    EXPECT_EQ(0xfffffffffffffffe_u64, maybe_cast<uint64_t>(0xfffffffffffffffeU));
    EXPECT_EQ(0xffffffffffffffff_u64, maybe_cast<uint64_t>(0xffffffffffffffffU));
    EXPECT_EQ(-0x8000000000000000_n64, maybe_cast<int64_t>(-0x8000000000000000));
    EXPECT_EQ(-0x7fffffffffffffff_n64, maybe_cast<int64_t>(-0x7fffffffffffffff));
    EXPECT_EQ(-0x0000000000000001_n64, maybe_cast<int64_t>(-0x0000000000000001));
    EXPECT_EQ(+0x0000000000000000_p64, maybe_cast<int64_t>(0x0000000000000000));
    EXPECT_EQ(+0x0000000000000001_p64, maybe_cast<int64_t>(0x0000000000000001));
    EXPECT_EQ(+0x7ffffffffffffffe_p64, maybe_cast<int64_t>(0x7ffffffffffffffe));
    EXPECT_EQ(+0x7fffffffffffffff_p64, maybe_cast<int64_t>(0x7fffffffffffffff));

    EXPECT_EQ(0X0000000000000000_u64, maybe_cast<uint64_t>(0X0000000000000000U));
    EXPECT_EQ(0X0000000000000001_u64, maybe_cast<uint64_t>(0X0000000000000001U));
    EXPECT_EQ(0XFFFFFFFFFFFFFFFE_u64, maybe_cast<uint64_t>(0XFFFFFFFFFFFFFFFEU));
    EXPECT_EQ(0XFFFFFFFFFFFFFFFF_u64, maybe_cast<uint64_t>(0XFFFFFFFFFFFFFFFFU));
    EXPECT_EQ(-0X8000000000000000_n64, maybe_cast<int64_t>(-0X8000000000000000));
    EXPECT_EQ(-0X7FFFFFFFFFFFFFFF_n64, maybe_cast<int64_t>(-0X7FFFFFFFFFFFFFFF));
    EXPECT_EQ(-0X0000000000000001_n64, maybe_cast<int64_t>(-0X0000000000000001));
    EXPECT_EQ(+0X0000000000000000_p64, maybe_cast<int64_t>(0X0000000000000000));
    EXPECT_EQ(+0X0000000000000001_p64, maybe_cast<int64_t>(0X0000000000000001));
    EXPECT_EQ(+0X7FFFFFFFFFFFFFFE_p64, maybe_cast<int64_t>(0X7FFFFFFFFFFFFFFE));
    EXPECT_EQ(+0X7FFFFFFFFFFFFFFF_p64, maybe_cast<int64_t>(0X7FFFFFFFFFFFFFFF));
}
