#pragma once
//    lua-internal.hpp - engine internals shared between the lua-*.cpp modules.
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

// Not part of the host-facing API: only lua-*.cpp files include this.

#include "fwd.hpp"

#include "lua-compat.hpp"

#include <cstddef>

#include "../strings/xstring.hpp"
#include "../strings/zstring.hpp"

#include "lua-engine.hpp"


namespace tmwa
{
namespace map
{
// Engine-owned registry tables (doc/lua-engine.md section 2.3).
enum class LuaTable
{
    SANDBOX,        // the sandbox environment table (every chunk's _ENV)
    CONSTS,         // const_db name -> int
    PARAMS,         // params.txt name -> SP id (the third const_db column)
    PLAYERS,        // blockid -> player handle
    NPCS,           // blockid -> NPC handle
    NPCS_BYNAME,    // full name -> NPC handle
    MOB_DEATH_FNS,  // blockid -> function-form mob death handler
    LOADED_FILES,   // import() dedup: path -> true
    PRELUDE,        // the table returned by the prelude chunk (lua-prelude.cpp)
    INT_DEFAULT_MT, // shared metatable: missing keys read 0 (p.tmp etc.)
    STR_DEFAULT_MT, // shared metatable: missing keys read "" (p.tmpstr etc.)
};

// Push one of the engine tables onto L's stack.
void lua_push_engine_table(lua_State* L, LuaTable which);

// Context stack (g_ctx): the drivers push/pop; bindings read the top.
void lua_ctx_push(LuaCtx ctx);
void lua_ctx_pop();
LuaCtx lua_current_ctx();

// Per-thread context: the dialog coroutine stores its ctx so it can be
// re-pushed on every resume.
void lua_thread_ctx_set(lua_State* T, LuaCtx ctx);
bool lua_thread_ctx_get(lua_State* T, LuaCtx* out);
void lua_thread_ctx_clear(lua_State* T);

// param name -> SP id, from the const_db param entries. False if unknown.
bool lua_param_lookup(XString name, int* sp_out);

// The built-in prelude source (lua-prelude.cpp).
ZString lua_prelude_source();

// The memory-capped lua_Alloc, implemented in the poison-free TU
// lua-alloc.cpp (realloc/free are poisoned everywhere else).
void* lua_capped_alloc(void* ud, void* ptr, size_t osize, size_t nsize);
// Current total allocation, and the failure count (allocations refused by
// the cap). The limit is set from lua_conf by lua_init().
void lua_alloc_set_limit(size_t bytes);
size_t lua_alloc_used();
int lua_alloc_failures();
} // namespace map
} // namespace tmwa
