#ifndef TMWA_STRINGS_PAIR_HPP
#define TMWA_STRINGS_PAIR_HPP
//    strings/pair.hpp - Internal contiguous range.
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

#include "../sanity.hpp"

#include <cstring>

#include "fwd.hpp"

namespace strings
{
    // TODO instead typedef ranges::Contiguous<const char>
    // or whatever it becomes once it exists.
    // const_array is just a hack, as evidenced by the fact
    // that it's not really const.
    class XPair
    {
        const char *_begin;
        const char *_end;
    public:
        typedef XString TailSlice;
        typedef XString FullSlice;

        XPair(const char *b, const char *e)
        : _begin(b), _end(e)
        {}
        template<size_t n>
        XPair(char (&arr)[n]) = delete;
        template<size_t n>
        XPair(const char (&arr)[n])
        : _begin(arr), _end(arr + strlen(arr))
        {}

        const char *begin() const { return _begin; }
        const char *end() const { return _end; }
        size_t size() { return end() - begin(); }
    };
    struct ZPair : XPair
    {
        typedef ZString TailSlice;
        typedef XString FullSlice;

        ZPair(const char *b, const char *e)
        : XPair(b, e)
        {}
        template<size_t n>
        ZPair(char (&arr)[n]) = delete;
        template<size_t n>
        ZPair(const char (&arr)[n])
        : XPair(arr)
        {}
    };
} // namespace strings

#endif // TMWA_STRINGS_PAIR_HPP
