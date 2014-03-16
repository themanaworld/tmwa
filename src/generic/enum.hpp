#ifndef TMWA_GENERIC_ENUM_HPP
#define TMWA_GENERIC_ENUM_HPP

# include "../sanity.hpp"

# include <type_traits>

# include "../compat/iter.hpp"

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
    eptr(std::nullptr_t=nullptr)
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

#endif // TMWA_GENERIC_ENUM_HPP
