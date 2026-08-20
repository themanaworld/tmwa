#include "lua-events.hpp"
//    lua-events.cpp - event dispatch, host hooks, command registry.
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
#include <list>
#include <map>
#include <vector>

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/vstring.hpp"

#include "../compat/fun.hpp"
#include "../compat/time_t.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "../high/core.hpp"

#include "globals.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "lua-callback.hpp"
#include "lua-dialog.hpp"
#include "lua-engine.hpp"
#include "lua-handles.hpp"
#include "lua-internal.hpp"
#include "lua-npc.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// ------------------------------------------------------------------------
// module state (doc/lua-engine.md section 2.3; engine-private, not in
// globals.hpp)

// broadcast/clock hook index, maintained by lua-npc.cpp registration/detach
static
std::vector<BlockId> g_hook_index[static_cast<int>(LuaHook::COUNT)];

// definition-order list of non-puppet script NPCs (on_init, label scans)
static
std::vector<BlockId> g_script_npcs;

// server.registercmd registry
static
std::map<RString, LuaCallback> g_commands;

// The mid-dialog event queue, keyed by the player's block id (design
// deviation, kept at integration: the design put this on
// map_session_data::eventqueuel; engine-side storage behaves identically
// and keeps map.hpp free of callback ownership).
static
std::map<uint32_t, std::list<LuaCallback>> g_event_queue;

// The 100 ms queue-replay timers (design deviation, kept at integration:
// the design ran these through the 32 player timer slots; a dedicated map
// avoids competing with p:addtimer slots and is cleared on detach).
struct LuaEventReplay
{
    Timer timer;
    LuaCallback cb;
    BlockId player;
};
static
std::map<int, LuaEventReplay> g_replays;
static
int g_replay_next_key = 0;

static
Timer g_clock_timer;
static
Timer g_mob_sweep_timer;

// ------------------------------------------------------------------------
// hook index / npc list maintenance (called from lua-npc.cpp)

void lua_hook_index_add(LuaHook hook, BlockId id)
{
    std::vector<BlockId>& v = g_hook_index[static_cast<int>(hook)];
    if (std::find(v.begin(), v.end(), id) == v.end())
        v.push_back(id);
}

void lua_hook_index_remove(BlockId id)
{
    for (std::vector<BlockId>& v : g_hook_index)
        v.erase(std::remove(v.begin(), v.end(), id), v.end());
}

void lua_events_npc_add(BlockId id)
{
    if (std::find(g_script_npcs.begin(), g_script_npcs.end(), id)
            == g_script_npcs.end())
        g_script_npcs.push_back(id);
}

void lua_events_npc_remove(BlockId id)
{
    g_script_npcs.erase(
            std::remove(g_script_npcs.begin(), g_script_npcs.end(), id),
            g_script_npcs.end());
}

// ------------------------------------------------------------------------
// args plumbing

// A handler's `args` value comes either from a C++ LuaArgs shape (host
// hooks) or from a Lua stack slot (the npc.event binding). Trivially
// destructible so it may live across raising calls.
struct EventArgs
{
    const LuaArgs* c = nullptr;
    lua_State* from = nullptr;
    int idx = 0;

    bool empty() const
    {
        if (c)
            return c->empty();
        if (from == nullptr || idx == 0)
            return true;
        return lua_isnoneornil(from, idx);
    }
};

