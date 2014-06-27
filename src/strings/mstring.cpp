#include "mstring.hpp"
//    strings/mstring.cpp - Functions for mstring.hpp
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

#include "xstring.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace strings
{
    MString::iterator MString::begin()
    {
        return _hack.begin();
    }
    MString::iterator MString::end()
    {
        return _hack.end();
    }
    MString::const_iterator MString::begin() const
    {
        return _hack.begin();
    }
    MString::const_iterator MString::end() const
    {
        return _hack.end();
    }
    MString::reverse_iterator MString::rbegin()
    {
        return reverse_iterator(end());
    }
    MString::reverse_iterator MString::rend()
    {
        return reverse_iterator(begin());
    }
    MString::const_reverse_iterator MString::rbegin() const
    {
        return const_reverse_iterator(end());
    }
    MString::const_reverse_iterator MString::rend() const
    {
        return const_reverse_iterator(begin());
    }

    size_t MString::size() const
    {
        return _hack.size();
    }
    MString::operator bool() const
    {
        return size();
    }
    bool MString::operator !() const
    {
        return !size();
    }

    MString& MString::operator += (MString rhs)
    {
        _hack.insert(_hack.end(), rhs.begin(), rhs.end());
        return *this;
    }
    MString& MString::operator += (char c)
    {
        _hack.push_back(c);
        return *this;
    }
    MString& MString::operator += (XString x)
    {
        _hack.insert(_hack.end(), x.begin(), x.end());
        return *this;
    }

    void MString::pop_back(size_t n)
    {
        while (n--)
            _hack.pop_back();
    }
    char& MString::front()
    {
        return _hack.front();
    }
    char& MString::back()
    {
        return _hack.back();
    }
} // namespace strings
} // namespace tmwa
