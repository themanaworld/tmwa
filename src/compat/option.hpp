#pragma once
//    option.hpp - a data type that may or may not exist
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "fwd.hpp"

#include <cassert>

#include <utility>


namespace tmwa
{
namespace option
{
    enum option_hack_type { option_hack_value };

    template<class T>
    class OptionRepr
    {
        __attribute__((aligned(alignof(T))))
        char _data[sizeof(T)];
        bool _some;
    public:
        void set_none() { _some = false; }
        // maybe add pre_set_some if it is useful for other specializations
        void post_set_some() { _some = true; }
        bool is_some() const { return _some; }
        T *ptr() { return reinterpret_cast<T *>(&_data); }
        const T *ptr() const { return reinterpret_cast<const T *>(&_data); }
    };
    template<class T>
    class OptionRepr<T&>;
    template<class T>
    class OptionRepr<T&&>;

    template<class T>
    Option<T> None(option_hack_type=option_hack_value)
    {
        return None;
    }

    template<class T>
    Option<T> Some(T v)
    {
        Option<T> rv = None;
        rv.do_construct(std::move(v));
        return rv;
    }

    // TODO all *_or and *_set methods should have a lazy version too
    template<class T>
    class Option
    {
        static_assert(std::is_pod<OptionRepr<T>>::value, "repr should itself be pod, copies are done manually");
        OptionRepr<T> repr;

        friend Option<T> Some<T>(T);

        void do_init()
        {
            repr.set_none();
        }
        template<class... U>
        void do_construct(U&&... u)
        {
            new(repr.ptr()) T(std::forward<U>(u)...);
            repr.post_set_some();
        }
        void do_move_construct(Option&& r)
        {
            if (r.repr.is_some())
            {
                do_construct(std::move(*r.repr.ptr()));
                r.do_destruct();
            }
        }
        void do_copy_construct(const Option& r)
        {
            if (r.repr.is_some())
            {
                do_construct(*r.repr.ptr());
            }
        }
        void do_move_assign(Option&& r)
        {
            if (repr.is_some())
            {
                if (r.repr.is_some())
                {
                    *repr.ptr() = std::move(*r.repr.ptr());
                }
                else
                {
                    do_destruct();
                }
                return;
            }
            else
            {
                do_move_construct(std::move(r));
            }
        }
        void do_copy_assign(const Option& r)
        {
            if (repr.is_some())
            {
                if (r.repr.is_some())
                {
                    *repr.ptr() = *r.repr.ptr();
                }
                else
                {
                    do_destruct();
                }
                return;
            }
            else
            {
                do_copy_construct(r);
            }
        }
        void do_destruct()
        {
            repr.ptr()->~T();
            repr.set_none();
        }
    public:
        Option() = delete;
        Option(Option(*)(option_hack_type))
        {
            do_init();
        }
        Option(Option&& r)
        {
            do_init();
            do_move_construct(std::move(r));
        }
        Option(const Option& r)
        {
            do_init();
            do_copy_construct(r);
        }
        Option& operator = (Option&& r)
        {
            do_move_assign(std::move(r));
            return *this;
        }
        Option& operator = (const Option& r)
        {
            do_copy_assign(r);
            return *this;
        }
        ~Option()
        {
            if (repr.is_some())
            {
                do_destruct();
            }
        }

        T move_or(T def)
        {
            if (repr.is_some())
            {
                def = std::move(*repr.ptr());
                do_destruct();
            }
            return def;
        }
        T copy_or(T def) const
        {
            if (repr.is_some())
            {
                def = *repr.ptr();
            }
            return def;
        }
        T& ref_or(T& def)
        {
            return repr.is_some() ? *repr.ptr() : def;
        }
        const T& ref_or(const T& def) const
        {
            return repr.is_some() ? *repr.ptr() : def;
        }
        T *ptr_or(T *def)
        {
            return repr.is_some() ? repr.ptr() : def;
        }
        const T *ptr_or(const T *def) const
        {
            return repr.is_some() ? repr.ptr() : def;
        }
        bool is_some() const
        {
            return repr.is_some();
        }
        bool is_none() const
        {
            return !is_some();
        }

        template<class F>
        auto move_map(F&& f) -> Option<decltype(std::forward<F>(f)(std::move(*repr.ptr())))>
        {
            if (repr.is_some())
            {
                auto rv = Some(std::forward<F>(f)(std::move(*repr.ptr())));
                do_destruct();
                return rv;
            }
            else
            {
                return None;
            }
        }
        template<class F>
        auto map(F&& f) -> Option<decltype(std::forward<F>(f)(*repr.ptr()))>
        {
            if (repr.is_some())
            {
                return Some(std::forward<F>(f)(*repr.ptr()));
            }
            else
            {
                return None;
            }
        }
        template<class F>
        auto map(F&& f) const -> Option<decltype(std::forward<F>(f)(*repr.ptr()))>
        {
            if (repr.is_some())
            {
                return Some(std::forward<F>(f)(*repr.ptr()));
            }
            else
            {
                return None;
            }
        }
        // shortcut for flatten(o.map()) that avoids explicit Some's inside
        template<class B, class F>
        auto cmap(B&& b, F&& f) const -> Option<decltype(std::forward<F>(f)(*repr.ptr()))>
        {
            if (repr.is_some() && std::forward<B>(b)(*repr.ptr()))
            {
                return Some(std::forward<F>(f)(*repr.ptr()));
            }
            else
            {
                return None;
            }
        }
        // wanting members is *so* common
        template<class M, class B>
        Option<M> pmd_get(const M B::*pmd) const
        {
            if (repr.is_some())
            {
                return Some((*repr.ptr()).*pmd);
            }
            else
            {
                return None;
            }
        }
        template<class M, class B>
        void pmd_set(M B::*pmd, M value)
        {
            if (repr.is_some())
            {
                ((*repr.ptr()).*pmd) = std::move(value);
            }
        }
        template<class M, class B>
        Option<M> pmd_pget(const M B::*pmd) const
        {
            if (repr.is_some())
            {
                return Some((**repr.ptr()).*pmd);
            }
            else
            {
                return None;
            }
        }
        template<class M, class B>
        void pmd_pset(M B::*pmd, M value)
        {
            if (repr.is_some())
            {
                ((**repr.ptr()).*pmd) = std::move(value);
            }
        }
    };

