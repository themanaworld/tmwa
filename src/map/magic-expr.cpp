#include "magic-expr.hpp"
//    magic-expr.cpp - Pure functions for the old magic backend.
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

#include <cassert>

#include <algorithm>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/vstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/dumb_ptr.hpp"
#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/cxxstdio_enums.hpp"

#include "battle.hpp"
#include "itemdb.hpp"
#include "magic-expr-eval.hpp"
#include "magic-interpreter.hpp"
#include "magic-interpreter-base.hpp"
#include "npc.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace magic
{
static
void free_area(dumb_ptr<area_t> area)
{
    if (!area)
        return;

    MATCH (*area)
    {
        CASE (const AreaUnion&, a)
        {
            free_area(a.a_union[0]);
            free_area(a.a_union[1]);
        }
    }

    area.delete_();
}

static
dumb_ptr<area_t> dup_area(dumb_ptr<area_t> area)
{
    MATCH (*area)
    {
        CASE (const location_t&, loc)
        {
            return dumb_ptr<area_t>::make(loc, area->size);
        }
        CASE (const AreaUnion&, a)
        {
            AreaUnion u;
            u.a_union[0] = dup_area(a.a_union[0]);
            u.a_union[1] = dup_area(a.a_union[1]);
            return dumb_ptr<area_t>::make(u, area->size);
        }
        CASE (const AreaRect&, rect)
        {
            return dumb_ptr<area_t>::make(rect, area->size);
        }
        CASE (const AreaBar&, bar)
        {
            return dumb_ptr<area_t>::make(bar, area->size);
        }
    }

    abort();
}

void magic_copy_var(val_t *dest, const val_t *src)
{
    MATCH (*src)
    {
        // mumble mumble not a public API ...
        default:
        {
            abort();
        }
        CASE (const ValUndef&, s)
        {
            *dest = s;
        }
        CASE (const ValInt&, s)
        {
            *dest = s;
        }
        CASE (const ValDir&, s)
        {
            *dest = s;
        }
        CASE (const ValString&, s)
        {
            *dest = ValString{s.v_string};
        }
        CASE (const ValEntityInt&, s)
        {
            *dest = s;
        }
        CASE (const ValEntityPtr&, s)
        {
            *dest = s;
        }
        CASE (const ValLocation&, s)
        {
            *dest = s;
        }
        CASE (const ValArea&, s)
        {
            *dest = ValArea{dup_area(s.v_area)};
        }
        CASE (const ValSpell&, s)
        {
            *dest = s;
        }
        CASE (const ValInvocationInt&, s)
        {
            *dest = s;
        }
        CASE (const ValInvocationPtr&, s)
        {
            *dest = s;
        }
        CASE (const ValFail&, s)
        {
            *dest = s;
        }
        CASE (const ValNegative1&, s)
        {
            *dest = s;
        }
    }
}

void magic_clear_var(val_t *v)
{
    MATCH (*v)
    {
        CASE (ValString&, s)
        {
            (void)s;
        }
        CASE (const ValArea&, a)
        {
            free_area(a.v_area);
        }
    }
}

static
AString show_entity(dumb_ptr<block_list> entity)
{
    switch (entity->bl_type)
    {
        case BL::PC:
            return entity->is_player()->status_key.name.to__actual();
        case BL::NPC:
            return entity->is_npc()->name;
        case BL::MOB:
            return entity->is_mob()->name;
        case BL::ITEM:
            assert (0 && "There is no way this code did what it was supposed to do!"_s);
            /* Sorry about this one... */
            // WTF? item_data is a Item, not a struct item_data
            // return ((struct item_data *) (&entity->is_item()->item_data))->name;
            abort();
        case BL::SPELL:
            return "%invocation(ERROR:this-should-not-be-an-entity)"_s;
        default:
            return "%unknown-entity"_s;
    }
}

static
void stringify(val_t *v)
{
    static earray<LString, DIR, DIR::COUNT> dirs //=
    {{
        "south"_s, "south-west"_s,
        "west"_s, "north-west"_s,
        "north"_s, "north-east"_s,
        "east"_s, "south-east"_s,
    }};
    AString buf;

    MATCH (*v)
    {
        default:
        {
            abort();
        }
        CASE (const ValUndef&, x)
        {
            (void)x;
            buf = "UNDEF"_s;
        }
        CASE (const ValInt&, x)
        {
            buf = STRPRINTF("%i"_fmt, x.v_int);
        }
        CASE (const ValString&, x)
        {
            (void)x;
            return;
        }
        CASE (const ValDir&, x)
        {
            buf = dirs[x.v_dir];
        }
        CASE (const ValEntityPtr&, x)
        {
            buf = show_entity(x.v_entity);
        }
        CASE (const ValLocation&, x)
        {
            buf = STRPRINTF("<\"%s\", %d, %d>"_fmt,
                    x.v_location.m->name_,
                    x.v_location.x,
                    x.v_location.y);
        }
        CASE (const ValArea&, x)
        {
            buf = "%area"_s;
            free_area(x.v_area);
        }
        CASE (const ValSpell&, x)
        {
            buf = x.v_spell->name;
            break;
        }
        CASE (const ValInvocationInt&, x)
        {
            dumb_ptr<invocation> invocation_ =
                map_id2bl(x.v_iid)->is_spell();
            buf = invocation_->spell->name;
        }
        CASE (const ValInvocationPtr&, x)
        {
            dumb_ptr<invocation> invocation_ =
                x.v_invocation;
            buf = invocation_->spell->name;
        }
    }

    *v = ValString{buf};
}

static
void intify(val_t *v)
{
    if (v->is<ValInt>())
        return;

    magic_clear_var(v);
    *v = ValInt{1};
}

static
dumb_ptr<area_t> area_union(dumb_ptr<area_t> area, dumb_ptr<area_t> other_area)
{
    AreaUnion a;
    a.a_union[0] = area;
    a.a_union[1] = other_area;
    int size = area->size + other_area->size;   /* Assume no overlap */
    return dumb_ptr<area_t>::make(a, size);
}

/**
 * Turns location into area, leaves other types untouched
 */
static
void make_area(val_t *v)
{
    if (ValLocation *l = v->get_if<ValLocation>())
    {
        auto a = dumb_ptr<area_t>::make(l->v_location, 1);
        *v = ValArea{a};
    }
}

static
void make_location(val_t *v)
{
    if (ValArea *a = v->get_if<ValArea>())
    {
        MATCH (*a->v_area)
        {
            CASE (const location_t&, location)
            {
                free_area(a->v_area);
                *v = ValLocation{location};
            }
        }
    }
}

static
void make_spell(val_t *v)
{
    assert(!v->is<ValInvocationInt>());
    if (ValInvocationPtr *p = v->get_if<ValInvocationPtr>())
    {
        dumb_ptr<invocation> invoc = p->v_invocation;
        if (!invoc)
        {
            *v = ValFail{};
        }
        else
        {
            *v = ValSpell{invoc->spell};
        }
    }
}

static
int fun_add(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (args[0].is<ValInt>() && args[1].is<ValInt>())
    {
        /* Integer addition */
        *result = ValInt{ARGINT(0) + ARGINT(1)};
    }
    else if (ARG_MAY_BE_AREA(0) && ARG_MAY_BE_AREA(1))
    {
        /* Area union */
        make_area(&args[0]);
        make_area(&args[1]);
        *result = ValArea{area_union(ARGAREA(0), ARGAREA(1))};
        ARGAREA(0) = nullptr; args[0] = ValUndef{};
        ARGAREA(1) = nullptr; args[1] = ValUndef{};
    }
    else
    {
        /* Anything else -> string concatenation */
        stringify(&args[0]);
        stringify(&args[1]);
        /* Yes, we could speed this up. */
        // ugh
        MString m;
        m += ARGSTR(0);
        m += ARGSTR(1);
        *result = ValString{AString(m)};
    }
    return 0;
}

static
int fun_sub(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) - ARGINT(1)};
    return 0;
}

