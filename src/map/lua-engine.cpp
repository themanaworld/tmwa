#include "lua-engine.hpp"
//    lua-engine.cpp - Lua state lifetime, sandbox, budgets, and the two drivers.
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

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/mstring.hpp"
#include "../strings/vstring.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/read.hpp"

#include "globals.hpp"
#include "map.hpp"
#include "lua_conf.hpp"
#include "lua-callback.hpp"
#include "lua-dialog.hpp"
#include "lua-events.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"
#include "lua-libs.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
LuaConf lua_conf;

// how often the count hook fires (VM instructions)
constexpr int BUDGET_CHECK_EVERY = 1000;

// ------------------------------------------------------------------------
// engine-private state (doc/lua-engine.md section 2.3; deliberately not in
// globals.hpp)

static
lua_State* g_L = nullptr;

constexpr int LUA_TABLE_COUNT = 11;
static
int g_table_refs[LUA_TABLE_COUNT] =
{
    lua_noref, lua_noref, lua_noref, lua_noref, lua_noref, lua_noref,
    lua_noref, lua_noref, lua_noref, lua_noref, lua_noref,
};
static
int g_stop_ref = lua_noref;
static
int g_thread_ctx_ref = lua_noref;

static
std::vector<LuaCtx> g_ctx;
static
std::vector<int> g_budget;
static
int g_budget_aborts = 0;

static
bool g_loading = false;
static
bool g_check_only_flag = false;
static
bool g_check_skip_oninit_flag = false;

lua_State* lua_state()
{
    return g_L;
}

void lua_push_engine_table(lua_State* L, LuaTable which)
{
    luac::push_ref(L, g_table_refs[static_cast<int>(which)]);
}

bool lua_loading()
{
    return g_loading;
}

void lua_loading_done()
{
    g_loading = false;
}

void lua_set_check_only(bool enable, bool skip_oninit)
{
    g_check_only_flag = enable;
    g_check_skip_oninit_flag = skip_oninit;
}

bool lua_check_only()
{
    return g_check_only_flag;
}

bool lua_check_skip_oninit()
{
    return g_check_skip_oninit_flag;
}

size_t lua_mem_used()
{
    return lua_alloc_used();
}

int lua_budget_aborts()
{
    return g_budget_aborts;
}

// ------------------------------------------------------------------------
// context stack

void lua_ctx_push(LuaCtx ctx)
{
    g_ctx.push_back(ctx);
}

void lua_ctx_pop()
{
    if (!g_ctx.empty())
        g_ctx.pop_back();
}

LuaCtx lua_current_ctx()
{
    if (g_ctx.empty())
        return LuaCtx();
    return g_ctx.back();
}

// The dialog coroutine's ctx, re-pushed on every resume. Keyed by the thread
// in a weak-keyed registry table; `what` is stored as light userdata because
// it is always a static string literal.
void lua_thread_ctx_set(lua_State* T, LuaCtx ctx)
{
    lua_State* L = g_L;
    luac::push_ref(L, g_thread_ctx_ref);
    lua_pushthread(T);
    lua_xmove(T, L, 1);
    lua_createtable(L, 0, 3);
    lua_pushinteger(L, unwrap<BlockId>(ctx.npc));
    lua_setfield(L, -2, "npc");
    lua_pushinteger(L, unwrap<BlockId>(ctx.player));
    lua_setfield(L, -2, "player");
    lua_pushlightuserdata(L, const_cast<char*>(ctx.what));
    lua_setfield(L, -2, "what");
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

bool lua_thread_ctx_get(lua_State* T, LuaCtx* out)
{
    lua_State* L = g_L;
    luac::push_ref(L, g_thread_ctx_ref);
    lua_pushthread(T);
    lua_xmove(T, L, 1);
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 2);
        return false;
    }
    LuaCtx ctx;
    lua_getfield(L, -1, "npc");
    ctx.npc = wrap<BlockId>(static_cast<uint32_t>(lua_tointeger(L, -1)));
    lua_pop(L, 1);
    lua_getfield(L, -1, "player");
    ctx.player = wrap<BlockId>(static_cast<uint32_t>(lua_tointeger(L, -1)));
    lua_pop(L, 1);
    lua_getfield(L, -1, "what");
    if (lua_islightuserdata(L, -1))
        ctx.what = static_cast<const char*>(lua_touserdata(L, -1));
    lua_pop(L, 3);
    *out = ctx;
    return true;
}

void lua_thread_ctx_clear(lua_State* T)
{
    lua_State* L = g_L;
    luac::push_ref(L, g_thread_ctx_ref);
    lua_pushthread(T);
    lua_xmove(T, L, 1);
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

// ------------------------------------------------------------------------
// instruction budget

static
void budget_hook(lua_State* L)
{
    if (g_budget.empty())
        return;
    g_budget.back() -= BUDGET_CHECK_EVERY;
    if (g_budget.back() <= 0)
    {
        ++g_budget_aborts;
        luaL_error(L, "instruction budget exhausted (%d VM instructions without yielding)",
                lua_conf.instruction_budget);
    }
}

void budget_push()
{
    g_budget.push_back(lua_conf.instruction_budget);
}

void budget_pop()
{
    if (!g_budget.empty())
        g_budget.pop_back();
}

// ------------------------------------------------------------------------
// stop() sentinel and the traceback message handler

bool lua_error_is_stop(lua_State* L, int idx)
{
    int abs = lua_absindex(L, idx);
    luac::push_ref(L, g_stop_ref);
    bool eq = lua_rawequal(L, abs, -1);
    lua_pop(L, 1);
    return eq;
}

static
int msgh(lua_State* L)
{
    // the stop() sentinel passes through untouched (no traceback, no log)
    if (lua_error_is_stop(L, 1))
        return 1;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        luac::traceback(L, L, lua_tostring(L, 1));
        return 1;
    }
    // non-string error object: describe it, then append the traceback
    luaL_tolstring(L, 1, nullptr);
    luac::traceback(L, L, lua_tostring(L, -1));
    return 1;
}

