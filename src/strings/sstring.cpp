#include "sstring.hpp"
//    strings/sstring.cpp - Functions for sstring.hpp
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
#include "zstring.hpp"
#include "xstring.hpp"
#include "literal.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace strings
{
    SString::SString()
    : _s(), _b(), _e()
    {}
    SString::SString(RString r)
    : _s(std::move(r)), _b(), _e(_s.size())
    {}
    SString::SString(AString a)
    : _s(std::move(a)), _b(), _e(_s.size())
    {}
    SString::SString(TString t)
    : _s(t._s), _b(0), _e(_s.size())
    {}
    SString::SString(const ZString& z)
    {
        *this = XString(z);
    }
    SString::SString(const XString& x)
    {
        const RString *r = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *rb = r ? &*r->begin() : nullptr;
        //const char *re = r ? &*r->end() : nullptr;
        if (r)
            *this = SString(*r, xb - rb, xe - rb);
        else
            *this = RString(x);
    }
    SString::SString(const LString& l)
    {
        *this = XString(l);
    }

    SString::SString(RString r, size_t b, size_t e)
    : _s(std::move(r)), _b(b), _e(e)
    {}
    static
    RString get_owned_slice(AString a, size_t *b, size_t *e)
    {
        if (a.base())
        {
            // it's futile
            return std::move(a);
        }
        // have to allocate anyway, so cut first
        size_t ob = *b;
        size_t oe = *e;
        *e -= *b;
        *b = 0;
        return a.xpslice(ob, oe);
    }
    SString::SString(AString a, size_t b, size_t e)
    : _s(get_owned_slice(std::move(a), &b, &e))
    , _b(b)
    , _e(e)
    {}
    SString::SString(XPair p)
    : _s(p), _b(0), _e(p.size())
    {}

    SString::iterator SString::begin() const
    {
        return &_s.begin()[_b];
    }
    SString::iterator SString::end() const
    {
        return &_s.begin()[_e];
    }
    const RString *SString::base() const
    {
        return &_s;
    }
} // namespace strings
} // namespace tmwa
