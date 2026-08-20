#include "lua-libs.hpp"
//    lua-lib-players.cpp - the `players` namespace.
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

#include "../compat/option.hpp"

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"
#include "../strings/zstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/socket.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "clif.hpp"
#include "map.hpp"
#include "lua-callback.hpp"
#include "lua-engine.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// Every binding follows the longjmp discipline of doc/lua-engine.md
// section 11. The list functions iterate the session table with only
// trivially-destructible locals alive, so the table pushes may raise
// (LUA_ERRMEM) without skipping any destructor.

static
dumb_ptr<map_session_data> session_of(Session* s)
{
    if (!s)
        return nullptr;
    dumb_ptr<map_session_data> sd =
            dumb_ptr<map_session_data>(static_cast<map_session_data*>(s->session_data.get()));
    if (sd && sd->state.auth)
        return sd;
    return nullptr;
}

// players.byid(id) -> handle or nil (nil when offline)
// Old: attachrid / map_id2sd uses
static
int lpl_byid(lua_State* L)
{
    int id = check_int(L, 1);
    dumb_ptr<map_session_data> sd;
    {
        sd = map_id_is_player(wrap<BlockId>(static_cast<uint32_t>(id)));
    }
    push_player(L, sd);
    return 1;
}

// players.byname(name) -> handle or nil (online only)
static
int lpl_byname(lua_State* L)
{
    ZString name = check_string(L, 1);
    dumb_ptr<map_session_data> sd = nullptr;
    if (name.size() <= 23)
    {
        sd = map_nick2sd(stringish<CharName>(name));
    }
    push_player(L, sd);
    return 1;
}

// players.bycharid(charid) -> handle or nil
// Old: the >= 150000 char-id rule in set/get
static
int lpl_bycharid(lua_State* L)
{
    int charid = check_int(L, 1);
    dumb_ptr<map_session_data> sd;
    {
        CharName nick = map_charid2nick(wrap<CharId>(static_cast<uint32_t>(charid)));
        sd = map_nick2sd(nick);
    }
    push_player(L, sd);
    return 1;
}

// players.all() -> 1-based sequence of handles (snapshot)
static
int lpl_all(lua_State* L)
{
    lua_newtable(L);
    int n = 0;
    for (io::FD i : iter_fds())
    {
        dumb_ptr<map_session_data> sd = session_of(get_session(i));
        if (sd == nullptr)
            continue;
        push_player(L, sd);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// players.onmap(map) -> 1-based sequence of handles (snapshot)
static
int lpl_onmap(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    bool known;
    {
        known = map_mapname2mapid(mapname).is_some();
        if (!known)
            lua_warn(STRPRINTF("players.onmap: unknown map '%s'"_fmt, mapname));
    }
    lua_newtable(L);
    if (!known)
        return 1;
    int n = 0;
    for (io::FD i : iter_fds())
    {
        dumb_ptr<map_session_data> sd = session_of(get_session(i));
        if (sd == nullptr)
            continue;
        if (sd->bl_m->name_ != mapname)
            continue;
        push_player(L, sd);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// players.inarea(map, x0, y0, x1, y1) -> 1-based sequence of handles
static
int lpl_inarea(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    bool known;
    {
        known = map_mapname2mapid(mapname).is_some();
        if (!known)
            lua_warn(STRPRINTF("players.inarea: unknown map '%s'"_fmt, mapname));
    }
    lua_newtable(L);
    if (!known)
        return 1;
    int n = 0;
    for (io::FD i : iter_fds())
    {
        dumb_ptr<map_session_data> sd = session_of(get_session(i));
        if (sd == nullptr)
            continue;
        if (sd->bl_m->name_ != mapname)
            continue;
        if (sd->bl_x < x0 || sd->bl_x > x1 || sd->bl_y < y0 || sd->bl_y > y1)
            continue;
        push_player(L, sd);
        lua_rawseti(L, -2, ++n);
    }
    return 1;
}

// players.count() -> int (authed players on this map server)
static
int lpl_count(lua_State* L)
{
    int n;
    {
        n = clif_countusers();
    }
    return luac::push_int(L, n);
}

// ------------------------------------------------------------------------

static
const luaL_Reg players_funcs[] =
{
    {"byid", lpl_byid},
    {"byname", lpl_byname},
    {"bycharid", lpl_bycharid},
    {"all", lpl_all},
    {"onmap", lpl_onmap},
    {"inarea", lpl_inarea},
    {"count", lpl_count},
    {nullptr, nullptr},
};

void lua_register_lib_players(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);
    lua_newtable(L);
    luac::register_funcs(L, -1, players_funcs);
    lua_setfield(L, env, "players");
}
} // namespace map
} // namespace tmwa
