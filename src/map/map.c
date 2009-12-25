// $Id: map.c,v 1.6 2004/09/25 17:37:01 MouseJstr Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef LCCWIN32
#include <winsock.h>
#else
#include <netdb.h>
#endif

#include "core.h"
#include "timer.h"
#include "db.h"
#include "grfio.h"
#include "malloc.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "mob.h"
#include "chat.h"
#include "itemdb.h"
#include "storage.h"
#include "skill.h"
#include "trade.h"
#include "party.h"
#include "battle.h"
#include "script.h"
#include "guild.h"
#include "atcommand.h"
#include "nullpo.h"
#include "socket.h"
#include "magic.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

// 極力 staticでローカルに収める
static struct dbt *id_db = NULL;
static struct dbt *map_db = NULL;
static struct dbt *nick_db = NULL;
static struct dbt *charid_db = NULL;

static int users = 0;
static struct block_list *object[MAX_FLOORITEM];
static int first_free_object_id = 0, last_object_id = 0;

#define block_free_max 1048576
static void *block_free[block_free_max];
static int block_free_count = 0, block_free_lock = 0;

#define BL_LIST_MAX 1048576
static struct block_list *bl_list[BL_LIST_MAX];
static int bl_list_count = 0;

struct map_data map[MAX_MAP_PER_SERVER];
int  map_num = 0;

int  map_port = 0;

int  autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
int  save_settings = 0xFFFF;
int  agit_flag = 0;
int  night_flag = 0;            // 0=day, 1=night [Yor]

struct charid2nick
{
    char nick[24];
    int  req_id;
};

char motd_txt[256] = "conf/motd.txt";
char help_txt[256] = "conf/help.txt";

char wisp_server_name[24] = "Server";   // can be modified in char-server configuration file

/*==========================================
 * 全map鯖総計での接続数設定
 * (char鯖から送られてくる)
 *------------------------------------------
 */
void map_setusers (int n)
{
    users = n;
}

/*==========================================
 * 全map鯖総計での接続数取得 (/wへの応答用)
 *------------------------------------------
 */
int map_getusers (void)
{
    return users;
}

//
// block削除の安全性確保処理
//

/*==========================================
 * blockをfreeするときfreeの変わりに呼ぶ
 * ロックされているときはバッファにためる
 *------------------------------------------
 */
int map_freeblock (void *bl)
{
    if (block_free_lock == 0)
    {
        free (bl);
        bl = NULL;
    }
    else
    {
        if (block_free_count >= block_free_max)
        {
            if (battle_config.error_log)
                printf
                    ("map_freeblock: *WARNING* too many free block! %d %d\n",
                     block_free_count, block_free_lock);
        }
        else
            block_free[block_free_count++] = bl;
    }
    return block_free_lock;
}

/*==========================================
 * blockのfreeを一時的に禁止する
 *------------------------------------------
 */
int map_freeblock_lock (void)
{
    return ++block_free_lock;
}

/*==========================================
 * blockのfreeのロックを解除する
 * このとき、ロックが完全になくなると
 * バッファにたまっていたblockを全部削除
 *------------------------------------------
 */
int map_freeblock_unlock (void)
{
    if ((--block_free_lock) == 0)
    {
        int  i;
//      if(block_free_count>0) {
//          if(battle_config.error_log)
//              printf("map_freeblock_unlock: free %d object\n",block_free_count);
//      }
        for (i = 0; i < block_free_count; i++)
        {
            free (block_free[i]);
            block_free[i] = NULL;
        }
        block_free_count = 0;
    }
    else if (block_free_lock < 0)
    {
        if (battle_config.error_log)
            printf ("map_freeblock_unlock: lock count < 0 !\n");
    }
    return block_free_lock;
}

//
// block化処理
//
/*==========================================
 * map[]のblock_listから繋がっている場合に
 * bl->prevにbl_headのアドレスを入れておく
 *------------------------------------------
 */
static struct block_list bl_head;

/*==========================================
 * map[]のblock_listに追加
 * mobは数が多いので別リスト
 *
 * 既にlink済みかの確認が無い。危険かも
 *------------------------------------------
 */
int map_addblock (struct block_list *bl)
{
    int  m, x, y;

    nullpo_retr (0, bl);

    if (bl->prev != NULL)
    {
        if (battle_config.error_log)
            printf ("map_addblock error : bl->prev!=NULL\n");
        return 0;
    }

    m = bl->m;
    x = bl->x;
    y = bl->y;
    if (m < 0 || m >= map_num ||
        x < 0 || x >= map[m].xs || y < 0 || y >= map[m].ys)
        return 1;

    if (bl->type == BL_MOB)
    {
        bl->next =
            map[m].block_mob[x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs];
        bl->prev = &bl_head;
        if (bl->next)
            bl->next->prev = bl;
        map[m].block_mob[x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs] = bl;
        map[m].block_mob_count[x / BLOCK_SIZE +
                               (y / BLOCK_SIZE) * map[m].bxs]++;
    }
    else
    {
        bl->next =
            map[m].block[x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs];
        bl->prev = &bl_head;
        if (bl->next)
            bl->next->prev = bl;
        map[m].block[x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs] = bl;
        map[m].block_count[x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs]++;
        if (bl->type == BL_PC)
            map[m].users++;
    }

    return 0;
}

/*==========================================
 * map[]のblock_listから外す
 * prevがNULLの場合listに繋がってない
 *------------------------------------------
 */
int map_delblock (struct block_list *bl)
{
    int  b;
    nullpo_retr (0, bl);

    // 既にblocklistから抜けている
    if (bl->prev == NULL)
    {
        if (bl->next != NULL)
        {
            // prevがNULLでnextがNULLでないのは有ってはならない
            if (battle_config.error_log)
                printf ("map_delblock error : bl->next!=NULL\n");
        }
        return 0;
    }

    b = bl->x / BLOCK_SIZE + (bl->y / BLOCK_SIZE) * map[bl->m].bxs;

    if (bl->type == BL_PC)
        map[bl->m].users--;

    if (bl->next)
        bl->next->prev = bl->prev;
    if (bl->prev == &bl_head)
    {
        // リストの頭なので、map[]のblock_listを更新する
        if (bl->type == BL_MOB)
        {
            map[bl->m].block_mob[b] = bl->next;
            if ((map[bl->m].block_mob_count[b]--) < 0)
                map[bl->m].block_mob_count[b] = 0;
        }
        else
        {
            map[bl->m].block[b] = bl->next;
            if ((map[bl->m].block_count[b]--) < 0)
                map[bl->m].block_count[b] = 0;
        }
    }
    else
    {
        bl->prev->next = bl->next;
    }
    bl->next = NULL;
    bl->prev = NULL;

    return 0;
}

/*==========================================
 * 周囲のPC人数を数える (現在未使用)
 *------------------------------------------
 */
int map_countnearpc (int m, int x, int y)
{
    int  bx, by, c = 0;
    struct block_list *bl = NULL;

    if (map[m].users == 0)
        return 0;
    for (by = y / BLOCK_SIZE - AREA_SIZE / BLOCK_SIZE - 1;
         by <= y / BLOCK_SIZE + AREA_SIZE / BLOCK_SIZE + 1; by++)
    {
        if (by < 0 || by >= map[m].bys)
            continue;
        for (bx = x / BLOCK_SIZE - AREA_SIZE / BLOCK_SIZE - 1;
             bx <= x / BLOCK_SIZE + AREA_SIZE / BLOCK_SIZE + 1; bx++)
        {
            if (bx < 0 || bx >= map[m].bxs)
                continue;
            bl = map[m].block[bx + by * map[m].bxs];
            for (; bl; bl = bl->next)
            {
                if (bl->type == BL_PC)
                    c++;
            }
        }
    }
    return c;
}