static
int fun_mul(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) * ARGINT(1)};
    return 0;
}

static
int fun_div(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (!ARGINT(1))
        return 1;               /* division by zero */
    *result = ValInt{ARGINT(0) / ARGINT(1)};
    return 0;
}

static
int fun_mod(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (!ARGINT(1))
        return 1;               /* division by zero */
    *result = ValInt{ARGINT(0) % ARGINT(1)};
    return 0;
}

static
int fun_or(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) || ARGINT(1)};
    return 0;
}

static
int fun_and(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) && ARGINT(1)};
    return 0;
}

static
int fun_not(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{!ARGINT(0)};
    return 0;
}

static
int fun_neg(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{~ARGINT(0)};
    return 0;
}

static
int fun_gte(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (args[0].is<ValString>() || args[1].is<ValString>())
    {
        stringify(&args[0]);
        stringify(&args[1]);
        *result = ValInt{ARGSTR(0) >= ARGSTR(1)};
    }
    else
    {
        intify(&args[0]);
        intify(&args[1]);
        *result = ValInt{ARGINT(0) >= ARGINT(1)};
    }
    return 0;
}

static
int fun_lt(dumb_ptr<env_t> env, val_t *result, Slice<val_t> args)
{
    fun_gte(env, result, args);
    result->get_if<ValInt>()->v_int ^= 1;
    return 0;
}

static
int fun_gt(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (args[0].is<ValString>() || args[1].is<ValString>())
    {
        stringify(&args[0]);
        stringify(&args[1]);
        *result = ValInt{ARGSTR(0) > ARGSTR(1)};
    }
    else
    {
        intify(&args[0]);
        intify(&args[1]);
        *result = ValInt{ARGINT(0) > ARGINT(1)};
    }
    return 0;
}

static
int fun_lte(dumb_ptr<env_t> env, val_t *result, Slice<val_t> args)
{
    fun_gt(env, result, args);
    result->get_if<ValInt>()->v_int ^= 1;
    return 0;
}

static
int fun_eq(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (args[0].is<ValString>() || args[1].is<ValString>())
    {
        stringify(&args[0]);
        stringify(&args[1]);
        *result = ValInt{ARGSTR(0) == ARGSTR(1)};
    }
    else if (args[0].is<ValDir>() && args[1].is<ValDir>())
        *result = ValInt{ARGDIR(0) == ARGDIR(1)};
    else if (args[0].is<ValEntityPtr>() && args[1].is<ValEntityPtr>())
        *result = ValInt{ARGENTITY(0) == ARGENTITY(1)};
    else if (args[0].is<ValLocation>() && args[1].is<ValLocation>())
        *result = ValInt{(ARGLOCATION(0).x == ARGLOCATION(1).x
                     && ARGLOCATION(0).y == ARGLOCATION(1).y
                     && ARGLOCATION(0).m == ARGLOCATION(1).m)};
    else if (args[0].is<ValArea>() && args[1].is<ValArea>())
        *result = ValInt{ARGAREA(0) == ARGAREA(1)}; /* Probably not that great an idea... */
    else if (args[0].is<ValSpell>() && args[1].is<ValSpell>())
        *result = ValInt{ARGSPELL(0) == ARGSPELL(1)};
    else if (args[0].is<ValInvocationPtr>() && args[1].is<ValInvocationPtr>())
        *result = ValInt{ARGINVOCATION(0) == ARGINVOCATION(1)};
    else
    {
        intify(&args[0]);
        intify(&args[1]);
        *result = ValInt{ARGINT(0) == ARGINT(1)};
    }
    return 0;
}

static
int fun_ne(dumb_ptr<env_t> env, val_t *result, Slice<val_t> args)
{
    fun_eq(env, result, args);
    result->get_if<ValInt>()->v_int ^= 1;
    return 0;
}

static
int fun_bitand(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) & ARGINT(1)};
    return 0;
}

static
int fun_bitor(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) | ARGINT(1)};
    return 0;
}

static
int fun_bitxor(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) ^ ARGINT(1)};
    return 0;
}

static
int fun_bitshl(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) << ARGINT(1)};
    return 0;
}

static
int fun_bitshr(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{ARGINT(0) >> ARGINT(1)};
    return 0;
}

static
int fun_max(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{std::max(ARGINT(0), ARGINT(1))};
    return 0;
}

static
int fun_min(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{std::min(ARGINT(0), ARGINT(1))};
    return 0;
}

static
int fun_if_then_else(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ARGINT(0))
        magic_copy_var(result, &args[1]);
    else
        magic_copy_var(result, &args[2]);
    return 0;
}

