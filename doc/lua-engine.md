# TMWA Lua engine architecture

This document is the binding C++ design for the Lua 5.4 scripting engine that replaces the
eAthena script engine in the map server. The script-facing surface it implements is
`lua-scripting.md` (the API reference); the content translation rules are in
`lua-porting-guide.md`. Line numbers in section 12 refer to commit f5c87302.

Ground rules (from DECISIONS.md, restated where they shape the code):

* C++11 only. `<lua.hpp>` is included before `poison.hpp`, only via `lua-compat.hpp`.
* One `lua_State`, created before config parsing, closed after mapreg save.
* System liblua is C: errors and yields longjmp and skip C++ destructors. Section 11 is the
  discipline that makes this safe.
* Handles carry integer ids re-resolved per call; no `__gc` anywhere.
* Host -> Lua only through the two drivers (section 3); never `lua_call` at top level;
  `lua_atpanic` logs and aborts.

---------------------------------------------------------------------------------------------------

## 1. Modules

New files under `src/map/`. Every `lua-*.cpp` includes its own header first, which includes
`lua-compat.hpp` (and thus `<lua.hpp>`) before anything poisoned; `"../poison.hpp"` last.

| file | responsibility, public functions |
|---|---|
| `lua-compat.hpp` | the Luau-portability funnel: the only file that spells version-specific `lua_*`/`luaL_*` names (section 10). Inline wrappers only. |
| `lua-types.hpp` | plain structs shared with host headers, no Lua includes: `LuaPrompt`, `LuaSession`, `LuaCallback`, `LuaTimerSlot`, `LuaAnswer`. Entries in `src/map/fwd.hpp`. |
| `lua-engine.cpp/.hpp` | state lifetime: `lua_init()`, `lua_final()`; memory-capped allocator; sandbox environment and constants (`lua_read_constdb(ZString path)`); `import` and chunk loading (`lua_load_content()`, chunk names = paths); traceback handler; error logging (`lua_report_error`); instruction budget hook, `budget_push()/budget_pop()`; the two drivers `lua_run_sync(LuaCtx, int nargs)` and `lua_run_dialog(dumb_ptr<map_session_data>, dumb_ptr<npc_data>, LuaCtx, int nargs)`; `stop()` sentinel; `g_check_only`; `--dump-lua-api`; global C-implemented utilities (`rand`, `idiv`, `imod`, `int32`, `atoi`, `tostr`, `chr`, `ord`, `l`, `if_then_else`, `min/max/average`, `sqrt/cbrt/pow`, `bit32`, array helpers, `being`, `print`). |
| `lua-prelude.cpp` | the built-in Lua prelude as a C++ raw string, loaded into the sandbox at init: the `p:menu` table form (entry filtering, pairs, `cancel`, `title`), the Lua-to-Lua dispatch wrapper used by `npc.event` when the target handler must run inline in the current coroutine (pure Lua frames so yields cross it), small sugar (`arraystr`). Kept minimal; everything else is C++. |
| `lua-value.cpp/.hpp` | argument checking and pushing: `check_int`, `opt_int`, `check_bool`, `check_string` (`ZString` view), `opt_string`, `check_mapname`, `check_item` (id-or-name), `check_event` (`NpcEvent`, <= 23-byte label), `check_player`, `check_npc`, `check_being`, `push_string`, `push_player`, `push_npc`, `lua_warn(fmt, ...)`. All raise only from frames with trivial locals. |
| `lua-callback.cpp/.hpp` | `LuaCallback` value type (named event or function ref + owning NPC id), `lua_cb_named`, `lua_cb_from_stack`, `lua_cb_dup`, `lua_cb_release`, `lua_cb_fire`; the `g_live_refs` debug counter. |
| `lua-dialog.cpp/.hpp` | the per-session dialog state machine: `lua_dialog_resume(sd, npc_id, LuaPrompt kind, const LuaAnswer&)`, `lua_dialog_abandon(sd)`, `lua_session_attach(sd)`, `lua_session_detach(sd)`; the yielding bindings (`mes`, `mesq`, `mesn`, `clear`, `next`, `close`, `close2`, `menu`, `input`, `input_str`, `requestitem`, `requestlang`, `title`, `shop`, `openstorage`, `npcaction`, `camera`); the engine-owned `#itemdialog` NPC (`lua_create_item_dialog_npc()`). |
| `lua-events.cpp/.hpp` | event dispatch: `lua_npc_event(sd, NpcEvent, LuaArgs, bool force_inline)`, `lua_npc_broadcast(label, sd, args)`, label resolution against an NPC's definition table, the broadcast/clock hook index, the event queue integration with `npc_event_dequeue`, host hooks `lua_hook_login/logout/die/kill/mobkill/mob_death/clock`, `lua_run_oninit()`, command registry (`server.registercmd` storage, `lua_command_dispatch(sd, word, rest)` called from `magic_message`), `lua_fire_attack_spell(sd, target_id)`. |
| `lua-timers.cpp` | NPC OnTimer machinery (state kept verbatim from npc.cpp, incl. the `initnpctimer` no-op quirk), `addnpctimer`/`addtimer`/`areatimer` slot handling over `Array<LuaTimerSlot, MAX_EVENTTIMER>`. |
| `lua-npc.cpp/.hpp` | content constructors (`npc.script/warp/shop/monster/mapflag`), NPC lifecycle (`lua_npc_detach(nd)` from `npc_free_internal`), puppets, destroy, rename, enable/disable wrappers, the NPC handle metatable and methods, `npc.get/byid/exists/event/event_all`. |
| `lua-handle-player.cpp` | player handle metatable: params property dispatch (data-driven from params.txt), convenience properties, all `p:` methods except dialog; `p.vars`/`p.acc`/`p.acc2` proxies; `p.tmp`/`p.tmpstr` typed default tables. |
| `lua-handle-being.cpp` | being handle base metatable (`being(id)`), shared property/method dispatch player and NPC handles inherit. |
| `lua-lib-map.cpp`, `lua-lib-mob.cpp`, `lua-lib-item.cpp`, `lua-lib-players.cpp`, `lua-lib-server.cpp` | the `map`, `mob`, `item`, `players`, `server` namespaces (thin wrappers over map/mob/itemdb/pc/clif/intif). One file per namespace so parallel agents can implement them independently. |
| `lua-mapreg.cpp/.hpp` | mapreg storage (`mapreg_db`, `mapregstr_db`, dirty flag) moved from `script-startup.cpp`; byte-identical `name[,idx]\tvalue` file format; load/save/10 s autosave; the `world`/`world.str`/`world.array` proxies and `worldtmp`/`worldtmpstr` tables. |
| `lua-item-scripts.cpp/.hpp` | item chunk compilation (`lua_compile_item_script`), the item_db brace scanner hookup, `lua_item_use(sd, ref, nameid)` (dialog coroutine on `#itemdialog`), `lua_item_equip(sd, ref, slot, nameid)` (sync, `in_calcstatus` gate for `p:bonus`). |
| `lua-admin.cpp/.hpp` | `@setvar`/`@getvar` reimplementation (`lua_admin_setvar/getvar`: persistent scopes, `$` with index, and `@` temps via the player's `p.tmp`/`p.tmpstr` tables; the `.` NPC scope is dropped per D5), `@luastats` (memory, live refs, budget aborts). |

Kept and edited: `npc.cpp/.hpp` (click, touch, shops, enable, checknear, free, timer
plumbing, `magic_message` as the chat entry), `npc-parse.cpp/.hpp` shrunk to
`npc_addsrcfile/delsrcfile` plus the C++ builders `npc_create_warp/shop/monster`,
`npc_set_mapflag` shared by the Lua constructors and `@addwarp`.

Deleted (complete list): `src/map/script-buffer.hpp`, `script-call.cpp/.hpp`,
`script-call-internal.hpp`, `script-call.t.hpp`, `script-call-internal.tcc`,
`script-fun.cpp/.hpp/.t.hpp`, `script-parse.cpp/.hpp`, `script-parse-internal.hpp`,
`script-persist.hpp`, `script-startup.cpp/.hpp`, `script-startup-internal.hpp`,
`script-parse.py`, `script-persist.py`, `src/map/script-fun.cpp.orig`,
`src/map/npc-internal.hpp`, `src/debug-debug/map-script-parse.cpp`,
`src/debug-debug/map-script-persist.cpp`, `src/ast/script.cpp/.hpp`,
`src/ast/npc.cpp/.hpp`, `src/ast/npc_test.cpp`. (`src/ast/item.*` stays, edited: section 7.)

---------------------------------------------------------------------------------------------------

## 2. Data structures

### 2.1 On `map_session_data` (map.hpp)

```cpp
enum class LuaPrompt : uint8_t
{
    NONE,        // no prompt outstanding (no dialog, or handler running right now)
    NEXT,        // p:next()          answer: 0x00b9
    MENU,        // p:menu()          answer: 0x00b8 (1..menu_count or 0xff)
    INPUT_INT,   // p:input()         answer: 0x0143
    INPUT_STR,   // p:input_str()     answer: 0x01d5
    REQUESTITEM, // p:requestitem()   answer: 0x01d5 ("id,amount;...")
    REQUESTLANG, // p:requestlang()   answer: 0x01d5
    CLOSE2,      // p:close2()        answer: 0x0146 (0x00b9 also accepted)
    STORAGE,     // p:openstorage()   answer: storage_storageclose() only
    CLOSE,       // p:close()/p:shop()/menu cancel: coroutine finished, waiting for nothing
};

struct LuaSession                  // map_session_data::lua
{
    int serial = 0;                // bumped on attach; stale handles carry an older value
    int handle_ref = LUA_NOREF;    // registry ref of the player handle table (owner: session)
    int thread_ref = LUA_NOREF;    // registry ref of the dialog coroutine (owner: session)
    LuaPrompt prompt = LuaPrompt::NONE;
    int menu_count = 0;            // entries of the outstanding menu; selectable set kept Lua-side
    bool dialog_mes = false;       // a mes was sent since the last prompt/END (old state.npc_dialog_mes)
    bool running = false;          // currently inside lua_resume (re-entrancy guard)
    bool abandon_pending = false;  // abandon requested while running; driver finishes it
    bool in_calcstatus = false;    // equip scripts may call p:bonus freely while set
};
```

`map_session_data` keeps `npc_id`, `areanpc_id`, `npc_shopid`, `npc_flags.storage` (all
gates in clif/pc/trade/party read them unchanged, D4). Type changes:
`eventqueuel: std::list<LuaCallback>`; `eventtimer: Array<LuaTimerSlot, MAX_EVENTTIMER>`
where `struct LuaTimerSlot { Timer timer; LuaCallback cb; }`; `magic_attack: LuaCallback`.
Removed: `npc_pos`, `npc_menu`, `npc_amount`, `npc_str`, `npc_script`, `npc_scriptroot`,
`npc_stackbuf`, `state.menu_or_input`, `state.npc_dialog_mes`; `block_list` loses
`regm`/`regstrm`.

### 2.2 On `npc_data` / `npc_data_script`

```cpp
// npc_data (all subtypes)
int lua_handle_ref = LUA_NOREF;    // released in npc_free_internal
Array<LuaTimerSlot, MAX_EVENTTIMER> eventtimer;
// npc_data_script::scr
int def_ref = LUA_NOREF;           // the definition table (puppets: an extra ref on the parent's)
std::vector<interval_t> timer_intervals;   // sorted on_timer keys; the function is def.on_timer[ms]
std::vector<interval_t>::iterator next_event;
// kept: xs, ys, event_needs_map, parent, timer, timer_active, timerid, timertick
// removed: script (ScriptBuffer), label_listv; struct npc_timerevent_list, npc_label_list
// item_data
int use_script_ref = LUA_NOREF, equip_script_ref = LUA_NOREF;
// mob_data: npc_event (NpcEvent) kept as the identity tag for mob.killmonster/mob.mobcount;
// function-form death handlers live Lua-side in mob_death_fns[bl_id] (no new C++ field)
```

### 2.3 Engine registries

Engine-private globals in `lua-engine.cpp` (not `globals.hpp`): `lua_State* g_L`,
`g_sandbox_ref`, `g_consts_ref`, `g_players_ref` (blockid -> handle), `g_npcs_ref`
(blockid -> handle), `g_npcs_byname_ref`, `g_mob_death_fns_ref`, `g_loaded_files_ref`,
`std::vector<LuaCtx> g_ctx`, `bool g_check_only`, hook index
`std::map<LuaHook, std::vector<BlockId>> g_hook_index` (LuaHook = LOGIN, LOGOUT, DIE, KILL,
MOBKILL, CLOCK), `DMap<RString, LuaCallback> g_commands` (registercmd), `int g_live_refs`,
budget and allocator counters. `globals.hpp` keeps `npc_id`, `npcs_by_name`, `ev_tm_b`,
`npc_srcs`; drops `ev_db`, `spells_by_events`, `scriptlabel_db`, `probable_labels`,
`userfunc_db`, `str_datam`, `mapreg_db/mapregstr_db/mapreg_dirty` (move to lua-mapreg.cpp),
and all parser state.

An NPC's handlers are fields of its **definition table** (the table passed to
`npc.script{}`): `def.on_click` (sugar-normalised from `events[""]`), `def.on_touch`,
`def.on_init`, `def.on_timer[ms]`, `def.events[label]`. `npc.script` validates types,
normalises sugar fields into `events`, computes `timer_intervals`, and registers the NPC in
the hook index for each recognised broadcast label (`OnPCLoginEvent`, `OnPCLogoutEvent`,
`OnPCDieEvent`, `OnPCKillEvent`, `OnMobKillEvent`) and clock pattern (`OnMinuteMM`,
`OnClockHHMM`, `OnHourHH`, `OnDayMMDD`) found in `events`. There is no `ev_db`: label
resolution is `lua_npc_find_handler(nd, label)`: `""` -> click body; anything else ->
`events[label]`. Every old string form keeps working (`"Npc::OnTouch"`, `"Npc::"`,
`"::OnFoo"` broadcast).

