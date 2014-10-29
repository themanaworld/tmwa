#pragma once
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

#include "fwd.hpp"

#include "../strings/rstring.hpp"

#include "read.hpp"
#include "span.hpp"


namespace tmwa
{
namespace io
{
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
        LineReader(read_file_from_string, ZString name, XString content, int startline=1, FD fd=FD());
        LineReader(read_file_from_string, ZString name, LString content, int startline=1, FD fd=FD());

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
        LineCharReader(read_file_from_string, ZString name, XString content, int startline=1, int startcol=1, FD fd=FD());
        LineCharReader(read_file_from_string, ZString name, LString content, int startline=1, int startcol=1, FD fd=FD());

        bool get(LineChar& c);
        void adv();
        bool ok();
        bool is_open();
    };
} // namespace io
} // namespace tmwa
