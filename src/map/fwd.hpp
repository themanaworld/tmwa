#ifndef TMWA_MAP_FWD_HPP
#define TMWA_MAP_FWD_HPP
//    map/fwd.hpp - list of type names for map server
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include "../sanity.hpp"

// meh, add more when I feel like it
class BlockId;
struct block_list;
struct map_session_data;
struct npc_data;
struct mob_data;
struct flooritem_data;
struct invocation;
struct map_local;
class npc_data_script;
class npc_data_shop;
class npc_data_warp;
class npc_data_message;
struct NpcEvent;

struct item_data;

// magic
struct fun_t;
struct op_t;
struct expr_t;
struct val_t;
struct location_t;
struct area_t;
struct spell_t;
struct invocation;

#endif // TMWA_MAP_FWD_HPP
