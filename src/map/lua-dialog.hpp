#pragma once
//    lua-dialog.hpp - the per-session dialog state machine.
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

#include "../generic/dumb_ptr.hpp"

#include "../mmo/ids.hpp"

#include "lua-types.hpp"


namespace tmwa
{
namespace map
{
// Resume the suspended dialog coroutine of sd with the client's answer.
// Called from the clif packet handlers and storage_storageclose; performs the
// full validation ladder of doc/lua-engine.md section 4 (wrong npc_id, wrong
// prompt kind, checknear, deletion_pending, ...). Returns false when nothing
// was resumed.
bool lua_dialog_resume(dumb_ptr<map_session_data> sd, BlockId npc_id,
        LuaPrompt kind, const LuaAnswer& a);

// Drop the session's dialog coroutine without packets. While the coroutine is
// running this only sets abandon_pending; the driver ends the thread after
// resume returns. Called from npc_event_dequeue.
void lua_dialog_abandon(dumb_ptr<map_session_data> sd);

// Shared post-resume handling for both drivers (doc/lua-engine.md section 3
// step 5): yield-with-prompt keeps the thread; yield-with-CLOSE, normal
// return, stop() and errors END the dialog (close packet when text was
// shown).
void lua_dialog_after_resume(dumb_ptr<map_session_data> sd, int status);

// END: close and unref the thread, reset prompt state, npc_event_dequeue.
void lua_dialog_end(dumb_ptr<map_session_data> sd);

// Session lifecycle: attach creates the player handle (bumps serial), called
// from pc_authok; detach abandons the dialog, releases queued callbacks and
// the handle, called from map_quit.
void lua_session_attach(dumb_ptr<map_session_data> sd);
void lua_session_detach(dumb_ptr<map_session_data> sd);

// The engine-owned invisible #itemdialog NPC (doc/lua-engine.md section 4.3):
// created once in do_init after maps are loaded; item use scripts run their
// dialogs attached to it.
void lua_create_item_dialog_npc();
dumb_ptr<npc_data> lua_item_dialog_npc();

// Register the yielding dialog bindings (mes, mesq, mesn, clear, next, close,
// close2, menu, input, input_str, requestitem, requestlang, title, shop,
// openstorage, npcaction, camera) into the player-handle methods table at
// methods_idx. Called by the handle setup in lua_init.
void lua_dialog_register_methods(lua_State* L, int methods_idx);
} // namespace map
} // namespace tmwa
