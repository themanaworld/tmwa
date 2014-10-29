#pragma once
//    io/span.hpp - Tracking info about input
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
#include "../strings/zstring.hpp"
#include "../strings/literal.hpp"


namespace tmwa
{
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
        AString note_str(ZString msg) const { return message_str("note"_s, msg); }
        AString warning_str(ZString msg) const { return message_str("warning"_s, msg); }
        AString error_str(ZString msg) const { return message_str("error"_s, msg); }
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
        AString note_str(ZString msg) const { return message_str("note"_s, msg); }
        AString warning_str(ZString msg) const { return message_str("warning"_s, msg); }
        AString error_str(ZString msg) const { return message_str("error"_s, msg); }
        void message(ZString cat, ZString msg) const;
        void note(ZString msg) const { message("note"_s, msg); }
        void warning(ZString msg) const { message("warning"_s, msg); }
        void error(ZString msg) const { message("error"_s, msg); }
    };

    template<class T>
    struct Spanned
    {
        T data;
        LineSpan span;
    };

    template<class T>
    Spanned<T> respan(LineSpan span, T data)
    {
        return Spanned<T>{std::move(data), std::move(span)};
    }
} // namespace io
} // namespace tmwa
