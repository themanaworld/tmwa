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

#include "../strings/mstring.hpp"

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

    // Lua brace scanner (doc/lua-engine.md section 7.2): after '{', count
    // brace depth; braces inside "..." / '...' strings (with backslash
    // escapes) are ignored. Long brackets and '--' comments are NOT
    // recognised: item lines are one physical line. The text between the
    // outer braces is the chunk.
    static
    Result<ScriptBody> lex_lua_body(io::LineCharReader& lr)
    {
        io::LineChar c;
        while (lr.get(c) && c.ch() == ' ')
            lr.adv();
        if (!lr.get(c))
            return Err("unexpected EOF before item script"_s);
        if (c.ch() != '{')
            return Err(c.error_str("expected '{' to start item script"_s));
        ScriptBody rv;
        rv.span.begin = c;
        rv.span.end = c;
        lr.adv();
        MString accum;
        int depth = 1;
        char quote = '\0';
        bool escaped = false;
        while (true)
        {
            if (!lr.get(c))
                return Err("unexpected EOF in item script"_s);
            if (c.ch() == '\n')
                return Err(c.error_str("unexpected EOL in item script"_s));
            char ch = c.ch();
            if (quote)
            {
                if (escaped)
                    escaped = false;
                else if (ch == '\\')
                    escaped = true;
                else if (ch == quote)
                    quote = '\0';
            }
            else
            {
                if (ch == '"' || ch == '\'')
                    quote = ch;
                else if (ch == '{')
                    depth++;
                else if (ch == '}')
                {
                    depth--;
                    if (depth == 0)
                    {
                        rv.span.end = c;
                        lr.adv();
                        break;
                    }
                }
            }
            accum += ch;
            lr.adv();
        }
        rv.text = RString(accum);
        skip_comma_space(lr);
        return Ok(std::move(rv));
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
        item.use_script = TRY(lex_lua_body(lr));
        item.equip_script = TRY(lex_lua_body(lr));
        ItemOrComment rv = std::move(item);
        rv.span.begin = item.id.span.begin;
        rv.span.end = item.equip_script.span.end;
        return Some(Ok(std::move(rv)));
    }
} // namespace item
} // namespace ast
} // namespace tmwa
