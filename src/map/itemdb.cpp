#include "itemdb.hpp"
//    itemdb.cpp - Item definitions.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../ast/item.hpp"

#include "globals.hpp"
#include "script-parse.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
// Function declarations

/*==========================================
 * 名前で検索用
 *------------------------------------------
 */
// name = item alias, so we should find items aliases first. if not found then look for "jname" (full name)
static
void itemdb_searchname_sub(Borrowed<struct item_data> item, ItemName str, Borrowed<Option<Borrowed<struct item_data>>> dst)
{
    if (item->name == str)
        *dst = Some(item);
}

/*==========================================
 * 名前で検索
 *------------------------------------------
 */
Option<Borrowed<struct item_data>> itemdb_searchname(XString str_)
{
    ItemName str = stringish<ItemName>(str_);
    if (XString(str) != str_)
        return None;
    Option<P<struct item_data>> item = None;
    for (auto& pair : item_db)
        itemdb_searchname_sub(borrow(pair.second), str, borrow(item));
    return item;
}

/*==========================================
 * DBの存在確認
 *------------------------------------------
 */
Option<Borrowed<struct item_data>> itemdb_exists(ItemNameId nameid)
{
    return item_db.search(nameid);
}

/*==========================================
 * DBの検索
 *------------------------------------------
 */
Borrowed<struct item_data> itemdb_search(ItemNameId nameid)
{
    Option<P<struct item_data>> id_ = item_db.search(nameid);
    OMATCH_BEGIN_SOME (id, id_)
    {
        return id;
    }
    OMATCH_END ();

    P<struct item_data> id = item_db.init(nameid);

    id->nameid = nameid;
    id->value_buy = 10;
    id->value_sell = id->value_buy / 2;
    id->weight = 10;
    id->sex = SEX::NEUTRAL;
    id->elv = 0;

    id->type = ItemType::JUNK;

    return id;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip(ItemNameId nameid)
{
    ItemType type = itemdb_type(nameid);
    return !(type == ItemType::USE
        || type == ItemType::_2
        || type == ItemType::JUNK
        || type == ItemType::_6
        || type == ItemType::ARROW);
}

/*==========================================
 *
 *------------------------------------------
 */
bool itemdb_isequip2(Borrowed<struct item_data> data)
{
    ItemType type = data->type;
    return !(type == ItemType::USE
        || type == ItemType::_2
        || type == ItemType::JUNK
        || type == ItemType::_6
        || type == ItemType::ARROW);
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip3(ItemNameId nameid)
{
    ItemType type = itemdb_type(nameid);
    return (type == ItemType::WEAPON
        || type == ItemType::ARMOR
        || type == ItemType::_8);
}

bool itemdb_readdb(ZString filename)
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
        auto res = TRY_UNWRAP(ast::item::parse_item(in),
                {
                    PRINTF("read %s done (count=%d)\n"_fmt, filename, ln);
                    return true;
                });
        if (res.get_failure())
            PRINTF("%s\n"_fmt, res.get_failure());
        ast::item::ItemOrComment ioc = TRY_UNWRAP(std::move(res.get_success()), return false);

        MATCH_BEGIN (ioc)
        {
            MATCH_CASE (const ast::item::Comment&, c)
            {
                (void)c;
            }
            MATCH_CASE (const ast::item::Item&, item)
            {
                ln++;

                item_data idv {};
                idv.nameid = item.id.data;
                idv.name = item.name.data;
                idv.jname = item.jname.data;
                idv.type = item.type.data;
                idv.value_buy = item.buy_price.data ?: item.sell_price.data * 2;
                idv.value_sell = item.sell_price.data ?: item.buy_price.data / 2;
                idv.weight = item.weight.data;
                idv.atk = item.atk.data;
                idv.def = item.def.data;
                idv.range = item.range.data;
                idv.magic_bonus = item.magic_bonus.data;
                idv.sex = item.gender.data;
                idv.equip = item.loc.data;
                idv.wlv = item.wlv.data;
                idv.elv = item.elv.data;
                idv.look = item.view.data;

                idv.use_script = compile_script(STRPRINTF("use script %d"_fmt, idv.nameid), item.use_script, true);
                idv.equip_script = compile_script(STRPRINTF("equip script %d"_fmt, idv.nameid), item.equip_script, true);

                Borrowed<struct item_data> id = itemdb_search(idv.nameid);
                *id = std::move(idv);
            }
        }
        MATCH_END ();
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void itemdb_final(struct item_data *id)
{
    id->use_script.reset();
    id->equip_script.reset();
}

/*==========================================
 *
 *------------------------------------------
 */
void do_final_itemdb(void)
{
    for (auto& pair : item_db)
        itemdb_final(&pair.second);
    item_db.clear();
}
} // namespace map
} // namespace tmwa
