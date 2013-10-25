#include "map.hpp"

#include <sys/time.h>
#include <sys/wait.h>

#include <netdb.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <fstream>

#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../common/core.hpp"
#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/extract.hpp"
#include "../common/io.hpp"
#include "../common/random2.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "grfio.hpp"
#include "itemdb.hpp"
#include "magic.hpp"
#include "magic-interpreter.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "script.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "trade.hpp"

#include "../poison.hpp"

DMap<int, dumb_ptr<block_list>> id_db;

UPMap<MapName, map_abstract> maps_db;

static
DMap<CharName, dumb_ptr<map_session_data>> nick_db;

struct charid2nick
{
    CharName nick;
    int req_id;
};

static
Map<int, struct charid2nick> charid_db;

static
int users = 0;
static
dumb_ptr<block_list> object[MAX_FLOORITEM];
static
int first_free_object_id = 0, last_object_id = 0;

interval_t autosave_time = DEFAULT_AUTOSAVE_INTERVAL;
int save_settings = 0xFFFF;

FString motd_txt = "conf/motd.txt";
FString help_txt = "conf/help.txt";

CharName wisp_server_name = stringish<CharName>("Server");   // can be modified in char-server configuration file

static
void map_delmap(MapName mapname);

void SessionDeleter::operator()(SessionData *sd)
{
    really_delete1 static_cast<map_session_data *>(sd);
}

bool extract(XString str, NpcEvent *ev)
{
    XString mid;
    return extract(str, record<':'>(&ev->npc, &mid, &ev->label)) && !mid;
}

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

static
int block_free_lock = 0;
static
std::vector<dumb_ptr<block_list>> block_free;

void MapBlockLock::freeblock(dumb_ptr<block_list> bl)
{
    if (block_free_lock == 0)
        bl.delete_();
    else
        block_free.push_back(bl);
}

MapBlockLock::MapBlockLock()
{
    ++block_free_lock;
}

MapBlockLock::~MapBlockLock()
{
    assert (block_free_lock > 0);
    if ((--block_free_lock) == 0)
    {
        for (dumb_ptr<block_list> bl : block_free)
            bl.delete_();
        block_free.clear();
    }
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
int map_addblock(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);

    if (bl->bl_prev)
    {
        if (battle_config.error_log)
            PRINTF("map_addblock error : bl->bl_prev!=NULL\n");
        return 0;
    }

    map_local *m = bl->bl_m;
    int x = bl->bl_x;
    int y = bl->bl_y;
    if (!m ||
        x < 0 || x >= m->xs || y < 0 || y >= m->ys)
        return 1;

    if (bl->bl_type == BL::MOB)
    {
        bl->bl_next = m->blocks.ref(x / BLOCK_SIZE, y / BLOCK_SIZE).mobs_only;
        bl->bl_prev = dumb_ptr<block_list>(&bl_head);
        if (bl->bl_next)
            bl->bl_next->bl_prev = bl;
        m->blocks.ref(x / BLOCK_SIZE, y / BLOCK_SIZE).mobs_only = bl;
    }
    else
    {
        bl->bl_next = m->blocks.ref(x / BLOCK_SIZE, y / BLOCK_SIZE).normal;
        bl->bl_prev = dumb_ptr<block_list>(&bl_head);
        if (bl->bl_next)
            bl->bl_next->bl_prev = bl;
        m->blocks.ref(x / BLOCK_SIZE, y / BLOCK_SIZE).normal = bl;
        if (bl->bl_type == BL::PC)
            m->users++;
    }

    return 0;
}

/*==========================================
 * map[]のblock_listから外す
 * prevがNULLの場合listに繋がってない
 *------------------------------------------
 */
int map_delblock(dumb_ptr<block_list> bl)
{
    nullpo_ret(bl);

    // 既にblocklistから抜けている
    if (!bl->bl_prev)
    {
        if (bl->bl_next)
        {
            // prevがNULLでnextがNULLでないのは有ってはならない
            if (battle_config.error_log)
                PRINTF("map_delblock error : bl->bl_next!=NULL\n");
        }
        return 0;
    }

    if (bl->bl_type == BL::PC)
        bl->bl_m->users--;

    if (bl->bl_next)
        bl->bl_next->bl_prev = bl->bl_prev;
    if (bl->bl_prev == dumb_ptr<block_list>(&bl_head))
    {
        // リストの頭なので、map[]のblock_listを更新する
        if (bl->bl_type == BL::MOB)
        {
            bl->bl_m->blocks.ref(bl->bl_x / BLOCK_SIZE, bl->bl_y / BLOCK_SIZE).mobs_only = bl->bl_next;
        }
        else
        {
            bl->bl_m->blocks.ref(bl->bl_x / BLOCK_SIZE, bl->bl_y / BLOCK_SIZE).normal = bl->bl_next;
        }
    }
    else
    {
        bl->bl_prev->bl_next = bl->bl_next;
    }
    bl->bl_next = NULL;
    bl->bl_prev = NULL;

    return 0;
}

