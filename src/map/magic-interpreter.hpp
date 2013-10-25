#ifndef MAGIC_INTERPRETER_HPP
#define MAGIC_INTERPRETER_HPP

#include "magic-interpreter.t.hpp"

#include <cassert>

#include "../strings/fwd.hpp"
#include "../strings/fstring.hpp"

#include "magic.hpp"
#include "map.hpp"
#include "script.hpp"
#include "skill.t.hpp"

struct fun_t;
struct op_t;
struct expr_t;
struct val_t;
struct location_t;
struct area_t;
struct spell_t;
struct invocation;

struct location_t
{
    map_local *m;
    int x, y;
};

struct area_t
{
    union au
    {
        location_t a_loc;
        struct
        {
            location_t loc;
            int width, depth;
            DIR dir;
        } a_bar;
        struct
        {
            location_t loc;
            int width, height;
        } a_rect;
        dumb_ptr<area_t> a_union[2];

        au() { really_memzero_this(this); }
        ~au() = default;
        au(const au&) = default;
        au& operator = (const au&) = default;
    } a;
    int size;
    AREA ty;
};

struct val_t
{
    union vu
    {
        int v_int;
        DIR v_dir;
        dumb_string v_string;
        /* Used ONLY during operation/function invocation; otherwise we use v_int */
        dumb_ptr<block_list> v_entity;
        dumb_ptr<area_t> v_area;
        location_t v_location;
        /* Used ONLY during operation/function invocation; otherwise we use v_int */
        dumb_ptr<invocation> v_invocation;
        dumb_ptr<spell_t> v_spell;

        vu() { really_memzero_this(this); }
        ~vu() = default;
        vu(const vu&) = default;
        vu& operator = (const vu&) = default;
    } v;
    TYPE ty;
};

/* ----------- */
/* Expressions */
/* ----------- */

#define MAX_ARGS 7              /* Max. # of args used in builtin primitive functions */

struct e_location_t
{
    dumb_ptr<expr_t> m, x, y;
};

struct e_area_t
{
    union a0
    {
        e_location_t a_loc;
        struct
        {
            e_location_t loc;
            dumb_ptr<expr_t> width, depth, dir;
        } a_bar;
        struct
        {
            e_location_t loc;
            dumb_ptr<expr_t> width, height;
        } a_rect;
        dumb_ptr<e_area_t> a_union[2];

        a0() { really_memzero_this(this); }
        ~a0() = default;
        a0(const a0&) = default;
        a0& operator = (const a0&) = default;
    } a;
    AREA ty;
};

struct expr_t
{
    union eu
    {
        val_t e_val;
        e_location_t e_location;
        e_area_t e_area;
        struct
        {
            fun_t *funp;
            int line_nr, column;
            int args_nr;
            dumb_ptr<expr_t> args[MAX_ARGS];
        } e_funapp;
        int e_id;
        struct
        {
            dumb_ptr<expr_t> expr;
            int id;
        } e_field;

        eu() { really_memzero_this(this); }
        ~eu() = default;
        eu(const eu&) = default;
        eu& operator = (const eu&) = default;
    } e;
    EXPR ty;
};

struct effect_t
{
    dumb_ptr<effect_t> next;
    union e0
    {
        struct
        {
            int id;
            dumb_ptr<expr_t> expr;
        } e_assign;
        struct
        {
            int id;
            dumb_ptr<expr_t> area;
            dumb_ptr<effect_t> body;
            FOREACH_FILTER filter;
        } e_foreach;
        struct
        {
            int id;
            dumb_ptr<expr_t> start, stop;
            dumb_ptr<effect_t> body;
        } e_for;
        struct
        {
            dumb_ptr<expr_t> cond;
            dumb_ptr<effect_t> true_branch, false_branch;
        } e_if;
        dumb_ptr<expr_t> e_sleep;        /* sleep time */
        dumb_ptr<const ScriptBuffer> e_script;
        struct
        {
            op_t *opp;
            int args_nr;
            int line_nr, column;
            dumb_ptr<expr_t> args[MAX_ARGS];
        } e_op;
        struct
        {
            std::vector<int> *formalv;
            dumb_ptr<std::vector<dumb_ptr<expr_t>>> actualvp;
            dumb_ptr<effect_t> body;
        } e_call;

        e0() { really_memzero_this(this); }
        ~e0() = default;
        e0(const e0&) = default;
        e0& operator = (const e0&) = default;
    } e;
    EFFECT ty;
};

/* ---------- */
/* Components */
/* ---------- */

struct component_t
{
    dumb_ptr<component_t> next;
    int item_id;
    int count;
};


struct effect_set_t
{
    dumb_ptr<effect_t> effect, at_trigger, at_end;
};

struct spellguard_t
{
    dumb_ptr<spellguard_t> next;
    union su
    {
        dumb_ptr<expr_t> s_condition;
        dumb_ptr<expr_t> s_mana;
        dumb_ptr<expr_t> s_casttime;
        dumb_ptr<component_t> s_components;
        dumb_ptr<component_t> s_catalysts;
        dumb_ptr<spellguard_t> s_alt;   /* either `next' or `s.s_alt' */
        effect_set_t s_effect;
        su() { really_memzero_this(this); }
        ~su() = default;
        su(const su&) = default;
        su& operator = (const su&) = default;
    } s;
    SPELLGUARD ty;
};

