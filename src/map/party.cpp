#include "party.hpp"
//    party.cpp - Small groups of temporary allies.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 Freeyorp
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "../compat/nullpo.hpp"

#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "../mmo/ids.hpp"
#include "../high/mmo.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "map.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
// 座標やＨＰ送信の間隔
constexpr interval_t PARTY_SEND_XYHP_INVERVAL = 1_s;

static
Map<PartyId, PartyMost> party_db;

static
void party_check_conflict(dumb_ptr<map_session_data> sd);
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
Option<PartyPair> party_search(PartyId party_id)
{
    Option<P<PartyMost>> party_most_ = party_db.search(party_id);
    return party_most_.map([party_id](P<PartyMost> party_most)
            {
                return PartyPair{party_id, party_most};
            });
}

static
void party_searchname_sub(PartyPair p, PartyName str, Borrowed<Option<PartyPair>> dst)
{
    if (p->name == str)
        *dst = Some(p);
}

// パーティ名検索
Option<PartyPair> party_searchname(PartyName str)
{
    Option<PartyPair> p = None;
    for (auto& pair : party_db)
    {
        PartyPair tmp{pair.first, borrow(pair.second)};
        party_searchname_sub(tmp, str, borrow(p));
    }
    return p;
}

/* Process a party creation request. */
int party_create(dumb_ptr<map_session_data> sd, PartyName name)
{
    nullpo_retz(sd);

    name = stringish<PartyName>(name.strip());

    /* The party name is empty/invalid. */
    if (!name)
        clif_party_created(sd, 1);

    /* Make sure the character isn't already in a party. */
    if (!sd->status.party_id)
        intif_create_party(sd, name);
    else
        clif_party_created(sd, 2);

    return 0;
}

/* Relay the result of a party creation request. */
void party_created(AccountId account_id, int fail, PartyId party_id, PartyName name)
{
    dumb_ptr<map_session_data> sd;
    sd = map_id2sd(account_to_block(account_id));

    nullpo_retv(sd);

    /* The party name is valid and not already taken. */
    if (!fail)
    {
        sd->status.party_id = party_id;

        if (party_search(party_id).is_some())
        {
            PRINTF("party_created(): ID already exists!\n"_fmt);
            exit(1);
        }

        Borrowed<PartyMost> p = party_db.init(party_id);
        p->name = name;

        /* The party was created successfully. */
        clif_party_created(sd, 0);
    }

    else
        clif_party_created(sd, 1);
}

// 情報要求
void party_request_info(PartyId party_id)
{
    intif_request_partyinfo(party_id);
}

// 所属キャラの確認
static
int party_check_member(PartyPair p)
{
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        map_session_data *sd = static_cast<map_session_data *>(s->session_data.get());
        if (sd && sd->state.auth)
        {
            if (sd->status.party_id == p.party_id)
            {
                int j, f = 1;
                for (j = 0; j < MAX_PARTY; j++)
                {               // パーティにデータがあるか確認
                    if (p->member[j].account_id == sd->status_key.account_id)
                    {
                        if (p->member[j].name == sd->status_key.name)
                            f = 0;  // データがある
                        else
                        {
                            // I can prove it was already zeroed
                            // p->member[j].sd = nullptr; // 同垢別キャラだった
                        }
                    }
                }
                if (f)
                {
                    sd->status.party_id = PartyId();
                    if (battle_config.error_log)
                        PRINTF("party: check_member %d[%s] is not member\n"_fmt,
                                sd->status_key.account_id, sd->status_key.name);
                }
            }
        }
    }
    return 0;
}

// 情報所得失敗（そのIDのキャラを全部未所属にする）
int party_recv_noinfo(PartyId party_id)
{
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        map_session_data *sd = static_cast<map_session_data *>(s->session_data.get());
        if (sd && sd->state.auth)
        {
            if (sd->status.party_id == party_id)
                sd->status.party_id = PartyId();
        }
    }
    return 0;
}

static
PartyPair handle_info(const PartyPair sp)
{
    Option<PartyPair> p_ = party_search(sp.party_id);
    if OPTION_IS_SOME(p, p_)
    {
        *p.party_most = *sp.party_most;
        return p;
    }
    {
        PartyPair p{sp.party_id, party_db.init(sp.party_id)};

        // 最初のロードなのでユーザーのチェックを行う
        *p.party_most = *sp.party_most;
        party_check_member(p);
        return p;
    }
}

