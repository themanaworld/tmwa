#include "intif.hpp"

#include <cstdlib>
#include <cstring>

#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/socket.hpp"

#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "map.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "storage.hpp"

#include "../poison.hpp"

static
const int packet_len_table[] =
{
    -1, -1, 27, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    -1, 7, 0, 0, 0, 0, 0, 0, -1, 11, 0, 0, 0, 0, 0, 0,
    35, -1, 11, 15, 34, 29, 7, -1, 0, 0, 0, 0, 0, 0, 0, 0,
    10, -1, 15, 0, 79, 19, 7, -1, 0, -1, -1, -1, 14, 67, 186, -1,
    9, 9, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    11, -1, 7, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


//-----------------------------------------------------------------
// inter serverへの送信

// Message for all GMs on all map servers
void intif_GMmessage(XString mes)
{
    WFIFOW(char_session, 0) = 0x3000;
    size_t len = mes.size() + 1;
    WFIFOW(char_session, 2) = 4 + len;
    WFIFO_STRING(char_session, 4, mes, len);
    WFIFOSET(char_session, WFIFOW(char_session, 2));
}

// The transmission of Wisp/Page to inter-server (player not found on this server)
void intif_wis_message(dumb_ptr<map_session_data> sd, CharName nick, ZString mes)
{
    nullpo_retv(sd);

    size_t mes_len = mes.size() + 1;
    WFIFOW(char_session, 0) = 0x3001;
    WFIFOW(char_session, 2) = mes_len + 52;
    WFIFO_STRING(char_session, 4, sd->status_key.name.to__actual(), 24);
    WFIFO_STRING(char_session, 28, nick.to__actual(), 24);
    WFIFO_STRING(char_session, 52, mes, mes_len);
    WFIFOSET(char_session, WFIFOW(char_session, 2));

    if (battle_config.etc_log)
        PRINTF("intif_wis_message from %s to %s)\n",
                sd->status_key.name, nick);
}

// The reply of Wisp/page
static
void intif_wis_replay(int id, int flag)
{
    WFIFOW(char_session, 0) = 0x3002;
    WFIFOL(char_session, 2) = id;
    WFIFOB(char_session, 6) = flag;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    WFIFOSET(char_session, 7);

    if (battle_config.etc_log)
        PRINTF("intif_wis_replay: id: %d, flag:%d\n", id, flag);
}

// The transmission of GM only Wisp/Page from server to inter-server
void intif_wis_message_to_gm(CharName Wisp_name, int min_gm_level, ZString mes)
{
    size_t mes_len = mes.size() + 1;
    WFIFOW(char_session, 0) = 0x3003;
    WFIFOW(char_session, 2) = mes_len + 30;
    WFIFO_STRING(char_session, 4, Wisp_name.to__actual(), 24);
    WFIFOW(char_session, 28) = min_gm_level;
    WFIFO_STRING(char_session, 30, mes, mes_len);
    WFIFOSET(char_session, WFIFOW(char_session, 2));

    if (battle_config.etc_log)
        PRINTF("intif_wis_message_to_gm: from: '%s', min level: %d, message: '%s'.\n",
                Wisp_name, min_gm_level, mes);
}

// アカウント変数送信
void intif_saveaccountreg(dumb_ptr<map_session_data> sd)
{
    int j, p;

    nullpo_retv(sd);

    WFIFOW(char_session, 0) = 0x3004;
    WFIFOL(char_session, 4) = sd->bl_id;
    for (j = 0, p = 8; j < sd->status.account_reg_num; j++, p += 36)
    {
        WFIFO_STRING(char_session, p, sd->status.account_reg[j].str, 32);
        WFIFOL(char_session, p + 32) = sd->status.account_reg[j].value;
    }
    WFIFOW(char_session, 2) = p;
    WFIFOSET(char_session, p);
}

// アカウント変数要求
void intif_request_accountreg(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    WFIFOW(char_session, 0) = 0x3005;
    WFIFOL(char_session, 2) = sd->bl_id;
    WFIFOSET(char_session, 6);
}

// 倉庫データ要求
void intif_request_storage(int account_id)
{
    WFIFOW(char_session, 0) = 0x3010;
    WFIFOL(char_session, 2) = account_id;
    WFIFOSET(char_session, 6);
}

// 倉庫データ送信
void intif_send_storage(struct storage *stor)
{
    nullpo_retv(stor);
    WFIFOW(char_session, 0) = 0x3011;
    WFIFOW(char_session, 2) = sizeof(struct storage) + 8;
    WFIFOL(char_session, 4) = stor->account_id;
    WFIFO_STRUCT(char_session, 8, *stor);
    WFIFOSET(char_session, WFIFOW(char_session, 2));
}

// パーティ作成要求
void intif_create_party(dumb_ptr<map_session_data> sd, PartyName name)
{
    nullpo_retv(sd);

    WFIFOW(char_session, 0) = 0x3020;
    WFIFOL(char_session, 2) = sd->status_key.account_id;
    WFIFO_STRING(char_session, 6, name, 24);
    WFIFO_STRING(char_session, 30, sd->status_key.name.to__actual(), 24);
    WFIFO_STRING(char_session, 54, sd->bl_m->name_, 16);
    WFIFOW(char_session, 70) = sd->status.base_level;
    WFIFOSET(char_session, 72);
}

// パーティ情報要求
void intif_request_partyinfo(int party_id)
{
    WFIFOW(char_session, 0) = 0x3021;
    WFIFOL(char_session, 2) = party_id;
    WFIFOSET(char_session, 6);
}

// パーティ追加要求
void intif_party_addmember(int party_id, int account_id)
{
    dumb_ptr<map_session_data> sd;
    sd = map_id2sd(account_id);
    if (sd != NULL)
    {
        WFIFOW(char_session, 0) = 0x3022;
        WFIFOL(char_session, 2) = party_id;
        WFIFOL(char_session, 6) = account_id;
        WFIFO_STRING(char_session, 10, sd->status_key.name.to__actual(), 24);
        WFIFO_STRING(char_session, 34, sd->bl_m->name_, 16);
        WFIFOW(char_session, 50) = sd->status.base_level;
        WFIFOSET(char_session, 52);
    }
}

// パーティ設定変更
void intif_party_changeoption(int party_id, int account_id, int exp, int item)
{
    WFIFOW(char_session, 0) = 0x3023;
    WFIFOL(char_session, 2) = party_id;
    WFIFOL(char_session, 6) = account_id;
    WFIFOW(char_session, 10) = exp;
    WFIFOW(char_session, 12) = item;
    WFIFOSET(char_session, 14);
}

// パーティ脱退要求
void intif_party_leave(int party_id, int account_id)
{
    WFIFOW(char_session, 0) = 0x3024;
    WFIFOL(char_session, 2) = party_id;
    WFIFOL(char_session, 6) = account_id;
    WFIFOSET(char_session, 10);
}

// パーティ移動要求
void intif_party_changemap(dumb_ptr<map_session_data> sd, int online)
{
    if (sd != NULL)
    {
        WFIFOW(char_session, 0) = 0x3025;
        WFIFOL(char_session, 2) = sd->status.party_id;
        WFIFOL(char_session, 6) = sd->status_key.account_id;
        WFIFO_STRING(char_session, 10, sd->bl_m->name_, 16);
        WFIFOB(char_session, 26) = online;
        WFIFOW(char_session, 27) = sd->status.base_level;
        WFIFOSET(char_session, 29);
    }
}

// パーティ会話送信
void intif_party_message(int party_id, int account_id, XString mes)
{
    size_t len = mes.size() + 1;
    WFIFOW(char_session, 0) = 0x3027;
    WFIFOW(char_session, 2) = len + 12;
    WFIFOL(char_session, 4) = party_id;
    WFIFOL(char_session, 8) = account_id;
    WFIFO_STRING(char_session, 12, mes, len);
    WFIFOSET(char_session, len + 12);
}

// パーティ競合チェック要求
void intif_party_checkconflict(int party_id, int account_id, CharName nick)
{
    WFIFOW(char_session, 0) = 0x3028;
    WFIFOL(char_session, 2) = party_id;
    WFIFOL(char_session, 6) = account_id;
    WFIFO_STRING(char_session, 10, nick.to__actual(), 24);
    WFIFOSET(char_session, 34);
}

//-----------------------------------------------------------------
// Packets receive from inter server

// Wisp/Page reception
static
int intif_parse_WisMessage(Session *s)
{
    // rewritten by [Yor]
    dumb_ptr<map_session_data> sd;

    CharName from = stringish<CharName>(RFIFO_STRING<24>(s, 8));
    CharName to = stringish<CharName>(RFIFO_STRING<24>(s, 32));

    size_t len = RFIFOW(s, 2) - 56;
    AString buf = RFIFO_STRING(s, 56, len);

    if (battle_config.etc_log)
    {
        PRINTF("intif_parse_wismessage: id: %d, from: %s, to: %s\n",
             RFIFOL(s, 4),
             from,
             to);
    }
    sd = map_nick2sd(to); // Searching destination player
    if (sd != NULL && sd->status_key.name == to)
    {
        // exactly same name (inter-server have checked the name before)
        {
            // if source player not found in ignore list
            {
                clif_wis_message(sd->sess, from, buf);
                intif_wis_replay(RFIFOL(s, 4), 0);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            }
        }
    }
    else
        intif_wis_replay(RFIFOL(s, 4), 1);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    return 0;
}

// Wisp/page transmission result reception
static
int intif_parse_WisEnd(Session *s)
{
    dumb_ptr<map_session_data> sd;

    CharName name = stringish<CharName>(RFIFO_STRING<24>(s, 2));
    uint8_t flag = RFIFOB(s, 26);
    if (battle_config.etc_log)
        // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        PRINTF("intif_parse_wisend: player: %s, flag: %d\n",
                name, flag);
    sd = map_nick2sd(name);
    if (sd != NULL)
        clif_wis_end(sd->sess, flag);

    return 0;
}

