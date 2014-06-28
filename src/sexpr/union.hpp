#pragma once
//    union.hpp - A (unsafe!) convenience wrapper for classes in unions.
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

#include <utility>
#include <type_traits>
#include <cstddef>
#include <new>

#include "fwd.hpp"


namespace tmwa
{
namespace sexpr
{
    template<class... T>
    class Union;

    template<>
    class Union<>
    {
    public:
        template<class T>
        constexpr static size_t index()
        {
            return -1;
        }
        Union() = default;
        ~Union() = default;
        Union(const Union&) = delete;
        Union& operator = (const Union&) = delete;
    };

    template<class F, class... R>
    class Union<F, R...>
    {
        static_assert(!std::is_const<F>::value, "union elements are not const");
        static_assert(!std::is_volatile<F>::value, "union elements are not volatile");
        static_assert(!std::is_reference<F>::value, "union elements are not references");
        static_assert(Union<R...>::template index<F>() == size_t(-1), "unions do not contain duplicates");
        union Impl
        {
            F first;
            Union<R...> rest;

            Impl() {}
            ~Impl() {}
        } data;
    public:
        template<class T>
        constexpr static size_t index()
        {
            return std::is_same<F, T>::value
                ? 0
                : 1 + Union<R...>::template index<T>()
                ?: -1;
        }
        template<class T>
        void get(T*& p) { data.rest.get(p); }
        void get(F*& p) { p = std::addressof(data.first); }
        template<class T>
        void get(const T*& p) const { data.rest.get(p); }
        void get(const F*& p) const { p = std::addressof(data.first); }

        template<class T>
        T *get() { T *out; get(out); return out; }
        template<class T>
        const T *get() const { const T *out; get(out); return out; }

        Union() = default;
        ~Union() = default;
        Union(const Union&) = delete;
        Union& operator = (const Union&) = delete;

        template<class T, class... A>
        void construct(A&&... a)
        {
            new (get<T>()) T(std::forward<A>(a)...);
        }

        template<class T>
        void destruct()
        {
            get<T>()->~T();
        }
    };
} // namespace sexpr
} // namespace tmwa
