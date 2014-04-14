#include "int_party.hpp"
//    int_party.cpp - Internal party handling.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <cstdlib>
#include <cstring>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"

#include "../mmo/extract.hpp"
#include "../mmo/mmo.hpp"
#include "../mmo/socket.hpp"

#include "char.hpp"
#include "inter.hpp"

#include "../poison.hpp"

AString party_txt = "save/party.txt"_s;

static
Map<int, struct party> party_db;
static
int party_newid = 100;

static
void mapif_party_broken(int party_id, int flag);
static
int party_check_empty(struct party *p);
static
void mapif_parse_PartyLeave(Session *s, int party_id, int account_id);

// パーティデータの文字列への変換
static
AString inter_party_tostr(struct party *p)
{
    MString str;
    str += STRPRINTF(
            "%d\t"
            "%s\t"
            "%d,%d\t"_fmt,
            p->party_id,
            p->name,
            p->exp, p->item);
    for (int i = 0; i < MAX_PARTY; i++)
    {
        struct party_member *m = &p->member[i];
        if (!m->account_id)
            continue;
        str += STRPRINTF(
                "%d,%d\t"
                "%s\t"_fmt,
                m->account_id, m->leader,
                m->name);
    }

    return AString(str);
}

static
bool extract(XString str, party *p)
{
    *p = party();

    // not compatible with the normal extract()ors since it has
    // a variable-size element that uses the same separator
    std::vector<XString> bits;
    if (!extract(str, vrec<'\t'>(&bits)))
        return false;
    auto begin = bits.begin();
    auto end = bits.end();
    if (begin == end || !extract(*begin, &p->party_id))
        return false;
    ++begin;
    if (begin == end || !extract(*begin, &p->name))
        return false;
    ++begin;
    if (begin == end || !extract(*begin, record<','>(&p->exp, &p->item)))
        return false;
    ++begin;

    for (int i = 0; begin != end && i < MAX_PARTY; ++i)
    {
        struct party_member *m = &p->member[i];

        if (begin == end || !extract(*begin, record<','>(&m->account_id, &m->leader)))
            return false;
        ++begin;
        if (begin == end || !extract(*begin, &m->name))
            return false;
        ++begin;
        if (!m->account_id)
            --i;
    }

    return true;
}

static
void party_check_deleted_init(struct party *p)
{
    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (!p->member[i].account_id)
            continue;
        const CharPair *c = search_character(p->member[i].name);
        if (!c || c->key.account_id != p->member[i].account_id)
        {
            CHAR_LOG("WARNING: deleting obsolete party member %d %s of %d %s\n"_fmt,
                    p->member[i].account_id, p->member[i].name,
                    p->party_id, p->name);
            PRINTF("WARNING: deleting obsolete party member %d %s of %d %s\n"_fmt,
                    p->member[i].account_id, p->member[i].name,
                    p->party_id, p->name);
            p->member[i] = party_member{};
        }
    }
}

// パーティデータのロード
void inter_party_init(void)
{
    io::ReadFile in(party_txt);
    if (!in.is_open())
        return;

    // TODO: convert to use char_id, and change to extract()
    AString line;
    int c = 0;
    while (in.getline(line))
    {
        int i, j = 0;
        if (SSCANF(line, "%d\t%%newid%%\n%n"_fmt, &i, &j) == 1 && j > 0
            && party_newid <= i)
        {
            party_newid = i;
            continue;
        }

        struct party p {};
        if (extract(line, &p) && p.party_id > 0)
        {
            if (p.party_id >= party_newid)
                party_newid = p.party_id + 1;
            party_check_deleted_init(&p);
            party_db.insert(p.party_id, p);
            party_check_empty(&p);
        }
        else
        {
            PRINTF("int_party: broken data [%s] line %d\n"_fmt, party_txt,
                    c + 1);
        }
        c++;
    }
}

// パーティーデータのセーブ用
static
void inter_party_save_sub(struct party *data, io::WriteFile& fp)
{
    AString line = inter_party_tostr(data);
    fp.put_line(line);
}

// パーティーデータのセーブ
int inter_party_save(void)
{
    io::WriteLock fp(party_txt);
    if (!fp.is_open())
    {
        PRINTF("int_party: cant write [%s] !!! data is lost !!!\n"_fmt,
                party_txt);
        return 1;
    }
    for (auto& pair : party_db)
        inter_party_save_sub(&pair.second, fp);

    return 0;
}

// パーティ名検索用
static
void search_partyname_sub(struct party *p, PartyName str, struct party **dst)
{
    if (p->name == str)
        *dst = p;
}

// パーティ名検索
static
struct party *search_partyname(PartyName str)
{
    struct party *p = NULL;
    for (auto& pair : party_db)
        search_partyname_sub(&pair.second, str, &p);