static
void push_lua_args(lua_State* L, const LuaArgs& args)
{
    switch (args.kind)
    {
    case LuaArgs::Kind::NONE:
        lua_pushnil(L);
        return;
    case LuaArgs::Kind::TARGET_ID:
        lua_createtable(L, 0, 1);
        lua_pushinteger(L, args.i1);
        lua_setfield(L, -2, "target_id");
        return;
    case LuaArgs::Kind::KILL:
        lua_createtable(L, 0, 1);
        lua_pushinteger(L, args.i1);
        lua_setfield(L, -2, "victimrid");
        return;
    case LuaArgs::Kind::MOBKILL:
        lua_createtable(L, 0, 3);
        lua_pushinteger(L, args.i1);
        lua_setfield(L, -2, "mobID");
        lua_pushinteger(L, args.i2);
        lua_setfield(L, -2, "mobX");
        lua_pushinteger(L, args.i3);
        lua_setfield(L, -2, "mobY");
        return;
    case LuaArgs::Kind::ARGSTRING:
        // the argument string is the handler's args value itself
        luac::push_string(L, args.str);
        return;
    case LuaArgs::Kind::ITEM_USE:
        lua_createtable(L, 0, 1);
        lua_pushinteger(L, args.i1);
        lua_setfield(L, -2, "itemId");
        return;
    case LuaArgs::Kind::ITEM_EQUIP:
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, args.i1);
        lua_setfield(L, -2, "itemId");
        lua_pushinteger(L, args.i2);
        lua_setfield(L, -2, "slotId");
        return;
    }
    lua_pushnil(L);
}

static
void push_event_args(lua_State* L, const EventArgs& args)
{
    if (args.c)
    {
        push_lua_args(L, *args.c);
        return;
    }
    if (args.from == nullptr || args.idx == 0)
    {
        lua_pushnil(L);
        return;
    }
    // single interpreter state: args.from is always the same L
    lua_pushvalue(L, args.idx);
}

// ------------------------------------------------------------------------
// handler resolution and the two run helpers

// "" resolves the click body, anything else events[label]
// (doc/lua-engine.md 2.3: there is no ev_db, handlers are fields of the
// definition table; puppets resolve through the parent's shared table).
bool lua_npc_push_handler(lua_State* L, dumb_ptr<npc_data> nd, XString label)
{
    if (!lua_npc_push_def(L, nd))
        return false;
    luac::push_string(L, "events"_s);
    lua_rawget(L, -2);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 2);
        return false;
    }
    luac::push_string(L, label);
    lua_rawget(L, -2);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 3);
        return false;
    }
    lua_remove(L, -2);      // the events table
    lua_remove(L, -2);      // the definition table
    return true;
}

static
bool npc_handler_exists(lua_State* L, dumb_ptr<npc_data> nd, XString label)
{
    if (!lua_npc_push_handler(L, nd, label))
        return false;
    lua_pop(L, 1);
    return true;
}

bool lua_npc_has_handler(dumb_ptr<npc_data> nd, XString label)
{
    lua_State* L = lua_state();
    if (L == nullptr || nd == nullptr)
        return false;
    return npc_handler_exists(L, nd, label);
}

// -1: no handler; 0: handler ran and failed; 1: handler ran (or was ended
// by stop()).
static
int run_label_sync(dumb_ptr<npc_data> nd, XString label,
        dumb_ptr<map_session_data> sd, const EventArgs& args,
        const char* what)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return -1;
    if (!lua_npc_push_handler(L, nd, label))
        return -1;
    lua_push_npc_handle(L, nd);
    lua_push_player_handle(L, sd);      // nil for null
    push_event_args(L, args);
    LuaCtx ctx;
    ctx.npc = nd->bl_id;
    ctx.player = sd != nullptr ? sd->bl_id : BlockId();
    ctx.what = what;
    return lua_run_sync(ctx, 3) ? 1 : 0;
}

// Start a dialog coroutine for the handler. Returns false when there is no
// handler. Sets sd->npc_id (the caller verified sd is free).
static
bool run_label_dialog(dumb_ptr<npc_data> nd, XString label,
        dumb_ptr<map_session_data> sd, const EventArgs& args,
        const char* what)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return false;
    if (!lua_npc_push_handler(L, nd, label))
        return false;
    lua_push_npc_handle(L, nd);
    lua_push_player_handle(L, sd);
    push_event_args(L, args);
    LuaCtx ctx;
    ctx.npc = nd->bl_id;
    ctx.player = sd->bl_id;
    ctx.what = what;
    sd->npc_id = nd->bl_id;
    lua_run_dialog(sd, nd, ctx, 3);
    return true;
}

