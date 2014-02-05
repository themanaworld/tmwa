#include "line.hpp"

#include <gtest/gtest.h>

#include "../strings/zstring.hpp"

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

TEST(io, line1)
{
    io::LineReader lr("<string1>", string_pipe("Hello World\n"));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello World");
    EXPECT_EQ(hi.filename, "<string1>");
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line2)
{
    io::LineReader lr("<string2>", string_pipe("Hello\nWorld"));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello");
    EXPECT_EQ(hi.filename, "<string2>");
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World");
    EXPECT_EQ(hi.filename, "<string2>");
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line3)
{
    io::LineReader lr("<string3>", string_pipe("Hello\rWorld"));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello");
    EXPECT_EQ(hi.filename, "<string3>");
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World");
    EXPECT_EQ(hi.filename, "<string3>");
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line4)
{
    io::LineReader lr("<string4>", string_pipe("Hello\r\nWorld"));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello");
    EXPECT_EQ(hi.filename, "<string4>");
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World");
    EXPECT_EQ(hi.filename, "<string4>");
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line5)
{
    io::LineReader lr("<string5>", string_pipe("Hello\n\rWorld"));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello");
    EXPECT_EQ(hi.filename, "<string5>");
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "");
    EXPECT_EQ(hi.filename, "<string5>");
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World");
    EXPECT_EQ(hi.filename, "<string5>");
    EXPECT_EQ(hi.line, 3);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}

TEST(io, linechar1)
{
    io::LineCharReader lr("<stringchar1>", string_pipe("Hi Wu\n"));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi Wu");
    EXPECT_EQ(c.filename, "<stringchar1>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi Wu");
    EXPECT_EQ(c.filename, "<stringchar1>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), ' ');
    EXPECT_EQ(c.text, "Hi Wu");
    EXPECT_EQ(c.filename, "<stringchar1>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Hi Wu");
    EXPECT_EQ(c.filename, "<stringchar1>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 4);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Hi Wu");
    EXPECT_EQ(c.filename, "<stringchar1>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 5);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi Wu");
    EXPECT_EQ(c.filename, "<stringchar1>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 6);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar2)
{
    io::LineCharReader lr("<stringchar2>", string_pipe("Hi\nWu"));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar2>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar2>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar2>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar2>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar2>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar2>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar3)
{
    io::LineCharReader lr("<stringchar3>", string_pipe("Hi\rWu"));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar3>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar3>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar3>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar3>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar3>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar3>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar4)
{
    io::LineCharReader lr("<stringchar4>", string_pipe("Hi\r\nWu"));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar4>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar4>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar4>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar4>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar4>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar4>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar5)
{
    io::LineCharReader lr("<stringchar5>", string_pipe("Hi\n\rWu"));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 3);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 3);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu");
    EXPECT_EQ(c.filename, "<stringchar5>");
    EXPECT_EQ(c.line, 3);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
