#include "quest.hpp"
//    ast/quest.cpp - Structure of tmwa questdb
//
//    Copyright © 2015 Ed Pasek <pasekei@gmail.com>
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
namespace quest
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

#define SPAN_EXTRACT(bitexpr, var) ({ auto bit = bitexpr; if (!extract(bit.data, &var.data)) return Err(bit.span.error_str("failed to extract "_s #var)); var.span = bit.span; })

#define EOL_ERROR(lr) ({ io::LineChar c; lr.get(c) ? Err(c.error_str("unexpected EOL"_s)) : Err("unexpected EOF before unexpected EOL"_s); })
    Option<Result<QuestOrComment>> parse_quest(io::LineCharReader& lr)
    {
        Spanned<RString> first = TRY_UNWRAP(lex_nonscript(lr, true), return None);
        if (first.data.startswith("//"_s))
        {
            Comment comment;
            comment.comment = first.data;
            QuestOrComment rv = std::move(comment);
            rv.span = first.span;
            return Some(Ok(std::move(rv)));
        }
        Quest quest;
        SPAN_EXTRACT(first, quest.questid);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), quest.quest_var);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), quest.quest_vr);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), quest.quest_shift);
        SPAN_EXTRACT(TRY_UNWRAP(lex_nonscript(lr, false), return EOL_ERROR(lr)), quest.quest_mask);
        QuestOrComment rv = std::move(quest);
        rv.span.begin = quest.questid.span.begin;
        rv.span.end = quest.quest_mask.span.end;
        return Some(Ok(std::move(rv)));
    }
} // namespace quest
} // namespace ast
} // namespace tmwa
