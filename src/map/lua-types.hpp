#pragma once
//    lua-types.hpp - plain structs shared between the Lua engine and host headers.
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

// No Lua includes here: this header is included by map.hpp and other host
// headers. Registry references are plain ints; lua_noref mirrors LUA_NOREF
// (static_assert'd in lua-compat.hpp).

#include "fwd.hpp"

#include <cstdint>

#include "../strings/rstring.hpp"
#include "../strings/zstring.hpp"

#include "../net/timer.t.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"


namespace tmwa
{
namespace map
{
// mirrors LUA_NOREF without including Lua headers
constexpr int lua_noref = -2;

// The prompt outstanding on a suspended dialog coroutine, i.e. which client
// answer packet is expected next (doc/lua-engine.md section 2.1).
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

// Per-session engine state, lives on map_session_data as `sd->lua`.
struct LuaSession
{
    int serial = 0;                // bumped on attach; stale handles carry an older value
    int handle_ref = lua_noref;    // registry ref of the player handle table (owner: session)
    int thread_ref = lua_noref;    // registry ref of the dialog coroutine (owner: session)
    LuaPrompt prompt = LuaPrompt::NONE;
    int menu_count = 0;            // entries of the outstanding menu; selectable set kept Lua-side
    bool dialog_mes = false;       // a mes was sent since the last prompt/END (old state.npc_dialog_mes)
    bool running = false;          // currently inside lua_resume (re-entrancy guard)
    bool abandon_pending = false;  // abandon requested while running; driver finishes it
    bool in_calcstatus = false;    // equip scripts may call p:bonus freely while set
};

// The client's answer to an outstanding prompt. Trivially destructible.
struct LuaAnswer
{
    int menu = 0;
    int amount = 0;
    ZString str;
};

// A named event or a Lua function reference, plus the NPC pushed as `self`
// for the function form. Ownership discipline: doc/lua-engine.md section 2.4.
struct LuaCallback
{
    NpcEvent event;           // named form {npc, label}; npc may be "" (broadcast) or start with '~'
    int fn_ref = lua_noref;   // function form: registry ref (owner: this value's holder)
    BlockId self_npc;         // NPC pushed as `self` for function-form callbacks (may be 0)

    explicit operator bool() const
    {
        NpcEvent ev = event;  // NpcEvent::operator bool is not const
        return bool(ev) || fn_ref != lua_noref;
    }
};

// One player or NPC event-timer slot (replaces the NpcEvent-based eventtimer).
struct LuaTimerSlot
{
    Timer timer;
    LuaCallback cb;
};

// Event-specific extra arguments carried from C++ into a handler's `args`
// table (doc/lua-api.md section 10). SCAFFOLD NOTE: the design documents
// leave the C++ shape of LuaArgs open; this tagged struct covers every args
// table the API defines. `empty()` drives the queue rule (engine-supplied
// extras count as args, EXCEPT the per-spawn mob death info, which queued in
// the old engine and still does: MOB_DEATH is therefore "empty").
struct LuaArgs
{
    enum class Kind : uint8_t
    {
        NONE,        // args = nil
        TARGET_ID,   // { target_id = i1 }   (foreach, overrideattack)
        KILL,        // { victimrid = i1 }   (OnPCKillEvent)
        MOBKILL,     // { mobID = i1, mobX = i2, mobY = i3 } (OnMobKillEvent)
        ARGSTRING,   // the argument string (registered commands)
        ITEM_USE,    // { itemId = i1 }
        ITEM_EQUIP,  // { itemId = i1, slotId = i2 }
    };
    Kind kind = Kind::NONE;
    int i1 = 0, i2 = 0, i3 = 0;
    RString str;

    bool empty() const
    {
        return kind == Kind::NONE;
    }

    static LuaArgs none()
    {
        return LuaArgs();
    }
    static LuaArgs target(BlockId id)
    {
        LuaArgs a;
        a.kind = Kind::TARGET_ID;
        a.i1 = static_cast<int>(unwrap<BlockId>(id));
        return a;
    }
    static LuaArgs kill(BlockId victim)
    {
        LuaArgs a;
        a.kind = Kind::KILL;
        a.i1 = static_cast<int>(unwrap<BlockId>(victim));
        return a;
    }
    static LuaArgs mobkill(int species, int x, int y)
    {
        LuaArgs a;
        a.kind = Kind::MOBKILL;
        a.i1 = species;
        a.i2 = x;
        a.i3 = y;
        return a;
    }
    static LuaArgs argstring(RString s)
    {
        LuaArgs a;
        a.kind = Kind::ARGSTRING;
        a.str = s;
        return a;
    }
    static LuaArgs item_use(int item_id)
    {
        LuaArgs a;
        a.kind = Kind::ITEM_USE;
        a.i1 = item_id;
        return a;
    }
    static LuaArgs item_equip(int item_id, int slot_id)
    {
        LuaArgs a;
        a.kind = Kind::ITEM_EQUIP;
        a.i1 = item_id;
        a.i2 = slot_id;
        return a;
    }
};
} // namespace map
} // namespace tmwa