/*==========================================
 * セル上のPCとMOBの数を数える (グランドクロス用)
 *------------------------------------------
 */
int map_count_oncell(map_local *m, int x, int y)
{
    int bx, by;
    dumb_ptr<block_list> bl = NULL;
    int count = 0;

    if (x < 0 || y < 0 || (x >= m->xs) || (y >= m->ys))
        return 1;
    bx = x / BLOCK_SIZE;
    by = y / BLOCK_SIZE;

    bl = m->blocks.ref(bx, by).normal;
    for (; bl; bl = bl->bl_next)
    {
        if (bl->bl_x == x && bl->bl_y == y && bl->bl_type == BL::PC)
            count++;
    }
    bl = m->blocks.ref(bx, by).mobs_only;
    for (; bl; bl = bl->bl_next)
    {
        if (bl->bl_x == x && bl->bl_y == y)
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
void map_foreachinarea(std::function<void(dumb_ptr<block_list>)> func,
        map_local *m,
        int x0, int y0, int x1, int y1,
        BL type)
{
    std::vector<dumb_ptr<block_list>> bl_list;

    if (!m)
        return;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= m->xs)
        x1 = m->xs - 1;
    if (y1 >= m->ys)
        y1 = m->ys - 1;
    if (type == BL::NUL || type != BL::MOB)
        for (int by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (int bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                dumb_ptr<block_list> bl = m->blocks.ref(bx, by).normal;
                for (; bl; bl = bl->bl_next)
                {
                    if (type != BL::NUL && bl->bl_type != type)
                        continue;
                    if (bl->bl_x >= x0 && bl->bl_x <= x1
                        && bl->bl_y >= y0 && bl->bl_y <= y1)
                        bl_list.push_back(bl);
                }
            }
        }
    if (type == BL::NUL || type == BL::MOB)
        for (int by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (int bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                dumb_ptr<block_list> bl = m->blocks.ref(bx, by).mobs_only;
                for (; bl; bl = bl->bl_next)
                {
                    if (bl->bl_x >= x0 && bl->bl_x <= x1
                        && bl->bl_y >= y0 && bl->bl_y <= y1)
                        bl_list.push_back(bl);
                }
            }
        }

    MapBlockLock lock;

    for (dumb_ptr<block_list> bl : bl_list)
        if (bl->bl_prev)
            func(bl);
}

/*==========================================
 * 矩形(x0,y0)-(x1,y1)が(dx,dy)移動した時の
 * 領域外になる領域(矩形かL字形)内のobjに
 * 対してfuncを呼ぶ
 *
 * dx,dyは-1,0,1のみとする（どんな値でもいいっぽい？）
 *------------------------------------------
 */
void map_foreachinmovearea(std::function<void(dumb_ptr<block_list>)> func,
        map_local *m,
        int x0, int y0, int x1, int y1,
        int dx, int dy,
        BL type)
{
    std::vector<dumb_ptr<block_list>> bl_list;
    // Note: the x0, y0, x1, y1 are bl.bl_x, bl.bl_y ± AREA_SIZE,
    // but only a small subset actually needs to be done.
    if (dx == 0 || dy == 0)
    {
        assert (dx || dy);
        // for aligned movement, only needs to check a rectangular area
        if (dx == 0)
        {
            if (dy < 0)
                y0 = y1 + dy + 1;
            else
                y1 = y0 + dy - 1;
        }
        else if (dy == 0)
        {
            if (dx < 0)
                x0 = x1 + dx + 1;
            else
                x1 = x0 + dx - 1;
        }
        if (x0 < 0)
            x0 = 0;
        if (y0 < 0)
            y0 = 0;
        if (x1 >= m->xs)
            x1 = m->xs - 1;
        if (y1 >= m->ys)
            y1 = m->ys - 1;
        for (int by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (int bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                dumb_ptr<block_list> bl = m->blocks.ref(bx, by).normal;
                for (; bl; bl = bl->bl_next)
                {
                    if (type != BL::NUL && bl->bl_type != type)
                        continue;
                    if (bl->bl_x >= x0 && bl->bl_x <= x1
                        && bl->bl_y >= y0 && bl->bl_y <= y1)
                        bl_list.push_back(bl);
                }
                bl = m->blocks.ref(bx, by).mobs_only;
                for (; bl; bl = bl->bl_next)
                {
                    if (type != BL::NUL && bl->bl_type != type)
                        continue;
                    if (bl->bl_x >= x0 && bl->bl_x <= x1
                        && bl->bl_y >= y0 && bl->bl_y <= y1)
                        bl_list.push_back(bl);
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
        if (x1 >= m->xs)
            x1 = m->xs - 1;
        if (y1 >= m->ys)
            y1 = m->ys - 1;
        for (int by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++)
        {
            for (int bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++)
            {
                dumb_ptr<block_list> bl = m->blocks.ref(bx, by).normal;
                for (; bl; bl = bl->bl_next)
                {
                    if (type != BL::NUL && bl->bl_type != type)
                        continue;
                    if (!(bl->bl_x >= x0 && bl->bl_x <= x1
                            && bl->bl_y >= y0 && bl->bl_y <= y1))
                        continue;
                    if ((dx > 0 && bl->bl_x < x0 + dx)
                        || (dx < 0 && bl->bl_x > x1 + dx)
                        || (dy > 0 && bl->bl_y < y0 + dy)
                        || (dy < 0 && bl->bl_y > y1 + dy))
                        bl_list.push_back(bl);
                }
                bl = m->blocks.ref(bx, by).mobs_only;
                for (; bl; bl = bl->bl_next)
                {
                    if (type != BL::NUL && bl->bl_type != type)
                        continue;
                    if (!(bl->bl_x >= x0 && bl->bl_x <= x1
                             && bl->bl_y >= y0 && bl->bl_y <= y1))
                        continue;
                    if ((dx > 0 && bl->bl_x < x0 + dx)
                        || (dx < 0 && bl->bl_x > x1 + dx)
                        || (dy > 0 && bl->bl_y < y0 + dy)
                        || (dy < 0 && bl->bl_y > y1 + dy))
                        bl_list.push_back(bl);
                }
            }
        }

    }

    MapBlockLock lock;

    for (dumb_ptr<block_list> bl : bl_list)
        if (bl->bl_prev)
            func(bl);
}

// -- moonsoul  (added map_foreachincell which is a rework of map_foreachinarea but
//           which only checks the exact single x/y passed to it rather than an
//           area radius - may be more useful in some instances)
//
void map_foreachincell(std::function<void(dumb_ptr<block_list>)> func,
        map_local *m,
        int x, int y,
        BL type)
{
    std::vector<dumb_ptr<block_list>> bl_list;
    int by = y / BLOCK_SIZE;
    int bx = x / BLOCK_SIZE;

    if (type == BL::NUL || type != BL::MOB)
    {
        dumb_ptr<block_list> bl = m->blocks.ref(bx, by).normal;
        for (; bl; bl = bl->bl_next)
        {
            if (type != BL::NUL && bl->bl_type != type)
                continue;
            if (bl->bl_x == x && bl->bl_y == y)
                bl_list.push_back(bl);
        }
    }

    if (type == BL::NUL || type == BL::MOB)
    {
        dumb_ptr<block_list> bl = m->blocks.ref(bx, by).mobs_only;
        for (; bl; bl = bl->bl_next)
        {
            if (bl->bl_x == x && bl->bl_y == y)
                bl_list.push_back(bl);
        }
    }

    MapBlockLock lock;

    for (dumb_ptr<block_list> bl : bl_list)
        if (bl->bl_prev)
            func(bl);
}

/*==========================================
 * 床アイテムやエフェクト用の一時obj割り当て
 * object[]への保存とid_db登録まで
 *
 * bl->bl_idもこの中で設定して問題無い?
 *------------------------------------------
 */
int map_addobject(dumb_ptr<block_list> bl)
{
    int i;
    if (!bl)
    {
        PRINTF("map_addobject nullpo?\n");
        return 0;
    }
    if (first_free_object_id < 2 || first_free_object_id >= MAX_FLOORITEM)
        first_free_object_id = 2;
    for (i = first_free_object_id; i < MAX_FLOORITEM; i++)
        if (!object[i])
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
    if (!object[id])
        return 0;

    if (object[id]->bl_type != type)
    {
        FPRINTF(stderr, "Incorrect type: expected %d, got %d\n",
                type,
                object[id]->bl_type);
        abort();
    }

    map_delblock(object[id]);
    id_db.put(id, dumb_ptr<block_list>());
//  map_freeblock(object[id]);
    object[id] = nullptr;

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
    dumb_ptr<block_list> obj = object[id];

    if (obj == NULL)
        return 0;

    map_delobjectnofree(id, type);
    if (obj->bl_type == BL::PC)     // [Fate] Not sure where else to put this... I'm not sure where delobject for PCs is called from
        pc_cleanup(obj->is_player());

    MapBlockLock::freeblock(obj);

    return 0;
}

/*==========================================
 * 全一時obj相手にfuncを呼ぶ
 *
 *------------------------------------------
 */
void map_foreachobject(std::function<void(dumb_ptr<block_list>)> func,
        BL type)
{
    std::vector<dumb_ptr<block_list>> bl_list;
    for (int i = 2; i <= last_object_id; i++)
    {
        if (!object[i])
            continue;
        {
            if (type != BL::NUL && object[i]->bl_type != type)
                continue;
            bl_list.push_back(object[i]);
        }
    }

    MapBlockLock lock;

    for (dumb_ptr<block_list> bl : bl_list)
        if (bl->bl_prev || bl->bl_next)
            func(bl);
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
    dumb_ptr<block_list> obj = object[id];
    assert (obj && obj->bl_type == BL::ITEM);
    dumb_ptr<flooritem_data> fitem = obj->is_item();
    if (!tid)
        fitem->cleartimer.cancel();
    clif_clearflooritem(fitem, 0);
    map_delobject(fitem->bl_id, BL::ITEM);
}

std::pair<uint16_t, uint16_t> map_randfreecell(map_local *m,
        uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    for (int itr : random_::iterator(w * h))
    {
        int dx = itr % w;
        int dy = itr / w;
        if (!bool(read_gatp(m, x + dx, y + dy) & MapCell::UNWALKABLE))
            return {static_cast<uint16_t>(x + dx), static_cast<uint16_t>(y + dy)};
    }
    return {static_cast<uint16_t>(0), static_cast<uint16_t>(0)};
}

/// Return a randomly selected passable cell within a given range.
static
std::pair<uint16_t, uint16_t> map_searchrandfreecell(map_local *m, int x, int y, int range)
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
        map_local *m, int x, int y,
        dumb_ptr<map_session_data> *owners, interval_t *owner_protection,
        interval_t lifetime, int dispersal)
{
    dumb_ptr<flooritem_data> fitem = NULL;

    nullpo_ret(item_data);
    auto xy = map_searchrandfreecell(m, x, y, dispersal);
    if (xy.first == 0 && xy.second == 0)
        return 0;

    fitem.new_();
    fitem->bl_type = BL::ITEM;
    fitem->bl_prev = fitem->bl_next = NULL;
    fitem->bl_m = m;
    fitem->bl_x = xy.first;
    fitem->bl_y = xy.second;
    fitem->first_get_id = 0;
    fitem->first_get_tick = tick_t();
    fitem->second_get_id = 0;
    fitem->second_get_tick = tick_t();
    fitem->third_get_id = 0;
    fitem->third_get_tick = tick_t();

    fitem->bl_id = map_addobject(fitem);
    if (fitem->bl_id == 0)
    {
        fitem.delete_();
        return 0;
    }

    tick_t tick = gettick();

    if (owners[0])
        fitem->first_get_id = owners[0]->bl_id;
    fitem->first_get_tick = tick + owner_protection[0];

    if (owners[1])
        fitem->second_get_id = owners[1]->bl_id;
    fitem->second_get_tick = tick + owner_protection[1];

    if (owners[2])
        fitem->third_get_id = owners[2]->bl_id;
    fitem->third_get_tick = tick + owner_protection[2];

    fitem->item_data = *item_data;
    fitem->item_data.amount = amount;
    // TODO - talk to 4144 about maybe removing this.
    // It has no effect on the server itself, it is visual only.
    // If it is desirable to prevent items from visibly stacking
    // on the ground, that can be done with client-side randomness.
    // Currently, it yields the numbers {3 6 9 12}.
    fitem->subx = random_::in(1, 4) * 3;
    fitem->suby = random_::in(1, 4) * 3;
    fitem->cleartimer = Timer(gettick() + lifetime,
            std::bind(map_clearflooritem_timer, ph::_1, ph::_2,
                fitem->bl_id));

    map_addblock(fitem);
    clif_dropflooritem(fitem);

    return fitem->bl_id;
}

int map_addflooritem(struct item *item_data, int amount,
        map_local *m, int x, int y,
        dumb_ptr<map_session_data> first_sd,
        dumb_ptr<map_session_data> second_sd,
        dumb_ptr<map_session_data> third_sd)
{
    dumb_ptr<map_session_data> owners[3] = { first_sd, second_sd, third_sd };
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
void map_addchariddb(int charid, CharName name)
{
    struct charid2nick *p = charid_db.search(charid);
    if (p == NULL)
        p = charid_db.init(charid);

    p->nick = name;
    p->req_id = 0;
}

/*==========================================
 * id_dbへblを追加
 *------------------------------------------
 */
void map_addiddb(dumb_ptr<block_list> bl)
{
    nullpo_retv(bl);

    id_db.put(bl->bl_id, bl);
}

/*==========================================
 * id_dbからblを削除
 *------------------------------------------
 */
void map_deliddb(dumb_ptr<block_list> bl)
{
    nullpo_retv(bl);

    id_db.put(bl->bl_id, nullptr);
}

/*==========================================
 * nick_dbへsdを追加
 *------------------------------------------
 */
void map_addnickdb(dumb_ptr<map_session_data> sd)
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
void map_quit(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    if (sd->trade_partner)      // 取引を中断する
        trade_tradecancel(sd);

    if (sd->party_invite > 0)   // パーティ勧誘を拒否する
        party_reply_invite(sd, sd->party_invite_account, 0);

    party_send_logout(sd);     // パーティのログアウトメッセージ送信

    pc_cleareventtimer(sd);    // イベントタイマを破棄する

    skill_castcancel(sd, 0);  // 詠唱を中断する
    skill_stop_dancing(sd, 1);    // ダンス/演奏中断

    skill_status_change_clear(sd, 1); // ステータス異常を解除する
    pc_stop_walking(sd, 0);
    pc_stopattack(sd);
    pc_delinvincibletimer(sd);

    pc_calcstatus(sd, 4);

    clif_clearchar(sd, BeingRemoveWhy::QUIT);

    if (pc_isdead(sd))
        pc_setrestartvalue(sd, 2);
    pc_makesavestatus(sd);
    //クローンスキルで覚えたスキルは消す

    //The storage closing routines will save the char if needed. [Skotlex]
    if (!sd->state.storage_open)
        chrif_save(sd);
    else if (sd->state.storage_open)
        storage_storage_quit(sd);

    sd->npc_stackbuf.clear();

    map_delblock(sd);

    id_db.put(sd->bl_id, nullptr);
    nick_db.put(sd->status.name, nullptr);
    charid_db.erase(sd->status.char_id);
}

/*==========================================
 * id番号のPCを探す。居なければNULL
 *------------------------------------------
 */
dumb_ptr<map_session_data> map_id2sd(int id)
{
    // This is bogus.
    // However, there might be differences for de-auth'ed accounts.
// remove search from db, because:
// 1 - all players, npc, items and mob are in this db (to search, it's not speed, and search in session is more sure)
// 2 - DB seems not always correct. Sometimes, when a player disconnects, its id (account value) is not removed and structure
//     point to a memory area that is not more a session_data and value are incorrect (or out of available memory) -> crash
// replaced by searching in all session.
// by searching in session, we are sure that fd, session, and account exist.
/*
        dumb_ptr<block_list> bl;

        bl=numdb_search(id_db,id);
        if (bl && bl->bl_type==BL::PC)
                return (struct map_session_data*)bl;
        return NULL;
*/
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        if (session[i]->session_data)
        {
            map_session_data *sd = static_cast<map_session_data *>(session[i]->session_data.get());
            if (sd->bl_id == id)
                return dumb_ptr<map_session_data>(sd);
        }
    }

    return NULL;
}

/*==========================================
 * char_id番号の名前を探す
 *------------------------------------------
 */
CharName map_charid2nick(int id)
{
    struct charid2nick *p = charid_db.search(id);

    if (p == NULL)
        return CharName();
    if (p->req_id != 0)
        return CharName();
    return p->nick;
}

/*========================================*/
/* [Fate] Operations to iterate over active map sessions */

static
dumb_ptr<map_session_data> map_get_session(int i)
{
    if (i >= 0 && i < fd_max)
    {
        if (!session[i])
            return nullptr;
        map_session_data *d = static_cast<map_session_data *>(session[i]->session_data.get());
        if (d && d->state.auth)
            return dumb_ptr<map_session_data>(d);
    }

    return NULL;
}

static
dumb_ptr<map_session_data> map_get_session_forward(int start)
{
    for (int i = start; i < fd_max; i++)
    {
        dumb_ptr<map_session_data> d = map_get_session(i);
        if (d)
            return d;
    }

    return NULL;
}

static
dumb_ptr<map_session_data> map_get_session_backward(int start)
{
    int i;
    for (i = start; i >= 0; i--)
    {
        dumb_ptr<map_session_data> d = map_get_session(i);
        if (d)
            return d;
    }

    return NULL;
}

dumb_ptr<map_session_data> map_get_first_session(void)
{
    return map_get_session_forward(0);
}

dumb_ptr<map_session_data> map_get_next_session(dumb_ptr<map_session_data> d)
{
    return map_get_session_forward(d->fd + 1);
}

dumb_ptr<map_session_data> map_get_last_session(void)
{
    return map_get_session_backward(fd_max);
}

dumb_ptr<map_session_data> map_get_prev_session(dumb_ptr<map_session_data> d)
{
    return map_get_session_backward(d->fd - 1);
}

/*==========================================
 * Search session data from a nick name
 * (without sensitive case if necessary)
 * return map_session_data pointer or NULL
 *------------------------------------------
 */
dumb_ptr<map_session_data> map_nick2sd(CharName nick)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        map_session_data *pl_sd = static_cast<map_session_data *>(session[i]->session_data.get());
        if (pl_sd && pl_sd->state.auth)
        {
            {
                if (pl_sd->status.name == nick)
                    return dumb_ptr<map_session_data>(pl_sd);
            }
        }
    }
    return NULL;
}

/*==========================================
 * id番号の物を探す
 * 一時objectの場合は配列を引くのみ
 *------------------------------------------
 */
dumb_ptr<block_list> map_id2bl(int id)
{
    dumb_ptr<block_list> bl = NULL;
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
int map_addnpc(map_local *m, dumb_ptr<npc_data> nd)
{
    int i;
    if (!m)
        return -1;
    for (i = 0; i < m->npc_num && i < MAX_NPC_PER_MAP; i++)
        if (m->npc[i] == NULL)
            break;
    if (i == MAX_NPC_PER_MAP)
    {
        if (battle_config.error_log)
            PRINTF("too many NPCs in one map %s\n", m->name_);
        return -1;
    }
    if (i == m->npc_num)
    {
        m->npc_num++;
    }

    nullpo_ret(nd);

    m->npc[i] = nd;
    nd->n = i;
    id_db.put(nd->bl_id, nd);

    return i;
}

static
void map_removenpc(void)
{
    int n = 0;

    for (auto& mitp : maps_db)
    {
        if (!mitp.second->gat)
            continue;
        map_local *m = static_cast<map_local *>(mitp.second.get());
        for (int i = 0; i < m->npc_num && i < MAX_NPC_PER_MAP; i++)
        {
            if (m->npc[i] != NULL)
            {
                clif_clearchar(m->npc[i], BeingRemoveWhy::QUIT);
                map_delblock(m->npc[i]);
                id_db.put(m->npc[i]->bl_id, nullptr);
                if (m->npc[i]->npc_subtype == NpcSubtype::SCRIPT)
                {
//                    free(m->npc[i]->u.scr.script);
//                    free(m->npc[i]->u.scr.label_list);
                }
                m->npc[i].delete_();
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
map_local *map_mapname2mapid(MapName name)
{
    map_abstract *md = maps_db.get(name);
    if (md == NULL || md->gat == NULL)
        return nullptr;
    return static_cast<map_local *>(md);
}

/*==========================================
 * 他鯖map名からip,port変換
 *------------------------------------------
 */
int map_mapname2ipport(MapName name, IP4Address *ip, int *port)
{
    map_abstract *md = maps_db.get(name);
    if (md == NULL || md->gat)
        return -1;
    map_remote *mdos = static_cast<map_remote *>(md);
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
DIR map_calc_dir(dumb_ptr<block_list> src, int x, int y)
{
    DIR dir = DIR::S;
    int dx, dy;

    nullpo_retr(DIR::S, src);

    dx = x - src->bl_x;
    dy = y - src->bl_y;
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
MapCell map_getcell(map_local *m, int x, int y)
{
    if (x < 0 || x >= m->xs - 1 || y < 0 || y >= m->ys - 1)
        return MapCell::UNWALKABLE;
    return m->gat[x + y * m->xs];
}

/*==========================================
 * (m,x,y)の状態をtにする
 *------------------------------------------
 */
void map_setcell(map_local *m, int x, int y, MapCell t)
{
    if (x < 0 || x >= m->xs || y < 0 || y >= m->ys)
        return;
    m->gat[x + y * m->xs] = t;
}

/*==========================================
 * 他鯖管理のマップをdbに追加
 *------------------------------------------
 */
int map_setipport(MapName name, IP4Address ip, int port)
{
    map_abstract *md = maps_db.get(name);
    if (md == NULL)
    {
        // not exist -> add new data
        auto mdos = make_unique<map_remote>();
        mdos->name_ = name;
        mdos->gat = NULL;
        mdos->ip = ip;
        mdos->port = port;
        maps_db.put(mdos->name_, std::move(mdos));
    }
    else
    {
        if (md->gat)
        {
            // local -> check data
            if (ip != clif_getip() || port != clif_getport())
            {
                PRINTF("from char server : %s -> %s:%d\n",
                        name, ip, port);
                return 1;
            }
        }
        else
        {
            // update
            map_remote *mdos = static_cast<map_remote *>(md);
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
bool map_readmap(map_local *m, size_t num, MapName fn)
{
    // read & convert fn
    std::vector<uint8_t> gat_v = grfio_reads(fn);
    if (gat_v.empty())
        return false;
    size_t s = gat_v.size() - 4;

    int xs = m->xs = gat_v[0] | gat_v[1] << 8;
    int ys = m->ys = gat_v[2] | gat_v[3] << 8;
    PRINTF("\rLoading Maps [%zu/%zu]: %-30s  (%i, %i)",
            num, maps_db.size(),
            fn, xs, ys);
    fflush(stdout);

    assert (s == xs * ys);
    m->gat = make_unique<MapCell[]>(s);

    m->npc_num = 0;
    m->users = 0;
    really_memzero_this(&m->flag);
    if (battle_config.pk_mode)
        m->flag.pvp = 1;    // make all maps pvp for pk_mode [Valaris]
    MapCell *gat_m = reinterpret_cast<MapCell *>(&gat_v[4]);
    std::copy(gat_m, gat_m + s, &m->gat[0]);

    size_t bxs = (xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t bys = (ys + BLOCK_SIZE - 1) / BLOCK_SIZE;
    m->blocks.reset(bxs, bys);

    return true;
}

/*==========================================
 * 全てのmapデータを読み込む
 *------------------------------------------
 */
static
int map_readallmap(void)
{
    int maps_removed = 0;
    int num = 0;

    for (auto& mit : maps_db)
    {
        {
            {
                map_local *ml = static_cast<map_local *>(mit.second.get());
                if (!map_readmap(ml, num, mit.first))
                {
                    // Can't remove while implicitly iterating,
                    // and I don't feel like explicitly iterating.
                    //map_delmap(map[i].name);
                    maps_removed++;
                }
                else
                    num++;
            }
        }
    }

    PRINTF("\rMaps Loaded: %-65zu\n", maps_db.size());
    if (maps_removed)
    {
        PRINTF("Cowardly refusing to keep going after removing %d maps.\n",
                maps_removed);
        exit(1);
    }
    return 0;
}

/*==========================================
 * 読み込むmapを追加する
 *------------------------------------------
 */
static
void map_addmap(MapName mapname)
{
    if (mapname == "clear")
    {
        maps_db.clear();
        return;
    }

    auto newmap = make_unique<map_local>();
    newmap->name_ = mapname;
    // novice challenge: figure out why this is necessary,
    // and why the previous version worked
    MapName name = newmap->name_;
    maps_db.put(name, std::move(newmap));
}

/*==========================================
 * 読み込むmapを削除する
 *------------------------------------------
 */
void map_delmap(MapName mapname)
{
    if (mapname == "all")
    {
        maps_db.clear();
        return;
    }

    maps_db.put(mapname, nullptr);
}

constexpr int LOGFILE_SECONDS_PER_CHUNK_SHIFT = 10;

static
FILE *map_logfile = NULL;
static
FString map_logfile_name;
static
long map_logfile_index;

static
void map_close_logfile(void)
{
    if (map_logfile)
    {
        FString filename = STRPRINTF("%s.%ld", map_logfile_name, map_logfile_index);
        const char *args[] =
        {
            "gzip",
            "-f",
            filename.c_str(),
            NULL
        };
        char **argv = const_cast<char **>(args);

        fclose(map_logfile);
        map_logfile = NULL;

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

    FString filename_buf = STRPRINTF(
            "%s.%ld",
            map_logfile_name,
            map_logfile_index);
    map_logfile = fopen(filename_buf.c_str(), "w+");
    if (!map_logfile)
        perror(map_logfile_name.c_str());
}

static
void map_set_logfile(FString filename)
{
    struct timeval tv;

    map_logfile_name = std::move(filename);
    gettimeofday(&tv, NULL);

    map_start_logfile(tv.tv_sec);

    MAP_LOG("log-start v5");
}

void map_log(XString line)
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
int map_config_read(ZString cfgName)
{
    struct hostent *h = NULL;

    std::ifstream in(cfgName.c_str());
    if (!in.is_open())
    {
        PRINTF("Map configuration file not found at: %s\n", cfgName);
        exit(1);
    }

    FString line;
    while (io::getline(in, line))
    {
        XString w1;
        ZString w2;
        if (!split_key_value(line, &w1, &w2))
            continue;
        if (w1 == "userid")
        {
            AccountName name = stringish<AccountName>(w2);
            chrif_setuserid(name);
        }
        else if (w1 == "passwd")
        {
            AccountPass pass = stringish<AccountPass>(w2);
            chrif_setpasswd(pass);
        }
        else if (w1 == "char_ip")
        {
            h = gethostbyname(w2.c_str());
            IP4Address w2ip;
            if (h != NULL)
            {
                w2ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
                PRINTF("Character server IP address : %s -> %s\n",
                        w2, w2ip);
            }
            else
            {
                PRINTF("Bad IP value: %s\n", line);
                abort();
            }
            chrif_setip(w2ip);
        }
        else if (w1 == "char_port")
        {
            chrif_setport(atoi(w2.c_str()));
        }
        else if (w1 == "map_ip")
        {
            h = gethostbyname(w2.c_str());
            IP4Address w2ip;
            if (h != NULL)
            {
                w2ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
                PRINTF("Map server IP address : %s -> %s\n",
                        w2, w2ip);
            }
            else
            {
                PRINTF("Bad IP value: %s\n", line);
                abort();
            }
            clif_setip(w2ip);
        }
        else if (w1 == "map_port")
        {
            clif_setport(atoi(w2.c_str()));
        }
        else if (w1 == "map")
        {
            MapName name = VString<15>(w2);
            map_addmap(name);
        }
        else if (w1 == "delmap")
        {
            MapName name = VString<15>(w2);
            map_delmap(name);
        }
        else if (w1 == "npc")
        {
            npc_addsrcfile(w2);
        }
        else if (w1 == "delnpc")
        {
            npc_delsrcfile(w2);
        }
        else if (w1 == "autosave_time")
        {
            autosave_time = std::chrono::seconds(atoi(w2.c_str()));
            if (autosave_time <= interval_t::zero())
                autosave_time = DEFAULT_AUTOSAVE_INTERVAL;
        }
        else if (w1 == "motd_txt")
        {
            motd_txt = w2;
        }
        else if (w1 == "help_txt")
        {
            help_txt = w2;
        }
        else if (w1 == "mapreg_txt")
        {
            mapreg_txt = w2;
        }
        else if (w1 == "gm_log")
        {
            gm_log = std::move(w2);
        }
        else if (w1 == "log_file")
        {
            map_set_logfile(w2);
        }
        else if (w1 == "import")
        {
            map_config_read(w2);
        }
    }

    return 0;
}

static
void cleanup_sub(dumb_ptr<block_list> bl)
{
    nullpo_retv(bl);

    switch (bl->bl_type)
    {
        case BL::PC:
            map_delblock(bl);  // There is something better...
            break;
        case BL::NPC:
            npc_delete(bl->is_npc());
            break;
        case BL::MOB:
            mob_delete(bl->is_mob());
            break;
        case BL::ITEM:
            map_clearflooritem(bl->bl_id);
            break;
        case BL::SPELL:
            spell_free_invocation(bl->is_spell());
            break;
    }
}

/*==========================================
 * map鯖終了時処理
 *------------------------------------------
 */
void term_func(void)
{
    for (auto& mit : maps_db)
    {
        if (!mit.second->gat)
            continue;
        map_local *map_id = static_cast<map_local *>(mit.second.get());

        map_foreachinarea(cleanup_sub,
                map_id,
                0, 0,
                map_id->xs, map_id->ys,
                BL::NUL);
    }

    for (int i = 0; i < fd_max; i++)
        delete_session(i);

    map_removenpc();

    maps_db.clear();

    do_final_script();
    do_final_itemdb();
    do_final_storage();

    map_close_logfile();
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
int do_init(int argc, ZString *argv)
{
    int i;

    ZString MAP_CONF_NAME = "conf/map_athena.conf";
    ZString BATTLE_CONF_FILENAME = "conf/battle_athena.conf";
    ZString ATCOMMAND_CONF_FILENAME = "conf/atcommand_athena.conf";

    for (i = 1; i < argc; i++)
    {

        if (argv[i] == "--help" || argv[i] == "-h"
            || argv[i] == "-?" || argv[i] == "/?")
            map_helpscreen();
        else if (argv[i] == "--map_config")
            MAP_CONF_NAME = argv[++i];
        else if (argv[i] == "--battle_config")
            BATTLE_CONF_FILENAME = argv[++i];
        else if (argv[i] == "--atcommand_config")
            ATCOMMAND_CONF_FILENAME = argv[++i];
        else if (argv[i] == "--write-atcommand-config")
        {
            ZString filename = argv[++i];
            atcommand_config_write(filename);
            exit(0);
        }
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
            clif_getport());

    return 0;
}

int map_scriptcont(dumb_ptr<map_session_data> sd, int id)
{
    dumb_ptr<block_list> bl = map_id2bl(id);

    if (!bl)
        return 0;

    switch (bl->bl_type)
    {
        case BL::NPC:
            return npc_scriptcont(sd, id);
        case BL::SPELL:
            spell_execute_script(bl->is_spell());
            break;
    }

    return 0;
}
