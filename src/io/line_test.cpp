#include "line.hpp"
//    io/line_test.cpp - Testsuite for input from files, line-by-line
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

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

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

TEST(io, line1)
{
    io::LineReader lr("<string1>"_s, string_pipe("Hello World\n"_s));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello World"_s);
    EXPECT_EQ(hi.filename, "<string1>"_s);
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line2)
{
    QuietFd q;
    io::LineReader lr("<string2>"_s, string_pipe("Hello\nWorld"_s));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello"_s);
    EXPECT_EQ(hi.filename, "<string2>"_s);
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World"_s);
    EXPECT_EQ(hi.filename, "<string2>"_s);
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line3)
{
    QuietFd q;
    io::LineReader lr("<string3>"_s, string_pipe("Hello\rWorld"_s));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello"_s);
    EXPECT_EQ(hi.filename, "<string3>"_s);
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World"_s);
    EXPECT_EQ(hi.filename, "<string3>"_s);
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line4)
{
    QuietFd q;
    io::LineReader lr("<string4>"_s, string_pipe("Hello\r\nWorld"_s));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello"_s);
    EXPECT_EQ(hi.filename, "<string4>"_s);
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World"_s);
    EXPECT_EQ(hi.filename, "<string4>"_s);
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}
TEST(io, line5)
{
    QuietFd q;
    io::LineReader lr("<string5>"_s, string_pipe("Hello\n\rWorld"_s));
    io::Line hi;
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "Hello"_s);
    EXPECT_EQ(hi.filename, "<string5>"_s);
    EXPECT_EQ(hi.line, 1);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, ""_s);
    EXPECT_EQ(hi.filename, "<string5>"_s);
    EXPECT_EQ(hi.line, 2);
    EXPECT_EQ(hi.column, 0);
    EXPECT_TRUE(lr.read_line(hi));
    EXPECT_EQ(hi.text, "World"_s);
    EXPECT_EQ(hi.filename, "<string5>"_s);
    EXPECT_EQ(hi.line, 3);
    EXPECT_EQ(hi.column, 0);
    EXPECT_FALSE(lr.read_line(hi));
}

TEST(io, linechar1)
{
    io::LineCharReader lr("<stringchar1>"_s, string_pipe("Hi Wu\n"_s));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar1>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar1>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), ' ');
    EXPECT_EQ(c.text, "Hi Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar1>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Hi Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar1>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 4);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Hi Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar1>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 5);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar1>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 6);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar2)
{
    QuietFd q;
    io::LineCharReader lr("<stringchar2>"_s, string_pipe("Hi\nWu"_s));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar2>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar2>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar2>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar2>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar2>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar2>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar3)
{
    QuietFd q;
    io::LineCharReader lr("<stringchar3>"_s, string_pipe("Hi\rWu"_s));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar3>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar3>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar3>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar3>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar3>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar3>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar4)
{
    QuietFd q;
    io::LineCharReader lr("<stringchar4>"_s, string_pipe("Hi\r\nWu"_s));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar4>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar4>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar4>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar4>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar4>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar4>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}
TEST(io, linechar5)
{
    QuietFd q;
    io::LineCharReader lr("<stringchar5>"_s, string_pipe("Hi\n\rWu"_s));
    io::LineChar c;
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'H');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'i');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Hi"_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 1);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, ""_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 2);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'W');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 3);
    EXPECT_EQ(c.column, 1);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), 'u');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 3);
    EXPECT_EQ(c.column, 2);
    lr.adv();
    EXPECT_TRUE(lr.get(c));
    EXPECT_EQ(c.ch(), '\n');
    EXPECT_EQ(c.text, "Wu"_s);
    EXPECT_EQ(c.filename, "<stringchar5>"_s);
    EXPECT_EQ(c.line, 3);
    EXPECT_EQ(c.column, 3);
    lr.adv();
    EXPECT_FALSE(lr.get(c));
}

TEST(io, linespan)
{
    io::LineCharReader lr("<span>"_s, string_pipe("Hello,\nWorld!\n"_s));
    io::LineSpan span;
    do
    {
        lr.get(span.begin);
        lr.adv();
    }
    while (span.begin.ch() != 'e');
    do
    {
        lr.get(span.end);
        lr.adv();
    }
    while (span.end.ch() != 'o');
    EXPECT_EQ(span.message_str("info"_s, "meh"_s),
            "<span>:1:2: info: meh\n"
            "Hello,\n"
            " ^~~~\n"_s
    );
    span.begin = span.end;
    do
    {
        lr.get(span.end);
        lr.adv();
    }
    while (span.end.ch() != 'r');

    EXPECT_EQ(span.begin.note_str("foo"_s),
            "<span>:1:5: note: foo\n"
            "Hello,\n"
            "    ^\n"_s
    );
    EXPECT_EQ(span.end.warning_str("bar"_s),
            "<span>:2:3: warning: bar\n"
            "World!\n"
            "  ^\n"_s
    );
    EXPECT_EQ(span.error_str("qux"_s),
            "<span>:1:5: error: qux\n"
            "Hello,\n"
            "    ^~ ...\n"
            "World!\n"
            "~~~\n"_s
    );
}
} // namespace tmwa
