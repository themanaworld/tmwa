// $Id: party.c,v 1.2 2004/09/22 02:59:47 Akitasha Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "party.h"
#include "db.h"
#include "timer.h"
#include "socket.h"
#include "nullpo.h"
#include "malloc.h"
#include "pc.h"
#include "map.h"
#include "battle.h"
#include "intif.h"
#include "clif.h"
#include "skill.h"
#include "tmw.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define PARTY_SEND_XYHP_INVERVAL	1000    // 座標やＨＰ送信の間隔

static struct dbt *party_db;

int  party_send_xyhp_timer (int tid, unsigned int tick, int id, int data);
/*==========================================
 * 終了
 *------------------------------------------
 */
static int party_db_final (void *key, void *data, va_list ap)
{
    free (data);
    return 0;
}

void do_final_party (void)
{
    if (party_db)
        numdb_final (party_db, party_db_final);
}

// 初期化
void do_init_party (void)
{
    party_db = numdb_init ();
    add_timer_func_list (party_send_xyhp_timer, "party_send_xyhp_timer");
    add_timer_interval (gettick () + PARTY_SEND_XYHP_INVERVAL,
                        party_send_xyhp_timer, 0, 0,
                        PARTY_SEND_XYHP_INVERVAL);
}

// 検索
struct party *party_search (int party_id)
{
    return numdb_search (party_db, party_id);
}

int party_searchname_sub (void *key, void *data, va_list ap)
{
    struct party *p = (struct party *) data, **dst;
    char *str;
    str = va_arg (ap, char *);
    dst = va_arg (ap, struct party **);
    if (strcmpi (p->name, str) == 0)
        *dst = p;
    return 0;
}

// パーティ名検索
struct party *party_searchname (char *str)
{
    struct party *p = NULL;
    numdb_foreach (party_db, party_searchname_sub, str, &p);
    return p;
}

/* Process a party creation request. */
int party_create (struct map_session_data *sd, char *name)
{
    char pname[24];
    nullpo_retr (0, sd);

    strncpy (pname, name, 24);
    pname[23] = '\0';
    tmw_TrimStr (pname);

    /* The party name is empty/invalid. */
    if (!*pname)
        clif_party_created (sd, 1);

    /* Make sure the character isn't already in a party. */
    if (sd->status.party_id == 0)
        intif_create_party (sd, pname);
    else
        clif_party_created (sd, 2);

    return 0;
}

/* Relay the result of a party creation request. */
int party_created (int account_id, int fail, int party_id, char *name)
{
    struct map_session_data *sd;
    sd = map_id2sd (account_id);

    nullpo_retr (0, sd);

    /* The party name is valid and not already taken. */
    if (!fail)
    {
        struct party *p;
        sd->status.party_id = party_id;

        if ((p = numdb_search (party_db, party_id)) != NULL)
        {
            printf ("party_created(): ID already exists!\n");
            exit (1);
        }

        p = (struct party *) aCalloc (1, sizeof (struct party));
        p->party_id = party_id;
        memcpy (p->name, name, 24);
        numdb_insert (party_db, party_id, p);

        /* The party was created successfully. */
        clif_party_created (sd, 0);
    }

    else
        clif_party_created (sd, 1);

    return 0;
}

// 情報要求
int party_request_info (int party_id)
{
    return intif_request_partyinfo (party_id);
}

// 所属キャラの確認
int party_check_member (struct party *p)
{
    int  i;
    struct map_session_data *sd;

    nullpo_retr (0, p);

    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
        {
            if (sd->status.party_id == p->party_id)
            {
                int  j, f = 1;
                for (j = 0; j < MAX_PARTY; j++)
                {               // パーティにデータがあるか確認
                    if (p->member[j].account_id == sd->status.account_id)
                    {
                        if (strcmp (p->member[j].name, sd->status.name) == 0)
                            f = 0;  // データがある
                        else
                            p->member[j].sd = NULL; // 同垢別キャラだった
                    }
                }
                if (f)
                {
                    sd->status.party_id = 0;
                    if (battle_config.error_log)
                        printf ("party: check_member %d[%s] is not member\n",
                                sd->status.account_id, sd->status.name);
                }
            }
        }
    }
    return 0;
}

