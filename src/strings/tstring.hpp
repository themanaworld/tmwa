#ifndef TMWA_STRINGS_TSTRING_HPP
#define TMWA_STRINGS_TSTRING_HPP
//    strings/tstring.hpp - A tail slice of a string.
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


namespace tmwa
{
namespace strings
{
    /// An owning string that represents a tail slice of an RString.
    /// Guaranteed to be NUL-terminated.
    class TString : public _crtp_string<TString, TString, ZPair>
    {
        friend class SString;
        RString _s;
        size_t _o;
    public:
        TString();
        TString(RString b, size_t i=0);
        TString(AString b, size_t i=0);
        //TString(const TString&)
        TString(const SString&);
        TString(const ZString&);
        TString(const XString&);
        template<uint8_t n>
        TString(const VString<n>& v);
        TString(const LString&);
        //template<class It>
        //TString(It b, It e) : _s(b, e), _o(0) {}
        TString(XPair p);

        iterator begin() const;
        iterator end() const;
        const RString *base() const;
        const char *c_str() const;
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    const char *decay_for_printf(const TString& ts);
} // namespace strings
} // namespace tmwa

# include "tstring.tcc"

#endif // TMWA_STRINGS_TSTRING_HPP
