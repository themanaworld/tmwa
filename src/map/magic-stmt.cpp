#include <cassert>

#include "../common/cxxstdio.hpp"
#include "../common/random2.hpp"
#include "../common/timer.hpp"

#include "magic-expr.hpp"
#include "magic-expr-eval.hpp"
#include "magic-interpreter.hpp"
#include "magic-interpreter-aux.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "pc.hpp"
#include "skill.hpp"

#include "../poison.hpp"

/* used for local spell effects */
constexpr int INVISIBLE_NPC = 127;

//#define DEBUG

#ifdef DEBUG
static
void print_val(val_t *v)
{
    switch (v->ty)
    {
        case TYPE::UNDEF:
            FPRINTF(stderr, "UNDEF");
            break;
        case TYPE::INT:
            FPRINTF(stderr, "%d", v->v.v_int);
            break;
        case TYPE::DIR:
            FPRINTF(stderr, "dir%d", v->v.v_int);
            break;
        case TYPE::STRING:
            FPRINTF(stderr, "`%s'", v->v.v_string);
            break;
        default:
            FPRINTF(stderr, "ty%d", v->ty);
            break;
    }
}

static
void dump_env(env_t *env)
{
    int i;
    for (i = 0; i < env->base_env->vars_nr; i++)
    {
        val_t *v = &env->vars[i];
        val_t *bv = &env->base_env->vars[i];

        FPRINTF(stderr, "%02x %30s ", i, env->base_env->var_name[i]);
        print_val(v);
        FPRINTF(stderr, "\t(");
        print_val(bv);
        FPRINTF(stderr, ")\n");
    }
}
#endif

static
void clear_activation_record(cont_activation_record_t *ar)
{
    switch (ar->ty)
    {
        case CONT_STACK::FOREACH:
            free(ar->c.c_foreach.entities);
            break;
        case CONT_STACK::PROC:
            free(ar->c.c_proc.old_actuals);
            break;
    }
}

static
void invocation_timer_callback(TimerData *, tick_t, int id)
{
    invocation_t *invocation = (invocation_t *) map_id2bl(id);

    assert (invocation != NULL);
    {
        spell_execute(invocation);
    }
}

static
void clear_stack(invocation_t *invocation)
{
    int i;

    for (i = 0; i < invocation->stack_size; i++)
        clear_activation_record(&invocation->stack[i]);

    invocation->stack_size = 0;
}

void spell_free_invocation(invocation_t *invocation)
{
    if (invocation->status_change_refs)
    {
        free(invocation->status_change_refs);
        /* The following cleanup shouldn't be necessary, but I've added it to help tracking a certain bug */
        invocation->status_change_refs = NULL;
        invocation->status_change_refs_nr = 0;
    }

    if (bool(invocation->flags & INVOCATION_FLAG::BOUND))
    {
        entity_t *e = map_id2bl(invocation->subject);
        if (e && e->type == BL::PC)
            spell_unbind((character_t *) e, invocation);
    }

    clear_stack(invocation);

    invocation->timer.cancel();

    magic_free_env(invocation->env);

    map_delblock(&invocation->bl);
    map_delobject(invocation->bl.id, BL::SPELL);    // also frees the object
//        free(invocation);
}

static
void char_set_weapon_icon(character_t *subject, int count,
        StatusChange icon, int look)
{
    const StatusChange old_icon = subject->attack_spell_icon_override;

    subject->attack_spell_icon_override = icon;
    subject->attack_spell_look_override = look;

    if (old_icon != StatusChange::ZERO && old_icon != icon)
        clif_status_change(&subject->bl, old_icon, 0);

    clif_fixpcpos(subject);
    if (count)
    {
        clif_changelook(&subject->bl, LOOK::WEAPON, look);
        if (icon != StatusChange::ZERO)
            clif_status_change(&subject->bl, icon, 1);
    }
    else
    {
        /* Set it to `normal' */
        clif_changelook(&subject->bl, LOOK::WEAPON,
                static_cast<uint16_t>(subject->status.weapon));
    }
}

static
void char_set_attack_info(character_t *subject, interval_t speed, int range)
{
    subject->attack_spell_delay = speed;
    subject->attack_spell_range = range;

    if (speed == interval_t::zero())
    {
        pc_calcstatus(subject, 1);
        clif_updatestatus(subject, SP::ASPD);
        clif_updatestatus(subject, SP::ATTACKRANGE);
    }
    else
    {
        subject->aspd = speed;
        clif_updatestatus(subject, SP::ASPD);
        clif_updatestatus(subject, SP::ATTACKRANGE);
    }
}

void magic_stop_completely(character_t *c)
{
    // Zap all status change references to spells
    for (StatusChange i : erange(StatusChange(), StatusChange::MAX_STATUSCHANGE))
        c->sc_data[i].spell_invocation = 0;

    while (c->active_spells)
        spell_free_invocation(c->active_spells);

    if (c->attack_spell_override)
    {
        invocation_t *attack_spell =
            (invocation_t *) map_id2bl(c->attack_spell_override);
        if (attack_spell)
            spell_free_invocation(attack_spell);
        c->attack_spell_override = 0;
        char_set_weapon_icon(c, 0, StatusChange::ZERO, 0);
        char_set_attack_info(c, interval_t::zero(), 0);
    }
}

/* Spell execution has finished normally or we have been notified by a finished skill timer */
static
void try_to_finish_invocation(invocation_t *invocation)
{
    if (invocation->status_change_refs_nr == 0 && !invocation->current_effect)
    {
        if (invocation->end_effect)
        {
            clear_stack(invocation);
            invocation->current_effect = invocation->end_effect;
            invocation->end_effect = NULL;
            spell_execute(invocation);
        }
        else
            spell_free_invocation(invocation);
    }
}

