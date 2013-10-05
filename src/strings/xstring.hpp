#ifndef TMWA_STRINGS_XSTRING_HPP
#define TMWA_STRINGS_XSTRING_HPP
//    strings/xstring.hpp - A full borrowed slice.
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

#include "base.hpp"

namespace strings
{
    /// A non-owning string that is not guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class XString : public _crtp_string<XString, FString, XPair>
    {
        iterator _b, _e;
        // optional
        const FString *_base;
    public:
        // do I really want this?
        XString();
        XString(std::nullptr_t) = delete;
        // no MString
        XString(const FString& s);
        XString(const TString& s);
        XString(const SString& s);
        XString(const ZString& s);
        template<uint8_t n>
        XString(const VString<n>& s);
        template<size_t n>
        XString(char (&s)[n]) = delete;
        template<size_t n>
        XString(const char (&s)[n]);
        // mostly internal
        XString(const char *b, const char *e, const FString *base_);
        XString(decltype(really_construct_from_a_pointer) e, const char *s, const FString *base_);
        XString(XPair p);

        iterator begin() const;
        iterator end() const;
        const FString *base() const;
    };
} // namespace strings

#include "xstring.tcc"

#endif // TMWA_STRINGS_XSTRING_HPP
