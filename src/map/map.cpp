#include "map.hpp"

#include <sys/time.h>
#include <sys/wait.h>

#include <netdb.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <fstream>

#include "../common/core.hpp"
#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/random2.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chat.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "grfio.hpp"
#include "itemdb.hpp"
#include "magic.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "script.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "trade.hpp"

#include "../poison.hpp"

DMap<int, struct block_list *> id_db;

static
DMap<std::string, struct map_data *> map_db;

static
DMap<std::string, struct map_session_data *> nick_db;

struct charid2nick
{
    char nick[24];
    int req_id;
};

static
Map<int, struct charid2nick> charid_db;

static
int users = 0;
static
struct block_list *object[MAX_FLOORITEM];
static
int first_free_object_id = 0, last_object_id = 0;

constexpr int block_free_max = 1048576;
static
void *block_free[block_free_max];
static
int block_free_count = 0, block_free_lock = 0;

constexpr int BL_LIST_MAX = 1048576;
static
struct block_list *bl_list[BL_LIST_MAX];
static
int bl_list_count = 0;

struct map_data map[MAX_MAP_PER_SERVER];
int map_num = 0;

static
int map_port = 0;

interval_t autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
int save_settings = 0xFFFF;

char motd_txt[256] = "conf/motd.txt";
char help_txt[256] = "conf/help.txt";

char wisp_server_name[24] = "Server";   // can be modified in char-server configuration file

static
int map_delmap(const char *mapname);

/*==========================================
 * 全map鯖総計での接続数設定
 * (char鯖から送られてくる)
 *------------------------------------------
 */
void map_setusers(int n)
{
    users = n;
}

/*==========================================
 * 全map鯖総計での接続数取得 (/wへの応答用)
 *------------------------------------------
 */
