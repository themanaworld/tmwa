#include "item.hpp"
//    ast/item.cpp - Structure of itemdb
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

#include "../io/extract.hpp"
#include "../io/line.hpp"

#include "../mmo/extract_enums.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace ast
{
namespace item
{
    using io::respan;

    static
    void skip_comma_space(io::LineCharReader& lr)
    {
        io::LineChar c;
        if (lr.get(c) && c.ch() == ',')
        {
            lr.adv();
            while (lr.get(c) && c.ch() == ' ')
            {
                lr.adv();
            }
        }
    }
    static
    Option<Spanned<RString>> lex_nonscript(io::LineCharReader& lr, bool first)
    {
        io::LineChar c;
        if (first)
        {
            while (lr.get(c) && c.ch() == '\n')
            {
                lr.adv();
            }
        }
        if (!lr.get(c))
        {
            return None;
        }
        io::LineSpan span;
        MString accum;
        accum += c.ch();
        span.begin = c;
        span.end = c;
        lr.adv();
        if (c.ch() != '/')
            first = false;

        if (first && lr.get(c) && c.ch() == '/')
        {
            accum += c.ch();
            span.end = c;
            lr.adv();
            while (lr.get(c) && c.ch() != '\n')
            {
                accum += c.ch();
                span.end = c;
                lr.adv();
            }
            return Some(respan(span, RString(accum)));
        }

        while (lr.get(c) && c.ch() != ',' && c.ch() != '\n')
        {
            accum += c.ch();
            span.end = c;
            lr.adv();
        }
        skip_comma_space(lr);
        return Some(respan(span, RString(accum)));
    }

    static
    Result<ast::script::ScriptBody> lex_script(io::LineCharReader& lr)
    {
        ast::script::ScriptOptions opt;
        opt.implicit_start = true;
        opt.implicit_end = true;
        opt.one_line = true;
        opt.no_event = true;
        auto rv = ast::script::parse_script_body(lr, opt);
        if (rv.get_success().is_some())
        {
            skip_comma_space(lr);
        }
        return rv;
    }

#define SPAN_EXTRACT(bitexpr, var) ({ auto bit = bitexpr; if (!extract(bit.data, &var.data)) return Err(bit.span.error_str("failed to extract "_s #var)); var.span = bit.span; })

#define EOL_ERROR(lr) ({ io::LineChar c; lr.get(c) ? Err(c.error_str("unexpected EOL"_s)) : Err("unexpected EOF before unexpected EOL"_s); })
    Option<Result<ItemOrComment>> parse_item(io::LineCharReader& lr)
    {
        Spanned<RString> first = TRY_UNWRAP(lex_nonscript(lr, true), return None);
        if (first.data.startswith("//"_s))
        {
            Comment comment;
            comment.comment = first.data;
            ItemOrComment rv = std::move(comment);
            rv.span = first.span;
            return Some(Ok(std::move(rv)));
        }
        Item item;
        SPAN_EXTRACT(first, item.id);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.name);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.type);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.buy_price);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.sell_price);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.weight);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.atk);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.def);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.range);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.magic_bonus);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.slot_unused);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.gender);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.loc);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.wlv);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.elv);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.view);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), item.mode);
        item.use_script = TRY(lex_script(lr));
        item.equip_script = TRY(lex_script(lr));
        ItemOrComment rv = std::move(item);
        rv.span.begin = item.id.span.begin;
        rv.span.end = item.equip_script.span.end;
        return Some(Ok(std::move(rv)));
    }
} // namespace item
} // namespace ast
} // namespace tmwa
