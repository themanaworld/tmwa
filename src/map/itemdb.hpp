#ifndef ITEMDB_HPP
#define ITEMDB_HPP

#include "../common/mmo.hpp"

#include "map.t.hpp"
#include "script.hpp"

struct item_data
{
    int nameid;
    char name[24], jname[24];
    char prefix[24], suffix[24];
    char cardillustname[64];
    int value_buy;
    int value_sell;
    ItemType type;
    int sex;
    EPOS equip;
    int weight;
    int atk;
    int def;
    int range;
    int magic_bonus;
    int slot;
    ItemLook look;
    int elv;
    int wlv;
    int refine;
    std::unique_ptr<const ScriptBuffer> use_script;
    std::unique_ptr<const ScriptBuffer> equip_script;
};

struct random_item_data
{
    int nameid;
    int per;
};

struct item_data *itemdb_searchname(const char *name);
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
void do_init_itemdb(void);

#endif // ITEMDB_HPP
