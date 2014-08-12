#pragma once
//    magic-interpreter.hpp - Old magic.
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

#include "magic-interpreter.t.hpp"

#include <cassert>

#include <memory>

#include "../strings/fwd.hpp"
#include "../strings/rstring.hpp"

#include "../generic/fwd.hpp"

#include "../sexpr/variant.hpp"

#include "../net/timer.t.hpp"

#include "../mmo/ids.hpp"
#include "../mmo/utils.hpp"

#include "map.hpp"
#include "script.hpp"
#include "skill.t.hpp"


namespace tmwa
{
namespace magic
{
struct location_t
{
    map_local *m;
    int x, y;
};

struct AreaUnion
{
    dumb_ptr<area_t> a_union[2];
};
struct AreaRect
{
    location_t loc;
    int width, height;
};
struct AreaBar
{
    location_t loc;
    int width, depth;
    DIR dir;
};

using AreaVariantBase = Variant<
    location_t,
    AreaUnion,
    AreaRect,
    AreaBar
>;

struct area_t : AreaVariantBase
{
    int size;

    area_t() = delete;
    area_t(area_t&&) = default;
    area_t(const area_t&) = delete;
    area_t& operator = (area_t&&) = default;
    area_t& operator = (const area_t&) = delete;

    area_t(location_t v, int sz) : AreaVariantBase(std::move(v)), size(sz) {}
    area_t(AreaUnion v, int sz) : AreaVariantBase(std::move(v)), size(sz) {}
    area_t(AreaRect v, int sz) : AreaVariantBase(std::move(v)), size(sz) {}
    area_t(AreaBar v, int sz) : AreaVariantBase(std::move(v)), size(sz) {}
};

struct ValUndef
{
};
struct ValInt
{
    int v_int;
};
struct ValDir
{
    DIR v_dir;
};
struct ValString
{
    RString v_string;
};
struct ValEntityInt
{
    BlockId v_eid;
};
struct ValEntityPtr
{
    dumb_ptr<block_list> v_entity;
};
struct ValLocation
{
    location_t v_location;
};
struct ValArea
{
    dumb_ptr<area_t> v_area;
};
struct ValSpell
{
    dumb_ptr<spell_t> v_spell;
};
struct ValInvocationInt
{
    BlockId v_iid;
};
struct ValInvocationPtr
{
    dumb_ptr<invocation> v_invocation;
};
struct ValFail
{
};
struct ValNegative1
{
};

using ValVariantBase = Variant<
    ValUndef,
    ValInt,
    ValDir,
    ValString,
    ValEntityInt,
    ValEntityPtr,
    ValLocation,
    ValArea,
    ValSpell,
    ValInvocationInt,
    ValInvocationPtr,
    ValFail,
    ValNegative1
>;
struct val_t : ValVariantBase
{
    val_t() noexcept : ValVariantBase(ValUndef{}) {}
    val_t(val_t&&) = default;
    val_t(const val_t&) = delete;
    val_t& operator = (val_t&&) = default;
    val_t& operator = (const val_t&) = delete;

    val_t(ValUndef v) : ValVariantBase(std::move(v)) {}
    val_t(ValInt v) : ValVariantBase(std::move(v)) {}
    val_t(ValDir v) : ValVariantBase(std::move(v)) {}
    val_t(ValString v) : ValVariantBase(std::move(v)) {}
    val_t(ValEntityInt v) : ValVariantBase(std::move(v)) {}
    val_t(ValEntityPtr v) : ValVariantBase(std::move(v)) {}
    val_t(ValLocation v) : ValVariantBase(std::move(v)) {}
    val_t(ValArea v) : ValVariantBase(std::move(v)) {}
    val_t(ValSpell v) : ValVariantBase(std::move(v)) {}
    val_t(ValInvocationInt v) : ValVariantBase(std::move(v)) {}
    val_t(ValInvocationPtr v) : ValVariantBase(std::move(v)) {}
    val_t(ValFail v) : ValVariantBase(std::move(v)) {}
    val_t(ValNegative1 v) : ValVariantBase(std::move(v)) {}
};


/* ----------- */
/* Expressions */
/* ----------- */

#define MAX_ARGS 7              /* Max. # of args used in builtin primitive functions */

struct e_area_t;

struct e_location_t
{
    dumb_ptr<expr_t> m, x, y;

    e_location_t() noexcept : m(), x(), y() {}
};
struct ExprAreaUnion
{
    dumb_ptr<e_area_t> a_union[2];
};
struct ExprAreaRect
{
    e_location_t loc;
    dumb_ptr<expr_t> width, height;
};
struct ExprAreaBar
{
    e_location_t loc;
    dumb_ptr<expr_t> width, depth, dir;
};

using ExprAreaVariantBase = Variant<
    e_location_t,
    ExprAreaUnion,
    ExprAreaRect,
    ExprAreaBar
>;

struct e_area_t : ExprAreaVariantBase
{
    e_area_t() = delete;
    e_area_t(e_area_t&&) = default;
    e_area_t(const e_area_t&) = delete;
    e_area_t& operator = (e_area_t&&) = default;
    e_area_t& operator = (const e_area_t&) = delete;