    return p;
}

// EXP公平分配できるかチェック
static
int party_check_exp_share(struct party *p)
{
    int i;
    int maxlv = 0, minlv = 0x7fffffff;

    for (i = 0; i < MAX_PARTY; i++)
    {
        int lv = p->member[i].lv;
        if (p->member[i].online)
        {
            if (lv < minlv)
                minlv = lv;
            if (maxlv < lv)
                maxlv = lv;
        }
    }

    return (maxlv == 0 || maxlv - minlv <= party_share_level);
}

// パーティが空かどうかチェック
int party_check_empty(struct party *p)
{
    int i;

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id > 0)
        {
            return 0;
        }
    }
    // 誰もいないので解散
    mapif_party_broken(p->party_id, 0);
    party_db.erase(p->party_id);

    return 1;
}

// キャラの競合がないかチェック用
static
void party_check_conflict_sub(struct party *p,
        int party_id, int account_id, CharName nick)
{
    int i;

    if (p->party_id == party_id)    // 本来の所属なので問題なし
        return;

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id == account_id
            && p->member[i].name == nick)
        {
            // 別のパーティに偽の所属データがあるので脱退
            PRINTF("int_party: party conflict! %d %d %d\n"_fmt, account_id,
                    party_id, p->party_id);
            mapif_parse_PartyLeave(nullptr, p->party_id, account_id);
        }
    }
}

// キャラの競合がないかチェック
static
void party_check_conflict(int party_id, int account_id, CharName nick)
{
    for (auto& pair : party_db)
        party_check_conflict_sub(&pair.second,
                party_id, account_id, nick);
}

//-------------------------------------------------------------------
// map serverへの通信

// パーティ作成可否
static
void mapif_party_created(Session *s, int account_id, struct party *p)
{
    WFIFOW(s, 0) = 0x3820;
    WFIFOL(s, 2) = account_id;
    if (p != NULL)
    {
        WFIFOB(s, 6) = 0;
        WFIFOL(s, 7) = p->party_id;
        WFIFO_STRING(s, 11, p->name, 24);
        PRINTF("int_party: created! %d %s\n"_fmt, p->party_id, p->name);
    }
    else
    {
        WFIFOB(s, 6) = 1;
        WFIFOL(s, 7) = 0;
        WFIFO_STRING(s, 11, "error"_s, 24);
    }
    WFIFOSET(s, 35);
}

// パーティ情報見つからず
static
void mapif_party_noinfo(Session *s, int party_id)
{
    WFIFOW(s, 0) = 0x3821;
    WFIFOW(s, 2) = 8;
    WFIFOL(s, 4) = party_id;
    WFIFOSET(s, 8);
    PRINTF("int_party: info not found %d\n"_fmt, party_id);
}

// パーティ情報まとめ送り
static
void mapif_party_info(Session *s, struct party *p)
{
    unsigned char buf[4 + sizeof(struct party)];

    WBUFW(buf, 0) = 0x3821;
    WBUF_STRUCT(buf, 4, *p);
    WBUFW(buf, 2) = 4 + sizeof(struct party);
    if (!s)
        mapif_sendall(buf, WBUFW(buf, 2));
    else
        mapif_send(s, buf, WBUFW(buf, 2));
}

// パーティメンバ追加可否
static
void mapif_party_memberadded(Session *s, int party_id, int account_id, int flag)
{
    WFIFOW(s, 0) = 0x3822;
    WFIFOL(s, 2) = party_id;
    WFIFOL(s, 6) = account_id;
    WFIFOB(s, 10) = flag;
    WFIFOSET(s, 11);
}

// パーティ設定変更通知
static
void mapif_party_optionchanged(Session *s, struct party *p, int account_id,
                               int flag)
{
    unsigned char buf[15];

    WBUFW(buf, 0) = 0x3823;
    WBUFL(buf, 2) = p->party_id;
    WBUFL(buf, 6) = account_id;
    WBUFW(buf, 10) = p->exp;
    WBUFW(buf, 12) = p->item;
    WBUFB(buf, 14) = flag;
    if (flag == 0)
        mapif_sendall(buf, 15);
    else
        mapif_send(s, buf, 15);
    PRINTF("int_party: option changed %d %d %d %d %d\n"_fmt, p->party_id,
            account_id, p->exp, p->item, flag);
}

// パーティ脱退通知
static
void mapif_party_leaved(int party_id, int account_id, CharName name)
{
    unsigned char buf[34];

    WBUFW(buf, 0) = 0x3824;
    WBUFL(buf, 2) = party_id;
    WBUFL(buf, 6) = account_id;
    WBUF_STRING(buf, 10, name.to__actual(), 24);
    mapif_sendall(buf, 34);
    PRINTF("int_party: party leaved %d %d %s\n"_fmt, party_id, account_id, name);
}

