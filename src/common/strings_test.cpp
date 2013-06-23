#include "../../src/common/strings.hpp"

#include <gtest/gtest.h>

template<typename T>
class StringTest : public ::testing::Test
{
};
TYPED_TEST_CASE_P(StringTest);

TYPED_TEST_P(StringTest, basic)
{
    TypeParam hi("Hello");
    EXPECT_EQ(5, hi.size());
    EXPECT_EQ(hi, hi);
    const char hi2[] = "Hello\0random garbage";
    EXPECT_EQ(hi, hi2);
    TypeParam hi0;
    EXPECT_EQ(0, hi0.size());
}

TYPED_TEST_P(StringTest, iterators)
{
    TypeParam hi("Hello");
    EXPECT_EQ(hi.begin(), hi.begin());
    EXPECT_NE(hi.begin(), hi.end());
    EXPECT_EQ(5, std::distance(hi.begin(), hi.end()));
    const char *hi2 = "Hello";
    EXPECT_TRUE(std::equal(hi.begin(), hi.end(), hi2));
}

TYPED_TEST_P(StringTest, xslice)
{
    TypeParam hi("Hello, World!");
    EXPECT_EQ(" World!", hi.xslice_t(6));
    EXPECT_EQ("Hello,", hi.xslice_h(6));
    EXPECT_EQ("World!", hi.xrslice_t(6));
    EXPECT_EQ("Hello, ", hi.xrslice_h(6));
    EXPECT_EQ("World", hi.xlslice(7, 5));
    EXPECT_EQ("World", hi.xpslice(7, 12));
    EXPECT_TRUE(hi.startswith("Hello"));
    EXPECT_TRUE(hi.endswith("World!"));
}

TYPED_TEST_P(StringTest, oslice)
{
    TypeParam hi("Hello, World!");
    EXPECT_EQ(" World!", hi.oslice_t(6));
    EXPECT_EQ("Hello,", hi.oslice_h(6));
    EXPECT_EQ("World!", hi.orslice_t(6));
    EXPECT_EQ("Hello, ", hi.orslice_h(6));
    EXPECT_EQ("World", hi.olslice(7, 5));
    EXPECT_EQ("World", hi.opslice(7, 12));
}

REGISTER_TYPED_TEST_CASE_P(StringTest,
        basic, iterators, xslice, oslice);

typedef ::testing::Types<
    FString, TString, SString, ZString, XString, VString<255>
> MostStringTypes;
INSTANTIATE_TYPED_TEST_CASE_P(StringStuff, StringTest, MostStringTypes);

TEST(VStringTest, basic)
{
    VString<5> hi = "Hello";
    EXPECT_EQ(5, hi.size());
    EXPECT_EQ(hi, hi);
    // truncation
    VString<5> hi2(ZString::really_construct_from_a_pointer, "Hello, world!");
    EXPECT_EQ(5, hi2.size());
    EXPECT_EQ(hi, hi2);
    // short
    hi = "hi";
    EXPECT_EQ(2, hi.size());
    VString<5> hi0;
    EXPECT_EQ(0, hi0.size());
}

template<typename T>
class NulStringTest : public ::testing::Test
{
};
TYPED_TEST_CASE_P(NulStringTest);

TYPED_TEST_P(NulStringTest, basic)
{
    TypeParam hi("hello");
    EXPECT_EQ(hi.size(), strlen(hi.c_str()));
    EXPECT_STREQ("hello", hi.c_str());
}

REGISTER_TYPED_TEST_CASE_P(NulStringTest,
        basic);

typedef ::testing::Types<
    FString, TString, ZString, VString<255>
> NulStringTypes;
INSTANTIATE_TYPED_TEST_CASE_P(NulStringStuff, NulStringTest, NulStringTypes);
