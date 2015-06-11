#include "npc.hpp"
//    ast/npc.cpp - Structure of non-player characters (including mobs).
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

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/line.hpp"

#include "../mmo/extract_enums.hpp"

#include "../high/extract_mmo.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace ast
{
namespace npc
{
    using io::respan;

#define TRY_EXTRACT(bit, var) ({ if (!extract(bit.data, &var.data)) return Err(bit.span.error_str("failed to extract "_s #var)); var.span = bit.span; })

    static
    Result<Warp> parse_warp(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        if (bits.size() != 4)
        {
            return Err(span.error_str("expect 4 |component|s"_s));
        }
        if (bits[0].data.size() != 3)
        {
            return Err(bits[0].span.error_str("in |component 1| expect 3 ,component,s"_s));
        }
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "warp"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }
        if (bits[3].data.size() != 5)
        {
            return Err(bits[3].span.error_str("in |component 4| expect 5 ,component,s"_s));
        }

        Warp warp;
        TRY_EXTRACT(bits[0].data[0], warp.m);
        TRY_EXTRACT(bits[0].data[1], warp.x);
        TRY_EXTRACT(bits[0].data[2], warp.y);
        warp.key_span = bits[1].data[0].span;
        TRY_EXTRACT(bits[2].data[0], warp.name);
        if (bits[3].data[0].data == "-1"_s)
            bits[3].data[0].data = "4294967295"_s;
        TRY_EXTRACT(bits[3].data[0], warp.xs);
        warp.xs.data += 2;
        if (bits[3].data[1].data == "-1"_s)
            bits[3].data[1].data = "4294967295"_s;
        TRY_EXTRACT(bits[3].data[1], warp.ys);
        warp.ys.data += 2;
        TRY_EXTRACT(bits[3].data[2], warp.to_m);
        TRY_EXTRACT(bits[3].data[3], warp.to_x);
        TRY_EXTRACT(bits[3].data[4], warp.to_y);
        return Ok(std::move(warp));
    }
    static
    Result<Shop> parse_shop(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        if (bits.size() != 4)
        {
            return Err(span.error_str("expect 4 |component|s"_s));
        }
        if (bits[0].data.size() != 4)
        {
            return Err(bits[0].span.error_str("in |component 1| expect 4 ,component,s"_s));
        }
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "shop"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }
        if (bits[3].data.size() < 2)
        {
            return Err(bits[3].span.error_str("in |component 4| expect at least 2 ,component,s"_s));
        }

        Shop shop;
        TRY_EXTRACT(bits[0].data[0], shop.m);
        TRY_EXTRACT(bits[0].data[1], shop.x);
        TRY_EXTRACT(bits[0].data[2], shop.y);
        TRY_EXTRACT(bits[0].data[3], shop.d);
        shop.key_span = bits[1].data[0].span;
        TRY_EXTRACT(bits[2].data[0], shop.name);
        TRY_EXTRACT(bits[3].data[0], shop.npc_class);
        shop.items.span = bits[3].span;
        shop.items.span.begin = bits[3].data[1].span.begin;
        for (size_t i = 1; i < bits[3].data.size(); ++i)
        {
            shop.items.data.emplace_back();
            auto& item = shop.items.data.back();
            auto& data = bits[3].data[i];
            assert(data.span.begin.line == data.span.end.line);
            item.span = data.span;

            XString name, value;
            if (!extract(data.data, record<':'>(&name, &value)))
                return Err(data.span.error_str("Failed to split item:value"_s));
            item.data.name.span = item.span;
            item.data.name.span.end.column = item.data.name.span.begin.column + name.size() - 1;
            item.data.value.span = item.span;
            item.data.value.span.begin.column = item.data.name.span.begin.column + name.size() + 1;
            if (name.endswith(' '))
            {
                item.data.name.span.warning("Shop item has useless space before the colon"_s);
                name = name.rstrip();
            }
            if (name.is_digit10())
            {
                item.data.name.span.warning("Shop item is an id; should be a name"_s);
            }
            if (!extract(name, &item.data.name.data))
            {
                return Err(item.data.name.span.error_str("item name problem (too long?)"_s));
            }
            item.data.value_multiply = false;
            if (value.startswith('-'))
            {
                item.data.value.span.begin.warning("Shop value multiplier should use '*' instead of '-' now"_s);
                item.data.value_multiply = true;
                item.data.value.span.begin.column += 1;
                value = value.xslice_t(1);
            }
            else if (value.startswith('*'))
            {
                item.data.value_multiply = true;
                item.data.value.span.begin.column += 1;
                value = value.xslice_t(1);
            }
            if (!extract(value, &item.data.value.data))
            {
                return Err(item.data.value.span.error_str("invalid item value"_s));
            }
        }
        return Ok(std::move(shop));
    }
    static
    Result<Monster> parse_monster(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        if (bits.size() != 4)
        {
            return Err(span.error_str("expect 4 |component|s"_s));
        }
        if (bits[0].data.size() != 3 && bits[0].data.size() != 5)
        {
            return Err(bits[0].span.error_str("in |component 1| expect 3 or 5 ,component,s"_s));
        }
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "monster"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }
        if (bits[3].data.size() != 2 && bits[3].data.size() != 4 && bits[3].data.size() != 5)
        {
            return Err(bits[3].span.error_str("in |component 4| expect 2, 4, or 5 ,component,s"_s));
        }

        Monster mob;
        TRY_EXTRACT(bits[0].data[0], mob.m);
        TRY_EXTRACT(bits[0].data[1], mob.x);
        TRY_EXTRACT(bits[0].data[2], mob.y);
        if (bits[0].data.size() >= 5)
        {
            TRY_EXTRACT(bits[0].data[3], mob.xs);
            TRY_EXTRACT(bits[0].data[4], mob.ys);
        }
        else
        {
            mob.xs.data = 0;
            mob.xs.span = bits[0].data[2].span;
            mob.xs.span.end.column++;
            mob.xs.span.begin.column = mob.xs.span.end.column;
            mob.ys.data = 0;
            mob.ys.span = mob.xs.span;
        }
        mob.key_span = bits[1].data[0].span;
        TRY_EXTRACT(bits[2].data[0], mob.name);
        TRY_EXTRACT(bits[3].data[0], mob.mob_class);
        TRY_EXTRACT(bits[3].data[1], mob.num);
        if (bits[3].data.size() >= 4)
        {
            static_assert(std::is_same<decltype(mob.delay1.data)::period, std::chrono::milliseconds::period>::value, "delay1 is milliseconds");
            static_assert(std::is_same<decltype(mob.delay2.data)::period, std::chrono::milliseconds::period>::value, "delay2 is milliseconds");
            if (bits[3].data[2].data.is_digit10())
                bits[3].data[2].span.warning("delay1 lacks units; defaulting to ms"_s);
            if (bits[3].data[3].data.is_digit10())
                bits[3].data[3].span.warning("delay2 lacks units; defaulting to ms"_s);
            TRY_EXTRACT(bits[3].data[2], mob.delay1);
            TRY_EXTRACT(bits[3].data[3], mob.delay2);
            if (bits[3].data.size() >= 5)
            {
                TRY_EXTRACT(bits[3].data[4], mob.event);
            }
            else
            {
                mob.event.data = NpcEvent();
                mob.event.span = bits[3].data[3].span;
                mob.event.span.end.column++;
                mob.event.span.begin.column = mob.event.span.end.column;
            }
        }
        else
        {
            mob.delay1.data = std::chrono::milliseconds::zero();
            mob.delay1.span = bits[3].data[1].span;
            mob.delay1.span.end.column++;
            mob.delay1.span.begin.column = mob.delay1.span.end.column;
            mob.delay2.data = std::chrono::milliseconds::zero();
            mob.delay2.span = mob.delay1.span;
            mob.event.data = NpcEvent();
            mob.event.span = mob.delay1.span;
        }
        return Ok(std::move(mob));
    }
    static
    Result<MapFlag> parse_mapflag(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        if (bits.size() != 3 && bits.size() != 4)
        {
            return Err(span.error_str("expect 3 or 4 |component|s"_s));
        }
        if (bits[0].data.size() != 1)
        {
            return Err(bits[0].span.error_str("in |component 1| expect 1 ,component,s"_s));
        }
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "mapflag"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }

        MapFlag mapflag;
        TRY_EXTRACT(bits[0].data[0], mapflag.m);
        mapflag.key_span = bits[1].data[0].span;
        TRY_EXTRACT(bits[2].data[0], mapflag.name);
        if (bits.size() >= 4)
        {
            mapflag.vec_extra.span = bits[3].span;
            for (auto& bit : bits[3].data)
            {
                mapflag.vec_extra.data.emplace_back();
                TRY_EXTRACT(bit, mapflag.vec_extra.data.back());
            }
        }
        else
        {
            mapflag.vec_extra.data = {};
            mapflag.vec_extra.span = bits[2].span;
            mapflag.vec_extra.span.end.column++;
            mapflag.vec_extra.span.begin.column = mapflag.vec_extra.span.end.column;
        }
        return Ok(std::move(mapflag));
    }
    static
    Result<ScriptFunction> parse_script_function_head(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        //  ScriptFunction:     function|script|Fun Name{code}
        if (bits.size() != 3)
        {
            return Err(span.error_str("expect 3 |component|s"_s));
        }
        assert(bits[0].data.size() == 1);
        assert(bits[0].data[0].data == "function"_s);
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "script"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }

        ScriptFunction script_function;
        script_function.key1_span = bits[0].data[0].span;
        TRY_EXTRACT(bits[2].data[0], script_function.name);
        // also expect '{' and parse real script (in caller)
        return Ok(std::move(script_function));
    }
    static
    Result<ScriptNone> parse_script_none_head(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        //  ScriptNone:         -|script|script name{code}
        if (bits.size() != 3)
        {
            return Err(span.error_str("expect 3 |component|s"_s));
        }
        assert(bits[0].data.size() == 1);
        assert(bits[0].data[0].data == "-"_s);
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "script"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }

        ScriptNone script_none;
        script_none.key1_span = bits[0].data[0].span;
        TRY_EXTRACT(bits[2].data[0], script_none.name);
        // also expect '{' and parse real script (in caller)
        return Ok(std::move(script_none));
    }
    static
    Result<ScriptMap> parse_script_map_head(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits)
    {
        //  ScriptMap:          m,x,y,d|script|script name|class,xs,ys{code}
        if (bits.size() != 4)
        {
            return Err(span.error_str("expect 4 |component|s"_s));
        }
        if (bits[0].data.size() != 4)
        {
            return Err(bits[0].span.error_str("in |component 1| expect 3 ,component,s"_s));
        }
        assert(bits[1].data.size() == 1);
        assert(bits[1].data[0].data == "script"_s);
        if (bits[2].data.size() != 1)
        {
            return Err(bits[2].span.error_str("in |component 3| expect 1 ,component,s"_s));
        }
        if (bits[3].data.size() != 1 && bits[3].data.size() != 3)
        {
            return Err(bits[3].span.error_str("in |component 4| expect 1 or 3 ,component,s"_s));
        }

        ScriptMap script_map;
        TRY_EXTRACT(bits[0].data[0], script_map.m);
        TRY_EXTRACT(bits[0].data[1], script_map.x);
        TRY_EXTRACT(bits[0].data[2], script_map.y);
        TRY_EXTRACT(bits[0].data[3], script_map.d);
        TRY_EXTRACT(bits[2].data[0], script_map.name);
        TRY_EXTRACT(bits[3].data[0], script_map.npc_class);
        if (bits[3].data.size() >= 3)
        {
            TRY_EXTRACT(bits[3].data[1], script_map.xs);
            script_map.xs.data = script_map.xs.data * 2 + 1;
            TRY_EXTRACT(bits[3].data[2], script_map.ys);
            script_map.ys.data = script_map.ys.data * 2 + 1;
        }
        else
        {
            script_map.xs.data = 0;
            script_map.xs.span = script_map.npc_class.span;
            script_map.xs.span.end.column++;
            script_map.xs.span.begin = script_map.xs.span.end;
            script_map.ys.data = 0;
            script_map.ys.span = script_map.xs.span;
        }
        // also expect '{' and parse real script (in caller)
        return Ok(std::move(script_map));
    }
    static
    Result<Script> parse_script_any(io::LineSpan span, std::vector<Spanned<std::vector<Spanned<RString>>>>& bits, io::LineCharReader& lr)
    {
        // 3 cases:
        //  ScriptFunction:     function|script|Fun Name{code}
        //  ScriptNone:         -|script|script name|32767{code}
        //  ScriptMap:          m,x,y,d|script|script name|class,xs,ys{code}
        if (bits[0].data[0].data == "function"_s)
        {
            Script rv = TRY(parse_script_function_head(span, bits));
            rv.key_span = bits[1].data[0].span;

            ast::script::ScriptOptions opt;
            opt.implicit_start = true;
            opt.default_label = "OnCall"_s;
            opt.no_event = true;
            rv.body = TRY(ast::script::parse_script_body(lr, opt));
            return Ok(std::move(rv));
        }
        else if (bits[0].data[0].data == "-"_s)
        {
            Script rv = TRY(parse_script_none_head(span, bits));
            rv.key_span = bits[1].data[0].span;

            ast::script::ScriptOptions opt;
            opt.implicit_start = true;
            opt.default_label = "OnCall"_s;
            rv.body = TRY(ast::script::parse_script_body(lr, opt));
            return Ok(std::move(rv));
        }
        else
        {
            ScriptMap script_map = TRY(parse_script_map_head(span, bits));
            bool no_touch = !script_map.xs.data && !script_map.ys.data;
            Script rv = std::move(script_map);
            rv.key_span = bits[1].data[0].span;

            ast::script::ScriptOptions opt;
            opt.implicit_start = true;
            opt.default_label = "OnClick"_s;
            opt.no_touch = no_touch;
            rv.body = TRY(ast::script::parse_script_body(lr, opt));
            return Ok(std::move(rv));
        }
    }

    /// Try to extract a top-level token
    /// Return None at EOL, or Some(span)
    /// This will alternate betweeen returning words and separators
    static
    Option<Spanned<RString>> lex(io::LineCharReader& lr, bool first)
    {
        // you know, I just realized a lot of the if (.get()) checks are not
        // actually going to fail, since LineCharReader guarantees the \n
        // occurs before EOF
        io::LineChar c;
        // at start of line, skip whitespace
        if (first)
        {
            while (lr.get(c) && (/*c.ch() == ' ' ||*/ c.ch() == '\n'))
            {
                lr.adv();
            }
        }
        // at end of file, end of line, or start of script, signal end
        if (!lr.get(c) || c.ch() == '\n' || c.ch() == '{')
        {
            return None;
        }
        // separators are simple ... NOT.
        // Reasonably, we should only ever eat a single char here.
        // Unfortunately, we aren't dealing with reasonable people here.
        if (c.ch() == '|' || c.ch() == ',')
        {
            lr.adv();
            while (true)
            {
                io::LineChar c2;
                if (!lr.get(c2) || c2.ch() == '\n' || c2.ch() == '{')
                {
                    c.warning("Separator at EOL"_s);
                    return None;
                }
                if (c2.ch() == ',' || c2.ch() == '|')
                {
                    lr.adv();
                    c = c2;
                    c.warning("Adjacent separators"_s);
                    continue;
                }
                break;
            }
            return Some(respan({c, c}, RString(VString<1>(c.ch()))));
        }
        io::LineSpan span;
        MString accum;
        accum += c.ch();
        span.begin = c;
        span.end = c;
        lr.adv();
        if (c.ch() != '/')
            first = false;

        // if one-char token followed by an end-of-line or separator, stop
        if (!lr.get(c) || c.ch() == '\n' || c.ch() == '{' || c.ch() == ',' || c.ch() == '|')
        {
            return Some(respan(span, RString(accum)));
        }

        accum += c.ch();
        span.end = c;
        lr.adv();

        // if first token on line, can get comment
        if (first && c.ch() == '/')
        {
            while (lr.get(c) && c.ch() != '\n')
            {
                accum += c.ch();
                span.end = c;
                lr.adv();
            }
            return Some(respan(span, RString(accum)));
        }
        // otherwise, collect until an end of line or separator
        while (lr.get(c) && c.ch() != '\n' && c.ch() != '{' && c.ch() != ',' && c.ch() != '|')
        {
            accum += c.ch();
            span.end = c;
            lr.adv();
        }
        return Some(respan(span, RString(accum)));
    }

    Option<Result<TopLevel>> parse_top(io::LineCharReader& in)
    {
        Spanned<std::vector<Spanned<std::vector<Spanned<RString>>>>> bits;

        // special logic for the first 'bit'
        {
            Spanned<RString> mayc = TRY_UNWRAP(lex(in, true),
                    {
                        io::LineChar c;
                        if (in.get(c) && c.ch() == '{')
                        {
                            return Err(c.error_str("Unexpected script open"_s));
                        }
                        return None;
                    });
            if (mayc.data.startswith("//"_s))
            {
                Comment com;
                com.comment = std::move(mayc.data);
                TopLevel rv = std::move(com);
                rv.span = std::move(mayc.span);
                return Some(Ok(std::move(rv)));
            }

            if (mayc.data == "|"_s || mayc.data == ","_s)
                return Err(mayc.span.error_str("Unexpected separator"_s));
            bits.span = mayc.span;
            bits.data.emplace_back();
            bits.data.back().span = mayc.span;
            bits.data.back().data.push_back(mayc);
        }

        while (true)
        {
            Spanned<RString> sep = TRY_UNWRAP(lex(in, false),
                    break);
            if (sep.data == "|"_s)
            {
                bits.data.emplace_back();
            }
            else if (sep.data != ","_s)
            {
                return Err(sep.span.error_str("Expected separator"_s));
            }

            Spanned<RString> word = TRY_UNWRAP(lex(in, false),
                    return Err(sep.span.error_str("Expected word after separator"_s)));
            if (bits.data.back().data.empty())
                bits.data.back().span = word.span;
            else
                bits.data.back().span.end = word.span.end;
            bits.span.end = word.span.end;
            bits.data.back().data.push_back(std::move(word));
        }

        if (bits.data.size() < 2)
            return Err(bits.span.error_str("Expected a line with |s in it"_s));
        for (auto& bit : bits.data)
        {
            if (bit.data.empty())
                return Err(bit.span.error_str("Empty components are not cool"_s));
        }
        if (bits.data[1].data.size() != 1)
            return Err(bits.data[1].span.error_str("Expected a single word in type position"_s));
        Spanned<RString>& w2 = bits.data[1].data[0];
        if (w2.data == "warp"_s)
        {
            TopLevel rv = TRY(parse_warp(bits.span, bits.data));
            rv.span = bits.span;
            return Some(Ok(std::move(rv)));
        }
        else if (w2.data == "shop"_s)
        {
            TopLevel rv = TRY(parse_shop(bits.span, bits.data));
            rv.span = bits.span;
            return Some(Ok(std::move(rv)));
        }
        else if (w2.data == "monster"_s)
        {
            TopLevel rv = TRY(parse_monster(bits.span, bits.data));
            rv.span = bits.span;
            return Some(Ok(std::move(rv)));
        }
        else if (w2.data == "mapflag"_s)
        {
            TopLevel rv = TRY(parse_mapflag(bits.span, bits.data));
            rv.span = bits.span;
            return Some(Ok(std::move(rv)));
        }
        else if (w2.data == "script"_s)
        {
            TopLevel rv = TRY_MOVE(parse_script_any(bits.span, bits.data, in));
            rv.span = bits.span;
            return Some(Ok(std::move(rv)));
        }
        else
        {
            return Err(w2.span.error_str("Unknown type"_s));
        }
    }
} // namespace npc
} // namespace ast
} // namespace tmwa
