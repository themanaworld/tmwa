#include "magic.h"
#include "magic-interpreter.h"
#include "magic-expr.h"
#include "magic-interpreter-aux.h"

static void set_int_p (val_t * v, int i, int t)
{
    v->ty = t;
    v->v.v_int = i;
}

#define set_int(v, i) set_int_p(v, i, TY_INT)
#define set_dir(v, i) set_int_p(v, i, TY_DIR)

#define SETTER(tty, dyn_ty, field) (val_t *v, tty x) { v->ty = dyn_ty; v->v.field = x; }

static void set_string SETTER (char *, TY_STRING, v_string);

static void set_entity (val_t * v, entity_t * e)
{
    v->ty = TY_ENTITY;
    v->v.v_int = e->id;
}

static void set_invocation (val_t * v, invocation_t * i)
{
    v->ty = TY_INVOCATION;
    v->v.v_int = i->bl.id;
}

static void set_spell SETTER (spell_t *, TY_SPELL, v_spell);

#define setenv(f, v, x) f(&(env->vars[v]), x)

#define set_env_int(v, x) setenv(set_int, v, x)
#define set_env_dir(v, x) setenv(set_dir, v, x)
#define set_env_string(v, x) setenv(set_string, v, x)
#define set_env_entity(v, x) setenv(set_entity, v, x)
#define set_env_location(v, x) setenv(set_location, v, x)
#define set_env_area(v, x) setenv(set_area, v, x)
#define set_env_invocation(v, x) setenv(set_invocation, v, x)
#define set_env_spell(v, x) setenv(set_spell, v, x)

magic_conf_t magic_conf;        /* Global magic conf */
env_t magic_default_env = { &magic_conf, NULL };

static int spells_sorted = 0;

char *magic_find_invocation (char *spellname)
{
    int  i;

    for (i = 0; i < abs (magic_conf.spells_nr); i++)
        if (!strcmp (magic_conf.spells[i]->name, spellname))
            return magic_conf.spells[i]->invocation;

    return NULL;
}

static int spell_compare (const void *lhs, const void *rhs)
{
    return strcmp ((*((spell_t **) lhs))->invocation,
                   (*((spell_t **) rhs))->invocation);
}

spell_t *magic_find_spell (char *invocation)
{
    spell_t key;
    spell_t *keyp = &key;
    spell_t **retval;

    if (!spells_sorted)
    {
        qsort (magic_conf.spells, magic_conf.spells_nr, sizeof (spell_t *),
               spell_compare);
        spells_sorted = 1;
    }

    key.invocation = invocation;

    retval =
        ((spell_t **)
         bsearch (&keyp, magic_conf.spells, magic_conf.spells_nr,
                  sizeof (spell_t *), spell_compare));

    if (!retval)
        return NULL;
    else
        return *retval;
}

/* -------------------------------------------------------------------------------- */
/* Spell anchors */
/* -------------------------------------------------------------------------------- */

static int compare_teleport_anchor (const void *lhs, const void *rhs)
{
    return strcmp ((*((teleport_anchor_t **) lhs))->invocation,
                   (*((teleport_anchor_t **) rhs))->invocation);
}

char *magic_find_anchor_invocation (char *anchor_name)
{
    int  i;

    for (i = 0; i < abs (magic_conf.anchors_nr); i++)
        if (!strcmp (magic_conf.anchors[i]->name, anchor_name))
            return magic_conf.anchors[i]->invocation;

    return NULL;
}

teleport_anchor_t *magic_find_anchor (char *name)
{
    teleport_anchor_t key;
    teleport_anchor_t *keyp = &key;
    teleport_anchor_t **retval;

    if (magic_conf.anchors_nr > 0)
    {                           /* unsorted */
        qsort (magic_conf.anchors, magic_conf.anchors_nr,
               sizeof (teleport_anchor_t *), compare_teleport_anchor);
        magic_conf.anchors_nr = -magic_conf.anchors_nr;
    }

    key.invocation = name;

    retval = (teleport_anchor_t **) bsearch (&keyp,
                                             magic_conf.anchors,
                                             -magic_conf.anchors_nr,
                                             sizeof (teleport_anchor_t *),
                                             compare_teleport_anchor);

    if (!retval)
        return NULL;
    else
        return *retval;
}

