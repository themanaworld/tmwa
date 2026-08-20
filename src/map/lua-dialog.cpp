#include "lua-dialog.hpp"
//    lua-dialog.cpp - the per-session dialog state machine.
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
#include <map>

#include "../strings/astring.hpp"
#include "../strings/literal.hpp"
#include "../strings/mstring.hpp"

#include "../io/cxxstdio.hpp"

#include "clif.hpp"
#include "globals.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "storage.hpp"
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
// ------------------------------------------------------------------------
// module state

// the engine-owned #itemdialog NPC (doc/lua-engine.md section 4.3)
static
dumb_ptr<npc_data_script> g_itemdialog_npc;

// session serial source (doc/lua-engine.md section 2.1)
static
int g_session_serial = 0;

// "log once per (session, kind)" bookkeeping for mismatched answer packets
// (doc/lua-engine.md section 4 step 8). Keyed by block id; reset when the
// session serial changes; erased on detach.
struct MismatchLog
{
    int serial = 0;
    unsigned mask = 0;
};
static
std::map<uint32_t, MismatchLog> g_mismatch_logged;

static
bool mismatch_log_once(dumb_ptr<map_session_data> sd, LuaPrompt kind)
{
    MismatchLog& ml = g_mismatch_logged[unwrap<BlockId>(sd->bl_id)];
    if (ml.serial != sd->lua.serial)
    {
        ml.serial = sd->lua.serial;
        ml.mask = 0;
    }
    unsigned bit = 1u << static_cast<unsigned>(kind);
    if (ml.mask & bit)
        return false;
    ml.mask |= bit;
    return true;
}

// ------------------------------------------------------------------------
// helpers

// The close packet of the old builtin_close: a Close button when text was
// shown since the last prompt, else npc_action 5 (close without a window).
static
void send_close_packet(dumb_ptr<map_session_data> sd)
{
    if (sd->lua.dialog_mes)
        clif_scriptclose(sd, sd->npc_id);
    else
        clif_npc_action(sd, sd->npc_id, 5, 0, 0, 0);
}

// Close and unref the session's coroutine and reset the prompt state.
// No packets, no npc_event_dequeue: that is lua_dialog_end's job.
static
void dialog_teardown(dumb_ptr<map_session_data> sd)
{
    LuaSession& ls = sd->lua;
    lua_State* L = lua_state();
    if (ls.thread_ref != lua_noref && L != nullptr)
    {
        luac::push_ref(L, ls.thread_ref);
        lua_State* T = lua_tothread(L, -1);
        if (T != nullptr)
        {
            luac::close_thread(T, L);
            lua_thread_ctx_clear(T);
        }
        lua_pop(L, 1);
        luac::unref(L, ls.thread_ref);
    }
    ls.thread_ref = lua_noref;
    ls.prompt = LuaPrompt::NONE;
    ls.dialog_mes = false;
    ls.menu_count = 0;
    ls.abandon_pending = false;
}

void lua_dialog_end(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    dialog_teardown(sd);
    // clears npc_id and schedules one queued event 100 ms later, exactly as
    // the old engine did
    npc_event_dequeue(sd);
}

void lua_dialog_abandon(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    if (sd->lua.thread_ref == lua_noref)
        return;
    if (sd->lua.running)
    {
        // never close the thread under the interpreter's feet; the driver
        // finishes the abandon after resume returns
        sd->lua.abandon_pending = true;
        return;
    }
    dialog_teardown(sd);
}

// ------------------------------------------------------------------------
// shared post-resume handling (doc/lua-engine.md section 3 step 5)

