#include "chat.hpp"

#include <cstdlib>
#include <cstring>

#include "../common/nullpo.hpp"

#include "map.hpp"
#include "npc.hpp"
#include "pc.hpp"

#include "../poison.hpp"

static
int chat_triggerevent(struct chat_data *cd);
static
int chat_npckickall(struct chat_data *cd);

/*==========================================
 * チャットルームから抜ける
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data *sd)
{
    struct chat_data *cd;
    int i, leavechar;

    nullpo_retr(1, sd);

    cd = (struct chat_data *) map_id2bl(sd->chatID);
    if (cd == NULL)
        return 1;

    for (i = 0, leavechar = -1; i < cd->users; i++)
    {
        if (cd->usersd[i] == sd)
        {
            leavechar = i;
            break;
        }
    }
    if (leavechar < 0)          // そのchatに所属していないらしい (バグ時のみ)
        return -1;

    cd->users--;
    pc_setchatid(sd, 0);

    if (cd->users == 0 && (*cd->owner)->type == BL::PC)
    {
        // 全員居なくなった&PCのチャットなので消す
        map_delobject(cd->bl.id, BL::CHAT); // freeまでしてくれる
    }
    else
    {
        for (i = leavechar; i < cd->users; i++)
            cd->usersd[i] = cd->usersd[i + 1];
        if (leavechar == 0 && (*cd->owner)->type == BL::PC)
        {
            // PCのチャットなので所有者が抜けたので位置変更
            cd->bl.x = cd->usersd[0]->bl.x;
            cd->bl.y = cd->usersd[0]->bl.y;
        }
    }

    return 0;
}

/*==========================================
 * npcチャットルーム作成
 *------------------------------------------
 */
int chat_createnpcchat(struct npc_data *nd, int limit, int pub, int trigger,
                        const char *title, int titlelen, const char *ev)
{
    struct chat_data *cd;

    nullpo_retr(1, nd);

    CREATE(cd, struct chat_data, 1);

    cd->limit = cd->trigger = limit;
    if (trigger > 0)
        cd->trigger = trigger;
    cd->pub = pub;
    cd->users = 0;
    memcpy(cd->pass, "", 8);
    if (titlelen >= sizeof(cd->title) - 1)
        titlelen = sizeof(cd->title) - 1;
    memcpy(cd->title, title, titlelen);
    cd->title[titlelen] = 0;

    cd->bl.m = nd->bl.m;
    cd->bl.x = nd->bl.x;
    cd->bl.y = nd->bl.y;
    cd->bl.type = BL::CHAT;
    cd->owner_ = (struct block_list *) nd;
    cd->owner = &cd->owner_;
    memcpy(cd->npc_event, ev, sizeof(cd->npc_event));

    cd->bl.id = map_addobject(&cd->bl);
    if (cd->bl.id == 0)
    {
        free(cd);
        return 0;
    }
    nd->chat_id = cd->bl.id;

    return 0;
}

/*==========================================
 * npcチャットルーム削除
 *------------------------------------------
 */
int chat_deletenpcchat(struct npc_data *nd)
{
    struct chat_data *cd;

    nullpo_ret(nd);
    cd = (struct chat_data *) map_id2bl(nd->chat_id);
    nullpo_ret(cd);

    chat_npckickall(cd);
    map_delobject(cd->bl.id, BL::CHAT); // freeまでしてくれる
    nd->chat_id = 0;

    return 0;
}

/*==========================================
 * 規定人数以上でイベントが定義されてるなら実行
 *------------------------------------------
 */
int chat_triggerevent(struct chat_data *cd)
{
    nullpo_ret(cd);

    if (cd->users >= cd->trigger && cd->npc_event[0])
        npc_event_do(cd->npc_event);
    return 0;
}

/*==========================================
 * イベントの有効化
 *------------------------------------------
 */
int chat_enableevent(struct chat_data *cd)
{
    nullpo_ret(cd);

    cd->trigger &= 0x7f;
    chat_triggerevent(cd);
    return 0;
}

/*==========================================
 * イベントの無効化
 *------------------------------------------
 */
int chat_disableevent(struct chat_data *cd)
{
    nullpo_ret(cd);

    cd->trigger |= 0x80;
    return 0;
}

/*==========================================
 * チャットルームから全員蹴り出す
 *------------------------------------------
 */
int chat_npckickall(struct chat_data *cd)
{
    nullpo_ret(cd);

    while (cd->users > 0)
    {
        chat_leavechat(cd->usersd[cd->users - 1]);
    }
    return 0;
}
