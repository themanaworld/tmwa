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

#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/ip.hpp"
#include "../net/socket.hpp"
#include "../net/timer.hpp"
#include "../net/timestamp-utils.hpp"

#include "../proto2/char-map.hpp"

#include "../mmo/human_time_diff.hpp"
#include "../high/mmo.hpp"

#include "../wire/packets.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "map_conf.hpp"
#include "npc.hpp"
#include "pc.hpp"
#include "storage.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
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

    Packet_Payload<0x2b01> payload_01;
    payload_01.account_id = block_to_account(sd->bl_id);
    payload_01.char_id = sd->char_id_;
    payload_01.char_key = sd->status_key;
    payload_01.char_data = sd->status;
    send_ppacket<0x2b01>(char_session, payload_01);

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
    Packet_Fixed<0x2af8> fixed_f8;
    fixed_f8.account_name = map_conf.userid;
    fixed_f8.account_pass = map_conf.passwd;
    fixed_f8.unused = 0;
    fixed_f8.ip = map_conf.map_ip;
    fixed_f8.port = map_conf.map_port;
    send_fpacket<0x2af8, 60>(s, fixed_f8);

    return 0;
}

/*==========================================
 * マップ送信
 *------------------------------------------
 */
static
int chrif_sendmap(Session *s)
{
    std::vector<Packet_Repeat<0x2afa>> repeat_fa;
    for (auto& pair : maps_db)
    {
        map_abstract *ma = pair.second.get();
        if (!ma->gat)
            continue;
        Packet_Repeat<0x2afa> info;
        info.map_name = ma->name_;
        repeat_fa.push_back(info);
    }
    send_packet_repeatonly<0x2afa, 4, 16>(s, repeat_fa);

    return 0;
}

/*==========================================
 * マップ受信
 *------------------------------------------
 */
static
int chrif_recvmap(Session *, Packet_Head<0x2b04> head, const std::vector<Packet_Repeat<0x2b04>>& repeat)
{
    if (chrif_state < 2)        // まだ準備中
        return -1;

    IP4Address ip = head.ip;
    uint16_t port = head.port;
    for (const Packet_Repeat<0x2b04>& i : repeat)
    {
        MapName map = i.map_name;
        map_setipport(map, ip, port);
    }
    if (battle_config.etc_log)
        PRINTF("recv map on %s:%d (%zu maps)\n"_fmt,
                ip, port, repeat.size());

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

    if (!char_session)
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

    Packet_Fixed<0x2b05> fixed_05;
    fixed_05.account_id = block_to_account(sd->bl_id);
    fixed_05.login_id1 = sd->login_id1;
    fixed_05.login_id2 = sd->login_id2;
    fixed_05.char_id = sd->status_key.char_id;
    fixed_05.map_name = name;
    fixed_05.x = x;
    fixed_05.y = y;
    fixed_05.map_ip = ip;
    fixed_05.map_port = port;
    fixed_05.sex = sd->status.sex;
    fixed_05.client_ip = s_ip;
    send_fpacket<0x2b05, 49>(char_session, fixed_05);

    return 0;
}

/*==========================================
 * マップ鯖間移動ack
 *------------------------------------------
 */