void lua_report_error(lua_State* failed, LuaCtx ctx)
{
    AString msg;
    {
        if (lua_type(failed, -1) != LUA_TSTRING)
            luaL_tolstring(failed, -1, nullptr);
        else
            lua_pushvalue(failed, -1);
        size_t len;
        ZString z = luac::to_string(failed, -1, &len);
        msg = AString(XString(z));
        lua_pop(failed, 1);
    }
    if (failed != g_L)
    {
        // errors from a coroutine resume carry no traceback yet
        luac::traceback(g_L, failed, msg.c_str());
        size_t len;
        ZString z = luac::to_string(g_L, -1, &len);
        msg = AString(XString(z));
        lua_pop(g_L, 1);
    }
    AString npc_name;
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(ctx.npc);
        if (nd != nullptr)
            npc_name = AString(nd->name);
    }
    PRINTF("lua: error in %s (npc=%s player=%d):\n%s\n"_fmt,
            ZString(strings::really_construct_from_a_pointer, ctx.what, nullptr),
            npc_name, unwrap<BlockId>(ctx.player), msg);
    lua_pop(failed, 1);
}

// ------------------------------------------------------------------------
// the two drivers (doc/lua-engine.md section 3)

bool lua_run_sync(LuaCtx ctx, int nargs)
{
    lua_State* L = g_L;
    int base = lua_gettop(L) - nargs;   // stack slot of the function
    luac::push_cfunction(L, msgh, "traceback");
    lua_insert(L, base);
    lua_ctx_push(ctx);
    budget_push();
    int status = lua_pcall(L, nargs, 0, base);
    budget_pop();
    lua_ctx_pop();
    bool ok = (status == LUA_OK);
    if (!ok)
    {
        if (lua_error_is_stop(L, -1))
        {
            ok = true;
            lua_pop(L, 1);
        }
        else
        {
            lua_report_error(L, ctx);
        }
    }
    lua_remove(L, base);   // the message handler
    return ok;
}

void lua_run_dialog(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> dialog_npc,
        LuaCtx ctx, int nargs)
{
    lua_State* L = g_L;
    if (sd == nullptr || dialog_npc == nullptr)
    {
        lua_pop(L, nargs + 1);
        return;
    }
    // precondition: no coroutine yet (callers check sd->npc_id first)
    if (sd->lua.thread_ref != lua_noref)
    {
        lua_warn("dialog driver: session already has a coroutine (handler dropped)"_s);
        lua_pop(L, nargs + 1);
        return;
    }
    // callers set npc_id on the click/event paths; assert-and-heal here
    if (sd->npc_id != dialog_npc->bl_id)
    {
        lua_warn("dialog driver: npc_id was not set by the caller"_s);
        sd->npc_id = dialog_npc->bl_id;
    }
    sd->lua.dialog_mes = false;
    sd->lua.prompt = LuaPrompt::NONE;
    sd->lua.menu_count = 0;

    lua_State* T = lua_newthread(L);
    // 5.4 hooks are per thread: the new coroutine needs its own budget hook
    luac::set_instruction_hook(T, budget_hook, BUDGET_CHECK_EVERY);
    lua_insert(L, -(nargs + 2));        // [T, func, args...]
    lua_xmove(L, T, nargs + 1);         // L: [T]; T: [func, args...]
    lua_pushvalue(L, -1);
    sd->lua.thread_ref = luac::ref(L);  // pops the copy
    // the original T stays on L's stack as the resume holder: the registry
    // ref may be released during the run (self:destroy() -> dequeue ->
    // abandon), but a running coroutine must stay anchored
    lua_thread_ctx_set(T, ctx);

    int nres = 0;
    sd->lua.running = true;
    lua_ctx_push(ctx);
    budget_push();
    int status = luac::resume(T, L, nargs, &nres);
    budget_pop();
    lua_ctx_pop();
    sd->lua.running = false;
    if (status == LUA_YIELD && nres > 0)
        lua_pop(T, nres);   // yield values are not used

    lua_dialog_after_resume(sd, status);
    lua_pop(L, 1);          // the resume holder
}

// ------------------------------------------------------------------------
// constants

bool lua_read_constdb(ZString filename)
{
    io::ReadFile in(filename);
    if (!in.is_open())
    {
        PRINTF("can't read %s\n"_fmt, filename);
        return false;
    }

    lua_State* L = g_L;
    bool rv = true;
    AString line_;
    while (in.getline(line_))
    {
        // same parse as the old read_constdb (script-startup.cpp): whole-line
        // '//' comments, "name value [param-flag]"
        LString comment = "//"_s;
        XString line = line_.xislice_h(std::search(line_.begin(), line_.end(),
                    comment.begin(), comment.end())).rstrip();
        if (!line)
            continue;

        auto _it = std::find(line.begin(), line.end(), ' ');
        auto name = line.xislice_h(_it);
        auto _rest = line.xislice_t(_it);
        while (_rest.startswith(' '))
            _rest = _rest.xslice_t(1);
        auto _it2 = std::find(_rest.begin(), _rest.end(), ' ');
        auto val_ = _rest.xislice_h(_it2);
        auto type_ = _rest.xislice_t(_it2);
        while (type_.startswith(' '))
            type_ = type_.xslice_t(1);

        int val;
        int type = 0;
        if (std::find_if_not(name.begin(), name.end(),
                    [](char c)
                    {
                        return ('0' <= c && c <= '9')
                            || ('A' <= c && c <= 'Z')
                            || ('a' <= c && c <= 'z')
                            || (c == '_');
                    }) != name.end()
                || !extract(val_, &val)
                || (!extract(type_, &type) && type_))
        {
            PRINTF("Bad const line: %s\n"_fmt, line_);
            rv = false;
            continue;
        }
        // third column nonzero: a param name (property on handles, not a
        // global constant); zero/absent: a read-only global constant
        lua_push_engine_table(L,
                type ? LuaTable::PARAMS : LuaTable::CONSTS);
        luac::push_string(L, name);
        lua_pushinteger(L, val);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }
    return rv;
}