// had_prompt: a prompt was outstanding when this resume cycle started (true
// on the lua_dialog_resume path, false on the initial lua_run_dialog run);
// after an error it decides whether the client is left with an open,
// buttonless window that needs a close packet.
static
void dialog_after_resume(dumb_ptr<map_session_data> sd, int status,
        bool had_prompt)
{
    LuaSession& ls = sd->lua;
    lua_State* L = lua_state();

    if (status == LUA_YIELD)
    {
        if (ls.abandon_pending)
        {
            // npc_event_dequeue already ran (that is what set the flag):
            // just finish the abandon, no packets, no second dequeue
            dialog_teardown(sd);
            return;
        }
        if (ls.prompt == LuaPrompt::NONE)
        {
            // a raw coroutine.yield() reached the dialog driver: no
            // primitive sent a packet, so the client would wait forever
            // and every later dialog packet would be ignored
            lua_warn("dialog driver: raw coroutine.yield from a dialog "
                    "handler (dialog aborted)"_s);
            if (ls.dialog_mes || had_prompt)
                clif_scriptclose(sd, sd->npc_id);
            lua_dialog_end(sd);
            return;
        }
        if (ls.prompt != LuaPrompt::CLOSE)
            // the yielding primitive sent its packet and set the prompt;
            // wait for the client
            return;
        // p:close(), p:shop(), CLOSE yield: finished
        lua_dialog_end(sd);
        return;
    }

    if (status == LUA_OK)
    {
        if (ls.abandon_pending)
        {
            dialog_teardown(sd);
            return;
        }
        // handler fell off the end with a dialog open: automatic close
        if (ls.dialog_mes)
            clif_scriptclose(sd, sd->npc_id);
        lua_dialog_end(sd);
        return;
    }

    // error: the error object sits on top of the coroutine's stack
    bool is_stop = false;
    if (ls.thread_ref != lua_noref && L != nullptr)
    {
        luac::push_ref(L, ls.thread_ref);
        lua_State* T = lua_tothread(L, -1);
        lua_pop(L, 1);
        if (T != nullptr)
        {
            if (lua_error_is_stop(T, -1))
            {
                lua_pop(T, 1);
                is_stop = true;
            }
            else
            {
                LuaCtx ctx;
                if (!lua_thread_ctx_get(T, &ctx))
                {
                    ctx.npc = sd->npc_id;
                    ctx.player = sd->bl_id;
                    ctx.what = "dialog";
                }
                lua_report_error(T, ctx);
            }
        }
    }
    if (ls.abandon_pending)
    {
        dialog_teardown(sd);
        return;
    }
    if (is_stop)
    {
        // stop() behaves like a normal return
        if (ls.dialog_mes)
            clif_scriptclose(sd, sd->npc_id);
    }
    else
    {
        // do not leave the client buttonless
        if (ls.dialog_mes || had_prompt)
            clif_scriptclose(sd, sd->npc_id);
    }
    lua_dialog_end(sd);
}

void lua_dialog_after_resume(dumb_ptr<map_session_data> sd, int status)
{
    if (sd == nullptr)
        return;
    dialog_after_resume(sd, status, false);
}

// ------------------------------------------------------------------------
// answer parsing: requestitem

// Parse "id[,amount];id[,amount];..." like the old builtin_requestitem:
// ids < 1 are skipped, ids not present in the inventory are skipped, and in
// the names form ids whose item name is empty are skipped; at most max
// entries are accepted. Returns the number of accepted ids in out[].
static
int requestitem_collect(dumb_ptr<map_session_data> sd, ZString str, int max,
        bool names, int* out)
{
    int n = 0;
    XString rest = str;
    while (n < max)
    {
        auto semi = std::find(rest.begin(), rest.end(), ';');
        XString seg = rest.xislice_h(semi);
        XString val = seg.xislice_h(std::find(seg.begin(), seg.end(), ','));
        // atoi semantics without the poisoned c_str dance: optional spaces
        // and sign, then digits
        int num = 0;
        {
            auto it = val.begin();
            while (it != val.end() && (*it == ' ' || *it == '\t'))
                ++it;
            bool neg = false;
            if (it != val.end() && (*it == '-' || *it == '+'))
            {
                neg = (*it == '-');
                ++it;
            }
            while (it != val.end() && *it >= '0' && *it <= '9')
            {
                if (num < 100000000)
                    num = num * 10 + (*it - '0');
                ++it;
            }
            if (neg)
                num = -num;
        }
        if (num >= 1 && num <= 65535)
        {
            ItemNameId nameid = wrap<ItemNameId>(static_cast<uint16_t>(num));
            bool have = false;
            for (IOff0 i : IOff0::iter())
                if (sd->status.inventory[i].nameid == nameid)
                {
                    have = true;
                    break;
                }
            if (have && names)
            {
                ItemName name = itemdb_search(nameid)->name;
                if (!name.size())
                    have = false;
            }
            if (have)
                out[n++] = num;
        }
        if (semi == rest.end())
            break;
        rest = rest.xislice_t(semi + 1);
    }
    return n;
}

