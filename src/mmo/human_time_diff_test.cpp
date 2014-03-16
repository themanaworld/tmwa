#include "human_time_diff.hpp"

#include <gtest/gtest.h>

// a sequence of [-+]?[0-9]+([ay]|m|[jd]|h|mn|s)

TEST(humantimediff, single)
{
    HumanTimeDiff diff;

    EXPECT_TRUE(extract("42y", &diff));
    EXPECT_EQ(42, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42m", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(42, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42d", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(42, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42h", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(42, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42mn", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(42, diff.minute);
    EXPECT_EQ(0, diff.second);

    EXPECT_TRUE(extract("42s", &diff));
    EXPECT_EQ(0, diff.year);
    EXPECT_EQ(0, diff.month);
    EXPECT_EQ(0, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(42, diff.second);

    EXPECT_TRUE(extract("+42y", &diff));
    EXPECT_EQ(42, diff.year);
    EXPECT_TRUE(extract("-42y", &diff));
    EXPECT_EQ(-42, diff.year);
    EXPECT_FALSE(extract("++42y", &diff));
    EXPECT_FALSE(extract("+-42y", &diff));
    EXPECT_FALSE(extract("-+42y", &diff));
    EXPECT_FALSE(extract("--42y", &diff));
    EXPECT_FALSE(extract("4+2y", &diff));
    EXPECT_FALSE(extract("42z", &diff));
}

TEST(humantimediff, multiple)
{
    HumanTimeDiff diff;

    EXPECT_TRUE(extract("42y23m-2d", &diff));
    EXPECT_EQ(42, diff.year);
    EXPECT_EQ(23, diff.month);
    EXPECT_EQ(-2, diff.day);
    EXPECT_EQ(0, diff.hour);
    EXPECT_EQ(0, diff.minute);
    EXPECT_EQ(0, diff.second);
    EXPECT_FALSE(extract("1y2y", &diff));
}