bool lua_param_lookup(XString name, int* sp_out)
{
    lua_State* L = g_L;
    lua_push_engine_table(L, LuaTable::PARAMS);
    luac::push_string(L, name);
    lua_rawget(L, -2);
    bool found = (lua_type(L, -1) == LUA_TNUMBER);
    if (found)
        *sp_out = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 2);
    return found;
}

// ------------------------------------------------------------------------
// chunk loading and import

bool lua_load_chunk_sandboxed(XString chunkname, XString src)
{
    lua_State* L = g_L;
    if (!luac::load_chunk(L, chunkname, src))
    {
        size_t len;
        ZString err = luac::to_string(L, -1, &len);
        PRINTF("lua: compile error: %s\n"_fmt, AString(XString(err)));
        lua_pop(L, 1);
        return false;
    }
    lua_push_engine_table(L, LuaTable::SANDBOX);
    luac::set_chunk_env(L, -2, -1);
    lua_pop(L, 1);
    return true;
}

static
bool slurp_file(ZString path, AString* out)
{
    io::ReadFile in(path);
    if (!in.is_open())
        return false;
    MString acc;
    AString line;
    while (in.getline(line))
    {
        acc += line;
        acc += '\n';
    }
    *out = AString(acc);
    return true;
}

bool lua_import(ZString path)
{
    lua_State* L = g_L;
    // each file loads only once
    lua_push_engine_table(L, LuaTable::LOADED_FILES);
    luac::push_string(L, path);
    lua_rawget(L, -2);
    bool seen = !lua_isnil(L, -1);
    lua_pop(L, 1);
    if (seen)
    {
        lua_pop(L, 1);
        return true;
    }
    luac::push_string(L, path);
    lua_pushboolean(L, 1);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    AString src;
    if (!slurp_file(path, &src))
    {
        PRINTF("lua: can't read %s\n"_fmt, path);
        return false;
    }
    // chunk name = the path ("@" marks it as a file name for tracebacks)
    AString chunkname = STRPRINTF("@%s"_fmt, path);
    if (!lua_load_chunk_sandboxed(chunkname, src))
        return false;
    LuaCtx ctx;
    ctx.what = "import";
    return lua_run_sync(ctx, 0);
}

bool lua_load_content()
{
    bool ok = true;
    for (AString& fn : npc_srcs)
    {
        if (!lua_import(fn))
            ok = false;
    }
    return ok;
}

// ------------------------------------------------------------------------
// global utility functions (doc/lua-api.md section 2)

static
int parse_atoi(const char* b, const char* e)
{
    // the old conv_num used C atoi: optional whitespace, optional sign,
    // leading digits, else 0; saturates at the int32 bounds
    while (b != e && (*b == ' ' || *b == '\t'))
        ++b;
    bool neg = false;
    if (b != e && (*b == '+' || *b == '-'))
    {
        neg = (*b == '-');
        ++b;
    }
    long long acc = 0;
    for (; b != e && '0' <= *b && *b <= '9'; ++b)
    {
        acc = acc * 10 + (*b - '0');
        if (acc > 4294967295LL)
            break;
    }
    if (neg)
        acc = -acc;
    if (acc > 2147483647LL)
        return 2147483647;
    if (acc < -2147483647LL - 1)
        return -2147483647 - 1;
    return static_cast<int>(acc);
}

static
int wrap_int32(long long v)
{
    uint32_t u = static_cast<uint32_t>(static_cast<uint64_t>(v));
    if (u <= 0x7fffffffu)
        return static_cast<int>(u);
    return static_cast<int>(static_cast<int64_t>(u) - 4294967296LL);
}

static
int lp_rand(lua_State* L)
{
    int a = check_int(L, 1);
    if (lua_isnoneornil(L, 2))
    {
        // rand(n): [0, n), 0 if n <= 0
        if (a <= 0)
            return luac::push_int(L, 0);
        return luac::push_int(L, random_::to(a));
    }
    int b = check_int(L, 2);
    if (a > b)
    {
        int t = a;
        a = b;
        b = t;
    }
    return luac::push_int(L, random_::in(a, b));
}

static
int lp_idiv(lua_State* L)
{
    int a = check_int(L, 1);
    int b = check_int(L, 2);
    if (b == 0)
        return luaL_error(L, "division by zero");
    if (a == -2147483647 - 1 && b == -1)
        return luac::push_int(L, a);
    return luac::push_int(L, a / b);
}

static
int lp_imod(lua_State* L)
{
    int a = check_int(L, 1);
    int b = check_int(L, 2);
    if (b == 0)
        return luaL_error(L, "division by zero");
    if (b == -1)
        return luac::push_int(L, 0);
    return luac::push_int(L, a % b);
}

static
int lp_int32(lua_State* L)
{
    if (luac::is_integer_value(L, 1))
    {
        long long v = lua_tointeger(L, 1);
        return luac::push_int(L, wrap_int32(v));
    }
    lua_Number d = luaL_checknumber(L, 1);
    if (!(d >= -9.2233720368547758e18 && d < 9.2233720368547758e18))
        return luaL_argerror(L, 1, "integer expected");
    long long v = static_cast<long long>(d);
    if (static_cast<lua_Number>(v) != d)
        return luaL_argerror(L, 1, "integer expected");
    return luac::push_int(L, wrap_int32(v));
}

static
int lp_atoi(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TNUMBER)
        return luac::push_int(L, check_int(L, 1));
    ZString s = check_string(L, 1);
    const char* b = s.c_str();
    return luac::push_int(L, parse_atoi(b, b + s.size()));
}

static
int lp_tostr(lua_State* L)
{
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        lua_pushvalue(L, 1);
        return 1;
    }
    if (lua_type(L, 1) == LUA_TNUMBER)
    {
        int v = check_int(L, 1);
        lua_pushfstring(L, "%d", v);
        return 1;
    }
    return luaL_argerror(L, 1, "integer or string expected");
}

