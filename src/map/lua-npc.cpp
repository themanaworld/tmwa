#include "lua-npc.hpp"
//    lua-npc.cpp - NPC content constructors, lifecycle, handle and namespace.
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
#include <vector>

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../compat/fun.hpp"
#include "../compat/option.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"

#include "../net/timer.hpp"

#include "globals.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mapflag.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "npc-parse.hpp"
#include "pc.hpp"
#include "lua-callback.hpp"
#include "lua-dialog.hpp"
#include "lua-engine.hpp"
#include "lua-events.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"
#include "lua-libs.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// ------------------------------------------------------------------------
// module state

// blockid -> definition table (doc/lua-engine.md 2.2 def_ref; engine-side
// until npc_data grows the field). Puppets share the parent's table under
// their own id, which also serves as the "extra ref on the parent's".
static
int g_defs_ref = lua_noref;

// registry key of the NPC handle metatable
static
const char npc_mt_name[] = "tmwa:npc";

void lua_npc_final()
{
    lua_State* L = lua_state();
    if (L != nullptr && g_defs_ref != lua_noref)
        luac::unref(L, g_defs_ref);
    g_defs_ref = lua_noref;
}

static
void ensure_defs(lua_State* L)
{
    if (g_defs_ref != lua_noref)
        return;
    lua_newtable(L);
    g_defs_ref = luac::ref(L);
}

static
void defs_set(lua_State* L, BlockId id, int def_idx)
{
    ensure_defs(L);
    int abs = lua_absindex(L, def_idx);
    luac::push_ref(L, g_defs_ref);
    lua_pushinteger(L, unwrap<BlockId>(id));
    lua_pushvalue(L, abs);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

static
void defs_remove(lua_State* L, BlockId id)
{
    if (g_defs_ref == lua_noref)
        return;
    luac::push_ref(L, g_defs_ref);
    lua_pushinteger(L, unwrap<BlockId>(id));
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

bool lua_npc_push_def(lua_State* L, dumb_ptr<npc_data> nd)
{
    if (nd == nullptr || g_defs_ref == lua_noref)
        return false;
    luac::push_ref(L, g_defs_ref);
    lua_pushinteger(L, unwrap<BlockId>(nd->bl_id));
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 2);
        return false;
    }
    lua_remove(L, -2);
    return true;
}

// ------------------------------------------------------------------------
// handles

// Convention shared with lua-handle-being.cpp / lua-handle-player.cpp: the
// handle table carries its block id as the raw field "id".
static
BlockId handle_own_id(lua_State* L, int idx)
{
    if (!lua_istable(L, idx))
        return BlockId();
    int abs = lua_absindex(L, idx);
    luac::push_string(L, "id"_s);
    lua_rawget(L, abs);
    int v = 0;
    if (!luac::to_int(L, -1, &v) || v < 0)
        v = 0;
    lua_pop(L, 1);
    return wrap<BlockId>(static_cast<uint32_t>(v));
}

void lua_push_npc_handle(lua_State* L, dumb_ptr<npc_data> nd)
{
    if (nd == nullptr)
    {
        lua_pushnil(L);
        return;
    }
    lua_push_engine_table(L, LuaTable::NPCS);
    lua_pushinteger(L, unwrap<BlockId>(nd->bl_id));
    lua_rawget(L, -2);
    if (lua_istable(L, -1))
    {
        lua_remove(L, -2);
        return;
    }
    lua_pop(L, 1);
    // create the handle: { id = <blockid>, __id = <blockid>, vars = {},
    // varstr = {} }; handle_extract (lua-handle-being.cpp) identifies a
    // handle by its raw "__id" field, so NPC handles must carry it too
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, unwrap<BlockId>(nd->bl_id));
    lua_setfield(L, -2, "id");
    lua_pushinteger(L, unwrap<BlockId>(nd->bl_id));
    lua_setfield(L, -2, "__id");
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::INT_DEFAULT_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "vars");
    lua_newtable(L);
    lua_push_engine_table(L, LuaTable::STR_DEFAULT_MT);
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "varstr");
    lua_getfield(L, LUA_REGISTRYINDEX, npc_mt_name);
    if (lua_istable(L, -1))
        lua_setmetatable(L, -2);
    else
        lua_pop(L, 1);
    // cache it
    lua_pushinteger(L, unwrap<BlockId>(nd->bl_id));
    lua_pushvalue(L, -2);
    lua_rawset(L, -4);      // NPCS[id] = handle
    lua_remove(L, -2);      // drop the NPCS table
}

// ------------------------------------------------------------------------
// registration / detach

static
bool is_clock_label(XString k)
{
    struct Pat
    {
        LString prefix;
        size_t digits;
    };
    static const Pat pats[4] =
    {
        {"OnMinute"_s, 2},
        {"OnClock"_s, 4},
        {"OnHour"_s, 2},
        {"OnDay"_s, 4},
    };
    for (const Pat& p : pats)
    {
        if (k.size() != p.prefix.size() + p.digits || !k.startswith(p.prefix))
            continue;
        XString rest = k.xslice_t(p.prefix.size());
        bool all = true;
        for (char c : rest)
            if (c < '0' || c > '9')
            {
                all = false;
                break;
            }
        if (all)
            return true;
    }
    return false;
}

void lua_npc_register(dumb_ptr<npc_data> nd)
{
    lua_State* L = lua_state();
    if (L == nullptr || nd == nullptr)
        return;
    lua_push_npc_handle(L, nd);     // creates and caches the handle
    lua_push_engine_table(L, LuaTable::NPCS_BYNAME);
    luac::push_string(L, XString(nd->name));
    lua_pushvalue(L, -3);
    lua_rawset(L, -3);
    lua_pop(L, 2);                  // byname table and the handle

    dumb_ptr<npc_data_script> nds = nd->is_script();
    if (nds == nullptr || nds->scr.parent)
        return;     // only non-puppet script NPCs join broadcasts/on_init
    lua_events_npc_add(nd->bl_id);
    if (!lua_npc_push_def(L, nd))
        return;
    luac::push_string(L, "events"_s);
    lua_rawget(L, -2);
    if (lua_istable(L, -1))
    {
        lua_pushnil(L);
        while (lua_next(L, -2))
        {
            lua_pop(L, 1);      // the handler
            if (lua_type(L, -1) == LUA_TSTRING)
            {
                size_t klen;
                ZString k = luac::to_string(L, -1, &klen);
                XString kx = k;
                if (kx == "OnPCLoginEvent"_s)
                    lua_hook_index_add(LuaHook::LOGIN, nd->bl_id);
                else if (kx == "OnPCLogoutEvent"_s)
                    lua_hook_index_add(LuaHook::LOGOUT, nd->bl_id);
                else if (kx == "OnPCDieEvent"_s)
                    lua_hook_index_add(LuaHook::DIE, nd->bl_id);
                else if (kx == "OnPCKillEvent"_s)
                    lua_hook_index_add(LuaHook::KILL, nd->bl_id);
                else if (kx == "OnMobKillEvent"_s)
                    lua_hook_index_add(LuaHook::MOBKILL, nd->bl_id);
                else if (is_clock_label(kx))
                    lua_hook_index_add(LuaHook::CLOCK, nd->bl_id);
            }
        }
    }
    lua_pop(L, 2);      // events (or non-table) and def
}

