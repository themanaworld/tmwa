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

#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"

namespace strings
{
    TString::TString()
    : _s(), _o()
    {}
    TString::TString(FString b, size_t i)
    : _s(std::move(b)), _o(i)
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
        const FString *f = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *fb = f ? &*f->begin() : nullptr;
        const char *fe = f ? &*f->end() : nullptr;
        if (f && xe == fe)
            *this = TString(*f, xb - fb);
        else
            *this = FString(x);
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
    const FString *TString::base() const
    {
        return &_s;
    }
    const char *TString::c_str() const
    {
        return &*begin();
    }

    TS TS::oslice_t(size_t o) const
    { return TS(_s, _o + o); }
    SS TS::oslice_h(size_t o) const
    { return SS(_s, _o, _o + o); }
    TS TS::orslice_t(size_t no) const
    { return TS(_s, _s.size() - no); }
    SS TS::orslice_h(size_t no) const
    { return SS(_s, _o, _s.size() - no); }
    TS TS::oislice_t(iterator it) const
    { return TS(_s, _o + it - begin()); }
    SS TS::oislice_h(iterator it) const
    { return SS(_s, _o, _o + it - begin()); }
    SS TS::olslice(size_t o, size_t l) const
    { return SS(_s, _o + o, _o + o + l); }
    SS TS::opslice(size_t b, size_t e) const
    { return SS(_s, _o + b, _o + e); }
    SS TS::oislice(iterator b, iterator e) const
    { return SS(_s, _o + b - begin(), _o + e - begin()); }

    const char *decay_for_printf(const TString& ts)
    {
        return ts.c_str();
    }
} // namespace strings
