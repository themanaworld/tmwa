#ifndef TMWA_STRINGS_FSTRING_HPP
#define TMWA_STRINGS_FSTRING_HPP
//    strings/fstring.hpp - An owned, reference-counted immutable string.
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

#include <cstdarg>
#include <cstring>

#include <memory>
#include <vector>

#include "base.hpp"

namespace strings
{
    /// An owning string that has reached its final contents.
    /// The storage is NUL-terminated
    /// TODO reimplement without std::shared_ptr
    class FString : public _crtp_string<FString, FString, ZPair>
    {
        std::shared_ptr<std::vector<char>> _hack2;

        template<class It>
        void _assign(It b, It e);
    public:
        FString();

        explicit FString(const MString& s);

        template<size_t n>
        FString(char (&s)[n]) = delete;

        template<size_t n>
        FString(const char (&s)[n]);

        template<class It>
        FString(It b, It e);

        FString(XPair p);
        //FString(const FString&)
        FString(const TString&);
        FString(const SString&);
        FString(ZString);
        FString(XString);
        template<uint8_t n>
        FString(const VString<n>& v);

        iterator begin() const;
        iterator end() const;
        const FString *base() const;
        const char *c_str() const;

        TString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        TString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        TString oislice_t(iterator it) const;
        SString oislice_h(iterator it) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;
        SString oislice(iterator b, iterator e) const;
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    const char *decay_for_printf(const FString& fs);

    __attribute__((format(printf, 2, 0)))
    int do_vprint(FString& out, const char *fmt, va_list ap);

    class StringConverter
    {
        FString& out;
        char *mid;
    public:
        StringConverter(FString& s);
        ~StringConverter();
        char **operator &();
    };

    StringConverter convert_for_scanf(FString& s);
} // namespace strings

#include "fstring.tcc"

#endif // TMWA_STRINGS_FSTRING_HPP
