#include "chrif.hpp"

#include <arpa/inet.h>

#include <cstring>

#include "../common/cxxstdio.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"
#include "../common/utils.hpp"

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

int char_fd;
static
char char_ip_str[16];
static
int char_ip;
static
int char_port = 6121;
static
char userid[24], passwd[24];
static
int chrif_state;

// 設定ファイル読み込み関係
/*==========================================
 *
 *------------------------------------------
 */
void chrif_setuserid(const char *id)
{
    strzcpy(userid, id, sizeof(userid));
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setpasswd(const char *pwd)
{
    strzcpy(passwd, pwd, sizeof(passwd));
}

char *chrif_getpasswd(void)
{
    return passwd;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setip(const char *ip)
{
    strzcpy(char_ip_str, ip, sizeof(char_ip_str));
    char_ip = inet_addr(char_ip_str);
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

    if (char_fd < 0)
        return -1;

    pc_makesavestatus(sd);

    WFIFOW(char_fd, 0) = 0x2b01;
    WFIFOW(char_fd, 2) = sizeof(sd->status) + 12;
    WFIFOL(char_fd, 4) = sd->bl_id;
    WFIFOL(char_fd, 8) = sd->char_id;
    WFIFO_STRUCT(char_fd, 12, sd->status);
    WFIFOSET(char_fd, WFIFOW(char_fd, 2));

    //For data sync
    if (sd->state.storage_open)
        storage_storage_save(sd->status.account_id, 0);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_connect(int fd)
{
    WFIFOW(fd, 0) = 0x2af8;
    WFIFO_STRING(fd, 2, userid, 24);
    WFIFO_STRING(fd, 26, passwd, 24);
    WFIFOL(fd, 50) = 0;
    WFIFOL(fd, 54) = clif_getip().s_addr;
    WFIFOW(fd, 58) = clif_getport();  // [Valaris] thanks to fov
    WFIFOSET(fd, 60);

    return 0;
}

/*==========================================
 * マップ送信
 *------------------------------------------
 */
static
int chrif_sendmap(int fd)
{
    int i = 0;

    WFIFOW(fd, 0) = 0x2afa;
    for (auto& pair : maps_db)
    {
        map_abstract *ma = pair.second.get();
        if (!ma->gat)
            continue;
        WFIFO_STRING(fd, 4 + i * 16, ma->name_, 16);
        i++;
    }
    WFIFOW(fd, 2) = 4 + i * 16;
    WFIFOSET(fd, WFIFOW(fd, 2));

    return 0;
}

/*==========================================
 * マップ受信
 *------------------------------------------
 */
static
int chrif_recvmap(int fd)
{
    int i, j, port;

    if (chrif_state < 2)        // まだ準備中
        return -1;

    struct in_addr ip;
    ip.s_addr = RFIFOL(fd, 4);
    port = RFIFOW(fd, 8);
    for (i = 10, j = 0; i < RFIFOW(fd, 2); i += 16, j++)
    {
        char map[16];
        RFIFO_STRING(fd, i, map, 16);
        map_setipport(map, ip, port);
    }
    if (battle_config.etc_log)
        PRINTF("recv map on %s:%d (%d maps)\n", ip2str(ip), port, j);

    return 0;
}

/*==========================================
 * マップ鯖間移動のためのデータ準備要求
 *------------------------------------------
 */
int chrif_changemapserver(dumb_ptr<map_session_data> sd, char *name, int x,
                           int y, struct in_addr ip, short port)
{
    int i, s_ip;

    nullpo_retr(-1, sd);

    s_ip = 0;
    for (i = 0; i < fd_max; i++)
        if (session[i] && dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get())) == sd)
        {
            s_ip = session[i]->client_addr.sin_addr.s_addr;
            break;
        }

    WFIFOW(char_fd, 0) = 0x2b05;
    WFIFOL(char_fd, 2) = sd->bl_id;
    WFIFOL(char_fd, 6) = sd->login_id1;
    WFIFOL(char_fd, 10) = sd->login_id2;
    WFIFOL(char_fd, 14) = sd->status.char_id;
    WFIFO_STRING(char_fd, 18, name, 16);
    WFIFOW(char_fd, 34) = x;
    WFIFOW(char_fd, 36) = y;
    WFIFOL(char_fd, 38) = ip.s_addr;
    WFIFOL(char_fd, 42) = port;
    WFIFOB(char_fd, 44) = sd->status.sex;
    WFIFOL(char_fd, 45) = s_ip;
    WFIFOSET(char_fd, 49);

    return 0;
}

/*==========================================
 * マップ鯖間移動ack
 *------------------------------------------
 */
static
int chrif_changemapserverack(int fd)
{
    dumb_ptr<map_session_data> sd = map_id2sd(RFIFOL(fd, 2));

    if (sd == NULL || sd->status.char_id != RFIFOL(fd, 14))
        return -1;

    if (RFIFOL(fd, 6) == 1)
    {
        if (battle_config.error_log)
            PRINTF("map server change failed.\n");
        pc_authfail(sd->fd);
        return 0;
    }
    char mapname[16];
    RFIFO_STRING(fd, 18, mapname, 16);
    uint16_t x = RFIFOW(fd, 34);
    uint16_t y = RFIFOW(fd, 36);
    auto ip = in_addr{RFIFOL(fd, 38)};
    uint16_t port = RFIFOW(fd, 42);
    clif_changemapserver(sd, mapname, x, y, ip, port);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_connectack(int fd)
{
    if (RFIFOB(fd, 2))
    {
        PRINTF("Connected to char-server failed %d.\n", RFIFOB(fd, 2));
        exit(1);
    }
    PRINTF("Connected to char-server (connection #%d).\n", fd);
    chrif_state = 1;

    chrif_sendmap(fd);

    PRINTF("chrif: OnCharIfInit event done. (%d events)\n",
            npc_event_doall("OnCharIfInit"));
    PRINTF("chrif: OnInterIfInit event done. (%d events)\n",
            npc_event_doall("OnInterIfInit"));

    // <Agit> Run Event [AgitInit]
//  PRINTF("NPC_Event:[OnAgitInit] do (%d) events (Agit Initialize).\n", npc_event_doall("OnAgitInit"));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_sendmapack(int fd)
{
    if (RFIFOB(fd, 2))
    {
        PRINTF("chrif : send map list to char server failed %d\n",
                RFIFOB(fd, 2));
        exit(1);
    }

    RFIFO_STRING(fd, 3, wisp_server_name, 24);

    chrif_state = 2;

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_authreq(dumb_ptr<map_session_data> sd)
{
    int i;

    nullpo_retr(-1, sd);

    if (!sd || !char_fd || !sd->bl_id || !sd->login_id1)
        return -1;

    for (i = 0; i < fd_max; i++)
        if (session[i] && dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get())) == sd)
        {
            WFIFOW(char_fd, 0) = 0x2afc;
            WFIFOL(char_fd, 2) = sd->bl_id;
            WFIFOL(char_fd, 6) = sd->char_id;
            WFIFOL(char_fd, 10) = sd->login_id1;
            WFIFOL(char_fd, 14) = sd->login_id2;
            WFIFOL(char_fd, 18) = session[i]->client_addr.sin_addr.s_addr;
            WFIFOSET(char_fd, 22);
            break;
        }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_charselectreq(dumb_ptr<map_session_data> sd)
{
    int i, s_ip;

    nullpo_retr(-1, sd);

    if (!sd || !char_fd || !sd->bl_id || !sd->login_id1)
        return -1;

    s_ip = 0;
    for (i = 0; i < fd_max; i++)
        if (session[i] && dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get())) == sd)
        {
            s_ip = session[i]->client_addr.sin_addr.s_addr;
            break;
        }

    WFIFOW(char_fd, 0) = 0x2b02;
    WFIFOL(char_fd, 2) = sd->bl_id;
    WFIFOL(char_fd, 6) = sd->login_id1;
    WFIFOL(char_fd, 10) = sd->login_id2;
    WFIFOL(char_fd, 14) = s_ip;
    WFIFOSET(char_fd, 18);

    return 0;
}

/*==========================================
 * キャラ名問い合わせ
 *------------------------------------------
 */
int chrif_searchcharid(int char_id)
{
    if (!char_id)
        return -1;

    WFIFOW(char_fd, 0) = 0x2b08;
    WFIFOL(char_fd, 2) = char_id;
    WFIFOSET(char_fd, 6);

    return 0;
}

/*==========================================
 * GMに変化要求
 *------------------------------------------
 */
int chrif_changegm(int id, const char *pass, int len)
{
    if (battle_config.etc_log)
        PRINTF("chrif_changegm: account: %d, password: '%s'.\n", id, pass);

    WFIFOW(char_fd, 0) = 0x2b0a;
    WFIFOW(char_fd, 2) = len + 8;
    WFIFOL(char_fd, 4) = id;
    WFIFO_STRING(char_fd, 8, pass, len);
    WFIFOSET(char_fd, len + 8);

    return 0;
}

/*==========================================
 * Change Email
 *------------------------------------------
 */
int chrif_changeemail(int id, const char *actual_email,
                       const char *new_email)
{
    if (battle_config.etc_log)
        PRINTF("chrif_changeemail: account: %d, actual_email: '%s', new_email: '%s'.\n",
             id, actual_email, new_email);

    WFIFOW(char_fd, 0) = 0x2b0c;
    WFIFOL(char_fd, 2) = id;
    WFIFO_STRING(char_fd, 6, actual_email, 40);
    WFIFO_STRING(char_fd, 46, new_email, 40);
    WFIFOSET(char_fd, 86);

    return 0;
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
int chrif_char_ask_name(int id, char *character_name, short operation_type,
                         int year, int month, int day, int hour, int minute,
                         int second)
{
    WFIFOW(char_fd, 0) = 0x2b0e;
    WFIFOL(char_fd, 2) = id;   // account_id of who ask (for answer) -1 if nobody
    WFIFO_STRING(char_fd, 6, character_name, 24);
    WFIFOW(char_fd, 30) = operation_type;  // type of operation
    if (operation_type == 2)
    {
        WFIFOW(char_fd, 32) = year;
        WFIFOW(char_fd, 34) = month;
        WFIFOW(char_fd, 36) = day;
        WFIFOW(char_fd, 38) = hour;
        WFIFOW(char_fd, 40) = minute;
        WFIFOW(char_fd, 42) = second;
    }
    PRINTF("chrif : sended 0x2b0e\n");
    WFIFOSET(char_fd, 44);

    return 0;
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
int chrif_char_ask_name_answer(int fd)
{
    int acc;
    dumb_ptr<map_session_data> sd;
    char player_name[24];

    acc = RFIFOL(fd, 2);       // account_id of who has asked (-1 if nobody)
    RFIFO_STRING(fd, 6, player_name, 24);

    sd = map_id2sd(acc);
    if (acc >= 0 && sd != NULL)
    {
        std::string output;
        if (RFIFOW(fd, 32) == 1)   // player not found
            output = STRPRINTF("The player '%s' doesn't exist.",
                    player_name);
        else
        {
            switch (RFIFOW(fd, 30))
            {
                case 1:        // block
                    switch (RFIFOW(fd, 32))
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
                    switch (RFIFOW(fd, 32))
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
                    switch (RFIFOW(fd, 32))
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
                    switch (RFIFOW(fd, 32))
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
                    switch (RFIFOW(fd, 32))
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
        if (!output.empty())
            clif_displaymessage(sd->fd, output);
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
int chrif_changedgm(int fd)
{
    int acc, level;
    dumb_ptr<map_session_data> sd = NULL;

    acc = RFIFOL(fd, 2);
    level = RFIFOL(fd, 6);

    sd = map_id2sd(acc);

    if (battle_config.etc_log)
        PRINTF("chrif_changedgm: account: %d, GM level 0 -> %d.\n", acc,
                level);
    if (sd != NULL)
    {
        if (level > 0)
            clif_displaymessage(sd->fd, "GM modification success.");
        else
            clif_displaymessage(sd->fd, "Failure of GM modification.");
    }

    return 0;
}

/*==========================================
 * 性別変化終了 (modified by Yor)
 *------------------------------------------
 */
static
int chrif_changedsex(int fd)
{
    int acc, sex, i;
    dumb_ptr<map_session_data> sd;

    acc = RFIFOL(fd, 2);
    sex = RFIFOL(fd, 6);
    if (battle_config.etc_log)
        PRINTF("chrif_changedsex %d.\n", acc);
    sd = map_id2sd(acc);
    if (acc > 0)
    {
        if (sd != NULL && sd->status.sex != sex)
        {
            sd->sex = sd->status.sex = !sd->status.sex;
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
            clif_displaymessage(sd->fd,
                                 "Your sex has been changed (need disconexion by the server)...");
            clif_setwaitclose(sd->fd); // forced to disconnect for the change
        }
    }
    else
    {
        if (sd != NULL)
        {
            PRINTF("chrif_changedsex failed.\n");
        }
    }

    return 0;
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
        if (reg->str[0] && reg->value != 0)
        {
            WFIFO_STRING(char_fd, p, reg->str, 32);
            WFIFOL(char_fd, p + 32) = reg->value;
            p += 36;
        }
    }
    WFIFOW(char_fd, 0) = 0x2b10;
    WFIFOW(char_fd, 2) = p;
    WFIFOL(char_fd, 4) = sd->bl_id;
    WFIFOSET(char_fd, p);

    return 0;
}

/*==========================================
 * アカウント変数通知
 *------------------------------------------
 */
static
int chrif_accountreg2(int fd)
{
    int j, p;
    dumb_ptr<map_session_data> sd;

    if ((sd = map_id2sd(RFIFOL(fd, 4))) == NULL)
        return 1;

    for (p = 8, j = 0; p < RFIFOW(fd, 2) && j < ACCOUNT_REG2_NUM;
         p += 36, j++)
    {
        RFIFO_STRING(fd, p, sd->status.account_reg2[j].str, 32);
        sd->status.account_reg2[j].value = RFIFOL(fd, p + 32);
    }
    sd->status.account_reg2_num = j;
//  PRINTF("chrif: accountreg2\n");

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
    if (char_fd < 0)
        return -1;

    WFIFOW(char_fd, 0) = 0x2b16;
    WFIFOL(char_fd, 2) = char_id;
    WFIFOSET(char_fd, 6);
    return 0;
}

/*==========================================
 * Disconnection of a player (account has been deleted in login-server) by [Yor]
 *------------------------------------------
 */
static
int chrif_accountdeletion(int fd)
{
    int acc;
    dumb_ptr<map_session_data> sd;

    acc = RFIFOL(fd, 2);
    if (battle_config.etc_log)
        PRINTF("chrif_accountdeletion %d.\n", acc);
    sd = map_id2sd(acc);
    if (acc > 0)
    {
        if (sd != NULL)
        {
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            clif_displaymessage(sd->fd,
                                 "Your account has been deleted (disconnection)...");
            clif_setwaitclose(sd->fd); // forced to disconnect for the change
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
int chrif_accountban(int fd)
{
    int acc;
    dumb_ptr<map_session_data> sd;

    acc = RFIFOL(fd, 2);
    if (battle_config.etc_log)
        PRINTF("chrif_accountban %d.\n", acc);
    sd = map_id2sd(acc);
    if (acc > 0)
    {
        if (sd != NULL)
        {
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            if (RFIFOB(fd, 6) == 0)
            {                   // 0: change of statut, 1: ban
                switch (RFIFOL(fd, 7))
                {               // status or final date of a banishment
                    case 1:    // 0 = Unregistered ID
                        clif_displaymessage(sd->fd,
                                             "Your account has 'Unregistered'.");
                        break;
                    case 2:    // 1 = Incorrect Password
                        clif_displaymessage(sd->fd,
                                             "Your account has an 'Incorrect Password'...");
                        break;
                    case 3:    // 2 = This ID is expired
                        clif_displaymessage(sd->fd,
                                             "Your account has expired.");
                        break;
                    case 4:    // 3 = Rejected from Server
                        clif_displaymessage(sd->fd,
                                             "Your account has been rejected from server.");
                        break;
                    case 5:    // 4 = You have been blocked by the GM Team
                        clif_displaymessage(sd->fd,
                                             "Your account has been blocked by the GM Team.");
                        break;
                    case 6:    // 5 = Your Game's EXE file is not the latest version
                        clif_displaymessage(sd->fd,
                                             "Your Game's EXE file is not the latest version.");
                        break;
                    case 7:    // 6 = Your are Prohibited to log in until %s
                        clif_displaymessage(sd->fd,
                                             "Your account has been prohibited to log in.");
                        break;
                    case 8:    // 7 = Server is jammed due to over populated
                        clif_displaymessage(sd->fd,
                                             "Server is jammed due to over populated.");
                        break;
                    case 9:    // 8 = No MSG (actually, all states after 9 except 99 are No MSG, use only this)
                        clif_displaymessage(sd->fd,
                                             "Your account has not more authorised.");
                        break;
                    case 100:  // 99 = This ID has been totally erased
                        clif_displaymessage(sd->fd,
                                             "Your account has been totally erased.");
                        break;
                    default:
                        clif_displaymessage(sd->fd,
                                             "Your account has not more authorised.");
                        break;
                }
            }
            else if (RFIFOB(fd, 6) == 1)
            {
                // 0: change of statut, 1: ban
                TimeT timestamp = static_cast<time_t>(RFIFOL(fd, 7));    // status or final date of a banishment
                char tmpstr[] = WITH_TIMESTAMP("Your account has been banished until ");
                REPLACE_TIMESTAMP(tmpstr, timestamp);
                clif_displaymessage(sd->fd, const_(tmpstr));
            }
            clif_setwaitclose(sd->fd); // forced to disconnect for the change
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
int chrif_recvgmaccounts(int fd)
{
    PRINTF("From login-server: receiving of %d GM accounts information.\n",
            pc_read_gm_account(fd));

    return 0;
}

/*==========================================
 * Request to reload GM accounts and their levels: send to char-server by [Yor]
 *------------------------------------------
 */
int chrif_reloadGMdb(void)
{

    WFIFOW(char_fd, 0) = 0x2af7;
    WFIFOSET(char_fd, 2);

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
            dumb_ptr<map_session_data> pc = bl->as_player();
            struct storage *stor = account2storage2(pc->status.account_id);
            int j;

            for (j = 0; j < MAX_INVENTORY; j++)
                IFIX(pc->status.inventory[j].nameid);
            for (j = 0; j < MAX_CART; j++)
                IFIX(pc->status.cart[j].nameid);
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
            dumb_ptr<mob_data> mob = bl->as_mob();
            for (struct item& itm : mob->lootitemv)
                FIX(itm);
            break;
        }

        case BL::ITEM:
        {
            dumb_ptr<flooritem_data> item = bl->as_item();
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
void ladmin_itemfrob(int fd)
{
    int source_id = RFIFOL(fd, 2);
    int dest_id = RFIFOL(fd, 6);
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
void chrif_parse(int fd)
{
    int packet_len, cmd;

    // only char-server can have an access to here.
    // so, if it isn't the char-server, we disconnect the session (fd != char_fd).
    if (fd != char_fd || session[fd]->eof)
    {
        if (fd == char_fd)
        {
            PRINTF("Map-server can't connect to char-server (connection #%d).\n",
                 fd);
            char_fd = -1;
        }
        delete_session(fd);
        return;
    }

    while (RFIFOREST(fd) >= 2)
    {
        cmd = RFIFOW(fd, 0);
        if (cmd < 0x2af8
            || cmd >=
            0x2af8 +
            (sizeof(packet_len_table) / sizeof(packet_len_table[0]))
            || packet_len_table[cmd - 0x2af8] == 0)
        {

            int r = intif_parse(fd);  // intifに渡す

            if (r == 1)
                continue;       // intifで処理した
            if (r == 2)
                return;       // intifで処理したが、データが足りない

            session[fd]->eof = 1;
            return;
        }
        packet_len = packet_len_table[cmd - 0x2af8];
        if (packet_len == -1)
        {
            if (RFIFOREST(fd) < 4)
                return;
            packet_len = RFIFOW(fd, 2);
        }
        if (RFIFOREST(fd) < packet_len)
            return;

        switch (cmd)
        {
            case 0x2af9:
                chrif_connectack(fd);
                break;
            case 0x2afa:
                ladmin_itemfrob(fd);
                break;
            case 0x2afb:
                chrif_sendmapack(fd);
                break;
            case 0x2afd:
            {
                int id = RFIFOL(fd, 4);
                int login_id2 = RFIFOL(fd, 8);
                TimeT connect_until_time = static_cast<time_t>(RFIFOL(fd, 12));
                short tmw_version = RFIFOW(fd, 16);
                struct mmo_charstatus st {};
                RFIFO_STRUCT(fd, 18, st);
                pc_authok(id, login_id2,
                        connect_until_time, tmw_version,
                        &st);
            }
                break;
            case 0x2afe:
                pc_authfail(RFIFOL(fd, 2));
                break;
            case 0x2b00:
                map_setusers(RFIFOL(fd, 2));
                break;
            case 0x2b03:
                clif_charselectok(RFIFOL(fd, 2));
                break;
            case 0x2b04:
                chrif_recvmap(fd);
                break;
            case 0x2b06:
                chrif_changemapserverack(fd);
                break;
            case 0x2b09:
            {
                int charid = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
                map_addchariddb(charid, name);
            }
                break;
            case 0x2b0b:
                chrif_changedgm(fd);
                break;
            case 0x2b0d:
                chrif_changedsex(fd);
                break;
            case 0x2b0f:
                chrif_char_ask_name_answer(fd);
                break;
            case 0x2b11:
                chrif_accountreg2(fd);
                break;
            case 0x2b12:
                chrif_divorce(RFIFOL(fd, 2), RFIFOL(fd, 6));
                break;
            case 0x2b13:
                chrif_accountdeletion(fd);
                break;
            case 0x2b14:
                chrif_accountban(fd);
                break;
            case 0x2b15:
                chrif_recvgmaccounts(fd);
                break;

            default:
                if (battle_config.error_log)
                    PRINTF("chrif_parse : unknown packet %d %d\n", fd,
                            RFIFOW(fd, 0));
                session[fd]->eof = 1;
                return;
        }
        RFIFOSKIP(fd, packet_len);
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

    if (char_fd <= 0 || session[char_fd] == NULL)
        return;

    WFIFOW(char_fd, 0) = 0x2aff;
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (sd && sd->state.auth &&
            !((battle_config.hide_GM_session
               || sd->state.shroud_active
               || bool(sd->status.option & Option::HIDE)) && pc_isGM(sd)))
        {
            WFIFOL(char_fd, 6 + 4 * users) = sd->status.char_id;
            users++;
        }
    }
    WFIFOW(char_fd, 2) = 6 + 4 * users;
    WFIFOW(char_fd, 4) = users;
    WFIFOSET(char_fd, 6 + 4 * users);
}

/*==========================================
 * timer関数
 * char鯖との接続を確認し、もし切れていたら再度接続する
 *------------------------------------------
 */
static
void check_connect_char_server(TimerData *, tick_t)
{
    if (char_fd <= 0 || session[char_fd] == NULL)
    {
        PRINTF("Attempt to connect to char-server...\n");
        chrif_state = 0;
        if ((char_fd = make_connection(char_ip, char_port)) < 0)
            return;
        session[char_fd]->func_parse = chrif_parse;
        realloc_fifo(char_fd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

        chrif_connect(char_fd);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_chrif (void)
{
    Timer(gettick() + std::chrono::seconds(1),
            check_connect_char_server,
            std::chrono::seconds(10)
    ).detach();
    Timer(gettick() + std::chrono::seconds(1),
            send_users_tochar,
            std::chrono::seconds(5)
    ).detach();

    return 0;
}
