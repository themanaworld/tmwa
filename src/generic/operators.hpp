#ifndef TMWA_GENERIC_OPERATORS_HPP
#define TMWA_GENERIC_OPERATORS_HPP

namespace _operators
{
    class Comparable {};

    template<class T>
    bool operator == (T l, T r)
    {
        return l.value == r.value;
    }

    template<class T>
    bool operator != (T l, T r)
    {
        return l.value != r.value;
    }

    template<class T>
    bool operator < (T l, T r)
    {
        return l.value < r.value;
    }

    template<class T>
    bool operator <= (T l, T r)
    {
        return l.value <= r.value;
    }

    template<class T>
    bool operator > (T l, T r)
    {
        return l.value > r.value;
    }

    template<class T>
    bool operator >= (T l, T r)
    {
        return l.value >= r.value;
    }
}

using _operators::Comparable;

#endif // TMWA_GENERIC_OPERATORS_HPP
