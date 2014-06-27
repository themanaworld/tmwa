#include "xstring.hpp"
//    strings/xstring.cpp - Functions for xstring.hpp
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

#include "rstring.hpp"
#include "astring.hpp"
#include "tstring.hpp"
#include "sstring.hpp"
#include "zstring.hpp"
#include "literal.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace strings
{
    XString::XString()
    : _b(""), _e(_b), _base()
    {}
    XString::XString(const RString& s)
    : _b(&*s.begin()), _e(&*s.end()), _base(s.base())
    {}
    XString::XString(const AString& s)
    : _b(&*s.begin()), _e(&*s.end()), _base(s.base())
    {}
    XString::XString(const TString& s)
    : _b(&*s.begin()), _e(&*s.end()), _base(s.base())
    {}
    XString::XString(const SString& s)
    : _b(&*s.begin()), _e(&*s.end()), _base(s.base())
    {}
    XString::XString(const ZString& s)
    : _b(&*s.begin()), _e(&*s.end()), _base(s.base())
    {}
    XString::XString(const LString& s)
    : _b(&*s.begin()), _e(&*s.end()), _base(s.base())
    {}

    XString::XString(const char *b, const char *e, const RString *base_)
    : _b(b), _e(e), _base(base_)
    {}
    XString::XString(decltype(really_construct_from_a_pointer) e, const char *s, const RString *base_)
    {
        *this = ZString(e, s, base_);
    }
    XString::XString(XPair p)
    : _b(p.begin()), _e(p.end()), _base(nullptr)
    {}

    XString::iterator XString::begin() const
    {
        return _b;
    }
    XString::iterator XString::end() const
    {
        return _e;
    }
    const RString *XString::base() const
    {
        return _base;
    }
} // namespace strings
} // namespace tmwa