int map_getusers(void)
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
int map_freeblock(void *bl)
{
    if (block_free_lock == 0)
    {
        free(bl);
        bl = NULL;
    }
    else
    {
        if (block_free_count >= block_free_max)
        {
            if (battle_config.error_log)
                PRINTF("map_freeblock: *WARNING* too many free block! %d %d\n",
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
int map_freeblock_lock(void)
{
    return ++block_free_lock;
}

/*==========================================
 * blockのfreeのロックを解除する
 * このとき、ロックが完全になくなると
 * バッファにたまっていたblockを全部削除
 *------------------------------------------
 */
int map_freeblock_unlock(void)
{
    if ((--block_free_lock) == 0)
    {
        int i;
//      if(block_free_count>0) {
//          if(battle_config.error_log)
//              PRINTF("map_freeblock_unlock: free %d object\n",block_free_count);
//      }
        for (i = 0; i < block_free_count; i++)
        {
            free(block_free[i]);
            block_free[i] = NULL;
        }
        block_free_count = 0;
    }
    else if (block_free_lock < 0)
    {
        if (battle_config.error_log)
            PRINTF("map_freeblock_unlock: lock count < 0 !\n");
    }
    return block_free_lock;
}

/// This is a dummy entry that is shared by all the linked lists,
/// so that any entry can unlink itself without worrying about
/// whether it was the the head of the list.
static
struct block_list bl_head;

/*==========================================
 * map[]のblock_listに追加
 * mobは数が多いので別リスト
 *
 * 既にlink済みかの確認が無い。危険かも
 *------------------------------------------
 */
int map_addblock(struct block_list *bl)
{
    int m, x, y;

    nullpo_ret(bl);

    if (bl->prev != NULL)
    {
        if (battle_config.error_log)
            PRINTF("map_addblock error : bl->prev!=NULL\n");
        return 0;
    }

    m = bl->m;
    x = bl->x;
    y = bl->y;
    if (m < 0 || m >= map_num ||
        x < 0 || x >= map[m].xs || y < 0 || y >= map[m].ys)
        return 1;

    if (bl->type == BL::MOB)
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
        if (bl->type == BL::PC)
            map[m].users++;
    }

    return 0;
}

/*==========================================
 * map[]のblock_listから外す
 * prevがNULLの場合listに繋がってない
 *------------------------------------------
 */
int map_delblock(struct block_list *bl)
{
    int b;
    nullpo_ret(bl);

    // 既にblocklistから抜けている
    if (bl->prev == NULL)
    {
        if (bl->next != NULL)
        {
            // prevがNULLでnextがNULLでないのは有ってはならない
            if (battle_config.error_log)
                PRINTF("map_delblock error : bl->next!=NULL\n");
        }
        return 0;
    }

    b = bl->x / BLOCK_SIZE + (bl->y / BLOCK_SIZE) * map[bl->m].bxs;

    if (bl->type == BL::PC)
        map[bl->m].users--;

    if (bl->next)
        bl->next->prev = bl->prev;
    if (bl->prev == &bl_head)
    {
        // リストの頭なので、map[]のblock_listを更新する
        if (bl->type == BL::MOB)
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
 * セル上のPCとMOBの数を数える (グランドクロス用)
 *------------------------------------------
 */
int map_count_oncell(int m, int x, int y)
{
    int bx, by;
    struct block_list *bl = NULL;
    int i, c;
    int count = 0;

    if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
        return 1;
    bx = x / BLOCK_SIZE;
    by = y / BLOCK_SIZE;

    bl = map[m].block[bx + by * map[m].bxs];
    c = map[m].block_count[bx + by * map[m].bxs];
    for (i = 0; i < c && bl; i++, bl = bl->next)
    {
        if (bl->x == x && bl->y == y && bl->type == BL::PC)
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
void map_foreachinarea(std::function<void(struct block_list *)> func,
        int m,
        int x0, int y0, int x1, int y1,
        BL type)
{
    int bx, by;
    struct block_list *bl = NULL;
    int blockcount = bl_list_count, i, c;

    if (m < 0)
        return;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= map[m].xs)
        x1 = map[m].xs - 1;
    if (y1 >= map[m].ys)
        y1 = map[m].ys - 1;
    if (type == BL::NUL || type != BL::MOB)
        for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                bl = map[m].block[bx + by * map[m].bxs];
                c = map[m].block_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type != BL::NUL && bl->type != type)
                        continue;
                    if (bl && bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                        && bl->y <= y1 && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
            }
        }
    if (type == BL::NUL || type == BL::MOB)
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
            PRINTF("map_foreachinarea: *WARNING* block count too many!\n");
    }

    map_freeblock_lock();      // メモリからの解放を禁止する

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev)   // 有効かどうかチェック
            func(bl_list[i]);

    map_freeblock_unlock();    // 解放を許可する

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
void map_foreachinmovearea(std::function<void(struct block_list *)> func,
        int m,
        int x0, int y0, int x1, int y1,
        int dx, int dy,
        BL type)
{
    int bx, by;
    struct block_list *bl = NULL;
    int blockcount = bl_list_count, i, c;

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
                    if (bl && type != BL::NUL && bl->type != type)
                        continue;
                    if (bl && bl->x >= x0 && bl->x <= x1 && bl->y >= y0
                        && bl->y <= y1 && bl_list_count < BL_LIST_MAX)
                        bl_list[bl_list_count++] = bl;
                }
                bl = map[m].block_mob[bx + by * map[m].bxs];
                c = map[m].block_mob_count[bx + by * map[m].bxs];
                for (i = 0; i < c && bl; i++, bl = bl->next)
                {
                    if (bl && type != BL::NUL && bl->type != type)
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
                    if (bl && type != BL::NUL && bl->type != type)
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
                    if (bl && type != BL::NUL && bl->type != type)
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
            PRINTF("map_foreachinarea: *WARNING* block count too many!\n");
    }

    map_freeblock_lock();      // メモリからの解放を禁止する

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev)   // 有効かどうかチェック
            func(bl_list[i]);

    map_freeblock_unlock();    // 解放を許可する

    bl_list_count = blockcount;
}

// -- moonsoul  (added map_foreachincell which is a rework of map_foreachinarea but
//           which only checks the exact single x/y passed to it rather than an
//           area radius - may be more useful in some instances)
//
void map_foreachincell(std::function<void(struct block_list *)> func,
        int m,
        int x, int y,
        BL type)
{
    int bx, by;
    struct block_list *bl = NULL;
    int blockcount = bl_list_count, i, c;

    by = y / BLOCK_SIZE;
    bx = x / BLOCK_SIZE;

    if (type == BL::NUL || type != BL::MOB)
    {
        bl = map[m].block[bx + by * map[m].bxs];
        c = map[m].block_count[bx + by * map[m].bxs];
        for (i = 0; i < c && bl; i++, bl = bl->next)
        {
            if (type != BL::NUL && bl && bl->type != type)
                continue;
            if (bl && bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
                bl_list[bl_list_count++] = bl;
        }
    }

    if (type == BL::NUL || type == BL::MOB)
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
            PRINTF("map_foreachincell: *WARNING* block count too many!\n");
    }

    map_freeblock_lock();      // メモリからの解放を禁止する

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev)   // 有効かどうかチェック
            func(bl_list[i]);

    map_freeblock_unlock();    // 解放を許可する

    bl_list_count = blockcount;
}

/*==========================================
 * 床アイテムやエフェクト用の一時obj割り当て
 * object[]への保存とid_db登録まで
 *
 * bl->idもこの中で設定して問題無い?
 *------------------------------------------
 */
int map_addobject(struct block_list *bl)
{
    int i;
    if (bl == NULL)
    {
        PRINTF("map_addobject nullpo?\n");
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
            PRINTF("no free object id\n");
        return 0;
    }
    first_free_object_id = i;
    if (last_object_id < i)
        last_object_id = i;
    object[i] = bl;
    id_db.put(i, bl);
    return i;
}

/*==========================================
 * 一時objectの解放
 *      map_delobjectのfreeしないバージョン
 *------------------------------------------
 */
