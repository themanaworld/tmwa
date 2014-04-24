//    strings/base.tcc - Inline functions for strings/base.hpp
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

#include <cstddef>

#include <algorithm>

#include "pair.hpp"

namespace strings
{
    namespace detail
    {
        constexpr
        bool is_print(char c)
        {
            return ' ' <= c && c <= '~';
        }
        constexpr
        bool is_graph(char c)
        {
            return is_print(c) && c != ' ';
        }
        constexpr
        bool is_lower(char c)
        {
            return 'a' <= c && c <= 'z';
        }
        constexpr
        bool is_upper(char c)
        {
            return 'A' <= c && c <= 'Z';
        }
        constexpr
        bool is_alpha(char c)
        {
            return is_lower(c) || is_upper(c);
        }
        constexpr
        bool is_digit2(char c)
        {
            return '0' <= c && c <= '1';
        }
        constexpr
        bool is_digit8(char c)
        {
            return '0' <= c && c <= '7';
        }
        constexpr
        bool is_digit10(char c)
        {
            return '0' <= c && c <= '9';
        }
        constexpr
        bool is_digit16(char c)
        {
            return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f');
        }
        constexpr
        bool is_alnum(char c)
        {
            return is_alpha(c) || is_digit10(c);
        }

        constexpr
        char to_lower(char c)
        {
            return is_upper(c) ? c | ' ' : c;
        }
        constexpr
        char to_upper(char c)
        {
            return is_lower(c) ? c & ~' ' : c;
        }
    } // namespace detail

