#pragma once
//    lua-item-scripts.hpp - item use/equip script compilation and execution.
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

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../strings/xstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/ids.hpp"


namespace tmwa
{
namespace map
{
// Compile one item_db script column with the prelude "local p, args = ...",
// chunk name "=item_db:<id>:use|equip", env = sandbox. Empty body stores
// lua_noref (fast-path skip) and succeeds. Returns false on a compile error
// (already logged; the caller makes it a startup fatal).
bool lua_compile_item_script(XString body, ItemNameId nameid, bool is_equip,
        int* out_ref);

// Release a compiled item script's registry ref (no-op for lua_noref);
// used when a duplicate item_db id overwrites an earlier row.
void lua_item_script_unref(int script_ref);

// Use script: dialog coroutine on the #itemdialog NPC, args = { itemId }.
// The caller (pc_useitem) captures script_ref before pc_delitem.
void lua_item_use(dumb_ptr<map_session_data> sd, int script_ref,
        ItemNameId nameid);

// Equip script: synchronous inside pc_calcstatus, args = { itemId, slotId },
// with sd->lua.in_calcstatus set around the run.
void lua_item_equip(dumb_ptr<map_session_data> sd, int script_ref, int slot,
        ItemNameId nameid);
} // namespace map
} // namespace tmwa