void lua_npc_detach(dumb_ptr<npc_data> nd)
{
    if (nd == nullptr)
        return;
    lua_npc_timer_detach(nd);
    lua_hook_index_remove(nd->bl_id);
    lua_events_npc_remove(nd->bl_id);
    lua_State* L = lua_state();
    if (L == nullptr)
        return;
    defs_remove(L, nd->bl_id);
    lua_push_engine_table(L, LuaTable::NPCS);
    lua_pushinteger(L, unwrap<BlockId>(nd->bl_id));
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    // drop the by-name entry only if it still points at this NPC
    lua_push_engine_table(L, LuaTable::NPCS_BYNAME);
    luac::push_string(L, XString(nd->name));
    lua_rawget(L, -2);
    bool mine = (handle_own_id(L, -1) == nd->bl_id);
    lua_pop(L, 1);
    if (mine)
    {
        luac::push_string(L, XString(nd->name));
        lua_pushnil(L);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

// ------------------------------------------------------------------------
// the NPC handle metatable

// number of OnTimer labels, from def.__timers (computed by npc.script)
static
int npc_timer_label_count(lua_State* L, dumb_ptr<npc_data> nd)
{
    if (!lua_npc_push_def(L, nd))
        return 0;
    luac::push_string(L, "__timers"_s);
    lua_rawget(L, -2);
    int n = 0;
    if (lua_istable(L, -1))
        n = static_cast<int>(luac::raw_len(L, -1));
    lua_pop(L, 2);
    return n;
}

// __index: raw fields (id, vars, varstr) never get here; dispatch order is
// methods, then properties, then params (doc/lua-api.md sections 6 and 7).
static
int ln_npc_index(lua_State* L)
{
    lua_pushvalue(L, 2);
    lua_gettable(L, lua_upvalueindex(1));   // methods (may chain to being)
    if (!lua_isnil(L, -1))
        return 1;
    lua_pop(L, 1);
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "npc handle: invalid key");
    size_t klen;
    ZString key = luac::to_string(L, 2, &klen);
    BlockId id = handle_own_id(L, 1);
    dumb_ptr<npc_data> nd = map_id_is_npc(id);
    bool known =
        key == "name"_s || key == "basename"_s || key == "suffix"_s
        || key == "map"_s || key == "x"_s || key == "y"_s
        || key == "dir"_s || key == "sprite"_s || key == "sex"_s
        || key == "enabled"_s || key == "parent"_s || key == "type"_s;
    if (nd == nullptr)
    {
        int sp;
        if (known || lua_param_lookup(key, &sp))
        {
            // dead handle: property reads give nil after a warning
            lua_warn(STRPRINTF("stale NPC handle (id %d): reading '%s'"_fmt,
                        unwrap<BlockId>(id), key));
            lua_pushnil(L);
            return 1;
        }
        return luaL_error(L, "npc handle has no field '%s' "
                "(use self.vars for script data)", key.c_str());
    }
    if (key == "name"_s)
    {
        push_string(L, XString(nd->name));
        return 1;
    }
    if (key == "basename"_s || key == "suffix"_s)
    {
        NpcName nm = nd->name;
        XString whole = nm;
        auto it = std::find(whole.begin(), whole.end(), '#');
        if (key == "basename"_s)
            push_string(L, whole.xislice_h(it));
        else
            push_string(L, whole.xislice_t(it));
        return 1;
    }
    if (key == "map"_s)
    {
        push_string(L, nd->bl_m->name_);   // MapName converts via operator XString
        return 1;
    }
    if (key == "x"_s)
        return luac::push_int(L, nd->bl_x);
    if (key == "y"_s)
        return luac::push_int(L, nd->bl_y);
    if (key == "dir"_s)
        return luac::push_int(L, static_cast<int>(nd->dir));
    if (key == "sprite"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<Species>(nd->npc_class)));
    if (key == "sex"_s)
        return luac::push_int(L, static_cast<int>(nd->sex));
    if (key == "enabled"_s)
    {
        lua_pushboolean(L, !(nd->flag & 1));
        return 1;
    }
    if (key == "parent"_s)
    {
        dumb_ptr<npc_data_script> nds = nd->is_script();
        dumb_ptr<npc_data> parent;
        if (nds != nullptr && nds->scr.parent)
            parent = map_id_is_npc(nds->scr.parent);
        lua_push_npc_handle(L, parent);
        return 1;
    }
    if (key == "type"_s)
    {
        lua_pushliteral(L, "npc");
        return 1;
    }
    int sp;
    if (lua_param_lookup(key, &sp))
    {
        dumb_ptr<block_list> bl = nd;
        return luac::push_int(L, pc_readparam(bl, static_cast<SP>(sp)));
    }
    return luaL_error(L, "npc handle has no field '%s' "
            "(use self.vars for script data)", key.c_str());
}

static
int ln_npc_newindex(lua_State* L)
{
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "npc handle: invalid key");
    size_t klen;
    ZString key = luac::to_string(L, 2, &klen);
    if (key == "dir"_s || key == "sprite"_s || key == "sex"_s)
    {
        int v = check_int(L, 3);
        if (key == "dir"_s && (v < 0 || v > 7))
            return luaL_argerror(L, 3, "direction 0..7 expected");
        dumb_ptr<npc_data> nd = map_id_is_npc(handle_own_id(L, 1));
        if (nd == nullptr)
        {
            lua_warn(STRPRINTF("stale NPC handle: write to '%s' ignored"_fmt,
                        key));
            return 0;
        }
        if (key == "dir"_s)
        {
            nd->dir = static_cast<DIR>(v);
            clif_setnpcdirection(nd, nd->dir);
        }
        else if (key == "sprite"_s)
        {
            // the fakenpcname X, X, sprite idiom: disable + re-enable with
            // the new class
            npc_enable(nd->name, 0);
            nd->npc_class = wrap<Species>(static_cast<uint16_t>(v));
            npc_enable(nd->name, 1);
        }
        else
            nd->sex = static_cast<SEX>(static_cast<uint8_t>(v));
        return 0;
    }
    int sp;
    if (lua_param_lookup(key, &sp))
    {
        int v = check_int(L, 3);
        dumb_ptr<npc_data> nd = map_id_is_npc(handle_own_id(L, 1));
        if (nd == nullptr)
        {
            lua_warn(STRPRINTF("stale NPC handle: write to '%s' ignored"_fmt,
                        key));
            return 0;
        }
        dumb_ptr<block_list> bl = nd;
        pc_setparam(bl, static_cast<SP>(sp), v);
        return 0;
    }
    bool readonly =
        key == "id"_s || key == "vars"_s || key == "varstr"_s
        || key == "name"_s || key == "basename"_s || key == "suffix"_s
        || key == "map"_s || key == "x"_s || key == "y"_s
        || key == "enabled"_s || key == "parent"_s || key == "type"_s;
    if (readonly)
        return luaL_error(L, "npc handle: field '%s' is read-only",
                key.c_str());
    return luaL_error(L, "npc handle has no field '%s' "
            "(use self.vars for script data)", key.c_str());
}

// ------------------------------------------------------------------------
// NPC methods

static
int ln_self_enable(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    if (nd == nullptr)
        return 0;
    npc_enable(nd->name, 1);
    return 0;
}

static
int ln_self_disable(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    if (nd == nullptr)
        return 0;
    npc_enable(nd->name, 0);
    return 0;
}

// same-map move: disable + move + enable (script-fun.cpp:4818-4845)
static
int ln_self_warp(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int x = check_int(L, 2);
    int y = check_int(L, 3);
    if (nd == nullptr)
        return 0;
    P<map_local> m = nd->bl_m;
    if (!nd->bl_prev
            || x < 0 || x > m->xs - 1
            || y < 0 || y > m->ys - 1)
        return 0;
    npc_enable(nd->name, 0);
    map_delblock(nd);
    nd->bl_x = x;
    nd->bl_y = y;
    map_addblock(nd);
    npc_enable(nd->name, 1);
    return 0;
}

