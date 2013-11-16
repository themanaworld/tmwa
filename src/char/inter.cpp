#include "inter.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <vector>

#include "../strings/mstring.hpp"
#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"

#include "../common/db.hpp"
#include "../common/extract.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"
#include "../common/utils.hpp"

#include "char.hpp"
#include "int_party.hpp"
#include "int_storage.hpp"

#include "../poison.hpp"

// Existence time of Wisp/page data (60 seconds)
// that is the waiting time of answers of all map-servers
constexpr std::chrono::minutes WISDATA_TTL = std::chrono::minutes(1);

static
FString accreg_txt = "save/accreg.txt";

struct accreg
{
    int account_id, reg_num;
    struct global_reg reg[ACCOUNT_REG_NUM];
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

struct WisData
{
    int id, fd, count;
    tick_t tick;
    CharName src, dst;
    FString msg;
};
static
Map<int, struct WisData> wis_db;
static
std::vector<int> wis_dellistv;

//--------------------------------------------------------

// アカウント変数を文字列へ変換
static
FString inter_accreg_tostr(struct accreg *reg)
{
    MString str;
    str += STRPRINTF("%d\t", reg->account_id);
    for (int j = 0; j < reg->reg_num; j++)
        str += STRPRINTF("%s,%d ", reg->reg[j].str, reg->reg[j].value);
    return FString(str);
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
    std::copy(vars.begin(), vars.end(), reg->reg);
    reg->reg_num = vars.size();
    return true;
}

// アカウント変数の読み込み
static
int inter_accreg_init(void)
{
    int c = 0;

    io::ReadFile in(accreg_txt);
    if (!in.is_open())
        return 1;
    FString line;
    while (in.getline(line))
    {
        struct accreg reg {};
        if (extract(line, &reg))
        {
            accreg_db.insert(reg.account_id, reg);
        }
        else
        {
            PRINTF("inter: accreg: broken data [%s] line %d\n", accreg_txt,
                    c);
        }
        c++;
    }

    return 0;
}

// アカウント変数のセーブ用
static
void inter_accreg_save_sub(struct accreg *reg, io::WriteFile& fp)
{
    if (reg->reg_num > 0)
    {
        FString line = inter_accreg_tostr(reg);
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
        PRINTF("int_accreg: cant write [%s] !!! data is lost !!!\n",
                accreg_txt);
        return 1;
    }
    for (auto& pair : accreg_db)
        inter_accreg_save_sub(&pair.second, fp);

    return 0;
}

//--------------------------------------------------------

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
static
int inter_config_read(ZString cfgName)
{
    io::ReadFile in(cfgName);
    if (!in.is_open())
    {
        PRINTF("file not found: %s\n", cfgName);
        return 1;
    }

    FString line;
    while (in.getline(line))
    {
        XString w1;
        ZString w2;
        if (!split_key_value(line, &w1, &w2))
            continue;

        if (w1 == "storage_txt")
        {
            storage_txt = w2;
        }
        else if (w1 == "party_txt")
        {
            party_txt = w2;
        }
        else if (w1 == "accreg_txt")
        {
            accreg_txt = w2;
        }
        else if (w1 == "party_share_level")
        {
            party_share_level = atoi(w2.c_str());
            if (party_share_level < 0)
                party_share_level = 0;
        }
        else if (w1 == "import")
        {
            inter_config_read(w2);
        }
        else
        {
            FString w1z = w1;
            PRINTF("WARNING: unknown inter config key: %s\n", w1z);
        }
    }

    return 0;
}

// セーブ
void inter_save(void)
{
    inter_party_save();
    inter_storage_save();
    inter_accreg_save();
}

// 初期化
void inter_init(ZString file)
{
    inter_config_read(file);

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

// Wisp/page transmission to all map-server
static
void mapif_wis_message(struct WisData *wd)
{
    size_t str_size = wd->msg.size() + 1;
    uint8_t buf[56 + str_size];

    WBUFW(buf, 0) = 0x3801;
    WBUFW(buf, 2) = 56 + str_size;
    WBUFL(buf, 4) = wd->id;
    WBUF_STRING(buf, 8, wd->src.to__actual(), 24);
    WBUF_STRING(buf, 32, wd->dst.to__actual(), 24);
    WBUF_STRING(buf, 56, wd->msg, str_size);
    wd->count = mapif_sendall(buf, WBUFW(buf, 2));
}

// Wisp/page transmission result to map-server
static
void mapif_wis_end(struct WisData *wd, int flag)
{
    uint8_t buf[27];

    WBUFW(buf, 0) = 0x3802;
    WBUF_STRING(buf, 2, wd->src.to__actual(), 24);
    WBUFB(buf, 26) = flag;     // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
    mapif_send(wd->fd, buf, 27);
}

// アカウント変数送信
static
void mapif_account_reg(int fd)
{
    size_t len = RFIFOW(fd, 2);
    uint8_t buf[len];
    RFIFO_BUF_CLONE(fd, buf, len);
    WBUFW(buf, 0) = 0x3804;
    mapif_sendallwos(fd, buf, WBUFW(buf, 2));
}

// アカウント変数要求返信
static
void mapif_account_reg_reply(int fd, int account_id)
{
    struct accreg *reg = accreg_db.search(account_id);

    WFIFOW(fd, 0) = 0x3804;
    WFIFOL(fd, 4) = account_id;
    if (reg == NULL)
    {
        WFIFOW(fd, 2) = 8;
    }
    else
    {
        int j, p;
        for (j = 0, p = 8; j < reg->reg_num; j++, p += 36)
        {
            WFIFO_STRING(fd, p, reg->reg[j].str, 32);
            WFIFOL(fd, p + 32) = reg->reg[j].value;
        }
        WFIFOW(fd, 2) = p;
    }
    WFIFOSET(fd, WFIFOW(fd, 2));
}

//--------------------------------------------------------

// Existence check of WISP data
static
void check_ttl_wisdata_sub(struct WisData *wd, tick_t tick)
{
    if (tick > wd->tick + WISDATA_TTL)
        wis_dellistv.push_back(wd->id);
}

static
void check_ttl_wisdata(void)
{
    tick_t tick = gettick();

    // this code looks silly now, but let's wait a bit to refactor properly
    {
        wis_dellistv.clear();
        for (auto& pair : wis_db)
            check_ttl_wisdata_sub(&pair.second, tick);
        for (int it : wis_dellistv)
        {
            struct WisData *wd = wis_db.search(it);
            assert (wd);
            PRINTF("inter: wis data id=%d time out : from %s to %s\n",
                    wd->id, wd->src, wd->dst);
            // removed. not send information after a timeout. Just no answer for the player
            //mapif_wis_end(wd, 1); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            wis_db.erase(wd->id);
        }
    }
}

//--------------------------------------------------------
// received packets from map-server

// GMメッセージ送信
static
void mapif_parse_GMmessage(int fd)
{
    size_t msg_len = RFIFOW(fd, 2);
    size_t str_len = msg_len - 4;
    FString buf = RFIFO_STRING(fd, 4, str_len);

    mapif_GMmessage(buf);
}

// Wisp/page request to send
static
void mapif_parse_WisRequest(int fd)
{
    static int wisid = 0;

    if (RFIFOW(fd, 2) - 52 <= 0)
    {                           // normaly, impossible, but who knows...
        PRINTF("inter: Wis message doesn't exist.\n");
        return;
    }

    CharName from = stringish<CharName>(RFIFO_STRING<24>(fd, 4));
    CharName to = stringish<CharName>(RFIFO_STRING<24>(fd, 28));

    // search if character exists before to ask all map-servers
    const mmo_charstatus *mcs = search_character(to);
    if (!mcs)
    {
        uint8_t buf[27];
        WBUFW(buf, 0) = 0x3802;
        WBUF_STRING(buf, 2, from.to__actual(), 24);
        WBUFB(buf, 26) = 1;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        mapif_send(fd, buf, 27);
        // Character exists. So, ask all map-servers
    }
    else
    {
        // to be sure of the correct name, rewrite it
        to = mcs->name;
        // if source is destination, don't ask other servers.
        if (from == to)
        {
            uint8_t buf[27];
            WBUFW(buf, 0) = 0x3802;
            WBUF_STRING(buf, 2, from.to__actual(), 24);
            WBUFB(buf, 26) = 1;    // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
            mapif_send(fd, buf, 27);
        }
        else
        {
            struct WisData wd {};

            // Whether the failure of previous wisp/page transmission (timeout)
            check_ttl_wisdata();

            wd.id = ++wisid;
            wd.fd = fd;
            size_t len = RFIFOW(fd, 2) - 52;
            wd.src = from;
            wd.dst = to;
            wd.msg = RFIFO_STRING(fd, 52, len);
            wd.tick = gettick();
            wis_db.insert(wd.id, wd);
            mapif_wis_message(&wd);
        }
    }
}

// Wisp/page transmission result
static
int mapif_parse_WisReply(int fd)
{
    int id = RFIFOL(fd, 2), flag = RFIFOB(fd, 6);
    struct WisData *wd = wis_db.search(id);

    if (wd == NULL)
        return 0;               // This wisp was probably suppress before, because it was timeout of because of target was found on another map-server

    if ((--wd->count) <= 0 || flag != 1)
    {
        mapif_wis_end(wd, flag);   // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
        wis_db.erase(id);
    }

    return 0;
}

// Received wisp message from map-server for ALL gm (just copy the message and resends it to ALL map-servers)
static
void mapif_parse_WisToGM(int fd)
{
    size_t len = RFIFOW(fd, 2);
    uint8_t buf[len];
    // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B

    RFIFO_BUF_CLONE(fd, buf, len);
    WBUFW(buf, 0) = 0x3803;
    mapif_sendall(buf, len);
}

// アカウント変数保存要求
static
void mapif_parse_AccReg(int fd)
{
    int j, p;
    struct accreg *reg = accreg_db.search(RFIFOL(fd, 4));

    if (reg == NULL)
    {
        int account_id = RFIFOL(fd, 4);
        reg = accreg_db.init(account_id);
        reg->account_id = account_id;
    }

    for (j = 0, p = 8; j < ACCOUNT_REG_NUM && p < RFIFOW(fd, 2);
         j++, p += 36)
    {
        reg->reg[j].str = stringish<VarName>(RFIFO_STRING<32>(fd, p));
        reg->reg[j].value = RFIFOL(fd, p + 32);
    }
    reg->reg_num = j;

    // 他のMAPサーバーに送信
    mapif_account_reg(fd);
}

// アカウント変数送信要求
static
void mapif_parse_AccRegRequest(int fd)
{
    mapif_account_reg_reply(fd, RFIFOL(fd, 2));
}

//--------------------------------------------------------

// map server からの通信（１パケットのみ解析すること）
// エラーなら0(false)、処理できたなら1、
// パケット長が足りなければ2をかえさなければならない
int inter_parse_frommap(int fd)
{
    int cmd = RFIFOW(fd, 0);
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
         inter_check_length(fd,
                             inter_recv_packet_length[cmd - 0x3000])) == 0)
        return 2;

    switch (cmd)
    {
        case 0x3000:
            mapif_parse_GMmessage(fd);
            break;
        case 0x3001:
            mapif_parse_WisRequest(fd);
            break;
        case 0x3002:
            mapif_parse_WisReply(fd);
            break;
        case 0x3003:
            mapif_parse_WisToGM(fd);
            break;
        case 0x3004:
            mapif_parse_AccReg(fd);
            break;
        case 0x3005:
            mapif_parse_AccRegRequest(fd);
            break;
        default:
            if (inter_party_parse_frommap(fd))
                break;
            if (inter_storage_parse_frommap(fd))
                break;
            return 0;
    }
    RFIFOSKIP(fd, len);

    return 1;
}

// RFIFOのパケット長確認
// 必要パケット長があればパケット長、まだ足りなければ0
int inter_check_length(int fd, int length)
{
    if (length == -1)
    {                           // 可変パケット長
        if (RFIFOREST(fd) < 4) // パケット長が未着
            return 0;
        length = RFIFOW(fd, 2);
    }

    if (RFIFOREST(fd) < length)    // パケットが未着
        return 0;

    return length;
}
