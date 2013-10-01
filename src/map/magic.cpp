#include <cstring>

#include "../common/cxxstdio.hpp"

#include "magic-interpreter.hpp"

#include "pc.hpp"

#include "magic-expr.hpp"
#include "magic-interpreter-base.hpp"
#include "magic-interpreter-lexer.hpp"
#include "src/map/magic-interpreter-parser.hpp"
#include "magic-stmt.hpp"
#include "magic.hpp"

#include "../poison.hpp"

#undef DEBUG

/// Return a pair of strings, {spellname, parameter}
/// Parameter may be empty.
static
std::pair<XString, XString> magic_tokenise(XString src)
{
    auto seeker = std::find(src.begin(), src.end(), ' ');

    if (seeker == src.end())
    {
        return {src, XString()};
    }
    else
    {
        XString rv1 = src.xislice_h(seeker);
        ++seeker;

        while (seeker != src.end() && *seeker == ' ')
            ++seeker;

        // Note: this very well could be empty
        XString rv2 = src.xislice_t(seeker);
        return {rv1, rv2};
    }
}

int magic_message(dumb_ptr<map_session_data> caster, XString source_invocation)
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
    XString spell_invocation = pair.first;
    XString parameter = pair.second;

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
