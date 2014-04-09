#ifndef TMWA_MAP_ITEMDB_HPP
#define TMWA_MAP_ITEMDB_HPP
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

# include "../sanity.hpp"

# include "../mmo/mmo.hpp"

# include "map.t.hpp"
# include "script.hpp"

struct item_data
{
    int nameid;
    ItemName name, jname;
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
    std::unique_ptr<const ScriptBuffer> use_script;
    std::unique_ptr<const ScriptBuffer> equip_script;
};

struct random_item_data
{
    int nameid;
    int per;
};

inline
struct item_data *itemdb_searchname(ItemName) = delete;
struct item_data *itemdb_searchname(XString name);
struct item_data *itemdb_search(int nameid);
struct item_data *itemdb_exists(int nameid);

inline
ItemType itemdb_type(int n)
{
    return itemdb_search(n)->type;
}
inline
ItemLook itemdb_look(int n)
{
    return itemdb_search(n)->look;
}
inline
int itemdb_weight(int n)
{
    return itemdb_search(n)->weight;
}
inline
const ScriptBuffer *itemdb_equipscript(int n)
{
    return itemdb_search(n)->equip_script.get();
}
inline
int itemdb_wlv(int n)
{
    return itemdb_search(n)->wlv;
}
inline
int itemdb_value_sell(int n)
{
    return itemdb_search(n)->value_sell;
}

int itemdb_isequip(int);
int itemdb_isequip2(struct item_data *);
int itemdb_isequip3(int);

void itemdb_reload(void);

void do_final_itemdb(void);
bool itemdb_readdb(ZString filename);

#endif // TMWA_MAP_ITEMDB_HPP
