#ifndef TMWA_COMPAT_MEMORY_HPP
#define TMWA_COMPAT_MEMORY_HPP
//    memory.hpp - I forget ...
//
//    Copyright © 2013-2014 Ben Longbons <b.r.longbons@gmail.com>
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
