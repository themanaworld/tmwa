#ifndef TMWA_SEXPR_LEXER_HPP
#define TMWA_SEXPR_LEXER_HPP
//    lexer.hpp - tokenize a stream of S-expressions
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

# include <vector>

# include "../strings/astring.hpp"
# include "../strings/vstring.hpp"
# include "../strings/xstring.hpp"
# include "../strings/zstring.hpp"

# include "../io/line.hpp"

namespace sexpr
{
    enum Lexeme
    {
        TOK_EOF = 0,
        TOK_ERROR,
        TOK_OPEN,
        TOK_CLOSE,
        TOK_STRING,
        TOK_TOKEN,
    };

    class Lexer
    {
        io::LineCharReader _in;
        Lexeme _current;
        AString _string;
        io::LineSpan _span;
        std::vector<io::LineChar> _depth;
    private:
        Lexeme _adv();
    public:
        explicit
        Lexer(ZString filename)
        : _in(filename), _current(TOK_EOF), _span(), _depth()
        { adv(); }
        // for unit tests
        Lexer(ZString fake, io::FD fd)
        : _in(fake, fd), _current(TOK_EOF), _span(), _depth()
        { adv(); }
        Lexeme peek() { return _current; }
        void adv() { _current = _adv(); }
        ZString val_string() { return _string; }
        io::LineSpan span() { return _span; }
    };

    VString<4> escape(char c);
    AString escape(XString s);

    LString token_name(Lexeme tok);
} // namespace sexpr

#endif // TMWA_SEXPR_LEXER_HPP
