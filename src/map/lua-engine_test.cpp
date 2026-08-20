#include "lua-engine.hpp"
//    lua-engine_test.cpp - unit tests for the Lua binding layer.
//
//    Copyright © 2026 The Mana World Development Team
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Covers doc/lua-engine.md section 14.1, restricted to what runs without a
// live map server (no maps, no sessions, no char link). Deliberately skipped
// here and left to the dialog/e2e tests:
//   - the dialog coroutine driver (mes/next/menu/input resumes) and anything
//     needing a map_session_data or npc_data instance
//   - check_player/check_npc/check_being handle round trips (need id_db
//     entries) and check_item against a populated item_db
//   - the memory cap (LUA_ERRMEM): provoking it means allocating hundreds of
//     MB inside the test, and lua_init() reads the limit before the fixture
//     could safely lower it
//   - lua_item_use / lua_item_equip execution (need a session and the
//     #itemdialog NPC); only compilation is tested here

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/mstring.hpp"
#include "../strings/rstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"
#include "../io/write.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "globals.hpp"
#include "map_conf.hpp"
#include "lua_conf.hpp"
#include "lua-callback.hpp"
#include "lua-internal.hpp"
#include "lua-item-scripts.hpp"
#include "lua-mapreg.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
namespace
{
bool contains(XString hay, XString needle)
{
    return std::search(hay.begin(), hay.end(),
            needle.begin(), needle.end()) != hay.end();
}

AString last_error;

// Load src under the sandbox env and pcall it for nresults results.
// On success the results stay on the stack; on failure the error message
// lands in last_error and the stack is balanced.
bool eval_multi(ZString src, int nresults)
{
    last_error = AString();
    lua_State* L = lua_state();
    if (!lua_load_chunk_sandboxed("=test"_s, src))
    {
        last_error = "compile error"_s;
        return false;
    }
    budget_push();
    int status = lua_pcall(L, 0, nresults, 0);
    budget_pop();
    if (status != LUA_OK)
    {
        size_t len;
        ZString z = luac::to_string(L, -1, &len);
        if (len)
            last_error = AString(XString(z));
        else
            last_error = "(non-string error object)"_s;
        lua_pop(L, 1);
        return false;
    }
    return true;
}

int eval_int(ZString src)
{
    lua_State* L = lua_state();
    if (!eval_multi(src, 1))
    {
        ADD_FAILURE() << "eval failed: " << last_error.c_str();
        return -999999;
    }
    int v = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return v;
}

bool eval_bool(ZString src)
{
    lua_State* L = lua_state();
    if (!eval_multi(src, 1))
    {
        ADD_FAILURE() << "eval failed: " << last_error.c_str();
        return false;
    }
    bool b = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return b;
}

AString eval_str(ZString src)
{
    lua_State* L = lua_state();
    if (!eval_multi(src, 1))
    {
        ADD_FAILURE() << "eval failed: " << last_error.c_str();
        return AString();
    }
    size_t len;
    ZString z = luac::to_string(L, -1, &len);
    AString out = AString(XString(z));
    lua_pop(L, 1);
    return out;
}

// Call a C function under pcall; the nargs arguments must already be on the
// stack. Returns the lua_pcall status; results (or the error) stay pushed.
int call_c(lua_State* L, lua_CFunction fn, int nargs, int nresults)
{
    luac::push_cfunction(L, fn, "test-fn");
    lua_insert(L, -(nargs + 1));
    return lua_pcall(L, nargs, nresults, 0);
}

AString pop_error(lua_State* L)
{
    size_t len;
    ZString z = luac::to_string(L, -1, &len);
    AString out = AString(XString(z));
    lua_pop(L, 1);
    return out;
}

int t_check_int(lua_State* L)
{
    return luac::push_int(L, check_int(L, 1));
}

int t_check_string(lua_State* L)
{
    ZString z = check_string(L, 1);
    luac::push_string(L, z);
    return 1;
}

int t_check_event(lua_State* L)
{
    NpcEvent ev = check_event(L, 1);
    push_string(L, ev.npc);
    push_string(L, ev.label);
    return 2;
}

AString slurp(ZString path)
{
    io::ReadFile in(path);
    if (!in.is_open())
        return AString();
    MString acc;
    AString line;
    while (in.getline(line))
    {
        acc += line;
        acc += '\n';
    }
    return AString(acc);
}

class LuaEngineTest : public ::testing::Test
{
protected:
    LuaConf saved_conf_;
    RString saved_mapreg_txt_;

