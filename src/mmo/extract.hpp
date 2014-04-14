#ifndef TMWA_MMO_EXTRACT_HPP
#define TMWA_MMO_EXTRACT_HPP
//    extract.hpp - a simple, hierarchical, tokenizer
//
//    Copyright © 2012-2013 Ben Longbons <b.r.longbons@gmail.com>
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

# include <cerrno>
# include <cstdlib>

# include <algorithm>
# include <vector>

# include "../strings/xstring.hpp"

# include "mmo.hpp"
# include "utils.hpp"

template<class T, typename=typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, char>::value && !std::is_same<T, bool>::value>::type>
bool extract(XString str, T *iv)
{
    if (!str || str.size() > 20)
        return false;
    if (!((str.front() == '-' && std::is_signed<T>::value)
            || ('0' <= str.front() && str.front() <= '9')))
        return false;
    // needs a NUL, but can't always be given one. TODO VString?
    char buf[20 + 1];
    std::copy(str.begin(), str.end(), buf);
    buf[str.size()] = '\0';

    char *end;
    errno = 0;
    if (std::is_signed<T>::value)
    {
        long long v = strtoll(buf, &end, 10);
        if (errno || *end)
            return false;
        *iv = v;
        return *iv == v;
    }
    else
    {
        unsigned long long v = strtoull(buf, &end, 10);
        if (errno || *end)
            return false;
        *iv = v;
        return *iv == v;
    }
}

inline
bool extract(XString str, TimeT *tv)
{
    return extract(str, &tv->value);
}

// extra typename=void to workaround some duplicate overload rule
template<class T, typename=typename std::enable_if<std::is_enum<T>::value>::type, typename=void>
bool extract(XString str, T *iv)
{
    typedef typename underlying_type<T>::type U;
    U v;
    // defer to integer version
    if (!extract(str, &v))
        return false;
    // TODO check bounds using enum min/max as in SSCANF
    *iv = static_cast<T>(v);
    return true;
}

bool extract(XString str, XString *rv);

bool extract(XString str, AString *rv);

template<uint8_t N>
bool extract(XString str, VString<N> *out)
{
    if (str.size() > N)
        return false;
    *out = str;
    return true;
}

template<class T>
class LStripper
{
public:
    T impl;
};

template<class T>
LStripper<T> lstripping(T v)
{
    return {v};
}

template<class T>
bool extract(XString str, LStripper<T> out)
{
    return extract(str.lstrip(), out.impl);
}

// basically just a std::tuple
// but it provides its data members publically
template<char split, int n, class... T>
class Record;
template<char split, int n>
class Record<split, n>
{
};
template<char split, int n, class F, class... R>
class Record<split, n, F, R...>
{
public:
    F frist;
    Record<split, n - 1, R...> rest;
public:
    Record(F f, R... r)
    : frist(f), rest(r...)
    {}
};
template<char split, class... T>
Record<split, sizeof...(T), T...> record(T... t)
{
    return Record<split, sizeof...(T), T...>(t...);
}
template<char split, int n, class... T>
Record<split, n, T...> record(T... t)
{
    static_assert(0 < n && n < sizeof...(T), "don't be silly");
    return Record<split, n, T...>(t...);
}

template<char split, int n>
bool extract(XString str, Record<split, n>)
{
    return !str;
}

template<char split, int n, class F, class... R>
bool extract(XString str, Record<split, n, F, R...> rec)
{
    XString::iterator s = std::find(str.begin(), str.end(), split);
    XString::iterator s2 = s;
    if (s2 != str.end())
        ++s2;
    XString head = str.xislice_h(s);
    XString tail = str.xislice_t(s2);
    if (s == str.end())
        return (extract(head, rec.frist) && n <= 1)
            || (!head && n <= 0);

    return (extract(head, rec.frist) || n <= 0)
        && extract(tail, rec.rest);
}

template<char split, class T>
struct VRecord
{
    std::vector<T> *arr;
};

template<char split, class T>
VRecord<split, T> vrec(std::vector<T> *arr)
{
    return {arr};
}

template<char split, class T>
bool extract(XString str, VRecord<split, T> rec)
{
    if (!str)
        return true;
    XString::iterator s = std::find(str.begin(), str.end(), split);
    rec.arr->emplace_back();
    if (s == str.end())
        return extract(str, &rec.arr->back());
    return extract(str.xislice_h(s), &rec.arr->back())
        && extract(str.xislice_t(s + 1), rec);
}

bool extract(XString str, struct global_reg *var);

bool extract(XString str, struct item *it);

inline
bool extract(XString str, MapName *m)
{
    XString::iterator it = std::find(str.begin(), str.end(), '.');
    str = str.xislice_h(it);
    VString<15> tmp;
    bool rv = extract(str, &tmp);
    *m = tmp;
    return rv;
}

inline
bool extract(XString str, CharName *out)
{
    VString<23> tmp;
    if (extract(str, &tmp))
    {
        *out = CharName(tmp);
        return true;
    }
    return false;
}

#endif // TMWA_MMO_EXTRACT_HPP
