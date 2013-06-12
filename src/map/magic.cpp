#include <cstring>

#include "../common/cxxstdio.hpp"

#include "magic-interpreter.hpp"
#include "pc.hpp"

#include "../poison.hpp"

#undef DEBUG

/// Return a pair of strings, {spellname, parameter}
/// Parameter may be empty.
static
std::pair<std::string, std::string> magic_tokenise(std::string src)
{
    std::string retval = std::move(src);
    const std::string::iterator rvb = retval.begin(), rve = retval.end();
    std::string::iterator seeker = std::find(rvb, rve, ' ');

    if (seeker == retval.end())
    {
        return {retval, std::string()};
    }
    else
    {
        std::string rv1(rvb, seeker);
        ++seeker;

        while (seeker != rve && *seeker == ' ')
            ++seeker;

        // Note: this very well could be empty
        std::string rv2(seeker, rve);
        return {rv1, rv2};
    }
}

int magic_message(dumb_ptr<map_session_data> caster, const std::string& source_invocation)
{
    if (pc_isdead(caster))
        return 0;

    int power = caster->matk1;

    // This was the only thing worth saving from magic_preprocess_message.
    // May it rest only, and never rise again.
    // For more information on how this code worked, travel through time
    // and watch all the comments I wrote for myself while trying to figure
    // out if it was safe to delete.
    if (caster->state.shroud_active && caster->state.shroud_disappears_on_talk)
        magic_unshroud(caster);

    auto pair = magic_tokenise(source_invocation);
    std::string spell_invocation = std::move(pair.first);
    std::string parameter = std::move(pair.second);

    dumb_ptr<spell_t> spell = magic_find_spell(spell_invocation);

    if (spell)
    {
        int near_miss;
        dumb_ptr<env_t> env =
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
            dumb_ptr<invocation> invocation = spell_instantiate(effects, env);

            spell_bind(caster, invocation);
            spell_execute(invocation);

            return bool(spell->flags & SPELL_FLAG::SILENT) ? -1 : 1;
        }
        else
            magic_free_env(env);

        return 1;
    }

    return 0;                   /* Not a spell */
}

void do_init_magic(void)
{
    magic_init(MAGIC_CONFIG_FILE);
}
