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

#include "../ints/udl.hpp"

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/write.hpp"

#include "../proto2/char-map.hpp"

#include "../mmo/ids.hpp"

#include "../high/extract_mmo.hpp"
#include "../high/mmo.hpp"

#include "../wire/packets.hpp"

#include "char.hpp"
#include "inter.hpp"

#include "../poison.hpp"


namespace tmwa
{
AString party_txt = "save/party.txt"_s;

static
Map<PartyId, PartyMost> party_db;
static
PartyId party_newid = wrap<PartyId>(100_u32);

static
void mapif_party_broken(PartyId party_id, int flag);
static
int party_check_empty(PartyPair p);
static
void mapif_parse_PartyLeave(Session *s, PartyId party_id, AccountId account_id);

// パーティデータの文字列への変換
static
AString inter_party_tostr(PartyPair p)
{
    MString str;
    str += STRPRINTF(
            "%d\t"
            "%s\t"
            "%d,%d\t"_fmt,
            p.party_id,
            p->name,
            p->exp, p->item);
    for (int i = 0; i < MAX_PARTY; i++)
    {
        PartyMember *m = &p->member[i];
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
bool impl_extract(XString str, PartyPair *pp)
{
    PartyPair& p = *pp;

    // not compatible with the normal extract()ors since it has
    // a variable-size element that uses the same separator
    std::vector<XString> bits;
    if (!extract(str, vrec<'\t'>(&bits)))
        return false;
    auto begin = bits.begin();
    auto end = bits.end();
    if (begin == end || !extract(*begin, &p.party_id))
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
        PartyMember *m = &p->member[i];

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
void party_check_deleted_init(PartyPair p)
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
                    p.party_id, p->name);
            PRINTF("WARNING: deleting obsolete party member %d %s of %d %s\n"_fmt,
                    p->member[i].account_id, p->member[i].name,
                    p.party_id, p->name);
            p->member[i] = PartyMember{};
        }
    }
}

// パーティデータのロード
void inter_party_init(void)
{
    io::ReadFile in(party_txt);
    if (!in.is_open())
        return;

    // TODO: convert to use char_id
    AString line;
    int c = 0;
    while (in.getline(line))
    {
        PartyId i;
        if (extract(line, record<'\t'>(&i, "%newid%"_s))
            && party_newid < i)
        {
            party_newid = i;
            continue;
        }

        PartyMost pm;
        PartyPair pp{PartyId(), borrow(pm)};
        if (extract(line, &pp) && pp.party_id)
        {
            if (party_newid < next(pp.party_id))
                party_newid = next(pp.party_id);
            party_check_deleted_init(pp);
            party_db.insert(pp.party_id, pm);
            // note: this is still referring to the noncanonical copy of
            // the PartyMost pointer. This is okay, though.
            party_check_empty(pp);
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
void inter_party_save_sub(PartyPair data, io::WriteFile& fp)
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
    {
        PartyPair tmp{pair.first, borrow(pair.second)};
        inter_party_save_sub(tmp, fp);
    }

    return 0;
}

// パーティ名検索用
static
void search_partyname_sub(PartyPair p, PartyName str, Borrowed<Option<PartyPair>> dst)
{
    if (p->name == str)
        *dst = Some(p);
}

// パーティ名検索
static
Option<PartyPair> search_partyname(PartyName str)
{
    Option<PartyPair> p = None;
    for (auto& pair : party_db)
    {
        PartyPair tmp{pair.first, borrow(pair.second)};
        search_partyname_sub(tmp, str, borrow(p));
    }

    return p;
}

// EXP公平分配できるかチェック
static
int party_check_exp_share(PartyPair p)
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
int party_check_empty(const PartyPair p)
{
    int i;

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id)
        {
            return 0;
        }
    }
    // 誰もいないので解散
    mapif_party_broken(p.party_id, 0);
    party_db.erase(p.party_id);

    return 1;
}

// キャラの競合がないかチェック用
static
void party_check_conflict_sub(PartyPair p,
        PartyId party_id, AccountId account_id, CharName nick)
{
    int i;

    if (p.party_id == party_id)    // 本来の所属なので問題なし
        return;

    for (i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id == account_id
            && p->member[i].name == nick)
        {
            // 別のパーティに偽の所属データがあるので脱退
            PRINTF("int_party: party conflict! %d %d %d\n"_fmt, account_id,
                    party_id, p.party_id);
            mapif_parse_PartyLeave(nullptr, p.party_id, account_id);
        }
    }
}

