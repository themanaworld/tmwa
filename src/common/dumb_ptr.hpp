#ifndef TMWA_COMMON_DUMB_PTR_HPP
#define TMWA_COMMON_DUMB_PTR_HPP
//    ptr.hpp - temporary new/delete wrappers
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

#include <cstring>

#include <algorithm>

#include "const_array.hpp"

// unmanaged new/delete-able pointer
// should be replaced by std::unique_ptr<T>
template<class T>
class dumb_ptr
{
    template<class U>
    friend class dumb_ptr;
    T *impl;
public:
    explicit
    dumb_ptr(T *p=nullptr)
    : impl(p)
    {}
    template<class U>
    dumb_ptr(dumb_ptr<U> p)
    : impl(p.impl)
    {}
    dumb_ptr(std::nullptr_t)
    : impl(nullptr)
    {}

    void delete_()
    {
        delete impl;
        *this = nullptr;
    }
    template<class... A>
    void new_(A&&... a)
    {
        impl = new T(std::forward<A>(a)...);
    }
    template<class... A>
    static
    dumb_ptr<T> make(A&&... a)
    {
        return dumb_ptr<T>(new T(std::forward<A>(a)...));
    }
    dumb_ptr& operator = (std::nullptr_t)
    {
        impl = nullptr;
        return *this;
    }

    T& operator *() const
    {
        return *impl;
    }
    T *operator->() const
    {
        return impl;
    }

    explicit
    operator bool() const
    {
        return impl;
    }
    bool operator !() const
    {
        return !impl;
    }

    friend bool operator == (dumb_ptr l, dumb_ptr r)
    {
        return l.impl == r.impl;
    }
    friend bool operator != (dumb_ptr l, dumb_ptr r)
    {
        return !(l == r);
    }
};

// unmanaged new/delete-able pointer
// should be replaced by std::unique_ptr<T[]> or std::vector<T>
template<class T>
class dumb_ptr<T[]>
{
    T *impl;
    size_t sz;
public:
    dumb_ptr() : impl(), sz() {}
    dumb_ptr(std::nullptr_t)
    : impl(nullptr), sz(0) {}
    dumb_ptr(T *p, size_t z)
    : impl(p)
    , sz(z)
    {}

    void delete_()
    {
        delete[] impl;
        *this = nullptr;
    }
    void new_(size_t z)
    {
        impl = new T[z]();
        sz = z;
    }
    static
    dumb_ptr<T[]> make(size_t z)
    {
        return dumb_ptr<T[]>(new T[z](), z);
    }
    dumb_ptr& operator = (std::nullptr_t)
    {
        impl = nullptr;
        sz = 0;
        return *this;
    }

    size_t size() const
    {
        return sz;
    }
    void resize(size_t z)
    {
        if (z == sz)
            return;
        T *np = new T[z]();
        // not exception-safe, but we don't have a dtor anyway
        size_t i = std::min(z, sz);
        while (i-->0)
            np[i] = std::move(impl[i]);
        delete[] impl;
        impl = np;
        sz = z;
    }

    T& operator[](size_t i) const
    {
        return impl[i];
    }

    explicit
    operator bool() const
    {
        return impl;
    }
    bool operator !() const
    {
        return !impl;
    }

    friend bool operator == (dumb_ptr l, dumb_ptr r)
    {
        return l.impl == r.impl;
    }
    friend bool operator != (dumb_ptr l, dumb_ptr r)
    {
        return !(l == r);
    }
};

struct dumb_string
{
    dumb_ptr<char[]> impl;

    dumb_string()
    : impl()
    {}
    dumb_string(char *) = delete;
    // copy ctor, copy assign, and dtor are all default

