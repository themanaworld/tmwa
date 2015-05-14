#include "npc.hpp"
//    ast/npc_test.cpp - Testsuite for npc parser.
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
namespace npc
{
#define EXPECT_SPAN(span, bl,bc, el,ec)     \
    ({                                      \
        EXPECT_EQ((span).begin.line, bl);   \
        EXPECT_EQ((span).begin.column, bc); \
        EXPECT_EQ((span).end.line, el);     \
        EXPECT_EQ((span).end.column, ec);   \
    })

    TEST(npcast, eof)
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
            auto res = parse_top(lr);
            EXPECT_TRUE(res.is_none());
        }
    }
    TEST(npcast, comment)
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
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
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
    TEST(npcast, warp)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3         4
            //234567890123456789012345678901234567890123456789
            "map.gat,1,2|warp|To Other Map|3,4,other.gat,7,8"_s,
            "map.gat,1,2|warp|To Other Map|3,4,other.gat,7,8\n"_s,
            "map.gat,1,2|warp|To Other Map|3,4,other.gat,7,8{"_s,
            // no optional fields in warp
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,47);
            auto p = top.get_if<Warp>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->m.span, 1,1, 1,7);
                EXPECT_EQ(p->m.data, stringish<MapName>("map"_s));
                EXPECT_SPAN(p->x.span, 1,9, 1,9);
                EXPECT_EQ(p->x.data, 1);
                EXPECT_SPAN(p->y.span, 1,11, 1,11);
                EXPECT_EQ(p->y.data, 2);
                EXPECT_SPAN(p->key_span, 1,13, 1,16);
                EXPECT_SPAN(p->name.span, 1,18, 1,29);
                EXPECT_EQ(p->name.data, stringish<NpcName>("To Other Map"_s));
                EXPECT_SPAN(p->xs.span, 1,31, 1,31);
                EXPECT_EQ(p->xs.data, 5);
                EXPECT_SPAN(p->ys.span, 1,33, 1,33);
                EXPECT_EQ(p->ys.data, 6);
                EXPECT_SPAN(p->to_m.span, 1,35, 1,43);
                EXPECT_EQ(p->to_m.data, stringish<MapName>("other"_s));
                EXPECT_SPAN(p->to_x.span, 1,45, 1,45);
                EXPECT_EQ(p->to_x.data, 7);
                EXPECT_SPAN(p->to_y.span, 1,47, 1,47);
                EXPECT_EQ(p->to_y.data, 8);
            }
        }
    }
    TEST(npcast, shop)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3         4         5
            //2345678901234567890123456789012345678901234567890123456789
            "map.gat,1,2,3|shop|Flower Shop|4,5:6,Named:7,Spaced :*8"_s,
            "map.gat,1,2,3|shop|Flower Shop|4,5:6,Named:7,Spaced :*8\n"_s,
            "map.gat,1,2,3|shop|Flower Shop|4,5:6,Named:7,Spaced :*8{"_s,
            // no optional fields in shop
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,55);
            auto p = top.get_if<Shop>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->m.span, 1,1, 1,7);
                EXPECT_EQ(p->m.data, stringish<MapName>("map"_s));
                EXPECT_SPAN(p->x.span, 1,9, 1,9);
                EXPECT_EQ(p->x.data, 1);
                EXPECT_SPAN(p->y.span, 1,11, 1,11);
                EXPECT_EQ(p->y.data, 2);
                EXPECT_SPAN(p->d.span, 1,13, 1,13);
                EXPECT_EQ(p->d.data, DIR::NW);
                EXPECT_SPAN(p->key_span, 1,15, 1,18);
                EXPECT_SPAN(p->name.span, 1,20, 1,30);
                EXPECT_EQ(p->name.data, stringish<NpcName>("Flower Shop"_s));
                EXPECT_SPAN(p->npc_class.span, 1,32, 1,32);
                EXPECT_EQ(p->npc_class.data, wrap<Species>(4));
                EXPECT_SPAN(p->items.span, 1,34, 1,55);
                EXPECT_EQ(p->items.data.size(), 3);
                EXPECT_SPAN(p->items.data[0].span, 1,34, 1,36);
                EXPECT_SPAN(p->items.data[0].data.name.span, 1,34, 1,34);
                EXPECT_EQ(p->items.data[0].data.name.data, stringish<ItemName>("5"_s));
                EXPECT_EQ(p->items.data[0].data.value_multiply, false);
                EXPECT_SPAN(p->items.data[0].data.value.span, 1,36, 1,36);
                EXPECT_EQ(p->items.data[0].data.value.data, 6);
                EXPECT_SPAN(p->items.data[1].span, 1,38, 1,44);
                EXPECT_SPAN(p->items.data[1].data.name.span, 1,38, 1,42);
                EXPECT_EQ(p->items.data[1].data.name.data, stringish<ItemName>("Named"_s));
                EXPECT_EQ(p->items.data[1].data.value_multiply, false);
                EXPECT_SPAN(p->items.data[1].data.value.span, 1,44, 1,44);
                EXPECT_EQ(p->items.data[1].data.value.data, 7);
                EXPECT_SPAN(p->items.data[2].span, 1,46, 1,55);
                EXPECT_SPAN(p->items.data[2].data.name.span, 1,46, 1,52);
                EXPECT_EQ(p->items.data[2].data.name.data, stringish<ItemName>("Spaced"_s));
                EXPECT_EQ(p->items.data[2].data.value_multiply, true);
                EXPECT_SPAN(p->items.data[2].data.value.span, 1,55, 1,55);
                EXPECT_EQ(p->items.data[2].data.value.data, 8);
            }
        }
    }
    TEST(npcast, monster)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3         4         5         6
            //23456789012345678901234567890123456789012345678901234567890123456789
            "map.gat,1,2,3,4|monster|Feeping Creature|5,6,7000,8000,Npc::Event"_s,
            "map.gat,1,2,3,4|monster|Feeping Creature|5,6,7000,8000,Npc::Event\n"_s,
            "map.gat,1,2,3,4|monster|Feeping Creature|5,6,7000,8000,Npc::Event{"_s,
            "Map.gat,1,2,3,4|monster|Feeping Creature|5,6,7000,8000"_s,
            "Map.gat,1,2,3,4|monster|Feeping Creature|5,6,7000,8000\n"_s,
            "Map.gat,1,2,3,4|monster|Feeping Creature|5,6,7000,8000{"_s,
            "nap.gat,1,20304|monster|Feeping Creature|506,700008000"_s,
            "nap.gat,1,20304|monster|Feeping Creature|506,700008000\n"_s,
            "nap.gat,1,20304|monster|Feeping Creature|506,700008000{"_s,
        };
        for (auto input : inputs)
        {
            bool first = input.startswith('m');
            bool second = input.startswith('M');
            bool third = input.startswith('n');
            assert(first + second + third == 1);
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,first?65:54);
            auto p = top.get_if<Monster>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->m.span, 1,1, 1,7);
                if (first)
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("map"_s));
                }
                else if (second)
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("Map"_s));
                }
                else
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("nap"_s));
                }
                EXPECT_SPAN(p->x.span, 1,9, 1,9);
                EXPECT_EQ(p->x.data, 1);
                if (!third)
                {
                    EXPECT_SPAN(p->y.span, 1,11, 1,11);
                    EXPECT_EQ(p->y.data, 2);
                    EXPECT_SPAN(p->xs.span, 1,13, 1,13);
                    EXPECT_EQ(p->xs.data, 3);
                    EXPECT_SPAN(p->ys.span, 1,15, 1,15);
                    EXPECT_EQ(p->ys.data, 4);
                }
                else
                {
                    EXPECT_SPAN(p->y.span, 1,11, 1,15);
                    EXPECT_EQ(p->y.data, 20304);
                    EXPECT_SPAN(p->xs.span, 1,16, 1,16);
                    EXPECT_EQ(p->xs.data, 0);
                    EXPECT_SPAN(p->ys.span, 1,16, 1,16);
                    EXPECT_EQ(p->ys.data, 0);
                }
                EXPECT_SPAN(p->key_span, 1,17, 1,23);
                EXPECT_SPAN(p->name.span, 1,25, 1,40);
                EXPECT_EQ(p->name.data, stringish<MobName>("Feeping Creature"_s));
                if (!third)
                {
                    EXPECT_SPAN(p->mob_class.span, 1,42, 1,42);
                    EXPECT_EQ(p->mob_class.data, wrap<Species>(5));
                    EXPECT_SPAN(p->num.span, 1,44, 1,44);
                    EXPECT_EQ(p->num.data, 6);
                    EXPECT_SPAN(p->delay1.span, 1,46, 1,49);
                    EXPECT_EQ(p->delay1.data, 7_s);
                    EXPECT_SPAN(p->delay2.span, 1,51, 1,54);
                    EXPECT_EQ(p->delay2.data, 8_s);
                }
                else
                {
                    EXPECT_SPAN(p->mob_class.span, 1,42, 1,44);
                    EXPECT_EQ(p->mob_class.data, wrap<Species>(506));
                    EXPECT_SPAN(p->num.span, 1,46, 1,54);
                    EXPECT_EQ(p->num.data, 700008000);
                    EXPECT_SPAN(p->delay1.span, 1,55, 1,55);
                    EXPECT_EQ(p->delay1.data, 0_s);
                    EXPECT_SPAN(p->delay2.span, 1,55, 1,55);
                    EXPECT_EQ(p->delay2.data, 0_s);
                }
                if (first)
                {
                    EXPECT_SPAN(p->event.span, 1,56, 1,65);
                    EXPECT_EQ(p->event.data.npc, stringish<NpcName>("Npc"_s));
                    EXPECT_EQ(p->event.data.label, stringish<ScriptLabel>("Event"_s));
                }
                else
                {
                    EXPECT_SPAN(p->event.span, 1,55, 1,55);
                    EXPECT_EQ(p->event.data.npc, NpcName());
                    EXPECT_EQ(p->event.data.label, ScriptLabel());
                }
            }
        }
    }
    TEST(npcast, mapflag)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3
            //23456789012345678901234567890123456789
            "map.gat|mapflag|flagname"_s,
            "map.gat|mapflag|flagname\n"_s,
            "map.gat|mapflag|flagname{"_s,
            "Map.gat|mapflag|flagname|optval"_s,
            "Map.gat|mapflag|flagname|optval\n"_s,
            "Map.gat|mapflag|flagname|optval{"_s,
            "nap.gat|mapflag|flagname|aa,b,c"_s,
            "nap.gat|mapflag|flagname|aa,b,c\n"_s,
            "nap.gat|mapflag|flagname|aa,b,c{"_s,
        };
        for (auto input : inputs)
        {
            bool first = input.startswith('m');
            bool second = input.startswith('M');
            bool third = input.startswith('n');
            EXPECT_EQ(first + second + third, 1);
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,first?24:31);
            auto p = top.get_if<MapFlag>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->m.span, 1,1, 1,7);
                if (first)
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("map"_s));
                }
                if (second)
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("Map"_s));
                }
                if (third)
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("nap"_s));
                }
                EXPECT_SPAN(p->key_span, 1,9, 1,15);
                EXPECT_SPAN(p->name.span, 1,17, 1,24);
                EXPECT_EQ(p->name.data, "flagname"_s);
                if (first)
                {
                    EXPECT_SPAN(p->vec_extra.span, 1,25, 1,25);
                    EXPECT_EQ(p->vec_extra.data.size(), 0);
                }
                if (second)
                {
                    EXPECT_SPAN(p->vec_extra.span, 1,26, 1,31);
                    EXPECT_EQ(p->vec_extra.data.size(), 1);
                    EXPECT_SPAN(p->vec_extra.data[0].span, 1,26, 1,31);
                    EXPECT_EQ(p->vec_extra.data[0].data, "optval"_s);
                }
                if (third)
                {
                    EXPECT_SPAN(p->vec_extra.span, 1,26, 1,31);
                    EXPECT_EQ(p->vec_extra.data.size(), 3);
                    EXPECT_SPAN(p->vec_extra.data[0].span, 1,26, 1,27);
                    EXPECT_EQ(p->vec_extra.data[0].data, "aa"_s);
                    EXPECT_SPAN(p->vec_extra.data[1].span, 1,29, 1,29);
                    EXPECT_EQ(p->vec_extra.data[1].data, "b"_s);
                    EXPECT_SPAN(p->vec_extra.data[2].span, 1,31, 1,31);
                    EXPECT_EQ(p->vec_extra.data[2].data, "c"_s);
                }
            }
        }
    }

    TEST(npcast, scriptfun)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3
            //23456789012345678901234567890123456789
            "function|script|Fun Name{end;}"_s,
            //                         123456
            "function|script|Fun Name\n{end;}\n"_s,
            //                           1234567
            "function|script|Fun Name\n \n {end;} "_s,
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,24);
            auto script = top.get_if<Script>();
            EXPECT_TRUE(script);
            auto p = script->get_if<ScriptFunction>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->key1_span, 1,1, 1,8);
                EXPECT_SPAN(script->key_span, 1,10, 1,15);
                EXPECT_SPAN(p->name.span, 1,17, 1,24);
                EXPECT_EQ(p->name.data, "Fun Name"_s);
                if (input.endswith('}'))
                {
                    EXPECT_SPAN(script->body.span, 1,25, 1,30);
                }
                else if (input.endswith('\n'))
                {
                    EXPECT_SPAN(script->body.span, 2,1, 2,6);
                }
                else if (input.endswith(' '))
                {
                    EXPECT_SPAN(script->body.span, 3,2, 3,7);
                }
                else
                {
                    FAIL();
                }
                EXPECT_EQ(script->body.braced_body, "{end;}"_s);
            }
        }
    }
    TEST(npcast, scriptspell)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3
            //23456789012345678901234567890123456789
            "spell|script|#FunName{end;}"_s,
            //                         123456
            "spell|script|#FunName\n{end;}\n"_s,
            //                           1234567
            "spell|script|#FunName\n \n {end;} "_s,
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,21);
            auto script = top.get_if<Script>();
            EXPECT_TRUE(script);
            auto p = script->get_if<ScriptMap>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(script->key_span, 1,7, 1,12);
                EXPECT_SPAN(p->name.span, 1,14, 1,21);
                EXPECT_EQ(p->name.data, stringish<NpcName>("#FunName"_s));
                if (input.endswith('}'))
                {
                    EXPECT_SPAN(script->body.span, 1,22, 1,27);
                }
                else if (input.endswith('\n'))
                {
                    EXPECT_SPAN(script->body.span, 2,1, 2,6);
                }
                else if (input.endswith(' '))
                {
                    EXPECT_SPAN(script->body.span, 3,2, 3,7);
                }
                else
                {
                    FAIL();
                }
                EXPECT_EQ(script->body.braced_body, "{end;}"_s);
            }
        }
    }
    TEST(npcast, scriptnone)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3
            //23456789012345678901234567890123456789
            "-|script|#config|32767{end;}"_s,
            //                    123456
            "-|script|#config|32767\n{end;}\n"_s,
            //                      1234567
            "-|script|#config|32767\n \n {end;} "_s,
        };
        for (auto input : inputs)
        {
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,22);
            auto script = top.get_if<Script>();
            EXPECT_TRUE(script);
            auto p = script->get_if<ScriptNone>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->key1_span, 1,1, 1,1);
                EXPECT_SPAN(script->key_span, 1,3, 1,8);
                EXPECT_SPAN(p->name.span, 1,10, 1,16);
                EXPECT_EQ(p->name.data, stringish<NpcName>("#config"_s));
                EXPECT_SPAN(p->key4_span, 1,18, 1,22);
                if (input.endswith('}'))
                {
                    EXPECT_SPAN(script->body.span, 1,23, 1,28);
                }
                else if (input.endswith('\n'))
                {
                    EXPECT_SPAN(script->body.span, 2,1, 2,6);
                }
                else if (input.endswith(' '))
                {
                    EXPECT_SPAN(script->body.span, 3,2, 3,7);
                }
                else
                {
                    FAIL();
                }
                EXPECT_EQ(script->body.braced_body, "{end;}"_s);
            }
        }
    }
    TEST(npcast, scriptmap)
    {
        QuietFd q;
        LString inputs[] =
        {
            //        1         2         3
            //23456789012345678901234567890123456789
            "map.gat,1,2,3|script|Asdf|4,5,6{end;}"_s,
            "map.gat,1,2,3|script|Asdf|4,5,6\n{end;}\n"_s,
            "map.gat,1,2,3|script|Asdf|4,5,6\n \n {end;} "_s,
            "Map.gat,1,2,3|script|Asdf|40506{end;}"_s,
            "Map.gat,1,2,3|script|Asdf|40506\n{end;}\n"_s,
            "Map.gat,1,2,3|script|Asdf|40506\n \n {end;} "_s,
        };
        for (auto input : inputs)
        {
            bool second = input.startswith('M');
            io::LineCharReader lr(io::from_string, "<string>"_s, input);
            auto res = TRY_UNWRAP(parse_top(lr), FAIL());
            EXPECT_TRUE(res.get_success().is_some());
            auto top = TRY_UNWRAP(std::move(res.get_success()), FAIL());
            EXPECT_SPAN(top.span, 1,1, 1,31);
            auto script = top.get_if<Script>();
            EXPECT_TRUE(script);
            auto p = script->get_if<ScriptMap>();
            EXPECT_TRUE(p);
            if (p)
            {
                EXPECT_SPAN(p->m.span, 1,1, 1,7);
                if (!second)
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("map"_s));
                }
                else
                {
                    EXPECT_EQ(p->m.data, stringish<MapName>("Map"_s));
                }
                EXPECT_SPAN(p->x.span, 1,9, 1,9);
                EXPECT_EQ(p->x.data, 1);
                EXPECT_SPAN(p->y.span, 1,11, 1,11);
                EXPECT_EQ(p->y.data, 2);
                EXPECT_SPAN(p->d.span, 1,13, 1,13);
                EXPECT_EQ(p->d.data, DIR::NW);
                EXPECT_SPAN(script->key_span, 1,15, 1,20);
                EXPECT_SPAN(p->name.span, 1,22, 1,25);
                EXPECT_EQ(p->name.data, stringish<NpcName>("Asdf"_s));
                if (!second)
                {
                    EXPECT_SPAN(p->npc_class.span, 1,27, 1,27);
                    EXPECT_EQ(p->npc_class.data, wrap<Species>(4));
                    EXPECT_SPAN(p->xs.span, 1,29, 1,29);
                    EXPECT_EQ(p->xs.data, 11);
                    EXPECT_SPAN(p->ys.span, 1,31, 1,31);
                    EXPECT_EQ(p->ys.data, 13);
                }
                else
                {
                    EXPECT_SPAN(p->npc_class.span, 1,27, 1,31);
                    EXPECT_EQ(p->npc_class.data, wrap<Species>(40506));
                    EXPECT_SPAN(p->xs.span, 1,32, 1,32);
                    EXPECT_EQ(p->xs.data, 0);
                    EXPECT_SPAN(p->ys.span, 1,32, 1,32);
                    EXPECT_EQ(p->ys.data, 0);
                }
                if (input.endswith('}'))
                {
                    EXPECT_SPAN(script->body.span, 1,32, 1,37);
                }
                else if (input.endswith('\n'))
                {
                    EXPECT_SPAN(script->body.span, 2,1, 2,6);
                }
                else if (input.endswith(' '))
                {
                    EXPECT_SPAN(script->body.span, 3,2, 3,7);
                }
                else
                {
                    FAIL();
                }
                EXPECT_EQ(script->body.braced_body, "{end;}"_s);
            }
        }
    }
} // namespace npc
} // namespace ast
} // namespace tmwa