// キャラの競合がないかチェック
static
void party_check_conflict(PartyId party_id, AccountId account_id, CharName nick)
{
    for (auto& pair : party_db)
    {
        PartyPair tmp{pair.first, borrow(pair.second)};
        party_check_conflict_sub(tmp,
                party_id, account_id, nick);
    }
}

//-------------------------------------------------------------------
// map serverへの通信

// パーティ作成可否
static
void mapif_party_created(Session *s, AccountId account_id, Option<PartyPair> p_)
{
    Packet_Fixed<0x3820> fixed_20;
    fixed_20.account_id = account_id;
    OMATCH_BEGIN (p_)
    {
        OMATCH_CASE_SOME (p)
        {
            fixed_20.error = 0;
            fixed_20.party_id = p.party_id;
            fixed_20.party_name = p->name;
            PRINTF("int_party: created! %d %s\n"_fmt, p.party_id, p->name);
        }
        OMATCH_CASE_NONE ()
        {
            fixed_20.error = 1;
            fixed_20.party_id = PartyId();
            fixed_20.party_name = stringish<PartyName>("error"_s);
        }
    }
    OMATCH_END ();
    send_fpacket<0x3820, 35>(s, fixed_20);
}

// パーティ情報見つからず
static
void mapif_party_noinfo(Session *s, PartyId party_id)
{
    Packet_Head<0x3821> head_21;
    Packet_Option<0x3821> option_21;
    head_21.party_id = party_id;
    send_opacket<0x3821, 8, sizeof(NetPacket_Option<0x3821>)>(s, head_21, false, option_21);
    PRINTF("int_party: info not found %d\n"_fmt, party_id);
}

// パーティ情報まとめ送り
static
void mapif_party_info(Session *s, const PartyPair p)
{
    Packet_Head<0x3821> head_21;
    head_21.party_id = p.party_id;
    Packet_Option<0x3821> option_21;
    option_21.party_most = *p.party_most;
    if (!s)
    {
        for (Session *ss : iter_map_sessions())
        {
            send_opacket<0x3821, 8, sizeof(NetPacket_Option<0x3821>)>(ss, head_21, true, option_21);
        }
    }
    else
    {
        send_opacket<0x3821, 8, sizeof(NetPacket_Option<0x3821>)>(s, head_21, true, option_21);
    }
}

// パーティメンバ追加可否
static
void mapif_party_memberadded(Session *s, PartyId party_id, AccountId account_id, int flag)
{
    Packet_Fixed<0x3822> fixed_22;
    fixed_22.party_id = party_id;
    fixed_22.account_id = account_id;
    fixed_22.flag = flag;
    send_fpacket<0x3822, 11>(s, fixed_22);
}

// パーティ設定変更通知
static
void mapif_party_optionchanged(Session *s, PartyPair p, AccountId account_id,
                               int flag)
{
    Packet_Fixed<0x3823> fixed_23;
    fixed_23.party_id = p.party_id;
    fixed_23.account_id = account_id;
    fixed_23.exp = p->exp;
    fixed_23.item = p->item;
    fixed_23.flag = flag;
    if (flag == 0)
    {
        for (Session *ss : iter_map_sessions())
        {
            send_fpacket<0x3823, 15>(ss, fixed_23);
        }
    }
    else
    {
        send_fpacket<0x3823, 15>(s, fixed_23);
    }
    PRINTF("int_party: option changed %d %d %d %d %d\n"_fmt, p.party_id,
            account_id, p->exp, p->item, flag);
}

// パーティ脱退通知
static
void mapif_party_leaved(PartyId party_id, AccountId account_id, CharName name)
{
    Packet_Fixed<0x3824> fixed_24;
    fixed_24.party_id = party_id;
    fixed_24.account_id = account_id;
    fixed_24.char_name = name;
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x3824, 34>(ss, fixed_24);
    }

    PRINTF("int_party: party leaved %d %d %s\n"_fmt, party_id, account_id, name);
}

