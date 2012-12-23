#ifndef ITEMDB_HPP
#define ITEMDB_HPP

#include "map.hpp"
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
    int look;
    int elv;
    int wlv;
    int refine;
    const ScriptCode *use_script;
    const ScriptCode *equip_script;
    struct
    {
        unsigned available:1;
        unsigned value_notdc:1;
        unsigned value_notoc:1;
        unsigned no_equip:3;
        unsigned no_drop:1;
        unsigned no_use:1;
    } flag;
    int view_id;
};

struct random_item_data
{
    int nameid;
    int per;
};

struct item_data *itemdb_searchname(const char *name);
struct item_data *itemdb_search(int nameid);
struct item_data *itemdb_exists(int nameid);
#define itemdb_type(n) itemdb_search(n)->type
#define itemdb_look(n) itemdb_search(n)->look
#define itemdb_weight(n) itemdb_search(n)->weight
#define itemdb_equipscript(n) itemdb_search(n)->equip_script
#define itemdb_wlv(n) itemdb_search(n)->wlv
#define itemdb_available(n) (itemdb_exists(n) && itemdb_search(n)->flag.available)
#define itemdb_viewid(n) (itemdb_search(n)->view_id)

int itemdb_searchrandomid(int flags);

#define itemdb_value_sell(n) itemdb_search(n)->value_sell
#define itemdb_value_notdc(n) itemdb_search(n)->flag.value_notdc
#define itemdb_value_notoc(n) itemdb_search(n)->flag.value_notoc

int itemdb_isequip(int);
int itemdb_isequip2(struct item_data *);
int itemdb_isequip3(int);

void itemdb_reload(void);

void do_final_itemdb(void);
int do_init_itemdb(void);

#endif // ITEMDB_HPP
