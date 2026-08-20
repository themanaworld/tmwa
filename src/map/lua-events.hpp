#pragma once
//    lua-events.hpp - event dispatch, host hooks, command registry.
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

#include "fwd.hpp"

#include "lua-compat.hpp"

#include <ctime>

#include "../strings/rstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/zstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "lua-types.hpp"


namespace tmwa
{
namespace map
{
// The one event entry point, preserving every rule of the old npc_event
// (doc/lua-engine.md section 5): broadcast for empty ev.npc, '~' phony tags,
// the mid-dialog queue rule, the same-map/in-area gate, disabled NPCs.
// force_inline is only ever set from the prelude dispatch wrapper.
bool lua_npc_event(dumb_ptr<map_session_data> sd, NpcEvent ev, LuaArgs args,
        bool force_inline = false);

// Broadcast to every non-puppet NPC with a handler for label; returns how
// many ran.
int lua_npc_broadcast(ScriptLabel label, dumb_ptr<map_session_data> sd,
        LuaArgs args);

// Resolve label against nd's definition table ("" -> click body, else
// events[label]) and push the handler function; false if there is none.
bool lua_npc_push_handler(lua_State* L, dumb_ptr<npc_data> nd, XString label);

// True when nd's definition table has a handler for label (host-side query;
// npc_touch_areanpc uses it for the no-OnTouch "fall back to click" rule).
bool lua_npc_has_handler(dumb_ptr<npc_data> nd, XString label);

// The npc_click script path: run the NPC's click body as a dialog coroutine
// (sets sd->npc_id). False when the NPC has no click body.
bool lua_npc_click(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd);

// Fire a function-form callback (consumes fn_ref, including its live-ref
// count) through the normal event path: queue rules apply when sd is
// mid-dialog; self_npc is pushed as `self` (nil when zero).
void lua_fire_fn_ref(int fn_ref, BlockId self_npc,
        dumb_ptr<map_session_data> sd, LuaArgs args);

// Host hooks (replace the npc_event_doall_l call sites).
void lua_hook_login(dumb_ptr<map_session_data> sd);
void lua_hook_logout(dumb_ptr<map_session_data> sd);
void lua_hook_die(dumb_ptr<map_session_data> sd);
void lua_hook_kill(dumb_ptr<map_session_data> killer,
        dumb_ptr<map_session_data> victim);
void lua_hook_mobkill(dumb_ptr<map_session_data> killer, Species mob_class,
        int x, int y);
void lua_hook_mob_death(dumb_ptr<mob_data> md,
        dumb_ptr<map_session_data> killer);
// 1 s clock: OnMinuteMM/OnClockHHMM/OnHourHH/OnDayMMDD dispatch, keeping the
// ev_tm_b change detection and the fire-all-on-first-tick behaviour.
void lua_hook_clock(const struct tm& now);

// Run every on_init in definition order (fatal on error), then start the
// clock timer; called from do_init after content loading.
void lua_run_oninit();

// Forget a function-form mob death handler (mob_delete, once-spawn expiry).
void lua_mob_forget(BlockId id);

// Command registry (server.registercmd): store replaces (and releases) any
// previous handler for word.
void lua_register_command(RString word, LuaCallback cb);
// Chat entry from magic_message: false if word is not registered; otherwise
// abandons an open dialog and runs the handler as a dialog coroutine.
bool lua_command_dispatch(dumb_ptr<map_session_data> sd, XString word,
        ZString rest);

// pc_attack_timer: fire sd->magic_attack synchronously with
// args = { target_id }.
void lua_fire_attack_spell(dumb_ptr<map_session_data> sd, BlockId target_id);

// Release the command registry (lua_final, before close_state).
void lua_events_final();

// ---------------------------------------------------------------------
// Additions to the scaffold surface (noted in the implementation report).

// The broadcast/clock hook index (doc/lua-engine.md section 2.3), owned by
// lua-events.cpp and maintained by lua-npc.cpp registration/detach.
enum class LuaHook
{
    LOGIN,
    LOGOUT,
    DIE,
    KILL,
    MOBKILL,
    CLOCK,
    COUNT,
};
void lua_hook_index_add(LuaHook hook, BlockId id);
// removes the NPC from every hook
void lua_hook_index_remove(BlockId id);
// definition-order list of non-puppet script NPCs (on_init, label scans)
void lua_events_npc_add(BlockId id);
void lua_events_npc_remove(BlockId id);

// As lua_npc_event, but the handler's args value is the Lua value at
// args_idx on L's stack (0 = none). Used by the npc.event / npc.event_all
// bindings in lua-npc.cpp, whose args argument is an arbitrary Lua table.
bool lua_npc_event_from_stack(lua_State* L, dumb_ptr<map_session_data> sd,
        NpcEvent ev, int args_idx, bool force_inline);

// The mid-dialog event queue lives engine-side in lua-events.cpp (design
// deviation, kept at integration: doc/lua-engine.md 2.1 put it on
// map_session_data::eventqueuel). npc_event_dequeue calls lua_event_dequeue
// once per END: it pops the front queued event and schedules it 100 ms
// later (the old pc_addeventtimer replay); returns true if one was
// scheduled. lua_event_queue_clear releases everything for the player
// (pc_authok, lua_session_detach on map_quit).
bool lua_event_dequeue(dumb_ptr<map_session_data> sd);
void lua_event_queue_clear(dumb_ptr<map_session_data> sd);
} // namespace map
} // namespace tmwa
