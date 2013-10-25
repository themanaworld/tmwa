#include "intif.hpp"

#include <cstdlib>
#include <cstring>

#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"

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
    WFIFOW(char_fd, 0) = 0x3000;
    size_t len = mes.size() + 1;
    WFIFOW(char_fd, 2) = 4 + len;
    WFIFO_STRING(char_fd, 4, mes, len);
    WFIFOSET(char_fd, WFIFOW(char_fd, 2));
}

// The transmission of Wisp/Page to inter-server (player not found on this server)
void intif_wis_message(dumb_ptr<map_session_data> sd, CharName nick, ZString mes)
{
    nullpo_retv(sd);

    size_t mes_len = mes.size() + 1;
    WFIFOW(char_fd, 0) = 0x3001;
    WFIFOW(char_fd, 2) = mes_len + 52;
    WFIFO_STRING(char_fd, 4, sd->status.name.to__actual(), 24);
    WFIFO_STRING(char_fd, 28, nick.to__actual(), 24);
    WFIFO_STRING(char_fd, 52, mes, mes_len);
    WFIFOSET(char_fd, WFIFOW(char_fd, 2));

    if (battle_config.etc_log)
        PRINTF("intif_wis_message from %s to %s)\n",
                sd->status.name, nick);
}

// The reply of Wisp/page
static
void intif_wis_replay(int id, int flag)
{
    WFIFOW(char_fd, 0) = 0x3002;
    WFIFOL(char_fd, 2) = id;
    WFIFOB(char_fd, 6) = flag;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    WFIFOSET(char_fd, 7);

    if (battle_config.etc_log)
        PRINTF("intif_wis_replay: id: %d, flag:%d\n", id, flag);
}

// The transmission of GM only Wisp/Page from server to inter-server
void intif_wis_message_to_gm(CharName Wisp_name, int min_gm_level, ZString mes)
{
    size_t mes_len = mes.size() + 1;
    WFIFOW(char_fd, 0) = 0x3003;
    WFIFOW(char_fd, 2) = mes_len + 30;
    WFIFO_STRING(char_fd, 4, Wisp_name.to__actual(), 24);
    WFIFOW(char_fd, 28) = min_gm_level;
    WFIFO_STRING(char_fd, 30, mes, mes_len);
    WFIFOSET(char_fd, WFIFOW(char_fd, 2));

    if (battle_config.etc_log)
        PRINTF("intif_wis_message_to_gm: from: '%s', min level: %d, message: '%s'.\n",
                Wisp_name, min_gm_level, mes);
}

// アカウント変数送信
void intif_saveaccountreg(dumb_ptr<map_session_data> sd)
{
    int j, p;

    nullpo_retv(sd);

    WFIFOW(char_fd, 0) = 0x3004;
    WFIFOL(char_fd, 4) = sd->bl_id;
    for (j = 0, p = 8; j < sd->status.account_reg_num; j++, p += 36)
    {
        WFIFO_STRING(char_fd, p, sd->status.account_reg[j].str, 32);
        WFIFOL(char_fd, p + 32) = sd->status.account_reg[j].value;
    }
    WFIFOW(char_fd, 2) = p;
    WFIFOSET(char_fd, p);
}

// アカウント変数要求
void intif_request_accountreg(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    WFIFOW(char_fd, 0) = 0x3005;
    WFIFOL(char_fd, 2) = sd->bl_id;
    WFIFOSET(char_fd, 6);
}

// 倉庫データ要求
void intif_request_storage(int account_id)
{
    WFIFOW(char_fd, 0) = 0x3010;
    WFIFOL(char_fd, 2) = account_id;
    WFIFOSET(char_fd, 6);
}

// 倉庫データ送信
void intif_send_storage(struct storage *stor)
{
    nullpo_retv(stor);
    WFIFOW(char_fd, 0) = 0x3011;
    WFIFOW(char_fd, 2) = sizeof(struct storage) + 8;
    WFIFOL(char_fd, 4) = stor->account_id;
    WFIFO_STRUCT(char_fd, 8, *stor);
    WFIFOSET(char_fd, WFIFOW(char_fd, 2));
}

// パーティ作成要求
void intif_create_party(dumb_ptr<map_session_data> sd, PartyName name)
{
    nullpo_retv(sd);

    WFIFOW(char_fd, 0) = 0x3020;
    WFIFOL(char_fd, 2) = sd->status.account_id;
    WFIFO_STRING(char_fd, 6, name, 24);
    WFIFO_STRING(char_fd, 30, sd->status.name.to__actual(), 24);
    WFIFO_STRING(char_fd, 54, sd->bl_m->name_, 16);
    WFIFOW(char_fd, 70) = sd->status.base_level;
    WFIFOSET(char_fd, 72);
}

