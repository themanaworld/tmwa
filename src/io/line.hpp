#ifndef TMWA_IO_LINE_HPP
#define TMWA_IO_LINE_HPP
//    io/line.hpp - Input from files, line-by-line
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

# include "fwd.hpp"

# include "../strings/rstring.hpp"
# include "../strings/astring.hpp"
# include "../strings/zstring.hpp"

# include "fd.hpp"
# include "read.hpp"


namespace io
{
    // TODO split this out
    struct Line
    {
        RString text;

        RString filename;
        // 1-based
        uint16_t line, column;

        AString message_str(ZString cat, ZString msg) const;
        void message(ZString cat, ZString msg) const;
        void note(ZString msg) const { message("note"_s, msg); }
        void warning(ZString msg) const { message("warning"_s, msg); }
        void error(ZString msg) const { message("error"_s, msg); }
    };

    // psst, don't tell anyone
    struct LineChar : Line
    {
        char ch()
        {
            size_t c = column - 1;
            if (c == text.size())
                return '\n';
            return text[c];
        }
    };

    struct LineSpan
    {
        LineChar begin, end;

        AString message_str(ZString cat, ZString msg) const;
        void message(ZString cat, ZString msg) const;
        void note(ZString msg) const { message("note"_s, msg); }
        void warning(ZString msg) const { message("warning"_s, msg); }
        void error(ZString msg) const { message("error"_s, msg); }
    };

    class LineReader
    {
    protected:
        RString filename;
        uint16_t line, column;
        ReadFile rf;
    public:
        explicit
        LineReader(ZString name);
        LineReader(LineReader&&) = delete;
        LineReader& operator = (LineReader&&) = delete;
        // needed for unit tests
        LineReader(ZString name, FD fd);

        bool read_line(Line& l);
        bool is_open();
    };

    class LineCharReader : private LineReader
    {
        RString line_text;
    public:
        explicit
        LineCharReader(ZString name);
        LineCharReader(LineCharReader&&) = delete;
        LineCharReader& operator = (LineCharReader&&) = delete;
        LineCharReader(ZString name, FD fd);

        bool get(LineChar& c);
        void adv();
        bool ok();
        bool is_open();
    };
} // namespace io

#endif // TMWA_IO_LINE_HPP
