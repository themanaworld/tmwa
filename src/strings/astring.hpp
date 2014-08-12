#pragma once
//    strings/astring.hpp - An owned, reference-counted immutable string.
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

#include "base.hpp"
#include "rstring.hpp"


namespace tmwa
{
namespace strings
{
    /// An owning string that has reached its final contents.
    /// The storage is NUL-terminated
    class AString : public _crtp_string<AString, AString, ZPair>
    {
#ifdef __clang__
        __attribute__((unused))
#endif
        RString align[0];
        char data[255];
        unsigned char special;
        RString *r_ptr() { return reinterpret_cast<RString *>(data); }
        const RString *r_ptr() const { return reinterpret_cast<const RString *>(data); }
    public:
        AString();
        AString(const AString&);
        AString(AString&&);
        AString& operator = (const AString&);
        AString& operator = (AString&&);
        AString(RString);
        ~AString();

        explicit AString(const MString& s);

        template<class It>
        AString(It b, It e);

        AString(XPair p);
        //AString(const AString&)
        AString(const TString&);
        AString(const SString&);
        AString(ZString);
        AString(XString);
        template<uint8_t n>
        AString(const VString<n>& v);
        AString(LString s);

        iterator begin() const;
        iterator end() const;
        const RString *base() const;
        const char *c_str() const;
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    const char *decay_for_printf(const AString& fs);

    __attribute__((format(printf, 2, 0)))
    int do_vprint(AString& out, const char *fmt, va_list ap);
} // namespace strings
} // namespace tmwa

#include "astring.tcc"
