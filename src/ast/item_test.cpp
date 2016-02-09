#include "item.hpp"
//    ast/item_test.cpp - Testsuite for itemdb parser.
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

#include "../io/line.hpp"

#include "../tests/fdhack.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace ast
{
namespace item
{
#define EXPECT_SPAN(span, bl,bc, el,ec)     \
    ({                                      \
        EXPECT_EQ((span).begin.line, bl);   \
        EXPECT_EQ((span).begin.column, bc); \
        EXPECT_EQ((span).end.line, el);     \
        EXPECT_EQ((span).end.column, ec);   \
    })

    TEST(itemast, eof)
    {
        QuietFd q;
        LString inputs[] =
        {
            ""_s,
            "\n"_s,
            "\n\n"_s,
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = parse_item(lr);
            EXPECT_TRUE(res.is_none());
        }
    }
    TEST(itemast, comment)
    {
        QuietFd q;
        LString inputs[] =
        {
            //23456789
            "// hello"_s,
            "// hello\n "_s,
            "// hello\nabc"_s,
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_item(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,8);
            auto p = top.get_if<Comment>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_EQ(p->comment, "// hello"_s);
            }
        }
    }
    TEST(itemast, item)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3         4         5
            //2345678901234567890123456789012345678901234567890123456789
            "1,abc ,      3,4,5,6,7,8,9,10,xx,2,16,12,13,11, {end;}, {}"_s,
            "1,abc ,      3,4,5,6,7,8,9,10,xx,2,16,12,13,11, {end;}, {}\n"_s,
            "1,abc ,      3,4,5,6,7,8,9,10,xx,2,16,12,13,11, {end;}, {}\nabc"_s,
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_item(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,58);
            auto p = top.get_if<Item>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->id.span, 1,1, 1,1);
                EXPECT_EQ(p->id.data, wrap<ItemNameId>(1));
                EXPECT_SPAN(p->name.span, 1,3, 1,6);
                EXPECT_EQ(p->name.data, stringish<ItemName>("abc "_s));
                EXPECT_SPAN(p->type.span, 1,14, 1,14);
                EXPECT_EQ(p->type.data, ItemType::JUNK);
                EXPECT_SPAN(p->buy_price.span, 1,16, 1,16);
                EXPECT_EQ(p->buy_price.data, 4);
                EXPECT_SPAN(p->sell_price.span, 1,18, 1,18);
                EXPECT_EQ(p->sell_price.data, 5);
                EXPECT_SPAN(p->weight.span, 1,20, 1,20);
                EXPECT_EQ(p->weight.data, 6);
                EXPECT_SPAN(p->atk.span, 1,22, 1,22);
                EXPECT_EQ(p->atk.data, 7);
                EXPECT_SPAN(p->def.span, 1,24, 1,24);
                EXPECT_EQ(p->def.data, 8);
                EXPECT_SPAN(p->range.span, 1,26, 1,26);
                EXPECT_EQ(p->range.data, 9);
                EXPECT_SPAN(p->magic_bonus.span, 1,28, 1,29);
                EXPECT_EQ(p->magic_bonus.data, 10);
                EXPECT_SPAN(p->slot_unused.span, 1,31, 1,32);
                EXPECT_EQ(p->slot_unused.data, "xx"_s);
                EXPECT_SPAN(p->gender.span, 1,34, 1,34);
                EXPECT_EQ(p->gender.data, SEX::UNSPECIFIED);
                EXPECT_SPAN(p->loc.span, 1,36, 1,37);
                EXPECT_EQ(p->loc.data, EPOS::MISC1);
                EXPECT_SPAN(p->wlv.span, 1,39, 1,40);
                EXPECT_EQ(p->wlv.data, 12);
                EXPECT_SPAN(p->elv.span, 1,42, 1,43);
                EXPECT_EQ(p->elv.data, 13);
                EXPECT_SPAN(p->view.span, 1,45, 1,46);
                EXPECT_EQ(p->view.data, ItemLook::BOW);
                EXPECT_SPAN(p->use_script.span, 1,49, 1,54);
                EXPECT_EQ(p->use_script.braced_body, "{end;}"_s);
                EXPECT_SPAN(p->equip_script.span, 1,57, 1,58);
                EXPECT_EQ(p->equip_script.braced_body, "{}"_s);
            }
        }
    }
} // namespace item
} // namespace ast
} // namespace tmwa