// Received wisp message from map-server via char-server for ALL gm
static
void mapif_parse_WisToGM(Session *s)
{
    // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
    int min_gm_level, len;

    if (RFIFOW(s, 2) - 30 <= 0)
        return;

    len = RFIFOW(s, 2) - 30;

    min_gm_level = RFIFOW(s, 28);
    CharName Wisp_name = stringish<CharName>(RFIFO_STRING<24>(s, 4));
    AString message = RFIFO_STRING(s, 30, len);
    // information is sended to all online GM
    for (io::FD i : iter_fds())
    {
        Session *s2 = get_session(i);
        if (!s2)
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s2->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            if (pc_isGM(pl_sd) >= min_gm_level)
                clif_wis_message(s2, Wisp_name, message);
        }
    }
}

// アカウント変数通知
static
int intif_parse_AccountReg(Session *s)
{
    int j, p;
    dumb_ptr<map_session_data> sd = map_id2sd(RFIFOL(s, 4));
    if (sd == NULL)
        return 1;
    for (p = 8, j = 0; p < RFIFOW(s, 2) && j < ACCOUNT_REG_NUM;
         p += 36, j++)
    {
        sd->status.account_reg[j].str = stringish<VarName>(RFIFO_STRING<32>(s, p));
        sd->status.account_reg[j].value = RFIFOL(s, p + 32);
    }
    sd->status.account_reg_num = j;
//  PRINTF("intif: accountreg\n");

    return 0;
}

