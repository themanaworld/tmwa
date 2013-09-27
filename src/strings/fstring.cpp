#include "fstring.hpp"
//    strings/fstring.cpp - Functions for fstring.hpp
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

#include "mstring.hpp"
#include "tstring.hpp"
#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"
#include "vstring.hpp"

namespace strings
{
    FString::FString()
    {
        const char *sadness = "";
        _assign(sadness, sadness);
    }

    FString::FString(const MString& s)
    {
        _assign(s.begin(), s.end());
    }

    FString::FString(XPair p)
    {
        _assign(p.begin(), p.end());
    }

    FString::FString(const TString& t)
    {
        *this = XString(t);
    }
    FString::FString(const SString& s)
    {
        *this = XString(s);
    }
    FString::FString(ZString z)
    {
        *this = XString(z);
    }
    FString::FString(XString x)
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
        return &_hack2->begin()[0];
    }
    FString::iterator FString::end() const
    {
        return &_hack2->end()[-1];
    }
    const FString *FString::base() const
    {
        return this;
    }
    const char *FString::c_str() const
    {
        return &*begin();
    }

    TS FS::oslice_t(size_t o) const
    { return TS(*this, o); }
    SS FS::oslice_h(size_t o) const
    { return SS(*this, 0, o); }
    TS FS::orslice_t(size_t no) const
    { return TS(*this, size() - no); }
    SS FS::orslice_h(size_t no) const
    { return SS(*this, 0, size() - no); }
    TS FS::oislice_t(iterator it) const
    { return TS(*this, it - begin()); }
    SS FS::oislice_h(iterator it) const
    { return SS(*this, 0, it - begin()); }
    SS FS::olslice(size_t o, size_t l) const
    { return SS(*this, o, o + l); }
    SS FS::opslice(size_t b, size_t e) const
    { return SS(*this, b, e); }
    SS FS::oislice(iterator b, iterator e) const
    { return SS(*this, b - begin(), e - begin()); }

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
