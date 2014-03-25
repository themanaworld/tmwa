#ifndef AST_HPP
#define AST_HPP

# include <deque>
# include <vector>

# include "../strings/rstring.hpp"

#if __GNUC__ == 4 && __GNUC_MINOR__ == 6
#define override
#endif

// We just leak
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

struct TopLevel;
struct Constant;
struct Teleport;
struct Procedure;
struct Spell;
struct SpellArg;
struct Effect;
struct EffectList;
struct SimpleEffect;
struct ScriptEffect;
struct Assignment;
struct ForeachEffect;
struct ForEffect;
struct IfEffect;
struct ExplicitCallEffect;
struct SleepEffect;
struct SpellDef;
struct SpellBod;
struct SpellBodGuarded;
struct SpellBodList;
struct SpellBodEffect;
struct SpellGuard;
struct SpellGuardOr;
struct SpellGuardList;
struct SpellGuardRequire;
struct SpellGuardCatalysts;
struct SpellGuardComponents;
struct SpellGuardMana;
struct SpellGuardCasttime;
struct Item;
struct Expression;
struct SimpleExpr;
struct BinExpr;
struct CallExpr;
struct Location;
struct AreaLoc;
struct AreaRect;
struct AreaBar;


struct TopLevel
{
    virtual void dump() = 0;
};

struct Constant : TopLevel
{
    RString name;
    Expression *body;

    Constant(RString n, Expression *b)
    : name(n), body(b)
    {}

    virtual void dump() override;
};

struct Teleport : TopLevel
{
    RString name;
    Expression *ident;
    Expression *body;

    Teleport(RString n, Expression *i, Expression *b)
    : name(n), ident(i), body(b)
    {}

    virtual void dump() override;
};

struct Procedure : TopLevel
{
    RString name;
    std::vector<RString> *args;
    std::deque<Effect *> *body;

    Procedure(RString n, std::vector<RString> *a, std::deque<Effect *> *b)
    : name(n), args(a), body(b)
    {}

    virtual void dump() override;
};

struct Spell : TopLevel
{
    std::vector<RString> *flags;
    RString name;
    SpellArg *arg;
    Expression *ident;
    SpellDef *body;

    Spell(std::vector<RString> *f, RString n, SpellArg *a, Expression *i, SpellDef *b)
    : flags(f), name(n), arg(a), ident(i), body(b)
    {}

    virtual void dump() override;
};

struct SpellArg
{
    RString varname;
    RString vartype;

    SpellArg() : varname(), vartype() {}
    SpellArg(RString n, RString t) : varname(n), vartype(t) {}
};

struct Effect
{
    virtual void print() = 0;
};

struct EffectList : Effect
{
    std::deque<Effect *> *body;

    EffectList(std::deque<Effect *> *b)
    : body(b)
    {}

    virtual void print() override;
};
struct SimpleEffect : Effect
{
    RString text;

    SimpleEffect(RString t) : text(t) {}

    virtual void print() override;
};
struct ScriptEffect : Effect
{
    RString text;

    ScriptEffect(RString t) : text(t) {}

    virtual void print() override;
};

struct Assignment : TopLevel, Effect
{
    RString name;
    Expression *body;

    Assignment(RString n, Expression *b)
    : name(n), body(b)
    {}

    // toplevel
    virtual void dump() override;
    // effect
    virtual void print() override;
};

struct ForeachEffect : Effect
{
    RString selection;
    RString var;
    Expression *expr;
    Effect *effect;

    ForeachEffect(RString s, RString v, Expression *x, Effect *f)
    : selection(s), var(v), expr(x), effect(f)
    {}

    virtual void print() override;
};

struct ForEffect : Effect
{
    RString var;
    Expression *low;
    Expression *high;
    Effect *effect;

    ForEffect(RString v, Expression *l, Expression *h, Effect *f)
    : var(v), low(l), high(h), effect(f)
    {}

    virtual void print() override;
};

struct IfEffect : Effect
{
    Expression *cond;
    Effect *if_true;
    Effect *if_false_maybe;

    IfEffect(Expression *c, Effect *t, Effect *f=nullptr)
    : cond(c), if_true(t), if_false_maybe(f)
    {}

    virtual void print() override;
};

struct ExplicitCallEffect : Effect
{
    RString userfunc;
    std::vector<Expression *> *args;

    ExplicitCallEffect(RString f, std::vector<Expression *> *a)
    : userfunc(f), args(a)
    {}

