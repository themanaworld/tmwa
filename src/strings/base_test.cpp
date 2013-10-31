#include "base.hpp"

#include <gtest/gtest.h>

#include "vstring.hpp"
#include "xstring.hpp"
#include "fstring.hpp"

using namespace strings;

struct _test : VString<1> {};
struct _test2 : VString<1> {};

static_assert(string_comparison_allowed<_test, _test>::value, "tt");
static_assert(string_comparison_allowed<VString<1>, VString<1>>::value, "vv");
static_assert(!string_comparison_allowed<_test, XString>::value, "tx");
static_assert(!string_comparison_allowed<_test, VString<1>>::value, "tv");
static_assert(!string_comparison_allowed<_test, _test2>::value, "t2");
static_assert(string_comparison_allowed<VString<1>, XString>::value, "vx");
static_assert(string_comparison_allowed<XString, XString>::value, "xx");
static_assert(string_comparison_allowed<XString, FString>::value, "xf");

TEST(strings, contains)
{
    XString hi = "Hello";
    EXPECT_TRUE(hi.contains_any("Hi"));
    EXPECT_FALSE(hi.contains_any("hi"));
}
