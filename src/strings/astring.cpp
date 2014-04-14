#include "astring.hpp"
//    strings/astring.cpp - Functions for astring.hpp
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

#include <cstdlib>

#include "mstring.hpp"
#include "tstring.hpp"
#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"
#include "vstring.hpp"

//#include "../poison.hpp"

namespace strings
{
    static_assert(sizeof(AString) == 256, "AString");

    static
    void acpy(char (&dst)[255], const char (&src)[255])
    {
        std::copy(src + 0, src + 255, dst);
    }

    // TODO dedup all this code once I drop gcc 4.6 support
    AString::AString()
    : data{}, special()
    {
        new(r_ptr()) RString();
        special = 255;
    }

    AString::AString(const AString& a)
    : data(), special(a.special)
    {
        acpy(data, a.data);
        if (special == 255)
            new(r_ptr()) RString(*a.r_ptr());
    }
    AString::AString(AString&& a)
    : data(), special(a.special)
    {
        acpy(data, a.data);
        if (special == 255)
            new(r_ptr()) RString(std::move(*a.r_ptr()));
    }
    AString& AString::operator = (const AString& a)
    {
        if (this == &a)
            return *this;
        if (special == 255)
            r_ptr()->~RString();
        acpy(data, a.data);
        special = a.special;
        if (special == 255)
            new(r_ptr()) RString(*a.r_ptr());
        return *this;
    }
    AString& AString::operator = (AString&& a)
    {
        std::swap(data, a.data);
        std::swap(special, a.special);
        return *this;
    }
    AString::AString(RString r)
    : data{}, special()
    {
        new(r_ptr()) RString(std::move(r));
        special = 255;
    }
    AString::~AString()
    {
        if (special == 255)
            r_ptr()->~RString();
    }

    AString::AString(const MString& s)
    : data{}, special()
    {
        if (s.size() > 255 || s.size() == 0)
        {
            new(r_ptr()) RString(s);
            special = 255;
        }
        else
        {
            *std::copy(s.begin(), s.end(), data) = '\0';
            special = 255 - s.size();
        }
    }

    AString::AString(XPair p)
    : data{}, special()
    {
        new(r_ptr()) RString();
        special = 255;
        *this = XString(p);
    }

    AString::AString(const TString& t)
    : data{}, special()
    {
        new(r_ptr()) RString();
        special = 255;
        *this = XString(t);
    }
    AString::AString(const SString& s)
    : data{}, special()
    {
        new(r_ptr()) RString();
        special = 255;
        *this = XString(s);
    }
    AString::AString(ZString z)
    : data{}, special()
    {
        new(r_ptr()) RString();
        special = 255;
        *this = XString(z);
    }
    AString::AString(XString x)
    : data{}, special()
    {
        if (const RString *r = x.base())
        {
            if (&*r->begin() == &*x.begin() && &*r->end() == &*x.end())
            {
                new(r_ptr()) RString(*r);
                special = 255;
                return;
            }
        }
        if (x.size() > 255 || x.size() == 0)
        {
            new(r_ptr()) RString(x);
            special = 255;
        }
        else
        {
            *std::copy(x.begin(), x.end(), data) = '\0';
            special = 255 - x.size();
        }
    }
    AString::AString(LString l)
    : data{}, special()
    {
        new(r_ptr()) RString();
        special = 255;
        *this = XString(l);
    }

    AString::iterator AString::begin() const
    {
        if (special == 255)
            return &*r_ptr()->begin();
        else
            return data + 0;
    }
    AString::iterator AString::end() const
    {
        if (special == 255)
            return &*r_ptr()->end();
        else
            return data + (255 - special);
    }
    const RString *AString::base() const
    {
        if (special == 255)
            return r_ptr();
        else
            return nullptr;
    }
    const char *AString::c_str() const
    {
        return &*begin();
    }

    const char *decay_for_printf(const AString& as)
    {
        return as.c_str();
    }

    int do_vprint(AString& out, const char *fmt, va_list ap)
    {
        // TODO try with a fixed-size buffer first, then write
        // directory into the allocated output?
        int len;
        {
            va_list ap2;
            va_copy(ap2, ap);
            len = vsnprintf(nullptr, 0, fmt, ap2);
            va_end(ap2);
        }
        char buffer[len + 1];
        vsnprintf(buffer, len + 1, fmt, ap);

        out = AString(buffer, buffer + len);
        return len;
    }

    AStringConverter::AStringConverter(AString& s)
    : out(s), mid(nullptr)
    {}

    AStringConverter::~AStringConverter()
    {
        if (mid)
        {
            out = ZString(really_construct_from_a_pointer, mid, nullptr);
            free(mid);
        }
    }

    char **AStringConverter::operator &()
    {
        return &mid;
    }

    AStringConverter convert_for_scanf(AString& s)
    {
        return AStringConverter(s);
    }
} // namespace strings