// パーティ情報要求
void intif_request_partyinfo(int party_id)
{
    WFIFOW(char_fd, 0) = 0x3021;
    WFIFOL(char_fd, 2) = party_id;
    WFIFOSET(char_fd, 6);
}

// パーティ追加要求
void intif_party_addmember(int party_id, int account_id)
{
    dumb_ptr<map_session_data> sd;
    sd = map_id2sd(account_id);
    if (sd != NULL)
    {
        WFIFOW(char_fd, 0) = 0x3022;
        WFIFOL(char_fd, 2) = party_id;
        WFIFOL(char_fd, 6) = account_id;
        WFIFO_STRING(char_fd, 10, sd->status.name.to__actual(), 24);
        WFIFO_STRING(char_fd, 34, sd->bl_m->name_, 16);
        WFIFOW(char_fd, 50) = sd->status.base_level;
        WFIFOSET(char_fd, 52);
    }
}

// パーティ設定変更
void intif_party_changeoption(int party_id, int account_id, int exp, int item)
{
    WFIFOW(char_fd, 0) = 0x3023;
    WFIFOL(char_fd, 2) = party_id;
    WFIFOL(char_fd, 6) = account_id;
    WFIFOW(char_fd, 10) = exp;
    WFIFOW(char_fd, 12) = item;
    WFIFOSET(char_fd, 14);
}

// パーティ脱退要求
void intif_party_leave(int party_id, int account_id)
{
    WFIFOW(char_fd, 0) = 0x3024;
    WFIFOL(char_fd, 2) = party_id;
    WFIFOL(char_fd, 6) = account_id;
    WFIFOSET(char_fd, 10);
}

// パーティ移動要求
void intif_party_changemap(dumb_ptr<map_session_data> sd, int online)
{
    if (sd != NULL)
    {
        WFIFOW(char_fd, 0) = 0x3025;
        WFIFOL(char_fd, 2) = sd->status.party_id;
        WFIFOL(char_fd, 6) = sd->status.account_id;
        WFIFO_STRING(char_fd, 10, sd->bl_m->name_, 16);
        WFIFOB(char_fd, 26) = online;
        WFIFOW(char_fd, 27) = sd->status.base_level;
        WFIFOSET(char_fd, 29);
    }
}

// パーティ会話送信
void intif_party_message(int party_id, int account_id, XString mes)
{
    size_t len = mes.size() + 1;
    WFIFOW(char_fd, 0) = 0x3027;
    WFIFOW(char_fd, 2) = len + 12;
    WFIFOL(char_fd, 4) = party_id;
    WFIFOL(char_fd, 8) = account_id;
    WFIFO_STRING(char_fd, 12, mes, len);
    WFIFOSET(char_fd, len + 12);
}

// パーティ競合チェック要求
void intif_party_checkconflict(int party_id, int account_id, CharName nick)
{
    WFIFOW(char_fd, 0) = 0x3028;
    WFIFOL(char_fd, 2) = party_id;
    WFIFOL(char_fd, 6) = account_id;
    WFIFO_STRING(char_fd, 10, nick.to__actual(), 24);
    WFIFOSET(char_fd, 34);
}

//-----------------------------------------------------------------
// Packets receive from inter server

// Wisp/Page reception
static
int intif_parse_WisMessage(int fd)
{
    // rewritten by [Yor]
    dumb_ptr<map_session_data> sd;

    CharName from = stringish<CharName>(RFIFO_STRING<24>(fd, 8));
    CharName to = stringish<CharName>(RFIFO_STRING<24>(fd, 32));

    size_t len = RFIFOW(fd, 2) - 56;
    FString buf = RFIFO_STRING(fd, 56, len);

    if (battle_config.etc_log)
    {
        PRINTF("intif_parse_wismessage: id: %d, from: %s, to: %s\n",
             RFIFOL(fd, 4),
             from,
             to);
    }
    sd = map_nick2sd(to); // Searching destination player
    if (sd != NULL && sd->status.name == to)
    {
        // exactly same name (inter-server have checked the name before)
        {
            // if source player not found in ignore list
            {
                clif_wis_message(sd->fd, from, buf);
                intif_wis_replay(RFIFOL(fd, 4), 0);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            }
        }
    }
    else
        intif_wis_replay(RFIFOL(fd, 4), 1);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    return 0;
}

