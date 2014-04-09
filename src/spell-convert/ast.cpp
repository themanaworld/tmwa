#include "ast.hpp"
//    ast.cpp - Hacky converter between magic formats.
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../io/cxxstdio.hpp"

#include "../poison.hpp"

void Constant::dump()
{
    PRINTF("(CONST %s\n", name);
    body->show();
    PRINTF(")\n");
}
void Teleport::dump()
{
    PRINTF("(TELEPORT-ANCHOR %s\n", name);
    ident->show();
    body->show();
    PRINTF(")\n");
}
void Procedure::dump()
{
    PRINTF("(PROCEDURE %s\n", name);
    PRINTF("(");
    for (RString& a : *args)
        PRINTF(" %s ", a);
    PRINTF(")\n");
    for (Effect *f : *body)
    {
        f->print();
    }
    PRINTF(")\n");
}
void Spell::dump()
{
    PRINTF("(SPELL \n");
    PRINTF("(");
    for (RString fl : *flags)
        PRINTF(" %s ", fl);
    PRINTF(")");
    PRINTF("%s", name);
    ident->show();
    PRINTF("( %s %s )", arg->vartype, arg->varname);
    for (Assignment *a : *body->lets)
    {
        PRINTF("(LET %s ", a->name);
        a->body->show();
        PRINTF(")\n");
    }
    for (SpellBod *b : *body->body)
    {
        b->say();
    }
    PRINTF(")\n");
}
void Assignment::dump()
{
    PRINTF("(SET %s\n", name);
    body->show();
    PRINTF(")\n");
}

void EffectList::print()
{
    PRINTF("(BLOCK\n");
    for (Effect *ef : *body)
        ef->print();
    PRINTF(")\n");
}
void SimpleEffect::print()
{
    PRINTF("( %s )", text);
}
void ScriptEffect::print()
{
    PRINTF("(SCRIPT %s )", text);
}
void Assignment::print()
{
    PRINTF("(SET %s\n", name);
    body->show();
    PRINTF(")\n");
}
void ForeachEffect::print()
{
    PRINTF("(FOREACH %s %s ", selection, var);
    expr->show();
    PRINTF("\n");
    effect->print();
    PRINTF(")");
}
void ForEffect::print()
{
    PRINTF("(FOR %s", var);
    low->show();
    high->show();
    effect->print();
    PRINTF(")");
}
void IfEffect::print()
{
    PRINTF("(IF ");
    cond->show();
    if_true->print();
    if (if_false_maybe)
        if_false_maybe->print();
    PRINTF(")");
}
void ExplicitCallEffect::print()
{
    PRINTF("(CALL %s ", userfunc);
    for (Expression *x : *args)
        x->show();
    PRINTF(")");
}
void SleepEffect::print()
{
    PRINTF("(WAIT ");
    time_->show();
    PRINTF(")");
}
void CallExpr::print()
{
    PRINTF("(%s ", func);
    for (Expression *x : *args)
        x->show();
    PRINTF(")");
}

void SimpleExpr::show()
{
    PRINTF(" %s ", content);
}
void BinExpr::show()
{
    PRINTF("(%s ", op);
    left->show();
    right->show();
    PRINTF(")");
}
void CallExpr::show()
{
    PRINTF("(%s ", func);
    for (Expression *x : *args)
        x->show();
    PRINTF(")");
}
void AreaLoc::show()
{
    PRINTF("(@ ");
    loc->map->show();
    loc->x->show();
    loc->y->show();
    PRINTF(")");
}
void AreaRect::show()
{
    PRINTF("(@+ ");
    AreaLoc{loc}.show();
    width->show();
    height->show();
    PRINTF(")");
}
void AreaBar::show()
{
    PRINTF("(TOWARD ");
    AreaLoc{loc}.show();
    dir->show();
    width->show();
    depth->show();
    PRINTF(")");
}

void SpellBodGuarded::say()
{
    PRINTF("(=> ");
    guard->declare();
    body->say();
    PRINTF(")");
}
void SpellBodList::say()
{
    PRINTF("(|\n");
    for (SpellBod *b : *body)
        b->say();
    PRINTF(")");
}
void SpellBodEffect::say()
{
    PRINTF("(EFFECT\n");
    for (Effect *f : *body)
        f->print();
    if (maybe_trigger)
    {
        PRINTF("(ATTRIGGER\n");
        for (Effect *f : *maybe_trigger)
            f->print();
        PRINTF(")");
    }
    if (maybe_end)
    {
        PRINTF("(ATEND\n");
        for (Effect *f : *maybe_end)
            f->print();
        PRINTF(")");
    }
    PRINTF(")");
}

void SpellGuardOr::declare()
{
    PRINTF("(OR\n");
    for (SpellGuard *sg : *any)
        sg->declare();
    PRINTF(")");
}
void SpellGuardList::declare()
{
    PRINTF("(GUARD\n");
    for (SpellGuard *sg : *all)
        sg->declare();
    PRINTF(")");
}
void SpellGuardRequire::declare()
{
    PRINTF("(REQUIRE ");
    expr->show();
    PRINTF(")");
}
static
void do_item(Item *itm)
{
    if (itm->count)
        PRINTF("( %s %s )", itm->count, itm->item);
    else
        PRINTF(" %s ", itm->item);
}
void SpellGuardCatalysts::declare()
{
    PRINTF("(CATALYSTS ");
    for (Item *itm : *items)
        do_item(itm);
    PRINTF(")");
}
void SpellGuardComponents::declare()
{
    PRINTF("(COMPONENTS ");
    for (Item *itm : *items)
        do_item(itm);
    PRINTF(")");
}
void SpellGuardMana::declare()
{
    PRINTF("(MANA ");
    sp->show();
    PRINTF(")");
}
void SpellGuardCasttime::declare()
{
    PRINTF("(CASTTIME ");
    time_->show();
    PRINTF(")");
}
