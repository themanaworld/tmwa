#ifndef TMWA_COMMON_STRINGS_HPP
#define TMWA_COMMON_STRINGS_HPP
//    strings.hpp - All the string classes you'll ever need.
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

#include "sanity.hpp"

#include <cassert>
#include <cstdarg>
#include <cstring>

#include <algorithm>
#include <deque>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

#include "utils2.hpp"

// It is a common mistake to assume that one string class for everything.
// Because C++ and TMWA have a C legacy, there are a few more here
// than would probably be necessary in an ideal language.
namespace strings
{
    // owning
    class MString;
    class FString;
    class TString; // C legacy version of SString
    class SString; // is this one really worth it?

    // non-owning
    class ZString; // C legacy version of XString
    class XString;

    // semi-owning
    template<uint8_t len>
    class VString;

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
    template<class T, class O, class Z, class X=XString>
    class _crtp_string
    {
    public:
        // this will have to be changed if MString decides to join in.
        typedef _iterator<T> iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
    private:
        const T& _ref() const { return static_cast<const T&>(*this); }
        iterator begin() const { return _ref().begin(); }
        iterator end() const { return _ref().end(); }
        const FString *base() const { return _ref().base(); }
    public:
        size_t size() const { return end() - begin(); }
        reverse_iterator rbegin() const { return reverse_iterator(end()); }
        reverse_iterator rend() const { return reverse_iterator(begin()); }
        explicit
        operator bool() const { return size(); }
        bool operator !() const { return !size(); }

        char operator[](size_t i) const { return begin()[i]; }
        char front() const { return *begin(); }
        char back() const { return end()[-1]; }
        const char *data() { return &*begin(); }

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

        bool startswith(XString x) const;
        bool endswith(XString x) const;
        bool startswith(char c) const;
        bool endswith(char c) const;

        bool contains(char c) const;
        bool contains_seq(XString s) const;
        bool contains_any(XString s) const;

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


    /// An owning string that is still expected to change.
    /// The storage might not be contiguous, but it still offers
    /// random-access iterators.
    /// TODO implement a special one, to avoid quirks of std::string.
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
        iterator begin() { return _hack.begin(); }
        iterator end() { return _hack.end(); }
        const_iterator begin() const { return _hack.begin(); }
        const_iterator end() const { return _hack.end(); }
        reverse_iterator rbegin() { return reverse_iterator(end()); }
        reverse_iterator rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

        size_t size() const { return _hack.size(); }
        explicit
        operator bool() const { return size(); }
        bool operator !() const { return !size(); }

        MString& operator += (MString rhs)
        {
            _hack.insert(_hack.end(), rhs.begin(), rhs.end());
            return *this;
        }
        MString& operator += (char c)
        {
            _hack.push_back(c);
            return *this;
        }
        MString& operator += (XString xs);

        void pop_back(size_t n=1)
        {
            while (n--)
                _hack.pop_back();
        }
        char& front()
        {
            return _hack.front();
        }
        char& back()
        {
            return _hack.back();
        }
    };

    /// An owning string that has reached its final contents.
    /// The storage is NUL-terminated
    /// TODO implement a special one, that guarantees refcounting.
    class FString : public _crtp_string<FString, FString, ZString, XString>
    {
        std::shared_ptr<std::vector<char>> _hack2;