// パーティマップ更新通知
static
void mapif_party_membermoved(struct party *p, int idx)
{
    assert (idx < MAX_PARTY);
    unsigned char buf[29];

    WBUFW(buf, 0) = 0x3825;
    WBUFL(buf, 2) = p->party_id;
    WBUFL(buf, 6) = p->member[idx].account_id;
    WBUF_STRING(buf, 10, p->member[idx].map, 16);
    WBUFB(buf, 26) = p->member[idx].online;
    WBUFW(buf, 27) = p->member[idx].lv;
    mapif_sendall(buf, 29);
}

// パーティ解散通知
void mapif_party_broken(int party_id, int flag)
{
    unsigned char buf[7];
    WBUFW(buf, 0) = 0x3826;
    WBUFL(buf, 2) = party_id;
    WBUFB(buf, 6) = flag;
    mapif_sendall(buf, 7);
    PRINTF("int_party: broken %d\n"_fmt, party_id);
    CHAR_LOG("int_party: broken %d\n"_fmt, party_id);
}

// パーティ内発言
static
void mapif_party_message(int party_id, int account_id, XString mes)
{
    size_t len = mes.size() + 1;
    unsigned char buf[len + 12];

    WBUFW(buf, 0) = 0x3827;
    WBUFW(buf, 2) = len + 12;
    WBUFL(buf, 4) = party_id;
    WBUFL(buf, 8) = account_id;
    WBUF_STRING(buf, 12, mes, len);
    mapif_sendall(buf, len + 12);
}

//-------------------------------------------------------------------
// map serverからの通信

// パーティ
static
void mapif_parse_CreateParty(Session *s, int account_id, PartyName name, CharName nick,
        MapName map, int lv)
{
    {
        if (!name.is_print())
        {
            PRINTF("int_party: illegal party name [%s]\n"_fmt, name);
            mapif_party_created(s, account_id, NULL);
            return;
        }
    }

    if (search_partyname(name) != NULL)
    {
        PRINTF("int_party: same name party exists [%s]\n"_fmt, name);
        mapif_party_created(s, account_id, NULL);
        return;
    }
    struct party p {};
    p.party_id = party_newid++;
    p.name = name;
    p.exp = 0;
    p.item = 0;
    p.member[0].account_id = account_id;
    p.member[0].name = nick;
    p.member[0].map = map;
    p.member[0].leader = 1;
    p.member[0].online = 1;
    p.member[0].lv = lv;

    party_db.insert(p.party_id, p);

    mapif_party_created(s, account_id, &p);
    mapif_party_info(s, &p);
}

// パーティ情報要求
static
void mapif_parse_PartyInfo(Session *s, int party_id)
{
    struct party *p = party_db.search(party_id);
    if (p != NULL)
        mapif_party_info(s, p);
    else
        mapif_party_noinfo(s, party_id);
}

// パーティ追加要求
static
void mapif_parse_PartyAddMember(Session *s, int party_id, int account_id,
        CharName nick, MapName map, int lv)
{
    struct party *p = party_db.search(party_id);
    if (p == NULL)
    {
        mapif_party_memberadded(s, party_id, account_id, 1);
        return;
    }

    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id == 0)
        {
            int flag = 0;

            p->member[i].account_id = account_id;
            p->member[i].name = nick;
            p->member[i].map = map;
            p->member[i].leader = 0;
            p->member[i].online = 1;
            p->member[i].lv = lv;
            mapif_party_memberadded(s, party_id, account_id, 0);
            mapif_party_info(nullptr, p);

            if (p->exp > 0 && !party_check_exp_share(p))
            {
                p->exp = 0;
                flag = 0x01;
            }
            if (flag)
                mapif_party_optionchanged(s, p, 0, 0);
            return;
        }
    }
    mapif_party_memberadded(s, party_id, account_id, 1);
}

// パーティー設定変更要求
static
void mapif_parse_PartyChangeOption(Session *s, int party_id, int account_id,
        int exp, int item)
{
    struct party *p = party_db.search(party_id);
    if (p == NULL)
        return;

    p->exp = exp;
    int flag = 0;
    if (exp > 0 && !party_check_exp_share(p))
    {
        flag |= 0x01;
        p->exp = 0;
    }

    p->item = item;

    mapif_party_optionchanged(s, p, account_id, flag);
}

// パーティ脱退要求
void mapif_parse_PartyLeave(Session *, int party_id, int account_id)
{
    struct party *p = party_db.search(party_id);
    if (!p)
        return;
    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id != account_id)
            continue;
        mapif_party_leaved(party_id, account_id, p->member[i].name);

        p->member[i] = party_member{};
        if (party_check_empty(p) == 0)
            mapif_party_info(nullptr, p);   // まだ人がいるのでデータ送信
        return;
    }
}

