#pragma once
//    borrow.hpp - a non-null, unowned, pointer
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

#include <cstdlib>

#include <iterator>

#include "option.hpp"

// unit tests currention in option_test.cpp

namespace tmwa
{
    // TODO see if const-by-default is a thing
    template<class T>
    class Borrowed
    {
        T *stupid;
    public:
        explicit
        Borrowed(T *p) : stupid(p)
        {
            if (!p) abort();
        }

        T& operator *() const
        {
            return *stupid;
        }

        T *operator ->() const
        {
            return stupid;
        }

        template<class U>
        Borrowed<U> downcast_to() const
        {
            static_assert(std::is_base_of<T, U>::value, "base check");
            static_assert(!std::is_same<T, U>::value, "same check");
            return Borrowed<U>(static_cast<U *>(stupid));
        }

        template<class U>
        Borrowed<U> upcast_to() const
        {
            static_assert(std::is_base_of<U, T>::value, "base check");
            static_assert(!std::is_same<T, U>::value, "same check");
            return Borrowed<U>(stupid);
        }
    };

    namespace option
    {
        template<class T>
        class OptionRepr<Borrowed<T>>
        {
            T *stupider;
        public:
            void set_none() { stupider = nullptr; }
            void post_set_some() {}
            bool is_some() const { return stupider != nullptr; }
            Borrowed<T> *ptr() { return reinterpret_cast<Borrowed<T> *>(&stupider); }
            const Borrowed<T> *ptr() const { return reinterpret_cast<const Borrowed<T> *>(&stupider); }
        };
    }

    template<class T>
    using P = Borrowed<T>;

    template<class T>
    Borrowed<T> borrow(T& ref)
    {
        return Borrowed<T>(&ref);
    }

    template<class T>
    T *as_raw_pointer(Option<Borrowed<T>> ptr)
    {
        return &*TRY_UNWRAP(ptr, return nullptr);
    }
} // namespace tmwa
