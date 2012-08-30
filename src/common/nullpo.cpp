#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "nullpo.hpp"

static void nullpo_info_core (const char *file, int line, const char *func,
                              const char *fmt, va_list ap);

/// Null check and print format
bool nullpo_chk_f (const char *file, int line, const char *func,
                   const void *target, const char *fmt, ...)
{
    va_list ap;

    if (target)
        return 0;

    va_start (ap, fmt);
    nullpo_info_core (file, line, func, fmt, ap);
    va_end (ap);
    return 1;
}
bool nullpo_chk (const char *file, int line, const char *func,
                 const void *target)
{
    if (target)
        return 0;

    nullpo_info_core (file, line, func, NULL, NULL);
    return 1;
}

/// External functions
void nullpo_info_f (const char *file, int line, const char *func,
                    const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    nullpo_info_core (file, line, func, fmt, ap);
    va_end (ap);
}
void nullpo_info (const char *file, int line, const char *func)
{
    nullpo_info_core (file, line, func, NULL, NULL);
}

/// Actual output function
static void nullpo_info_core (const char *file, int line, const char *func,
                              const char *fmt, va_list ap) __attribute__((format(printf, 4, 0)));
static void nullpo_info_core (const char *file, int line, const char *func,
                              const char *fmt, va_list ap)
{
    if (!file)
        file = "??";
    if (!func || !*func)
        func = "unknown";

    fprintf (stderr, "%s:%d: in func `%s': NULL pointer\n", file, line, func);
    if (fmt && *fmt)
    {
        vfprintf (stderr, fmt, ap);
        if (fmt[strlen (fmt) - 1] != '\n')
            fputc('\n', stderr);
    }
}