// パーティマップ更新要求
static
void mapif_parse_PartyChangeMap(Session *s, int party_id, int account_id,
        MapName map, int online, int lv)
{
    struct party *p = party_db.search(party_id);
    if (p == NULL)
        return;

    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id != account_id)
            continue;
        int flag = 0;

        p->member[i].map = map;
        p->member[i].online = online;
        p->member[i].lv = lv;
        mapif_party_membermoved(p, i);

        if (p->exp > 0 && !party_check_exp_share(p))
        {
            p->exp = 0;
            flag = 1;
        }
        if (flag)
            mapif_party_optionchanged(s, p, 0, 0);
        return;
    }
}

// パーティ解散要求
static
void mapif_parse_BreakParty(Session *, int party_id)
{
    struct party *p = party_db.search(party_id);
    if (p == NULL)
        return;

    party_db.erase(party_id);
    mapif_party_broken(party_id, 0 /*unknown*/);
}

// パーティメッセージ送信
static
void mapif_parse_PartyMessage(Session *, int party_id, int account_id, XString mes)
{
    mapif_party_message(party_id, account_id, mes);
}

// パーティチェック要求
static
void mapif_parse_PartyCheck(Session *, int party_id, int account_id, CharName nick)
{
    party_check_conflict(party_id, account_id, nick);
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_party_parse_frommap(Session *ms)
{
    switch (RFIFOW(ms, 0))
    {
        case 0x3020:
        {
            int account = RFIFOL(ms, 2);
            PartyName name = stringish<PartyName>(RFIFO_STRING<24>(ms, 6));
            CharName nick = stringish<CharName>(RFIFO_STRING<24>(ms, 30));
            MapName map = RFIFO_STRING<16>(ms, 54);
            uint16_t lv = RFIFOW(ms, 70);
            mapif_parse_CreateParty(ms,
                    account,
                    name,
                    nick,
                    map,
                    lv);
        }
            break;
        case 0x3021:
        {
            int party_id = RFIFOL(ms, 2);
            mapif_parse_PartyInfo(ms, party_id);
        }
            break;
        case 0x3022:
        {
            int party_id = RFIFOL(ms, 2);
            int account_id = RFIFOL(ms, 6);
            CharName nick = stringish<CharName>(RFIFO_STRING<24>(ms, 10));
            MapName map = RFIFO_STRING<16>(ms, 34);
            uint16_t lv = RFIFOW(ms, 50);
            mapif_parse_PartyAddMember(ms,
                    party_id,
                    account_id,
                    nick,
                    map,
                    lv);
        }
            break;
        case 0x3023:
        {
            int party_id = RFIFOL(ms, 2);
            int account_id = RFIFOL(ms, 6);
            uint16_t exp = RFIFOW(ms, 10);
            uint16_t item = RFIFOW(ms, 12);
            mapif_parse_PartyChangeOption(ms,
                    party_id,
                    account_id,
                    exp,
                    item);
        }
            break;
        case 0x3024:
        {
            int party_id = RFIFOL(ms, 2);
            int account_id = RFIFOL(ms, 6);
            mapif_parse_PartyLeave(ms,
                    party_id,
                    account_id);
        }
            break;
        case 0x3025:
        {
            int party_id = RFIFOL(ms, 2);
            int account_id = RFIFOL(ms, 6);
            MapName map = RFIFO_STRING<16>(ms, 10);
            uint8_t online = RFIFOB(ms, 26);
            uint16_t lv = RFIFOW(ms, 27);
            mapif_parse_PartyChangeMap(ms,
                    party_id,
                    account_id,
                    map,
                    online,
                    lv);
        }
            break;
        case 0x3026:
        {
            int party_id = RFIFOL(ms, 2);
            mapif_parse_BreakParty(ms, party_id);
        }
            break;
        case 0x3027:
        {
            size_t len = RFIFOW(ms, 2) - 12;
            int party_id = RFIFOL(ms, 4);
            int account_id = RFIFOL(ms, 8);
            AString mes = RFIFO_STRING(ms, 12, len);
            mapif_parse_PartyMessage(ms,
                    party_id,
                    account_id,
                    mes);
        }
            break;
        case 0x3028:
        {
            int party_id = RFIFOL(ms, 2);
            int account_id = RFIFOL(ms, 6);
            CharName nick = stringish<CharName>(RFIFO_STRING<24>(ms, 10));
            mapif_parse_PartyCheck(ms,
                    party_id,
                    account_id,
                    nick);
        }
            break;
        default:
            return 0;
    }

    return 1;
}

// サーバーから脱退要求（キャラ削除用）
void inter_party_leave(int party_id, int account_id)
{
    mapif_parse_PartyLeave(nullptr, party_id, account_id);
}
