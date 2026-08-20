#pragma once
//    lua-value.hpp - argument checking and value pushing for the bindings.
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

// All check_* helpers may raise Lua errors and therefore must only be called
// from frames with trivially-destructible locals (doc/lua-engine.md
// section 11). Game-state failures (offline player, unknown item) do NOT
// raise: they log via lua_warn and return a null/zero value.

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../strings/xstring.hpp"
#include "../strings/zstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"


namespace tmwa
{
namespace map
{
// integer or integral float within int32; raises otherwise (no string coercion)
int check_int(lua_State* L, int idx);
int opt_int(lua_State* L, int idx, int def);
// boolean only; raises otherwise
bool check_bool(lua_State* L, int idx);
// string only (numbers are not display text here); embedded NUL raises.
// The returned view aliases the Lua stack slot: do not keep it across calls
// that may pop or mutate that slot.
ZString check_string(lua_State* L, int idx);
ZString opt_string(lua_State* L, int idx, ZString def);
// string of at most 15 chars, returned as a MapName (existence not checked:
// unknown-map policy differs per caller)
MapName check_mapname(lua_State* L, int idx);
// item id or exact item name; unknown item logs a warning and returns the
// zero id (callers mirror the old "log and continue" behaviour)
ItemNameId check_item(lua_State* L, int idx);
// "Npc::Label" (also "Npc::" and "::OnX"); either part longer than 23 bytes
// raises
NpcEvent check_event(lua_State* L, int idx);
// handle arguments: raise if the value is not a handle; return null (after
// one warning) if the object no longer exists
dumb_ptr<map_session_data> check_player(lua_State* L, int idx);
dumb_ptr<npc_data> check_npc(lua_State* L, int idx);
dumb_ptr<block_list> check_being(lua_State* L, int idx);

void push_string(lua_State* L, XString s);
// push the (cached) handle table for the object; nil for null
void push_player(lua_State* L, dumb_ptr<map_session_data> sd);
void push_npc(lua_State* L, dumb_ptr<npc_data> nd);

// Log a script-visible warning with the current NPC/event/player context.
// DESIGN DEVIATION: the design sketches lua_warn(fmt, ...); tmwa's cxxstdio
// needs compile-time format literals, so callers format with STRPRINTF and
// pass the finished message.
void lua_warn(XString msg);
} // namespace map
} // namespace tmwa