// The npc_click script path (npc.cpp): start the dialog coroutine on the
// NPC's click body. The caller verified sd is free and the NPC is clickable.
bool lua_npc_click(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd)
{
    if (sd == nullptr || nd == nullptr)
        return false;
    EventArgs ea;
    LuaArgs none = LuaArgs::none();
    ea.c = &none;
    return run_label_dialog(nd, ""_s, sd, ea, "click");
}

// ------------------------------------------------------------------------
// the event queue (engine-side storage, see lua-events.hpp)

static
void queue_push(dumb_ptr<map_session_data> sd, LuaCallback cb)
{
    g_event_queue[unwrap<BlockId>(sd->bl_id)].push_back(cb);
}

static
void replay_fire(TimerData*, tick_t, int key)
{
    auto it = g_replays.find(key);
    if (it == g_replays.end())
        return;
    LuaCallback cb = it->second.cb;
    it->second.cb = LuaCallback();
    BlockId player = it->second.player;
    g_replays.erase(it);
    dumb_ptr<map_session_data> sd = map_id_is_player(player);
    if (sd == nullptr)
    {
        lua_cb_release(cb);
        return;
    }
    // through the normal event path: the queue rules apply again, exactly
    // like the old pc_eventtimer -> npc_event replay
    lua_cb_fire(cb, sd, LuaArgs::none());
}

bool lua_event_dequeue(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return false;
    auto qit = g_event_queue.find(unwrap<BlockId>(sd->bl_id));
    if (qit == g_event_queue.end() || qit->second.empty())
        return false;
    LuaCallback cb = qit->second.front();
    qit->second.pop_front();
    if (qit->second.empty())
        g_event_queue.erase(qit);
    int key = ++g_replay_next_key;
    LuaEventReplay& r = g_replays[key];
    r.cb = cb;
    r.player = sd->bl_id;
    r.timer = Timer(gettick() + 100_ms,
            std::bind(replay_fire, ph::_1, ph::_2, key));
    return true;
}

void lua_event_queue_clear(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    auto qit = g_event_queue.find(unwrap<BlockId>(sd->bl_id));
    if (qit != g_event_queue.end())
    {
        for (LuaCallback& cb : qit->second)
            lua_cb_release(cb);
        g_event_queue.erase(qit);
    }
    for (auto it = g_replays.begin(); it != g_replays.end();)
    {
        if (it->second.player == sd->bl_id)
        {
            lua_cb_release(it->second.cb);
            it = g_replays.erase(it);
        }
        else
            ++it;
    }
}

// ------------------------------------------------------------------------
// broadcast

static
bool fixed_label_hook(XString label, LuaHook* out)
{
    if (label == "OnPCLoginEvent"_s)
        *out = LuaHook::LOGIN;
    else if (label == "OnPCLogoutEvent"_s)
        *out = LuaHook::LOGOUT;
    else if (label == "OnPCDieEvent"_s)
        *out = LuaHook::DIE;
    else if (label == "OnPCKillEvent"_s)
        *out = LuaHook::KILL;
    else if (label == "OnMobKillEvent"_s)
        *out = LuaHook::MOBKILL;
    else
        return false;
    return true;
}

// Old doall semantics (npc.cpp:284-291): puppets skipped, disabled flag
// ignored, npc_id untouched, one failing NPC does not stop the others.
static
int broadcast_core(ScriptLabel label, dumb_ptr<map_session_data> sd,
        const EventArgs& args)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return 0;
    int c = 0;
    LuaHook hook;
    // copies: a handler may register or destroy NPCs while we iterate
    std::vector<BlockId> ids;
    if (fixed_label_hook(XString(label), &hook))
        ids = g_hook_index[static_cast<int>(hook)];
    else
        ids = g_script_npcs;
    for (BlockId id : ids)
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(id);
        if (nd == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
            continue;
        dumb_ptr<npc_data_script> nds = nd->is_script();
        if (nds != nullptr && nds->scr.parent)
            continue;   // puppets only respond to direct events
        if (run_label_sync(nd, XString(label), sd, args, "broadcast") != -1)
            ++c;
    }
    return c;
}

