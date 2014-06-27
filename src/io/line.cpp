#include "line.hpp"
//    io/line.cpp - Input from files, line-by-line
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

#include "../strings/astring.hpp"
#include "../strings/mstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "cxxstdio.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace io
{
    AString Line::message_str(ZString cat, ZString msg) const
    {
        MString out;
        if (column)
            out += STRPRINTF("%s:%u:%u: %s: %s\n"_fmt,
                    filename, line, column, cat, msg);
        else
            out += STRPRINTF("%s:%u: %s: %s\n"_fmt,
                    filename, line, cat, msg);
        out += STRPRINTF("%s\n"_fmt, text);
        out += STRPRINTF("%*c\n"_fmt, column, '^');
        return AString(out);
    }

    void Line::message(ZString cat, ZString msg) const
    {
        if (column)
            FPRINTF(stderr, "%s:%u:%u: %s: %s\n"_fmt,
                    filename, line, column, cat, msg);
        else
            FPRINTF(stderr, "%s:%u: %s: %s\n"_fmt,
                    filename, line, cat, msg);
        FPRINTF(stderr, "%s\n"_fmt, text);
        FPRINTF(stderr, "%*c\n"_fmt, column, '^');
    }

    AString LineSpan::message_str(ZString cat, ZString msg) const
    {
        assert (begin.column);
        assert (end.column);
        assert (begin.line < end.line || begin.column <= end.column);

        MString out;
        if (begin.line == end.line)
        {
            out += STRPRINTF("%s:%u:%u: %s: %s\n"_fmt,
                    begin.filename, begin.line, begin.column, cat, msg);
            out += STRPRINTF("%s\n"_fmt, begin.text);
            out += STRPRINTF("%*c"_fmt, begin.column, '^');
            for (unsigned c = begin.column; c != end.column; ++c)
                out += '~';
            out += '\n';
        }
        else
        {
            out += STRPRINTF("%s:%u:%u: %s: %s\n"_fmt,
                    begin.filename, begin.line, begin.column, cat, msg);
            out += STRPRINTF("%s\n"_fmt, begin.text);
            out += STRPRINTF("%*c"_fmt, begin.column, '^');
            for (unsigned c = begin.column; c != begin.text.size(); ++c)
                out += '~';
            out += " ...\n"_s;
            out += STRPRINTF("%s\n"_fmt, end.text);
            for (unsigned c = 0; c != end.column; ++c)
                out += '~';
            out += '\n';
        }
        return AString(out);
    }

    void LineSpan::message(ZString cat, ZString msg) const
    {
        FPRINTF(stderr, "%s"_fmt, message_str(cat, msg));
    }

    LineReader::LineReader(ZString name)
    : filename(name), line(0), column(0), rf(name)
    {}

    LineReader::LineReader(ZString name, FD fd)
    : filename(name), line(0), column(0), rf(fd)
    {}

    bool LineReader::read_line(Line& l)
    {
        AString text;
        if (rf.getline(text))
        {
            l.text = text;
            l.filename = filename;
            l.line = ++line;
            l.column = 0; // whole line
            return true;
        }
        return false;
    }

    bool LineReader::is_open()
    {
        return rf.is_open();
    }

    LineCharReader::LineCharReader(ZString name)
    : LineReader(name)
    {
        column = 1; // not 0, not whole line
        if (rf.is_open())
            adv();
        if (!line)
            column = 0;
    }
    // sigh, copy-paste
    // in just a couple months I can drop support for gcc 4.6 though
    LineCharReader::LineCharReader(ZString name, FD fd)
    : LineReader(name, fd)
    {
        column = 1; // not 0, not whole line
        if (rf.is_open())
            adv();
        if (!line)
            column = 0;
    }

    bool LineCharReader::get(LineChar& c)
    {
        if (!column)
            return false;

        c.text = line_text;
        c.filename = filename;
        c.line = line;
        c.column = column;
        return true;
    }

    void LineCharReader::adv()
    {
        if (column - 1 == line_text.size())
        {
            Line tmp;
            if (!read_line(tmp))
            {
                // eof
                column = 0;
                return;
            }
            line_text = tmp.text;
            column = 1;
        }
        else
            column++;
    }

    bool LineCharReader::is_open()
    {
        return line != 0;
    }
} // namespace io
} // namespace tmwa