static
int trigger_spell(int subject, int spell)
{
    invocation_t *invocation = (invocation_t *) map_id2bl(spell);

    if (!invocation)
        return 0;

    invocation = spell_clone_effect(invocation);

    spell_bind((character_t *) map_id2bl(subject), invocation);
    magic_clear_var(&invocation->env->vars[VAR_CASTER]);
    invocation->env->vars[VAR_CASTER].ty = TYPE::ENTITY;
    invocation->env->vars[VAR_CASTER].v.v_int = subject;

    return invocation->bl.id;
}

static
void entity_warp(entity_t *target, int destm, int destx, int desty);

static
void char_update(character_t *character)
{
    entity_warp((entity_t *) character, character->bl.m, character->bl.x,
                 character->bl.y);
}

static
void timer_callback_effect(TimerData *, tick_t, int id, int data)
{
    entity_t *target = map_id2bl(id);
    if (target)
        clif_misceffect(target, data);
}

static
void entity_effect(entity_t *entity, int effect_nr, interval_t delay)
{
    Timer(gettick() + delay,
            std::bind(&timer_callback_effect, ph::_1, ph::_2,
                entity->id, effect_nr)
    ).detach();
}

void magic_unshroud(character_t *other_char)
{
    other_char->state.shroud_active = 0;
    // Now warp the caster out of and back into here to refresh everyone's display
    char_update(other_char);
    clif_displaymessage(other_char->fd, "Your shroud has been dispelled!");
//        entity_effect(&other_char->bl, MAGIC_EFFECT_REVEAL);
}

static
void timer_callback_effect_npc_delete(TimerData *, tick_t, int npc_id)
{
    struct npc_data *effect_npc = (struct npc_data *) map_id2bl(npc_id);
    npc_free(effect_npc);
}

static
struct npc_data *local_spell_effect(int m, int x, int y, int effect,
        interval_t tdelay)
{
    /* 1 minute should be enough for all interesting spell effects, I hope */
    std::chrono::seconds delay = std::chrono::seconds(30);
    struct npc_data *effect_npc = npc_spawn_text(m, x, y,
            INVISIBLE_NPC, "", "?");
    int effect_npc_id = effect_npc->bl.id;

    entity_effect(&effect_npc->bl, effect, tdelay);
    Timer(gettick() + delay,
            std::bind(timer_callback_effect_npc_delete, ph::_1, ph::_2,
                effect_npc_id)
    ).detach();

    return effect_npc;
}

static
int op_sfx(env_t *, int, val_t *args)
{
    interval_t delay = static_cast<interval_t>(ARGINT(2));

    if (ARG_TYPE(0) == TYPE::ENTITY)
    {
        entity_effect(ARGENTITY(0), ARGINT(1), delay);
    }
    else if (ARG_TYPE(0) == TYPE::LOCATION)
    {
        local_spell_effect(ARGLOCATION(0).m,
                            ARGLOCATION(0).x,
                            ARGLOCATION(0).y, ARGINT(1), delay);
    }
    else
        return 1;

    return 0;
}

static
int op_instaheal(env_t *env, int, val_t *args)
{
    entity_t *caster = (VAR(VAR_CASTER).ty == TYPE::ENTITY)
        ? map_id2bl(VAR(VAR_CASTER).v.v_int) : NULL;
    entity_t *subject = ARGENTITY(0);
    if (!caster)
        caster = subject;

    if (caster->type == BL::PC && subject->type == BL::PC)
    {
        character_t *caster_pc = (character_t *) caster;
        character_t *subject_pc = (character_t *) subject;
        MAP_LOG_PC(caster_pc, "SPELLHEAL-INSTA PC%d FOR %d",
                    subject_pc->status.char_id, ARGINT(1));
    }

    battle_heal(caster, subject, ARGINT(1), ARGINT(2), 0);
    return 0;
}

static
int op_itemheal(env_t *env, int args_nr, val_t *args)
{
    entity_t *subject = ARGENTITY(0);
    if (subject->type == BL::PC)
    {
        pc_itemheal((struct map_session_data *) subject,
                     ARGINT(1), ARGINT(2));
    }
    else
        return op_instaheal(env, args_nr, args);

    return 0;
}

namespace e
{
enum class Shroud
{
    HIDE_NAME_TALKING_FLAG      = 1 << 0,
    DISAPPEAR_ON_PICKUP_FLAG    = 1 << 1,
    DISAPPEAR_ON_TALK_FLAG      = 1 << 2,
};
ENUM_BITWISE_OPERATORS(Shroud)
}
using e::Shroud;

// differs from ARGPC by checking
#define ARGCHAR(n) (ARGENTITY(n)->type == BL::PC) ? (character_t *)(ARGENTITY(n)) : NULL

static
int op_shroud(env_t *, int, val_t *args)
{
    character_t *subject = ARGCHAR(0);
    Shroud arg = static_cast<Shroud>(ARGINT(1));

    if (!subject)
        return 0;

    subject->state.shroud_active = 1;
    subject->state.shroud_hides_name_talking =
        bool(arg & Shroud::HIDE_NAME_TALKING_FLAG);
    subject->state.shroud_disappears_on_pickup =
        bool(arg & Shroud::DISAPPEAR_ON_PICKUP_FLAG);
    subject->state.shroud_disappears_on_talk =
        bool(arg & Shroud::DISAPPEAR_ON_TALK_FLAG);
    return 0;
}

static
int op_reveal(env_t *, int, val_t *args)
{
    character_t *subject = ARGCHAR(0);

    if (subject && subject->state.shroud_active)
        magic_unshroud(subject);

    return 0;
}

static
int op_message(env_t *, int, val_t *args)
{
    character_t *subject = ARGCHAR(0);

    if (subject)
        clif_displaymessage(subject->fd, ARGSTR(1));

    return 0;
}

static
void timer_callback_kill_npc(TimerData *, tick_t, int npc_id)
{
    struct npc_data *npc = (struct npc_data *) map_id2bl(npc_id);
    if (npc)
        npc_free(npc);
}