int lua_npc_broadcast(ScriptLabel label, dumb_ptr<map_session_data> sd,
        LuaArgs args)
{
    EventArgs ea;
    ea.c = &args;
    return broadcast_core(label, sd, ea);
}

// ------------------------------------------------------------------------
// the central targeted dispatcher (doc/lua-engine.md section 5, preserving
// npc.cpp:546-644)

static
bool npc_event_core(dumb_ptr<map_session_data> sd, NpcEvent ev,
        const EventArgs& args, bool force_inline)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return false;
    NpcName evnpc = ev.npc;
    ScriptLabel evlabel = ev.label;

    // 1. empty npc: broadcast by label
    if (!evnpc)
        return broadcast_core(evlabel, sd, args) > 0;

    // 2. phony tags (mob spawn identity); the synthetic ~lua#<n> keys are
    // handled by lua_hook_mob_death, which has the mob and can look the
    // function up by block id
    if (evnpc.front() == '~')
        return false;

    bool is_touch = (evlabel == stringish<ScriptLabel>("OnTouch"_s));

    // 3. resolve the NPC
    dumb_ptr<npc_data> nd = npc_name2id(evnpc);
    if (nd == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
    {
        if (!is_touch)
            lua_warn(STRPRINTF("event not found [%s]"_fmt, ev));
        return false;
    }

    // 5. orphan puppet GC
    dumb_ptr<npc_data_script> nds = nd->is_script();
    if (nds != nullptr && nds->scr.parent
            && map_id2bl(nds->scr.parent) == nullptr)
    {
        npc_free(nd);
        return false;
    }

    // 4. resolve the handler ("" = click body, else events[label])
    if (!npc_handler_exists(L, nd, XString(evlabel)))
    {
        if (!is_touch)
            lua_warn(STRPRINTF("event not found [%s]"_fmt, ev));
        return false;
    }

    if (sd != nullptr)
    {
        // 6a. map-placed non-puppet NPCs only hear players on the same map
        // and inside the xs/ys box (npc.cpp:609-621)
        if (nds != nullptr && nds->scr.event_needs_map)
        {
            int xs = nds->scr.xs;
            int ys = nds->scr.ys;
            if (nd->bl_m != sd->bl_m)
                return false;
            if (xs > 0
                    && (sd->bl_x < nd->bl_x - xs / 2
                        || nd->bl_x + xs / 2 < sd->bl_x))
                return false;
            if (ys > 0
                    && (sd->bl_y < nd->bl_y - ys / 2
                        || nd->bl_y + ys / 2 < sd->bl_y))
                return false;
        }

        // 6b. self-heal a dialog with a freed NPC
        if (sd->npc_id && map_id_is_npc(sd->npc_id) == nullptr)
            npc_event_dequeue(sd);

        // 6c. the queue rule: mid-dialog and argument-less -> defer
        if (sd->npc_id && sd->lua.thread_ref != lua_noref
                && args.empty() && !force_inline)
        {
            queue_push(sd, lua_cb_named(ev));
            return true;   // queued
        }

        // 6d. disabled NPC with a player drops the event
        if (nd->flag & 1)
        {
            npc_event_dequeue(sd);
            return false;
        }

        // 6e. free player: dialog coroutine. Mid-dialog with args (or
        // force_inline): nested synchronous run, npc_id untouched (the old
        // engine clobbered npc_id here; deliberate fix, the observable
        // behaviour is the same because the handler cannot prompt).
        if (sd->lua.thread_ref == lua_noref)
            return run_label_dialog(nd, XString(evlabel), sd, args,
                    "npc event");
        return run_label_sync(nd, XString(evlabel), sd, args,
                "npc event") != -1;
    }

    // 7. no player: synchronous, disabled flag ignored (old donpcevent)
    return run_label_sync(nd, XString(evlabel), nullptr, args,
            "npc event") != -1;
}

bool lua_npc_event(dumb_ptr<map_session_data> sd, NpcEvent ev, LuaArgs args,
        bool force_inline)
{
    EventArgs ea;
    ea.c = &args;
    return npc_event_core(sd, ev, ea, force_inline);
}

