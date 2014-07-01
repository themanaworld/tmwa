#include "lexer.hpp"
//    lexer_test.cpp - Testsuite for sexpr tokenizer.
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

#include <gtest/gtest.h>

#include "../strings/vstring.hpp"

#include "../tests/fdhack.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
io::FD string_pipe(ZString sz)
{
    io::FD rfd, wfd;
    if (-1 == io::FD::pipe(rfd, wfd))
        return io::FD();
    if (sz.size() != wfd.write(sz.c_str(), sz.size()))
    {
        rfd.close();
        wfd.close();
        return io::FD();
    }
    wfd.close();
    return rfd;
}

TEST(sexpr, escape)
{
    EXPECT_EQ(sexpr::escape('\0'), "\\x00"_s);
    EXPECT_EQ(sexpr::escape('\x1f'), "\\x1f"_s);
    EXPECT_EQ(sexpr::escape('\x20'), " "_s);
    EXPECT_EQ(sexpr::escape('\x7e'), "~"_s);
    EXPECT_EQ(sexpr::escape('\x7f'), "\\x7f"_s);
    EXPECT_EQ(sexpr::escape('\x80'), "\\x80"_s);
    EXPECT_EQ(sexpr::escape('\xff'), "\\xff"_s);
    EXPECT_EQ(sexpr::escape('\a'), "\\a"_s);
    EXPECT_EQ(sexpr::escape('\b'), "\\b"_s);
    EXPECT_EQ(sexpr::escape('\e'), "\\e"_s);
    EXPECT_EQ(sexpr::escape('\f'), "\\f"_s);
    //EXPECT_EQ(sexpr::escape('\n'), "\\n"_s);
    EXPECT_EQ(sexpr::escape('\n'), "\n"_s);
    EXPECT_EQ(sexpr::escape('\r'), "\\r"_s);
    EXPECT_EQ(sexpr::escape('\t'), "\\t"_s);
    EXPECT_EQ(sexpr::escape('\v'), "\\v"_s);
    EXPECT_EQ(sexpr::escape('\\'), "\\\\"_s);
    EXPECT_EQ(sexpr::escape('\"'), "\\\""_s);

    EXPECT_EQ(sexpr::escape("\x1f\x20\x7e\x7f\x80\xff\a\b\e\f\r\t\v\\\""_s),
            "\"\\x1f ~\\x7f\\x80\\xff\\a\\b\\e\\f\\r\\t\\v\\\\\\\"\""_s);
}

TEST(sexpr, lexer)
{
    io::LineSpan span;
    sexpr::Lexer lexer("<lexer-test1>"_s, string_pipe(" foo( ) 123\"\" \n"_s));
    EXPECT_EQ(lexer.peek(), sexpr::TOK_TOKEN);
    EXPECT_EQ(lexer.val_string(), "foo"_s);
    EXPECT_EQ(lexer.span().message_str("error"_s, "test"_s),
            "<lexer-test1>:1:2: error: test\n"
            " foo( ) 123\"\" \n"
            " ^~~\n"_s
    );
    lexer.adv();
    EXPECT_EQ(lexer.peek(), sexpr::TOK_OPEN);
    EXPECT_EQ(lexer.span().message_str("error"_s, "test"_s),
            "<lexer-test1>:1:5: error: test\n"
            " foo( ) 123\"\" \n"
            "    ^\n"_s
    );
    lexer.adv();
    EXPECT_EQ(lexer.peek(), sexpr::TOK_CLOSE);
    EXPECT_EQ(lexer.span().message_str("error"_s, "test"_s),
            "<lexer-test1>:1:7: error: test\n"
            " foo( ) 123\"\" \n"
            "      ^\n"_s
    );
    lexer.adv();
    EXPECT_EQ(lexer.peek(), sexpr::TOK_TOKEN);
    EXPECT_EQ(lexer.val_string(), "123"_s);
    EXPECT_EQ(lexer.span().message_str("error"_s, "test"_s),
            "<lexer-test1>:1:9: error: test\n"
            " foo( ) 123\"\" \n"
            "        ^~~\n"_s
    );
    lexer.adv();
    EXPECT_EQ(lexer.peek(), sexpr::TOK_STRING);
    EXPECT_EQ(lexer.val_string(), ""_s);
    EXPECT_EQ(lexer.span().message_str("error"_s, "test"_s),
            "<lexer-test1>:1:12: error: test\n"
            " foo( ) 123\"\" \n"
            "           ^~\n"_s
    );
    lexer.adv();
    EXPECT_EQ(lexer.peek(), sexpr::TOK_EOF);
}

TEST(sexpr, lexbad)
{
    QuietFd q;
    {
        io::LineSpan span;
        sexpr::Lexer lexer("<lexer-bad>"_s, string_pipe("(\n"_s));
        EXPECT_EQ(lexer.peek(), sexpr::TOK_OPEN);
        lexer.adv();
        EXPECT_EQ(lexer.peek(), sexpr::TOK_ERROR);
    }
    for (ZString bad : {
            ZString(")\n"_s),
            ZString("\"\n"_s),
            ZString("'\n"_s),
            ZString("\\\n"_s),
            ZString("\"\\"_s),
            ZString("\"\\z\""_s),
    })
    {
        io::LineSpan span;
        sexpr::Lexer lexer("<lexer-bad>"_s, string_pipe(bad));
        EXPECT_EQ(lexer.peek(), sexpr::TOK_ERROR);
    }
}
} // namespace tmwa
