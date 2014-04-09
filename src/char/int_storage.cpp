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

#include <cstdlib>
#include <cstring>

#include <functional>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"

#include "../mmo/extract.hpp"
#include "../mmo/mmo.hpp"
#include "../mmo/socket.hpp"

#include "../poison.hpp"

// ファイル名のデフォルト
// inter_config_read()で再設定される
AString storage_txt = "save/storage.txt";

static
Map<int, struct storage> storage_db;

// 倉庫データを文字列に変換
static
AString storage_tostr(struct storage *p)
{
    MString str;
    str += STRPRINTF(
            "%d,%d\t",
            p->account_id, p->storage_amount);

    int f = 0;
    for (int i = 0; i < MAX_STORAGE; i++)
        if (p->storage_[i].nameid && p->storage_[i].amount)
        {
            str += STRPRINTF(
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",
                    p->storage_[i].id,
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

    str += '\t';

    if (!f)
        return AString();
    return AString(str);
}

// 文字列を倉庫データに変換
static
bool extract(XString str, struct storage *p)
{
    std::vector<struct item> storage_items;
    if (!extract(str,
                record<'\t'>(
                    record<','>(
                        &p->account_id,
                        &p->storage_amount),
                    vrec<' '>(&storage_items))))
        return false;

    if (p->account_id <= 0)
        return false;

    if (storage_items.size() > MAX_STORAGE)
        return false;
    std::copy(storage_items.begin(), storage_items.end(), p->storage_.begin());

    if (p->storage_amount != storage_items.size())
        PRINTF("WARNING: storage desync for %d\n", p->account_id);
    return true;
}

// アカウントから倉庫データインデックスを得る（新規倉庫追加可能）
struct storage *account2storage(int account_id)
{
    struct storage *s = storage_db.search(account_id);
    if (s == NULL)
    {
        s = storage_db.init(account_id);
        s->account_id = account_id;
    }
    return s;
}

//---------------------------------------------------------
// 倉庫データを読み込む
void inter_storage_init(void)
{
    int c = 0;

    io::ReadFile in(storage_txt);
    if (!in.is_open())
    {
        PRINTF("cant't read : %s\n", storage_txt);
        return;
    }

    AString line;
    while (in.getline(line))
    {
        struct storage s {};
        if (extract(line, &s))
        {
            storage_db.insert(s.account_id, s);
        }
        else
        {
            PRINTF("int_storage: broken data [%s] line %d\n",
                    storage_txt, c);
        }
        c++;
    }
}

static
void inter_storage_save_sub(struct storage *data, io::WriteFile& fp)
{
    AString line = storage_tostr(data);
    if (line)
        fp.put_line(line);
}

//---------------------------------------------------------
// 倉庫データを書き込む
int inter_storage_save(void)
{
    io::WriteLock fp(storage_txt);

    if (!fp.is_open())
    {
        PRINTF("int_storage: cant write [%s] !!! data is lost !!!\n",
                storage_txt);
        return 1;
    }
    for (auto& pair : storage_db)
        inter_storage_save_sub(&pair.second, fp);
    return 0;
}

// 倉庫データ削除
void inter_storage_delete(int account_id)
{
    storage_db.erase(account_id);
}

//---------------------------------------------------------
// map serverへの通信

// 倉庫データの送信
static
void mapif_load_storage(Session *ss, int account_id)
{
    struct storage *st = account2storage(account_id);
    WFIFOW(ss, 0) = 0x3810;
    WFIFOW(ss, 2) = sizeof(struct storage) + 8;
    WFIFOL(ss, 4) = account_id;
    WFIFO_STRUCT(ss, 8, *st);
    WFIFOSET(ss, WFIFOW(ss, 2));
}

// 倉庫データ保存完了送信
static
void mapif_save_storage_ack(Session *ss, int account_id)
{
    WFIFOW(ss, 0) = 0x3811;
    WFIFOL(ss, 2) = account_id;
    WFIFOB(ss, 6) = 0;
    WFIFOSET(ss, 7);
}

//---------------------------------------------------------
// map serverからの通信

// 倉庫データ要求受信
static
void mapif_parse_LoadStorage(Session *ss)
{
    mapif_load_storage(ss, RFIFOL(ss, 2));
}

// 倉庫データ受信＆保存
static
void mapif_parse_SaveStorage(Session *ss)
{
    struct storage *st;
    int account_id = RFIFOL(ss, 4);
    int len = RFIFOW(ss, 2);
    if (sizeof(struct storage) != len - 8)
    {
        PRINTF("inter storage: data size error %zu %d\n",
                sizeof(struct storage), len - 8);
    }
    else
    {
        st = account2storage(account_id);
        RFIFO_STRUCT(ss, 8, *st);
        mapif_save_storage_ack(ss, account_id);
    }
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_storage_parse_frommap(Session *ms)
{
    switch (RFIFOW(ms, 0))
    {
        case 0x3010:
            mapif_parse_LoadStorage(ms);
            break;
        case 0x3011:
            mapif_parse_SaveStorage(ms);
            break;
        default:
            return 0;
    }
    return 1;
}