// 情報所得失敗（そのIDのキャラを全部未所属にする）
int party_recv_noinfo (int party_id)
{
    int  i;
    struct map_session_data *sd;
    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
        {
            if (sd->status.party_id == party_id)
                sd->status.party_id = 0;
        }
    }
    return 0;
}

// 情報所得
int party_recv_info (struct party *sp)
{
    struct party *p;
    int  i;

    nullpo_retr (0, sp);

    if ((p = numdb_search (party_db, sp->party_id)) == NULL)
    {
        p = (struct party *) aCalloc (1, sizeof (struct party));
        numdb_insert (party_db, sp->party_id, p);

        // 最初のロードなのでユーザーのチェックを行う
        party_check_member (sp);
    }
    memcpy (p, sp, sizeof (struct party));

    for (i = 0; i < MAX_PARTY; i++)
    {                           // sdの設定
        struct map_session_data *sd = map_id2sd (p->member[i].account_id);
        p->member[i].sd = (sd != NULL
                           && sd->status.party_id == p->party_id) ? sd : NULL;
    }

    clif_party_info (p, -1);

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 設定情報の送信
//      struct map_session_data *sd = map_id2sd(p->member[i].account_id);
        struct map_session_data *sd = p->member[i].sd;
        if (sd != NULL && sd->party_sended == 0)
        {
            clif_party_option (p, sd, 0x100);
            sd->party_sended = 1;
        }
    }

    return 0;
}

/* Process party invitation from sd to account_id. */
int party_invite (struct map_session_data *sd, int account_id)
{
    struct map_session_data *tsd = map_id2sd (account_id);
    struct party *p = party_search (sd->status.party_id);
    int  i;
    int  full = 1; /* Indicates whether or not there's room for one more. */

    nullpo_retr (0, sd);

    if (!tsd || !p || !tsd->fd)
        return 0;

    if (!battle_config.invite_request_check)
    {
        /* Disallow the invitation under these conditions. */
        if (tsd->guild_invite > 0 || tsd->trade_partner || tsd->npc_id
            || tsd->npc_shopid || pc_checkskill (tsd, NV_PARTY) < 1)
        {
            clif_party_inviteack (sd, tsd->status.name, 1);
            return 0;
        }
    }

    /* The target player is already in a party, or has a pending invitation. */
    if (tsd->status.party_id > 0 || tsd->party_invite > 0)
    {
        clif_party_inviteack (sd, tsd->status.name, 0);
        return 0;
    }

    for (i = 0; i < MAX_PARTY; i++)
    {
        /*
         * A character from the target account is already in the same party.
         * The response isn't strictly accurate, as they're separate
         * characters, but we're making do with what was already in place and
         * leaving this (mostly) alone for now.
         */
        if (p->member[i].account_id == account_id)
        {
            clif_party_inviteack (sd, tsd->status.name, 1);
            return 0;
        }

        if (!p->member[i].account_id)
            full = 0;
    }

    /* There isn't enough room for a new member. */
    if (full)
    {
        clif_party_inviteack (sd, tsd->status.name, 3);
        return 0;
    }

    /* Otherwise, relay the invitation to the target player. */
    tsd->party_invite = sd->status.party_id;
    tsd->party_invite_account = sd->status.account_id;

    clif_party_invite (sd, tsd);
    return 0;
}

/* Process response to party invitation. */
int party_reply_invite (struct map_session_data *sd, int account_id, int flag)
{
    nullpo_retr (0, sd);

    /* There is no pending invitation. */
    if (!sd->party_invite || !sd->party_invite_account)
        return 0;

    /*
     * Only one invitation can be pending, so these have to be the same. Since
     * the client continues to send the wrong ID, and it's already managed on
     * this side of things, the sent ID is being ignored.
     */
    account_id = sd->party_invite_account;

    /* The invitation was accepted. */
    if (flag == 1)
        intif_party_addmember (sd->party_invite, sd->status.account_id);
    /* The invitation was rejected. */
    else
    {
        /* This is the player who sent the invitation. */
        struct map_session_data *tsd = NULL;

        sd->party_invite = 0;
        sd->party_invite_account = 0;

        if ((tsd = map_id2sd (account_id)))
            clif_party_inviteack (tsd, sd->status.name, 1);
    }
    return 0;
}

