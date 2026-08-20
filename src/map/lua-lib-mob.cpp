#include "lua-libs.hpp"
//    lua-lib-mob.cpp - the `mob` namespace.
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

#include "../compat/fun.hpp"
#include "../compat/option.hpp"

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
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
// section 11: raising checks first (trivial locals only), then a worker
// scope with tmwa types and no raising Lua API, then result pushes.

// script-fun.cpp's summon attitude values (that file is deleted with the old
// engine, so the enum lives here now)
enum class MonsterAttitude
{
    HOSTILE     = 0,
    FRIENDLY    = 1,
    SERVANT     = 2,
    FROZEN      = 3,
};

// Event tag arguments ("Npc::OnX", "~Tag", or the old unparseable-therefore-
// empty quirk). Mirrors the old extract(): a string without "::" leaves the
// event empty, which matches mobs spawned without an event (kept quirk);
// "~Tag" without "::" is the API's phony-tag convenience.
static
NpcEvent check_eventtag(lua_State* L, int idx)
{
    ZString z = check_string(L, idx);
    NpcEvent ev;
    XString whole = z;
    LString colons = "::"_s;
    auto it = std::search(whole.begin(), whole.end(),
            colons.begin(), colons.end());
    if (it != whole.end())
    {
        XString npc = whole.xislice_h(it);
        XString label = whole.xislice_t(it + 2);
        if (npc.size() > 23)
            luaL_argerror(L, idx, "event NPC name too long (max 23)");
        if (label.size() > 23)
            luaL_argerror(L, idx, "event label too long (max 23)");
        ev.npc = stringish<NpcName>(npc);
        ev.label = stringish<ScriptLabel>(label);
    }
    else if (whole.size() && whole.front() == '~')
    {
        if (whole.size() > 23)
            luaL_argerror(L, idx, "event tag too long (max 23)");
        ev.npc = stringish<NpcName>(whole);
    }
    return ev;
}

