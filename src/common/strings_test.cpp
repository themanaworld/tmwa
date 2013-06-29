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

    __attribute__((unused))
    const FString *base = hi.base();
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

TYPED_TEST_P(StringTest, convert)
{
    constexpr bool is_zstring = std::is_same<TypeParam, ZString>::value;
    typedef typename std::conditional<is_zstring, TString, SString>::type Sstring;
    typedef typename std::conditional<is_zstring, ZString, XString>::type Xstring;
    FString f = "f";
    TString t = "t";
    Sstring s = "s";
    ZString z = "z";
    Xstring x = "x";
    VString<255> v = "v";

    TypeParam f2 = f;
    TypeParam t2 = t;
    TypeParam s2 = s;
    TypeParam z2 = z;
    TypeParam x2 = x;
    TypeParam v2 = v;

    EXPECT_EQ(f2, f);
    EXPECT_EQ(t2, t);
    EXPECT_EQ(s2, s);
    EXPECT_EQ(z2, z);
    EXPECT_EQ(x2, x);
    EXPECT_EQ(v2, v);

    TypeParam f3, t3, s3, z3, x3, v3;
    f3 = f;
    t3 = t;
    s3 = s;
    z3 = z;
    x3 = x;
    v3 = v;

    EXPECT_EQ(f3, f);
    EXPECT_EQ(t3, t);
    EXPECT_EQ(s3, s);
    EXPECT_EQ(z3, z);
    EXPECT_EQ(x3, x);
    EXPECT_EQ(v3, v);
}

REGISTER_TYPED_TEST_CASE_P(StringTest,
        basic, iterators, xslice, oslice, convert);

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
