// included by utils.hpp as a porting aid.
// Eventually it will be promoted to one or more normal headers.

#include <type_traits>
#include <iterator>

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
    It b, e;
public:
    IteratorPair(It b, It e)
    : b(b), e(e)
    {}

    It begin() { return b; }
    It end() { return e; }
};

template<class It>
IteratorPair<It> iterator_pair(It b, It e)
{
    return {b, e};
}

#ifndef HAVE_STD_UNDERLYING_TYPE
// Note: you *must* correctly define/not define this - it conflicts!
namespace std
{
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
}
#endif // HAVE_STD_UNDERLYING_TYPE

template<class E>
class EnumValueIterator
{
    typedef typename std::underlying_type<E>::type U;
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