// Wisp/page transmission result reception
static
int intif_parse_WisEnd(int fd)
{
    dumb_ptr<map_session_data> sd;

    CharName name = stringish<CharName>(RFIFO_STRING<24>(fd, 2));
    uint8_t flag = RFIFOB(fd, 26);
    if (battle_config.etc_log)
        // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        PRINTF("intif_parse_wisend: player: %s, flag: %d\n",
                name, flag);
    sd = map_nick2sd(name);
    if (sd != NULL)
        clif_wis_end(sd->fd, flag);

    return 0;
}

// Received wisp message from map-server via char-server for ALL gm
static
void mapif_parse_WisToGM(int fd)
{
    // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
    int min_gm_level, len;

    if (RFIFOW(fd, 2) - 30 <= 0)
        return;

    len = RFIFOW(fd, 2) - 30;

    min_gm_level = RFIFOW(fd, 28);
    CharName Wisp_name = stringish<CharName>(RFIFO_STRING<24>(fd, 4));
    FString message = RFIFO_STRING(fd, 30, len);
    // information is sended to all online GM
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
            if (pc_isGM(pl_sd) >= min_gm_level)
                clif_wis_message(i, Wisp_name, message);
    }
}

// アカウント変数通知
static
int intif_parse_AccountReg(int fd)
{
    int j, p;
    dumb_ptr<map_session_data> sd;

    if ((sd = map_id2sd(RFIFOL(fd, 4))) == NULL)
        return 1;
    for (p = 8, j = 0; p < RFIFOW(fd, 2) && j < ACCOUNT_REG_NUM;
         p += 36, j++)
    {
        sd->status.account_reg[j].str = stringish<VarName>(RFIFO_STRING<32>(fd, p));
        sd->status.account_reg[j].value = RFIFOL(fd, p + 32);
    }
    sd->status.account_reg_num = j;
//  PRINTF("intif: accountreg\n");

    return 0;
}

// 倉庫データ受信
static
int intif_parse_LoadStorage(int fd)
{
    struct storage *stor;
    dumb_ptr<map_session_data> sd;

    sd = map_id2sd(RFIFOL(fd, 4));
    if (sd == NULL)
    {
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: user not found %d\n",
                    RFIFOL(fd, 4));
        return 1;
    }
    stor = account2storage(RFIFOL(fd, 4));
    if (stor->storage_status == 1)
    {                           // Already open.. lets ignore this update
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: storage received for a client already open (User %d:%d)\n",
                 sd->status.account_id, sd->status.char_id);
        return 1;
    }
    if (stor->dirty)
    {                           // Already have storage, and it has been modified and not saved yet! Exploit! [Skotlex]
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: received storage for an already modified non-saved storage! (User %d:%d)\n",
                 sd->status.account_id, sd->status.char_id);
        return 1;
    }

    if (RFIFOW(fd, 2) - 8 != sizeof(struct storage))
    {
        if (battle_config.error_log)
            PRINTF("intif_parse_LoadStorage: data size error %d %zu\n",
                    RFIFOW(fd, 2) - 8, sizeof(struct storage));
        return 1;
    }
    if (battle_config.save_log)
        PRINTF("intif_openstorage: %d\n", RFIFOL(fd, 4));
    RFIFO_STRUCT(fd, 8, *stor);
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
void intif_parse_SaveStorage(int fd)
{
    if (battle_config.save_log)
        PRINTF("intif_savestorage: done %d %d\n", RFIFOL(fd, 2),
                RFIFOB(fd, 6));
    storage_storage_saved(RFIFOL(fd, 2));
}

// パーティ作成可否
static
void intif_parse_PartyCreated(int fd)
{
    if (battle_config.etc_log)
        PRINTF("intif: party created\n");
    int account_id = RFIFOL(fd, 2);
    int fail = RFIFOB(fd, 6);
    int party_id = RFIFOL(fd, 7);
    PartyName name = stringish<PartyName>(RFIFO_STRING<24>(fd, 11));
    party_created(account_id, fail, party_id, name);
}

// パーティ情報
static
void intif_parse_PartyInfo(int fd)
{
    if (RFIFOW(fd, 2) == 8)
    {
        if (battle_config.error_log)
            PRINTF("intif: party noinfo %d\n", RFIFOL(fd, 4));
        party_recv_noinfo(RFIFOL(fd, 4));
        return;
    }

//  PRINTF("intif: party info %d\n",RFIFOL(fd,4));
    if (RFIFOW(fd, 2) != sizeof(struct party) + 4)
    {
        if (battle_config.error_log)
            PRINTF("intif: party info : data size error %d %d %zu\n",
                    RFIFOL(fd, 4), RFIFOW(fd, 2),
                    sizeof(struct party) + 4);
    }
    party p {};
    RFIFO_STRUCT(fd, 4, p);
    party_recv_info(&p);
}

