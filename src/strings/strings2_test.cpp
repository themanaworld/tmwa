#include "all.hpp"

#include <gtest/gtest.h>

TEST(StringTests, traits2)
{
    ZString print_non = "\t\e";
    ZString print_mix = "n\t";
    FString print_all = "n ";
    EXPECT_FALSE(print_non.has_print());
    EXPECT_TRUE(print_mix.has_print());
    EXPECT_TRUE(print_all.has_print());
    EXPECT_FALSE(print_non.is_print());
    EXPECT_FALSE(print_mix.is_print());
    EXPECT_TRUE(print_all.is_print());
    EXPECT_EQ("__", print_non.to_print());
    EXPECT_EQ("n_", print_mix.to_print());
    EXPECT_EQ("n ", print_all.to_print());
    EXPECT_EQ(print_all.begin(), print_all.to_print().begin());

    ZString graph_non = " \e";
    ZString graph_mix = "n ";
    FString graph_all = "n.";
    EXPECT_FALSE(graph_non.has_graph());
    EXPECT_TRUE(graph_mix.has_graph());
    EXPECT_TRUE(graph_all.has_graph());
    EXPECT_FALSE(graph_non.is_graph());
    EXPECT_FALSE(graph_mix.is_graph());
    EXPECT_TRUE(graph_all.is_graph());

    ZString lower_non = "0A";
    ZString lower_mix = "Oa";
    FString lower_all = "oa";
    EXPECT_FALSE(lower_non.has_lower());
    EXPECT_TRUE(lower_mix.has_lower());
    EXPECT_TRUE(lower_all.has_lower());
    EXPECT_FALSE(lower_non.is_lower());
    EXPECT_FALSE(lower_mix.is_lower());
    EXPECT_TRUE(lower_all.is_lower());
    EXPECT_EQ("0a", lower_non.to_lower());
    EXPECT_EQ("oa", lower_mix.to_lower());
    EXPECT_EQ("oa", lower_all.to_lower());
    EXPECT_EQ(lower_all.begin(), lower_all.to_lower().begin());

    ZString upper_non = "0a";
    ZString upper_mix = "oA";
    FString upper_all = "OA";
    EXPECT_FALSE(upper_non.has_upper());
    EXPECT_TRUE(upper_mix.has_upper());
    EXPECT_TRUE(upper_all.has_upper());
    EXPECT_FALSE(upper_non.is_upper());
    EXPECT_FALSE(upper_mix.is_upper());
    EXPECT_TRUE(upper_all.is_upper());
    EXPECT_EQ("0A", upper_non.to_upper());
    EXPECT_EQ("OA", upper_mix.to_upper());
    EXPECT_EQ("OA", upper_all.to_upper());
    EXPECT_EQ(upper_all.begin(), upper_all.to_upper().begin());

    ZString alpha_non = " 0";
    ZString alpha_mix = "n ";
    FString alpha_all = "nA";
    EXPECT_FALSE(alpha_non.has_alpha());
    EXPECT_TRUE(alpha_mix.has_alpha());
    EXPECT_TRUE(alpha_all.has_alpha());
    EXPECT_FALSE(alpha_non.is_alpha());
    EXPECT_FALSE(alpha_mix.is_alpha());
    EXPECT_TRUE(alpha_all.is_alpha());

    ZString digit2_non = "a9";
    ZString digit2_mix = "20";
    FString digit2_all = "01";
    EXPECT_FALSE(digit2_non.has_digit2());
    EXPECT_TRUE(digit2_mix.has_digit2());
    EXPECT_TRUE(digit2_all.has_digit2());
    EXPECT_FALSE(digit2_non.is_digit2());
    EXPECT_FALSE(digit2_mix.is_digit2());
    EXPECT_TRUE(digit2_all.is_digit2());

    ZString digit8_non = "a9";
    ZString digit8_mix = "80";
    FString digit8_all = "37";
    EXPECT_FALSE(digit8_non.has_digit8());
    EXPECT_TRUE(digit8_mix.has_digit8());
    EXPECT_TRUE(digit8_all.has_digit8());
    EXPECT_FALSE(digit8_non.is_digit8());
    EXPECT_FALSE(digit8_mix.is_digit8());
    EXPECT_TRUE(digit8_all.is_digit8());

    ZString digit10_non = "az";
    ZString digit10_mix = "a9";
    FString digit10_all = "42";
    EXPECT_FALSE(digit10_non.has_digit10());
    EXPECT_TRUE(digit10_mix.has_digit10());
    EXPECT_TRUE(digit10_all.has_digit10());
    EXPECT_FALSE(digit10_non.is_digit10());
    EXPECT_FALSE(digit10_mix.is_digit10());
    EXPECT_TRUE(digit10_all.is_digit10());

    ZString digit16_non = "gz";
    ZString digit16_mix = "ao";
    FString digit16_all = "be";
    EXPECT_FALSE(digit16_non.has_digit16());
    EXPECT_TRUE(digit16_mix.has_digit16());
    EXPECT_TRUE(digit16_all.has_digit16());
    EXPECT_FALSE(digit16_non.is_digit16());
    EXPECT_FALSE(digit16_mix.is_digit16());
    EXPECT_TRUE(digit16_all.is_digit16());

    ZString alnum_non = " .";
    ZString alnum_mix = "n ";
    FString alnum_all = "n0";
    EXPECT_FALSE(alnum_non.has_alnum());
    EXPECT_TRUE(alnum_mix.has_alnum());
    EXPECT_TRUE(alnum_all.has_alnum());
    EXPECT_FALSE(alnum_non.is_alnum());
    EXPECT_FALSE(alnum_mix.is_alnum());
    EXPECT_TRUE(alnum_all.is_alnum());
}
