#include "read.hpp"

#include <gtest/gtest.h>

#include "../strings/zstring.hpp"

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
    io::ReadFile rf(string_pipe("Hello"));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read2)
{
    io::ReadFile rf(string_pipe("Hello\n"));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read3)
{
    io::ReadFile rf(string_pipe("Hello\r"));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read4)
{
    io::ReadFile rf(string_pipe("Hello\r\n"));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read5)
{
    io::ReadFile rf(string_pipe("Hello\n\r"));
    AString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_FALSE(hi);
    EXPECT_FALSE(rf.getline(hi));
}