static
int op_messenger_npc(env_t *, int, val_t *args)
{
    struct npc_data *npc;
    location_t *loc = &ARGLOCATION(0);

    npc = npc_spawn_text(loc->m, loc->x, loc->y,
            ARGINT(1), ARGSTR(2), ARGSTR(3));

    Timer(gettick() + static_cast<interval_t>(ARGINT(4)),
            std::bind(timer_callback_kill_npc, ph::_1, ph::_2,
                npc->bl.id)
    ).detach();

    return 0;
}

static
void entity_warp(entity_t *target, int destm, int destx, int desty)
{
    if (target->type == BL::PC || target->type == BL::MOB)
    {

        switch (target->type)
        {
            case BL::PC:
            {
                character_t *character = (character_t *) target;
                char *map_name;
                clif_clearchar(&character->bl, BeingRemoveWhy::WARPED);
                map_delblock(&character->bl);
                character->bl.x = destx;
                character->bl.y = desty;
                character->bl.m = destm;

                pc_touch_all_relevant_npcs(character);

                // Note that touching NPCs may have triggered warping and thereby updated x and y:
                map_name = map[character->bl.m].name;

                // Warp part #1: update relevant data, interrupt trading etc.:
                pc_setpos(character, map_name, character->bl.x, character->bl.y, BeingRemoveWhy::GONE);
                // Warp part #2: now notify the client
                clif_changemap(character, map_name,
                                character->bl.x, character->bl.y);
                break;
            }
            case BL::MOB:
                target->x = destx;
                target->y = desty;
                target->m = destm;
                clif_fixmobpos((struct mob_data *) target);
                break;
        }
    }
}

static
int op_move(env_t *, int, val_t *args)
{
    entity_t *subject = ARGENTITY(0);
    DIR dir = ARGDIR(1);

    int newx = subject->x + dirx[dir];
    int newy = subject->y + diry[dir];

    if (!bool(map_getcell(subject->m, newx, newy) & MapCell::UNWALKABLE))
        entity_warp(subject, subject->m, newx, newy);

    return 0;
}

static
int op_warp(env_t *, int, val_t *args)
{
    entity_t *subject = ARGENTITY(0);
    location_t *loc = &ARGLOCATION(1);

    entity_warp(subject, loc->m, loc->x, loc->y);

    return 0;
}

static
int op_banish(env_t *, int, val_t *args)
{
    entity_t *subject = ARGENTITY(0);

    if (subject->type == BL::MOB)
    {
        struct mob_data *mob = (struct mob_data *) subject;

        if (bool(mob->mode & MobMode::SUMMONED))
            mob_catch_delete(mob, BeingRemoveWhy::WARPED);
    }

    return 0;
}

static
void record_status_change(invocation_t *invocation, int bl_id,
        StatusChange sc_id)
{
    int index = invocation->status_change_refs_nr++;
    status_change_ref_t *cr;

    RECREATE(invocation->status_change_refs, status_change_ref_t, invocation->status_change_refs_nr);

    cr = &invocation->status_change_refs[index];

    cr->sc_type = sc_id;
    cr->bl_id = bl_id;
}

static
int op_status_change(env_t *env, int, val_t *args)
{
    entity_t *subject = ARGENTITY(0);
    int invocation_id = VAR(VAR_INVOCATION).ty == TYPE::INVOCATION
        ? VAR(VAR_INVOCATION).v.v_int : 0;
    invocation_t *invocation = (invocation_t *) map_id2bl(invocation_id);

    assert (!ARGINT(3));
    assert (!ARGINT(4));
    assert (!ARGINT(5));
    skill_status_effect(subject, static_cast<StatusChange>(ARGINT(1)),
            ARGINT(2),
            static_cast<interval_t>(ARGINT(6)), invocation_id);

    if (invocation && subject->type == BL::PC)
        record_status_change(invocation, subject->id, StatusChange(ARGINT(1)));

    return 0;
}

static
int op_stop_status_change(env_t *, int, val_t *args)
{
    entity_t *subject = ARGENTITY(0);

    StatusChange sc = static_cast<StatusChange>(ARGINT(1));
    skill_status_change_end(subject, sc, nullptr);

    return 0;
}

static
int op_override_attack(env_t *env, int, val_t *args)
{
    entity_t *psubject = ARGENTITY(0);
    int charges = ARGINT(1);
    interval_t attack_delay = static_cast<interval_t>(ARGINT(2));
    int attack_range = ARGINT(3);
    StatusChange icon = StatusChange(ARGINT(4));
    int look = ARGINT(5);
    int stopattack = ARGINT(6);
    character_t *subject;

    if (psubject->type != BL::PC)
        return 0;

    subject = (character_t *) psubject;

    if (subject->attack_spell_override)
    {
        invocation_t *old_invocation =
            (invocation_t *) map_id2bl(subject->attack_spell_override);
        if (old_invocation)
            spell_free_invocation(old_invocation);
    }

    subject->attack_spell_override =
        trigger_spell(subject->bl.id, VAR(VAR_INVOCATION).v.v_int);
    subject->attack_spell_charges = charges;

    if (subject->attack_spell_override)
    {
        invocation_t *attack_spell =
            (invocation_t *) map_id2bl(subject->attack_spell_override);
        if (attack_spell && stopattack)
            attack_spell->flags |= INVOCATION_FLAG::STOPATTACK;

        char_set_weapon_icon(subject, charges, icon, look);
        char_set_attack_info(subject, attack_delay, attack_range);
    }

    return 0;
}

static
int op_create_item(env_t *, int, val_t *args)
{
    struct item item;
    entity_t *entity = ARGENTITY(0);
    character_t *subject;
    int stackable;
    int count = ARGINT(2);
    if (count <= 0)
        return 0;

    if (entity->type == BL::PC)
        subject = (character_t *) entity;
    else
        return 0;

    GET_ARG_ITEM(1, item, stackable);

    if (!stackable)
        while (count--)
            pc_additem(subject, &item, 1);
    else
        pc_additem(subject, &item, count);

    return 0;
}

inline
bool AGGRAVATION_MODE_ATTACKS_CASTER(int n)
{
    return n == 0 || n == 2;
}
inline
bool AGGRAVATION_MODE_MAKES_AGGRESSIVE(int n)
{
    return n > 0;
}

