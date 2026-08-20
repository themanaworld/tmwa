#include "lua-handles.hpp"
//    lua-handle-being.cpp - the being handle base metatable.
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

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../compat/borrow.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "../mmo/skill.t.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "skill.hpp"
#include "lua-engine.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


// Handle table convention (shared by lua-handle-player.cpp and lua-npc.cpp,
// doc/lua-api.md section 1.1):
//   - a handle is a plain Lua table with the integer block id in the raw
//     field "__id"; player handles (and the player vars proxies) also carry
//     the session serial in "__serial" so a handle from a previous login of
//     the same account resolves as stale.
//   - metatables live in the registry under luaL_newmetatable keys:
//     "tmwa.being" (mobs, floor items), "tmwa.player", "tmwa.npc".
//     "tmwa.npc" is pre-created here with the generic being dispatch so NPC
//     handles get being-level members even before lua-npc.cpp installs its
//     own __index/__newindex over the same table.
//   - the shared being methods table is upvalue 1 of the being __index and
//     is passed to the player registration so player handles inherit every
//     being method (doc/lua-api.md section 6: "player and NPC handles are
//     also beings").

namespace tmwa
{
namespace map
{
// implemented in lua-handle-player.cpp (internal hook between the two
// handle TUs; not part of any public header)
void lua_register_player_handle_parts(lua_State* L, int being_methods_idx,
        int env_idx);

static int g_being_methods_ref = lua_noref;
static int g_being_mt_ref = lua_noref;
static int g_beings_cache_ref = lua_noref;

// ------------------------------------------------------------------------
// small helpers

static
int abs_index(lua_State* L, int idx)
{
    return idx > 0 ? idx : lua_gettop(L) + idx + 1;
}

// Extract the block id out of a handle table. is_handle reports whether the
// value looked like a handle at all (a table with an integer "__id"); a
// player handle whose "__serial" no longer matches the live session resolves
// to the zero id (stale: the account logged out, possibly back in).
static
BlockId handle_extract(lua_State* L, int idx, bool* is_handle)
{
    *is_handle = false;
    idx = abs_index(L, idx);
    if (lua_type(L, idx) != LUA_TTABLE)
        return BlockId();
    lua_pushlstring(L, "__id", 4);
    lua_rawget(L, idx);
    if (!luac::is_integer_value(L, -1))
    {
        lua_pop(L, 1);
        return BlockId();
    }
    long long raw = static_cast<long long>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    if (raw <= 0 || raw > 0x7fffffff)
        return BlockId();
    *is_handle = true;
    BlockId id = wrap<BlockId>(static_cast<uint32_t>(raw));
    lua_pushlstring(L, "__serial", 8);
    lua_rawget(L, idx);
    if (luac::is_integer_value(L, -1))
    {
        int serial = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        dumb_ptr<map_session_data> sd = map_id_is_player(id);
        if (sd != nullptr && sd->lua.serial != serial)
            // the account logged back in: this handle belongs to the old
            // session and must not touch the new one
            return BlockId();
    }
    else
        lua_pop(L, 1);
    return id;
}

BlockId lua_handle_id(lua_State* L, int idx)
{
    bool is_handle;
    return handle_extract(L, idx, &is_handle);
}

BlockId lua_check_handle_id(lua_State* L, int idx)
{
    bool is_handle;
    BlockId id = handle_extract(L, idx, &is_handle);
    if (!is_handle)
        luaL_argerror(L, idx, "handle expected");
    return id;
}

// Accept an integer or an integral float; integers outside int32 wrap
// (doc/lua-api.md 1.2: values wrap to int32 at every write boundary).
// Shared with lua-handle-player.cpp.
int lua_check_int32_wrap(lua_State* L, int idx)
{
    if (luac::is_integer_value(L, idx))
    {
        long long v = static_cast<long long>(lua_tointeger(L, idx));
        return static_cast<int32_t>(static_cast<uint32_t>(
                    static_cast<uint64_t>(v)));
    }
    int out;
    if (luac::to_int(L, idx, &out))
        return out;
    luaL_argerror(L, idx, "integer expected");
    return 0; // unreachable
}

// The sc_start tick heuristic (doc/lua-api.md 5.4), kept byte-exact from the
// old builtin_sc_start: tick < 1000 means seconds unless the type always
// uses milliseconds. Shared with lua-handle-player.cpp via being methods.
static
interval_t sc_start_tick(int type, int ms)
{
    if (ms < 1000)
    {
        switch (static_cast<StatusChange>(type))
        {
            // these always use milliseconds (cooldowns are often < 1 s)
            case StatusChange::SC_PHYS_SHIELD:
            case StatusChange::SC_PHYS_SHIELD_ITEM:
            case StatusChange::SC_MBARRIER:
            case StatusChange::SC_COOLDOWN:
            case StatusChange::SC_COOLDOWN_MG:
            case StatusChange::SC_COOLDOWN_MT:
            case StatusChange::SC_COOLDOWN_R:
            case StatusChange::SC_COOLDOWN_AR:
            case StatusChange::SC_COOLDOWN_ENCH:
            case StatusChange::SC_COOLDOWN_KOY:
            case StatusChange::SC_COOLDOWN_UPMARMU:
            case StatusChange::SC_COOLDOWN_SG:
            case StatusChange::SC_COOLDOWN_CG:
            case StatusChange::SC_SLOWMOVE:
            case StatusChange::SC_CANTMOVE:
                break;
            default:
                // legacy "seconds" semantics (potions)
                ms *= 1000;
        }
    }
    return static_cast<interval_t>(ms);
}

// second argument of distance/target/injure/aggravate: a handle or a raw
// block id; an id that resolves to nothing raises (the old code crashed)
static
dumb_ptr<block_list> check_being_or_id(lua_State* L, int idx)
{
    BlockId id;
    if (lua_type(L, idx) == LUA_TTABLE)
        id = lua_check_handle_id(L, idx);
    else
    {
        int num = check_int(L, idx);
        if (num > 0)
            id = wrap<BlockId>(static_cast<uint32_t>(num));
    }
    dumb_ptr<block_list> bl;
    if (id)
        bl = map_id2bl(id);
    if (bl == nullptr)
        luaL_argerror(L, idx, "no such being");
    return bl;
}

// ------------------------------------------------------------------------
// being methods (shared: copied into the player methods table too)

static
int lb_exists(lua_State* L)
{
    BlockId id = lua_check_handle_id(L, 1);
    bool alive = false;
    if (id)
        alive = map_id2bl(id) != nullptr;
    lua_pushboolean(L, alive);
    return 1;
}

static
int lb_sc_start(lua_State* L)
{
    dumb_ptr<block_list> bl = check_being(L, 1);
    int type = check_int(L, 2);
    int ms = check_int(L, 3);
    int val1 = check_int(L, 4);
    if (bl == nullptr)
        return 0;
    {
        skill_status_change_start(bl, static_cast<StatusChange>(type), val1,
                sc_start_tick(type, ms));
    }
    return 0;
}

static
int lb_sc_end(lua_State* L)
{
    dumb_ptr<block_list> bl = check_being(L, 1);
    int type = check_int(L, 2);
    if (bl == nullptr)
        return 0;
    {
        skill_status_change_end(bl, static_cast<StatusChange>(type), nullptr);
    }
    return 0;
}

static
int lb_sc_check(lua_State* L)
{
    dumb_ptr<block_list> bl = check_being(L, 1);
    int type = check_int(L, 2);
    if (bl == nullptr)
    {
        lua_pushboolean(L, 0);
        return 1;
    }
    bool active;
    {
        active = skill_status_change_active(bl,
                static_cast<StatusChange>(type)) != 0;
    }
    lua_pushboolean(L, active);
    return 1;
}

static
int lb_distance(lua_State* L)
{
    dumb_ptr<block_list> self = check_being(L, 1);
    dumb_ptr<block_list> other = check_being_or_id(L, 2);
    if (self == nullptr)
        return 0;
    int dist;
    {
        if (self->bl_m != other->bl_m)
            dist = 0x7fffffff;
        else
        {
            int dx = abs(self->bl_x - other->bl_x);
            int dy = abs(self->bl_y - other->bl_y);
            dist = static_cast<int>(sqrt((dx * dx) + (dy * dy)));
        }
    }
    luac::push_int(L, dist);
    return 1;
}

static
int lb_target(lua_State* L)
{
    dumb_ptr<block_list> source = check_being(L, 1);
    dumb_ptr<block_list> target = check_being_or_id(L, 2);
    int flag = check_int(L, 3);
    if (source == nullptr)
    {
        luac::push_int(L, 0);
        return 1;
    }
    int val = 0;
    {
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
        // 0x08 (line of sight through collision) was never implemented
        if (flag & 0x10)
        {
            // old code called pc_iskiller with an unchecked is_player() on
            // the source; guard it so a non-player source cannot abort the
            // server (assertions are live in production builds)
            if (target->bl_type != BL::PC
                || target->bl_m->flag.get(MapFlag::PVP)
                || (source->bl_type == BL::PC
                    && pc_iskiller(source->is_player(), target->is_player())))
                val |= 0x10;    // target can be attacked by source
        }
        if (flag & 0x20)
        {
            if (battle_check_range(source, target, 0))
                val |= 0x20;    // target is in line of sight
        }
    }
    luac::push_int(L, val);
    return 1;
}

static
int lb_injure(lua_State* L)
{
    dumb_ptr<block_list> source = check_being(L, 1);
    dumb_ptr<block_list> target = check_being_or_id(L, 2);
    int damage = check_int(L, 3);
    if (source == nullptr)
        return 0;
    {
        if (source->bl_type == BL::PC)
            pc_setstand(source->is_player());
        // display damage first: dealing damage may deallocate the target
        clif_damage(source, target, gettick(),
                interval_t::zero(), interval_t::zero(),
                damage, 0, DamageType::NORMAL);
        battle_damage(source, target, damage, 0);
    }
    return 0;
}

static
int lb_aggravate(lua_State* L)
{
    dumb_ptr<block_list> self = check_being(L, 1);
    dumb_ptr<block_list> target;
    if (!lua_isnoneornil(L, 2))
        target = check_being_or_id(L, 2);
    if (self == nullptr)
        return 0;
    if (self->bl_type != BL::MOB)
    {
        lua_warn("aggravate: not a mob"_s);
        return 0;
    }
    {
        if (target == nullptr)
            // the old builtin defaulted to the attached player
            target = map_id2bl(lua_current_ctx().player);
        if (target != nullptr)
            mob_aggravate(self->is_mob(), target);
    }
    if (target == nullptr)
        lua_warn("aggravate: no target"_s);
    return 0;
}

static
int lb_issummon(lua_State* L)
{
    dumb_ptr<block_list> bl = check_being(L, 1);
    bool summoned = false;
    if (bl != nullptr && bl->bl_type == BL::MOB)
        summoned = bool(bl->is_mob()->mode & MobMode::SUMMONED);
    lua_pushboolean(L, summoned);
    return 1;
}

static
int lb_misceffect(lua_State* L)
{
    dumb_ptr<block_list> bl = check_being(L, 1);
    int type = check_int(L, 2);
    if (bl == nullptr)
        return 0;
    {
        clif_misceffect(bl, type);
    }
    return 0;
}

static
int lb_emotion(lua_State* L)
{
    dumb_ptr<block_list> bl = check_being(L, 1);
    int type = check_int(L, 2);
    if (bl == nullptr)
        return 0;
    if (type < 0 || type > 200)
        return 0;   // old behaviour: silent no-op
    {
        clif_emotion(bl, type);
    }
    return 0;
}

static
const luaL_Reg being_funcs[] =
{
    {"exists", lb_exists},
    {"sc_start", lb_sc_start},
    {"sc_end", lb_sc_end},
    {"sc_check", lb_sc_check},
    {"distance", lb_distance},
    {"target", lb_target},
    {"injure", lb_injure},
    {"aggravate", lb_aggravate},
    {"issummon", lb_issummon},
    {"misceffect", lb_misceffect},
    {"emotion", lb_emotion},
    {nullptr, nullptr},
};

// ------------------------------------------------------------------------
// being property dispatch (doc/lua-api.md section 6)

// upvalue 1: the being methods table
static
int being_index(lua_State* L)
{
    // stack: handle, key
    if (lua_type(L, 2) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(1));
    if (!lua_isnil(L, -1))
        return 1;   // a method
    lua_pop(L, 1);

    size_t klen;
    ZString key = luac::to_string(L, 2, &klen);
    BlockId id = lua_handle_id(L, 1);
    dumb_ptr<block_list> bl;
    if (id)
        bl = map_id2bl(id);
    if (bl == nullptr)
    {
        lua_warn(STRPRINTF("stale being handle (read of '%s')"_fmt, key));
        lua_pushnil(L);
        return 1;
    }

    if (key == "id"_s)
        return luac::push_int(L,
                static_cast<int>(unwrap<BlockId>(bl->bl_id)));
    if (key == "type"_s)
    {
        LString t = "unknown"_s;
        switch (bl->bl_type)
        {
            case BL::PC: t = "player"_s; break;
            case BL::NPC: t = "npc"_s; break;
            case BL::MOB: t = "mob"_s; break;
            case BL::ITEM: t = "item"_s; break;
            default: break;
        }
        push_string(L, t);
        return 1;
    }
    if (key == "x"_s)
        return luac::push_int(L, bl->bl_x);
    if (key == "y"_s)
        return luac::push_int(L, bl->bl_y);
    if (key == "map"_s)
    {
        if (bl->bl_m == borrow(undefined_gat))
            push_string(L, ""_s);
        else
            push_string(L, bl->bl_m->name_);
        return 1;
    }

    int sp;
    if (lua_param_lookup(key, &sp))
    {
        int v;
        {
            v = pc_readparam(bl, static_cast<SP>(sp));
        }
        return luac::push_int(L, v);
    }

    lua_pushnil(L);
    return 1;
}

static
int being_newindex(lua_State* L)
{
    // stack: handle, key, value
    if (lua_type(L, 2) != LUA_TSTRING)
        return luaL_error(L, "being handle keys must be strings");
    size_t klen;
    ZString key = luac::to_string(L, 2, &klen);

    int sp;
    if (lua_param_lookup(key, &sp))
    {
        int v = lua_check_int32_wrap(L, 3);
        BlockId id = lua_handle_id(L, 1);
        dumb_ptr<block_list> bl;
        if (id)
            bl = map_id2bl(id);
        if (bl == nullptr)
        {
            // stale handle: writes are ignored with a warning
            lua_warn(STRPRINTF("stale being handle (write of '%s')"_fmt, key));
            return 0;
        }
        {
            // pc_setparam no-ops (with its own logging) for params that do
            // not apply to this being type, exactly as the old `set` builtin
            pc_setparam(bl, static_cast<SP>(sp), v);
        }
        return 0;
    }

    return luaL_error(L, "being handle has no field '%s'",
            lua_tostring(L, 2));
}

// ------------------------------------------------------------------------
// registration and handle creation

void lua_register_handle_metatables(lua_State* L)
{
    // contract (lua-handles.hpp): the sandbox environment is on top
    int env = lua_gettop(L);

    // the shared being methods table
    lua_newtable(L);
    int bm = lua_gettop(L);
    luac::register_funcs(L, bm, being_funcs);
    lua_pushvalue(L, bm);
    g_being_methods_ref = luac::ref(L);

    // being metatable: registry["tmwa.being"]
    luaL_newmetatable(L, "tmwa.being");
    lua_pushvalue(L, bm);
    luac::push_cclosure(L, being_index, "being.__index", 1);
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, being_newindex, "being.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pushvalue(L, -1);
    g_being_mt_ref = luac::ref(L);
    lua_pop(L, 1);

    // NPC metatable placeholder: registry["tmwa.npc"], generic being
    // dispatch until lua-npc.cpp installs the NPC-specific __index over the
    // same table (luaL_newmetatable returns the existing one)
    luaL_newmetatable(L, "tmwa.npc");
    lua_pushvalue(L, bm);
    luac::push_cclosure(L, being_index, "npc.__index", 1);
    lua_setfield(L, -2, "__index");
    luac::push_cfunction(L, being_newindex, "npc.__newindex");
    lua_setfield(L, -2, "__newindex");
    lua_pop(L, 1);

    // weak-valued cache of mob/floor-item handles (block ids are never
    // reused while the server runs, so a cached table stays correct; the
    // weak values let unused handles be collected)
    lua_newtable(L);
    lua_newtable(L);
    lua_pushlstring(L, "v", 1);
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    g_beings_cache_ref = luac::ref(L);

    // player metatable, methods, proxies (lua-handle-player.cpp)
    lua_register_player_handle_parts(L, bm, env);

    lua_pop(L, 1);   // bm; env is on top again
}

void lua_push_being_handle(lua_State* L, dumb_ptr<block_list> bl)
{
    if (bl == nullptr)
    {
        lua_pushnil(L);
        return;
    }
    if (bl->bl_type == BL::PC)
    {
        lua_push_player_handle(L, bl->is_player());
        return;
    }
    if (bl->bl_type == BL::NPC)
    {
        lua_push_npc_handle(L, bl->is_npc());
        return;
    }
    int id = static_cast<int>(unwrap<BlockId>(bl->bl_id));
    luac::push_ref(L, g_beings_cache_ref);
    lua_rawgeti(L, -1, id);
    if (lua_type(L, -1) == LUA_TTABLE)
    {
        lua_remove(L, -2);   // the cache
        return;
    }
    lua_pop(L, 1);
    lua_createtable(L, 0, 1);
    lua_pushinteger(L, id);
    lua_setfield(L, -2, "__id");
    luac::push_ref(L, g_being_mt_ref);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, -1);
    lua_rawseti(L, -3, id);   // cache[id] = handle
    lua_remove(L, -2);        // the cache
}
} // namespace map
} // namespace tmwa
