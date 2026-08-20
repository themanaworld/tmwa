#pragma once
//    npc-parse.hpp - NPC builders shared by the Lua constructors and @addwarp.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "fwd.hpp"

#include <vector>

#include "../net/timer.t.hpp"

#include "map.hpp"


namespace tmwa
{
namespace map
{
// The conf 'npc:'/'delnpc:' list (now .lua files, loaded by the engine).
void npc_addsrcfile(AString name);
void npc_delsrcfile(XString name);
void register_npc_name(dumb_ptr<npc_data> nd);

// C++ builders behind the Lua content constructors (lua-npc.cpp) and
// @addwarp. Each is the corresponding old npc_load_* body carved free of
// the AST and bytecode parts.

// xs_file/ys_file are the raw file numbers; the builder adds 2 exactly like
// the old warp parser (-1 keeps meaning "span 1"). nullptr on unknown map.
dumb_ptr<npc_data_warp> npc_create_warp(MapName mapname, int x, int y,
        int xs_file, int ys_file, MapName to_map, int to_x, int to_y);

// item list already resolved to absolute prices. nullptr on unknown map.
dumb_ptr<npc_data_shop> npc_create_shop(NpcName name, MapName mapname,
        int x, int y, DIR dir, Species npc_class,
        std::vector<npc_item_list> items);

// applies battle_config.mob_count_rate; returns the number of mobs
// spawned, -1 on unknown map.
int npc_create_monster(MapName mapname, int x, int y, int xs, int ys,
        MobName name, Species mob_class, int amount,
        interval_t delay1, interval_t delay2, NpcEvent event);

// NOPVP clears PVP; NOSAVE/RESAVE use extra_map/x/y; MASK uses mask.
bool npc_set_mapflag(MapName mapname, MapFlag mf, MapName extra_map,
        int extra_x, int extra_y, int mask);

// placed == false: floating NPC on undefined_gat, INVISIBLE_CLASS forced;
// placed == true: on the map, spawned to clients. xs/ys are the stored
// diameters (already 2n+1 or 0). nullptr on unknown map.
dumb_ptr<npc_data_script> npc_create_script_npc(NpcName name,
        MapName mapname, bool placed, int x, int y, DIR dir,
        Species npc_class, int xs, int ys);
} // namespace map
} // namespace tmwa