void magic_area_rect(map_local **m, int *x, int *y, int *width, int *height,
        area_t& area_)
{
    MATCH (area_)
    {
        CASE (const AreaUnion&, a)
        {
            (void)a;
            abort();
        }
        CASE (const location_t&, a_loc)
        {
            *m = a_loc.m;
            *x = a_loc.x;
            *y = a_loc.y;
            *width = 1;
            *height = 1;
        }
        CASE (const AreaRect&, a_rect)
        {
            *m = a_rect.loc.m;
            *x = a_rect.loc.x;
            *y = a_rect.loc.y;
            *width = a_rect.width;
            *height = a_rect.height;
        }
        CASE (const AreaBar&, a_bar)
        {
            int tx = a_bar.loc.x;
            int ty = a_bar.loc.y;
            int twidth = a_bar.width;
            int tdepth = a_bar.width;
            *m = a_bar.loc.m;

            switch (a_bar.dir)
            {
                case DIR::S:
                    *x = tx - twidth;
                    *y = ty;
                    *width = twidth * 2 + 1;
                    *height = tdepth;
                    break;

                case DIR::W:
                    *x = tx - tdepth;
                    *y = ty - twidth;
                    *width = tdepth;
                    *height = twidth * 2 + 1;
                    break;

                case DIR::N:
                    *x = tx - twidth;
                    *y = ty - tdepth;
                    *width = twidth * 2 + 1;
                    *height = tdepth;
                    break;

                case DIR::E:
                    *x = tx;
                    *y = ty - twidth;
                    *width = tdepth;
                    *height = twidth * 2 + 1;
                    break;

                default:
                    FPRINTF(stderr,
                            "Error: Trying to compute area of NE/SE/NW/SW-facing bar"_fmt);
                    *x = tx;
                    *y = ty;
                    *width = *height = 1;
            }
        }
    }
}

int magic_location_in_area(map_local *m, int x, int y, dumb_ptr<area_t> area)
{
    MATCH (*area)
    {
        CASE (const AreaUnion&, a)
        {
            return magic_location_in_area(m, x, y, a.a_union[0])
                || magic_location_in_area(m, x, y, a.a_union[1]);
        }
        CASE (const location_t&, a_loc)
        {
            (void)a_loc;
            // TODO this can be simplified
            map_local *am;
            int ax, ay, awidth, aheight;
            magic_area_rect(&am, &ax, &ay, &awidth, &aheight, *area);
            return (am == m
                    && (x >= ax) && (y >= ay)
                    && (x < ax + awidth) && (y < ay + aheight));
        }
        CASE (const AreaRect&, a_rect)
        {
            (void)a_rect;
            // TODO this is too complicated
            map_local *am;
            int ax, ay, awidth, aheight;
            magic_area_rect(&am, &ax, &ay, &awidth, &aheight, *area);
            return (am == m
                    && (x >= ax) && (y >= ay)
                    && (x < ax + awidth) && (y < ay + aheight));
        }
        CASE (const AreaBar&, a_bar)
        {
            (void)a_bar;
            // TODO this is wrong
            map_local *am;
            int ax, ay, awidth, aheight;
            magic_area_rect(&am, &ax, &ay, &awidth, &aheight, *area);
            return (am == m
                    && (x >= ax) && (y >= ay)
                    && (x < ax + awidth) && (y < ay + aheight));
        }
    }
    abort();
}

static
int fun_is_in(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{magic_location_in_area(ARGLOCATION(0).m,
                                        ARGLOCATION(0).x,
                                        ARGLOCATION(0).y, ARGAREA(1))};
    return 0;
}

static
int fun_skill(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ENTITY_TYPE(0) != BL::PC
        // don't convert to enum until after the range check
        // (actually it would be okay, I checked)
        || ARGINT(1) < 0
        || ARGINT(1) >= static_cast<uint16_t>(MAX_SKILL))
    {
        *result = ValInt{0};
    }
    else
    {
        SkillID id = static_cast<SkillID>(ARGINT(1));
        *result = ValInt{ARGPC(0)->status.skill[id].lv};
    }
    return 0;
}

static
int fun_his_shroud(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{(ENTITY_TYPE(0) == BL::PC && ARGPC(0)->state.shroud_active)};
    return 0;
}

#define BATTLE_GETTER(name)                                             \
static                                                                  \
int fun_get_##name(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)   \
{                                                                       \
    *result = ValInt{battle_get_##name(ARGENTITY(0))};                  \
    return 0;                                                           \
}

BATTLE_GETTER(str)
BATTLE_GETTER(agi)
BATTLE_GETTER(vit)
BATTLE_GETTER(dex)
BATTLE_GETTER(luk)
BATTLE_GETTER(int)
BATTLE_GETTER(lv)
BATTLE_GETTER(hp)
BATTLE_GETTER(mdef)
BATTLE_GETTER(def)
BATTLE_GETTER(max_hp)
static
int fun_get_dir(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValDir{battle_get_dir(ARGENTITY(0))};
    return 0;
}

#define MMO_GETTER(name)                                                \
static                                                                  \
int fun_get_##name(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)   \
{                                                                       \
    if (ENTITY_TYPE(0) == BL::PC)                                       \
        *result = ValInt{ARGPC(0)->status.name};                        \
    else                                                                \
        *result = ValInt{0};                                            \
    return 0;                                                           \
}

MMO_GETTER(sp)
MMO_GETTER(max_sp)

static
int fun_name_of(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (args[0].is<ValEntityPtr>())
    {
        *result = ValString{show_entity(ARGENTITY(0))};
        return 0;
    }
    else if (args[0].is<ValSpell>())
    {
        *result = ValString{ARGSPELL(0)->name};
        return 0;
    }
    else if (args[0].is<ValInvocationPtr>())
    {
        *result = ValString{ARGINVOCATION(0)->spell->name};
        return 0;
    }
    return 1;
}

/* [Freeyorp] I'm putting this one in as name_of seems to have issues with summoned or spawned mobs. */
static
int fun_mob_id(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ENTITY_TYPE(0) != BL::MOB)
        return 1;
    *result = ValInt{unwrap<Species>(ARGMOB(0)->mob_class)};
    return 0;
}

inline
void COPY_LOCATION(block_list& dest, location_t& src)
{
    dest.bl_x = src.x;
    dest.bl_y = src.y;
    dest.bl_m = src.m;
}