// 情報所得
int party_recv_info(const PartyPair sp)
{
    int i;

    PartyPair p = handle_info(sp);

    for (i = 0; i < MAX_PARTY; i++)
    {                           // sdの設定
        dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(p->member[i].account_id));
        p->member[i].sd = (sd != nullptr
                           && sd->status.party_id == p.party_id) ? sd.operator->() : nullptr;
    }

    clif_party_info(p, nullptr);

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 設定情報の送信
//      dumb_ptr<map_session_data> sd = map_id2sd(p->member[i].account_id);
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(p->member[i].sd);
        if (sd != nullptr && sd->party_sended == 0)
        {
            clif_party_option(p, sd, 0x100);
            sd->party_sended = 1;
        }
    }

    return 0;
}

/* Process party invitation from sd to account_id. */
int party_invite(dumb_ptr<map_session_data> sd, AccountId account_id)
{
    dumb_ptr<map_session_data> tsd = map_id2sd(account_to_block(account_id));
    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return 0);
    int i;
    int full = 1; /* Indicates whether or not there's room for one more. */

    nullpo_retz(sd);

    if (!tsd || !tsd->sess)
        return 0;

    if (!battle_config.invite_request_check)
    {
        /* Disallow the invitation under these conditions. */
        if (tsd->trade_partner || tsd->npc_id
            || tsd->npc_shopid || pc_checkskill(tsd, SkillID::NV_PARTY) < 1)
        {
            clif_party_inviteack(sd, tsd->status_key.name, 1);
            return 0;
        }
    }

    /* The target player is already in a party, or has a pending invitation. */
    if (tsd->status.party_id || tsd->party_invite)
    {
        clif_party_inviteack(sd, tsd->status_key.name, 0);
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
            clif_party_inviteack(sd, tsd->status_key.name, 1);
            return 0;
        }

        if (!p->member[i].account_id)
            full = 0;
    }

    /* There isn't enough room for a new member. */
    if (full)
    {
        clif_party_inviteack(sd, tsd->status_key.name, 3);
        return 0;
    }

    /* Otherwise, relay the invitation to the target player. */
    tsd->party_invite = sd->status.party_id;
    tsd->party_invite_account = sd->status_key.account_id;

    clif_party_invite(sd, tsd);
    return 0;
}

/* Process response to party invitation. */
int party_reply_invite(dumb_ptr<map_session_data> sd, AccountId account_id, int flag)
{
    nullpo_retz(sd);

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
        intif_party_addmember(sd->party_invite, sd->status_key.account_id);
    /* The invitation was rejected. */
    else
    {
        /* This is the player who sent the invitation. */
        dumb_ptr<map_session_data> tsd = nullptr;

        sd->party_invite = PartyId();
        sd->party_invite_account = AccountId();

        if ((tsd = map_id2sd(account_to_block(account_id))))
            clif_party_inviteack(tsd, sd->status_key.name, 1);
    }
    return 0;
}

// パーティが追加された
int party_member_added(PartyId party_id, AccountId account_id, int flag)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(account_id)), sd2;

    if (sd == nullptr)
    {
        if (flag == 0)
        {
            if (battle_config.error_log)
                PRINTF("party: member added error %d is not online\n"_fmt,
                        account_id);
            intif_party_leave(party_id, account_id);   // キャラ側に登録できなかったため脱退要求を出す
        }
        return 0;
    }
    sd2 = map_id2sd(account_to_block(sd->party_invite_account));
    sd->party_invite = PartyId();
    sd->party_invite_account = AccountId();

    PartyPair p = TRY_UNWRAP(party_search(party_id),
    {
        PRINTF("party_member_added: party %d not found.\n"_fmt, party_id);
        intif_party_leave(party_id, account_id);
        return 0;
    });

    if (flag == 1)
    {                           // 失敗
        if (sd2 != nullptr)
            clif_party_inviteack(sd2, sd->status_key.name, 0);
        return 0;
    }

    // 成功
    sd->party_sended = 0;
    sd->status.party_id = party_id;

    if (sd2 != nullptr)
        clif_party_inviteack(sd2, sd->status_key.name, 2);

    // いちおう競合確認
    party_check_conflict(sd);

    party_send_xy_clear(p);

    return 0;
}

// パーティ除名要求
int party_removemember(dumb_ptr<map_session_data> sd, AccountId account_id)
{
    int i;

    nullpo_retz(sd);

    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return 0);

    for (i = 0; i < MAX_PARTY; i++)
    {                           // リーダーかどうかチェック
        if (p->member[i].account_id == sd->status_key.account_id)
        {
            if (p->member[i].leader == 0)
                return 0;
        }
    }

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 所属しているか調べる
        if (p->member[i].account_id == account_id)
        {
            intif_party_leave(p.party_id, account_id);
            return 0;
        }
    }
    return 0;
}