bool lua_npc_event_from_stack(lua_State* L, dumb_ptr<map_session_data> sd,
        NpcEvent ev, int args_idx, bool force_inline)
{
    EventArgs ea;
    ea.from = L;
    ea.idx = args_idx;
    return npc_event_core(sd, ev, ea, force_inline);
}

// ------------------------------------------------------------------------
// function-form callbacks

void lua_fire_fn_ref(int fn_ref, BlockId self_npc,
        dumb_ptr<map_session_data> sd, LuaArgs args)
{
    lua_State* L = lua_state();
    if (fn_ref == lua_noref)
        return;
    if (L == nullptr)
        return;
    if (sd != nullptr)
    {
        if (sd->npc_id && map_id_is_npc(sd->npc_id) == nullptr)
            npc_event_dequeue(sd);
        // the queue rule applies to function callbacks too
        if (sd->npc_id && sd->lua.thread_ref != lua_noref && args.empty())
        {
            LuaCallback cb;
            cb.fn_ref = fn_ref;   // the queue takes over the reference
            cb.self_npc = self_npc;
            queue_push(sd, cb);
            return;
        }
    }
    dumb_ptr<npc_data> self_nd = map_id_is_npc(self_npc);
    luac::push_ref(L, fn_ref);
    // the stack slot anchors the function; drop the registry reference now
    // so every exit path below is balanced
    lua_cb_release_ref(L, fn_ref);
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        return;
    }
    lua_push_npc_handle(L, self_nd);    // nil when the NPC is gone
    lua_push_player_handle(L, sd);
    push_lua_args(L, args);
    LuaCtx ctx;
    ctx.npc = self_nd != nullptr ? self_nd->bl_id : BlockId();
    ctx.player = sd != nullptr ? sd->bl_id : BlockId();
    ctx.what = "callback";
    if (sd != nullptr && sd->lua.thread_ref == lua_noref)
    {
        // dialog-capable: anchor the dialog on the callback's NPC, falling
        // back to the engine-owned #itemdialog NPC when it has none (e.g.
        // p:addtimer functions registered from item scripts)
        dumb_ptr<npc_data> dialog_npc = self_nd;
        if (dialog_npc == nullptr)
            dialog_npc = lua_item_dialog_npc();
        if (dialog_npc != nullptr)
        {
            ctx.npc = dialog_npc->bl_id;
            sd->npc_id = dialog_npc->bl_id;
            lua_run_dialog(sd, dialog_npc, ctx, 3);
            return;
        }
    }
    lua_run_sync(ctx, 3);
}

// ------------------------------------------------------------------------
// host hooks

void lua_hook_login(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    lua_npc_broadcast(stringish<ScriptLabel>("OnPCLoginEvent"_s), sd,
            LuaArgs::none());
}

void lua_hook_logout(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    lua_npc_broadcast(stringish<ScriptLabel>("OnPCLogoutEvent"_s), sd,
            LuaArgs::none());
}

void lua_hook_die(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    lua_npc_broadcast(stringish<ScriptLabel>("OnPCDieEvent"_s), sd,
            LuaArgs::none());
}

void lua_hook_kill(dumb_ptr<map_session_data> killer,
        dumb_ptr<map_session_data> victim)
{
    if (killer == nullptr || victim == nullptr)
        return;
    lua_npc_broadcast(stringish<ScriptLabel>("OnPCKillEvent"_s), killer,
            LuaArgs::kill(victim->bl_id));
}

void lua_hook_mobkill(dumb_ptr<map_session_data> killer, Species mob_class,
        int x, int y)
{
    if (killer == nullptr)
        return;
    lua_npc_broadcast(stringish<ScriptLabel>("OnMobKillEvent"_s), killer,
            LuaArgs::mobkill(
                static_cast<int>(unwrap<Species>(mob_class)), x, y));
}