/*==========================================
 * セル上のPCとMOBの数を数える (グランドクロス用)
 *------------------------------------------
 */
int map_count_oncell (int m, int x, int y)
{
    int  bx, by;
    struct block_list *bl = NULL;
    int  i, c;
    int  count = 0;

    if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
        return 1;
    bx = x / BLOCK_SIZE;
    by = y / BLOCK_SIZE;

    bl = map[m].block[bx + by * map[m].bxs];
    c = map[m].block_count[bx + by * map[m].bxs];
    for (i = 0; i < c && bl; i++, bl = bl->next)
    {
        if (bl->x == x && bl->y == y && bl->type == BL_PC)
            count++;
    }
    bl = map[m].block_mob[bx + by * map[m].bxs];
    c = map[m].block_mob_count[bx + by * map[m].bxs];
    for (i = 0; i < c && bl; i++, bl = bl->next)
    {
        if (bl->x == x && bl->y == y)
            count++;
    }
    if (!count)
        count = 1;
    return count;
}

/*==========================================
 * map m (x0,y0)-(x1,y1)内の全objに対して
 * funcを呼ぶ
 * type!=0 ならその種類のみ
 *------------------------------------------
 */
void map_foreachinarea (int (*func) (struct block_list *, va_list), int m,
                        int x0, int y0, int x1, int y1, int type, ...)
{
    int  bx, by;
    struct block_list *bl = NULL;
    va_list ap = NULL;
    int  blockcount = bl_list_count, i, c;

    if (m < 0)
        return;
    va_start (ap, type);
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= map[m].xs)
        x1 = map[m].xs - 1;
    if (y1 >= map[m].ys)
        y1 = map[m].ys - 1;
    if (type == 0 || type != BL_MOB)
        for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                bl = map[m].block[bx + by * map[m].bxs];
                c = map[m].block_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type && bl->type != type)
                        continue;
                    if (bl && bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                        && bl->y <= y1 && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
            }
        }
    if (type == 0 || type == BL_MOB)
        for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                bl = map[m].block_mob[bx + by * map[m].bxs];
                c = map[m].block_mob_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                        && bl->y <= y1 && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
            }
        }

    if (bl_list_count >= BL_LIST_MAX)
    {
        if (battle_config.error_log)
            printf ("map_foreachinarea: *WARNING* block count too many!\n");
    }

    map_freeblock_lock ();      // メモリからの解放を禁止する

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev)   // 有効かどうかチェック
            func (bl_list[i], ap);

    map_freeblock_unlock ();    // 解放を許可する

    va_end (ap);
    bl_list_count = blockcount;
}

/*==========================================
 * 矩形(x0,y0)-(x1,y1)が(dx,dy)移動した時の
 * 領域外になる領域(矩形かL字形)内のobjに
 * 対してfuncを呼ぶ
 *
 * dx,dyは-1,0,1のみとする（どんな値でもいいっぽい？）
 *------------------------------------------
 */
void map_foreachinmovearea (int (*func) (struct block_list *, va_list), int m,
                            int x0, int y0, int x1, int y1, int dx, int dy,
                            int type, ...)
{
    int  bx, by;
    struct block_list *bl = NULL;
    va_list ap = NULL;
    int  blockcount = bl_list_count, i, c;

    va_start (ap, type);
    if (dx == 0 || dy == 0)
    {
        // 矩形領域の場合
        if (dx == 0)
        {
            if (dy < 0)
            {
                y0 = y1 + dy + 1;
            }
            else
            {
                y1 = y0 + dy - 1;
            }
        }
        else if (dy == 0)
        {
            if (dx < 0)
            {
                x0 = x1 + dx + 1;
            }
            else
            {
                x1 = x0 + dx - 1;
            }
        }
        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= map[m].xs)
            x1 = map[m].xs - 1;
        if (y1 >= map[m].ys)
            y1 = map[m].ys - 1;
        for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                bl = map[m].block[bx + by * map[m].bxs];
                c = map[m].block_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type && bl->type != type)
                        continue;
                    if (bl && bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                        && bl->y <= y1 && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
                bl = map[m].block_mob[bx + by * map[m].bxs];
                c = map[m].block_mob_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type && bl->type != type)
                        continue;
                    if (bl && bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                        && bl->y <= y1 && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
            }
        }
    }
    else
    {
        // L字領域の場合

        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= map[m].xs)
            x1 = map[m].xs - 1;
        if (y1 >= map[m].ys)
            y1 = map[m].ys - 1;
        for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                bl = map[m].block[bx + by * map[m].bxs];
                c = map[m].block_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type && bl->type != type)
                        continue;
                    if ((bl)
                        && !(bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                             && bl->y <= y1))
                        continue;
                    if ((bl)
                        && ((dx > 0 && bl->x < x0 + dx)
                            || (dx < 0 && bl->x > x1 + dx) || (dy > 0
                                                               && bl->y <
                                                               y0 + dy)
                            || (dy < 0 && bl->y > y1 + dy))
                        && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
                bl = map[m].block_mob[bx + by * map[m].bxs];
                c = map[m].block_mob_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type && bl->type != type)
                        continue;
                    if ((bl)
                        && !(bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                             && bl->y <= y1))
                        continue;
                    if ((bl)
                        && ((dx > 0 && bl->x < x0 + dx)
                            || (dx < 0 && bl->x > x1 + dx) || (dy > 0
                                                               && bl->y <
                                                               y0 + dy)
                            || (dy < 0 && bl->y > y1 + dy))
                        && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
            }
        }

    }

    if (bl_list_count >= BL_LIST_MAX)
    {
        if (battle_config.error_log)
            printf ("map_foreachinarea: *WARNING* block count too many!\n");
    }

    map_freeblock_lock ();      // メモリからの解放を禁止する

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev)   // 有効かどうかチェック
            func (bl_list[i], ap);

    map_freeblock_unlock ();    // 解放を許可する

    va_end (ap);
    bl_list_count = blockcount;
}

