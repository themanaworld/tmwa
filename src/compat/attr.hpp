#ifndef TMWA_COMPAT_ATTR_HPP
#define TMWA_COMPAT_ATTR_HPP

# include "../sanity.hpp"


# ifdef __clang__
#  define FALLTHROUGH [[clang::fallthrough]]
# else
#  define FALLTHROUGH /* fallthrough */
# endif

#endif // TMWA_COMPAT_ATTR_HPP
