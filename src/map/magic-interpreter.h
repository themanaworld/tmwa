/* Magic interpreter */

#ifndef MAGIC_INTERPRETER_H
#define MAGIC_INTERPRETER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "malloc.h"
#include "nullpo.h"

#include "battle.h"
#include "chat.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "itemdb.h"
#include "magic.h"
#include "map.h"
#include "mob.h"
#include "npc.h"
#include "pc.h"
#include "party.h"
#include "script.h"
#include "skill.h"
#include "storage.h"
#include "trade.h"

#include "../common/timer.h"
#include "../common/socket.h"

#define SPELLARG_NONE	0       /* No spell parameter */
#define SPELLARG_PC	1           /* Spell parameter describes pc (defaults to self) */
#define SPELLARG_STRING	2       /* Spell parameter describes pc (defaults to self) */

/* ------ */
/* Values */
/* ------ */

#define TY_UNDEF	0
#define TY_INT		1
#define TY_DIR		2
#define TY_STRING	3
#define TY_ENTITY	5
#define TY_LOCATION	6
#define TY_AREA		7
#define TY_SPELL	8
#define TY_INVOCATION	9
#define TY_FAIL		127

#define DIR_S	0
#define DIR_SW	1
#define DIR_W	2
#define DIR_NW	3
#define DIR_N	4
#define DIR_NE	5
#define DIR_E	6
#define DIR_SE	7

struct expr;
struct val;
struct location;
struct area;
struct spell;
struct invocation;

typedef struct location
{
    int  m;
    int  x, y;
} location_t;

#define AREA_LOCATION	0
#define AREA_UNION	1
#define AREA_RECT	2
#define AREA_BAR	3

typedef struct area
{
    union a
    {
        location_t a_loc;
        struct
        {
            location_t loc;
            int  width, depth, dir;
        } a_bar;
        struct
        {
            location_t loc;
            int  width, height;
        } a_rect;
        struct area *a_union[2];
    } a;
    int  size;
    unsigned char ty;
} area_t;

typedef struct val
{
    union v
    {
        int  v_int;
        char *v_string;
        entity_t *v_entity;     /* Used ONLY during operation/function invocation; otherwise we use v_int */
        area_t *v_area;
        location_t v_location;
        struct invocation *v_invocation;    /* Used ONLY during operation/function invocation; otherwise we use v_int */
        struct spell *v_spell;
    } v;
    unsigned char ty;
} val_t;

/* ----------- */
/* Expressions */
/* ----------- */

#define MAX_ARGS 7              /* Max. # of args used in builtin primitive functions */

#define EXPR_VAL	0
#define EXPR_LOCATION	1
#define EXPR_AREA	2
#define EXPR_FUNAPP	3
#define EXPR_ID		4
#define EXPR_SPELLFIELD	5

typedef struct e_location
{
    struct expr *m, *x, *y;
} e_location_t;

typedef struct e_area
{
    union a0
    {
        e_location_t a_loc;
        struct
        {
            e_location_t loc;
            struct expr *width, *depth, *dir;
        } a_bar;
        struct
        {
            e_location_t loc;
            struct expr *width, *height;
        } a_rect;
        struct e_area *a_union[2];
    } a;
    unsigned char ty;
} e_area_t;

typedef struct expr
{
    union e
    {
        val_t e_val;
        e_location_t e_location;
        e_area_t e_area;
        struct
        {
            int  id, line_nr, column;
            int  args_nr;
            struct expr *args[MAX_ARGS];
        } e_funapp;
        int  e_id;
        struct
        {
            struct expr *expr;
            int  id;
        } e_field;
    } e;
    unsigned char ty;
} expr_t;

/* ------- */
/* Effects */
/* ------- */

#define EFFECT_SKIP	0
#define EFFECT_ABORT	1
#define EFFECT_ASSIGN	2
#define EFFECT_FOREACH	3
#define EFFECT_FOR	4
#define EFFECT_IF	5
#define EFFECT_SLEEP	6
#define EFFECT_SCRIPT	7
#define EFFECT_BREAK	8
#define EFFECT_OP	9
#define EFFECT_END	10
#define EFFECT_CALL	11