// パーティマップ更新通知
static
void mapif_party_membermoved(PartyPair p, int idx)
{
    assert (idx < MAX_PARTY);

    Packet_Fixed<0x3825> fixed_25;
    fixed_25.party_id = p.party_id;
    fixed_25.account_id = p->member[idx].account_id;
    fixed_25.map_name = p->member[idx].map;
    fixed_25.online = p->member[idx].online;
    fixed_25.level = p->member[idx].lv;
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x3825, 29>(ss, fixed_25);
    }
}

// パーティ解散通知
void mapif_party_broken(PartyId party_id, int flag)
{
    Packet_Fixed<0x3826> fixed_26;
    fixed_26.party_id = party_id;
    fixed_26.flag = flag;
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x3826, 7>(ss, fixed_26);
    }

    PRINTF("int_party: broken %d\n"_fmt, party_id);
    CHAR_LOG("int_party: broken %d\n"_fmt, party_id);
}

// パーティ内発言
static
void mapif_party_message(PartyId party_id, AccountId account_id, XString mes)
{
    Packet_Head<0x3827> head_27;
    XString repeat_27 = mes;
    head_27.party_id = party_id;
    head_27.account_id = account_id;
    for (Session *ss : iter_map_sessions())
    {
        send_vpacket<0x3827, 12, 1>(ss, head_27, repeat_27);
    }
}

//-------------------------------------------------------------------
// map serverからの通信

// パーティ
static
void mapif_parse_CreateParty(Session *s, AccountId account_id, PartyName name, CharName nick,
        MapName map, int lv)
{
    {
        if (!name.is_print())
        {
            PRINTF("int_party: illegal party name [%s]\n"_fmt, name);
            mapif_party_created(s, account_id, None);
            return;
        }
    }

    if (search_partyname(name).is_some())
    {
        PRINTF("int_party: same name party exists [%s]\n"_fmt, name);
        mapif_party_created(s, account_id, None);
        return;
    }

    PartyMost p {};
    party_newid = next(party_newid);
    PartyPair pp{party_newid, borrow(p)};
    p.name = name;
    p.exp = 0;
    p.item = 0;
    p.member[0].account_id = account_id;
    p.member[0].name = nick;
    p.member[0].map = map;
    p.member[0].leader = 1;
    p.member[0].online = 1;
    p.member[0].lv = lv;

    party_db.insert(pp.party_id, p);

    // pointer to noncanonical version
    mapif_party_created(s, account_id, Some(pp));
    mapif_party_info(s, pp);
}

// パーティ情報要求
static
void mapif_parse_PartyInfo(Session *s, PartyId party_id)
{
    Option<P<PartyMost>> maybe_party_most = party_db.search(party_id);
    OMATCH_BEGIN (maybe_party_most)
    {
        OMATCH_CASE_SOME (party_most)
        {
            mapif_party_info(s, PartyPair{party_id, party_most});
        }
        OMATCH_CASE_NONE ()
        {
            mapif_party_noinfo(s, party_id);
        }
    }
    OMATCH_END ();
}

// パーティ追加要求
static
void mapif_parse_PartyAddMember(Session *s, PartyId party_id, AccountId account_id,
        CharName nick, MapName map, int lv)
{
    Option<P<PartyMost>> maybe_party_most = party_db.search(party_id);
    P<PartyMost> party_most = TRY_UNWRAP(maybe_party_most,
            {
                mapif_party_memberadded(s, party_id, account_id, 1);
                return;
            });
    PartyPair p{party_id, party_most};

    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (!p->member[i].account_id)
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
                mapif_party_optionchanged(s, p, AccountId(), 0);
            return;
        }
    }
    mapif_party_memberadded(s, party_id, account_id, 1);
}

// パーティー設定変更要求
static
void mapif_parse_PartyChangeOption(Session *s, PartyId party_id, AccountId account_id,
        int exp, int item)
{
    PartyPair p{party_id, TRY_UNWRAP(party_db.search(party_id), return)};

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
void mapif_parse_PartyLeave(Session *, PartyId party_id, AccountId account_id)
{
    PartyPair p{party_id, TRY_UNWRAP(party_db.search(party_id), return)};

    for (int i = 0; i < MAX_PARTY; i++)
    {
        if (p->member[i].account_id != account_id)
            continue;
        mapif_party_leaved(party_id, account_id, p->member[i].name);

        p->member[i] = PartyMember{};
        if (party_check_empty(p) == 0)
            mapif_party_info(nullptr, p);   // まだ人がいるのでデータ送信
        return;
    }
}

// パーティマップ更新要求
static
void mapif_parse_PartyChangeMap(Session *s, PartyId party_id, AccountId account_id,
        MapName map, int online, int lv)
{
    PartyPair p{party_id, TRY_UNWRAP(party_db.search(party_id), return)};

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
            mapif_party_optionchanged(s, p, AccountId(), 0);
        return;
    }
}

