#ifndef TMWA_SEXPR_BIND_HPP
#define TMWA_SEXPR_BIND_HPP
//    bind.hpp - Like std::bind, but with arbitrary arguments.
//
//    Copyright © 2012 Ben Longbons <b.r.longbons@gmail.com>
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

# include <utility>

# include "fwd.hpp"


namespace tmwa
{
namespace sexpr
{
    template<class F, class T>
    struct VariadicBind
    {
        // note: may be lvalue references
        F&& f;
        T&& t;
        template<class... A>
        auto operator()(A&&... a) -> decltype(std::forward<F>(f)(std::forward<T>(t), std::forward<A>(a)...))
        {
            return std::forward<F>(f)(std::forward<T>(t), std::forward<A>(a)...);
        }
    };
    template<class F, class T>
    VariadicBind<F, T> bind_variadic(F&& func, T&& arg1)
    {
        return VariadicBind<F, T>{std::forward<F>(func), std::forward<T>(arg1)};
    }
} // namespace sexpr
} // namespace tmwa

#endif //TMWA_SEXPR_VARIANT_HPP