// -- moonsoul  (added map_foreachincell which is a rework of map_foreachinarea but
//           which only checks the exact single x/y passed to it rather than an
//           area radius - may be more useful in some instances)
//
void map_foreachincell (int (*func) (struct block_list *, va_list), int m,
                        int x, int y, int type, ...)
{
    int  bx, by;
    struct block_list *bl = NULL;
    va_list ap = NULL;
    int  blockcount = bl_list_count, i, c;

    va_start (ap, type);

    by = y / BLOCK_SIZE;
    bx = x / BLOCK_SIZE;

    if (type == 0 || type != BL_MOB)
    {
        bl = map[m].block[bx + by * map[m].bxs];
        c = map[m].block_count[bx + by * map[m].bxs];
        for (i = 0; i < c && bl; i++, bl = bl->next)
        {
            if (type && bl && bl->type != type)
                continue;
            if (bl && bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
                bl_list[bl_list_count++] = bl;
        }
    }

    if (type == 0 || type == BL_MOB)
    {
        bl = map[m].block_mob[bx + by * map[m].bxs];
        c = map[m].block_mob_count[bx + by * map[m].bxs];
        for (i = 0; i < c && bl; i++, bl = bl->next)
        {
            if (bl && bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
                bl_list[bl_list_count++] = bl;
        }
    }

    if (bl_list_count >= BL_LIST_MAX)
    {
        if (battle_config.error_log)
            printf ("map_foreachincell: *WARNING* block count too many!\n");
    }

    map_freeblock_lock ();      // メモリからの解放を禁止する

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev)   // 有効かどうかチェック
            func (bl_list[i], ap);

    map_freeblock_unlock ();    // 解放を許可する

    va_end (ap);
    bl_list_count = blockcount;
}

/*==========================================
 * 床アイテムやエフェクト用の一時obj割り当て
 * object[]への保存とid_db登録まで
 *
 * bl->idもこの中で設定して問題無い?
 *------------------------------------------
 */
int map_addobject (struct block_list *bl)
{
    int  i;
    if (bl == NULL)
    {
        printf ("map_addobject nullpo?\n");
        return 0;
    }
    if (first_free_object_id < 2 || first_free_object_id >= MAX_FLOORITEM)
        first_free_object_id = 2;
    for (i = first_free_object_id; i < MAX_FLOORITEM; i++)
        if (object[i] == NULL)
            break;
    if (i >= MAX_FLOORITEM)
    {
        if (battle_config.error_log)
            printf ("no free object id\n");
        return 0;
    }
    first_free_object_id = i;
    if (last_object_id < i)
        last_object_id = i;
    object[i] = bl;
    numdb_insert (id_db, i, bl);
    return i;
}

/*==========================================
 * 一時objectの解放
 *	map_delobjectのfreeしないバージョン
 *------------------------------------------
 */
int map_delobjectnofree (int id, int type)
{
    if (object[id] == NULL)
        return 0;

    if (object[id]->type != type)
    {
        fprintf (stderr, "Incorrect type: expected %d, got %d\n", type,
                 object[id]->type);
        *((char *) 0) = 0;      // break for backtrace
    }

    map_delblock (object[id]);
    numdb_erase (id_db, id);
//  map_freeblock(object[id]);
    object[id] = NULL;

    if (first_free_object_id > id)
        first_free_object_id = id;

    while (last_object_id > 2 && object[last_object_id] == NULL)
        last_object_id--;

    return 0;
}

/*==========================================
 * 一時objectの解放
 * block_listからの削除、id_dbからの削除
 * object dataのfree、object[]へのNULL代入
 *
 * addとの対称性が無いのが気になる
 *------------------------------------------
 */
int map_delobject (int id, int type)
{
    struct block_list *obj = object[id];

    if (obj == NULL)
        return 0;

    map_delobjectnofree (id, type);
    if (obj->type == BL_PC)     // [Fate] Not sure where else to put this... I'm not sure where delobject for PCs is called from
        pc_cleanup ((struct map_session_data *) obj);

    map_freeblock (obj);

    return 0;
}

/*==========================================
 * 全一時obj相手にfuncを呼ぶ
 *
 *------------------------------------------
 */
void map_foreachobject (int (*func) (struct block_list *, va_list), int type,
                        ...)
{
    int  i;
    int  blockcount = bl_list_count;
    va_list ap = NULL;

    va_start (ap, type);

    for (i = 2; i <= last_object_id; i++)
    {
        if (object[i])
        {
            if (type && object[i]->type != type)
                continue;
            if (bl_list_count >= BL_LIST_MAX)
            {
                if (battle_config.error_log)
                    printf ("map_foreachobject: too many block !\n");
            }
            else
                bl_list[bl_list_count++] = object[i];
        }
    }

    map_freeblock_lock ();

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev || bl_list[i]->next)
            func (bl_list[i], ap);

    map_freeblock_unlock ();

    va_end (ap);
    bl_list_count = blockcount;
}

/*==========================================
 * 床アイテムを消す
 *
 * data==0の時はtimerで消えた時
 * data!=0の時は拾う等で消えた時として動作
 *
 * 後者は、map_clearflooritem(id)へ
 * map.h内で#defineしてある
 *------------------------------------------
 */
int map_clearflooritem_timer (int tid, unsigned int tick, int id, int data)
{
    struct flooritem_data *fitem = NULL;

    fitem = (struct flooritem_data *) object[id];
    if (fitem == NULL || fitem->bl.type != BL_ITEM
        || (!data && fitem->cleartimer != tid))
    {
        if (battle_config.error_log)
            printf ("map_clearflooritem_timer : error\n");
        return 1;
    }
    if (data)
        delete_timer (fitem->cleartimer, map_clearflooritem_timer);
    clif_clearflooritem (fitem, 0);
    map_delobject (fitem->bl.id, BL_ITEM);

    return 0;
}

/*==========================================
 * (m,x,y)の周囲rangeマス内の空き(=侵入可能)cellの
 * 内から適当なマス目の座標をx+(y<<16)で返す
 *
 * 現状range=1でアイテムドロップ用途のみ
 *------------------------------------------
 */
int map_searchrandfreecell (int m, int x, int y, int range)
{
    int  free_cell, i, j, c;

    for (free_cell = 0, i = -range; i <= range; i++)
    {
        if (i + y < 0 || i + y >= map[m].ys)
            continue;
        for (j = -range; j <= range; j++)
        {
            if (j + x < 0 || j + x >= map[m].xs)
                continue;
            if ((c = read_gat (m, j + x, i + y)) == 1 || c == 5)
                continue;
            free_cell++;
        }
    }
    if (free_cell == 0)
        return -1;
    free_cell = MRAND (free_cell);
    for (i = -range; i <= range; i++)
    {
        if (i + y < 0 || i + y >= map[m].ys)
            continue;
        for (j = -range; j <= range; j++)
        {
            if (j + x < 0 || j + x >= map[m].xs)
                continue;
            if ((c = read_gat (m, j + x, i + y)) == 1 || c == 5)
                continue;
            if (free_cell == 0)
            {
                x += j;
                y += i;
                i = range + 1;
                break;
            }
            free_cell--;
        }
    }

    return x + (y << 16);
}

/*==========================================
 * (m,x,y)を中心に3x3以内に床アイテム設置
 *
 * item_dataはamount以外をcopyする
 *------------------------------------------
 */
int map_addflooritem_any (struct item *item_data, int amount, int m, int x,
                          int y, struct map_session_data **owners,
                          int *owner_protection, int lifetime, int dispersal)
{
    int  xy, r;
    unsigned int tick;
    struct flooritem_data *fitem = NULL;

    nullpo_retr (0, item_data);

    if ((xy = map_searchrandfreecell (m, x, y, dispersal)) < 0)
        return 0;
    r = mt_random ();

    fitem = (struct flooritem_data *) aCalloc (1, sizeof (*fitem));
    fitem->bl.type = BL_ITEM;
    fitem->bl.prev = fitem->bl.next = NULL;
    fitem->bl.m = m;
    fitem->bl.x = xy & 0xffff;
    fitem->bl.y = (xy >> 16) & 0xffff;
    fitem->first_get_id = 0;
    fitem->first_get_tick = 0;
    fitem->second_get_id = 0;
    fitem->second_get_tick = 0;
    fitem->third_get_id = 0;
    fitem->third_get_tick = 0;

    fitem->bl.id = map_addobject (&fitem->bl);
    if (fitem->bl.id == 0)
    {
        free (fitem);
        return 0;
    }

    tick = gettick ();

    if (owners[0])
        fitem->first_get_id = owners[0]->bl.id;
    fitem->first_get_tick = tick + owner_protection[0];

    if (owners[1])
        fitem->second_get_id = owners[1]->bl.id;
    fitem->second_get_tick = tick + owner_protection[1];

    if (owners[2])
        fitem->third_get_id = owners[2]->bl.id;
    fitem->third_get_tick = tick + owner_protection[2];

    memcpy (&fitem->item_data, item_data, sizeof (*item_data));
    fitem->item_data.amount = amount;
    fitem->subx = (r & 3) * 3 + 3;
    fitem->suby = ((r >> 2) & 3) * 3 + 3;
    fitem->cleartimer =
        add_timer (gettick () + lifetime, map_clearflooritem_timer,
                   fitem->bl.id, 0);

    map_addblock (&fitem->bl);
    clif_dropflooritem (fitem);

    return fitem->bl.id;
}

