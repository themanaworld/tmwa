#pragma once
//    lua-npc.hpp - NPC content constructors, lifecycle, and handle plumbing.
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

// The npc namespace itself (npc.script/warp/shop/monster/mapflag, npc.get/
// byid/exists/event/event_all, enable/disable) is registered through
// lua_register_lib_npc in lua-libs.hpp and implemented in lua-npc.cpp.

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/ids.hpp"


namespace tmwa
{
namespace map
{
// Called from npc_free_internal: cancel and release the NPC's timer slots,
// unref its definition table (puppets: their extra ref on the parent's),
// remove it from the engine registries and the hook index, unref the handle.
void lua_npc_detach(dumb_ptr<npc_data> nd);

// Register a freshly built NPC in the engine registries (npcs, npcs_byname)
// and index its broadcast/clock labels. Used by the constructors and by
// self:rename.
void lua_npc_register(dumb_ptr<npc_data> nd);

// ---------------------------------------------------------------------
// Additions to the scaffold surface (noted in the implementation report).

// Push nd's definition table (puppets: the parent's, shared under the
// puppet's own id). False (nothing pushed) when nd has none (warp, shop,
// detached). Used by the label resolution in lua-events.cpp.
bool lua_npc_push_def(lua_State* L, dumb_ptr<npc_data> nd);

// Reset the module's registry references (called from lua_events_final so
// a lua_final/lua_init cycle in tests starts clean).
void lua_npc_final();
} // namespace map
} // namespace tmwa