// ------------------------------------------------------------------------
// resume (doc/lua-engine.md section 4)

bool lua_dialog_resume(dumb_ptr<map_session_data> sd, BlockId npc_id,
        LuaPrompt kind, const LuaAnswer& a)
{
    if (sd == nullptr)
        return false;
    LuaSession& ls = sd->lua;

    // 1. the storage re-entrancy path (p:warp -> pc_setpos ->
    //    storage_storageclose while the handler runs)
    if (ls.running)
    {
        PRINTF("lua: dialog resume while the handler is running (ignored)\n"_fmt);
        return false;
    }
    // 2. stale or forged packet
    if (npc_id != sd->npc_id)
        return false;
    // 3. npc_id set transiently by the shop-click path
    if (ls.thread_ref == lua_noref)
        return false;
    // 4. the NPC was freed
    dumb_ptr<npc_data> nd = map_id_is_npc(sd->npc_id);
    if (nd == nullptr)
    {
        npc_event_dequeue(sd);
        return false;
    }
    // 5. distance (INVISIBLE_CLASS passes: this is what makes #itemdialog
    //    work); the dialog stays, as old
    if (npc_checknear(sd, sd->npc_id))
    {
        clif_scriptclose(sd, sd->npc_id);
        return false;
    }
    // 6. NPC on its way out
    if (nd->deletion_pending != npc_data::NOT_DELETING)
    {
        clif_scriptclose(sd, sd->npc_id);
        npc_event_dequeue(sd);
        return false;
    }
    // 7. orphan puppet
    if (nd->npc_subtype == NpcSubtype::SCRIPT)
    {
        dumb_ptr<npc_data_script> nds = nd->is_script();
        if (nds->scr.parent && map_id2bl(nds->scr.parent) == nullptr)
        {
            npc_free(nd);
            return false;
        }
    }

    // 8. kind check against the outstanding prompt
    LuaPrompt prompt = ls.prompt;
    bool accepted = false;
    switch (prompt)
    {
        case LuaPrompt::NEXT:
            accepted = (kind == LuaPrompt::NEXT);
            break;
        case LuaPrompt::MENU:
            accepted = (kind == LuaPrompt::MENU);
            break;
        case LuaPrompt::INPUT_INT:
            accepted = (kind == LuaPrompt::INPUT_INT);
            break;
        case LuaPrompt::INPUT_STR:
        case LuaPrompt::REQUESTITEM:
        case LuaPrompt::REQUESTLANG:
            accepted = (kind == LuaPrompt::INPUT_STR);
            break;
        case LuaPrompt::CLOSE2:
            // old: close2 resumed on 0x0146 or 0x00b9
            accepted = (kind == LuaPrompt::CLOSE2 || kind == LuaPrompt::NEXT);
            break;
        case LuaPrompt::STORAGE:
            accepted = (kind == LuaPrompt::STORAGE);
            break;
        case LuaPrompt::CLOSE:
        case LuaPrompt::NONE:
            // ignore
            return false;
    }
    if (!accepted)
    {
        // a Close click (0x0146) while NEXT/MENU/INPUT_INT/INPUT_STR/
        // REQUESTITEM/REQUESTLANG is outstanding abandons the dialog: the
        // client already closed the window (doc/lua-api.md 5.3)
        if (kind == LuaPrompt::CLOSE2 && prompt != LuaPrompt::STORAGE)
        {
            lua_dialog_end(sd);
            return false;
        }
        if (mismatch_log_once(sd, kind))
            PRINTF("lua: dialog answer kind %d does not match prompt %d (player %d, ignored)\n"_fmt,
                    static_cast<int>(kind), static_cast<int>(prompt),
                    unwrap<BlockId>(sd->bl_id));
        return false;
    }

    // per-kind answer validation before anything is committed
    bool menu_cancel = false;
    if (prompt == LuaPrompt::MENU)
    {
        int count = ls.menu_count < 0 ? -ls.menu_count : ls.menu_count;
        bool cancelable = ls.menu_count < 0;
        if (a.menu == 0xff)
        {
            if (!cancelable)
            {
                // old client cancel: END silently, no packet
                lua_dialog_end(sd);
                return false;
            }
            menu_cancel = true;
        }
        else if (a.menu < 1 || a.menu > count)
        {
            // out of range or hidden by the empty-string cut: the prompt
            // stays outstanding
            lua_warn(STRPRINTF("menu answer %d out of range 1..%d (ignored)"_fmt,
                        a.menu, count));
            return false;
        }
    }
    if (prompt == LuaPrompt::INPUT_INT && a.amount < 0)
    {
        // old behaviour: a negative amount cancels the pending trade-style
        // input, closes the dialog and ends the script
        clif_tradecancelled(sd);
        send_close_packet(sd);
        lua_dialog_end(sd);
        return false;
    }

    // 9. push the answer onto the coroutine
    lua_State* L = lua_state();
    if (L == nullptr)
        return false;
    ls.prompt = LuaPrompt::NONE;
    int req_max = ls.menu_count < 0 ? -ls.menu_count : ls.menu_count;
    bool req_names = ls.menu_count < 0;
    ls.menu_count = 0;

    luac::push_ref(L, ls.thread_ref);   // the resume holder
    lua_State* T = lua_tothread(L, -1);
    if (T == nullptr)
    {
        lua_pop(L, 1);
        dialog_teardown(sd);
        return false;
    }

    int nargs = 0;
    switch (prompt)
    {
        case LuaPrompt::MENU:
            if (menu_cancel)
                lua_pushnil(T);     // the prelude maps nil to tbl.cancel
            else
                lua_pushinteger(T, a.menu);
            nargs = 1;
            break;
        case LuaPrompt::INPUT_INT:
            lua_pushinteger(T, a.amount);
            nargs = 1;
            break;
        case LuaPrompt::INPUT_STR:
        case LuaPrompt::REQUESTLANG:
            luac::push_string(T, a.str);
            nargs = 1;
            break;
        case LuaPrompt::REQUESTITEM:
        {
            int ids[16];
            int n = requestitem_collect(sd, a.str, req_max, req_names, ids);
            lua_createtable(T, n, 0);
            for (int j = 0; j < n; ++j)
            {
                if (req_names)
                {
                    ItemName name = itemdb_search(
                            wrap<ItemNameId>(static_cast<uint16_t>(ids[j])))->name;
                    luac::push_string(T, name);
                }
                else
                    lua_pushinteger(T, ids[j]);
                lua_rawseti(T, -2, j + 1);
            }
            nargs = 1;
            break;
        }
        case LuaPrompt::NEXT:
        case LuaPrompt::CLOSE2:
        case LuaPrompt::STORAGE:
        case LuaPrompt::CLOSE:
        case LuaPrompt::NONE:
            nargs = 0;
            break;
    }

    // 10. resume with budget and re-pushed ctx
    LuaCtx ctx;
    if (!lua_thread_ctx_get(T, &ctx))
    {
        ctx.npc = sd->npc_id;
        ctx.player = sd->bl_id;
        ctx.what = "dialog";
    }
    int nres = 0;
    ls.running = true;
    lua_ctx_push(ctx);
    budget_push();
    int status = luac::resume(T, L, nargs, &nres);
    budget_pop();
    lua_ctx_pop();
    ls.running = false;
    if (status == LUA_YIELD && nres > 0)
        lua_pop(T, nres);   // yield values are not used

    dialog_after_resume(sd, status, true);
    lua_pop(L, 1);          // the resume holder
    return true;
}