static
int chrif_changemapserverack(Session *, const Packet_Fixed<0x2b06>& fixed)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(fixed.account_id));

    if (sd == nullptr || sd->status_key.char_id != fixed.char_id)
        return -1;

    // I am fairly certain that this is not possible
    if (fixed.error == 1)
    {
        if (battle_config.error_log)
            PRINTF("Changing the map server failed.\n"_fmt);
        pc_authfail(sd->status_key.account_id);
        return 0;
    }
    MapName mapname = fixed.map_name;
    uint16_t x = fixed.x;
    uint16_t y = fixed.y;
    IP4Address ip = fixed.map_ip;
    uint16_t port = fixed.map_port;
    clif_changemapserver(sd, mapname, x, y, ip, port);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_connectack(Session *s, const Packet_Fixed<0x2af9>& fixed)
{
    if (fixed.code)
    {
        PRINTF("Connecting to char-server failed %d.\n"_fmt, fixed.code);
        exit(1);
    }
    PRINTF("Connected to char-server (connection #%d).\n"_fmt, s);
    chrif_state = 1;

    chrif_sendmap(s);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int chrif_sendmapack(Session *, Packet_Fixed<0x2afb> fixed)
{
    if (fixed.unknown) //impossible
    {
        PRINTF("chrif: sending the map list to char-server failed %d\n"_fmt,
                fixed.unknown);
        exit(1);
    }

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
            Packet_Fixed<0x2afc> fixed_fc;
            fixed_fc.account_id = block_to_account(sd->bl_id);
            fixed_fc.char_id = sd->char_id_;
            fixed_fc.login_id1 = sd->login_id1;
            fixed_fc.login_id2 = sd->login_id2;
            fixed_fc.ip = s->client_ip;
            send_fpacket<0x2afc, 22>(char_session, fixed_fc);
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

    Packet_Fixed<0x2b02> fixed_02;
    fixed_02.account_id = block_to_account(sd->bl_id);
    fixed_02.login_id1 = sd->login_id1;
    fixed_02.login_id2 = sd->login_id2;
    fixed_02.ip = s_ip;
    send_fpacket<0x2b02, 18>(char_session, fixed_02);

    return 0;
}

/*==========================================
 * Change Email
 *------------------------------------------
 */
void chrif_changeemail(AccountId id, AccountEmail actual_email,
        AccountEmail new_email)
{
    if (!char_session)
        return;

    if (battle_config.etc_log)
        PRINTF("chrif_changeemail: account: %d, actual_email: '%s', new_email: '%s'.\n"_fmt,
                id, actual_email, new_email);

    Packet_Fixed<0x2b0c> fixed_0c;
    fixed_0c.account_id = id;
    fixed_0c.old_email = actual_email;
    fixed_0c.new_email = new_email;
    send_fpacket<0x2b0c, 86>(char_session, fixed_0c);
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
void chrif_char_ask_name(AccountId id, CharName character_name, short operation_type,
        HumanTimeDiff modif)
{
    if (!char_session)
        return;

    Packet_Fixed<0x2b0e> fixed_0e;
    fixed_0e.account_id = id;   // who ask, or nobody
    fixed_0e.char_name = character_name;
    fixed_0e.operation = operation_type;  // type of operation
    if (operation_type == 2)
        fixed_0e.ban_add = modif;
    PRINTF("chrif: sent 0x2b0e\n"_fmt);
    send_fpacket<0x2b0e, 44>(char_session, fixed_0e);
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
 *   0: login-server request done
 *   1: player not found
 *   2: gm level too low
 *   3: login-server offline
 *------------------------------------------
 */
static
int chrif_char_ask_name_answer(Session *, const Packet_Fixed<0x2b0f>& fixed)
{
    AccountId acc = fixed.account_id;       // who asked, or nobody
    CharName player_name = fixed.char_name;

    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(acc));
    if (acc && sd != nullptr)
    {
        AString output;
        if (fixed.error == 1)   // player not found
            output = STRPRINTF("The player, '%s,' doesn't exist."_fmt,
                    player_name);
        else
        {
            switch (fixed.operation)
            {
                case 1:        // block
                    switch (fixed.error)
                    {
                        case 0:    // login-server request done
                            output = STRPRINTF(
                                    "Login-server has been asked to block '%s'."_fmt,
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level doesn't authorize you to block the player '%s'."_fmt,
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline, so it's impossible to block '%s'."_fmt,
                                    player_name);
                            break;
                    }
                    break;
                case 2:        // ban
                    switch (fixed.error)
                    {
                        case 0:    // login-server request done
                            output = STRPRINTF(
                                    "Login-server has been asked to ban '%s'."_fmt,
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level doesn't authorize you to ban '%s'."_fmt,
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline, so it's impossible to ban '%s'."_fmt,
                                    player_name);
                            break;
                    }
                    break;
                case 3:        // unblock
                    switch (fixed.error)
                    {
                        case 0:    // login-server request done
                            output = STRPRINTF(
                                    "Login-server has been asked to unblock '%s'."_fmt,
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level doesn't authorize you to unblock '%s'."_fmt,
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline, so it's impossible to unblock '%s'."_fmt,
                                    player_name);
                            break;
                    }
                    break;
                case 4:        // unban
                    switch (fixed.error)
                    {
                        case 0:    // login-server request done
                            output = STRPRINTF(
                                    "Login-server has been asked to unban '%s'."_fmt,
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level doesn't authorize you to unban '%s'."_fmt,
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline, so it's impossible to unban '%s'."_fmt,
                                    player_name);
                            break;
                    }
                    break;
                case 5:        // changesex
                    switch (fixed.error)
                    {
                        case 0:    // login-server request done
                            output = STRPRINTF(
                                    "Login-server has been asked to change the sex of '%s'."_fmt,
                                    player_name);
                            break;
                            //case 1: // player not found
                        case 2:    // gm level too low
                            output = STRPRINTF(
                                    "Your GM level doesn't authorize you to change the sex of '%s'."_fmt,
                                    player_name);
                            break;
                        case 3:    // login-server offline
                            output = STRPRINTF(
                                    "Login-server is offline, so it's impossible to change the sex of '%s'."_fmt,
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
        PRINTF("chrif_char_ask_name_answer failed because the player is not online.\n"_fmt);

    return 0;
}

/*==========================================
 * 性別変化終了 (modified by Yor)
 *------------------------------------------
 */
static
void chrif_changedsex(Session *, const Packet_Fixed<0x2b0d>& fixed)
{
    dumb_ptr<map_session_data> sd;

    AccountId acc = fixed.account_id;
    SEX sex = fixed.sex;
    if (battle_config.etc_log)
        PRINTF("chrif_changedsex %d.\n"_fmt, acc);
    sd = map_id2sd(account_to_block(acc));
    if (acc)
    {
        if (sd != nullptr && sd->status.sex != sex)
        {
            sd->sex = sd->status.sex = sex;
            // to avoid any problem with equipment and invalid sex, equipment is unequiped.
            for (IOff0 i : IOff0::iter())
            {
                if (sd->status.inventory[i].nameid
                    && bool(sd->status.inventory[i].equip))
                    pc_unequipitem(sd, i, CalcStatus::LATER);
            }
            pc_calcstatus(sd, 0);
            // save character
            chrif_save(sd);
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            // do same modify in login-server for the account, but no in char-server (it ask again login_id1 to login, and don't remember it)
            clif_fixpcpos(sd); // use clif_set0078_main_1d8 to send new sex to the client
        }
    }
    else
    {
        if (sd != nullptr)
        {
            PRINTF("chrif_changedsex failed.\n"_fmt);
        }
    }
}

/*==========================================
 * アカウント変数保存要求
 *------------------------------------------
 */
int chrif_saveaccountreg2(dumb_ptr<map_session_data> sd)
{
    nullpo_retr(-1, sd);

    if (!char_session)
        return -1;

    std::vector<Packet_Repeat<0x2b10>> repeat_10;
    for (size_t j = 0; j < sd->status.account_reg2_num; j++)
    {
        GlobalReg *reg = &sd->status.account_reg2[j];
        if (reg->str && reg->value != 0)
        {
            Packet_Repeat<0x2b10> info;
            info.name = reg->str;
            info.value = reg->value;
            repeat_10.push_back(info);
        }
    }

    Packet_Head<0x2b10> head_10;
    head_10.account_id = block_to_account(sd->bl_id);
    send_vpacket<0x2b10, 8, 36>(char_session, head_10, repeat_10);

    return 0;
}

/*==========================================
 * アカウント変数通知
 *------------------------------------------
 */
static
int chrif_accountreg2(Session *, const Packet_Head<0x2b11>& head, const std::vector<Packet_Repeat<0x2b11>>& repeat)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(head.account_id));
    if (sd == nullptr)
        return 1;

    size_t jlim = std::min(ACCOUNT_REG2_NUM, repeat.size());
    for (size_t j = 0; j < jlim; j++)
    {
        sd->status.account_reg2[j].str = repeat[j].name;
        sd->status.account_reg2[j].value = repeat[j].value;
    }
    sd->status.account_reg2_num = jlim;

    return 0;
}

/*==========================================
 * Divorce request from char server
 * triggered on account deletion or as an
 * ack from a map-server divorce request
 *------------------------------------------
 */
static
int chrif_divorce(CharId char_id, CharId partner_id)
{
    dumb_ptr<map_session_data> sd = nullptr;

    if (!char_id || !partner_id)
        return 0;

    sd = map_nick2sd(map_charid2nick(char_id));
    if (sd && sd->status.partner_id == partner_id)
    {
        sd->status.partner_id = CharId();
    }

    sd = map_nick2sd(map_charid2nick(partner_id));
    if (sd && sd->status.partner_id == char_id)
    {
        sd->status.partner_id = CharId();
    }

    return 0;
}

/*==========================================
 * Tell character server someone is divorced
 * Needed to divorce when partner is not connected to map server
 *-------------------------------------
 */
int chrif_send_divorce(CharId char_id)
{
    if (!char_session)
        return -1;

    Packet_Fixed<0x2b16> fixed_16;
    fixed_16.char_id = char_id;
    send_fpacket<0x2b16, 6>(char_session, fixed_16);
    return 0;
}

/*==========================================
 * Disconnection of a player (account has been deleted in login-server) by [Yor]
 *------------------------------------------
 */
static
int chrif_accountdeletion(Session *, const Packet_Fixed<0x2b13>& fixed)
{
    dumb_ptr<map_session_data> sd;

    AccountId acc = fixed.account_id;
    if (battle_config.etc_log)
        PRINTF("chrif_accountdeletion %d.\n"_fmt, acc);
    sd = map_id2sd(account_to_block(acc));
    if (acc)
    {
        if (sd != nullptr)
        {
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            clif_displaymessage(sd->sess,
                                 "Your account has been deleted. You will now be disconnected..."_s);
            clif_setwaitclose(sd->sess); // forced to disconnect for the change
        }
    }
    else
    {
        if (sd != nullptr)
            PRINTF("chrif_accountdeletion failed because the player is not online.\n"_fmt);
    }

    return 0;
}

/*==========================================
 * Disconnection of a player (account has been banned of has a status, from login-server) by [Yor]
 *------------------------------------------
 */
static
int chrif_accountban(Session *, const Packet_Fixed<0x2b14>& fixed)
{
    dumb_ptr<map_session_data> sd;

    AccountId acc = fixed.account_id;
    if (battle_config.etc_log)
        PRINTF("chrif_accountban %d.\n"_fmt, acc);
    sd = map_id2sd(account_to_block(acc));
    if (acc)
    {
        if (sd != nullptr)
        {
            sd->login_id1++;    // change identify, because if player come back in char within the 5 seconds, he can change its characters
            if (fixed.ban_not_status == 0)
            {                   // 0: change of statut, 1: ban
                switch (static_cast<time_t>(fixed.status_or_ban_until))
                {               // status or final date of a banishment
                    case 1:    // 0 = Unregistered ID
                        clif_displaymessage(sd->sess,
                                             "Your account has an unregistered ID."_s);
                        break;
                    case 2:    // 1 = Incorrect Password
                        clif_displaymessage(sd->sess,
                                             "Your password is incorrect."_s);
                        break;
                    case 3:    // 2 = This ID is expired
                        clif_displaymessage(sd->sess,
                                             "Your account has expired."_s);
                        break;
                    case 4:    // 3 = Rejected from Server
                        clif_displaymessage(sd->sess,
                                             "Your account has been rejected by the server."_s);
                        break;
                    case 5:    // 4 = You have been blocked by the GM Team
                        clif_displaymessage(sd->sess,
                                             "Your account has been blocked by the GM Team."_s);
                        break;
                    case 6:    // 5 = Your Game's EXE file is not the latest version
                        clif_displaymessage(sd->sess,
                                             "You need to update your client."_s);
                        break;
                    case 7:    // 6 = Your are Prohibited to log in until %s
                        clif_displaymessage(sd->sess,
                                             "Your account has been prohibited from logging in."_s);
                        break;
                    case 8:    // 7 = Server is jammed due to over populated
                        clif_displaymessage(sd->sess,
                                             "The server is overpopulated."_s);
                        break;
                    case 9:    // 8 = No MSG (actually, all states after 9 except 99 are No MSG, use only this)
                        clif_displaymessage(sd->sess,
                                             "Your account must be authorized."_s);
                        break;
                    case 100:  // 99 = This ID has been totally erased
                        clif_displaymessage(sd->sess,
                                             "Your account has been totally erased."_s);
                        break;
                    default:
                        clif_displaymessage(sd->sess,
                                             "Your account must be authorized."_s);
                        break;
                }
            }
            else if (fixed.ban_not_status == 1)
            {
                // 0: change of statut, 1: ban
                const TimeT timestamp = fixed.status_or_ban_until;    // status or final date of a banishment
                timestamp_seconds_buffer buffer;
                stamp_time(buffer, &timestamp);
                AString tmpstr = STRPRINTF("Your account has been banished until %s"_fmt, buffer);
                clif_displaymessage(sd->sess, tmpstr);
            }
            clif_setwaitclose(sd->sess); // forced to disconnect for the change
        }
    }
    else
    {
        if (sd != nullptr)
            PRINTF("chrif_accountban failed because the player is not online.\n"_fmt);
    }

    return 0;
}

/*==========================================
 * Receiving GM accounts and their levels from char-server by [Yor]
 *------------------------------------------
 */
static
int chrif_recvgmaccounts(Session *s, const std::vector<Packet_Repeat<0x2b15>>& repeat)
{
    PRINTF("Receiving information on %d GM accounts from login-server.\n"_fmt,
            pc_read_gm_account(s, repeat));

    return 0;
}

static
void chrif_delete(Session *s)
{
    assert (s == char_session);
    PRINTF("map-server can't connect to char-server (connection #%d).\n"_fmt,
            s);
    char_session = nullptr;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void chrif_parse(Session *s)
{
    assert (s == char_session);

    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(s, &packet_id))
    {
        switch (packet_id)
        {
            case 0x2af9:
            {
                Packet_Fixed<0x2af9> fixed;
                rv = recv_fpacket<0x2af9, 3>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_connectack(s, fixed);
                break;
            }
            case 0x2afb:
            {
                Packet_Fixed<0x2afb> fixed;
                rv = recv_fpacket<0x2afb, 27>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_sendmapack(s, fixed);
                break;
            }
            case 0x2afd:
            {
                Packet_Payload<0x2afd> payload;
                rv = recv_ppacket<0x2afd>(s, payload);
                if (rv != RecvResult::Complete)
                    break;

                AccountId id = payload.account_id;
                int login_id2 = payload.login_id2;
                short client_version = payload.packet_client_version;
                CharKey st_key = payload.char_key;
                CharData st_data = payload.char_data;
                pc_authok(id, login_id2,
                        client_version,
                        &st_key, &st_data);
                break;
            }
            case 0x2afe:
            {
                Packet_Fixed<0x2afe> fixed;
                rv = recv_fpacket<0x2afe, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                pc_authfail(fixed.account_id);
                break;
            }
            case 0x2b00:
            {
                Packet_Fixed<0x2b00> fixed;
                rv = recv_fpacket<0x2b00, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                map_setusers(fixed.users);
                break;
            }
            case 0x2b03:
            {
                Packet_Fixed<0x2b03> fixed;
                rv = recv_fpacket<0x2b03, 7>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                clif_charselectok(account_to_block(fixed.account_id));
                break;
            }
            case 0x2b04:
            {
                Packet_Head<0x2b04> head;
                std::vector<Packet_Repeat<0x2b04>> repeat;
                rv = recv_vpacket<0x2b04, 10, 16>(s, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                chrif_recvmap(s, head, repeat);
                break;
            }
            case 0x2b06:
            {
                Packet_Fixed<0x2b06> fixed;
                rv = recv_fpacket<0x2b06, 44>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_changemapserverack(s, fixed);
                break;
            }
            case 0x2b0d:
            {
                Packet_Fixed<0x2b0d> fixed;
                rv = recv_fpacket<0x2b0d, 7>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_changedsex(s, fixed);
                break;
            }
            case 0x2b0f:
            {
                Packet_Fixed<0x2b0f> fixed;
                rv = recv_fpacket<0x2b0f, 34>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_char_ask_name_answer(s, fixed);
                break;
            }
            case 0x2b11:
            {
                Packet_Head<0x2b11> head;
                std::vector<Packet_Repeat<0x2b11>> repeat;
                rv = recv_vpacket<0x2b11, 8, 36>(s, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                chrif_accountreg2(s, head, repeat);
                break;
            }
            case 0x2b12:
            {
                Packet_Fixed<0x2b12> fixed;
                rv = recv_fpacket<0x2b12, 10>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_divorce(fixed.char_id, fixed.partner_id);
                break;
            }
            case 0x2b13:
            {
                Packet_Fixed<0x2b13> fixed;
                rv = recv_fpacket<0x2b13, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_accountdeletion(s, fixed);
                break;
            }
            case 0x2b14:
            {
                Packet_Fixed<0x2b14> fixed;
                rv = recv_fpacket<0x2b14, 11>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                chrif_accountban(s, fixed);
                break;
            }
            case 0x2b15:
            {
                std::vector<Packet_Repeat<0x2b15>> repeat;
                rv = recv_packet_repeatonly<0x2b15, 4, 5>(s, repeat);
                if (rv != RecvResult::Complete)
                    break;

                chrif_recvgmaccounts(s, repeat);
                break;
            }
            default:
            {
                RecvResult r = intif_parse(s, packet_id);

                if (r == RecvResult::Complete)
                    break;
                if (r == RecvResult::Incomplete)
                    return;

                if (battle_config.error_log)
                    PRINTF("chrif_parse: unknown packet %d %d\n"_fmt, s,
                            packet_id);
                s->set_eof();
                return;
            }
        }
    }
    if (rv == RecvResult::Error)
        s->set_eof();
}

/*==========================================
 * timer関数
 * 今このmap鯖に繋がっているクライアント人数をchar鯖へ送る
 *------------------------------------------
 */
static
void send_users_tochar(TimerData *, tick_t)
{
    if (!char_session)
        return;

    Packet_Head<0x2aff> head_ff;
    std::vector<Packet_Repeat<0x2aff>> repeat_ff;
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
        if (sd && sd->state.auth && !sd->state.connect_new &&
            !((battle_config.hide_GM_session
               || sd->state.shroud_active
               || bool(sd->status.option & Opt0::HIDE)) && pc_isGM(sd)))
        {
            Packet_Repeat<0x2aff> info;
            info.char_id = sd->status_key.char_id;
            repeat_ff.push_back(info);
        }
    }
    head_ff.users = repeat_ff.size();
    send_vpacket<0x2aff, 6, 4>(char_session, head_ff, repeat_ff);
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
        PRINTF("Attempting to connect to char-server...\n"_fmt);
        chrif_state = 0;
        char_session = make_connection(map_conf.char_ip, map_conf.char_port,
                SessionParsers{.func_parse= chrif_parse, .func_delete= chrif_delete});
        if (!char_session)
            return;
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
    Timer(gettick() + 1_s,
            check_connect_char_server,
            10_s
    ).detach();
    Timer(gettick() + 1_s,
            send_users_tochar,
            5_s
    ).detach();
}
} // namespace map
} // namespace tmwa