#define FOREACH_FILTER_MOB	1
#define FOREACH_FILTER_PC	2
#define FOREACH_FILTER_ENTITY	3
#define FOREACH_FILTER_TARGET	4
#define FOREACH_FILTER_SPELL	5
#define FOREACH_FILTER_NPC	6

typedef struct effect
{
    struct effect *next;
    union e0
    {
        struct
        {
            int  id;
            expr_t *expr;
        } e_assign;
        struct
        {
            int  id;
            expr_t *area;
            struct effect *body;
            unsigned char filter;
        } e_foreach;
        struct
        {
            int  id;
            expr_t *start, *stop;
            struct effect *body;
        } e_for;
        struct
        {
            expr_t *cond;
            struct effect *true_branch, *false_branch;
        } e_if;
        expr_t *e_sleep;        /* sleep time */
        unsigned char *e_script;
        struct
        {
            int  id;
            int  args_nr;
            int  line_nr, column;
            expr_t *args[MAX_ARGS];
        } e_op;
        struct
        {
            int  args_nr, *formals;
            expr_t **actuals;
            struct effect *body;
        } e_call;
    } e;
    unsigned char ty;
} effect_t;

/* ---------- */
/* Components */
/* ---------- */

typedef struct component
{
    struct component *next;
    int  item_id;
    int  count;
} component_t;

/* ----------- */
/* Spellguards */
/* ----------- */

#define SPELLGUARD_CONDITION	0
#define SPELLGUARD_COMPONENTS	1
#define SPELLGUARD_CATALYSTS	2
#define SPELLGUARD_CHOICE	3
#define SPELLGUARD_MANA		4
#define SPELLGUARD_CASTTIME	5
#define SPELLGUARD_EFFECT	6

typedef struct effect_set
{
    effect_t *effect, *at_trigger, *at_end;
} effect_set_t;

typedef struct spellguard
{
    struct spellguard *next;
    union s
    {
        expr_t *s_condition;
        expr_t *s_mana;
        expr_t *s_casttime;
        component_t *s_components;
        component_t *s_catalysts;
        struct spellguard *s_alt;   /* either `next' or `s.s_alt' */
        effect_set_t s_effect;
    } s;
    unsigned char ty;
} spellguard_t;

/* ------ */
/* Spells */
/* ------ */

typedef struct letdef
{
    int  id;
    expr_t *expr;
} letdef_t;

#define SPELL_FLAG_LOCAL	(1 << 0)    // spell associated not with caster but with place
#define SPELL_FLAG_SILENT	(1 << 1)    // spell invocation never uttered
#define SPELL_FLAG_NONMAGIC	(1 << 2)    // `magic word' only:  don't require spellcasting ability

typedef struct spell
{
    char *name;
    char *invocation;
    int  index;                 // Relative location in the definitions file
    int  flags;
    int  arg;
    int  spellarg_ty;

    int  letdefs_nr;
    letdef_t *letdefs;

    spellguard_t *spellguard;
} spell_t;

/* ------- */
/* Anchors */
/* ------- */

typedef struct teleport_anchor
{
    char *name;
    char *invocation;
    expr_t *location;
} teleport_anchor_t;

/* ------------------- */
/* The big config blob */
/* ------------------- */

typedef struct
{
    int  vars_nr;
    char **var_name;
    val_t *vars;                /* Initial assignments, if any, or NULL */

    int  obscure_chance;
    int  min_casttime;

    int  spells_nr;
    spell_t **spells;

    int  anchors_nr;            /* NEGATIVE iff we have sorted the anchors */
    teleport_anchor_t **anchors;
} magic_conf_t;

/* Execution environment */

#define VAR_MIN_CASTTIME	0
#define VAR_OBSCURE_CHANCE	1
#define VAR_CASTER		2
#define VAR_SPELLPOWER		3
#define VAR_SPELL		4
#define VAR_INVOCATION		5
#define VAR_TARGET		6
#define VAR_SCRIPTTARGET	7
#define VAR_LOCATION		8

