#ifndef TMWA_SEXPR_VARIANT_HPP
#define TMWA_SEXPR_VARIANT_HPP
//    variant.hpp - A single value, multiple type container. Better than boost's.
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

# include <type_traits>
# include <cstdint>
# include <cstddef>
# include <utility>

# include "union.hpp"
# include "void.hpp"

# include "fwd.hpp"


namespace tmwa
{
namespace sexpr
{
# define JOIN(a, b) a##b

# define WITH_VAR(ty, var, expr)                                \
    for (bool JOIN(var, _guard) = true; JOIN(var, _guard); )    \
        for (ty var = expr; JOIN(var, _guard); JOIN(var, _guard) = false)
# define MATCH(expr)                    \
    WITH_VAR(auto&&, _match_var, expr)  \
    switch (tmwa::sexpr::VariantFriend::get_state(_match_var))
# define CASE(ty, var)                                                          \
    break;                                                                      \
    case tmwa::sexpr::VariantFriend::get_state_for<ty, decltype(_match_var)>(): \
    WITH_VAR(ty, var, tmwa::sexpr::VariantFriend::unchecked_get<ty>(_match_var))

    template<class... T>
    class Variant
    {
        static_assert(sizeof...(T), "A variant must not be empty");
    };
    template<class D, class... T>
    class Variant<D, T...>
    {
        constexpr static size_t state_count = 1 + sizeof...(T);

        // simplify things immensely
        friend class VariantFriend;

        typedef Union<D, T...> DataType;
        DataType data;
        size_t state;

        void do_destruct();
        template<class C, class... A>
        void do_construct(A&&... a);
    public:
        Variant();
        ~Variant();

        void reset();
        template<class C, class... A>
        void emplace(A&&... a);

        Variant(const Variant& r);
        Variant(Variant&& r);
        Variant& operator = (const Variant& r);
        Variant& operator = (Variant&& r);

        template<class E>
        Variant(E e)
        {
            do_construct<E, E>(std::move(e));
        }

        template<class E>
        Variant& operator = (E e)
        {
            emplace<E, E>(std::move(e));
            return *this;
        }

        // use these ONLY if only one type makes sense
        // otherwise use apply
        template<class E>
        bool is() const;

        template<class E>
        E *get_if();

        template<class E>
        const E *get_if() const;
    };

    template<class R, class F>
    void apply(R& r, F&& f);
    template<class R, class F, class V1, class... V>
    void apply(R& r, F&& f, V1&& v1, V&&... v);
    template<class F, class... V>
    void apply(Void&& r, F&& f, V&&... v);
} // namespace sexpr
} // namespace tmwa

# include "variant.tcc"

#endif //TMWA_SEXPR_VARIANT_HPP