/* -------------------------------------------------------------------------------- */
/* Spell guard checks */
/* -------------------------------------------------------------------------------- */

static env_t *alloc_env (magic_conf_t * conf)
{
    env_t *env = (env_t *) aCalloc (sizeof (env_t), 1);
    env->vars = (val_t *) aCalloc (sizeof (val_t), conf->vars_nr);
    env->base_env = conf;
    return env;
}

static env_t *clone_env (env_t * src)
{
    env_t *retval = alloc_env (src->base_env);
    int  i;

    for (i = 0; i < src->base_env->vars_nr; i++)
        magic_copy_var (&retval->vars[i], &src->vars[i]);

    return retval;
}

void magic_free_env (env_t * env)
{
    int  i;
    for (i = 0; i < env->base_env->vars_nr; i++)
        magic_clear_var (&env->vars[i]);
    free (env);
}

env_t *spell_create_env (magic_conf_t * conf, spell_t * spell,
                         character_t * caster, int spellpower, char *param)
{
    env_t *env = alloc_env (conf);

    switch (spell->spellarg_ty)
    {

        case SPELLARG_STRING:
            set_env_string (spell->arg, param);
            break;

        case SPELLARG_PC:
        {
            character_t *subject = map_nick2sd (param);
            if (!subject)
                subject = caster;
            set_env_entity (spell->arg, &subject->bl);
            free (param);
            break;
        }

        case SPELLARG_NONE:
            free (param);
            break;

        default:
            free (param);
            fprintf (stderr, "Unexpected spellarg type %d\n",
                     spell->spellarg_ty);
    }

    set_env_entity (VAR_CASTER, &caster->bl);
    set_env_int (VAR_SPELLPOWER, spellpower);
    set_env_spell (VAR_SPELL, spell);

    return env;
}

static void free_components (component_t ** component_holder)
{
    if (*component_holder == NULL)
        return;
    free_components (&(*component_holder)->next);
    free (*component_holder);
    *component_holder = NULL;
}

void magic_add_component (component_t ** component_holder, int id, int count)
{
    if (count <= 0)
        return;

    if (*component_holder == NULL)
    {
        component_t *component =
            (component_t *) malloc (sizeof (component_t));
        component->next = NULL;
        component->item_id = id;
        component->count = count;
        *component_holder = component;
    }
    else
    {
        component_t *component = *component_holder;
        if (component->item_id == id)
        {
            component->count += count;
            return;
        }
        else
            magic_add_component (&component->next, id, count);
        /* Tail-recurse; gcc can optimise this.  Not that it matters. */
    }
}

static void
copy_components (component_t ** component_holder, component_t * component)
{
    if (component == NULL)
        return;

    magic_add_component (component_holder, component->item_id,
                         component->count);
    copy_components (component_holder, component->next);
}

typedef struct spellguard_check
{
    component_t *catalysts, *components;
    int  mana, casttime;
} spellguard_check_t;

static int check_prerequisites (character_t * caster, component_t * component)
{
    while (component)
    {
        if (pc_count_all_items (caster, component->item_id)
            < component->count)
            return 0;           /* insufficient */

        component = component->next;
    }

    return 1;
}

static void consume_components (character_t * caster, component_t * component)
{
    while (component)
    {
        pc_remove_items (caster, component->item_id, component->count);
        component = component->next;
    }
}