void lua_hook_mob_death(dumb_ptr<mob_data> md,
        dumb_ptr<map_session_data> killer)
{
    // no event fires when the killer is not a player (as today)
    if (md == nullptr || killer == nullptr)
        return;
    NpcEvent ev = md->npc_event;
    if (!bool(ev))
        return;
    NpcName evnpc = ev.npc;
    if (evnpc && evnpc.front() == '~')
    {
        // function form: mob.monster stored a synthetic ~lua#<n> tag on the
        // mob and the function in MOB_DEATH_FNS[bl_id] (a function, or a
        // {fn=..., self=<npc id>} pair)
        if (!evnpc.startswith("~lua#"_s))
            return;   // plain phony tag: identity only, nothing fires
        lua_State* L = lua_state();
        if (L == nullptr)
            return;
        LuaCallback cb;
        lua_push_engine_table(L, LuaTable::MOB_DEATH_FNS);
        lua_pushinteger(L, unwrap<BlockId>(md->bl_id));
        lua_rawget(L, -2);
        if (lua_isfunction(L, -1))
        {
            cb = lua_cb_from_stack(L, -1);
        }
        else if (lua_istable(L, -1))
        {
            luac::push_string(L, "fn"_s);
            lua_rawget(L, -2);
            if (lua_isfunction(L, -1))
                cb = lua_cb_from_stack(L, -1);
            lua_pop(L, 1);
            luac::push_string(L, "self"_s);
            lua_rawget(L, -2);
            if (lua_type(L, -1) == LUA_TNUMBER)
                cb.self_npc = wrap<BlockId>(
                        static_cast<uint32_t>(lua_tointeger(L, -1)));
            lua_pop(L, 1);
        }
        lua_pop(L, 2);   // the value and the table
        lua_mob_forget(md->bl_id);
        if (cb.fn_ref != lua_noref)
            // dialog path with the killer; queued if mid-dialog (the mob
            // info does not count as args, matching the old engine)
            lua_cb_fire(cb, killer, LuaArgs::none());
        return;
    }
    // named per-spawn event: dialog path, queued if mid-dialog
    lua_npc_event(killer, ev, LuaArgs::none());
}

void lua_mob_forget(BlockId id)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return;
    lua_push_engine_table(L, LuaTable::MOB_DEATH_FNS);
    lua_pushinteger(L, unwrap<BlockId>(id));
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
}

