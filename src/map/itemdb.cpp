#include "itemdb.hpp"

#include <cstdlib>
#include <cstring>

#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/random.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"

#include "../poison.hpp"

constexpr int MAX_RANDITEM = 2000;

// ** ITEMDB_OVERRIDE_NAME_VERBOSE **
//   定義すると、itemdb.txtとgrfで名前が異なる場合、表示します.
//#define ITEMDB_OVERRIDE_NAME_VERBOSE  1

static
Map<int, struct item_data> item_db;

// Function declarations

static
void itemdb_read(void);
static
int itemdb_readdb(void);

/*==========================================
 * 名前で検索用
 *------------------------------------------
 */
// name = item alias, so we should find items aliases first. if not found then look for "jname" (full name)
static
void itemdb_searchname_sub(struct item_data *item, const char *str, struct item_data **dst)
{
    if (strcasecmp(item->name, str) == 0) //by lupus
        *dst = item;
}

/*==========================================
 * 名前で検索
 *------------------------------------------
 */
struct item_data *itemdb_searchname(const char *str)
{
    struct item_data *item = NULL;
    for (auto& pair : item_db)
        itemdb_searchname_sub(&pair.second, str, &item);
    return item;
}

/*==========================================
 * DBの存在確認
 *------------------------------------------
 */
struct item_data *itemdb_exists(int nameid)
{
    return item_db.search(nameid);
}

/*==========================================
 * DBの検索
 *------------------------------------------
 */
struct item_data *itemdb_search(int nameid)
{
    struct item_data *id = item_db.search(nameid);
    if (id)
        return id;

    id = item_db.init(nameid);

    id->nameid = nameid;
    id->value_buy = 10;
    id->value_sell = id->value_buy / 2;
    id->weight = 10;
    id->sex = 2;
    id->elv = 0;

    if (nameid > 500 && nameid < 600)
        id->type = ItemType::USE;
    else if (nameid > 600 && nameid < 700)
        id->type = ItemType::_2;
    else if ((nameid > 700 && nameid < 1100) ||
             (nameid > 7000 && nameid < 8000))
        id->type = ItemType::JUNK;
    else if (nameid >= 1750 && nameid < 1771)
        id->type = ItemType::ARROW;
    else if (nameid > 1100 && nameid < 2000)
        id->type = ItemType::WEAPON;
    else if ((nameid > 2100 && nameid < 3000) ||
             (nameid > 5000 && nameid < 6000))
        id->type = ItemType::ARMOR;
    else if (nameid > 4000 && nameid < 5000)
        id->type = ItemType::_6;

    return id;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip(int nameid)
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
int itemdb_isequip2(struct item_data *data)
{
    if (!data)
        return false;
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
int itemdb_isequip3(int nameid)
{
    ItemType type = itemdb_type(nameid);
    return (type == ItemType::WEAPON
        || type == ItemType::ARMOR
        || type == ItemType::_8);
}

/*==========================================
 * アイテムデータベースの読み込み
 *------------------------------------------
 */
static
int itemdb_readdb(void)
{
    FILE *fp;
    char line[1024];
    int ln = 0, lines = 0;
    int nameid, j;
    struct item_data *id;
    int i = 0;
    const char *filename[] = { "db/item_db.txt", "db/item_db2.txt" };

    for (i = 0; i < 2; i++)
    {

        fp = fopen_(filename[i], "r");
        if (fp == NULL)
        {
            if (i > 0)
                continue;
            PRINTF("can't read %s\n", filename[i]);
            exit(1);
        }

        lines = 0;
        while (fgets(line, 1020, fp))
        {
            lines++;
            if (line[0] == '/' && line[1] == '/')
                continue;
            char *str[32] {};
            char *p;
            char *np;
            for (j = 0, np = p = line; j < 17 && p; j++)
            {
                while (*p == '\t' || *p == ' ')
                    p++;
                str[j] = p;
                p = strchr(p, ',');
                if (p)
                {
                    *p++ = 0;
                    np = p;
                }
            }
            if (str[0] == NULL)
                continue;

            nameid = atoi(str[0]);
            if (nameid <= 0 || nameid >= 20000)
                continue;
            ln++;

            //ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Gender,Loc,wLV,eLV,View
            id = itemdb_search(nameid);
            strzcpy(id->name, str[1], 24);
            strzcpy(id->jname, str[2], 24);
            id->type = ItemType(atoi(str[3]));
            id->value_buy = atoi(str[4]);
            id->value_sell = atoi(str[5]);
            if (id->value_buy == 0 && id->value_sell == 0)
            {
            }
            else if (id->value_buy == 0)
            {
                id->value_buy = id->value_sell * 2;
            }
            else if (id->value_sell == 0)
            {
                id->value_sell = id->value_buy / 2;
            }
            id->weight = atoi(str[6]);
            id->atk = atoi(str[7]);
            id->def = atoi(str[8]);
            id->range = atoi(str[9]);
            id->magic_bonus = atoi(str[10]);
            id->slot = atoi(str[11]);
            id->sex = atoi(str[12]);
            id->equip = EPOS(atoi(str[13]));
            id->wlv = atoi(str[14]);
            id->elv = atoi(str[15]);
            id->look = static_cast<ItemLook>(atoi(str[16]));

            id->use_script = NULL;
            id->equip_script = NULL;

            if ((p = strchr(np, '{')) == NULL)
                continue;
            id->use_script = parse_script(p, lines);

            if ((p = strchr(p + 1, '{')) == NULL)
                continue;
            id->equip_script = parse_script(p, lines);
        }
        fclose_(fp);
        PRINTF("read %s done (count=%d)\n", filename[i], ln);
    }
    return 0;
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

void itemdb_reload(void)
{
    /*
     *
     * <empty item databases>
     * itemdb_read();
     *
     */

    do_init_itemdb();
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

/*
static
FILE *dfp;
static
int itemdebug(void *key,void *data,_va_list ap){
//      struct item_data *id=(struct item_data *)data;
        FPRINTF(dfp,"%6d", (int)key);
        return 0;
}
void itemdebugtxt()
{
        dfp=fopen_("itemdebug.txt","wt");
        numdb_foreach(item_db,itemdebug);
        fclose_(dfp);
}
*/

/*====================================
 * Removed item_value_db, don't re-add
 *------------------------------------
 */
static
void itemdb_read(void)
{
    itemdb_readdb();
}

/*==========================================
 *
 *------------------------------------------
 */
void do_init_itemdb(void)
{
    itemdb_read();
}
