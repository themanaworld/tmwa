#ifndef TMWA_IO_CXXSTDIO_HPP
#define TMWA_IO_CXXSTDIO_HPP
//    cxxstdio.hpp - pass C++ types through scanf/printf
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

# include "../sanity.hpp"

# include <cstdarg>
# include <cstdio>

// TODO get rid of these header order violations
# include "../common/const_array.hpp"
# include "../common/utils2.hpp"

# include "fwd.hpp"


namespace cxxstdio
{
    // other implementations of do_vprint or do_vscan are injected by ADL.
    inline __attribute__((format(printf, 2, 0)))
    int do_vprint(FILE *out, const char *fmt, va_list ap)
    {
        return vfprintf(out, fmt, ap);
    }

    inline __attribute__((format(scanf, 2, 0)))
    int do_vscan(FILE *in, const char *fmt, va_list ap)
    {
        return vfscanf(in, fmt, ap);
    }

# if 0
    inline __attribute__((format(scanf, 2, 0)))
    int do_vscan(const char *in, const char *fmt, va_list ap)
    {
        return vsscanf(in, fmt, ap);
    }
# else
    inline
    int do_vscan(const char *, const char *, va_list) = delete;
# endif

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


    template<class T, typename=typename std::enable_if<!std::is_class<T>::value>::type>
    typename remove_enum<T>::type decay_for_printf(T v)
    {
        typedef typename remove_enum<T>::type repr_type;
        return repr_type(v);
    }

    template<class T, typename=decltype(decay_for_printf(std::declval<T&&>()))>
    T&& convert_for_printf(T&& v)
    {
        return std::forward<T>(v);
    }

    template<class T, typename = typename std::enable_if<!std::is_enum<T>::value>::type>
    T& convert_for_scanf(T& v)
    {
        return v;
    }

# if 0
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
# else
    template<class E>
    constexpr
    E get_enum_min_value(E)
    {
        return E::min_value;
    }
    template<class E>
    constexpr
    E get_enum_max_value(E)
    {
        return E::max_value;
    }
# endif

    template<class E>
    class EnumConverter
    {
        E& out;
        typedef typename underlying_type<E>::type U;
# if 0
        constexpr static
        U min_value = U(get_enum_min_value<E>(E(std::numeric_limits<U>::min())));
        constexpr static
        U max_value = U(get_enum_max_value<E>(E(std::numeric_limits<U>::max())));
# else
        constexpr static
        U min_value = U(get_enum_min_value(E()));
        constexpr static
        U max_value = U(get_enum_max_value(E()));
# endif
        U mid;
    public:
        EnumConverter(E& e)
        : out(e), mid(0)
        {}
        ~EnumConverter()
        {
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wtype-limits"
            if (min_value <= mid && mid <= max_value)
# pragma GCC diagnostic pop
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
                    decay_for_printf(convert_for_printf(std::forward<A>(a)))...);
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

# define XPRINTF(out, fmt, ...)                                                         \
    (/*[&]() -> int*/                                                                   \
    {                                                                                   \
        struct format_impl                                                              \
        {                                                                               \
            constexpr static                                                            \
            const char *print_format() { return fmt; }                                  \
        };                                                                              \
        /*return*/ cxxstdio::PrintFormatter<format_impl>::print(out, ## __VA_ARGS__);   \
    }/*()*/)

# define XSCANF(out, fmt, ...)                                                      \
    (/*[&]() -> int*/                                                               \
    {                                                                               \
        struct format_impl                                                          \
        {                                                                           \
            constexpr static                                                        \
            const char *scan_format() { return fmt; }                               \
        };                                                                          \
        /*return*/ cxxstdio::ScanFormatter<format_impl>::scan(out, ## __VA_ARGS__); \
    }/*()*/)

# define FPRINTF(file, fmt, ...)     XPRINTF(/*no_cast<FILE *>*/(file), fmt, ## __VA_ARGS__)
# define FSCANF(file, fmt, ...)      XSCANF(no_cast<FILE *>(file), fmt, ## __VA_ARGS__)
# define PRINTF(fmt, ...)            FPRINTF(stdout, fmt, ## __VA_ARGS__)
# define SPRINTF(str, fmt, ...)      XPRINTF(base_cast<FString&>(str), fmt, ## __VA_ARGS__)
# define SNPRINTF(str, n, fmt, ...)  XPRINTF(base_cast<VString<n-1>&>(str), fmt, ## __VA_ARGS__)
# define SCANF(fmt, ...)             FSCANF(stdin, fmt, ## __VA_ARGS__)
# define SSCANF(str, fmt, ...)       XSCANF(/*ZString or compatible*/str, fmt, ## __VA_ARGS__)

# define STRPRINTF(fmt, ...)                        \
    (/*[&]() -> FString*/                           \
    {                                               \
        FString _out_impl;                          \
        SPRINTF(_out_impl, fmt, ## __VA_ARGS__);    \
        /*return*/ _out_impl;                       \
    }/*()*/)

# define STRNPRINTF(n, fmt, ...)                        \
    (/*[&]() -> VString<n - 1>*/                        \
    {                                                   \
        VString<n - 1> _out_impl;                       \
        SNPRINTF(_out_impl, n, fmt, ## __VA_ARGS__);    \
        /*return*/ _out_impl;                           \
    }/*()*/)

} // namespace cxxstdio

#endif // TMWA_IO_CXXSTDIO_HPP
