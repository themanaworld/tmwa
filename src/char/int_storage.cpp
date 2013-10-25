#include "int_storage.hpp"

#include <cstdlib>
#include <cstring>

#include <functional>
#include <fstream>

#include "../strings/mstring.hpp"
#include "../strings/fstring.hpp"
#include "../strings/xstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/extract.hpp"
#include "../common/io.hpp"
#include "../common/lock.hpp"
#include "../common/mmo.hpp"
#include "../common/socket.hpp"

#include "../poison.hpp"

// ファイル名のデフォルト
// inter_config_read()で再設定される
FString storage_txt = "save/storage.txt";

static
Map<int, struct storage> storage_db;

// 倉庫データを文字列に変換
static
FString storage_tostr(struct storage *p)
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
                    p->storage_[i].identify,
                    p->storage_[i].refine,
                    p->storage_[i].attribute,
                    p->storage_[i].card[0],
                    p->storage_[i].card[1],
                    p->storage_[i].card[2],
                    p->storage_[i].card[3]);
            f++;
        }

    str += '\t';

    if (!f)
        return FString();
    return FString(str);
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
    std::copy(storage_items.begin(), storage_items.end(), p->storage_);

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
int inter_storage_init(void)
{
    int c = 0;

    std::ifstream in(storage_txt.c_str());
    if (!in.is_open())
    {
        PRINTF("cant't read : %s\n", storage_txt);
        return 1;
    }

    FString line;
    while (io::getline(in, line))
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

    return 0;
}

static
void inter_storage_save_sub(struct storage *data, FILE *fp)
{
    FString line = storage_tostr(data);
    if (line)
        FPRINTF(fp, "%s\n", line);
}

//---------------------------------------------------------
// 倉庫データを書き込む
int inter_storage_save(void)
{
    FILE *fp;
    int lock;

    if ((fp = lock_fopen(storage_txt, &lock)) == NULL)
    {
        PRINTF("int_storage: cant write [%s] !!! data is lost !!!\n",
                storage_txt);
        return 1;
    }
    for (auto& pair : storage_db)
        inter_storage_save_sub(&pair.second, fp);
    lock_fclose(fp, storage_txt, &lock);
//  PRINTF("int_storage: %s saved.\n",storage_txt);
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
void mapif_load_storage(int fd, int account_id)
{
    struct storage *s = account2storage(account_id);
    WFIFOW(fd, 0) = 0x3810;
    WFIFOW(fd, 2) = sizeof(struct storage) + 8;
    WFIFOL(fd, 4) = account_id;
    WFIFO_STRUCT(fd, 8, *s);
    WFIFOSET(fd, WFIFOW(fd, 2));
}

// 倉庫データ保存完了送信
static
void mapif_save_storage_ack(int fd, int account_id)
{
    WFIFOW(fd, 0) = 0x3811;
    WFIFOL(fd, 2) = account_id;
    WFIFOB(fd, 6) = 0;
    WFIFOSET(fd, 7);
}

//---------------------------------------------------------
// map serverからの通信

// 倉庫データ要求受信
static
void mapif_parse_LoadStorage(int fd)
{
    mapif_load_storage(fd, RFIFOL(fd, 2));
}

// 倉庫データ受信＆保存
static
void mapif_parse_SaveStorage(int fd)
{
    struct storage *s;
    int account_id = RFIFOL(fd, 4);
    int len = RFIFOW(fd, 2);
    if (sizeof(struct storage) != len - 8)
    {
        PRINTF("inter storage: data size error %zu %d\n",
                sizeof(struct storage), len - 8);
    }
    else
    {
        s = account2storage(account_id);
        RFIFO_STRUCT(fd, 8, *s);
        mapif_save_storage_ack(fd, account_id);
    }
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_storage_parse_frommap(int fd)
{
    switch (RFIFOW(fd, 0))
    {
        case 0x3010:
            mapif_parse_LoadStorage(fd);
            break;
        case 0x3011:
            mapif_parse_SaveStorage(fd);
            break;
        default:
            return 0;
    }
    return 1;
}
