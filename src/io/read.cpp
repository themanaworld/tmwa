#include "read.hpp"
//    io/read.cpp - Input from files
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

#include <fcntl.h>

#include "../strings/astring.hpp"
#include "../strings/mstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace io
{
    ReadFile::ReadFile(FD f)
    : fd(f), start(0), end(0)
    {
    }
    ReadFile::ReadFile(ZString name)
    : fd(FD::open(name, O_RDONLY | O_CLOEXEC)), start(0), end(0)
    {
    }
    ReadFile::ReadFile(const DirFd& dir, ZString name)
    : fd(dir.open_fd(name, O_RDONLY | O_CLOEXEC)), start(0), end(0)
    {
    }
    ReadFile::~ReadFile()
    {
        fd.close();
        fd = FD();
    }

    bool ReadFile::get(char& c)
    {
        if (start == end)
        {
            if (fd == FD())
                return false;
            ssize_t rv = fd.read(&buf, sizeof(buf));
            if (rv == 0 || rv == -1)
            {
                fd.close();
                fd = FD();
                return false;
            }
            start = 0;
            end = rv;
        }
        c = buf[start++];
        return true;
    }
    size_t ReadFile::get(char *out, size_t len)
    {
        for (size_t i = 0; i < len; ++i)
        {
            if (!get(out[i]))
                return i;
        }
        return len;
    }
    bool ReadFile::getline(AString& line)
    {
        MString tmp;
        char c;
        bool anything = false;
        bool happy = false;
        bool unhappy = false;
        while (get(c))
        {
            anything = true;
            if (c == '\r')
            {
                unhappy = true;
                if (!get(c))
                    break;
                if (c != '\n')
                    --start;
                else
                    happy = true;
                break;
            }
            if (c == '\n')
            {
                happy = true;
                break;
            }
            tmp += c;
        }
        if (unhappy)
        {
            if (happy)
                FPRINTF(stderr, "warning: file contains CR\n"_fmt);
            else
                FPRINTF(stderr, "warning: file contains bare CR\n"_fmt);
        }
        else if (!happy && anything)
            FPRINTF(stderr, "warning: file does not contain a trailing newline\n"_fmt);
        line = AString(tmp);
        return anything;
    }

    bool ReadFile::is_open()
    {
        return fd != FD();
    }
} // namespace io
} // namespace tmwa
