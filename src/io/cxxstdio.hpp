#pragma once
//    cxxstdio.hpp - pass C++ types through printf
//
//    Copyright © 2011-2013 Ben Longbons <b.r.longbons@gmail.com>
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

#include "fwd.hpp"

#include <cstdarg>
#include <cstdio>

#include "../compat/cast.hpp"

#include "../diagnostics.hpp"


namespace tmwa
{
namespace cxxstdio
{
    // other implementations of do_vprint are injected by ADL.
    inline __attribute__((format(printf, 2, 0)))
    int do_vprint(FILE *out, const char *fmt, va_list ap)
    {
        return vfprintf(out, fmt, ap);
    }

    template<class T>
    inline __attribute__((format(printf, 2, 3)))
    int do_print(T&& t, const char *fmt, ...)
    {
        int rv;
        va_list ap;
        va_start(ap, fmt);
        rv = do_vprint(std::forward<T>(t), fmt, ap);
        va_end(ap);
        return rv;
    }

    template<class T, typename=typename std::enable_if<!std::is_class<T>::value>::type>
    T decay_for_printf(T v)
    {
        return v;
    }

    template<class T, typename=decltype(decay_for_printf(std::declval<T&&>()))>
    T&& convert_for_printf(T&& v)
    {
        return std::forward<T>(v);
    }

    inline
    const char *convert_for_printf(const char *) = delete;

    template<class Format>
    class PrintFormatter
    {
    public:
        template<class T, class... A>
        static
        int print(T&& t, A&&... a)
        {
            constexpr static
            const char *print_format = Format::print_format().format_string();
            return do_print(std::forward<T>(t), print_format,
                    decay_for_printf(convert_for_printf(std::forward<A>(a)))...);
        }
    };

#define XPRINTF(out, fmt, ...)                                              \
    ({                                                                      \
        struct format_impl                                                  \
        {                                                                   \
            constexpr static                                                \
            FormatString print_format() { return fmt; }                     \
        };                                                                  \
        cxxstdio::PrintFormatter<format_impl>::print(out, ## __VA_ARGS__);  \
    })

#define FPRINTF(file, fmt, ...)     XPRINTF(/*no_cast<FILE *>*/(file), fmt, ## __VA_ARGS__)
#define PRINTF(fmt, ...)            FPRINTF(stdout, fmt, ## __VA_ARGS__)
#define SPRINTF(str, fmt, ...)      XPRINTF(base_cast<AString&>(str), fmt, ## __VA_ARGS__)
#define SNPRINTF(str, n, fmt, ...)  XPRINTF(base_cast<VString<n-1>&>(str), fmt, ## __VA_ARGS__)

#define STRPRINTF(fmt, ...)                         \
    ({                                              \
        AString _out_impl;                          \
        SPRINTF(_out_impl, fmt, ## __VA_ARGS__);    \
        _out_impl;                                  \
    })

#define STRNPRINTF(n, fmt, ...)                         \
    ({                                                  \
        VString<n - 1> _out_impl;                       \
        SNPRINTF(_out_impl, n, fmt, ## __VA_ARGS__);    \
        _out_impl;                                      \
    })

} // namespace cxxstdio
} // namespace tmwa