// パーティが追加された
int party_member_added (int party_id, int account_id, int flag)
{
    struct map_session_data *sd = map_id2sd (account_id), *sd2;
    struct party *p = party_search (party_id);

    if (sd == NULL)
    {
        if (flag == 0)
        {
            if (battle_config.error_log)
                printf ("party: member added error %d is not online\n",
                        account_id);
            intif_party_leave (party_id, account_id);   // キャラ側に登録できなかったため脱退要求を出す
        }
        return 0;
    }
    sd2 = map_id2sd (sd->party_invite_account);
    sd->party_invite = 0;
    sd->party_invite_account = 0;

    if (p == NULL)
    {
        printf ("party_member_added: party %d not found.\n", party_id);
        intif_party_leave (party_id, account_id);
        return 0;
    }

    if (flag == 1)
    {                           // 失敗
        if (sd2 != NULL)
            clif_party_inviteack (sd2, sd->status.name, 0);
        return 0;
    }

    // 成功
    sd->party_sended = 0;
    sd->status.party_id = party_id;

    if (sd2 != NULL)
        clif_party_inviteack (sd2, sd->status.name, 2);

    // いちおう競合確認
    party_check_conflict (sd);

    party_send_xy_clear (p);

    return 0;
}

// パーティ除名要求
int party_removemember (struct map_session_data *sd, int account_id,
                        char *name)
{
    struct party *p;
    int  i;

    nullpo_retr (0, sd);

    if ((p = party_search (sd->status.party_id)) == NULL)
        return 0;

    for (i = 0; i < MAX_PARTY; i++)
    {                           // リーダーかどうかチェック
        if (p->member[i].account_id == sd->status.account_id)
            if (p->member[i].leader == 0)
                return 0;
    }

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 所属しているか調べる
        if (p->member[i].account_id == account_id)
        {
            intif_party_leave (p->party_id, account_id);
            return 0;
        }
    }
    return 0;
}

// パーティ脱退要求
int party_leave (struct map_session_data *sd)
{
    struct party *p;
    int  i;

    nullpo_retr (0, sd);

    if ((p = party_search (sd->status.party_id)) == NULL)
        return 0;

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 所属しているか
        if (p->member[i].account_id == sd->status.account_id)
        {
            intif_party_leave (p->party_id, sd->status.account_id);
            return 0;
        }
    }
    return 0;
}

// パーティメンバが脱退した
int party_member_leaved (int party_id, int account_id, char *name)
{
    struct map_session_data *sd = map_id2sd (account_id);
    struct party *p = party_search (party_id);
    if (p != NULL)
    {
        int  i;
        for (i = 0; i < MAX_PARTY; i++)
            if (p->member[i].account_id == account_id)
            {
                clif_party_leaved (p, sd, account_id, name, 0x00);
                p->member[i].account_id = 0;
                p->member[i].sd = NULL;
            }
    }
    if (sd != NULL && sd->status.party_id == party_id)
    {
        sd->status.party_id = 0;
        sd->party_sended = 0;
    }
    return 0;
}

// パーティ解散通知
int party_broken (int party_id)
{
    struct party *p;
    int  i;
    if ((p = party_search (party_id)) == NULL)
        return 0;

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].sd != NULL)
        {
            clif_party_leaved (p, p->member[i].sd,
                               p->member[i].account_id, p->member[i].name,
                               0x10);
            p->member[i].sd->status.party_id = 0;
            p->member[i].sd->party_sended = 0;
        }
    }
    numdb_erase (party_db, party_id);
    return 0;
}

