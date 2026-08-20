#include "lua-item-scripts.hpp"
//    lua-item-scripts.cpp - item use/equip script compilation and execution.
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

#include <set>

#include "../strings/astring.hpp"

#include "../io/cxxstdio.hpp"

#include "../ints/wrap.hpp"

#include "map.hpp"
#include "lua-dialog.hpp"
#include "lua-engine.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// doc/lua-engine.md section 7.2: an equip script that errors (typically by
// trying to yield: dialog primitives are not available inside pc_calcstatus)
// would produce the same log line on every recalculation. The first failure
// is reported through the normal error path; afterwards the item id lands in
// this set and its equip script is skipped, so the error is logged once per
// item id and the remaining equip scripts still run.
static
std::set<ItemNameId> equip_script_failed;

bool lua_compile_item_script(XString body, ItemNameId nameid, bool is_equip,
        int* out_ref)
{
    *out_ref = lua_noref;

    // empty (or whitespace-only) column: nothing to run, fast-path skip
    bool empty = true;
    for (char c : body)
    {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        {
            empty = false;
            break;
        }
    }
    if (empty)
        return true;

    lua_State* L = lua_state();
    if (!L)
        return false;

    AString chunkname = STRPRINTF("=item_db:%d:%s"_fmt,
            unwrap<ItemNameId>(nameid), is_equip ? "equip"_s : "use"_s);
    AString src = STRPRINTF("local p, args = ...\n%s"_fmt, AString(body));
    if (!lua_load_chunk_sandboxed(chunkname, src))
        return false;   // the compile error is already logged
    *out_ref = luac::ref(L);
    return true;
}

void lua_item_script_unref(int script_ref)
{
    lua_State* L = lua_state();
    if (L != nullptr)
        luac::unref(L, script_ref);
}

void lua_item_use(dumb_ptr<map_session_data> sd, int script_ref,
        ItemNameId nameid)
{
    if (sd == nullptr || script_ref == lua_noref)
        return;
    lua_State* L = lua_state();
    if (!L)
        return;
    dumb_ptr<npc_data> nd = lua_item_dialog_npc();
    if (nd == nullptr)
    {
        lua_warn("item use script: no #itemdialog NPC"_s);
        return;
    }

    // stack for the driver: [function, p, args]
    luac::push_ref(L, script_ref);
    push_player(L, sd);
    lua_newtable(L);
    lua_pushinteger(L, unwrap<ItemNameId>(nameid));
    lua_setfield(L, -2, "itemId");

    LuaCtx ctx;
    ctx.npc = nd->bl_id;
    ctx.player = sd->bl_id;
    ctx.what = "item use script";
    // clif_parse_UseItem only forwards the packet when npc_id is empty, so
    // this cannot clobber an active dialog (same as the other dialog
    // entry points in lua-events.cpp)
    sd->npc_id = nd->bl_id;
    lua_run_dialog(sd, nd, ctx, 2);
}

void lua_item_equip(dumb_ptr<map_session_data> sd, int script_ref, int slot,
        ItemNameId nameid)
{
    if (sd == nullptr || script_ref == lua_noref)
        return;
    lua_State* L = lua_state();
    if (!L)
        return;
    if (equip_script_failed.count(nameid))
        return;   // already failed and logged once, see above

    // stack for the driver: [function, p, args]
    luac::push_ref(L, script_ref);
    push_player(L, sd);
    lua_newtable(L);
    lua_pushinteger(L, unwrap<ItemNameId>(nameid));
    lua_setfield(L, -2, "itemId");
    lua_pushinteger(L, slot);
    lua_setfield(L, -2, "slotId");

    LuaCtx ctx;
    ctx.player = sd->bl_id;
    ctx.what = "item equip script";

    bool outer = sd->lua.in_calcstatus;
    sd->lua.in_calcstatus = true;
    bool ok = lua_run_sync(ctx, 2);
    sd->lua.in_calcstatus = outer;
    if (!ok)
        equip_script_failed.insert(nameid);
}
} // namespace map
} // namespace tmwa