    e_area_t(e_location_t v) : ExprAreaVariantBase(std::move(v)) {}
    e_area_t(ExprAreaUnion v) : ExprAreaVariantBase(std::move(v)) {}
    e_area_t(ExprAreaRect v) : ExprAreaVariantBase(std::move(v)) {}
    e_area_t(ExprAreaBar v) : ExprAreaVariantBase(std::move(v)) {}
};

struct ExprFunApp
{
    fun_t *funp;
    int line_nr, column;
    int args_nr;
    dumb_ptr<expr_t> args[MAX_ARGS];
};
struct ExprId
{
    int e_id;
};
struct ExprField
{
    dumb_ptr<expr_t> expr;
    int id;
};

using ExprVariantBase = Variant<
    val_t,
    e_location_t,
    e_area_t,
    ExprFunApp,
    ExprId,
    ExprField
>;
struct expr_t : ExprVariantBase
{
    expr_t() = delete;
    expr_t(expr_t&&) = default;
    expr_t(const expr_t&) = delete;
    expr_t& operator = (expr_t&&) = default;
    expr_t& operator = (const expr_t&) = delete;

    expr_t(val_t v) : ExprVariantBase(std::move(v)) {}
    expr_t(e_location_t v) : ExprVariantBase(std::move(v)) {}
    expr_t(e_area_t v) : ExprVariantBase(std::move(v)) {}
    expr_t(ExprFunApp v) : ExprVariantBase(std::move(v)) {}
    expr_t(ExprId v) : ExprVariantBase(std::move(v)) {}
    expr_t(ExprField v) : ExprVariantBase(std::move(v)) {}
};


struct effect_t;

struct EffectSkip
{
};
struct EffectAbort
{
};
struct EffectAssign
{
    int id;
    dumb_ptr<expr_t> expr;
};
struct EffectForEach
{
    int id;
    dumb_ptr<expr_t> area;
    dumb_ptr<effect_t> body;
    FOREACH_FILTER filter;
};
struct EffectFor
{
    int id;
    dumb_ptr<expr_t> start, stop;
    dumb_ptr<effect_t> body;
};
struct EffectIf
{
    dumb_ptr<expr_t> cond;
    dumb_ptr<effect_t> true_branch, false_branch;
};
struct EffectSleep
{
    dumb_ptr<expr_t> e_sleep;        /* sleep time */
};
struct EffectScript
{
    dumb_ptr<const ScriptBuffer> e_script;
};
struct EffectBreak
{
};
struct EffectOp
{
    op_t *opp;
    int args_nr;
    int line_nr, column;
    dumb_ptr<expr_t> args[MAX_ARGS];
};
struct EffectEnd
{
};
struct EffectCall
{
    std::vector<int> *formalv;
    dumb_ptr<std::vector<dumb_ptr<expr_t>>> actualvp;
    dumb_ptr<effect_t> body;
};

using EffectVariantBase = Variant<
    EffectSkip,
    EffectAbort,
    EffectAssign,
    EffectForEach,
    EffectFor,
    EffectIf,
    EffectSleep,
    EffectScript,
    EffectBreak,
    EffectOp,
    EffectEnd,
    EffectCall
>;
struct effect_t : EffectVariantBase
{
    dumb_ptr<effect_t> next;

    effect_t() = delete;
    effect_t(effect_t&&) = default;
    effect_t(const effect_t&) = delete;
    effect_t& operator = (effect_t&&) = default;
    effect_t& operator = (const effect_t&) = delete;

    effect_t(EffectSkip v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectAbort v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectAssign v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectForEach v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectFor v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectIf v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectSleep v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectScript v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectBreak v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectOp v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectEnd v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
    effect_t(EffectCall v, dumb_ptr<effect_t> n) : EffectVariantBase(std::move(v)), next(n) {}
};

/* ---------- */
/* Components */
/* ---------- */

struct component_t
{
    dumb_ptr<component_t> next;
    ItemNameId item_id;
    int count;
};


struct spellguard_t;
struct GuardCondition
{
    dumb_ptr<expr_t> s_condition;
};
struct GuardMana
{
    dumb_ptr<expr_t> s_mana;
};
struct GuardCastTime
{
    dumb_ptr<expr_t> s_casttime;
};
struct GuardComponents
{
    dumb_ptr<component_t> s_components;
};
struct GuardCatalysts
{
    dumb_ptr<component_t> s_catalysts;
};
struct GuardChoice
{
    dumb_ptr<spellguard_t> s_alt;   /* either `next' or `s.s_alt' */
};
struct effect_set_t
{
    dumb_ptr<effect_t> effect, at_trigger, at_end;
};

using SpellGuardVariantBase = Variant<
    GuardCondition,
    GuardMana,
    GuardCastTime,
    GuardComponents,
    GuardCatalysts,
    GuardChoice,
    effect_set_t
>;
struct spellguard_t : SpellGuardVariantBase
{
    dumb_ptr<spellguard_t> next;

