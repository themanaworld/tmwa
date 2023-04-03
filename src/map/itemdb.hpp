#pragma once
//    itemdb.hpp - Item definitions.
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

#include "fwd.hpp"

#include "../mmo/ids.hpp"
#include "../high/mmo.hpp"

#include "map.t.hpp"
#include "script-buffer.hpp"


namespace tmwa
{
namespace map
{
struct item_data
{
    ItemNameId nameid;
    ItemName name;
    int value_buy;
    int value_sell;
    ItemType type;
    SEX sex;
    EPOS equip;
    int weight;
    int atk;
    int def;
    int range;
    int magic_bonus;
    ItemLook look;
    int elv;
    int wlv;
    ItemMode mode;

    std::unique_ptr<const ScriptBuffer> use_script;
    std::unique_ptr<const ScriptBuffer> equip_script;
};

struct random_item_data
{
    int nameid;
    int per;
};

inline
Option<Borrowed<struct item_data>> itemdb_searchname(ItemName) = delete;
Option<Borrowed<struct item_data>> itemdb_searchname(XString name);
// TODO this function should die
Borrowed<struct item_data> itemdb_search(ItemNameId nameid);
Option<Borrowed<struct item_data>> itemdb_exists(ItemNameId nameid);

inline
ItemType itemdb_type(ItemNameId n)
{
    return itemdb_search(n)->type;
}
inline
ItemLook itemdb_look(ItemNameId n)
{
    return itemdb_search(n)->look;
}
inline
int itemdb_weight(ItemNameId n)
{
    return itemdb_search(n)->weight;
}
inline
const ScriptBuffer *itemdb_equipscript(ItemNameId n)
{
    return itemdb_search(n)->equip_script.get();
}
inline
int itemdb_wlv(ItemNameId n)
{
    return itemdb_search(n)->wlv;
}
inline
int itemdb_value_sell(ItemNameId n)
{
    return itemdb_search(n)->value_sell;
}

int itemdb_isequip(ItemNameId);
bool itemdb_isequip2(Borrowed<struct item_data>);
int itemdb_isequip3(ItemNameId);

void itemdb_reload(void);

void do_final_itemdb(void);
bool itemdb_readdb(ZString filename);
} // namespace map
} // namespace tmwa
