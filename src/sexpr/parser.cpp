#include "parser.hpp"
//    parser.cpp - build a tree of S-expressions
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

#include <cerrno>

#include "../poison.hpp"

namespace sexpr
{
    bool token_is_int(ZString s, int64_t& out, bool& ok)
    {
        if (!s)
            return false;
        if (s.startswith('-') || s.xslice_h(1).is_digit10())
        {
            const char *z = s.c_str();
            char *end = nullptr;
            errno = 0;
            out = strtoll(z, &end, 0);
            if (errno)
                ok = false;
            return !*end;
        }
        return false;
    }

    bool parse(Lexer& lex, SExpr& out)
    {
        out._list.clear();
        out._str = RString();

        bool rv = true;
        out._span.begin = lex.span().begin;
        switch (lex.peek())
        {
        default:
            return false;
        case TOK_STRING:
            out._type = STRING;
            out._str = lex.val_string();
            break;
        case TOK_TOKEN:
            out._type = TOKEN;
            out._str = lex.val_string();
            if (token_is_int(out._str, out._int, rv))
                out._type = INT;
            break;
        case TOK_OPEN:
            out._type = LIST;
            lex.adv();
            while (lex.peek() != TOK_CLOSE)
            {
                SExpr tmp;
                if (!parse(lex, tmp))
                    return false;
                out._list.push_back(std::move(tmp));
            }
            break;
        }
        out._span.end = lex.span().end;
        lex.adv();
        return rv;
    }
} // namespace sexpr
