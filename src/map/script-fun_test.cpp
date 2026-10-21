//    Copyright 2026  Thorbjørn Lindeijer
//
//    This file is part of TMWA.
//
//    TMWA is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    TMWA is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with TMWA.  If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>

#include "script-parse.hpp"
#include "script-call.hpp"
#include "globals.hpp"
#include "../ast/script.hpp"
#include "../generic/intern-pool.hpp"

namespace tmwa
{
namespace map
{

namespace
{
// Runs a script snippet and returns the value it stored in $gtest_result.
int run_snippet(XString src)
{
    ast::script::ScriptBody body{RString(src), {}};
    std::unique_ptr<const ScriptBuffer> buf =
            compile_script("gtest snippet"_s, body, true);
    EXPECT_TRUE(buf != nullptr);
    run_script(ScriptPointer{Borrowed<const ScriptBuffer>(buf.get()), 0},
               wrap<BlockId>(0), wrap<BlockId>(0));
    return mapreg_db.get(SIR::from(variable_names.intern("$gtest_result"_s)));
}
} // anonymous namespace

TEST(getarraysize, empty_array_returns_zero)
{
    EXPECT_EQ(run_snippet(
            "{ set $gtest_result, getarraysize(.@gtest_empty); }"_s),
        0);
}

TEST(getarraysize, first_element_lands_at_zero)
{
    EXPECT_EQ(run_snippet(
            "{ set .@gtest_arr[0], 7;"
            "set $gtest_result, getarraysize(.@gtest_arr); }"_s),
        1);
}

TEST(getarraysize, reports_highest_index_plus_one)
{
    EXPECT_EQ(run_snippet(
            "{ set .@gtest_arr[4], 7;"
            "set .@gtest_arr[2], 8;"
            "set $gtest_result, getarraysize(.@gtest_arr); }"_s),
        5);
}

TEST(getarraysize, append_idiom_is_dense)
{
    EXPECT_EQ(run_snippet(
            "{ set .@gtest_arr[getarraysize(.@gtest_arr)], 10;"
            "set .@gtest_arr[getarraysize(.@gtest_arr)], 20;"
            "set $gtest_result, .@gtest_arr[0] + getarraysize(.@gtest_arr); }"_s),
        12);
}

TEST(getarraysize, string_array)
{
    EXPECT_EQ(run_snippet(
            "{ set $gtest_result, getarraysize(.@gtest_str$); }"_s),
        0);
    EXPECT_EQ(run_snippet(
            "{ set .@gtest_str$[0], \"x\";"
            "set $gtest_result, getarraysize(.@gtest_str$); }"_s),
        1);
}

} // namespace map
} // namespace tmwa
