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

#include <memory>

#include "../compat/result.hpp"

#include "../mmo/clif.t.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

#include "../net/timer.t.hpp"

#include "script.hpp"


namespace tmwa
{
namespace npc
{
namespace parse
{
    using io::Spanned;

    struct TopLevel
    {
        io::LineSpan span;

        virtual ~TopLevel();
    };
    struct Comment : TopLevel
    {
        RString comment;
    };
    struct Warp : TopLevel
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
        Spanned<int> value;
    };
    struct Shop : TopLevel
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        Spanned<DIR> d;
        io::LineSpan key_span;
        Spanned<NpcName> name;
        Spanned<Species> npc_class;
        Spanned<std::vector<Spanned<ShopItem>>> items;
    };
    struct Monster : TopLevel
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
    struct MapFlag : TopLevel
    {
        Spanned<MapName> m;
        io::LineSpan key_span;
        // TODO should this extract all the way?
        Spanned<RString> name;
        Spanned<RString> opt_extra;
    };
    struct Script : TopLevel
    {
        io::LineSpan key_span;
        // see src/script/parser.hpp
        script::parse::ScriptBody body;
    };
    struct ScriptFunction : Script
    {
        io::LineSpan key1_span;
        Spanned<RString> name;
    };
    struct ScriptNone : Script
    {
        io::LineSpan key1_span;
        Spanned<NpcName> name;
        io::LineSpan key4_span;
    };
    struct ScriptMapNone : Script
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        Spanned<DIR> d;
        Spanned<NpcName> name;
        io::LineSpan key4_span;
    };
    struct ScriptMap : Script
    {
        Spanned<MapName> m;
        Spanned<unsigned> x, y;
        Spanned<DIR> d;
        Spanned<NpcName> name;
        Spanned<Species> npc_class;
        Spanned<unsigned> xs, ys;
    };
    // other Script subclasses elsewhere? (for item and magic scripts)

    Result<std::unique_ptr<TopLevel>> parse_top(io::LineCharReader& in);
} // namespace parse
} // namespace npc
} // namespace tmwa
