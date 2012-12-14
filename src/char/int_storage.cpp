// $Id: int_storage.c,v 1.1.1.1 2004/09/10 17:26:51 MagicalTux Exp $

#include <string.h>
#include <stdlib.h>

#include "../common/mmo.hpp"
#include "../common/socket.hpp"
#include "../common/db.hpp"
#include "../common/lock.hpp"
#include "char.hpp"
#include "inter.hpp"
#include "int_storage.hpp"

// ファイル名のデフォルト
// inter_config_read()で再設定される
char storage_txt[1024] = "save/storage.txt";

static struct dbt *storage_db;

// 倉庫データを文字列に変換
static
int storage_tostr(char *str, struct storage *p)
{
    int i, f = 0;
    char *str_p = str;
    str_p += sprintf(str_p, "%d,%d\t", p->account_id, p->storage_amount);

    for (i = 0; i < MAX_STORAGE; i++)
        if ((p->storage_[i].nameid) && (p->storage_[i].amount))
        {
            str_p += sprintf(str_p, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",
                              p->storage_[i].id, p->storage_[i].nameid,
                              p->storage_[i].amount, p->storage_[i].equip,
                              p->storage_[i].identify, p->storage_[i].refine,
                              p->storage_[i].attribute,
                              p->storage_[i].card[0], p->storage_[i].card[1],
                              p->storage_[i].card[2], p->storage_[i].card[3]);
            f++;
        }

    *(str_p++) = '\t';

    *str_p = '\0';
    if (!f)
        str[0] = 0;
    return 0;
}

// 文字列を倉庫データに変換
static
int storage_fromstr(char *str, struct storage *p)
{
    int tmp_int[256];
    int set, next, len, i;

    set = sscanf(str, "%d,%d%n", &tmp_int[0], &tmp_int[1], &next);
    p->storage_amount = tmp_int[1];

    if (set != 2)
        return 1;
    if (str[next] == '\n' || str[next] == '\r')
        return 0;
    next++;
    for (i = 0; str[next] && str[next] != '\t' && i < MAX_STORAGE; i++)
    {
        if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
                    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
                    &tmp_int[4], &tmp_int[5], &tmp_int[6],
                    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10],
                    &tmp_int[10], &len) == 12)
        {
            p->storage_[i].id = tmp_int[0];
            p->storage_[i].nameid = tmp_int[1];
            p->storage_[i].amount = tmp_int[2];
            p->storage_[i].equip = tmp_int[3];
            p->storage_[i].identify = tmp_int[4];
            p->storage_[i].refine = tmp_int[5];
            p->storage_[i].attribute = tmp_int[6];
            p->storage_[i].card[0] = tmp_int[7];
            p->storage_[i].card[1] = tmp_int[8];
            p->storage_[i].card[2] = tmp_int[9];
            p->storage_[i].card[3] = tmp_int[10];
            next += len;
            if (str[next] == ' ')
                next++;
        }

        else if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
                         &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
                         &tmp_int[4], &tmp_int[5], &tmp_int[6],
                         &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10],
                         &len) == 11)
        {
            p->storage_[i].id = tmp_int[0];
            p->storage_[i].nameid = tmp_int[1];
            p->storage_[i].amount = tmp_int[2];
            p->storage_[i].equip = tmp_int[3];
            p->storage_[i].identify = tmp_int[4];
            p->storage_[i].refine = tmp_int[5];
            p->storage_[i].attribute = tmp_int[6];
            p->storage_[i].card[0] = tmp_int[7];
            p->storage_[i].card[1] = tmp_int[8];
            p->storage_[i].card[2] = tmp_int[9];
            p->storage_[i].card[3] = tmp_int[10];
            next += len;
            if (str[next] == ' ')
                next++;
        }

        else
            return 1;
    }
    if (i >= MAX_STORAGE && str[next] && str[next] != '\t')
        printf("storage_fromstr: Found a storage line with more items than MAX_STORAGE (%d), remaining items have been discarded!\n",
             MAX_STORAGE);
    return 0;
}

