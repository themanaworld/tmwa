#include "party.hpp"

#include <cstring>

#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "map.hpp"
#include "pc.hpp"
#include "tmw.hpp"

#include "../poison.hpp"

// 座標やＨＰ送信の間隔
constexpr interval_t PARTY_SEND_XYHP_INVERVAL = std::chrono::seconds(1);

static
Map<int, struct party> party_db;

static
int party_check_conflict(struct map_session_data *sd);
static
void party_send_xyhp_timer(TimerData *tid, tick_t tick);

// 初期化
void do_init_party(void)
{
    Timer(gettick() + PARTY_SEND_XYHP_INVERVAL,
            party_send_xyhp_timer,
            PARTY_SEND_XYHP_INVERVAL
    ).detach();
}

// 検索
struct party *party_search(int party_id)
{
    return party_db.search(party_id);
}

static
void party_searchname_sub(struct party *p, const char *str, struct party **dst)
{
    if (strcasecmp(p->name, str) == 0)
        *dst = p;
}

// パーティ名検索
struct party *party_searchname(const char *str)
{
    struct party *p = NULL;
    for (auto& pair : party_db)
        party_searchname_sub(&pair.second, str, &p);
    return p;
}

/* Process a party creation request. */
int party_create(struct map_session_data *sd, const char *name)
{
    char pname[24];
    nullpo_ret(sd);

    strncpy(pname, name, 24);
    pname[23] = '\0';
    tmw_TrimStr(pname);

    /* The party name is empty/invalid. */
    if (!*pname)
        clif_party_created(sd, 1);

    /* Make sure the character isn't already in a party. */
    if (sd->status.party_id == 0)
        intif_create_party(sd, pname);
    else
        clif_party_created(sd, 2);

    return 0;
}

/* Relay the result of a party creation request. */
int party_created(int account_id, int fail, int party_id, const char *name)
{
    struct map_session_data *sd;
    sd = map_id2sd(account_id);

    nullpo_ret(sd);

    /* The party name is valid and not already taken. */
    if (!fail)
    {
        sd->status.party_id = party_id;

        struct party *p = party_db.search(party_id);
        if (p != NULL)
        {
            PRINTF("party_created(): ID already exists!\n");
            exit(1);
        }

        p = party_db.init(party_id);
        p->party_id = party_id;
        memcpy(p->name, name, 24);

        /* The party was created successfully. */
        clif_party_created(sd, 0);
    }

    else
        clif_party_created(sd, 1);

    return 0;
}

// 情報要求
int party_request_info(int party_id)
{
    return intif_request_partyinfo(party_id);
}

// 所属キャラの確認
static
int party_check_member(struct party *p)
{
    nullpo_ret(p);

    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        map_session_data *sd = static_cast<map_session_data *>(session[i]->session_data.get());
        if (sd && sd->state.auth)
        {
            if (sd->status.party_id == p->party_id)
            {
                int j, f = 1;
                for (j = 0; j < MAX_PARTY; j++)
                {               // パーティにデータがあるか確認
                    if (p->member[j].account_id == sd->status.account_id)
                    {
                        if (strcmp(p->member[j].name, sd->status.name) == 0)
                            f = 0;  // データがある
                        else
                        {
                            // I can prove it was already zeroed
                            // p->member[j].sd = NULL; // 同垢別キャラだった
                        }
                    }
                }
                if (f)
                {
                    sd->status.party_id = 0;
                    if (battle_config.error_log)
                        PRINTF("party: check_member %d[%s] is not member\n",
                                sd->status.account_id, sd->status.name);
                }
            }
        }
    }
    return 0;
}

// 情報所得失敗（そのIDのキャラを全部未所属にする）
int party_recv_noinfo(int party_id)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        map_session_data *sd = static_cast<map_session_data *>(session[i]->session_data.get());
        if (sd && sd->state.auth)
        {
            if (sd->status.party_id == party_id)
                sd->status.party_id = 0;
        }
    }
    return 0;
}

