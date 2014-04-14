#include "inter.hpp"
//    inter.cpp - Internal server.
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

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/extract.hpp"
#include "../mmo/socket.hpp"
#include "../mmo/timer.hpp"
#include "../mmo/utils.hpp"

#include "char.hpp"
#include "int_party.hpp"
#include "int_storage.hpp"

#include "../poison.hpp"

static
AString accreg_txt = "save/accreg.txt"_s;

struct accreg
{
    int account_id, reg_num;
    Array<struct global_reg, ACCOUNT_REG_NUM> reg;
};
static
Map<int, struct accreg> accreg_db;

int party_share_level = 10;

// 受信パケット長リスト
static
int inter_recv_packet_length[] =
{
    -1, -1, 7, -1, -1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, -1, 0, 0, 0, 0, 0, 0, 10, -1, 0, 0, 0, 0, 0, 0,
    72, 6, 52, 14, 10, 29, 6, -1, 34, 0, 0, 0, 0, 0, 0, 0,
    -1, 6, -1, 0, 55, 19, 6, -1, 14, -1, -1, -1, 14, 19, 186, -1,
    5, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    48, 14, -1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

//--------------------------------------------------------

// アカウント変数を文字列へ変換
static
AString inter_accreg_tostr(struct accreg *reg)
{
    assert(reg->reg_num < ACCOUNT_REG_NUM);
    MString str;
    str += STRPRINTF("%d\t"_fmt, reg->account_id);
    for (int j = 0; j < reg->reg_num; j++)
        str += STRPRINTF("%s,%d "_fmt, reg->reg[j].str, reg->reg[j].value);
    return AString(str);
}

// アカウント変数を文字列から変換
static
bool extract(XString str, struct accreg *reg)
{
    std::vector<struct global_reg> vars;
    if (!extract(str,
                record<'\t'>(
                    &reg->account_id,
                    vrec<' '>(&vars))))
        return false;
    if (reg->account_id <= 0)
        return false;

    if (vars.size() > ACCOUNT_REG_NUM)
        return false;
    std::copy(vars.begin(), vars.end(), reg->reg.begin());
    reg->reg_num = vars.size();
    return true;
}

// アカウント変数の読み込み
static
void inter_accreg_init(void)
{
    int c = 0;

    io::ReadFile in(accreg_txt);
    if (!in.is_open())
        return;
    AString line;
    while (in.getline(line))
    {
        struct accreg reg {};
        if (extract(line, &reg))
        {
            accreg_db.insert(reg.account_id, reg);
        }
        else
        {
            PRINTF("inter: accreg: broken data [%s] line %d\n"_fmt, accreg_txt,
                    c);
        }
        c++;
    }
}

// アカウント変数のセーブ用
static
void inter_accreg_save_sub(struct accreg *reg, io::WriteFile& fp)
{
    if (reg->reg_num > 0)
    {
        AString line = inter_accreg_tostr(reg);
        fp.put_line(line);
    }
}

// アカウント変数のセーブ
static
int inter_accreg_save(void)
{
    io::WriteLock fp(accreg_txt);
    if (!fp.is_open())
    {
        PRINTF("int_accreg: cant write [%s] !!! data is lost !!!\n"_fmt,
                accreg_txt);
        return 1;
    }
    for (auto& pair : accreg_db)
        inter_accreg_save_sub(&pair.second, fp);

    return 0;
}

bool inter_config(XString w1, ZString w2)
{
    {
        if (w1 == "storage_txt"_s)
        {
            storage_txt = w2;
        }
        else if (w1 == "party_txt"_s)
        {
            party_txt = w2;
        }
        else if (w1 == "accreg_txt"_s)
        {
            accreg_txt = w2;
        }
        else if (w1 == "party_share_level"_s)
        {
            party_share_level = atoi(w2.c_str());
            if (party_share_level < 0)
                party_share_level = 0;
        }
        else
        {
            return false;
        }
    }

    return true;
}

// セーブ
void inter_save(void)
{
    inter_party_save();
    inter_storage_save();
    inter_accreg_save();
}

// 初期化
void inter_init2()
{
    inter_party_init();
    inter_storage_init();
    inter_accreg_init();
}

//--------------------------------------------------------
// sended packets to map-server

// GMメッセージ送信
static
void mapif_GMmessage(XString mes)
{
    size_t str_len = mes.size() + 1;
    size_t msg_len = str_len + 4;
    uint8_t buf[msg_len];

    WBUFW(buf, 0) = 0x3800;
    WBUFW(buf, 2) = msg_len;
    WBUF_STRING(buf, 4, mes, str_len);
    mapif_sendall(buf, msg_len);
}

// Wisp/page transmission to correct map-server
static
void mapif_wis_message(Session *tms, CharName src, CharName dst, XString msg)
{
    const CharPair *mcs = search_character(src);
    assert (mcs);

    size_t str_size = msg.size() + 1;
    uint8_t buf[56 + str_size];

    WBUFW(buf, 0) = 0x3801;
    WBUFW(buf, 2) = 56 + str_size;
    WBUFL(buf, 4) = mcs->key.char_id; // formerly, whisper ID
    WBUF_STRING(buf, 8, src.to__actual(), 24);
    WBUF_STRING(buf, 32, dst.to__actual(), 24);
    WBUF_STRING(buf, 56, msg, str_size);
    mapif_send(tms, buf, WBUFW(buf, 2));
}

// Wisp/page transmission result to map-server
static
void mapif_wis_end(Session *sms, CharName sender, int flag)
{
    uint8_t buf[27];

    WBUFW(buf, 0) = 0x3802;
    WBUF_STRING(buf, 2, sender.to__actual(), 24);
    WBUFB(buf, 26) = flag;     // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    mapif_send(sms, buf, 27);
}

// アカウント変数送信
static
void mapif_account_reg(Session *s)
{
    size_t len = RFIFOW(s, 2);
    uint8_t buf[len];
    RFIFO_BUF_CLONE(s, buf, len);
    WBUFW(buf, 0) = 0x3804;
    mapif_sendallwos(s, buf, WBUFW(buf, 2));
}

// アカウント変数要求返信
static
void mapif_account_reg_reply(Session *s, int account_id)
{
    struct accreg *reg = accreg_db.search(account_id);

    WFIFOW(s, 0) = 0x3804;
    WFIFOL(s, 4) = account_id;
    if (reg == NULL)
    {
        WFIFOW(s, 2) = 8;
    }
    else
    {
        assert (reg->reg_num < ACCOUNT_REG_NUM);
        int j, p;
        for (j = 0, p = 8; j < reg->reg_num; j++, p += 36)
        {
            WFIFO_STRING(s, p, reg->reg[j].str, 32);
            WFIFOL(s, p + 32) = reg->reg[j].value;
        }
        WFIFOW(s, 2) = p;
    }
    WFIFOSET(s, WFIFOW(s, 2));
}

//--------------------------------------------------------
// received packets from map-server

// GMメッセージ送信
static
void mapif_parse_GMmessage(Session *s)
{
    size_t msg_len = RFIFOW(s, 2);
    size_t str_len = msg_len - 4;
    AString buf = RFIFO_STRING(s, 4, str_len);

    mapif_GMmessage(buf);
}

// Wisp/page request to send
static
void mapif_parse_WisRequest(Session *sms)
{
    if (RFIFOW(sms, 2) - 52 <= 0)
    {                           // normaly, impossible, but who knows...
        PRINTF("inter: Wis message doesn't exist.\n"_fmt);
        return;
    }

    CharName from = stringish<CharName>(RFIFO_STRING<24>(sms, 4));
    CharName to = stringish<CharName>(RFIFO_STRING<24>(sms, 28));

    // search if character exists before to ask all map-servers
    const CharPair *mcs = search_character(to);
    if (!mcs)
    {
        uint8_t buf[27];
        WBUFW(buf, 0) = 0x3802;
        WBUF_STRING(buf, 2, from.to__actual(), 24);
        WBUFB(buf, 26) = 1;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        mapif_send(sms, buf, 27);
        // Character exists. So, ask all map-servers
    }
    else
    {
        // to be sure of the correct name, rewrite it
        to = mcs->key.name;
        // if source is destination, don't ask other servers.
        if (from == to)
        {
            uint8_t buf[27];
            WBUFW(buf, 0) = 0x3802;
            WBUF_STRING(buf, 2, from.to__actual(), 24);
            WBUFB(buf, 26) = 1;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            mapif_send(sms, buf, 27);
        }
        else
        {
            size_t len = RFIFOW(sms, 2) - 52;
            Session *tms = server_for(mcs); // for to
            AString msg = RFIFO_STRING(sms, 52, len);
            if (tms)
            {
                mapif_wis_message(tms, from, to, msg);
            }
            else
            {
                mapif_wis_end(sms, from, 1);
            }
        }
    }
}

// Wisp/page transmission result
static
int mapif_parse_WisReply(Session *tms)
{
    int id = RFIFOL(tms, 2), flag = RFIFOB(tms, 6);

    const CharPair *smcs = search_character_id(id);
    CharName from = smcs->key.name;
    Session *sms = server_for(smcs);
    {
        mapif_wis_end(sms, from, flag);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    }

    return 0;
}

// Received wisp message from map-server for ALL gm (just copy the message and resends it to ALL map-servers)
static
void mapif_parse_WisToGM(Session *s)
{
    size_t len = RFIFOW(s, 2);
    uint8_t buf[len];
    // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B

    RFIFO_BUF_CLONE(s, buf, len);
    WBUFW(buf, 0) = 0x3803;
    mapif_sendall(buf, len);
}

// アカウント変数保存要求
static
void mapif_parse_AccReg(Session *s)
{
    int j, p;
    struct accreg *reg = accreg_db.search(RFIFOL(s, 4));

    if (reg == NULL)
    {
        int account_id = RFIFOL(s, 4);
        reg = accreg_db.init(account_id);
        reg->account_id = account_id;
    }

    for (j = 0, p = 8; j < ACCOUNT_REG_NUM && p < RFIFOW(s, 2);
         j++, p += 36)
    {
        reg->reg[j].str = stringish<VarName>(RFIFO_STRING<32>(s, p));
        reg->reg[j].value = RFIFOL(s, p + 32);
    }
    reg->reg_num = j;

    // 他のMAPサーバーに送信
    mapif_account_reg(s);
}

// アカウント変数送信要求
static
void mapif_parse_AccRegRequest(Session *s)
{
    mapif_account_reg_reply(s, RFIFOL(s, 2));
}

//--------------------------------------------------------

// map server からの通信（１パケットのみ解析すること）
// エラーなら0(false)、処理できたなら1、
// パケット長が足りなければ2をかえさなければならない
int inter_parse_frommap(Session *ms)
{
    int cmd = RFIFOW(ms, 0);
    int len = 0;

    // inter鯖管轄かを調べる
    if (cmd < 0x3000
        || cmd >=
        0x3000 +
        (sizeof(inter_recv_packet_length) /
         sizeof(inter_recv_packet_length[0])))
        return 0;

    // パケット長を調べる
    if ((len =
         inter_check_length(ms,
                             inter_recv_packet_length[cmd - 0x3000])) == 0)
        return 2;

    switch (cmd)
    {
        case 0x3000:
            mapif_parse_GMmessage(ms);
            break;
        case 0x3001:
            mapif_parse_WisRequest(ms);
            break;
        case 0x3002:
            mapif_parse_WisReply(ms);
            break;
        case 0x3003:
            mapif_parse_WisToGM(ms);
            break;
        case 0x3004:
            mapif_parse_AccReg(ms);
            break;
        case 0x3005:
            mapif_parse_AccRegRequest(ms);
            break;
        default:
            if (inter_party_parse_frommap(ms))
                break;
            if (inter_storage_parse_frommap(ms))
                break;
            return 0;
    }
    RFIFOSKIP(ms, len);

    return 1;
}

// RFIFOのパケット長確認
// 必要パケット長があればパケット長、まだ足りなければ0
int inter_check_length(Session *s, int length)
{
    if (length == -1)
    {                           // 可変パケット長
        if (RFIFOREST(s) < 4) // パケット長が未着
            return 0;
        length = RFIFOW(s, 2);
    }

    if (RFIFOREST(s) < length)    // パケットが未着
        return 0;

    return length;
}
