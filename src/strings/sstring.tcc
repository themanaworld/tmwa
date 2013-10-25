//    strings/sstring.tcc - Inline functions for fstring.hpp
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

#include "vstring.hpp"

namespace strings
{
    template<uint8_t n>
    SString::SString(const VString<n>& v)
    : _s(v), _b(0), _e(_s.size())
    {}

    template<size_t n>
    SString::SString(const char (&s)[n])
    : _s(s), _b(0), _e(_s.size())
    {}
} // namespace strings
