#include "lua-npc.hpp"
//    lua-timers.cpp - NPC OnTimer machinery and event-timer slot handling.
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

#include <cassert>

#include <algorithm>
#include <map>
#include <vector>

#include "../compat/fun.hpp"

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"

#include "../generic/array.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "map.hpp"
#include "npc.hpp"
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
// This TU has no header of its own: the machine and NPC-slot functions are
// declared in npc.hpp, the player-slot functions in pc.hpp. Exported
// surface:
//   npc_timerevent_start/stop(nd)         the old OnTimer machine
//   npc_gettimerevent_tick(nd)
//   npc_settimerevent_tick(nd, t)
//   lua_npc_timer_setup(nd, intervals)    seed the machine (npc.script)
//   lua_pc_addeventtimer(sd, tick, cb)    player one-shot slot (p:addtimer,
//                                         map.areatimer, queue replay)
//   lua_pc_cleareventtimer(sd)            release all player slots (logout)
//   lua_npc_addeventtimer(nd, tick, cb)   NPC one-shot slot (addnpctimer)
//   lua_npc_cleareventtimer(nd)           release all NPC slots
//   lua_npc_timer_detach(nd)              full timer cleanup for
//                                         lua_npc_detach / npc_free
//
// DESIGN DEVIATION (kept at integration): doc/lua-engine.md 2.2 renames the
// interval list to scr.timer_intervals and puts the LuaTimerSlot arrays on
// map_session_data / npc_data. The machine instead keeps the old
// scr.timer_eventv / scr.next_event fields (npc_timerevent_list with the
// bytecode pos unused), which keeps the state machine byte-for-byte, and
// the slot arrays live in the side tables below. Behaviour is exactly the
// design's; only the storage location differs.

// ------------------------------------------------------------------------
// LuaTimerSlot storage (see the deviation note above)

static
std::map<uint32_t, Array<LuaTimerSlot, MAX_EVENTTIMER>> g_pc_slots;
static
std::map<uint32_t, Array<LuaTimerSlot, MAX_EVENTTIMER>> g_npc_slots;

// ------------------------------------------------------------------------
// NPC OnTimer machine, kept verbatim from npc.cpp:407-544 (commit f5c87302),
// including the initnpctimer no-op quirk while the timer is still counting
// toward its first label. Only the handler invocation changed: the old
// run_script(te->pos) became lua_run_sync of the "OnTimer<ms>" handler.

static
void npc_timerevent(TimerData*, tick_t tick, BlockId id, interval_t data);

// Run the OnTimer handler for one interval: resolve "OnTimer<ms>" against
// the NPC's definition table (lua-npc.cpp mirrors on_timer[ms] into
// events["OnTimer<ms>"], which also keeps the old string forms working) and
// run it synchronously without a player, as the old engine did (rid 0).
static
void lua_npc_run_timer_handler(dumb_ptr<npc_data_script> nd, interval_t when)
{
    lua_State* L = lua_state();
    if (L == nullptr)
        return;
    AString label = STRPRINTF("OnTimer%d"_fmt,
            static_cast<int>(when.count()));
    if (!lua_npc_push_handler(L, nd, label))
        return;
    lua_push_npc_handle(L, nd);     // self
    lua_pushnil(L);                 // p
    lua_pushnil(L);                 // args
    LuaCtx ctx;
    ctx.npc = nd->bl_id;
    ctx.player = BlockId();
    ctx.what = "on_timer";
    lua_run_sync(ctx, 3);
}

/// Callback for npc OnTimer*: labels.
/// This will be called later if you call npc_timerevent_start.
/// This function may only expire, but not deactivate, the counter.
static
void npc_timerevent(TimerData*, tick_t tick, BlockId id, interval_t data)
{
    dumb_ptr<block_list> bl = map_id2bl(id);
    if (bl == nullptr)
        return;
    dumb_ptr<npc_data_script> nd = bl->is_npc()->is_script();
    assert (nd != nullptr);
    assert (nd->npc_subtype == NpcSubtype::SCRIPT);
    assert (nd->scr.next_event != nd->scr.timer_eventv.end());

    if (nd->scr.parent && map_id2bl(nd->scr.parent) == nullptr)
    {
        npc_free(nd);
        return;
    }

    nd->scr.timertick = tick;
    const auto te = nd->scr.next_event;

    interval_t t = nd->scr.timer += data;
    assert (t == te->timer);
    ++nd->scr.next_event;
    if (nd->scr.next_event != nd->scr.timer_eventv.end())
    {
        interval_t next = nd->scr.next_event->timer - t;
        nd->scr.timerid = Timer(tick + next,
                std::bind(npc_timerevent, ph::_1, ph::_2,
                    id, next));
    }

    // the next interval is scheduled BEFORE the handler runs, so a
    // stopnpctimer inside the handler cancels the chain, as today
    lua_npc_run_timer_handler(nd, te->timer);
}

