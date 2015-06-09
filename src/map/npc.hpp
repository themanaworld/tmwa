#pragma once
//    npc.hpp - Noncombatants.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <cstdint>

#include "../range/slice.hpp"

#include "../net/timer.t.hpp"

#include "map.hpp"
#include "script-call.t.hpp"


namespace tmwa
{
namespace map
{
constexpr BlockId START_NPC_NUM = wrap<BlockId>(110000000);

// TODO make these species, see npc_class in npc_data
constexpr Species WARP_CLASS = wrap<Species>(45);
constexpr Species FAKE_NPC_CLASS = wrap<Species>(127);
constexpr Species INVISIBLE_CLASS = wrap<Species>(32767);

int npc_event_dequeue(dumb_ptr<map_session_data> sd);
int npc_event(dumb_ptr<map_session_data> sd, NpcEvent npcname, int);
int npc_addeventtimer(dumb_ptr<block_list> bl, interval_t tick, NpcEvent name);
int npc_touch_areanpc(dumb_ptr<map_session_data>, Borrowed<map_local>, int, int);
int npc_click(dumb_ptr<map_session_data>, BlockId);
int npc_scriptcont(dumb_ptr<map_session_data>, BlockId);
int npc_buysellsel(dumb_ptr<map_session_data>, BlockId, int);
int npc_buylist(dumb_ptr<map_session_data>, const std::vector<Packet_Repeat<0x00c8>>&);
int npc_selllist(dumb_ptr<map_session_data>, const std::vector<Packet_Repeat<0x00c9>>&);

int npc_enable(NpcName name, bool flag);
dumb_ptr<npc_data> npc_name2id(NpcName name);

BlockId npc_get_new_npc_id(void);

int magic_message(dumb_ptr<map_session_data> caster, XString source_invocation);
/**
 * Uninstalls and frees an NPC
 */
void npc_free(dumb_ptr<npc_data> npc);

int npc_event_do_oninit(void);

int npc_event_doall_l(ScriptLabel name, BlockId rid, Slice<argrec_t> argv);
int npc_event_do_l(NpcEvent name, BlockId rid, Slice<argrec_t> argv);
inline
int npc_event_doall(ScriptLabel name)
{
    return npc_event_doall_l(name, BlockId(), nullptr);
}
inline
int npc_event_do(NpcEvent name)
{
    return npc_event_do_l(name, BlockId(), nullptr);
}

void npc_timerevent_start(dumb_ptr<npc_data_script> nd);
void npc_timerevent_stop(dumb_ptr<npc_data_script> nd);
interval_t npc_gettimerevent_tick(dumb_ptr<npc_data_script> nd);
void npc_settimerevent_tick(dumb_ptr<npc_data_script> nd, interval_t newtimer);
int npc_delete(dumb_ptr<npc_data> nd);
} // namespace map
} // namespace tmwa