inline
void COPY_LOCATION(location_t& dest, block_list& src)
{
    dest.x = src.bl_x;
    dest.y = src.bl_y;
    dest.m = src.bl_m;
}

static
int fun_location(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    location_t loc;
    COPY_LOCATION(loc, *(ARGENTITY(0)));
    *result = ValLocation{loc};
    return 0;
}

static
int fun_random(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    int delta = ARGINT(0);
    if (delta < 0)
        delta = -delta;
    if (delta == 0)
    {
        *result = ValInt{0};
        return 0;
    }
    *result = ValInt{random_::to(delta)};

    if (ARGINT(0) < 0)
        result->get_if<ValInt>()->v_int *= -1;
    return 0;
}

static
int fun_random_dir(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ARGINT(0))
        *result = ValDir{random_::choice({DIR::S, DIR::SW, DIR::W, DIR::NW, DIR::N, DIR::NE, DIR::E, DIR::SE})};
    else
        *result = ValDir{random_::choice({DIR::S, DIR::W, DIR::N, DIR::E})};
    return 0;
}

static
int fun_hash_entity(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{static_cast<int32_t>(unwrap<BlockId>(ARGENTITY(0)->bl_id))};
    return 0;
}

// ret -1: not a string, ret 1: no such item, ret 0: OK
int magic_find_item(Slice<val_t> args, int index, Item *item_, int *stackable)
{
    struct item_data *item_data;
    int must_add_sequentially;

    if (args[index].is<ValInt>())
        item_data = itemdb_exists(wrap<ItemNameId>(static_cast<uint16_t>(ARGINT(index))));
    else if (args[index].is<ValString>())
        item_data = itemdb_searchname(ARGSTR(index));
    else
        return -1;

    if (!item_data)
        return 1;

    // Very elegant.
    must_add_sequentially = (
            item_data->type == ItemType::WEAPON
            || item_data->type == ItemType::ARMOR
            || item_data->type == ItemType::_7
            || item_data->type == ItemType::_8);

    if (stackable)
        *stackable = !must_add_sequentially;

    *item_ = Item();
    item_->nameid = item_data->nameid;

    return 0;
}

static
int fun_count_item(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    dumb_ptr<map_session_data> chr = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : nullptr;
    int stackable;
    Item item;

    GET_ARG_ITEM(1, item, stackable);

    if (!chr)
        return 1;

    *result = ValInt{pc_count_all_items(chr, item.nameid)};
    return 0;
}

static
int fun_is_equipped(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    dumb_ptr<map_session_data> chr = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : nullptr;
    int stackable;
    Item item;
    bool retval = false;

    GET_ARG_ITEM(1, item, stackable);

    if (!chr)
        return 1;

    for (EQUIP i : EQUIPs)
    {
        IOff0 idx = chr->equip_index_maybe[i];
        if (idx.ok() && chr->status.inventory[idx].nameid == item.nameid)
        {
            retval = true;
            break;
        }
    }

    *result = ValInt{retval};
    return 0;
}

static
int fun_is_married(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{(ENTITY_TYPE(0) == BL::PC && ARGPC(0)->status.partner_id)};
    return 0;
}

static
int fun_is_dead(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{(ENTITY_TYPE(0) == BL::PC && pc_isdead(ARGPC(0)))};
    return 0;
}

static
int fun_is_pc(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{(ENTITY_TYPE(0) == BL::PC)};
    return 0;
}

static
int fun_partner(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ENTITY_TYPE(0) == BL::PC && ARGPC(0)->status.partner_id)
    {
        *result =
            ValEntityPtr{map_nick2sd(map_charid2nick(ARGPC(0)->status.partner_id))};
        return 0;
    }
    else
        return 1;
}

static
int fun_awayfrom(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    location_t *loc = &ARGLOCATION(0);
    int dx = dirx[ARGDIR(1)];
    int dy = diry[ARGDIR(1)];
    int distance = ARGINT(2);
    while (distance--
        && !bool(read_gatp(loc->m, loc->x + dx, loc->y + dy)
            & MapCell::UNWALKABLE))
    {
        loc->x += dx;
        loc->y += dy;
    }

    *result = ValLocation{*loc};
    return 0;
}

static
int fun_failed(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{args[0].is<ValFail>()};
    return 0;
}

static
int fun_npc(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    NpcName name = stringish<NpcName>(ARGSTR(0));
    dumb_ptr<npc_data> npc = npc_name2id(name);
    *result = ValEntityPtr{npc};
    return npc == nullptr;
}

static
int fun_pc(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    CharName name = stringish<CharName>(ARGSTR(0));
    dumb_ptr<map_session_data> chr = map_nick2sd(name);
    *result = ValEntityPtr{chr};
    return chr == nullptr;
}

static
int fun_distance(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ARGLOCATION(0).m != ARGLOCATION(1).m)
        *result = ValInt{0x7fffffff};
    else
        *result = ValInt{std::max(abs(ARGLOCATION(0).x - ARGLOCATION(1).x),
                         abs(ARGLOCATION(0).y - ARGLOCATION(1).y))};
    return 0;
}

static
int fun_rdistance(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ARGLOCATION(0).m != ARGLOCATION(1).m)
        *result = ValInt{0x7fffffff};
    else
    {
        int dx = ARGLOCATION(0).x - ARGLOCATION(1).x;
        int dy = ARGLOCATION(0).y - ARGLOCATION(1).y;
        *result = ValInt{static_cast<int>(sqrt((dx * dx) + (dy * dy)))};
    }
    return 0;
}

static
int fun_anchor(dumb_ptr<env_t> env, val_t *result, Slice<val_t> args)
{
    dumb_ptr<teleport_anchor_t> anchor = magic_find_anchor(ARGSTR(0));

    if (!anchor)
        return 1;

    magic_eval(env, result, anchor->location);

    make_area(result);
    if (!result->is<ValArea>())
    {
        magic_clear_var(result);
        return 1;
    }

    return 0;
}

static
int fun_line_of_sight(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    block_list e1, e2;

    COPY_LOCATION(e1, ARGLOCATION(0));
    COPY_LOCATION(e2, ARGLOCATION(1));

    *result = ValInt{battle_check_range(dumb_ptr<block_list>(&e1), dumb_ptr<block_list>(&e2), 0)};

    return 0;
}

