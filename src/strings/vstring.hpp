#ifndef TMWA_STRINGS_VSTRING_HPP
#define TMWA_STRINGS_VSTRING_HPP
//    strings/vstring.hpp - A small string that stores its own value.
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
    template<uint8_t n>
    class VString : public _crtp_string<VString<n>, VString<n>, ZPair>
    {
        char _data[n];
        unsigned char _special;
    public:
        typedef typename _crtp_string<VString<n>, VString<n>, ZPair>::iterator iterator;
        VString(XString x);
        VString(FString f);
        VString(TString t);
        VString(SString s);
        VString(ZString z);
        template<uint8_t m>
        VString(VString<m> v);
        template<size_t m>
        VString(char (&s)[m]) = delete;
        template<size_t m>
        VString(const char (&s)[m]);
        VString(decltype(really_construct_from_a_pointer) e, const char *s);
        VString(char c);
        VString();
        VString(XPair p);

        iterator begin() const;
        iterator end() const;
        const FString *base() const;
        const char *c_str() const;

        VString oslice_t(size_t o) const { return this->xslice_t(o); }
        VString oslice_h(size_t o) const { return this->xslice_h(o); }
        VString orslice_t(size_t no) const { return this->xrslice_t(no); }
        VString orslice_h(size_t no) const { return this->xrslice_h(no); }
        VString oislice_t(iterator it) const { return this->xislice_t(it); }
        VString oislice_h(iterator it) const { return this->xislice_h(it); }
        VString olslice(size_t o, size_t l) const { return this->xlslice(o, l); }
        VString opslice(size_t b, size_t e) const { return this->xpslice(b, e); }
        VString oislice(iterator b, iterator e) const { return this->xislice(b, e); }
    };

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    template<uint8_t n>
    const char *decay_for_printf(const VString<n>& vs);

    template<uint8_t len>
    __attribute__((format(printf, 2, 0)))
    int do_vprint(VString<len>& out, const char *fmt, va_list ap);
} // namespace strings

#include "vstring.tcc"

#endif // TMWA_STRINGS_VSTRING_HPP