// random reposition in a rectangle (script-fun.cpp:4852-4909)
static
int ln_self_areawarp(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    bool avoid_collision = lua_toboolean(L, 6);
    if (nd == nullptr)
        return 0;
    if (x1 < x0 || y1 < y0)
        return 0;
    int max = (y1 - y0 + 1) * (x1 - x0 + 1) * 3;
    if (max > 1000)
        max = 1000;
    P<map_local> m = nd->bl_m;
    int x, y;
    if (avoid_collision)
    {
        int j = 0;
        do
        {
            x = random_::in(x0, x1);
            y = random_::in(y0, y1);
        }
        while (bool(map_getcell(m, x, y) & MapCell::UNWALKABLE)
                && (++j) < max);
        if (j >= max)
            return 0;   // no walkable cell found (old behaviour)
    }
    else
    {
        x = random_::in(x0, x1);
        y = random_::in(y0, y1);
    }
    npc_enable(nd->name, 0);
    map_delblock(nd);
    nd->bl_x = x;
    nd->bl_y = y;
    map_addblock(nd);
    npc_enable(nd->name, 1);
    return 0;
}

// setdirection(dir, sit, save [, p]) (script-fun.cpp:3498-3535)
static
int ln_self_setdirection(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int dirv = check_int(L, 2);
    bool sit = check_bool(L, 3);
    bool save = check_bool(L, 4);
    dumb_ptr<map_session_data> sd = nullptr;
    if (!lua_isnoneornil(L, 5))
        sd = check_player(L, 5);
    if (dirv < 0 || dirv > 7)
        return luaL_argerror(L, 2, "direction 0..7 expected");
    if (nd == nullptr)
        return 0;
    DIR dir = static_cast<DIR>(dirv);
    DamageType action = sit ? DamageType::SIT : DamageType::STAND;
    if (save)
    {
        nd->dir = dir;
        nd->sit = action;
    }
    if (sd != nullptr)
    {
        clif_sitnpc_towards(sd, nd, action);
        clif_setnpcdirection_towards(sd, nd, dir);
    }
    else
    {
        clif_sitnpc(nd, action);
        clif_setnpcdirection(nd, dir);
    }
    return 0;
}

static
int ln_self_talk(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    ZString text = check_string(L, 2);
    dumb_ptr<map_session_data> sd = nullptr;
    if (!lua_isnoneornil(L, 3))
        sd = check_player(L, 3);
    if (nd == nullptr)
        return 0;
    if (sd != nullptr)
        clif_message_towards(sd, nd, XString(text));
    else
        clif_message(nd, XString(text));
    return 0;
}

static
int ln_self_emotion(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int type = check_int(L, 2);
    dumb_ptr<map_session_data> sd = nullptr;
    if (!lua_isnoneornil(L, 3))
        sd = check_player(L, 3);
    if (nd == nullptr || type < 0 || type > 200)
        return 0;
    if (sd != nullptr)
        clif_emotion_towards(nd, sd, type);
    else
        clif_emotion(nd, type);
    return 0;
}

static
int ln_self_misceffect(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int fx = check_int(L, 2);
    if (nd == nullptr)
        return 0;
    clif_misceffect(nd, fx);
    return 0;
}

static
int ln_self_specialeffect(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int fx = check_int(L, 2);
    if (nd == nullptr)
        return 0;
    clif_specialeffect(nd, fx, 0);
    return 0;
}

static
int ln_self_announce(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    ZString text = check_string(L, 2);
    int flag = check_int(L, 3);
    if (nd == nullptr)
        return 0;
    if (flag & 0x0f)
        clif_GMmessage(nd, XString(text), flag);
    else
        intif_GMmessage(XString(text));
    return 0;
}

// fixed rename semantics (doc/lua-api.md section 7): really updates the
// name registries, unlike the old fakenpcname
static
int ln_self_rename(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    ZString nn = check_string(L, 2);
    if (nn.size() == 0 || nn.size() > 23)
        return luaL_argerror(L, 2, "NPC name of 1..23 bytes expected");
    bool has_sprite = !lua_isnoneornil(L, 3);
    int sprite = has_sprite ? check_int(L, 3) : 0;
    if (nd == nullptr)
        return 0;
    NpcName newname = stringish<NpcName>(nn);
    dumb_ptr<npc_data> other = npc_name2id(newname);
    if (other != nullptr && other != nd)
    {
        lua_warn(STRPRINTF("rename: NPC name '%s' is already taken"_fmt,
                    newname));
        return 0;
    }
    NpcName oldname = nd->name;
    npc_enable(oldname, 0);
    npcs_by_name.put(oldname, dumb_ptr<npc_data>());
    // engine by-name registry
    lua_push_engine_table(L, LuaTable::NPCS_BYNAME);
    luac::push_string(L, XString(oldname));
    lua_pushnil(L);
    lua_rawset(L, -3);
    nd->name = newname;
    if (has_sprite)
        nd->npc_class = wrap<Species>(static_cast<uint16_t>(sprite));
    npcs_by_name.put(newname, nd);
    luac::push_string(L, XString(newname));
    lua_push_npc_handle(L, nd);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    npc_enable(newname, 1);
    return 0;
}

// ---- OnTimer machine passthrough (state machine bodies stay in npc.cpp /
// lua-timers.cpp under their old names, doc/lua-engine.md section 6)

static
dumb_ptr<npc_data_script> timer_self(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    dumb_ptr<npc_data_script> nds = nd != nullptr ? nd->is_script() : nullptr;
    if (nd != nullptr && nds == nullptr)
        lua_warn("npc timer method on a non-script NPC"_s);
    return nds;
}

static
int ln_self_initnpctimer(lua_State* L)
{
    dumb_ptr<npc_data_script> nds = timer_self(L);
    if (nds == nullptr)
        return 0;
    // identical to the old builtin, including the no-op quirk while the
    // timer is counting toward its first label
    npc_settimerevent_tick(nds, interval_t::zero());
    npc_timerevent_start(nds);
    return 0;
}

static
int ln_self_startnpctimer(lua_State* L)
{
    dumb_ptr<npc_data_script> nds = timer_self(L);
    if (nds == nullptr)
        return 0;
    npc_timerevent_start(nds);
    return 0;
}

static
int ln_self_stopnpctimer(lua_State* L)
{
    dumb_ptr<npc_data_script> nds = timer_self(L);
    if (nds == nullptr)
        return 0;
    npc_timerevent_stop(nds);
    return 0;
}

static
int ln_self_getnpctimer(lua_State* L)
{
    dumb_ptr<npc_data_script> nds = timer_self(L);
    int type = check_int(L, 2);
    if (nds == nullptr)
        return luac::push_int(L, 0);
    switch (type)
    {
    case 0:
        return luac::push_int(L,
                static_cast<int>(npc_gettimerevent_tick(nds).count()));
    case 1:
        return luac::push_int(L, nds->scr.timer_active ? 1 : 0);
    case 2:
        return luac::push_int(L, npc_timer_label_count(L, nds));
    }
    return luaL_argerror(L, 2, "timer query type 0..2 expected");
}

static
int ln_self_setnpctimer(lua_State* L)
{
    dumb_ptr<npc_data_script> nds = timer_self(L);
    int ms = check_int(L, 2);
    if (nds == nullptr)
        return 0;
    npc_settimerevent_tick(nds, interval_t(ms));
    return 0;
}

static
int ln_self_addnpctimer(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    int ms = check_int(L, 2);
    LuaCallback cb = lua_cb_from_stack(L, 3);   // may raise, cb is trivial
    if (nd == nullptr)
    {
        lua_cb_release(cb);
        return 0;
    }
    // the slot lives on self (doc/lua-api.md deviation 10)
    cb.self_npc = nd->bl_id;
    if (!lua_npc_addeventtimer(nd, interval_t(ms), cb))
        lua_warn("addnpctimer: event timer slots are full"_s);
    return 0;
}