void magic_random_location(location_t *dest, dumb_ptr<area_t> area)
{
    MATCH (*area)
    {
        CASE (const AreaUnion&, a)
        {
            if (random_::chance({a.a_union[0]->size, area->size}))
                magic_random_location(dest, a.a_union[0]);
            else
                magic_random_location(dest, a.a_union[1]);
        }
        CASE (const location_t&, a_loc)
        {
            (void)a_loc;
            // TODO this can be simplified
            map_local *m;
            int x, y, w, h;
            magic_area_rect(&m, &x, &y, &w, &h, *area);

            if (w <= 1)
                w = 1;

            if (h <= 1)
                h = 1;

            // This is not exactly the same as the old logic,
            // but it's better.
            auto pair = map_randfreecell(m, x, y, w, h);

            dest->m = m;
            dest->x = pair.first;
            dest->y = pair.second;
        }
        CASE (const AreaRect&, a_rect)
        {
            (void)a_rect;
            // TODO this can be simplified
            map_local *m;
            int x, y, w, h;
            magic_area_rect(&m, &x, &y, &w, &h, *area);

            if (w <= 1)
                w = 1;

            if (h <= 1)
                h = 1;

            // This is not exactly the same as the old logic,
            // but it's better.
            auto pair = map_randfreecell(m, x, y, w, h);

            dest->m = m;
            dest->x = pair.first;
            dest->y = pair.second;
        }
        CASE (const AreaBar&, a_bar)
        {
            (void)a_bar;
            // TODO this is wrong
            map_local *m;
            int x, y, w, h;
            magic_area_rect(&m, &x, &y, &w, &h, *area);

            if (w <= 1)
                w = 1;

            if (h <= 1)
                h = 1;

            // This is not exactly the same as the old logic,
            // but it's better.
            auto pair = map_randfreecell(m, x, y, w, h);

            dest->m = m;
            dest->x = pair.first;
            dest->y = pair.second;
        }
    }
}

static
int fun_pick_location(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    location_t loc;
    magic_random_location(&loc, ARGAREA(0));
    *result = ValLocation{loc};
    return 0;
}

static
int fun_read_script_int(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    dumb_ptr<block_list> subject_p = ARGENTITY(0);
    VarName var_name = stringish<VarName>(ARGSTR(1));
    int array_index = 0;

    if (subject_p->bl_type != BL::PC)
        return 1;

    *result = ValInt{get_script_var_i(subject_p->is_player(), var_name, array_index)};
    return 0;
}

static
int fun_read_script_str(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    dumb_ptr<block_list> subject_p = ARGENTITY(0);
    VarName var_name = stringish<VarName>(ARGSTR(1));
    int array_index = 0;

    if (subject_p->bl_type != BL::PC)
        return 1;

    *result = ValString{get_script_var_s(subject_p->is_player(), var_name, array_index)};
    return 0;
}

static
int fun_rbox(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    location_t loc = ARGLOCATION(0);
    int radius = ARGINT(1);

    AreaRect a_rect;
    a_rect.loc.m = loc.m;
    a_rect.loc.x = loc.x - radius;
    a_rect.loc.y = loc.y - radius;
    a_rect.width = radius * 2 + 1;
    a_rect.height = radius * 2 + 1;
    *result = ValArea{dumb_ptr<area_t>::make(a_rect, a_rect.width * a_rect.height)};

    return 0;
}

static
int fun_running_status_update(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    if (ENTITY_TYPE(0) != BL::PC && ENTITY_TYPE(0) != BL::MOB)
        return 1;

    StatusChange sc = static_cast<StatusChange>(ARGINT(1));
    *result = ValInt{bool(battle_get_sc_data(ARGENTITY(0))[sc].timer)};
    return 0;
}

static
int fun_status_option(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{(bool((ARGPC(0))->status.option & static_cast<Option>(ARGINT(1))))};
    return 0;
}

static
int fun_element(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{static_cast<int>(battle_get_element(ARGENTITY(0)).element)};
    return 0;
}

static
int fun_element_level(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{battle_get_element(ARGENTITY(0)).level};
    return 0;
}

static
int fun_is_exterior(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
#warning "Evil assumptions!"
    *result = ValInt{ARGLOCATION(0).m->name_[4] == '1'};
    return 0;
}

static
int fun_contains_string(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{nullptr != strstr(ARGSTR(0).c_str(), ARGSTR(1).c_str())};
    return 0;
}

static
int fun_strstr(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    const char *offset = strstr(ARGSTR(0).c_str(), ARGSTR(1).c_str());
    *result = ValInt{static_cast<int32_t>(offset - ARGSTR(0).c_str())};
    return offset == nullptr;
}

static
int fun_strlen(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{static_cast<int32_t>(strlen(ARGSTR(0).c_str()))};
    return 0;
}

static
int fun_substr(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    RString src = ARGSTR(0);
    int offset = ARGINT(1);
    int len = ARGINT(2);

    if (len < 0)
        len = 0;
    if (offset < 0)
        offset = 0;

    if (offset > src.size())
        offset = src.size();

    if (offset + len > src.size())
        len = src.size() - offset;

    auto begin = src.begin() + offset;
    auto end = begin + len;
    *result = ValString{RString(begin, end)};

    return 0;
}

static
int fun_sqrt(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    *result = ValInt{static_cast<int>(sqrt(ARGINT(0)))};
    return 0;
}

static
int fun_map_level(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
#warning "Evil assumptions!"
    *result = ValInt{ARGLOCATION(0).m->name_[4] - '0'};
    return 0;
}

static
int fun_map_nr(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
#warning "Evil assumptions!"
    MapName mapname = ARGLOCATION(0).m->name_;

    *result = ValInt{((mapname[0] - '0') * 100)
        + ((mapname[1] - '0') * 10) + ((mapname[2] - '0'))};
    return 0;
}

