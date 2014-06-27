#include "lexer.hpp"
//    lexer.cpp - tokenize a stream of S-expressions
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

#include "../strings/mstring.hpp"
#include "../strings/vstring.hpp"
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace sexpr
{
    Lexeme Lexer::_adv()
    {
        LString whitespace = " \t\n\r\v\f"_s;
        while (true)
        {
            if (!_in.get(_span.begin))
            {
                if (!_depth.empty())
                {
                    _depth.back().error("Unmatched '('"_s);
                    return TOK_ERROR;
                }
                return TOK_EOF;
            }
            char co = _span.begin.ch();
            if (!whitespace.contains(co))
                break;
            _in.adv();
        }

        char co = _span.begin.ch();
        _in.adv();
        _span.end = _span.begin;
        switch (co)
        {
        case '(':
            _string = "("_s;
            _depth.push_back(_span.end);
            return TOK_OPEN;
        case ')':
            _string = ")"_s;
            if (_depth.empty())
            {
                _span.end.error("Unmatched ')'"_s);
                return TOK_ERROR;
            }
            _depth.pop_back();
            return TOK_CLOSE;
        case '"':
            {
                MString collect;
                // read until " and consume it
                // but handle \s
                while (true)
                {
                    if (!_in.get(_span.end))
                    {
                        _span.error("EOF in string literal"_s);
                        return TOK_ERROR;
                    }
                    char ch = _span.end.ch();
                    _in.adv();
                    if (ch == '"')
                        break;

                    if (ch != '\\')
                    {
                        collect += ch;
                        continue;
                    }

                    if (!_in.get(_span.end))
                    {
                        _span.end.error("EOF at backslash in string"_s);
                        return TOK_ERROR;
                    }
                    ch = _span.end.ch();
                    _in.adv();
                    switch (ch)
                    {
                    default:
                        _span.end.error("Unknown backslash sequence"_s);
                        return TOK_ERROR;
                    case 'a': collect += '\a'; break;
                    case 'b': collect += '\b'; break;
                    case 'e': collect += '\e'; break;
                    case 'f': collect += '\f'; break;
                    case 'n': collect += '\n'; break;
                    case 'r': collect += '\r'; break;
                    case 't': collect += '\t'; break;
                    case 'v': collect += '\v'; break;
                    case '\\': collect += '\\'; break;
                    case '\"': collect += '\"'; break;
                    case 'x':
                        {
                            unsigned char tmp = 0;
                            for (int i = 0; i < 2; ++i)
                            {
                                tmp *= 16;
                                if (!_in.get(_span.end))
                                {
                                    _span.end.error("EOF after \\x in string"_s);
                                    return TOK_ERROR;
                                }
                                char cx = _span.end.ch();
                                _in.adv();
                                if ('0' <= cx && cx <= '9')
                                    tmp += cx - '0';
                                else if ('A' <= cx && cx <= 'F')
                                    tmp += cx - 'A' + 10;
                                else if ('a' <= cx && cx <= 'a')
                                    tmp += cx - 'a' + 10;
                                else
                                {
                                    _span.end.error("Non-hex char after \\x"_s);
                                    return TOK_ERROR;
                                }
                            }
                            collect += tmp;
                        }
                    }
                }
                _string = AString(collect);
                return TOK_STRING;
            }
        case '\'':
        case '\\':
            _span.end.error("forbidden character"_s);
            return TOK_ERROR;
        default:
            // this includes integers - they are differentiated in parsing
            {
                MString collect;
                collect += co;
                // read until whitespace, (, ), ", or EOF
                io::LineChar tmp;
                while (_in.get(tmp))
                {
                    char ct = tmp.ch();
                    if (ct == '\'' || ct == '\\')
                        // error later
                        break;
                    if (ct == '(' || ct == ')' || ct == '"')
                        break;
                    if (whitespace.contains(ct))
                        break;
                    collect += ct;
                    _span.end = tmp;
                    _in.adv();
                }
                _string = AString(collect);
                if (!_string.is_print())
                    _span.error("String is not entirely printable"_s);
                return TOK_TOKEN;
            }
        }
    }

    VString<4> escape(char c)
    {
        switch (c)
        {
        case '\a': return "\\a"_s;
        case '\b': return "\\b"_s;
        case '\e': return "\\e"_s;
        case '\f': return "\\f"_s;
        //case '\n': return "\\n"_s;
        case '\r': return "\\r"_s;
        case '\t': return "\\t"_s;
        case '\v': return "\\v"_s;
        case '\\': return "\\\\"_s;
        case '\"': return "\\\""_s;
        default:
            if (c == '\n')
                return c;
            if (' ' <= c && c <= '~')
                return c;
            else
                return STRNPRINTF(5, "\\x%02x"_fmt, static_cast<uint8_t>(c));
        }
    }
    AString escape(XString s)
    {
        MString m;
        m += '"';
        for (char c : s)
            m += escape(c);
        m += '"';
        return AString(m);
    }

    LString token_name(Lexeme tok)
    {
        switch (tok)
        {
        case TOK_EOF:
            return "EOF"_s;
        case TOK_OPEN:
            return "OPEN"_s;
        case TOK_CLOSE:
            return "CLOSE"_s;
        case TOK_STRING:
            return "STRING"_s;
        case TOK_TOKEN:
            return "TOKEN"_s;
        default:
            return "ERROR"_s;
        }
    }
} // namespace sexpr
} // namespace tmwa
