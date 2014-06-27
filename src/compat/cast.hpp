#ifndef TMWA_COMPAT_CAST_HPP
#define TMWA_COMPAT_CAST_HPP
//    cast.hpp - Change the type of a variable.
//
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include <utility>
# include <type_traits>


namespace tmwa
{
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
} // namespace tmwa

#endif // TMWA_COMPAT_CAST_HPP