// 情報所得
int party_recv_info(const struct party *sp)
{
    int i;

    nullpo_ret(sp);

    struct party *p = party_db.search(sp->party_id);
    if (p == NULL)
    {
        p = party_db.init(sp->party_id);

        // 最初のロードなのでユーザーのチェックを行う
        *p = *sp;
        party_check_member(p);
    }
    else
        *p = *sp;

    for (i = 0; i < MAX_PARTY; i++)
    {                           // sdの設定
        struct map_session_data *sd = map_id2sd(p->member[i].account_id);
        p->member[i].sd = (sd != NULL
                           && sd->status.party_id == p->party_id) ? sd : NULL;
    }

    clif_party_info(p, -1);

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 設定情報の送信
//      struct map_session_data *sd = map_id2sd(p->member[i].account_id);
        struct map_session_data *sd = p->member[i].sd;
        if (sd != NULL && sd->party_sended == 0)
        {
            clif_party_option(p, sd, 0x100);
            sd->party_sended = 1;
        }
    }

    return 0;
}

/* Process party invitation from sd to account_id. */
int party_invite(struct map_session_data *sd, int account_id)
{
    struct map_session_data *tsd = map_id2sd(account_id);
    struct party *p = party_search(sd->status.party_id);
    int i;
    int full = 1; /* Indicates whether or not there's room for one more. */

    nullpo_ret(sd);

    if (!tsd || !p || !tsd->fd)
        return 0;

    if (!battle_config.invite_request_check)
    {
        /* Disallow the invitation under these conditions. */
        if (tsd->trade_partner || tsd->npc_id
            || tsd->npc_shopid || pc_checkskill(tsd, SkillID::NV_PARTY) < 1)
        {
            clif_party_inviteack(sd, tsd->status.name, 1);
            return 0;
        }
    }

    /* The target player is already in a party, or has a pending invitation. */
    if (tsd->status.party_id > 0 || tsd->party_invite > 0)
    {
        clif_party_inviteack(sd, tsd->status.name, 0);
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
            clif_party_inviteack(sd, tsd->status.name, 1);
            return 0;
        }

        if (!p->member[i].account_id)
            full = 0;
    }

    /* There isn't enough room for a new member. */
    if (full)
    {
        clif_party_inviteack(sd, tsd->status.name, 3);
        return 0;
    }

    /* Otherwise, relay the invitation to the target player. */
    tsd->party_invite = sd->status.party_id;
    tsd->party_invite_account = sd->status.account_id;

    clif_party_invite(sd, tsd);
    return 0;
}

/* Process response to party invitation. */
int party_reply_invite(struct map_session_data *sd, int account_id, int flag)
{
    nullpo_ret(sd);

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
        intif_party_addmember(sd->party_invite, sd->status.account_id);
    /* The invitation was rejected. */
    else
    {
        /* This is the player who sent the invitation. */
        struct map_session_data *tsd = NULL;

        sd->party_invite = 0;
        sd->party_invite_account = 0;

        if ((tsd = map_id2sd(account_id)))
            clif_party_inviteack(tsd, sd->status.name, 1);
    }
    return 0;
}

// パーティが追加された
int party_member_added(int party_id, int account_id, int flag)
{
    struct map_session_data *sd = map_id2sd(account_id), *sd2;
    struct party *p = party_search(party_id);

    if (sd == NULL)
    {
        if (flag == 0)
        {
            if (battle_config.error_log)
                PRINTF("party: member added error %d is not online\n",
                        account_id);
            intif_party_leave(party_id, account_id);   // キャラ側に登録できなかったため脱退要求を出す
        }
        return 0;
    }
    sd2 = map_id2sd(sd->party_invite_account);
    sd->party_invite = 0;
    sd->party_invite_account = 0;

    if (p == NULL)
    {
        PRINTF("party_member_added: party %d not found.\n", party_id);
        intif_party_leave(party_id, account_id);
        return 0;
    }

    if (flag == 1)
    {                           // 失敗
        if (sd2 != NULL)
            clif_party_inviteack(sd2, sd->status.name, 0);
        return 0;
    }

    // 成功
    sd->party_sended = 0;
    sd->status.party_id = party_id;

    if (sd2 != NULL)
        clif_party_inviteack(sd2, sd->status.name, 2);

    // いちおう競合確認
    party_check_conflict(sd);

    party_send_xy_clear(p);

    return 0;
}

// パーティ除名要求
int party_removemember(struct map_session_data *sd, int account_id, const char *)
{
    struct party *p;
    int i;

    nullpo_ret(sd);

    if ((p = party_search(sd->status.party_id)) == NULL)
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
            intif_party_leave(p->party_id, account_id);
            return 0;
        }
    }
    return 0;
}

