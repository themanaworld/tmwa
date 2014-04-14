#ifndef TMWA_SEXPR_PARSER_HPP
#define TMWA_SEXPR_PARSER_HPP
//    parser.hpp - build a tree of S-expressions
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

# include "../sanity.hpp"

# include <cstdlib>

# include "../strings/zstring.hpp"

# include "../io/line.hpp"

# include "lexer.hpp"

namespace sexpr
{
    enum Type
    {
        LIST,
        INT,
        STRING,
        TOKEN,
    };

    struct SExpr
    {
        Type _type;
        int64_t _int;
        RString _str;
        std::vector<SExpr> _list;

        io::LineSpan _span;

        SExpr() : _type(), _int(), _str(), _list(), _span() {}
    };

    inline
    bool operator == (const SExpr& l, const SExpr& r)
    {
        if (l._type != r._type)
            return false;
        switch (l._type)
        {
        case LIST:
            return l._list == r._list;
        case INT:
            return l._int == r._int;
        case STRING:
        case TOKEN:
            return l._str == r._str;
        }
        abort();
    }
    inline
    bool operator != (const SExpr& l, const SExpr& r)
    {
        return !(l == r);
    }

    bool token_is_int(ZString s, int64_t& out, bool& ok);
    /// return false on error or eof, check lex.peek() == TOK_EOF to see
    bool parse(Lexer& lex, SExpr& out);
} // namespace sexpr

#endif // TMWA_SEXPR_PARSER_HPP
