#ifndef TMWA_COMPAT_MEMORY_HPP
#define TMWA_COMPAT_MEMORY_HPP

# include "../sanity.hpp"

# include <memory>


template<class T>
struct is_array_of_unknown_bound
: std::is_same<T, typename std::remove_extent<T>::type[]>
{};

template<class T, class D=std::default_delete<T>, class... A>
typename std::enable_if<!is_array_of_unknown_bound<T>::value, std::unique_ptr<T, D>>::type make_unique(A&&... a)
{
    return std::unique_ptr<T, D>(new T(std::forward<A>(a)...));
}

template<class T, class D=std::default_delete<T>>
typename std::enable_if<is_array_of_unknown_bound<T>::value, std::unique_ptr<T, D>>::type make_unique(size_t sz)
{
    typedef typename std::remove_extent<T>::type E;
    return std::unique_ptr<E[], D>(new E[sz]());
}

#endif // TMWA_COMPAT_MEMORY_HPP