    void SetUp() override
    {
        saved_conf_ = lua_conf;
        // small budget so the runaway-loop test is fast; every other test
        // stays far below it
        lua_conf.instruction_budget = 300000;
        saved_mapreg_txt_ = map_conf.mapreg_txt;
        mapreg_db.clear();
        mapregstr_db.clear();
        mapreg_dirty = 0;
        lua_init();
    }

    void TearDown() override
    {
        // every scenario must give its callback references back
        EXPECT_EQ(lua_live_refs(), 0);
        lua_set_check_only(false, false);
        lua_final();
        lua_conf = saved_conf_;
        map_conf.mapreg_txt = saved_mapreg_txt_;
        mapreg_db.clear();
        mapregstr_db.clear();
        mapreg_dirty = 0;
    }
};

// ------------------------------------------------------------------------
// integer policy (check_int: integer or integral float in int32, no string
// coercion)

TEST_F(LuaEngineTest, CheckIntAcceptsIntegersAndIntegralFloats)
{
    lua_State* L = lua_state();

    lua_pushinteger(L, 3);
    ASSERT_EQ(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_EQ(lua_tointeger(L, -1), 3);
    lua_pop(L, 1);

    lua_pushnumber(L, 3.0);
    ASSERT_EQ(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_EQ(lua_tointeger(L, -1), 3);
    lua_pop(L, 1);

    lua_pushinteger(L, 2147483647);
    ASSERT_EQ(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_EQ(lua_tointeger(L, -1), 2147483647);
    lua_pop(L, 1);

    lua_pushinteger(L, -2147483647 - 1);
    ASSERT_EQ(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_EQ(lua_tointeger(L, -1), -2147483647 - 1);
    lua_pop(L, 1);
}

TEST_F(LuaEngineTest, CheckIntRejectsFractionsStringsAndOverflow)
{
    lua_State* L = lua_state();

    lua_pushnumber(L, 3.5);
    ASSERT_NE(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "integer expected"_s));

    luac::push_string(L, "3"_s);
    ASSERT_NE(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "integer expected"_s));

    lua_pushinteger(L, static_cast<lua_Integer>(2147483648LL));
    ASSERT_NE(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "integer expected"_s));

    lua_pushinteger(L, static_cast<lua_Integer>(-2147483649LL));
    ASSERT_NE(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "integer expected"_s));

    lua_pushboolean(L, 1);
    ASSERT_NE(call_c(L, t_check_int, 1, 1), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "integer expected"_s));
}

TEST_F(LuaEngineTest, Int32Wraps)
{
    EXPECT_EQ(eval_int("return int32(2147483648)"_s), -2147483647 - 1);
    EXPECT_EQ(eval_int("return int32(4294967296)"_s), 0);
    EXPECT_EQ(eval_int("return int32(-2147483649)"_s), 2147483647);
    EXPECT_EQ(eval_int("return int32(2147483647 + 1)"_s), -2147483647 - 1);
    EXPECT_EQ(eval_int("return int32(7)"_s), 7);
    // fractional argument raises
    EXPECT_FALSE(eval_multi("return int32(1.5)"_s, 1));
    EXPECT_TRUE(contains(last_error, "integer expected"_s));
}

// ------------------------------------------------------------------------
// string round trips

TEST_F(LuaEngineTest, CheckStringLongRoundTrip)
{
    lua_State* L = lua_state();
    std::vector<char> big(100000, 'x');
    big[0] = 'a';
    big[big.size() - 1] = 'z';
    lua_pushlstring(L, big.data(), big.size());
    ASSERT_EQ(call_c(L, t_check_string, 1, 1), LUA_OK);
    size_t len;
    ZString z = luac::to_string(L, -1, &len);
    ASSERT_EQ(len, big.size());
    EXPECT_TRUE(std::equal(big.begin(), big.end(), z.begin()));
    lua_pop(L, 1);

    // and through the interpreter
    EXPECT_EQ(eval_int("return #string.rep('y', 65536)"_s), 65536);
}

TEST_F(LuaEngineTest, CheckStringRejectsEmbeddedNul)
{
    lua_State* L = lua_state();
    const char nasty[] = {'a', '\0', 'b'};
    lua_pushlstring(L, nasty, 3);
    ASSERT_NE(call_c(L, t_check_string, 1, 1), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "NUL"_s));

    // via a builtin that goes through check_string
    EXPECT_TRUE(eval_bool(
            "local ok = pcall(ord, string.char(65, 0, 66)) return not ok"_s));
}

