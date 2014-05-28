#ifndef TMWA_COMPAT_ITER_HPP
#define TMWA_COMPAT_ITER_HPP
//    iter.hpp - tools for dealing with iterators
//
//    Copyright © 2012-2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

# include "fwd.hpp"

# include <iterator>


/// Simple class to use a pair of iterators with foreach
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

template<class T>
class PassthroughMath
{
public:
    static
    T inced(T v) { return ++v; }
};

// An iterator that just directly contains an integer-like value
// TODO port this once the new integer API happens
template<class T, class Math=PassthroughMath<T>>
class ValueIterator
{
    T value;
public:
    typedef std::forward_iterator_tag iterator_category;
    typedef void difference_type;
    typedef T value_type;
    typedef void reference;
    typedef void pointer;
public:
    ValueIterator(T v)
    : value(v)
    {}

    T operator *()
    {
        return value;
    }
    ValueIterator& operator++ ()
    {
        value = Math::inced(value);
        return *this;
    }
    friend bool operator == (ValueIterator l, ValueIterator r)
    {
        return l.value == r.value;
    }
    friend bool operator != (ValueIterator l, ValueIterator r)
    {
        return !(l == r);
    }
};

template<class T>
IteratorPair<ValueIterator<T>> value_range(T b, T e)
{
    return {b, e};
}


template<class T, class F, class C>
class FilterIterator
{
    F filter;
    C *container;

    using InnerIterator = decltype(std::begin(*container));
    InnerIterator impl;
public:
    void post_adv()
    {
        while (impl != std::end(*container))
        {
            if (filter(*impl))
                break;
            ++impl;
        }
    }

    FilterIterator(C *c, F f)
    : filter(f), container(c), impl(std::begin(*c))
    {
        post_adv();
    }

    void operator ++()
    {
        ++impl;
        post_adv();
    }

    T operator *()
    {
        return *impl;
    }

    friend
    bool operator != (FilterIterator l, FilterIterator)
    {
        return l.impl != std::end(*l.container);
    }
};

template<class T>
bool is_truthy(T v)
{
    return v;
}

template<class T, class F=decltype(is_truthy<T>)*, class C>
IteratorPair<FilterIterator<T, F, C>> filter_iterator(C *c, F f=is_truthy<T>)
{
    return {FilterIterator<T, F, C>(c, f), FilterIterator<T, F, C>(c, f)};
}

#endif // TMWA_COMPAT_ITER_HPP