// 倉庫データ受信
static
int intif_parse_LoadStorage(Session *s)
{
    struct storage *stor;
    dumb_ptr<map_session_data> sd;

    sd = map_id2sd(RFIFOL(s, 4));
    if (sd == NULL)
    {
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: user not found %d\n",
                    RFIFOL(s, 4));
        return 1;
    }
    stor = account2storage(RFIFOL(s, 4));
    if (stor->storage_status == 1)
    {                           // Already open.. lets ignore this update
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: storage received for a client already open (User %d:%d)\n",
                 sd->status_key.account_id, sd->status_key.char_id);
        return 1;
    }
    if (stor->dirty)
    {                           // Already have storage, and it has been modified and not saved yet! Exploit! [Skotlex]
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: received storage for an already modified non-saved storage! (User %d:%d)\n",
                 sd->status_key.account_id, sd->status_key.char_id);
        return 1;
    }

    if (RFIFOW(s, 2) - 8 != sizeof(struct storage))
    {
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: data size error %d %zu\n",
                    RFIFOW(s, 2) - 8, sizeof(struct storage));
        return 1;
    }
    if (battle_config.save_log)
        PRINTF("intif_openstorage: %d\n", RFIFOL(s, 4));
    RFIFO_STRUCT(s, 8, *stor);
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
void intif_parse_SaveStorage(Session *s)
{
    if (battle_config.save_log)
        PRINTF("intif_savestorage: done %d %d\n", RFIFOL(s, 2),
                RFIFOB(s, 6));
    storage_storage_saved(RFIFOL(s, 2));
}

// パーティ作成可否
static
void intif_parse_PartyCreated(Session *s)
{
    if (battle_config.etc_log)
        PRINTF("intif: party created\n");
    int account_id = RFIFOL(s, 2);
    int fail = RFIFOB(s, 6);
    int party_id = RFIFOL(s, 7);
    PartyName name = stringish<PartyName>(RFIFO_STRING<24>(s, 11));
    party_created(account_id, fail, party_id, name);
}

// パーティ情報
static
void intif_parse_PartyInfo(Session *s)
{
    if (RFIFOW(s, 2) == 8)
    {
        if (battle_config.error_log)
            PRINTF("intif: party noinfo %d\n", RFIFOL(s, 4));
        party_recv_noinfo(RFIFOL(s, 4));
        return;
    }

//  PRINTF("intif: party info %d\n",RFIFOL(fd,4));
    if (RFIFOW(s, 2) != sizeof(struct party) + 4)
    {
        if (battle_config.error_log)
            PRINTF("intif: party info : data size error %d %d %zu\n",
                    RFIFOL(s, 4), RFIFOW(s, 2),
                    sizeof(struct party) + 4);
    }
    party p {};
    RFIFO_STRUCT(s, 4, p);
    party_recv_info(&p);
}

