#include "lua-libs.hpp"
//    lua-lib-server.cpp - the `server` namespace.
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

#include "../compat/option.hpp"
#include "../compat/time_t.hpp"

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"
#include "../strings/zstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "../high/core.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "map.hpp"
#include "path.hpp"
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
// Every binding follows the longjmp discipline of doc/lua-engine.md
// section 11: raising checks first (trivial locals only), then a worker
// scope with tmwa types and no raising Lua API, then result pushes.

// Display text: strings, or integral numbers formatted %d (doc/lua-api.md
// section 1.3). check phase: raises for anything else.
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

// server.announce(text, flag [, source])   Old: announce
// flag & 0xf == 0: server-wide via the char server; otherwise `source`
// (player or NPC handle) is required (replaces the old rid/oid pick).
static
int ls_announce(lua_State* L)
{
    check_display_text(L, 1);
    int flag = check_int(L, 2);
    if (flag & 0x0f)
    {
        dumb_ptr<block_list> bl = check_being(L, 3);
        if (bl == nullptr)
            return 0;           // stale handle: warned by check_being
        {
            AString text = display_text(L, 1);
            clif_GMmessage(bl, text, flag);
        }
    }
    else
    {
        AString text = display_text(L, 1);
        intif_GMmessage(text);
    }
    return 0;
}

// server.getusers(flag) -> int; 1 = world-wide count   Old: getusers
static
int ls_getusers(lua_State* L)
{
    int flag = check_int(L, 1);
    int val = 0;
    {
        switch (flag & 0x07)
        {
            case 0:
                // the old builtin's map-local count was disabled too
                lua_warn("server.getusers(0) is disabled; use map.getmapusers() instead"_s);
                break;
            case 1:
                val = map_getusers();
                break;
        }
    }
    return luac::push_int(L, val);
}

// server.gettimetick(type) -> int; 0 ms tick (wraps), 1 seconds since UTC
// midnight, 2 unix seconds   Old: gettimetick
static
int ls_gettimetick(lua_State* L)
{
    int type = check_int(L, 1);
    int val;
    {
        switch (type)
        {
            case 1:
            {
                struct tm t = TimeT::now();
                val = t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
                break;
            }
            case 2:
                val = static_cast<time_t>(TimeT::now());
                break;
            case 0:
            default:
                val = static_cast<int>(gettick().time_since_epoch().count());
                break;
        }
    }
    return luac::push_int(L, val);
}

// server.gettime(type) -> int (UTC); 1 sec, 2 min, 3 hour, 4 weekday (0-6),
// 5 month day, 6 month (1-12), 7 year; else -1   Old: gettime
static
int ls_gettime(lua_State* L)
{
    int type = check_int(L, 1);
    int val;
    {
        struct tm t = TimeT::now();
        switch (type)
        {
            case 1: val = t.tm_sec; break;
            case 2: val = t.tm_min; break;
            case 3: val = t.tm_hour; break;
            case 4: val = t.tm_wday; break;
            case 5: val = t.tm_mday; break;
            case 6: val = t.tm_mon + 1; break;
            case 7: val = t.tm_year + 1900; break;
            default: val = -1; break;
        }
    }
    return luac::push_int(L, val);
}

// server.tick() -> int (ms tick, same as gettimetick(0))   Old: gettick()
static
int ls_tick(lua_State* L)
{
    int val;
    {
        val = static_cast<int>(gettick().time_since_epoch().count());
    }
    return luac::push_int(L, val);
}

// server.debugmes(text)   Old: debugmes (rid/oid become the current context)
static
int ls_debugmes(lua_State* L)
{
    check_display_text(L, 1);
    {
        LuaCtx ctx = lua_current_ctx();
        AString mes = display_text(L, 1);
        PRINTF("script debug: %d %d: '%s'\n"_fmt,
                unwrap<BlockId>(ctx.player), unwrap<BlockId>(ctx.npc), mes);
    }
    return 0;
}