// パーティの設定変更要求
int party_changeoption (struct map_session_data *sd, int exp, int item)
{
    struct party *p;

    nullpo_retr (0, sd);

    if (sd->status.party_id == 0
        || (p = party_search (sd->status.party_id)) == NULL)
        return 0;
    intif_party_changeoption (sd->status.party_id, sd->status.account_id, exp,
                              item);
    return 0;
}

// パーティの設定変更通知
int party_optionchanged (int party_id, int account_id, int exp, int item,
                         int flag)
{
    struct party *p;
    struct map_session_data *sd = map_id2sd (account_id);
    if ((p = party_search (party_id)) == NULL)
        return 0;

    if (!(flag & 0x01))
        p->exp = exp;
    if (!(flag & 0x10))
        p->item = item;
    clif_party_option (p, sd, flag);
    return 0;
}

// パーティメンバの移動通知
int party_recv_movemap (int party_id, int account_id, char *map, int online,
                        int lv)
{
    struct party *p;
    int  i;
    if ((p = party_search (party_id)) == NULL)
        return 0;
    for (i = 0; i < MAX_PARTY; i++)
    {
        struct party_member *m = &p->member[i];
        if (m == NULL)
        {
            printf ("party_recv_movemap nullpo?\n");
            return 0;
        }
        if (m->account_id == account_id)
        {
            memcpy (m->map, map, 16);
            m->online = online;
            m->lv = lv;
            break;
        }
    }
    if (i == MAX_PARTY)
    {
        if (battle_config.error_log)
            printf ("party: not found member %d on %d[%s]", account_id,
                    party_id, p->name);
        return 0;
    }

    for (i = 0; i < MAX_PARTY; i++)
    {                           // sd再設定
        struct map_session_data *sd = map_id2sd (p->member[i].account_id);
        p->member[i].sd = (sd != NULL
                           && sd->status.party_id == p->party_id) ? sd : NULL;
    }

    party_send_xy_clear (p);    // 座標再通知要請

    clif_party_info (p, -1);
    return 0;
}

// パーティメンバの移動
int party_send_movemap (struct map_session_data *sd)
{
    struct party *p;

    nullpo_retr (0, sd);

    if (sd->status.party_id == 0)
        return 0;
    intif_party_changemap (sd, 1);

    if (sd->party_sended != 0)  // もうパーティデータは送信済み
        return 0;

    // 競合確認 
    party_check_conflict (sd);

    // あるならパーティ情報送信
    if ((p = party_search (sd->status.party_id)) != NULL)
    {
        party_check_member (p); // 所属を確認する
        if (sd->status.party_id == p->party_id)
        {
            clif_party_info (p, sd->fd);
            clif_party_option (p, sd, 0x100);
            sd->party_sended = 1;
        }
    }

    return 0;
}

// パーティメンバのログアウト
int party_send_logout (struct map_session_data *sd)
{
    struct party *p;

    nullpo_retr (0, sd);

    if (sd->status.party_id > 0)
        intif_party_changemap (sd, 0);

    // sdが無効になるのでパーティ情報から削除
    if ((p = party_search (sd->status.party_id)) != NULL)
    {
        int  i;
        for (i = 0; i < MAX_PARTY; i++)
            if (p->member[i].sd == sd)
                p->member[i].sd = NULL;
    }

    return 0;
}

// パーティメッセージ送信
int party_send_message (struct map_session_data *sd, char *mes, int len)
{
    if (sd->status.party_id == 0)
        return 0;
    intif_party_message (sd->status.party_id, sd->status.account_id, mes,
                         len);
    return 0;
}

// パーティメッセージ受信
int party_recv_message (int party_id, int account_id, char *mes, int len)
{
    struct party *p;
    if ((p = party_search (party_id)) == NULL)
        return 0;
    clif_party_message (p, account_id, mes, len);
    return 0;
}

// パーティ競合確認
int party_check_conflict (struct map_session_data *sd)
{
    nullpo_retr (0, sd);

    intif_party_checkconflict (sd->status.party_id, sd->status.account_id,
                               sd->status.name);
    return 0;
}