/* ------ */
/* Spells */
/* ------ */

struct letdef_t
{
    int id;
    dumb_ptr<expr_t> expr;
};

struct spell_t
{
    FString name;
    FString invocation;
    SPELL_FLAG flags;
    int arg;
    SPELLARG spellarg_ty;

    std::vector<letdef_t> letdefv;

    dumb_ptr<spellguard_t> spellguard;
};

/* ------- */
/* Anchors */
/* ------- */

struct teleport_anchor_t
{
    FString name;
    FString invocation;
    dumb_ptr<expr_t> location;
};

/* ------------------- */
/* The big config blob */
/* ------------------- */

struct magic_conf_t
{
    struct mcvar
    {
        FString name;
        val_t val;
    };
    // This should probably be done by a dedicated "intern pool" class
    std::vector<mcvar> varv;

    std::map<FString, dumb_ptr<spell_t>> spells_by_name, spells_by_invocation;

    std::map<FString, dumb_ptr<teleport_anchor_t>> anchors_by_name, anchors_by_invocation;
};

/* Execution environment */

// these are not an enum they're a nasty intern hack
#define VAR_MIN_CASTTIME        0
#define VAR_OBSCURE_CHANCE      1
#define VAR_CASTER              2
#define VAR_SPELLPOWER          3
#define VAR_SPELL               4
#define VAR_INVOCATION          5
#define VAR_TARGET              6
#define VAR_SCRIPTTARGET        7
#define VAR_LOCATION            8

struct magic_config;

struct env_t
{
    magic_conf_t *base_env;
    std::unique_ptr<val_t[]> varu;

    val_t& VAR(size_t i)
    {
        assert (varu);
        if (varu[i].ty == TYPE::UNDEF)
            return base_env->varv[i].val;
        else
            return varu[i];
    }

};

#define MAX_STACK_SIZE 32

struct cont_activation_record_t
{
    dumb_ptr<effect_t> return_location;
    union cu
    {
        struct
        {
            int id;
            TYPE ty;
            dumb_ptr<effect_t> body;
            dumb_ptr<std::vector<int>> entities_vp;
            int index;
        } c_foreach;
        struct
        {
            int id;
            dumb_ptr<effect_t> body;
            int current;
            int stop;
        } c_for;
        struct
        {
            int args_nr;
            int *formalap;
            dumb_ptr<val_t[]> old_actualpa;
        } c_proc;

        cu() { really_memzero_this(this); }
        ~cu() = default;
        cu(const cu&) = default;
        cu& operator = (const cu&) = default;
    } c;
    CONT_STACK ty;
};

struct status_change_ref_t
{
    StatusChange sc_type;
    int bl_id;
};

struct invocation : block_list
{
    dumb_ptr<invocation> next_invocation; /* used for spells directly associated with a caster: they form a singly-linked list */
    INVOCATION_FLAG flags;

    dumb_ptr<env_t> env;
    dumb_ptr<spell_t> spell;
    int caster;                /* this is the person who originally invoked the spell */
    int subject;               /* when this person dies, the spell dies with it */

    Timer timer;                 /* spell timer, if any */

    int stack_size;
    cont_activation_record_t stack[MAX_STACK_SIZE];

    int script_pos;            /* Script position; if nonzero, resume the script we were running. */
    dumb_ptr<effect_t> current_effect;
    dumb_ptr<effect_t> trigger_effect;   /* If non-NULL, this is used to spawn a cloned effect based on the same environment */
    dumb_ptr<effect_t> end_effect;       /* If non-NULL, this is executed when the spell terminates naturally, e.g. when all status changes have run out or all delays are over. */

    /* Status change references:  for status change updates, keep track of whom we updated where */
    std::vector<status_change_ref_t> status_change_refv;

};

inline dumb_ptr<invocation> block_list::as_spell() { return dumb_ptr<invocation>(static_cast<invocation *>(this)); }
inline dumb_ptr<invocation> block_list::is_spell() { return bl_type == BL::SPELL ? as_spell() : nullptr; }

extern magic_conf_t magic_conf; /* Global magic conf */
extern env_t magic_default_env; /* Fake default environment */

/**
 * Adds a component selection to a component holder (which may initially be NULL)
 */
void magic_add_component(dumb_ptr<component_t> *component_holder, int id, int count);

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

/* The following is used only by the parser: */
struct args_rec_t
{
    dumb_ptr<std::vector<dumb_ptr<expr_t>>> argvp;
};

struct proc_t
{
    FString name;
    std::vector<int> argv;
    dumb_ptr<effect_t> body;

    proc_t()
    : name()
    , argv()
    , body()
    {}
};

// must be called after itemdb initialisation
int magic_init(const char *);
void spell_update_location(dumb_ptr<invocation> invocation);

#endif // MAGIC_INTERPRETER_HPP
