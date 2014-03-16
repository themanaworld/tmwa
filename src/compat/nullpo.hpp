/// return wrappers for unexpected NULL pointers
#ifndef TMWA_COMPAT_NULLPO_HPP
#define TMWA_COMPAT_NULLPO_HPP
/// Uncomment this to live dangerously
/// (really exist to detect mostly-unused variables)
//# define BUG_FREE

/// All functions print to standard error (was: standard output)
/// nullpo_ret(cond) - return 0 if given pointer is NULL
/// nullpo_retv(cond) - just return (function returns void)
/// nullpo_retr(rv, cond) - return given value instead

# ifndef BUG_FREE
#  define nullpo_retr(ret, t)                                   \
    if (nullpo_chk(__FILE__, __LINE__, __PRETTY_FUNCTION__, t)) \
        return ret;
# else // BUG_FREE
#  define nullpo_retr(ret, t) /*t*/
# endif // BUG_FREE

# define nullpo_ret(t) nullpo_retr(0, t)
# define nullpo_retv(t) nullpo_retr(, t)

# include "../sanity.hpp"

/// Used by macros in this header
bool nullpo_chk(const char *file, int line, const char *func,
        const void *target);

template<class T>
bool nullpo_chk(const char *file, int line, const char *func, T target)
{
    return nullpo_chk(file, line, func, target.operator->());
}
template<class T>
bool nullpo_chk(const char *file, int line, const char *func, T *target)
{
    return nullpo_chk(file, line, func, static_cast<const void *>(target));
}

#endif // TMWA_COMPAT_NULLPO_HPP