int map_delobjectnofree(int id, BL type)
{
    if (object[id] == NULL)
        return 0;

    if (object[id]->type != type)
    {
        FPRINTF(stderr, "Incorrect type: expected %d, got %d\n",
                type,
                object[id]->type);
        abort();
    }

    map_delblock(object[id]);
    id_db.put(id, nullptr);
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
int map_delobject(int id, BL type)
{
    struct block_list *obj = object[id];

    if (obj == NULL)
        return 0;

    map_delobjectnofree(id, type);
    if (obj->type == BL::PC)     // [Fate] Not sure where else to put this... I'm not sure where delobject for PCs is called from
        pc_cleanup((struct map_session_data *) obj);

    map_freeblock(obj);

    return 0;
}

/*==========================================
 * 全一時obj相手にfuncを呼ぶ
 *
 *------------------------------------------
 */
void map_foreachobject(std::function<void(struct block_list *)> func,
        BL type)
{
    int i;
    int blockcount = bl_list_count;

    for (i = 2; i <= last_object_id; i++)
    {
        if (object[i])
        {
            if (type != BL::NUL && object[i]->type != type)
                continue;
            if (bl_list_count >= BL_LIST_MAX)
            {
                if (battle_config.error_log)
                    PRINTF("map_foreachobject: too many block !\n");
            }
            else
                bl_list[bl_list_count++] = object[i];
        }
    }

    map_freeblock_lock();

    for (i = blockcount; i < bl_list_count; i++)
        if (bl_list[i]->prev || bl_list[i]->next)
            func(bl_list[i]);

    map_freeblock_unlock();

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
void map_clearflooritem_timer(TimerData *tid, tick_t, int id)
{
    struct flooritem_data *fitem = NULL;

    fitem = (struct flooritem_data *) object[id];
    if (fitem == NULL || fitem->bl.type != BL::ITEM
        || (tid && fitem->cleartimer != tid))
    {
        if (battle_config.error_log)
            PRINTF("map_clearflooritem_timer : error\n");
        return;
    }
    if (!tid)
        delete_timer(fitem->cleartimer);
    clif_clearflooritem(fitem, 0);
    map_delobject(fitem->bl.id, BL::ITEM);
}

std::pair<uint16_t, uint16_t> map_randfreecell(int m, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    for (int itr : random_::iterator(w * h))
    {
        int dx = itr % w;
        int dy = itr / w;
        if (!bool(read_gat(m, x + dx, y + dy) & MapCell::UNWALKABLE))
            return {static_cast<uint16_t>(x + dx), static_cast<uint16_t>(y + dy)};
    }
    return {static_cast<uint16_t>(0), static_cast<uint16_t>(0)};
}

/// Return a randomly selected passable cell within a given range.
static
std::pair<uint16_t, uint16_t> map_searchrandfreecell(int m, int x, int y, int range)
{
    int whole_range = 2 * range + 1;
    return map_randfreecell(m, x - range, y - range, whole_range, whole_range);
}

/*==========================================
 * (m,x,y)を中心に3x3以内に床アイテム設置
 *
 * item_dataはamount以外をcopyする
 *------------------------------------------
 */
int map_addflooritem_any(struct item *item_data, int amount,
        int m, int x, int y,
        struct map_session_data **owners, interval_t *owner_protection,
        interval_t lifetime, int dispersal)
{
    struct flooritem_data *fitem = NULL;

    nullpo_ret(item_data);
    auto xy = map_searchrandfreecell(m, x, y, dispersal);
    if (xy.first == 0 && xy.second == 0)
        return 0;

    CREATE(fitem, struct flooritem_data, 1);
    fitem->bl.type = BL::ITEM;
    fitem->bl.prev = fitem->bl.next = NULL;
    fitem->bl.m = m;
    fitem->bl.x = xy.first;
    fitem->bl.y = xy.second;
    fitem->first_get_id = 0;
    fitem->first_get_tick = tick_t();
    fitem->second_get_id = 0;
    fitem->second_get_tick = tick_t();
    fitem->third_get_id = 0;
    fitem->third_get_tick = tick_t();

    fitem->bl.id = map_addobject(&fitem->bl);
    if (fitem->bl.id == 0)
    {
        free(fitem);
        return 0;
    }

    tick_t tick = gettick();

    if (owners[0])
        fitem->first_get_id = owners[0]->bl.id;
    fitem->first_get_tick = tick + owner_protection[0];

    if (owners[1])
        fitem->second_get_id = owners[1]->bl.id;
    fitem->second_get_tick = tick + owner_protection[1];

    if (owners[2])
        fitem->third_get_id = owners[2]->bl.id;
    fitem->third_get_tick = tick + owner_protection[2];

    memcpy(&fitem->item_data, item_data, sizeof(*item_data));
    fitem->item_data.amount = amount;
    // TODO - talk to 4144 about maybe removing this.
    // It has no effect on the server itself, it is visual only.
    // If it is desirable to prevent items from visibly stacking
    // on the ground, that can be done with client-side randomness.
    // Currently, it yields the numbers {3 6 9 12}.
    fitem->subx = random_::in(1, 4) * 3;
    fitem->suby = random_::in(1, 4) * 3;
    fitem->cleartimer = add_timer(gettick() + lifetime,
            std::bind(map_clearflooritem_timer, ph::_1, ph::_2,
                fitem->bl.id));

    map_addblock(&fitem->bl);
    clif_dropflooritem(fitem);

    return fitem->bl.id;
}

int map_addflooritem(struct item *item_data, int amount,
        int m, int x, int y,
        struct map_session_data *first_sd,
        struct map_session_data *second_sd,
        struct map_session_data *third_sd)
{
    struct map_session_data *owners[3] = { first_sd, second_sd, third_sd };
    interval_t owner_protection[3];

    {
        owner_protection[0] = static_cast<interval_t>(battle_config.item_first_get_time);
        owner_protection[1] = owner_protection[0] + static_cast<interval_t>(battle_config.item_second_get_time);
        owner_protection[2] = owner_protection[1] + static_cast<interval_t>(battle_config.item_third_get_time);
    }

    return map_addflooritem_any(item_data, amount, m, x, y,
            owners, owner_protection,
            static_cast<interval_t>(battle_config.flooritem_lifetime), 1);
}

/*==========================================
 * charid_dbへ追加(返信待ちがあれば返信)
 *------------------------------------------
 */
void map_addchariddb(int charid, const char *name)
{
    struct charid2nick *p = charid_db.search(charid);
    if (p == NULL)
        p = charid_db.init(charid);

    memcpy(p->nick, name, 24);
    p->req_id = 0;
}

/*==========================================
 * id_dbへblを追加
 *------------------------------------------
 */
void map_addiddb(struct block_list *bl)
{
    nullpo_retv(bl);

    id_db.put(bl->id, bl);
}

/*==========================================
 * id_dbからblを削除
 *------------------------------------------
 */
void map_deliddb(struct block_list *bl)
{
    nullpo_retv(bl);

    id_db.put(bl->id, nullptr);
}

/*==========================================
 * nick_dbへsdを追加
 *------------------------------------------
 */
void map_addnickdb(struct map_session_data *sd)
{
    nullpo_retv(sd);

    nick_db.put(sd->status.name, sd);
}

/*==========================================
 * PCのquit処理 map.c内分
 *
 * quit処理の主体が違うような気もしてきた
 *------------------------------------------
 */
void map_quit(struct map_session_data *sd)
{
    nullpo_retv(sd);

    if (sd->chatID)             // チャットから出る
        chat_leavechat(sd);

    if (sd->trade_partner)      // 取引を中断する
        trade_tradecancel(sd);

    if (sd->party_invite > 0)   // パーティ勧誘を拒否する
        party_reply_invite(sd, sd->party_invite_account, 0);

    party_send_logout(sd);     // パーティのログアウトメッセージ送信

    pc_cleareventtimer(sd);    // イベントタイマを破棄する

    skill_castcancel(&sd->bl, 0);  // 詠唱を中断する
    skill_stop_dancing(&sd->bl, 1);    // ダンス/演奏中断

    skill_status_change_clear(&sd->bl, 1); // ステータス異常を解除する
    pc_stop_walking(sd, 0);
    pc_stopattack(sd);
    pc_delinvincibletimer(sd);
    skill_gangsterparadise(sd, 0);

    pc_calcstatus(sd, 4);

    clif_clearchar(&sd->bl, BeingRemoveWhy::QUIT);

    if (pc_isdead(sd))
        pc_setrestartvalue(sd, 2);
    pc_makesavestatus(sd);
    //クローンスキルで覚えたスキルは消す

    //The storage closing routines will save the char if needed. [Skotlex]
    if (!sd->state.storage_open)
        chrif_save(sd);
    else if (sd->state.storage_open)
        storage_storage_quit(sd);

    if (sd->npc_stackbuf && sd->npc_stackbuf != NULL)
        free(sd->npc_stackbuf);

    map_delblock(&sd->bl);

    id_db.put(sd->bl.id, nullptr);
    nick_db.put(sd->status.name, nullptr);
    charid_db.erase(sd->status.char_id);
}

/*==========================================
 * id番号のPCを探す。居なければNULL
 *------------------------------------------
 */
struct map_session_data *map_id2sd(int id)
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
        if (bl && bl->type==BL::PC)
                return (struct map_session_data*)bl;
        return NULL;
*/
    int i;
    struct map_session_data *sd = NULL;

    for (i = 0; i < fd_max; i++)
        if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->bl.id == id)
            return sd;

    return NULL;
}

