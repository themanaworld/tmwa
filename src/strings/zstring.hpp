#ifndef TMWA_STRINGS_ZSTRING_HPP
#define TMWA_STRINGS_ZSTRING_HPP
//    strings/zstring.hpp - A borrowed tail slice of a string.
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

# include "../sanity.hpp"

# include <cstring>

# include "base.hpp"

namespace strings
{
    /// A non-owning string that is guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class ZString : public _crtp_string<ZString, AString, ZPair>
    {
        iterator _b, _e;
        // optional
        const RString *_base;
    public:
        ZString();
        // no MString
        ZString(const RString& s);
        ZString(const AString& s);
        ZString(const TString& s);
        ZString(const SString&) = delete;
        //ZString(ZString);
        ZString(const XString&) = delete;
        template<uint8_t n>
        ZString(const VString<n>& s);
        // dangerous
        ZString(const char *b, const char *e, const RString *base_);
        ZString(decltype(really_construct_from_a_pointer), const char *s, const RString *base_);
        template<size_t n>
        ZString(char (&s)[n]) = delete;
        template<size_t n>
        ZString(const char (&s)[n], const RString *base_=nullptr);

        iterator begin() const;
        iterator end() const;
        const RString *base() const;
        const char *c_str() const;
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    const char *decay_for_printf(const ZString& zs);

    __attribute__((format(scanf, 2, 0)))
    int do_vscan(ZString in, const char *fmt, va_list ap);
} // namespace strings

# include "zstring.tcc"

#endif // TMWA_STRINGS_ZSTRING_HPP
