#pragma once
//    result.hpp - A possibly failed return value
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

#include "../strings/rstring.hpp"

#include "option.hpp"

namespace tmwa
{
    namespace result
    {
        enum ResultMagicFlag { magic_flag };

        template<class T>
        class Result
        {
            Option<T> success;
            RString failure;
        public:
            Result(ResultMagicFlag, T v)
            : success(Some(std::move(v))), failure()
            {}
            Result(ResultMagicFlag, RString msg)
            : success(None), failure(std::move(msg))
            {}

            bool is_ok() { return success.is_some(); }
            bool is_err() { return !is_ok(); }

            Option<T>& get_success() { return success; }
            RString& get_failure() { return failure; }
        };

        template<class T>
        Result<T> Ok(T v)
        {
            return Result<T>(magic_flag, std::move(v));
        }

        struct Err
        {
            RString message;
            Err(RString m) : message(std::move(m)) {}

            template<class T>
            operator Result<T>()
            {
                return Result<T>(magic_flag, message);
            }
            template<class T>
            operator Option<Result<T>>()
            {
                return Some(Result<T>(magic_flag, message));
            }
        };

        template<class T>
        bool operator == (const Result<T>& l, const Result<T>& r)
        {
            return l.get_success() == r.get_success() && l.get_failure() == r.get_failure();
        }
        template<class T>
        bool operator != (const Result<T>& l, const Result<T>& r)
        {
            return !(l == r);
        }
    } // namespace result
    using result::Result;
    using result::Ok;
    using result::Err;

#define TRY(r) ({ auto _res = r; TRY_UNWRAP(_res.get_success(), return ::tmwa::Err(_res.get_failure())); })
    // TODO the existence of this as a separate macro is a bug.
#define TRY_MOVE(r) ({ auto _res = r; TRY_UNWRAP(std::move(_res.get_success()), return ::tmwa::Err(_res.get_failure())); })
} // namespace tmwa