// Collect the sorted on_timer interval list from a definition table's
// __timers field (seeded by npc.script; shared by puppets).
static
std::vector<interval_t> def_timer_intervals(lua_State* L, int def_idx)
{
    std::vector<interval_t> intervals;
    lua_getfield(L, def_idx, "__timers");
    if (lua_istable(L, -1))
    {
        int n = static_cast<int>(luac::raw_len(L, -1));
        for (int i = 1; i <= n; ++i)
        {
            lua_rawgeti(L, -1, i);
            int ms;
            if (luac::to_int(L, -1, &ms) && ms > 0)
                intervals.push_back(interval_t(ms));
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return intervals;
}

// puppet: a temp NPC sharing all of the parent's handlers
// (script-fun.cpp:1358-1470 minus the label machinery)
static
int ln_self_puppet(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    MapName mapname = check_mapname(L, 2);
    int x = check_int(L, 3);
    int y = check_int(L, 4);
    ZString nm = check_string(L, 5);
    if (nm.size() == 0 || nm.size() > 23)
        return luaL_argerror(L, 5, "NPC name of 1..23 bytes expected");
    int sprite = check_int(L, 6);
    bool has_area = !lua_isnoneornil(L, 7) && !lua_isnoneornil(L, 8);
    int xs = has_area ? check_int(L, 7) : 0;
    int ys = has_area ? check_int(L, 8) : 0;
    dumb_ptr<npc_data_script> parent_nd =
        nd != nullptr ? nd->is_script() : nullptr;
    if (parent_nd == nullptr)
    {
        lua_warn("puppet: self is not a script NPC"_s);
        lua_pushnil(L);
        return 1;
    }
    NpcName pname = stringish<NpcName>(nm);
    if (npc_name2id(pname) != nullptr)
    {
        // name taken (old returned 0)
        lua_pushnil(L);
        return 1;
    }
    Option<P<map_local>> m_ = map_mapname2mapid(mapname);
    P<map_local> m = TRY_UNWRAP(m_,
            {
                lua_warn(STRPRINTF("puppet: unknown map '%s'"_fmt, mapname));
                lua_pushnil(L);
                return 1;
            });

    dumb_ptr<npc_data_script> pd;
    pd.new_();
    pd->bl_prev = pd->bl_next = nullptr;
    pd->scr.event_needs_map = false;
    pd->name = pname;
    pd->sex = SEX::UNSPECIFIED;
    pd->bl_m = m;
    pd->bl_x = x;
    pd->bl_y = y;
    if (has_area)
    {
        pd->scr.xs = xs * 2 + 1;
        pd->scr.ys = ys * 2 + 1;
    }
    pd->bl_id = npc_get_new_npc_id();
    pd->scr.parent = parent_nd->bl_id;
    pd->dir = DIR::S;
    pd->flag = 0;
    pd->sit = DamageType::STAND;
    pd->npc_class = wrap<Species>(static_cast<uint16_t>(sprite));
    pd->speed = 200_ms;
    pd->option = Opt0::ZERO;
    pd->opt1 = Opt1::ZERO;
    pd->opt2 = Opt2::ZERO;
    pd->opt3 = Opt3::ZERO;
    pd->bl_type = BL::NPC;
    pd->npc_subtype = NpcSubtype::SCRIPT;
    npc_script++;
    pd->deletion_pending = npc_data::NOT_DELETING;
    pd->n = map_addnpc(pd->bl_m, pd);
    map_addblock(pd);
    clif_spawnnpc(pd);
    register_npc_name(pd);

    // share the parent's definition table under the puppet's id (its
    // "extra ref on the parent's", doc/lua-engine.md 2.2)
    if (lua_npc_push_def(L, parent_nd))
    {
        defs_set(L, pd->bl_id, -1);
        // the puppet gets its own OnTimer counter over the parent's
        // interval list (the old builtin_puppet copied timer_eventv)
        lua_npc_timer_setup(pd, def_timer_intervals(L, lua_gettop(L)));
        lua_pop(L, 1);
    }
    lua_npc_register(pd);
    lua_push_npc_handle(L, pd);
    return 1;
}

// deferred free (script-fun.cpp:1313-1352); does NOT terminate the handler
static
int ln_self_destroy(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    if (nd == nullptr || nd->npc_subtype != NpcSubtype::SCRIPT)
        return 0;
    // if a player is attached to this dialog, detach them (the old builtin
    // dequeued the script's rid; here: the context player)
    {
        LuaCtx ctx = lua_current_ctx();
        dumb_ptr<map_session_data> sd = map_id_is_player(ctx.player);
        if (sd != nullptr && sd->npc_id)
            npc_event_dequeue(sd);
    }
    for (int i = 0; i < MAX_EVENTTIMER; i++)
        nd->eventtimer[i].cancel();
    nd->deletion_pending = npc_data::DELETION_QUEUED;
    // deferred so we do not free under an iterating dispatcher
    nd->eventtimer[0] = Timer(gettick(), std::bind(npc_free, nd));
    return 0;
}

static
int ln_self_exists(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    dumb_ptr<npc_data> nd = map_id_is_npc(handle_own_id(L, 1));
    lua_pushboolean(L,
            nd != nullptr && nd->deletion_pending == npc_data::NOT_DELETING);
    return 1;
}

// ------------------------------------------------------------------------
// npc.event / npc.event_all / self:event

// resolve_c of the dispatch wrapper: pushes (handler, self) when the inline
// path applies, i.e. npc.event targets the same player whose dialog
// coroutine is running right now (doc/lua-engine.md section 5). Every gate
// the normal path applies must pass; on any miss we return nothing and the
// C path handles it identically.
static
int ln_event_resolve(lua_State* L)
{
    NpcEvent ev = check_event(L, 1);
    if (lua_isnoneornil(L, 2))
        return 0;
    dumb_ptr<map_session_data> sd = check_player(L, 2);
    if (sd == nullptr)
        return 0;
    if (!sd->lua.running || sd->lua.thread_ref == lua_noref)
        return 0;
    if (lua_current_ctx().player != sd->bl_id)
        return 0;
    NpcName evnpc = ev.npc;
    if (!evnpc || evnpc.front() == '~')
        return 0;
    dumb_ptr<npc_data> nd = npc_name2id(evnpc);
    if (nd == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
        return 0;
    dumb_ptr<npc_data_script> nds = nd->is_script();
    if (nds != nullptr && nds->scr.parent
            && map_id2bl(nds->scr.parent) == nullptr)
        return 0;   // the C path frees the orphan
    if (nds != nullptr && nds->scr.event_needs_map)
    {
        int xs = nds->scr.xs;
        int ys = nds->scr.ys;
        if (nd->bl_m != sd->bl_m)
            return 0;
        if (xs > 0
                && (sd->bl_x < nd->bl_x - xs / 2
                    || nd->bl_x + xs / 2 < sd->bl_x))
            return 0;
        if (ys > 0
                && (sd->bl_y < nd->bl_y - ys / 2
                    || nd->bl_y + ys / 2 < sd->bl_y))
            return 0;
    }
    if (nd->flag & 1)
        return 0;   // disabled: the C path drops it (with dequeue)
    ScriptLabel label = ev.label;
    if (!lua_npc_push_handler(L, nd, XString(label)))
        return 0;
    lua_push_npc_handle(L, nd);
    return 2;
}

// event_c of the dispatch wrapper (also the fallback npc.event)
static
int ln_event_c(lua_State* L)
{
    NpcEvent ev = check_event(L, 1);
    dumb_ptr<map_session_data> sd = nullptr;
    if (!lua_isnoneornil(L, 2))
    {
        sd = check_player(L, 2);
        if (sd == nullptr)
        {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    // events a player fires from their own running dialog never queue
    bool force_inline = sd != nullptr && sd->lua.running
        && lua_current_ctx().player == sd->bl_id;
    int args_idx = lua_isnoneornil(L, 3) ? 0 : 3;
    bool ok;
    {
        ok = lua_npc_event_from_stack(L, sd, ev, args_idx, force_inline);
    }
    lua_pushboolean(L, ok);
    return 1;
}

static
int ln_event_all(lua_State* L)
{
    ZString label = check_string(L, 1);
    if (label.size() > 23)
        return luaL_argerror(L, 1, "label too long (max 23)");
    dumb_ptr<map_session_data> sd = nullptr;
    if (!lua_isnoneornil(L, 2))
    {
        sd = check_player(L, 2);
        if (sd == nullptr)
            return 0;
    }
    NpcEvent ev;
    ev.label = stringish<ScriptLabel>(label);
    int args_idx = lua_isnoneornil(L, 3) ? 0 : 3;
    {
        lua_npc_event_from_stack(L, sd, ev, args_idx, false);
    }
    return 0;
}

// C fallback for self:event when the Lua wrapper failed to build (the
// normal path is the pure-Lua closure so inline dispatch can yield)
static
int ln_self_event_c(lua_State* L)
{
    dumb_ptr<npc_data> nd = check_npc(L, 1);
    ZString label = check_string(L, 2);
    if (label.size() > 23)
        return luaL_argerror(L, 2, "label too long (max 23)");
    dumb_ptr<map_session_data> sd = nullptr;
    if (!lua_isnoneornil(L, 3))
        sd = check_player(L, 3);
    if (nd == nullptr)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    NpcEvent ev;
    ev.npc = nd->name;
    ev.label = stringish<ScriptLabel>(label);
    bool force_inline = sd != nullptr && sd->lua.running
        && lua_current_ctx().player == sd->bl_id;
    int args_idx = lua_isnoneornil(L, 4) ? 0 : 4;
    bool ok;
    {
        ok = lua_npc_event_from_stack(L, sd, ev, args_idx, force_inline);
    }
    lua_pushboolean(L, ok);
    return 1;
}

// ------------------------------------------------------------------------
// npc namespace lookups

static
int ln_ns_get(lua_State* L)
{
    ZString nm = check_string(L, 1);
    dumb_ptr<npc_data> nd = nullptr;
    if (nm.size() >= 1 && nm.size() <= 23)
        nd = npc_name2id(stringish<NpcName>(nm));
    if (nd != nullptr && nd == lua_item_dialog_npc())
        nd = nullptr;   // the engine-owned #itemdialog NPC stays hidden
    lua_push_npc_handle(L, nd);
    return 1;
}

static
int ln_ns_byid(lua_State* L)
{
    int id = check_int(L, 1);
    dumb_ptr<npc_data> nd = nullptr;
    if (id > 0)
        nd = map_id_is_npc(wrap<BlockId>(static_cast<uint32_t>(id)));
    if (nd != nullptr && nd == lua_item_dialog_npc())
        nd = nullptr;
    lua_push_npc_handle(L, nd);
    return 1;
}

static
int ln_ns_exists(lua_State* L)
{
    ZString nm = check_string(L, 1);
    dumb_ptr<npc_data> nd = nullptr;
    if (nm.size() >= 1 && nm.size() <= 23)
        nd = npc_name2id(stringish<NpcName>(nm));
    if (nd != nullptr && nd == lua_item_dialog_npc())
        nd = nullptr;
    lua_pushboolean(L,
            nd != nullptr && nd->deletion_pending == npc_data::NOT_DELETING);
    return 1;
}

static
int ln_ns_enable_common(lua_State* L, bool flag)
{
    ZString nm = check_string(L, 1);
    dumb_ptr<npc_data> nd = nullptr;
    if (nm.size() >= 1 && nm.size() <= 23)
        nd = npc_name2id(stringish<NpcName>(nm));
    if (nd == nullptr)
    {
        lua_warn(STRPRINTF("npc.%s: no such npc '%s'"_fmt,
                    flag ? "enable"_s : "disable"_s, nm));
        lua_pushboolean(L, 0);
        return 1;
    }
    npc_enable(nd->name, flag ? 1 : 0);
    lua_pushboolean(L, 1);
    return 1;
}

static
int ln_ns_enable(lua_State* L)
{
    return ln_ns_enable_common(L, true);
}

static
int ln_ns_disable(lua_State* L)
{
    return ln_ns_enable_common(L, false);
}

// ------------------------------------------------------------------------
// constructor helpers

static
void check_table_keys(lua_State* L, const LString* allowed, size_t n,
        const char* what)
{
    // the constructor table is at index 1
    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        lua_pop(L, 1);      // the value
        bool ok = false;
        if (lua_type(L, -1) == LUA_TSTRING)
        {
            size_t klen;
            ZString k = luac::to_string(L, -1, &klen);
            for (size_t i = 0; i < n; ++i)
                if (k == allowed[i])
                {
                    ok = true;
                    break;
                }
        }
        if (!ok)
        {
            const char* k = lua_tostring(L, -1);
            luaL_error(L, "%s: unknown key '%s'", what, k ? k : "?");
        }
    }
}

// pushes t[name] (raw); false and nothing pushed when nil/absent
static
bool push_raw_field(lua_State* L, int idx, const char* name)
{
    int abs = lua_absindex(L, idx);
    lua_pushstring(L, name);
    lua_rawget(L, abs);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

static
int opt_int_field(lua_State* L, const char* what, const char* name, int def,
        bool* present)
{
    if (present)
        *present = false;
    if (!push_raw_field(L, 1, name))
        return def;
    int v;
    if (!luac::to_int(L, -1, &v))
        return luaL_error(L, "%s: '%s' must be an integer", what, name);
    lua_pop(L, 1);
    if (present)
        *present = true;
    return v;
}

static
int req_int_field(lua_State* L, const char* what, const char* name)
{
    bool present;
    int v = opt_int_field(L, what, name, 0, &present);
    if (!present)
        return luaL_error(L, "%s: missing '%s'", what, name);
    return v;
}

static
MapName opt_mapname_field(lua_State* L, const char* what, const char* name,
        bool* present)
{
    MapName m;
    if (present)
        *present = false;
    if (!push_raw_field(L, 1, name))
        return m;
    size_t len;
    ZString z = luac::to_string(L, -1, &len);
    if (lua_type(L, -1) != LUA_TSTRING || len == 0 || len > 15)
        luaL_error(L, "%s: '%s' must be a map name of 1..15 bytes",
                what, name);
    m = stringish<MapName>(z);
    lua_pop(L, 1);
    if (present)
        *present = true;
    return m;
}

static
MapName req_mapname_field(lua_State* L, const char* what, const char* name)
{
    bool present;
    MapName m = opt_mapname_field(L, what, name, &present);
    if (!present)
        luaL_error(L, "%s: missing '%s'", what, name);
    return m;
}

// a VString<23> name field ("name" of npc.script/shop, "name" of monster)
static
VString<23> req_name23_field(lua_State* L, const char* what,
        const char* name)
{
    VString<23> out;
    if (!push_raw_field(L, 1, name))
        luaL_error(L, "%s: missing '%s'", what, name);
    size_t len;
    ZString z = luac::to_string(L, -1, &len);
    if (lua_type(L, -1) != LUA_TSTRING || len == 0 || len > 23)
        luaL_error(L, "%s: '%s' must be a string of 1..23 bytes",
                what, name);
    out = stringish<VString<23>>(z);
    lua_pop(L, 1);
    return out;
}

// ------------------------------------------------------------------------
// npc.script{}

static
void set_sugar_handler(lua_State* L, int ev_idx, const char* field,
        const char* label)
{
    if (!push_raw_field(L, 1, field))
        return;
    if (!lua_isfunction(L, -1))
    {
        luaL_error(L, "npc.script: '%s' must be a function", field);
        return;
    }
    lua_pushstring(L, label);
    lua_rawget(L, ev_idx);
    if (!lua_isnil(L, -1))
    {
        luaL_error(L, "npc.script: handler '%s' given in both forms", field);
        return;
    }
    lua_pop(L, 1);
    lua_pushstring(L, label);
    lua_insert(L, -2);      // [label, fn]
    lua_rawset(L, ev_idx);
}

static
const LString script_keys[] =
{
    "name"_s, "map"_s, "x"_s, "y"_s, "dir"_s, "sprite"_s, "xs"_s, "ys"_s,
    "on_click"_s, "on_touch"_s, "on_init"_s, "on_timer"_s, "events"_s,
};

static
int ln_ns_script(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    check_table_keys(L, script_keys,
            sizeof(script_keys) / sizeof(script_keys[0]), "npc.script");
    NpcName name = stringish<NpcName>(
            req_name23_field(L, "npc.script", "name"));
    // duplicate NPC names are a fatal load error (doc/lua-api.md section 8)
    if (npc_name2id(name) != nullptr)
        return luaL_error(L, "npc.script: duplicate NPC name '%s'",
                name.c_str());
    bool placed = false;
    MapName mapname = opt_mapname_field(L, "npc.script", "map", &placed);
    bool has_x, has_y, has_dir, has_sprite, has_xs, has_ys;
    int x = opt_int_field(L, "npc.script", "x", 0, &has_x);
    int y = opt_int_field(L, "npc.script", "y", 0, &has_y);
    int dirv = opt_int_field(L, "npc.script", "dir", 0, &has_dir);
    int sprite = opt_int_field(L, "npc.script", "sprite", 32767,
            &has_sprite);
    int xs_file = opt_int_field(L, "npc.script", "xs", 0, &has_xs);
    int ys_file = opt_int_field(L, "npc.script", "ys", 0, &has_ys);
    if (placed)
    {
        if (!has_x || !has_y)
            return luaL_error(L, "npc.script: a map NPC needs 'x' and 'y'");
    }
    else if (has_x || has_y || has_dir || has_xs || has_ys)
        return luaL_error(L,
                "npc.script: 'x'/'y'/'dir'/'xs'/'ys' require 'map'");
    if (dirv < 0 || dirv > 7)
        return luaL_error(L, "npc.script: 'dir' must be 0..7");
    if (sprite < -1 || sprite > 65535)
        return luaL_error(L, "npc.script: 'sprite' must be -1 or 0..65535");
    if (sprite == -1)
        sprite = 0;     // NEGATIVE_SPECIES, as the old file parser mapped it
    if (xs_file < 0 || ys_file < 0)
        return luaL_error(L, "npc.script: 'xs'/'ys' must be >= 0");
    // the touch radius is stored as a diameter, exactly like the old file
    // parser (ast/npc.cpp:363-365)
    int xs = has_xs ? xs_file * 2 + 1 : 0;
    int ys = has_ys ? ys_file * 2 + 1 : 0;

    // ---- build the definition table: def.events[label] = handler with
    // "" = click body; the sugar fields are normalised into events
    lua_newtable(L);
    int def = lua_gettop(L);
    lua_newtable(L);
    int ev = lua_gettop(L);
    if (push_raw_field(L, 1, "events"))
    {
        if (!lua_istable(L, -1))
            return luaL_error(L, "npc.script: 'events' must be a table");
        int uev = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, uev))
        {
            if (lua_type(L, -2) != LUA_TSTRING)
                return luaL_error(L,
                        "npc.script: events keys must be strings");
            size_t klen;
            luac::to_string(L, -2, &klen);
            if (klen > 23)
                return luaL_error(L,
                        "npc.script: event label too long (max 23)");
            if (!lua_isfunction(L, -1))
                return luaL_error(L,
                        "npc.script: events values must be functions");
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_rawset(L, ev);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);      // the user events table
    }
    set_sugar_handler(L, ev, "on_click", "");
    set_sugar_handler(L, ev, "on_touch", "OnTouch");
    set_sugar_handler(L, ev, "on_init", "OnInit");
    if (push_raw_field(L, 1, "on_timer"))
    {
        if (!lua_istable(L, -1))
            return luaL_error(L, "npc.script: 'on_timer' must be a table");
        int ot = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, ot))
        {
            int ms;
            if (!luac::to_int(L, -2, &ms) || ms <= 0)
                return luaL_error(L,
                        "npc.script: on_timer keys must be integers > 0");
            if (!lua_isfunction(L, -1))
                return luaL_error(L,
                        "npc.script: on_timer values must be functions");
            lua_pushfstring(L, "OnTimer%d", ms);
            lua_pushvalue(L, -1);
            lua_rawget(L, ev);
            if (!lua_isnil(L, -1))
                return luaL_error(L,
                        "npc.script: label 'OnTimer%d' given in both forms",
                        ms);
            lua_pop(L, 1);
            lua_pushvalue(L, -2);   // the function
            lua_rawset(L, ev);      // events["OnTimer<ms>"] = fn
            lua_pop(L, 1);          // the function; the key drives lua_next
        }
        lua_pop(L, 1);      // the on_timer table
    }
    // sorted OnTimer intervals -> def.__timers (doc/lua-engine.md 2.2
    // timer_intervals; engine-side until npc_data_script grows the field)
    {
        std::vector<int> timers;
        lua_pushnil(L);
        while (lua_next(L, ev))
        {
            lua_pop(L, 1);
            if (lua_type(L, -1) == LUA_TSTRING)
            {
                size_t klen;
                ZString k = luac::to_string(L, -1, &klen);
                XString kx = k;
                int t = 0;
                if (kx.startswith("OnTimer"_s)
                        && extract(kx.xslice_t(7), &t) && t > 0)
                    timers.push_back(t);
            }
        }
        std::sort(timers.begin(), timers.end());
        lua_createtable(L, static_cast<int>(timers.size()), 0);
        for (size_t i = 0; i < timers.size(); ++i)
        {
            lua_pushinteger(L, timers[i]);
            lua_rawseti(L, -2, static_cast<int>(i) + 1);
        }
        lua_setfield(L, def, "__timers");
    }
    lua_pushvalue(L, ev);
    lua_setfield(L, def, "events");
    lua_settop(L, def);

    // ---- runtime object
    dumb_ptr<npc_data_script> nd;
    {
        nd = npc_create_script_npc(name, mapname, placed, x, y,
                static_cast<DIR>(dirv),
                wrap<Species>(static_cast<uint16_t>(sprite)), xs, ys);
    }
    if (nd == nullptr)
        return luaL_error(L, "npc.script: cannot create '%s' "
                "(unknown map '%s'?)", name.c_str(), mapname.c_str());
    defs_set(L, nd->bl_id, def);
    lua_npc_register(nd);
    // seed the OnTimer machine from the sorted def.__timers list
    lua_npc_timer_setup(nd, def_timer_intervals(L, def));
    lua_push_npc_handle(L, nd);
    return 1;
}

// ------------------------------------------------------------------------
// npc.warp{}

static
const LString warp_keys[] =
{
    "map"_s, "x"_s, "y"_s, "xs"_s, "ys"_s, "to_map"_s, "to_x"_s, "to_y"_s,
};

static
int ln_ns_warp(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    check_table_keys(L, warp_keys,
            sizeof(warp_keys) / sizeof(warp_keys[0]), "npc.warp");
    MapName mapname = req_mapname_field(L, "npc.warp", "map");
    int x = req_int_field(L, "npc.warp", "x");
    int y = req_int_field(L, "npc.warp", "y");
    // xs/ys are the raw old file numbers; the builder adds 2 like the old
    // parser (-1 keeps meaning "huge")
    int xs = req_int_field(L, "npc.warp", "xs");
    int ys = req_int_field(L, "npc.warp", "ys");
    MapName to_map = req_mapname_field(L, "npc.warp", "to_map");
    int to_x = req_int_field(L, "npc.warp", "to_x");
    int to_y = req_int_field(L, "npc.warp", "to_y");
    dumb_ptr<npc_data_warp> nd;
    {
        nd = npc_create_warp(mapname, x, y, xs, ys, to_map, to_x, to_y);
    }
    if (nd == nullptr)
        return luaL_error(L, "npc.warp: unknown map '%s'", mapname.c_str());
    lua_npc_register(nd);
    return 0;
}

// ------------------------------------------------------------------------
// npc.shop{}

static
ItemNameId shop_item_lookup(lua_State* L, int idx, int* value_buy)
{
    ItemNameId nameid;
    int vb = 0;
    if (lua_type(L, idx) == LUA_TSTRING)
    {
        size_t len;
        ZString nm = luac::to_string(L, idx, &len);
        Option<P<struct item_data>> id_ = itemdb_searchname(XString(nm));
        OMATCH_BEGIN_SOME (id, id_)
        {
            nameid = id->nameid;
            vb = id->value_buy;
        }
        OMATCH_END ();
    }
    else
    {
        int v;
        if (luac::to_int(L, idx, &v) && v > 0 && v <= 65535)
        {
            Option<P<struct item_data>> id_ =
                itemdb_exists(wrap<ItemNameId>(static_cast<uint16_t>(v)));
            OMATCH_BEGIN_SOME (id, id_)
            {
                nameid = id->nameid;
                vb = id->value_buy;
            }
            OMATCH_END ();
        }
    }
    if (value_buy)
        *value_buy = vb;
    return nameid;
}

// price forms: an int is absolute, "*N" multiplies value_buy (old syntax)
static
bool shop_price_parse(lua_State* L, int idx, int value_buy, int* out)
{
    int v;
    if (luac::to_int(L, idx, &v))
    {
        *out = v;
        return true;
    }
    if (lua_type(L, idx) == LUA_TSTRING)
    {
        size_t len;
        ZString s = luac::to_string(L, idx, &len);
        XString sx = s;
        if (sx.size() >= 2 && sx.front() == '*')
        {
            int mult = 0;
            if (extract(sx.xslice_t(1), &mult))
            {
                *out = value_buy * mult;
                return true;
            }
        }
    }
    return false;
}

static
const LString shop_keys[] =
{
    "name"_s, "map"_s, "x"_s, "y"_s, "dir"_s, "sprite"_s, "items"_s,
};

static
int ln_ns_shop(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    check_table_keys(L, shop_keys,
            sizeof(shop_keys) / sizeof(shop_keys[0]), "npc.shop");
    NpcName name = stringish<NpcName>(req_name23_field(L, "npc.shop",
                "name"));
    if (npc_name2id(name) != nullptr)
        return luaL_error(L, "npc.shop: duplicate NPC name '%s'",
                name.c_str());
    MapName mapname = req_mapname_field(L, "npc.shop", "map");
    int x = req_int_field(L, "npc.shop", "x");
    int y = req_int_field(L, "npc.shop", "y");
    int dirv = opt_int_field(L, "npc.shop", "dir", 0, nullptr);
    int sprite = req_int_field(L, "npc.shop", "sprite");
    if (dirv < 0 || dirv > 7)
        return luaL_error(L, "npc.shop: 'dir' must be 0..7");
    if (sprite < -1 || sprite > 65535)
        return luaL_error(L, "npc.shop: 'sprite' must be -1 or 0..65535");
    if (sprite == -1)
        sprite = 0;     // NEGATIVE_SPECIES, as the old file parser mapped it
    if (!push_raw_field(L, 1, "items"))
        return luaL_error(L, "npc.shop: missing 'items'");
    if (!lua_istable(L, -1))
        return luaL_error(L, "npc.shop: 'items' must be a table");
    int items_idx = lua_gettop(L);
    int n = static_cast<int>(luac::raw_len(L, items_idx));
    if (n <= 0)
        return luaL_error(L, "npc.shop: 'items' is empty");
    // pass 1: validate everything (raising is allowed here: only trivially
    // destructible locals exist)
    for (int i = 1; i <= n; ++i)
    {
        lua_rawgeti(L, items_idx, i);
        if (!lua_istable(L, -1))
            return luaL_error(L,
                    "npc.shop: items[%d] must be {item, price}", i);
        lua_rawgeti(L, -1, 1);
        ItemNameId nameid = shop_item_lookup(L, -1, nullptr);
        if (!nameid)
            return luaL_error(L, "npc.shop: unknown item in items[%d]", i);
        lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        int price;
        if (!shop_price_parse(L, -1, 0, &price))
            return luaL_error(L, "npc.shop: bad price in items[%d] "
                    "(integer or \"*N\" expected)", i);
        lua_pop(L, 2);
    }
    // pass 2: build the resolved list and the runtime object; no raising
    // Lua API while the vector lives (doc/lua-engine.md section 11)
    dumb_ptr<npc_data_shop> nd;
    {
        std::vector<npc_item_list> shop_items;
        shop_items.reserve(n);
        for (int i = 1; i <= n; ++i)
        {
            lua_rawgeti(L, items_idx, i);
            lua_rawgeti(L, -1, 1);
            npc_item_list entry;
            int value_buy = 0;
            entry.nameid = shop_item_lookup(L, -1, &value_buy);
            lua_pop(L, 1);
            lua_rawgeti(L, -1, 2);
            entry.value = 0;
            shop_price_parse(L, -1, value_buy, &entry.value);
            lua_pop(L, 2);
            shop_items.push_back(entry);
        }
        nd = npc_create_shop(name, mapname, x, y, static_cast<DIR>(dirv),
                wrap<Species>(static_cast<uint16_t>(sprite)),
                std::move(shop_items));
    }
    if (nd == nullptr)
        return luaL_error(L, "npc.shop: unknown map '%s'", mapname.c_str());
    lua_npc_register(nd);
    return 0;
}

// ------------------------------------------------------------------------
// npc.monster{}

static
const LString monster_keys[] =
{
    "map"_s, "x"_s, "y"_s, "xs"_s, "ys"_s, "name"_s, "species"_s,
    "amount"_s, "delay1"_s, "delay2"_s, "event"_s,
};

static
int ln_ns_monster(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    check_table_keys(L, monster_keys,
            sizeof(monster_keys) / sizeof(monster_keys[0]), "npc.monster");
    MapName mapname = req_mapname_field(L, "npc.monster", "map");
    int x = req_int_field(L, "npc.monster", "x");
    int y = req_int_field(L, "npc.monster", "y");
    int xs = opt_int_field(L, "npc.monster", "xs", 0, nullptr);
    int ys = opt_int_field(L, "npc.monster", "ys", 0, nullptr);
    MobName name = stringish<MobName>(req_name23_field(L, "npc.monster",
                "name"));
    int species = req_int_field(L, "npc.monster", "species");
    int amount = req_int_field(L, "npc.monster", "amount");
    int delay1 = opt_int_field(L, "npc.monster", "delay1", 0, nullptr);
    int delay2 = opt_int_field(L, "npc.monster", "delay2", 0, nullptr);
    if (amount < 1)
        return luaL_error(L, "npc.monster: 'amount' must be >= 1");
    if (species < 0 || species > 65535
            || mobdb_checkid(wrap<Species>(static_cast<uint16_t>(species)))
                == Species())
        return luaL_error(L, "npc.monster: unknown species %d", species);
    NpcEvent ev;
    if (push_raw_field(L, 1, "event"))
    {
        ev = check_event(L, lua_gettop(L));
        lua_pop(L, 1);
    }
    int spawned;
    {
        spawned = npc_create_monster(mapname, x, y, xs, ys, name,
                wrap<Species>(static_cast<uint16_t>(species)), amount,
                interval_t(delay1), interval_t(delay2), ev);
    }
    if (spawned < 0)
        return luaL_error(L, "npc.monster: unknown map '%s'",
                mapname.c_str());
    return 0;
}

// ------------------------------------------------------------------------
// npc.mapflag{}

static
const LString mapflag_keys[] =
{
    "map"_s, "flag"_s, "to"_s, "x"_s, "y"_s, "mask"_s,
};

static
int ln_ns_mapflag(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);
    check_table_keys(L, mapflag_keys,
            sizeof(mapflag_keys) / sizeof(mapflag_keys[0]), "npc.mapflag");
    MapName mapname = req_mapname_field(L, "npc.mapflag", "map");
    if (!push_raw_field(L, 1, "flag"))
        return luaL_error(L, "npc.mapflag: missing 'flag'");
    size_t flen;
    ZString flagname = luac::to_string(L, -1, &flen);
    MapFlag mf;
    if (lua_type(L, -1) != LUA_TSTRING || !extract(XString(flagname), &mf))
        return luaL_error(L, "npc.mapflag: unknown flag '%s'",
                flagname.c_str());
    lua_pop(L, 1);
    bool has_to, has_x, has_y, has_mask;
    MapName to_map = opt_mapname_field(L, "npc.mapflag", "to", &has_to);
    int x = opt_int_field(L, "npc.mapflag", "x", 0, &has_x);
    int y = opt_int_field(L, "npc.mapflag", "y", 0, &has_y);
    int mask = opt_int_field(L, "npc.mapflag", "mask", 0, &has_mask);
    if (mf == MapFlag::NOSAVE || mf == MapFlag::RESAVE)
    {
        if (!has_to || !has_x || !has_y)
            return luaL_error(L, "npc.mapflag: flag needs 'to', 'x', 'y'");
        if (has_mask)
            return luaL_error(L, "npc.mapflag: 'mask' not expected here");
    }
    else if (mf == MapFlag::MASK)
    {
        if (!has_mask)
            return luaL_error(L, "npc.mapflag: flag 'mask' needs 'mask'");
        if (has_to || has_x || has_y)
            return luaL_error(L,
                    "npc.mapflag: 'to'/'x'/'y' not expected here");
    }
    else if (has_to || has_x || has_y || has_mask)
        return luaL_error(L,
                "npc.mapflag: no extra keys expected for this flag");
    bool ok;
    {
        ok = npc_set_mapflag(mapname, mf, to_map, x, y, mask);
    }
    if (!ok)
        return luaL_error(L, "npc.mapflag: cannot set '%s' on '%s'",
                flagname.c_str(), mapname.c_str());
    return 0;
}