// パーティ脱退要求
int party_leave(struct map_session_data *sd)
{
    struct party *p;
    int i;

    nullpo_ret(sd);

    if ((p = party_search(sd->status.party_id)) == NULL)
        return 0;

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 所属しているか
        if (p->member[i].account_id == sd->status.account_id)
        {
            intif_party_leave(p->party_id, sd->status.account_id);
            return 0;
        }
    }
    return 0;
}

// パーティメンバが脱退した
int party_member_leaved(int party_id, int account_id, const char *name)
{
    struct map_session_data *sd = map_id2sd(account_id);
    struct party *p = party_search(party_id);
    if (p != NULL)
    {
        int i;
        for (i = 0; i < MAX_PARTY; i++)
            if (p->member[i].account_id == account_id)
            {
                clif_party_leaved(p, sd, account_id, name, 0x00);
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
int party_broken(int party_id)
{
    struct party *p;
    int i;
    if ((p = party_search(party_id)) == NULL)
        return 0;

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].sd != NULL)
        {
            clif_party_leaved(p, p->member[i].sd,
                               p->member[i].account_id, p->member[i].name,
                               0x10);
            p->member[i].sd->status.party_id = 0;
            p->member[i].sd->party_sended = 0;
        }
    }
    party_db.erase(party_id);
    return 0;
}

// パーティの設定変更要求
int party_changeoption(struct map_session_data *sd, int exp, int item)
{
    struct party *p;

    nullpo_ret(sd);

    if (sd->status.party_id == 0
        || (p = party_search(sd->status.party_id)) == NULL)
        return 0;
    intif_party_changeoption(sd->status.party_id, sd->status.account_id, exp,
                              item);
    return 0;
}

// パーティの設定変更通知
int party_optionchanged(int party_id, int account_id, int exp, int item,
                         int flag)
{
    struct party *p;
    struct map_session_data *sd = map_id2sd(account_id);
    if ((p = party_search(party_id)) == NULL)
        return 0;

    if (!(flag & 0x01))
        p->exp = exp;
    if (!(flag & 0x10))
        p->item = item;
    clif_party_option(p, sd, flag);
    return 0;
}

// パーティメンバの移動通知
int party_recv_movemap(int party_id, int account_id, const char *mapname, int online,
                        int lv)
{
    struct party *p;
    int i;
    if ((p = party_search(party_id)) == NULL)
        return 0;
    for (i = 0; i < MAX_PARTY; i++)
    {
        struct party_member *m = &p->member[i];
        if (m == NULL)
        {
            PRINTF("party_recv_movemap nullpo?\n");
            return 0;
        }
        if (m->account_id == account_id)
        {
            memcpy(m->map, mapname, 16);
            m->online = online;
            m->lv = lv;
            break;
        }
    }
    if (i == MAX_PARTY)
    {
        if (battle_config.error_log)
            PRINTF("party: not found member %d on %d[%s]", account_id,
                    party_id, p->name);
        return 0;
    }

    for (i = 0; i < MAX_PARTY; i++)
    {                           // sd再設定
        struct map_session_data *sd = map_id2sd(p->member[i].account_id);
        p->member[i].sd = (sd != NULL
                           && sd->status.party_id == p->party_id) ? sd : NULL;
    }

    party_send_xy_clear(p);    // 座標再通知要請

    clif_party_info(p, -1);
    return 0;
}

// パーティメンバの移動
int party_send_movemap(struct map_session_data *sd)
{
    struct party *p;

    nullpo_ret(sd);

    if (sd->status.party_id == 0)
        return 0;
    intif_party_changemap(sd, 1);

    if (sd->party_sended != 0)  // もうパーティデータは送信済み
        return 0;

    // 競合確認
    party_check_conflict(sd);

    // あるならパーティ情報送信
    if ((p = party_search(sd->status.party_id)) != NULL)
    {
        party_check_member(p); // 所属を確認する
        if (sd->status.party_id == p->party_id)
        {
            clif_party_info(p, sd->fd);
            clif_party_option(p, sd, 0x100);
            sd->party_sended = 1;
        }
    }

    return 0;
}