// server.wgm(text)   Old: wgm
static
int ls_wgm(lua_State* L)
{
    check_display_text(L, 1);
    {
        AString message = display_text(L, 1);
        intif_wis_message_to_gm(WISP_SERVER_NAME,
                battle_config.hack_info_GM_level,
                STRPRINTF("[GM] %s"_fmt, message));
    }
    return 0;
}

// server.getbattleconfig(key) -> int, -1 unknown   Old: getbattleconfig
static
int ls_getbattleconfig(lua_State* L)
{
    ZString key = check_string(L, 1);
    int32_t value = -1;
    {
        if (!get_battle_conf(battle_config, key, &value))
            lua_warn(STRPRINTF("server.getbattleconfig: unknown battle config setting: %s"_fmt,
                        key));
    }
    return luac::push_int(L, value);
}

// server.mapexit(); sets runflag = 0 and does NOT terminate the handler
// (as before)   Old: mapexit
static
int ls_mapexit(lua_State*)
{
    runflag = false;
    return 0;
}

// server.isloggedin(id) -> bool   Old: isloggedin
static
int ls_isloggedin(lua_State* L)
{
    int id = check_int(L, 1);
    bool r;
    {
        r = map_id2sd(wrap<BlockId>(static_cast<uint32_t>(id))) != nullptr;
    }
    lua_pushboolean(L, r);
    return 1;
}

// server.distance(id1, id2) -> int   Old: distance (id-taking form; the old
// builtin dereferenced null on bad ids, here it warns and returns 0)
static
int ls_distance(lua_State* L)
{
    int id1 = check_int(L, 1);
    int id2 = check_int(L, 2);
    int distance = 0;
    {
        dumb_ptr<block_list> source =
                map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id1)));
        dumb_ptr<block_list> target =
                map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id2)));
        if (source == nullptr || target == nullptr)
        {
            lua_warn("server.distance: invalid block id"_s);
            return luac::push_int(L, 0);
        }
        if (source->bl_m != target->bl_m)
        {
            // FIXME make it work even if source and target are not on the
            // same map
            distance = 0x7fffffff;
        }
        else
        {
            int dx = abs(source->bl_x - target->bl_x);
            int dy = abs(source->bl_y - target->bl_y);
            distance = sqrt((dx * dx) + (dy * dy));
        }
    }
    return luac::push_int(L, distance);
}

// server.target(src, tgt, flags) -> int bitmask   Old: target
static
int ls_target(lua_State* L)
{
    int id1 = check_int(L, 1);
    int id2 = check_int(L, 2);
    int flag = check_int(L, 3);
    int val = 0;
    {
        dumb_ptr<block_list> source =
                map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id1)));
        dumb_ptr<block_list> target =
                map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id2)));
        if (!source || !target)
            return luac::push_int(L, 0);

        if (flag & 0x01)
        {
            int x0 = source->bl_x - AREA_SIZE;
            int y0 = source->bl_y - AREA_SIZE;
            int x1 = source->bl_x + AREA_SIZE;
            int y1 = source->bl_y + AREA_SIZE;
            if (target->bl_x >= x0 && target->bl_x <= x1
                && target->bl_y >= y0 && target->bl_y <= y1)
                val |= 0x01;    // target is in visible range
        }

        if (flag & 0x02)
        {
            int range = battle_get_range(source);
            int x2 = source->bl_x - range;
            int y2 = source->bl_y - range;
            int x3 = source->bl_x + range;
            int y3 = source->bl_y + range;
            if (target->bl_x >= x2 && target->bl_x <= x3
                && target->bl_y >= y2 && target->bl_y <= y3)
                val |= 0x02;    // target is in attack range
        }

        if (flag & 0x04)
        {
            struct walkpath_data wpd;
            if (!path_search(&wpd, source->bl_m, source->bl_x, source->bl_y,
                        target->bl_x, target->bl_y, 0))
                val |= 0x04;    // clear walkable path to target
        }

        // TODO 0x08 target is visible (not behind collision)

        if (flag & 0x10)
        {
            if (target->bl_type != BL::PC
                || (target->bl_type == BL::PC
                    && (target->bl_m->flag.get(MapFlag::PVP)
                        || pc_iskiller(source->is_player(),
                                target->is_player()))))
                val |= 0x10;    // target can be attacked by source
        }

        if (flag & 0x20)
        {
            if (battle_check_range(source, target, 0))
                val |= 0x20;    // target is in line of sight
        }
    }
    return luac::push_int(L, val);
}

