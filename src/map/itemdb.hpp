#ifndef ITEMDB_HPP
#define ITEMDB_HPP

# include "../common/mmo.hpp"

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

struct item_data *itemdb_searchname(ItemName name);
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

#endif // ITEMDB_HPP