struct magic_config;

typedef struct env
{
    magic_conf_t *base_env;
    val_t *vars;
} env_t;

#define MAX_STACK_SIZE 32

#define CONT_STACK_FOREACH	0
#define CONT_STACK_FOR		1
#define CONT_STACK_PROC		2

typedef struct cont_activation_record
{
    effect_t *return_location;
    union c
    {
        struct
        {
            int  id, ty;
            effect_t *body;
            int  entities_nr;
            int *entities;
            int  index;
        } c_foreach;
        struct
        {
            int  id;
            effect_t *body;
            int  current;
            int  stop;
        } c_for;
        struct
        {
            int  args_nr, *formals;
            val_t *old_actuals;
        } c_proc;
    } c;
    unsigned char ty;
} cont_activation_record_t;

typedef struct status_change_ref
{
    int  sc_type;
    int  bl_id;
} status_change_ref_t;

#define INVOCATION_FLAG_BOUND		(1 << 0)    /* Bound directly to the caster (i.e., ignore its location) */
#define INVOCATION_FLAG_ABORTED		(1 << 1)    /* Used `abort' to terminate */
#define INVOCATION_FLAG_STOPATTACK	(1 << 2)    /* On magical attacks:  if we run out of steam, stop attacking altogether */

typedef struct invocation
{
    struct block_list bl;

    struct invocation *next_invocation; /* used for spells directly associated with a caster: they form a singly-linked list */
    int  flags;

    env_t *env;
    spell_t *spell;
    int  caster;                /* this is the person who originally invoked the spell */
    int  subject;               /* when this person dies, the spell dies with it */

    int  timer;                 /* spell timer, if any */

    int  stack_size;
    cont_activation_record_t stack[MAX_STACK_SIZE];

    int  script_pos;            /* Script position; if nonzero, resume the script we were running. */
    effect_t *current_effect;
    effect_t *trigger_effect;   /* If non-NULL, this is used to spawn a cloned effect based on the same environment */
    effect_t *end_effect;       /* If non-NULL, this is executed when the spell terminates naturally, e.g. when all status changes have run out or all delays are over. */

    /* Status change references:  for status change updates, keep track of whom we updated where */
    int  status_change_refs_nr;
    status_change_ref_t *status_change_refs;

} invocation_t;

extern magic_conf_t magic_conf; /* Global magic conf */
extern env_t magic_default_env; /* Fake default environment */

/**
 * Adds a component selection to a component holder (which may initially be NULL)
 */
void magic_add_component (component_t ** component_holder, int id, int count);

teleport_anchor_t *magic_find_anchor (char *name);

/**
 * The parameter `param' must have been dynamically allocated; ownership is transferred to the resultant env_t.
 */
env_t *spell_create_env (magic_conf_t * conf, spell_t * spell,
                         character_t * caster, int spellpower, char *param);

void magic_free_env (env_t * env);

/**
 * near_miss is set to nonzero iff the spell only failed due to ephemereal issues (spell delay in effect, out of mana, out of components)
 */
effect_set_t *spell_trigger (spell_t * spell, character_t * caster,
                             env_t * env, int *near_miss);

invocation_t *spell_instantiate (effect_set_t * effect, env_t * env);

/**
 * Bind a spell to a subject (this is a no-op for `local' spells).
 */
void spell_bind (character_t * subject, invocation_t * invocation);

int                             // 1 on failure
     spell_unbind (character_t * subject, invocation_t * invocation);

/**
 * Clones a spell to run the at_effect field
 */
invocation_t *spell_clone_effect (invocation_t * source);

spell_t *magic_find_spell (char *invocation);

/* The following is used only by the parser: */
typedef struct args_rec
{
    int  args_nr;
    expr_t **args;
} args_rec_t;

typedef struct
{
    char *name;
    int  args_nr;
    int *args;
    effect_t *body;
} proc_t;

#endif /* !defined (MAGIC_INTERPRETER_H) */
