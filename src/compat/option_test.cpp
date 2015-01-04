#include "option.hpp"
//    option_test.cpp - Testsuite for a type that may or may not exist
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

#include "../strings/literal.hpp"

#include "borrow.hpp"

#include "../diagnostics.hpp"
//#include "../poison.hpp"


namespace tmwa
{
TEST(Option, somenone)
{
    {
        option::Option<int> opt = option::None;
        opt = option::None;
    }
    {
        option::Option<int> opt = option::None<int>;
        opt = option::None<int>;
    }
    {
        option::Option<int> opt = option::None<int>();
        opt = option::None<int>();
    }
    {
        option::Option<int> opt = option::Some(123);
        opt = option::Some(123);
    }
    {
        option::Option<int> opt = option::Some<int>(123);
        opt = option::Some<int>(123);
    }
}
TEST(Option, somenonenocopy)
{
    struct Foo
    {
        Foo() = default;
        Foo(Foo&&) = default;
        Foo(const Foo&) = delete;
        Foo& operator = (Foo&&) = default;
        Foo& operator = (const Foo&) = delete;
    };
    {
        option::Option<Foo> opt = option::None;
        opt = option::None;
    }
    // clang <= 3.4 is buggy
    // since clang doesn't version, there is no way to restrict it to clang 3.5+
#ifndef __clang__
    {
        option::Option<Foo> opt = option::None<Foo>;
        opt = option::None<Foo>;
    }
#endif
    {
        option::Option<Foo> opt = option::None<Foo>();
        opt = option::None<Foo>();
    }
    {
        option::Option<Foo> opt = option::Some(Foo());
        opt = option::Some(Foo());
    }
    {
        option::Option<Foo> opt = option::Some<Foo>(Foo());
        opt = option::Some<Foo>(Foo());
    }
}
TEST(Option, customrepr)
{
    int iv = 123;
    Borrowed<int> i = borrow(iv);

    EXPECT_EQ(&iv, as_raw_pointer(Some(i)));

    {
        option::Option<Borrowed<int>> opt = option::None;
        opt = option::None;
    }
    {
        option::Option<Borrowed<int>> opt = option::None<Borrowed<int>>;
        opt = option::None<Borrowed<int>>;
    }
    {
        option::Option<Borrowed<int>> opt = option::None<Borrowed<int>>();
        opt = option::None<Borrowed<int>>();
    }
    {
        option::Option<Borrowed<int>> opt = option::Some(i);
        opt = option::Some(i);
    }
    {
        option::Option<Borrowed<int>> opt = option::Some<Borrowed<int>>(i);
        opt = option::Some<Borrowed<int>>(i);
    }
}

TEST(Option, destruct)
{
    struct BugCheck
    {
        bool *destroyed;

        BugCheck(bool *d)
        : destroyed(d)
        {}
        BugCheck(BugCheck&& r)
        : destroyed(r.destroyed)
        {
            r.destroyed = nullptr;
        }
        BugCheck& operator = (BugCheck&& r)
        {
            std::swap(destroyed, r.destroyed);
            return *this;
        }
        ~BugCheck()
        {
            if (!destroyed)
                return;
            if (*destroyed)
                abort();
            *destroyed = true;
        }
    };

    bool destroyed = false;

    Option<BugCheck> bug = Some(BugCheck(&destroyed));
    bug = None;
}

TEST(Option, def)
{
    struct Tracked
    {
        int id;
        int gen;

        Tracked(int i, int g=0) : id(i), gen(g) {}
        Tracked(Tracked&&) = default;
        Tracked(const Tracked& r) : id(r.id), gen(r.gen + 1) {}
        Tracked& operator = (Tracked&&) = default;
        Tracked& operator = (const Tracked& r) { id = r.id; gen = r.gen + 1; return *this; }

        bool operator == (const Tracked& r) const
        {
            return this->id == r.id && this->gen == r.gen;
        }
        bool operator != (const Tracked& r) const
        {
            return !(*this == r);
        }
    };

    {
        option::Option<Tracked> o = option::None;
        EXPECT_EQ(o.move_or(Tracked(1)), Tracked(1));
        EXPECT_EQ(o.copy_or(Tracked(2)), Tracked(2));
        Tracked t3(3);
        Tracked& r3 = o.ref_or(t3);
        EXPECT_EQ(&r3, &t3);
        Tracked t4(4);
        Tracked *r4 = o.ptr_or(&t4);
        EXPECT_EQ(r4, &t4);
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
    }
    {
        const option::Option<Tracked> o = option::None;
        EXPECT_EQ(o.copy_or(Tracked(2)), Tracked(2));
        Tracked t3(3);
        const Tracked& r3 = o.ref_or(t3);
        EXPECT_EQ(&r3, &t3);
        Tracked t4(4);
        const Tracked *r4 = o.ptr_or(&t4);
        EXPECT_EQ(r4, &t4);
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
    }
    {
        option::Option<Tracked> o = option::Some(Tracked(0));
        EXPECT_EQ(o.move_or(Tracked(1)), Tracked(0));
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        o = option::Some(Tracked(0));
        EXPECT_EQ(o.copy_or(Tracked(2)), Tracked(0, 1));
        Tracked t3(3);
        Tracked& r3 = o.ref_or(t3);
        EXPECT_NE(&r3, &t3);
        Tracked t4(4);
        Tracked *r4 = o.ptr_or(&t4);
        EXPECT_NE(r4, &t4);
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        EXPECT_EQ(&r3, r4);
        EXPECT_EQ(r4, reinterpret_cast<Tracked *>(&o));
    }
    {
        const option::Option<Tracked> o = option::Some(Tracked(0));
        EXPECT_EQ(o.copy_or(Tracked(2)), Tracked(0, 1));
        Tracked t3(3);
        const Tracked& r3 = o.ref_or(t3);
        EXPECT_NE(&r3, &t3);
        Tracked t4(4);
        const Tracked *r4 = o.ptr_or(&t4);
        EXPECT_NE(r4, &t4);
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        EXPECT_EQ(&r3, r4);
        EXPECT_EQ(r4, reinterpret_cast<const Tracked *>(&o));
    }
}

TEST(Option, map)
{
    struct Foo
    {
        Foo() = default;
        Foo(Foo&&) = default;
        Foo(const Foo&) = delete;
        Foo& operator = (Foo&&) = default;
        Foo& operator = (const Foo&) = delete;
    };

    // move
    {
        option::Option<Foo> o = option::None;
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        option::Option<int> i = o.move_map([](Foo){ return 0; });
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        EXPECT_EQ(i.ptr_or(nullptr), nullptr);
    }
    {
        option::Option<Foo> o = option::Some(Foo());
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        option::Option<int> i = o.move_map([](Foo){ return 1; });
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        EXPECT_NE(i.ptr_or(nullptr), nullptr);
        EXPECT_EQ(i.copy_or(0), 1);
    }
    // mut ref
    {
        option::Option<Foo> o = option::None;
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        option::Option<int> i = o.map([](Foo&){ return 0; });
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        EXPECT_EQ(i.ptr_or(nullptr), nullptr);
    }
    {
        option::Option<Foo> o = option::Some(Foo());
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        option::Option<int> i = o.map([](Foo&){ return 1; });
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        EXPECT_NE(i.ptr_or(nullptr), nullptr);
        EXPECT_EQ(i.copy_or(0), 1);
    }
    // const ref
    {
        option::Option<Foo> o = option::None;
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        option::Option<int> i = o.map([](const Foo&){ return 0; });
        EXPECT_EQ(o.ptr_or(nullptr), nullptr);
        EXPECT_EQ(i.ptr_or(nullptr), nullptr);
    }
    {
        option::Option<Foo> o = option::Some(Foo());
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        option::Option<int> i = o.map([](const Foo&){ return 1; });
        EXPECT_NE(o.ptr_or(nullptr), nullptr);
        EXPECT_NE(i.ptr_or(nullptr), nullptr);
        EXPECT_EQ(i.copy_or(0), 1);
    }
}

TEST(Option, member)
{
    struct Foo
    {
        int bar = 404;
    };

    Option<Foo> vng = None;
    EXPECT_EQ(vng.pmd_get(&Foo::bar).copy_or(42), 42);
    Option<Foo> vsg = Some(Foo());
    EXPECT_EQ(vsg.pmd_get(&Foo::bar).copy_or(42), 404);

    Option<Foo> vns = None;
    vns.pmd_set(&Foo::bar, 42);
    EXPECT_EQ(vns.copy_or(Foo()).bar, 404);
    Option<Foo> vss = Some(Foo());
    vss.pmd_set(&Foo::bar, 42);
    EXPECT_EQ(vss.copy_or(Foo()).bar, 42);

    Foo foo, alt;

    Option<P<Foo>> png = None;
    EXPECT_EQ(png.pmd_pget(&Foo::bar).copy_or(42), 42);
    Option<P<Foo>> psg = Some(borrow(foo));
    EXPECT_EQ(psg.pmd_pget(&Foo::bar).copy_or(42), 404);

    Option<P<Foo>> pns = None;
    pns.pmd_pset(&Foo::bar, 42);
    EXPECT_EQ(pns.copy_or(borrow(alt))->bar, 404);
    EXPECT_EQ(foo.bar, 404);
    Option<P<Foo>> pss = Some(borrow(foo));
    pss.pmd_pset(&Foo::bar, 42);
    EXPECT_EQ(pss.copy_or(borrow(alt))->bar, 42);
    EXPECT_EQ(foo.bar, 42);
    EXPECT_EQ(alt.bar, 404);
}

#if __cplusplus >= 201300 // c++14 as given by gcc 4.9
# define DECLTYPE_AUTO decltype(auto)
#else
# define DECLTYPE_AUTO auto&&
#endif

TEST(Option, unwrap)
{
    int x;

    Option<int> v = Some(1);
    Option<int>& l = v;
    Option<int>&& r = std::move(v);
    const Option<int> cv = v;
    // significantly, see the mut
    const Option<int>& cl = v;
    const Option<int>&& cr = std::move(v);

    auto fv = [&]() -> Option<int> { return v; };
    auto fl = [&]() -> Option<int>& { return l; };
    auto fr = [&]() -> Option<int>&& { return std::move(r); };
    auto fcv = [&]() -> const Option<int> { return v; };
    auto fcl = [&]() -> const Option<int>& { return l; };
    auto fcr = [&]() -> const Option<int>&& { return std::move(r); };

    DIAG_PUSH();
    DIAG_I(useless_cast);

#define CHECK(v, t)                                                             \
    {                                                                           \
        DECLTYPE_AUTO out = TRY_UNWRAP(v, abort() );                            \
        DECLTYPE_AUTO cmp = static_cast<t>(x);                                  \
        static_assert(std::is_same<decltype(out), decltype(cmp)>::value, #v);   \
    }

    CHECK(v, int&);
    CHECK(cv, const int&);
    CHECK(l, int&);
    CHECK(cl, const int&);
    CHECK(r, int&);
    CHECK(cr, const int&);
    // repeat the same forcing expressions, since that matters with decltype
    CHECK((v), int&);
    CHECK((cv), const int&);
    CHECK((l), int&);
    CHECK((cl), const int&);
    CHECK((r), int&);
    CHECK((cr), const int&);

    CHECK(fv(), int);
    CHECK(fcv(), int);
    CHECK(fl(), int&);
    CHECK(fcl(), const int&);
    CHECK(fr(), int&&);
    CHECK(fcr(), const int&&);

    DIAG_POP();
#undef CHECK

    v = None; TRY_UNWRAP(v, v = Some(1));
    v = None; TRY_UNWRAP(l, v = Some(1));
    v = None; TRY_UNWRAP(cl, v = Some(1));
    v = None; TRY_UNWRAP(r, v = Some(1));
    v = None; TRY_UNWRAP(cr, v = Some(1));

    v = None; TRY_UNWRAP(fl(), v = Some(1));
    v = None; TRY_UNWRAP(fcl(), v = Some(1));
    v = None; TRY_UNWRAP(fr(), v = Some(1));
    v = None; TRY_UNWRAP(fcr(), v = Some(1));

    v = None;
    OMATCH_BEGIN (v)
    {
        OMATCH_CASE_SOME (o)
        {
            EXPECT_NE(o, o);
        }
        OMATCH_CASE_NONE ()
        {
            SUCCEED();
        }
    }
    OMATCH_END ();

    v = Some(1);
    OMATCH_BEGIN (v)
    {
        OMATCH_CASE_SOME (o)
        {
            EXPECT_EQ(o, 1);
        }
        OMATCH_CASE_NONE ()
        {
            FAIL();
        }
    }
    OMATCH_END ();
}

TEST(Option, flatten)
{
    using option::Option;
    using option::Some;
    using option::None;

    struct Foo
    {
        int x;
    };
    auto f1 = Some(Foo{42});
    auto f2 = Some(f1);
    auto f3 = Some(f2);
    EXPECT_EQ(flatten(f1).copy_or(Foo{404}).x, 42);
    EXPECT_EQ(flatten(f2).copy_or(Foo{404}).x, 42);
    EXPECT_EQ(flatten(f3).copy_or(Foo{404}).x, 42);

    decltype(f1) n1 = None;
    decltype(f2) n2a = None;
    decltype(f2) n2b = Some(n1);
    decltype(f3) n3a = None;
    decltype(f3) n3b = Some(n2a);
    decltype(f3) n3c = Some(n2b);
    EXPECT_EQ(flatten(n1).copy_or(Foo{404}).x, 404);
    EXPECT_EQ(flatten(n2a).copy_or(Foo{404}).x, 404);
    EXPECT_EQ(flatten(n2b).copy_or(Foo{404}).x, 404);
    EXPECT_EQ(flatten(n3a).copy_or(Foo{404}).x, 404);
    EXPECT_EQ(flatten(n3b).copy_or(Foo{404}).x, 404);
    EXPECT_EQ(flatten(n3c).copy_or(Foo{404}).x, 404);
}

#define EQ(a, b) ({ EXPECT_TRUE(a == b); EXPECT_FALSE(a != b); EXPECT_FALSE(a < b); EXPECT_TRUE(a <= b); EXPECT_FALSE(a > b); EXPECT_TRUE(a >= b); })
#define LT(a, b) ({ EXPECT_FALSE(a == b); EXPECT_TRUE(a != b); EXPECT_TRUE(a < b); EXPECT_TRUE(a <= b); EXPECT_FALSE(a > b); EXPECT_FALSE(a >= b); })
#define GT(a, b) ({ EXPECT_FALSE(a == b); EXPECT_TRUE(a != b); EXPECT_FALSE(a < b); EXPECT_FALSE(a <= b); EXPECT_TRUE(a > b); EXPECT_TRUE(a >= b); })

TEST(Option, cmp)
{
    using option::Option;
    using option::Some;
    using option::None;

    Option<int> none = None;

    EQ(none, none);
    EQ(none, None);
    LT(none, Some(-1));
    LT(none, Some(0));
    LT(none, Some(1));
    EQ((None), none);
    // EQ((None), None); // actually a function template
    LT((None), Some(-1));
    LT((None), Some(0));
    LT((None), Some(1));
    GT(Some(-1), none);
    GT(Some(-1), None);
    EQ(Some(-1), Some(-1));
    LT(Some(-1), Some(0));
    LT(Some(-1), Some(1));
    GT(Some(0), none);
    GT(Some(0), None);
    GT(Some(0), Some(-1));
    EQ(Some(0), Some(0));
    LT(Some(0), Some(1));
    GT(Some(1), none);
    GT(Some(1), None);
    GT(Some(1), Some(-1));
    GT(Some(1), Some(0));
    EQ(Some(1), Some(1));
}

} // namespace tmwa