// パーティメッセージ送信
static
void mapif_parse_PartyMessage(Session *, PartyId party_id, AccountId account_id, XString mes)
{
    mapif_party_message(party_id, account_id, mes);
}

// パーティチェック要求
static
void mapif_parse_PartyCheck(Session *, PartyId party_id, AccountId account_id, CharName nick)
{
    party_check_conflict(party_id, account_id, nick);
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
RecvResult inter_party_parse_frommap(Session *ms, uint16_t packet_id)
{
    RecvResult rv = RecvResult::Error;
    switch (packet_id)
    {
        case 0x3020:
        {
            Packet_Fixed<0x3020> fixed;
            rv = recv_fpacket<0x3020, 72>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            AccountId account = fixed.account_id;
            PartyName name = fixed.party_name;
            CharName nick = fixed.char_name;
            MapName map = fixed.map_name;
            uint16_t lv = fixed.level;
            mapif_parse_CreateParty(ms,
                    account,
                    name,
                    nick,
                    map,
                    lv);
            break;
        }
        case 0x3021:
        {
            Packet_Fixed<0x3021> fixed;
            rv = recv_fpacket<0x3021, 6>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = fixed.party_id;
            mapif_parse_PartyInfo(ms, party_id);
            break;
        }
        case 0x3022:
        {
            Packet_Fixed<0x3022> fixed;
            rv = recv_fpacket<0x3022, 52>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = fixed.party_id;
            AccountId account_id = fixed.account_id;
            CharName nick = fixed.char_name;
            MapName map = fixed.map_name;
            uint16_t lv = fixed.level;
            mapif_parse_PartyAddMember(ms,
                    party_id,
                    account_id,
                    nick,
                    map,
                    lv);
            break;
        }
        case 0x3023:
        {
            Packet_Fixed<0x3023> fixed;
            rv = recv_fpacket<0x3023, 14>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = fixed.party_id;
            AccountId account_id = fixed.account_id;
            uint16_t exp = fixed.exp;
            uint16_t item = fixed.item;
            mapif_parse_PartyChangeOption(ms,
                    party_id,
                    account_id,
                    exp,
                    item);
            break;
        }
        case 0x3024:
        {
            Packet_Fixed<0x3024> fixed;
            rv = recv_fpacket<0x3024, 10>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = fixed.party_id;
            AccountId account_id = fixed.account_id;
            mapif_parse_PartyLeave(ms,
                    party_id,
                    account_id);
            break;
        }
        case 0x3025:
        {
            Packet_Fixed<0x3025> fixed;
            rv = recv_fpacket<0x3025, 29>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = fixed.party_id;
            AccountId account_id = fixed.account_id;
            MapName map = fixed.map_name;
            uint8_t online = fixed.online;
            uint16_t lv = fixed.level;
            mapif_parse_PartyChangeMap(ms,
                    party_id,
                    account_id,
                    map,
                    online,
                    lv);
            break;
        }
        case 0x3027:
        {
            Packet_Head<0x3027> head;
            AString repeat;
            rv = recv_vpacket<0x3027, 12, 1>(ms, head, repeat);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = head.party_id;
            AccountId account_id = head.account_id;
            AString& mes = repeat;
            mapif_parse_PartyMessage(ms,
                    party_id,
                    account_id,
                    mes);
            break;
        }
        case 0x3028:
        {
            Packet_Fixed<0x3028> fixed;
            rv = recv_fpacket<0x3028, 34>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            PartyId party_id = fixed.party_id;
            AccountId account_id = fixed.account_id;
            CharName nick = fixed.char_name;
            mapif_parse_PartyCheck(ms,
                    party_id,
                    account_id,
                    nick);
            break;
        }
        default:
            return RecvResult::Error;
    }

    return rv;
}

// サーバーから脱退要求（キャラ削除用）
void inter_party_leave(PartyId party_id, AccountId account_id)
{
    mapif_parse_PartyLeave(nullptr, party_id, account_id);
}
} // namespace tmwa