// パーティ脱退要求
int party_leave(dumb_ptr<map_session_data> sd)
{
    int i;

    nullpo_retz(sd);

    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return 0);

    for (i = 0; i < MAX_PARTY; i++)
    {                           // 所属しているか
        if (p->member[i].account_id == sd->status_key.account_id)
        {
            intif_party_leave(p.party_id, sd->status_key.account_id);
            return 0;
        }
    }
    return 0;
}

// パーティメンバが脱退した
int party_member_leaved(PartyId party_id, AccountId account_id, CharName name)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(account_id));
    Option<PartyPair> p_ = party_search(party_id);
    if OPTION_IS_SOME(p, p_)
    {
        int i;
        for (i = 0; i < MAX_PARTY; i++)
            if (p->member[i].account_id == account_id)
            {
                clif_party_leaved(p, sd, account_id, name, 0x00);
                p->member[i].account_id = AccountId();
                p->member[i].sd = nullptr;
            }
    }
    if (sd != nullptr && sd->status.party_id == party_id)
    {
        sd->status.party_id = PartyId();
        sd->party_sended = 0;
    }
    return 0;
}

// パーティ解散通知
int party_broken(PartyId party_id)
{
    int i;
    PartyPair p = TRY_UNWRAP(party_search(party_id), return 0);

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].sd != nullptr)
        {
            clif_party_leaved(p, dumb_ptr<map_session_data>(p->member[i].sd),
                               p->member[i].account_id, p->member[i].name,
                               0x10);
            p->member[i].sd->status.party_id = PartyId();
            p->member[i].sd->party_sended = 0;
        }
    }
    party_db.erase(party_id);
    return 0;
}

// パーティの設定変更要求
int party_changeoption(dumb_ptr<map_session_data> sd, int exp, int item)
{
    nullpo_retz(sd);

    if (!sd->status.party_id)
        return 0;
    if (party_search(sd->status.party_id).is_none())
        return 0;
    intif_party_changeoption(sd->status.party_id, sd->status_key.account_id, exp,
                              item);
    return 0;
}

// パーティの設定変更通知
int party_optionchanged(PartyId party_id, AccountId account_id, int exp, int item,
                         int flag)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(account_id));
    PartyPair p = TRY_UNWRAP(party_search(party_id), return 0);

    if (!(flag & 0x01))
        p->exp = exp;
    if (!(flag & 0x10))
        p->item = item;
    clif_party_option(p, sd, flag);
    return 0;
}

// パーティメンバの移動通知
void party_recv_movemap(PartyId party_id, AccountId account_id, MapName mapname,
        int online, int lv)
{
    int i;
    PartyPair p = TRY_UNWRAP(party_search(party_id), return);
    for (i = 0; i < MAX_PARTY; i++)
    {
        PartyMember *m = &p->member[i];
        if (m == nullptr)
        {
            PRINTF("party_recv_movemap nullpo?\n"_fmt);
            return;
        }
        if (m->account_id == account_id)
        {
            m->map = mapname;
            m->online = online;
            m->lv = lv;
            break;
        }
    }
    if (i == MAX_PARTY)
    {
        if (battle_config.error_log)
            PRINTF("party: not found member %d on %d[%s]"_fmt, account_id,
                    party_id, p->name);
        return;
    }

    for (i = 0; i < MAX_PARTY; i++)
    {                           // sd再設定
        dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(p->member[i].account_id));
        p->member[i].sd = (sd != nullptr
                           && sd->status.party_id == p.party_id) ? sd.operator->() : nullptr;
    }

    party_send_xy_clear(p);    // 座標再通知要請

    clif_party_info(p, nullptr);
}

// パーティメンバの移動
int party_send_movemap(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (!sd->status.party_id)
        return 0;
    intif_party_changemap(sd, 1);

    if (sd->party_sended != 0)  // もうパーティデータは送信済み
        return 0;

    // 競合確認
    party_check_conflict(sd);

    // あるならパーティ情報送信
    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return 0);
    {
        party_check_member(p); // 所属を確認する
        if (sd->status.party_id == p.party_id)
        {
            clif_party_info(p, sd->sess);
            clif_party_option(p, sd, 0x100);
            sd->party_sended = 1;
        }
    }

    return 0;
}