// Store the function at fn_idx as the death handler of mob `id` in the
// engine's mob_death_fns table (doc/lua-engine.md section 2.2). The table
// anchors the function; lua_mob_forget / the 10-minute sweep remove it.
static
void mob_store_death_fn(lua_State* L, BlockId id, int fn_idx)
{
    if (!id)
        return;
    int fn = lua_absindex(L, fn_idx);
    lua_push_engine_table(L, LuaTable::MOB_DEATH_FNS);
    lua_pushinteger(L, static_cast<int>(unwrap<BlockId>(id)));
    lua_pushvalue(L, fn);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

// Fresh synthetic identity tag for a function-form death event.
static
NpcName mob_synthetic_tag()
{
    static int counter = 0;
    NpcName tag;
    SNPRINTF(tag, 24, "~lua#%d"_fmt, ++counter);
    return tag;
}

// ------------------------------------------------------------------------
// spawns

// mob.monster(map, x, y, name, species, amount [, event]) -> eventtag
// Old: monster ("this" and x,y <= 0 conveniences are gone, deviation 14)
static
int lmob_monster(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x = check_int(L, 2);
    int y = check_int(L, 3);
    MobName mobname;
    {
        ZString name_ = check_string(L, 4);
        mobname = stringish<MobName>(name_);
    }
    Species mob_class = wrap<Species>(check_int(L, 5));
    int amount = check_int(L, 6);
    if (lua_type(L, 7) == LUA_TFUNCTION)
    {
        // function form: synthetic tag; each spawned mob's id maps to the
        // function in mob_death_fns
        NpcEvent ev;
        ev.npc = mob_synthetic_tag();
        for (int i = 0; i < amount; i++)
        {
            BlockId id;
            {
                id = mob_once_spawn(nullptr, mapname, x, y, mobname,
                        mob_class, 1, ev);
            }
            if (!id)
                break;          // bad species/map: every spawn would fail
            mob_store_death_fn(L, id, 7);
        }
        push_string(L, ev.npc);
        return 1;
    }
    NpcEvent ev;
    if (!lua_isnoneornil(L, 7))
        ev = check_eventtag(L, 7);
    {
        mob_once_spawn(nullptr, mapname, x, y, mobname, mob_class, amount, ev);
    }
    // the event string actually stored on the mobs
    if (!lua_isnoneornil(L, 7))
        lua_pushvalue(L, 7);
    else
        lua_pushliteral(L, "");
    return 1;
}

// mob.areamonster(map, x0, y0, x1, y1, name, species, amount [, event])
// Old: areamonster
static
int lmob_areamonster(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x0 = check_int(L, 2);
    int y0 = check_int(L, 3);
    int x1 = check_int(L, 4);
    int y1 = check_int(L, 5);
    MobName mobname;
    {
        ZString name_ = check_string(L, 6);
        mobname = stringish<MobName>(name_);
    }
    Species mob_class = wrap<Species>(check_int(L, 7));
    int amount = check_int(L, 8);
    if (lua_type(L, 9) == LUA_TFUNCTION)
    {
        // function form: one spawn call per mob so every block id is known.
        // (Only the last-good-cell fallback across mobs differs from a
        // single amount-N call; that fallback needs a nearly full map.)
        NpcEvent ev;
        ev.npc = mob_synthetic_tag();
        for (int i = 0; i < amount; i++)
        {
            BlockId id;
            {
                id = mob_once_spawn_area(nullptr, mapname, x0, y0, x1, y1,
                        mobname, mob_class, 1, ev);
            }
            if (!id)
                break;
            mob_store_death_fn(L, id, 9);
        }
        push_string(L, ev.npc);
        return 1;
    }
    NpcEvent ev;
    if (!lua_isnoneornil(L, 9))
        ev = check_eventtag(L, 9);
    {
        mob_once_spawn_area(nullptr, mapname, x0, y0, x1, y1, mobname,
                mob_class, amount, ev);
    }
    if (!lua_isnoneornil(L, 9))
        lua_pushvalue(L, 9);
    else
        lua_pushliteral(L, "");
    return 1;
}

// mob.summon(map, x, y, owner, name, species, attitude, lifespan_ms
//            [, event]) -> eventtag   Old: summon
static
int lmob_summon(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    int x = check_int(L, 2);
    int y = check_int(L, 3);
    // owner: handle or id
    BlockId owner_id;
    if (lua_istable(L, 4))
        owner_id = lua_check_handle_id(L, 4);
    else
        owner_id = wrap<BlockId>(static_cast<uint32_t>(check_int(L, 4)));
    MobName mobname;
    {
        ZString name_ = check_string(L, 5);
        mobname = stringish<MobName>(name_);
    }
    Species monster_id = wrap<Species>(check_int(L, 6));
    MonsterAttitude monster_attitude =
            static_cast<MonsterAttitude>(check_int(L, 7));
    int lifespan_ms = check_int(L, 8);
    bool fn_form = (lua_type(L, 9) == LUA_TFUNCTION);
    NpcEvent ev;
    if (fn_form)
        ev.npc = mob_synthetic_tag();
    else if (!lua_isnoneornil(L, 9))
        ev = check_eventtag(L, 9);

    BlockId mob_id;
    {
        dumb_ptr<block_list> owner_e = map_id2bl(owner_id);
        if (!owner_e)
        {
            lua_warn("mob.summon: bad owner"_s);
            lua_pushliteral(L, "");
            return 1;
        }
        dumb_ptr<map_session_data> owner = nullptr;
        if (monster_attitude == MonsterAttitude::SERVANT
            && owner_e->bl_type == BL::PC)
            owner = owner_e->is_player();

        interval_t lifespan = static_cast<interval_t>(lifespan_ms);
        mob_id = mob_once_spawn(owner, mapname, x, y, mobname, monster_id,
                1, ev);
        dumb_ptr<mob_data> mob = map_id_is_mob(mob_id);

        if (mob)
        {
            mob->mode = get_mob_db(monster_id).mode;

            switch (monster_attitude)
            {
                case MonsterAttitude::SERVANT:
                    mob->state.special_mob_ai = 1;
                    mob->mode |= MobMode::AGGRESSIVE;
                    break;

                case MonsterAttitude::FRIENDLY:
                    mob->mode = MobMode::CAN_ATTACK
                            | (mob->mode & MobMode::CAN_MOVE);
                    break;

                case MonsterAttitude::HOSTILE:
                    mob->mode = MobMode::CAN_ATTACK | MobMode::AGGRESSIVE
                            | (mob->mode & MobMode::CAN_MOVE);
                    if (owner)
                    {
                        mob->target_id = owner->bl_id;
                        mob->attacked_id = owner->bl_id;
                    }
                    break;

                case MonsterAttitude::FROZEN:
                    mob->mode = MobMode::ZERO;
                    break;
            }

            // | MobMode::TURNS_AGAINST_BAD_MASTER is fun but bugged (was the
            // source of AFK PK city exploits, etc.)
            mob->mode |= MobMode::SUMMONED;
            mob->deletetimer = Timer(gettick() + lifespan,
                    std::bind(mob_timer_delete, ph::_1, ph::_2, mob_id));

            if (owner)
            {
                mob->master_id = owner->bl_id;
                mob->master_dist = 6;
            }
        }
    }
    if (fn_form && mob_id)
        mob_store_death_fn(L, mob_id, 9);
    if (fn_form)
        push_string(L, ev.npc);
    else if (!lua_isnoneornil(L, 9))
        lua_pushvalue(L, 9);
    else
        lua_pushliteral(L, "");
    return 1;
}

// ------------------------------------------------------------------------
// removal and counting

static
void killmonster_sub(dumb_ptr<block_list> bl, NpcEvent event)
{
    dumb_ptr<mob_data> md = bl->is_mob();
    if (event)
    {
        if (event == md->npc_event)
            mob_delete(md);
        return;
    }
    else
    {
        if (md->spawn.delay1 == static_cast<interval_t>(-1)
            && md->spawn.delay2 == static_cast<interval_t>(-1))
            mob_delete(md);
        return;
    }
}

// mob.killmonster(map, event); "All" = every once-spawned mob
// Old: killmonster
static
int lmob_killmonster(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    ZString event_ = check_string(L, 2);
    NpcEvent event;
    if (event_ != "All"_s)
        event = check_eventtag(L, 2);
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
        {
            lua_warn(STRPRINTF("mob.killmonster: unknown map '%s'"_fmt,
                        mapname));
            return 0;
        });
        map_foreachinarea(std::bind(killmonster_sub, ph::_1, event),
                m, 0, 0, m->xs, m->ys, BL::MOB);
    }
    return 0;
}

