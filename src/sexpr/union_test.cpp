#include "union.hpp"
//    union_test.cpp - Just include the header file and try to instantiate.
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

#include "../poison.hpp"


namespace tmwa
{
namespace sexpr
{
namespace
{
    struct Foo
    {
        Foo();
        Foo(const Foo&);
        Foo& operator = (const Foo&);
        ~Foo();
    };
} // anonymous namespace
static Union<int, Foo> u;

static_assert(u.index<int>() == 0, "int");
static_assert(u.index<Foo>() == 1, "Foo");
static_assert(u.index<char>() == size_t(-1), "char");
} // namespace sexpr
} // namespace tmwa
