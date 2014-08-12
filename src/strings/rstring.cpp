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


namespace tmwa
{
namespace strings
{
    RString::RString()
    : u{.begin= ""}, maybe_end(u.begin)
    {
    }
    RString::RString(LString l)
    : u{.begin= &*l.begin()}, maybe_end(&*l.end())
    {
    }

    RString::RString(const RString& r)
    : u(r.u), maybe_end(r.maybe_end)
    {
        if (!maybe_end)
            u.owned->count++;
    }
    RString::RString(RString&& r)
    : RString()
    {
        *this = std::move(r);
    }
    RString& RString::operator = (const RString& r)
    {
        // order important for self-assign
        if (!r.maybe_end)
            r.u.owned->count++;
        if (!maybe_end && !u.owned->count--)
            ::operator delete(u.owned);
        u = r.u;
        maybe_end = r.maybe_end;
        return *this;
    }
    RString& RString::operator = (RString&& r)
    {
        std::swap(u, r.u);
        std::swap(maybe_end, r.maybe_end);
        return *this;
    }

    RString::RString(AString a)
    : RString()
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
        if (!maybe_end && !u.owned->count--)
            ::operator delete(u.owned);
    }

    RString::RString(const MString& s)
    : RString(s.begin(), s.end())
    {
    }

    RString::RString(XPair p)
    : RString(p.begin(), p.end())
    {
    }

    RString::RString(const TString& t)
    : RString(XString(t))
    {
    }
    RString::RString(const SString& s)
    : RString(XString(s))
    {
    }
    RString::RString(ZString z)
    : RString(XString(z))
    {
    }
    RString::RString(XString x)
    : RString()
    {
        // long term this stuff will change again
        const RString *f = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *fb = f ? &*f->begin() : nullptr;
        const char *fe = f ? &*f->end() : nullptr;
        if (f && xb == fb && xe == fe)
            *this = *f;
        else
            *this = RString(x.begin(), x.end());
    }

    RString::iterator RString::begin() const
    {
        if (maybe_end)
            return u.begin;
        return u.owned->body;
    }
    RString::iterator RString::end() const
    {
        if (maybe_end)
            return maybe_end;
        return u.owned->body + u.owned->size;
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
} // namespace tmwa
