#pragma once
//    strings/rstring.hpp - An owned, reference-counted immutable string.
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

#include "fwd.hpp"

#include <cstdarg>

#include "base.hpp"


namespace tmwa
{
namespace strings
{
    /// An owning string that has reached its final contents.
    /// The storage is NUL-terminated
    class RString : public _crtp_string<RString, RString, ZPair>
    {
        struct Rep
        {
            size_t count;
            size_t size;
            char body[];
        };

        union
        {
            Rep *owned;
            const char *begin;
        } u;
        const char *maybe_end;

    public:
        RString();
        RString(LString s);
        RString(const RString&);
        RString(RString&&);
        RString& operator = (const RString&);
        RString& operator = (RString&&);
        RString(AString);
        ~RString();

        explicit RString(const MString& s);

        template<class It>
        RString(It b, It e);

        RString(XPair p);
        //RString(const RString&)
        RString(const TString&);
        RString(const SString&);
        RString(ZString);
        RString(XString);
        template<uint8_t n>
        RString(const VString<n>& v);

        iterator begin() const;
        iterator end() const;
        const RString *base() const;
        const char *c_str() const;
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    const char *decay_for_printf(const RString& fs);

    __attribute__((format(printf, 2, 0)))
    int do_vprint(RString& out, const char *fmt, va_list ap);
} // namespace strings
} // namespace tmwa

#include "rstring.tcc"
