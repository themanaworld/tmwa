#pragma once
//    npc-parse.hpp - Noncombatants.
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


namespace tmwa
{
int npc_parse_warp(XString w1, XString, NpcName w3, XString w4);

/**
 * Spawns and installs a talk-only NPC
 *
 * \param message The message to speak.  If message is nullptr, the NPC will not do anything at all.
 */
dumb_ptr<npc_data> npc_spawn_text(Borrowed<map_local> m, int x, int y,
        Species class_, NpcName name, AString message);

void npc_addsrcfile(AString name);
void npc_delsrcfile(XString name);
bool do_init_npc(void);
} // namespace tmwa