int map_addflooritem (struct item *item_data, int amount, int m, int x, int y,
                      struct map_session_data *first_sd,
                      struct map_session_data *second_sd,
                      struct map_session_data *third_sd, int type)
{
    struct map_session_data *owners[3] = { first_sd, second_sd, third_sd };
    int  owner_protection[3];

    if (type)
    {
        owner_protection[0] = battle_config.mvp_item_first_get_time;
        owner_protection[1] =
            owner_protection[0] + battle_config.mvp_item_second_get_time;
        owner_protection[2] =
            owner_protection[1] + battle_config.mvp_item_third_get_time;
    }
    else
    {
        owner_protection[0] = battle_config.item_first_get_time;
        owner_protection[1] =
            owner_protection[0] + battle_config.item_second_get_time;
        owner_protection[2] =
            owner_protection[1] + battle_config.item_third_get_time;
    }

    return map_addflooritem_any (item_data, amount, m, x, y,
                                 owners, owner_protection,
                                 battle_config.flooritem_lifetime, 1);
}

/* 	int xy,r; */
/* 	unsigned int tick; */
/* 	struct flooritem_data *fitem=NULL; */

/* 	nullpo_retr(0, item_data); */

/* 	if((xy=map_searchrandfreecell(m,x,y,1))<0) */
/* 		return 0; */
/* 	r=rand(); */

/* 	fitem = (struct flooritem_data *)aCalloc(1,sizeof(*fitem)); */
/* 	fitem->bl.type=BL_ITEM; */
/* 	fitem->bl.prev = fitem->bl.next = NULL; */
/* 	fitem->bl.m=m; */
/* 	fitem->bl.x=xy&0xffff; */
/* 	fitem->bl.y=(xy>>16)&0xffff; */
/* 	fitem->first_get_id = 0; */
/* 	fitem->first_get_tick = 0; */
/* 	fitem->second_get_id = 0; */
/* 	fitem->second_get_tick = 0; */
/* 	fitem->third_get_id = 0; */
/* 	fitem->third_get_tick = 0; */

/* 	fitem->bl.id = map_addobject(&fitem->bl); */
/* 	if(fitem->bl.id==0){ */
/* 		free(fitem); */
/* 		return 0; */
/* 	} */

/* 	tick = gettick(); */
/* 	if(first_sd) { */
/* 		fitem->first_get_id = first_sd->bl.id; */
/* 		if(type) */
/* 			fitem->first_get_tick = tick + battle_config.mvp_item_first_get_time; */
/* 		else */
/* 			fitem->first_get_tick = tick + battle_config.item_first_get_time; */
/* 	} */
/* 	if(second_sd) { */
/* 		fitem->second_get_id = second_sd->bl.id; */
/* 		if(type) */
/* 			fitem->second_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time; */
/* 		else */
/* 			fitem->second_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time; */
/* 	} */
/* 	if(third_sd) { */
/* 		fitem->third_get_id = third_sd->bl.id; */
/* 		if(type) */
/* 			fitem->third_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time + battle_config.mvp_item_third_get_time; */
/* 		else */
/* 			fitem->third_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time + battle_config.item_third_get_time; */
/* 	} */

/* 	memcpy(&fitem->item_data,item_data,sizeof(*item_data)); */
/* 	fitem->item_data.amount=amount; */
/* 	fitem->subx=(r&3)*3+3; */
/* 	fitem->suby=((r>>2)&3)*3+3; */
/* 	fitem->cleartimer=add_timer(gettick()+battle_config.flooritem_lifetime,map_clearflooritem_timer,fitem->bl.id,0); */

/* 	map_addblock(&fitem->bl); */
/* 	clif_dropflooritem(fitem); */

/* 	return fitem->bl.id; */
/* } */

/*==========================================
 * charid_dbへ追加(返信待ちがあれば返信)
 *------------------------------------------
 */
void map_addchariddb (int charid, char *name)
{
    struct charid2nick *p = NULL;
    int  req = 0;

    p = numdb_search (charid_db, charid);
    if (p == NULL)
    {                           // データベースにない
        p = (struct charid2nick *) aCalloc (1, sizeof (struct charid2nick));
        p->req_id = 0;
    }
    else
        numdb_erase (charid_db, charid);

    req = p->req_id;
    memcpy (p->nick, name, 24);
    p->req_id = 0;
    numdb_insert (charid_db, charid, p);
    if (req)
    {                           // 返信待ちがあれば返信
        struct map_session_data *sd = map_id2sd (req);
        if (sd != NULL)
            clif_solved_charname (sd, charid);
    }
}

/*==========================================
 * charid_dbへ追加（返信要求のみ）
 *------------------------------------------
 */
int map_reqchariddb (struct map_session_data *sd, int charid)
{
    struct charid2nick *p = NULL;

    nullpo_retr (0, sd);

    p = numdb_search (charid_db, charid);
    if (p != NULL)              // データベースにすでにある
        return 0;
    p = (struct charid2nick *) aCalloc (1, sizeof (struct charid2nick));
    p->req_id = sd->bl.id;
    numdb_insert (charid_db, charid, p);
    return 0;
}

/*==========================================
 * id_dbへblを追加
 *------------------------------------------
 */
void map_addiddb (struct block_list *bl)
{
    nullpo_retv (bl);

    numdb_insert (id_db, bl->id, bl);
}

/*==========================================
 * id_dbからblを削除
 *------------------------------------------
 */
void map_deliddb (struct block_list *bl)
{
    nullpo_retv (bl);

    numdb_erase (id_db, bl->id);
}

/*==========================================
 * nick_dbへsdを追加
 *------------------------------------------
 */
void map_addnickdb (struct map_session_data *sd)
{
    nullpo_retv (sd);

    strdb_insert (nick_db, sd->status.name, sd);
}

/*==========================================
 * PCのquit処理 map.c内分
 *
 * quit処理の主体が違うような気もしてきた
 *------------------------------------------
 */
int map_quit (struct map_session_data *sd)
{
    nullpo_retr (0, sd);

    if (sd->chatID)             // チャットから出る
        chat_leavechat (sd);

    if (sd->trade_partner)      // 取引を中断する
        trade_tradecancel (sd);

    if (sd->party_invite > 0)   // パーティ勧誘を拒否する
        party_reply_invite (sd, sd->party_invite_account, 0);

    if (sd->guild_invite > 0)   // ギルド勧誘を拒否する
        guild_reply_invite (sd, sd->guild_invite, 0);
    if (sd->guild_alliance > 0) // ギルド同盟勧誘を拒否する
        guild_reply_reqalliance (sd, sd->guild_alliance_account, 0);

    party_send_logout (sd);     // パーティのログアウトメッセージ送信

    guild_send_memberinfoshort (sd, 0); // ギルドのログアウトメッセージ送信

    pc_cleareventtimer (sd);    // イベントタイマを破棄する

    skill_castcancel (&sd->bl, 0);  // 詠唱を中断する
    skill_stop_dancing (&sd->bl, 1);    // ダンス/演奏中断

    if (sd->sc_data && sd->sc_data[SC_BERSERK].timer != -1) //バーサーク中の終了はHPを100に
        sd->status.hp = 100;

    skill_status_change_clear (&sd->bl, 1); // ステータス異常を解除する
    skill_clear_unitgroup (&sd->bl);    // スキルユニットグループの削除
    skill_cleartimerskill (&sd->bl);
    pc_stop_walking (sd, 0);
    pc_stopattack (sd);
    pc_delinvincibletimer (sd);
    pc_delspiritball (sd, sd->spiritball, 1);
    skill_gangsterparadise (sd, 0);

    pc_calcstatus (sd, 4);

    clif_clearchar_area (&sd->bl, 2);

    if (pc_isdead (sd))
        pc_setrestartvalue (sd, 2);
    pc_makesavestatus (sd);
    //クローンスキルで覚えたスキルは消す

    //The storage closing routines will save the char if needed. [Skotlex]
    if (!sd->state.storage_flag)
        chrif_save (sd);
    else if (sd->state.storage_flag == 1)
        storage_storage_quit (sd);
    else if (sd->state.storage_flag == 2)
        storage_guild_storage_quit (sd, 1);

    if (sd->npc_stackbuf && sd->npc_stackbuf != NULL)
        free (sd->npc_stackbuf);

    map_delblock (&sd->bl);

    numdb_erase (id_db, sd->bl.id);
    strdb_erase (nick_db, sd->status.name);
    numdb_erase (charid_db, sd->status.char_id);

    return 0;
}