static
int lp_chr(lua_State* L)
{
    int v = check_int(L, 1);
    char c = static_cast<char>(v & 0xff);
    lua_pushlstring(L, &c, 1);
    return 1;
}

static
int lp_ord(lua_State* L)
{
    ZString s = check_string(L, 1);
    if (!s.size())
        return luac::push_int(L, 0);
    return luac::push_int(L, static_cast<unsigned char>(*s.begin()));
}

static
int lp_l(lua_State* L)
{
    // translation placeholder: returns its first argument unchanged
    check_string(L, 1);
    lua_pushvalue(L, 1);
    return 1;
}

static
int lp_if_then_else(lua_State* L)
{
    luaL_checkany(L, 2);
    luaL_checkany(L, 3);
    bool cond;
    if (lua_isboolean(L, 1))
        cond = lua_toboolean(L, 1);
    else
        cond = (check_int(L, 1) != 0);
    lua_pushvalue(L, cond ? 2 : 3);
    return 1;
}

static
int lp_min(lua_State* L)
{
    int n = lua_gettop(L);
    if (n < 2)
        return luaL_error(L, "min: expected at least 2 arguments (use arrmin for arrays)");
    int best = check_int(L, 1);
    for (int i = 2; i <= n; ++i)
    {
        int v = check_int(L, i);
        if (v < best)
            best = v;
    }
    return luac::push_int(L, best);
}

static
int lp_max(lua_State* L)
{
    int n = lua_gettop(L);
    if (n < 2)
        return luaL_error(L, "max: expected at least 2 arguments (use arrmax for arrays)");
    int best = check_int(L, 1);
    for (int i = 2; i <= n; ++i)
    {
        int v = check_int(L, i);
        if (v > best)
            best = v;
    }
    return luac::push_int(L, best);
}

static
int lp_average(lua_State* L)
{
    int n = lua_gettop(L);
    if (n < 2)
        return luaL_error(L, "average: expected at least 2 arguments");
    long long sum = 0;
    for (int i = 1; i <= n; ++i)
        sum += check_int(L, i);
    return luac::push_int(L, static_cast<int>(sum / n));
}

static
int lp_sqrt(lua_State* L)
{
    int v = check_int(L, 1);
    if (v < 0)
        return luac::push_int(L, 0);
    return luac::push_int(L, static_cast<int>(std::sqrt(static_cast<double>(v))));
}

static
int lp_cbrt(lua_State* L)
{
    int v = check_int(L, 1);
    return luac::push_int(L, static_cast<int>(std::cbrt(static_cast<double>(v))));
}

static
int lp_pow(lua_State* L)
{
    int a = check_int(L, 1);
    int b = check_int(L, 2);
    double r = std::pow(static_cast<double>(a), static_cast<double>(b));
    // the old builtin cast the double straight to int; guard the UB cases
    if (!(r >= -2147483648.0 && r <= 2147483647.0))
        return luac::push_int(L, 0);
    return luac::push_int(L, static_cast<int>(r));
}

// ---- array helpers: always go through gettable/settable so the world.array
// proxies (metatable-based) work transparently; indices are 0..255 as before

static
int lp_setarray(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int start = check_int(L, 2);
    if (start < 0 || start > 255)
        return luaL_argerror(L, 2, "start index 0..255 expected");
    int n = lua_gettop(L) - 2;
    for (int k = 0; k < n; ++k)
    {
        int idx = start + k;
        if (idx > 255)
            break;   // clamp at 255, as before
        lua_pushinteger(L, idx);
        lua_pushvalue(L, 3 + k);
        lua_settable(L, 1);
    }
    return 0;
}

static
int lp_cleararray(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int start = check_int(L, 2);
    luaL_checkany(L, 3);
    int count = check_int(L, 4);
    if (start < 0 || start > 255)
        return luaL_argerror(L, 2, "start index 0..255 expected");
    for (int k = 0; k < count; ++k)
    {
        int idx = start + k;
        if (idx > 255)
            break;
        lua_pushinteger(L, idx);
        lua_pushvalue(L, 3);
        lua_settable(L, 1);
    }
    return 0;
}

static
int lp_getarraysize(lua_State* L)
{
    // accepts a non-table (returns 0) so getarraysize(p.tmp.never_set) is safe
    if (lua_type(L, 1) != LUA_TTABLE)
        return luac::push_int(L, 0);
    int last = 0;
    for (int i = 0; i < 256; ++i)
    {
        lua_pushinteger(L, i);
        lua_gettable(L, 1);
        bool nonempty;
        switch (lua_type(L, -1))
        {
        case LUA_TNIL:
            nonempty = false;
            break;
        case LUA_TNUMBER:
            nonempty = (lua_tonumber(L, -1) != 0);
            break;
        case LUA_TSTRING:
        {
            size_t len;
            luac::to_string(L, -1, &len);
            nonempty = (len != 0);
            break;
        }
        default:
            nonempty = true;
            break;
        }
        lua_pop(L, 1);
        if (nonempty)
            last = i + 1;
    }
    // deliberate deviation (doc/lua-api.md section 13): empty array -> 0
    return luac::push_int(L, last);
}

