#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "magic-interpreter.h"

#undef DEBUG

static char *
magic_preprocess_message(character_t *character, char *start, char *end)
{
        if (character->state.shroud_active
            && character->state.shroud_disappears_on_talk)
                magic_unshroud(character);

        if (character->state.shroud_active
            && character->state.shroud_hides_name_talking) {
                int len = strlen(end);
                strcpy(start, "? ");
                memmove(start + 2, end, len + 1);
                return start + 4;
        } else
                return end + 2; /* step past blank */
}

#define ISBLANK(c) ((c) == ' ')

/* Returns a dynamically allocated copy of `src'.
 * `*parameter' may point within that copy or be NULL. */
static char *
magic_tokenise(char *src, char **parameter)
{
        char *retval = strdup(src);
        char *seeker = retval;

        while (*seeker && !ISBLANK(*seeker))
                ++seeker;

        if (!*seeker)
                *parameter = NULL;
        else {
            *seeker = 0; /* Terminate invocation */
            ++seeker;

            while (ISBLANK (*seeker))
                    ++seeker;

            *parameter = seeker;
        }

        return retval;
}

int
magic_message(character_t *caster,
              char *spell_, size_t spell_len)
{
        int power = caster->status.base_level + caster->status.int_;
        char *invocation_base = spell_ + 8;
        char *source_invocation = strchr(invocation_base, ':');
        spell_t *spell;
        char *parameter;
        char *spell_invocation;

        if (!source_invocation)
                return 0;

        /* Pre-message filter in case some spell alters output */
        source_invocation = magic_preprocess_message(caster, invocation_base, source_invocation);

        spell_invocation = magic_tokenise(source_invocation, &parameter);
        parameter = parameter ? strdup(parameter) : strdup("");

        spell = magic_find_spell(spell_invocation);
        free(spell_invocation);

        if (spell) {
                env_t *env = spell_create_env(&magic_conf, spell, caster, power, parameter);
                effect_set_t *effects = spell_trigger(spell, caster, env);

#ifdef DEBUG
                fprintf(stderr, "Found spell `%s', triggered = %d\n", spell_, effects != NULL);
#endif

                if (effects) {
                        invocation_t *invocation = spell_instantiate(effects, env);

                        /* We have a proper spell effect-- obscure the invocation! */
                        while (*source_invocation) {
                                if (((rand() * 100.0) / (RAND_MAX * 1.0)) < magic_conf.obscure_chance)
                                        *source_invocation = '*';
                                ++source_invocation;
                        }

                        spell_bind(caster, invocation);
                        spell_execute(invocation);

                        return (spell->flags & SPELL_FLAG_SILENT)? -1 : 1;
                } else {
                        magic_free_env(env);
                }
                return 0;
        }

        return 0; /* Not a spell */
}

int
magic_init(char *conffile); // must be called after itemdb initialisation

void
do_init_magic()
{
        magic_init(MAGIC_CONFIG_FILE);
}