/*==========================================
 * id番号のPCを探す。居なければNULL
 *------------------------------------------
 */
struct map_session_data *map_id2sd (int id)
{
// remove search from db, because:
// 1 - all players, npc, items and mob are in this db (to search, it's not speed, and search in session is more sure)
// 2 - DB seems not always correct. Sometimes, when a player disconnects, its id (account value) is not removed and structure
//     point to a memory area that is not more a session_data and value are incorrect (or out of available memory) -> crash
// replaced by searching in all session.
// by searching in session, we are sure that fd, session, and account exist.
/*
	struct block_list *bl;

	bl=numdb_search(id_db,id);
	if(bl && bl->type==BL_PC)
		return (struct map_session_data*)bl;
	return NULL;
*/
    int  i;
    struct map_session_data *sd = NULL;

    for (i = 0; i < fd_max; i++)
        if (session[i] && (sd = session[i]->session_data) && sd->bl.id == id)
            return sd;

    return NULL;
}

/*==========================================
 * char_id番号の名前を探す
 *------------------------------------------
 */
char *map_charid2nick (int id)
{
    struct charid2nick *p = numdb_search (charid_db, id);

    if (p == NULL)
        return NULL;
    if (p->req_id != 0)
        return NULL;
    return p->nick;
}

/*========================================*/
/* [Fate] Operations to iterate over active map sessions */

static struct map_session_data *map_get_session (int i)
{
    struct map_session_data *d;

    if (i >= 0 && i < fd_max
        && session[i] && (d = session[i]->session_data) && d->state.auth)
        return d;

    return NULL;
}

static struct map_session_data *map_get_session_forward (int start)
{
    int  i;
    for (i = start; i < fd_max; i++)
    {
        struct map_session_data *d = map_get_session (i);
        if (d)
            return d;
    }

    return NULL;
}

static struct map_session_data *map_get_session_backward (int start)
{
    int  i;
    for (i = start; i >= 0; i--)
    {
        struct map_session_data *d = map_get_session (i);
        if (d)
            return d;
    }

    return NULL;
}

struct map_session_data *map_get_first_session ()
{
    return map_get_session_forward (0);
}

struct map_session_data *map_get_next_session (struct map_session_data *d)
{
    return map_get_session_forward (d->fd + 1);
}

struct map_session_data *map_get_last_session ()
{
    return map_get_session_backward (fd_max);
}

struct map_session_data *map_get_prev_session (struct map_session_data *d)
{
    return map_get_session_backward (d->fd - 1);
}

/*==========================================
 * Search session data from a nick name
 * (without sensitive case if necessary)
 * return map_session_data pointer or NULL
 *------------------------------------------
 */
struct map_session_data *map_nick2sd (char *nick)
{
    int  i, quantity = 0, nicklen;
    struct map_session_data *sd = NULL;
    struct map_session_data *pl_sd = NULL;

    if (nick == NULL)
        return NULL;

    nicklen = strlen (nick);

    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (pl_sd = session[i]->session_data)
            && pl_sd->state.auth)
        {
            // Without case sensitive check (increase the number of similar character names found)
            if (strnicmp (pl_sd->status.name, nick, nicklen) == 0)
            {
                // Strict comparison (if found, we finish the function immediatly with correct value)
                if (strcmp (pl_sd->status.name, nick) == 0)
                    return pl_sd;
                quantity++;
                sd = pl_sd;
            }
        }
    }
    // Here, the exact character name is not found
    // We return the found index of a similar account ONLY if there is 1 similar character
    if (quantity == 1)
        return sd;

    // Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
    return NULL;
}

/*==========================================
 * id番号の物を探す
 * 一時objectの場合は配列を引くのみ
 *------------------------------------------
 */
struct block_list *map_id2bl (int id)
{
    struct block_list *bl = NULL;
    if (id < sizeof (object) / sizeof (object[0]))
        bl = object[id];
    else
        bl = numdb_search (id_db, id);

    return bl;
}

/*==========================================
 * id_db内の全てにfuncを実行
 *------------------------------------------
 */
int map_foreachiddb (int (*func) (void *, void *, va_list), ...)
{
    va_list ap = NULL;

    va_start (ap, func);
    numdb_foreach (id_db, func, ap);
    va_end (ap);
    return 0;
}

/*==========================================
 * map.npcへ追加 (warp等の領域持ちのみ)
 *------------------------------------------
 */
int map_addnpc (int m, struct npc_data *nd)
{
    int  i;
    if (m < 0 || m >= map_num)
        return -1;
    for (i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++)
        if (map[m].npc[i] == NULL)
            break;
    if (i == MAX_NPC_PER_MAP)
    {
        if (battle_config.error_log)
            printf ("too many NPCs in one map %s\n", map[m].name);
        return -1;
    }
    if (i == map[m].npc_num)
    {
        map[m].npc_num++;
    }

    nullpo_retr (0, nd);

    map[m].npc[i] = nd;
    nd->n = i;
    numdb_insert (id_db, nd->bl.id, nd);

    return i;
}

void map_removenpc (void)
{
    int  i, m, n = 0;

    for (m = 0; m < map_num; m++)
    {
        for (i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++)
        {
            if (map[m].npc[i] != NULL)
            {
                clif_clearchar_area (&map[m].npc[i]->bl, 2);
                map_delblock (&map[m].npc[i]->bl);
                numdb_erase (id_db, map[m].npc[i]->bl.id);
                if (map[m].npc[i]->bl.subtype == SCRIPT)
                {
//                    free(map[m].npc[i]->u.scr.script);
//                    free(map[m].npc[i]->u.scr.label_list);
                }
                free (map[m].npc[i]);
                map[m].npc[i] = NULL;
                n++;
            }
        }
    }
    printf ("%d NPCs removed.\n", n);
}

/*==========================================
 * map名からmap番号へ変換
 *------------------------------------------
 */
int map_mapname2mapid (char *name)
{
    struct map_data *md = NULL;

    md = strdb_search (map_db, name);
    if (md == NULL || md->gat == NULL)
        return -1;
    return md->m;
}

/*==========================================
 * 他鯖map名からip,port変換
 *------------------------------------------
 */
