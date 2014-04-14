#ifndef TMWA_MMO_UTILS_HPP
#define TMWA_MMO_UTILS_HPP
//    utils.hpp - Useful stuff that hasn't been categorized.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include <sys/types.h>

# include <cstdio>
# include <ctime>

# include <type_traits>

# include "../strings/fwd.hpp"
# include "../strings/vstring.hpp"

# include "../generic/operators.hpp"

# include "../io/fwd.hpp"

template<class T>
struct is_trivially_copyable
: std::integral_constant<bool,
    // come back when GCC actually implements the public traits properly
    __has_trivial_copy(T)
    && __has_trivial_assign(T)
    && __has_trivial_destructor(T)>
{};

bool e_mail_check(XString email);
int config_switch (ZString str);

template<class T>
void really_memzero_this(T *v)
{
    static_assert(is_trivially_copyable<T>::value, "only for mostly-pod types");
    static_assert(std::is_class<T>::value || std::is_union<T>::value, "Only for user-defined structures (for now)");
    memset(v, '\0', sizeof(*v));
}
template<class T, size_t n>
void really_memzero_this(T (&)[n]) = delete;

// Exists in place of time_t, to give it a predictable printf-format.
// (on x86 and amd64, time_t == long, but not on x32)
static_assert(sizeof(long long) >= sizeof(time_t), "long long >= time_t");
struct TimeT : Comparable
{
    long long value;

    // conversion
    TimeT(time_t t=0) : value(t) {}
    TimeT(struct tm t) : value(timegm(&t)) {}
    operator time_t() const { return value; }
    operator struct tm() const { time_t v = value; return *gmtime(&v); }

    explicit operator bool() const { return value; }
    bool operator !() const { return !value; }

    // prevent surprises
    template<class T>
    TimeT(T) = delete;
    template<class T>
    operator T() const = delete;

    static
    TimeT now()
    {
        // poisoned, but this is still in header-land
        return time(NULL);
    }

    bool error() const
    {
        return value == -1;
    }
    bool okay() const
    {
        return !error();
    }
};

inline
long long convert_for_printf(TimeT t)
{
    return t.value;
}

inline
long long& convert_for_scanf(TimeT& t)
{
    return t.value;
}

struct timestamp_seconds_buffer : VString<19> {};
struct timestamp_milliseconds_buffer : VString<23> {};
void stamp_time(timestamp_seconds_buffer&, const TimeT *t=nullptr);
void stamp_time(timestamp_milliseconds_buffer&);

void log_with_timestamp(io::WriteFile& out, XString line);

// TODO VString?
# define TIMESTAMP_DUMMY "YYYY-MM-DD HH:MM:SS"
static_assert(sizeof(TIMESTAMP_DUMMY) == sizeof(timestamp_seconds_buffer),
        "timestamp size");
# define WITH_TIMESTAMP(str) str TIMESTAMP_DUMMY
//  str:            prefix: YYYY-MM-DD HH:MM:SS
//  sizeof:        01234567890123456789012345678
//  str + sizeof:                               ^
//  -1:                     ^
// there's probably a better way to do this now
# define REPLACE_TIMESTAMP(str, t)                          \
    stamp_time(                                             \
            reinterpret_cast<timestamp_seconds_buffer *>(   \
                str + sizeof(str)                           \
            )[-1],                                          \
            &t                                              \
    )

#endif // TMWA_MMO_UTILS_HPP
