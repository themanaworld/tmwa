#include "lua-libs.hpp"
//    lua-lib-map.cpp - the `map` namespace.
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

#include <cmath>

#include <algorithm>

#include "../compat/fun.hpp"
#include "../compat/option.hpp"

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/socket.hpp"
#include "../net/timer.hpp"

#include "../mmo/ids.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "pc.hpp"
#include "lua-callback.hpp"
#include "lua-engine.hpp"
#include "lua-events.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// Implemented in lua-timers.cpp (doc/lua-engine.md section 6); that TU has
// no header of its own, its declarations move into pc.hpp at host
// integration time. Declared here so this TU builds against the
// pre-integration tree. Takes ownership of cb (a full slot table releases
// it and logs).
int lua_pc_addeventtimer(dumb_ptr<map_session_data> sd, interval_t tick,
        LuaCallback cb);

// Every binding follows the longjmp discipline of doc/lua-engine.md
// section 11: raising checks first (trivial locals only), then a worker
// scope with tmwa types and no raising Lua API, then result pushes.

// optional 0/1-or-boolean flag argument (the old builtins used conv_num)
static
bool opt_flag(lua_State* L, int idx)
{
    if (lua_isnoneornil(L, idx))
        return false;
    if (lua_isboolean(L, idx))
        return lua_toboolean(L, idx);
    return check_int(L, idx) != 0;
}

// Display text: strings, or integral numbers formatted %d (doc/lua-api.md
// section 1.3). check phase: raises for anything else. The formatted result
// is produced later, inside the caller's worker scope, via display_text().
static
void check_display_text(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TSTRING)
    {
        check_string(L, idx);
        return;
    }
    int dummy;
    if (!luac::to_int(L, idx, &dummy))
        luaL_argerror(L, idx, "text expected");
}

// worker-scope companion of check_display_text: never raises
static
AString display_text(lua_State* L, int idx)
{
    if (lua_type(L, idx) == LUA_TSTRING)
    {
        size_t len;
        return AString(luac::to_string(L, idx, &len));
    }
    int v = 0;
    luac::to_int(L, idx, &v);
    return STRPRINTF("%d"_fmt, v);
}

// ------------------------------------------------------------------------
// warps

static
void areawarp_sub(dumb_ptr<block_list> bl, MapName to_map, int x, int y)
{
    pc_setpos(bl->is_player(), to_map, x, y, BeingRemoveWhy::GONE);
}

// map.areawarp(map, x0, y0, x1, y1, to_map, to_x, to_y)   Old: areawarp
static
int lm_areawarp(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    MapName to_map = check_mapname(L, 6);
    int x = check_int(L, 7);
    int y = check_int(L, 8);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.areawarp: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        map_foreachinarea(std::bind(areawarp_sub, ph::_1, to_map, x, y),
                m, x0, y0, x1, y1, BL::PC);
    }
    return 0;
}

// map.mapwarp(map, to_map, to_x, to_y)   Old: mapwarp
static
int lm_mapwarp(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    MapName to_map = check_mapname(L, 2);
    int x = check_int(L, 3);
    int y = check_int(L, 4);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.mapwarp: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        map_foreachinarea(std::bind(areawarp_sub, ph::_1, to_map, x, y),
                m, 0, 0, m->xs, m->ys, BL::PC);
    }
    return 0;
}

// ------------------------------------------------------------------------
// user counts

static
void getareausers_sub(dumb_ptr<block_list> bl, int* users)
{
    if (bool(bl->is_player()->status.option & Opt0::HIDE))
        return;
    (*users)++;
}

static
void getareausers_living_sub(dumb_ptr<block_list> bl, int* users)
{
    if (bool(bl->is_player()->status.option & Opt0::HIDE))
        return;
    if (!pc_isdead(bl->is_player()))
        (*users)++;
}

// map.getmapusers(map) -> int, -1 unknown map   Old: getmapusers
static
int lm_getmapusers(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int users = 0;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
                { return luac::push_int(L, -1); });
        map_foreachinarea(std::bind(getareausers_sub, ph::_1, &users),
                m, 0, 0, m->xs, m->ys, BL::PC);
    }
    return luac::push_int(L, users);
}

