#pragma once
//    quest.hpp - Quest Log.
//
//    Copyright © 2015 Ed Pasek <pasekei@gmail.com>
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

#include "../mmo/ids.hpp"
#include "../high/mmo.hpp"

#include "map.hpp"
#include "script-buffer.hpp"

namespace tmwa
{
namespace map
{
constexpr int MAX_QUEST_DB (60355+1);
struct quest_data
{
    QuestId questid;
    VarName quest_var;
    VarName quest_vr;
    int quest_shift;
    int quest_mask;
};
inline
Option<Borrowed<struct quest_data>> questdb_searchname(VarName) = delete;
Option<Borrowed<struct quest_data>> questdb_searchname(XString quest_var);
Borrowed<struct quest_data> questdb_search(QuestId questid);
Option<Borrowed<struct quest_data>> questdb_exists(QuestId questid);

// get quest var by quest name / mask / bit
bool quest_readdb(ZString filename);
} // namespace map
} // namespace tmwa
