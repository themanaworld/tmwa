#include "rstring.hpp"
//    strings/rstring.cpp - Functions for rstring.hpp
//
//    Copyright © 2013-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "mstring.hpp"
#include "astring.hpp"
#include "tstring.hpp"
#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"
#include "literal.hpp"

// doing sneaky tricks here
//#include "../poison.hpp"

namespace strings
{
    static_assert(sizeof(RString) == sizeof(const char *), "RString");

    uint8_t RString::empty_string_rep[sizeof(Rep) + 1];

    RString::RString()
    : owned(reinterpret_cast<Rep *>(&empty_string_rep))
    {
        owned->count++;
    }

    RString::RString(const RString& r)
    : owned(r.owned)
    {
        owned->count++;
    }
    RString::RString(RString&& r)
    : owned(reinterpret_cast<Rep *>(&empty_string_rep))
    {
        std::swap(owned, r.owned);
        r.owned->count++;
    }
    RString& RString::operator = (const RString& r)
    {
        // order important for self-assign
        r.owned->count++;
        // owned can be nullptr from ctors
        // TODO do ctors *properly* (requires gcc 4.7 to stay sane)
        if (owned && !owned->count--)
            ::operator delete(owned);
        owned = r.owned;
        return *this;
    }
    RString& RString::operator = (RString&& r)
    {
        std::swap(owned, r.owned);
        return *this;
    }
    RString::RString(AString a)
    : owned(nullptr)
    {
        if (RString *r = const_cast<RString *>(a.base()))
        {
            *this = std::move(*r);
        }
        else
        {
            *this = XPair(a);
        }
    }
    RString::~RString()
    {
        if (owned && !owned->count--)
            ::operator delete(owned);
        owned = nullptr;
    }

    RString::RString(const MString& s)
    : owned(nullptr)
    {
        _assign(s.begin(), s.end());
    }

    RString::RString(XPair p)
    : owned(nullptr)
    {
        _assign(p.begin(), p.end());
    }

    RString::RString(const TString& t)
    : owned(nullptr)
    {
        *this = XString(t);
    }
    RString::RString(const SString& s)
    : owned(nullptr)
    {
        *this = XString(s);
    }
    RString::RString(ZString z)
    : owned(nullptr)
    {
        *this = XString(z);
    }
    RString::RString(XString x)
    : owned(nullptr)
    {
        const RString *f = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *fb = f ? &*f->begin() : nullptr;
        const char *fe = f ? &*f->end() : nullptr;
        if (f && xb == fb && xe == fe)
            *this = *f;
        else
            _assign(x.begin(), x.end());
    }
    RString::RString(LString l)
    : owned(nullptr)
    {
        *this = XString(l);
    }

    RString::iterator RString::begin() const
    {
        return owned->body;
    }
    RString::iterator RString::end() const
    {
        return owned->body + owned->size;
    }
    const RString *RString::base() const
    {
        return this;
    }
    const char *RString::c_str() const
    {
        return &*begin();
    }

    const char *decay_for_printf(const RString& fs)
    {
        return fs.c_str();
    }

    int do_vprint(RString& out, const char *fmt, va_list ap)
    {
        int len;
        {
            va_list ap2;
            va_copy(ap2, ap);
            len = vsnprintf(nullptr, 0, fmt, ap2);
            va_end(ap2);
        }
        char buffer[len + 1];
        vsnprintf(buffer, len + 1, fmt, ap);

        out = RString(buffer, buffer + len);
        return len;
    }
} // namespace strings
