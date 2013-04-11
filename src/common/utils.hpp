#ifndef UTILS_HPP
#define UTILS_HPP

#include "sanity.hpp"

#include <cstdio>
#include <cstring>

#include <string>

#include "const_array.hpp"
#include "operators.hpp"
#include "utils2.hpp"

/*
Notes about memory allocation in tmwAthena:
There used to be 3 sources of allocation: these macros,
a{C,M,Re}alloc in common/malloc.{h,c}, and direct calls.
I deleted malloc.{h,c} because it was redundant;
future calls should either use this or depend on the coming segfault.
*/
template<class T>
void create_impl(T *& result, size_t number)
{
    result = (T *)calloc(number, sizeof(T));
    if (!result && number)
    {
        perror("SYSERR: malloc failure");
        abort();
    }
}
template<class T>
void recreate_impl(T *& result, size_t number)
{
    result = (T *)realloc(result, sizeof(T) * number);
    if (!result && number)
    {
        perror("SYSERR: realloc failure");
        abort();
    }
}

# define CREATE(result, type, number) \
    create_impl<type>(result, number)

# define RECREATE(result, type, number) \
    recreate_impl<type>(result, number)

int remove_control_chars(char *str);
int e_mail_check(const char *email);
int config_switch (const char *str);
const char *ip2str(struct in_addr ip, bool extra_dot = false);

bool split_key_value(const std::string& line, std::string *w1, std::string *w2);

inline
void strzcpy(char *dest, const char *src, size_t n)
{
    if (n)
    {
        strncpy(dest, src, n);
        dest[n-1] = '\0';
    }
}

// Exists in place of time_t, to give it a predictable printf-format.
// (on x86 and amd64, time_t == long, but not on x32)
static_assert(sizeof(long long) >= sizeof(time_t), "long long >= time_t");
struct TimeT : Comparable
{
    long long value;

    // conversion
    TimeT(time_t t=0) : value(t) {}
    TimeT(struct tm t) : value(timegm(&t)) {}
    operator time_t() { return value; }
    operator struct tm() { time_t v = value; return *gmtime(&v); }

    explicit operator bool() { return value; }
    bool operator !() { return !value; }

    // prevent surprises
    template<class T>
    TimeT(T) = delete;
    template<class T>
    operator T() = delete;

    static
    TimeT now()
    {
        // poisoned, but this is still in header-land
        return time(NULL);
    }

    bool error()
    {
        return value == -1;
    }
    bool okay()
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

typedef char timestamp_seconds_buffer[20];
typedef char timestamp_milliseconds_buffer[24];
void stamp_time(timestamp_seconds_buffer&, TimeT *t=nullptr);
void stamp_time(timestamp_milliseconds_buffer&);

void log_with_timestamp(FILE *out, const_string line);

#define TIMESTAMP_DUMMY "YYYY-MM-DD HH:MM:SS"
static_assert(sizeof(TIMESTAMP_DUMMY) == sizeof(timestamp_seconds_buffer),
        "timestamp size");
#define WITH_TIMESTAMP(str) str TIMESTAMP_DUMMY
//  str:            prefix: YYYY-MM-DD HH:MM:SS
//  sizeof:        01234567890123456789012345678
//  str + sizeof:                               ^
//  -1:                     ^
#define REPLACE_TIMESTAMP(str, t)                           \
    stamp_time(                                             \
            reinterpret_cast<timestamp_seconds_buffer *>(   \
                str + sizeof(str)                           \
            )[-1],                                          \
            &t                                              \
    )

#endif //UTILS_HPP
