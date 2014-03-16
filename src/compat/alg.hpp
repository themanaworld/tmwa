#ifndef TMWA_COMPAT_ALG_HPP
#define TMWA_COMPAT_ALG_HPP

# include "../sanity.hpp"

# include <type_traits>


template<class A, class B>
typename std::common_type<A, B>::type min(A a, B b)
{
    return a < b ? a : b;
}

template<class A, class B>
typename std::common_type<A, B>::type max(A a, B b)
{
    return b < a ? a : b;
}

#endif // TMWA_COMPAT_ALG_HPP