// ------------------------------------------------------------------------
// registration

static
const luaL_Reg npc_methods[] =
{
    {"enable", ln_self_enable},
    {"disable", ln_self_disable},
    {"warp", ln_self_warp},
    {"areawarp", ln_self_areawarp},
    {"setdirection", ln_self_setdirection},
    {"talk", ln_self_talk},
    {"emotion", ln_self_emotion},
    {"misceffect", ln_self_misceffect},
    {"specialeffect", ln_self_specialeffect},
    {"announce", ln_self_announce},
    {"rename", ln_self_rename},
    {"initnpctimer", ln_self_initnpctimer},
    {"startnpctimer", ln_self_startnpctimer},
    {"stopnpctimer", ln_self_stopnpctimer},
    {"getnpctimer", ln_self_getnpctimer},
    {"setnpctimer", ln_self_setnpctimer},
    {"addnpctimer", ln_self_addnpctimer},
    {"puppet", ln_self_puppet},
    {"destroy", ln_self_destroy},
    {"exists", ln_self_exists},
    // "event" is installed separately (the pure-Lua wrapper)
    {nullptr, nullptr},
};

static
const luaL_Reg npc_ns_funcs[] =
{
    {"script", ln_ns_script},
    {"warp", ln_ns_warp},
    {"shop", ln_ns_shop},
    {"monster", ln_ns_monster},
    {"mapflag", ln_ns_mapflag},
    {"get", ln_ns_get},
    {"byid", ln_ns_byid},
    {"exists", ln_ns_exists},
    {"enable", ln_ns_enable},
    {"disable", ln_ns_disable},
    {"event_all", ln_event_all},
    // "event" is installed separately (the pure-Lua wrapper)
    {nullptr, nullptr},
};