static int
spellguard_can_satisfy (spellguard_check_t * check, character_t * caster,
                        env_t * env, int *near_miss)
{
    unsigned int tick = gettick ();

    int  retval = check_prerequisites (caster, check->catalysts);

/*
        fprintf(stderr, "MC(%d/%s)? %d%d%d%d (%u <= %u)\n",
                caster->bl.id, caster->status.name,
                retval, 
                caster->cast_tick <= tick,
                check->mana <= caster->status.sp,
                check_prerequisites(caster, check->components),
                caster->cast_tick, tick);
*/

    if (retval && near_miss)
        *near_miss = 1;         // close enough!

    retval = retval && caster->cast_tick <= tick    /* Hasn't cast a spell too recently */
        && check->mana <= caster->status.sp
        && check_prerequisites (caster, check->components);

    if (retval)
    {
        unsigned int casttime = (unsigned int) check->casttime;

        if (VAR (VAR_MIN_CASTTIME).ty == TY_INT)
            casttime = MAX (casttime, VAR (VAR_MIN_CASTTIME).v.v_int);

        caster->cast_tick = tick + casttime;    /* Make sure not to cast too frequently */

        consume_components (caster, check->components);
        pc_heal (caster, 0, -check->mana);
    }

    return retval;
}

static effect_set_t *spellguard_check_sub (spellguard_check_t * check,
                                           spellguard_t * guard,
                                           character_t * caster, env_t * env,
                                           int *near_miss)
{
    if (guard == NULL)
        return NULL;

    switch (guard->ty)
    {
        case SPELLGUARD_CONDITION:
            if (!magic_eval_int (env, guard->s.s_condition))
                return NULL;
            break;

        case SPELLGUARD_COMPONENTS:
            copy_components (&check->components, guard->s.s_components);
            break;

        case SPELLGUARD_CATALYSTS:
            copy_components (&check->catalysts, guard->s.s_catalysts);
            break;

        case SPELLGUARD_CHOICE:
        {
            spellguard_check_t altcheck = *check;
            effect_set_t *retval;

            altcheck.components = NULL;
            altcheck.catalysts = NULL;

            copy_components (&altcheck.catalysts, check->catalysts);
            copy_components (&altcheck.components, check->components);

            retval =
                spellguard_check_sub (&altcheck, guard->next, caster, env,
                                      near_miss);
            free_components (&altcheck.catalysts);
            free_components (&altcheck.components);
            if (retval)
                return retval;
            else
                return spellguard_check_sub (check, guard->s.s_alt, caster,
                                             env, near_miss);
        }

        case SPELLGUARD_MANA:
            check->mana += magic_eval_int (env, guard->s.s_mana);
            break;

        case SPELLGUARD_CASTTIME:
            check->casttime += magic_eval_int (env, guard->s.s_mana);
            break;

        case SPELLGUARD_EFFECT:
            if (spellguard_can_satisfy (check, caster, env, near_miss))
                return &guard->s.s_effect;
            else
                return NULL;

        default:
            fprintf (stderr, "Unexpected spellguard type %d\n", guard->ty);
            return NULL;
    }

    return spellguard_check_sub (check, guard->next, caster, env, near_miss);
}

static effect_set_t *check_spellguard (spellguard_t * guard,
                                       character_t * caster, env_t * env,
                                       int *near_miss)
{
    spellguard_check_t check;
    effect_set_t *retval;
    check.catalysts = NULL;
    check.components = NULL;
    check.mana = check.casttime = 0;

    retval = spellguard_check_sub (&check, guard, caster, env, near_miss);

    free_components (&check.catalysts);
    free_components (&check.components);

    return retval;
}

/* -------------------------------------------------------------------------------- */
/* Public API */
/* -------------------------------------------------------------------------------- */

effect_set_t *spell_trigger (spell_t * spell, character_t * caster,
                             env_t * env, int *near_miss)
{
    int  i;
    spellguard_t *guard = spell->spellguard;

    if (near_miss)
        *near_miss = 0;

    for (i = 0; i < spell->letdefs_nr; i++)
        magic_eval (env,
                    &env->vars[spell->letdefs[i].id], spell->letdefs[i].expr);

    return check_spellguard (guard, caster, env, near_miss);
}