static
void mobcount_sub(dumb_ptr<block_list> bl, NpcEvent event, int* c)
{
    if (event == bl->is_mob()->npc_event)
        (*c)++;
}

// mob.mobcount(map, event) -> int; KEEPS the minus-one (0 matching mobs
// returns -1; do not "fix" this, 98 call sites compare < 0 / <= 0)
// Old: mobcount
static
int lmob_mobcount(lua_State* L)
{
    MapName mapname = check_mapname(L, 1);
    NpcEvent event = check_eventtag(L, 2);
    int c = 0;
    {
        P<map_local> m = TRY_UNWRAP(map_mapname2mapid(mapname),
                { return luac::push_int(L, -1); });
        map_foreachinarea(std::bind(mobcount_sub, ph::_1, event, &c),
                m, 0, 0, m->xs, m->ys, BL::MOB);
    }
    return luac::push_int(L, c - 1);
}

// ------------------------------------------------------------------------
// mob db queries

// mob.mobinfo(species, what) -> int or string, -1 invalid   Old: mobinfo
// `what` keeps the old MobInfo numbering (script-fun.t.hpp, deleted with the
// old engine): 0 ID, 1 ENG_NAME, 2 JAP_NAME, 3 LVL, 4 HP, 5 SP, 6 BASE_EXP,
// 7 JOB_EXP, 8 RANGE1, 9 ATK1, 10 ATK2, 11 DEF, 12 MDEF, 13 CRITICAL_DEF,
// 14..19 STR..LUK, 20 RANGE2, 21 RANGE3, 22 SCALE, 23 RACE, 24 ELEMENT,
// 25 ELEMENT_LVL, 26 MODE, 27 SPEED, 28 ADELAY, 29 AMOTION, 30 DMOTION,
// 31 MUTATION_NUM, 32 MUTATION_POWER, 33+3n DROPID(n), 34+3n DROPNAME(n),
// 35+3n DROPPERCENT(n) for n = 0..9.
static
int lmob_mobinfo(lua_State* L)
{
    Species mob_id = wrap<Species>(check_int(L, 1));
    int request = check_int(L, 2);
    int info = 0;
    ItemName drop_name;
    MobName mob_name;
    // 0 = int, 1 = mob name, 2 = drop name, -1 = invalid
    int mode = 0;
    {
        if (mobdb_checkid(mob_id) == Species())
            mode = -1;
        else if (request >= 33 && request <= 62)
        {
            int index = (request - 33) / 3;
            int kind = (request - 33) % 3;
            const mob_db_& db = get_mob_db(mob_id);
            if (kind == 0)
                info = unwrap<ItemNameId>(db.dropitem[index].nameid);
            else if (kind == 2)
                info = db.dropitem[index].p.num;
            else
            {
                drop_name = itemdb_search(db.dropitem[index].nameid)->name;
                mode = 2;
            }
        }
        else
        {
            const mob_db_& db = get_mob_db(mob_id);
            switch (request)
            {
                case 0: info = unwrap<Species>(mob_id); break;
                case 1: mob_name = db.name; mode = 1; break;
                case 2: mob_name = db.jname; mode = 1; break;
                case 3: info = db.lv; break;
                case 4: info = db.max_hp; break;
                case 5: info = db.max_sp; break;
                case 6: info = db.base_exp; break;
                case 7: info = db.job_exp; break;
                case 8: info = db.range; break;
                case 9: info = db.atk1; break;
                case 10: info = db.atk2; break;
                case 11: info = db.def; break;
                case 12: info = db.mdef; break;
                case 13: info = db.critical_def; break;
                case 14: info = db.attrs[ATTR::STR]; break;
                case 15: info = db.attrs[ATTR::AGI]; break;
                case 16: info = db.attrs[ATTR::VIT]; break;
                case 17: info = db.attrs[ATTR::INT]; break;
                case 18: info = db.attrs[ATTR::DEX]; break;
                case 19: info = db.attrs[ATTR::LUK]; break;
                case 20: info = db.range2; break;
                case 21: info = db.range3; break;
                case 22: info = db.size; break;
                case 23: info = static_cast<int>(db.race); break;
                case 24: info = static_cast<int>(db.element.element); break;
                case 25: info = db.element.level; break;
                case 26: info = static_cast<int>(db.mode); break;
                case 27: info = db.speed.count(); break;
                case 28: info = db.adelay.count(); break;
                case 29: info = db.amotion.count(); break;
                case 30: info = db.dmotion.count(); break;
                case 31: info = db.mutations_nr; break;
                case 32: info = db.mutation_power; break;
                default:
                    lua_warn("mob.mobinfo: unknown request"_s);
                    mode = -1;
                    break;
            }
        }
    }
    if (mode == 1)
        push_string(L, mob_name);
    else if (mode == 2)
        push_string(L, drop_name);
    else if (mode == -1)
        luac::push_int(L, -1);
    else
        luac::push_int(L, info);
    return 1;
}