/// Start (or resume) counting ticks to the next npc_timerevent.
/// If the tick is already high enough, just set it to expired.
void npc_timerevent_start(dumb_ptr<npc_data_script> nd)
{
    if (nd == nullptr)
        return;

    if (nd->scr.timer_active)
        return;
    nd->scr.timer_active = true;

    if (nd->scr.timer_eventv.empty())
        return;
    if (nd->scr.timer == nd->scr.timer_eventv.back().timer)
        return;
    assert (nd->scr.timer < nd->scr.timer_eventv.back().timer);

    nd->scr.timertick = gettick();

    auto jt = nd->scr.next_event;
    assert (jt != nd->scr.timer_eventv.end());

    interval_t next = jt->timer - nd->scr.timer;
    nd->scr.timerid = Timer(gettick() + next,
            std::bind(npc_timerevent, ph::_1, ph::_2,
                nd->bl_id, next));
}

/// Stop the tick counter.
/// If the count was expired, just deactivate it.
void npc_timerevent_stop(dumb_ptr<npc_data_script> nd)
{
    if (nd == nullptr)
        return;

    if (!nd->scr.timer_active)
        return;
    nd->scr.timer_active = false;

    if (nd->scr.timerid)
    {
        nd->scr.timer += gettick() - nd->scr.timertick;
        nd->scr.timerid.cancel();
    }
}

/// Get the number of ticks on the counter.
/// If there is an actual timer running, this involves math.
interval_t npc_gettimerevent_tick(dumb_ptr<npc_data_script> nd)
{
    if (nd == nullptr)
        return interval_t::zero();

    interval_t tick = nd->scr.timer;

    if (nd->scr.timerid)
        tick += gettick() - nd->scr.timertick;
    return tick;
}

/// Helper method to update the "next event" iterator.
/// Note that now the iterator is always valid unless it is at the end.
/// Previously, it was invalid when the counter was deactivated.
static
void npc_timerevent_calc_next(dumb_ptr<npc_data_script> nd)
{
    npc_timerevent_list phony {};
    phony.timer = nd->scr.timer;

    // find the first element such that el.timer > phony.timer;
    auto jt = std::upper_bound(nd->scr.timer_eventv.begin(),
            nd->scr.timer_eventv.end(), phony,
            [](const npc_timerevent_list& l, const npc_timerevent_list& r)
            {
                return l.timer < r.timer;
            }
    );
    nd->scr.next_event = jt;
}

/// Set the tick counter.
/// If the timer was active, this means stopping and restarting the timer.
/// Note: active includes expired.
void npc_settimerevent_tick(dumb_ptr<npc_data_script> nd,
        interval_t newtimer)
{
    if (nd == nullptr)
        return;

    if (nd->scr.timer_eventv.empty())
        return;
    if (newtimer > nd->scr.timer_eventv.back().timer)
        newtimer = nd->scr.timer_eventv.back().timer;
    if (newtimer < interval_t::zero())
        newtimer = interval_t::zero();
    if (newtimer == nd->scr.timer)
        return;

    bool flag = nd->scr.timer_active;

    if (flag)
        npc_timerevent_stop(nd);
    nd->scr.timer = newtimer;
    npc_timerevent_calc_next(nd);
    if (flag)
        npc_timerevent_start(nd);
}

/// Seed the OnTimer machine from the sorted on_timer interval keys computed
/// by npc.script (puppets: the parent's list).
void lua_npc_timer_setup(dumb_ptr<npc_data_script> nd,
        std::vector<interval_t> intervals)
{
    if (nd == nullptr)
        return;
    std::sort(intervals.begin(), intervals.end());
    intervals.erase(std::unique(intervals.begin(), intervals.end()),
            intervals.end());
    nd->scr.timerid.cancel();
    nd->scr.timer_eventv.clear();
    for (interval_t t : intervals)
    {
        npc_timerevent_list e {};
        e.timer = t;
        e.pos = 0;      // transitional storage only; no bytecode
        nd->scr.timer_eventv.push_back(e);
    }
    nd->scr.timer = interval_t::zero();
    nd->scr.next_event = nd->scr.timer_eventv.begin();
    nd->scr.timer_active = false;
    nd->scr.timertick = tick_t();
}

// ------------------------------------------------------------------------
// one-shot event-timer slots (players and NPCs), doc/lua-engine.md section 6

static
void lua_pc_eventtimer(TimerData*, tick_t, BlockId id, int slot)
{
    auto it = g_pc_slots.find(unwrap<BlockId>(id));
    if (it == g_pc_slots.end())
        return;
    LuaCallback cb = it->second[slot].cb;
    it->second[slot].cb = LuaCallback();
    dumb_ptr<map_session_data> sd = map_id2sd(id);
    if (sd == nullptr)
    {
        lua_cb_release(cb);
        return;
    }
    // through the normal event path: the mid-dialog queue rule applies
    lua_cb_fire(cb, sd, LuaArgs::none());
}

