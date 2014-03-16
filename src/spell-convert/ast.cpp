#include "ast.hpp"

#include "../io/cxxstdio.hpp"

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
    time->show();
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
    time->show();
    PRINTF(")");
}