// map.getareausers(map, x0, y0, x1, y1 [, living]) -> int   Old: getareausers
static
int lm_getareausers(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    bool living = opt_flag(L, 6);
    int users = 0;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
                { return luac::push_int(L, -1); });
        map_foreachinarea(std::bind(living ? getareausers_living_sub
                                           : getareausers_sub, ph::_1, &users),
                m, x0, y0, x1, y1, BL::PC);
    }
    return luac::push_int(L, users);
}

// ------------------------------------------------------------------------
// map flags, pvp, mask

// map.getmapflag(map, flag) -> int, -1 unknown map   Old: getmapflag
static
int lm_getmapflag(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int i = check_int(L, 2);
    int r = -1;
    {
        MapFlag mf = map_flag_from_int(i);
        Option<P<map_local>> m_ = map_mapname2mapid(mapname);
        OMATCH_BEGIN_SOME (m, m_)
        {
            r = m->flag.get(mf);
        }
        OMATCH_END ();
    }
    return luac::push_int(L, r);
}

// map.setmapflag(map, flag)   Old: setmapflag
static
int lm_setmapflag(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int i = check_int(L, 2);
    {
        MapFlag mf = map_flag_from_int(i);
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.setmapflag: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        m->flag.set(mf, 1);
    }
    return 0;
}

// map.removemapflag(map, flag)   Old: removemapflag
static
int lm_removemapflag(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int i = check_int(L, 2);
    {
        MapFlag mf = map_flag_from_int(i);
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.removemapflag: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        m->flag.set(mf, 0);
    }
    return 0;
}

// map.pvpon(map)   Old: pvpon
static
int lm_pvpon(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.pvpon: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        if (!m->flag.get(MapFlag::PVP) && !m->flag.get(MapFlag::NOPVP))
        {
            m->flag.set(MapFlag::PVP, 1);

            // disable ranking functions if pk_mode is on [Valaris]
            if (battle_config.pk_mode)
                return 0;

            for (io::FD i : iter_fds())
            {
                Session* s = get_session(i);
                if (!s)
                    continue;
                dumb_ptr<map_session_data> pl_sd =
                        dumb_ptr<map_session_data>(static_cast<map_session_data*>(s->session_data.get()));
                if (pl_sd && pl_sd->state.auth)
                {
                    if (m == pl_sd->bl_m && !pl_sd->pvp_timer)
                    {
                        pl_sd->pvp_timer = Timer(gettick() + 200_ms,
                                std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                                    pl_sd->bl_id));
                        clif_map_pvp(pl_sd);
                    }
                }
            }
        }
    }
    return 0;
}

// map.pvpoff(map)   Old: pvpoff
static
int lm_pvpoff(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.pvpoff: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        if (m->flag.get(MapFlag::PVP))
        {
            m->flag.set(MapFlag::PVP, 0);

            // disable ranking options if pk_mode is on [Valaris]
            if (battle_config.pk_mode)
                return 0;

            for (io::FD i : iter_fds())
            {
                Session* s = get_session(i);
                if (!s)
                    continue;
                dumb_ptr<map_session_data> pl_sd =
                        dumb_ptr<map_session_data>(static_cast<map_session_data*>(s->session_data.get()));
                if (pl_sd && pl_sd->state.auth)
                {
                    if (m == pl_sd->bl_m)
                    {
                        pl_sd->pvp_timer.cancel();
                        clif_map_pvp(pl_sd);
                    }
                }
            }
        }
    }
    return 0;
}

// map.mask(map) -> int, -1 unknown map   Old: getmask (map fallback)
static
int lm_mask(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int mask = -1;
    {
        Option<P<map_local>> m_ = map_mapname2mapid(mapname);
        OMATCH_BEGIN_SOME (m, m_)
        {
            mask = m->mask;
        }
        OMATCH_END ();
    }
    return luac::push_int(L, mask);
}

