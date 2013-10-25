//    strings/fstring.tcc - Inline functions for fstring.hpp
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

namespace strings
{
    template<class It>
    void FString::_assign(It b, It e)
    {
        if (b == e)
        {
            // TODO use a special empty object
            // return;
        }
        if (!std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<It>::iterator_category>::value)
        {
            // can't use std::distance
            _hack2 = std::make_shared<std::vector<char>>();
            for (; b != e; ++b)
                _hack2->push_back(*b);
            _hack2->push_back('\0');
            _hack2->shrink_to_fit();
            return;
        }
        size_t diff = std::distance(b, e);
        _hack2 = std::make_shared<std::vector<char>>(diff + 1, '\0');
        std::copy(b, e, _hack2->begin());
    }

    template<size_t n>
    FString::FString(const char (&s)[n])
    {
        _assign(s, s + strlen(s));
    }

    template<class It>
    FString::FString(It b, It e)
    {
        _assign(b, e);
    }

    template<uint8_t n>
    FString::FString(const VString<n>& v)
    {
        _assign(v.begin(), v.end());
    }
} // namespace strings
