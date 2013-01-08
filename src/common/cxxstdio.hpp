#ifndef CXXSTDIO_HPP
#define CXXSTDIO_HPP
//    cxxstdio.hpp - pass C++ types through scanf/printf
//
//    Copyright © 2011-2012 Ben Longbons <b.r.longbons@gmail.com>
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

#include "utils2.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <string>

#include "const_array.hpp"

namespace cxxstdio
{
    inline __attribute__((format(printf, 2, 0)))
    int do_vprint(FILE *out, const char *fmt, va_list ap)
    {
        return vfprintf(out, fmt, ap);
    }

    inline __attribute__((format(printf, 2, 0)))
    int do_vprint(std::string& out, const char *fmt, va_list ap)
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

        out = buffer;
        return len;
    }

    inline __attribute__((format(scanf, 2, 0)))
    int do_vscan(FILE *in, const char *fmt, va_list ap)
    {
        return vfscanf(in, fmt, ap);
    }

#if 0
    inline __attribute__((format(scanf, 2, 0)))
    int do_vscan(const char *in, const char *fmt, va_list ap)
    {
        return vsscanf(in, fmt, ap);
    }
#else
    int do_vscan(const char *, const char *, va_list) = delete;
#endif

    inline __attribute__((format(scanf, 2, 0)))
    int do_vscan(const std::string& in, const char *fmt, va_list ap)
    {
        return vsscanf(in.c_str(), fmt, ap);
    }


    template<class T>
    inline __attribute__((format(printf, 2, 3)))
    int do_print(T&& t, const char *fmt, ...) throw()
    {
        int rv;
        va_list ap;
        va_start(ap, fmt);
        rv = do_vprint(std::forward<T>(t), fmt, ap);
        va_end(ap);
        return rv;
    }

    template<class T>
    inline __attribute__((format(scanf, 2, 3)))
    int do_scan(T&& t, const char *fmt, ...) throw()
    {
        int rv;
        va_list ap;
        va_start(ap, fmt);
        rv = do_vscan(std::forward<T>(t), fmt, ap);
        va_end(ap);
        return rv;
    }


    template<class T>
    typename remove_enum<T>::type convert_for_printf(T v)
    {
        typedef typename remove_enum<T>::type repr_type;
        return repr_type(v);
    }

    template<class T, typename = typename std::enable_if<!std::is_enum<T>::value>::type>
    T& convert_for_scanf(T& v)
    {
        return v;
    }

#if 0
    template<class E>
    constexpr
    E get_enum_min_value(decltype(E::min_value))
    {
        return E::min_value;
    }
    template<class E>
    constexpr
    E get_enum_min_value(E def)
    {
        return def;
    }

    template<class E>
    constexpr
    E get_enum_max_value(decltype(E::max_value))
    {
        return E::max_value;
    }
    template<class E>
    constexpr
    E get_enum_max_value(E def)
    {
        return def;
    }
#else
    template<class E>
    constexpr
    E get_enum_min_value(E)
    {
        return E::min_value;
    }
    template<class E>
    constexpr
    E get_max_value(E)
    {
        return E::max_value;
    }
#endif

    template<class E>
    class EnumConverter
    {
        E& out;
        typedef typename underlying_type<E>::type U;
#if 0
        constexpr static
        U min_value = U(get_enum_min_value<E>(E(std::numeric_limits<U>::min())));
        constexpr static
        U max_value = U(get_enum_max_value<E>(E(std::numeric_limits<U>::max())));
#else
        constexpr static
        U min_value = U(get_enum_min_value(E()));
        constexpr static
        U max_value = U(get_enum_max_value(E()));
#endif
        U mid;
    public:
        EnumConverter(E& e)
        : out(e), mid(0)
        {}
        ~EnumConverter()
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
            if (min_value <= mid && mid <= max_value)
#pragma GCC diagnostic pop
                out = E(mid);
        }
        U *operator &()
        {
            return &mid;
        }
    };

    template<class T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
    EnumConverter<T> convert_for_scanf(T& v)
    {
        return v;
    }


    inline
    const char *convert_for_printf(const std::string& s)
    {
        return s.c_str();
    }

    class StringConverter
    {
        std::string& out;
        char *mid;
    public:
        StringConverter(std::string& s)
        : out(s), mid(nullptr)
        {}
        ~StringConverter()
        {
            if (mid)
            {
                out = mid;
                free(mid);
            }
        }
        char **operator &()
        {
            return &mid;
        }
    };

    inline
    StringConverter convert_for_scanf(std::string& s)
    {
        return StringConverter(s);
    }

    template<class Format>
    class PrintFormatter
    {
    public:
        template<class T, class... A>
        static
        int print(T&& t, A&&... a)
        {
            constexpr static
            const char *print_format = Format::print_format();
            return do_print(std::forward<T>(t), print_format,
                    convert_for_printf(std::forward<A>(a))...);
        }
    };

    template<class Format>
    class ScanFormatter
    {
    public:
        template<class T, class... A>
        static
        int scan(T&& t, A&&... a)
        {
            constexpr static
            const char *scan_format = Format::scan_format();
            return do_scan(std::forward<T>(t), scan_format,
                    &convert_for_scanf(*a)...);
        }
    };

#define FPRINTF(file, fmt, args...)                                     \
    ([&]() -> int                                                       \
    {                                                                   \
        struct format_impl                                              \
        {                                                               \
            constexpr static                                            \
            const char *print_format() { return fmt; }                  \
        };                                                              \
        return cxxstdio::PrintFormatter<format_impl>::print(file, ## args);\
    }())

#define FSCANF(file, fmt, args...)                                      \
    ([&]() -> int                                                       \
    {                                                                   \
        struct format_impl                                              \
        {                                                               \
            constexpr static                                            \
            const char *scan_format() { return fmt; }                   \
        };                                                              \
        return cxxstdio::ScanFormatter<format_impl>::scan(file, ## args);  \
    }())

#define PRINTF(fmt, args...)            FPRINTF(stdout, fmt, ## args)
#define SPRINTF(str, fmt, args...)      FPRINTF(str, fmt, ## args)
#define SCANF(fmt, args...)             FSCANF(stdin, fmt, ## args)
#define SSCANF(str, fmt, args...)       FSCANF(str, fmt, ## args)

#define STRPRINTF(fmt, args...)         \
    ([&]() -> std::string               \
    {                                   \
        std::string _out_impl;          \
        SPRINTF(_out_impl, fmt, ## args);  \
        return _out_impl;               \
    }())

} // namespace cxxstdio

#endif // CXXSTDIO_HPP
