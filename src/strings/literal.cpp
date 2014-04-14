#include "literal.hpp"
//    strings/literal.cpp - A string stored in the readonly data segment.
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

#include <cstdio>

#include "../poison.hpp"

namespace strings
{
    LString::LString(const char *b, const char *e)
    : _b(b), _e(e)
    {}

    LString::iterator LString::begin() const
    {
        return _b;
    }
    LString::iterator LString::end() const
    {
        return _e;
    }
    const RString *LString::base() const
    {
        return nullptr;
    }
    const char *LString::c_str() const
    {
        return &*begin();
    }

    const char *decay_for_printf(const LString& zs)
    {
        return zs.c_str();
    }

    __attribute__((format(scanf, 2, 0)))
    int do_vscan(LString in, const char *fmt, va_list ap)
    {
        return vsscanf(in.c_str(), fmt, ap);
    }
} // namespace strings