static
int fun_dir_towards(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    int dx;
    int dy;

    if (ARGLOCATION(0).m != ARGLOCATION(1).m)
        return 1;

    dx = ARGLOCATION(1).x - ARGLOCATION(0).x;
    dy = ARGLOCATION(1).y - ARGLOCATION(0).y;

    if (ARGINT(1))
    {
        /* 8-direction mode */
        if (abs(dx) > abs(dy) * 2)
        {                       /* east or west */
            if (dx < 0)
                *result = ValDir{DIR::W};
            else
                *result = ValDir{DIR::E};
        }
        else if (abs(dy) > abs(dx) * 2)
        {                       /* north or south */
            if (dy > 0)
                *result = ValDir{DIR::S};
            else
                *result = ValDir{DIR::N};
        }
        else if (dx < 0)
        {                       /* north-west or south-west */
            if (dy < 0)
                *result = ValDir{DIR::NW};
            else
                *result = ValDir{DIR::SW};
        }
        else
        {                       /* north-east or south-east */
            if (dy < 0)
                *result = ValDir{DIR::NE};
            else
                *result = ValDir{DIR::SE};
        }
    }
    else
    {
        /* 4-direction mode */
        if (abs(dx) > abs(dy))
        {                       /* east or west */
            if (dx < 0)
                *result = ValDir{DIR::W};
            else
                *result = ValDir{DIR::E};
        }
        else
        {                       /* north or south */
            if (dy > 0)
                *result = ValDir{DIR::S};
            else
                *result = ValDir{DIR::N};
        }
    }

    return 0;
}

static
int fun_extract_healer_xp(dumb_ptr<env_t>, val_t *result, Slice<val_t> args)
{
    dumb_ptr<map_session_data> sd = (ENTITY_TYPE(0) == BL::PC) ? ARGPC(0) : nullptr;

    if (!sd)
        *result = ValInt{0};
    else
        *result = ValInt{pc_extract_healer_exp(sd, ARGINT(1))};
    return 0;
}

#define MAGIC_FUNCTION(name, args, ret, impl) {name, {name, args, ret, impl}}
#define MAGIC_FUNCTION1(name, args, ret) MAGIC_FUNCTION(#name##_s, args, ret, fun_##name)
static // should be LString, but no heterogenous lookup yet
std::map<ZString, fun_t> functions =
{
    MAGIC_FUNCTION("+"_s, ".."_s, '.', fun_add),
    MAGIC_FUNCTION("-"_s, "ii"_s, 'i', fun_sub),
    MAGIC_FUNCTION("*"_s, "ii"_s, 'i', fun_mul),
    MAGIC_FUNCTION("/"_s, "ii"_s, 'i', fun_div),
    MAGIC_FUNCTION("%"_s, "ii"_s, 'i', fun_mod),
    MAGIC_FUNCTION("||"_s, "ii"_s, 'i', fun_or),
    MAGIC_FUNCTION("&&"_s, "ii"_s, 'i', fun_and),
    MAGIC_FUNCTION("<"_s, ".."_s, 'i', fun_lt),
    MAGIC_FUNCTION(">"_s, ".."_s, 'i', fun_gt),
    MAGIC_FUNCTION("<="_s, ".."_s, 'i', fun_lte),
    MAGIC_FUNCTION(">="_s, ".."_s, 'i', fun_gte),
    MAGIC_FUNCTION("=="_s, ".."_s, 'i', fun_eq),
    MAGIC_FUNCTION("!="_s, ".."_s, 'i', fun_ne),
    MAGIC_FUNCTION("|"_s, ".."_s, 'i', fun_bitor),
    MAGIC_FUNCTION("&"_s, "ii"_s, 'i', fun_bitand),
    MAGIC_FUNCTION("^"_s, "ii"_s, 'i', fun_bitxor),
    MAGIC_FUNCTION("<<"_s, "ii"_s, 'i', fun_bitshl),
    MAGIC_FUNCTION(">>"_s, "ii"_s, 'i', fun_bitshr),
    MAGIC_FUNCTION1(not, "i"_s, 'i'),
    MAGIC_FUNCTION1(neg, "i"_s, 'i'),
    MAGIC_FUNCTION1(max, "ii"_s, 'i'),
    MAGIC_FUNCTION1(min, "ii"_s, 'i'),
    MAGIC_FUNCTION1(is_in, "la"_s, 'i'),
    MAGIC_FUNCTION1(if_then_else, "i__"_s, '_'),
    MAGIC_FUNCTION1(skill, "ei"_s, 'i'),
    MAGIC_FUNCTION("str"_s, "e"_s, 'i', fun_get_str),
    MAGIC_FUNCTION("agi"_s, "e"_s, 'i', fun_get_agi),
    MAGIC_FUNCTION("vit"_s, "e"_s, 'i', fun_get_vit),
    MAGIC_FUNCTION("dex"_s, "e"_s, 'i', fun_get_dex),
    MAGIC_FUNCTION("luk"_s, "e"_s, 'i', fun_get_luk),
    MAGIC_FUNCTION("int"_s, "e"_s, 'i', fun_get_int),
    MAGIC_FUNCTION("level"_s, "e"_s, 'i', fun_get_lv),
    MAGIC_FUNCTION("mdef"_s, "e"_s, 'i', fun_get_mdef),
    MAGIC_FUNCTION("def"_s, "e"_s, 'i', fun_get_def),
    MAGIC_FUNCTION("hp"_s, "e"_s, 'i', fun_get_hp),
    MAGIC_FUNCTION("max_hp"_s, "e"_s, 'i', fun_get_max_hp),
    MAGIC_FUNCTION("sp"_s, "e"_s, 'i', fun_get_sp),
    MAGIC_FUNCTION("max_sp"_s, "e"_s, 'i', fun_get_max_sp),
    MAGIC_FUNCTION("dir"_s, "e"_s, 'd', fun_get_dir),
    MAGIC_FUNCTION1(name_of, "."_s, 's'),
    MAGIC_FUNCTION1(mob_id, "e"_s, 'i'),
    MAGIC_FUNCTION1(location, "e"_s, 'l'),
    MAGIC_FUNCTION1(random, "i"_s, 'i'),
    MAGIC_FUNCTION1(random_dir, "i"_s, 'd'),
    MAGIC_FUNCTION1(hash_entity, "e"_s, 'i'),
    MAGIC_FUNCTION1(is_married, "e"_s, 'i'),
    MAGIC_FUNCTION1(partner, "e"_s, 'e'),
    MAGIC_FUNCTION1(awayfrom, "ldi"_s, 'l'),
    MAGIC_FUNCTION1(failed, "_"_s, 'i'),
    MAGIC_FUNCTION1(pc, "s"_s, 'e'),
    MAGIC_FUNCTION1(npc, "s"_s, 'e'),
    MAGIC_FUNCTION1(distance, "ll"_s, 'i'),
    MAGIC_FUNCTION1(rdistance, "ll"_s, 'i'),
    MAGIC_FUNCTION1(anchor, "s"_s, 'a'),
    MAGIC_FUNCTION("random_location"_s, "a"_s, 'l', fun_pick_location),
    MAGIC_FUNCTION("script_int"_s, "es"_s, 'i', fun_read_script_int),
    MAGIC_FUNCTION("script_str"_s, "es"_s, 's', fun_read_script_str),
    MAGIC_FUNCTION1(rbox, "li"_s, 'a'),
    MAGIC_FUNCTION1(count_item, "e."_s, 'i'),
    MAGIC_FUNCTION1(line_of_sight, "ll"_s, 'i'),
    MAGIC_FUNCTION1(running_status_update, "ei"_s, 'i'),
    MAGIC_FUNCTION1(status_option, "ei"_s, 'i'),
    MAGIC_FUNCTION1(element, "e"_s, 'i'),
    MAGIC_FUNCTION1(element_level, "e"_s, 'i'),
    MAGIC_FUNCTION1(his_shroud, "e"_s, 'i'),
    MAGIC_FUNCTION1(is_equipped, "e."_s, 'i'),
    MAGIC_FUNCTION1(is_exterior, "l"_s, 'i'),
    MAGIC_FUNCTION1(contains_string, "ss"_s, 'i'),
    MAGIC_FUNCTION1(strstr, "ss"_s, 'i'),
    MAGIC_FUNCTION1(strlen, "s"_s, 'i'),
    MAGIC_FUNCTION1(substr, "sii"_s, 's'),
    MAGIC_FUNCTION1(sqrt, "i"_s, 'i'),
    MAGIC_FUNCTION1(map_level, "l"_s, 'i'),
    MAGIC_FUNCTION1(map_nr, "l"_s, 'i'),
    MAGIC_FUNCTION1(dir_towards, "lli"_s, 'd'),
    MAGIC_FUNCTION1(is_dead, "e"_s, 'i'),
    MAGIC_FUNCTION1(is_pc, "e"_s, 'i'),
    MAGIC_FUNCTION("extract_healer_experience"_s, "ei"_s, 'i', fun_extract_healer_xp),
};