/// Add a player one-shot timer (p:addtimer, map.areatimer, queue replay).
/// Takes ownership of cb; a full slot table releases it and logs (old:
/// silent drop). Returns 1 on success, 0 when dropped.
int lua_pc_addeventtimer(dumb_ptr<map_session_data> sd, interval_t tick,
        LuaCallback cb)
{
    if (sd == nullptr)
    {
        lua_cb_release(cb);
        return 0;
    }
    Array<LuaTimerSlot, MAX_EVENTTIMER>& slots =
            g_pc_slots[unwrap<BlockId>(sd->bl_id)];
    int i;
    for (i = 0; i < MAX_EVENTTIMER; i++)
        if (!slots[i].timer)
            break;
    if (i == MAX_EVENTTIMER)
    {
        lua_warn(STRPRINTF("addtimer: all %d timer slots busy for player %d (timer dropped)"_fmt,
                    MAX_EVENTTIMER, unwrap<BlockId>(sd->bl_id)));
        lua_cb_release(cb);
        return 0;
    }
    lua_cb_release(slots[i].cb);    // defensive; fired slots are empty
    slots[i].cb = cb;
    slots[i].timer = Timer(gettick() + tick,
            std::bind(lua_pc_eventtimer, ph::_1, ph::_2,
                sd->bl_id, i));
    return 1;
}

/// Cancel every player one-shot timer and release the callbacks (logout).
void lua_pc_cleareventtimer(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    auto it = g_pc_slots.find(unwrap<BlockId>(sd->bl_id));
    if (it == g_pc_slots.end())
        return;
    for (int i = 0; i < MAX_EVENTTIMER; i++)
    {
        it->second[i].timer.cancel();
        lua_cb_release(it->second[i].cb);
    }
    g_pc_slots.erase(it);
}

static
void lua_npc_eventtimer(TimerData*, tick_t, BlockId id, int slot)
{
    auto it = g_npc_slots.find(unwrap<BlockId>(id));
    if (it == g_npc_slots.end())
        return;
    LuaCallback cb = it->second[slot].cb;
    it->second[slot].cb = LuaCallback();
    dumb_ptr<npc_data> nd = map_id_is_npc(id);
    if (nd == nullptr)
    {
        lua_cb_release(cb);
        return;
    }
    // no player: lua_cb_fire runs the handler synchronously
    lua_cb_fire(cb, nullptr, LuaArgs::none());
}

/// Add an NPC one-shot timer (self:addnpctimer). Takes ownership of cb.
int lua_npc_addeventtimer(dumb_ptr<npc_data> nd, interval_t tick,
        LuaCallback cb)
{
    if (nd == nullptr)
    {
        lua_cb_release(cb);
        return 0;
    }
    Array<LuaTimerSlot, MAX_EVENTTIMER>& slots =
            g_npc_slots[unwrap<BlockId>(nd->bl_id)];
    int i;
    for (i = 0; i < MAX_EVENTTIMER; i++)
        if (!slots[i].timer)
            break;
    if (i == MAX_EVENTTIMER)
    {
        lua_warn(STRPRINTF("addnpctimer: all %d timer slots busy for npc %d (timer dropped)"_fmt,
                    MAX_EVENTTIMER, unwrap<BlockId>(nd->bl_id)));
        lua_cb_release(cb);
        return 0;
    }
    lua_cb_release(slots[i].cb);    // defensive; fired slots are empty
    slots[i].cb = cb;
    slots[i].timer = Timer(gettick() + tick,
            std::bind(lua_npc_eventtimer, ph::_1, ph::_2,
                nd->bl_id, i));
    return 1;
}

/// Cancel every NPC one-shot timer and release the callbacks.
void lua_npc_cleareventtimer(dumb_ptr<npc_data> nd)
{
    if (nd == nullptr)
        return;
    auto it = g_npc_slots.find(unwrap<BlockId>(nd->bl_id));
    if (it == g_npc_slots.end())
        return;
    for (int i = 0; i < MAX_EVENTTIMER; i++)
    {
        it->second[i].timer.cancel();
        lua_cb_release(it->second[i].cb);
    }
    g_npc_slots.erase(it);
}

/// Full timer cleanup for one NPC: the OnTimer machine and the one-shot
/// slots. Called from lua_npc_detach (npc_free_internal) and self:destroy.
void lua_npc_timer_detach(dumb_ptr<npc_data> nd)
{
    if (nd == nullptr)
        return;
    if (nd->npc_subtype == NpcSubtype::SCRIPT)
    {
        dumb_ptr<npc_data_script> nds = nd->is_script();
        nds->scr.timerid.cancel();
        nds->scr.timer_active = false;
        nds->scr.timer_eventv.clear();
        nds->scr.next_event = nds->scr.timer_eventv.end();
    }
    lua_npc_cleareventtimer(nd);
}

} // namespace map
} // namespace tmwa