        template<class It>
        void _assign(It b, It e)
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
            }
            size_t diff = std::distance(b, e);
            _hack2 = std::make_shared<std::vector<char>>(diff + 1, '\0');
            std::copy(b, e, _hack2->begin());
        }
    public:
        FString()
        {
            const char *sadness = "";
            _assign(sadness, sadness);
        }

        explicit FString(const MString& s)
        {
            _assign(s.begin(), s.end());
        }

        template<size_t n>
        FString(char (&s)[n]) = delete;

        template<size_t n>
        FString(const char (&s)[n])
        {
            _assign(s, s + strlen(s));
        }

        template<class It>
        FString(It b, It e)
        {
            _assign(b, e);
        }


        iterator begin() const { return &_hack2->begin()[0]; }
        iterator end() const { return &_hack2->end()[-1]; }
        const FString *base() const { return this; }
        const char *c_str() const { return &*begin(); }

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

    /// An owning string that represents a tail slice of an FString.
    /// Guaranteed to be NUL-terminated.
    class TString : public _crtp_string<TString, TString, ZString, XString>
    {
        friend class SString;
        FString _s;
        size_t _o;
    public:
        TString() : _s(), _o() {}
        TString(FString b, size_t i=0) : _s(std::move(b)), _o(i) {}
        template<size_t n>
        TString(char (&s)[n]) = delete;
        template<size_t n>
        TString(const char (&s)[n]) : _s(s), _o(0) {}
        //template<class It>
        //TString(It b, It e) : _s(b, e), _o(0) {}

        iterator begin() const { return &_s.begin()[_o]; }
        iterator end() const { return &*_s.end(); }
        const FString *base() const { return &_s; }
        const char *c_str() const { return &*begin(); }

        TString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        TString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        TString oislice_t(iterator it) const;
        SString oislice_h(iterator it) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;
        SString oislice(iterator b, iterator e) const;

        operator FString() const
        { if (_o) return FString(begin(), end()); else return _s; }
    };

    /// An owning string that represents a arbitrary slice of an FString.
    /// Not guaranteed to be NUL-terminated.
    class SString : public _crtp_string<SString, SString, XString, XString>
    {
        FString _s;
        size_t _b, _e;
    public:
        SString() : _s(), _b(), _e() {}
        SString(FString f) : _s(std::move(f)), _b(), _e(_s.size()) {}
        SString(TString t) : _s(t._s), _b(0), _e(_s.size()) {}
        template<size_t n>
        SString(char (&s)[n]) = delete;
        template<size_t n>
        SString(const char (&s)[n]) : _s(s), _b(0), _e(_s.size()) {}
        //template<class It>
        //SString(It b, It e) : _s(b, e), _b(0), _e(_s.size()) {}
        SString(FString f, size_t b, size_t e) : _s(std::move(f)), _b(b), _e(e) {}

        iterator begin() const { return &_s.begin()[_b]; }
        iterator end() const { return &_s.begin()[_e]; }
        const FString *base() const { return &_s; }

        SString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        SString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        SString oislice_t(iterator it) const;
        SString oislice_h(iterator it) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;
        SString oislice(iterator b, iterator e) const;

        operator FString() const
        { if (_b == 0 && _e == _s.size()) return _s; else return FString(begin(), end()); }
        operator TString() const
        { if (_e == _s.size()) return TString(_s, _b); else return FString(begin(), end()); }
    };

    /// A non-owning string that is guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class ZString : public _crtp_string<ZString, FString, ZString, XString>
    {
        iterator _b, _e;
        // optional
        const FString *_base;
    public:
        enum { really_construct_from_a_pointer };
        ZString() { *this = ZString(""); }
        // no MString
        ZString(const FString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        ZString(const TString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        ZString(const SString&) = delete;
        // dangerous
        ZString(const char *b, const char *e, const FString *base_) : _b(b), _e(e), _base(base_) {}
        ZString(decltype(really_construct_from_a_pointer), const char *s, const FString *base_) : _b(s), _e(s + strlen(s)), _base(base_) {}
        template<size_t n>
        ZString(char (&s)[n]) = delete;
        template<size_t n>
        ZString(const char (&s)[n], const FString *base_=nullptr) : _b(s), _e(s + strlen(s)), _base(base_) {}

        iterator begin() const { return _b; }
        iterator end() const { return _e; }
        const FString *base() const { return _base; }
        const char *c_str() const { return &*begin(); }

        ZString oslice_t(size_t o) const;
        XString oslice_h(size_t o) const;
        ZString orslice_t(size_t no) const;
        XString orslice_h(size_t no) const;
        ZString oislice_t(iterator it) const;
        XString oislice_h(iterator it) const;
        XString olslice(size_t o, size_t l) const;
        XString opslice(size_t b, size_t e) const;
        XString oislice(iterator b, iterator e) const;

        operator FString() const
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator TString() const
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator SString() const
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
    };

    /// A non-owning string that is not guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class XString : public _crtp_string<XString, FString, XString, XString>
    {
        iterator _b, _e;
        // optional
        const FString *_base;
    public:
        // do I really want this?
        XString() : _b(""), _e(_b), _base() {}
        XString(std::nullptr_t) = delete;
        // no MString
        XString(const FString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        XString(const TString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        XString(const SString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        XString(const ZString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        template<size_t n>
        XString(char (&s)[n]) = delete;
        template<size_t n>
        XString(const char (&s)[n]) : _b(s), _e(s + strlen(s)), _base(nullptr) {}
        // mostly internal
        XString(const char *b, const char *e, const FString *base_) : _b(b), _e(e), _base(base_) {}
        XString(decltype(ZString::really_construct_from_a_pointer) e, const char *s, const FString *base_)
        {
            *this = ZString(e, s, base_);
        }

        iterator begin() const { return _b; }
        iterator end() const { return _e; }
        const FString *base() const { return _base; }

        XString oslice_t(size_t o) const { return xslice_t(o); }
        XString oslice_h(size_t o) const { return xslice_h(o); }
        XString orslice_t(size_t no) const { return xrslice_t(no); }
        XString orslice_h(size_t no) const { return xrslice_h(no); }
        XString oislice_t(iterator it) const { return xislice_t(it); }
        XString oislice_h(iterator it) const { return xislice_h(it); }
        XString olslice(size_t o, size_t l) const { return xlslice(o, l); }
        XString opslice(size_t b, size_t e) const { return xpslice(b, e); }
        XString oislice(iterator b, iterator e) const { return xislice(b, e); }

        operator FString() const
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator TString() const
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator SString() const
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator ZString() const = delete;
    };

    template<uint8_t n>
    class VString : public _crtp_string<VString<n>, VString<n>, ZString, XString>
    {
        char _data[n];
        unsigned char _special;
    public:
        typedef typename _crtp_string<VString<n>, VString<n>, ZString, XString>::iterator iterator;
        VString(XString x) : _data(), _special()
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
        VString(FString f)
        {
            *this = XString(f);
        }
        VString(TString t)
        {
            *this = XString(t);
        }
        VString(SString s)
        {
            *this = XString(s);
        }
        VString(ZString z)
        {
            *this = XString(z);
        }
        template<size_t m>
        VString(char (&s)[m]) = delete;
        template<size_t m>
        VString(const char (&s)[m])
        {
            static_assert(m <= n + 1, "string would truncate");
            *this = XString(s);
        }
        VString(decltype(ZString::really_construct_from_a_pointer) e, const char *s)
        {
            *this = XString(e, s, nullptr);
        }
        VString(char c)
        {
            *this = XString(&c, &c + 1, nullptr);
        }
        VString()
        {
            *this = XString();
        }

        // hopefully this is obvious
        iterator begin() const { return std::begin(_data); }
        iterator end() const { return std::end(_data) - _special; }
        const FString *base() const { return nullptr; }
        const char *c_str() const { return &*begin(); }

        VString oslice_t(size_t o) const { return this->xslice_t(o); }
        VString oslice_h(size_t o) const { return this->xslice_h(o); }
        VString orslice_t(size_t no) const { return this->xrslice_t(no); }
        VString orslice_h(size_t no) const { return this->xrslice_h(no); }
        VString oislice_t(iterator it) const { return this->xislice_t(it); }
        VString oislice_h(iterator it) const { return this->xislice_h(it); }
        VString olslice(size_t o, size_t l) const { return this->xlslice(o, l); }
        VString opslice(size_t b, size_t e) const { return this->xpslice(b, e); }
        VString oislice(iterator b, iterator e) const { return this->xislice(b, e); }

        operator FString() const { return FString(begin(), end()); }
        operator TString() const { return FString(begin(), end()); }
        operator SString() const { return FString(begin(), end()); }
        operator ZString() const { return ZString(_data); }
        operator XString() const { return XString(&*begin(), &*end(), nullptr); }

        template<uint8_t m>
        operator VString<m>() const
        {
            static_assert(m > n, "can only grow");
            XString x = *this;
            return VString<m>(XString(x));
        }
    };

    // not really intended for public use
    inline
    int xstr_compare(XString l, XString r)
    {
        bool less = std::lexicographical_compare(
                l.begin(), l.end(),
                r.begin(), r.end());
        bool greater = std::lexicographical_compare(
                r.begin(), r.end(),
                l.begin(), l.end());
        return greater - less;
    }


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

    struct _test : VString<1> {};
    struct _test2 : VString<1> {};

    static_assert(string_comparison_allowed<_test, _test>::value, "tt");
    static_assert(string_comparison_allowed<VString<1>, VString<1>>::value, "vv");
    static_assert(!string_comparison_allowed<_test, XString>::value, "tx");
    static_assert(!string_comparison_allowed<_test, VString<1>>::value, "tv");
    static_assert(!string_comparison_allowed<_test, _test2>::value, "t2");
    static_assert(string_comparison_allowed<VString<1>, XString>::value, "vx");
    static_assert(string_comparison_allowed<XString, XString>::value, "xx");
    static_assert(string_comparison_allowed<XString, FString>::value, "xf");

    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator == (const L& l, const R& r) -> decltype((xstr_compare(l, r), true))
    {
        return xstr_compare(l, r) == 0;
    }
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator != (const L& l, const R& r) -> decltype((xstr_compare(l, r), true))
    {
        return xstr_compare(l, r) != 0;
    }
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator < (const L& l, const R& r) -> decltype((xstr_compare(l, r), true))
    {
        return xstr_compare(l, r) < 0;
    }
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator <= (const L& l, const R& r) -> decltype((xstr_compare(l, r), true))
    {
        return xstr_compare(l, r) <= 0;
    }
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator > (const L& l, const R& r) -> decltype((xstr_compare(l, r), true))
    {
        return xstr_compare(l, r) > 0;
    }
    template<class L, class R, typename=typename std::enable_if<string_comparison_allowed<L, R>::value>::type>
    auto operator >= (const L& l, const R& r) -> decltype((xstr_compare(l, r), true))
    {
        return xstr_compare(l, r) >= 0;
    }

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
    }

    // sadness
    typedef MString MS;
    typedef FString FS;
    typedef SString SS;
    typedef TString TS;
    typedef ZString ZS;
    typedef XString XS;

    // _crtp_string
    template<class T, class O, class Z, class X>
    Z _crtp_string<T, O, Z, X>::xslice_t(size_t o) const
    { return Z(&begin()[o], &*end(), base()); }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::xslice_h(size_t o) const
    { return X(&*begin(), &begin()[o], base()); }
    template<class T, class O, class Z, class X>
    Z _crtp_string<T, O, Z, X>::xrslice_t(size_t no) const
    { return Z(&end()[-no], &*end(), base()); }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::xrslice_h(size_t no) const
    { return X(&*begin(), &end()[-no], base()); }
    template<class T, class O, class Z, class X>
    Z _crtp_string<T, O, Z, X>::xislice_t(iterator it) const
    { return Z(&*it, &*end(), base()); }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::xislice_h(iterator it) const
    { return X(&*begin(), &*it, base()); }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::xlslice(size_t o, size_t l) const
    { return X(&begin()[o], &begin()[o + l], base()); }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::xpslice(size_t b, size_t e) const
    { return X(&begin()[b], &begin()[e], base()); }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::xislice(iterator b, iterator e) const
    { return X(&*b, &*e, base()); }
    template<class T, class O, class Z, class X>
    Z _crtp_string<T, O, Z, X>::lstrip() const
    {
        Z z = _ref();
        while (z.startswith(' '))
            z = z.xslice_t(1);
        return z;
    }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::rstrip() const
    {
        X x = _ref();
        while (x.endswith(' '))
            x = x.xrslice_h(1);
        return x;
    }
    template<class T, class O, class Z, class X>
    X _crtp_string<T, O, Z, X>::strip() const
    { return lstrip().rstrip(); }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::startswith(XS x) const
    { return size() >= x.size() && xslice_h(x.size()) == x; }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::endswith(XS x) const
    { return size() > x.size() && xrslice_t(x.size()) == x; }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::startswith(char c) const
    { return size() && front() == c; }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::endswith(char c) const
    { return size() && back() == c; }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::contains(char c) const
    { return std::find(begin(), end(), c) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::contains_seq(XString s) const
    { return std::search(begin(), end(), s.begin(), s.end()) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::contains_any(XString s) const
    { return std::find_if(begin(), end(), [s](char c) { return s.contains(c); }) != end(); }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_print() const
    { return std::find_if(begin(), end(), detail::is_print) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_print() const
    { return std::find_if_not(begin(), end(), detail::is_print) == end(); }
    template<class T, class O, class Z, class X>
    O _crtp_string<T, O, Z, X>::to_print() const
    {
        if (is_print()) return _ref();
        char buf[size()];
        char *const b = buf;
        char *const e = std::transform(begin(), end(), b, [](char c) { return detail::is_print(c) ? c : '_'; });
        return XString(b, e, nullptr);
    }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_graph() const
    { return std::find_if(begin(), end(), detail::is_graph) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_graph() const
    { return std::find_if_not(begin(), end(), detail::is_graph) == end(); }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_lower() const
    { return std::find_if(begin(), end(), detail::is_lower) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_lower() const
    { return std::find_if_not(begin(), end(), detail::is_lower) == end(); }
    template<class T, class O, class Z, class X>
    O _crtp_string<T, O, Z, X>::to_lower() const
    {
        if (!has_upper()) return _ref();
        char buf[size()];
        char *const b = buf;
        char *const e = std::transform(begin(), end(), b, detail::to_lower);
        return XString(b, e, nullptr);
    }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_upper() const
    { return std::find_if(begin(), end(), detail::is_upper) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_upper() const
    { return std::find_if_not(begin(), end(), detail::is_upper) == end(); }
    template<class T, class O, class Z, class X>
    O _crtp_string<T, O, Z, X>::to_upper() const
    {
        if (!has_lower()) return _ref();
        char buf[size()];
        char *const b = buf;
        char *const e = std::transform(begin(), end(), b, detail::to_upper);
        return XString(b, e, nullptr);
    }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_alpha() const
    { return std::find_if(begin(), end(), detail::is_alpha) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_alpha() const
    { return std::find_if_not(begin(), end(), detail::is_alpha) == end(); }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_digit2() const
    { return std::find_if(begin(), end(), detail::is_digit2) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_digit2() const
    { return std::find_if_not(begin(), end(), detail::is_digit2) == end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_digit8() const
    { return std::find_if(begin(), end(), detail::is_digit8) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_digit8() const
    { return std::find_if_not(begin(), end(), detail::is_digit8) == end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_digit10() const
    { return std::find_if(begin(), end(), detail::is_digit10) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_digit10() const
    { return std::find_if_not(begin(), end(), detail::is_digit10) == end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_digit16() const
    { return std::find_if(begin(), end(), detail::is_digit16) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_digit16() const
    { return std::find_if_not(begin(), end(), detail::is_digit16) == end(); }

    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::has_alnum() const
    { return std::find_if(begin(), end(), detail::is_alnum) != end(); }
    template<class T, class O, class Z, class X>
    bool _crtp_string<T, O, Z, X>::is_alnum() const
    { return std::find_if_not(begin(), end(), detail::is_alnum) == end(); }

    // MString
    inline
    MS& MS::operator += (XS x)
    {
        _hack.insert(_hack.end(), x.begin(), x.end());
        return *this;
    }

    // FString
    inline
    TS FS::oslice_t(size_t o) const
    { return TS(*this, o); }
    inline
    SS FS::oslice_h(size_t o) const
    { return SS(*this, 0, o); }
    inline
    TS FS::orslice_t(size_t no) const
    { return TS(*this, size() - no); }
    inline
    SS FS::orslice_h(size_t no) const
    { return SS(*this, 0, size() - no); }
    inline
    TS FS::oislice_t(iterator it) const
    { return TS(*this, it - begin()); }
    inline
    SS FS::oislice_h(iterator it) const
    { return SS(*this, 0, it - begin()); }
    inline
    SS FS::olslice(size_t o, size_t l) const
    { return SS(*this, o, o + l); }
    inline
    SS FS::opslice(size_t b, size_t e) const
    { return SS(*this, b, e); }
    inline
    SS FS::oislice(iterator b, iterator e) const
    { return SS(*this, b - begin(), e - begin()); }

    // TString
    inline
    TS TS::oslice_t(size_t o) const
    { return TS(_s, _o + o); }
    inline
    SS TS::oslice_h(size_t o) const
    { return SS(_s, _o, _o + o); }
    inline
    TS TS::orslice_t(size_t no) const
    { return TS(_s, _s.size() - no); }
    inline
    SS TS::orslice_h(size_t no) const
    { return SS(_s, _o, _s.size() - no); }
    inline
    TS TS::oislice_t(iterator it) const
    { return TS(_s, _o + it - begin()); }
    inline
    SS TS::oislice_h(iterator it) const
    { return SS(_s, _o, _o + it - begin()); }
    inline
    SS TS::olslice(size_t o, size_t l) const
    { return SS(_s, _o + o, _o + o + l); }
    inline
    SS TS::opslice(size_t b, size_t e) const
    { return SS(_s, _o + b, _o + e); }
    inline
    SS TS::oislice(iterator b, iterator e) const
    { return SS(_s, _o + b - begin(), _o + e - begin()); }

    // SString
    inline
    SS SS::oslice_t(size_t o) const
    { return SS(_s, _b + o, _e); }
    inline
    SS SS::oslice_h(size_t o) const
    { return SS(_s, _b, _b + o); }
    inline
    SS SS::orslice_t(size_t no) const
    { return SS(_s, _e - no, _e); }
    inline
    SS SS::orslice_h(size_t no) const
    { return SS(_s, _b, _e - no); }
    inline
    SS SS::oislice_t(iterator it) const
    { return SS(_s, _b + it - begin(), _e); }
    inline
    SS SS::oislice_h(iterator it) const
    { return SS(_s, _b, _b + it - begin()); }
    inline
    SS SS::olslice(size_t o, size_t l) const
    { return SS(_s, _b + o, _b + o + l); }
    inline
    SS SS::opslice(size_t b, size_t e) const
    { return SS(_s, _b + b, _b + e); }
    inline
    SS SS::oislice(iterator b, iterator e) const
    { return SS(_s, _b + b - begin(), _b + e - begin()); }

    // ZString
    inline
    ZS ZS::oslice_t(size_t o) const
    { return ZS(really_construct_from_a_pointer, &begin()[o], base()); }
    inline
    XS ZS::oslice_h(size_t o) const
    { return XS(&*begin(), &begin()[o], base()); }
    inline
    ZS ZS::orslice_t(size_t no) const
    { return ZS(really_construct_from_a_pointer, &end()[-no], base()); }
    inline
    XS ZS::orslice_h(size_t no) const
    { return XS(&*begin(), &end()[-no], base()); }
    inline
    ZS ZS::oislice_t(iterator it) const
    { return ZS(really_construct_from_a_pointer, &*it, base()); }
    inline
    XS ZS::oislice_h(iterator it) const
    { return XS(&*begin(), &*it, base()); }
    inline
    XS ZS::olslice(size_t o, size_t l) const
    { return XS(&begin()[o], &begin()[o + l], base()); }
    inline
    XS ZS::opslice(size_t b, size_t e) const
    { return XS(&begin()[b], &begin()[e], base()); }
    inline
    XS ZS::oislice(iterator b, iterator e) const
    { return XS(&*b, &*e, base()); }


    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    // Actually, it might now.
    inline
    const char *decay_for_printf(const FString& fs) { return fs.c_str(); }
    inline
    const char *decay_for_printf(const TString& ts) { return ts.c_str(); }
    inline
    const char *decay_for_printf(const ZString& zs) { return zs.c_str(); }
    template<uint8_t n>
    inline
    const char *decay_for_printf(const VString<n>& vs) { return vs.c_str(); }

    template<uint8_t len>
    inline __attribute__((format(printf, 2, 0)))
    int do_vprint(VString<len>& out, const char *fmt, va_list ap)
    {
        char buffer[len + 1];
        vsnprintf(buffer, len + 1, fmt, ap);

        out = const_(buffer);
        return len;
    }

    inline __attribute__((format(printf, 2, 0)))
    int do_vprint(FString& out, const char *fmt, va_list ap)
    {
        int len;
        {
            va_list ap2;
            va_copy(ap2, ap);
            len = vsnprintf(nullptr, 0, fmt, ap2);
            va_end(ap2);
        }
        char buffer[len + 1];
        vsnprintf(buffer, len + 1, fmt, ap);

        out = FString(buffer, buffer + len);
        return len;
    }

    inline __attribute__((format(scanf, 2, 0)))
    int do_vscan(ZString in, const char *fmt, va_list ap)
    {
        return vsscanf(in.c_str(), fmt, ap);
    }

    class StringConverter
    {
        FString& out;
        char *mid;
    public:
        StringConverter(FString& s)
        : out(s), mid(nullptr)
        {}
        ~StringConverter()
        {
            if (mid)
            {
                out = ZString(ZString::really_construct_from_a_pointer, mid, nullptr);
                free(mid);
            }
        }
        char **operator &()
        {
            return &mid;
        }
    };

    inline
    StringConverter convert_for_scanf(FString& s)
    {
        return StringConverter(s);
    }
} // namespace strings

// owning
using strings::MString;
using strings::FString;
using strings::SString;
using strings::TString;

// non-owning
using strings::ZString;
using strings::XString;

// semi-owning
using strings::VString;

#endif // TMWA_COMMON_STRINGS_HPP
