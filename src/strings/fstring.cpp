#include "fstring.hpp"
//    strings/fstring.cpp - Functions for fstring.hpp
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

#include "mstring.hpp"
#include "tstring.hpp"
#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"
#include "vstring.hpp"

namespace strings
{
    uint8_t FString::empty_string_rep[sizeof(Rep) + 1];

    FString::FString()
    : owned(reinterpret_cast<Rep *>(&empty_string_rep))
    {
        owned->count++;
    }

    FString::FString(const FString& r)
    : owned(r.owned)
    {
        owned->count++;
    }
    FString::FString(FString&& r)
    : owned(reinterpret_cast<Rep *>(&empty_string_rep))
    {
        std::swap(owned, r.owned);
        r.owned->count++;
    }
    FString& FString::operator = (const FString& r)
    {
        // order important for self-assign
        r.owned->count++;
        // owned can be NULL from ctors
        // TODO do ctors *properly* (requires gcc 4.7 to stay sane)
        if (owned && !owned->count--)
            ::operator delete(owned);
        owned = r.owned;
        return *this;
    }
    FString& FString::operator = (FString&& r)
    {
        std::swap(owned, r.owned);
        return *this;
    }
    FString::~FString()
    {
        if (owned && !owned->count--)
            ::operator delete(owned);
        owned = nullptr;
    }

    FString::FString(const MString& s)
    : owned(nullptr)
    {
        _assign(s.begin(), s.end());
    }

    FString::FString(XPair p)
    : owned(nullptr)
    {
        _assign(p.begin(), p.end());
    }

    FString::FString(const TString& t)
    : owned(nullptr)
    {
        *this = XString(t);
    }
    FString::FString(const SString& s)
    : owned(nullptr)
    {
        *this = XString(s);
    }
    FString::FString(ZString z)
    : owned(nullptr)
    {
        *this = XString(z);
    }
    FString::FString(XString x)
    : owned(nullptr)
    {
        const FString *f = x.base();
        const char *xb = &*x.begin();
        const char *xe = &*x.end();
        const char *fb = f ? &*f->begin() : nullptr;
        const char *fe = f ? &*f->end() : nullptr;
        if (f && xb == fb && xe == fe)
            *this = *f;
        else
            _assign(x.begin(), x.end());
    }

    FString::iterator FString::begin() const
    {
        return owned->body;
    }
    FString::iterator FString::end() const
    {
        return owned->body + owned->size;
    }
    const FString *FString::base() const
    {
        return this;
    }
    const char *FString::c_str() const
    {
        return &*begin();
    }

    const char *decay_for_printf(const FString& fs)
    {
        return fs.c_str();
    }

    int do_vprint(FString& out, const char *fmt, va_list ap)
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

        out = FString(buffer, buffer + len);
        return len;
    }

    StringConverter::StringConverter(FString& s)
    : out(s), mid(nullptr)
    {}

    StringConverter::~StringConverter()
    {
        if (mid)
        {
            out = ZString(really_construct_from_a_pointer, mid, nullptr);
            free(mid);
        }
    }

    char **StringConverter::operator &()
    {
        return &mid;
    }

    StringConverter convert_for_scanf(FString& s)
    {
        return StringConverter(s);
    }
} // namespace strings
