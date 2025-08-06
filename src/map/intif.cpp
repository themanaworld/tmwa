#include "intif.hpp"
//    intif.cpp - Network interface to the internal server.
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

#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/socket.hpp"

#include "../high/mmo.hpp"

#include "../proto2/char-map.hpp"

#include "../wire/packets.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "map.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "storage.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
//-----------------------------------------------------------------
// inter serverへの送信

// Message for all GMs on all map servers
void intif_GMmessage(XString mes)
{
    if (!char_session)
        return;

    send_packet_repeatonly<0x3000, 4, 1>(char_session, mes);
}

// The transmission of Wisp/Page to inter-server (player not found on this server)
void intif_wis_message(dumb_ptr<map_session_data> sd, CharName nick, ZString mes)
{
    nullpo_retv(sd);

    if (!char_session)
        return;

    Packet_Head<0x3001> head_01;
    head_01.from_char_name = sd->status_key.name;
    head_01.to_char_name = nick;
    send_vpacket<0x3001, 52, 1>(char_session, head_01, mes);

    if (battle_config.etc_log)
        PRINTF("intif_wis_message from %s to %s)\n"_fmt,
                sd->status_key.name, nick);
}

// The reply of Wisp/page
static
void intif_wis_replay(CharId id, int flag)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3002> fixed_02;
    fixed_02.char_id = id;
    fixed_02.flag = flag;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    send_fpacket<0x3002, 7>(char_session, fixed_02);

    if (battle_config.etc_log)
        PRINTF("intif_wis_replay: id: %d, flag:%d\n"_fmt, id, flag);
}

// The transmission of GM only Wisp/Page from server to inter-server
void intif_wis_message_to_gm(CharName Wisp_name, GmLevel min_gm_level, ZString mes)
{
    if (!char_session)
        return;

    Packet_Head<0x3003> head_03;
    head_03.char_name = Wisp_name;
    head_03.min_gm_level = min_gm_level;
    send_vpacket<0x3003, 30, 1>(char_session, head_03, mes);

    if (battle_config.etc_log)
        PRINTF("intif_wis_message_to_gm: from: '%s', min level: %d, message: '%s'.\n"_fmt,
                Wisp_name, min_gm_level, mes);
}

// アカウント変数送信 | Account Variable Submission
void intif_saveaccountreg(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);
    if (!char_session)
        return;

    assert (sd->status.account_reg_num <= ACCOUNT_REG_NUM);

    Packet_Head<0x3004> head_04;
    head_04.account_id = block_to_account(sd->bl_id);
    std::vector<Packet_Repeat<0x3004>> repeat_04(sd->status.account_reg_num);
    for (size_t j = 0; j < sd->status.account_reg_num; j++)
    {
        repeat_04[j].name = sd->status.account_reg[j].str;
        repeat_04[j].value = sd->status.account_reg[j].value;
    }
    send_vpacket<0x3004, 8, 36>(char_session, head_04, repeat_04);
}

// アカウント変数要求 | Account variable requests
void intif_request_accountreg(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    if (!char_session)
        return;

    Packet_Fixed<0x3005> fixed_05;
    fixed_05.account_id = block_to_account(sd->bl_id);
    send_fpacket<0x3005, 6>(char_session, fixed_05);
}

// 倉庫データ要求
void intif_request_storage(AccountId account_id)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3010> fixed_10;
    fixed_10.account_id = account_id;
    send_fpacket<0x3010, 6>(char_session, fixed_10);
}

// 倉庫データ送信
void intif_send_storage(Borrowed<Storage> stor)
{
    if (!char_session)
        return;

    Packet_Payload<0x3011> payload_11;
    payload_11.account_id = stor->account_id;
    payload_11.storage = *stor;
    send_ppacket<0x3011>(char_session, payload_11);
}