    static dumb_string copy(const char *b, const char *e)
    {
        dumb_string rv;
        rv.impl.new_((e - b) + 1);
        std::copy(b, e, &rv.impl[0]);
        return rv;
    }
    static dumb_string copy(const char *sz)
    {
        return dumb_string::copy(sz, sz + strlen(sz));
    }
    static dumb_string copys(const std::string& s)
    {
        return dumb_string::copy(&*s.begin(), &*s.end());
    }
    static dumb_string copyn(const char *sn, size_t n)
    {
        return dumb_string::copy(sn, sn + strnlen(sn, n));
    }
    static dumb_string copyc(const_string s)
    {
        return dumb_string::copy(s.begin(), s.end());
    }

    static
    dumb_string fake(const char *p)
    {
        dumb_string rv;
        rv.impl = dumb_ptr<char[]>(const_cast<char *>(p), strlen(p));
        return rv;
    }

    dumb_string dup() const
    {
        return dumb_string::copy(&impl[0]);
    }
    void delete_()
    {
        impl.delete_();
    }

    const char *c_str() const
    {
        return &impl[0];
    }

    std::string str() const
    {
        return c_str();
    }

    operator const_string() const
    {
        return const_string(c_str());
    }

    char& operator[](size_t i) const
    {
        return impl[i];
    }

    explicit
    operator bool() const
    {
        return bool(impl);
    }
    bool operator !() const
    {
        return !impl;
    }

#if 0
    friend bool operator == (dumb_string l, dumb_string r)
    {
        return l.impl == r.impl;
    }
    friend bool operator != (dumb_string l, dumb_string r)
    {
        return !(l == r);
    }
#endif
};

namespace operators
{
    inline
    bool operator == (dumb_string l, dumb_string r)
    {
        return strcmp(l.c_str(), r.c_str()) == 0;
    }
    inline
    bool operator != (dumb_string l, dumb_string r)
    {
        return strcmp(l.c_str(), r.c_str()) != 0;
    }
    inline
    bool operator < (dumb_string l, dumb_string r)
    {
        return strcmp(l.c_str(), r.c_str()) < 0;
    }
    inline
    bool operator <= (dumb_string l, dumb_string r)
    {
        return strcmp(l.c_str(), r.c_str()) <= 0;
    }
    inline
    bool operator > (dumb_string l, dumb_string r)
    {
        return strcmp(l.c_str(), r.c_str()) > 0;
    }
    inline
    bool operator >= (dumb_string l, dumb_string r)
    {
        return strcmp(l.c_str(), r.c_str()) >= 0;
    }

    inline
    bool operator == (const char *l, dumb_string r)
    {
        return strcmp(l, r.c_str()) == 0;
    }
    inline
    bool operator != (const char *l, dumb_string r)
    {
        return strcmp(l, r.c_str()) != 0;
    }
    inline
    bool operator < (const char *l, dumb_string r)
    {
        return strcmp(l, r.c_str()) < 0;
    }
    inline
    bool operator <= (const char *l, dumb_string r)
    {
        return strcmp(l, r.c_str()) <= 0;
    }
    inline
    bool operator > (const char *l, dumb_string r)
    {
        return strcmp(l, r.c_str()) > 0;
    }
    inline
    bool operator >= (const char *l, dumb_string r)
    {
        return strcmp(l, r.c_str()) >= 0;
    }

    inline
    bool operator == (dumb_string l, const char *r)
    {
        return strcmp(l.c_str(), r) == 0;
    }
    inline
    bool operator != (dumb_string l, const char *r)
    {
        return strcmp(l.c_str(), r) != 0;
    }
    inline
    bool operator < (dumb_string l, const char *r)
    {
        return strcmp(l.c_str(), r) < 0;
    }
    inline
    bool operator <= (dumb_string l, const char *r)
    {
        return strcmp(l.c_str(), r) <= 0;
    }
    inline
    bool operator > (dumb_string l, const char *r)
    {
        return strcmp(l.c_str(), r) > 0;
    }
    inline
    bool operator >= (dumb_string l, const char *r)
    {
        return strcmp(l.c_str(), r) >= 0;
    }
}

inline
const char *convert_for_printf(dumb_string ds)
{
    return ds.c_str();
}

#endif // TMWA_COMMON_DUMB_PTR_HPP