// パーティメンバのログアウト
int party_send_logout(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->status.party_id)
        intif_party_changemap(sd, 0);

    // sdが無効になるのでパーティ情報から削除
    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return 0);
    {
        int i;
        for (i = 0; i < MAX_PARTY; i++)
            if (dumb_ptr<map_session_data>(p->member[i].sd) == sd)
                p->member[i].sd = nullptr;
    }

    return 0;
}

// パーティメッセージ送信
void party_send_message(dumb_ptr<map_session_data> sd, XString mes)
{
    if (!sd->status.party_id)
        return;
    intif_party_message(sd->status.party_id, sd->status_key.account_id, mes);
}

// パーティメッセージ受信
void party_recv_message(PartyId party_id, AccountId account_id, XString mes)
{
    PartyPair p = TRY_UNWRAP(party_search(party_id), return);
    clif_party_message(p, account_id, mes);
}

// パーティ競合確認
void party_check_conflict(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    intif_party_checkconflict(sd->status.party_id,
            sd->status_key.account_id, sd->status_key.name);
}

// 位置やＨＰ通知用
static
void party_send_xyhp_timer_sub(PartyPair p)
{
    int i;

    for (i = 0; i < MAX_PARTY; i++)
    {
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(p->member[i].sd);
        if (sd != nullptr)
        {
            // 座標通知
            if (sd->party_x != sd->bl_x || sd->party_y != sd->bl_y)
            {
                clif_party_xy(p, sd);
                sd->party_x = sd->bl_x;
                sd->party_y = sd->bl_y;
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
    {
        PartyPair tmp{pair.first, borrow(pair.second)};
        party_send_xyhp_timer_sub(tmp);
    }
}

// 位置通知クリア
void party_send_xy_clear(PartyPair p)
{
    int i;

    for (i = 0; i < MAX_PARTY; i++)
    {
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(p->member[i].sd);
        if (sd != nullptr)
        {
            sd->party_x = -1;
            sd->party_y = -1;
            sd->party_hp = -1;
        }
    }
}

// HP通知の必要性検査用（map_foreachinmoveareaから呼ばれる）
void party_send_hp_check(dumb_ptr<block_list> bl, PartyId party_id, int *flag)
{
    dumb_ptr<map_session_data> sd;

    nullpo_retv(bl);
    sd = bl->is_player();

    if (sd->status.party_id == party_id)
    {
        *flag = 1;
        sd->party_hp = -1;
    }
}

// 経験値公平分配
int party_exp_share(PartyPair p, Borrowed<map_local> mapid, int base_exp, int job_exp)
{
    dumb_ptr<map_session_data> sd;
    int i, c;

    for (i = c = 0; i < MAX_PARTY; i++)
    {
        sd = dumb_ptr<map_session_data>(p->member[i].sd);
        if (sd != nullptr && sd->bl_m == mapid)
            c++;
    }
    if (c == 0)
        return 0;
    for (i = 0; i < MAX_PARTY; i++)
    {
        sd = dumb_ptr<map_session_data>(p->member[i].sd);
        if (sd != nullptr && sd->bl_m == mapid)
            pc_gainexp_reason(sd, base_exp / c + 1, job_exp / c + 1,
            PC_GAINEXP_REASON::SHARING);
    }
    return 0;
}

// 同じマップのパーティメンバー全体に処理をかける
// type==0 同じマップ
//     !=0 画面内
void party_foreachsamemap(std::function<void(dumb_ptr<block_list>)> func,
        dumb_ptr<map_session_data> sd, int type)
{
    int i;
    int x0, y0, x1, y1;
    dumb_ptr<map_session_data> list[MAX_PARTY];
    int blockcount = 0;

    nullpo_retv(sd);

    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return);

    x0 = sd->bl_x - AREA_SIZE;
    y0 = sd->bl_y - AREA_SIZE;
    x1 = sd->bl_x + AREA_SIZE;
    y1 = sd->bl_y + AREA_SIZE;

    for (i = 0; i < MAX_PARTY; i++)
    {
        PartyMember *m = &p->member[i];
        if (m->sd != nullptr)
        {
            if (sd->bl_m != m->sd->bl_m)
                continue;
            if (type != 0 &&
                (m->sd->bl_x < x0 || m->sd->bl_y < y0 ||
                 m->sd->bl_x > x1 || m->sd->bl_y > y1))
                continue;
            list[blockcount++] = dumb_ptr<map_session_data>(m->sd);
        }
    }

    MapBlockLock lock;

    for (i = 0; i < blockcount; i++)
        if (list[i]->bl_prev)      // 有効かどうかチェック
            func(list[i]);
}
} // namespace tmwa
