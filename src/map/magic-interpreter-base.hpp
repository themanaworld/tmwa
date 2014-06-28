#pragma once
//    magic-interpreter-base.hpp - Core of the old magic system.
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

#include "fwd.hpp"

#include "../strings/fwd.hpp"

#include "../generic/fwd.hpp"

#include "../mmo/fwd.hpp"


namespace tmwa
{
extern magic_conf_t magic_conf; /* Global magic conf */
extern env_t magic_default_env; /* Fake default environment */

/**
 * Adds a component selection to a component holder (which may initially be nullptr)
 */
void magic_add_component(dumb_ptr<component_t> *component_holder, ItemNameId id, int count);

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

dumb_ptr<teleport_anchor_t> magic_find_anchor(XString name);

dumb_ptr<env_t> spell_create_env(magic_conf_t *conf, dumb_ptr<spell_t> spell,
        dumb_ptr<map_session_data> caster, int spellpower, XString param);

void magic_free_env(dumb_ptr<env_t> env);

/**
 * near_miss is set to nonzero iff the spell only failed due to ephemereal issues (spell delay in effect, out of mana, out of components)
 */
effect_set_t *spell_trigger(dumb_ptr<spell_t> spell,
        dumb_ptr<map_session_data> caster,
        dumb_ptr<env_t> env, int *near_miss);

dumb_ptr<invocation> spell_instantiate(effect_set_t *effect, dumb_ptr<env_t> env);

/**
 * Bind a spell to a subject (this is a no-op for `local' spells).
 */
void spell_bind(dumb_ptr<map_session_data> subject, dumb_ptr<invocation> invocation);

// 1 on failure
int spell_unbind(dumb_ptr<map_session_data> subject, dumb_ptr<invocation> invocation);

/**
 * Clones a spell to run the at_effect field
 */
dumb_ptr<invocation> spell_clone_effect(dumb_ptr<invocation> source);

dumb_ptr<spell_t> magic_find_spell(XString invocation);

void spell_update_location(dumb_ptr<invocation> invocation);
} // namespace tmwa
