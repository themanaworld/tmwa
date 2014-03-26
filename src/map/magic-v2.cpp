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

#include <set>

#include "../sexpr/parser.hpp"

#include "../mmo/dumb_ptr.hpp"

#include "itemdb.hpp"
#include "magic-expr.hpp"

#include "../poison.hpp"

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
        new_var.val.ty = TYPE::UNDEF;
        magic_conf.varv.push_back(new_var);

        return i;
    }
    inline
    bool INTERN_ASSERT(ZString name, int id)
    {
        int zid = intern_id(name);
        if (zid != id)
        {
            FPRINTF(stderr,
                "[magic-conf] INTERNAL ERROR: Builtin special var %s interned to %d, not %d as it should be!\n",
                name, zid, id);
        }
        return zid == id;
    }

    static
    bool init0()
    {
        bool ok = true;

        ok &= INTERN_ASSERT("min_casttime", VAR_MIN_CASTTIME);
        ok &= INTERN_ASSERT("obscure_chance", VAR_OBSCURE_CHANCE);
        ok &= INTERN_ASSERT("caster", VAR_CASTER);
        ok &= INTERN_ASSERT("spellpower", VAR_SPELLPOWER);
        ok &= INTERN_ASSERT("self_spell", VAR_SPELL);
        ok &= INTERN_ASSERT("self_invocation", VAR_INVOCATION);
        ok &= INTERN_ASSERT("target", VAR_TARGET);
        ok &= INTERN_ASSERT("script_target", VAR_SCRIPTTARGET);
        ok &= INTERN_ASSERT("location", VAR_LOCATION);

        return ok;
    }


    static
    bool bind_constant(io::LineSpan span, RString name, val_t *val)
    {
        if (!const_defm.insert({name, *val}).second)
        {
            span.error(STRPRINTF("Redefinition of constant '%s'", name));
            return false;
        }
        return true;
    }
    static
    val_t *find_constant(RString name)
    {
        auto it = const_defm.find(name);
        if (it != const_defm.end())
            return &it->second;

        return NULL;
    }
    static
    dumb_ptr<effect_t> new_effect(EFFECT ty)
    {
        auto effect = dumb_ptr<effect_t>::make();
        effect->ty = ty;
        return effect;
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
        if (src->ty == EFFECT::IF)
        {
            set_effect_continuation(src->e.e_if.true_branch, continuation);
            set_effect_continuation(src->e.e_if.false_branch, continuation);
        }

        if (src->next)
            set_effect_continuation(src->next, continuation);
        else
            src->next = continuation;

        return retval;
    }
    static
    dumb_ptr<spellguard_t> new_spellguard(SPELLGUARD ty)
    {
        dumb_ptr<spellguard_t> retval = dumb_ptr<spellguard_t>::make();
        retval->ty = ty;
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
        if (a->ty == SPELLGUARD::CHOICE)
            spellguard_implication(a->s.s_alt, b);

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
            span.error(STRPRINTF("Attempt to redefine spell '%s'", spell->name));
            return false;
        }

        auto pair2 = magic_conf.spells_by_invocation.insert({spell->invocation, spell});
        if (!pair2.second)
        {
            span.error(STRPRINTF("Attempt to redefine spell invocation '%s'", spell->invocation));
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
            span.error(STRPRINTF("Attempt to redefine teleport anchor '%s'", anchor->name));
            return false;
        }

        auto pair2 = magic_conf.anchors_by_invocation.insert({anchor->name, anchor});
        if (!pair2.second)
        {
            span.error(STRPRINTF("Attempt to redefine anchor invocation '%s'", anchor->invocation));
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
            span.error("procedure already exists");
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
            span.error(STRPRINTF("Unknown procedure '%s'", name));
            return false;
        }

        proc_t *p = &pi->second;

        if (p->argv.size() != argvp->size())
        {
            span.error(STRPRINTF("Procedure %s/%zu invoked with %zu parameters",
                        name, p->argv.size(), argvp->size()));
            return false;
        }

        retval = new_effect(EFFECT::CALL);
        retval->e.e_call.body = p->body;
        retval->e.e_call.formalv = &p->argv;
        retval->e.e_call.actualvp = argvp;
        return true;
    }
    static
    bool op_effect(io::LineSpan span, ZString name, const_array<dumb_ptr<expr_t>> argv, dumb_ptr<effect_t>& effect)
    {
        op_t *op = magic_get_op(name);
        if (!op)
        {
            span.error(STRPRINTF("Unknown operation '%s'", name));
            return false;
        }
        if (op->signature.size() != argv.size())
        {
            span.error(STRPRINTF("Incorrect number of arguments to operation '%s': Expected %zu, found %zu",
                    name, op->signature.size(), argv.size()));
            return false;
        }

        effect = new_effect(EFFECT::OP);
        effect->e.e_op.line_nr = span.begin.line;
        effect->e.e_op.column = span.begin.column;
        effect->e.e_op.opp = op;
        assert (argv.size() <= MAX_ARGS);
        effect->e.e_op.args_nr = argv.size();

        std::copy(argv.begin(), argv.end(), effect->e.e_op.args);
        return true;
    }

    static
    dumb_ptr<expr_t> dot_expr(dumb_ptr<expr_t> expr, int id)
    {
        dumb_ptr<expr_t> retval = magic_new_expr(EXPR::SPELLFIELD);
        retval->e.e_field.id = id;
        retval->e.e_field.expr = expr;

        return retval;
    }
    static
    bool fun_expr(io::LineSpan span, ZString name, const_array<dumb_ptr<expr_t>> argv, dumb_ptr<expr_t>& expr)
    {
        fun_t *fun = magic_get_fun(name);
        if (!fun)
        {
            span.error(STRPRINTF("Unknown function '%s'", name));
            return false;
        }
        if (fun->signature.size() != argv.size())
        {
            span.error(STRPRINTF("Incorrect number of arguments to function '%s': Expected %zu, found %zu",
                    name, fun->signature.size(), argv.size()));
            return false;
        }
        expr = magic_new_expr(EXPR::FUNAPP);
        expr->e.e_funapp.line_nr = span.begin.line;
        expr->e.e_funapp.column = span.begin.column;
        expr->e.e_funapp.funp = fun;

        assert (argv.size() <= MAX_ARGS);
        expr->e.e_funapp.args_nr = argv.size();

        std::copy(argv.begin(), argv.end(), expr->e.e_funapp.args);
        return true;
    }
    static
    dumb_ptr<expr_t> BIN_EXPR(io::LineSpan span, ZString name, dumb_ptr<expr_t> left, dumb_ptr<expr_t> right)
    {
        dumb_ptr<expr_t> e[2];
        e[0] = left;
        e[1] = right;
        dumb_ptr<expr_t> rv;
        if (!fun_expr(span, name, const_array<dumb_ptr<expr_t>>(e, 2), rv))
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
        return s._list[0]._str == "DISABLED";
    }

    static
    bool parse_loc(const SExpr& s, e_location_t& loc)
    {
        if (s._type != sexpr::LIST)
            return fail(s, "loc not list");
        if (s._list.size() != 4)
            return fail(s, "loc not 3 args");
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "loc cmd not tok");
        if (s._list[0]._str != "@")
            return fail(s._list[0], "loc cmd not cmd");
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
                val.ty = TYPE::INT;
                val.v.v_int = x._int;
                if (val.v.v_int != x._int)
                    return fail(x, "integer too large");

                out = magic_new_expr(EXPR::VAL);
                out->e.e_val = val;
                return true;
            }
        case sexpr::STRING:
            {
                val_t val;
                val.ty = TYPE::STRING;
                val.v.v_string = dumb_string::copys(x._str);

                out = magic_new_expr(EXPR::VAL);
                out->e.e_val = val;
                return true;
            }
        case sexpr::TOKEN:
            {
                ZString dirs[8] = {
                    ZString("S"), ZString("SW"), ZString("W"), ZString("NW"), ZString("N"), ZString("NE"), ZString("E"), ZString("SE"),
                };
                auto begin = std::begin(dirs);
                auto end = std::end(dirs);
                auto it = std::find(begin, end, x._str);
                if (it != end)
                {
                    val_t val;
                    val.ty = TYPE::DIR;
                    val.v.v_dir = static_cast<DIR>(it - begin);

                    out = magic_new_expr(EXPR::VAL);
                    out->e.e_val = val;
                    return true;
                }
            }
            {
                if (val_t *val = find_constant(x._str))
                {
                    out = magic_new_expr(EXPR::VAL);
                    out->e.e_val = *val;
                    return true;
                }
                else
                {
                    out = magic_new_expr(EXPR::ID);
                    out->e.e_id = intern_id(x._str);
                    return true;
                }
            }
            break;
        case sexpr::LIST:
            if (x._list.empty())
                return fail(x, "empty list");
            {
                if (x._list[0]._type != sexpr::TOKEN)
                    return fail(x._list[0], "op not token");
                ZString op = x._list[0]._str;
                // area
                if (op == "@")
                {
                    e_location_t loc;
                    if (!parse_loc(x, loc))
                        return false;
                    out = magic_new_expr(EXPR::AREA);
                    out->e.e_area.ty = AREA::LOCATION;
                    out->e.e_area.a.a_loc = loc;
                    return true;
                }
                if (op == "@+")
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
                    out = magic_new_expr(EXPR::AREA);
                    out->e.e_area.ty = AREA::RECT;
                    out->e.e_area.a.a_rect.loc = loc;
                    out->e.e_area.a.a_rect.width = width;
                    out->e.e_area.a.a_rect.height = height;
                    return true;
                }
                if (op == "TOWARDS")
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
                    out = magic_new_expr(EXPR::AREA);
                    out->e.e_area.ty = AREA::BAR;
                    out->e.e_area.a.a_bar.loc = loc;
                    out->e.e_area.a.a_bar.dir = dir;
                    out->e.e_area.a.a_bar.width = width;
                    out->e.e_area.a.a_bar.depth = depth;
                    return true;
                }
                if (op == ".")
                {
                    if (x._list.size() != 3)
                        return fail(x, ". not 2");
                    dumb_ptr<expr_t> expr;
                    if (!parse_expression(x._list[1], expr))
                        return false;
                    if (x._list[2]._type != sexpr::TOKEN)
                        return fail(x._list[2], ".elem not name");
                    ZString elem = x._list[2]._str;
                    out = dot_expr(expr, intern_id(elem));
                    return true;
                }
                static
                std::set<ZString> ops =
                {
                    "<", ">", "<=", ">=", "==", "!=",
                    "+", "-", "*", "%", "/",
                    "&", "^", "|", "<<", ">>",
                    "&&", "||",
                };
                // TODO implement unary operators
                if (ops.count(op))
                {
                    // operators are n-ary and left-associative
                    if (x._list.size() < 3)
                        return fail(x, "operator not at least 2 args");
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
    bool parse_item(const SExpr& s, int& id, int& count)
    {
        if (s._type == sexpr::STRING)
        {
            count = 1;

            item_data *item = itemdb_searchname(s._str);
            if (!item)
                return fail(s, "no such item");
            id = item->nameid;
            return true;
        }
        if (s._type != sexpr::LIST)
            return fail(s, "item not string or list");
        if (s._list.size() != 2)
            return fail(s, "item list is not pair");
        if (s._list[0]._type != sexpr::INT)
            return fail(s._list[0], "item pair first not int");
        count = s._list[0]._int;
        if (s._list[1]._type != sexpr::STRING)
            return fail(s._list[1], "item pair second not name");

        item_data *item = itemdb_searchname(s._list[1]._str);
        if (!item)
            return fail(s, "no such item");
        id = item->nameid;
        return true;
    }

    static
    bool parse_spellguard(const SExpr& s, dumb_ptr<spellguard_t>& out)
    {
        if (s._type != sexpr::LIST)
            return fail(s, "not list");
        if (s._list.empty())
            return fail(s, "empty list");
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "not token");
        ZString cmd = s._list[0]._str;
        if (cmd == "OR")
        {
            auto begin = s._list.begin() + 1;
            auto end = s._list.end();
            if (begin == end)
                return fail(s, "missing arguments");
            if (!parse_spellguard(*begin, out))
                return false;
            ++begin;
            for (; begin != end; ++begin)
            {
                dumb_ptr<spellguard_t> alt;
                if (!parse_spellguard(*begin, alt))
                    return false;
                dumb_ptr<spellguard_t> choice = new_spellguard(SPELLGUARD::CHOICE);
                choice->next = out;
                choice->s.s_alt = alt;
                out = choice;
            }
            return true;
        }
        if (cmd == "GUARD")
        {
            auto begin = s._list.begin() + 1;
            auto end = s._list.end();
            while (is_comment(end[-1]))
                --end;
            if (begin == end)
                return fail(s, "missing arguments");
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
        if (cmd == "REQUIRE")
        {
            if (s._list.size() != 2)
                return fail(s, "not one argument");
            dumb_ptr<expr_t> condition;
            if (!parse_expression(s._list[1], condition))
                return false;
            out = new_spellguard(SPELLGUARD::CONDITION);
            out->s.s_condition = condition;
            return true;
        }
        if (cmd == "MANA")
        {
            if (s._list.size() != 2)
                return fail(s, "not one argument");
            dumb_ptr<expr_t> mana;
            if (!parse_expression(s._list[1], mana))
                return false;
            out = new_spellguard(SPELLGUARD::MANA);
            out->s.s_mana = mana;
            return true;
        }
        if (cmd == "CASTTIME")
        {
            if (s._list.size() != 2)
                return fail(s, "not one argument");
            dumb_ptr<expr_t> casttime;
            if (!parse_expression(s._list[1], casttime))
                return false;
            out = new_spellguard(SPELLGUARD::CASTTIME);
            out->s.s_casttime = casttime;
            return true;
        }
        if (cmd == "CATALYSTS")
        {
            dumb_ptr<component_t> items = nullptr;
            for (auto it = s._list.begin() + 1, end = s._list.end(); it != end; ++it)
            {
                int id, count;
                if (!parse_item(*it, id, count))
                    return false;
                magic_add_component(&items, id, count);
            }
            out = new_spellguard(SPELLGUARD::CATALYSTS);
            out->s.s_catalysts = items;
            return true;
        }
        if (cmd == "COMPONENTS")
        {
            dumb_ptr<component_t> items = nullptr;
            for (auto it = s._list.begin() + 1, end = s._list.end(); it != end; ++it)
            {
                int id, count;
                if (!parse_item(*it, id, count))
                    return false;
                magic_add_component(&items, id, count);
            }
            out = new_spellguard(SPELLGUARD::COMPONENTS);
            out->s.s_components = items;
            return true;
        }
        return fail(s._list[0], "unknown guard");
    }

    static
    bool build_effect_list(std::vector<SExpr>::const_iterator begin,
            std::vector<SExpr>::const_iterator end, dumb_ptr<effect_t>& out)
    {
        // these backward lists could be forward by keeping the reference
        // I know this is true because Linus said so
        out = new_effect(EFFECT::SKIP);
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
            return fail(s, "not list");
        if (s._list.empty())
            return fail(s, "empty list");
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "not token");
        ZString cmd = s._list[0]._str;
        if (cmd == "BLOCK")
        {
            return build_effect_list(s._list.begin() + 1, s._list.end(), out);
        }
        if (cmd == "SET")
        {
            if (s._list.size() != 3)
                return fail(s, "not 2 args");
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "not token");
            ZString name = s._list[1]._str;
            if (find_constant(name))
                return fail(s._list[1], "assigning to constant");
            dumb_ptr<expr_t> expr;
            if (!parse_expression(s._list[2], expr))
                return false;

            out = new_effect(EFFECT::ASSIGN);
            out->e.e_assign.id = intern_id(name);
            out->e.e_assign.expr = expr;
            return true;
        }
        if (cmd == "SCRIPT")
        {
            if (s._list.size() != 2)
                return fail(s, "not 1 arg");
            if (s._list[1]._type != sexpr::STRING)
                return fail(s._list[1], "not string");
            ZString body = s._list[1]._str;
            std::unique_ptr<const ScriptBuffer> script = parse_script(body, s._list[1]._span.begin.line, true);
            if (!script)
                return fail(s._list[1], "script does not compile");
            out = new_effect(EFFECT::SCRIPT);
            out->e.e_script = dumb_ptr<const ScriptBuffer>(script.release());
            return true;
        }
        if (cmd == "SKIP")
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg");
            out = new_effect(EFFECT::SKIP);
            return true;
        }
        if (cmd == "ABORT")
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg");
            out = new_effect(EFFECT::ABORT);
            return true;
        }
        if (cmd == "END")
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg");
            out = new_effect(EFFECT::END);
            return true;
        }
        if (cmd == "BREAK")
        {
            if (s._list.size() != 1)
                return fail(s, "not 0 arg");
            out = new_effect(EFFECT::BREAK);
            return true;
        }
        if (cmd == "FOREACH")
        {
            if (s._list.size() != 5)
                return fail(s, "not 4 arg");
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "foreach type not token");
            ZString type = s._list[1]._str;
            FOREACH_FILTER filter;
            if (type == "PC")
                filter = FOREACH_FILTER::PC;
            else if (type == "MOB")
                filter = FOREACH_FILTER::MOB;
            else if (type == "ENTITY")
                filter = FOREACH_FILTER::ENTITY;
            else if (type == "SPELL")
                filter = FOREACH_FILTER::SPELL;
            else if (type == "TARGET")
                filter = FOREACH_FILTER::TARGET;
            else if (type == "NPC")
                filter = FOREACH_FILTER::NPC;
            else
                return fail(s._list[1], "unknown foreach filter");
            if (s._list[2]._type != sexpr::TOKEN)
                return fail(s._list[2], "foreach var not token");
            ZString var = s._list[2]._str;
            dumb_ptr<expr_t> area;
            dumb_ptr<effect_t> effect;
            if (!parse_expression(s._list[3], area))
                return false;
            if (!parse_effect(s._list[4], effect))
                return false;
            out = new_effect(EFFECT::FOREACH);
            out->e.e_foreach.id = intern_id(var);
            out->e.e_foreach.area = area;
            out->e.e_foreach.body = effect;
            out->e.e_foreach.filter = filter;
            return true;
        }
        if (cmd == "FOR")
        {
            if (s._list.size() != 5)
                return fail(s, "not 4 arg");
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "for var not token");
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
            out = new_effect(EFFECT::FOR);
            out->e.e_for.id = intern_id(var);
            out->e.e_for.start = low;
            out->e.e_for.stop = high;
            out->e.e_for.body = effect;
            return true;
        }
        if (cmd == "IF")
        {
            if (s._list.size() != 3 && s._list.size() != 4)
                return fail(s, "not 2 or 3 args");
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
                if_false = new_effect(EFFECT::SKIP);
            out = new_effect(EFFECT::IF);
            out->e.e_if.cond = cond;
            out->e.e_if.true_branch = if_true;
            out->e.e_if.false_branch = if_false;
            return true;
        }
        if (cmd == "WAIT")
        {
            if (s._list.size() != 2)
                return fail(s, "not 1 arg");
            dumb_ptr<expr_t> expr;
            if (!parse_expression(s._list[1], expr))
                return false;
            out = new_effect(EFFECT::SLEEP);
            out->e.e_sleep = expr;
            return true;
        }
        if (cmd == "CALL")
        {
            if (s._list.size() < 2)
                return fail(s, "call what?");
            if (s._list[1]._type != sexpr::TOKEN)
                return fail(s._list[1], "call token please");
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
            return fail(s, "not list");
        if (s._list.empty())
            return fail(s, "empty list");
        if (s._list[0]._type != sexpr::TOKEN)
            return fail(s._list[0], "not token");
        ZString cmd = s._list[0]._str;
        if (cmd == "=>")
        {
            if (s._list.size() != 3)
                return fail(s, "list does not have exactly 2 arguments");
            dumb_ptr<spellguard_t> guard;
            if (!parse_spellguard(s._list[1], guard))
                return false;
            dumb_ptr<spellguard_t> body;
            if (!parse_spellbody(s._list[2], body))
                return false;
            out = spellguard_implication(guard, body);
            return true;
        }
        if (cmd == "|")
        {
            if (s._list.size() == 1)
                return fail(s, "spellbody choice empty");
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
                out = new_spellguard(SPELLGUARD::CHOICE);
                out->next = tmp;
                out->s.s_alt = alt;
            }
            return true;
        }
        if (cmd == "EFFECT")
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
                    && end[-1]._list[0]._str == "ATEND")
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
                    && end[-1]._list[0]._str == "ATTRIGGER")
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
            out = new_spellguard(SPELLGUARD::EFFECT);
            out->s.s_effect.effect = effect;
            out->s.s_effect.at_trigger = attrig;
            out->s.s_effect.at_end = atend;
            return true;
        }
        return fail(s._list[0], "unknown spellbody");
    }

    static
    bool parse_top_set(const std::vector<SExpr>& in)
    {
        if (in.size() != 3)
            return fail(in[0], "not 2 arguments");
        ZString name = in[1]._str;
        dumb_ptr<expr_t> expr;
        if (!parse_expression(in[2], expr))
            return false;
        if (find_constant(name))
            return fail(in[1], "assign constant");
        size_t var_id = intern_id(name);
        magic_eval(dumb_ptr<env_t>(&magic_default_env), &magic_conf.varv[var_id].val, expr);
        return true;
    }
    static
    bool parse_const(io::LineSpan span, const std::vector<SExpr>& in)
    {
        if (in.size() != 3)
            return fail(in[0], "not 2 arguments");
        if (in[1]._type != sexpr::TOKEN)
            return fail(in[1], "not token");
        ZString name = in[1]._str;
        dumb_ptr<expr_t> expr;
        if (!parse_expression(in[2], expr))
            return false;
        val_t tmp;
        magic_eval(dumb_ptr<env_t>(&magic_default_env), &tmp, expr);
        return bind_constant(span, name, &tmp);
    }
    static
    bool parse_anchor(io::LineSpan span, const std::vector<SExpr>& in)
    {
        if (in.size() != 4)
            return fail(in[0], "not 3 arguments");
        auto anchor = dumb_ptr<teleport_anchor_t>::make();
        if (in[1]._type != sexpr::TOKEN)
            return fail(in[1], "not token");
        anchor->name = in[1]._str;
        if (in[2]._type != sexpr::STRING)
            return fail(in[2], "not string");
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
            return fail(in[0], "not at least 3 arguments");
        auto proc = dumb_ptr<proc_t>::make();
        if (in[1]._type != sexpr::TOKEN)
            return fail(in[1], "name not token");
        proc->name = in[1]._str;
        if (in[2]._type != sexpr::LIST)
            return fail(in[2], "args not list");
        for (const SExpr& arg : in[2]._list)
        {
            if (arg._type != sexpr::TOKEN)
                return fail(arg, "arg not token");
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
            return fail(in[0], "not at least 5 arguments");
        if (in[1]._type != sexpr::LIST)
            return fail(in[1], "flags not list");

        auto spell = dumb_ptr<spell_t>::make();

        for (const SExpr& s : in[1]._list)
        {
            if (s._type != sexpr::TOKEN)
                return fail(s, "flag not token");
            SPELL_FLAG flag = SPELL_FLAG::ZERO;
            if (s._str == "LOCAL")
                flag = SPELL_FLAG::LOCAL;
            else if (s._str == "NONMAGIC")
                flag = SPELL_FLAG::NONMAGIC;
            else if (s._str == "SILENT")
                flag = SPELL_FLAG::SILENT;
            else
                return fail(s, "unknown flag");
            if (bool(spell->flags & flag))
                return fail(s, "duplicate flag");
            spell->flags |= flag;
        }
        if (in[2]._type != sexpr::TOKEN)
            return fail(in[2], "name not token");
        spell->name = in[2]._str;
        if (in[3]._type != sexpr::STRING)
            return fail(in[3], "invoc not string");
        spell->invocation = in[3]._str;
        if (in[4]._type != sexpr::LIST)
            return fail(in[4], "spellarg not list");
        if (in[4]._list.size() == 0)
        {
            spell->spellarg_ty = SPELLARG::NONE;
        }
        else
        {
            if (in[4]._list.size() != 2)
                return fail(in[4], "spellarg not empty list or pair");
            if (in[4]._list[0]._type != sexpr::TOKEN)
                return fail(in[4]._list[0], "spellarg type not token");
            if (in[4]._list[1]._type != sexpr::TOKEN)
                return fail(in[4]._list[1], "spellarg name not token");
            ZString ty = in[4]._list[0]._str;
            if (ty == "PC")
                spell->spellarg_ty = SPELLARG::PC;
            else if (ty == "STRING")
                spell->spellarg_ty = SPELLARG::STRING;
            else
                return fail(in[4]._list[0], "unknown spellarg type");
            ZString an = in[4]._list[1]._str;
            spell->arg = intern_id(an);
        }
        std::vector<SExpr>::const_iterator it = in.begin() + 5;
        for (;; ++it)
        {
            if (it == in.end())
                return fail(it[-1], "end of list scanning LET defs");
            if (is_comment(*it))
                continue;
            if (it->_type != sexpr::LIST || it->_list.empty())
                break;
            if (it->_list[0]._type != sexpr::TOKEN || it->_list[0]._str != "LET")
                break;

            if (it->_list[1]._type != sexpr::TOKEN)
                return fail(it->_list[1], "let name not token");
            ZString name = it->_list[1]._str;
            if (find_constant(name))
                return fail(it->_list[1], "constant exists");
            dumb_ptr<expr_t> expr;
            if (!parse_expression(it->_list[2], expr))
                return false;
            letdef_t let;
            let.id = intern_id(name);
            let.expr = expr;
            spell->letdefv.push_back(let);
        }
        if (it + 1 != in.end())
            return fail(*it, "expected only one body entry besides LET");

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
            span.error("Empty list at top");
            return false;
        }
        if (vs[0]._type != sexpr::TOKEN)
            return fail(vs[0], "top not token");
        ZString cmd = vs[0]._str;
        if (cmd == "CONST")
            return parse_const(span, vs);
        if (cmd == "PROCEDURE")
            return parse_proc(span, vs);
        if (cmd == "SET")
            return parse_top_set(vs);
        if (cmd == "SPELL")
            return parse_spell(span, vs);
        if (cmd == "TELEPORT-ANCHOR")
            return parse_anchor(span, vs);
        return fail(vs[0], "Unknown top-level command");
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
                return fail(s, "top-level entity not a list or comment");
            if (!parse_top(s._span, s._list))
                return false;
        }
        // handle low-level errors
        if (in.peek() != sexpr::TOK_EOF)
        {
            in.span().error("parser gave up before end of file");
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
        in.span().error(STRPRINTF("next token: %s '%s'", sexpr::token_name(in.peek()), in.val_string()));
    }
    return rv;
}