static
int op_aggravate(env_t *, int, val_t *args)
{
    entity_t *victim = ARGENTITY(2);
    int mode = ARGINT(1);
    entity_t *target = ARGENTITY(0);
    struct mob_data *other;

    if (target->type == BL::MOB)
        other = (struct mob_data *) target;
    else
        return 0;

    mob_target(other, victim, battle_get_range(victim));

    if (AGGRAVATION_MODE_MAKES_AGGRESSIVE(mode))
        other->mode = MobMode::war | (other->mode & MobMode::SENSIBLE_MASK);

    if (AGGRAVATION_MODE_ATTACKS_CASTER(mode))
    {
        other->target_id = victim->id;
        other->attacked_id = victim->id;
    }

    return 0;
}

enum class MonsterAttitude
{
    HOSTILE     = 0,
    FRIENDLY    = 1,
    SERVANT     = 2,
    FROZEN      = 3,
};

static
int op_spawn(env_t *, int, val_t *args)
{
    area_t *area = ARGAREA(0);
    entity_t *owner_e = ARGENTITY(1);
    int monster_id = ARGINT(2);
    MonsterAttitude monster_attitude = static_cast<MonsterAttitude>(ARGINT(3));
    int monster_count = ARGINT(4);
    interval_t monster_lifetime = static_cast<interval_t>(ARGINT(5));
    int i;

    character_t *owner = NULL;
    if (monster_attitude == MonsterAttitude::SERVANT
        && owner_e->type == BL::PC)
        owner = (character_t *) owner_e;

    for (i = 0; i < monster_count; i++)
    {
        location_t loc;
        magic_random_location(&loc, area);

        int mob_id;
        struct mob_data *mob;

        mob_id = mob_once_spawn(owner, map[loc.m].name, loc.x, loc.y, "--ja--",    // Is that needed?
                                 monster_id, 1, "");

        mob = (struct mob_data *) map_id2bl(mob_id);

        if (mob)
        {
            mob->mode = mob_db[monster_id].mode;

            switch (monster_attitude)
            {
                case MonsterAttitude::SERVANT:
                    mob->state.special_mob_ai = 1;
                    mob->mode |= MobMode::AGGRESSIVE;
                    break;

                case MonsterAttitude::FRIENDLY:
                    mob->mode = MobMode::CAN_ATTACK | (mob->mode & MobMode::CAN_MOVE);
                    break;

                case MonsterAttitude::HOSTILE:
                    mob->mode = MobMode::CAN_ATTACK | MobMode::AGGRESSIVE | (mob->mode & MobMode::CAN_MOVE);
                    if (owner)
                    {
                        mob->target_id = owner->bl.id;
                        mob->attacked_id = owner->bl.id;
                    }
                    break;

                case MonsterAttitude::FROZEN:
                    mob->mode = MobMode::ZERO;
                    break;
            }

            mob->mode |=
                MobMode::SUMMONED | MobMode::TURNS_AGAINST_BAD_MASTER;

            mob->deletetimer = Timer(gettick() + monster_lifetime,
                    std::bind(mob_timer_delete, ph::_1, ph::_2,
                        mob_id));

            if (owner)
            {
                mob->master_id = owner->bl.id;
                mob->master_dist = 6;
            }
        }
    }

    return 0;
}

static
const char *get_invocation_name(env_t *env)
{
    invocation_t *invocation;

    if (VAR(VAR_INVOCATION).ty != TYPE::INVOCATION)
        return "?";
    invocation = (invocation_t *) map_id2bl(VAR(VAR_INVOCATION).v.v_int);

    if (invocation)
        return invocation->spell->name;
    else
        return "??";
}

static
int op_injure(env_t *env, int, val_t *args)
{
    entity_t *caster = ARGENTITY(0);
    entity_t *target = ARGENTITY(1);
    int damage_caused = ARGINT(2);
    int mp_damage = ARGINT(3);
    int target_hp = battle_get_hp(target);
    int mdef = battle_get_mdef(target);

    if (target->type == BL::PC && !map[target->m].flag.pvp && !((character_t *) target)->special_state.killable && (caster->type != BL::PC || !((character_t *) caster)->special_state.killer))
        return 0;               /* Cannot damage other players outside of pvp */

    if (target != caster)
    {
        /* Not protected against own spells */
        damage_caused = (damage_caused * (100 - mdef)) / 100;
        mp_damage = (mp_damage * (100 - mdef)) / 100;
    }

    damage_caused = (damage_caused > target_hp) ? target_hp : damage_caused;

    if (damage_caused < 0)
        damage_caused = 0;

    // display damage first, because dealing damage may deallocate the target.
    clif_damage(caster, target,
            gettick(), interval_t::zero(), interval_t::zero(),
            damage_caused, 0, DamageType::NORMAL, 0);

    if (caster->type == BL::PC)
    {
        character_t *caster_pc = (character_t *) caster;
        if (target->type == BL::MOB)
        {
            struct mob_data *mob = (struct mob_data *) target;

            MAP_LOG_PC(caster_pc, "SPELLDMG MOB%d %d FOR %d BY %s",
                        mob->bl.id, mob->mob_class, damage_caused,
                        get_invocation_name(env));
        }
    }
    battle_damage(caster, target, damage_caused, mp_damage);

    return 0;
}

static
int op_emote(env_t *, int, val_t *args)
{
    entity_t *victim = ARGENTITY(0);
    int emotion = ARGINT(1);
    clif_emotion(victim, emotion);

    return 0;
}

static
int op_set_script_variable(env_t *, int, val_t *args)
{
    character_t *c = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : NULL;

    if (!c)
        return 1;

    pc_setglobalreg(c, ARGSTR(1), ARGINT(2));

    return 0;
}

static
int op_set_hair_colour(env_t *, int, val_t *args)
{
    character_t *c = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : NULL;

    if (!c)
        return 1;

    pc_changelook(c, LOOK::HAIR_COLOR, ARGINT(1));

    return 0;
}

