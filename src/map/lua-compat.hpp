#pragma once
//    lua-compat.hpp - the Lua portability funnel.
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

// This is the ONLY file allowed to spell version-specific lua_*/luaL_* names
// (see doc/lua-engine.md section 10; tools/check-lua-compat.py enforces the
// forbidden list). Everything else the bindings use must be in the 5.4/Luau
// common subset: luaL_check*, lua_push*, lua_to*, lua_getfield/setfield,
// lua_newtable/createtable, 4-arg lua_pcall, lua_yield, lua_error, luaL_error,
// luaL_argerror, luaL_newmetatable, lua_setmetatable, lua_newthread,
// lua_xmove, lua_status, stack ops, lua_next, lua_rawget/rawset/rawgeti/
// rawseti. Those may be used directly in any lua-*.cpp.
//
// Include order rule: this header (and thus <lua.hpp>) must be included
// before "../poison.hpp" in every TU that uses it; module headers include it
// first for that reason.

#include "fwd.hpp"

#include <lua.hpp>

#include <cstdlib>

#include "../strings/xstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/astring.hpp"

#include "../io/cxxstdio.hpp"

#include "lua-types.hpp"


namespace tmwa
{
namespace map
{
// lua-alloc.cpp (declared here too so the panic handler can report usage)
size_t lua_alloc_used();
size_t lua_alloc_limit();

namespace luac
{
// lua_noref (lua-types.hpp) must match the real constant so that host structs
// can carry "no reference" without including Lua headers.
static_assert(lua_noref == LUA_NOREF, "lua_noref must equal LUA_NOREF");

inline
int compat_panic(lua_State* L)
{
    size_t len = 0;
    const char* msg = lua_tolstring(L, -1, &len);
    if (msg)
        FPRINTF(stderr, "lua: PANIC: %s\n"_fmt, ZString(msg, msg + len, nullptr));
    else
        FPRINTF(stderr, "lua: PANIC: (non-string error object)\n"_fmt);
    FPRINTF(stderr, "lua: PANIC: memory used %zu of limit %zu\n"_fmt,
            lua_alloc_used(), lua_alloc_limit());
    abort();
}

inline
lua_State* new_state(lua_Alloc alloc, void* ud)
{
    lua_State* L = lua_newstate(alloc, ud);
    if (L)
        lua_atpanic(L, compat_panic);
    return L;
}

inline
void close_state(lua_State* L)
{
    lua_close(L);
}

// Load a chunk from source text. On success pushes the compiled function and
// returns true; on failure pushes the error message and returns false.
// 5.4: text-only luaL_loadbufferx; Luau: luau_compile + luau_load.
inline
bool load_chunk(lua_State* L, XString chunkname, XString src)
{
    AString cn = AString(chunkname);
    const char* data = src.size() ? &*src.begin() : "";
    int status = luaL_loadbufferx(L, data, src.size(), cn.c_str(), "t");
    return status == LUA_OK;
}

inline
void push_cfunction(lua_State* L, lua_CFunction fn, const char* debugname)
{
    // 5.4 has no per-function debug name; Luau wants one.
    (void)debugname;
    lua_pushcfunction(L, fn);
}

inline
void push_cclosure(lua_State* L, lua_CFunction fn, const char* debugname, int nup)
{
    (void)debugname;
    lua_pushcclosure(L, fn, nup);
}

// Register a NULL-terminated luaL_Reg list into the table at tableidx.
inline
void register_funcs(lua_State* L, int tableidx, const luaL_Reg* l)
{
    lua_pushvalue(L, tableidx);
    luaL_setfuncs(L, l, 0);
    lua_pop(L, 1);
}

// Pop the value on top of the stack and return a registry reference to it.
inline
int ref(lua_State* L)
{
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

inline
void unref(lua_State* L, int r)
{
    luaL_unref(L, LUA_REGISTRYINDEX, r);
}

inline
void push_ref(lua_State* L, int r)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, r);
}

inline
void push_globals(lua_State* L)
{
    lua_pushglobaltable(L);
}

// Make the table at envidx the _ENV of the chunk at chunkidx.
// Neither stack slot is popped.
inline
void set_chunk_env(lua_State* L, int chunkidx, int envidx)
{
    int chunkabs = lua_absindex(L, chunkidx);
    lua_pushvalue(L, envidx);
    if (!lua_setupvalue(L, chunkabs, 1))
    {
        // chunk does not capture _ENV (touches no globals): nothing to set
        lua_pop(L, 1);
    }
}

// Luau: luaL_sandbox; 5.4: nothing to do.
inline
void sandbox_finalise(lua_State*)
{
}

inline
int push_int(lua_State* L, int v)
{
    lua_pushinteger(L, v);
    return 1;
}

// Accept an integer or an integral float, in int32 range. No string coercion.
inline
bool to_int(lua_State* L, int idx, int* out)
{
    if (lua_isinteger(L, idx))
    {
        lua_Integer v = lua_tointeger(L, idx);
        if (v < -2147483647 - 1 || v > 2147483647)
            return false;
        *out = static_cast<int>(v);
        return true;
    }
    if (lua_type(L, idx) == LUA_TNUMBER)
    {
        lua_Number d = lua_tonumber(L, idx);
        if (!(d >= -2147483648.0 && d <= 2147483647.0))
            return false;
        int v = static_cast<int>(d);
        if (static_cast<lua_Number>(v) != d)
            return false;
        *out = v;
        return true;
    }
    return false;
}

inline
void push_string(lua_State* L, XString x)
{
    if (x.size())
        lua_pushlstring(L, &*x.begin(), x.size());
    else
        lua_pushlstring(L, "", 0);
}

// View of a string value on the stack. Returns a default ZString for
// non-strings (numbers are NOT converted; use lua_type checks first).
inline
ZString to_string(lua_State* L, int idx, size_t* len)
{
    if (lua_type(L, idx) != LUA_TSTRING)
    {
        *len = 0;
        return ZString();
    }
    const char* s = lua_tolstring(L, idx, len);
    return ZString(s, s + *len, nullptr);
}

inline
int resume(lua_State* T, lua_State* from, int nargs, int* nres)
{
    return lua_resume(T, from, nargs, nres);
}

inline
void close_thread(lua_State* T, lua_State* from)
{
#if LUA_VERSION_RELEASE_NUM >= 50406
    lua_closethread(T, from);
#else
    (void)from;
    lua_resetthread(T);
#endif
}

// Push onto L a traceback of thread T, prefixed with msg.
inline
void traceback(lua_State* L, lua_State* T, const char* msg)
{
    luaL_traceback(L, T, msg, 0);
}

// Instruction-count hook (the budget). 5.4: lua_sethook with LUA_MASKCOUNT;
// Luau: the interrupt callback. NOTE (5.4): hooks are per thread; the drivers
// must install the hook on every new coroutine thread as well.
inline
void (*&instruction_hook_target())(lua_State*)
{
    static void (*target)(lua_State*) = nullptr;
    return target;
}

inline
void instruction_hook_thunk(lua_State* L, lua_Debug*)
{
    void (*target)(lua_State*) = instruction_hook_target();
    if (target)
        target(L);
}

inline
void set_instruction_hook(lua_State* L, void (*cb)(lua_State*), int every)
{
    instruction_hook_target() = cb;
    lua_sethook(L, instruction_hook_thunk, LUA_MASKCOUNT, every);
}

inline
void* thread_data(lua_State* L)
{
    return lua_getextraspace(L);
}

inline
size_t raw_len(lua_State* L, int idx)
{
    return lua_rawlen(L, idx);
}

inline
bool is_integer_value(lua_State* L, int idx)
{
    return lua_isinteger(L, idx);
}
} // namespace luac
} // namespace map
} // namespace tmwa