// The npc.event dispatch wrapper. DESIGN DEVIATION: doc/lua-engine.md
// section 9 puts this in the prelude, but lua_init loads the prelude only
// after build_sandbox (which calls lua_register_lib_npc), so the identical
// pure-Lua chunk is compiled here instead. The point of the Lua layer is
// unchanged: when the target handler runs inline in the caller's dialog
// coroutine, only Lua frames sit between the coroutine and a later yield.
static
const char npc_event_chunk[] =
"local resolve_c, event_c = ...\n"
"local function npc_event(ev, p, args)\n"
"    local handler, self = resolve_c(ev, p)\n"
"    if handler ~= nil then\n"
"        handler(self, p, args)\n"
"        return true\n"
"    end\n"
"    return event_c(ev, p, args)\n"
"end\n"
"local function self_event(self, label, p, args)\n"
"    return npc_event(self.name .. \"::\" .. label, p, args)\n"
"end\n"
"return npc_event, self_event\n";

void lua_register_lib_npc(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);
    ensure_defs(L);

    // the shared methods table
    lua_newtable(L);
    int methods = lua_gettop(L);
    luac::register_funcs(L, methods, npc_methods);
    // chain to the being methods if lua-handle-being.cpp published them
    // under the shared registry key (skip quietly when absent)
    lua_getfield(L, LUA_REGISTRYINDEX, "tmwa:being:methods");
    if (lua_istable(L, -1))
    {
        lua_createtable(L, 0, 1);
        lua_insert(L, -2);      // [mt, being methods]
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, methods);
    }
    else
        lua_pop(L, 1);

    // the handle metatable, kept in the registry under npc_mt_name
    luaL_newmetatable(L, npc_mt_name);
    lua_pushvalue(L, methods);
    luac::push_cclosure(L, ln_npc_index, "npc.__index", 1);
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, ln_npc_newindex, "npc.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);

    // the npc namespace table
    lua_newtable(L);
    int ns = lua_gettop(L);
    luac::register_funcs(L, ns, npc_ns_funcs);

    // npc.event and self:event
    bool wrapped = false;
    {
        XString src = XString(npc_event_chunk,
                npc_event_chunk + sizeof(npc_event_chunk) - 1, nullptr);
        if (luac::load_chunk(L, "=npc-event"_s, src))
        {
            luac::push_cfunction(L, ln_event_resolve, "npc.event.resolve");
            luac::push_cfunction(L, ln_event_c, "npc.event.c");
            if (lua_pcall(L, 2, 2, 0) == LUA_OK)
            {
                // results: npc_event closure, self_event closure
                lua_setfield(L, methods, "event");
                lua_setfield(L, ns, "event");
                wrapped = true;
            }
            else
            {
                size_t len;
                ZString err = luac::to_string(L, -1, &len);
                PRINTF("lua: internal: npc.event wrapper failed: %s\n"_fmt,
                        AString(XString(err)));
                lua_pop(L, 1);
            }
        }
        else
        {
            size_t len;
            ZString err = luac::to_string(L, -1, &len);
            PRINTF("lua: internal: npc.event wrapper does not compile: "
                    "%s\n"_fmt, AString(XString(err)));
            lua_pop(L, 1);
        }
    }
    if (!wrapped)
    {
        // C fallback: identical semantics except that inline dispatch
        // cannot cross the C frame (nested handlers cannot yield)
        luac::push_cfunction(L, ln_event_c, "npc.event");
        lua_setfield(L, ns, "event");
        luac::push_cfunction(L, ln_self_event_c, "self.event");
        lua_setfield(L, methods, "event");
    }

    lua_pushvalue(L, ns);
    lua_setfield(L, env, "npc");
    lua_pop(L, 2);      // ns and methods
}
} // namespace map
} // namespace tmwa