static
int op_set_hair_style(env_t *, int, val_t *args)
{
    character_t *c = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : NULL;

    if (!c)
        return 1;

    pc_changelook(c, LOOK::HAIR, ARGINT(1));

    return 0;
}

static
int op_drop_item_for (env_t *, int args_nr, val_t *args)
{
    struct item item;
    int stackable;
    location_t *loc = &ARGLOCATION(0);
    int count = ARGINT(2);
    interval_t interval = static_cast<interval_t>(ARGINT(3));
    character_t *c = ((args_nr > 4) && (ENTITY_TYPE(4) == BL::PC)) ? ARGPC(4) : NULL;
    interval_t delay = (args_nr > 5) ? static_cast<interval_t>(ARGINT(5)) : interval_t::zero();
    interval_t delaytime[3] = { delay, delay, delay };
    character_t *owners[3] = { c, NULL, NULL };

    GET_ARG_ITEM(1, item, stackable);

    if (stackable)
        map_addflooritem_any(&item, count, loc->m, loc->x, loc->y,
                owners, delaytime, interval, 0);
    else
        while (count-- > 0)
            map_addflooritem_any(&item, 1, loc->m, loc->x, loc->y,
                    owners, delaytime, interval, 0);

    return 0;
}

static
int op_gain_exp(env_t *, int, val_t *args)
{
    character_t *c = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : NULL;

    if (!c)
        return 1;

    pc_gainexp_reason(c, ARGINT(1), ARGINT(2),
            PC_GAINEXP_REASON(ARGINT(3)));
    return 0;
}

static
op_t operations[] = {
    {"sfx", ".ii", op_sfx},
    {"instaheal", "eii", op_instaheal},
    {"itemheal", "eii", op_itemheal},
    {"shroud", "ei", op_shroud},
    {"unshroud", "e", op_reveal},
    {"message", "es", op_message},
    {"messenger_npc", "lissi", op_messenger_npc},
    {"move", "ed", op_move},
    {"warp", "el", op_warp},
    {"banish", "e", op_banish},
    {"status_change", "eiiiiii", op_status_change},
    {"stop_status_change", "ei", op_stop_status_change},
    {"override_attack", "eiiiiii", op_override_attack},
    {"create_item", "e.i", op_create_item},
    {"aggravate", "eie", op_aggravate},
    {"spawn", "aeiiii", op_spawn},
    {"injure", "eeii", op_injure},
    {"emote", "ei", op_emote},
    {"set_script_variable", "esi", op_set_script_variable},
    {"set_hair_colour", "ei", op_set_hair_colour},
    {"set_hair_style", "ei", op_set_hair_style},
    {"drop_item", "l.ii", op_drop_item_for},
    {"drop_item_for", "l.iiei", op_drop_item_for},
    {"gain_experience", "eiii", op_gain_exp},
    {NULL, NULL, NULL}
};

static
int operations_sorted = 0;
static
int operation_count;

static
int compare_operations(const void *lhs, const void *rhs)
{
    return strcmp(((const op_t *) lhs)->name, ((const op_t *) rhs)->name);
}

op_t *magic_get_op(char *name, int *index)
{
    op_t key;

    if (!operations_sorted)
    {
        op_t *opc = operations;

        while (opc->name)
            ++opc;

        operation_count = opc - operations;

        qsort(operations, operation_count, sizeof(op_t),
               compare_operations);
        operations_sorted = 1;
    }

    key.name = name;
    op_t *op = (op_t *)bsearch(&key, operations, operation_count, sizeof(op_t),
                        compare_operations);

    if (op && index)
        *index = op - operations;

    return op;
}

void spell_effect_report_termination(int invocation_id, int bl_id,
        StatusChange sc_id, int)
{
    int i;
    int index = -1;
    invocation_t *invocation = (invocation_t *) map_id2bl(invocation_id);

    if (!invocation || invocation->bl.type != BL::SPELL)
        return;

    for (i = 0; i < invocation->status_change_refs_nr; i++)
    {
        status_change_ref_t *cr = &invocation->status_change_refs[i];
        if (cr->sc_type == sc_id && cr->bl_id == bl_id)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        entity_t *entity = map_id2bl(bl_id);
        if (entity->type == BL::PC)
            FPRINTF(stderr,
                     "[magic] INTERNAL ERROR: spell-effect-report-termination:  tried to terminate on unexpected bl %d, sc %d\n",
                     bl_id, sc_id);
        return;
    }

    if (index == invocation->status_change_refs_nr - 1)
        invocation->status_change_refs_nr--;
    else                        /* Copy last change ref to the one we are deleting */
        invocation->status_change_refs[index] =
            invocation->
            status_change_refs[--invocation->status_change_refs_nr];

    try_to_finish_invocation(invocation);
}

static
effect_t *return_to_stack(invocation_t *invocation)
{
    if (!invocation->stack_size)
        return NULL;
    else
    {
        cont_activation_record_t *ar =
            invocation->stack + (invocation->stack_size - 1);
        switch (ar->ty)
        {

            case CONT_STACK::PROC:
            {
                effect_t *ret = ar->return_location;
                int i;

                for (i = 0; i < ar->c.c_proc.args_nr; i++)
                {
                    val_t *var =
                        &invocation->env->vars[ar->c.c_proc.formals[i]];
                    magic_clear_var(var);
                    *var = ar->c.c_proc.old_actuals[i];
                }

                clear_activation_record(ar);
                --invocation->stack_size;

                return ret;
            }

            case CONT_STACK::FOREACH:
            {
                int entity_id;
                val_t *var = &invocation->env->vars[ar->c.c_foreach.id];

                do
                {
                    if (ar->c.c_foreach.index >= ar->c.c_foreach.entities_nr)
                    {
                        effect_t *ret = ar->return_location;
                        clear_activation_record(ar);
                        --invocation->stack_size;
                        return ret;
                    }

                    entity_id =
                        ar->c.c_foreach.entities[ar->c.c_foreach.index++];
                }
                while (!entity_id || !map_id2bl(entity_id));

                magic_clear_var(var);
                var->ty = ar->c.c_foreach.ty;
                var->v.v_int = entity_id;

                return ar->c.c_foreach.body;
            }

            case CONT_STACK::FOR:
                if (ar->c.c_for.current > ar->c.c_for.stop)
                {
                    effect_t *ret = ar->return_location;
                    clear_activation_record(ar);
                    --invocation->stack_size;
                    return ret;
                }

                magic_clear_var(&invocation->env->vars[ar->c.c_for.id]);
                invocation->env->vars[ar->c.c_for.id].ty = TYPE::INT;
                invocation->env->vars[ar->c.c_for.id].v.v_int =
                    ar->c.c_for.current++;

                return ar->c.c_for.body;

            default:
                FPRINTF(stderr,
                         "[magic] INTERNAL ERROR: While executing spell `%s':  stack corruption\n",
                         invocation->spell->name);
                return NULL;
        }
    }
}

