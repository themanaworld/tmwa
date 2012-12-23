// included by utils.hpp as a porting aid.
// Eventually it will be promoted to one or more normal headers.

#include <iterator>
#include <type_traits>

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

template<class It>
class IteratorPair
{
    It _b, _e;
public:
    IteratorPair(It b, It e)
    : _b(b), _e(e)
    {}

    It begin() { return _b; }
    It end() { return _e; }
};

template<class It>
IteratorPair<It> iterator_pair(It b, It e)
{
    return {b, e};
}

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


#define ENUM_BITWISE_OPERATORS(E)               \
inline                                          \
E operator & (E l, E r)                         \
{                                               \
    typedef typename underlying_type<E>::type U;\
    return E(U(l) & U(r));                      \
}                                               \
inline                                          \
E operator | (E l, E r)                         \
{                                               \
    typedef typename underlying_type<E>::type U;\
    return E(U(l) | U(r));                      \
}                                               \
inline                                          \
E operator ^ (E l, E r)                         \
{                                               \
    typedef typename underlying_type<E>::type U;\
    return E(U(l) ^ U(r));                      \
}                                               \
inline                                          \
E& operator &= (E& l, E r)                      \
{                                               \
    return l = l & r;                           \
}                                               \
inline                                          \
E& operator |= (E& l, E r)                      \
{                                               \
    return l = l | r;                           \
}                                               \
inline                                          \
E& operator ^= (E& l, E r)                      \
{                                               \
    return l = l ^ r;                           \
}                                               \
inline                                          \
E operator ~ (E r)                              \
{                                               \
    return E(-1) ^ r;                           \
}

template<class E>
class EnumValueIterator
{
    typedef typename underlying_type<E>::type U;
    E value;
public:
    EnumValueIterator(E v)
    : value(v)
    {}

    E operator *()
    {
      return value;
    }
    EnumValueIterator& operator++ ()
    {
       value = E(U(value) + 1);
       return *this;
    }
    EnumValueIterator& operator-- ()
    {
       value = E(U(value) - 1);
       return *this;
    }
    friend bool operator == (EnumValueIterator l, EnumValueIterator r)
    {
       return l.value == r.value;
    }
    friend bool operator != (EnumValueIterator l, EnumValueIterator r)
    {
       return !(l == r);
    }
};

template<class E>
IteratorPair<EnumValueIterator<E>> erange(E b, E e)
{
    return {b, e};
}

namespace std { namespace placeholders {} }
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
