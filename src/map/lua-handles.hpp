#pragma once
//    lua-handles.hpp - being/player/NPC handle tables and metatables.
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

// Handles are Lua tables carrying the integer block id, re-resolved on every
// call; no __gc anywhere (doc/lua-api.md section 1.1). The being base lives
// in lua-handle-being.cpp, the player specialisation in lua-handle-player.cpp,
// the NPC one in lua-npc.cpp.
//
// Shared layout (established by lua-handle-being.cpp, which implements
// lua_handle_id): a handle table stores its block id in the raw integer
// field "__id"; player handles (and the p.vars/p.acc/p.acc2 proxies) also
// store the session serial in "__serial". Metatables are registered with
// luaL_newmetatable under "tmwa.being", "tmwa.player" and "tmwa.npc";
// "tmwa.npc" is pre-created with the generic being dispatch and lua-npc.cpp
// installs its NPC-specific __index/__newindex over that same table.

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/ids.hpp"


namespace tmwa
{
namespace map
{
// Create the shared handle metatables (being base, player, NPC) and register
// the dialog methods; called once from lua_init with the sandbox environment
// on top of the stack (for the global `being`).
void lua_register_handle_metatables(lua_State* L);

// Push the handle table for the object (cached: players in the session's
// handle_ref / players registry, NPCs in npcs_ref). Pushes nil for null.
void lua_push_player_handle(lua_State* L, dumb_ptr<map_session_data> sd);
void lua_push_npc_handle(lua_State* L, dumb_ptr<npc_data> nd);
// Any block id (player, mob, NPC, floor item); pushes nil if the id is dead.
void lua_push_being_handle(lua_State* L, dumb_ptr<block_list> bl);

// Read the block id out of a handle table at idx. lua_handle_id returns the
// zero id when the value is not a handle; lua_check_handle_id raises instead
// (argument-check path).
BlockId lua_handle_id(lua_State* L, int idx);
BlockId lua_check_handle_id(lua_State* L, int idx);
} // namespace map
} // namespace tmwa
