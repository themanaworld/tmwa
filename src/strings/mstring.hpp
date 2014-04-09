#ifndef TMWA_STRINGS_MSTRING_HPP
#define TMWA_STRINGS_MSTRING_HPP
//    strings/mstring.hpp - A mutable string.
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

# include "../sanity.hpp"

# include <deque>

# include "base.hpp"

namespace strings
{
    /// An owning string that is still expected to change.
    /// The storage might not be contiguous, but it still offers
    /// random-access iterators.
    /// TODO implement a special one, to avoid quirks of std::deque.
    class MString
    {
    public:
        typedef std::deque<char>::iterator iterator;
        typedef std::deque<char>::const_iterator const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    private:
        std::deque<char> _hack;
    public:
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        reverse_iterator rbegin();
        reverse_iterator rend();
        const_reverse_iterator rbegin() const;
        const_reverse_iterator rend() const;

        size_t size() const;
        explicit
        operator bool() const;
        bool operator !() const;

        MString& operator += (MString rhs);
        MString& operator += (char c);
        MString& operator += (XString xs);

        void pop_back(size_t n=1);
        char& front();
        char& back();
    };
} // namespace strings

#endif // TMWA_STRINGS_MSTRING_HPP
