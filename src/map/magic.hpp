#ifndef TMWA_MAP_MAGIC_HPP
#define TMWA_MAP_MAGIC_HPP
//    magic.hpp - Entry to the magic system.
//
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

# include "fwd.hpp"

# include "../strings/fwd.hpp"

# include "../mmo/dumb_ptr.hpp"

# include "map.t.hpp"
# include "skill.t.hpp"

/**
 * Try to cast magic.
 *
 * As an intended side effect, the magic message may be distorted (text only).
 * No, it can't. Thank God.
 *
 * \param caster Player attempting to cast magic
 * \param source_invocation The prospective incantation
 * \return 1 or -1 if the input message was magic and was handled by this function, 0 otherwise.  -1 is returned when the
 *         message should not be repeated.
 */
int magic_message(dumb_ptr<map_session_data> caster, XString source_invocation);

/**
 * Removes the shroud from a character
 *
 * \param character The character to remove the shroud from
 */
void magic_unshroud(dumb_ptr<map_session_data> character);

/**
 * Notifies a running spell that a status_change timer triggered by the spell has expired
 *
 * \param invocation The invocation to notify
 * \param bl_id ID of the PC for whom this happened
 * \param sc_id ID of the status change entry that finished
 * \param supplanted Whether the status_change finished normally (0) or was supplanted by a new status_change (1)
 */
void spell_effect_report_termination(BlockId invocation, BlockId bl_id,
        StatusChange sc_id, int supplanted);

/**
 * Identifies the invocation used to trigger a spell
 *
 * Returns empty string if not found
 */
AString magic_find_invocation(XString spellname);

/**
 * Identifies the invocation used to denote a teleport location
 *
 * Returns empty string if not found
 */
AString magic_find_anchor_invocation(XString teleport_location);

/**
 * Execute a spell invocation and sets up timers to finish
 */
void spell_execute(dumb_ptr<invocation> invocation);

/**
 * Continue an NPC script embedded in a spell
 */
void spell_execute_script(dumb_ptr<invocation> invocation);

/**
 * Stops all magic bound to the specified character
 *
 */
void magic_stop_completely(dumb_ptr<map_session_data> c);

/**
 * Attacks with a magical spell charged to the character
 *
 * Returns 0 if there is no charged spell or the spell is depleted.
 */
int spell_attack(BlockId caster, BlockId target);

void spell_free_invocation(dumb_ptr<invocation> invocation);

#endif // TMWA_MAP_MAGIC_HPP