static
cont_activation_record_t *add_stack_entry(invocation_t *invocation,
        CONT_STACK ty, effect_t *return_location)
{
    cont_activation_record_t *ar =
        invocation->stack + invocation->stack_size++;
    if (invocation->stack_size >= MAX_STACK_SIZE)
    {
        FPRINTF(stderr,
                 "[magic] Execution stack size exceeded in spell `%s'; truncating effect\n",
                 invocation->spell->name);
        invocation->stack_size--;
        return NULL;
    }

    ar->ty = ty;
    ar->return_location = return_location;
    return ar;
}

static
void find_entities_in_area_c(entity_t *target,
        int *entities_allocd_p,
        int *entities_nr_p,
        int **entities_p,
        FOREACH_FILTER filter)
{
/* The following macro adds an entity to the result list: */
#define ADD_ENTITY(e)                                                   \
        if (*entities_nr_p == *entities_allocd_p) {                     \
                /* Need more space */                                   \
                (*entities_allocd_p) += 32;                             \
                RECREATE(*entities_p, int, *entities_allocd_p); \
        }                                                               \
        (*entities_p)[(*entities_nr_p)++] = e;

    switch (target->type)
    {

        case BL::PC:
            if (filter == FOREACH_FILTER::PC
                || filter == FOREACH_FILTER::ENTITY
                || (filter == FOREACH_FILTER::TARGET
                    && map[target->m].flag.pvp))
                break;
            else if (filter == FOREACH_FILTER::SPELL)
            {                   /* Check all spells bound to the caster */
                invocation_t *invoc = ((character_t *) target)->active_spells;
                /* Add all spells locked onto thie PC */

                while (invoc)
                {
                    ADD_ENTITY(invoc->bl.id);
                    invoc = invoc->next_invocation;
                }
            }
            return;

        case BL::MOB:
            if (filter == FOREACH_FILTER::MOB
                || filter == FOREACH_FILTER::ENTITY
                || filter == FOREACH_FILTER::TARGET)
                break;
            else
                return;

        case BL::SPELL:
            if (filter == FOREACH_FILTER::SPELL)
            {
                invocation_t *invocation = (invocation_t *) target;

                /* Check whether the spell is `bound'-- if so, we'll consider it iff we see the caster(case BL::PC). */
                if (bool(invocation->flags & INVOCATION_FLAG::BOUND))
                    return;
                else
                    break;      /* Add the spell */
            }
            else
                return;

        case BL::NPC:
            if (filter == FOREACH_FILTER::NPC)
                break;
            else
                return;

        default:
            return;
    }

    ADD_ENTITY(target->id);
#undef ADD_ENTITY
}

static
void find_entities_in_area(area_t *area, int *entities_allocd_p,
        int *entities_nr_p, int **entities_p, FOREACH_FILTER filter)
{
    switch (area->ty)
    {
        case AREA::UNION:
            find_entities_in_area(area->a.a_union[0], entities_allocd_p,
                                   entities_nr_p, entities_p, filter);
            find_entities_in_area(area->a.a_union[1], entities_allocd_p,
                                   entities_nr_p, entities_p, filter);
            break;

        default:
        {
            int m, x, y, width, height;
            magic_area_rect(&m, &x, &y, &width, &height, area);
            map_foreachinarea(std::bind(find_entities_in_area_c, ph::_1, entities_allocd_p, entities_nr_p, entities_p, filter),
                               m, x, y, x + width, y + height,
                               BL::NUL /* filter elsewhere */);
        }
    }
}

static
effect_t *run_foreach(invocation_t *invocation, effect_t *foreach,
                              effect_t *return_location)
{
    val_t area;
    FOREACH_FILTER filter = foreach->e.e_foreach.filter;
    int id = foreach->e.e_foreach.id;
    effect_t *body = foreach->e.e_foreach.body;

    magic_eval(invocation->env, &area, foreach->e.e_foreach.area);

    if (area.ty != TYPE::AREA)
    {
        magic_clear_var(&area);
        FPRINTF(stderr,
                 "[magic] Error in spell `%s':  FOREACH loop over non-area\n",
                 invocation->spell->name);
        return return_location;
    }
    else
    {
        cont_activation_record_t *ar =
            add_stack_entry(invocation, CONT_STACK::FOREACH, return_location);
        int entities_allocd = 64;
        int *entities;
        int entities_nr = 0;

        if (!ar)
            return return_location;

        CREATE(entities, int, entities_allocd);
        find_entities_in_area(area.v.v_area, &entities_allocd, &entities_nr,
                &entities, filter);
        RECREATE(entities, int, entities_nr);
        // iterator_pair will go away when this gets properly containerized.
        random_::shuffle(iterator_pair(entities, entities + entities_nr));

        ar->c.c_foreach.id = id;
        ar->c.c_foreach.body = body;
        ar->c.c_foreach.index = 0;
        ar->c.c_foreach.entities_nr = entities_nr;
        ar->c.c_foreach.entities = entities;
        ar->c.c_foreach.ty =
            (filter == FOREACH_FILTER::SPELL) ? TYPE::INVOCATION : TYPE::ENTITY;

        magic_clear_var(&area);

        return return_to_stack(invocation);
    }
}

