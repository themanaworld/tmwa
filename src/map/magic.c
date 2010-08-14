#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "magic-interpreter.h"

#undef DEBUG

static char *magic_preprocess_message (character_t * character, char *start,
                                       char *end)
{
    if (character->state.shroud_active
        && character->state.shroud_disappears_on_talk)
        magic_unshroud (character);

    if (character->state.shroud_active
        && character->state.shroud_hides_name_talking)
    {
        int  len = strlen (end);
        strcpy (start, "? ");
        memmove (start + 2, end, len + 1);
        return start + 4;
    }
    else
        return end + 2;         /* step past blank */
}

#define ISBLANK(c) ((c) == ' ')

/* Returns a dynamically allocated copy of `src'.
 * `*parameter' may point within that copy or be NULL. */
static char *magic_tokenise (char *src, char **parameter)
{
    char *retval = strdup (src);
    char *seeker = retval;

    while (*seeker && !ISBLANK (*seeker))
        ++seeker;

    if (!*seeker)
        *parameter = NULL;
    else
    {
        *seeker = 0;            /* Terminate invocation */
        ++seeker;

        while (ISBLANK (*seeker))
            ++seeker;

        *parameter = seeker;
    }

    return retval;
}

int magic_message (character_t * caster, char *spell_, size_t spell_len)
{
    if (pc_isdead (caster))
        return 0;

    int  power = caster->matk1;
    char *invocation_base = spell_ + 8;
    char *source_invocation =
        1 + invocation_base + strlen (caster->status.name);
    spell_t *spell;
    char *parameter;
    char *spell_invocation;

    if (!source_invocation)
        return 0;

    /* Pre-message filter in case some spell alters output */
    source_invocation =
        magic_preprocess_message (caster, invocation_base, source_invocation);

    spell_invocation = magic_tokenise (source_invocation, &parameter);
    parameter = parameter ? strdup (parameter) : strdup ("");

    spell = magic_find_spell (spell_invocation);
    free (spell_invocation);

    if (spell)
    {
        int  near_miss;
        env_t *env =
            spell_create_env (&magic_conf, spell, caster, power, parameter);
        effect_set_t *effects;

        if ((spell->flags & SPELL_FLAG_NONMAGIC) || (power >= 1))
            effects = spell_trigger (spell, caster, env, &near_miss);
        else
            effects = NULL;

#ifdef DEBUG
        fprintf (stderr, "Found spell `%s', triggered = %d\n", spell_,
                 effects != NULL);
#endif
        if (caster->status.option & OPTION_HIDE)
            return 0;           // No spellcasting while hidden

        MAP_LOG_PC (caster, "CAST %s %s",
                    spell->name, effects ? "SUCCESS" : "FAILURE");

        if (effects)
        {
            invocation_t *invocation = spell_instantiate (effects, env);

            spell_bind (caster, invocation);
            spell_execute (invocation);

            return (spell->flags & SPELL_FLAG_SILENT) ? -1 : 1;
        }
        else
            magic_free_env (env);

        return 1;
    }
    else
        free (parameter);

    return 0;                   /* Not a spell */
}

int  magic_init (char *conffile);   // must be called after itemdb initialisation

void do_init_magic ()
{
    magic_init (MAGIC_CONFIG_FILE);
}