// ------------------------------------------------------------------------
// session lifecycle

void lua_session_attach(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    sd->lua = LuaSession();
    sd->lua.serial = ++g_session_serial;
    lua_State* L = lua_state();
    if (L == nullptr)
        return;
    // creates the handle table and caches it (session handle_ref + the
    // players registry); lua-handle-player.cpp owns the details
    lua_push_player_handle(L, sd);
    lua_pop(L, 1);
}

void lua_session_detach(dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return;
    lua_dialog_abandon(sd);
    // release every queued event callback and the attack-spell override
    lua_event_queue_clear(sd);
    lua_cb_release(sd->magic_attack);
    g_mismatch_logged.erase(unwrap<BlockId>(sd->bl_id));
    lua_State* L = lua_state();
    if (L != nullptr)
    {
        // drop the players-registry entry (blockid -> handle)
        lua_push_engine_table(L, LuaTable::PLAYERS);
        lua_pushinteger(L, unwrap<BlockId>(sd->bl_id));
        lua_pushnil(L);
        lua_rawset(L, -3);
        lua_pop(L, 1);
        if (sd->lua.handle_ref != lua_noref)
            luac::unref(L, sd->lua.handle_ref);
    }
    sd->lua.handle_ref = lua_noref;
}

// ------------------------------------------------------------------------
// the #itemdialog NPC (doc/lua-engine.md section 4.3)

