#ifndef TMWA_STRINGS_SSTRING_HPP
#define TMWA_STRINGS_SSTRING_HPP
//    strings/sstring.hpp - A full slice of an RString.
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

# include "fwd.hpp"

# include "base.hpp"
# include "rstring.hpp"

namespace strings
{
    /// An owning string that represents a arbitrary slice of an RString.
    /// Not guaranteed to be NUL-terminated.
    class SString : public _crtp_string<SString, SString, XPair>
    {
        RString _s;
        size_t _b, _e;
    public:
        SString();
        SString(RString f);
        SString(AString f);
        SString(TString t);
        //SString(const SString&);
        SString(const ZString&);
        SString(const XString&);
        template<uint8_t n>
        SString(const VString<n>& v);
        SString(const LString&);
        //template<class It>
        //SString(It b, It e) : _s(b, e), _b(0), _e(_s.size()) {}
        SString(RString f, size_t b, size_t e);
        SString(AString f, size_t b, size_t e);
        SString(XPair p);

        iterator begin() const;
        iterator end() const;
        const RString *base() const;
    };
} // namespace strings

# include "sstring.tcc"

#endif // TMWA_STRINGS_SSTRING_HPP
