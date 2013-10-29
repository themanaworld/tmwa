#include "read.hpp"

#include <gtest/gtest.h>

#include "../strings/zstring.hpp"

static
int string_pipe(ZString sz)
{
    // if this doesn't work we'll get test failures anyway
    int pfd[2];
    pipe(pfd);
    write(pfd[1], sz.c_str(), sz.size());
    close(pfd[1]);
    return pfd[0];
}

TEST(io, read1)
{
    io::ReadFile rf(string_pipe("Hello"));
    FString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read2)
{
    io::ReadFile rf(string_pipe("Hello\n"));
    FString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read3)
{
    io::ReadFile rf(string_pipe("Hello\r"));
    FString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read4)
{
    io::ReadFile rf(string_pipe("Hello\r\n"));
    FString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_FALSE(rf.getline(hi));
}
TEST(io, read5)
{
    io::ReadFile rf(string_pipe("Hello\n\r"));
    FString hi;
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_EQ(hi, "Hello");
    EXPECT_TRUE(rf.getline(hi));
    EXPECT_FALSE(hi);
    EXPECT_FALSE(rf.getline(hi));
}