static
int lp_array_search(lua_State* L)
{
    luaL_checkany(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    int start = opt_int(L, 3, 0);
    if (start < 0)
        start = 0;
    for (int i = start; i < 256; ++i)
    {
        lua_pushinteger(L, i);
        lua_gettable(L, 2);
        bool eq = lua_rawequal(L, -1, 1);
        lua_pop(L, 1);
        if (eq)
            return luac::push_int(L, i);
    }
    return luac::push_int(L, -1);
}

static
int lp_explode_common(lua_State* L, bool as_int)
{
    ZString s = check_string(L, 1);
    ZString sep = check_string(L, 2);
    if (!sep.size())
        return luaL_argerror(L, 2, "separator expected");
    char sc = *sep.c_str();   // split on the FIRST character only, as before
    lua_newtable(L);
    const char* b = s.c_str();
    const char* e = b + s.size();
    const char* piece = b;
    int idx = 0;
    for (const char* p = b; ; ++p)
    {
        if (p == e || *p == sc)
        {
            if (as_int)
                lua_pushinteger(L, parse_atoi(piece, p));
            else
                lua_pushlstring(L, piece, static_cast<size_t>(p - piece));
            lua_rawseti(L, -2, idx);
            ++idx;
            if (p == e || idx >= 256)
                break;
            piece = p + 1;
        }
    }
    return 1;
}

static
int lp_explode(lua_State* L)
{
    return lp_explode_common(L, false);
}

static
int lp_explode_int(lua_State* L)
{
    return lp_explode_common(L, true);
}

static
int lp_arrmin(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int start = opt_int(L, 2, 0);
    if (start < 0)
        start = 0;
    // old one-argument min(arr): initial value -10 (0xFFFFFFF6), kept verbatim
    int best = -10;
    for (int i = start; i < 256; ++i)
    {
        lua_pushinteger(L, i);
        lua_gettable(L, 1);
        if (lua_type(L, -1) == LUA_TNUMBER)
        {
            int v = static_cast<int>(lua_tonumber(L, -1));
            if (v < best)
                best = v;
        }
        lua_pop(L, 1);
    }
    return luac::push_int(L, best);
}

static
int lp_arrmax(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int start = opt_int(L, 2, 0);
    if (start < 0)
        start = 0;
    // old one-argument max(arr): initial value 0, kept verbatim
    int best = 0;
    for (int i = start; i < 256; ++i)
    {
        lua_pushinteger(L, i);
        lua_gettable(L, 1);
        if (lua_type(L, -1) == LUA_TNUMBER)
        {
            int v = static_cast<int>(lua_tonumber(L, -1));
            if (v > best)
                best = v;
        }
        lua_pop(L, 1);
    }
    return luac::push_int(L, best);
}

static
int lp_array_common(lua_State* L, LuaTable default_mt)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    check_string(L, 2);
    lua_pushvalue(L, 2);
    lua_gettable(L, 1);
    if (lua_istable(L, -1))
        return 1;
    lua_pop(L, 1);
    lua_newtable(L);
    lua_push_engine_table(L, default_mt);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, -2);
    lua_settable(L, 1);   // scope[name] = t
    return 1;             // t stays on top
}

static
int lp_array(lua_State* L)
{
    return lp_array_common(L, LuaTable::INT_DEFAULT_MT);
}

static
int lp_arraystr(lua_State* L)
{
    return lp_array_common(L, LuaTable::STR_DEFAULT_MT);
}

static
int lp_being(lua_State* L)
{
    int id = check_int(L, 1);
    dumb_ptr<block_list> bl;
    if (id > 0)
        bl = map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id)));
    lua_push_being_handle(L, bl);   // nil for null
    return 1;
}

static
int lp_stop(lua_State* L)
{
    luac::push_ref(L, g_stop_ref);
    return lua_error(L);
}

static
int lp_import(lua_State* L)
{
    ZString path = check_string(L, 1);
    if (!g_loading)
        return luaL_error(L, "import() is only allowed while content files load");
    bool ok;
    {
        ok = lua_import(path);
    }
    if (!ok)
        return luaL_error(L, "import '%s' failed", path.c_str());
    return 0;
}

static
int lp_print(lua_State* L)
{
    int n = lua_gettop(L);
    int pushed = 0;
    for (int i = 1; i <= n; ++i)
    {
        if (i > 1)
        {
            lua_pushliteral(L, "\t");
            ++pushed;
        }
        luaL_tolstring(L, i, nullptr);
        ++pushed;
    }
    if (!pushed)
        lua_pushliteral(L, "");
    else if (pushed > 1)
        lua_concat(L, pushed);
    size_t len;
    ZString z = luac::to_string(L, -1, &len);
    LuaCtx ctx = lua_current_ctx();
    PRINTF("lua: print [%s npc=%d]: %s\n"_fmt,
            ZString(strings::really_construct_from_a_pointer, ctx.what, nullptr),
            unwrap<BlockId>(ctx.npc), AString(XString(z)));
    lua_pop(L, 1);
    return 0;
}

static
int lp_collectgarbage(lua_State* L)
{
    ZString what = opt_string(L, 1, ""_s);
    if (what != "count"_s)
        return luaL_error(L, "collectgarbage is restricted to 'count'");
    int kb = lua_gc(L, LUA_GCCOUNT);
    int b = lua_gc(L, LUA_GCCOUNTB);
    lua_pushnumber(L, kb + b / 1024.0);
    return 1;
}

static
int lp_readonly_newindex(lua_State* L)
{
    return luaL_error(L, "attempt to modify a read-only table");
}

// os.date forced to UTC: prepend '!' to the format unless already present
static
int lp_os_date(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 2)
    {
        lua_settop(L, 2);
        n = 2;
    }
    lua_pushvalue(L, lua_upvalueindex(1));   // the real os.date
    if (n == 0 || lua_isnil(L, 1))
        lua_pushliteral(L, "!%c");
    else
    {
        ZString s = check_string(L, 1);
        if (s.startswith('!'))
            lua_pushvalue(L, 1);
        else
        {
            lua_pushliteral(L, "!");
            lua_pushvalue(L, 1);
            lua_concat(L, 2);
        }
    }
    if (n == 2)
        lua_pushvalue(L, 2);
    lua_call(L, (n == 2) ? 2 : 1, LUA_MULTRET);
    return lua_gettop(L) - n;
}

// math.random / rand share the engine RNG (generic/random.hpp)
static
int lp_math_random(lua_State* L)
{
    int n = lua_gettop(L);
    if (n == 0)
    {
        lua_pushnumber(L, random_::to(1 << 24) / 16777216.0);
        return 1;
    }
    if (n == 1)
    {
        int m = check_int(L, 1);
        if (m < 1)
            return luaL_argerror(L, 1, "interval is empty");
        return luac::push_int(L, random_::in(1, m));
    }
    int a = check_int(L, 1);
    int b = check_int(L, 2);
    if (a > b)
        return luaL_argerror(L, 2, "interval is empty");
    return luac::push_int(L, random_::in(a, b));
}