static
effect_t *run_for (invocation_t *invocation, effect_t *for_,
                          effect_t *return_location)
{
    cont_activation_record_t *ar;
    int id = for_->e.e_for.id;
    val_t start;
    val_t stop;

    magic_eval(invocation->env, &start, for_->e.e_for.start);
    magic_eval(invocation->env, &stop, for_->e.e_for.stop);

    if (start.ty != TYPE::INT || stop.ty != TYPE::INT)
    {
        magic_clear_var(&start);
        magic_clear_var(&stop);
        FPRINTF(stderr,
                 "[magic] Error in spell `%s':  FOR loop start or stop point is not an integer\n",
                 invocation->spell->name);
        return return_location;
    }

    ar = add_stack_entry(invocation, CONT_STACK::FOR, return_location);

    if (!ar)
        return return_location;

    ar->c.c_for.id = id;
    ar->c.c_for.current = start.v.v_int;
    ar->c.c_for.stop = stop.v.v_int;
    ar->c.c_for.body = for_->e.e_for.body;

    return return_to_stack(invocation);
}

static
effect_t *run_call(invocation_t *invocation,
                           effect_t *return_location)
{
    effect_t *current = invocation->current_effect;
    cont_activation_record_t *ar;
    int args_nr = current->e.e_call.args_nr;
    int *formals = current->e.e_call.formals;
    val_t *old_actuals;
    CREATE(old_actuals, val_t, args_nr);
    int i;

    ar = add_stack_entry(invocation, CONT_STACK::PROC, return_location);
    ar->c.c_proc.args_nr = args_nr;
    ar->c.c_proc.formals = formals;
    ar->c.c_proc.old_actuals = old_actuals;
    for (i = 0; i < args_nr; i++)
    {
        val_t *env_val = &invocation->env->vars[formals[i]];
        val_t result;
        magic_copy_var(&old_actuals[i], env_val);
        magic_eval(invocation->env, &result, current->e.e_call.actuals[i]);
        *env_val = result;
    }

    return current->e.e_call.body;
}

#ifdef DEBUG
static
void print_cfg(int i, effect_t *e)
{
    int j;
    for (j = 0; j < i; j++)
        PRINTF("    ");

    PRINTF("%p: ", e);

    if (!e)
    {
        puts(" -- end --");
        return;
    }

    switch (e->ty)
    {
        case EFFECT::SKIP:
            puts("SKIP");
            break;
        case EFFECT::END:
            puts("END");
            break;
        case EFFECT::ABORT:
            puts("ABORT");
            break;
        case EFFECT::ASSIGN:
            puts("ASSIGN");
            break;
        case EFFECT::FOREACH:
            puts("FOREACH");
            print_cfg(i + 1, e->e.e_foreach.body);
            break;
        case EFFECT::FOR:
            puts("FOR");
            print_cfg(i + 1, e->e.e_for.body);
            break;
        case EFFECT::IF:
            puts("IF");
            for (j = 0; j < i; j++)
                PRINTF("    ");
            puts("THEN");
            print_cfg(i + 1, e->e.e_if.true_branch);
            for (j = 0; j < i; j++)
                PRINTF("    ");
            puts("ELSE");
            print_cfg(i + 1, e->e.e_if.false_branch);
            break;
        case EFFECT::SLEEP:
            puts("SLEEP");
            break;
        case EFFECT::SCRIPT:
            puts("NpcSubtype::SCRIPT");
            break;
        case EFFECT::BREAK:
            puts("BREAK");
            break;
        case EFFECT::OP:
            puts("OP");
            break;
    }
    print_cfg(i, e->next);
}
#endif

/**
 * Execute a spell invocation until we abort, finish, or hit the next `sleep'.
 *
 * Use spell_execute() to automate handling of timers
 *
 * Returns: 0 if finished (all memory is freed implicitly)
 *          >1 if we hit `sleep'; the result is the number of ticks we should sleep for.
 *          -1 if we paused to wait for a user action (via script interaction)
 */
