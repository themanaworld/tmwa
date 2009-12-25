#ifndef MAGIC_H_
#define MAGIC_H_

#include "clif.h"
#include "intif.h"

#define MAGIC_CONFIG_FILE "conf/magic.conf"

typedef struct map_session_data character_t;
typedef struct block_list entity_t;

struct invocation;              /* Spell invocation */

/**
 * Try to cast magic.
 *
 * As an intended side effect, the magic message may be distorted (text only).
 *
 * \param caster Player attempting to cast magic
 * \param spell The prospective incantation
 * \param spell_len Number of characters in the incantation
 * \return 1 or -1 if the input message was magic and was handled by this function, 0 otherwise.  -1 is returned when the
 *         message should not be repeated.
 */
int  magic_message (character_t * caster, char *spell, size_t spell_len);

/**
 * Removes the shroud from a character
 *
 * \param character The character to remove the shroud from
 */
void magic_unshroud (character_t * character);

/**
 * Notifies a running spell that a status_change timer triggered by the spell has expired
 *
 * \param invocation The invocation to notify
 * \param bl_id ID of the PC for whom this happened
 * \param type sc_id ID of the status change entry that finished
 * \param supplanted Whether the status_change finished normally (0) or was supplanted by a new status_change (1)
 */
void
spell_effect_report_termination (int invocation, int bl_id, int sc_id,
                                 int supplanted);

/**
 * Initialise all spells, read config data
 */
void do_init_magic ();

/**
 * Identifies the invocation used to trigger a spell
 *
 * Returns NULL if not found
 */
char *magic_find_invocation (char *spellame);

/**
 * Identifies the invocation used to denote a teleport location
 *
 * Returns NULL if not found
 */
char *magic_find_anchor_invocation (char *teleport_location);

/**
 * Execute a spell invocation and sets up timers to finish
 */
void spell_execute (struct invocation *invocation);

/**
 * Continue an NPC script embedded in a spell
 */
void spell_execute_script (struct invocation *invocation);

/**
 * Stops all magic bound to the specified character
 *
 */
void magic_stop_completely (character_t * c);

/**
 * Attacks with a magical spell charged to the character
 *
 * Returns 0 if there is no charged spell or the spell is depleted.
 */
int  spell_attack (int caster, int target);

void spell_free_invocation (struct invocation *invocation);

#endif /* !defined(MAGIC_H_) */
