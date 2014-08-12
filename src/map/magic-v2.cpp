#include "magic-v2.hpp"
//    magic-v2.cpp - second generation magic parser
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

#include <cstddef>

#include <algorithm>
#include <map>
#include <set>

#include "../strings/rstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/line.hpp"

#include "../sexpr/parser.hpp"

#include "itemdb.hpp"
#include "magic-expr.hpp"
#include "magic-interpreter.hpp"
#include "magic-interpreter-base.hpp"
#include "magic-stmt.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace magic
{
namespace magic_v2
{
    static
    std::map<RString, proc_t> procs;
    static
    std::map<RString, val_t> const_defm;

    static
    size_t intern_id(ZString id_name)
    {
        // TODO use InternPool
        size_t i;
        for (i = 0; i < magic_conf.varv.size(); i++)
            if (id_name == magic_conf.varv[i].name)
                return i;

        // i = magic_conf.varv.size();
        /* Must add new */
        magic_conf_t::mcvar new_var {};
        new_var.name = id_name;
        new_var.val = ValUndef();
        magic_conf.varv.push_back(std::move(new_var));

        return i;
    }
    inline
    bool INTERN_ASSERT(ZString name, int id)
    {
        int zid = intern_id(name);
        if (zid != id)
        {
            FPRINTF(stderr,
                    "[magic-conf] INTERNAL ERROR: Builtin special var %s interned to %d, not %d as it should be!\n"_fmt,
                    name, zid, id);
        }
        return zid == id;
    }

    static
    bool init0()
    {
        bool ok = true;

        ok &= INTERN_ASSERT("min_casttime"_s, VAR_MIN_CASTTIME);
        ok &= INTERN_ASSERT("obscure_chance"_s, VAR_OBSCURE_CHANCE);
        ok &= INTERN_ASSERT("caster"_s, VAR_CASTER);
        ok &= INTERN_ASSERT("spellpower"_s, VAR_SPELLPOWER);
        ok &= INTERN_ASSERT("self_spell"_s, VAR_SPELL);
        ok &= INTERN_ASSERT("self_invocation"_s, VAR_INVOCATION);
        ok &= INTERN_ASSERT("target"_s, VAR_TARGET);
        ok &= INTERN_ASSERT("script_target"_s, VAR_SCRIPTTARGET);
        ok &= INTERN_ASSERT("location"_s, VAR_LOCATION);

        return ok;
    }


    static
    bool bind_constant(io::LineSpan span, RString name, val_t val)
    {
        if (!const_defm.insert(std::make_pair(name, std::move(val))).second)
        {
            span.error(STRPRINTF("Redefinition of constant '%s'"_fmt, name));
            return false;
        }
        return true;
    }
    static
    const val_t *find_constant(RString name)
    {
        auto it = const_defm.find(name);
        if (it != const_defm.end())
            return &it->second;

        return nullptr;
    }
    static
    dumb_ptr<effect_t> set_effect_continuation(dumb_ptr<effect_t> src, dumb_ptr<effect_t> continuation)
    {
        dumb_ptr<effect_t> retval = src;
        /* This function is completely analogous to `spellguard_implication' above; read the control flow implications above first before pondering it. */

        if (src == continuation)
            return retval;

        /* For FOR and FOREACH, we use special stack handlers and thus don't have to set
         * the continuation.  It's only IF that we need to handle in this fashion. */
        MATCH (*src)
        {
            CASE (EffectIf&, e_if)
            {
                set_effect_continuation(e_if.true_branch, continuation);
                set_effect_continuation(e_if.false_branch, continuation);
            }
        }

        if (src->next)
            set_effect_continuation(src->next, continuation);
        else
            src->next = continuation;

        return retval;
    }
    static
    dumb_ptr<spellguard_t> spellguard_implication(dumb_ptr<spellguard_t> a, dumb_ptr<spellguard_t> b)
    {
        dumb_ptr<spellguard_t> retval = a;

        if (a == b)
        {
            /* This can happen due to reference sharing:
             * e.g.,
             *  (R0 -> (R1 | R2)) => (R3)
             * yields
             *  (R0 -> (R1 -> R3 | R2 -> R3))
             *
             * So if we now add => R4 to that, we want
             *  (R0 -> (R1 -> R3 -> R4 | R2 -> R3 -> R4))
             *
             * but we only need to add it once, because the R3 reference is shared.
             */
            return retval;
        }

        /* If the premise is a disjunction, b is the continuation of _all_ branches */
        MATCH (*a)
        {
            CASE(const GuardChoice&, s)
            {
                spellguard_implication(s.s_alt, b);
            }
        }

        if (a->next)
            spellguard_implication(a->next, b);
        else
            // this is the important bit
            a->next = b;

        return retval;
    }


    static
    bool add_spell(io::LineSpan span, dumb_ptr<spell_t> spell)
    {
        auto pair1 = magic_conf.spells_by_name.insert({spell->name, spell});
        if (!pair1.second)
        {
            span.error(STRPRINTF("Attempt to redefine spell '%s'"_fmt, spell->name));
            return false;
        }

        auto pair2 = magic_conf.spells_by_invocation.insert({spell->invocation, spell});
        if (!pair2.second)
        {
            span.error(STRPRINTF("Attempt to redefine spell invocation '%s'"_fmt, spell->invocation));
            magic_conf.spells_by_name.erase(pair1.first);
            return false;
        }
        return true;
    }
    static
    bool add_teleport_anchor(io::LineSpan span, dumb_ptr<teleport_anchor_t> anchor)
    {
        auto pair1 = magic_conf.anchors_by_name.insert({anchor->name, anchor});
        if (!pair1.second)
        {
            span.error(STRPRINTF("Attempt to redefine teleport anchor '%s'"_fmt, anchor->name));
            return false;
        }

        auto pair2 = magic_conf.anchors_by_invocation.insert({anchor->name, anchor});
        if (!pair2.second)
        {
            span.error(STRPRINTF("Attempt to redefine anchor invocation '%s'"_fmt, anchor->invocation));
            magic_conf.anchors_by_name.erase(pair1.first);
            return false;
        }
        return true;
    }

    static
    bool install_proc(io::LineSpan span, dumb_ptr<proc_t> proc)
    {
        RString name = proc->name;
        if (!procs.insert({name, std::move(*proc)}).second)
        {
            span.error("procedure already exists"_s);
            return false;
        }
        return true;
    }
    static
    bool call_proc(io::LineSpan span, ZString name, dumb_ptr<std::vector<dumb_ptr<expr_t>>> argvp, dumb_ptr<effect_t>& retval)
    {
        auto pi = procs.find(name);
        if (pi == procs.end())
        {
            span.error(STRPRINTF("Unknown procedure '%s'"_fmt, name));
            return false;
        }

        proc_t *p = &pi->second;

        if (p->argv.size() != argvp->size())
        {
            span.error(STRPRINTF("Procedure %s/%zu invoked with %zu parameters"_fmt,
                        name, p->argv.size(), argvp->size()));
            return false;
        }

        EffectCall e_call;
        e_call.body = p->body;
        e_call.formalv = &p->argv;
        e_call.actualvp = argvp;
        retval = dumb_ptr<effect_t>::make(e_call, nullptr);
        return true;
    }
    static
    bool op_effect(io::LineSpan span, ZString name, Slice<dumb_ptr<expr_t>> argv, dumb_ptr<effect_t>& effect)
    {
        op_t *op = magic_get_op(name);
        if (!op)
        {
            span.error(STRPRINTF("Unknown operation '%s'"_fmt, name));
            return false;
        }
        if (op->signature.size() != argv.size())
        {
            span.error(STRPRINTF("Incorrect number of arguments to operation '%s': Expected %zu, found %zu"_fmt,
                        name, op->signature.size(), argv.size()));
            return false;
        }

        EffectOp e_op;
        e_op.line_nr = span.begin.line;
        e_op.column = span.begin.column;
        e_op.opp = op;
        assert (argv.size() <= MAX_ARGS);
        e_op.args_nr = argv.size();

        std::copy(argv.begin(), argv.end(), e_op.args);
        effect = dumb_ptr<effect_t>::make(e_op, nullptr);
        return true;
    }

    static
    dumb_ptr<expr_t> dot_expr(dumb_ptr<expr_t> expr, int id)
    {
        ExprField e_field;
        e_field.id = id;
        e_field.expr = expr;
        dumb_ptr<expr_t> retval = dumb_ptr<expr_t>::make(e_field);

        return retval;
    }
    static
    bool fun_expr(io::LineSpan span, ZString name, Slice<dumb_ptr<expr_t>> argv, dumb_ptr<expr_t>& expr)
    {
        fun_t *fun = magic_get_fun(name);
        if (!fun)
        {
            span.error(STRPRINTF("Unknown function '%s'"_fmt, name));
            return false;
        }
        if (fun->signature.size() != argv.size())
        {
            span.error(STRPRINTF("Incorrect number of arguments to function '%s': Expected %zu, found %zu"_fmt,
                        name, fun->signature.size(), argv.size()));
            return false;
        }
        ExprFunApp e_funapp;
        e_funapp.line_nr = span.begin.line;
        e_funapp.column = span.begin.column;
        e_funapp.funp = fun;

        assert (argv.size() <= MAX_ARGS);
        e_funapp.args_nr = argv.size();

        std::copy(argv.begin(), argv.end(), e_funapp.args);
        expr = dumb_ptr<expr_t>::make(e_funapp);
        return true;
    }
    static
    dumb_ptr<expr_t> BIN_EXPR(io::LineSpan span, ZString name, dumb_ptr<expr_t> left, dumb_ptr<expr_t> right)
    {
        dumb_ptr<expr_t> e[2];
        e[0] = left;
        e[1] = right;
        dumb_ptr<expr_t> rv;
        if (!fun_expr(span, name, e, rv))
            abort();
        return rv;
    }

    static
    bool fail(const sexpr::SExpr& s, ZString msg)
    {
        s._span.error(msg);
        return false;
    }
}

namespace magic_v2
{
    using sexpr::SExpr;

    static
    bool parse_expression(const SExpr& x, dumb_ptr<expr_t>& out);
    static
    bool parse_effect(const SExpr& s, dumb_ptr<effect_t>& out);
    static
    bool parse_spellguard(const SExpr& s, dumb_ptr<spellguard_t>& out);
    static
    bool parse_spellbody(const SExpr& s, dumb_ptr<spellguard_t>& out);

    // Note: anything with dumb_ptr leaks memory on failure
    // once this is all done, we can convert it to unique_ptr
    // (may require bimaps somewhere)
    static
    bool is_comment(const SExpr& s)
    {
        if (s._type == sexpr::STRING)
            return true;
        if (s._type != sexpr::LIST)
            return false;
        if (s._list.empty())
            return false;
        if (s._list[0]._type != sexpr::TOKEN)
            return false;
        return s._list[0]._str == "DISABLED"_s;
    }

    static
    bool parse_loc(const SExpr& s, e_location_t& loc)
    {
        if (s._type != sexpr::LIST)
            return fail(s, "loc not list"_s);
        if (s._list.size() != 4)
            return fail(s, "loc not 3 args"_s);
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "loc cmd not tok"_s);
        if (s._list[0]._str != "@"_s)
            return fail(s._list[0], "loc cmd not cmd"_s);
        return parse_expression(s._list[1], loc.m)
            && parse_expression(s._list[2], loc.x)
            && parse_expression(s._list[3], loc.y);
    }

    static
    bool parse_expression(const SExpr& x, dumb_ptr<expr_t>& out)
    {
        switch (x._type)
        {
        case sexpr::INT:
            {
                val_t val;
                val = ValInt{static_cast<int32_t>(x._int)};
                if (val.get_if<ValInt>()->v_int != x._int)
                    return fail(x, "integer too large"_s);

                out = dumb_ptr<expr_t>::make(std::move(val));
                return true;
            }
        case sexpr::STRING:
            {
                val_t val;
                val = ValString{x._str};

                out = dumb_ptr<expr_t>::make(std::move(val));
                return true;
            }
        case sexpr::TOKEN:
            {
                earray<LString, DIR, DIR::COUNT> dirs //=
                {{
                    "S"_s, "SW"_s, "W"_s, "NW"_s,
                    "N"_s, "NE"_s, "E"_s, "SE"_s,
                }};
                auto begin = std::begin(dirs);
                auto end = std::end(dirs);
                auto it = std::find(begin, end, x._str);
                if (it != end)
                {
                    val_t val;
                    val = ValDir{static_cast<DIR>(it - begin)};

                    out = dumb_ptr<expr_t>::make(std::move(val));
                    return true;
                }
            }
            {
                if (const val_t *val = find_constant(x._str))
                {
                    val_t copy;
                    magic_copy_var(&copy, val);
                    out = dumb_ptr<expr_t>::make(std::move(copy));
                    return true;
                }
                else
                {
                    ExprId e;
                    e.e_id = intern_id(x._str);
                    out = dumb_ptr<expr_t>::make(e);
                    return true;
                }
            }
            break;
        case sexpr::LIST:
            if (x._list.empty())
                return fail(x, "empty list"_s);
            {
                if (x._list[0]._type != sexpr::TOKEN)
                    return fail(x._list[0], "op not token"_s);
                ZString op = x._list[0]._str;
                // area
                if (op == "@"_s)
                {
                    e_location_t loc;
                    if (!parse_loc(x, loc))
                        return false;
                    out = dumb_ptr<expr_t>::make(loc);
                    return true;
                }
                if (op == "@+"_s)
                {
                    e_location_t loc;
                    dumb_ptr<expr_t> width;
                    dumb_ptr<expr_t> height;
                    if (!parse_loc(x._list[1], loc))
                        return false;
                    if (!parse_expression(x._list[2], width))
                        return false;
                    if (!parse_expression(x._list[3], height))
                        return false;
                    ExprAreaRect a_rect;
                    a_rect.loc = loc;
                    a_rect.width = width;
                    a_rect.height = height;
                    out = dumb_ptr<expr_t>::make(a_rect);
                    return true;
                }
                if (op == "TOWARDS"_s)
                {
                    e_location_t loc;
                    dumb_ptr<expr_t> dir;
                    dumb_ptr<expr_t> width;
                    dumb_ptr<expr_t> depth;
                    if (!parse_loc(x._list[1], loc))
                        return false;
                    if (!parse_expression(x._list[2], dir))
                        return false;
                    if (!parse_expression(x._list[3], width))
                        return false;
                    if (!parse_expression(x._list[4], depth))
                        return false;
                    ExprAreaBar a_bar;
                    a_bar.loc = loc;
                    a_bar.dir = dir;
                    a_bar.width = width;
                    a_bar.depth = depth;
                    out = dumb_ptr<expr_t>::make(a_bar);
                    return true;
                }
                if (op == "."_s)
                {
                    if (x._list.size() != 3)
                        return fail(x, ". not 2"_s);
                    dumb_ptr<expr_t> expr;
                    if (!parse_expression(x._list[1], expr))
                        return false;
                    if (x._list[2]._type != sexpr::TOKEN)
                        return fail(x._list[2], ".elem not name"_s);
                    ZString elem = x._list[2]._str;
                    out = dot_expr(expr, intern_id(elem));
                    return true;
                }
                static // TODO LString
                std::set<ZString> ops =
                {
                    "<"_s, ">"_s, "<="_s, ">="_s, "=="_s, "!="_s,
                    "+"_s, "-"_s, "*"_s, "%"_s, "/"_s,
                    "&"_s, "^"_s, "|"_s, "<<"_s, ">>"_s,
                    "&&"_s, "||"_s,
                };
                // TODO implement unary operators
                if (ops.count(op))
                {
                    // operators are n-ary and left-associative
                    if (x._list.size() < 3)
                        return fail(x, "operator not at least 2 args"_s);
                    auto begin = x._list.begin() + 1;
                    auto end = x._list.end();
                    if (!parse_expression(*begin, out))
                        return false;
                    ++begin;
                    for (; begin != end; ++begin)
                    {
                        dumb_ptr<expr_t> tmp;
                        if (!parse_expression(*begin, tmp))
                            return false;
                        out = BIN_EXPR(x._span, op, out, tmp);
                    }
                    return true;
                }
                std::vector<dumb_ptr<expr_t>> argv;
                for (auto it = x._list.begin() + 1, end = x._list.end(); it != end; ++it)
                {
                    dumb_ptr<expr_t> expr;
                    if (!parse_expression(*it, expr))
                        return false;
                    argv.push_back(expr);
                }
                return fun_expr(x._span, op, argv, out);
            }
            break;
        }
        abort();
    }

    static
    bool parse_item(const SExpr& s, ItemNameId& id, int& count)
    {
        if (s._type == sexpr::STRING)
        {
            count = 1;

            item_data *item = itemdb_searchname(s._str);
            if (!item)
                return fail(s, "no such item"_s);
            id = item->nameid;
            return true;
        }
        if (s._type != sexpr::LIST)
            return fail(s, "item not string or list"_s);
        if (s._list.size() != 2)
            return fail(s, "item list is not pair"_s);
        if (s._list[0]._type != sexpr::INT)
            return fail(s._list[0], "item pair first not int"_s);
        count = s._list[0]._int;
        if (s._list[1]._type != sexpr::STRING)
            return fail(s._list[1], "item pair second not name"_s);

        item_data *item = itemdb_searchname(s._list[1]._str);
        if (!item)
            return fail(s, "no such item"_s);
        id = item->nameid;
        return true;
    }

    static
    bool parse_spellguard(const SExpr& s, dumb_ptr<spellguard_t>& out)
    {
        if (s._type != sexpr::LIST)
            return fail(s, "not list"_s);
        if (s._list.empty())
            return fail(s, "empty list"_s);
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "not token"_s);
        ZString cmd = s._list[0]._str;
        if (cmd == "OR"_s)
        {
            auto begin = s._list.begin() + 1;
            auto end = s._list.end();
            if (begin == end)
                return fail(s, "missing arguments"_s);
            if (!parse_spellguard(*begin, out))
                return false;
            ++begin;
            for (; begin != end; ++begin)
            {
                dumb_ptr<spellguard_t> alt;
                if (!parse_spellguard(*begin, alt))
                    return false;
                GuardChoice choice;
                auto next = out;
                choice.s_alt = alt;
                out = dumb_ptr<spellguard_t>::make(choice, next);
            }
            return true;
        }
        if (cmd == "GUARD"_s)
        {
            auto begin = s._list.begin() + 1;
            auto end = s._list.end();
            while (is_comment(end[-1]))
                --end;
            if (begin == end)
                return fail(s, "missing arguments"_s);
            if (!parse_spellguard(end[-1], out))
                return false;
            --end;
            for (; begin != end; --end)
            {
                if (is_comment(end[-1]))
                    continue;
                dumb_ptr<spellguard_t> implier;
                if (!parse_spellguard(end[-1], implier))
                    return false;
                out = spellguard_implication(implier, out);
            }
            return true;
        }
        if (cmd == "REQUIRE"_s)
        {
            if (s._list.size() != 2)
                return fail(s, "not one argument"_s);
            dumb_ptr<expr_t> condition;
            if (!parse_expression(s._list[1], condition))
                return false;
            GuardCondition cond;
            cond.s_condition = condition;
            out = dumb_ptr<spellguard_t>::make(cond, nullptr);
            return true;
        }
        if (cmd == "MANA"_s)
        {
            if (s._list.size() != 2)
                return fail(s, "not one argument"_s);
            dumb_ptr<expr_t> mana;
            if (!parse_expression(s._list[1], mana))
                return false;
            GuardMana sp;
            sp.s_mana = mana;
            out = dumb_ptr<spellguard_t>::make(sp, nullptr);
            return true;
        }
        if (cmd == "CASTTIME"_s)
        {
            if (s._list.size() != 2)
                return fail(s, "not one argument"_s);
            dumb_ptr<expr_t> casttime;
            if (!parse_expression(s._list[1], casttime))
                return false;
            GuardCastTime ct;
            ct.s_casttime = casttime;
            out = dumb_ptr<spellguard_t>::make(ct, nullptr);
            return true;
        }
        if (cmd == "CATALYSTS"_s)
        {
            dumb_ptr<component_t> items = nullptr;
            for (auto it = s._list.begin() + 1, end = s._list.end(); it != end; ++it)
            {
                ItemNameId id;
                int count;
                if (!parse_item(*it, id, count))
                    return false;
                magic_add_component(&items, id, count);
            }
            GuardCatalysts cat;
            cat.s_catalysts = items;
            out = dumb_ptr<spellguard_t>::make(cat, nullptr);
            return true;
        }
        if (cmd == "COMPONENTS"_s)
        {
            dumb_ptr<component_t> items = nullptr;
            for (auto it = s._list.begin() + 1, end = s._list.end(); it != end; ++it)
            {
                ItemNameId id;
                int count;
                if (!parse_item(*it, id, count))
                    return false;
                magic_add_component(&items, id, count);
            }
            GuardComponents comp;
            comp.s_components = items;
            out = dumb_ptr<spellguard_t>::make(comp, nullptr);
            return true;
        }
        return fail(s._list[0], "unknown guard"_s);
    }

    static
    bool build_effect_list(std::vector<SExpr>::const_iterator begin,
            std::vector<SExpr>::const_iterator end, dumb_ptr<effect_t>& out)
    {
        // these backward lists could be forward by keeping the reference
        // I know this is true because Linus said so
        out = dumb_ptr<effect_t>::make(EffectSkip{}, nullptr);
        while (end != begin)
        {
            const SExpr& s = *--end;
            if (is_comment(s))
                continue;
            dumb_ptr<effect_t> chain;
            if (!parse_effect(s, chain))
                return false;
            out = set_effect_continuation(chain, out);
        }
        return true;
    }

    static
    bool parse_effect(const SExpr& s, dumb_ptr<effect_t>& out)
    {
        if (s._type != sexpr::LIST)
            return fail(s, "not list"_s);
        if (s._list.empty())
            return fail(s, "empty list"_s);
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "not token"_s);
        ZString cmd = s._list[0]._str;
        if (cmd == "BLOCK"_s)
        {
            return build_effect_list(s._list.begin() + 1, s._list.end(), out);
        }
        if (cmd == "SET"_s)
        {
            if (s._list.size() != 3)
                return fail(s, "not 2 args"_s);
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "not token"_s);
            ZString name = s._list[1]._str;
            if (find_constant(name))
                return fail(s._list[1], "assigning to constant"_s);
            dumb_ptr<expr_t> expr;
            if (!parse_expression(s._list[2], expr))
                return false;

            EffectAssign e_assign;
            e_assign.id = intern_id(name);
            e_assign.expr = expr;
            out = dumb_ptr<effect_t>::make(e_assign, nullptr);
            return true;
        }
        if (cmd == "SCRIPT"_s)
        {
            if (s._list.size() != 2)
                return fail(s, "not 1 arg"_s);
            if (s._list[1]._type != sexpr::STRING)
                return fail(s._list[1], "not string"_s);
            ZString body = s._list[1]._str;
            std::unique_ptr<const ScriptBuffer> script = parse_script(body, s._list[1]._span.begin.line, true);
            if (!script)
                return fail(s._list[1], "script does not compile"_s);
            EffectScript e;
            e.e_script = dumb_ptr<const ScriptBuffer>(script.release());
            out = dumb_ptr<effect_t>::make(e, nullptr);
            return true;
        }
        if (cmd == "SKIP"_s)
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg"_s);
            out = dumb_ptr<effect_t>::make(EffectSkip{}, nullptr);
            return true;
        }
        if (cmd == "ABORT"_s)
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg"_s);
            out = dumb_ptr<effect_t>::make(EffectAbort{}, nullptr);
            return true;
        }
        if (cmd == "END"_s)
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg"_s);
            out = dumb_ptr<effect_t>::make(EffectEnd{}, nullptr);
            return true;
        }
        if (cmd == "BREAK"_s)
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg"_s);
            out = dumb_ptr<effect_t>::make(EffectBreak{}, nullptr);
            return true;
        }
        if (cmd == "FOREACH"_s)
        {
            if (s._list.size() != 5)
                return fail(s, "not 4 arg"_s);
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "foreach type not token"_s);
            ZString type = s._list[1]._str;
            FOREACH_FILTER filter;
            if (type == "PC"_s)
                filter = FOREACH_FILTER::PC;
            else if (type == "MOB"_s)
                filter = FOREACH_FILTER::MOB;
            else if (type == "ENTITY"_s)
                filter = FOREACH_FILTER::ENTITY;
            else if (type == "SPELL"_s)
                filter = FOREACH_FILTER::SPELL;
            else if (type == "TARGET"_s)
                filter = FOREACH_FILTER::TARGET;
            else if (type == "NPC"_s)
                filter = FOREACH_FILTER::NPC;
            else
                return fail(s._list[1], "unknown foreach filter"_s);
            if (s._list[2]._type != sexpr::TOKEN)
                return fail(s._list[2], "foreach var not token"_s);
            ZString var = s._list[2]._str;
            dumb_ptr<expr_t> area;
            dumb_ptr<effect_t> effect;
            if (!parse_expression(s._list[3], area))
                return false;
            if (!parse_effect(s._list[4], effect))
                return false;

            EffectForEach e_foreach;
            e_foreach.id = intern_id(var);
            e_foreach.area = area;
            e_foreach.body = effect;
            e_foreach.filter = filter;
            out = dumb_ptr<effect_t>::make(e_foreach, nullptr);
            return true;
        }
        if (cmd == "FOR"_s)
        {
            if (s._list.size() != 5)
                return fail(s, "not 4 arg"_s);
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "for var not token"_s);
            ZString var = s._list[1]._str;
            dumb_ptr<expr_t> low;
            dumb_ptr<expr_t> high;
            dumb_ptr<effect_t> effect;
            if (!parse_expression(s._list[2], low))
                return false;
            if (!parse_expression(s._list[3], high))
                return false;
            if (!parse_effect(s._list[4], effect))
                return false;

            EffectFor e_for;
            e_for.id = intern_id(var);
            e_for.start = low;
            e_for.stop = high;
            e_for.body = effect;
            out = dumb_ptr<effect_t>::make(e_for, nullptr);
            return true;
        }
        if (cmd == "IF"_s)
        {
            if (s._list.size() != 3 && s._list.size() != 4)
                return fail(s, "not 2 or 3 args"_s);
            dumb_ptr<expr_t> cond;
            dumb_ptr<effect_t> if_true;
            dumb_ptr<effect_t> if_false;
            if (!parse_expression(s._list[1], cond))
                return false;
            if (!parse_effect(s._list[2], if_true))
                return false;
            if (s._list.size() == 4)
            {
                if (!parse_effect(s._list[3], if_false))
                    return false;
            }
            else
                if_false = dumb_ptr<effect_t>::make(EffectSkip{}, nullptr);

            EffectIf e_if;
            e_if.cond = cond;
            e_if.true_branch = if_true;
            e_if.false_branch = if_false;
            out = dumb_ptr<effect_t>::make(e_if, nullptr);
            return true;
        }
        if (cmd == "WAIT"_s)
        {
            if (s._list.size() != 2)
                return fail(s, "not 1 arg"_s);
            dumb_ptr<expr_t> expr;
            if (!parse_expression(s._list[1], expr))
                return false;
            EffectSleep e;
            e.e_sleep = expr;
            out = dumb_ptr<effect_t>::make(e, nullptr);
            return true;
        }
        if (cmd == "CALL"_s)
        {
            if (s._list.size() < 2)
                return fail(s, "call what?"_s);
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "call token please"_s);
            ZString func = s._list[1]._str;
            auto argvp = dumb_ptr<std::vector<dumb_ptr<expr_t>>>::make();
            for (auto it = s._list.begin() + 2, end = s._list.end(); it != end; ++it)
            {
                dumb_ptr<expr_t> expr;
                if (!parse_expression(*it, expr))
                    return false;
                argvp->push_back(expr);
            }
            return call_proc(s._span, func, argvp, out);
        }
        auto argv = std::vector<dumb_ptr<expr_t>>();
        for (auto it = s._list.begin() + 1, end = s._list.end(); it != end; ++it)
        {
            dumb_ptr<expr_t> expr;
            if (!parse_expression(*it, expr))
                return false;
            argv.push_back(expr);
        }
        return op_effect(s._span, cmd, argv, out);
    }

    static
    bool parse_spellbody(const SExpr& s, dumb_ptr<spellguard_t>& out)
    {
        if (s._type != sexpr::LIST)
            return fail(s, "not list"_s);
        if (s._list.empty())
            return fail(s, "empty list"_s);
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "not token"_s);
        ZString cmd = s._list[0]._str;
        if (cmd == "=>"_s)
        {
            if (s._list.size() != 3)
                return fail(s, "list does not have exactly 2 arguments"_s);
            dumb_ptr<spellguard_t> guard;
            if (!parse_spellguard(s._list[1], guard))
                return false;
            dumb_ptr<spellguard_t> body;
            if (!parse_spellbody(s._list[2], body))
                return false;
            out = spellguard_implication(guard, body);
            return true;
        }
        if (cmd == "|"_s)
        {
            if (s._list.size() == 1)
                return fail(s, "spellbody choice empty"_s);
            auto begin = s._list.begin() + 1;
            auto end = s._list.end();
            if (!parse_spellbody(*begin, out))
                return false;
            ++begin;
            for (; begin != end; ++begin)
            {
                dumb_ptr<spellguard_t> alt;
                if (!parse_spellbody(*begin, alt))
                    return false;
                auto tmp = out;
                GuardChoice choice;
                choice.s_alt = alt;
                out = dumb_ptr<spellguard_t>::make(choice, tmp);
            }
            return true;
        }
        if (cmd == "EFFECT"_s)
        {
            auto begin = s._list.begin() + 1;
            auto end = s._list.end();

            dumb_ptr<effect_t> effect, attrig, atend;

            // decreasing end can never pass begin, since we know that
            // begin[-1] is token EFFECT
            while (is_comment(end[-1]))
                --end;
            if (end[-1]._type == sexpr::LIST && !end[-1]._list.empty()
                    && end[-1]._list[0]._type == sexpr::TOKEN
                    && end[-1]._list[0]._str == "ATEND"_s)
            {
                auto atb = end[-1]._list.begin() + 1;
                auto ate = end[-1]._list.end();
                if (!build_effect_list(atb, ate, atend))
                    return false;
                --end;

                while (is_comment(end[-1]))
                    --end;
            }
            else
            {
                atend = nullptr;
            }
            if (end[-1]._type == sexpr::LIST && !end[-1]._list.empty()
                    && end[-1]._list[0]._type == sexpr::TOKEN
                    && end[-1]._list[0]._str == "ATTRIGGER"_s)
            {
                auto atb = end[-1]._list.begin() + 1;
                auto ate = end[-1]._list.end();
                if (!build_effect_list(atb, ate, attrig))
                    return false;
                --end;
            }
            else
            {
                attrig = nullptr;
            }
            if (!build_effect_list(begin, end, effect))
                return false;
            effect_set_t s_effect;
            s_effect.effect = effect;
            s_effect.at_trigger = attrig;
            s_effect.at_end = atend;
            out = dumb_ptr<spellguard_t>::make(s_effect, nullptr);
            return true;
        }
        return fail(s._list[0], "unknown spellbody"_s);
    }

    static
    bool parse_top_set(const std::vector<SExpr>& in)
    {
        if (in.size() != 3)
            return fail(in[0], "not 2 arguments"_s);
        ZString name = in[1]._str;
        dumb_ptr<expr_t> expr;
        if (!parse_expression(in[2], expr))
            return false;
        if (find_constant(name))
            return fail(in[1], "assign constant"_s);
        size_t var_id = intern_id(name);
        magic_eval(dumb_ptr<env_t>(&magic_default_env), &magic_conf.varv[var_id].val, expr);
        return true;
    }
    static
    bool parse_const(io::LineSpan span, const std::vector<SExpr>& in)
    {
        if (in.size() != 3)
            return fail(in[0], "not 2 arguments"_s);
        if (in[1]._type != sexpr::TOKEN)
            return fail(in[1], "not token"_s);
        ZString name = in[1]._str;
        dumb_ptr<expr_t> expr;
        if (!parse_expression(in[2], expr))
            return false;
        val_t tmp;
        magic_eval(dumb_ptr<env_t>(&magic_default_env), &tmp, expr);
        return bind_constant(span, name, std::move(tmp));
    }
    static
    bool parse_anchor(io::LineSpan span, const std::vector<SExpr>& in)
    {
        if (in.size() != 4)
            return fail(in[0], "not 3 arguments"_s);
        auto anchor = dumb_ptr<teleport_anchor_t>::make();
        if (in[1]._type != sexpr::TOKEN)
            return fail(in[1], "not token"_s);
        anchor->name = in[1]._str;
        if (in[2]._type != sexpr::STRING)
            return fail(in[2], "not string"_s);
        anchor->invocation = in[2]._str;
        dumb_ptr<expr_t> expr;
        if (!parse_expression(in[3], expr))
            return false;
        anchor->location = expr;
        return add_teleport_anchor(span, anchor);
    }
    static
    bool parse_proc(io::LineSpan span, const std::vector<SExpr>& in)
    {
        if (in.size() < 4)
            return fail(in[0], "not at least 3 arguments"_s);
        auto proc = dumb_ptr<proc_t>::make();
        if (in[1]._type != sexpr::TOKEN)
            return fail(in[1], "name not token"_s);
        proc->name = in[1]._str;
        if (in[2]._type != sexpr::LIST)
            return fail(in[2], "args not list"_s);
        for (const SExpr& arg : in[2]._list)
        {
            if (arg._type != sexpr::TOKEN)
                return fail(arg, "arg not token"_s);
            proc->argv.push_back(intern_id(arg._str));
        }
        if (!build_effect_list(in.begin() + 3, in.end(), proc->body))
            return false;
        return install_proc(span, proc);
    }
    static
    bool parse_spell(io::LineSpan span, const std::vector<SExpr>& in)
    {
        if (in.size() < 6)
            return fail(in[0], "not at least 5 arguments"_s);
        if (in[1]._type != sexpr::LIST)
            return fail(in[1], "flags not list"_s);

        auto spell = dumb_ptr<spell_t>::make();

        for (const SExpr& s : in[1]._list)
        {
            if (s._type != sexpr::TOKEN)
                return fail(s, "flag not token"_s);
            SPELL_FLAG flag = SPELL_FLAG::ZERO;
            if (s._str == "LOCAL"_s)
                flag = SPELL_FLAG::LOCAL;
            else if (s._str == "NONMAGIC"_s)
                flag = SPELL_FLAG::NONMAGIC;
            else if (s._str == "SILENT"_s)
                flag = SPELL_FLAG::SILENT;
            else
                return fail(s, "unknown flag"_s);
            if (bool(spell->flags & flag))
                return fail(s, "duplicate flag"_s);
            spell->flags |= flag;
        }
        if (in[2]._type != sexpr::TOKEN)
            return fail(in[2], "name not token"_s);
        spell->name = in[2]._str;
        if (in[3]._type != sexpr::STRING)
            return fail(in[3], "invoc not string"_s);
        spell->invocation = in[3]._str;
        if (in[4]._type != sexpr::LIST)
            return fail(in[4], "spellarg not list"_s);
        if (in[4]._list.size() == 0)
        {
            spell->spellarg_ty = SPELLARG::NONE;
        }
        else
        {
            if (in[4]._list.size() != 2)
                return fail(in[4], "spellarg not empty list or pair"_s);
            if (in[4]._list[0]._type != sexpr::TOKEN)
                return fail(in[4]._list[0], "spellarg type not token"_s);
            if (in[4]._list[1]._type != sexpr::TOKEN)
                return fail(in[4]._list[1], "spellarg name not token"_s);
            ZString ty = in[4]._list[0]._str;
            if (ty == "PC"_s)
                spell->spellarg_ty = SPELLARG::PC;
            else if (ty == "STRING"_s)
                spell->spellarg_ty = SPELLARG::STRING;
            else
                return fail(in[4]._list[0], "unknown spellarg type"_s);
            ZString an = in[4]._list[1]._str;
            spell->arg = intern_id(an);
        }
        std::vector<SExpr>::const_iterator it = in.begin() + 5;
        for (;; ++it)
        {
            if (it == in.end())
                return fail(it[-1], "end of list scanning LET defs"_s);
            if (is_comment(*it))
                continue;
            if (it->_type != sexpr::LIST || it->_list.empty())
                break;
            if (it->_list[0]._type != sexpr::TOKEN || it->_list[0]._str != "LET"_s)
                break;

            if (it->_list[1]._type != sexpr::TOKEN)
                return fail(it->_list[1], "let name not token"_s);
            ZString name = it->_list[1]._str;
            if (find_constant(name))
                return fail(it->_list[1], "constant exists"_s);
            dumb_ptr<expr_t> expr;
            if (!parse_expression(it->_list[2], expr))
                return false;
            letdef_t let;
            let.id = intern_id(name);
            let.expr = expr;
            spell->letdefv.push_back(let);
        }
        if (it + 1 != in.end())
            return fail(*it, "expected only one body entry besides LET"_s);

        // formally, 'guard' only refers to the first argument of '=>'
        // but internally, spellbodies use the same thing
        dumb_ptr<spellguard_t> guard;
        if (!parse_spellbody(*it, guard))
            return false;
        spell->spellguard = guard;
        return add_spell(span, spell);
    }

    static
    bool parse_top(io::LineSpan span, const std::vector<SExpr>& vs)
    {
        if (vs.empty())
        {
            span.error("Empty list at top"_s);
            return false;
        }
        if (vs[0]._type != sexpr::TOKEN)
            return fail(vs[0], "top not token"_s);
        ZString cmd = vs[0]._str;
        if (cmd == "CONST"_s)
            return parse_const(span, vs);
        if (cmd == "PROCEDURE"_s)
            return parse_proc(span, vs);
        if (cmd == "SET"_s)
            return parse_top_set(vs);
        if (cmd == "SPELL"_s)
            return parse_spell(span, vs);
        if (cmd == "TELEPORT-ANCHOR"_s)
            return parse_anchor(span, vs);
        return fail(vs[0], "Unknown top-level command"_s);
    }

    static
    bool loop(sexpr::Lexer& in)
    {
        SExpr s;
        while (sexpr::parse(in, s))
        {
            if (is_comment(s))
                continue;
            if (s._type != sexpr::LIST)
                return fail(s, "top-level entity not a list or comment"_s);
            if (!parse_top(s._span, s._list))
                return false;
        }
        // handle low-level errors
        if (in.peek() != sexpr::TOK_EOF)
        {
            in.span().error("parser gave up before end of file"_s);
            return false;
        }
        return true;
    }
} // namespace magic_v2

bool magic_init0()
{
    return magic_v2::init0();
}

bool load_magic_file_v2(ZString filename)
{
    sexpr::Lexer in(filename);
    bool rv = magic_v2::loop(in);
    if (!rv)
    {
        in.span().error(STRPRINTF("next token: %s '%s'"_fmt, sexpr::token_name(in.peek()), in.val_string()));
    }
    return rv;
}
} // namespace magic
} // namespace tmwa