static
int lp_math_randomseed(lua_State* L)
{
    // the engine RNG is not reseedable from scripts
    (void)L;
    return 0;
}

// ------------------------------------------------------------------------
// bit32 (D2: Lua 5.4 has no bit32; results are signed int32)

static
uint32_t bit32_arg(lua_State* L, int idx)
{
    return static_cast<uint32_t>(check_int(L, idx));
}

static
int push_u32(lua_State* L, uint32_t u)
{
    if (u <= 0x7fffffffu)
        return luac::push_int(L, static_cast<int>(u));
    return luac::push_int(L,
            static_cast<int>(static_cast<int64_t>(u) - 4294967296LL));
}

static
int lp_bit_band(lua_State* L)
{
    uint32_t r = 0xffffffffu;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i)
        r &= bit32_arg(L, i);
    return push_u32(L, r);
}

static
int lp_bit_bor(lua_State* L)
{
    uint32_t r = 0;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i)
        r |= bit32_arg(L, i);
    return push_u32(L, r);
}

static
int lp_bit_bxor(lua_State* L)
{
    uint32_t r = 0;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i)
        r ^= bit32_arg(L, i);
    return push_u32(L, r);
}

static
int lp_bit_bnot(lua_State* L)
{
    return push_u32(L, ~bit32_arg(L, 1));
}

static
uint32_t bit32_shift(uint32_t u, int disp, bool arith)
{
    // Lua 5.2 bit32 semantics: negative disp shifts the other way, |disp|
    // >= 32 clears (arshift fills with the sign bit)
    if (disp <= -32)
        return 0;
    if (disp < 0)
        return u << -disp;
    if (disp >= 32)
    {
        if (arith && (u & 0x80000000u))
            return 0xffffffffu;
        return 0;
    }
    if (arith && (u & 0x80000000u))
        return (u >> disp) | ~(0xffffffffu >> disp);
    return u >> disp;
}

static
int lp_bit_lshift(lua_State* L)
{
    uint32_t u = bit32_arg(L, 1);
    int disp = check_int(L, 2);
    uint32_t r;
    if (disp <= -32 || disp >= 32)
        r = 0;
    else if (disp >= 0)
        r = u << disp;
    else
        r = u >> -disp;
    return push_u32(L, r);
}

static
int lp_bit_rshift(lua_State* L)
{
    uint32_t u = bit32_arg(L, 1);
    int disp = check_int(L, 2);
    return push_u32(L, bit32_shift(u, disp, false));
}

static
int lp_bit_arshift(lua_State* L)
{
    uint32_t u = bit32_arg(L, 1);
    int disp = check_int(L, 2);
    return push_u32(L, bit32_shift(u, disp, true));
}

static
int lp_bit_btest(lua_State* L)
{
    uint32_t r = 0xffffffffu;
    int n = lua_gettop(L);
    for (int i = 1; i <= n; ++i)
        r &= bit32_arg(L, i);
    lua_pushboolean(L, r != 0);
    return 1;
}

static
int lp_bit_extract(lua_State* L)
{
    uint32_t u = bit32_arg(L, 1);
    int field = check_int(L, 2);
    int width = opt_int(L, 3, 1);
    if (field < 0)
        return luaL_argerror(L, 2, "field cannot be negative");
    if (width <= 0)
        return luaL_argerror(L, 3, "width must be positive");
    if (field + width > 32)
        return luaL_error(L, "trying to access non-existent bits");
    uint32_t mask = (width == 32) ? 0xffffffffu : ((1u << width) - 1);
    return push_u32(L, (u >> field) & mask);
}

static
int lp_bit_replace(lua_State* L)
{
    uint32_t u = bit32_arg(L, 1);
    uint32_t v = bit32_arg(L, 2);
    int field = check_int(L, 3);
    int width = opt_int(L, 4, 1);
    if (field < 0)
        return luaL_argerror(L, 3, "field cannot be negative");
    if (width <= 0)
        return luaL_argerror(L, 4, "width must be positive");
    if (field + width > 32)
        return luaL_error(L, "trying to access non-existent bits");
    uint32_t mask = (width == 32) ? 0xffffffffu : ((1u << width) - 1);
    return push_u32(L, (u & ~(mask << field)) | ((v & mask) << field));
}

// ------------------------------------------------------------------------
// the sandbox environment metatable

static
int env_index(lua_State* L)
{
    // upvalue 1: consts, upvalue 2: params; (t, key) on the stack
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(1));
    if (!lua_isnil(L, -1))
        return 1;
    lua_pop(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "undefined global");
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(2));
    if (!lua_isnil(L, -1))
        return luaL_error(L, "undefined global '%s' (did you mean p.%s?)",
                lua_tostring(L, 2), lua_tostring(L, 2));
    return luaL_error(L, "undefined global '%s'", lua_tostring(L, 2));
}

static
int env_newindex(lua_State* L)
{
    // upvalue 1: consts; (t, key, value) on the stack
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(1));
    if (!lua_isnil(L, -1))
        return luaL_error(L, "attempt to assign constant '%s'",
                lua_tostring(L, 2));
    lua_pop(L, 1);
    if (!g_loading)
    {
        // new globals after on_init: warning; error when checking scripts
        if (g_check_only_flag)
        {
            luaL_tolstring(L, 2, nullptr);
            return luaL_error(L, "new global '%s' created after startup",
                    lua_tostring(L, -1));
        }
        size_t len;
        ZString key = luac::to_string(L, 2, &len);
        if (!key.size())
            key = "?"_s;
        lua_warn(STRPRINTF("new global '%s' created after startup"_fmt, key));
    }
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 1);
    return 0;
}

static
int lp_default_int_index(lua_State* L)
{
    (void)L;
    lua_pushinteger(L, 0);
    return 1;
}

static
int lp_default_str_index(lua_State* L)
{
    lua_pushliteral(L, "");
    return 1;
}

