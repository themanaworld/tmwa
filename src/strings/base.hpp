#ifndef TMWA_STRINGS_BASE_HPP
#define TMWA_STRINGS_BASE_HPP
//    strings/base.hpp - CRTP base for string implementations.
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
# include "pair.hpp"

# include <cstddef>

# include <iterator>


namespace tmwa
{
// It is a common mistake to assume that one string class for everything.
// Because C++ and TMWA have a C legacy, there are a few more here
// than would probably be necessary in an ideal language.
namespace strings
{
    // TODO reimplement some things in terms of Slice and Slice::iterator?

    // simple pointer-wrapping iterator that can be used to get distinct
    // types for different containers.
    template<class Tag>
    class _iterator
    {
        typedef _iterator X;

        const char *_ptr;
    public:
        typedef ptrdiff_t difference_type;
        typedef char value_type;
        typedef const char *pointer;
        typedef const char& reference;
        typedef std::random_access_iterator_tag iterator_category;

        _iterator(const char *p=nullptr) : _ptr(p) {}

        // iterator
        reference operator *() const { return *_ptr; }
        X& operator ++() { ++_ptr; return *this; }
        // equality comparable
        friend bool operator == (X l, X r) { return l._ptr == r._ptr; }
        // input iterator
        friend bool operator != (X l, X r) { return !(l == r); }
        pointer operator->() const { return _ptr; }
        X operator++ (int) { X out = *this; ++*this; return out; }
        // forward iterator is mostly semantical, and the ctor is above
        // bidirectional iterator
        X& operator --() { --_ptr; return *this; }
        X operator-- (int) { X out = *this; --*this; return out; }
        // random access iterator
        X& operator += (difference_type n) { _ptr += n; return *this; }
        friend X operator + (X a, difference_type n) { return a += n; }
        friend X operator + (difference_type n, X a) { return a += n; }
        X& operator -= (difference_type n) { _ptr -= n; return *this; }
        friend X operator - (X a, difference_type n) { return a -= n; }
        friend difference_type operator - (X b, X a) { return b._ptr - a._ptr; }
        reference operator[](difference_type n) const { return _ptr[n]; }
        friend bool operator < (X a, X b) { return a._ptr < b._ptr; }
        friend bool operator > (X a, X b) { return b < a; }
        friend bool operator >= (X a, X b) { return !(a < b); }
        friend bool operator <= (X a, X b) { return !(a > b); }
    };

    /// A helper class that implements all the interesting stuff that can
    /// be done on any constant string, in terms of .begin() and .end().
    template<class T, class O, class P>
    class _crtp_string
    {
        typedef typename P::TailSlice Z;
        typedef typename P::FullSlice X;
    public:
        // this will have to be changed if MString decides to join in.
        typedef _iterator<T> iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
    private:
        const T& _ref() const;
        iterator begin() const;
        iterator end() const;
        const RString *base() const;
    public:
        size_t size() const;
        reverse_iterator rbegin() const;
        reverse_iterator rend() const;
        explicit
        operator bool() const;
        bool operator !() const;
        operator P() const;

        // the existence of this has led to bugs
        // it's not really sane from a unicode perspective anyway ...
        // prefer startswith or extract
        __attribute__((deprecated))
        char operator[](size_t i) const;
        char front() const;
        char back() const;
        const char *data();

        Z xslice_t(size_t o) const;
        X xslice_h(size_t o) const;
        Z xrslice_t(size_t no) const;
        X xrslice_h(size_t no) const;
        Z xislice_t(iterator it) const;
        X xislice_h(iterator it) const;
        X xlslice(size_t o, size_t l) const;
        X xpslice(size_t b, size_t e) const;
        X xislice(iterator b, iterator e) const;
        Z lstrip() const;
        X rstrip() const;
        X strip() const;

        bool startswith(XPair x) const;
        bool endswith(XPair x) const;
        bool startswith(char c) const;
        bool endswith(char c) const;

        bool contains(char c) const;
        bool contains_seq(XPair s) const;
        bool contains_any(XPair s) const;

        bool has_print() const;
        bool is_print() const;
        __attribute__((deprecated))
        O to_print() const;

        bool is_graph() const;
        bool has_graph() const;

        bool has_lower() const;
        bool is_lower() const;
        O to_lower() const;

        bool has_upper() const;
        bool is_upper() const;
        O to_upper() const;

        bool has_alpha() const; // equivalent to has_lower || has_upper
        bool is_alpha() const; // NOT equivalent to is_lower || is_upper

        bool has_digit2() const;
        bool is_digit2() const;
        bool has_digit8() const;
        bool is_digit8() const;
        bool has_digit10() const;
        bool is_digit10() const;
        bool has_digit16() const;
        bool is_digit16() const;

        bool has_alnum() const; // equivalent to has_alpha || has_digit10
        bool is_alnum() const; // NOT equivalent to is_alpha || is_digit10
    };

    template<class L, class R>
    class string_comparison_allowed
    {
        constexpr static bool l_is_vstring_exact = std::is_same<VString<sizeof(L) - 1>, L>::value;
        constexpr static bool l_is_vstring_approx = std::is_base_of<VString<sizeof(L) - 1>, L>::value;
        constexpr static bool r_is_vstring_exact = std::is_same<VString<sizeof(R) - 1>, R>::value;
        constexpr static bool r_is_vstring_approx = std::is_base_of<VString<sizeof(R) - 1>, R>::value;

        constexpr static bool l_is_restricted = l_is_vstring_approx && !l_is_vstring_exact;
        constexpr static bool r_is_restricted = r_is_vstring_approx && !r_is_vstring_exact;
    public:
        constexpr static bool value = std::is_same<L, R>::value || (!l_is_restricted && !r_is_restricted);
    };

    // not really intended for public use
    int pair_compare(XPair l, XPair r);

    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator == (const L& l, const R& r) -> decltype((pair_compare(l, r), true));
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator != (const L& l, const R& r) -> decltype((pair_compare(l, r), true));
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator < (const L& l, const R& r) -> decltype((pair_compare(l, r), true));
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator <= (const L& l, const R& r) -> decltype((pair_compare(l, r), true));
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator > (const L& l, const R& r) -> decltype((pair_compare(l, r), true));
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator >= (const L& l, const R& r) -> decltype((pair_compare(l, r), true));
} // namespace strings
} // namespace tmwa

# include "base.tcc"

#endif // TMWA_STRINGS_BASE_HPP
