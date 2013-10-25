#include "all.hpp"

#include <algorithm>

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

TYPED_TEST_P(StringTest, order)
{
    TypeParam a;
    TypeParam b("Hello");
    TypeParam c("Hello,");
    TypeParam d("World!");

    // not using EXPECT_LT, etc. for better visibility

    EXPECT_FALSE(a < a);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(a < d);
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(b < b);
    EXPECT_TRUE(b < c);
    EXPECT_TRUE(b < d);
    EXPECT_FALSE(c < a);
    EXPECT_FALSE(c < b);
    EXPECT_FALSE(c < c);
    EXPECT_TRUE(c < d);
    EXPECT_FALSE(d < a);
    EXPECT_FALSE(d < b);
    EXPECT_FALSE(d < c);
    EXPECT_FALSE(d < d);

    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(a <= d);
    EXPECT_FALSE(b <= a);
    EXPECT_TRUE(b <= b);
    EXPECT_TRUE(b <= c);
    EXPECT_TRUE(b <= d);
    EXPECT_FALSE(c <= a);
    EXPECT_FALSE(c <= b);
    EXPECT_TRUE(c <= c);
    EXPECT_TRUE(c <= d);
    EXPECT_FALSE(d <= a);
    EXPECT_FALSE(d <= b);
    EXPECT_FALSE(d <= c);
    EXPECT_TRUE(d <= d);

    EXPECT_TRUE(a >= a);
    EXPECT_FALSE(a >= b);
    EXPECT_FALSE(a >= c);
    EXPECT_FALSE(a >= d);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(b >= b);
    EXPECT_FALSE(b >= c);
    EXPECT_FALSE(b >= d);
    EXPECT_TRUE(c >= a);
    EXPECT_TRUE(c >= b);
    EXPECT_TRUE(c >= c);
    EXPECT_FALSE(c >= d);
    EXPECT_TRUE(d >= a);
    EXPECT_TRUE(d >= b);
    EXPECT_TRUE(d >= c);
    EXPECT_TRUE(d >= d);

    EXPECT_FALSE(a > a);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(a > c);
    EXPECT_FALSE(a > d);
    EXPECT_TRUE(b > a);
    EXPECT_FALSE(b > b);
    EXPECT_FALSE(b > c);
    EXPECT_FALSE(b > d);
    EXPECT_TRUE(c > a);
    EXPECT_TRUE(c > b);
    EXPECT_FALSE(c > c);
    EXPECT_FALSE(c > d);
    EXPECT_TRUE(d > a);
    EXPECT_TRUE(d > b);
    EXPECT_TRUE(d > c);
    EXPECT_FALSE(d > d);

    EXPECT_TRUE(a == a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(a == d);
    EXPECT_FALSE(b == a);
    EXPECT_TRUE(b == b);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(b == d);
    EXPECT_FALSE(c == a);
    EXPECT_FALSE(c == b);
    EXPECT_TRUE(c == c);
    EXPECT_FALSE(c == d);
    EXPECT_FALSE(d == a);
    EXPECT_FALSE(d == b);
    EXPECT_FALSE(d == c);
    EXPECT_TRUE(d == d);

    EXPECT_FALSE(a != a);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(a != d);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(b != b);
    EXPECT_TRUE(b != c);
    EXPECT_TRUE(b != d);
    EXPECT_TRUE(c != a);
    EXPECT_TRUE(c != b);
    EXPECT_FALSE(c != c);
    EXPECT_TRUE(c != d);
    EXPECT_TRUE(d != a);
    EXPECT_TRUE(d != b);
    EXPECT_TRUE(d != c);
    EXPECT_FALSE(d != d);
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
    typename TypeParam::iterator it = std::find(hi.begin(), hi.end(), ' ');
    EXPECT_EQ(" World!", hi.xislice_t(it));
    EXPECT_EQ("Hello,", hi.xislice_h(it));
    EXPECT_EQ("World", hi.xlslice(7, 5));
    EXPECT_EQ("World", hi.xpslice(7, 12));
    EXPECT_EQ("World", hi.xislice(hi.begin() + 7, hi.begin() + 12));
    EXPECT_TRUE(hi.startswith("Hello"));
    EXPECT_TRUE(hi.endswith("World!"));
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
    const char l[] = "l";
    VString<5> hi = "hello";

    TypeParam f2 = f;
    TypeParam t2 = t;
    TypeParam s2 = s;
    TypeParam z2 = z;
    TypeParam x2 = x;
    TypeParam v2 = v;
    TypeParam l2 = l;
    TypeParam hi2 = hi;

    EXPECT_EQ(f, f2);
    EXPECT_EQ(t, t2);
    EXPECT_EQ(s, s2);
    EXPECT_EQ(z, z2);
    EXPECT_EQ(x, x2);
    EXPECT_EQ(v, v2);
    EXPECT_EQ(l, l2);
    EXPECT_EQ(hi, hi2);

    TypeParam f3, t3, s3, z3, x3, v3, l3, hi3;
    f3 = f;
    t3 = t;
    s3 = s;
    z3 = z;
    x3 = x;
    v3 = v;
    l3 = l;
    hi3 = hi;

    EXPECT_EQ(f, f3);
    EXPECT_EQ(t, t3);
    EXPECT_EQ(s, s3);
    EXPECT_EQ(z, z3);
    EXPECT_EQ(x, x3);
    EXPECT_EQ(v, v3);
    EXPECT_EQ(l, l3);
    EXPECT_EQ(hi, hi3);

    TypeParam f4(f);
    TypeParam t4(t);
    TypeParam s4(s);
    TypeParam z4(z);
    TypeParam x4(x);
    TypeParam v4(v);
    TypeParam l4(l);
    TypeParam hi4(hi);

    EXPECT_EQ(f, f4);
    EXPECT_EQ(t, t4);
    EXPECT_EQ(s, s4);
    EXPECT_EQ(z, z4);
    EXPECT_EQ(x, x4);
    EXPECT_EQ(v, v4);
    EXPECT_EQ(l, l4);
    EXPECT_EQ(hi, hi4);
}

REGISTER_TYPED_TEST_CASE_P(StringTest,
        basic, order, iterators, xslice, convert);

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
    VString<5> hi2(strings::really_construct_from_a_pointer, "Hello, world!");
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