void lua_create_item_dialog_npc()
{
    if (g_itemdialog_npc != nullptr)
        return;
    dumb_ptr<npc_data_script> nd;
    nd.new_();
    nd->scr.event_needs_map = false;
    nd->name = stringish<NpcName>("#itemdialog"_s);
    nd->sex = SEX::UNSPECIFIED;
    nd->bl_prev = nd->bl_next = nullptr;
    nd->bl_m = borrow(undefined_gat);
    nd->bl_x = 0;
    nd->bl_y = 0;
    nd->bl_id = npc_get_new_npc_id();
    nd->dir = DIR::S;
    nd->flag = 0;
    nd->sit = DamageType::STAND;
    nd->npc_class = INVISIBLE_CLASS;
    nd->speed = 200_ms;
    nd->option = Opt0::ZERO;
    nd->opt1 = Opt1::ZERO;
    nd->opt2 = Opt2::ZERO;
    nd->opt3 = Opt3::ZERO;
    nd->deletion_pending = npc_data::NOT_DELETING;
    nd->bl_type = BL::NPC;
    nd->npc_subtype = NpcSubtype::SCRIPT;
    // findable by id (npc_checknear, packet round trips) but deliberately
    // NOT register_npc_name'd and NOT in the engine NPC registries: npc.get
    // never returns it and broadcasts never see it
    map_addiddb(nd);
    g_itemdialog_npc = nd;
}

dumb_ptr<npc_data> lua_item_dialog_npc()
{
    return g_itemdialog_npc;
}

// ------------------------------------------------------------------------
// the yielding bindings (doc/lua-api.md 5.3)

// True when L is sd's dialog coroutine, currently inside a resume.
static
bool in_dialog_context(lua_State* L, dumb_ptr<map_session_data> sd)
{
    if (sd == nullptr)
        return false;
    LuaSession& ls = sd->lua;
    if (!ls.running || ls.thread_ref == lua_noref)
        return false;
    lua_State* M = lua_state();
    luac::push_ref(M, ls.thread_ref);
    lua_State* T = lua_tothread(M, -1);
    lua_pop(M, 1);
    return T == L;
}

// Common prologue of every yielding primitive. May raise: call it before any
// non-trivial local exists.
static
dumb_ptr<map_session_data> check_dialog_player(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr || !in_dialog_context(L, sd))
    {
        luaL_error(L, "dialog primitive outside a dialog context");
        return nullptr;    // unreachable
    }
    return sd;
}

// One p:mes argument: a string (kept verbatim) or an integer (formatted %d).
// Raises for anything else; call while locals are trivial.
static
void send_mes_arg(lua_State* L, dumb_ptr<map_session_data> sd, int idx)
{
    if (lua_type(L, idx) == LUA_TNUMBER)
    {
        int v = check_int(L, idx);
        AString t = STRPRINTF("%d"_fmt, v);
        clif_scriptmes(sd, sd->npc_id, t);
        return;
    }
    ZString s = check_string(L, idx);
    clif_scriptmes(sd, sd->npc_id, s);
}

static
int lp_mes(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    int top = lua_gettop(L);
    if (top == 1)
    {
        // p:mes(): one blank line
        clif_scriptmes(sd, sd->npc_id, ""_s);
        sd->lua.dialog_mes = true;
        return 0;
    }
    for (int i = 2; i <= top; ++i)
        send_mes_arg(L, sd, i);
    sd->lua.dialog_mes = true;
    return 0;
}

static
int lp_mesq(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    if (lua_type(L, 2) == LUA_TNUMBER)
    {
        int v = check_int(L, 2);
        AString t = STRPRINTF("\"%d\""_fmt, v);
        clif_scriptmes(sd, sd->npc_id, t);
    }
    else
    {
        ZString s = check_string(L, 2);
        AString t = STRPRINTF("\"%s\""_fmt, s);
        clif_scriptmes(sd, sd->npc_id, t);
    }
    sd->lua.dialog_mes = true;
    return 0;
}