    template<class T, class O, class P>
    const T& _crtp_string<T, O, P>::_ref() const
    {
        return static_cast<const T&>(*this);
    }
    template<class T, class O, class P>
    typename _crtp_string<T, O, P>::iterator _crtp_string<T, O, P>::begin() const
    {
        return _ref().begin();
    }
    template<class T, class O, class P>
    typename _crtp_string<T, O, P>::iterator _crtp_string<T, O, P>::end() const
    {
        return _ref().end();
    }
    template<class T, class O, class P>
    const RString *_crtp_string<T, O, P>::base() const
    {
        return _ref().base();
    }
    template<class T, class O, class P>
    size_t _crtp_string<T, O, P>::size() const
    {
        return end() - begin();
    }
    template<class T, class O, class P>
    typename _crtp_string<T, O, P>::reverse_iterator _crtp_string<T, O, P>::rbegin() const
    {
        return reverse_iterator(end());
    }
    template<class T, class O, class P>
    typename _crtp_string<T, O, P>::reverse_iterator _crtp_string<T, O, P>::rend() const
    {
        return reverse_iterator(begin());
    }
    template<class T, class O, class P>
    _crtp_string<T, O, P>::operator bool() const
    {
        return size();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::operator !() const
    {
        return !size();
    }
    template<class T, class O, class P>
    _crtp_string<T, O, P>::operator P() const
    {
        return {&*begin(), &*end()};
    }

    template<class T, class O, class P>
    __attribute__((deprecated))
    char _crtp_string<T, O, P>::operator[](size_t i) const
    {
        return begin()[i];
    }
    template<class T, class O, class P>
    char _crtp_string<T, O, P>::front() const
    {
        return *begin();
    }
    template<class T, class O, class P>
    char _crtp_string<T, O, P>::back() const
    {
        return end()[-1];
    }
    template<class T, class O, class P>
    const char *_crtp_string<T, O, P>::data()
    {
        return &*begin();
    }

    template<class T, class O, class P>
    typename P::TailSlice _crtp_string<T, O, P>::xslice_t(size_t o) const
    {
        return typename P::TailSlice(&begin()[o], &*end(), base());
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::xslice_h(size_t o) const
    {
        return typename P::FullSlice(&*begin(), &begin()[o], base());
    }
    template<class T, class O, class P>
    typename P::TailSlice _crtp_string<T, O, P>::xrslice_t(size_t no) const
    {
        return typename P::TailSlice(&end()[-no], &*end(), base());
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::xrslice_h(size_t no) const
    {
        return typename P::FullSlice(&*begin(), &end()[-no], base());
    }
    template<class T, class O, class P>
    typename P::TailSlice _crtp_string<T, O, P>::xislice_t(iterator it) const
    {
        return typename P::TailSlice(&*it, &*end(), base());
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::xislice_h(iterator it) const
    {
        return typename P::FullSlice(&*begin(), &*it, base());
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::xlslice(size_t o, size_t l) const
    {
        return typename P::FullSlice(&begin()[o], &begin()[o + l], base());
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::xpslice(size_t b, size_t e) const
    {
        return typename P::FullSlice(&begin()[b], &begin()[e], base());
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::xislice(iterator b, iterator e) const
    {
        return typename P::FullSlice(&*b, &*e, base());
    }
    template<class T, class O, class P>
    typename P::TailSlice _crtp_string<T, O, P>::lstrip() const
    {
        typename P::TailSlice z = _ref();
        while (z.startswith(' '))
            z = z.xslice_t(1);
        return z;
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::rstrip() const
    {
        typename P::FullSlice x = _ref();
        while (x.endswith(' '))
            x = x.xrslice_h(1);
        return x;
    }
    template<class T, class O, class P>
    typename P::FullSlice _crtp_string<T, O, P>::strip() const
    {
        return lstrip().rstrip();
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::startswith(XPair x) const
    {
        return size() >= x.size() && pair_compare(xslice_h(x.size()), x) == 0;
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::endswith(XPair x) const
    {
        return size() > x.size() && pair_compare(xrslice_t(x.size()), x) == 0;
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::startswith(char c) const
    {
        return size() && front() == c;
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::endswith(char c) const
    {
        return size() && back() == c;
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::contains(char c) const
    {
        return std::find(begin(), end(), c) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::contains_seq(XPair s) const
    {
        return std::search(begin(), end(), s.begin(), s.end()) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::contains_any(XPair s) const
    {
        return std::find_if(s.begin(), s.end(), [this](char c) { return this->contains(c); }) != s.end();
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_print() const
    {
        return std::find_if(begin(), end(), detail::is_print) != end(); }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_print() const
    {
        return std::find_if_not(begin(), end(), detail::is_print) == end(); }
    template<class T, class O, class P>
    O _crtp_string<T, O, P>::to_print() const
    {
        if (is_print()) return _ref();
        char buf[size()];
        char *const b = buf;
        char *const e = std::transform(begin(), end(), b, [](char c) { return detail::is_print(c) ? c : '_'; });
        return XPair(b, e);
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_graph() const
    {
        return std::find_if(begin(), end(), detail::is_graph) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_graph() const
    {
        return std::find_if_not(begin(), end(), detail::is_graph) == end();
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_lower() const
    {
        return std::find_if(begin(), end(), detail::is_lower) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_lower() const
    {
        return std::find_if_not(begin(), end(), detail::is_lower) == end();
    }
    template<class T, class O, class P>
    O _crtp_string<T, O, P>::to_lower() const
    {
        if (!has_upper()) return _ref();
        char buf[size()];
        char *const b = buf;
        char *const e = std::transform(begin(), end(), b, detail::to_lower);
        return XPair(b, e);
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_upper() const
    {
        return std::find_if(begin(), end(), detail::is_upper) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_upper() const
    {
        return std::find_if_not(begin(), end(), detail::is_upper) == end();
    }
    template<class T, class O, class P>
    O _crtp_string<T, O, P>::to_upper() const
    {
        if (!has_lower()) return _ref();
        char buf[size()];
        char *const b = buf;
        char *const e = std::transform(begin(), end(), b, detail::to_upper);
        return XPair(b, e);
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_alpha() const
    {
        return std::find_if(begin(), end(), detail::is_alpha) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_alpha() const
    {
        return std::find_if_not(begin(), end(), detail::is_alpha) == end();
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_digit2() const
    {
        return std::find_if(begin(), end(), detail::is_digit2) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_digit2() const
    {
        return std::find_if_not(begin(), end(), detail::is_digit2) == end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_digit8() const
    {
        return std::find_if(begin(), end(), detail::is_digit8) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_digit8() const
    {
        return std::find_if_not(begin(), end(), detail::is_digit8) == end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_digit10() const
    {
        return std::find_if(begin(), end(), detail::is_digit10) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_digit10() const
    {
        return std::find_if_not(begin(), end(), detail::is_digit10) == end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_digit16() const
    {
        return std::find_if(begin(), end(), detail::is_digit16) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_digit16() const
    {
        return std::find_if_not(begin(), end(), detail::is_digit16) == end();
    }

    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::has_alnum() const
    {
        return std::find_if(begin(), end(), detail::is_alnum) != end();
    }
    template<class T, class O, class P>
    bool _crtp_string<T, O, P>::is_alnum() const
    {
        return std::find_if_not(begin(), end(), detail::is_alnum) == end();
    }

    // not really intended for public use
    inline
    int pair_compare(XPair l, XPair r)
    {
        bool less = std::lexicographical_compare(
                l.begin(), l.end(),
                r.begin(), r.end());
        bool greater = std::lexicographical_compare(
                r.begin(), r.end(),
                l.begin(), l.end());
        return greater - less;
    }

    template<class L, class R, typename>
    auto operator == (const L& l, const R& r) -> decltype((pair_compare(l, r), true))
    {
        return pair_compare(l, r) == 0;
    }
    template<class L, class R, typename>
    auto operator != (const L& l, const R& r) -> decltype((pair_compare(l, r), true))
    {
        return pair_compare(l, r) != 0;
    }
    template<class L, class R, typename>
    auto operator < (const L& l, const R& r) -> decltype((pair_compare(l, r), true))
    {
        return pair_compare(l, r) < 0;
    }
    template<class L, class R, typename>
    auto operator <= (const L& l, const R& r) -> decltype((pair_compare(l, r), true))
    {
        return pair_compare(l, r) <= 0;
    }
    template<class L, class R, typename>
    auto operator > (const L& l, const R& r) -> decltype((pair_compare(l, r), true))
    {
        return pair_compare(l, r) > 0;
    }
    template<class L, class R, typename>
    auto operator >= (const L& l, const R& r) -> decltype((pair_compare(l, r), true))
    {
        return pair_compare(l, r) >= 0;
    }
} // namespace strings
