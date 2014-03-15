#ifndef UTILS2_HPP
#define UTILS2_HPP

# include "../sanity.hpp"

# include <algorithm>
# include <functional>
# include <iterator>
# include <memory>
# include <type_traits>

# include "iter.hpp"

# ifdef __clang__
#  define FALLTHROUGH [[clang::fallthrough]]
# else
#  define FALLTHROUGH /* fallthrough */
# endif

template<class T, class E, E max>
struct earray
{
    // no ctor/dtor and one public member variable for easy initialization
    T _data[size_t(max)];

    T& operator[](E v)
    {
        return _data[size_t(v)];
    }

    const T& operator[](E v) const
    {
        return _data[size_t(v)];
    }

    T *begin()
    {
        return _data;
    }

    T *end()
    {
        return _data + size_t(max);
    }

    const T *begin() const
    {
        return _data;
    }

    const T *end() const
    {
        return _data + size_t(max);
    }

    friend bool operator == (const earray& l, const earray& r)
    {
        return std::equal(l.begin(), l.end(), r.begin());
    }

    friend bool operator != (const earray& l, const earray& r)
    {
        return !(l == r);
    }
};

template<class T, class E>
class eptr
{
    T *_data;
public:
    eptr(decltype(nullptr)=nullptr)
    : _data(nullptr)
    {}

    template<E max>
    eptr(earray<T, E, max>& arr)
    : _data(arr._data)
    {}

    T& operator [](E v)
    {
        return _data[size_t(v)];
    }

    explicit operator bool()
    {
        return _data;
    }

    bool operator not()
    {
        return not _data;
    }
};

// std::underlying_type isn't supported until gcc 4.7
// this is a poor man's emulation
template<class E>
struct underlying_type
{
    static_assert(std::is_enum<E>::value, "Only enums have underlying type!");
    typedef typename std::conditional<
        std::is_signed<E>::value,
        typename std::make_signed<E>::type,
        typename std::make_unsigned<E>::type
    >::type type;
};

template<class E, bool=std::is_enum<E>::value>
struct remove_enum
{
    typedef E type;
};
template<class E>
struct remove_enum<E, true>
{
    typedef typename underlying_type<E>::type type;
};


# define ENUM_BITWISE_OPERATORS(E)      \
inline                                  \
E operator & (E l, E r)                 \
{                                       \
    typedef underlying_type<E>::type U; \
    return E(U(l) & U(r));              \
}                                       \
inline                                  \
E operator | (E l, E r)                 \
{                                       \
    typedef underlying_type<E>::type U; \
    return E(U(l) | U(r));              \
}                                       \
inline                                  \
E operator ^ (E l, E r)                 \
{                                       \
    typedef underlying_type<E>::type U; \
    return E(U(l) ^ U(r));              \
}                                       \
inline                                  \
E& operator &= (E& l, E r)              \
{                                       \
    return l = l & r;                   \
}                                       \
inline                                  \
E& operator |= (E& l, E r)              \
{                                       \
    return l = l | r;                   \
}                                       \
inline                                  \
E& operator ^= (E& l, E r)              \
{                                       \
    return l = l ^ r;                   \
}                                       \
inline                                  \
E operator ~ (E r)                      \
{                                       \
    return E(-1) ^ r;                   \
}

template<class E>
class EnumMath
{
    typedef typename underlying_type<E>::type U;
public:
    static
    E inced(E v)
    {
        return E(U(v) + 1);
    }
};

template<class E>
IteratorPair<ValueIterator<E, EnumMath<E>>> erange(E b, E e)
{
    return {b, e};
}

namespace ph = std::placeholders;

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

#endif // UTILS2_HPP