static
int lp_mesn(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    if (lua_gettop(L) >= 2 && !lua_isnil(L, 2))
    {
        ZString s = check_string(L, 2);
        AString t = STRPRINTF("[%s]"_fmt, s);
        clif_scriptmes(sd, sd->npc_id, t);
        sd->lua.dialog_mes = true;
        return 0;
    }
    // default: the dialog NPC's basename; inside an item-use dialog (the
    // #itemdialog NPC) the name is required
    if (g_itemdialog_npc != nullptr && sd->npc_id == g_itemdialog_npc->bl_id)
        return luaL_error(L, "mesn: a name is required in an item dialog");
    dumb_ptr<npc_data> nd = map_id_is_npc(sd->npc_id);
    if (nd == nullptr)
        return luaL_error(L, "mesn: no NPC attached");
    {
        NpcName name = nd->name;
        AString base = AString(name.xislice_h(
                std::find(name.begin(), name.end(), '#')));
        AString t = STRPRINTF("[%s]"_fmt, base);
        clif_scriptmes(sd, sd->npc_id, t);
    }
    sd->lua.dialog_mes = true;
    return 0;
}

static
int lp_clear(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    clif_npc_action(sd, sd->npc_id, 9, 0, 0, 0);
    return 0;
}

static
int lp_title(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    ZString s = check_string(L, 2);
    if (sd == nullptr)
        return 0;
    clif_npc_send_title(sd->sess, sd->npc_id, s);
    return 0;
}