// the MOB_DEATH_FNS table is additionally swept every 10 minutes for ids of
// mobs that died without a player killer or were deleted without the
// mob_delete hook
static
void mob_sweep_tick(TimerData*, tick_t)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return;
    std::vector<int> dead;
    lua_push_engine_table(L, LuaTable::MOB_DEATH_FNS);
    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        lua_pop(L, 1);   // the value
        if (luac::is_integer_value(L, -1))
        {
            int id = static_cast<int>(lua_tointeger(L, -1));
            if (id > 0 && map_id2bl(
                        wrap<BlockId>(static_cast<uint32_t>(id))) == nullptr)
                dead.push_back(id);
        }
    }
    for (int id : dead)
    {
        lua_pushinteger(L, id);
        lua_pushnil(L);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

// ------------------------------------------------------------------------
// clock (npc.cpp:298-343 semantics: UTC, ev_tm_b change detection, first
// tick fires everything because ev_tm_b starts at -1)

static
void clock_broadcast(XString label)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return;
    std::vector<BlockId> ids =
        g_hook_index[static_cast<int>(LuaHook::CLOCK)];
    for (BlockId id : ids)
    {
        dumb_ptr<npc_data> nd = map_id_is_npc(id);
        if (nd == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
            continue;
        dumb_ptr<npc_data_script> nds = nd->is_script();
        if (nds != nullptr && nds->scr.parent)
            continue;
        if (!npc_handler_exists(L, nd, label))
            continue;
        EventArgs none;
        run_label_sync(nd, label, nullptr, none, "clock");
    }
}

void lua_hook_clock(const struct tm& now)
{
    ScriptLabel buf;
    if (now.tm_min != ev_tm_b.tm_min)
    {
        SNPRINTF(buf, 24, "OnMinute%02d"_fmt, now.tm_min);
        clock_broadcast(buf);
        SNPRINTF(buf, 24, "OnClock%02d%02d"_fmt, now.tm_hour, now.tm_min);
        clock_broadcast(buf);
    }
    if (now.tm_hour != ev_tm_b.tm_hour)
    {
        SNPRINTF(buf, 24, "OnHour%02d"_fmt, now.tm_hour);
        clock_broadcast(buf);
    }
    if (now.tm_mday != ev_tm_b.tm_mday)
    {
        SNPRINTF(buf, 24, "OnDay%02d%02d"_fmt, now.tm_mon + 1, now.tm_mday);
        clock_broadcast(buf);
    }
    ev_tm_b = now;
}

static
void clock_tick(TimerData*, tick_t)
{
    struct tm t = TimeT::now();
    lua_hook_clock(t);
}

// ------------------------------------------------------------------------
// on_init

void lua_run_oninit()
{
    lua_State* L = lua_state();
    if (L != nullptr && !lua_check_skip_oninit())
    {
        int c = 0;
        std::vector<BlockId> ids = g_script_npcs;   // definition order
        for (BlockId id : ids)
        {
            dumb_ptr<npc_data> nd = map_id_is_npc(id);
            if (nd == nullptr)
                continue;
            dumb_ptr<npc_data_script> nds = nd->is_script();
            if (nds != nullptr && nds->scr.parent)
                continue;
            EventArgs none;
            int r = run_label_sync(nd, "OnInit"_s, nullptr, none, "on_init");
            if (r == 0)
                runflag = false;   // load-time errors are fatal (D1)
            if (r == 1)
                ++c;
        }
        PRINTF("lua: OnInit done (%d npc)\n"_fmt, c);
    }
    // after this, new-global writes warn (error under --check-scripts)
    lua_loading_done();
    if (!lua_check_only())
    {
        // the 1 s clock (first tick after 100 ms fires everything) and the
        // 10 minute mob-death-handler sweep
        g_clock_timer = Timer(gettick() + 100_ms, clock_tick, 1_s);
        g_mob_sweep_timer = Timer(gettick() + 10_min, mob_sweep_tick, 10_min);
    }
}

// ------------------------------------------------------------------------
// command registry (server.registercmd / magic_message)

void lua_register_command(RString word, LuaCallback cb)
{
    auto it = g_commands.find(word);
    if (it != g_commands.end())
    {
        lua_cb_release(it->second);
        it->second = cb;
        return;
    }
    g_commands.insert(std::make_pair(word, cb));
}

bool lua_command_dispatch(dumb_ptr<map_session_data> sd, XString word,
        ZString rest)
{
    if (sd == nullptr)
        return false;
    auto it = g_commands.find(RString(word));
    if (it == g_commands.end())
        return false;
    lua_State* L = lua_state();
    if (L == nullptr)
        return false;
    // transient copy: the registry keeps the reference, we only borrow it
    LuaCallback cb = it->second;

    if (cb.fn_ref != lua_noref)
    {
        // function form: (p, argstring), dialog-capable (D6)
        if (sd->npc_id)
            npc_event_dequeue(sd);   // abandon-then-run
        luac::push_ref(L, cb.fn_ref);
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return true;
        }
        lua_push_player_handle(L, sd);
        luac::push_string(L, rest);
        dumb_ptr<npc_data> dialog_npc = map_id_is_npc(cb.self_npc);
        if (dialog_npc == nullptr)
            dialog_npc = lua_item_dialog_npc();
        LuaCtx ctx;
        ctx.player = sd->bl_id;
        ctx.what = "command";
        if (dialog_npc != nullptr)
        {
            ctx.npc = dialog_npc->bl_id;
            sd->npc_id = dialog_npc->bl_id;
            lua_run_dialog(sd, dialog_npc, ctx, 2);
        }
        else
            lua_run_sync(ctx, 2);
        return true;
    }

    // event form: "Npc::OnX" or a plain NPC name (the click body). The old
    // magic_message consumed the chat word only when the NPC exists
    // (npc.cpp:199-223); keep that.
    NpcEvent ev = cb.event;
    NpcName evnpc = ev.npc;
    if (!evnpc || npc_name2id(evnpc) == nullptr)
        return false;
    if (sd->npc_id)
        npc_event_dequeue(sd);   // abandon-then-run (doc/lua-engine.md 5)
    LuaArgs args = LuaArgs::argstring(RString(XString(rest)));
    lua_npc_event(sd, ev, args);
    return true;
}

