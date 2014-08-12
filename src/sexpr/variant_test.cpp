#include "variant.hpp"
//    variant_test.cpp - Testsuite for multi-type container that's better than boost's.
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

#include <gtest/gtest.h>

#include "../strings/vstring.hpp"

//#include "../poison.hpp"


namespace tmwa
{
    struct Tracker
    {
        int id, moves, copies;

        explicit
        Tracker(int i)
        : id(i), moves(0), copies(0)
        {}
        Tracker(Tracker&& rhs)
        : id(rhs.id), moves(rhs.moves + 1), copies(rhs.copies)
        { rhs.id = 0; }
        Tracker(const Tracker& rhs)
        : id(rhs.id), moves(rhs.moves), copies(rhs.copies + 1)
        {}
        Tracker& operator = (Tracker&& rhs)
        { id = rhs.id; moves = rhs.moves + 1; copies = rhs.copies; rhs.id = 0; return *this; }
        Tracker& operator = (const Tracker& rhs)
        { id = rhs.id; moves = rhs.moves; copies = rhs.copies + 1; return *this; }
    };
    struct Foo : Tracker
    {
        // needed for first param of variant
        Foo() noexcept : Tracker(0) { abort(); }

        Foo(int i) : Tracker(i) {}
    };
    struct Bar : Tracker
    {
        Bar(int i) : Tracker(i) {}
    };
    struct Qux : Tracker
    {
        // needed for first param of variant
        Qux() noexcept : Tracker(0) { abort(); }

        Qux(int i) : Tracker(i) {}
        Qux(Qux&&) = default;
        Qux(const Qux&) = delete;
        Qux& operator = (Qux&&) = default;
        Qux& operator = (const Qux&) = default;
    };

TEST(variant, match)
{
    struct Sub : sexpr::Variant<Foo, Bar>
    {
        Sub()
        : sexpr::Variant<Foo, Bar>(Foo(1))
        {}
    };
    Sub v1;
    MATCH (v1)
    {
        // This is not a public API, it's just for testing.
    default:
        FAIL();

        CASE(Foo, f)
        {
            (void)f;
            SUCCEED();
        }
        CASE(Bar, b)
        {
            (void)b;
            FAIL();
        }
    }
    v1.emplace<Bar>(2);
    MATCH (v1)
    {
        // This is not a public API, it's just for testing.
    default:
        FAIL();

        CASE(Foo, f)
        {
            (void)f;
            FAIL();
        }
        CASE(Bar, b)
        {
            (void)b;
            SUCCEED();
        }
    }
}

TEST(variant, copymove1)
{
    sexpr::Variant<Qux> moveonly(Qux(3));
    (void)moveonly;
}

TEST(variant, copymove2)
{
    struct Move
    {
        Move() = default;
        Move(Move&&) = default;
        Move(const Move&) = delete;
        Move& operator = (Move&&) = default;
        Move& operator = (const Move&) = delete;
        ~Move() = default;
    };
    struct Copy
    {
        Copy() = default;
        Copy(Copy&&) = default;
        Copy(const Copy&) = default;
        Copy& operator = (Copy&&) = default;
        Copy& operator = (const Copy&) = default;
        ~Copy() = default;
    };

    using VarMv = sexpr::Variant<Move>;
    using VarCp = sexpr::Variant<Copy>;

    VarMv mv1;
    VarMv mv3 = std::move(mv1);
    mv1 = std::move(mv3);
    VarCp cp1;
    VarCp cp2 = cp1;
    VarCp cp3 = std::move(cp1);
    cp1 = cp2;
    cp1 = std::move(cp3);
}
} // namespace tmwa