// パーティメンバのログアウト
int party_send_logout(struct map_session_data *sd)
{
    struct party *p;

    nullpo_ret(sd);

    if (sd->status.party_id > 0)
        intif_party_changemap(sd, 0);

    // sdが無効になるのでパーティ情報から削除
    if ((p = party_search(sd->status.party_id)) != NULL)
    {
        int i;
        for (i = 0; i < MAX_PARTY; i++)
            if (p->member[i].sd == sd)
                p->member[i].sd = NULL;
    }

    return 0;
}

// パーティメッセージ送信
int party_send_message(struct map_session_data *sd, const char *mes, int len)
{
    if (sd->status.party_id == 0)
        return 0;
    intif_party_message(sd->status.party_id, sd->status.account_id, mes,
                         len);
    return 0;
}

// パーティメッセージ受信
int party_recv_message(int party_id, int account_id, const char *mes, int len)
{
    struct party *p;
    if ((p = party_search(party_id)) == NULL)
        return 0;
    clif_party_message(p, account_id, mes, len);
    return 0;
}

// パーティ競合確認
int party_check_conflict(struct map_session_data *sd)
{
    nullpo_ret(sd);

    intif_party_checkconflict(sd->status.party_id, sd->status.account_id,
                               sd->status.name);
    return 0;
}

// 位置やＨＰ通知用
static
void party_send_xyhp_timer_sub(struct party *p)
{
    int i;

    nullpo_retv(p);

    for (i = 0; i < MAX_PARTY; i++)
    {
        struct map_session_data *sd;
        if ((sd = p->member[i].sd) != NULL)
        {
            // 座標通知
            if (sd->party_x != sd->bl.x || sd->party_y != sd->bl.y)
            {
                clif_party_xy(p, sd);
                sd->party_x = sd->bl.x;
                sd->party_y = sd->bl.y;
            }
            // ＨＰ通知
            if (sd->party_hp != sd->status.hp)
            {
                clif_party_hp(p, sd);
                sd->party_hp = sd->status.hp;
            }

        }
    }
}

// 位置やＨＰ通知
void party_send_xyhp_timer(TimerData *, tick_t)
{
    for (auto& pair : party_db)
        party_send_xyhp_timer_sub(&pair.second);
}

// 位置通知クリア
int party_send_xy_clear(struct party *p)
{
    int i;

    nullpo_ret(p);

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
void party_send_hp_check(struct block_list *bl, int party_id, int *flag)
{
    struct map_session_data *sd;

    nullpo_retv(bl);
    sd = (struct map_session_data *) bl;

    if (sd->status.party_id == party_id)
    {
        *flag = 1;
        sd->party_hp = -1;
    }
}

// 経験値公平分配
int party_exp_share(struct party *p, int mapid, int base_exp, int job_exp)
{
    struct map_session_data *sd;
    int i, c;

    nullpo_ret(p);

    for (i = c = 0; i < MAX_PARTY; i++)
        if ((sd = p->member[i].sd) != NULL && sd->bl.m == mapid)
            c++;
    if (c == 0)
        return 0;
    for (i = 0; i < MAX_PARTY; i++)
        if ((sd = p->member[i].sd) != NULL && sd->bl.m == mapid)
            pc_gainexp_reason(sd, base_exp / c + 1, job_exp / c + 1,
            PC_GAINEXP_REASON::SHARING);
    return 0;
}

// 同じマップのパーティメンバー全体に処理をかける
// type==0 同じマップ
//     !=0 画面内
void party_foreachsamemap(std::function<void(struct block_list *)> func,
        struct map_session_data *sd, int type)
{
    struct party *p;
    int i;
    int x0, y0, x1, y1;
    struct block_list *list[MAX_PARTY];
    int blockcount = 0;

    nullpo_retv(sd);

    if ((p = party_search(sd->status.party_id)) == NULL)
        return;

    x0 = sd->bl.x - AREA_SIZE;
    y0 = sd->bl.y - AREA_SIZE;
    x1 = sd->bl.x + AREA_SIZE;
    y1 = sd->bl.y + AREA_SIZE;

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

    map_freeblock_lock();      // メモリからの解放を禁止する

    for (i = 0; i < blockcount; i++)
        if (list[i]->prev)      // 有効かどうかチェック
            func(list[i]);

    map_freeblock_unlock();    // 解放を許可する
}