TEST_F(LuaEngineTest, CheckEventParsesAndEnforcesLimits)
{
    lua_State* L = lua_state();

    luac::push_string(L, "Npc::OnLabel"_s);
    ASSERT_EQ(call_c(L, t_check_event, 1, 2), LUA_OK);
    {
        size_t len;
        ZString npc = luac::to_string(L, -2, &len);
        EXPECT_EQ(npc, "Npc"_s);
        ZString label = luac::to_string(L, -1, &len);
        EXPECT_EQ(label, "OnLabel"_s);
    }
    lua_pop(L, 2);

    // "::OnX" broadcast form: empty NPC part
    luac::push_string(L, "::OnX"_s);
    ASSERT_EQ(call_c(L, t_check_event, 1, 2), LUA_OK);
    {
        size_t len;
        ZString npc = luac::to_string(L, -2, &len);
        EXPECT_EQ(npc, ""_s);
    }
    lua_pop(L, 2);

    // no "::" separator raises
    luac::push_string(L, "NoSeparator"_s);
    ASSERT_NE(call_c(L, t_check_event, 1, 2), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "expected"_s));

    // NPC part longer than 23 bytes raises (VString<23> limit)
    luac::push_string(L, "abcdefghijklmnopqrstuvwx::OnX"_s);
    ASSERT_NE(call_c(L, t_check_event, 1, 2), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "too long"_s));

    // label part longer than 23 bytes raises
    luac::push_string(L, "Npc::abcdefghijklmnopqrstuvwx"_s);
    ASSERT_NE(call_c(L, t_check_event, 1, 2), LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "too long"_s));
}

// ------------------------------------------------------------------------
// the sandbox

TEST_F(LuaEngineTest, SandboxEscapeHatchesAbsent)
{
    LString escapes[] =
    {
        "return io"_s,
        "return require"_s,
        "return load"_s,
        "return loadstring"_s,
        "return dofile"_s,
        "return loadfile"_s,
        "return debug"_s,
        "return package"_s,
        "return rawequal2"_s,   // control: arbitrary unknown name
    };
    for (LString src : escapes)
    {
        EXPECT_FALSE(eval_multi(src, 1)) << ZString(src).c_str();
        EXPECT_TRUE(contains(last_error, "undefined global"_s))
                << last_error.c_str();
    }
}

TEST_F(LuaEngineTest, SandboxOsIsLimited)
{
    EXPECT_TRUE(eval_bool(
            "return type(os.time) == 'function'"
            " and type(os.clock) == 'function'"
            " and type(os.date) == 'function'"
            " and os.execute == nil"
            " and os.getenv == nil"
            " and os.remove == nil"
            " and os.rename == nil"
            " and os.exit == nil"
            " and os.tmpname == nil"_s));
    // os.date is forced to UTC and still works
    EXPECT_TRUE(eval_bool("return #os.date('%Y') == 4"_s));
}

TEST_F(LuaEngineTest, SandboxPrintIsRedirected)
{
    // print is the engine's C function (logs through PRINTF), not Lua's
    EXPECT_TRUE(eval_bool("return type(print) == 'function'"_s));
    EXPECT_TRUE(eval_multi("print('lua-engine_test print check', 1, nil)"_s, 0));
}

TEST_F(LuaEngineTest, StrictGlobalsRaiseOnUnknownRead)
{
    EXPECT_FALSE(eval_multi("return no_such_global_xyz"_s, 1));
    EXPECT_TRUE(contains(last_error, "undefined global 'no_such_global_xyz'"_s))
            << last_error.c_str();
}

TEST_F(LuaEngineTest, NewGlobalsAllowedWhileLoading)
{
    // g_loading is true right after lua_init(): content files may define
    // new globals freely
    EXPECT_TRUE(lua_loading());
    EXPECT_EQ(eval_int("some_new_global_ok = 17 return some_new_global_ok"_s), 17);
}

TEST_F(LuaEngineTest, NewGlobalAfterStartupRaisesInCheckMode)
{
    lua_loading_done();
    lua_set_check_only(true, false);
    EXPECT_FALSE(eval_multi("late_global_xyz = 1"_s, 0));
    EXPECT_TRUE(contains(last_error, "new global"_s)) << last_error.c_str();
    // without check mode it warns but succeeds
    lua_set_check_only(false, false);
    EXPECT_EQ(eval_int("warned_global_xyz = 5 return warned_global_xyz"_s), 5);
}