// パーティ作成要求
void intif_create_party(dumb_ptr<map_session_data> sd, PartyName name)
{
    nullpo_retv(sd);

    if (!char_session)
        return;

    Packet_Fixed<0x3020> fixed_20;
    fixed_20.account_id = sd->status_key.account_id;
    fixed_20.party_name = name;
    fixed_20.char_name = sd->status_key.name;
    fixed_20.map_name = sd->bl_m->name_;
    fixed_20.level = sd->status.base_level;
    send_fpacket<0x3020, 72>(char_session, fixed_20);
}

// パーティ情報要求
void intif_request_partyinfo(PartyId party_id)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3021> fixed_21;
    fixed_21.party_id = party_id;
    send_fpacket<0x3021, 6>(char_session, fixed_21);
}

// パーティ追加要求
void intif_party_addmember(PartyId party_id, AccountId account_id)
{
    if (!char_session)
        return;

    dumb_ptr<map_session_data> sd;
    sd = map_id2sd(account_to_block(account_id));
    if (sd != nullptr)
    {
        Packet_Fixed<0x3022> fixed_22;
        fixed_22.party_id = party_id;
        fixed_22.account_id = account_id;
        fixed_22.char_name = sd->status_key.name;
        fixed_22.map_name = sd->bl_m->name_;
        fixed_22.level = sd->status.base_level;
        send_fpacket<0x3022, 52>(char_session, fixed_22);
    }
}

// パーティ設定変更
void intif_party_changeoption(PartyId party_id, AccountId account_id, int exp, int item)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3023> fixed_23;
    fixed_23.party_id = party_id;
    fixed_23.account_id = account_id;
    fixed_23.exp = exp;
    fixed_23.item = item;
    send_fpacket<0x3023, 14>(char_session, fixed_23);
}

void intif_party_changeleader(PartyId party_id, AccountId account_id, int leader)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3026> fixed_26;
    fixed_26.party_id = party_id;
    fixed_26.account_id = account_id;
    fixed_26.leader = leader;
    send_fpacket<0x3026, 11>(char_session, fixed_26);
}

// パーティ脱退要求
void intif_party_leave(PartyId party_id, AccountId account_id)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3024> fixed_24;
    fixed_24.party_id = party_id;
    fixed_24.account_id = account_id;
    send_fpacket<0x3024, 10>(char_session, fixed_24);
}

// パーティ移動要求
void intif_party_changemap(dumb_ptr<map_session_data> sd, int online)
{
    if (!char_session)
        return;

    if (sd != nullptr)
    {
        Packet_Fixed<0x3025> fixed_25;
        fixed_25.party_id = sd->status.party_id;
        fixed_25.account_id = sd->status_key.account_id;
        fixed_25.map_name = sd->bl_m->name_;
        fixed_25.online = online;
        fixed_25.level = sd->status.base_level;
        send_fpacket<0x3025, 29>(char_session, fixed_25);
    }
}

// パーティ会話送信
void intif_party_message(PartyId party_id, AccountId account_id, XString mes)
{
    if (!char_session)
        return;

    Packet_Head<0x3027> head_27;
    head_27.party_id = party_id;
    head_27.account_id = account_id;
    send_vpacket<0x3027, 12, 1>(char_session, head_27, mes);
}

// パーティ競合チェック要求
void intif_party_checkconflict(PartyId party_id, AccountId account_id, CharName nick)
{
    if (!char_session)
        return;

    Packet_Fixed<0x3028> fixed_28;
    fixed_28.party_id = party_id;
    fixed_28.account_id = account_id;
    fixed_28.char_name = nick;
    send_fpacket<0x3028, 34>(char_session, fixed_28);
}

//-----------------------------------------------------------------
// Packets receive from inter server

// Wisp/Page reception
static
int intif_parse_WisMessage(Session *, const Packet_Head<0x3801>& head, AString& buf)
{
    // rewritten by [Yor]
    dumb_ptr<map_session_data> sd;

    CharName from = head.src_char_name;
    CharName to = head.dst_char_name;

    if (battle_config.etc_log)
    {
        PRINTF("intif_parse_wismessage: id: %d, from: %s, to: %s\n"_fmt,
                head.whisper_id,
                from,
                to);
    }
    sd = map_nick2sd(to); // Searching destination player
    if (sd != nullptr && sd->status_key.name == to)
    {
        // exactly same name (inter-server have checked the name before)
        {
            // if source player not found in ignore list
            {
                clif_wis_message(sd->sess, from, buf);
                intif_wis_replay(head.whisper_id, 0);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            }
        }
    }
    else
        intif_wis_replay(head.whisper_id, 1);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    return 0;
}

