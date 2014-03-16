#include "parser.hpp"

#include <gtest/gtest.h>

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

TEST(sexpr, parser)
{
    sexpr::SExpr s;
    io::LineSpan span;
    sexpr::Lexer lexer("<parser-test1>", string_pipe(" foo( ) 123\"\" \n"));

    EXPECT_TRUE(sexpr::parse(lexer, s));
    EXPECT_EQ(s._type, sexpr::TOKEN);
    EXPECT_EQ(s._str, "foo");

    EXPECT_TRUE(sexpr::parse(lexer, s));
    EXPECT_EQ(s._type, sexpr::LIST);
    EXPECT_EQ(s._list, std::vector<sexpr::SExpr>());

    EXPECT_TRUE(sexpr::parse(lexer, s));
    EXPECT_EQ(s._type, sexpr::INT);
    EXPECT_EQ(s._int, 123);

    EXPECT_TRUE(sexpr::parse(lexer, s));
    EXPECT_EQ(s._type, sexpr::STRING);
    EXPECT_EQ(s._str, "");

    EXPECT_FALSE(sexpr::parse(lexer, s));
    EXPECT_EQ(lexer.peek(), sexpr::TOK_EOF);
}

TEST(sexpr, parselist)
{
    sexpr::SExpr s;
    sexpr::Lexer lexer("<parser-test1>", string_pipe("(foo)(bar)\n"));

    EXPECT_TRUE(sexpr::parse(lexer, s));
    EXPECT_EQ(s._type, sexpr::LIST);
    EXPECT_EQ(s._list.size(), 1);
    EXPECT_EQ(s._list[0]._type, sexpr::TOKEN);
    EXPECT_EQ(s._list[0]._str, "foo");

    EXPECT_TRUE(sexpr::parse(lexer, s));
    EXPECT_EQ(s._type, sexpr::LIST);
    EXPECT_EQ(s._list.size(), 1);
    EXPECT_EQ(s._list[0]._type, sexpr::TOKEN);
    EXPECT_EQ(s._list[0]._str, "bar");

    EXPECT_FALSE(sexpr::parse(lexer, s));
    EXPECT_EQ(lexer.peek(), sexpr::TOK_EOF);
}

TEST(sexpr, parsebad)
{
    for (ZString bad : {
            ZString("(\n"),
            ZString(")\n"),
            ZString("\"\n"),
            ZString("'\n"),
            ZString("\\\n"),
            ZString("\"\\"),
            ZString("\"\\z\""),
            ZString("(()\n"),
            ZString("((\n"),
            ZString("((\"\n"),
    })
    {
        sexpr::SExpr s;
        io::LineSpan span;
        sexpr::Lexer lexer("<parse-bad>", string_pipe(bad));
        EXPECT_FALSE(sexpr::parse(lexer, s));
        EXPECT_EQ(lexer.peek(), sexpr::TOK_ERROR);
    }
}
