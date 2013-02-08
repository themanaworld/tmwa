#include <cstring>

#include "../common/cxxstdio.hpp"

#include "magic-interpreter.hpp"
#include "pc.hpp"

#include "../poison.hpp"

#undef DEBUG

static
char *magic_preprocess_message(character_t *character, char *start,
                                       char *end)
{
    if (character->state.shroud_active
        && character->state.shroud_disappears_on_talk)
        magic_unshroud(character);

    if (character->state.shroud_active
        && character->state.shroud_hides_name_talking)
    {
        int len = strlen(end);
        strcpy(start, "? ");
        memmove(start + 2, end, len + 1);
        return start + 4;
    }
    else
        return end + 2;         /* step past blank */
}

/* Returns a dynamically allocated copy of `src'.
 * `*parameter' may point within that copy or be NULL. */
static
char *magic_tokenise(char *src, char **parameter)
{
    char *retval = strdup(src);
    char *seeker = retval;

    while (*seeker && *seeker != ' ')
        ++seeker;

    if (!*seeker)
        *parameter = NULL;
    else
    {
        *seeker = 0;            /* Terminate invocation */
        ++seeker;

        while (*seeker == ' ')
            ++seeker;

        *parameter = seeker;
    }

    return retval;
}

int magic_message(character_t *caster, char *spell_, size_t)
{
    if (pc_isdead(caster))
        return 0;

    int power = caster->matk1;
    char *invocation_base = spell_ + 8;
    char *source_invocation =
        1 + invocation_base + strlen(caster->status.name);
    spell_t *spell;
    char *parameter;
    char *spell_invocation;

    if (!source_invocation)
        return 0;

    /* Pre-message filter in case some spell alters output */
    source_invocation =
        magic_preprocess_message(caster, invocation_base, source_invocation);

    spell_invocation = magic_tokenise(source_invocation, &parameter);
    parameter = parameter ? strdup(parameter) : strdup("");

    spell = magic_find_spell(spell_invocation);
    free(spell_invocation);

    if (spell)
    {
        int near_miss;
        env_t *env =
            spell_create_env(&magic_conf, spell, caster, power, parameter);
        effect_set_t *effects;

        if (bool(spell->flags & SPELL_FLAG::NONMAGIC) || (power >= 1))
            effects = spell_trigger(spell, caster, env, &near_miss);
        else
            effects = NULL;

#ifdef DEBUG
        FPRINTF(stderr, "Found spell `%s', triggered = %d\n", spell_,
                 effects != NULL);
#endif
        if (bool(caster->status.option & Option::HIDE))
            return 0;           // No spellcasting while hidden

        MAP_LOG_PC(caster, "CAST %s %s",
                    spell->name, effects ? "SUCCESS" : "FAILURE");

        if (effects)
        {
            invocation_t *invocation = spell_instantiate(effects, env);

            spell_bind(caster, invocation);
            spell_execute(invocation);

            return bool(spell->flags & SPELL_FLAG::SILENT) ? -1 : 1;
        }
        else
            magic_free_env(env);

        return 1;
    }
    else
        free(parameter);

    return 0;                   /* Not a spell */
}

void do_init_magic(void)
{
    magic_init(MAGIC_CONFIG_FILE);
}
