#include "lua-callback.hpp"
//    lua-callback.cpp - LuaCallback operations and the live-reference counter.
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

#include "lua-engine.hpp"
#include "lua-events.hpp"
#include "lua-internal.hpp"
#include "lua-value.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
static
int g_live_refs = 0;

int lua_live_refs()
{
    return g_live_refs;
}

LuaCallback lua_cb_named(NpcEvent ev)
{
    LuaCallback cb;
    cb.event = ev;
    return cb;
}

LuaCallback lua_cb_from_stack(lua_State* L, int idx)
{
    // longjmp discipline: LuaCallback is trivially destructible, and the
    // only raising call (check_event) happens before any reference is taken
    LuaCallback cb;
    if (lua_isfunction(L, idx))
    {
        lua_pushvalue(L, idx);
        cb.fn_ref = luac::ref(L);
        cb.self_npc = lua_current_ctx().npc;
        ++g_live_refs;
        return cb;
    }
    cb.event = check_event(L, idx);
    return cb;
}

LuaCallback lua_cb_dup(const LuaCallback& cb)
{
    LuaCallback copy = cb;
    if (cb.fn_ref != lua_noref)
    {
        lua_State* L = lua_state();
        luac::push_ref(L, cb.fn_ref);
        copy.fn_ref = luac::ref(L);
        ++g_live_refs;
    }
    return copy;
}

void lua_cb_release(LuaCallback& cb)
{
    if (cb.fn_ref != lua_noref)
    {
        luac::unref(lua_state(), cb.fn_ref);
        --g_live_refs;
    }
    cb = LuaCallback();
}

void lua_cb_release_ref(lua_State* L, int fn_ref)
{
    if (fn_ref != lua_noref)
    {
        luac::unref(L, fn_ref);
        --g_live_refs;
    }
}

void lua_cb_fire(LuaCallback cb, dumb_ptr<map_session_data> sd, LuaArgs args)
{
    if (cb.fn_ref != lua_noref)
    {
        // ownership of fn_ref (and its live-ref count) transfers to
        // lua_fire_fn_ref, which unrefs via lua_cb_release_ref
        int fn_ref = cb.fn_ref;
        cb.fn_ref = lua_noref;
        lua_fire_fn_ref(fn_ref, cb.self_npc, sd, args);
        return;
    }
    NpcEvent ev = cb.event;
    if (bool(ev))
        lua_npc_event(sd, ev, args);
}
} // namespace map
} // namespace tmwa
