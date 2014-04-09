#include "chrif.hpp"
//    chrif.cpp - Network interface to the character server.
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

#include <arpa/inet.h>

#include <cstring>

#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/socket.hpp"
#include "../mmo/timer.hpp"
#include "../mmo/utils.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "pc.hpp"
#include "storage.hpp"

#include "../poison.hpp"

static
const int packet_len_table[0x20] =
{
    60, 3, 10, 27, 22, -1, 6, -1,   // 2af8-2aff
    6, -1, 18, 7, -1, 49, 44, 0,    // 2b00-2b07
    6, 30, -1, 10, 86, 7, 44, 34,   // 2b08-2b0f
    -1, -1, 10, 6, 11, -1, 0, 0,    // 2b10-2b17
};

Session *char_session;
static
IP4Address char_ip;
static
int char_port = 6121;
static
AccountName userid;
static
AccountPass passwd;
static
int chrif_state;

// 設定ファイル読み込み関係
/*==========================================
 *
 *------------------------------------------
 */
void chrif_setuserid(AccountName id)
{
    userid = id;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setpasswd(AccountPass pwd)
{
    passwd = pwd;
}

AccountPass chrif_getpasswd(void)
{
    return passwd;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setip(IP4Address ip)
{
    char_ip = ip;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setport(int port)
{
    char_port = port;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_isconnect(void)
{
    return chrif_state == 2;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_save(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(-1, sd);

    if (!char_session)
        return -1;

    pc_makesavestatus(sd);

    WFIFOW(char_session, 0) = 0x2b01;
    WFIFOW(char_session, 2) = sizeof(sd->status_key) + sizeof(sd->status) + 12;
    WFIFOL(char_session, 4) = sd->bl_id;
    WFIFOL(char_session, 8) = sd->char_id;
    WFIFO_STRUCT(char_session, 12, sd->status_key);
    WFIFO_STRUCT(char_session, 12 + sizeof(sd->status_key), sd->status);
    WFIFOSET(char_session, WFIFOW(char_session, 2));

    //For data sync
    if (sd->state.storage_open)
        storage_storage_save(sd->status_key.account_id, 0);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_connect(Session *s)
{
    WFIFOW(s, 0) = 0x2af8;
    WFIFO_STRING(s, 2, userid, 24);
    WFIFO_STRING(s, 26, passwd, 24);
    WFIFOL(s, 50) = 0;
    WFIFOIP(s, 54) = clif_getip();
    WFIFOW(s, 58) = clif_getport();  // [Valaris] thanks to fov
    WFIFOSET(s, 60);

    return 0;
}

/*==========================================
 * マップ送信
 *------------------------------------------
 */
static
int chrif_sendmap(Session *s)
{
    int i = 0;

    WFIFOW(s, 0) = 0x2afa;
    for (auto& pair : maps_db)
    {
        map_abstract *ma = pair.second.get();
        if (!ma->gat)
            continue;
        WFIFO_STRING(s, 4 + i * 16, ma->name_, 16);
        i++;
    }
    WFIFOW(s, 2) = 4 + i * 16;
    WFIFOSET(s, WFIFOW(s, 2));

    return 0;
}

/*==========================================
 * マップ受信
 *------------------------------------------
 */
static
int chrif_recvmap(Session *s)
{
    int i, j;

    if (chrif_state < 2)        // まだ準備中
        return -1;

    IP4Address ip = RFIFOIP(s, 4);
    uint16_t port = RFIFOW(s, 8);
    for (i = 10, j = 0; i < RFIFOW(s, 2); i += 16, j++)
    {
        MapName map = RFIFO_STRING<16>(s, i);
        map_setipport(map, ip, port);
    }
    if (battle_config.etc_log)
        PRINTF("recv map on %s:%d (%d maps)\n",
                ip, port, j);

    return 0;
}

/*==========================================
 * マップ鯖間移動のためのデータ準備要求
 *------------------------------------------
 */
int chrif_changemapserver(dumb_ptr<map_session_data> sd,
        MapName name, int x, int y, IP4Address ip, short port)
{
    nullpo_retr(-1, sd);

    IP4Address s_ip;
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        if (dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get())) == sd)
        {
            assert (s == sd->sess);
            s_ip = s->client_ip;
            break;
        }
    }

    WFIFOW(char_session, 0) = 0x2b05;
    WFIFOL(char_session, 2) = sd->bl_id;
    WFIFOL(char_session, 6) = sd->login_id1;
    WFIFOL(char_session, 10) = sd->login_id2;
    WFIFOL(char_session, 14) = sd->status_key.char_id;
    WFIFO_STRING(char_session, 18, name, 16);
    WFIFOW(char_session, 34) = x;
    WFIFOW(char_session, 36) = y;
    WFIFOIP(char_session, 38) = ip;
    WFIFOL(char_session, 42) = port;
    WFIFOB(char_session, 44) = static_cast<uint8_t>(sd->status.sex);
    WFIFOIP(char_session, 45) = s_ip;
    WFIFOSET(char_session, 49);

    return 0;
}

/*==========================================
 * マップ鯖間移動ack
 *------------------------------------------
 */
static
int chrif_changemapserverack(Session *s)
{
    dumb_ptr<map_session_data> sd = map_id2sd(RFIFOL(s, 2));

    if (sd == NULL || sd->status_key.char_id != RFIFOL(s, 14))
        return -1;

    if (RFIFOL(s, 6) == 1)
    {
        if (battle_config.error_log)
            PRINTF("map server change failed.\n");
        pc_authfail(sd->status_key.account_id);
        return 0;
    }
    MapName mapname = RFIFO_STRING<16>(s, 18);
    uint16_t x = RFIFOW(s, 34);
    uint16_t y = RFIFOW(s, 36);
    IP4Address ip = RFIFOIP(s, 38);
    uint16_t port = RFIFOW(s, 42);
    clif_changemapserver(sd, mapname, x, y, ip, port);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_connectack(Session *s)
{
    if (RFIFOB(s, 2))
    {
        PRINTF("Connected to char-server failed %d.\n", RFIFOB(s, 2));
        exit(1);
    }
    PRINTF("Connected to char-server (connection #%d).\n", s);
    chrif_state = 1;

    chrif_sendmap(s);

    PRINTF("chrif: OnCharIfInit event done. (%d events)\n",
            npc_event_doall(stringish<ScriptLabel>("OnCharIfInit")));
    PRINTF("chrif: OnInterIfInit event done. (%d events)\n",
            npc_event_doall(stringish<ScriptLabel>("OnInterIfInit")));

    // <Agit> Run Event [AgitInit]
//  PRINTF("NPC_Event:[OnAgitInit] do (%d) events (Agit Initialize).\n", npc_event_doall("OnAgitInit"));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_sendmapack(Session *s)
{
    if (RFIFOB(s, 2))
    {
        PRINTF("chrif : send map list to char server failed %d\n",
                RFIFOB(s, 2));
        exit(1);
    }

    wisp_server_name = stringish<CharName>(RFIFO_STRING<24>(s, 3));

    chrif_state = 2;

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_authreq(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(-1, sd);

    if (!sd || !char_session || !sd->bl_id || !sd->login_id1)
        return -1;

    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        if (dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get())) == sd)
        {
            assert (s == sd->sess);
            WFIFOW(char_session, 0) = 0x2afc;
            WFIFOL(char_session, 2) = sd->bl_id;
            WFIFOL(char_session, 6) = sd->char_id;
            WFIFOL(char_session, 10) = sd->login_id1;
            WFIFOL(char_session, 14) = sd->login_id2;
            WFIFOIP(char_session, 18) = s->client_ip;
            WFIFOSET(char_session, 22);
            break;
        }
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_charselectreq(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(-1, sd);

    if (!sd || !char_session || !sd->bl_id || !sd->login_id1)
        return -1;

    IP4Address s_ip;
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        if (dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get())) == sd)
        {
            assert (s == sd->sess);
            s_ip = s->client_ip;
            break;
        }
    }

    WFIFOW(char_session, 0) = 0x2b02;
    WFIFOL(char_session, 2) = sd->bl_id;
    WFIFOL(char_session, 6) = sd->login_id1;
    WFIFOL(char_session, 10) = sd->login_id2;
    WFIFOIP(char_session, 14) = s_ip;
    WFIFOSET(char_session, 18);

    return 0;
}

/*==========================================
 * GMに変化要求
 *------------------------------------------
 */
void chrif_changegm(int id, ZString pass)
{
    if (battle_config.etc_log)
        PRINTF("chrif_changegm: account: %d, password: '%s'.\n", id, pass);

    size_t len = pass.size() + 1;
    WFIFOW(char_session, 0) = 0x2b0a;
    WFIFOW(char_session, 2) = len + 8;
    WFIFOL(char_session, 4) = id;
    WFIFO_STRING(char_session, 8, pass, len);
    WFIFOSET(char_session, len + 8);
}

/*==========================================
 * Change Email
 *------------------------------------------
 */
void chrif_changeemail(int id, AccountEmail actual_email,
        AccountEmail new_email)
{
    if (battle_config.etc_log)
        PRINTF("chrif_changeemail: account: %d, actual_email: '%s', new_email: '%s'.\n",
             id, actual_email, new_email);

    WFIFOW(char_session, 0) = 0x2b0c;
    WFIFOL(char_session, 2) = id;
    WFIFO_STRING(char_session, 6, actual_email, 40);
    WFIFO_STRING(char_session, 46, new_email, 40);
    WFIFOSET(char_session, 86);
}

/*==========================================
 * Send message to char-server with a character name to do some operations (by Yor)
 * Used to ask Char-server about a character name to have the account number to modify account file in login-server.
 * type of operation:
 *   1: block
 *   2: ban
 *   3: unblock
 *   4: unban
 *   5: changesex
 *------------------------------------------
 */
void chrif_char_ask_name(int id, CharName character_name, short operation_type,
        HumanTimeDiff modif)
{
    WFIFOW(char_session, 0) = 0x2b0e;
    WFIFOL(char_session, 2) = id;   // account_id of who ask (for answer) -1 if nobody
    WFIFO_STRING(char_session, 6, character_name.to__actual(), 24);
    WFIFOW(char_session, 30) = operation_type;  // type of operation
    if (operation_type == 2)
        WFIFO_STRUCT(char_session, 32, modif);
    PRINTF("chrif : sended 0x2b0e\n");
    WFIFOSET(char_session, 44);
}

/*==========================================
 * Answer after a request about a character name to do some operations (by Yor)
 * Used to answer of chrif_char_ask_name.
 * type of operation:
 *   1: block
 *   2: ban
 *   3: unblock
 *   4: unban
 *   5: changesex
 * type of answer:
 *   0: login-server resquest done
 *   1: player not found
 *   2: gm level too low
 *   3: login-server offline
 *------------------------------------------
 */
static
int chrif_char_ask_name_answer(Session *s)
{
    int acc = RFIFOL(s, 2);       // account_id of who has asked (-1 if nobody)
    CharName player_name = stringish<CharName>(RFIFO_STRING<24>(s, 6));

    dumb_ptr<map_session_data> sd = map_id2sd(acc);
    if (acc >= 0 && sd != NULL)
    {
        AString output;
        if (RFIFOW(s, 32) == 1)   // player not found
            output = STRPRINTF("The player '%s' doesn't exist.",
                    player_name);
        else
        {
            switch (RFIFOW(s, 30))
            {
                case 1:        // block
                    switch (RFIFOW(s, 32))
                    {
                        case 0:    // login-server resquest done
                            output = STRPRINTF(
                                    "Login-server has been asked to block the player '%s'.",
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level don't authorise you to block the player '%s'.",
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline. Impossible to block the the player '%s'.",
                                    player_name);
                            break;
                    }
                    break;
                case 2:        // ban
                    switch (RFIFOW(s, 32))
                    {
                        case 0:    // login-server resquest done
                            output = STRPRINTF(
                                    "Login-server has been asked to ban the player '%s'.",
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level don't authorise you to ban the player '%s'.",
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline. Impossible to ban the the player '%s'.",
                                    player_name);
                            break;
                    }
                    break;
                case 3:        // unblock
                    switch (RFIFOW(s, 32))
                    {
                        case 0:    // login-server resquest done
                            output = STRPRINTF(
                                    "Login-server has been asked to unblock the player '%s'.",
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level don't authorise you to unblock the player '%s'.",
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline. Impossible to unblock the the player '%s'.",
                                    player_name);
                            break;
                    }
                    break;
                case 4:        // unban
                    switch (RFIFOW(s, 32))
                    {
                        case 0:    // login-server resquest done
                            output = STRPRINTF(
                                    "Login-server has been asked to unban the player '%s'.",
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level don't authorise you to unban the player '%s'.",
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline. Impossible to unban the the player '%s'.",
                                    player_name);
                            break;
                    }
                    break;
                case 5:        // changesex
                    switch (RFIFOW(s, 32))
                    {
                        case 0:    // login-server resquest done
                            output = STRPRINTF(
                                    "Login-server has been asked to change the sex of the player '%s'.",
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level don't authorise you to change the sex of the player '%s'.",
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline. Impossible to change the sex of the the player '%s'.",
                                    player_name);
                            break;
                    }
                    break;
            }
        }
        if (output)
            clif_displaymessage(sd->sess, output);
    }
    else
        PRINTF("chrif_char_ask_name_answer failed - player not online.\n");

    return 0;
}

/*==========================================
 * End of GM change(@GM) (modified by Yor)
 *------------------------------------------
 */
static
void chrif_changedgm(Session *s)
{
    int acc, level;
    dumb_ptr<map_session_data> sd = NULL;

    acc = RFIFOL(s, 2);
    level = RFIFOL(s, 6);

    sd = map_id2sd(acc);

    if (battle_config.etc_log)
        PRINTF("chrif_changedgm: account: %d, GM level 0 -> %d.\n", acc,
                level);
    if (sd != NULL)
    {
        if (level > 0)
            clif_displaymessage(sd->sess, "GM modification success.");
        else
            clif_displaymessage(sd->sess, "Failure of GM modification.");
    }
}

/*==========================================
 * 性別変化終了 (modified by Yor)
 *------------------------------------------
 */
static
void chrif_changedsex(Session *s)
{
    int acc, i;
    dumb_ptr<map_session_data> sd;

    acc = RFIFOL(s, 2);
    SEX sex = static_cast<SEX>(RFIFOB(s, 6));
    if (battle_config.etc_log)
        PRINTF("chrif_changedsex %d.\n", acc);
    sd = map_id2sd(acc);
    if (acc > 0)
    {
        if (sd != NULL && sd->status.sex != sex)
        {
            if (sd->status.sex == SEX::MALE)
                sd->sex = sd->status.sex = SEX::FEMALE;
            else if (sd->status.sex == SEX::FEMALE)
                sd->sex = sd->status.sex = SEX::MALE;
            // to avoid any problem with equipment and invalid sex, equipment is unequiped.
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                if (sd->status.inventory[i].nameid
                    && bool(sd->status.inventory[i].equip))
                    pc_unequipitem(sd, i, CalcStatus::NOW);
            }
            // save character
            chrif_save(sd);
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            // do same modify in login-server for the account, but no in char-server (it ask again login_id1 to login, and don't remember it)
            clif_displaymessage(sd->sess,
                                 "Your sex has been changed (need disconexion by the server)...");
            clif_setwaitclose(sd->sess); // forced to disconnect for the change
        }
    }
    else
    {
        if (sd != NULL)
        {
            PRINTF("chrif_changedsex failed.\n");
        }
    }
}

/*==========================================
 * アカウント変数保存要求
 *------------------------------------------
 */
int chrif_saveaccountreg2(dumb_ptr<map_session_data> sd)
{
    int p, j;
    nullpo_retr(-1, sd);

    p = 8;
    for (j = 0; j < sd->status.account_reg2_num; j++)
    {
        struct global_reg *reg = &sd->status.account_reg2[j];
        if (reg->str && reg->value != 0)
        {
            WFIFO_STRING(char_session, p, reg->str, 32);
            WFIFOL(char_session, p + 32) = reg->value;
            p += 36;
        }
    }
    WFIFOW(char_session, 0) = 0x2b10;
    WFIFOW(char_session, 2) = p;
    WFIFOL(char_session, 4) = sd->bl_id;
    WFIFOSET(char_session, p);

    return 0;
}

/*==========================================
 * アカウント変数通知
 *------------------------------------------
 */
static
int chrif_accountreg2(Session *s)
{
    int j, p;
    dumb_ptr<map_session_data> sd = map_id2sd(RFIFOL(s, 4));
    if (sd == NULL)
        return 1;

    for (p = 8, j = 0; p < RFIFOW(s, 2) && j < ACCOUNT_REG2_NUM;
         p += 36, j++)
    {
        sd->status.account_reg2[j].str = stringish<VarName>(RFIFO_STRING<32>(s, p));
        sd->status.account_reg2[j].value = RFIFOL(s, p + 32);
    }
    sd->status.account_reg2_num = j;

    return 0;
}

/*==========================================
 * Divorce request from char server
 * triggered on account deletion or as an
 * ack from a map-server divorce request
 *------------------------------------------
 */
static
int chrif_divorce(int char_id, int partner_id)
{
    dumb_ptr<map_session_data> sd = NULL;

    if (!char_id || !partner_id)
        return 0;

    sd = map_nick2sd(map_charid2nick(char_id));
    if (sd && sd->status.partner_id == partner_id)
    {
        sd->status.partner_id = 0;

        if (sd->npc_flags.divorce)
        {
            sd->npc_flags.divorce = 0;
            map_scriptcont(sd, sd->npc_id);
        }
    }

    sd = map_nick2sd(map_charid2nick(partner_id));
    nullpo_ret(sd);
    if (sd->status.partner_id == char_id)
        sd->status.partner_id = 0;

    return 0;
}

/*==========================================
 * Tell character server someone is divorced
 * Needed to divorce when partner is not connected to map server
 *-------------------------------------
 */
int chrif_send_divorce(int char_id)
{
    if (!char_session)
        return -1;

    WFIFOW(char_session, 0) = 0x2b16;
    WFIFOL(char_session, 2) = char_id;
    WFIFOSET(char_session, 6);
    return 0;
}

/*==========================================
 * Disconnection of a player (account has been deleted in login-server) by [Yor]
 *------------------------------------------
 */
static
int chrif_accountdeletion(Session *s)
{
    int acc;
    dumb_ptr<map_session_data> sd;

    acc = RFIFOL(s, 2);
    if (battle_config.etc_log)
        PRINTF("chrif_accountdeletion %d.\n", acc);
    sd = map_id2sd(acc);
    if (acc > 0)
    {
        if (sd != NULL)
        {
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            clif_displaymessage(sd->sess,
                                 "Your account has been deleted (disconnection)...");
            clif_setwaitclose(sd->sess); // forced to disconnect for the change
        }
    }
    else
    {
        if (sd != NULL)
            PRINTF("chrif_accountdeletion failed - player not online.\n");
    }

    return 0;
}

/*==========================================
 * Disconnection of a player (account has been banned of has a status, from login-server) by [Yor]
 *------------------------------------------
 */
static
int chrif_accountban(Session *s)
{
    int acc;
    dumb_ptr<map_session_data> sd;

    acc = RFIFOL(s, 2);
    if (battle_config.etc_log)
        PRINTF("chrif_accountban %d.\n", acc);
    sd = map_id2sd(acc);
    if (acc > 0)
    {
        if (sd != NULL)
        {
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            if (RFIFOB(s, 6) == 0)
            {                   // 0: change of statut, 1: ban
                switch (RFIFOL(s, 7))
                {               // status or final date of a banishment
                    case 1:    // 0 = Unregistered ID
                        clif_displaymessage(sd->sess,
                                             "Your account has 'Unregistered'.");
                        break;
                    case 2:    // 1 = Incorrect Password
                        clif_displaymessage(sd->sess,
                                             "Your account has an 'Incorrect Password'...");
                        break;
                    case 3:    // 2 = This ID is expired
                        clif_displaymessage(sd->sess,
                                             "Your account has expired.");
                        break;
                    case 4:    // 3 = Rejected from Server
                        clif_displaymessage(sd->sess,
                                             "Your account has been rejected from server.");
                        break;
                    case 5:    // 4 = You have been blocked by the GM Team
                        clif_displaymessage(sd->sess,
                                             "Your account has been blocked by the GM Team.");
                        break;
                    case 6:    // 5 = Your Game's EXE file is not the latest version
                        clif_displaymessage(sd->sess,
                                             "Your Game's EXE file is not the latest version.");
                        break;
                    case 7:    // 6 = Your are Prohibited to log in until %s
                        clif_displaymessage(sd->sess,
                                             "Your account has been prohibited to log in.");
                        break;
                    case 8:    // 7 = Server is jammed due to over populated
                        clif_displaymessage(sd->sess,
                                             "Server is jammed due to over populated.");
                        break;
                    case 9:    // 8 = No MSG (actually, all states after 9 except 99 are No MSG, use only this)
                        clif_displaymessage(sd->sess,
                                             "Your account has not more authorised.");
                        break;
                    case 100:  // 99 = This ID has been totally erased
                        clif_displaymessage(sd->sess,
                                             "Your account has been totally erased.");
                        break;
                    default:
                        clif_displaymessage(sd->sess,
                                             "Your account has not more authorised.");
                        break;
                }
            }
            else if (RFIFOB(s, 6) == 1)
            {
                // 0: change of statut, 1: ban
                TimeT timestamp = static_cast<time_t>(RFIFOL(s, 7));    // status or final date of a banishment
                char tmpstr[] = WITH_TIMESTAMP("Your account has been banished until ");
                REPLACE_TIMESTAMP(tmpstr, timestamp);
                clif_displaymessage(sd->sess, const_(tmpstr));
            }
            clif_setwaitclose(sd->sess); // forced to disconnect for the change
        }
    }
    else
    {
        if (sd != NULL)
            PRINTF("chrif_accountban failed - player not online.\n");
    }

    return 0;
}

/*==========================================
 * Receiving GM accounts and their levels from char-server by [Yor]
 *------------------------------------------
 */
static
int chrif_recvgmaccounts(Session *s)
{
    PRINTF("From login-server: receiving of %d GM accounts information.\n",
            pc_read_gm_account(s));

    return 0;
}

/*==========================================
 * Request to reload GM accounts and their levels: send to char-server by [Yor]
 *------------------------------------------
 */
int chrif_reloadGMdb(void)
{

    WFIFOW(char_session, 0) = 0x2af7;
    WFIFOSET(char_session, 2);

    return 0;
}

/*========================================
 * Map item IDs
 *----------------------------------------
 */

static
void ladmin_itemfrob_fix_item(int source, int dest, struct item *item)
{
    if (item && item->nameid == source)
    {
        item->nameid = dest;
        item->equip = EPOS::ZERO;
    }
}

static
void ladmin_itemfrob_c2(dumb_ptr<block_list> bl, int source_id, int dest_id)
{
#define IFIX(v) if (v == source_id) {v = dest_id; }
#define FIX(item) ladmin_itemfrob_fix_item(source_id, dest_id, &item)

    if (!bl)
        return;

    switch (bl->bl_type)
    {
        case BL::PC:
        {
            dumb_ptr<map_session_data> pc = bl->is_player();
            struct storage *stor = account2storage2(pc->status_key.account_id);
            int j;

            for (j = 0; j < MAX_INVENTORY; j++)
                IFIX(pc->status.inventory[j].nameid);
            // cart is no longer supported
            // IFIX(pc->status.weapon);
            IFIX(pc->status.shield);
            IFIX(pc->status.head_top);
            IFIX(pc->status.head_mid);
            IFIX(pc->status.head_bottom);

            if (stor)
                for (j = 0; j < stor->storage_amount; j++)
                    FIX(stor->storage_[j]);

            for (j = 0; j < MAX_INVENTORY; j++)
            {
                struct item_data *item = pc->inventory_data[j];
                if (item && item->nameid == source_id)
                {
                    item->nameid = dest_id;
                    if (bool(item->equip))
                        pc_unequipitem(pc, j, CalcStatus::NOW);
                    item->nameid = dest_id;
                }
            }

            break;
        }

        case BL::MOB:
        {
            dumb_ptr<mob_data> mob = bl->is_mob();
            for (struct item& itm : mob->lootitemv)
                FIX(itm);
            break;
        }

        case BL::ITEM:
        {
            dumb_ptr<flooritem_data> item = bl->is_item();
            FIX(item->item_data);
            break;
        }
    }
#undef FIX
#undef IFIX
}

static
void ladmin_itemfrob_c(dumb_ptr<block_list> bl, int source_id, int dest_id)
{
    ladmin_itemfrob_c2(bl, source_id, dest_id);
}

static
void ladmin_itemfrob(Session *s)
{
    int source_id = RFIFOL(s, 2);
    int dest_id = RFIFOL(s, 6);
    dumb_ptr<block_list> bl = map_get_first_session();

    // flooritems
    map_foreachobject(std::bind(ladmin_itemfrob_c, ph::_1, source_id, dest_id),
            BL::NUL /* any object */);

    // player characters (and, hopefully, mobs)
    while (bl->bl_next)
    {
        ladmin_itemfrob_c2(bl, source_id, dest_id);
        bl = bl->bl_next;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void chrif_parse(Session *s)
{
    int packet_len, cmd;

    // only char-server can have an access to here.
    // so, if it isn't the char-server, we disconnect the session (fd != char_fd).
    if (s != char_session || s->eof)
    {
        if (s == char_session)
        {
            PRINTF("Map-server can't connect to char-server (connection #%d).\n",
                 s);
            char_session = nullptr;
        }
        delete_session(s);
        return;
    }

    while (RFIFOREST(s) >= 2)
    {
        cmd = RFIFOW(s, 0);
        if (cmd < 0x2af8
            || cmd >=
            0x2af8 +
            (sizeof(packet_len_table) / sizeof(packet_len_table[0]))
            || packet_len_table[cmd - 0x2af8] == 0)
        {

            int r = intif_parse(s);  // intifに渡す

            if (r == 1)
                continue;       // intifで処理した
            if (r == 2)
                return;       // intifで処理したが、データが足りない

            s->eof = 1;
            return;
        }
        packet_len = packet_len_table[cmd - 0x2af8];
        if (packet_len == -1)
        {
            if (RFIFOREST(s) < 4)
                return;
            packet_len = RFIFOW(s, 2);
        }
        if (RFIFOREST(s) < packet_len)
            return;

        switch (cmd)
        {
            case 0x2af9:
                chrif_connectack(s);
                break;
            case 0x2afa:
                ladmin_itemfrob(s);
                break;
            case 0x2afb:
                chrif_sendmapack(s);
                break;
            case 0x2afd:
            {
                int id = RFIFOL(s, 4);
                int login_id2 = RFIFOL(s, 8);
                TimeT connect_until_time = static_cast<time_t>(RFIFOL(s, 12));
                short tmw_version = RFIFOW(s, 16);
                CharKey st_key;
                CharData st_data;
                RFIFO_STRUCT(s, 18, st_key);
                RFIFO_STRUCT(s, 18 + sizeof(st_key), st_data);
                pc_authok(id, login_id2,
                        connect_until_time, tmw_version,
                        &st_key, &st_data);
            }
                break;
            case 0x2afe:
                pc_authfail(RFIFOL(s, 2));
                break;
            case 0x2b00:
                map_setusers(RFIFOL(s, 2));
                break;
            case 0x2b03:
                clif_charselectok(RFIFOL(s, 2));
                break;
            case 0x2b04:
                chrif_recvmap(s);
                break;
            case 0x2b06:
                chrif_changemapserverack(s);
                break;
            case 0x2b0b:
                chrif_changedgm(s);
                break;
            case 0x2b0d:
                chrif_changedsex(s);
                break;
            case 0x2b0f:
                chrif_char_ask_name_answer(s);
                break;
            case 0x2b11:
                chrif_accountreg2(s);
                break;
            case 0x2b12:
                chrif_divorce(RFIFOL(s, 2), RFIFOL(s, 6));
                break;
            case 0x2b13:
                chrif_accountdeletion(s);
                break;
            case 0x2b14:
                chrif_accountban(s);
                break;
            case 0x2b15:
                chrif_recvgmaccounts(s);
                break;

            default:
                if (battle_config.error_log)
                    PRINTF("chrif_parse : unknown packet %d %d\n", s,
                            RFIFOW(s, 0));
                s->eof = 1;
                return;
        }
        RFIFOSKIP(s, packet_len);
    }
}

/*==========================================
 * timer関数
 * 今このmap鯖に繋がっているクライアント人数をchar鯖へ送る
 *------------------------------------------
 */
static
void send_users_tochar(TimerData *, tick_t)
{
    int users = 0;

    if (!char_session)
        return;

    WFIFOW(char_session, 0) = 0x2aff;
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
        if (sd && sd->state.auth &&
            !((battle_config.hide_GM_session
               || sd->state.shroud_active
               || bool(sd->status.option & Option::HIDE)) && pc_isGM(sd)))
        {
            WFIFOL(char_session, 6 + 4 * users) = sd->status_key.char_id;
            users++;
        }
    }
    WFIFOW(char_session, 2) = 6 + 4 * users;
    WFIFOW(char_session, 4) = users;
    WFIFOSET(char_session, 6 + 4 * users);
}

/*==========================================
 * timer関数
 * char鯖との接続を確認し、もし切れていたら再度接続する
 *------------------------------------------
 */
static
void check_connect_char_server(TimerData *, tick_t)
{
    if (!char_session)
    {
        PRINTF("Attempt to connect to char-server...\n");
        chrif_state = 0;
        char_session = make_connection(char_ip, char_port);
        if (!char_session)
            return;
        char_session->func_parse = chrif_parse;
        realloc_fifo(char_session, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

        chrif_connect(char_session);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void do_init_chrif(void)
{
    Timer(gettick() + std::chrono::seconds(1),
            check_connect_char_server,
            std::chrono::seconds(10)
    ).detach();
    Timer(gettick() + std::chrono::seconds(1),
            send_users_tochar,
            std::chrono::seconds(5)
    ).detach();
}