### 2.4 `LuaCallback` and ownership

```cpp
struct LuaCallback
{
    NpcEvent event;           // named form {npc, label}; npc may be "" (broadcast) or start with '~'
    int fn_ref = LUA_NOREF;   // function form: registry ref (owner: this value's holder)
    BlockId self_npc;         // NPC pushed as `self` for function-form callbacks (may be 0)
    explicit operator bool() const;
};
LuaCallback lua_cb_named(NpcEvent ev);                 // no ref
LuaCallback lua_cb_from_stack(lua_State*, int idx);    // function or "Npc::Label" string; takes a ref
LuaCallback lua_cb_dup(const LuaCallback&);            // extra ref (areatimer stores one per player)
void lua_cb_release(LuaCallback&);                     // unref, idempotent
void lua_cb_fire(LuaCallback cb /*consumed*/, dumb_ptr<map_session_data> sd, LuaArgs args);
```

Ownership rule: every `LuaCallback` value is owned by exactly one holder and released by
that holder on every exit path:

| holder | release points |
|---|---|
| `sd->eventtimer[i].cb` | fire (`pc_eventtimer` moves it out first), `pc_cleareventtimer` (logout) |
| `nd->eventtimer[i].cb` | fire (`npc_eventtimer`), `npc_free_internal`, `destroy` cancel |
| `sd->eventqueuel` entries | dequeue (moved into a timer slot), `pc_authok` clear, `map_quit` |
| `sd->magic_attack` | `p:overrideattack()` replace/discharge, death reset, `map_quit` |
| `g_commands` entries | re-register, `lua_final` |
| locals inside firing functions | consumed by `lua_cb_fire` |

