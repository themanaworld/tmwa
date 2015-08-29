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

#include <vector>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/array.hpp"
#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/span.hpp"
#include "../io/write.hpp"

#include "../mmo/config_parse.hpp"

#include "../proto2/char-map.hpp"

#include "../high/extract_mmo.hpp"
#include "../high/mmo.hpp"

#include "../wire/packets.hpp"

#include "char.hpp"
#include "globals.hpp"
#include "inter_conf.hpp"
#include "int_party.hpp"
#include "int_guild.hpp"
#include "int_storage.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace char_
{
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
bool impl_extract(XString str, struct accreg *reg)
{
    std::vector<GlobalReg> vars;
    if (!extract(str,
                record<'\t'>(
                    &reg->account_id,
                    vrec<' '>(&vars))))
        return false;
    if (!reg->account_id)
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

    io::ReadFile in(inter_conf.accreg_txt);
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
            PRINTF("inter: accreg: broken data [%s] line %d\n"_fmt, inter_conf.accreg_txt,
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
    io::WriteLock fp(inter_conf.accreg_txt);
    if (!fp.is_open())
    {
        PRINTF("int_accreg: cant write [%s] !!! data is lost !!!\n"_fmt,
                inter_conf.accreg_txt);
        return 1;
    }
    for (auto& pair : accreg_db)
        inter_accreg_save_sub(&pair.second, fp);

    return 0;
}

// セーブ
void inter_save(void)
{
    inter_party_save();
    inter_guild_save();
    inter_storage_save();
    inter_accreg_save();
}

// 初期化
void inter_init2()
{
    inter_party_init();
    inter_guild_init();
    inter_storage_init();
    inter_accreg_init();
}

//--------------------------------------------------------
// sended packets to map-server

// GMメッセージ送信
static
void mapif_GMmessage(XString mes)
{
    for (Session *ss : iter_map_sessions())
    {
        send_packet_repeatonly<0x3800, 4, 1>(ss, mes);
    }
}

// Wisp/page transmission to correct map-server
static
void mapif_wis_message(Session *tms, CharName src, CharName dst, XString msg)
{
    const CharPair *mcs = search_character(src);
    assert (mcs);

    Packet_Head<0x3801> head_01;
    head_01.whisper_id = mcs->key.char_id;
    head_01.src_char_name = src;
    head_01.dst_char_name = dst;
    send_vpacket<0x3801, 56, 1>(tms, head_01, msg);
}

// Wisp/page transmission result to map-server
static
void mapif_wis_end(Session *sms, CharName sender, int flag)
{
    Packet_Fixed<0x3802> fixed_02;
    fixed_02.sender_char_name = sender;
    fixed_02.flag = flag;     // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    send_fpacket<0x3802, 27>(sms, fixed_02);
}

// アカウント変数送信
static
void mapif_account_reg(Session *s, AccountId account_id, const std::vector<Packet_Repeat<0x3004>>& repeat)
{
    Packet_Head<0x3804> head_04;
    head_04.account_id = account_id;
    std::vector<Packet_Repeat<0x3804>> repeat_04(repeat.size());
    for (size_t i = 0; i < repeat.size(); ++i)
    {
        repeat_04[i].name = repeat[i].name;
        repeat_04[i].value = repeat[i].value;
    }

    for (Session *ss : iter_map_sessions())
    {
        if (ss == s)
            continue;
        send_vpacket<0x3804, 8, 36>(ss, head_04, repeat_04);
    }
}

// アカウント変数要求返信
static
void mapif_account_reg_reply(Session *s, AccountId account_id)
{
    Option<P<struct accreg>> reg_ = accreg_db.search(account_id);

    Packet_Head<0x3804> head_04;
    head_04.account_id = account_id;
    std::vector<Packet_Repeat<0x3804>> repeat_04;
    OMATCH_BEGIN_SOME (reg, reg_)
    {
        repeat_04.resize(reg->reg_num);
        assert (reg->reg_num < ACCOUNT_REG_NUM);
        for (size_t j = 0; j < reg->reg_num; ++j)
        {
            repeat_04[j].name = reg->reg[j].str;
            repeat_04[j].value = reg->reg[j].value;
        }
    }
    OMATCH_END ();
    send_vpacket<0x3804, 8, 36>(s, head_04, repeat_04);
}

//--------------------------------------------------------
// received packets from map-server

// GMメッセージ送信
static __attribute__((warn_unused_result))
RecvResult mapif_parse_GMmessage(Session *s)
{
    AString repeat;
    RecvResult rv = recv_packet_repeatonly<0x3000, 4, 1>(s, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    AString& buf = repeat;
    mapif_GMmessage(buf);

    return rv;
}

// Wisp/page request to send
static __attribute__((warn_unused_result))
RecvResult mapif_parse_WisRequest(Session *sms)
{
    Packet_Head<0x3001> head;
    AString repeat;
    RecvResult rv = recv_vpacket<0x3001, 52, 1>(sms, head, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    CharName from = head.from_char_name;
    CharName to = head.to_char_name;

    // search if character exists before to ask all map-servers
    const CharPair *mcs = search_character(to);
    if (!mcs)
    {
        Packet_Fixed<0x3802> fixed_02;
        fixed_02.sender_char_name = from;
        fixed_02.flag = 1;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        send_fpacket<0x3802, 27>(sms, fixed_02);
        // Character exists. So, ask all map-servers
    }
    else
    {
        // to be sure of the correct name, rewrite it
        to = mcs->key.name;
        // if source is destination, don't ask other servers.
        if (from == to)
        {
            Packet_Fixed<0x3802> fixed_02;
            fixed_02.sender_char_name = from;
            fixed_02.flag = 1;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            send_fpacket<0x3802, 27>(sms, fixed_02);
        }
        else
        {
            Session *tms = server_for(mcs); // for to
            AString& msg = repeat;
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

    return rv;
}

// Wisp/page transmission result
static __attribute__((warn_unused_result))
RecvResult mapif_parse_WisReply(Session *tms)
{
    Packet_Fixed<0x3002> fixed;
    RecvResult rv = recv_fpacket<0x3002, 7>(tms, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    CharId id = fixed.char_id;
    uint8_t flag = fixed.flag;

    const CharPair *smcs = search_character_id(id);
    CharName from = smcs->key.name;
    Session *sms = server_for(smcs);

    if (sms)
    {
        mapif_wis_end(sms, from, flag);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    }

    return rv;
}

// Received wisp message from map-server for ALL gm (just copy the message and resends it to ALL map-servers)
static __attribute__((warn_unused_result))
RecvResult mapif_parse_WisToGM(Session *s)
{
    Packet_Head<0x3003> head;
    AString repeat;
    RecvResult rv = recv_vpacket<0x3003, 30, 1>(s, head, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    Packet_Head<0x3803> head_03;
    head_03.char_name = head.char_name;
    head_03.min_gm_level = head.min_gm_level;
    for (Session *ss : iter_map_sessions())
    {
        send_vpacket<0x3803, 30, 1>(ss, head_03, repeat);
    }

    return rv;
}

// アカウント変数保存要求
static __attribute__((warn_unused_result))
RecvResult mapif_parse_AccReg(Session *s)
{
    Packet_Head<0x3004> head;
    std::vector<Packet_Repeat<0x3004>> repeat;
    RecvResult rv = recv_vpacket<0x3004, 8, 36>(s, head, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    P<struct accreg> reg = accreg_db.init(head.account_id);
    {
        reg->account_id = head.account_id;
    }

    size_t jlim = std::min(repeat.size(), ACCOUNT_REG_NUM);
    for (size_t j = 0; j < jlim; ++j)
    {
        reg->reg[j].str = repeat[j].name;
        reg->reg[j].value = repeat[j].value;
    }
    reg->reg_num = jlim;

    // 他のMAPサーバーに送信
    mapif_account_reg(s, head.account_id, repeat);

    return rv;
}

// アカウント変数送信要求
static __attribute__((warn_unused_result))
RecvResult mapif_parse_AccRegRequest(Session *s)
{
    Packet_Fixed<0x3005> fixed;
    RecvResult rv = recv_fpacket<0x3005, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    mapif_account_reg_reply(s, fixed.account_id);

    return rv;
}

//--------------------------------------------------------

// map server からの通信（１パケットのみ解析すること）
// エラーなら0(false)、処理できたなら1、
// パケット長が足りなければ2をかえさなければならない
RecvResult inter_parse_frommap(Session *ms, uint16_t packet_id)
{
    int cmd = packet_id;

    RecvResult rv;

    switch (cmd)
    {
        case 0x3000:
            rv = mapif_parse_GMmessage(ms);
            break;
        case 0x3001:
            rv = mapif_parse_WisRequest(ms);
            break;
        case 0x3002:
            rv = mapif_parse_WisReply(ms);
            break;
        case 0x3003:
            rv = mapif_parse_WisToGM(ms);
            break;
        case 0x3004:
            rv = mapif_parse_AccReg(ms);
            break;
        case 0x3005:
            rv = mapif_parse_AccRegRequest(ms);
            break;
        default:
            rv = inter_party_parse_frommap(ms, packet_id);
            if (rv != RecvResult::Error)
                return rv;
            rv = inter_storage_parse_frommap(ms, packet_id);
            if (rv != RecvResult::Error)
                return rv;
            rv = inter_guild_parse_frommap(ms, packet_id);
            if (rv != RecvResult::Error)
                return rv;
            return RecvResult::Error;
    }

    return rv;
}
} // namespace char_
} // namespace tmwa