// map.setmask(map, mask)   Old: mapmask (map store part)
static
int lm_setmask(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int mask = check_int(L, 2);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.setmask: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        m->mask = mask;
    }
    return 0;
}

// ------------------------------------------------------------------------
// geometry queries

// map.iscollision(map, x, y) -> bool; raises on unknown map (deviation 13)
static
int lm_iscollision(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x = check_int(L, 2);
    int y = check_int(L, 3);
    bool r;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            return luaL_error(L, "map.iscollision: unknown map '%s'",
                    mapname.c_str());
        });
        r = bool(map_getcell(m, x, y) & MapCell::UNWALKABLE);
    }
    lua_pushboolean(L, r);
    return 1;
}

// map.getmapmaxx(map) -> int; raises on unknown map   Old: getmapmaxx
static
int lm_getmapmaxx(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int r;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            return luaL_error(L, "map.getmapmaxx: unknown map '%s'",
                    mapname.c_str());
        });
        r = m->xs - 1;
    }
    return luac::push_int(L, r);
}

// map.getmapmaxy(map) -> int; raises on unknown map   Old: getmapmaxy
static
int lm_getmapmaxy(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int r;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            return luaL_error(L, "map.getmapmaxy: unknown map '%s'",
                    mapname.c_str());
        });
        r = m->ys - 1;
    }
    return luac::push_int(L, r);
}

// map.getmaphash(map) -> int; raises on unknown map   Old: getmaphash
static
int lm_getmaphash(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int r;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            return luaL_error(L, "map.getmaphash: unknown map '%s'",
                    mapname.c_str());
        });
        r = m->hash;
    }
    return luac::push_int(L, r);
}

// map.getmapnamefromhash(h) -> string ("" if none)   Old: getmapnamefromhash
static
int lm_getmapnamefromhash(lua_State* L)
{
    int hash = check_int(L, 1);
    MapName mapname;
    {
        // linear scan, same cast as the old builtin (remote maps included)
        for (auto& mit : maps_db)
        {
            map_local* ml = static_cast<map_local*>(mit.second.get());
            if (ml->hash == hash)
            {
                mapname = ml->name_;
                break;
            }
        }
    }
    push_string(L, mapname);
    return 1;
}

// map.mapexists(map) -> bool   Old: mapexists
static
int lm_mapexists(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    bool r;
    {
        r = map_mapname2mapid(mapname).is_some();
    }
    lua_pushboolean(L, r);
    return 1;
}

// map.numberofmaps() -> int   Old: numberofmaps
static
int lm_numberofmaps(lua_State* L)
{
    int n;
    {
        n = maps_db.size();
    }
    return luac::push_int(L, n);
}

// map.getmapnamebyindex(i) -> string ("" out of range)  Old: getmapnamebyindex
static
int lm_getmapnamebyindex(lua_State* L)
{
    int index = check_int(L, 1);
    MapName mapname;
    {
        int count = 0;
        for (auto& mit : maps_db)
        {
            if (count == index)
            {
                mapname = mit.second->name_;
                break;
            }
            ++count;
        }
    }
    push_string(L, mapname);
    return 1;
}

// map.distance(map, x0, y0, x1, y1) -> int (euclidean, truncated)
static
int lm_distance(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    int distance;
    {
        if (map_mapname2mapid(mapname).is_none())
            lua_warn(STRPRINTF("map.distance: unknown map '%s'"_fmt, mapname));
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        distance = sqrt((dx * dx) + (dy * dy));
    }
    return luac::push_int(L, distance);
}

// ------------------------------------------------------------------------
// announcements

static
void mapannounce_sub(dumb_ptr<block_list> bl, XString str, int flag)
{
    clif_GMmessage(bl, str, flag | 3);
}

// map.mapannounce(map, text, flag)   Old: mapannounce
static
int lm_mapannounce(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    check_display_text(L, 2);
    int flag = check_int(L, 3);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.mapannounce: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        AString text = display_text(L, 2);
        map_foreachinarea(std::bind(mapannounce_sub, ph::_1, text, flag & 0x10),
                m, 0, 0, m->xs, m->ys, BL::PC);
    }
    return 0;
}

