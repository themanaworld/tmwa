#pragma once
//    ast/quest.hpp - Structure of tmwa questdb
//
//    Copyright © 2015 Ed Pasek <pasekei@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "fwd.hpp"

#include "../compat/result.hpp"

#include "../io/span.hpp"

#include "../sexpr/variant.hpp"

#include "../mmo/clif.t.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/strs.hpp"

namespace tmwa
{
namespace ast
{
namespace quest
{
    using io::Spanned;

    struct Comment
    {
        RString comment;
    };
    struct Quest
    {
        Spanned<QuestId> questid;
        Spanned<VarName> quest_var;
        Spanned<VarName> quest_vr;
        Spanned<int> quest_shift;
        Spanned<int> quest_mask;
    };

    using QuestOrCommentBase = Variant<Comment, Quest>;
    struct QuestOrComment : QuestOrCommentBase
    {
        QuestOrComment(Comment o) : QuestOrCommentBase(std::move(o)) {}
        QuestOrComment(Quest o) : QuestOrCommentBase(std::move(o)) {}
        io::LineSpan span;
    };

    Option<Result<QuestOrComment>> parse_quest(io::LineCharReader& lr);
} // namespace quest
} // namespace ast
} // namespace tmwa