fun_t *magic_get_fun(ZString name)
{
    auto it = functions.find(name);
    if (it == functions.end())
        return nullptr;
    return &it->second;
}

// 1 on failure
static
int eval_location(dumb_ptr<env_t> env, location_t *dest, const e_location_t *expr)
{
    val_t m, x, y;
    magic_eval(env, &m, expr->m);
    magic_eval(env, &x, expr->x);
    magic_eval(env, &y, expr->y);

    if (m.is<ValString>()
        && x.is<ValInt>() && y.is<ValInt>())
    {
        MapName name = VString<15>(ZString(m.get_if<ValString>()->v_string));
        map_local *map_id = map_mapname2mapid(name);
        magic_clear_var(&m);
        if (!map_id)
            return 1;
        dest->m = map_id;
        dest->x = x.get_if<ValInt>()->v_int;
        dest->y = y.get_if<ValInt>()->v_int;
        return 0;
    }
    else
    {
        magic_clear_var(&m);
        magic_clear_var(&x);
        magic_clear_var(&y);
        return 1;
    }
}

static
dumb_ptr<area_t> eval_area(dumb_ptr<env_t> env, const e_area_t& expr_)
{
    MATCH (expr_)
    {
        CASE (const e_location_t&, a_loc)
        {
            location_t loc;
            int size = 1;
            if (eval_location(env, &loc, &a_loc))
            {
                return nullptr;
            }
            else
            {
                return dumb_ptr<area_t>::make(loc, size);
            }
        }
        CASE (const ExprAreaUnion&, a)
        {
            AreaUnion u;
            bool fail = false;
            for (int i = 0; i < 2; i++)
            {
                u.a_union[i] = eval_area(env, *a.a_union[i]);
                if (!u.a_union[i])
                    fail = true;
            }

            if (fail)
            {
                for (int i = 0; i < 2; i++)
                {
                    if (u.a_union[i])
                        free_area(u.a_union[i]);
                }
                return nullptr;
            }
            int size = u.a_union[0]->size + u.a_union[1]->size;
            return dumb_ptr<area_t>::make(u, size);
        }
        CASE (const ExprAreaRect&, a_rect)
        {
            val_t width, height;
            magic_eval(env, &width, a_rect.width);
            magic_eval(env, &height, a_rect.height);

            AreaRect a_rect_;
            if (width.is<ValInt>()
                && height.is<ValInt>()
                && !eval_location(env, &(a_rect_.loc),
                                   &a_rect.loc))
            {
                a_rect_.width = width.get_if<ValInt>()->v_int;
                a_rect_.height = height.get_if<ValInt>()->v_int;

                int size = a_rect_.width * a_rect_.height;
                magic_clear_var(&width);
                magic_clear_var(&height);
                return dumb_ptr<area_t>::make(a_rect_, size);
            }
            else
            {
                magic_clear_var(&width);
                magic_clear_var(&height);
                return nullptr;
            }
        }
        CASE (const ExprAreaBar&, a_bar)
        {
            val_t width, depth, dir;
            magic_eval(env, &width, a_bar.width);
            magic_eval(env, &depth, a_bar.depth);
            magic_eval(env, &dir, a_bar.dir);

            AreaBar a_bar_;
            if (width.is<ValInt>()
                && depth.is<ValInt>()
                && dir.is<ValDir>()
                && !eval_location(env, &a_bar_.loc,
                                   &a_bar.loc))
            {
                a_bar_.width = width.get_if<ValInt>()->v_int;
                a_bar_.depth = depth.get_if<ValInt>()->v_int;
                a_bar_.dir = dir.get_if<ValDir>()->v_dir;

                int size = (a_bar_.width * 2 + 1) * a_bar_.depth;
                magic_clear_var(&width);
                magic_clear_var(&depth);
                magic_clear_var(&dir);
                return dumb_ptr<area_t>::make(a_bar_, size);
            }
            else
            {
                magic_clear_var(&width);
                magic_clear_var(&depth);
                magic_clear_var(&dir);
                return nullptr;
            }
        }
    }
    abort();
}