// ------------------------------------------------------------------------
// sandbox construction

static
void copy_global_field(lua_State* L, int env, const char* name)
{
    luac::push_globals(L);
    lua_getfield(L, -1, name);
    lua_setfield(L, env, name);
    lua_pop(L, 1);
}

// push a shallow copy of the global library `name`
static
void copy_lib_push(lua_State* L, const char* name)
{
    luac::push_globals(L);
    lua_getfield(L, -1, name);   // lib
    lua_remove(L, -2);           // drop globals
    lua_newtable(L);             // copy
    lua_pushnil(L);
    while (lua_next(L, -3))
    {
        // stack: lib, copy, key, value
        lua_pushvalue(L, -2);
        lua_insert(L, -2);       // lib, copy, key, key, value
        lua_rawset(L, -4);       // copy[key] = value
    }
    lua_remove(L, -2);           // drop lib; copy stays
}

static
const luaL_Reg bit32_funcs[] =
{
    {"band", lp_bit_band},
    {"bor", lp_bit_bor},
    {"bxor", lp_bit_bxor},
    {"bnot", lp_bit_bnot},
    {"lshift", lp_bit_lshift},
    {"rshift", lp_bit_rshift},
    {"arshift", lp_bit_arshift},
    {"btest", lp_bit_btest},
    {"extract", lp_bit_extract},
    {"replace", lp_bit_replace},
    {nullptr, nullptr},
};

static
const luaL_Reg global_funcs[] =
{
    {"rand", lp_rand},
    {"idiv", lp_idiv},
    {"imod", lp_imod},
    {"int32", lp_int32},
    {"atoi", lp_atoi},
    {"tostr", lp_tostr},
    {"chr", lp_chr},
    {"ord", lp_ord},
    {"l", lp_l},
    {"if_then_else", lp_if_then_else},
    {"min", lp_min},
    {"max", lp_max},
    {"average", lp_average},
    {"sqrt", lp_sqrt},
    {"cbrt", lp_cbrt},
    {"pow", lp_pow},
    {"setarray", lp_setarray},
    {"cleararray", lp_cleararray},
    {"getarraysize", lp_getarraysize},
    {"array_search", lp_array_search},
    {"explode", lp_explode},
    {"explode_int", lp_explode_int},
    {"arrmin", lp_arrmin},
    {"arrmax", lp_arrmax},
    {"array", lp_array},
    {"arraystr", lp_arraystr},
    {"being", lp_being},
    {"stop", lp_stop},
    {"import", lp_import},
    {"print", lp_print},
    {"collectgarbage", lp_collectgarbage},
    {nullptr, nullptr},
};

static
const char* const base_whitelist[] =
{
    "tostring", "tonumber", "type", "pairs", "ipairs", "next", "select",
    "error", "assert", "pcall", "xpcall", "rawget", "rawset", "rawequal",
    "rawlen", "setmetatable", "getmetatable", "_VERSION",
};

static
void build_sandbox(lua_State* L)
{
    lua_newtable(L);
    int env = lua_gettop(L);

    // whitelisted base functions (D2)
    for (const char* name : base_whitelist)
        copy_global_field(L, env, name);
    // unpack = table.unpack
    luac::push_globals(L);
    lua_getfield(L, -1, "table");
    lua_getfield(L, -1, "unpack");
    lua_setfield(L, env, "unpack");
    lua_pop(L, 2);

    // library copies
    copy_lib_push(L, "string");
    lua_setfield(L, env, "string");
    copy_lib_push(L, "table");
    lua_setfield(L, env, "table");
    copy_lib_push(L, "utf8");
    lua_setfield(L, env, "utf8");
    copy_lib_push(L, "math");
    // math.random / math.randomseed backed by the engine RNG
    luac::push_cfunction(L, lp_math_random, "math.random");
    lua_setfield(L, -2, "random");
    luac::push_cfunction(L, lp_math_randomseed, "math.randomseed");
    lua_setfield(L, -2, "randomseed");
    lua_setfield(L, env, "math");

    // read-only coroutine: empty proxy with __index into the copy
    copy_lib_push(L, "coroutine");
    lua_newtable(L);              // proxy
    lua_newtable(L);              // mt
    lua_pushvalue(L, -3);         // the copy
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, lp_readonly_newindex, "coroutine.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");
    lua_setmetatable(L, -2);
    lua_setfield(L, env, "coroutine");
    lua_pop(L, 1);                // the copy

    // os = { time, clock, date(UTC-forced) }
    luac::push_globals(L);
    lua_getfield(L, -1, "os");    // globals, os
    lua_newtable(L);              // globals, os, osx
    lua_getfield(L, -2, "time");
    lua_setfield(L, -2, "time");
    lua_getfield(L, -2, "clock");
    lua_setfield(L, -2, "clock");
    lua_getfield(L, -2, "date");
    luac::push_cclosure(L, lp_os_date, "os.date", 1);
    lua_setfield(L, -2, "date");
    lua_setfield(L, env, "os");
    lua_pop(L, 2);

    // bit32
    lua_newtable(L);
    luac::register_funcs(L, -1, bit32_funcs);
    lua_setfield(L, env, "bit32");

    // engine globals (rand, idiv, ..., being, stop, import, print,
    // collectgarbage restricted to "count")
    luac::register_funcs(L, env, global_funcs);

    // handle metatables and the namespace tables (implemented in their own
    // modules; see lua-handles.hpp / lua-libs.hpp)
    lua_register_handle_metatables(L);
    lua_register_lib_npc(L, env);
    lua_register_lib_map(L, env);
    lua_register_lib_mob(L, env);
    lua_register_lib_item(L, env);
    lua_register_lib_players(L, env);
    lua_register_lib_server(L, env);
    lua_register_lib_world(L, env);

    // strict-globals metatable: unknown reads raise, constant writes raise,
    // new globals warn once loading is done (doc/lua-engine.md section 8)
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::CONSTS);
    lua_push_engine_table(L, LuaTable::PARAMS);
    luac::push_cclosure(L, env_index, "env.__index", 2);
    lua_setfield(L, -2, "__index");
    lua_push_engine_table(L, LuaTable::CONSTS);
    luac::push_cclosure(L, env_newindex, "env.__newindex", 1);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, env);

    luac::sandbox_finalise(L);
    g_table_refs[static_cast<int>(LuaTable::SANDBOX)] = luac::ref(L);
}