static void spell_set_location (invocation_t * invocation, entity_t * entity)
{
    magic_clear_var (&invocation->env->vars[VAR_LOCATION]);
    invocation->env->vars[VAR_LOCATION].ty = TY_LOCATION;
    invocation->env->vars[VAR_LOCATION].v.v_location.m = entity->m;
    invocation->env->vars[VAR_LOCATION].v.v_location.x = entity->x;
    invocation->env->vars[VAR_LOCATION].v.v_location.y = entity->y;
}

void spell_update_location (invocation_t * invocation)
{
    if (invocation->spell->flags & SPELL_FLAG_LOCAL)
        return;
    else
    {
        character_t *owner = (character_t *) map_id2bl (invocation->subject);
        if (!owner)
            return;

        spell_set_location (invocation, (entity_t *) owner);
    }
}

invocation_t *spell_instantiate (effect_set_t * effect_set, env_t * env)
{
    invocation_t *retval =
        (invocation_t *) aCalloc (sizeof (invocation_t), 1);
    entity_t *caster;

    retval->env = env;

    retval->caster = VAR (VAR_CASTER).v.v_int;
    retval->spell = VAR (VAR_SPELL).v.v_spell;
    retval->stack_size = 0;
    retval->current_effect = effect_set->effect;
    retval->trigger_effect = effect_set->at_trigger;
    retval->end_effect = effect_set->at_end;

    caster = map_id2bl (retval->caster);    // must still exist
    retval->bl.id = map_addobject (&retval->bl);
    retval->bl.type = BL_SPELL;
    retval->bl.m = caster->m;
    retval->bl.x = caster->x;
    retval->bl.y = caster->y;

    map_addblock (&retval->bl);
    set_env_invocation (VAR_INVOCATION, retval);

    return retval;
}

invocation_t *spell_clone_effect (invocation_t * base)
{
    invocation_t *retval = (invocation_t *) malloc (sizeof (invocation_t));
    env_t *env;

    memcpy (retval, base, sizeof (invocation_t));

    retval->env = clone_env (retval->env);
    env = retval->env;
    retval->current_effect = retval->trigger_effect;
    retval->next_invocation = NULL;
    retval->end_effect = NULL;
    retval->script_pos = 0;
    retval->stack_size = 0;
    retval->timer = 0;
    retval->subject = 0;
    retval->status_change_refs_nr = 0;
    retval->status_change_refs = NULL;
    retval->flags = 0;

    retval->bl.id = 0;
    retval->bl.prev = NULL;
    retval->bl.next = NULL;

    retval->bl.id = map_addobject (&retval->bl);
    set_env_invocation (VAR_INVOCATION, retval);

    return retval;
}

void spell_bind (character_t * subject, invocation_t * invocation)
{
    /* Only bind nonlocal spells */

    if (!(invocation->spell->flags & SPELL_FLAG_LOCAL))
    {
        if (invocation->flags & INVOCATION_FLAG_BOUND
            || invocation->subject || invocation->next_invocation)
        {
            int *i = NULL;
            fprintf (stderr,
                     "[magic] INTERNAL ERROR: Attempt to re-bind spell invocation `%s'\n",
                     invocation->spell->name);
            *i = 1;
            return;
        }

        invocation->next_invocation = subject->active_spells;
        subject->active_spells = invocation;
        invocation->flags |= INVOCATION_FLAG_BOUND;
        invocation->subject = subject->bl.id;
    }

    spell_set_location (invocation, (entity_t *) subject);
}

int spell_unbind (character_t * subject, invocation_t * invocation)
{
    invocation_t **seeker = &subject->active_spells;

    while (*seeker)
    {
        if (*seeker == invocation)
        {
            *seeker = invocation->next_invocation;

            invocation->flags &= ~INVOCATION_FLAG_BOUND;
            invocation->next_invocation = NULL;
            invocation->subject = 0;

            return 0;
        }
        seeker = &((*seeker)->next_invocation);
    }

    return 1;
}