`g_live_refs` is incremented by `lua_cb_from_stack`/`lua_cb_dup` and decremented by release;
unit tests assert it returns to zero after every scenario; `@luastats` prints it live.

---------------------------------------------------------------------------------------------------

## 3. Execution model: the two drivers

Every host -> Lua entry goes through exactly one of:

```cpp
// Synchronous: lua_pcall on the main state with the traceback handler. The handler cannot
// yield (dialog primitives raise). Used by: on_init, on_timer, addnpctimer, clock events,
// OnPC*Event / OnMobKillEvent broadcasts, foreach callbacks, overrideattack handlers,
// equip scripts, npc.event without a player.
bool lua_run_sync(LuaCtx ctx, int nargs);

// Dialog coroutine: lua_newthread anchored in the registry, resumed by the packet handlers.
// Used by: npc_click, on_touch, npc.event with a player, player event timers (addtimer,
// areatimer, queue replay), registered command handlers, per-spawn mob death events with a
// killer, item use scripts.
void lua_run_dialog(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> dialog_npc,
                    LuaCtx ctx, int nargs);
```

`LuaCtx { BlockId npc; BlockId player; const char* what; }` is pushed on `g_ctx` for the
duration of the call and re-pushed on every resume (the coroutine stores its ctx in a
per-thread table), so `p:mesn()` default, `p:addtimer` self capture, and error messages know
the context after a yield.

`lua_run_sync`: push traceback handler below the function; `budget_push()`;
`lua_pcall(L, nargs, 0, msgh)`; on error, if the error object is the `stop()` sentinel do
nothing, else log the traceback with NPC/player/what context; `budget_pop()`. Sync handlers
never own dialogs, so nothing is done to the session's coroutine.

`lua_run_dialog`:
1. Precondition `sd->lua.thread_ref == LUA_NOREF` (callers check `sd->npc_id` first);
   violation logs and drops the handler (refs released).
2. `sd->npc_id = dialog_npc->bl_id` (asserted; set by the caller on the click/event paths);
   `dialog_mes = false; prompt = NONE`.
3. `T = lua_newthread(L)`; `lua_xmove` function + args to `T`; `thread_ref = ref(...)`;
   **the thread also stays on L's stack as the resume holder until step 6**;
   `thread_ctx[T] = ctx`.
4. `running = true; budget_push(); status = resume(T, L, nargs, &nres); budget_pop();
   running = false`.
5. `lua_dialog_after_resume(sd, status)` (shared with section 4):
   * `LUA_YIELD` and `prompt != CLOSE`: the yielding primitive already sent its packet and
     set `prompt`; if `abandon_pending`: end the thread now (no packets); else return.
   * `LUA_YIELD` and `prompt == CLOSE` (`p:close()`, `p:shop()`, menu cancel): finished; END.
   * `LUA_OK` (returned): if `dialog_mes`, send `clif_scriptclose` (automatic close); END.
   * error: `stop()` sentinel behaves like `LUA_OK`; otherwise log `traceback(T)`; if
     `dialog_mes` or a prompt was outstanding, send `clif_scriptclose` so the client is not
     left buttonless; END.
   * END = `lua_dialog_end(sd)`: `close_thread(T)`, unref, `thread_ref = LUA_NOREF`,
     `prompt = NONE`, `dialog_mes = false`, clear `thread_ctx[T]`, then
     `npc_event_dequeue(sd)` (clears `npc_id`, schedules one queued event 100 ms later via
     `pc_addeventtimer`, exactly as today). This is the only place besides
     `npc_event_dequeue` itself that clears `npc_id`.
6. Pop the resume holder.

Why the resume holder: a running coroutine must stay anchored while inside `lua_resume`; the
registry ref may be released during the run (`self:destroy()` -> `npc_event_dequeue` ->
`lua_dialog_abandon`), so the driver keeps its own stack slot. Why `running`:
`storage_storageclose` can be reached from inside a handler (`p:warp` -> `pc_setpos` ->
`storage_storageclose`); the guard turns that into a no-op instead of a nested resume.
Why `abandon_pending`: `lua_dialog_abandon` called while `running` must not close the thread
under the interpreter's feet; it sets the flag and the driver ends the thread after resume
returns.

`budget_push/pop` nest: a sync handler run from inside a coroutine (`map.foreach`) gets a
fresh budget and restores the outer one.

---------------------------------------------------------------------------------------------------

## 4. Dialog state machine: resume paths and validation

```cpp
struct LuaAnswer { int menu = 0; int amount = 0; ZString str; };  // trivially destructible
bool lua_dialog_resume(dumb_ptr<map_session_data> sd, BlockId npc_id,
                       LuaPrompt kind, const LuaAnswer& a);
```

Callers (the complete set): `clif_parse_NpcNextClicked` (NEXT),
`clif_parse_NpcSelectMenu` (MENU, `a.menu`), `clif_parse_NpcAmountInput` (INPUT_INT,
`a.amount`), `clif_parse_NpcStringInput` (INPUT_STR, `a.str`; the driver distinguishes
INPUT_STR/REQUESTITEM/REQUESTLANG by `sd->lua.prompt`), `clif_parse_NpcCloseClicked`
(CLOSE2), `storage_storageclose` (STORAGE). `map_scriptcont` and `npc_scriptcont` are
deleted; clif calls `lua_dialog_resume` directly.

Validation, in order (any failure returns false, nothing resumed):

1. `sd->lua.running` -> log and ignore (the storage re-entrancy path).
2. `npc_id != sd->npc_id` -> ignore (stale/forged packet, as old `npc_scriptcont`).
3. `thread_ref == LUA_NOREF` -> ignore (`npc_id` set transiently by the shop-click path).
4. `map_id_is_npc(sd->npc_id)` null -> `npc_event_dequeue(sd)` (abandons) and return.
5. `npc_checknear(sd, nd)` (unchanged; INVISIBLE_CLASS passes, which is what makes the
   `#itemdialog` NPC work): fail -> `clif_scriptclose`, return (dialog stays, as old).
6. `nd->deletion_pending` -> `clif_scriptclose` + `npc_event_dequeue`, return.
7. Orphan puppet -> `npc_free(nd)`, return.
8. Kind check against `sd->lua.prompt`:
   * `NEXT` accepts NEXT.
   * `MENU` accepts MENU; `a.menu == 0xff` -> if the menu's table form declared a `cancel`
     value (recorded Lua-side), resume with it; else END silently (old cancel).
     Out-of-range or not-selectable -> log and ignore (prompt stays).
   * `INPUT_INT` accepts INPUT_INT; `a.amount < 0` -> `clif_tradecancelled` + close packet +
     END (old behaviour).
   * `INPUT_STR`, `REQUESTITEM`, `REQUESTLANG` accept INPUT_STR.
   * `CLOSE2` accepts CLOSE2 and NEXT (old: close2 resumed on 0x0146 or 0x00b9).
   * `STORAGE` accepts STORAGE only.
   * **`CLOSE2`-kind packet (0x0146) while prompt is NEXT/MENU/INPUT_INT/INPUT_STR/
     REQUESTITEM/REQUESTLANG: abandon the dialog** (END without packets: the client already
     closed the window). Rationale: the old engine resumed the script at `npc_pos`, which
     could grant rewards on a closed window; ignoring instead would soft-lock the player
     (`npc_id` stays set with the window gone). Abandoning is the only safe answer.
   * `CLOSE`, `NONE`: ignore.
   * Any other mismatch: log once per (session, kind) and ignore.
