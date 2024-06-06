#include "int_storage.hpp"
//    int_storage.cpp - Internal storage handling.
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

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/write.hpp"

#include "proto2/char-map.hpp"

#include "../mmo/cxxstdio_enums.hpp"

#include "../high/extract_mmo.hpp"
#include "../high/mmo.hpp"

#include "../wire/packets.hpp"

#include "globals.hpp"
#include "char/inter_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace char_
{
// 倉庫データを文字列に変換
static
AString storage_tostr(Storage *p)
{
    MString str;
    str += STRPRINTF(
            "%d,%d\t"_fmt,
            p->account_id, p->storage_amount);

    int f = 0;
    for (SOff0 i : SOff0::iter())
    {
        if (p->storage_[i].nameid && p->storage_[i].amount)
        {
            str += STRPRINTF(
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d "_fmt,
                    0 /*id*/,
                    p->storage_[i].nameid,
                    p->storage_[i].amount,
                    p->storage_[i].equip,
                    0 /*identify*/,
                    0 /*refine*/,
                    0 /*attribute*/,
                    0 /*card[0]*/,
                    0 /*card[1]*/,
                    0 /*card[2]*/,
                    0 /*card[3]*/);
            // shouldn't that include 'broken' also? Oh, well ...
            f++;
        }
    }

    str += '\t';

    if (!f)
        return AString();
    return AString(str);
}
} // namespace char_

// 文字列を倉庫データに変換
static
bool impl_extract(XString str, Storage *p)
{
    std::vector<Item> storage_items;
    if (!extract(str,
                record<'\t'>(
                    record<','>(
                        &p->account_id,
                        &p->storage_amount),
                    vrec<' '>(&storage_items))))
        return false;

    if (!p->account_id)
        return false;

    if (storage_items.size() > MAX_STORAGE)
        return false;
    std::copy(storage_items.begin(), storage_items.end(), p->storage_.begin());

    if (p->storage_amount != storage_items.size())
        PRINTF("WARNING: storage desync for %d\n"_fmt, p->account_id);
    return true;
}

namespace char_
{
// アカウントから倉庫データインデックスを得る（新規倉庫追加可能）
Borrowed<Storage> account2storage(AccountId account_id)
{
    P<Storage> s = storage_db.init(account_id);
    s->account_id = account_id;
    return s;
}

//---------------------------------------------------------
// 倉庫データを読み込む
void inter_storage_init(void)
{
    int c = 0;

    io::ReadFile in(inter_conf.storage_txt);
    if (!in.is_open())
    {
        PRINTF("cant't read : %s\n"_fmt, inter_conf.storage_txt);
        return;
    }

    AString line;
    while (in.getline(line))
    {
        Storage s {};
        if (extract(line, &s))
        {
            storage_db.insert(s.account_id, s);
        }
        else
        {
            PRINTF("int_storage: broken data [%s] line %d\n"_fmt,
                    inter_conf.storage_txt, c);
        }
        c++;
    }
}

static
void inter_storage_save_sub(Storage *data, io::WriteFile& fp)
{
    AString line = storage_tostr(data);
    if (line)
        fp.put_line(line);
}

//---------------------------------------------------------
// 倉庫データを書き込む
int inter_storage_save(void)
{
    io::WriteLock fp(inter_conf.storage_txt);

    if (!fp.is_open())
    {
        PRINTF("int_storage: cant write [%s] !!! data is lost !!!\n"_fmt,
                inter_conf.storage_txt);
        return 1;
    }
    for (auto& pair : storage_db)
        inter_storage_save_sub(&pair.second, fp);
    return 0;
}

// 倉庫データ削除
void inter_storage_delete(AccountId account_id)
{
    storage_db.erase(account_id);
}

//---------------------------------------------------------
// map serverへの通信

// 倉庫データの送信
static
void mapif_load_storage(Session *ss, AccountId account_id)
{
    P<Storage> st = account2storage(account_id);
    Packet_Payload<0x3810> payload_10;
    payload_10.account_id = account_id;
    payload_10.storage = *st;
    send_ppacket<0x3810>(ss, payload_10);
}

// 倉庫データ保存完了送信
static
void mapif_save_storage_ack(Session *ss, AccountId account_id)
{
    Packet_Fixed<0x3811> fixed_11;
    fixed_11.account_id = account_id;
    fixed_11.unknown = 0;
    send_fpacket<0x3811, 7>(ss, fixed_11);
}

//---------------------------------------------------------
// map serverからの通信

// 倉庫データ要求受信
static __attribute__((warn_unused_result))
RecvResult mapif_parse_LoadStorage(Session *ss)
{
    Packet_Fixed<0x3010> fixed;
    RecvResult rv = recv_fpacket<0x3010, 6>(ss, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    AccountId account_id = fixed.account_id;
    mapif_load_storage(ss, account_id);

    return rv;
}

// 倉庫データ受信＆保存
static __attribute__((warn_unused_result))
RecvResult mapif_parse_SaveStorage(Session *ss)
{
    Packet_Payload<0x3011> payload;
    RecvResult rv = recv_ppacket<0x3011>(ss, payload);
    if (rv != RecvResult::Complete)
        return rv;

    AccountId account_id = payload.account_id;

    {
        P<Storage> st = account2storage(account_id);
        *st = payload.storage;
        mapif_save_storage_ack(ss, account_id);
    }

    return rv;
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
RecvResult inter_storage_parse_frommap(Session *ms, uint16_t packet_id)
{
    RecvResult rv;
    switch (packet_id)
    {
        case 0x3010:
            rv = mapif_parse_LoadStorage(ms);
            break;
        case 0x3011:
            rv = mapif_parse_SaveStorage(ms);
            break;
        default:
            return RecvResult::Error;
    }
    return rv;
}
} // namespace char_
} // namespace tmwa
