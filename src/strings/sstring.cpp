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

#include "tstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"

namespace strings
{
    SString::SString()
    : _s(), _b(), _e()
    {}
    SString::SString(FString f)
    : _s(std::move(f)), _b(), _e(_s.size())
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
        const FString *f = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *fb = f ? &*f->begin() : nullptr;
        //const char *fe = f ? &*f->end() : nullptr;
        if (f)
            *this = SString(*f, xb - fb, xe - fb);
        else
            *this = FString(x);
    }

    SString::SString(FString f, size_t b, size_t e)
    : _s(std::move(f)), _b(b), _e(e)
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
    const FString *SString::base() const
    {
        return &_s;
    }
} // namespace strings