/*==========================================
 * char_id番号の名前を探す
 *------------------------------------------
 */
char *map_charid2nick(int id)
{
    struct charid2nick *p = charid_db.search(id);

    if (p == NULL)
        return NULL;
    if (p->req_id != 0)
        return NULL;
    return p->nick;
}

/*========================================*/
/* [Fate] Operations to iterate over active map sessions */

static
struct map_session_data *map_get_session(int i)
{
    struct map_session_data *d;

    if (i >= 0 && i < fd_max
        && session[i] && (d = (struct map_session_data *)session[i]->session_data) && d->state.auth)
        return d;

    return NULL;
}

static
struct map_session_data *map_get_session_forward(int start)
{
    int i;
    for (i = start; i < fd_max; i++)
    {
        struct map_session_data *d = map_get_session(i);
        if (d)
            return d;
    }

    return NULL;
}

static
struct map_session_data *map_get_session_backward(int start)
{
    int i;
    for (i = start; i >= 0; i--)
    {
        struct map_session_data *d = map_get_session(i);
        if (d)
            return d;
    }

    return NULL;
}

struct map_session_data *map_get_first_session(void)
{
    return map_get_session_forward(0);
}

struct map_session_data *map_get_next_session(struct map_session_data *d)
{
    return map_get_session_forward(d->fd + 1);
}

