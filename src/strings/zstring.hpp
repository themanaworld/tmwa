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

#include <cstring>

#include "base.hpp"

namespace strings
{
    /// A non-owning string that is guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class ZString : public _crtp_string<ZString, FString, ZPair>
    {
        iterator _b, _e;
        // optional
        const FString *_base;
    public:
        ZString();
        // no MString
        ZString(const FString& s);
        ZString(const TString& s);
        ZString(const SString&) = delete;
        //ZString(ZString);
        ZString(const XString&) = delete;
        template<uint8_t n>
        ZString(const VString<n>& s);
        // dangerous
        ZString(const char *b, const char *e, const FString *base_);
        ZString(decltype(really_construct_from_a_pointer), const char *s, const FString *base_);
        template<size_t n>
        ZString(char (&s)[n]) = delete;
        template<size_t n>
        ZString(const char (&s)[n], const FString *base_=nullptr);

        iterator begin() const;
        iterator end() const;
        const FString *base() const;
        const char *c_str() const;

        ZString oslice_t(size_t o) const;
        XString oslice_h(size_t o) const;
        ZString orslice_t(size_t no) const;
        XString orslice_h(size_t no) const;
        ZString oislice_t(iterator it) const;
        XString oislice_h(iterator it) const;
        XString olslice(size_t o, size_t l) const;
        XString opslice(size_t b, size_t e) const;
        XString oislice(iterator b, iterator e) const;
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    const char *decay_for_printf(const ZString& zs);

    __attribute__((format(scanf, 2, 0)))
    int do_vscan(ZString in, const char *fmt, va_list ap);
} // namespace strings

#include "zstring.tcc"

#endif // TMWA_STRINGS_ZSTRING_HPP
