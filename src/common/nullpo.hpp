/// return wrappers for unexpected NULL pointers
#ifndef NULLPO_HPP
#define NULLPO_HPP
/// Comment this out to live dangerously
# define NULLPO_CHECK

/// All functions print to standard error (was: standard output)
/// nullpo_ret(cond) - return 0 if given pointer is NULL
/// nullpo_retv(cond) - just return (function returns void)
/// nullpo_retr(rv, cond) - return given value instead

# ifdef NULLPO_CHECK
# define NLP_MARK __FILE__, __LINE__, __func__
#  define nullpo_retr(ret, t) \
    if (nullpo_chk(NLP_MARK, t)) \
        return ret;
# else // NULLPO_CHECK
#  define nullpo_retr(ret, t) t;
# endif // NULLPO_CHECK

# define nullpo_ret(t) nullpo_retr(0, t)
# define nullpo_retv(t) nullpo_retr(, t)

# include "sanity.hpp"

/// Used by macros in this header
bool nullpo_chk(const char *file, int line, const char *func,
                 const void *target);

/// Used only by map/battle.c
void nullpo_info(const char *file, int line, const char *func);

#endif // NULLPO_HPP