    spellguard_t() = delete;
    spellguard_t(spellguard_t&&) = default;
    spellguard_t(const spellguard_t&) = delete;
    spellguard_t& operator = (spellguard_t&&) = default;
    spellguard_t& operator = (const spellguard_t&) = delete;

    spellguard_t(GuardCondition v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
    spellguard_t(GuardMana v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
    spellguard_t(GuardCastTime v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
    spellguard_t(GuardComponents v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
    spellguard_t(GuardCatalysts v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
    spellguard_t(GuardChoice v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
    spellguard_t(effect_set_t v, dumb_ptr<spellguard_t> n) : SpellGuardVariantBase(std::move(v)), next(n) {}
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
    RString name;
    RString invocation;
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
    RString name;
    RString invocation;
    dumb_ptr<expr_t> location;
};

/* ------------------- */
/* The big config blob */
/* ------------------- */

struct magic_conf_t
{
    struct mcvar
    {
        RString name;
        val_t val;
    };
    // This should probably be done by a dedicated "intern pool" class
    std::vector<mcvar> varv;

    std::map<RString, dumb_ptr<spell_t>> spells_by_name, spells_by_invocation;

    std::map<RString, dumb_ptr<teleport_anchor_t>> anchors_by_name, anchors_by_invocation;
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

struct env_t
{
    magic_conf_t *base_env;
    std::unique_ptr<val_t[]> varu;

    val_t& VAR(size_t i)
    {
        assert (varu);
        if (varu[i].is<ValUndef>())
            return base_env->varv[i].val;
        else
            return varu[i];
    }

};

struct CarForEach
{
    int id;
    bool ty_is_spell_not_entity;
    dumb_ptr<effect_t> body;
    dumb_ptr<std::vector<BlockId>> entities_vp;
    int index;
};
struct CarFor
{
    int id;
    dumb_ptr<effect_t> body;
    int current;
    int stop;
};
struct CarProc
{
    int args_nr;
    int *formalap;
    dumb_ptr<val_t[]> old_actualpa;
};

using CarVariantBase = Variant<
    CarForEach,
    CarFor,
    CarProc
>;

struct cont_activation_record_t : CarVariantBase
{
    dumb_ptr<effect_t> return_location;

    cont_activation_record_t() = delete;
    cont_activation_record_t(cont_activation_record_t&&) = default;
    cont_activation_record_t(const cont_activation_record_t&) = delete;
    cont_activation_record_t& operator = (cont_activation_record_t&&) = default;
    cont_activation_record_t& operator = (const cont_activation_record_t&) = delete;

    cont_activation_record_t(CarForEach v, dumb_ptr<effect_t> rl) : CarVariantBase(std::move(v)), return_location(rl) {}
    cont_activation_record_t(CarFor v, dumb_ptr<effect_t> rl) : CarVariantBase(std::move(v)), return_location(rl) {}
    cont_activation_record_t(CarProc v, dumb_ptr<effect_t> rl) : CarVariantBase(std::move(v)), return_location(rl) {}
};

struct status_change_ref_t
{
    StatusChange sc_type;
    BlockId bl_id;
};

struct invocation : block_list
{
    dumb_ptr<invocation> next_invocation; /* used for spells directly associated with a caster: they form a singly-linked list */
    INVOCATION_FLAG flags;

    dumb_ptr<env_t> env;
    dumb_ptr<spell_t> spell;
    BlockId caster;                /* this is the person who originally invoked the spell */
    BlockId subject;               /* when this person dies, the spell dies with it */

    Timer timer;                 /* spell timer, if any */

    std::vector<cont_activation_record_t> stack;

    int script_pos;            /* Script position; if nonzero, resume the script we were running. */
    dumb_ptr<effect_t> current_effect;
    dumb_ptr<effect_t> trigger_effect;   /* If non-nullptr, this is used to spawn a cloned effect based on the same environment */
    dumb_ptr<effect_t> end_effect;       /* If non-nullptr, this is executed when the spell terminates naturally, e.g. when all status changes have run out or all delays are over. */

    /* Status change references:  for status change updates, keep track of whom we updated where */
    std::vector<status_change_ref_t> status_change_refv;

};
} // namespace magic

// inlines for map.hpp
inline dumb_ptr<magic::invocation> block_list::as_spell() { return dumb_ptr<magic::invocation>(static_cast<magic::invocation *>(this)); }
inline dumb_ptr<magic::invocation> block_list::is_spell() { return bl_type == BL::SPELL ? as_spell() : nullptr; }

namespace magic
{
/* The following is used only by the parser: */
struct args_rec_t
{
    dumb_ptr<std::vector<dumb_ptr<expr_t>>> argvp;
};

struct proc_t
{
    RString name;
    std::vector<int> argv;
    dumb_ptr<effect_t> body;

    proc_t()
    : name()
    , argv()
    , body()
    {}
};
} // namespace magic
} // namespace tmwa
