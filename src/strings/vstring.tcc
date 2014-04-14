//    strings/vstring.tcc - Inline functions for vstring.hpp
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

#include <cassert>
#include <cstdio>

#include "../compat/cast.hpp"

#include "rstring.hpp"
#include "astring.hpp"
#include "tstring.hpp"
#include "sstring.hpp"
#include "zstring.hpp"
#include "xstring.hpp"

namespace strings
{
    template<uint8_t n>
    VString<n>::VString(XString x) : _data(), _special()
    {
        if (x.size() > n)
            // we're hoping this doesn't happen
            // hopefully there will be few enough users of this class
            x = x.xslice_h(n);
        char *e = std::copy(x.begin(), x.end(), std::begin(_data));
        _special = std::end(_data) - e;
        assert (_special == n - x.size()); // 0 when it needs to be
    }
    // poor man's delegated constructors
    // needed for gcc 4.6 compatibility
    template<uint8_t n>
    VString<n>::VString(RString f)
    {
        *this = XString(f);
    }
    template<uint8_t n>
    VString<n>::VString(AString a)
    {
        *this = XString(a);
    }
    template<uint8_t n>
    VString<n>::VString(TString t)
    {
        *this = XString(t);
    }
    template<uint8_t n>
    VString<n>::VString(SString s)
    {
        *this = XString(s);
    }
    template<uint8_t n>
    VString<n>::VString(ZString z)
    {
        *this = XString(z);
    }
    template<uint8_t n>
    template<uint8_t m>
    VString<n>::VString(VString<m> v)
    {
        static_assert(m < n, "can only grow");
        *this = XString(v);
    }
    template<uint8_t n>
    VString<n>::VString(LString l)
    {
        *this = XString(l);
    }
    template<uint8_t n>
    VString<n>::VString(decltype(really_construct_from_a_pointer) e, const char *s)
    {
        *this = XString(e, s, nullptr);
    }
    template<uint8_t n>
    VString<n>::VString(char c)
    {
        *this = XString(&c, &c + 1, nullptr);
    }
    template<uint8_t n>
    VString<n>::VString()
    {
        *this = XString();
    }
    template<uint8_t n>
    VString<n>::VString(XPair p)
    {
        *this = XString(p);
    }

    // hopefully this is obvious
    template<uint8_t n>
    typename VString<n>::iterator VString<n>::begin() const
    {
        return std::begin(_data);
    }
    template<uint8_t n>
    typename VString<n>::iterator VString<n>::end() const
    {
        return std::end(_data) - _special;
    }
    template<uint8_t n>
    const RString *VString<n>::base() const
    {
        return nullptr;
    }
    template<uint8_t n>
    const char *VString<n>::c_str() const
    {
        return &*begin();
    }

    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    template<uint8_t n>
    inline
    const char *decay_for_printf(const VString<n>& vs)
    {
        return vs.c_str();
    }

    template<uint8_t len>
    inline
    int do_vprint(VString<len>& out, const char *fmt, va_list ap)
    {
        char buffer[len + 1];
        vsnprintf(buffer, len + 1, fmt, ap);

        out = VString<len>(strings::really_construct_from_a_pointer, buffer);
        return len;
    }
} // namespace strings