TEST_F(LuaEngineTest, CollectgarbageRestrictedToCount)
{
    lua_State* L = lua_state();
    ASSERT_TRUE(eval_multi("return collectgarbage('count')"_s, 1));
    EXPECT_EQ(lua_type(L, -1), LUA_TNUMBER);
    EXPECT_GT(lua_tonumber(L, -1), 0.0);
    lua_pop(L, 1);

    EXPECT_FALSE(eval_multi("collectgarbage('stop')"_s, 0));
    EXPECT_TRUE(contains(last_error, "restricted"_s));
    EXPECT_FALSE(eval_multi("collectgarbage('collect')"_s, 0));
    EXPECT_FALSE(eval_multi("collectgarbage()"_s, 0));
}

TEST_F(LuaEngineTest, StopSentinelEndsHandlerWithoutError)
{
    ASSERT_TRUE(lua_load_chunk_sandboxed("=stoptest"_s, "stop()"_s));
    LuaCtx ctx;
    ctx.what = "stoptest";
    // stop() terminates the handler but the driver reports success
    EXPECT_TRUE(lua_run_sync(ctx, 0));
}

// ------------------------------------------------------------------------
// bit32

TEST_F(LuaEngineTest, Bit32KnownVectors)
{
    struct Case
    {
        LString expr;
        int expected;
    };
    Case cases[] =
    {
        {"return bit32.band(0xF0F0, 0x0FF0)"_s, 0x00F0},
        {"return bit32.band()"_s, -1},
        {"return bit32.band(0x7fffffff, -1)"_s, 0x7fffffff},
        {"return bit32.bor(1, 2, 4)"_s, 7},
        {"return bit32.bor()"_s, 0},
        {"return bit32.bxor(0xFF, 0x0F)"_s, 0xF0},
        {"return bit32.bnot(0)"_s, -1},
        {"return bit32.bnot(-1)"_s, 0},
        {"return bit32.lshift(1, 31)"_s, -2147483647 - 1},
        {"return bit32.lshift(1, 32)"_s, 0},
        {"return bit32.lshift(1, -1)"_s, 0},
        {"return bit32.lshift(16, -3)"_s, 2},
        {"return bit32.rshift(-1, 28)"_s, 15},
        {"return bit32.rshift(1, -4)"_s, 16},
        {"return bit32.rshift(-1, 32)"_s, 0},
        {"return bit32.arshift(-16, 2)"_s, -4},
        {"return bit32.arshift(-1, 40)"_s, -1},
        {"return bit32.arshift(1, 40)"_s, 0},
        {"return bit32.arshift(16, 2)"_s, 4},
        {"return bit32.extract(0xABCD, 4, 8)"_s, 0xBC},
        {"return bit32.extract(-2147483648, 31)"_s, 1},
        {"return bit32.extract(-1, 0, 32)"_s, -1},
        {"return bit32.replace(0, 0xFF, 8, 8)"_s, 0xFF00},
        {"return bit32.replace(-1, 0, 0, 16)"_s, -65536},
        {"return bit32.replace(0, -1, 0, 32)"_s, -1},
    };
    for (Case& c : cases)
        EXPECT_EQ(eval_int(c.expr), c.expected) << ZString(c.expr).c_str();

    EXPECT_TRUE(eval_bool("return bit32.btest(4, 4)"_s));
    EXPECT_FALSE(eval_bool("return bit32.btest(1, 2)"_s));
    EXPECT_TRUE(eval_bool("return bit32.btest()"_s));

    // out-of-range field/width raise
    EXPECT_FALSE(eval_multi("return bit32.extract(0, 30, 8)"_s, 1));
    EXPECT_TRUE(contains(last_error, "non-existent bits"_s));
    EXPECT_FALSE(eval_multi("return bit32.extract(0, -1)"_s, 1));
    EXPECT_FALSE(eval_multi("return bit32.replace(0, 0, 0, 0)"_s, 1));
}

// ------------------------------------------------------------------------
// idiv / imod (C truncation semantics)

TEST_F(LuaEngineTest, IdivImodTruncateTowardZero)
{
    EXPECT_EQ(eval_int("return idiv(7, 2)"_s), 3);
    EXPECT_EQ(eval_int("return idiv(-7, 2)"_s), -3);
    EXPECT_EQ(eval_int("return idiv(7, -2)"_s), -3);
    EXPECT_EQ(eval_int("return idiv(-7, -2)"_s), 3);
    EXPECT_EQ(eval_int("return idiv(6.0, 2)"_s), 3);
    EXPECT_EQ(eval_int("return idiv(-2147483648, -1)"_s), -2147483647 - 1);

    EXPECT_EQ(eval_int("return imod(7, 2)"_s), 1);
    EXPECT_EQ(eval_int("return imod(-7, 2)"_s), -1);
    EXPECT_EQ(eval_int("return imod(7, -2)"_s), 1);
    EXPECT_EQ(eval_int("return imod(-7, -2)"_s), -1);
    EXPECT_EQ(eval_int("return imod(-2147483648, -1)"_s), 0);
}