// mob.getmobdrops(species) -> table, status   Old: getmobdrops (the
// $@MobDrop_* result arrays become the returned 1-based sequence of
// {item=, name=, rate=}; status 0 unknown mob, 1 has drops, 2 no drops)
static
int lmob_getmobdrops(lua_State* L)
{
    Species mob_id = wrap<Species>(check_int(L, 1));
    bool known;
    {
        known = mobdb_checkid(mob_id) != Species();
    }
    lua_newtable(L);
    if (!known)
    {
        luac::push_int(L, 0);
        return 2;
    }
    int status = 1;
    int i = 0;
    for (; i < MaxDrops; ++i)
    {
        ItemNameId nameid;
        int rate = 0;
        ItemName name;
        {
            const mob_db_& db = get_mob_db(mob_id);
            nameid = db.dropitem[i].nameid;
            if (nameid)
            {
                rate = db.dropitem[i].p.num;
                name = itemdb_search(nameid)->name;
            }
        }
        if (!nameid)
        {
            if (i == 0)
                status = 2;
            break;
        }
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, unwrap<ItemNameId>(nameid));
        lua_setfield(L, -2, "item");
        push_string(L, name);
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, rate);
        lua_setfield(L, -2, "rate");
        lua_rawseti(L, -2, i + 1);
    }
    luac::push_int(L, status);
    return 2;
}

// mob.checkid(species) -> bool
static
int lmob_checkid(lua_State* L)
{
    Species mob_id = wrap<Species>(check_int(L, 1));
    bool r;
    {
        r = mobdb_checkid(mob_id) != Species();
    }
    lua_pushboolean(L, r);
    return 1;
}

// ------------------------------------------------------------------------

static
const luaL_Reg mob_funcs[] =
{
    {"monster", lmob_monster},
    {"areamonster", lmob_areamonster},
    {"summon", lmob_summon},
    {"killmonster", lmob_killmonster},
    {"mobcount", lmob_mobcount},
    {"mobinfo", lmob_mobinfo},
    {"getmobdrops", lmob_getmobdrops},
    {"checkid", lmob_checkid},
    {nullptr, nullptr},
};

void lua_register_lib_mob(lua_State* L, int env_idx)
{
    int env = lua_absindex(L, env_idx);
    lua_newtable(L);
    luac::register_funcs(L, -1, mob_funcs);
    lua_setfield(L, env, "mob");
}
} // namespace map
} // namespace tmwa