// ------------------------------------------------------------------------
// init / final

void lua_init()
{
    lua_alloc_set_limit(static_cast<size_t>(lua_conf.memory_limit_mb)
            * 1024 * 1024);
    g_L = luac::new_state(lua_capped_alloc, nullptr);
    if (!g_L)
    {
        FPRINTF(stderr, "lua: cannot create interpreter state\n"_fmt);
        abort();
    }
    lua_State* L = g_L;
    g_loading = true;
    luaL_openlibs(L);
    luac::set_instruction_hook(L, budget_hook, BUDGET_CHECK_EVERY);

    // engine tables (SANDBOX and PRELUDE are filled below)
    for (LuaTable t : {LuaTable::CONSTS, LuaTable::PARAMS, LuaTable::PLAYERS,
            LuaTable::NPCS, LuaTable::NPCS_BYNAME, LuaTable::MOB_DEATH_FNS,
            LuaTable::LOADED_FILES})
    {
        lua_newtable(L);
        g_table_refs[static_cast<int>(t)] = luac::ref(L);
    }

    // shared 0 / "" default metatables (p.tmp, self.vars, worldtmp, ...)
    lua_newtable(L);
    luac::push_cfunction(L, lp_default_int_index, "default0.__index");
    lua_setfield(L, -2, "__index");
    g_table_refs[static_cast<int>(LuaTable::INT_DEFAULT_MT)] = luac::ref(L);
    lua_newtable(L);
    luac::push_cfunction(L, lp_default_str_index, "defaultstr.__index");
    lua_setfield(L, -2, "__index");
    g_table_refs[static_cast<int>(LuaTable::STR_DEFAULT_MT)] = luac::ref(L);

    // the stop() sentinel: any unique, engine-private object works
    lua_newtable(L);
    g_stop_ref = luac::ref(L);

    // dialog-thread ctx registry (weak keys: dead threads drop out even if a
    // clear is missed)
    lua_newtable(L);
    lua_newtable(L);
    lua_pushliteral(L, "k");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    g_thread_ctx_ref = luac::ref(L);

    build_sandbox(L);

    // the prelude (pure-Lua glue; returns its module table)
    if (!lua_load_chunk_sandboxed("=prelude"_s, lua_prelude_source()))
    {
        FPRINTF(stderr, "lua: internal error: prelude does not compile\n"_fmt);
        abort();
    }
    {
        luac::push_cfunction(L, msgh, "traceback");
        lua_insert(L, -2);
        int msgh_idx = lua_gettop(L) - 1;
        budget_push();
        int status = lua_pcall(L, 0, 1, msgh_idx);
        budget_pop();
        if (status != LUA_OK || !lua_istable(L, -1))
        {
            size_t len;
            ZString err = luac::to_string(L, -1, &len);
            FPRINTF(stderr, "lua: internal error: prelude failed: %s\n"_fmt,
                    AString(XString(err)));
            abort();
        }
        g_table_refs[static_cast<int>(LuaTable::PRELUDE)] = luac::ref(L);
        lua_pop(L, 1);   // the message handler
    }
}

void lua_final()
{
    if (!g_L)
        return;
    // release the command registry callbacks before the state dies
    lua_events_final();
    luac::close_state(g_L);
    g_L = nullptr;
    for (int& r : g_table_refs)
        r = lua_noref;
    g_stop_ref = lua_noref;
    g_thread_ctx_ref = lua_noref;
    g_ctx.clear();
    g_budget.clear();
}

// ------------------------------------------------------------------------
// --dump-lua-api

static
void dump_collect_table(lua_State* L, std::vector<AString>* out,
        const char* prefix)
{
    // table to enumerate on top of the stack; string keys only
    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        if (lua_type(L, -2) == LUA_TSTRING)
        {
            size_t len;
            ZString key = luac::to_string(L, -2, &len);
            const char* tn = lua_typename(L, lua_type(L, -1));
            out->push_back(STRPRINTF("%s %s %s"_fmt,
                        ZString(strings::really_construct_from_a_pointer, prefix, nullptr),
                        key,
                        ZString(strings::really_construct_from_a_pointer, tn, nullptr)));
            // one level into namespace tables
            if (lua_istable(L, -1))
            {
                AString pfx = STRPRINTF("member %s."_fmt, key);
                lua_pushnil(L);
                while (lua_next(L, -2))
                {
                    if (lua_type(L, -2) == LUA_TSTRING)
                    {
                        size_t len2;
                        ZString k2 = luac::to_string(L, -2, &len2);
                        const char* tn2 = lua_typename(L, lua_type(L, -1));
                        out->push_back(STRPRINTF("%s%s %s"_fmt, pfx, k2,
                                    ZString(strings::really_construct_from_a_pointer, tn2, nullptr)));
                    }
                    lua_pop(L, 1);
                }
            }
        }
        lua_pop(L, 1);
    }
}

void lua_dump_api()
{
    lua_State* L = g_L;
    std::vector<AString> lines;
    lua_push_engine_table(L, LuaTable::SANDBOX);
    dump_collect_table(L, &lines, "global");
    lua_pop(L, 1);
    lua_push_engine_table(L, LuaTable::CONSTS);
    dump_collect_table(L, &lines, "const");
    lua_pop(L, 1);
    lua_push_engine_table(L, LuaTable::PARAMS);
    dump_collect_table(L, &lines, "param");
    lua_pop(L, 1);
    std::sort(lines.begin(), lines.end());
    for (AString& line : lines)
        PRINTF("%s\n"_fmt, line);
}
} // namespace map
} // namespace tmwa