// Wisp/page transmission result reception
static
int intif_parse_WisEnd(Session *, const Packet_Fixed<0x3802>& fixed)
{
    dumb_ptr<map_session_data> sd;

    CharName name = fixed.sender_char_name;
    uint8_t flag = fixed.flag;
    if (battle_config.etc_log)
        // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        PRINTF("intif_parse_wisend: player: %s, flag: %d\n"_fmt,
                name, flag);
    sd = map_nick2sd(name);
    if (sd != nullptr)
        clif_wis_end(sd->sess, flag);

    return 0;
}

// Received wisp message from map-server via char-server for ALL gm
static
void mapif_parse_WisToGM(Session *, const Packet_Head<0x3803>& head, AString& message)
{
    // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
    GmLevel min_gm_level = head.min_gm_level;
    CharName Wisp_name = head.char_name;
    // information is sent to all online GM
    for (io::FD i : iter_fds())
    {
        Session *s2 = get_session(i);
        if (!s2)
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s2->session_data.get()));
        if (pl_sd && pl_sd->state.auth && !pl_sd->state.connect_new)
        {
            if (pc_isGM(pl_sd).satisfies(min_gm_level))
                clif_wis_message(s2, Wisp_name, message);
        }
    }
}

// アカウント変数通知
static
int intif_parse_AccountReg(Session *, const Packet_Head<0x3804>& head, const std::vector<Packet_Repeat<0x3804>>& repeat)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(head.account_id));
    if (sd == nullptr)
        return 1;

    size_t jlim = std::min(ACCOUNT_REG_NUM, repeat.size());
    for (size_t j = 0; j < jlim; j++)
    {
        sd->status.account_reg[j].str = repeat[j].name;
        sd->status.account_reg[j].value = repeat[j].value;
    }
    sd->status.account_reg_num = jlim;

    return 0;
}

// 倉庫データ受信
static
int intif_parse_LoadStorage(Session *, const Packet_Payload<0x3810>& payload)
{
    dumb_ptr<map_session_data> sd;

    sd = map_id2sd(account_to_block(payload.account_id));
    if (sd == nullptr)
    {
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: user not found %d\n"_fmt,
                    payload.account_id);
        return 1;
    }
    P<Storage> stor = account2storage(payload.account_id);
    if (stor->storage_status == 1)
    {                           // Already open.. lets ignore this update
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: storage received for a client already open (User %d:%d)\n"_fmt,
                    sd->status_key.account_id, sd->status_key.char_id);
        return 1;
    }
    if (stor->dirty)
    {                           // Already have storage, and it has been modified and not saved yet! Exploit! [Skotlex]
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: received storage for an already modified non-saved storage! (User %d:%d)\n"_fmt,
                    sd->status_key.account_id, sd->status_key.char_id);
        return 1;
    }

    if (battle_config.save_log)
        PRINTF("intif_openstorage: %d\n"_fmt, payload.account_id);
    *stor = payload.storage;
    stor->dirty = 0;
    stor->storage_status = 1;
    sd->state.storage_open = 1;
    clif_storageitemlist(sd, stor);
    clif_storageequiplist(sd, stor);
    clif_updatestorageamount(sd, stor);

    return 0;
}

// 倉庫データ送信成功
static
void intif_parse_SaveStorage(Session *, const Packet_Fixed<0x3811>& fixed)
{
    if (battle_config.save_log)
        PRINTF("intif_savestorage: done %d %d\n"_fmt, fixed.account_id,
                fixed.unknown);
    storage_storage_saved(fixed.account_id);
}