// アカウントから倉庫データインデックスを得る（新規倉庫追加可能）
struct storage *account2storage(int account_id)
{
    struct storage *s;
    s = (struct storage *) numdb_search(storage_db, account_id);
    if (s == NULL)
    {
        CREATE(s, struct storage, 1);
        memset(s, 0, sizeof(struct storage));
        s->account_id = account_id;
        numdb_insert(storage_db, s->account_id, s);
    }
    return s;
}

//---------------------------------------------------------
// 倉庫データを読み込む
int inter_storage_init(void)
{
    char line[65536];
    int c = 0, tmp_int;
    struct storage *s;
    FILE *fp;

    storage_db = numdb_init();

    fp = fopen_(storage_txt, "r");
    if (fp == NULL)
    {
        printf("cant't read : %s\n", storage_txt);
        return 1;
    }
    while (fgets(line, 65535, fp))
    {
        sscanf(line, "%d", &tmp_int);
        CREATE(s, struct storage, 1);
        s->account_id = tmp_int;
        if (s->account_id > 0 && storage_fromstr(line, s) == 0)
        {
            numdb_insert(storage_db, s->account_id, s);
        }
        else
        {
            printf("int_storage: broken data [%s] line %d\n", storage_txt,
                    c);
            free(s);
        }
        c++;
    }
    fclose_(fp);

    return 0;
}

static
void storage_db_final(db_key_t k, db_val_t data, va_list ap)
{
    struct storage *p = (struct storage *) data;
    if (p)
        free(p);
}

void inter_storage_final(void)
{
    numdb_final(storage_db, storage_db_final);
    return;
}

static
void inter_storage_save_sub(db_key_t key, db_val_t data, va_list ap)
{
    char line[65536];
    FILE *fp;
    storage_tostr(line, (struct storage *) data);
    fp = va_arg(ap, FILE *);
    if (*line)
        fprintf(fp, "%s\n", line);
}

//---------------------------------------------------------
// 倉庫データを書き込む
int inter_storage_save(void)
{
    FILE *fp;
    int lock;

    if (!storage_db)
        return 1;

    if ((fp = lock_fopen(storage_txt, &lock)) == NULL)
    {
        printf("int_storage: cant write [%s] !!! data is lost !!!\n",
                storage_txt);
        return 1;
    }
    numdb_foreach(storage_db, inter_storage_save_sub, fp);
    lock_fclose(fp, storage_txt, &lock);
//  printf("int_storage: %s saved.\n",storage_txt);
    return 0;
}

// 倉庫データ削除
int inter_storage_delete(int account_id)
{
    struct storage *s =
        (struct storage *) numdb_search(storage_db, account_id);
    if (s)
    {
        numdb_erase(storage_db, account_id);
        free(s);
    }
    return 0;
}

//---------------------------------------------------------
// map serverへの通信

// 倉庫データの送信
static
int mapif_load_storage(int fd, int account_id)
{
    struct storage *s = account2storage(account_id);
    WFIFOW(fd, 0) = 0x3810;
    WFIFOW(fd, 2) = sizeof(struct storage) + 8;
    WFIFOL(fd, 4) = account_id;
    memcpy(WFIFOP(fd, 8), s, sizeof(struct storage));
    WFIFOSET(fd, WFIFOW(fd, 2));
    return 0;
}

// 倉庫データ保存完了送信
static
int mapif_save_storage_ack(int fd, int account_id)
{
    WFIFOW(fd, 0) = 0x3811;
    WFIFOL(fd, 2) = account_id;
    WFIFOB(fd, 6) = 0;
    WFIFOSET(fd, 7);
    return 0;
}

//---------------------------------------------------------
// map serverからの通信

// 倉庫データ要求受信
static
int mapif_parse_LoadStorage(int fd)
{
    mapif_load_storage(fd, RFIFOL(fd, 2));
    return 0;
}

// 倉庫データ受信＆保存
static
int mapif_parse_SaveStorage(int fd)
{
    struct storage *s;
    int account_id = RFIFOL(fd, 4);
    int len = RFIFOW(fd, 2);
    if (sizeof(struct storage) != len - 8)
    {
        printf("inter storage: data size error %d %d\n",
                sizeof(struct storage), len - 8);
    }
    else
    {
        s = account2storage(account_id);
        memcpy(s, RFIFOP(fd, 8), sizeof(struct storage));
        mapif_save_storage_ack(fd, account_id);
    }
    return 0;
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