9. `prompt = NONE`; push the answer onto `T` (MENU: the 1-based index or the table-form
   value resolved Lua-side; INPUT_INT: int; INPUT_STR: string; REQUESTITEM: parse
   `"id[,amount];..."` in C++, keep ids present in the inventory, at most the requested max,
   push a sequence; REQUESTLANG: string; NEXT/CLOSE2/STORAGE: nothing).
10. Push the resume holder; `running = true; budget_push(); resume(T, L, nargs, &nres);
    budget_pop(); running = false`; `lua_dialog_after_resume`; pop the holder.

`p:close()` yields with `prompt = CLOSE` after sending its packet; the driver treats the
yield as END, so code after `p:close()` never runs. `p:close2()` sends the packets, yields
with CLOSE2, and continues on resume; `dialog_mes` is reset so a later `p:close()` sends
`npc_action 5` (no window), preserving the `close2; openstorage;` sequence exactly.
`p:openstorage()` calls `storage_storageopen`; refusal returns false without yielding;
otherwise `npc_flags.storage = 1`, `prompt = STORAGE`, yield; `storage_storageclose` keeps
its existing check and resumes. `storage_storage_quit` does not resume; `map_quit` abandons.

### 4.1 npc_id invariants (every existing reader keeps its meaning)

| state | meaning |
|---|---|
| `npc_id == 0` | free: walking, attacking, item use, trade, clicks allowed; `thread_ref == LUA_NOREF`, `prompt == NONE` |
| `npc_id != 0 && thread_ref != LUA_NOREF && prompt != NONE` | dialog suspended, waiting for the client |
| `npc_id != 0 && thread_ref != LUA_NOREF && prompt == NONE && running` | handler executing right now |
| `npc_id != 0 && thread_ref == LUA_NOREF` | transient: the shop-click path only (`npc_click` SHOP sets npc_id, opens buy/sell, then dequeues) |

### 4.2 Abandon and detach

`npc_event_dequeue(sd)` (name and callers kept): sets `npc_id = 0`, calls
`lua_dialog_abandon(sd)` (new), then schedules one queued event per END through
`pc_addeventtimer(sd, 100_ms, moved LuaCallback)`, unchanged semantics.

`lua_dialog_abandon(sd)`: if no thread, no-op. If `running`: set `abandon_pending` and
return (the driver ends the thread after resume returns). Else `close_thread`, unref, reset
prompt/mes/menu state. No packets: callers that need packets send them first.

Existing `npc_event_dequeue` callers (all unchanged): `clif_parse_LoadEndAck` (map change
ends a dialog), the eight "NPC was freed" self-heal sites in clif.cpp, `trade_tradecancel`,
`npc_click` SHOP path, the disabled-NPC event path, `destroy`, the driver's END.

`map_quit` calls `lua_session_detach(sd)` right after `pc_cleareventtimer(sd)`: abandons the
coroutine, releases every `LuaCallback` in `eventqueuel` and `magic_attack`, removes the
handle from `g_players_ref`, unrefs `handle_ref`. `p.tmp`/`p.tmpstr` are fields of the
handle table and die with it. A later session for the same account gets a new `serial`, so
stale handles stay stale.

`npc_free_internal` calls `lua_npc_detach(nd)`: cancel and release the NPC's timer slots,
unref `def_ref` (puppets: their extra ref), remove `g_npcs_ref[blockid]` and
`g_npcs_byname_ref[name]`, unref `lua_handle_ref`, remove hook index entries. Players in a
dialog with that NPC are not scanned (as today): their next packet hits step 4 above.
`self:destroy()` from its own dialog: dequeue sets `abandon_pending`, `npc_free` is deferred
on a 0 ms timer, the handler continues until it returns (the guide appends `return`).

### 4.3 The `#itemdialog` NPC

An engine-created floating NPC named `#itemdialog` (INVISIBLE_CLASS, created in `lua_init`,
never returned by `npc.get`, excluded from broadcasts). `pc_useitem` runs the use-script
chunk as a dialog coroutine with `dialog_npc = itemdialog` (only reachable when
`sd->npc_id == 0`, guaranteed by `clif_parse_UseItem`). While the use script has a dialog
open, `sd->npc_id == itemdialog->bl_id`: every walk/attack/item gate works as for any
dialog, `npc_checknear` passes (invisible), and packets round-trip with a real npc id. A use
script that never opens a dialog finishes immediately and `npc_event_dequeue` clears
`npc_id` without packets. This is what makes "use scripts may open dialogs" (D1) safe with
zero client assumptions.

---------------------------------------------------------------------------------------------------

## 5. Events: dispatch, queueing, commands

`lua_npc_event(sd /*nullable*/, NpcEvent ev, LuaArgs args, bool force_inline = false)`
preserves every rule of the old `npc_event` (npc.cpp:546-644):

1. `ev.npc` empty -> broadcast (`lua_npc_broadcast`): run each registered NPC's handler
   synchronously (hook index for the fixed labels, else scan non-puppets for
   `events[label]`), puppets skipped, disabled flag ignored, `npc_id` untouched; one failing
   NPC does not stop the others.
2. `ev.npc` starts with `~` -> return (phony tags), unless it is a synthetic `~lua#<n>` key
   (function-form callbacks).
3. Resolve the NPC; not found -> log "event not found" (silent for `OnTouch`, returning the
   "fall back to click" code), return.
4. Resolve the handler in the def table; missing -> same as 3.
5. `deletion_pending` -> drop; orphan puppet -> `npc_free`, drop.
6. With `sd`:
   a. `event_needs_map` (map-placed, non-puppet): same map and inside the xs/ys box, else
      return.
   b. Stale `sd->npc_id` (freed NPC) -> `npc_event_dequeue(sd)`.
   c. **Queue rule** (D4, preserved): if `sd->npc_id != 0 && thread_ref != LUA_NOREF
      && args.empty() && !force_inline` -> `eventqueuel.push_back(lua_cb_named(ev))`,
      return "queued". Engine-supplied extras (`target_id`, `argstring`, kill args) count as
      args; the per-spawn mob death `mob` info does NOT (those events queued in the old
      engine and still do).
   d. Disabled NPC -> `npc_event_dequeue(sd)`, drop.
   e. Dialog-capable entry -> `sd->npc_id = nd->bl_id; lua_run_dialog(...)`. Synchronous
      entry (foreach, overrideattack) -> `lua_run_sync` with `p` pushed, `npc_id` untouched.
7. Without `sd`: `lua_run_sync` with `p = nil`.

`force_inline` covers `npc.event(ev, p, args)` called from Lua inside the dialog coroutine
of that same player: the prelude dispatch wrapper resolves the handler and calls it directly
in the current coroutine through pure Lua frames, so the called handler may itself yield
(nested dialog calls never cross a C trampoline). From C++ the inline path is never used.

Entry-point table (which driver, which queue behaviour):

