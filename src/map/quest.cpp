#include "quest.hpp"
//    quest.cpp - Quest Log.
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

#include <algorithm>

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/line.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/extract_enums.hpp"

#include "../ast/quest.hpp"

#include "../poison.hpp"
#include "globals.hpp"
#include "script-parse.hpp"

namespace tmwa
{
namespace map
{
// Function declarations

static
void questdb_searchname_sub(Borrowed<struct quest_data> quest, VarName str, Borrowed<Option<Borrowed<struct quest_data>>> dst)
{
    if (quest->quest_var == str)
        *dst = Some(quest);
}

Option<Borrowed<struct quest_data>> questdb_searchname(XString str_)
{
    VarName str = stringish<VarName>(str_);
    if (XString(str) != str_)
        return None;
    Option<P<struct quest_data>> quest = None;
    for (auto& pair : quest_db)
        questdb_searchname_sub(borrow(pair.second), str, borrow(quest));
    return quest;
}

Borrowed<struct quest_data> questdb_search(QuestId questid)
{
    Option<P<struct quest_data>> id_ = quest_db.search(questid);
    OMATCH_BEGIN_SOME (id, id_)
    {
        return id;
    }
    OMATCH_END ();

    P<struct quest_data> id = quest_db.init(questid);

    id->questid = questid;

    return id;
}

Option<Borrowed<struct quest_data>> questdb_exists(QuestId questid)
{
    return quest_db.search(questid);
}

bool quest_readdb(ZString filename)
{
    io::LineCharReader in(filename);

    if (!in.is_open())
    {
        PRINTF("can't read %s\n"_fmt, filename);
        return false;
    }

    int ln = 0;

    while (true)
    {
        auto res = TRY_UNWRAP(ast::quest::parse_quest(in),
                {
                    PRINTF("read %s done (count=%d)\n"_fmt, filename, ln);
                    return true;
                });
        if (res.get_failure())
            PRINTF("%s\n"_fmt, res.get_failure());
        ast::quest::QuestOrComment ioc = TRY_UNWRAP(std::move(res.get_success()), return false);

        MATCH_BEGIN (ioc)
        {
            MATCH_CASE (const ast::quest::Comment&, c)
            {
                (void)c;
            }
            MATCH_CASE (const ast::quest::Quest&, quest)
            {
                ln++;

                quest_data qdv {};
                qdv.questid = quest.questid.data;
                qdv.quest_var = quest.quest_var.data;
                qdv.quest_vr = quest.quest_vr.data;
                qdv.quest_shift = quest.quest_shift.data;
                qdv.quest_mask = quest.quest_mask.data;

                Borrowed<struct quest_data> id = questdb_search(qdv.questid);
                *id = std::move(qdv);
            }
        }
        MATCH_END ();
    }
}
} // namespace map
} // namespace tmwa
