#include "script.hpp"
//    ast/script.cpp - Structure of tmwa-script
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "../io/line.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace ast
{
namespace script
{
    Result<ScriptBody> parse_script_body(io::LineCharReader& lr, ScriptOptions opt)
    {
        io::LineSpan span;
        io::LineChar c;
        while (true)
        {
            if (!lr.get(c))
            {
                return Err("error: unexpected EOF before '{' in parse_script_body"_s);
            }
            if (c.ch() == ' ' || (!opt.one_line && c.ch() == '\n'))
            {
                lr.adv();
                continue;
            }
            break;
        }
        if (c.ch() != '{')
        {
            return Err(c.error_str("expected opening '{'"_s));
        }

        MString accum;
        accum += c.ch();
        span.begin = c;
        lr.adv();
        while (true)
        {
            if (!lr.get(c))
                return Err(c.error_str("unexpected EOF before '}' in parse_script_body"_s));
            if (opt.one_line && c.ch() == '\n')
                return Err(c.error_str("unexpected EOL before '}' in parse_script_body"_s));
            accum += c.ch();
            span.end = c;
            lr.adv();
            if (c.ch() == '}')
            {
                return Ok(ScriptBody{RString(accum), std::move(span)});
            }
        }
    }
} // namespace script
} // namespace ast
} // namespace tmwa
