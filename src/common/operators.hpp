#ifndef OPERATORS_HPP
#define OPERATORS_HPP

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

#endif // OPERATORS_HPP