// server.injure(src, tgt, dmg)   Old: injure (id-taking form; the old
// builtin dereferenced null on bad ids, here it warns and returns)
static
int ls_injure(lua_State* L)
{
    int id1 = check_int(L, 1);
    int id2 = check_int(L, 2);
    int damage_caused = check_int(L, 3);
    {
        dumb_ptr<block_list> source =
                map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id1)));
        dumb_ptr<block_list> target =
                map_id2bl(wrap<BlockId>(static_cast<uint32_t>(id2)));
        if (source == nullptr || target == nullptr)
        {
            lua_warn("server.injure: invalid block id"_s);
            return 0;
        }

        if (source->bl_type == BL::PC)
            pc_setstand(source->is_player());

        // display damage first, because dealing damage may deallocate the
        // target
        clif_damage(source, target,
                gettick(), interval_t::zero(), interval_t::zero(),
                damage_caused, 0, DamageType::NORMAL);

        battle_damage(source, target, damage_caused, 0);
    }
    return 0;
}

// server.registercmd(word, handler)   Old: registercmd
// handler: "Npc::OnX", an NPC name (the click body, the old
// `registercmd word, strnpcinfo(0)` idiom), or a function (D6).
static
int ls_registercmd(lua_State* L)
{
    ZString word = check_string(L, 1);
    if (!word.size())
        luaL_argerror(L, 1, "non-empty command word expected");
    LuaCallback cb;
    if (lua_type(L, 2) == LUA_TSTRING)
    {
        ZString handler = check_string(L, 2);
        XString whole = handler;
        LString colons = "::"_s;
        auto it = std::search(whole.begin(), whole.end(),
                colons.begin(), colons.end());
        NpcEvent ev;
        if (it != whole.end())
        {
            ev = check_event(L, 2);
        }
        else
        {
            // bare NPC name: the click body
            if (!whole.size())
                luaL_argerror(L, 2, "event or NPC name expected");
            if (whole.size() > 23)
                luaL_argerror(L, 2, "NPC name too long (max 23)");
            ev.npc = stringish<NpcName>(whole);
        }
        cb = lua_cb_named(ev);
    }
    else
    {
        cb = lua_cb_from_stack(L, 2);   // function form; raises otherwise
    }
    {
        lua_register_command(RString(word), cb);
    }
    return 0;
}

// ------------------------------------------------------------------------

static
const luaL_Reg server_funcs[] =
{
    {"announce", ls_announce},
    {"getusers", ls_getusers},
    {"gettimetick", ls_gettimetick},
    {"gettime", ls_gettime},
    {"tick", ls_tick},
    {"debugmes", ls_debugmes},
    {"wgm", ls_wgm},
    {"getbattleconfig", ls_getbattleconfig},
    {"mapexit", ls_mapexit},
    {"isloggedin", ls_isloggedin},
    {"distance", ls_distance},
    {"target", ls_target},
    {"injure", ls_injure},
    {"registercmd", ls_registercmd},
    {nullptr, nullptr},
};

void lua_register_lib_server(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);
    lua_newtable(L);
    luac::register_funcs(L, -1, server_funcs);
    lua_setfield(L, env, "server");
}
} // namespace map
} // namespace tmwa
