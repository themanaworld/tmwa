#include "span.hpp"
//    io/span.cpp - Tracking info about input
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

#include "cxxstdio.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace io
{
    io::LineSpan Line::to_span() const
    {
        io::LineSpan rv;
        rv.begin.text = this->text;
        rv.begin.filename = this->filename;
        rv.begin.line = this->line;
        rv.begin.column = 1;
        rv.end.text = this->text;
        rv.end.filename = this->filename;
        rv.end.line = this->line;
        rv.end.column = this->text.size();
        return rv;
    }

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
} // namespace io
} // namespace tmwa