// 位置やＨＰ通知用
int party_send_xyhp_timer_sub (void *key, void *data, va_list ap)
{
    struct party *p = (struct party *) data;
    int  i;

    nullpo_retr (0, p);

    for (i = 0; i < MAX_PARTY; i++)
    {
        struct map_session_data *sd;
        if ((sd = p->member[i].sd) != NULL)
        {
            // 座標通知
            if (sd->party_x != sd->bl.x || sd->party_y != sd->bl.y)
            {
                clif_party_xy (p, sd);
                sd->party_x = sd->bl.x;
                sd->party_y = sd->bl.y;
            }
            // ＨＰ通知
            if (sd->party_hp != sd->status.hp)
            {
                clif_party_hp (p, sd);
                sd->party_hp = sd->status.hp;
            }

        }
    }
    return 0;
}

// 位置やＨＰ通知
int party_send_xyhp_timer (int tid, unsigned int tick, int id, int data)
{
    numdb_foreach (party_db, party_send_xyhp_timer_sub, tick);
    return 0;
}

// 位置通知クリア
int party_send_xy_clear (struct party *p)
{
    int  i;

    nullpo_retr (0, p);

    for (i = 0; i < MAX_PARTY; i++)
    {
        struct map_session_data *sd;
        if ((sd = p->member[i].sd) != NULL)
        {
            sd->party_x = -1;
            sd->party_y = -1;
            sd->party_hp = -1;
        }
    }
    return 0;
}

// HP通知の必要性検査用（map_foreachinmoveareaから呼ばれる）
int party_send_hp_check (struct block_list *bl, va_list ap)
{
    int  party_id;
    int *flag;
    struct map_session_data *sd;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);
    nullpo_retr (0, sd = (struct map_session_data *) bl);

    party_id = va_arg (ap, int);
    flag = va_arg (ap, int *);

    if (sd->status.party_id == party_id)
    {
        *flag = 1;
        sd->party_hp = -1;
    }
    return 0;
}

// 経験値公平分配
int party_exp_share (struct party *p, int map, int base_exp, int job_exp)
{
    struct map_session_data *sd;
    int  i, c;

    nullpo_retr (0, p);

    for (i = c = 0; i < MAX_PARTY; i++)
        if ((sd = p->member[i].sd) != NULL && sd->bl.m == map)
            c++;
    if (c == 0)
        return 0;
    for (i = 0; i < MAX_PARTY; i++)
        if ((sd = p->member[i].sd) != NULL && sd->bl.m == map)
            pc_gainexp (sd, base_exp / c + 1, job_exp / c + 1);
    return 0;
}

// 同じマップのパーティメンバー全体に処理をかける
// type==0 同じマップ
//     !=0 画面内
void party_foreachsamemap (int (*func) (struct block_list *, va_list),
                           struct map_session_data *sd, int type, ...)
{
    struct party *p;
    va_list ap;
    int  i;
    int  x0, y0, x1, y1;
    struct block_list *list[MAX_PARTY];
    int  blockcount = 0;

    nullpo_retv (sd);

    if ((p = party_search (sd->status.party_id)) == NULL)
        return;

    x0 = sd->bl.x - AREA_SIZE;
    y0 = sd->bl.y - AREA_SIZE;
    x1 = sd->bl.x + AREA_SIZE;
    y1 = sd->bl.y + AREA_SIZE;

    va_start (ap, type);

    for (i = 0; i < MAX_PARTY; i++)
    {
        struct party_member *m = &p->member[i];
        if (m->sd != NULL)
        {
            if (sd->bl.m != m->sd->bl.m)
                continue;
            if (type != 0 &&
                (m->sd->bl.x < x0 || m->sd->bl.y < y0 ||
                 m->sd->bl.x > x1 || m->sd->bl.y > y1))
                continue;
            list[blockcount++] = &m->sd->bl;
        }
    }

    map_freeblock_lock ();      // メモリからの解放を禁止する

    for (i = 0; i < blockcount; i++)
        if (list[i]->prev)      // 有効かどうかチェック
            func (list[i], ap);

    map_freeblock_unlock ();    // 解放を許可する

    va_end (ap);
}
