//    strings/astring.tcc - Inline functions for astring.hpp
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

#include "mstring.hpp"

namespace strings
{
    template<class It>
    AString::AString(It b, It e)
    : data{}, special()
    {
        if (!std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value)
        {
            // can't use std::distance
            MString m;
            for (; b != e; ++b)
                m += *b;
            *this = AString(m); // will recurse
            return;
        }
        auto d = std::distance(b, e);
        if (d > 255 || d == 0)
        {
            new(r_ptr()) RString(b, e);
            special = 255;
        }
        else
        {
            *std::copy(b, e, data) = '\0';
            special = 255 - d;
        }
    }

    template<uint8_t n>
    AString::AString(const VString<n>& v)
    : data{}, special()
    {
        *std::copy(v.begin(), v.end(), data) = '\0';
        special = 255 - v.size();
        if (!v)
            new(r_ptr()) RString();
    }
} // namespace strings