// ------------------------------------------------------------------------
// floor items

static
void getareadropitem_sub(dumb_ptr<block_list> bl, ItemNameId item, int* amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
        (*amount) += drop->item_data.amount;
}

static
void getareadropitem_sub_anddelete(dumb_ptr<block_list> bl, ItemNameId item,
        int* amount)
{
    dumb_ptr<flooritem_data> drop = bl->is_item();

    if (drop->item_data.nameid == item)
    {
        (*amount) += drop->item_data.amount;
        clif_clearflooritem(drop, nullptr);
        map_delobject(drop->bl_id, drop->bl_type);
    }
}

// map.getareadropitem(map, x0, y0, x1, y1, item [, delete]) -> int
// Old: getareadropitem
static
int lm_getareadropitem(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    ItemNameId nameid = check_item(L, 6);
    bool delitems = opt_flag(L, 7);
    int amount = 0;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
                { return luac::push_int(L, -1); });
        if (delitems)
            map_foreachinarea(std::bind(getareadropitem_sub_anddelete, ph::_1,
                        nameid, &amount),
                    m, x0, y0, x1, y1, BL::ITEM);
        else
            map_foreachinarea(std::bind(getareadropitem_sub, ph::_1,
                        nameid, &amount),
                    m, x0, y0, x1, y1, BL::ITEM);
    }
    return luac::push_int(L, amount);
}

// map.makeitem(item, amount, map, x, y)   Old: makeitem ("this" not accepted)
static
int lm_makeitem(lua_State* L)
{
    ItemNameId nameid = check_item(L, 1);
    int amount = check_int(L, 2);
    MapName mapname = check_mapname(L, 3);
    int x = check_int(L, 4);
    int y = check_int(L, 5);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("map.makeitem: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        if (nameid)
        {
            Item item_tmp {};
            item_tmp.nameid = nameid;
            map_addflooritem(&item_tmp, amount, m, x, y,
                    nullptr, nullptr, nullptr);
        }
    }
    return 0;
}

// ------------------------------------------------------------------------
// timers and iteration

static
void areatimer_sub(dumb_ptr<block_list> bl, interval_t tick, LuaCallback cb)
{
    dumb_ptr<map_session_data> sd = bl->is_player();
    if (sd == nullptr)
        return;
    lua_pc_addeventtimer(sd, tick, lua_cb_dup(cb));
}

