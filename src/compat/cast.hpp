#ifndef TMWA_COMPAT_CAST_HPP
#define TMWA_COMPAT_CAST_HPP

# include "../sanity.hpp"

# include <utility>
# include <type_traits>


template<class T>
const T& const_(T& t)
{
    return t;
}

template<class T, class U>
T no_cast(U&& u)
{
    typedef typename std::remove_reference<T>::type Ti;
    typedef typename std::remove_reference<U>::type Ui;
    typedef typename std::remove_cv<Ti>::type Tb;
    typedef typename std::remove_cv<Ui>::type Ub;
    static_assert(std::is_same<Tb, Ub>::value, "not no cast");
    return std::forward<U>(u);
}

template<class T, class U>
T base_cast(U&& u)
{
    static_assert(std::is_reference<T>::value, "must base cast with references");
    typedef typename std::remove_reference<T>::type Ti;
    typedef typename std::remove_reference<U>::type Ui;
    typedef typename std::remove_cv<Ti>::type Tb;
    typedef typename std::remove_cv<Ui>::type Ub;
    static_assert(std::is_base_of<Tb, Ub>::value, "not base cast");
    return std::forward<U>(u);
}

// use this when e.g. U is an int of unknown size
template<class T, class U>
T maybe_cast(U u)
{
    return u;
}

template<class T, class U>
typename std::remove_pointer<T>::type *sign_cast(U *u)
{
    typedef typename std::remove_pointer<T>::type T_;
    static_assert(sizeof(T_) == sizeof(U), "sign cast must be same size");
    return reinterpret_cast<T_ *>(u);
}

#endif // TMWA_COMPAT_CAST_HPP
