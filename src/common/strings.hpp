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
#include <cstring>

#include <iterator>
#include <string>

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
        reference operator *() { return *_ptr; }
        X& operator ++() { ++_ptr; return *this; }
        // equality comparable
        friend bool operator == (X l, X r) { return l._ptr == r._ptr; }
        // input iterator
        friend bool operator != (X l, X r) { return !(l == r); }
        pointer operator->() { return _ptr; }
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
        reference operator[](difference_type n) { return _ptr[n]; }
        friend bool operator < (X a, X b) { return a._ptr - b._ptr; }
        friend bool operator > (X a, X b) { return b < a; }
        friend bool operator >= (X a, X b) { return !(a < b); }
        friend bool operator <= (X a, X b) { return !(a > b); }
    };

    /// A helper class that implements all the interesting stuff that can
    /// be done on any constant string, in terms of .begin() and .end().
    template<class T>
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
        operator bool() { return size(); }
        bool operator !() { return !size(); }

        char operator[](size_t i) const { return begin()[i]; }
        char front() const { return *begin(); }
        char back() const { return end()[-1]; }
        const char *data() { return &*begin(); }

        XString xslice_t(size_t o) const;
        XString xslice_h(size_t o) const;
        XString xrslice_t(size_t no) const;
        XString xrslice_h(size_t no) const;
        XString xlslice(size_t o, size_t l) const;
        XString xpslice(size_t b, size_t e) const;
        bool startswith(XString x) const;
        bool endswith(XString x) const;
    };


    /// An owning string that is still expected to change.
    /// The storage might not be contiguous, but it still offers
    /// random-access iterators.
    /// TODO implement a special one, to avoid quirks of std::string.
    class MString
    {
    public:
        typedef char *iterator;
        typedef _iterator<MString> const_iterator;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    private:
        std::string _hack;
    public:
        template<size_t n>
        MString(char (&s)[n]) = delete;
        template<size_t n>
        MString(const char (&s)[n]) : _hack(s) {}
        template<class It>
        MString(It b, It e) : _hack(b, e) {}

        iterator begin() { return &*_hack.begin(); }
        iterator end() { return &*_hack.end(); }
        const_iterator begin() const { return &*_hack.begin(); }
        const_iterator end() const { return &*_hack.end(); }
        reverse_iterator rbegin() { return reverse_iterator(end()); }
        reverse_iterator rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    };

    /// An owning string that has reached its final contents.
    /// The storage is NUL-terminated
    /// TODO implement a special one, that guarantees refcounting.
    class FString : public _crtp_string<FString>
    {
        /*const*/ std::string _hack;
    public:
#ifndef __clang__
        __attribute__((warning("This should be removed in the next diff")))
#endif
        FString(std::string s) : _hack(std::move(s)) {}

        FString() : _hack() {}
        FString(const MString& s) : _hack(s.begin(), s.end()) {}
        template<size_t n>
        FString(char (&s)[n]) = delete;
        template<size_t n>
        FString(const char (&s)[n]) : _hack(s) {}
        template<class It>
        FString(It b, It e) : _hack(b, e) {}

        iterator begin() const { return &*_hack.begin(); }
        iterator end() const { return &*_hack.end(); }
        const FString *base() const { return this; }
        const char *c_str() const { return &*begin(); }

        TString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        TString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;
    };

    /// An owning string that represents a tail slice of an FString.
    /// Guaranteed to be NUL-terminated.
    class TString : public _crtp_string<TString>
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

        iterator begin() const { return &_s.begin()[_o]; }
        iterator end() const { return &*_s.end(); }
        const FString *base() const { return &_s; }
        const char *c_str() const { return &*begin(); }

        TString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        TString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;

        operator FString()
        { if (_o) return FString(begin(), end()); else return _s; }
    };

    /// An owning string that represents a arbitrary slice of an FString.
    /// Not guaranteed to be NUL-terminated.
    class SString : public _crtp_string<SString>
    {
        FString _s;
        size_t _b, _e;
    public:
        SString() : _s(), _b(), _e() {}
        SString(FString f) : _s(std::move(f)), _b(), _e(_s.size()) {}
        SString(TString t) : _s(t._s), _e(_s.size()) {}
        template<size_t n>
        SString(char (&s)[n]) = delete;
        template<size_t n>
        SString(const char (&s)[n]) : _s(s), _b(0), _e(_s.size()) {}

        SString(FString f, size_t b, size_t e) : _s(std::move(f)), _b(b), _e(e) {}

        iterator begin() const { return &_s.begin()[_b]; }
        iterator end() const { return &_s.begin()[_e]; }
        const FString *base() const { return &_s; }

        SString oslice_t(size_t o) const;
        SString oslice_h(size_t o) const;
        SString orslice_t(size_t no) const;
        SString orslice_h(size_t no) const;
        SString olslice(size_t o, size_t l) const;
        SString opslice(size_t b, size_t e) const;

        operator FString()
        { if (_b == 0 && _e == _s.size()) return _s; else return FString(begin(), end()); }
        operator TString()
        { if (_e == _s.size()) return TString(_s, _b); else return FString(begin(), end()); }
    };

    /// A non-owning string that is guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class ZString : public _crtp_string<ZString>
    {
        iterator _b, _e;
        // optional
        const FString *_base;
    public:
#ifndef __clang__
        __attribute__((warning("This should be removed in the next diff")))
#endif
        ZString(const std::string& s) : _b(&*s.begin()), _e(&*s.end()), _base(nullptr) {}

        enum { really_construct_from_a_pointer };
        ZString() { *this = ZString(""); }
        // no MString
        ZString(const FString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        ZString(const TString& s) : _b(&*s.begin()), _e(&*s.end()), _base(s.base()) {}
        ZString(const SString&) = delete;
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
        XString olslice(size_t o, size_t l) const;
        XString opslice(size_t b, size_t e) const;

        operator FString()
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator TString()
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator SString()
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
    };

    /// A non-owning string that is not guaranteed to be NUL-terminated.
    /// This should be only used as a parameter.
    class XString : public _crtp_string<XString>
    {
        iterator _b, _e;
        // optional
        const FString *_base;
    public:
        // do I really want this?
        XString() : _b(nullptr), _e(nullptr) {}
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
        const FString *base() const { return _base; };

        XString oslice_t(size_t o) const { return xslice_t(o); }
        XString oslice_h(size_t o) const { return xslice_h(o); }
        XString orslice_t(size_t no) const { return xrslice_t(no); }
        XString orslice_h(size_t no) const { return xrslice_h(no); }
        XString olslice(size_t o, size_t l) const { return xlslice(o, l); }
        XString opslice(size_t b, size_t e) const { return xpslice(b, e); }

        operator FString()
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator TString()
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator SString()
        { if (_base) return SString(*_base, &*_b - &*_base->begin(), &*_e - &*_base->begin()); else return FString(_b, _e); }
        operator ZString() = delete;
    };

    template<uint8_t n>
    class VString : public _crtp_string<VString<n>>
    {
        char _data[n];
        unsigned char _special;
        typedef typename _crtp_string<VString<n>>::iterator iterator;
    public:
        static_assert(n & 1, "Size should probably be odd.");
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
        VString()
        {
            *this = XString();
        }
        // hopefully this is obvious
        iterator begin() const { return std::begin(_data); }
        iterator end() const { return std::end(_data) - _special; }
        const FString *base() const { return nullptr; };
        const char *c_str() const { return &*begin(); }

        VString oslice_t(size_t o) const { return this->xslice_t(o); }
        VString oslice_h(size_t o) const { return this->xslice_h(o); }
        VString orslice_t(size_t no) const { return this->xrslice_t(no); }
        VString orslice_h(size_t no) const { return this->xrslice_h(no); }
        VString olslice(size_t o, size_t l) const { return this->xlslice(o, l); }
        VString opslice(size_t b, size_t e) const { return this->xpslice(b, e); }

        operator FString() const { return FString(begin(), end()); }
        operator TString() const { return FString(begin(), end()); }
        operator SString() const { return FString(begin(), end()); }
        operator ZString() const { return ZString(_data); }
        operator XString() const { return XString(&*begin(), &*end(), nullptr); }
    };


    // not really intended for public use
    inline
    int xstr_compare(XString l, XString r)
    {
        return std::lexicographical_compare(
                l.begin(), l.end(),
                r.begin(), r.end());
    }

    template<class L, class R>
    bool operator == (const L& l, const R& r)
    {
        return xstr_compare(l, r) == 0;
    }
    template<class L, class R>
    bool operator != (const L& l, const R& r)
    {
        return xstr_compare(l, r) != 0;
    }
    template<class L, class R>
    bool operator < (const L& l, const R& r)
    {
        return xstr_compare(l, r) < 0;
    }
    template<class L, class R>
    bool operator <= (const L& l, const R& r)
    {
        return xstr_compare(l, r) <= 0;
    }
    template<class L, class R>
    bool operator > (const L& l, const R& r)
    {
        return xstr_compare(l, r) > 0;
    }
    template<class L, class R>
    bool operator >= (const L& l, const R& r)
    {
        return xstr_compare(l, r) >= 0;
    }


    // sadness
    typedef MString MS;
    typedef FString FS;
    typedef SString SS;
    typedef TString TS;
    typedef ZString ZS;
    typedef XString XS;

    // _crtp_string
    template<class T>
    XS _crtp_string<T>::xslice_t(size_t o) const
    { return XS(&begin()[o], &*end(), base()); }
    template<class T>
    XS _crtp_string<T>::xslice_h(size_t o) const
    { return XS(&*begin(), &begin()[o], base()); }
    template<class T>
    XS _crtp_string<T>::xrslice_t(size_t no) const
    { return XS(&end()[-no], &*end(), base()); }
    template<class T>
    XS _crtp_string<T>::xrslice_h(size_t no) const
    { return XS(&*begin(), &end()[-no], base()); }
    template<class T>
    XS _crtp_string<T>::xlslice(size_t o, size_t l) const
    { return XS(&begin()[o], &begin()[o + l], base()); }
    template<class T>
    XS _crtp_string<T>::xpslice(size_t b, size_t e) const
    { return XS(&begin()[b], &begin()[e], base()); }
    template<class T>
    bool _crtp_string<T>::startswith(XS x) const
    { return size() > x.size() && xslice_h(x.size()) == x; }
    template<class T>
    bool _crtp_string<T>::endswith(XS x) const
    { return size() > x.size() && xrslice_t(x.size()) == x; }

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
    SS FS::olslice(size_t o, size_t l) const
    { return SS(*this, o, o + l); }
    inline
    SS FS::opslice(size_t b, size_t e) const
    { return SS(*this, b, e); }

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
    SS TS::olslice(size_t o, size_t l) const
    { return SS(_s, _o + o, _o + o + l); }
    inline
    SS TS::opslice(size_t b, size_t e) const
    { return SS(_s, _o + b, _o + e); }

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
    SS SS::olslice(size_t o, size_t l) const
    { return SS(_s, _b + o, _b + o + l); }
    inline
    SS SS::opslice(size_t b, size_t e) const
    { return SS(_s, _b + b, _b + e); }

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
    XS ZS::olslice(size_t o, size_t l) const
    { return XS(&begin()[o], &begin()[o + l], base()); }
    inline
    XS ZS::opslice(size_t b, size_t e) const
    { return XS(&begin()[b], &begin()[e], base()); }


    // cxxstdio helpers
    // I think the conversion will happen automatically. TODO test this.
    // Nope, it doesn't, since there's a template
    inline
    const char *convert_for_printf(const FString& fs) { return fs.c_str(); }
    inline
    const char *convert_for_printf(const TString& ts) { return ts.c_str(); }
    inline
    const char *convert_for_printf(const ZString& zs) { return zs.c_str(); }
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
