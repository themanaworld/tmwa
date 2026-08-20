#pragma once
//    lua-libs.hpp - registration of the script-facing namespace tables.
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

// Each function builds its namespace table(s) and stores them as fields of
// the sandbox environment table at env_idx. Called once from lua_init.
// One implementation file per namespace (doc/lua-engine.md section 1) so the
// implementation agents can work in parallel.

#include "fwd.hpp"

#include "lua-compat.hpp"


namespace tmwa
{
namespace map
{
void lua_register_lib_npc(lua_State* L, int env_idx);     // lua-npc.cpp
void lua_register_lib_map(lua_State* L, int env_idx);     // lua-lib-map.cpp
void lua_register_lib_mob(lua_State* L, int env_idx);     // lua-lib-mob.cpp
void lua_register_lib_item(lua_State* L, int env_idx);    // lua-lib-item.cpp
void lua_register_lib_players(lua_State* L, int env_idx); // lua-lib-players.cpp
void lua_register_lib_server(lua_State* L, int env_idx);  // lua-lib-server.cpp
// world / world.str / world.array plus worldtmp / worldtmpstr
void lua_register_lib_world(lua_State* L, int env_idx);   // lua-mapreg.cpp
} // namespace map
} // namespace tmwa