static
interval_t spell_run(invocation_t *invocation, int allow_delete)
{
    const int invocation_id = invocation->bl.id;
#define REFRESH_INVOCATION invocation = (invocation_t *) map_id2bl(invocation_id); if (!invocation) return interval_t::zero();

#ifdef DEBUG
    FPRINTF(stderr, "Resuming execution:  invocation of `%s'\n",
             invocation->spell->name);
    print_cfg(1, invocation->current_effect);
#endif
    while (invocation->current_effect)
    {
        effect_t *e = invocation->current_effect;
        effect_t *next = e->next;
        int i;

#ifdef DEBUG
        FPRINTF(stderr, "Next step of type %d\n", e->ty);
        dump_env(invocation->env);
#endif

        switch (e->ty)
        {
            case EFFECT::SKIP:
                break;

            case EFFECT::ABORT:
                invocation->flags |= INVOCATION_FLAG::ABORTED;
                invocation->end_effect = NULL;
                FALLTHROUGH;
            case EFFECT::END:
                clear_stack(invocation);
                next = NULL;
                break;

            case EFFECT::ASSIGN:
                magic_eval(invocation->env,
                            &invocation->env->vars[e->e.e_assign.id],
                            e->e.e_assign.expr);
                break;

            case EFFECT::FOREACH:
                next = run_foreach(invocation, e, next);
                break;

            case EFFECT::FOR:
                next = run_for (invocation, e, next);
                break;

            case EFFECT::IF:
                if (magic_eval_int(invocation->env, e->e.e_if.cond))
                    next = e->e.e_if.true_branch;
                else
                    next = e->e.e_if.false_branch;
                break;

            case EFFECT::SLEEP:
            {
                interval_t sleeptime = static_cast<interval_t>(
                        magic_eval_int(invocation->env, e->e.e_sleep));
                invocation->current_effect = next;
                if (sleeptime > interval_t::zero())
                    return sleeptime;
                break;
            }

            case EFFECT::SCRIPT:
            {
                character_t *caster =
                    (character_t *) map_id2bl(invocation->caster);
                if (caster)
                {
                    env_t *env = invocation->env;
                    argrec_t arg[] = { {"@target",
                                        VAR(VAR_TARGET).ty ==
                                        TYPE::ENTITY ? 0 : VAR(VAR_TARGET).
                                        v.v_int}
                    ,
                    {"@caster", invocation->caster}
                    ,
                    {"@caster_name$", caster ? caster->status.name : ""}
                    };
                    int message_recipient =
                        VAR(VAR_SCRIPTTARGET).ty ==
                        TYPE::ENTITY ? VAR(VAR_SCRIPTTARGET).
                        v.v_int : invocation->caster;
                    character_t *recipient =
                        (character_t *) map_id2bl(message_recipient);

                    if (recipient->npc_id
                        && recipient->npc_id != invocation->bl.id)
                        break;  /* Don't send multiple message boxes at once */

                    if (!invocation->script_pos)    // first time running this script?
                        clif_spawn_fake_npc_for_player(recipient,
                                                        invocation->bl.id);
                    // We have to do this or otherwise the client won't think that it's
                    // dealing with an NPC

                    int newpos = run_script_l(e->e.e_script,
                                                invocation->script_pos,
                                                message_recipient,
                                                invocation->bl.id,
                                                3, arg);
                    /* Returns the new script position, or -1 once the script is finished */
                    if (newpos != -1)
                    {
                        /* Must set up for continuation */
                        recipient->npc_id = invocation->bl.id;
                        recipient->npc_pos = invocation->script_pos = newpos;
                        return static_cast<interval_t>(-1);  /* Signal `wait for script' */
                    }
                    else
                        invocation->script_pos = 0;
                    clif_clearchar_id(invocation->bl.id, BeingRemoveWhy::DEAD, caster->fd);
                }
                REFRESH_INVOCATION; // Script may have killed the caster
                break;
            }

            case EFFECT::BREAK:
                next = return_to_stack(invocation);
                break;

            case EFFECT::OP:
            {
                op_t *op = &operations[e->e.e_op.id];
                val_t args[MAX_ARGS];

                for (i = 0; i < e->e.e_op.args_nr; i++)
                    magic_eval(invocation->env, &args[i], e->e.e_op.args[i]);

                if (!magic_signature_check("effect", op->name, op->signature,
                                            e->e.e_op.args_nr, args,
                                            e->e.e_op.line_nr,
                                            e->e.e_op.column))
                    op->op(invocation->env, e->e.e_op.args_nr, args);

                for (i = 0; i < e->e.e_op.args_nr; i++)
                    magic_clear_var(&args[i]);

                REFRESH_INVOCATION; // Effect may have killed the caster
                break;
            }

            case EFFECT::CALL:
                next = run_call(invocation, next);
                break;

            default:
                FPRINTF(stderr,
                         "[magic] INTERNAL ERROR: Unknown effect %d\n",
                         e->ty);
        }

        if (!next)
            next = return_to_stack(invocation);

        invocation->current_effect = next;
    }

    if (allow_delete)
        try_to_finish_invocation(invocation);
    return interval_t::zero();
#undef REFRESH_INVOCATION
}

static
void spell_execute_d(invocation_t *invocation, int allow_deletion)
{
    spell_update_location(invocation);
    interval_t delta = spell_run(invocation, allow_deletion);

    if (delta > interval_t::zero())
    {
        assert (!invocation->timer);
        invocation->timer = Timer(gettick() + delta,
                std::bind(invocation_timer_callback, ph::_1, ph::_2,
                    invocation->bl.id));
    }

    /* If 0, the script cleaned itself.  If -1(wait-for-script), we must wait for the user. */
}

void spell_execute(invocation_t *invocation)
{
    spell_execute_d(invocation, 1);
}

void spell_execute_script(invocation_t *invocation)
{
    if (invocation->script_pos)
        spell_execute_d(invocation, 1);
    /* Otherwise the script-within-the-spell has been terminated by some other means.
     * In practice this happens when the script doesn't wait for user input: the client
     * may still notify the server that it's done.  Without the above check, we'd be
     * running the same spell twice! */
}

int spell_attack(int caster_id, int target_id)
{
    character_t *caster = (character_t *) map_id2bl(caster_id);
    invocation_t *invocation;
    int stop_attack = 0;

    if (!caster)
        return 0;

    invocation = (invocation_t *) map_id2bl(caster->attack_spell_override);

    if (invocation && bool(invocation->flags & INVOCATION_FLAG::STOPATTACK))
        stop_attack = 1;

    if (invocation && caster->attack_spell_charges > 0)
    {
        magic_clear_var(&invocation->env->vars[VAR_TARGET]);
        invocation->env->vars[VAR_TARGET].ty = TYPE::ENTITY;
        invocation->env->vars[VAR_TARGET].v.v_int = target_id;

        invocation->current_effect = invocation->trigger_effect;
        invocation->flags &= ~INVOCATION_FLAG::ABORTED;
        spell_execute_d(invocation,
                         0 /* don't delete the invocation if done */ );

        // If the caster died, we need to refresh here:
        invocation =
            (invocation_t *) map_id2bl(caster->attack_spell_override);

        if (invocation && !bool(invocation->flags & INVOCATION_FLAG::ABORTED))   // If we didn't abort:
            caster->attack_spell_charges--;
    }

    if (invocation && caster->attack_spell_override != invocation->bl.id)
    {
        /* Attack spell changed / was refreshed */
        // spell_free_invocation(invocation); // [Fate] This would be a double free.
    }
    else if (!invocation || caster->attack_spell_charges <= 0)
    {
        caster->attack_spell_override = 0;
        char_set_weapon_icon(caster, 0, StatusChange::ZERO, 0);
        char_set_attack_info(caster, interval_t::zero(), 0);

        if (stop_attack)
            pc_stopattack(caster);

        if (invocation)
            spell_free_invocation(invocation);
    }

    return 1;
}
