#include "magic.hpp"
//    magic.cpp - Entry to the magic system.
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

#include <algorithm>
#include <utility>

#include "../strings/xstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/cxxstdio.hpp"

#include "magic-expr.hpp"
#include "magic-interpreter.hpp"
#include "magic-interpreter-base.hpp"
#include "magic-stmt.hpp"
#include "map.hpp"
#include "pc.hpp"

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
    if (bool(caster->status.option & Option::HIDE))
        return 0;           // No spellcasting while hidden

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
        FPRINTF(stderr, "Found spell `%s', triggered = %d\n"_fmt, spell_,
                 effects != NULL);
#endif

        MAP_LOG_PC(caster, "CAST %s %s"_fmt,
                    spell->name, effects ? "SUCCESS"_s : "FAILURE"_s);

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