struct map_session_data *map_get_last_session(void)
{
    return map_get_session_backward(fd_max);
}

struct map_session_data *map_get_prev_session(struct map_session_data *d)
{
    return map_get_session_backward(d->fd - 1);
}

/*==========================================
 * Search session data from a nick name
 * (without sensitive case if necessary)
 * return map_session_data pointer or NULL
 *------------------------------------------
 */
struct map_session_data *map_nick2sd(const char *nick)
{
    int i, quantity = 0, nicklen;
    struct map_session_data *sd = NULL;
    struct map_session_data *pl_sd = NULL;

    if (nick == NULL)
        return NULL;

    nicklen = strlen(nick);

    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (pl_sd = (struct map_session_data *)session[i]->session_data)
            && pl_sd->state.auth)
        {
            // Without case sensitive check (increase the number of similar character names found)
            if (strncasecmp(pl_sd->status.name, nick, nicklen) == 0)
            {
                // Strict comparison (if found, we finish the function immediatly with correct value)
                if (strcmp(pl_sd->status.name, nick) == 0)
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
struct block_list *map_id2bl(int id)
{
    struct block_list *bl = NULL;
    if (id < sizeof(object) / sizeof(object[0]))
        bl = object[id];
    else
        bl = id_db.get(id);

    return bl;
}

/*==========================================
 * map.npcへ追加 (warp等の領域持ちのみ)
 *------------------------------------------
 */
int map_addnpc(int m, struct npc_data *nd)
{
    int i;
    if (m < 0 || m >= map_num)
        return -1;
    for (i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++)
        if (map[m].npc[i] == NULL)
            break;
    if (i == MAX_NPC_PER_MAP)
    {
        if (battle_config.error_log)
            PRINTF("too many NPCs in one map %s\n", map[m].name);
        return -1;
    }
    if (i == map[m].npc_num)
    {
        map[m].npc_num++;
    }

    nullpo_ret(nd);

    map[m].npc[i] = nd;
    nd->n = i;
    id_db.put(nd->bl.id, (struct block_list *)nd);

    return i;
}

static
void map_removenpc(void)
{
    int i, m, n = 0;

    for (m = 0; m < map_num; m++)
    {
        for (i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++)
        {
            if (map[m].npc[i] != NULL)
            {
                clif_clearchar(&map[m].npc[i]->bl, BeingRemoveWhy::QUIT);
                map_delblock(&map[m].npc[i]->bl);
                id_db.put(map[m].npc[i]->bl.id, nullptr);
                if (map[m].npc[i]->bl.subtype == NpcSubtype::SCRIPT)
                {
//                    free(map[m].npc[i]->u.scr.script);
//                    free(map[m].npc[i]->u.scr.label_list);
                }
                free(map[m].npc[i]);
                map[m].npc[i] = NULL;
                n++;
            }
        }
    }
    PRINTF("%d NPCs removed.\n", n);
}

/*==========================================
 * map名からmap番号へ変換
 *------------------------------------------
 */
int map_mapname2mapid(const char *name)
{
    struct map_data *md = map_db.get(name);
    if (md == NULL || md->gat == NULL)
        return -1;
    return md->m;
}

/*==========================================
 * 他鯖map名からip,port変換
 *------------------------------------------
 */
int map_mapname2ipport(const char *name, struct in_addr *ip, int *port)
{
    struct map_data_other_server *mdos = (struct map_data_other_server *)map_db.get(name);
    if (mdos == NULL || mdos->gat)
        return -1;
    *ip = mdos->ip;
    *port = mdos->port;
    return 0;
}

/// Check compatibility of directions.
/// Directions are compatible if they are at most 45° apart.
///
/// @return false if compatible, true if incompatible.
bool map_check_dir(const DIR s_dir, const DIR t_dir)
{
    if (s_dir == t_dir)
        return false;

    const uint8_t sdir = static_cast<uint8_t>(s_dir);
    const uint8_t tdir = static_cast<uint8_t>(t_dir);
    if ((sdir + 1) % 8 == tdir)
        return false;
    if (sdir == (tdir + 1) % 8)
        return false;

    return true;
}

/*==========================================
 * 彼我の方向を計算
 *------------------------------------------
 */
DIR map_calc_dir(struct block_list *src, int x, int y)
{
    DIR dir = DIR::S;
    int dx, dy;

    nullpo_retr(DIR::S, src);

    dx = x - src->x;
    dy = y - src->y;
    if (dx == 0 && dy == 0)
    {
        dir = DIR::S;
    }
    else if (dx >= 0 && dy >= 0)
    {
        dir = DIR::SE;
        if (dx * 3 - 1 < dy)
            dir = DIR::S;
        if (dx > dy * 3)
            dir = DIR::E;
    }
    else if (dx >= 0 && dy <= 0)
    {
        dir = DIR::NE;
        if (dx * 3 - 1 < -dy)
            dir = DIR::N;
        if (dx > -dy * 3)
            dir = DIR::E;
    }
    else if (dx <= 0 && dy <= 0)
    {
        dir = DIR::NW;
        if (dx * 3 + 1 > dy)
            dir = DIR::N;
        if (dx < dy * 3)
            dir = DIR::W;
    }
    else
    {
        dir = DIR::SW;
        if (-dx * 3 - 1 < dy)
            dir = DIR::S;
        if (-dx > dy * 3)
            dir = DIR::W;
    }
    return dir;
}

// gat系
/*==========================================
 * (m,x,y)の状態を調べる
 *------------------------------------------
 */
MapCell map_getcell(int m, int x, int y)
{
    if (x < 0 || x >= map[m].xs - 1 || y < 0 || y >= map[m].ys - 1)
        return MapCell::UNWALKABLE;
    return map[m].gat[x + y * map[m].xs];
}

/*==========================================
 * (m,x,y)の状態をtにする
 *------------------------------------------
 */
void map_setcell(int m, int x, int y, MapCell t)
{
    if (x < 0 || x >= map[m].xs || y < 0 || y >= map[m].ys)
        return;
    map[m].gat[x + y * map[m].xs] = t;
}

/*==========================================
 * 他鯖管理のマップをdbに追加
 *------------------------------------------
 */
int map_setipport(const char *name, struct in_addr ip, int port)
{
    struct map_data *md = map_db.get(name);
    if (md == NULL)
    {
        struct map_data_other_server *mdos = NULL;
        // not exist -> add new data
        CREATE(mdos, struct map_data_other_server, 1);
        memcpy(mdos->name, name, 24);
        mdos->gat = NULL;
        mdos->ip = ip;
        mdos->port = port;
        map_db.put(mdos->name, (struct map_data *)mdos);
    }
    else
    {
        if (md->gat)
        {
            // local -> check data
            if (ip.s_addr != clif_getip().s_addr || port != clif_getport())
            {
                PRINTF("from char server : %s -> %s:%d\n", name, ip2str(ip),
                        port);
                return 1;
            }
        }
        else
        {
            // update
            struct map_data_other_server *mdos = NULL;
            mdos = (struct map_data_other_server *) md;
            mdos->ip = ip;
            mdos->port = port;
        }
    }
    return 0;
}

/*==========================================
 * マップ1枚読み込み
 *------------------------------------------
 */
static
bool map_readmap(int m, const_string fn)
{
    // read & convert fn
    std::vector<uint8_t> gat_v = grfio_reads(fn);
    if (gat_v.empty())
        return false;
    size_t s = gat_v.size() - 4;

    map[m].m = m;
    int xs = map[m].xs = gat_v[0] | gat_v[1] << 8;
    int ys = map[m].ys = gat_v[2] | gat_v[3] << 8;
    PRINTF("\rLoading Maps [%d/%d]: %-30s  (%i, %i)",
            m, map_num, std::string(fn.begin(), fn.end()), xs, ys);
    fflush(stdout);

    assert (s == xs * ys);
    map[m].gat = make_unique<MapCell[]>(s);
    if (map[m].gat == NULL)
    {
        PRINTF("out of memory : map_readmap gat\n");
        exit(1);
    }

    map[m].npc_num = 0;
    map[m].users = 0;
    memset(&map[m].flag, 0, sizeof(map[m].flag));
    if (battle_config.pk_mode)
        map[m].flag.pvp = 1;    // make all maps pvp for pk_mode [Valaris]
    MapCell *gat_m = reinterpret_cast<MapCell *>(&gat_v[4]);
    std::copy(gat_m, gat_m + s, &map[m].gat[0]);

    map[m].bxs = (xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
    map[m].bys = (ys + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t size = map[m].bxs * map[m].bys;

    CREATE(map[m].block, struct block_list *, size);
    CREATE(map[m].block_mob, struct block_list *, size);
    CREATE(map[m].block_count, int, size);
    CREATE(map[m].block_mob_count, int, size);

    map_db.put(map[m].name, &map[m]);

    return true;
}

/*==========================================
 * 全てのmapデータを読み込む
 *------------------------------------------
 */
static
int map_readallmap(void)
{
    int i, maps_removed = 0;

    for (i = 0; i < map_num; i++)
    {
        assert (strstr(map[i].name, ".gat") != NULL);
        {
            {
                if (!map_readmap(i, map[i].name))
                {
                    map_delmap(map[i].name);
                    maps_removed++;
                }
            }
        }
    }

    PRINTF("\rMaps Loaded: %d %60s\n", map_num, "");
    PRINTF("\rMaps Removed: %d \n", maps_removed);
    return 0;
}

/*==========================================
 * 読み込むmapを追加する
 *------------------------------------------
 */
static
int map_addmap(const char *mapname)
{
    if (strcasecmp(mapname, "clear") == 0)
    {
        map_num = 0;
        return 0;
    }

    if (map_num >= MAX_MAP_PER_SERVER - 1)
    {
        PRINTF("too many map\n");
        return 1;
    }
    memcpy(map[map_num].name, mapname, 24);
    map_num++;
    return 0;
}

/*==========================================
 * 読み込むmapを削除する
 *------------------------------------------
 */
static
int map_delmap(const char *mapname)
{
    int i;

    if (strcasecmp(mapname, "all") == 0)
    {
        map_num = 0;
        return 0;
    }

    for (i = 0; i < map_num; i++)
    {
        if (strcmp(map[i].name, mapname) == 0)
        {
            PRINTF("Removing map [ %s ] from maplist\n", map[i].name);
            memmove(map + i, map + i + 1,
                     sizeof(map[0]) * (map_num - i - 1));
            map_num--;
        }
    }
    return 0;
}

constexpr int LOGFILE_SECONDS_PER_CHUNK_SHIFT = 10;

static
FILE *map_logfile = NULL;
static
char *map_logfile_name = NULL;
static
long map_logfile_index;

static
void map_close_logfile(void)
{
    if (map_logfile)
    {
        std::string filename = STRPRINTF("%s.%ld", map_logfile_name, map_logfile_index);
        const char *args[] =
        {
            "gzip",
            "-f",
            filename.c_str(),
            NULL
        };
        char **argv = const_cast<char **>(args);

        fclose(map_logfile);

        if (!fork())
        {
            execvp("gzip", argv);
            _exit(1);
        }
        wait(NULL);
    }
}

static
void map_start_logfile(long index)
{
    map_logfile_index = index;

    std::string filename_buf = STRPRINTF(
            "%s.%ld",
            map_logfile_name,
            map_logfile_index);
    map_logfile = fopen(filename_buf.c_str(), "w+");
    if (!map_logfile)
        perror(map_logfile_name);
}

static
void map_set_logfile(const char *filename)
{
    struct timeval tv;

    map_logfile_name = strdup(filename);
    gettimeofday(&tv, NULL);

    map_start_logfile(tv.tv_sec);

    MAP_LOG("log-start v3");
}

void map_log(const_string line)
{
    if (!map_logfile)
        return;

    time_t t = TimeT::now();
    long i = t >> LOGFILE_SECONDS_PER_CHUNK_SHIFT;

    if (i != map_logfile_index)
    {
        map_close_logfile();
        map_start_logfile(i);
    }

    log_with_timestamp(map_logfile, line);
}

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
static
int map_config_read(const char *cfgName)
{
    struct hostent *h = NULL;

    std::ifstream in(cfgName);
    if (!in.is_open())
    {
        PRINTF("Map configuration file not found at: %s\n", cfgName);
        exit(1);
    }

    std::string line;
    while (std::getline(in, line))
    {
        std::string w1, w2;
        if (!split_key_value(line, &w1, &w2))
            continue;
        if (w1 == "userid")
        {
            chrif_setuserid(w2.c_str());
        }
        else if (w1 == "passwd")
        {
            chrif_setpasswd(w2.c_str());
        }
        else if (w1 == "char_ip")
        {
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                PRINTF("Character server IP address : %s -> %d.%d.%d.%d\n",
                     w2, (unsigned char) h->h_addr[0],
                     (unsigned char) h->h_addr[1],
                     (unsigned char) h->h_addr[2],
                     (unsigned char) h->h_addr[3]);
                SPRINTF(w2, "%d.%d.%d.%d", (unsigned char) h->h_addr[0],
                         (unsigned char) h->h_addr[1],
                         (unsigned char) h->h_addr[2],
                         (unsigned char) h->h_addr[3]);
            }
            chrif_setip(w2.c_str());
        }
        else if (w1 == "char_port")
        {
            chrif_setport(atoi(w2.c_str()));
        }
        else if (w1 == "map_ip")
        {
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                PRINTF("Map server IP address : %s -> %d.%d.%d.%d\n", w2,
                        (unsigned char) h->h_addr[0],
                        (unsigned char) h->h_addr[1],
                        (unsigned char) h->h_addr[2],
                        (unsigned char) h->h_addr[3]);
                SPRINTF(w2, "%d.%d.%d.%d", (unsigned char) h->h_addr[0],
                         (unsigned char) h->h_addr[1],
                         (unsigned char) h->h_addr[2],
                         (unsigned char) h->h_addr[3]);
            }
            clif_setip(w2.c_str());
        }
        else if (w1 == "map_port")
        {
            clif_setport(atoi(w2.c_str()));
        }
        else if (w1 == "map")
        {
            map_addmap(w2.c_str());
        }
        else if (w1 == "delmap")
        {
            map_delmap(w2.c_str());
        }
        else if (w1 == "npc")
        {
            npc_addsrcfile(w2.c_str());
        }
        else if (w1 == "delnpc")
        {
            npc_delsrcfile(w2.c_str());
        }
        else if (w1 == "autosave_time")
        {
            autosave_interval = std::chrono::seconds(atoi(w2.c_str()));
            if (autosave_interval <= interval_t::zero())
                autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
        }
        else if (w1 == "motd_txt")
        {
            strzcpy(motd_txt, w2.c_str(), sizeof(motd_txt));
        }
        else if (w1 == "help_txt")
        {
            strzcpy(help_txt, w2.c_str(), sizeof(help_txt));
        }
        else if (w1 == "mapreg_txt")
        {
            strzcpy(mapreg_txt, w2.c_str(), sizeof(mapreg_txt));
        }
        else if (w1 == "gm_log")
        {
            gm_logfile_name = strdup(w2.c_str());
        }
        else if (w1 == "log_file")
        {
            map_set_logfile(w2.c_str());
        }
        else if (w1 == "import")
        {
            map_config_read(w2.c_str());
        }
    }

    return 0;
}

static
void cleanup_sub(struct block_list *bl)
{
    nullpo_retv(bl);

    switch (bl->type)
    {
        case BL::PC:
            map_delblock(bl);  // There is something better...
            break;
        case BL::NPC:
            npc_delete((struct npc_data *) bl);
            break;
        case BL::MOB:
            mob_delete((struct mob_data *) bl);
            break;
        case BL::ITEM:
            map_clearflooritem(bl->id);
            break;
        case BL::SPELL:
            spell_free_invocation((struct invocation *) bl);
            break;
    }
}

/*==========================================
 * map鯖終了時処理
 *------------------------------------------
 */
void term_func(void)
{
    map_close_logfile();

    int map_id, i;

    for (map_id = 0; map_id < map_num; map_id++)
    {
        if (map[map_id].m)
            map_foreachinarea(cleanup_sub, map_id,
                    0, 0, map[map_id].xs, map[map_id].ys,
                    BL::NUL);
    }

    for (i = 0; i < fd_max; i++)
        delete_session(i);

    map_removenpc();

    for (i = 0; i <= map_num; i++)
    {
        map[i].gat = nullptr;
        if (map[i].block)
            free(map[i].block);
        if (map[i].block_mob)
            free(map[i].block_mob);
        if (map[i].block_count)
            free(map[i].block_count);
        if (map[i].block_mob_count)
            free(map[i].block_mob_count);
    }
    do_final_script();
    do_final_itemdb();
    do_final_storage();
}

/// --help was passed
// FIXME this should produce output
static __attribute__((noreturn))
void map_helpscreen(void)
{
    exit(1);
}

int compare_item(struct item *a, struct item *b)
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
int do_init(int argc, char *argv[])
{
    int i;

    const char *MAP_CONF_NAME = "conf/map_athena.conf";
    const char *BATTLE_CONF_FILENAME = "conf/battle_athena.conf";
    const char *ATCOMMAND_CONF_FILENAME = "conf/atcommand_athena.conf";

    for (i = 1; i < argc; i++)
    {

        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--h") == 0
            || strcmp(argv[i], "--?") == 0 || strcmp(argv[i], "/?") == 0)
            map_helpscreen();
        else if (strcmp(argv[i], "--map_config") == 0)
            MAP_CONF_NAME = argv[i + 1];
        else if (strcmp(argv[i], "--battle_config") == 0)
            BATTLE_CONF_FILENAME = argv[i + 1];
        else if (strcmp(argv[i], "--atcommand_config") == 0)
            ATCOMMAND_CONF_FILENAME = argv[i + 1];
    }

    map_config_read(MAP_CONF_NAME);
    battle_config_read(BATTLE_CONF_FILENAME);
    atcommand_config_read(ATCOMMAND_CONF_FILENAME);
    script_config_read();

    map_readallmap();

    do_init_chrif ();
    do_init_clif ();
    do_init_itemdb();
    do_init_mob();             // npcの初期化時内でmob_spawnして、mob_dbを参照するのでinit_npcより先
    do_init_script();
    do_init_npc();
    do_init_pc();
    do_init_party();
    do_init_storage();
    do_init_skill();
    do_init_magic();

    npc_event_do_oninit();     // npcのOnInitイベント実行

    if (battle_config.pk_mode == 1)
        PRINTF("The server is running in \033[1;31mPK Mode\033[0m.\n");

    PRINTF("The map-server is \033[1;32mready\033[0m (Server is listening on the port %d).\n\n",
         map_port);

    return 0;
}

int map_scriptcont(struct map_session_data *sd, int id)
{
    struct block_list *bl = map_id2bl(id);

    if (!bl)
        return 0;

    switch (bl->type)
    {
        case BL::NPC:
            return npc_scriptcont(sd, id);
        case BL::SPELL:
            spell_execute_script((struct invocation *) bl);
            break;
    }

    return 0;
}