// map.areatimer(map, x0, y0, x1, y1, ms, event)   Old: areatimer
// (the old leading bltype 0 argument is dropped: only players were supported)
static
int lm_areatimer(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    int ms = check_int(L, 6);
    LuaCallback cb = lua_cb_from_stack(L, 7);   // event string or function
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_cb_release(cb);
            lua_warn(STRPRINTF("map.areatimer: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        interval_t tick = static_cast<interval_t>(ms);
        map_foreachinarea(std::bind(areatimer_sub, ph::_1, tick, cb),
                m, x0, y0, x1, y1, BL::PC);
        lua_cb_release(cb);
    }
    return 0;
}

// state for one map.foreach fire; trivially copyable for std::bind
struct ForeachFire
{
    LuaCallback cb;     // owner: lm_foreach (subs use it without consuming)
    BlockId caller;
};

static
void foreach_fire(dumb_ptr<block_list> bl, ForeachFire ff)
{
    lua_State* L = lua_state();
    dumb_ptr<npc_data> nd = nullptr;
    if (ff.cb.fn_ref != lua_noref)
    {
        luac::push_ref(L, ff.cb.fn_ref);
        nd = map_id_is_npc(ff.cb.self_npc);
    }
    else
    {
        nd = npc_name2id(ff.cb.event.npc);
        if (nd == nullptr
            || nd->deletion_pending != npc_data::NOT_DELETING)
        {
            lua_warn(STRPRINTF("map.foreach: event not found [%s]"_fmt,
                        ff.cb.event));
            return;
        }
        // orphan puppet: freed here as on every other dispatch path
        dumb_ptr<npc_data_script> nds = nd->is_script();
        if (nds != nullptr && nds->scr.parent
            && map_id2bl(nds->scr.parent) == nullptr)
        {
            npc_free(nd);
            return;
        }
        if (nd->flag & 1)
            return;             // disabled NPC: dropped, as the old dispatch
        if (!lua_npc_push_handler(L, nd, ff.cb.event.label))
        {
            lua_warn(STRPRINTF("map.foreach: event not found [%s]"_fmt,
                        ff.cb.event));
            return;
        }
    }
    // handler(self, caller, args) with args.target_id
    lua_push_npc_handle(L, nd);
    lua_push_player_handle(L, map_id_is_player(ff.caller));
    lua_createtable(L, 0, 1);
    lua_pushinteger(L, static_cast<int>(unwrap<BlockId>(bl->bl_id)));
    lua_setfield(L, -2, "target_id");
    LuaCtx ctx;
    ctx.npc = nd != nullptr ? nd->bl_id : BlockId();
    ctx.player = ff.caller;
    ctx.what = "map.foreach";
    lua_run_sync(ctx, 3);
}

// map.foreach(bltype, map, x0, y0, x1, y1, event [, caller])   Old: foreach
// Synchronous nested execution; caller may be nil (deviation 11).
static
int lm_foreach(lua_State* L)
{
    int bl_num = check_int(L, 1);
    MapName mapname = check_mapname(L, 2);
    int x0 = check_int(L, 3);
    int y0 = check_int(L, 4);
    int x1 = check_int(L, 5);
    int y1 = check_int(L, 6);
    BL block_type;
    switch (bl_num)
    {
        case 0: block_type = BL::PC; break;
        case 1: block_type = BL::NPC; break;
        case 2: block_type = BL::MOB; break;
        case 3: block_type = BL::NUL; break;
        default:
            return luaL_argerror(L, 1, "bltype 0..3 expected");
    }
    dumb_ptr<map_session_data> caller = nullptr;
    if (!lua_isnoneornil(L, 8))
        caller = check_player(L, 8);
    ForeachFire ff;
    ff.cb = lua_cb_from_stack(L, 7);    // event string or function
    ff.caller = caller != nullptr ? caller->bl_id : BlockId();
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_cb_release(ff.cb);
            lua_warn(STRPRINTF("map.foreach: unknown map '%s'"_fmt, mapname));
            return 0;
        });
        map_foreachinarea(std::bind(foreach_fire, ph::_1, ff),
                m, x0, y0, x1, y1, block_type);
        lua_cb_release(ff.cb);
    }
    return 0;
}

// ------------------------------------------------------------------------

static
const luaL_Reg map_funcs[] =
{
    {"areawarp", lm_areawarp},
    {"mapwarp", lm_mapwarp},
    {"getmapusers", lm_getmapusers},
    {"getareausers", lm_getareausers},
    {"getmapflag", lm_getmapflag},
    {"setmapflag", lm_setmapflag},
    {"removemapflag", lm_removemapflag},
    {"pvpon", lm_pvpon},
    {"pvpoff", lm_pvpoff},
    {"iscollision", lm_iscollision},
    {"getmapmaxx", lm_getmapmaxx},
    {"getmapmaxy", lm_getmapmaxy},
    {"getmaphash", lm_getmaphash},
    {"getmapnamefromhash", lm_getmapnamefromhash},
    {"mapexists", lm_mapexists},
    {"numberofmaps", lm_numberofmaps},
    {"getmapnamebyindex", lm_getmapnamebyindex},
    {"mapannounce", lm_mapannounce},
    {"getareadropitem", lm_getareadropitem},
    {"makeitem", lm_makeitem},
    {"areatimer", lm_areatimer},
    {"foreach", lm_foreach},
    {"mask", lm_mask},
    {"setmask", lm_setmask},
    {"distance", lm_distance},
    {nullptr, nullptr},
};

void lua_register_lib_map(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);
    lua_newtable(L);
    luac::register_funcs(L, -1, map_funcs);
    lua_setfield(L, env, "map");
}
} // namespace map
} // namespace tmwa
