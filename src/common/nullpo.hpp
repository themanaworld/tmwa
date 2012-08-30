/// return wrappers for unexpected NULL pointers
#ifndef NULLPO_HPP
#define NULLPO_HPP
/// Comment this out to live dangerously
# define NULLPO_CHECK

/// All functions print to standard error (was: standard output)
/// nullpo_ret(cond) - return 0 if given pointer is NULL
/// nullpo_retv(cond) - just return (function returns void)
/// nullpo_retr(rv, cond) - return given value instead
/// the _f variants take a printf-format string and arguments

# ifdef NULLPO_CHECK
# define NLP_MARK __FILE__, __LINE__, __func__
#  define nullpo_ret(t) \
    if (nullpo_chk(NLP_MARK, t)) \
        return 0;
#  define nullpo_retv(t) \
    if (nullpo_chk(NLP_MARK, t)) \
        return;
#  define nullpo_retr(ret, t) \
    if (nullpo_chk(NLP_MARK, t)) \
        return ret;
#  define nullpo_ret_f(t, fmt, ...) \
    if (nullpo_chk_f(NLP_MARK, t, fmt, ##__VA_ARGS__)) \
        return 0;
#  define nullpo_retv_f(t, fmt, ...) \
    if (nullpo_chk_f(NLP_MARK, t, fmt, ##__VA_ARGS__)) \
        return;
#  define nullpo_retr_f(ret, t, fmt, ...) \
    if (nullpo_chk_f(NLP_MARK, t, fmt, ##__VA_ARGS__)) \
        return ret;
# else // NULLPO_CHECK
#  define nullpo_ret(t) t;
#  define nullpo_retv(t) t;
#  define nullpo_retr(ret, t) t;
#  define nullpo_ret_f(t, fmt, ...) t;
#  define nullpo_retv_f(t, fmt, ...) t;
#  define nullpo_retr_f(ret, t, fmt, ...) t;
# endif // NULLPO_CHECK

# include "sanity.hpp"

/// Used by macros in this header
bool nullpo_chk (const char *file, int line, const char *func,
                 const void *target);

/// Used by macros in this header
bool nullpo_chk_f (const char *file, int line, const char *func,
                   const void *target, const char *fmt, ...)
    __attribute__ ((format (printf, 5, 6)));

/// Used only by map/battle.c
void nullpo_info (const char *file, int line, const char *func);

/// Not used
void nullpo_info_f (const char *file, int line, const char *func,
                    const char *fmt, ...)
    __attribute__ ((format (printf, 4, 5)));

#endif // NULLPO_HPP