static
int lp_next(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    clif_scriptnext(sd, sd->npc_id);
    sd->lua.prompt = LuaPrompt::NEXT;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_close(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    send_close_packet(sd);
    sd->lua.prompt = LuaPrompt::CLOSE;
    // the driver treats a CLOSE yield as END: this never returns
    return lua_yield(L, 0);
}

static
int lp_close2(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    send_close_packet(sd);
    sd->lua.prompt = LuaPrompt::CLOSE2;
    // reset so a later p:close() sends npc_action 5 (no window), preserving
    // the old close2; openstorage; sequence exactly
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

// variadic p:menu(s1, s2, ...): the list stops at the first empty string
// (old behaviour: empty choices hide the rest); hidden entries are NOT
// selectable; cancel (0xff) ends the handler like p:close()
static
int lp_menu_variadic(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    int top = lua_gettop(L);
    // pass 1: validate and count with trivial locals only
    int nsent = 0;
    for (int i = 2; i <= top; ++i)
    {
        if (lua_type(L, i) == LUA_TNUMBER)
        {
            check_int(L, i);
            ++nsent;
            continue;
        }
        ZString s = check_string(L, i);   // raises on nil and non-strings
        if (!s.size())
            break;
        ++nsent;
    }
    if (nsent == 0)
        return luaL_error(L, "menu: no selectable entries");
    // pass 2: build and send "a:b:c:"
    {
        MString buf;
        for (int i = 2; i < 2 + nsent; ++i)
        {
            if (lua_type(L, i) == LUA_TNUMBER)
                buf += STRPRINTF("%d"_fmt, check_int(L, i));
            else
            {
                size_t len;
                buf += XString(luac::to_string(L, i, &len));
            }
            buf += ':';
        }
        AString display = AString(buf);
        clif_scriptmenu(sd, sd->npc_id, display);
    }
    sd->lua.menu_count = nsent;             // positive: not cancelable
    sd->lua.prompt = LuaPrompt::MENU;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

// the C menu primitive used by the prelude's table form:
// cmenu(p, display, nsent, cancelable) -> sent position, or nil on a
// cancelable cancel; a non-cancelable cancel ends the handler (no return)
static
int lp_menu_prim(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    ZString display = check_string(L, 2);
    int nsent = check_int(L, 3);
    bool cancelable = lua_toboolean(L, 4);
    if (nsent < 1)
        return luaL_error(L, "menu: no selectable entries");
    clif_scriptmenu(sd, sd->npc_id, display);
    // the cancelable flag rides on menu_count's sign; the selectable count
    // is its magnitude (the value-mapping tables live in the prelude frame)
    sd->lua.menu_count = cancelable ? -nsent : nsent;
    sd->lua.prompt = LuaPrompt::MENU;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_input(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    clif_scriptinput(sd, sd->npc_id);
    sd->lua.prompt = LuaPrompt::INPUT_INT;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_input_str(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    clif_scriptinputstr(sd, sd->npc_id);
    sd->lua.prompt = LuaPrompt::INPUT_STR;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_requestitem(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    int amount = opt_int(L, 2, 1);
    if (amount < 1)
        amount = 1;
    if (amount > 16)
        amount = 16;
    bool names = false;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3))
        names = check_bool(L, 3);
    clif_scriptinputstr(sd, sd->npc_id);
    clif_npc_action(sd, sd->npc_id, 10, amount, 0, 0);
    // request parameters ride on menu_count until the answer arrives:
    // magnitude = max accepted entries, sign = the names flag
    sd->lua.menu_count = names ? -amount : amount;
    sd->lua.prompt = LuaPrompt::REQUESTITEM;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_requestlang(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    clif_npc_action(sd, sd->npc_id, 0, 0, 0, 0);
    clif_scriptinputstr(sd, sd->npc_id);
    sd->lua.prompt = LuaPrompt::REQUESTLANG;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_shop(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    ZString name = check_string(L, 2);
    dumb_ptr<npc_data> shop_nd = npc_name2id(stringish<NpcName>(name));
    if (shop_nd == nullptr)
    {
        lua_warn(STRPRINTF("shop: no such npc: %s"_fmt, name));
        return 0;
    }
    // old builtin_shop: close first (END + close packet), then open the
    // buy/sell window of the named NPC
    send_close_packet(sd);
    clif_npcbuysell(sd, shop_nd->bl_id);
    sd->lua.prompt = LuaPrompt::CLOSE;
    // CLOSE yield: the driver ENDs the dialog; this never returns
    return lua_yield(L, 0);
}

static
int lp_openstorage(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_dialog_player(L);
    if (storage_storageopen(sd) != 0)
    {
        // refused (already open, storage not loaded yet, ...): no yield
        lua_pushboolean(L, 0);
        return 1;
    }
    sd->npc_flags.storage = 1;
    sd->lua.prompt = LuaPrompt::STORAGE;
    sd->lua.dialog_mes = false;
    return lua_yield(L, 0);
}

static
int lp_npcaction(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    int cmd = check_int(L, 2);
    int id = 0;
    if (lua_gettop(L) >= 3 && !lua_isnil(L, 3))
    {
        switch (lua_type(L, 3))
        {
            case LUA_TNUMBER:
                id = check_int(L, 3);
                break;
            case LUA_TTABLE:
                id = static_cast<int>(unwrap<BlockId>(lua_check_handle_id(L, 3)));
                break;
            default:
            {
                // an NPC name (the old cmd-2 form); unknown names log
                // instead of the old null deref
                ZString s = check_string(L, 3);
                dumb_ptr<npc_data> target = npc_name2id(stringish<NpcName>(s));
                if (target == nullptr)
                {
                    if (sd != nullptr)
                        lua_warn(STRPRINTF("npcaction: no such npc: %s"_fmt, s));
                    return 0;
                }
                id = static_cast<int>(unwrap<BlockId>(target->bl_id));
                break;
            }
        }
    }
    int x = opt_int(L, 4, 0);
    int y = opt_int(L, 5, 0);
    if (sd == nullptr)
        return 0;
    clif_npc_action(sd, sd->npc_id, static_cast<short>(cmd), id,
            static_cast<short>(x), static_cast<short>(y));
    return 0;
}

static
int lp_camera(lua_State* L)
{
    dumb_ptr<map_session_data> sd = check_player(L, 1);
    if (sd == nullptr)
        return 0;
    int nargs = lua_gettop(L) - 1;
    if (nargs == 0)
    {
        // p:camera(): restore
        clif_npc_action(sd, sd->npc_id, 3, 0, 0, 0);
        return 0;
    }
    if (nargs == 2 && lua_type(L, 2) == LUA_TNUMBER
            && lua_type(L, 3) == LUA_TNUMBER)
    {
        // p:camera(x, y)
        int x = check_int(L, 2);
        int y = check_int(L, 3);
        clif_npc_action(sd, sd->npc_id, 2, 0,
                static_cast<short>(x), static_cast<short>(y));
        return 0;
    }
    // p:camera(actor [, dx, dy])
    int dx = opt_int(L, 3, 0);
    int dy = opt_int(L, 4, 0);
    short cmd = 2;
    int id = 0;
    switch (lua_type(L, 2))
    {
        case LUA_TNUMBER:
            id = check_int(L, 2);
            break;
        case LUA_TTABLE:
            id = static_cast<int>(unwrap<BlockId>(lua_check_handle_id(L, 2)));
            break;
        default:
        {
            ZString s = check_string(L, 2);
            if (s == "relative"_s)
                cmd = 4;
            else if (s == "rid"_s || s == "player"_s)
                id = static_cast<int>(unwrap<BlockId>(sd->bl_id));
            else if (s == "oid"_s || s == "npc"_s)
            {
                BlockId oid = sd->npc_id;
                if (!oid)
                    oid = lua_current_ctx().npc;
                id = static_cast<int>(unwrap<BlockId>(oid));
            }
            else
            {
                dumb_ptr<npc_data> target = npc_name2id(stringish<NpcName>(s));
                if (target == nullptr)
                {
                    lua_warn(STRPRINTF("camera: no such npc: %s"_fmt, s));
                    return 0;
                }
                id = static_cast<int>(unwrap<BlockId>(target->bl_id));
            }
            break;
        }
    }
    clif_npc_action(sd, sd->npc_id, cmd, id,
            static_cast<short>(dx), static_cast<short>(dy));
    return 0;
}

// Pushes prelude.menu_table; an upvalue-free C helper so the dispatcher can
// look it up lazily (the prelude loads after the handle metatables are
// built).
static
int lp_get_menu_table(lua_State* L)
{
    lua_push_engine_table(L, LuaTable::PRELUDE);
    lua_getfield(L, -1, "menu_table");
    lua_remove(L, -2);
    return 1;
}

// p:menu must dispatch to the prelude's table form through pure Lua frames
// (a C trampoline would block the yield), so the registered method is a Lua
// closure built here.
static
const char menu_dispatch_src[] = R"lua(
local cmenu_v, cmenu, get_menu_table, type = ...
return function(p, x, ...)
    if type(x) == "table" then
        return get_menu_table()(p, x, cmenu)
    end
    return cmenu_v(p, x, ...)
end
)lua";

void lua_dialog_register_methods(lua_State* L, int methods_idx)
{
    int m = lua_absindex(L, methods_idx);

    static const luaL_Reg dialog_funcs[] =
    {
        { "mes",         lp_mes         },
        { "mesq",        lp_mesq        },
        { "mesn",        lp_mesn        },
        { "clear",       lp_clear       },
        { "title",       lp_title       },
        { "next",        lp_next        },
        { "close",       lp_close       },
        { "close2",      lp_close2      },
        { "input",       lp_input       },
        { "input_str",   lp_input_str   },
        { "requestitem", lp_requestitem },
        { "requestlang", lp_requestlang },
        { "shop",        lp_shop        },
        { "openstorage", lp_openstorage },
        { "npcaction",   lp_npcaction   },
        { "camera",      lp_camera      },
        { nullptr,       nullptr        },
    };
    luac::register_funcs(L, m, dialog_funcs);

    // build the menu dispatcher closure
    ZString src = ZString(menu_dispatch_src,
            menu_dispatch_src + sizeof(menu_dispatch_src) - 1, nullptr);
    if (!luac::load_chunk(L, "=dialog:menu"_s, src))
    {
        size_t len;
        ZString err = luac::to_string(L, -1, &len);
        PRINTF("lua: internal error: menu dispatcher does not compile: %s\n"_fmt,
                AString(XString(err)));
        lua_pop(L, 1);
        return;
    }
    luac::push_cfunction(L, lp_menu_variadic, "p.menu.variadic");
    luac::push_cfunction(L, lp_menu_prim, "p.menu.cmenu");
    luac::push_cfunction(L, lp_get_menu_table, "p.menu.get_menu_table");
    luac::push_globals(L);
    lua_getfield(L, -1, "type");
    lua_remove(L, -2);
    if (lua_pcall(L, 4, 1, 0) != LUA_OK)
    {
        size_t len;
        ZString err = luac::to_string(L, -1, &len);
        PRINTF("lua: internal error: menu dispatcher failed: %s\n"_fmt,
                AString(XString(err)));
        lua_pop(L, 1);
        return;
    }
    lua_setfield(L, m, "menu");
}
} // namespace map
} // namespace tmwa