| entry | driver | queue rule |
|---|---|---|
| `npc_click` (click, attack-click, touch without on_touch) | dialog | n/a (requires `npc_id == 0`) |
| `on_touch` via `npc_touch_areanpc` | dialog | as old |
| `npc.event("N::OnX", p)`, `p:event`, `p:addtimer`, `map.areatimer`, queue replay | dialog | queued if mid-dialog |
| same, called from that player's own running dialog | inline in the coroutine | never queued |
| registered command word (`magic_message`) | dialog | never queued: an open dialog is **abandoned first** (`npc_event_dequeue`), then the command runs. This matches the old clobber path observably (spell casting mid-dialog keeps working) without the stale-stack corruption |
| per-spawn mob death (killer attached) | dialog | queued if mid-dialog |
| item use script | dialog (on `#itemdialog`) | n/a |
| `npc.event` without player, `npc.event_all`, broadcasts | sync | never |
| `on_init`, `on_timer`, `addnpctimer`, clock | sync, no player | never |
| `OnPCLoginEvent/Logout/Die/Kill`, `OnMobKillEvent` | sync with player | never (old doall did not queue either) |
| `map.foreach` callbacks | sync with caller | never |
| `overrideattack` handler | sync with player | never |
| equip scripts | sync, `in_calcstatus` | never |

Host hooks (replace the `npc_event_doall_l` calls): `lua_hook_login(sd)`,
`lua_hook_logout(sd)`, `lua_hook_die(sd)`, `lua_hook_kill(killer, victim)`,
`lua_hook_mobkill(killer, Species, x, y)`, `lua_hook_mob_death(md, killer)`,
`lua_hook_clock(const tm&)` (1 s timer, keeps `ev_tm_b` minute/hour/day change detection and
the fire-all-on-first-tick behaviour), `lua_run_oninit()`.

Mob death: `mob.monster(..., event)` with a string stores `md->npc_event` as today; with a
function it stores a synthetic tag `~lua#<n>` in `md->npc_event` (so `mob.killmonster` and
`mob.mobcount` can match it; `mob.monster` returns the tag) and the function in
`mob_death_fns[md->bl_id]`. `mob_damage` fires the per-spawn handler (dialog path, queue
rules) then `lua_hook_mobkill`. `mob_delete` / once-spawn expiry call
`lua_mob_forget(bl_id)`; the `mob_death_fns` table is additionally swept every 10 minutes
for dead ids. No event fires when the killer is not a player (as today).

Commands: `server.registercmd(word, cb)` stores a `LuaCallback` in `g_commands` (releasing
any previous entry). `magic_message` (kept in npc.cpp) tokenises as today and calls
`lua_command_dispatch(sd, cb, rest)`: abandon-then-run per the table above; the handler runs
as a dialog coroutine with `(self, p, argstring)` for the event form or `(p, argstring)` for
the function form.

`p:overrideattack`: stores a `LuaCallback` in `sd->magic_attack`; `pc_attack_timer` fires it
via `lua_run_sync` with `args = {target_id}`.

---------------------------------------------------------------------------------------------------

## 6. Timers

* **NPC OnTimer machine**: `npc_timerevent_start/stop/settick/gettick` keep their bodies in
  `lua-timers.cpp`, operating on `timer_intervals`; the state machine is preserved verbatim,
  **including the `initnpctimer` no-op quirk** while the timer is counting toward its first
  label (decision: keep the quirk, zero audit cost for the 67 uses). Firing schedules the
  next interval before running the handler (so `stopnpctimer` inside the handler cancels the
  chain, as today), then `lua_run_sync(def.on_timer[interval], self)`.
* **Player event timers** (`p:addtimer`, `map.areatimer`, queue replay): 32
  `LuaTimerSlot`s on the session; `pc_addeventtimer(sd, interval, LuaCallback)`; full ->
  release + log. On fire, `pc_eventtimer` moves the callback out and `lua_cb_fire`s it
  through the normal event path (queue rules apply).
* **NPC one-shot timers** (`self:addnpctimer`): 32 slots on the NPC; fire via
  `lua_run_sync`, no player.
* **Clock**: the 1 s timer starts after `lua_run_oninit()` (first tick after 100 ms fires
  all), dispatching to the hook-indexed `OnMinute/OnClock/OnHour/OnDay` handlers by
  formatted label name.

---------------------------------------------------------------------------------------------------

## 7. Variables and item scripts

### 7.1 Proxies

* `p.vars` / `p.acc` / `p.acc2`: proxy tables (one per handle) whose `__index`/`__newindex`
  call `pc_readglobalreg/pc_setglobalreg`, `pc_readaccountreg(2)/pc_setaccountreg(2)` with
  `stringish<VarName>` names. Key must be a string, `[A-Za-z0-9_]{1,31}` (longer raises;
  trailing `$` raises "int only"); value `check_int` + int32 wrap; 0/nil deletes. The
  `#`/`##` prefixes are added by the proxy so wire and disk formats stay byte-identical.
  Cap overflow: the pc_ function logs (as today) and the Lua write returns normally.
  Quest-log aliasing and `PC_DIE_COUNTER` stay untouched inside `pc_setglobalreg`.
  `p.vars:names()` enumerates set names (for `@getvar` and maintenance scripts).
* `p.tmp`/`p.tmpstr`, `self.vars`/`self.varstr`, `worldtmp`/`worldtmpstr`: plain Lua tables
  with a default-value metatable (`__index` returns 0 or `""` for missing keys). They live
  in the owning handle (or globally for worldtmp) and are GC'd with it.
* `world`: `__index`/`__newindex` map names to mapreg index 0 (`world.str` for the `$X$`
  namespace); `world.get/set(name, idx)`; `world.array(name)` returns an index proxy that
  the generic `setarray`/`cleararray`/`getarraysize`/`array_search` helpers accept
  (metatable detection). Storage interns the full old-format name (`$NAME`, `$NAME$`) so
  `mapreg.txt` round-trips byte-identically; the 10 s autosave timer and load/save move into
  `lua-mapreg.cpp` unchanged.
* Params: the player `__index` checks methods, then convenience properties, then the param
  name set (from params.txt at load) -> `pc_readparam`; unknown names read nil, unknown
  writes raise. `__newindex` checks the writable-param set -> `pc_setparam`; read-only
  raises.

### 7.2 Item scripts

* `src/ast/item.cpp` gains `lex_lua_body`: after `{`, count brace depth; braces inside
  `"..."`/`'...'` (with backslash escapes) are ignored; long brackets and `--` comments are
  NOT recognised (item lines are one physical line; documented). The text between the outer
  braces is the chunk. `ScriptBody` becomes `{RString text; io::LineSpan span;}`.
* `itemdb_readdb`: compile each non-empty column with the prelude
  `"local p, args = ...\n" + body`, chunk name `=item_db:<id>:use|equip`, env = sandbox;
  store the registry ref in `item_data` (`use_script_ref`, `equip_script_ref`). Empty body
  -> `LUA_NOREF` (fast-path skip). Compile error -> fatal at startup.
* Use: `pc_useitem` captures the ref before `pc_delitem`, then
  `lua_item_use(sd, ref, nameid)`: dialog coroutine on `#itemdialog`, `args = {itemId}`.
* Equip: `pc_calcstatus` calls `lua_item_equip(sd, ref, slot, nameid)` at the three old call
  sites: `in_calcstatus = true; lua_run_sync(...); in_calcstatus = false`, with
  `args = {itemId, slotId}`. `p:bonus` outside `in_calcstatus` logs a rate-limited warning
  and still applies (old behaviour). A yield inside an equip script raises Lua's own
  "attempt to yield" error, logged once per item id; the remaining equip scripts still run.
  `lr_flag_is_arrow_2` handling in `pc_bonus` is untouched.

---------------------------------------------------------------------------------------------------

## 8. Errors, limits, sandbox

* **Load time**: any compile or runtime error while loading content (including `on_init`) is
  fatal (`runflag = false`); message with chunk name, line, traceback.
* **Run time**: every entry uses the traceback message handler; the log line carries the NPC
  name, event label, player, `file:line` of the first Lua frame, then the indented
  traceback. The handler is aborted; an open dialog is closed cleanly; the server continues.