TEST_F(LuaEngineTest, IdivImodZeroDivisorRaises)
{
    EXPECT_FALSE(eval_multi("return idiv(1, 0)"_s, 1));
    EXPECT_TRUE(contains(last_error, "division by zero"_s));
    EXPECT_FALSE(eval_multi("return imod(1, 0)"_s, 1));
    EXPECT_TRUE(contains(last_error, "division by zero"_s));
    // fractional argument raises before the divisor check
    EXPECT_FALSE(eval_multi("return idiv(3.5, 1)"_s, 1));
    EXPECT_TRUE(contains(last_error, "integer expected"_s));
    // no string coercion
    EXPECT_FALSE(eval_multi("return idiv('3', 1)"_s, 1));
    EXPECT_TRUE(contains(last_error, "integer expected"_s));
}

// ------------------------------------------------------------------------
// constants (const_db format: name value [param-flag])

TEST_F(LuaEngineTest, ConstDbLoadsConstantsAndParams)
{
    {
        io::WriteFile out("lua-test-constdb.txt"_s);
        ASSERT_TRUE(out.is_open());
        out.put_line("// a comment line"_s);
        out.put_line(""_s);
        out.put_line("TESTK 42"_s);
        out.put_line("NEGK -7"_s);
        out.put_line("Vit 8 1"_s);
        out.put_line("ZEROFLAG 5 0"_s);
        ASSERT_TRUE(out.close());
    }
    ASSERT_TRUE(lua_read_constdb("lua-test-constdb.txt"_s));

    EXPECT_EQ(eval_int("return TESTK"_s), 42);
    EXPECT_EQ(eval_int("return NEGK"_s), -7);
    EXPECT_EQ(eval_int("return TESTK + NEGK"_s), 35);
    EXPECT_EQ(eval_int("return ZEROFLAG"_s), 5);

    // third-column-nonzero entries are params, not globals; the error
    // message points at the handle property
    EXPECT_FALSE(eval_multi("return Vit"_s, 1));
    EXPECT_TRUE(contains(last_error, "did you mean p.Vit"_s))
            << last_error.c_str();
    int sp = 0;
    EXPECT_TRUE(lua_param_lookup("Vit"_s, &sp));
    EXPECT_EQ(sp, 8);
    EXPECT_FALSE(lua_param_lookup("NotAParam"_s, &sp));

    // constants are read-only
    EXPECT_FALSE(eval_multi("TESTK = 1"_s, 0));
    EXPECT_TRUE(contains(last_error, "attempt to assign constant"_s))
            << last_error.c_str();
}

TEST_F(LuaEngineTest, ConstDbRejectsBadLines)
{
    {
        io::WriteFile out("lua-test-constdb-bad.txt"_s);
        ASSERT_TRUE(out.is_open());
        out.put_line("GOODK 1"_s);
        out.put_line("BAD-NAME 3"_s);
        ASSERT_TRUE(out.close());
    }
    EXPECT_FALSE(lua_read_constdb("lua-test-constdb-bad.txt"_s));
    // the good line before the bad one still loads
    EXPECT_EQ(eval_int("return GOODK"_s), 1);

    EXPECT_FALSE(lua_read_constdb("lua-test-no-such-file.txt"_s));
}

// ------------------------------------------------------------------------
// LuaCallback reference accounting

