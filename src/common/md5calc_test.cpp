#include "md5calc.hpp"

#include <gtest/gtest.h>

#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "utils.hpp"

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
    EXPECT_EQ("d41d8cd98f00b204e9800998ecf8427e", MD5(""));
    EXPECT_EQ("0cc175b9c0f1b6a831c399e269772661", MD5("a"));
    EXPECT_EQ("900150983cd24fb0d6963f7d28e17f72", MD5("abc"));
    EXPECT_EQ("f96b697d7cb7938d525a2f31aaf161d0", MD5("message digest"));
    EXPECT_EQ("c3fcd3d76192e4007dfb496cca67e13b", MD5("abcdefghijklmnopqrstuvwxyz"));
    EXPECT_EQ("d174ab98d277d9f5a5611c2c9f419d9f", MD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"));
    EXPECT_EQ("57edf4a22be3c955ac49da2e2107b67a", MD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890"));
}
