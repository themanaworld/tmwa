#include "intern-pool.hpp"

#include <gtest/gtest.h>

#include "../strings/base.hpp"

TEST(InternPool, whydoesthisalwaysneedasecondname)
{
    InternPool p;
    EXPECT_EQ(0, p.size());
    EXPECT_EQ(0, p.intern("hello"));
    EXPECT_EQ(0, p.intern("hello"));
    EXPECT_EQ(1, p.size());
    EXPECT_EQ(1, p.intern("world"));
    EXPECT_EQ(0, p.intern("hello"));
    EXPECT_EQ(1, p.intern("world"));
    EXPECT_EQ(2, p.size());
    EXPECT_EQ("hello", p.outtern(0));
    EXPECT_EQ("world", p.outtern(1));
}