TEST_F(LuaEngineTest, CallbackRefAccounting)
{
    lua_State* L = lua_state();
    ASSERT_EQ(lua_live_refs(), 0);

    ASSERT_TRUE(eval_multi(
            "return function(self, p, args)"
            " worldtmp.cbfired = worldtmp.cbfired + 1 end"_s, 1));
    LuaCallback cb = lua_cb_from_stack(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(lua_live_refs(), 1);
    EXPECT_NE(cb.fn_ref, lua_noref);

    LuaCallback cb2 = lua_cb_dup(cb);
    EXPECT_EQ(lua_live_refs(), 2);

    lua_cb_release(cb2);
    EXPECT_EQ(lua_live_refs(), 1);
    // release is idempotent
    lua_cb_release(cb2);
    EXPECT_EQ(lua_live_refs(), 1);

    // firing consumes the reference and runs the function (no session:
    // self and p are nil inside the handler)
    LuaCallback cb3 = lua_cb_dup(cb);
    EXPECT_EQ(lua_live_refs(), 2);
    lua_cb_fire(cb3, dumb_ptr<map_session_data>(), LuaArgs::none());
    EXPECT_EQ(lua_live_refs(), 1);
    EXPECT_EQ(eval_int("return worldtmp.cbfired"_s), 1);

    lua_cb_release(cb);
    EXPECT_EQ(lua_live_refs(), 0);
}

TEST_F(LuaEngineTest, CallbackNamedFormTakesNoRef)
{
    lua_State* L = lua_state();
    luac::push_string(L, "SomeNpc::OnFoo"_s);
    LuaCallback cb = lua_cb_from_stack(L, -1);
    lua_pop(L, 1);
    EXPECT_EQ(lua_live_refs(), 0);
    EXPECT_EQ(cb.fn_ref, lua_noref);
    EXPECT_TRUE(bool(cb));
    NpcEvent ev = cb.event;
    EXPECT_EQ(ev.npc, stringish<NpcName>("SomeNpc"_s));
    lua_cb_release(cb);   // no-op for the named form
    EXPECT_EQ(lua_live_refs(), 0);

    LuaCallback none = lua_cb_named(NpcEvent());
    EXPECT_FALSE(bool(none));
}

// ------------------------------------------------------------------------
// mapreg (world proxies + save/load round trip)

TEST_F(LuaEngineTest, MapregProxyIntStrArrayAndDeleteOnZero)
{
    size_t base_ints = mapreg_db.size();
    size_t base_strs = mapregstr_db.size();

    // int namespace, index 0
    EXPECT_TRUE(eval_multi("world.DELME = 7"_s, 0));
    EXPECT_EQ(mapreg_db.size(), base_ints + 1);
    EXPECT_EQ(eval_int("return world.DELME"_s), 7);
    EXPECT_EQ(lua_mapreg_get_int("DELME"_s, 0), 7);
    // writing 0 deletes the entry
    EXPECT_TRUE(eval_multi("world.DELME = 0"_s, 0));
    EXPECT_EQ(mapreg_db.size(), base_ints);
    EXPECT_EQ(eval_int("return world.DELME"_s), 0);

    // string namespace
    EXPECT_TRUE(eval_multi("world.str.DELMES = 'hello'"_s, 0));
    EXPECT_EQ(mapregstr_db.size(), base_strs + 1);
    EXPECT_EQ(eval_str("return world.str.DELMES"_s), "hello"_s);
    // writing "" deletes the entry
    EXPECT_TRUE(eval_multi("world.str.DELMES = ''"_s, 0));
    EXPECT_EQ(mapregstr_db.size(), base_strs);
    EXPECT_EQ(eval_str("return world.str.DELMES"_s), ""_s);

    // world.get / world.set with explicit indices
    EXPECT_TRUE(eval_multi("world.set('WIDX', 5, 123)"_s, 0));
    EXPECT_EQ(eval_int("return world.get('WIDX', 5)"_s), 123);
    EXPECT_EQ(lua_mapreg_get_int("WIDX"_s, 5), 123);
    EXPECT_TRUE(eval_multi("world.set('WIDX', 5, 0)"_s, 0));
    EXPECT_EQ(mapreg_db.size(), base_ints);

    // int array proxy
    lua_State* L = lua_state();
    ASSERT_TRUE(eval_multi(
            "local a = world.array('ARR')\n"
            "a[3] = 7\n"
            "local v, s1 = a[3], a:size()\n"
            "a[10] = 1\n"
            "local s2 = a:size()\n"
            "a:clear(0, 256)\n"
            "return v, s1, s2, a:size()"_s, 4));
    EXPECT_EQ(lua_tointeger(L, -4), 7);
    EXPECT_EQ(lua_tointeger(L, -3), 4);
    EXPECT_EQ(lua_tointeger(L, -2), 11);
    EXPECT_EQ(lua_tointeger(L, -1), 0);
    lua_pop(L, 4);
    EXPECT_EQ(mapreg_db.size(), base_ints);

    // string array proxy ("NAME$")
    ASSERT_TRUE(eval_multi(
            "local a = world.array('SARR$')\n"
            "a[2] = 'hi'\n"
            "local v, s1 = a[2], a:size()\n"
            "a[2] = ''\n"
            "return v, s1, a:size()"_s, 3));
    {
        size_t len;
        EXPECT_EQ(luac::to_string(L, -3, &len), "hi"_s);
    }
    EXPECT_EQ(lua_tointeger(L, -2), 3);
    EXPECT_EQ(lua_tointeger(L, -1), 0);
    lua_pop(L, 3);
    EXPECT_EQ(mapregstr_db.size(), base_strs);

    // invalid names and out-of-range indices raise
    EXPECT_FALSE(eval_multi("return world['BAD$']"_s, 1));
    EXPECT_FALSE(eval_multi("world['bad name'] = 1"_s, 0));
    EXPECT_FALSE(eval_multi("return world.get('OK', 256)"_s, 1));
    EXPECT_FALSE(eval_multi("world.array('no spaces allowed')"_s, 0));
}

TEST_F(LuaEngineTest, MapregSaveLoadRoundTripsByteIdentically)
{
    map_conf.mapreg_txt = "lua-test-mapreg-a.txt"_s;

    EXPECT_TRUE(eval_multi(
            "world.TESTVAR = 5\n"
            "world.set('TESTARR', 3, 7)\n"
            "world.str.TESTSTR = 'hello world'\n"
            "world.set('TESTSARR$', 2, 'there')"_s, 0));
    // "$@..." temporaries are never saved
    lua_mapreg_set_int("@TMPONLY"_s, 0, 9);

    mapreg_final();   // saves (dirty)

    AString first = slurp("lua-test-mapreg-a.txt"_s);
    // old format: name[,idx]<TAB>value, ints first, then strings
    EXPECT_TRUE(contains(first, "$TESTVAR\t5\n"_s)) << first.c_str();
    EXPECT_TRUE(contains(first, "$TESTARR,3\t7\n"_s)) << first.c_str();
    EXPECT_TRUE(contains(first, "$TESTSTR$\thello world\n"_s)) << first.c_str();
    EXPECT_TRUE(contains(first, "$TESTSARR$,2\tthere\n"_s)) << first.c_str();
    EXPECT_FALSE(contains(first, "TMPONLY"_s)) << first.c_str();

    // wipe the in-memory store, reload from the file
    mapreg_db.clear();
    mapregstr_db.clear();
    EXPECT_EQ(eval_int("return world.TESTVAR"_s), 0);
    mapreg_init();
    EXPECT_EQ(eval_int("return world.TESTVAR"_s), 5);
    EXPECT_EQ(eval_int("return world.get('TESTARR', 3)"_s), 7);
    EXPECT_EQ(eval_str("return world.str.TESTSTR"_s), "hello world"_s);
    EXPECT_EQ(eval_str("return world.get('TESTSARR$', 2)"_s), "there"_s);

    // save the reloaded store to a second file: byte-identical
    map_conf.mapreg_txt = "lua-test-mapreg-b.txt"_s;
    mapreg_final();
    AString second = slurp("lua-test-mapreg-b.txt"_s);
    EXPECT_EQ(first, second);
    EXPECT_TRUE(bool(first));
}

// ------------------------------------------------------------------------
// item script compilation

TEST_F(LuaEngineTest, ItemScriptCompiles)
{
    lua_State* L = lua_state();
    int ref = lua_noref;
    ASSERT_TRUE(lua_compile_item_script(
            "worldtmp.itemran = worldtmp.itemran + 1"_s,
            wrap<ItemNameId>(static_cast<uint16_t>(999)), false, &ref));
    ASSERT_NE(ref, lua_noref);
    luac::push_ref(L, ref);
    EXPECT_TRUE(lua_isfunction(L, -1));
    // the chunk starts with "local p, args = ..." and runs sandboxed
    budget_push();
    int status = lua_pcall(L, 0, 0, 0);
    budget_pop();
    EXPECT_EQ(status, LUA_OK);
    EXPECT_EQ(eval_int("return worldtmp.itemran"_s), 1);
    luac::unref(L, ref);
}

TEST_F(LuaEngineTest, ItemScriptEmptyBodyIsSkipped)
{
    int ref = 12345;
    EXPECT_TRUE(lua_compile_item_script(""_s,
            wrap<ItemNameId>(static_cast<uint16_t>(998)), false, &ref));
    EXPECT_EQ(ref, lua_noref);
    ref = 12345;
    EXPECT_TRUE(lua_compile_item_script("  \n\t \n"_s,
            wrap<ItemNameId>(static_cast<uint16_t>(998)), true, &ref));
    EXPECT_EQ(ref, lua_noref);
}

TEST_F(LuaEngineTest, ItemScriptSyntaxErrorFailsCompile)
{
    int ref = 12345;
    EXPECT_FALSE(lua_compile_item_script("local = 3"_s,
            wrap<ItemNameId>(static_cast<uint16_t>(997)), false, &ref));
    EXPECT_EQ(ref, lua_noref);
}

// ------------------------------------------------------------------------
// the prelude menu table form (pure Lua: fake p and a cmenu stub)

TEST_F(LuaEngineTest, PreludeMenuTableForm)
{
    lua_State* L = lua_state();
    LString src = R"lua(
local M = ...
local log = {}
local p = { title = function(self, t) log.title = t end }
local function mkmenu(ret)
    return function(pp, display, nsent, cancelable)
        log.display = display
        log.nsent = nsent
        log.cancelable = cancelable
        return ret
    end
end
-- empty-string and false entries are skipped but keep their positions
local r1 = M.menu_table(p, {"a", "", "b", false, "c", title = "T"}, mkmenu(2))
local d1, n1, t1 = log.display, log.nsent, log.title
-- pair entries return the pair value plus the original index
local r2a, r2b = M.menu_table(p, {{"x", 10}, {"y", 20}}, mkmenu(2))
-- client cancel (cmenu returns nil) returns tbl.cancel
local r3 = M.menu_table(p, {"a", "b", cancel = 99}, mkmenu(nil))
local c3 = log.cancelable
-- without a cancel key the menu is not cancelable
M.menu_table(p, {"a"}, mkmenu(1))
local c4 = log.cancelable
-- no selectable entries is an error
local ok5 = pcall(M.menu_table, p, {"", false}, mkmenu(1))
-- a non-string entry is an error
local ok6, err6 = pcall(M.menu_table, p, {42}, mkmenu(1))
return r1, d1, n1, t1, r2a, r2b, r3, c3, c4, ok5, ok6, err6
)lua"_s;

    ASSERT_TRUE(lua_load_chunk_sandboxed("=menutest"_s, src));
    lua_push_engine_table(L, LuaTable::PRELUDE);
    budget_push();
    int status = lua_pcall(L, 1, 12, 0);
    budget_pop();
    if (status != LUA_OK)
        FAIL() << "menu chunk failed: " << pop_error(L).c_str();
    int base = lua_gettop(L) - 12;
    size_t len;
    EXPECT_EQ(lua_tointeger(L, base + 1), 3);                        // r1: origin of "b"
    EXPECT_EQ(luac::to_string(L, base + 2, &len), "a:b:c:"_s);       // d1
    EXPECT_EQ(lua_tointeger(L, base + 3), 3);                        // n1: 3 sent
    EXPECT_EQ(luac::to_string(L, base + 4, &len), "T"_s);            // t1: title
    EXPECT_EQ(lua_tointeger(L, base + 5), 20);                       // r2a: pair value
    EXPECT_EQ(lua_tointeger(L, base + 6), 2);                        // r2b: origin
    EXPECT_EQ(lua_tointeger(L, base + 7), 99);                       // r3: cancel value
    EXPECT_TRUE(lua_toboolean(L, base + 8));                         // c3: cancelable
    EXPECT_FALSE(lua_toboolean(L, base + 9));                        // c4: not cancelable
    EXPECT_FALSE(lua_toboolean(L, base + 10));                       // ok5: errored
    EXPECT_FALSE(lua_toboolean(L, base + 11));                       // ok6: errored
    EXPECT_TRUE(contains(XString(luac::to_string(L, base + 12, &len)),
            "menu entry 1"_s));
    lua_settop(L, base);
}

