#include "iter.hpp"
//    iter_test.cpp - Testsuite for tools for dealing with iterators
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

#include <algorithm>

#include "../ints/udl.hpp"

#include "../poison.hpp"


namespace tmwa
{
TEST(iterpair, strings)
{
    IteratorPair<ValueIterator<char>> pair = value_range('0', ':');
    const char *str = "0123456789";
    EXPECT_TRUE(std::equal(pair.begin(), pair.end(), str));
}

TEST(iterpair, signed8)
{
    IteratorPair<ValueIterator<int8_t>> pair = value_range(-128_n8, +127_p8);
    int8_t arr[255] =
    {
        -128, -127, -126, -125, -124, -123, -122, -121, -120,
        -119, -118, -117, -116, -115, -114, -113, -112, -111, -110,
        -109, -108, -107, -106, -105, -104, -103, -102, -101, -100,
        -99, -98, -97, -96, -95, -94, -93, -92, -91, -90,
        -89, -88, -87, -86, -85, -84, -83, -82, -81, -80,
        -79, -78, -77, -76, -75, -74, -73, -72, -71, -70,
        -69, -68, -67, -66, -65, -64, -63, -62, -61, -60,
        -59, -58, -57, -56, -55, -54, -53, -52, -51, -50,
        -49, -48, -47, -46, -45, -44, -43, -42, -41, -40,
        -39, -38, -37, -36, -35, -34, -33, -32, -31, -30,
        -29, -28, -27, -26, -25, -24, -23, -22, -21, -20,
        -19, -18, -17, -16, -15, -14, -13, -12, -11, -10,
        -9, -8, -7, -6, -5, -4, -3, -2, -1,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
        90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126,
    };
    EXPECT_TRUE(std::equal(pair.begin(), pair.end(), arr + 0));
}

TEST(iterpair, unsigned8)
{
    IteratorPair<ValueIterator<uint8_t>> pair = value_range(0_u8, 255_u8);
    uint8_t arr[255] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
        40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
        50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
        60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
        70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
        90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
        110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
        120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
        130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
        140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
        150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169,
        170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
        180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
        190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
        200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
        210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
        220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
        230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
        250, 251, 252, 253, 254,
    };
    EXPECT_TRUE(std::equal(pair.begin(), pair.end(), arr));
}

static
bool is_odd_ref(int& i)
{
    return i % 2;
}

TEST(iterpair, filter1)
{
    int arr[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

    int expected_arr[] = {1, 3, 5, 7};
    int *expected_it = expected_arr;
    int *expected_end = expected_arr + 4;

    for (int& i : filter_iterator<int&>(&arr, is_odd_ref))
    {
        EXPECT_EQ(i, *expected_it);
        ++expected_it;
    }
    EXPECT_EQ(expected_it, expected_end);
}

TEST(iterpair, filter2)
{
    std::vector<int> vals = {0, 1, 0, 2, 0, 3, 0};

    int sum = 0, count = 0;
    for (int i : filter_iterator<int>(&vals))
    {
        sum += i;
        count++;
    }
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(count, 3);
}

TEST(iterpair, filter3)
{
    int one = 1;
    int two = 2;
    int three = 3;
    std::vector<int *> vals = {nullptr, &one, nullptr, &two, nullptr, &three, nullptr};

    int sum = 0, count = 0;
    for (int *i : filter_iterator<int *>(&vals))
    {
        sum += *i;
        count++;
    }
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(count, 3);
}
} // namespace tmwa