// パーティ作成可否
static
void intif_parse_PartyCreated(Session *, const Packet_Fixed<0x3820>& fixed)
{
    if (battle_config.etc_log)
        PRINTF("intif: party created\n"_fmt);
    AccountId account_id = fixed.account_id;
    int fail = fixed.error;
    PartyId party_id = fixed.party_id;
    PartyName name = fixed.party_name;
    party_created(account_id, fail, party_id, name);
}

// パーティ情報
static
void intif_parse_PartyInfo(Session *, const Packet_Head<0x3821>& head, bool has_opt, const Packet_Option<0x3821>& option)
{
    if (!has_opt)
    {
        if (battle_config.error_log)
            PRINTF("intif: party noinfo %d\n"_fmt, head.party_id);
        party_recv_noinfo(head.party_id);
        return;
    }

    PartyId pi = head.party_id;
    PartyMost pm = option.party_most;
    PartyPair pp{pi, borrow(pm)};
    party_recv_info(pp);
}

// パーティ追加通知
static
void intif_parse_PartyMemberAdded(Session *, const Packet_Fixed<0x3822>& fixed)
{
    if (battle_config.etc_log)
        PRINTF("intif: party member added %d %d %d\n"_fmt, fixed.party_id,
                fixed.account_id, fixed.flag);
    party_member_added(fixed.party_id, fixed.account_id, fixed.flag);
}

// パーティ設定変更通知
static
void intif_parse_PartyOptionChanged(Session *, const Packet_Fixed<0x3823>& fixed)
{
    party_optionchanged(fixed.party_id, fixed.account_id, fixed.exp,
            fixed.item, fixed.flag);
}

static
void intif_parse_PartyLeaderChanged(Session *, const Packet_Fixed<0x3828>& fixed)
{
    int i;
    PartyPair p = TRY_UNWRAP(party_search(fixed.party_id), return);

    for (i = 0; i < MAX_PARTY; i++)
    {
        PartyMember *m = &p->member[i];

        if (m->account_id == fixed.account_id)
        {
            dumb_ptr<map_session_data> sd = map_id2sd(wrap<BlockId>(unwrap<AccountId>(fixed.account_id)));
            m->leader = (fixed.leader > 0 ? 1 : 0);

            if (sd != nullptr)
            {
                AString msg = STRPRINTF("You are %s a leader of %s."_fmt,
                    fixed.leader > 0 ? "now"_s : "no longer"_s, p->name);
                clif_displaymessage(sd->sess, msg);
            }
            break;
        }
    }

    clif_party_info(p, nullptr);
}

// パーティ脱退通知
static
void intif_parse_PartyMemberLeaved(Session *, const Packet_Fixed<0x3824>& fixed)
{
    PartyId party_id = fixed.party_id;
    AccountId account_id = fixed.account_id;
    CharName name = fixed.char_name;
    if (battle_config.etc_log)
        PRINTF("intif: party member leaved %d %d %s\n"_fmt,
                party_id, account_id, name);
    party_member_leaved(party_id, account_id, name);
}

// パーティ解散通知
static
void intif_parse_PartyBroken(Session *, const Packet_Fixed<0x3826>& fixed)
{
    party_broken(fixed.party_id);
}

// パーティ移動通知
static
void intif_parse_PartyMove(Session *, const Packet_Fixed<0x3825>& fixed)
{
    PartyId party_id = fixed.party_id;
    AccountId account_id = fixed.account_id;
    MapName map = fixed.map_name;
    uint8_t online = fixed.online;
    uint16_t lv = fixed.level;
    party_recv_movemap(party_id, account_id, map, online, lv);
}

// パーティメッセージ
static
void intif_parse_PartyMessage(Session *, const Packet_Head<0x3827>& head, AString& buf)
{
    party_recv_message(head.party_id, head.account_id, buf);
}

