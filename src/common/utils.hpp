#ifndef UTILS_HPP
#define UTILS_HPP

#include "sanity.hpp"

#include <cstdio>
#include <cstring>

#include <type_traits>

#include "const_array.hpp"
#include "operators.hpp"
#include "strings.hpp"
#include "utils2.hpp"

struct IP_String : VString<15> {};

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
IP_String ip2str(struct in_addr ip);
VString<15 + 1> ip2str_extradot(struct in_addr ip);

bool split_key_value(const FString& line, SString *w1, TString *w2);

inline
void really_memcpy(uint8_t *dest, const uint8_t *src, size_t n)
{
    memcpy(dest, src, n);
}

inline
void really_memmove(uint8_t *dest, const uint8_t *src, size_t n)
{
    memmove(dest, src, n);
}
inline
bool really_memequal(const uint8_t *a, const uint8_t *b, size_t n)
{
    return memcmp(a, b, n) == 0;
}

inline
void really_memset0(uint8_t *dest, size_t n)
{
    memset(dest, '\0', n);
}
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

void log_with_timestamp(FILE *out, XString line);

// TODO VString?
#define TIMESTAMP_DUMMY "YYYY-MM-DD HH:MM:SS"
static_assert(sizeof(TIMESTAMP_DUMMY) == sizeof(timestamp_seconds_buffer),
        "timestamp size");
#define WITH_TIMESTAMP(str) str TIMESTAMP_DUMMY
//  str:            prefix: YYYY-MM-DD HH:MM:SS
//  sizeof:        01234567890123456789012345678
//  str + sizeof:                               ^
//  -1:                     ^
// there's probably a better way to do this now
#define REPLACE_TIMESTAMP(str, t)                           \
    stamp_time(                                             \
            reinterpret_cast<timestamp_seconds_buffer *>(   \
                str + sizeof(str)                           \
            )[-1],                                          \
            &t                                              \
    )

#endif //UTILS_HPP
