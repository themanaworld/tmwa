//    variant.tcc - implementation of inlines and templates in variant.hpp
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

#include <cassert>
#include "bind.hpp"

#include "../diagnostics.hpp"

namespace tmwa
{
namespace sexpr
{
    template<size_t v>
    constexpr
    size_t not_negative_one()
    {
        return v;
    }
    template<>
    constexpr
    size_t not_negative_one<static_cast<size_t>(-1)>() = delete;

    class VariantFriend
    {
    public:
        template<class U, class... T, class... A>
        static void unchecked_do_construct(Variant<T...> *var, A&&... a)
        {
            var->template do_construct<U, A...>(std::forward<A>(a)...);
        }
        template<class E, class... T>
        static E& unchecked_get(Variant<T...>& var)
        {
            return *var.data.template get<E>();
        }
        template<class E, class... T>
        static const E& unchecked_get(const Variant<T...>& var)
        {
            return *var.data.template get<E>();
        }
        template<class E, class... T>
        static E&& unchecked_get(Variant<T...>&& var)
        {
            return std::move(*var.data.template get<E>());
        }
        template<class E, class... T>
        static const E&& unchecked_get(const Variant<T...>&& var)
        {
            return std::move(*var.data.template get<E>());
        }

        template<class E, class R, class F, class V1, class... V>
        static void _apply_unchecked(R& r, F&& f, V1&& v1, V&&... v)
        {
            apply(r, bind_variadic(std::forward<F>(f), VariantFriend::unchecked_get<E>(std::forward<V1>(v1))), std::forward<V>(v)...);
        }

        template<class... T, class R, class F, class V1, class... V>
        static void _apply_dispatch(const Variant<T...> *, R& r, F&& f, V1&& v1, V&&... v)
        {
            typedef void (*Function)(R&, F&&, V1&&, V&&...);
            constexpr static Function dispatch[sizeof...(T)] = { _apply_unchecked<T, R, F, V1, V...>... };
            assert(v1.state < sizeof...(T));
            dispatch[v1.state](r, std::forward<F>(f), std::forward<V1>(v1), std::forward<V>(v)...);
        }

        template<class... T>
        static size_t get_state(const Variant<T...>& variant)
        {
            return variant.state;
        }

        template<class W, class V>
        constexpr static size_t get_state_for()
        {
            return not_negative_one<std::remove_reference<V>::type::DataType::template index<W>()>();
        }
    };


    struct Destruct
    {
        template<class U>
        void operator ()(U& v)
        {
            v.~U();
        }
    };

    template<class D, class... T>
    void Variant<D, T...>::do_destruct()
    {
        apply(Void(), Destruct(), *this);
    }

    template<class D, class... T>
    template<class C, class... A>
    void Variant<D, T...>::do_construct(A&&... a)
    {
        try
        {
            data.template construct<C, A...>(std::forward<A>(a)...);
            state = not_negative_one<Union<D, T...>::template index<C>()>();
        }
        catch (...)
        {
#if GCC != 407 // apparent compiler bug, not reduced
            // 4.7.2 from wheezy is bad
            // 4.7.3 from jessie is good
            // 4.7.3 from ubuntu-toolchain-test is bad
            static_assert(std::is_nothrow_constructible<D>::value, "first element is nothrow constructible");
#endif
            data.template construct<D>();
            state = 0;
            throw;
        }
    }

    template<class D, class... T>
    Variant<D, T...>::Variant()
    {
        do_construct<D>();
        state = 0;
    }

    template<class D, class... T>
    Variant<D, T...>::~Variant()
    {
        do_destruct();
    }

    template<class D, class... T>
    void Variant<D, T...>::reset()
    {
        do_destruct();
        do_construct<D>();
    }

    template<class D, class... T>
    template<class C, class... A>
    void Variant<D, T...>::emplace(A&&... a)
    {
        do_destruct();
        do_construct<C, A...>(std::forward<A>(a)...);
    }

    template<class... T>
    class CopyConstruct
    {
        Variant<T...> *target;
    public:
        CopyConstruct(Variant<T...> *v) : target(v) {}
        template<class U>
        void operator ()(const U& u)
        {
            VariantFriend::unchecked_do_construct<U>(target, u);
        }
    };
    template<class... T>
    class MoveConstruct
    {
        Variant<T...> *target;
    public:
        MoveConstruct(Variant<T...> *v) : target(v) {}
        template<class U>
        void operator ()(U&& u)
        {
            VariantFriend::unchecked_do_construct<U>(target, std::move(u));
        }
    };

    // assignment requires unchecked access
    template<class... T>
    class CopyAssign
    {
        Union<T...> *data;
    public:
        CopyAssign(Union<T...> *d) : data(d) {}
        template<class U>
        void operator ()(const U& u)
        {
            *data->template get<U>() = u;
        }
    };

    template<class... T>
    class MoveAssign
    {
        Union<T...> *data;
    public:
        MoveAssign(Union<T...> *d) : data(d) {}
        template<class U>
        void operator () (U&& u)
        {
            *data->template get<U>() = std::move(u);
        }
    };

    template<class D, class... T>
    Variant<D, T...>::Variant(const Variant& r)
    {
        apply(Void(), CopyConstruct<D, T...>(this), r);
    }

    template<class D, class... T>
    Variant<D, T...>::Variant(Variant&& r)
    {
        apply(Void(), MoveConstruct<D, T...>(this), std::move(r));
    }

    template<class D, class... T>
    Variant<D, T...>& Variant<D, T...>::operator = (const Variant& r)
    {
        if (state == r.state)
            apply(Void(), CopyAssign<D, T...>(this), r);
        else
        {
            do_destruct();
            apply(Void(), CopyConstruct<D, T...>(this), r);
        }
        return *this;
    }

    template<class D, class... T>
    Variant<D, T...>& Variant<D, T...>::operator = (Variant&& r)
    {
        if (state == r.state)
            apply(Void(), MoveAssign<D, T...>(&data), std::move(r));
        else
        {
            do_destruct();
            apply(Void(), MoveConstruct<D, T...>(this), std::move(r));
        }
        return *this;
    }

    template<class D, class... T>
    template<class E>
    bool Variant<D, T...>::is() const
    {
        return get_if<E>();
    }

    template<class D, class... T>
    template<class E>
    E *Variant<D, T...>::get_if()
    {
        if (state == not_negative_one<Union<D, T...>::template index<E>()>())
            return data.template get<E>();
        return nullptr;
    }

    template<class D, class... T>
    template<class E>
    const E *Variant<D, T...>::get_if() const
    {
        if (state == not_negative_one<Union<D, T...>::template index<E>()>())
            return data.template get<E>();
        return nullptr;
    }

    template<class R, class F>
    void _apply_assign(std::true_type, R& r, F&& f)
    {
        std::forward<F>(f)();
        r = Void();
    }

    template<class R, class F>
    void _apply_assign(std::false_type, R& r, F&& f)
    {
        r = std::forward<F>(f)();
    }

    template<class R, class F>
    void apply(R& r, F&& f)
    {
        _apply_assign(std::is_void<decltype(std::forward<F>(f)())>(), r, std::forward<F>(f));
    }

    template<class R, class F, class V1, class... V>
    void apply(R& r, F&& f, V1&& v1, V&&... v)
    {
        VariantFriend::_apply_dispatch(&v1, r, std::forward<F>(f), std::forward<V1>(v1), std::forward<V>(v)...);
    }

    template<class F, class... V>
    void apply(Void&& r, F&& f, V&&... v)
    {
        apply(r, std::forward<F>(f), std::forward<V>(v)...);
    }

} // namespace sexpr
} // namespace tmwa