// パーティ追加通知
static
void intif_parse_PartyMemberAdded(Session *s)
{
    if (battle_config.etc_log)
        PRINTF("intif: party member added %d %d %d\n", RFIFOL(s, 2),
                RFIFOL(s, 6), RFIFOB(s, 10));
    party_member_added(RFIFOL(s, 2), RFIFOL(s, 6), RFIFOB(s, 10));
}

// パーティ設定変更通知
static
void intif_parse_PartyOptionChanged(Session *s)
{
    party_optionchanged(RFIFOL(s, 2), RFIFOL(s, 6), RFIFOW(s, 10),
                         RFIFOW(s, 12), RFIFOB(s, 14));
}

// パーティ脱退通知
static
void intif_parse_PartyMemberLeaved(Session *s)
{
    int party_id = RFIFOL(s, 2);
    int account_id = RFIFOL(s, 6);
    CharName name = stringish<CharName>(RFIFO_STRING<24>(s, 10));
    if (battle_config.etc_log)
        PRINTF("intif: party member leaved %d %d %s\n",
                party_id, account_id, name);
    party_member_leaved(party_id, account_id, name);
}

// パーティ解散通知
static
void intif_parse_PartyBroken(Session *s)
{
    party_broken(RFIFOL(s, 2));
}

// パーティ移動通知
static
void intif_parse_PartyMove(Session *s)
{
    int party_id = RFIFOL(s, 2);
    int account_id = RFIFOL(s, 6);
    MapName map = stringish<MapName>(RFIFO_STRING<16>(s, 10));
    uint8_t online = RFIFOB(s, 26);
    uint16_t lv = RFIFOW(s, 27);
    party_recv_movemap(party_id, account_id, map, online, lv);
}

// パーティメッセージ
static
void intif_parse_PartyMessage(Session *s)
{
    size_t len = RFIFOW(s, 2) - 12;
    AString buf = RFIFO_STRING(s, 12, len);
    party_recv_message(RFIFOL(s, 4), RFIFOL(s, 8), buf);
}

//-----------------------------------------------------------------
// inter serverからの通信
// エラーがあれば0(false)を返すこと
// パケットが処理できれば1,パケット長が足りなければ2を返すこと
int intif_parse(Session *s)
{
    int packet_len;
    int cmd = RFIFOW(s, 0);
    // パケットのID確認
    if (cmd < 0x3800
        || cmd >=
        0x3800 + (sizeof(packet_len_table) / sizeof(packet_len_table[0]))
        || packet_len_table[cmd - 0x3800] == 0)
    {
        return 0;
    }
    // パケットの長さ確認
    packet_len = packet_len_table[cmd - 0x3800];
    if (packet_len == -1)
    {
        if (RFIFOREST(s) < 4)
            return 2;
        packet_len = RFIFOW(s, 2);
    }
//  if(battle_config.etc_log)
//      PRINTF("intif_parse %d %x %d %d\n",fd,cmd,packet_len,RFIFOREST(fd));
    if (RFIFOREST(s) < packet_len)
    {
        return 2;
    }
    // 処理分岐
    switch (cmd)
    {
        case 0x3800:
        {
            AString mes = RFIFO_STRING(s, 4, packet_len - 4);
            clif_GMmessage(NULL, mes, 0);
        }
            break;
        case 0x3801:
            intif_parse_WisMessage(s);
            break;
        case 0x3802:
            intif_parse_WisEnd(s);
            break;
        case 0x3803:
            mapif_parse_WisToGM(s);
            break;
        case 0x3804:
            intif_parse_AccountReg(s);
            break;
        case 0x3810:
            intif_parse_LoadStorage(s);
            break;
        case 0x3811:
            intif_parse_SaveStorage(s);
            break;
        case 0x3820:
            intif_parse_PartyCreated(s);
            break;
        case 0x3821:
            intif_parse_PartyInfo(s);
            break;
        case 0x3822:
            intif_parse_PartyMemberAdded(s);
            break;
        case 0x3823:
            intif_parse_PartyOptionChanged(s);
            break;
        case 0x3824:
            intif_parse_PartyMemberLeaved(s);
            break;
        case 0x3825:
            intif_parse_PartyMove(s);
            break;
        case 0x3826:
            intif_parse_PartyBroken(s);
            break;
        case 0x3827:
            intif_parse_PartyMessage(s);
            break;
        default:
            if (battle_config.error_log)
                PRINTF("intif_parse : unknown packet %d %x\n", s,
                        RFIFOW(s, 0));
            return 0;
    }
    // パケット読み飛ばし
    RFIFOSKIP(s, packet_len);
    return 1;
}