    virtual void print() override;
};

struct SleepEffect : Effect
{
    Expression *time;

    SleepEffect(Expression *t)
    : time(t)
    {}

    virtual void print() override;
};

struct SpellDef
{
    std::vector<Assignment *> *lets;
    std::vector<SpellBod *> *body;
};

struct SpellBod
{
    virtual void say() = 0;
};

struct SpellBodGuarded : SpellBod
{
    SpellGuard *guard;
    SpellBod *body;

    SpellBodGuarded(SpellGuard *g, SpellBod *b)
    : guard(g), body(b)
    {}

    virtual void say() override;
};

struct SpellBodList : SpellBod
{
    std::vector<SpellBod *> *body;

    SpellBodList(std::vector<SpellBod *> *b)
    : body(b)
    {}

    virtual void say() override;
};

struct SpellBodEffect : SpellBod
{
    std::deque<Effect *> *body;
    std::deque<Effect *> *maybe_trigger;
    std::deque<Effect *> *maybe_end;

    SpellBodEffect(std::deque<Effect *> *b, std::deque<Effect *> *t, std::deque<Effect *> *e)
    : body(b), maybe_trigger(t), maybe_end(e)
    {}

    virtual void say() override;
};

struct SpellGuard
{
    virtual void declare() = 0;
};

struct SpellGuardOr : SpellGuard
{
    std::vector<SpellGuard *> *any;

    SpellGuardOr(std::vector<SpellGuard *> *a) : any(a) {}
    SpellGuardOr(SpellGuard *left, SpellGuard *right)
    : any(new std::vector<SpellGuard *>({left, right}))
    {}

    virtual void declare() override;
};
struct SpellGuardList : SpellGuard
{
    std::vector<SpellGuard *> *all;

    SpellGuardList(std::vector<SpellGuard *> *a) : all(a) {}

    virtual void declare() override;
};
struct SpellGuardRequire : SpellGuard
{
    Expression *expr;

    SpellGuardRequire(Expression *x) : expr(x) {}

    virtual void declare() override;
};
struct SpellGuardCatalysts : SpellGuard
{
    std::vector<Item *> *items;

    SpellGuardCatalysts(std::vector<Item *> *i) : items(i) {}

    virtual void declare() override;
};
struct SpellGuardComponents : SpellGuard
{
    std::vector<Item *> *items;

    SpellGuardComponents(std::vector<Item *> *i) : items(i) {}

    virtual void declare() override;
};
struct SpellGuardMana : SpellGuard
{
    Expression *sp;

    SpellGuardMana(Expression *x) : sp(x) {}

    virtual void declare() override;
};
struct SpellGuardCasttime : SpellGuard
{
    Expression *time;

    SpellGuardCasttime(Expression *x) : time(x) {}

    virtual void declare() override;
};

struct Item
{
    RString count;
    RString item;

    Item(RString c, RString i) : count(c), item(i) {}
};

struct Expression
{
    virtual void show() = 0;
};

struct SimpleExpr : Expression
{
    RString content;

    SimpleExpr(RString c) : content(c) {}

    virtual void show() override;
};

struct BinExpr : Expression
{
    Expression *left;
    RString op;
    Expression *right;

    BinExpr(Expression *l, RString o, Expression *r)
    : left(l), op(o), right(r)
    {}

    virtual void show() override;
};

struct CallExpr : Expression, Effect
{
    RString func;
    std::vector<Expression *> *args;

    CallExpr(RString f, std::vector<Expression *> *a)
    : func(f), args(a)
    {}

    // expression
    virtual void show() override;
    // effect
    virtual void print() override;
};

struct Location
{
    Expression *map;
    Expression *x;
    Expression *y;
};

struct AreaLoc : Expression
{
    Location *loc;

    AreaLoc(Location *l)
    : loc(l)
    {}

    virtual void show() override;
};

struct AreaRect : Expression
{
    Location *loc;
    Expression *width;
    Expression *height;

    AreaRect(Location *l, Expression *w, Expression *h)
    : loc(l), width(w), height(h)
    {}

    virtual void show() override;
};

struct AreaBar : Expression
{
    Location *loc;
    Expression *dir;
    Expression *width;
    Expression *depth;

    AreaBar(Location *l, Expression *a, Expression *w, Expression *d)
    : loc(l), dir(a), width(w), depth(d)
    {}

    virtual void show() override;
};

#ifdef override
#undef override
#endif

#endif // AST_HPP
