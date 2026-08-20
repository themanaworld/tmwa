#include "lua-value.hpp"
//    lua-value.cpp - argument checking and value pushing for the bindings.
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

#include <algorithm>

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../compat/option.hpp"

#include "../io/cxxstdio.hpp"

#include "itemdb.hpp"
#include "map.hpp"
#include "lua-engine.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// Every raising path in this file happens while only trivially-destructible
// locals are alive (doc/lua-engine.md section 11).

int check_int(lua_State* L, int idx)
{
    int out;
    if (luac::to_int(L, idx, &out))
        return out;
    luaL_argerror(L, idx, "integer expected");
    return 0; // unreachable
}

int opt_int(lua_State* L, int idx, int def)
{
    if (lua_isnoneornil(L, idx))
        return def;
    return check_int(L, idx);
}

bool check_bool(lua_State* L, int idx)
{
    if (!lua_isboolean(L, idx))
        luaL_argerror(L, idx, "boolean expected");
    return lua_toboolean(L, idx);
}

ZString check_string(lua_State* L, int idx)
{
    if (lua_type(L, idx) != LUA_TSTRING)
        luaL_argerror(L, idx, "string expected");
    size_t len;
    ZString z = luac::to_string(L, idx, &len);
    // wire formats are NUL-terminated: reject embedded NUL bytes
    if (std::find(z.begin(), z.begin() + len, '\0') != z.begin() + len)
        luaL_argerror(L, idx, "string contains NUL");
    return z;
}

ZString opt_string(lua_State* L, int idx, ZString def)
{
    if (lua_isnoneornil(L, idx))
        return def;
    return check_string(L, idx);
}

MapName check_mapname(lua_State* L, int idx)
{
    ZString z = check_string(L, idx);
    if (z.size() > 15)
        luaL_argerror(L, idx, "map name too long (max 15)");
    return stringish<MapName>(z);
}

ItemNameId check_item(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TSTRING)
    {
        ZString name = check_string(L, idx);
        ItemNameId nameid;
        {
            Option<P<struct item_data>> item_ = itemdb_searchname(name);
            OMATCH_BEGIN_SOME (item, item_)
            {
                nameid = item->nameid;
            }
            OMATCH_END ();
        }
        if (!nameid)
            lua_warn(STRPRINTF("unknown item name '%s'"_fmt, name));
        return nameid;
    }
    int num = check_int(L, idx);
    if (num <= 0 || num > 65535)
    {
        lua_warn(STRPRINTF("unknown item id %d"_fmt, num));
        return ItemNameId();
    }
    ItemNameId nameid = wrap<ItemNameId>(static_cast<uint16_t>(num));
    if (itemdb_exists(nameid).is_none())
    {
        lua_warn(STRPRINTF("unknown item id %d"_fmt, num));
        return ItemNameId();
    }
    return nameid;
}

NpcEvent check_event(lua_State* L, int idx)
{
    ZString z = check_string(L, idx);
    // "Npc::Label"; also "Npc::" (click body) and "::OnX" (broadcast)
    XString whole = z;
    LString colons = "::"_s;
    auto it = std::search(whole.begin(), whole.end(),
            colons.begin(), colons.end());
    if (it == whole.end())
        luaL_argerror(L, idx, "event \"Npc::Label\" expected");
    XString npc = whole.xislice_h(it);
    XString label = whole.xislice_t(it + 2);
    if (npc.size() > 23)
        luaL_argerror(L, idx, "event NPC name too long (max 23)");
    if (label.size() > 23)
        luaL_argerror(L, idx, "event label too long (max 23)");
    NpcEvent ev;
    ev.npc = stringish<NpcName>(npc);
    ev.label = stringish<ScriptLabel>(label);
    return ev;
}

dumb_ptr<map_session_data> check_player(lua_State* L, int idx)
{
    BlockId id = lua_check_handle_id(L, idx);
    dumb_ptr<map_session_data> sd = map_id_is_player(id);
    if (sd == nullptr)
        lua_warn(STRPRINTF("stale player handle (id %d)"_fmt,
                    unwrap<BlockId>(id)));
    return sd;
}

dumb_ptr<npc_data> check_npc(lua_State* L, int idx)
{
    BlockId id = lua_check_handle_id(L, idx);
    dumb_ptr<npc_data> nd = map_id_is_npc(id);
    if (nd == nullptr)
        lua_warn(STRPRINTF("stale NPC handle (id %d)"_fmt,
                    unwrap<BlockId>(id)));
    return nd;
}

dumb_ptr<block_list> check_being(lua_State* L, int idx)
{
    BlockId id = lua_check_handle_id(L, idx);
    dumb_ptr<block_list> bl = map_id2bl(id);
    if (bl == nullptr)
        lua_warn(STRPRINTF("stale being handle (id %d)"_fmt,
                    unwrap<BlockId>(id)));
    return bl;
}

void push_string(lua_State* L, XString s)
{
    luac::push_string(L, s);
}

void push_player(lua_State* L, dumb_ptr<map_session_data> sd)
{
    lua_push_player_handle(L, sd);
}

void push_npc(lua_State* L, dumb_ptr<npc_data> nd)
{
    lua_push_npc_handle(L, nd);
}

void lua_warn(XString msg)
{
    LuaCtx ctx = lua_current_ctx();
    AString npc_name;
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(ctx.npc);
        if (nd != nullptr)
            npc_name = AString(nd->name);
    }
    PRINTF("lua: warning: %s [%s npc=%s player=%d]\n"_fmt,
            AString(msg),
            ZString(strings::really_construct_from_a_pointer, ctx.what, nullptr),
            npc_name, unwrap<BlockId>(ctx.player));
}
} // namespace map
} // namespace tmwa
