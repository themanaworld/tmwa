#include "magic-interpreter-base.hpp"
//    magic-interpreter-base.cpp - Core of the old magic system.
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

#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/cxxstdio_enums.hpp"

#include "../net/timer.hpp"

#include "magic.hpp"
#include "magic-expr.hpp"
#include "magic-interpreter.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace magic
{
static
void set_int(val_t *v, int i)
{
    *v = ValInt{i};
}

static __attribute__((unused))
void set_dir(val_t *v, DIR d)
{
    *v = ValDir{d};
}


static
void set_string(val_t *v, RString x)
{
    *v = ValString{x};
}

static
void set_entity(val_t *v, dumb_ptr<block_list> e)
{
    *v = ValEntityInt{e->bl_id};
}

static
void set_invocation(val_t *v, dumb_ptr<invocation> i)
{
    *v = ValInvocationInt{i->bl_id};
}

static
void set_spell(val_t *v, dumb_ptr<spell_t> x)
{
    *v = ValSpell{x};
}

magic_conf_t magic_conf;        /* Global magic conf */
env_t magic_default_env = { &magic_conf, nullptr };

AString magic_find_invocation(XString spellname)
{
    auto it = magic_conf.spells_by_name.find(spellname);
    if (it != magic_conf.spells_by_name.end())
        return it->second->invocation;

    return AString();
}

dumb_ptr<spell_t> magic_find_spell(XString invocation)
{
    auto it = magic_conf.spells_by_invocation.find(invocation);
    if (it != magic_conf.spells_by_invocation.end())
        return it->second;

    return nullptr;
}

/* -------------------------------------------------------------------------------- */
/* Spell anchors */
/* -------------------------------------------------------------------------------- */

AString magic_find_anchor_invocation(XString anchor_name)
{
    auto it = magic_conf.anchors_by_name.find(anchor_name);

    if (it != magic_conf.anchors_by_name.end())
        return it->second->invocation;

    return AString();
}

dumb_ptr<teleport_anchor_t> magic_find_anchor(XString name)
{
    auto it = magic_conf.anchors_by_invocation.find(name);
    if (it != magic_conf.anchors_by_invocation.end())
        return it->second;

    return nullptr;
}

/* -------------------------------------------------------------------------------- */
/* Spell guard checks */
/* -------------------------------------------------------------------------------- */

static
dumb_ptr<env_t> alloc_env(magic_conf_t *conf)
{
    auto env = dumb_ptr<env_t>::make();
    env->varu = make_unique<val_t[]>(conf->varv.size());
    env->base_env = conf;
    return env;
}

static
dumb_ptr<env_t> clone_env(dumb_ptr<env_t> src)
{
    dumb_ptr<env_t> retval = alloc_env(src->base_env);

    for (int i = 0; i < src->base_env->varv.size(); i++)
        magic_copy_var(&retval->varu[i], &src->varu[i]);

    return retval;
}

void magic_free_env(dumb_ptr<env_t> env)
{
    for (int i = 0; i < env->base_env->varv.size(); i++)
        magic_clear_var(&env->varu[i]);
    // handled by std::unique_ptr now. Was a memory leak before.
    // delete[] env->vars;
    env.delete_();
}

dumb_ptr<env_t> spell_create_env(magic_conf_t *conf, dumb_ptr<spell_t> spell,
        dumb_ptr<map_session_data> caster, int spellpower, XString param)
{
    dumb_ptr<env_t> env = alloc_env(conf);

    switch (spell->spellarg_ty)
    {

        case SPELLARG::STRING:
            set_string(&env->varu[spell->arg], param);
            break;

        case SPELLARG::PC:
        {
            CharName name = stringish<CharName>(param);
            dumb_ptr<map_session_data> subject = map_nick2sd(name);
            if (!subject)
                subject = caster;
            set_entity(&env->varu[spell->arg], subject);
            break;
        }

        case SPELLARG::NONE:
            break;

        default:
            FPRINTF(stderr, "Unexpected spellarg type %d\n"_fmt,
                    spell->spellarg_ty);
    }

    set_entity(&env->varu[VAR_CASTER], caster);
    set_int(&env->varu[VAR_SPELLPOWER], spellpower);
    set_spell(&env->varu[VAR_SPELL], spell);

    return env;
}

static
void free_components(dumb_ptr<component_t> *component_holder)
{
    if (*component_holder == nullptr)
        return;
    free_components(&(*component_holder)->next);
    (*component_holder).delete_();
    *component_holder = nullptr;
}

void magic_add_component(dumb_ptr<component_t> *component_holder, ItemNameId id, int count)
{
    if (count <= 0)
        return;

    if (*component_holder == nullptr)
    {
        auto component = dumb_ptr<component_t>::make();
        component->next = nullptr;
        component->item_id = id;
        component->count = count;
        *component_holder = component;
    }
    else
    {
        dumb_ptr<component_t> component = *component_holder;
        if (component->item_id == id)
        {
            component->count += count;
            return;
        }
        else
            magic_add_component(&component->next, id, count);
        /* Tail-recurse; gcc can optimise this.  Not that it matters. */
    }
}

static
void copy_components(dumb_ptr<component_t> *component_holder, dumb_ptr<component_t> component)
{
    if (component == nullptr)
        return;

    magic_add_component(component_holder, component->item_id, component->count);
    copy_components(component_holder, component->next);
}

typedef struct spellguard_check
{
    dumb_ptr<component_t> catalysts, components;
    int mana;
    interval_t casttime;
} spellguard_check_t;

static
int check_prerequisites(dumb_ptr<map_session_data> caster, dumb_ptr<component_t> component)
{
    while (component)
    {
        if (pc_count_all_items(caster, component->item_id) < component->count)
            return 0;           /* insufficient */

        component = component->next;
    }

    return 1;
}

static
void consume_components(dumb_ptr<map_session_data> caster, dumb_ptr<component_t> component)
{
    while (component)
    {
        pc_remove_items(caster, component->item_id, component->count);
        component = component->next;
    }
}

static
int spellguard_can_satisfy(spellguard_check_t *check, dumb_ptr<map_session_data> caster,
        dumb_ptr<env_t> env, int *near_miss)
{
    tick_t tick = gettick();

    int retval = check_prerequisites(caster, check->catalysts);

    if (retval && near_miss)
        *near_miss = 1;         // close enough!

    retval = retval && caster->cast_tick <= tick    /* Hasn't cast a spell too recently */
        && check->mana <= caster->status.sp
        && check_prerequisites(caster, check->components);

    if (retval)
    {
        interval_t casttime = check->casttime;

        if (ValInt *v = env->VAR(VAR_MIN_CASTTIME).get_if<ValInt>())
        {
            casttime = std::max(casttime, static_cast<interval_t>(v->v_int));
        }

        caster->cast_tick = tick + casttime;    /* Make sure not to cast too frequently */

        consume_components(caster, check->components);
        pc_heal(caster, 0, -check->mana);
    }

    return retval;
}

static
const effect_set_t *spellguard_check_sub(spellguard_check_t *check,
        dumb_ptr<spellguard_t> guard,
        dumb_ptr<map_session_data> caster,
        dumb_ptr<env_t> env,
        int *near_miss)
{
    if (guard == nullptr)
        return nullptr;

    MATCH (*guard)
    {
        CASE (const GuardCondition&, s)
        {
            if (!magic_eval_int(env, s.s_condition))
                return nullptr;
        }
        CASE (const GuardComponents&, s)
        {
            copy_components(&check->components, s.s_components);
        }
        CASE (const GuardCatalysts&, s)
        {
            copy_components(&check->catalysts, s.s_catalysts);
        }
        CASE (const GuardChoice&, s)
        {
            spellguard_check_t altcheck = *check;
            const effect_set_t *retval;

            altcheck.components = nullptr;
            altcheck.catalysts = nullptr;

            copy_components(&altcheck.catalysts, check->catalysts);
            copy_components(&altcheck.components, check->components);

            retval =
                spellguard_check_sub(&altcheck, guard->next, caster, env,
                                      near_miss);
            free_components(&altcheck.catalysts);
            free_components(&altcheck.components);
            if (retval)
                return retval;
            else
                return spellguard_check_sub(check, s.s_alt, caster,
                                             env, near_miss);
        }
        CASE (const GuardMana&, s)
        {
            check->mana += magic_eval_int(env, s.s_mana);
        }
        CASE (const GuardCastTime&, s)
        {
            check->casttime += static_cast<interval_t>(magic_eval_int(env, s.s_casttime));
        }
        CASE (const effect_set_t&, s_effect)
        {
            if (spellguard_can_satisfy(check, caster, env, near_miss))
                return &s_effect;
            else
                return nullptr;
        }
    }

    return spellguard_check_sub(check, guard->next, caster, env, near_miss);
}

static
const effect_set_t *check_spellguard(dumb_ptr<spellguard_t> guard,
        dumb_ptr<map_session_data> caster, dumb_ptr<env_t> env,
        int *near_miss)
{
    spellguard_check_t check;
    const effect_set_t *retval;
    check.catalysts = nullptr;
    check.components = nullptr;
    check.mana = 0;
    check.casttime = interval_t::zero();

    retval = spellguard_check_sub(&check, guard, caster, env, near_miss);

    free_components(&check.catalysts);
    free_components(&check.components);

    return retval;
}

/* -------------------------------------------------------------------------------- */
/* Public API */
/* -------------------------------------------------------------------------------- */

const effect_set_t *spell_trigger(dumb_ptr<spell_t> spell, dumb_ptr<map_session_data> caster,
        dumb_ptr<env_t> env, int *near_miss)
{
    dumb_ptr<spellguard_t> guard = spell->spellguard;

    if (near_miss)
        *near_miss = 0;

    for (letdef_t& ld : spell->letdefv)
        magic_eval(env, &env->varu[ld.id], ld.expr);

    return check_spellguard(guard, caster, env, near_miss);
}

static
void spell_set_location(dumb_ptr<invocation> invocation, dumb_ptr<block_list> entity)
{
    magic_clear_var(&invocation->env->varu[VAR_LOCATION]);
    ValLocation v;
    v.v_location.m = entity->bl_m;
    v.v_location.x = entity->bl_x;
    v.v_location.y = entity->bl_y;
    invocation->env->varu[VAR_LOCATION] = v;
}

void spell_update_location(dumb_ptr<invocation> invocation)
{
    if (bool(invocation->spell->flags & SPELL_FLAG::LOCAL))
        return;
    else
    {
        dumb_ptr<block_list> owner_bl = map_id2bl(invocation->subject);
        if (!owner_bl)
            return;
        dumb_ptr<map_session_data> owner = owner_bl->is_player();

        spell_set_location(invocation, owner);
    }
}

dumb_ptr<invocation> spell_instantiate(const effect_set_t *effect_set, dumb_ptr<env_t> env)
{
    dumb_ptr<invocation> retval;
    retval.new_();
    dumb_ptr<block_list> caster;

    retval->env = env;

    retval->caster = env->VAR(VAR_CASTER).get_if<ValEntityInt>()->v_eid;
    retval->spell = env->VAR(VAR_SPELL).get_if<ValSpell>()->v_spell;
    retval->current_effect = effect_set->effect;
    retval->trigger_effect = effect_set->at_trigger;
    retval->end_effect = effect_set->at_end;

    caster = map_id2bl(retval->caster);    // must still exist
    retval->bl_id = map_addobject(retval);
    retval->bl_type = BL::SPELL;
    retval->bl_m = caster->bl_m;
    retval->bl_x = caster->bl_x;
    retval->bl_y = caster->bl_y;

    map_addblock(retval);
    set_invocation(&env->varu[VAR_INVOCATION], retval);

    return retval;
}

dumb_ptr<invocation> spell_clone_effect(dumb_ptr<invocation> base)
{
    dumb_ptr<invocation> retval;
    retval.new_();

    // block_list in general is not copyable
    // since this is the only call site, it is expanded here
    //*retval = *base;

    retval->next_invocation = nullptr;
    retval->flags = INVOCATION_FLAG::ZERO;
    dumb_ptr<env_t> env = retval->env = clone_env(base->env);
    retval->spell = base->spell;
    retval->caster = base->caster;
    retval->subject = BlockId();
    // retval->timer = 0;
    // retval->stack = undef;
    retval->script_pos = 0;
    // huh?
    retval->current_effect = base->trigger_effect;
    retval->trigger_effect = base->trigger_effect;
    retval->end_effect = nullptr;
    // retval->status_change_refs = nullptr;

    retval->bl_id = BlockId();
    retval->bl_prev = nullptr;
    retval->bl_next = nullptr;
    retval->bl_m = base->bl_m;
    retval->bl_x = base->bl_x;
    retval->bl_y = base->bl_y;
    retval->bl_type = base->bl_type;

    retval->bl_id = map_addobject(retval);
    set_invocation(&env->varu[VAR_INVOCATION], retval);

    return retval;
}

void spell_bind(dumb_ptr<map_session_data> subject, dumb_ptr<invocation> invocation)
{
    /* Only bind nonlocal spells */

    if (!bool(invocation->spell->flags & SPELL_FLAG::LOCAL))
    {
        if (bool(invocation->flags & INVOCATION_FLAG::BOUND)
            || invocation->subject || invocation->next_invocation)
        {
            int *i = nullptr;
            FPRINTF(stderr,
                    "[magic] INTERNAL ERROR: Attempt to re-bind spell invocation `%s'\n"_fmt,
                    invocation->spell->name);
            *i = 1;
            return;
        }

        invocation->next_invocation = subject->active_spells;
        subject->active_spells = invocation;
        invocation->flags |= INVOCATION_FLAG::BOUND;
        invocation->subject = subject->bl_id;
    }

    spell_set_location(invocation, subject);
}

int spell_unbind(dumb_ptr<map_session_data> subject, dumb_ptr<invocation> invocation_)
{
    dumb_ptr<invocation> *seeker = &subject->active_spells;

    while (*seeker)
    {
        if (*seeker == invocation_)
        {
            *seeker = invocation_->next_invocation;

            invocation_->flags &= ~INVOCATION_FLAG::BOUND;
            invocation_->next_invocation = nullptr;
            invocation_->subject = BlockId();

            return 0;
        }
        seeker = &((*seeker)->next_invocation);
    }

    return 1;
}
} // namespace magic
} // namespace tmwa
