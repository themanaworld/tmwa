#ifndef UTILS_HPP
#define UTILS_HPP

#include "sanity.hpp"

#include <cstdio>
#include <cstring>

#include <string>

#include "const_array.hpp"
#include "utils2.hpp"

/*
Notes about memory allocation in tmwAthena:
There used to be 3 sources of allocation: these macros,
a{C,M,Re}alloc in common/malloc.{h,c}, and direct calls.
I deleted malloc.{h,c} because it was redundant;
future calls should either use this or depend on the coming segfault.
*/
# define CREATE(result, type, number) \
   if (!((result) = (type *) calloc((number), sizeof(type))))   \
      { perror("SYSERR: malloc failure"); abort(); } else (void)0

# define RECREATE(result,type,number) \
  if (!((result) = (type *) realloc((result), sizeof(type) * (number))))\
      { perror("SYSERR: realloc failure"); abort(); } else (void)0

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

typedef char timestamp_seconds_buffer[20];
typedef char timestamp_milliseconds_buffer[24];
void stamp_time(timestamp_seconds_buffer&, time_t *t=nullptr);
void stamp_time(timestamp_milliseconds_buffer&);

void log_with_timestamp(FILE *out, const_string line);

#endif //UTILS_HPP