// ------------------------------------------------------------------------
// instruction budget

TEST_F(LuaEngineTest, RunawayLoopAbortsAndStateStaysUsable)
{
    int aborts_before = lua_budget_aborts();

    // through the raw pcall path: the hook error is catchable
    ASSERT_TRUE(lua_load_chunk_sandboxed("=runaway"_s,
            "local x = 0 while true do x = x + 1 end"_s));
    lua_State* L = lua_state();
    budget_push();
    int status = lua_pcall(L, 0, 0, 0);
    budget_pop();
    EXPECT_NE(status, LUA_OK);
    EXPECT_TRUE(contains(pop_error(L), "instruction budget exhausted"_s));
    EXPECT_EQ(lua_budget_aborts(), aborts_before + 1);

    // through the sync driver: reported as a failed handler
    ASSERT_TRUE(lua_load_chunk_sandboxed("=runaway2"_s,
            "while true do end"_s));
    LuaCtx ctx;
    ctx.what = "budget-test";
    EXPECT_FALSE(lua_run_sync(ctx, 0));
    EXPECT_EQ(lua_budget_aborts(), aborts_before + 2);

    // the state is fully usable afterwards
    EXPECT_EQ(eval_int("return 41 + 1"_s), 42);
    EXPECT_EQ(lua_gettop(L), 0);
}
} // namespace
} // namespace map
} // namespace tmwa