int map_mapname2ipport (char *name, int *ip, int *port)
{
    struct map_data_other_server *mdos = NULL;

    mdos = strdb_search (map_db, name);
    if (mdos == NULL || mdos->gat)
        return -1;
    *ip = mdos->ip;
    *port = mdos->port;
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_check_dir (int s_dir, int t_dir)
{
    if (s_dir == t_dir)
        return 0;
    switch (s_dir)
    {
        case 0:
            if (t_dir == 7 || t_dir == 1 || t_dir == 0)
                return 0;
            break;
        case 1:
            if (t_dir == 0 || t_dir == 2 || t_dir == 1)
                return 0;
            break;
        case 2:
            if (t_dir == 1 || t_dir == 3 || t_dir == 2)
                return 0;
            break;
        case 3:
            if (t_dir == 2 || t_dir == 4 || t_dir == 3)
                return 0;
            break;
        case 4:
            if (t_dir == 3 || t_dir == 5 || t_dir == 4)
                return 0;
            break;
        case 5:
            if (t_dir == 4 || t_dir == 6 || t_dir == 5)
                return 0;
            break;
        case 6:
            if (t_dir == 5 || t_dir == 7 || t_dir == 6)
                return 0;
            break;
        case 7:
            if (t_dir == 6 || t_dir == 0 || t_dir == 7)
                return 0;
            break;
    }
    return 1;
}

/*==========================================
 * 彼我の方向を計算
 *------------------------------------------
 */
int map_calc_dir (struct block_list *src, int x, int y)
{
    int  dir = 0;
    int  dx, dy;

    nullpo_retr (0, src);

    dx = x - src->x;
    dy = y - src->y;
    if (dx == 0 && dy == 0)
    {                           // 彼我の場所一致
        dir = 0;                // 上
    }
    else if (dx >= 0 && dy >= 0)
    {                           // 方向的に右上
        dir = 7;                // 右上
        if (dx * 3 - 1 < dy)
            dir = 0;            // 上
        if (dx > dy * 3)
            dir = 6;            // 右
    }
    else if (dx >= 0 && dy <= 0)
    {                           // 方向的に右下
        dir = 5;                // 右下
        if (dx * 3 - 1 < -dy)
            dir = 4;            // 下
        if (dx > -dy * 3)
            dir = 6;            // 右
    }
    else if (dx <= 0 && dy <= 0)
    {                           // 方向的に左下
        dir = 3;                // 左下
        if (dx * 3 + 1 > dy)
            dir = 4;            // 下
        if (dx < dy * 3)
            dir = 2;            // 左
    }
    else
    {                           // 方向的に左上
        dir = 1;                // 左上
        if (-dx * 3 - 1 < dy)
            dir = 0;            // 上
        if (-dx > dy * 3)
            dir = 2;            // 左
    }
    return dir;
}

// gat系
/*==========================================
 * (m,x,y)の状態を調べる
 *------------------------------------------
 */
int map_getcell (int m, int x, int y)
{
    if (x < 0 || x >= map[m].xs - 1 || y < 0 || y >= map[m].ys - 1)
        return 1;
    return map[m].gat[x + y * map[m].xs];
}

/*==========================================
 * (m,x,y)の状態をtにする
 *------------------------------------------
 */
int map_setcell (int m, int x, int y, int t)
{
    if (x < 0 || x >= map[m].xs || y < 0 || y >= map[m].ys)
        return t;
    return map[m].gat[x + y * map[m].xs] = t;
}

/*==========================================
 * 他鯖管理のマップをdbに追加
 *------------------------------------------
 */
int map_setipport (char *name, unsigned long ip, int port)
{
    struct map_data *md = NULL;
    struct map_data_other_server *mdos = NULL;

    md = strdb_search (map_db, name);
    if (md == NULL)
    {                           // not exist -> add new data
        mdos =
            (struct map_data_other_server *) aCalloc (1,
                                                      sizeof (struct
                                                              map_data_other_server));
        memcpy (mdos->name, name, 24);
        mdos->gat = NULL;
        mdos->ip = ip;
        mdos->port = port;
        strdb_insert (map_db, mdos->name, mdos);
    }
    else
    {
        if (md->gat)
        {                       // local -> check data
            if (ip != clif_getip () || port != clif_getport ())
            {
                printf ("from char server : %s -> %08lx:%d\n", name, ip,
                        port);
                return 1;
            }
        }
        else
        {                       // update
            mdos = (struct map_data_other_server *) md;
            mdos->ip = ip;
            mdos->port = port;
        }
    }
    return 0;
}

// 初期化周り
/*==========================================
 * 水場高さ設定
 *------------------------------------------
 */
static struct
{
    char mapname[24];
    int  waterheight;
}   *waterlist = NULL;

#define NO_WATER 1000000

static int map_waterheight (char *mapname)
{
    if (waterlist)
    {
        int  i;
        for (i = 0; waterlist[i].mapname[0] && i < MAX_MAP_PER_SERVER; i++)
            if (strcmp (waterlist[i].mapname, mapname) == 0)
                return waterlist[i].waterheight;
    }
    return NO_WATER;
}

static void map_readwater (char *watertxt)
{
    char line[1024], w1[1024];
    FILE *fp = NULL;
    int  n = 0;

    fp = fopen_ (watertxt, "r");
    if (fp == NULL)
    {
        printf ("file not found: %s\n", watertxt);
        return;
    }
    if (waterlist == NULL)
        waterlist = aCalloc (MAX_MAP_PER_SERVER, sizeof (*waterlist));
    while (fgets (line, 1020, fp) && n < MAX_MAP_PER_SERVER)
    {
        int  wh, count;
        if (line[0] == '/' && line[1] == '/')
            continue;
        if ((count = sscanf (line, "%s%d", w1, &wh)) < 1)
        {
            continue;
        }
        strcpy (waterlist[n].mapname, w1);
        if (count >= 2)
            waterlist[n].waterheight = wh;
        else
            waterlist[n].waterheight = 3;
        n++;
    }
    fclose_ (fp);
}

/*==========================================
 * マップ1枚読み込み
 *------------------------------------------
 */
static int map_readmap (int m, char *fn, char *alias)
{
    unsigned char *gat = "";
    int  s;
    int  x, y, xs, ys;
    struct gat_1cell
    {
        char type;
    }   *p;
    int  wh;
    size_t size;

    // read & convert fn
    gat = grfio_read (fn);
    if (gat == NULL)
        return -1;

    printf ("\rLoading Maps [%d/%d]: %-50s  ", m, map_num, fn);
    fflush (stdout);

    map[m].m = m;
    xs = map[m].xs = *(short *) (gat);
    ys = map[m].ys = *(short *) (gat + 2);
    printf ("\n%i %i\n", xs, ys);
    map[m].gat = calloc (s = map[m].xs * map[m].ys, 1);
    if (map[m].gat == NULL)
    {
        printf ("out of memory : map_readmap gat\n");
        exit (1);
    }

    map[m].npc_num = 0;
    map[m].users = 0;
    memset (&map[m].flag, 0, sizeof (map[m].flag));
    if (battle_config.pk_mode)
        map[m].flag.pvp = 1;    // make all maps pvp for pk_mode [Valaris]
    wh = map_waterheight (map[m].name);
    for (y = 0; y < ys; y++)
    {
        p = (struct gat_1cell *) (gat + y * xs + 4);
        for (x = 0; x < xs; x++)
        {
            /*if(wh!=NO_WATER && p->type==0){
             * // 水場判定
             * map[m].gat[x+y*xs]=(p->high[0]>wh || p->high[1]>wh || p->high[2]>wh || p->high[3]>wh) ? 3 : 0;
             * } else { */
            map[m].gat[x + y * xs] = p->type;
            //}
            p++;
        }
    }
    free (gat);

    map[m].bxs = (xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
    map[m].bys = (ys + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size = map[m].bxs * map[m].bys * sizeof (struct block_list *);

    map[m].block = calloc (size, 1);
    if (map[m].block == NULL)
    {
        printf ("out of memory : map_readmap block\n");
        exit (1);
    }

    map[m].block_mob = calloc (size, 1);
    if (map[m].block_mob == NULL)
    {
        printf ("out of memory : map_readmap block_mob\n");
        exit (1);
    }

    size = map[m].bxs * map[m].bys * sizeof (int);

    map[m].block_count = calloc (size, 1);
    if (map[m].block_count == NULL)
    {
        printf ("out of memory : map_readmap block\n");
        exit (1);
    }
    memset (map[m].block_count, 0, size);

    map[m].block_mob_count = calloc (size, 1);
    if (map[m].block_mob_count == NULL)
    {
        printf ("out of memory : map_readmap block_mob\n");
        exit (1);
    }
    memset (map[m].block_mob_count, 0, size);

    strdb_insert (map_db, map[m].name, &map[m]);

//  printf("%s read done\n",fn);

    return 0;
}

/*==========================================
 * 全てのmapデータを読み込む
 *------------------------------------------
 */
int map_readallmap (void)
{
    int  i, maps_removed = 0;
    char fn[256] = "";

    // 先に全部のャbプの存在を確認
    for (i = 0; i < map_num; i++)
    {
        if (strstr (map[i].name, ".gat") == NULL)
            continue;
        sprintf (fn, "data\\%s", map[i].name);
        if (grfio_size (fn) == -1)
        {
            map_delmap (map[i].name);
            maps_removed++;
        }
    }
    for (i = 0; i < map_num; i++)
    {
        if (strstr (map[i].name, ".gat") != NULL)
        {
            char *p = strstr (map[i].name, ">");    // [MouseJstr]
            if (p != NULL)
            {
                char alias[64];
                *p = '\0';
                strcpy (alias, map[i].name);
                strcpy (map[i].name, p + 1);
                sprintf (fn, "data\\%s", map[i].name);
                if (map_readmap (i, fn, alias) == -1)
                {
                    map_delmap (map[i].name);
                    maps_removed++;
                }
            }
            else
            {
                sprintf (fn, "data\\%s", map[i].name);
                if (map_readmap (i, fn, NULL) == -1)
                {
                    map_delmap (map[i].name);
                    maps_removed++;
                }
            }
        }
    }

    free (waterlist);
    printf ("\rMaps Loaded: %d %60s\n", map_num, "");
    printf ("\rMaps Removed: %d \n", maps_removed);
    return 0;
}

/*==========================================
 * 読み込むmapを追加する
 *------------------------------------------
 */
int map_addmap (char *mapname)
{
    if (strcmpi (mapname, "clear") == 0)
    {
        map_num = 0;
        return 0;
    }

    if (map_num >= MAX_MAP_PER_SERVER - 1)
    {
        printf ("too many map\n");
        return 1;
    }
    memcpy (map[map_num].name, mapname, 24);
    map_num++;
    return 0;
}

/*==========================================
 * 読み込むmapを削除する
 *------------------------------------------
 */
int map_delmap (char *mapname)
{
    int  i;

    if (strcmpi (mapname, "all") == 0)
    {
        map_num = 0;
        return 0;
    }

    for (i = 0; i < map_num; i++)
    {
        if (strcmp (map[i].name, mapname) == 0)
        {
            printf ("Removing map [ %s ] from maplist\n", map[i].name);
            memmove (map + i, map + i + 1,
                     sizeof (map[0]) * (map_num - i - 1));
            map_num--;
        }
    }
    return 0;
}

extern char *gm_logfile_name;

#define LOGFILE_SECONDS_PER_CHUNK_SHIFT 10

FILE *map_logfile = NULL;
char *map_logfile_name = NULL;
static long map_logfile_index;

static void map_close_logfile ()
{
    if (map_logfile)
    {
        char *filenameop_buf = malloc (strlen (map_logfile_name) + 50);
        sprintf (filenameop_buf, "gzip -f %s.%ld", map_logfile_name,
                 map_logfile_index);

        fclose (map_logfile);

        if (!system (filenameop_buf))
            perror (filenameop_buf);

        free (filenameop_buf);
    }
}

static void map_start_logfile (long suffix)
{
    char *filename_buf = malloc (strlen (map_logfile_name) + 50);
    map_logfile_index = suffix >> LOGFILE_SECONDS_PER_CHUNK_SHIFT;

    sprintf (filename_buf, "%s.%ld", map_logfile_name, map_logfile_index);
    map_logfile = fopen (filename_buf, "w+");
    if (!map_logfile)
        perror (map_logfile_name);

    free (filename_buf);
}

static void map_set_logfile (char *filename)
{
    struct timeval tv;

    map_logfile_name = strdup (filename);
    gettimeofday (&tv, NULL);

    map_start_logfile (tv.tv_sec);
    atexit (map_close_logfile);
    MAP_LOG ("log-start v3");
}

void map_write_log (char *format, ...)
{
    struct timeval tv;
    va_list args;
    va_start (args, format);

    gettimeofday (&tv, NULL);

    if ((tv.tv_sec >> LOGFILE_SECONDS_PER_CHUNK_SHIFT) != map_logfile_index)
    {
        map_close_logfile ();
        map_start_logfile (tv.tv_sec);
    }

    fprintf (map_logfile, "%ld.%06ld ", (long) tv.tv_sec, (long) tv.tv_usec);
    vfprintf (map_logfile, format, args);
    fputc ('\n', map_logfile);
}

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int map_config_read (char *cfgName)
{
    char line[1024], w1[1024], w2[1024];
    FILE *fp;
    struct hostent *h = NULL;

    fp = fopen_ (cfgName, "r");
    if (fp == NULL)
    {
        printf ("Map configuration file not found at: %s\n", cfgName);
        exit (1);
    }
    while (fgets (line, sizeof (line) - 1, fp))
    {
        if (line[0] == '/' && line[1] == '/')
            continue;
        if (sscanf (line, "%[^:]: %[^\r\n]", w1, w2) == 2)
        {
            if (strcmpi (w1, "userid") == 0)
            {
                chrif_setuserid (w2);
            }
            else if (strcmpi (w1, "passwd") == 0)
            {
                chrif_setpasswd (w2);
            }
            else if (strcmpi (w1, "char_ip") == 0)
            {
                h = gethostbyname (w2);
                if (h != NULL)
                {
                    printf
                        ("Character server IP address : %s -> %d.%d.%d.%d\n",
                         w2, (unsigned char) h->h_addr[0],
                         (unsigned char) h->h_addr[1],
                         (unsigned char) h->h_addr[2],
                         (unsigned char) h->h_addr[3]);
                    sprintf (w2, "%d.%d.%d.%d", (unsigned char) h->h_addr[0],
                             (unsigned char) h->h_addr[1],
                             (unsigned char) h->h_addr[2],
                             (unsigned char) h->h_addr[3]);
                }
                chrif_setip (w2);
            }
            else if (strcmpi (w1, "char_port") == 0)
            {
                chrif_setport (atoi (w2));
            }
            else if (strcmpi (w1, "map_ip") == 0)
            {
                h = gethostbyname (w2);
                if (h != NULL)
                {
                    printf ("Map server IP address : %s -> %d.%d.%d.%d\n", w2,
                            (unsigned char) h->h_addr[0],
                            (unsigned char) h->h_addr[1],
                            (unsigned char) h->h_addr[2],
                            (unsigned char) h->h_addr[3]);
                    sprintf (w2, "%d.%d.%d.%d", (unsigned char) h->h_addr[0],
                             (unsigned char) h->h_addr[1],
                             (unsigned char) h->h_addr[2],
                             (unsigned char) h->h_addr[3]);
                }
                clif_setip (w2);
            }
            else if (strcmpi (w1, "map_port") == 0)
            {
                clif_setport (atoi (w2));
                map_port = (atoi (w2));
            }
            else if (strcmpi (w1, "water_height") == 0)
            {
                map_readwater (w2);
            }
            else if (strcmpi (w1, "map") == 0)
            {
                map_addmap (w2);
            }
            else if (strcmpi (w1, "delmap") == 0)
            {
                map_delmap (w2);
            }
            else if (strcmpi (w1, "npc") == 0)
            {
                npc_addsrcfile (w2);
            }
            else if (strcmpi (w1, "delnpc") == 0)
            {
                npc_delsrcfile (w2);
            }
            else if (strcmpi (w1, "data_grf") == 0)
            {
                grfio_setdatafile (w2);
            }
            else if (strcmpi (w1, "sdata_grf") == 0)
            {
                grfio_setsdatafile (w2);
            }
            else if (strcmpi (w1, "adata_grf") == 0)
            {
                grfio_setadatafile (w2);
            }
            else if (strcmpi (w1, "autosave_time") == 0)
            {
                autosave_interval = atoi (w2) * 1000;
                if (autosave_interval <= 0)
                    autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
            }
            else if (strcmpi (w1, "motd_txt") == 0)
            {
                strcpy (motd_txt, w2);
            }
            else if (strcmpi (w1, "help_txt") == 0)
            {
                strcpy (help_txt, w2);
            }
            else if (strcmpi (w1, "mapreg_txt") == 0)
            {
                strcpy (mapreg_txt, w2);
            }
            else if (strcmpi (w1, "gm_log") == 0)
            {
                gm_logfile_name = strdup (w2);
            }
            else if (strcmpi (w1, "log_file") == 0)
            {
                map_set_logfile (w2);
            }
            else if (strcmpi (w1, "import") == 0)
            {
                map_config_read (w2);
            }
        }
    }
    fclose_ (fp);

    return 0;
}

int id_db_final (void *k, void *d, va_list ap)
{
    return 0;
}

int map_db_final (void *k, void *d, va_list ap)
{
    return 0;
}

int nick_db_final (void *k, void *d, va_list ap)
{
    return 0;
}

int charid_db_final (void *k, void *d, va_list ap)
{
    return 0;
}

static int cleanup_sub (struct block_list *bl, va_list ap)
{
    nullpo_retr (0, bl);

    switch (bl->type)
    {
        case BL_PC:
            map_delblock (bl);  // There is something better...
            break;
        case BL_NPC:
            npc_delete ((struct npc_data *) bl);
            break;
        case BL_MOB:
            mob_delete ((struct mob_data *) bl);
            break;
        case BL_ITEM:
            map_clearflooritem (bl->id);
            break;
        case BL_SKILL:
            skill_delunit ((struct skill_unit *) bl);
            break;
        case BL_SPELL:
            spell_free_invocation ((struct invocation *) bl);
            break;
    }

    return 0;
}

/*==========================================
 * map鯖終了時処理
 *------------------------------------------
 */
void do_final (void)
{
    int  map_id, i;

    for (map_id = 0; map_id < map_num; map_id++)
    {
        if (map[map_id].m)
            map_foreachinarea (cleanup_sub, map_id, 0, 0, map[map_id].xs,
                               map[map_id].ys, 0, 0);
    }

    for (i = 0; i < fd_max; i++)
        delete_session (i);

    map_removenpc ();
    timer_final ();

    numdb_final (id_db, id_db_final);
    strdb_final (map_db, map_db_final);
    strdb_final (nick_db, nick_db_final);
    numdb_final (charid_db, charid_db_final);

    for (i = 0; i <= map_num; i++)
    {
        if (map[i].gat)
            free (map[i].gat);
        if (map[i].block)
            free (map[i].block);
        if (map[i].block_mob)
            free (map[i].block_mob);
        if (map[i].block_count)
            free (map[i].block_count);
        if (map[i].block_mob_count)
            free (map[i].block_mob_count);
    }
    do_final_script ();
    do_final_itemdb ();
    do_final_storage ();
    do_final_guild ();
}

void map_helpscreen ()
{
    exit (1);
}

int compare_item (struct item *a, struct item *b)
{
    return ((a->nameid == b->nameid) &&
            (a->identify == b->identify) &&
            (a->refine == b->refine) &&
            (a->attribute == b->attribute) &&
            (a->card[0] == b->card[0]) &&
            (a->card[1] == b->card[1]) &&
            (a->card[2] == b->card[2]) && (a->card[3] == b->card[3]));
}

/*======================================================
 * Map-Server Init and Command-line Arguments [Valaris]
 *------------------------------------------------------
 */
int do_init (int argc, char *argv[])
{
    int  i;

    unsigned char *MAP_CONF_NAME = "conf/map_athena.conf";
    unsigned char *BATTLE_CONF_FILENAME = "conf/battle_athena.conf";
    unsigned char *ATCOMMAND_CONF_FILENAME = "conf/atcommand_athena.conf";
    unsigned char *SCRIPT_CONF_NAME = "conf/script_athena.conf";
    unsigned char *MSG_CONF_NAME = "conf/msg_athena.conf";
    unsigned char *GRF_PATH_FILENAME = "conf/grf-files.txt";

    for (i = 1; i < argc; i++)
    {

        if (strcmp (argv[i], "--help") == 0 || strcmp (argv[i], "--h") == 0
            || strcmp (argv[i], "--?") == 0 || strcmp (argv[i], "/?") == 0)
            map_helpscreen ();
        else if (strcmp (argv[i], "--map_config") == 0)
            MAP_CONF_NAME = argv[i + 1];
        else if (strcmp (argv[i], "--battle_config") == 0)
            BATTLE_CONF_FILENAME = argv[i + 1];
        else if (strcmp (argv[i], "--atcommand_config") == 0)
            ATCOMMAND_CONF_FILENAME = argv[i + 1];
        else if (strcmp (argv[i], "--script_config") == 0)
            SCRIPT_CONF_NAME = argv[i + 1];
        else if (strcmp (argv[i], "--msg_config") == 0)
            MSG_CONF_NAME = argv[i + 1];
        else if (strcmp (argv[i], "--grf_path_file") == 0)
            GRF_PATH_FILENAME = argv[i + 1];
    }

    map_config_read (MAP_CONF_NAME);
    battle_config_read (BATTLE_CONF_FILENAME);
    atcommand_config_read (ATCOMMAND_CONF_FILENAME);
    script_config_read (SCRIPT_CONF_NAME);
    msg_config_read (MSG_CONF_NAME);

    atexit (do_final);

    id_db = numdb_init ();
    map_db = strdb_init (16);
    nick_db = strdb_init (24);
    charid_db = numdb_init ();

    grfio_init (GRF_PATH_FILENAME);

    map_readallmap ();

    add_timer_func_list (map_clearflooritem_timer,
                         "map_clearflooritem_timer");

    do_init_chrif ();
    do_init_clif ();
    do_init_itemdb ();
    do_init_mob ();             // npcの初期化時内でmob_spawnして、mob_dbを参照するのでinit_npcより先
    do_init_script ();
    do_init_npc ();
    do_init_pc ();
    do_init_party ();
    do_init_guild ();
    do_init_storage ();
    do_init_skill ();
    do_init_magic ();

    npc_event_do_oninit ();     // npcのOnInitイベント実行

    if (battle_config.pk_mode == 1)
        printf ("The server is running in \033[1;31mPK Mode\033[0m.\n");

    printf
        ("The map-server is \033[1;32mready\033[0m (Server is listening on the port %d).\n\n",
         map_port);

    return 0;
}

int map_scriptcont (struct map_session_data *sd, int id)
{
    struct block_list *bl = map_id2bl (id);

    if (!bl)
        return 0;

    switch (bl->type)
    {
        case BL_NPC:
            return npc_scriptcont (sd, id);
        case BL_SPELL:
            spell_execute_script ((struct invocation *) bl);
            break;
    }

    return 0;
}