//-----------------------------------------------------------------
// inter serverからの通信
// エラーがあれば0(false)を返すこと
// パケットが処理できれば1,パケット長が足りなければ2を返すこと
RecvResult intif_parse(Session *s, uint16_t packet_id)
{
    RecvResult rv;

    switch (packet_id)
    {
        case 0x3800:
        {
            AString mes;
            rv = recv_packet_repeatonly<0x3800, 4, 1>(s, mes);
            if (rv != RecvResult::Complete)
                return rv;

            clif_GMmessage(nullptr, mes, 0);
            break;
        }
        case 0x3801:
        {
            Packet_Head<0x3801> head;
            AString repeat;
            rv = recv_vpacket<0x3801, 56, 1>(s, head, repeat);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_WisMessage(s, head, repeat);
            break;
        }
        case 0x3802:
        {
            Packet_Fixed<0x3802> fixed;
            rv = recv_fpacket<0x3802, 27>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_WisEnd(s, fixed);
            break;
        }
        case 0x3803:
        {
            Packet_Head<0x3803> head;
            AString repeat;
            rv = recv_vpacket<0x3803, 30, 1>(s, head, repeat);
            if (rv != RecvResult::Complete)
                return rv;

            mapif_parse_WisToGM(s, head, repeat);
            break;
        }
        case 0x3804:
        {
            Packet_Head<0x3804> head;
            std::vector<Packet_Repeat<0x3804>> repeat;
            rv = recv_vpacket<0x3804, 8, 36>(s, head, repeat);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_AccountReg(s, head, repeat);
            break;
        }
        case 0x3810:
        {
            Packet_Payload<0x3810> payload;
            rv = recv_ppacket<0x3810>(s, payload);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_LoadStorage(s, payload);
            break;
        }
        case 0x3811:
        {
            Packet_Fixed<0x3811> fixed;
            rv = recv_fpacket<0x3811, 7>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_SaveStorage(s, fixed);
            break;
        }
        case 0x3820:
        {
            Packet_Fixed<0x3820> fixed;
            rv = recv_fpacket<0x3820, 35>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyCreated(s, fixed);
            break;
        }
        case 0x3821:
        {
            Packet_Head<0x3821> head;
            bool has_opt;
            Packet_Option<0x3821> option;
            rv = recv_opacket<0x3821, 8, sizeof(NetPacket_Option<0x3821>)>(s, head, &has_opt, option);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyInfo(s, head, has_opt, option);
            break;
        }
        case 0x3822:
        {
            Packet_Fixed<0x3822> fixed;
            rv = recv_fpacket<0x3822, 11>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyMemberAdded(s, fixed);
            break;
        }
        case 0x3823:
        {
            Packet_Fixed<0x3823> fixed;
            rv = recv_fpacket<0x3823, 15>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyOptionChanged(s, fixed);
            break;
        }
        case 0x3824:
        {
            Packet_Fixed<0x3824> fixed;
            rv = recv_fpacket<0x3824, 34>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyMemberLeaved(s, fixed);
            break;
        }
        case 0x3825:
        {
            Packet_Fixed<0x3825> fixed;
            rv = recv_fpacket<0x3825, 29>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyMove(s, fixed);
            break;
        }
        case 0x3826:
        {
            Packet_Fixed<0x3826> fixed;
            rv = recv_fpacket<0x3826, 7>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyBroken(s, fixed);
            break;
        }
        case 0x3827:
        {
            Packet_Head<0x3827> head;
            AString repeat;
            rv = recv_vpacket<0x3827, 12, 1>(s, head, repeat);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyMessage(s, head, repeat);
            break;
        }
        case 0x3828:
        {
            Packet_Fixed<0x3828> fixed;
            rv = recv_fpacket<0x3828, 11>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            intif_parse_PartyLeaderChanged(s, fixed);
            break;
        }
        case 0x3829:
        {
            Packet_Fixed<0x3829> fixed;
            rv = recv_fpacket<0x3829, 22>(s, fixed);
            if (rv != RecvResult::Complete)
                return rv;

            chrif_parse_preauth(s, fixed);
            break;
        }
        default:
            return RecvResult::Error;
    }
    return rv;
}
} // namespace map
} // namespace tmwa