// パーティ追加通知
static
void intif_parse_PartyMemberAdded(int fd)
{
    if (battle_config.etc_log)
        PRINTF("intif: party member added %d %d %d\n", RFIFOL(fd, 2),
                RFIFOL(fd, 6), RFIFOB(fd, 10));
    party_member_added(RFIFOL(fd, 2), RFIFOL(fd, 6), RFIFOB(fd, 10));
}

// パーティ設定変更通知
static
void intif_parse_PartyOptionChanged(int fd)
{
    party_optionchanged(RFIFOL(fd, 2), RFIFOL(fd, 6), RFIFOW(fd, 10),
                         RFIFOW(fd, 12), RFIFOB(fd, 14));
}

// パーティ脱退通知
static
void intif_parse_PartyMemberLeaved(int fd)
{
    int party_id = RFIFOL(fd, 2);
    int account_id = RFIFOL(fd, 6);
    CharName name = stringish<CharName>(RFIFO_STRING<24>(fd, 10));
    if (battle_config.etc_log)
        PRINTF("intif: party member leaved %d %d %s\n",
                party_id, account_id, name);
    party_member_leaved(party_id, account_id, name);
}

// パーティ解散通知
static
void intif_parse_PartyBroken(int fd)
{
    party_broken(RFIFOL(fd, 2));
}

// パーティ移動通知
static
void intif_parse_PartyMove(int fd)
{
    int party_id = RFIFOL(fd, 2);
    int account_id = RFIFOL(fd, 6);
    MapName map = stringish<MapName>(RFIFO_STRING<16>(fd, 10));
    uint8_t online = RFIFOB(fd, 26);
    uint16_t lv = RFIFOW(fd, 27);
    party_recv_movemap(party_id, account_id, map, online, lv);
}

// パーティメッセージ
static
void intif_parse_PartyMessage(int fd)
{
    size_t len = RFIFOW(fd, 2) - 12;
    FString buf = RFIFO_STRING(fd, 12, len);
    party_recv_message(RFIFOL(fd, 4), RFIFOL(fd, 8), buf);
}

//-----------------------------------------------------------------
// inter serverからの通信
// エラーがあれば0(false)を返すこと
// パケットが処理できれば1,パケット長が足りなければ2を返すこと
int intif_parse(int fd)
{
    int packet_len;
    int cmd = RFIFOW(fd, 0);
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
        if (RFIFOREST(fd) < 4)
            return 2;
        packet_len = RFIFOW(fd, 2);
    }
//  if(battle_config.etc_log)
//      PRINTF("intif_parse %d %x %d %d\n",fd,cmd,packet_len,RFIFOREST(fd));
    if (RFIFOREST(fd) < packet_len)
    {
        return 2;
    }
    // 処理分岐
    switch (cmd)
    {
        case 0x3800:
        {
            FString mes = RFIFO_STRING(fd, 4, packet_len - 4);
            clif_GMmessage(NULL, mes, 0);
        }
            break;
        case 0x3801:
            intif_parse_WisMessage(fd);
            break;
        case 0x3802:
            intif_parse_WisEnd(fd);
            break;
        case 0x3803:
            mapif_parse_WisToGM(fd);
            break;
        case 0x3804:
            intif_parse_AccountReg(fd);
            break;
        case 0x3810:
            intif_parse_LoadStorage(fd);
            break;
        case 0x3811:
            intif_parse_SaveStorage(fd);
            break;
        case 0x3820:
            intif_parse_PartyCreated(fd);
            break;
        case 0x3821:
            intif_parse_PartyInfo(fd);
            break;
        case 0x3822:
            intif_parse_PartyMemberAdded(fd);
            break;
        case 0x3823:
            intif_parse_PartyOptionChanged(fd);
            break;
        case 0x3824:
            intif_parse_PartyMemberLeaved(fd);
            break;
        case 0x3825:
            intif_parse_PartyMove(fd);
            break;
        case 0x3826:
            intif_parse_PartyBroken(fd);
            break;
        case 0x3827:
            intif_parse_PartyMessage(fd);
            break;
        default:
            if (battle_config.error_log)
                PRINTF("intif_parse : unknown packet %d %x\n", fd,
                        RFIFOW(fd, 0));
            return 0;
    }
    // パケット読み飛ばし
    RFIFOSKIP(fd, packet_len);
    return 1;
}