* **Instruction budget**: a count hook (installed via the compat header, every 1000
  instructions) decrements a budget; at zero it raises
  `instruction budget exhausted (N VM instructions without yielding)`. Default
  `lua_conf.instruction_budget = 20000000` per host entry/resume; `budget_push/pop` nest.
  No `freeloop`.
* **Memory cap**: the `lua_Alloc` passed to `new_state` enforces
  `lua_conf.memory_limit_mb` (default 512): allocations beyond it fail, surfacing as
  `LUA_ERRMEM` through the normal error path. `@luastats` prints usage, `g_live_refs`, and
  budget-abort counts.
* **Sandbox** (built in `lua_init`): a fresh environment table with the whitelisted base
  functions, copies of `string`/`table`/`math`/`utf8`/read-only `coroutine`,
  `os = {time, clock, date(UTC-forced)}`, `bit32` and the engine globals; `print` redirected;
  `collectgarbage` restricted to `"count"`; no `load/loadfile/dofile/require/io/debug/
  package`. Every chunk gets it as `_ENV` via `set_chunk_env`.
* **Constants**: `lua_read_constdb` fills a `consts` table; the environment metatable's
  `__index` falls back to it and raises `undefined global 'X'` for unknown names (with the
  "did you mean p.X?" hint for param names); `__newindex` raises for constant names, allows
  new globals during load and `on_init`, and afterwards logs a warning (error when
  `g_check_only`).
* `lua_atpanic` logs and aborts.

---------------------------------------------------------------------------------------------------

## 9. The prelude

`lua-prelude.cpp` holds a small pure-Lua layer loaded into the sandbox at init, before any
content:

* the `p:menu` table form: entry validation, skip-but-count for `false`/`""` entries,
  pair `{text, value}` resolution, `title` and `cancel` keys; it assembles the display
  string and the selectable-index map, calls the C `menu` primitive (which yields), and maps
  the answer back to `value, index`.
* the `npc.event` inline-dispatch wrapper: when the target is a handler of the same player's
  running coroutine context, resolve the function (via a C helper) and call it directly so
  yields cross only Lua frames.
* `arraystr` and other trivial sugar.

Rationale: dialog primitives must be able to yield across every frame between the coroutine
and the yield point; pure Lua frames guarantee that, C trampolines do not.

---------------------------------------------------------------------------------------------------

## 10. `lua-compat.hpp`: the exact wrapped surface

Inline functions, each implemented once for Lua 5.4 (and later a second time for Luau):

```cpp
namespace tmwa { namespace map { namespace luac {
lua_State* new_state(lua_Alloc alloc, void* ud);        // lua_newstate + atpanic
void  close_state(lua_State*);
bool  load_chunk(lua_State*, XString chunkname, XString src);  // 5.4: luaL_loadbufferx(.., "t"); Luau: luau_compile + luau_load
void  push_cfunction(lua_State*, lua_CFunction, const char* debugname);
void  push_cclosure(lua_State*, lua_CFunction, const char* debugname, int nup);
void  register_funcs(lua_State*, int tableidx, const luaL_Reg*);   // 5.4: luaL_setfuncs
int   ref(lua_State*);                                  // pops top; 5.4: luaL_ref(REGISTRY); Luau: lua_ref
void  unref(lua_State*, int r);
void  push_ref(lua_State*, int r);
void  push_globals(lua_State*);                         // 5.4: lua_pushglobaltable; Luau: LUA_GLOBALSINDEX
void  set_chunk_env(lua_State*, int chunkidx, int envidx);  // 5.4: lua_setupvalue(_ENV); Luau: lua_setfenv
void  sandbox_finalise(lua_State*);                     // Luau: luaL_sandbox; 5.4: no-op
int   push_int(lua_State*, int);
bool  to_int(lua_State*, int idx, int* out);            // integral within int32
void  push_string(lua_State*, XString);
ZString to_string(lua_State*, int idx, size_t* len);
int   resume(lua_State* T, lua_State* from, int nargs, int* nres);  // 5.4: 4-arg lua_resume
void  close_thread(lua_State* T, lua_State* from);      // 5.4.6+: lua_closethread; 5.4.0-5: lua_resetthread (LUA_VERSION_RELEASE_NUM switch)
void  traceback(lua_State* L, lua_State* T, const char* msg);       // 5.4: luaL_traceback; Luau: lua_debugtrace
void  set_instruction_hook(lua_State*, void (*cb)(lua_State*), int every);  // 5.4: lua_sethook(COUNT); Luau: interrupt callback
void* thread_data(lua_State*);                          // 5.4: lua_getextraspace; Luau: lua_getthreaddata
size_t raw_len(lua_State*, int idx);                    // 5.4: lua_rawlen; Luau: lua_objlen
bool  is_integer_value(lua_State*, int idx);
}}}
```

Everything else the bindings use is in the 5.4/Luau common subset (`luaL_check*`,
`lua_push*`, `lua_to*`, `lua_getfield/setfield`, `lua_newtable/createtable`, 4-arg
`lua_pcall`, `lua_yield`, `lua_error`, `luaL_error`, `luaL_argerror`, `luaL_newmetatable`,
`lua_setmetatable`, `lua_newthread`, `lua_xmove`, `lua_status`, stack ops, `lua_next`,
`lua_rawget/rawset/rawgeti/rawseti`).

Forbidden outside `lua-compat.hpp`, enforced by `tools/check-lua-compat.py` wired into
ctest: `lua_callk`, `lua_pcallk`, `lua_yieldk`, `lua_newuserdatauv`, `lua_setiuservalue`,
`lua_toclose`, `lua_seti`, `lua_geti`, `lua_rawlen`, `luaL_setfuncs`, `luaL_newlib`,
`luaL_requiref`, `luaL_ref`, `luaL_unref`, `lua_pushglobaltable`, `lua_sethook`,
`lua_getextraspace`, `luaL_loadbuffer`, `luaL_loadstring`, `luaL_dostring`,
`lua_isinteger`, `luaL_traceback`, `lua_setwarnf`, `lua_dump`, `__gc`, `lua_Integer`.

Debug build option `TMWA_VENDOR_LUA` (CMake): compile a vendored Lua 5.4 as C++ so
error/yield unwinding uses exceptions and destructor-skipping bugs surface as clean failures
in CI. Release builds always link the system C liblua.

---------------------------------------------------------------------------------------------------

## 11. Longjmp discipline

Rules (D3), enforced by review, the compat grep, and the `TMWA_VENDOR_LUA` CI job:

1. Argument checks first, while only trivially-destructible locals exist.
2. The tmwa-typed worker runs in an inner scope and returns error info as values; no raising
   Lua API inside it.
3. Push results (or raise) only from frames with trivial locals again.
4. Yields only as `return lua_yield(L, n)` at tail position with trivial locals.
5. Non-fatal game failures log and return nil/false/0; they never raise.
6. No `Array::operator[]` on unchecked indices; no C++ exceptions in bindings.

Template for a binding:

```cpp
static int lp_getitem(lua_State* L)
{
    // 1. checks: may raise; only PODs/views live
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ItemNameId nameid = check_item(L, 2);
    int amount = check_int(L, 3);
    if (!sd || !nameid || amount <= 0)
        { lua_pushboolean(L, 0); return 1; }
    // 2. worker: tmwa types confined to this scope, no raising Lua API
    bool ok;
    {
        ok = getitem_worker(sd, nameid, amount);   // pc_additem + overflow drop
    }
    // 3. results
    lua_pushboolean(L, ok);
    return 1;
}
```

Template for a yielding function:

