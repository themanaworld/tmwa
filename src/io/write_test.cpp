#include "write.hpp"
//    io/write_test.cpp - Testsuite for output to files
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
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

#include <fcntl.h>
#include <unistd.h>

#include "../strings/astring.hpp"
#include "../strings/mstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

//#include "../poison.hpp"


namespace tmwa
{
static
io::FD pipew(io::FD& rfd)
{
    io::FD wfd;
    if (-1 == io::FD::pipe2(rfd, wfd, O_NONBLOCK))
    {
        rfd = io::FD();
        return io::FD();
    }
    return wfd;
}

class PipeWriter
{
private:
    io::FD rfd;
public:
    io::WriteFile wf;
public:
    PipeWriter(bool lb)
    : wf(pipew(rfd), lb)
    {}
    ~PipeWriter()
    {
        rfd.close();
    }
    AString slurp()
    {
        MString tmp;
        char buf[4096];
        while (true)
        {
            ssize_t rv = rfd.read(buf, sizeof(buf));
            if (rv == -1)
            {
                if (errno != EAGAIN)
                    return "Error, read failed :("_s;
                rv = 0;
            }
            if (rv == 0)
                break;
            tmp += XString(buf + 0, buf + rv, nullptr);
        }
        return AString(tmp);
    }
};

TEST(io, write1)
{
    PipeWriter pw(false);
    io::WriteFile& wf = pw.wf;
    wf.really_put("Hello, ", 7);
    EXPECT_EQ(""_s, pw.slurp());
    wf.put_line("World!\n"_s);
    EXPECT_EQ(""_s, pw.slurp());
    EXPECT_TRUE(wf.close());
    EXPECT_EQ("Hello, World!\n"_s, pw.slurp());
}

TEST(io, write2)
{
    PipeWriter pw(true);
    io::WriteFile& wf = pw.wf;
    wf.really_put("Hello, ", 7);
    EXPECT_EQ(""_s, pw.slurp());
    wf.put_line("World!"_s);
    wf.really_put("XXX", 3);
    EXPECT_EQ("Hello, World!\n"_s, pw.slurp());
    EXPECT_TRUE(wf.close());
    EXPECT_EQ("XXX"_s, pw.slurp());
}

TEST(io, write3)
{
    // TODO see if it's possible to get the real value
    constexpr size_t PIPE_CAPACITY = 65536;
    char buf[PIPE_CAPACITY];

    PipeWriter pw(false);
    io::WriteFile& wf = pw.wf;

    memset(buf, 'a', sizeof(buf));
    wf.really_put(buf, 1);
    EXPECT_EQ(""_s, pw.slurp());

    memset(buf, 'b', sizeof(buf));
    wf.really_put(buf, sizeof(buf));

    // write 1 + PIPE_CAPACITY
    // read 1 + N + (PIPE_CAPACITY - N)
    size_t remaining;
    {
        AString a = pw.slurp();
        XString x = a.xslice_t(1);
        EXPECT_EQ(a.front(), 'a');
        EXPECT_EQ(x.front(), 'b');
        EXPECT_EQ(x.back(), 'b');
        EXPECT_EQ(x, XString(buf, buf + x.size(), nullptr));
        remaining = sizeof(buf) - x.size();
    }

    EXPECT_TRUE(wf.close());
    EXPECT_EQ(pw.slurp(), XString(buf, buf + remaining, nullptr));
}
} // namespace tmwa
