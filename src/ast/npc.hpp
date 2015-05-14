#pragma once
//    ast/npc.hpp - Structure of non-player characters (including mobs).
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

#include "../compat/result.hpp"

#include "../io/span.hpp"

#include "../net/timer.t.hpp"

#include "../sexpr/variant.hpp"

#include "../mmo/clif.t.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "script.hpp"


namespace tmwa
{
namespace ast
{
namespace npc
{
    using io::Spanned;

    struct Comment
    {
        RString comment;
    };
    struct Warp
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        io::LineSpan key_span;
        Spanned<NpcName> name;
        Spanned<unsigned> xs, ys;
        Spanned<MapName> to_m;
        Spanned<unsigned> to_x, to_y;
    };
    struct ShopItem
    {
        Spanned<ItemName> name;
        bool value_multiply;
        Spanned<int> value;
    };
    struct Shop
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        Spanned<DIR> d;
        io::LineSpan key_span;
        Spanned<NpcName> name;
        Spanned<Species> npc_class;
        Spanned<std::vector<Spanned<ShopItem>>> items;
    };
    struct Monster
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        Spanned<unsigned> xs, ys;
        io::LineSpan key_span;
        Spanned<MobName> name;
        Spanned<Species> mob_class;
        Spanned<unsigned> num;
        Spanned<interval_t> delay1, delay2;
        Spanned<NpcEvent> event;
    };
    struct MapFlag
    {
        Spanned<MapName> m;
        io::LineSpan key_span;
        Spanned<RString> name;
        Spanned<std::vector<Spanned<RString>>> vec_extra;
    };
    struct ScriptFunction
    {
        io::LineSpan key1_span;
        Spanned<RString> name;
    };
    struct ScriptNone
    {
        io::LineSpan key1_span;
        Spanned<NpcName> name;
        io::LineSpan key4_span;
    };
    struct ScriptMap
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        Spanned<DIR> d;
        Spanned<NpcName> name;
        Spanned<Species> npc_class;
        Spanned<unsigned> xs, ys;
    };
    using ScriptBase = Variant<ScriptFunction, ScriptNone, ScriptMap>;
    struct Script : ScriptBase
    {
        Script() = default;
        Script(ScriptFunction s) : ScriptBase(std::move(s)) {}
        Script(ScriptNone s) : ScriptBase(std::move(s)) {}
        Script(ScriptMap s) : ScriptBase(std::move(s)) {}

        io::LineSpan key_span;
        ast::script::ScriptBody body;
    };
    using TopLevelBase = Variant<Comment, Warp, Shop, Monster, MapFlag, Script>;
    struct TopLevel : TopLevelBase
    {
        TopLevel() = default;
        TopLevel(Comment t) : TopLevelBase(std::move(t)) {}
        TopLevel(Warp t) : TopLevelBase(std::move(t)) {}
        TopLevel(Shop t) : TopLevelBase(std::move(t)) {}
        TopLevel(Monster t) : TopLevelBase(std::move(t)) {}
        TopLevel(MapFlag t) : TopLevelBase(std::move(t)) {}
        TopLevel(Script t) : TopLevelBase(std::move(t)) {}
        io::LineSpan span;
    };

    Option<Result<TopLevel>> parse_top(io::LineCharReader& in);
} // namespace npc
} // namespace ast
} // namespace tmwa