// This is called on arguments with begin=true,
// and on the return value with begin=false.
// In both cases, the ambiguous types are in pointer mode.
static
bool type_key_matches(char ty_key, val_t *arg, bool begin)
{
    switch (ty_key)
    {
        case 'i':
            if (begin)
                intify(arg);
            return arg->is<ValInt>();
        case 'd':
            return arg->is<ValDir>();
        case 's':
            if (begin)
                stringify(arg);
            return arg->is<ValString>();
        case 'e':
            return arg->is<ValEntityPtr>();
        case 'l':
            if (begin)
                make_location(arg);
            return arg->is<ValLocation>();
        case 'a':
            if (begin)
                make_area(arg);
            return arg->is<ValArea>();
        case 'S':
            if (begin)
                make_spell(arg);
            return arg->is<ValSpell>();
        case 'I':
            return arg->is<ValInvocationPtr>();
        default:
            return true;
    }
}

int magic_signature_check(ZString opname, ZString funname, ZString signature,
        Slice<val_t> args, int line, int column)
{
    int i;
    for (i = 0; i < args.size(); i++)
    {
        val_t *arg = &args[i];

        // whoa, it turns out the second p *does* shadow this one
        if (ValEntityInt *p1 = arg->get_if<ValEntityInt>())
        {
            /* Dereference entities in preparation for calling function */
            dumb_ptr<block_list> ent = map_id2bl(p1->v_eid);
            if (ent)
            {
                *arg = ValEntityPtr{ent};
            }
            else
            {
                *arg = ValFail{};
            }
        }
        else if (ValInvocationInt *p2 = arg->get_if<ValInvocationInt>())
        {
            dumb_ptr<invocation> invoc = map_id2bl(p2->v_iid)->is_spell();
            if (invoc)
            {
                *arg = ValInvocationPtr{invoc};
            }
            else
            {
                *arg = ValFail();
            }
        }

        char ty_key = signature[i];
        if (!ty_key)
        {
            FPRINTF(stderr,
                    "[magic-eval]:  L%d:%d: Too many arguments (%zu) to %s `%s'\n"_fmt,
                    line, column, args.size(), opname, funname);
            return 1;
        }

        if (arg->is<ValFail>() && ty_key != '_')
            return 1;           /* Fail `in a sane way':  This is a perfectly permissible error */

        // this also does conversions now
        if (type_key_matches(ty_key, arg, true))
            continue;

        if (arg->is<ValUndef>())
        {
            FPRINTF(stderr,
                    "[magic-eval]:  L%d:%d: Argument #%d to %s `%s' undefined\n"_fmt,
                    line, column, i + 1, opname, funname);
            return 1;
        }


        {                       /* Coercion failed? */
            if (!arg->is<ValFail>())
            {
                FPRINTF(stderr,
                        "[magic-eval]:  L%d:%d: Argument #%d to %s `%s' of incorrect type (sorry, types aren't integers anymore)\n"_fmt,
                        line, column, i + 1, opname, funname);
            }
            return 1;
        }
    }

    return 0;
}

void magic_eval(dumb_ptr<env_t> env, val_t *dest, dumb_ptr<expr_t> expr)
{
    MATCH (*expr)
    {
        CASE (const val_t&, e_val)
        {
            magic_copy_var(dest, &e_val);
        }

        CASE (const e_location_t&, e_location)
        {
            location_t loc;
            if (eval_location(env, &loc, &e_location))
                *dest = ValFail();
            else
                *dest = ValLocation{loc};
        }
        CASE (const e_area_t&, e_area)
        {
            if (dumb_ptr<area_t> area = eval_area(env, e_area))
                *dest = ValArea{area};
            else
                *dest = ValFail();
        }
        CASE (const ExprFunApp&, e_funapp)
        {
            val_t arguments[MAX_ARGS];
            int args_nr = e_funapp.args_nr;
            int i;
            fun_t *f = e_funapp.funp;

            for (i = 0; i < args_nr; ++i)
                magic_eval(env, &arguments[i], e_funapp.args[i]);
            if (magic_signature_check("function"_s, f->name, f->signature, Slice<val_t>(arguments, args_nr),
                        e_funapp.line_nr, e_funapp.column)
                    || f->fun(env, dest, Slice<val_t>(arguments, args_nr)))
                *dest = ValFail();
            else
            {
                assert (!dest->is<ValInvocationPtr>());
                assert (!dest->is<ValInvocationInt>());
                assert (!dest->is<ValEntityInt>());
                assert (type_key_matches(f->ret_ty, dest, false));

                /* translate entity back into persistent int */
                if (ValEntityPtr *ent = dest->get_if<ValEntityPtr>())
                {
                    if (ent->v_entity)
                        *dest = ValEntityInt{ent->v_entity->bl_id};
                    else
                        *dest = ValFail();
                }
                // what about invocation?
            }

            for (i = 0; i < args_nr; ++i)
                magic_clear_var(&arguments[i]);
        }
        CASE (const ExprId&, e)
        {
            val_t& v = env->VAR(e.e_id);
            magic_copy_var(dest, &v);
        }
        CASE (const ExprField&, e_field)
        {
            val_t v;
            int id = e_field.id;
            magic_eval(env, &v, e_field.expr);

            assert(!v.is<ValInvocationPtr>());
            if (ValInvocationInt *ii = v.get_if<ValInvocationInt>())
            {
                dumb_ptr<invocation> t = map_id2bl(ii->v_iid)->is_spell();

                if (!t)
                    *dest = ValUndef();
                else
                {
                    val_t& val = t->env->VAR(id);
                    magic_copy_var(dest, &val);
                }
            }
            else
            {
                FPRINTF(stderr,
                        "[magic] Attempt to access field %s on non-spell\n"_fmt,
                        env->base_env->varv[id].name);
                *dest = ValFail();
            }
        }
    }
}

int magic_eval_int(dumb_ptr<env_t> env, dumb_ptr<expr_t> expr)
{
    val_t result;
    magic_eval(env, &result, expr);

    if (result.is<ValFail>() || result.is<ValUndef>())
        return 0;

    intify(&result);

    return result.get_if<ValInt>()->v_int;
}

AString magic_eval_str(dumb_ptr<env_t> env, dumb_ptr<expr_t> expr)
{
    val_t result;
    magic_eval(env, &result, expr);

    if (result.is<ValFail>() || result.is<ValUndef>())
        return "?"_s;

    stringify(&result);

    return result.get_if<ValString>()->v_string;
}
} // namespace magic
} // namespace tmwa
