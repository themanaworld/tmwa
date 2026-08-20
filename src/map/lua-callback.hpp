#pragma once
//    lua-callback.hpp - LuaCallback operations and the live-reference counter.
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

// Ownership rule: every LuaCallback value is owned by exactly one holder and
// released by that holder on every exit path (doc/lua-engine.md section 2.4).

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/strs.hpp"

#include "lua-types.hpp"


namespace tmwa
{
namespace map
{
// named form; takes no reference
LuaCallback lua_cb_named(NpcEvent ev);
// from a stack slot: a function (takes a registry ref, records the current
// ctx NPC as self_npc) or a "Npc::Label" string. Raises on other types.
LuaCallback lua_cb_from_stack(lua_State* L, int idx);
// extra reference for the function form (areatimer stores one per player)
LuaCallback lua_cb_dup(const LuaCallback& cb);
// release the reference, idempotent (cb becomes empty)
void lua_cb_release(LuaCallback& cb);
// fire and consume: named form goes through lua_npc_event, function form
// through lua_fire_fn_ref (queue rules apply in both)
void lua_cb_fire(LuaCallback cb, dumb_ptr<map_session_data> sd, LuaArgs args);

// bookkeeping helper for modules that take over a fn_ref from a LuaCallback:
// unrefs and decrements the live counter
void lua_cb_release_ref(lua_State* L, int fn_ref);

// Debug counter: incremented by lua_cb_from_stack/lua_cb_dup, decremented on
// release. Unit tests assert it returns to zero; @luastats prints it.
int lua_live_refs();
} // namespace map
} // namespace tmwa