    template<class T>
    struct most_flattened_type
    {
        using type = T;

        static Option<type> flatten(Option<T> o)
        {
            return std::move(o);
        }
    };
    template<class T>
    struct most_flattened_type<Option<T>>
    {
        using type = typename most_flattened_type<T>::type;

        static Option<type> flatten(Option<Option<T>> o)
        {
            return most_flattened_type<T>::flatten(o.move_or(None));
        }
    };

    template<class T>
    Option<typename most_flattened_type<T>::type> flatten(Option<T> o)
    {
        return most_flattened_type<T>::flatten(std::move(o));
    }

    template<class T>
    bool operator == (const Option<T>& l, const Option<T>& r)
    {
        const T *l2 = l.ptr_or(nullptr);
        const T *r2 = r.ptr_or(nullptr);
        if (!l2 && !r2)
            return true;
        if (l2 && r2)
        {
            return *l2 == *r2;
        }
        return false;
    }
    template<class T>
    bool operator != (const Option<T>& l, const Option<T>& r)
    {
        return !(l == r);
    }
    template<class T>
    bool operator < (const Option<T>& l, const Option<T>& r)
    {
        const T *l2 = l.ptr_or(nullptr);
        const T *r2 = r.ptr_or(nullptr);

        if (!l2 && r2)
            return true;
        if (l2 && r2)
        {
            return *l2 < *r2;
        }
        return false;
    }
    template<class T>
    bool operator > (const Option<T>& l, const Option<T>& r)
    {
        return (r < l);
    }
    template<class T>
    bool operator <= (const Option<T>& l, const Option<T>& r)
    {
        return !(r < l);
    }
    template<class T>
    bool operator >= (const Option<T>& l, const Option<T>& r)
    {
        return !(l < r);
    }

    // workaround for the fact that most references can't escape
    template<class T>
    struct RefWrapper
    {
        T maybe_ref;

        T maybe_ref_fun() { return std::forward<T>(maybe_ref); }
    };

    template<class T>
    RefWrapper<T> option_unwrap(RefWrapper<Option<T>> o)
    { return RefWrapper<T>{std::move(*reinterpret_cast<OptionRepr<T>&>(o.maybe_ref).ptr())}; }
    template<class T>
    RefWrapper<T&> option_unwrap(RefWrapper<Option<T>&> o)
    { return RefWrapper<T&>{*reinterpret_cast<OptionRepr<T>&>(o.maybe_ref).ptr()}; }
    template<class T>
    RefWrapper<T&&> option_unwrap(RefWrapper<Option<T>&&> o)
    { return RefWrapper<T&&>{std::move(*reinterpret_cast<OptionRepr<T>&>(o.maybe_ref).ptr())}; }
    template<class T>
    RefWrapper<T> option_unwrap(RefWrapper<const Option<T>> o)
    { return RefWrapper<T>{std::move(*reinterpret_cast<const OptionRepr<T>&>(o.maybe_ref).ptr())}; }
    template<class T>
    RefWrapper<const T&> option_unwrap(RefWrapper<const Option<T>&> o)
    { return RefWrapper<const T&>{*reinterpret_cast<const OptionRepr<T>&>(o.maybe_ref).ptr()}; }
    template<class T>
    RefWrapper<const T&&> option_unwrap(RefWrapper<const Option<T>&&> o)
    { return RefWrapper<const T&&>{std::move(*reinterpret_cast<const OptionRepr<T>&>(o.maybe_ref).ptr())}; }

    // if you think you understand this, you're not trying hard enough.
#define TRY_UNWRAP(opt, falsy)                                  \
    ({                                                          \
        tmwa::option::RefWrapper<decltype((opt))> o = {(opt)};  \
        if (o.maybe_ref.is_none()) falsy;                       \
        tmwa::option::option_unwrap(std::move(o));              \
    }).maybe_ref_fun()

#define OMATCH_BEGIN(expr)              \
    {                                   \
        auto&& _omatch_var = (expr);    \
        switch (_omatch_var.is_some())  \
        {                               \
            {                           \
                {                       \
    /*}}}}*/
#define OMATCH_END()        \
    /*{{{{*/                \
                }           \
            }               \
        }                   \
        (void) _omatch_var; \
    }

#define OMATCH_BEGIN_SOME(var, expr)    \
    OMATCH_BEGIN (expr)                 \
    OMATCH_CASE_SOME (var)

#define OMATCH_CASE_SOME(var)                           \
    /*{{{{*/                                            \
        }                                               \
        break;                                          \
    }                                                   \
    {                                                   \
        case true:                                      \
        {                                               \
            auto&& var = *_omatch_var.ptr_or(nullptr);  \
    /*}}}}*/
#define OMATCH_CASE_NONE()  \
    /*{{{{*/                \
        }                   \
        break;              \
    }                       \
    {                       \
        case false:         \
        {                   \
    /*}}}}*/
} // namespace option

//using option::Option;
using option::None;
using option::Some;
} // namespace tmwa