// ------------------------------------------------------------------------
// overrideattack

void lua_fire_attack_spell(dumb_ptr<map_session_data> sd, BlockId target_id)
{
    if (sd == nullptr)
        return;
    if (!bool(sd->magic_attack))
        return;
    // function form: run synchronously without consuming the stored
    // reference (charges fire it repeatedly; release points are the
    // overrideattack replace/discharge, death reset, and map_quit)
    if (sd->magic_attack.fn_ref != lua_noref)
    {
        lua_State* L = lua_state();
        if (L == nullptr)
            return;
        dumb_ptr<npc_data> self_nd = map_id_is_npc(sd->magic_attack.self_npc);
        luac::push_ref(L, sd->magic_attack.fn_ref);
        if (!lua_isfunction(L, -1))
        {
            lua_pop(L, 1);
            return;
        }
        lua_push_npc_handle(L, self_nd);
        lua_push_player_handle(L, sd);
        LuaArgs fargs = LuaArgs::target(target_id);
        push_lua_args(L, fargs);
        LuaCtx fctx;
        fctx.npc = self_nd != nullptr ? self_nd->bl_id : BlockId();
        fctx.player = sd->bl_id;
        fctx.what = "attack spell";
        lua_run_sync(fctx, 3);
        return;
    }
    NpcEvent ev = sd->magic_attack.event;
    NpcName evnpc = ev.npc;
    if (!evnpc || evnpc.front() == '~')
        return;
    dumb_ptr<npc_data> nd = npc_name2id(evnpc);
    if (nd == nullptr || nd->deletion_pending != npc_data::NOT_DELETING)
        return;
    dumb_ptr<npc_data_script> nds = nd->is_script();
    if (nds != nullptr && nds->scr.parent
            && map_id2bl(nds->scr.parent) == nullptr)
    {
        npc_free(nd);
        return;
    }
    if (nds != nullptr && nds->scr.event_needs_map)
    {
        int xs = nds->scr.xs;
        int ys = nds->scr.ys;
        if (nd->bl_m != sd->bl_m)
            return;
        if (xs > 0
                && (sd->bl_x < nd->bl_x - xs / 2
                    || nd->bl_x + xs / 2 < sd->bl_x))
            return;
        if (ys > 0
                && (sd->bl_y < nd->bl_y - ys / 2
                    || nd->bl_y + ys / 2 < sd->bl_y))
            return;
    }
    if (nd->flag & 1)
        return;
    // synchronous with the player attached, npc_id untouched (doc section 5
    // entry table: overrideattack handlers never queue and never own a
    // dialog)
    LuaArgs args = LuaArgs::target(target_id);
    EventArgs ea;
    ea.c = &args;
    run_label_sync(nd, XString(ev.label), sd, ea, "attack spell");
}

// ------------------------------------------------------------------------
// shutdown

void lua_events_final()
{
    for (auto& pair : g_commands)
        lua_cb_release(pair.second);
    g_commands.clear();
    for (auto& pair : g_event_queue)
        for (LuaCallback& cb : pair.second)
            lua_cb_release(cb);
    g_event_queue.clear();
    for (auto& pair : g_replays)
        lua_cb_release(pair.second.cb);
    g_replays.clear();
    for (std::vector<BlockId>& v : g_hook_index)
        v.clear();
    g_script_npcs.clear();
    g_clock_timer = Timer();
    g_mob_sweep_timer = Timer();
    lua_npc_final();
}
} // namespace map
} // namespace tmwa
