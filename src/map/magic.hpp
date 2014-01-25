#ifndef MAGIC_HPP
#define MAGIC_HPP

# include "../strings/fwd.hpp"

# include "../common/dumb_ptr.hpp"

# include "map.hpp"
# include "skill.t.hpp"

# define MAGIC_CONFIG_FILE "conf/magic.conf"

struct invocation;              /* Spell invocation */

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
void spell_effect_report_termination(int invocation, int bl_id,
        StatusChange sc_id, int supplanted);

/**
 * Identifies the invocation used to trigger a spell
 *
 * Returns empty string if not found
 */
FString magic_find_invocation(XString spellname);

/**
 * Identifies the invocation used to denote a teleport location
 *
 * Returns empty string if not found
 */
FString magic_find_anchor_invocation(XString teleport_location);

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
int spell_attack(int caster, int target);

void spell_free_invocation(dumb_ptr<invocation> invocation);

#endif // MAGIC_HPP
