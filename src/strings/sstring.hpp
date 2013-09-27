#ifndef TMWA_STRINGS_SSTRING_HPP
#define TMWA_STRINGS_SSTRING_HPP
//    strings/sstring.hpp - A full slice of an FString.
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
#include "fstring.hpp"

namespace strings
{
    /// An owning string that represents a arbitrary slice of an FString.
    /// Not guaranteed to be NUL-terminated.
    class SString : public _crtp_string<SString, SString, XPair>
    {
        FString _s;
        size_t _b, _e;
    public:
        SString();
        SString(FString f);
        SString(TString t);
        //SString(const SString&);
        SString(const ZString&);
        SString(const XString&);
        template<uint8_t n>
        SString(const VString<n>& v);
        template<size_t n>
        SString(char (&s)[n]) = delete;
        template<size_t n>
        SString(const char (&s)[n]);
        //template<class It>
        //SString(It b, It e) : _s(b, e), _b(0), _e(_s.size()) {}
        SString(FString f, size_t b, size_t e);
        SString(XPair p);

        iterator begin() const;
        iterator end() const;
        const FString *base() const;

        SString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        SString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        SString oislice_t(iterator it) const;
        SString oislice_h(iterator it) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;
        SString oislice(iterator b, iterator e) const;
    };
} // namespace strings

#include "sstring.tcc"

#endif // TMWA_STRINGS_SSTRING_HPP