```cpp
static int lp_next(lua_State* L)
{
    // 1. checks
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (!sd) return 0;
    // 2. validate dialog context, send the packet: worker with trivial state only
    if (!dialog_prepare_prompt(L, sd, LuaPrompt::NEXT))   // raises via a trivial frame if
        return 0;                                         // called outside a dialog coroutine
    clif_scriptnext(sd, sd->npc_id);
    sd->lua.prompt = LuaPrompt::NEXT;
    // 3. yield from a frame with trivial locals, at tail position
    return lua_yield(L, 0);
}
```

---------------------------------------------------------------------------------------------------

## 12. Host integration: per-file edit list

Line numbers refer to commit f5c87302.

| file | edits |
|---|---|
| `src/map/map.hpp` | remove `state.menu_or_input` (:158), `state.npc_dialog_mes` (:159), `npc_pos` (:200), `npc_menu` (:201), `npc_amount` (:202), `npc_script`/`npc_scriptroot` (:205), `npc_stackbuf` (:206), `npc_str` (:207), `block_list::regm/regstrm` (:82-85); keep `npc_id/areanpc_id/npc_shopid` (:198) and `npc_flags.storage` (:209-211); add `LuaSession lua;`; `eventqueuel` (:288) -> `std::list<LuaCallback>`; `eventtimer` (:289, :368) -> `Array<LuaTimerSlot, MAX_EVENTTIMER>`; `magic_attack` (:223) -> `LuaCallback`; `npc_data` gains `lua_handle_ref`; `npc_data_script::scr` per section 2.2; remove `npc_timerevent_list`/`npc_label_list` (:331-339); drop the script-buffer/script-persist includes (:48-49); include `lua-types.hpp` |
| `src/map/fwd.hpp` | drop `ScriptState`, `str_data_t`, `SIR`, `event_data`; add `LuaCallback`, `LuaSession`, `LuaTimerSlot`, `LuaPrompt` |
| `src/map/clif.cpp` | `clif_parse_NpcSelectMenu` (:5021), `NextClicked` (:5039), `AmountInput` (:5056), `StringInput` (:5076), `CloseClicked` (:5096): stop writing the removed session fields, call `lua_dialog_resume(sd, fixed.npc_id, <kind>, a)`; `clif_parse_LoadEndAck`: keep the `npc_event_dequeue` line (:3677, now also abandons), replace the `OnPCLoginEvent` doall (:3752) with `lua_hook_login(sd)`; the eight npc_id self-heal sites (:3837-4776) unchanged; packet table unchanged |
| `src/map/pc.cpp` | equip-script `run_script_l` sites (:1301, :1314, :1340) -> `lua_item_equip(...)` (drop the `argrec_t` blocks); `pc_useitem` (:2405-2444) -> capture ref, `lua_item_use`; `pc_damage` (:3665-3684) -> `lua_hook_kill` / `lua_hook_die`; magic override reset (:3585-3590) adds `lua_cb_release`; `pc_logout` (:5726) -> `lua_hook_logout`; `pc_attack_timer` (:2881-2939) -> `lua_fire_attack_spell`; `pc_authok` (:825-996): clear queue with release, add `lua_session_attach(sd)` before the first `pc_calcstatus`; `pc_eventtimer` (:4779) / `pc_addeventtimer` (:4792) / `pc_cleareventtimer` (:4816) reworked for `LuaTimerSlot`; delete `pc_readreg/pc_setreg/pc_readregstr/pc_setregstr` (:4449-4497); keep `pc_*globalreg*`, `pc_*accountreg*`, `pc_readparam/pc_setparam`, `pc_bonus/2` |
| `src/map/mob.cpp` | `mob_damage` (:2705-2727): `lua_hook_mob_death(md, sd); lua_hook_mobkill(sd, class, x, y)`; `mob_delete`/`mob_setdelayspawn` add `lua_mob_forget`; `mob_once_spawn/_area` (:424-540) keep `NpcEvent` signatures |
| `src/map/npc.cpp/.hpp` | keep click/touch/shop/enable/free/checknear plumbing and `magic_message`; `npc_click` script path -> `lua_run_dialog`; delete `npc_event*` bodies (replaced by lua-events), `npc_scriptcont`, `npc_event_do_clock`, `npc_event_do_oninit`, bytecode timer parts (timer state machine moves to lua-timers verbatim); `npc_event_dequeue` per section 4.2; `npc_free_internal` calls `lua_npc_detach` |
| `src/map/npc-parse.cpp/.hpp` | shrink to `npc_addsrcfile/delsrcfile` + `npc_create_warp/shop/monster`, `npc_set_mapflag` |
| `src/map/atcommand.cpp` | `atcommand_set_var` (:5225) / `get_var` (:5284) -> `lua_admin_setvar/getvar` (persistent scopes, `$` with index, `@` temps; `.` scope dropped per D5); `@addwarp` (:4716) -> `npc_create_warp`; remove the script-call include (:30); add `@luastats` |
| `src/map/storage.cpp` | `storage_storageclose` (:259-263) -> `lua_dialog_resume(sd, sd->npc_id, STORAGE, {})` |
| `src/map/trade.cpp`, `party.cpp` | unchanged (read `npc_id`, call `npc_event_dequeue`) |
| `src/map/map.cpp` | `do_init` (:1567-1633) / `term_func` (:1533-1562) per section 13; delete `map_scriptcont` (:1518); `map_quit` (:796): `npc_stackbuf.clear()` -> `lua_session_detach(sd)`; `map_confs` (:1489): `const_db` -> `lua_read_constdb`, add `lua_conf`; parse `--check-scripts[=load]` |
| `src/map/map_conf.*` via `tools/config.py` | keep `npc:`/`delnpc:`/`mapreg_txt`; add `lua_conf`: `instruction_budget` (default 20000000), `memory_limit_mb` (default 512) |
| `src/map/itemdb.{hpp,cpp}` | `item_data` script fields -> two int refs; `itemdb_readdb` (:189-190) compiles via `lua_compile_item_script`; failure = startup fatal |
| `src/map/quest.{hpp,cpp}` | drop vestigial script includes |
| `src/map/globals.{hpp,cpp}` | per section 2.3 |
| `src/ast/item.{cpp,hpp}`, `item_test.cpp` | Lua brace scanner (`lex_lua_body`); `ScriptBody` -> text + span; tests for `{p:heal(15, 0, 1)}`, `{}`, `{t = "}"}` |
| `src/ast/fwd.hpp` | drop `script`/`npc` forward decls |
| `src/mmo/strs.*`, `src/high/extract_mmo.*`, `tools/protocol.py` | unchanged (`NpcEvent`, `ScriptLabel` kept) |

### CMake / packaging

* `find_package(Lua 5.4 REQUIRED)` (works with Fedora's lua-devel and Debian's
  liblua5.4-dev); `target_include_directories` and `target_link_libraries(tmwa-map
  ${LUA_LIBRARIES})`.
* New OBJECT library `tmwa-map-lib` = map sources minus `main.cpp`
  (`list(FILTER ... EXCLUDE REGEX ".*/main\\.cpp")`); `tmwa-map` = `main.cpp` + objects;
  map test executables (`src/map/*_test.cpp`) link `tmwa-map-lib + tmwa-core +
  ${LUA_LIBRARIES} + gtest`.
* Option `TMWA_VENDOR_LUA` (debug/CI): build the bundled Lua 5.4 sources as C++ and link
  them instead of the system library.
* ctest additions: unit tests, `tools/check-lua-compat.py`, optional e2e
  (`TMWA_E2E_TESTS`).
* Dockerfile/README: Lua 5.4 dependency.

---------------------------------------------------------------------------------------------------

## 13. Startup and shutdown order

