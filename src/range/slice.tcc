//    ranges/slice.tcc - Inline functions for strings/base.hpp
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
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

#include <cassert>

// simple pointer-wrapping iterator
template<class T>
class Slice<T>::iterator
{
    typedef iterator X;

    T *_ptr;
public:
    typedef ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T& reference;
    typedef std::random_access_iterator_tag iterator_category;

    iterator(T *p=nullptr) : _ptr(p) {}

    // iterator
    reference operator *() const { return *_ptr; }
    X& operator ++() { ++_ptr; return *this; }
    // equality comparable
    friend bool operator == (X l, X r) { return l._ptr == r._ptr; }
    // input iterator
    friend bool operator != (X l, X r) { return !(l == r); }
    pointer operator->() const { return _ptr; }
    X operator++ (int) { X out = *this; ++*this; return out; }
    // forward iterator is mostly semantical, and the ctor is above
    // bidirectional iterator
    X& operator --() { --_ptr; return *this; }
    X operator-- (int) { X out = *this; --*this; return out; }
    // random access iterator
    X& operator += (difference_type n) { _ptr += n; return *this; }
    friend X operator + (X a, difference_type n) { return a += n; }
    friend X operator + (difference_type n, X a) { return a += n; }
    X& operator -= (difference_type n) { _ptr -= n; return *this; }
    friend X operator - (X a, difference_type n) { return a -= n; }
    friend difference_type operator - (X b, X a) { return b._ptr - a._ptr; }
    reference operator[](difference_type n) const { return _ptr[n]; }
    friend bool operator < (X a, X b) { return a._ptr < b._ptr; }
    friend bool operator > (X a, X b) { return b < a; }
    friend bool operator >= (X a, X b) { return !(a < b); }
    friend bool operator <= (X a, X b) { return !(a > b); }
};

template<class T>
Slice<T>::Slice(std::nullptr_t) : _begin(nullptr), _end(nullptr)
{}

template<class T>
Slice<T>::Slice(T *b, T *e) : _begin(b), _end(e)
{}

template<class T>
Slice<T>::Slice(T *b, size_t l) : _begin(b), _end(b + l)
{}

template<class T>
template<class U, typename>
Slice<T>::Slice(Slice<U> o) : _begin(o.data()), _end(o.data() + o.size())
{}

template<class T>
template<size_t n, typename>
Slice<T>::Slice(T (&arr)[n]) : _begin(arr), _end(arr + n)
{}

template<class T>
Slice<T>::Slice(std::vector<T>& vec) : _begin(&*vec.begin()), _end(&*vec.end())
{}


template<class T>
typename Slice<T>::iterator Slice<T>::begin() const
{
    return _begin;
}

template<class T>
typename Slice<T>::iterator Slice<T>::end() const
{
    return _end;
}

template<class T>
T *Slice<T>::data() const
{
    return _begin;
}

template<class T>
size_t Slice<T>::size() const
{
    return _end - _begin;
}

template<class T>
Slice<T>::operator bool() const
{
    return _begin != _end;
}

template<class T>
bool Slice<T>::operator not() const
{
    return _begin == _end;
}

template<class T>
T& Slice<T>::front() const
{
    return _begin[0];
}

template<class T>
T& Slice<T>::back() const
{
    return _end[-1];
}

template<class T>
T& Slice<T>::pop_front()
{
    ++_begin;
    return _begin[0 - 1];
}

template<class T>
T& Slice<T>::pop_back()
{
    --_end;
    return _end[-1 + 1];
}

template<class T>
T& Slice<T>::operator[](size_t o)
{
    assert (o < size());
    return _begin[o];
}


template<class T>
Slice<T> Slice<T>::slice_t(size_t o) const
{
    return Slice(_begin + o, _end);
}

template<class T>
Slice<T> Slice<T>::slice_h(size_t o) const
{
    return Slice(_begin, _begin + o);
}

template<class T>
Slice<T> Slice<T>::rslice_t(size_t no) const
{
    return Slice(_end - no, _end);
}

template<class T>
Slice<T> Slice<T>::rslice_h(size_t no) const
{
    return Slice(_begin, _end - no);
}

template<class T>
Slice<T> Slice<T>::islice_t(iterator it) const
{
    return Slice(&*it, _end);
}

template<class T>
Slice<T> Slice<T>::islice_h(iterator it) const
{
    return Slice(_begin, &*it);
}

template<class T>
Slice<T> Slice<T>::lslice(size_t o, size_t l) const
{
    return Slice(_begin + o, _begin + o + l);
}

template<class T>
Slice<T> Slice<T>::pslice(size_t b, size_t e) const
{
    return Slice(_begin + b, _begin + e);
}

template<class T>
Slice<T> Slice<T>::islice(iterator b, iterator e) const
{
    return Slice(&*b, &*e);
}
