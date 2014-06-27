#include "tstring.hpp"
//    strings/tstring.cpp - Functions for tstring.hpp
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
#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"
#include "literal.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace strings
{
    TString::TString()
    : _s(), _o()
    {}
    TString::TString(RString b, size_t i)
    : _s(std::move(b)), _o(i)
    {}
    static
    RString get_owned_tlice(AString a, size_t *i)
    {
        if (a.base())
        {
            return std::move(a);
        }
        size_t oi = *i;
        *i = 0;
        return a.xslice_t(oi);
    }
    TString::TString(AString b, size_t i)
    : _s(get_owned_tlice(std::move(b), &i))
    , _o(i)
    {}
    TString::TString(const SString& s)
    {
        *this = XString(s);
    }
    TString::TString(const ZString& z)
    {
        *this = XString(z);
    }
    TString::TString(const XString& x)
    {
        const RString *r = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *rb = r ? &*r->begin() : nullptr;
        const char *re = r ? &*r->end() : nullptr;
        if (r && xe == re)
            *this = TString(*r, xb - rb);
        else
            *this = RString(x);
    }
    TString::TString(const LString& l)
    {
        *this = XString(l);
    }

    TString::TString(XPair p)
    : _s(p), _o(0)
    {}

    TString::iterator TString::begin() const
    {
        return &_s.begin()[_o];
    }
    TString::iterator TString::end() const
    {
        return &*_s.end();
    }
    const RString *TString::base() const
    {
        return &_s;
    }
    const char *TString::c_str() const
    {
        return &*begin();
    }

    const char *decay_for_printf(const TString& ts)
    {
        return ts.c_str();
    }
} // namespace strings
} // namespace tmwa