```
do_init:
  lua_init()                              // first: item_db compiles chunks during config parsing
  [config parsing: const_db -> constants + params set; item_db -> item chunk refs;
   npc: lines -> npc_srcs list]
  map_set_logfile(); map_readallmap()
  do_init_chrif(); do_init_clif(); do_init_mob2()
                                          // --check-scripts: clif does not listen, chrif does not connect
  mapreg_init()                           // load mapreg.txt, start the 10 s autosave (was do_init_script)
  lua_create_item_dialog_npc()
  runflag &= lua_load_content()           // each npc_srcs entry: import(file); any error -> false
  do_init_pc(); do_init_party()
  lua_run_oninit()                        // every on_init in definition order; then start the 1 s clock
                                          // timer; then new-global writes start warning
  if (g_check_only) exit(0 or 1)
term_func:
  ... map cleanup, sessions (map_quit -> lua_session_detach),
      map_removenpc (npc_free -> lua_npc_detach) ...
  mapreg_final()                          // save if dirty
  lua_final()                             // release command callbacks, close_state; after mapreg save
  do_final_itemdb() ...                   // refs are dead; itemdb only resets ints
```

---------------------------------------------------------------------------------------------------

## 14. Testing strategy

### 14.1 Unit tests (googletest, linking tmwa-map-lib)

* `lua-engine_test.cpp`: state init/teardown; sandbox escapes (`load`, `require`, `io`,
  `os.execute`, `dofile` absent; constant assignment raises; `collectgarbage("stop")`
  raises); `check_int` policy (3 and 3.0 accepted, 3.5 raises, int32 bounds, "3" as an int
  argument raises); string round trips (embedded NUL rejected, long strings, VString<23>
  limit at `check_event`); instruction budget aborts an infinite loop and the state stays
  usable; memory cap produces LUA_ERRMEM and the server survives; after every scenario
  `g_live_refs == 0` and the registry size is back to the baseline.
* `lua-dialog_test.cpp`: fake session + one `npc.script` on a stub map; scripted sequences:
  click -> mes -> next -> resume(NEXT) -> menu -> resume(MENU 2) -> close; wrong-kind resume
  ignored; menu out-of-range ignored; 0xff ends; table-form `cancel` returns the value;
  0x0146 during NEXT/MENU abandons; variadic menu stops at the first empty string; resume
  for a wrong npc id ignored; abandon mid-menu releases the thread ref; `self:destroy()`
  from its own handler (abandon_pending path); logout mid-dialog; NPC freed mid-dialog;
  event queued while mid-dialog fires after END; `close2 -> openstorage` ordering; nested
  storage-close resume blocked by `running`; input negative terminates with
  `clif_tradecancelled`; `stop()` sends close when a mes was shown; auto-close on fall-off.
* `lua-vars_test.cpp`: `p.vars` read/write/delete/caps/31-char limit/quest alias (fixture
  quest db); `p.acc`/`p.acc2` prefix handling and the accountreg flatten contract;
  `world` int/string/index bounds/dirty flag; mapreg save/load round trip byte-identical to
  a fixture `mapreg.txt`; typed defaults of `p.tmp`/`p.tmpstr`/`worldtmp`.
* `lua-timer_test.cpp`: NPC timer arithmetic incl. the `initnpctimer` quirk, interval
  ordering, stop-inside-handler, callback release on `npc_free`, player slots full.
* `lua-array_test.cpp`: `setarray`/`cleararray`/`getarraysize`/`array_search`/`explode`
  against a table of cases derived from the old engine, on plain tables and on
  `world.array` proxies.
* `item_test.cpp`: the brace scanner matrix.

### 14.2 End-to-end harness (`tools/e2e/`)

Python 3, stdlib only; packet layouts derived from `tools/protocol.py`. `server.py` starts
tmwa-login/char/map with a generated temp world dir (minimal conf, a small fixture `.wlk`
map, fixture dbs, pre-seeded account). `client.py` implements login -> char select -> map
connect -> LoadEndAck and the dialog verbs (`click_npc`, `next`, `menu`, `input`,
`input_str`, `close`, `say`, `expect`).

Scenarios (pytest): the full dialog flow against a sample NPC; menu cancel; wrong-order
packets ignored; 0x0146 mid-menu frees the player (walk accepted afterwards); storage
open/close resume; `@command` chat -> registered handler output; command mid-dialog abandons
the dialog and runs; mob spawn/kill death event; item use script that opens a dialog
(#itemdialog gates: walking blocked while open); reconnect mid-dialog; equip-script perf
smoke (100 simulated players, calcstatus churn stays under budget); `--check-scripts` exit
codes on a good and a broken tree.

### 14.3 `tmwa-map --check-scripts`

Full startup through `lua_run_oninit()` with no listening sockets and no chrif connect,
then exit: 0 if no error, 1 with all errors printed. **`on_init` runs by default** (it is
load-bearing: `server.registercmd`, puppet creation, config NPCs; and D1 makes on_init
errors fatal anyway); `--check-scripts=load` skips on_init for a faster syntax-only pass.
Converter `PORTME()` stubs are written so that loading succeeds but calling raises, and
`--check-scripts` reports their count without failing; this is the porting agents' inner
loop. Requires the real `world/map` working directory (conf paths are relative).

---------------------------------------------------------------------------------------------------

## 15. Tooling

### 15.1 Converter `tools/lua-port/convert-npc-data.py`

Deterministic and idempotent (re-running rewrites only stubs still marked PORTME). Reads the
old tree via the conf chain:

* Data entries are fully converted, no stubs: `warp` -> `npc.warp{...}` (file xs/ys numbers
  kept), `shop` -> `npc.shop{...}` (price forms kept), `monster` -> `npc.monster{...}`,
  `mapflag` -> `npc.mapflag{...}`.
* `script`/`function` bodies become bootable stubs: `on_click = function(self, p) PORTME()
  end` plus one stub per detected label pre-split into the right field (`on_touch`,
  `on_init`, `on_timer[N]`, `events.OnX`), with the ORIGINAL source embedded verbatim in a
  `--[==[ ... ]==]` block (fence level raised if the body contains it). `function|script|X`
  -> `function X(self, p) PORTME() end`. `PORTME` is a global that raises at call time, so a
  half-ported tree boots and `--check-scripts` still passes.
* Emits `-- AUDIT:` comments wherever its lexer sees a construct on the behaviour-change
  list: `sc_start` with a literal tick < 1000, `mobcount` comparisons, `getarraysize` uses,
  empty menu entries, `& | ^ << >>` expressions (precedence), `fakenpcname`, dialog
  primitives inside broadcast/timer labels.
* Rewrites `_import.txt` conf files and `scripts.conf` (`npc:` lines -> `.lua`).
* Seeds `funcdefs.tsv`: per callfunc function, the `@` variables it reads/writes (input for
  the porting guide's signature appendix).

### 15.2 Lint `tools/lua-port/lint.py`

Per file: `luac -p` (syntax); token scan for the forbidden subset (`goto`, `::`, native
bitwise operators, `//`, `<const>`, `<close>`, `require|load|loadfile|dofile|io\.|debug\.|
package\.`, `os.` beyond time/clock/date, `continue` as identifier); undefined-global check
(free reads/writes from `luac -l` GETTABUP/SETTABUP on _ENV) against the API surface
(`tmwa-map --dump-lua-api`), the const_db names, and content-defined globals (two passes);
`PORTME` report; warnings for tables indexed with both 0 and 1 literals in one file
(mixed-base hazard) and for handle-typed values assigned into vars tables where detectable.

### 15.3 `tools/check-lua-compat.py`

Greps `src/map/lua-*.cpp` (excluding `lua-compat.hpp`) for the forbidden Lua API list of
section 10; wired into ctest.
