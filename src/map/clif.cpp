#include "clif.hpp"

#include <arpa/inet.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

#include "../common/cxxstdio.hpp"
#include "../common/md5calc.hpp"
#include "../common/mt_rand.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"
#include "../common/version.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "magic.hpp"
#include "map.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "tmw.hpp"
#include "trade.hpp"

#include "../poison.hpp"

#define DUMP_UNKNOWN_PACKET     1

#define EMOTE_IGNORED 0x0e

static
const int packet_len_table[0x220] = {
    10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//#0x0040
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 55, 17, 3, 37, 46, -1, 23, -1, 3, 108, 3, 2,
    3, 28, 19, 11, 3, -1, 9, 5, 54, 53, 58, 60, 41, 2, 6, 6,
//#0x0080
    7, 3, 2, 2, 2, 5, 16, 12, 10, 7, 29, 23, -1, -1, -1, 0, // 0x8b unknown... size 2 or 23?
    7, 22, 28, 2, 6, 30, -1, -1, 3, -1, -1, 5, 9, 17, 17, 6,
    23, 6, 6, -1, -1, -1, -1, 8, 7, 6, 7, 4, 7, 0, -1, 6,
    8, 8, 3, 3, -1, 6, 6, -1, 7, 6, 2, 5, 6, 44, 5, 3,
//#0x00C0
    7, 2, 6, 8, 6, 7, -1, -1, -1, -1, 3, 3, 6, 6, 2, 27,
    3, 4, 4, 2, -1, -1, 3, -1, 6, 14, 3, -1, 28, 29, -1, -1,
    30, 30, 26, 2, 6, 26, 3, 3, 8, 19, 5, 2, 3, 2, 2, 2,
    3, 2, 6, 8, 21, 8, 8, 2, 2, 26, 3, -1, 6, 27, 30, 10,

//#0x0100
    2, 6, 6, 30, 79, 31, 10, 10, -1, -1, 4, 6, 6, 2, 11, -1,
    10, 39, 4, 10, 31, 35, 10, 18, 2, 13, 15, 20, 68, 2, 3, 16,
    6, 14, -1, -1, 21, 8, 8, 8, 8, 8, 2, 2, 3, 4, 2, -1,
    6, 86, 6, -1, -1, 7, -1, 6, 3, 16, 4, 4, 4, 6, 24, 26,
//#0x0140
    22, 14, 6, 10, 23, 19, 6, 39, 8, 9, 6, 27, -1, 2, 6, 6,
    110, 6, -1, -1, -1, -1, -1, 6, -1, 54, 66, 54, 90, 42, 6, 42,
    -1, -1, -1, -1, -1, 30, -1, 3, 14, 3, 30, 10, 43, 14, 186, 182,
    14, 30, 10, 3, -1, 6, 106, -1, 4, 5, 4, -1, 6, 7, -1, -1,
//#0x0180
    6, 3, 106, 10, 10, 34, 0, 6, 8, 4, 4, 4, 29, -1, 10, 6,
    90, 86, 24, 6, 30, 102, 9, 4, 8, 4, 14, 10, -1, 6, 2, 6,
    3, 3, 35, 5, 11, 26, -1, 4, 4, 6, 10, 12, 6, -1, 4, 4,
    11, 7, -1, 67, 12, 18, 114, 6, 3, 6, 26, 26, 26, 26, 2, 3,
//#0x01C0,   Set 0x1d5=-1
    2, 14, 10, -1, 22, 22, 4, 2, 13, 97, 0, 9, 9, 30, 6, 28,
    8, 14, 10, 35, 6, -1, 4, 11, 54, 53, 60, 2, -1, 47, 33, 6,
    30, 8, 34, 14, 2, 6, 26, 2, 28, 81, 6, 10, 26, 2, -1, -1,
    -1, -1, 20, 10, 32, 9, 34, 14, 2, 6, 48, 56, -1, 4, 5, 10,
//#0x200
    26, -1, 26, 10, 18, 26, 11, 34, 14, 36, 10, 19, 10, -1, 24, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// local define
enum class SendWho
{
    ALL_CLIENT,
#define ALL_CLIENT SendWho::ALL_CLIENT
    ALL_SAMEMAP,
#define ALL_SAMEMAP SendWho::ALL_SAMEMAP
    AREA,
#define AREA SendWho::AREA
    AREA_WOS,
#define AREA_WOS SendWho::AREA_WOS
    AREA_WOC,
#define AREA_WOC SendWho::AREA_WOC
    AREA_WOSC,
#define AREA_WOSC SendWho::AREA_WOSC
    AREA_CHAT_WOC,
#define AREA_CHAT_WOC SendWho::AREA_CHAT_WOC
    // temporary rename to avoid collision with BL::CHAT_
    CHAT_,
#define CHAT_ SendWho::CHAT_
    CHAT_WOS,
#define CHAT_WOS SendWho::CHAT_WOS
    PARTY,
#define PARTY SendWho::PARTY
    PARTY_WOS,
#define PARTY_WOS SendWho::PARTY_WOS
    PARTY_SAMEMAP,
#define PARTY_SAMEMAP SendWho::PARTY_SAMEMAP
    PARTY_SAMEMAP_WOS,
#define PARTY_SAMEMAP_WOS SendWho::PARTY_SAMEMAP_WOS
    PARTY_AREA,
#define PARTY_AREA SendWho::PARTY_AREA
    PARTY_AREA_WOS,
#define PARTY_AREA_WOS SendWho::PARTY_AREA_WOS
    SELF,
#define SELF SendWho::SELF
};

inline
void WBUFPOS(uint8_t *p, size_t pos, uint16_t x, uint16_t y)
{
    p += pos;
    p[0] = x >> 2;
    p[1] = (x << 6) | ((y >> 4) & 0x3f);
    p[2] = y << 4;
}
inline
void WBUFPOS2(uint8_t *p, size_t pos, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    p += pos;
    p[0] = x0 >> 2;
    p[1] = (x0 << 6) | ((y0 >> 4) & 0x3f);
    p[2] = (y0 << 4) | ((x1 >> 6) & 0x0f);
    p[3] = (x1 << 2) | ((y1 >> 8) & 0x03);
    p[4] = y1;
}

inline
void WFIFOPOS(int fd, size_t pos, uint16_t x, uint16_t y)
{
    WBUFPOS(static_cast<uint8_t *>(WFIFOP(fd, pos)), 0, x, y);
}
inline
void WFIFOPOS2(int fd, size_t pos, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    WBUFPOS2(static_cast<uint8_t *>(WFIFOP(fd, pos)), 0, x0, y0, x1, y1);
}

static
char map_ip_str[16];
static
struct in_addr map_ip;
static
int map_port = 5121;

static
int clif_changelook_towards(struct block_list *bl, LOOK type, int val,
                             struct map_session_data *dstsd);

/*==========================================
 * map鯖のip設定
 *------------------------------------------
 */
void clif_setip(const char *ip)
{
    memcpy(map_ip_str, ip, 16);
    map_ip.s_addr = inet_addr(map_ip_str);
}

/*==========================================
 * map鯖のport設定
 *------------------------------------------
 */
void clif_setport(int port)
{
    map_port = port;
}

/*==========================================
 * map鯖のip読み出し
 *------------------------------------------
 */
struct in_addr clif_getip(void)
{
    return map_ip;
}

/*==========================================
 * map鯖のport読み出し
 *------------------------------------------
 */
int clif_getport(void)
{
    return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void)
{
    int users = 0, i;
    struct map_session_data *sd;

    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd
            && sd->state.auth && !(battle_config.hide_GM_session
                                   && pc_isGM(sd)))
            users++;
    }
    return users;
}

/*==========================================
 * 全てのclientに対してfunc()実行
 *------------------------------------------
 */
int clif_foreachclient(std::function<void(struct map_session_data *)> func)
{
    int i;
    struct map_session_data *sd;

    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd
            && sd->state.auth)
            func(sd);
    }
    return 0;
}

static
int is_deaf(struct block_list *bl)
{
    struct map_session_data *sd = (struct map_session_data *) bl;
    if (!bl || bl->type != BL_PC)
        return 0;
    return sd->special_state.deaf;
}

static
void clif_emotion_towards(struct block_list *bl,
                                  struct block_list *target, int type);

static
char *clif_validate_chat(struct map_session_data *sd, int type,
        const char **message, size_t *message_len);

/*==========================================
 * clif_sendでAREA*指定時用
 *------------------------------------------
 */
static
void clif_send_sub(struct block_list *bl, const unsigned char *buf, int len,
        struct block_list *src_bl, SendWho type)
{
    nullpo_retv(bl);
    struct map_session_data *sd = (struct map_session_data *) bl;

    switch (type)
    {
        case AREA_WOS:
            if (bl && bl == src_bl)
                return;
            break;

        case AREA_CHAT_WOC:
            if (is_deaf(bl)
                && !(bl->type == BL_PC
                     && pc_isGM((struct map_session_data *) src_bl)))
            {
                clif_emotion_towards(src_bl, bl, EMOTE_IGNORED);
                return;
            }
            FALLTHROUGH;
        case AREA_WOC:
            if ((sd && sd->chatID) || (bl && bl == src_bl))
                return;

            break;
        case AREA_WOSC:
            if ((sd) && sd->chatID
                && sd->chatID == ((struct map_session_data *) src_bl)->chatID)
                return;
            break;
    }

    if (session[sd->fd] != NULL)
    {
        if (WFIFOP(sd->fd, 0) == buf)
        {
            PRINTF("WARNING: Invalid use of clif_send function\n");
            PRINTF("         Packet x%4x use a WFIFO of a player instead of to use a buffer.\n",
                 RBUFW(buf, 0));
            PRINTF("         Please correct your code.\n");
            // don't send to not move the pointer of the packet for next sessions in the loop
        }
        else
        {
            if (packet_len_table[RBUFW(buf, 0)])
            {                   // packet must exist
                memcpy(WFIFOP(sd->fd, 0), buf, len);
                WFIFOSET(sd->fd, len);
            }
        }
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_send(const uint8_t *buf, int len, struct block_list *bl, SendWho type)
{
    int i;
    struct map_session_data *sd;
    struct chat_data *cd;
    struct party *p = NULL;
    int x0 = 0, x1 = 0, y0 = 0, y1 = 0;

    if (type != ALL_CLIENT)
    {
        nullpo_ret(bl);

        if (bl->type == BL_PC)
        {
            struct map_session_data *sd2 = (struct map_session_data *) bl;
            if (bool(sd2->status.option & Option::INVISIBILITY))
            {
                // Obscure hidden GMs

                switch (type)
                {
                    case AREA:
                    case AREA_WOC:
                        type = SELF;
                        break;

                    case AREA_WOS:
                    case AREA_WOSC:
                        return 1;

                    default:
                        break;
                }
            }
        }
    }

    switch (type)
    {
        case ALL_CLIENT:       // 全クライアントに送信
            for (i = 0; i < fd_max; i++)
            {
                if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) != NULL
                    && sd->state.auth)
                {
                    if (packet_len_table[RBUFW(buf, 0)])
                    {           // packet must exist
                        memcpy(WFIFOP(i, 0), buf, len);
                        WFIFOSET(i, len);
                    }
                }
            }
            break;
        case ALL_SAMEMAP:      // 同じマップの全クライアントに送信
            for (i = 0; i < fd_max; i++)
            {
                if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) != NULL
                    && sd->state.auth && sd->bl.m == bl->m)
                {
                    if (packet_len_table[RBUFW(buf, 0)])
                    {           // packet must exist
                        memcpy(WFIFOP(i, 0), buf, len);
                        WFIFOSET(i, len);
                    }
                }
            }
            break;
        case AREA:
        case AREA_WOS:
        case AREA_WOC:
        case AREA_WOSC:
            map_foreachinarea(std::bind(clif_send_sub, ph::_1, buf, len, bl, type),
                    bl->m, bl->x - AREA_SIZE, bl->y - AREA_SIZE,
                    bl->x + AREA_SIZE, bl->y + AREA_SIZE, BL_PC);
            break;
        case AREA_CHAT_WOC:
            map_foreachinarea(std::bind(clif_send_sub, ph::_1, buf, len, bl, AREA_CHAT_WOC),
                    bl->m, bl->x - (AREA_SIZE), bl->y - (AREA_SIZE),
                    bl->x + (AREA_SIZE), bl->y + (AREA_SIZE), BL_PC);
            break;
        case CHAT_:
        case CHAT_WOS:
            cd = (struct chat_data *) bl;
            if (bl->type == BL_PC)
            {
                sd = (struct map_session_data *) bl;
                cd = (struct chat_data *) map_id2bl(sd->chatID);
            }
            else if (bl->type != BL_CHAT)
                break;
            if (cd == NULL)
                break;
            for (i = 0; i < cd->users; i++)
            {
                if (type == CHAT_WOS
                    && cd->usersd[i] == (struct map_session_data *) bl)
                    continue;
                if (packet_len_table[RBUFW(buf, 0)])
                {               // packet must exist
                    memcpy(WFIFOP(cd->usersd[i]->fd, 0), buf, len);
                    WFIFOSET(cd->usersd[i]->fd, len);
                }
            }
            break;

        case PARTY_AREA:       // 同じ画面内の全パーティーメンバに送信
        case PARTY_AREA_WOS:   // 自分以外の同じ画面内の全パーティーメンバに送信
            x0 = bl->x - AREA_SIZE;
            y0 = bl->y - AREA_SIZE;
            x1 = bl->x + AREA_SIZE;
            y1 = bl->y + AREA_SIZE;
            FALLTHROUGH;
        case PARTY:            // 全パーティーメンバに送信
        case PARTY_WOS:        // 自分以外の全パーティーメンバに送信
        case PARTY_SAMEMAP:    // 同じマップの全パーティーメンバに送信
        case PARTY_SAMEMAP_WOS:    // 自分以外の同じマップの全パーティーメンバに送信
            if (bl->type == BL_PC)
            {
                sd = (struct map_session_data *) bl;
                if (sd->partyspy > 0)
                {
                    p = party_search(sd->partyspy);
                }
                else
                {
                    if (sd->status.party_id > 0)
                        p = party_search(sd->status.party_id);
                }
            }
            if (p)
            {
                for (i = 0; i < MAX_PARTY; i++)
                {
                    if ((sd = p->member[i].sd) != NULL)
                    {
                        if (sd->bl.id == bl->id && (type == PARTY_WOS ||
                                                    type == PARTY_SAMEMAP_WOS
                                                    || type ==
                                                    PARTY_AREA_WOS))
                            continue;
                        if (type != PARTY && type != PARTY_WOS && bl->m != sd->bl.m)    // マップチェック
                            continue;
                        if ((type == PARTY_AREA || type == PARTY_AREA_WOS) &&
                            (sd->bl.x < x0 || sd->bl.y < y0 ||
                             sd->bl.x > x1 || sd->bl.y > y1))
                            continue;
                        if (packet_len_table[RBUFW(buf, 0)])
                        {       // packet must exist
                            memcpy(WFIFOP(sd->fd, 0), buf, len);
                            WFIFOSET(sd->fd, len);
                        }
                    }
                }
                for (i = 0; i < fd_max; i++)
                {
                    if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) != NULL
                        && sd->state.auth)
                    {
                        if (sd->partyspy == p->party_id)
                        {
                            if (packet_len_table[RBUFW(buf, 0)])
                            {   // packet must exist
                                memcpy(WFIFOP(sd->fd, 0), buf, len);
                                WFIFOSET(sd->fd, len);
                            }
                        }
                    }
                }
            }
            break;
        case SELF:
            sd = (struct map_session_data *) bl;
            if (packet_len_table[RBUFW(buf, 0)])
            {                   // packet must exist
                memcpy(WFIFOP(sd->fd, 0), buf, len);
                WFIFOSET(sd->fd, len);
            }
            break;

        default:
            if (battle_config.error_log)
                PRINTF("clif_send まだ作ってないよー\n");
            return -1;
    }

    return 0;
}

//
// パケット作って送信
//
/*==========================================
 *
 *------------------------------------------
 */
int clif_authok(struct map_session_data *sd)
{
    int fd;

    nullpo_ret(sd);

    if (!sd)
        return 0;

    if (!sd->fd)
        return 0;

    fd = sd->fd;

    WFIFOW(fd, 0) = 0x73;
    WFIFOL(fd, 2) = gettick();
    WFIFOPOS(fd, 6, sd->bl.x, sd->bl.y);
    WFIFOB(fd, 9) = 5;
    WFIFOB(fd, 10) = 5;
    WFIFOSET(fd, packet_len_table[0x73]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_authfail_fd(int fd, int type)
{
    if (!fd || !session[fd])
        return 0;

    WFIFOW(fd, 0) = 0x81;
    WFIFOL(fd, 2) = type;
    WFIFOSET(fd, packet_len_table[0x81]);

    clif_setwaitclose(fd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok(int id)
{
    struct map_session_data *sd;
    int fd;

    if ((sd = map_id2sd(id)) == NULL)
        return 1;

    if (!sd->fd)
        return 1;

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xb3;
    WFIFOB(fd, 2) = 1;
    WFIFOSET(fd, packet_len_table[0xb3]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_set009e(struct flooritem_data *fitem, uint8_t *buf)
{
    int view;

    nullpo_ret(fitem);

    //009e <ID>.l <name ID>.w <identify flag>.B <X>.w <Y>.w <subX>.B <subY>.B <amount>.w
    WBUFW(buf, 0) = 0x9e;
    WBUFL(buf, 2) = fitem->bl.id;
    if ((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
        WBUFW(buf, 6) = view;
    else
        WBUFW(buf, 6) = fitem->item_data.nameid;
    WBUFB(buf, 8) = fitem->item_data.identify;
    WBUFW(buf, 9) = fitem->bl.x;
    WBUFW(buf, 11) = fitem->bl.y;
    WBUFB(buf, 13) = fitem->subx;
    WBUFB(buf, 14) = fitem->suby;
    WBUFW(buf, 15) = fitem->item_data.amount;

    return packet_len_table[0x9e];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem(struct flooritem_data *fitem)
{
    uint8_t buf[64];

    nullpo_ret(fitem);

    if (fitem->item_data.nameid <= 0)
        return 0;
    clif_set009e(fitem, buf);
    clif_send(buf, packet_len_table[0x9e], &fitem->bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearflooritem(struct flooritem_data *fitem, int fd)
{
    unsigned char buf[16];

    nullpo_ret(fitem);

    WBUFW(buf, 0) = 0xa1;
    WBUFL(buf, 2) = fitem->bl.id;

    if (fd == 0)
    {
        clif_send(buf, packet_len_table[0xa1], &fitem->bl, AREA);
    }
    else
    {
        memcpy(WFIFOP(fd, 0), buf, 6);
        WFIFOSET(fd, packet_len_table[0xa1]);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar(struct block_list *bl, int type)
{
    unsigned char buf[16];

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x80;
    WBUFL(buf, 2) = bl->id;
    if (type == 9)
    {
        WBUFB(buf, 6) = 0;
        clif_send(buf, packet_len_table[0x80], bl, AREA);
    }
    else
    {
        WBUFB(buf, 6) = type;
        clif_send(buf, packet_len_table[0x80], bl,
                   type == 1 ? AREA : AREA_WOS);
    }

    return 0;
}

static
void clif_clearchar_delay_sub(timer_id, tick_t, custom_id_t id,
                                     custom_data_t data)
{
    struct block_list *bl = (struct block_list *) id;

    clif_clearchar(bl, data);
    map_freeblock(bl);
}

int clif_clearchar_delay(unsigned int tick, struct block_list *bl, int type)
{
    struct block_list *tmpbl;
    CREATE(tmpbl, struct block_list, 1);

    memcpy(tmpbl, bl, sizeof(struct block_list));
    add_timer(tick, clif_clearchar_delay_sub, (custom_id_t) tmpbl, type);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar_id(int id, int type, int fd)
{
    unsigned char buf[16];

    WBUFW(buf, 0) = 0x80;
    WBUFL(buf, 2) = id;
    WBUFB(buf, 6) = type;
    memcpy(WFIFOP(fd, 0), buf, 7);
    WFIFOSET(fd, packet_len_table[0x80]);

    return 0;
}

/*
static
int current_weapon(struct map_session_data *sd)
{
        if (sd->attack_spell_override)
                return sd->attack_spell_look_override;
        else {
                return sd->status.weapon;
        }
}
*/

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_set0078(struct map_session_data *sd, unsigned char *buf)
{
    nullpo_ret(sd);

    WBUFW(buf, 0) = 0x1d8;
    WBUFL(buf, 2) = sd->bl.id;
    WBUFW(buf, 6) = sd->speed;
    WBUFW(buf, 8) = uint16_t(sd->opt1);
    WBUFW(buf, 10) = uint16_t(sd->opt2);
    WBUFW(buf, 12) = uint16_t(sd->status.option);
    WBUFW(buf, 14) = sd->status.species;
    WBUFW(buf, 16) = sd->status.hair;
    if (sd->attack_spell_override)
        // should be WBUFW ?
        WBUFB(buf, 18) = sd->attack_spell_look_override;
    else
    {
        if (sd->equip_index[EQUIP::WEAPON] >= 0
            && sd->inventory_data[sd->equip_index[EQUIP::WEAPON]])
        {
            if (sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->view_id > 0)
                WBUFW(buf, 18) =
                    sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->view_id;
            else
                WBUFW(buf, 18) =
                    sd->status.inventory[sd->equip_index[EQUIP::WEAPON]].nameid;
        }
        else
            WBUFW(buf, 18) = 0;
    }
    if (sd->equip_index[EQUIP::SHIELD] >= 0
        && sd->equip_index[EQUIP::SHIELD] != sd->equip_index[EQUIP::WEAPON]
        && sd->inventory_data[sd->equip_index[EQUIP::SHIELD]])
    {
        if (sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->view_id > 0)
            WBUFW(buf, 20) = sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->view_id;
        else
            WBUFW(buf, 20) = sd->status.inventory[sd->equip_index[EQUIP::SHIELD]].nameid;
    }
    else
        WBUFW(buf, 20) = 0;
    WBUFW(buf, 22) = sd->status.head_bottom;
    WBUFW(buf, 24) = sd->status.head_top;
    WBUFW(buf, 26) = sd->status.head_mid;
    WBUFW(buf, 28) = sd->status.hair_color;
    WBUFW(buf, 30) = sd->status.clothes_color;
    WBUFW(buf, 32) = static_cast<uint8_t>(sd->head_dir);
    WBUFL(buf, 34) = 0 /*guild_id*/;
    WBUFW(buf, 38) = 0 /*guild_emblem_id*/;
    WBUFW(buf, 40) = sd->status.manner;
    WBUFW(buf, 42) = uint16_t(sd->opt3);
    WBUFB(buf, 44) = sd->status.karma;
    WBUFB(buf, 45) = sd->sex;
    WBUFPOS(buf, 46, sd->bl.x, sd->bl.y);
    // work around ICE in gcc 4.6
    uint8_t dir = static_cast<uint8_t>(sd->dir);
    WBUFB(buf, 48) |= dir;
    WBUFW(buf, 49) = (pc_isGM(sd) == 60 || pc_isGM(sd) == 99) ? 0x80 : 0;
    WBUFB(buf, 51) = sd->state.dead_sit;
    WBUFW(buf, 52) = 0;

    return packet_len_table[0x1d8];
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_set007b(struct map_session_data *sd, unsigned char *buf)
{
    nullpo_ret(sd);

    WBUFW(buf, 0) = 0x1da;
    WBUFL(buf, 2) = sd->bl.id;
    WBUFW(buf, 6) = sd->speed;
    WBUFW(buf, 8) = uint16_t(sd->opt1);
    WBUFW(buf, 10) = uint16_t(sd->opt2);
    WBUFW(buf, 12) = uint16_t(sd->status.option);
    WBUFW(buf, 14) = sd->status.species;
    WBUFW(buf, 16) = sd->status.hair;
    if (sd->equip_index[EQUIP::WEAPON] >= 0
        && sd->inventory_data[sd->equip_index[EQUIP::WEAPON]])
    {
        if (sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->view_id > 0)
            WBUFW(buf, 18) = sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->view_id;
        else
            WBUFW(buf, 18) = sd->status.inventory[sd->equip_index[EQUIP::WEAPON]].nameid;
    }
    else
        WBUFW(buf, 18) = 0;
    if (sd->equip_index[EQUIP::SHIELD] >= 0
        && sd->equip_index[EQUIP::SHIELD] != sd->equip_index[EQUIP::WEAPON]
        && sd->inventory_data[sd->equip_index[EQUIP::SHIELD]])
    {
        if (sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->view_id > 0)
            WBUFW(buf, 20) = sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->view_id;
        else
            WBUFW(buf, 20) = sd->status.inventory[sd->equip_index[EQUIP::SHIELD]].nameid;
    }
    else
        WBUFW(buf, 20) = 0;
    WBUFW(buf, 22) = sd->status.head_bottom;
    WBUFL(buf, 24) = gettick();
    WBUFW(buf, 28) = sd->status.head_top;
    WBUFW(buf, 30) = sd->status.head_mid;
    WBUFW(buf, 32) = sd->status.hair_color;
    WBUFW(buf, 34) = sd->status.clothes_color;
    WBUFW(buf, 36) = static_cast<uint8_t>(sd->head_dir);
    WBUFL(buf, 38) = 0/*guild_id*/;
    WBUFW(buf, 42) = 0/*guild_emblem_id*/;
    WBUFW(buf, 44) = sd->status.manner;
    WBUFW(buf, 46) = uint16_t(sd->opt3);
    WBUFB(buf, 48) = sd->status.karma;
    WBUFB(buf, 49) = sd->sex;
    WBUFPOS2(buf, 50, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
    WBUFW(buf, 55) = pc_isGM(sd) == 60 ? 0x80 : 0;
    WBUFB(buf, 57) = 5;
    WBUFW(buf, 58) = 0;

    return packet_len_table[0x1da];
}

/*==========================================
 * MOB表示1
 *------------------------------------------
 */
static
int clif_mob0078(struct mob_data *md, unsigned char *buf)
{
    int level;

    memset(buf, 0, packet_len_table[0x78]);

    nullpo_ret(md);

    WBUFW(buf, 0) = 0x78;
    WBUFL(buf, 2) = md->bl.id;
    WBUFW(buf, 6) = battle_get_speed(&md->bl);
    WBUFW(buf, 8) = uint16_t(md->opt1);
    WBUFW(buf, 10) = uint16_t(md->opt2);
    WBUFW(buf, 12) = uint16_t(md->option);
    WBUFW(buf, 14) = md->mob_class;
    // snip: stuff do do with disguise as a PC
    WBUFPOS(buf, 46, md->bl.x, md->bl.y);
    // work around ICE in gcc 4.6
    uint8_t dir = static_cast<uint8_t>(md->dir);
    WBUFB(buf, 48) |= dir;
    WBUFB(buf, 49) = 5;
    WBUFB(buf, 50) = 5;
    WBUFW(buf, 52) =
        ((level =
          battle_get_lv(&md->bl)) >
         battle_config.max_lv) ? battle_config.max_lv : level;

    return packet_len_table[0x78];
}

/*==========================================
 * MOB表示2
 *------------------------------------------
 */
static
int clif_mob007b(struct mob_data *md, unsigned char *buf)
{
    int level;

    memset(buf, 0, packet_len_table[0x7b]);

    nullpo_ret(md);

    WBUFW(buf, 0) = 0x7b;
    WBUFL(buf, 2) = md->bl.id;
    WBUFW(buf, 6) = battle_get_speed(&md->bl);
    WBUFW(buf, 8) = uint16_t(md->opt1);
    WBUFW(buf, 10) = uint16_t(md->opt2);
    WBUFW(buf, 12) = uint16_t(md->option);
    WBUFW(buf, 14) = md->mob_class;
    // snip: stuff for monsters disguised as PCs
    WBUFL(buf, 22) = gettick();

    WBUFPOS2(buf, 50, md->bl.x, md->bl.y, md->to_x, md->to_y);
    WBUFB(buf, 56) = 5;
    WBUFB(buf, 57) = 5;
    WBUFW(buf, 58) =
        ((level =
          battle_get_lv(&md->bl)) >
         battle_config.max_lv) ? battle_config.max_lv : level;

    return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_npc0078(struct npc_data *nd, unsigned char *buf)
{
    nullpo_ret(nd);

    memset(buf, 0, packet_len_table[0x78]);

    WBUFW(buf, 0) = 0x78;
    WBUFL(buf, 2) = nd->bl.id;
    WBUFW(buf, 6) = nd->speed;
    WBUFW(buf, 14) = nd->npc_class;
    WBUFPOS(buf, 46, nd->bl.x, nd->bl.y);
    // work around ICE in gcc 4.6
    uint8_t dir = static_cast<uint8_t>(nd->dir);
    WBUFB(buf, 48) |= dir;
    WBUFB(buf, 49) = 5;
    WBUFB(buf, 50) = 5;

    return packet_len_table[0x78];
}

/* These indices are derived from equip_pos in pc.c and some guesswork */
static
earray<EQUIP, LOOK, LOOK::COUNT> equip_points //=
{{
    EQUIP::NONE,    // base
    EQUIP::NONE,    // hair
    EQUIP::WEAPON,  // weapon
    EQUIP::LEGS,    // head botom -- leg armour
    EQUIP::HAT,     // head top -- hat
    EQUIP::TORSO,   // head mid -- torso armour
    EQUIP::NONE,    // hair colour
    EQUIP::NONE,    // clothes colour
    EQUIP::SHIELD,  // shield
    EQUIP::SHOES,   // shoes
    EQUIP::GLOVES,  // gloves
    EQUIP::CAPE,    // cape
    EQUIP::MISC1,   // misc1
    EQUIP::MISC2,   // misc2
}};

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpc(struct map_session_data *sd)
{
    unsigned char buf[128];

    nullpo_ret(sd);

    clif_set0078(sd, buf);

    WBUFW(buf, 0) = 0x1d9;
    WBUFW(buf, 51) = 0;
    clif_send(buf, packet_len_table[0x1d9], &sd->bl, AREA_WOS);

    if (map[sd->bl.m].flag.snow)
        clif_specialeffect(&sd->bl, 162, 1);
    if (map[sd->bl.m].flag.fog)
        clif_specialeffect(&sd->bl, 233, 1);
    if (map[sd->bl.m].flag.sakura)
        clif_specialeffect(&sd->bl, 163, 1);
    if (map[sd->bl.m].flag.leaves)
        clif_specialeffect(&sd->bl, 333, 1);
    if (map[sd->bl.m].flag.rain)
        clif_specialeffect(&sd->bl, 161, 1);

//        clif_changelook_accessories(&sd->bl, NULL);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc(struct npc_data *nd)
{
    unsigned char buf[64];
    int len;

    nullpo_ret(nd);

    if (nd->npc_class < 0 || nd->flag & 1 || nd->npc_class == INVISIBLE_CLASS)
        return 0;

    memset(buf, 0, packet_len_table[0x7c]);

    WBUFW(buf, 0) = 0x7c;
    WBUFL(buf, 2) = nd->bl.id;
    WBUFW(buf, 6) = nd->speed;
    WBUFW(buf, 20) = nd->npc_class;
    WBUFPOS(buf, 36, nd->bl.x, nd->bl.y);

    clif_send(buf, packet_len_table[0x7c], &nd->bl, AREA);

    len = clif_npc0078(nd, buf);
    clif_send(buf, len, &nd->bl, AREA);

    return 0;
}

int clif_spawn_fake_npc_for_player(struct map_session_data *sd, int fake_npc_id)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    if (!fd)
        return 0;

    WFIFOW(fd, 0) = 0x7c;
    WFIFOL(fd, 2) = fake_npc_id;
    WFIFOW(fd, 6) = 0;
    WFIFOW(fd, 8) = 0;
    WFIFOW(fd, 10) = 0;
    WFIFOW(fd, 12) = 0;
    WFIFOW(fd, 20) = 127;
    WFIFOPOS(fd, 36, sd->bl.x, sd->bl.y);
    WFIFOSET(fd, packet_len_table[0x7c]);

    WFIFOW(fd, 0) = 0x78;
    WFIFOL(fd, 2) = fake_npc_id;
    WFIFOW(fd, 6) = 0;
    WFIFOW(fd, 8) = 0;
    WFIFOW(fd, 10) = 0;
    WFIFOW(fd, 12) = 0;
    WFIFOW(fd, 14) = 127;      // identifies as NPC
    WFIFOW(fd, 20) = 127;
    WFIFOPOS(fd, 46, sd->bl.x, sd->bl.y);
    WFIFOPOS(fd, 36, sd->bl.x, sd->bl.y);
    WFIFOB(fd, 49) = 5;
    WFIFOB(fd, 50) = 5;
    WFIFOSET(fd, packet_len_table[0x78]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob(struct mob_data *md)
{
    unsigned char buf[64];
    int len;

    nullpo_ret(md);

    {
        memset(buf, 0, packet_len_table[0x7c]);

        WBUFW(buf, 0) = 0x7c;
        WBUFL(buf, 2) = md->bl.id;
        WBUFW(buf, 6) = md->stats[MOB_SPEED];
        WBUFW(buf, 8) = uint16_t(md->opt1);
        WBUFW(buf, 10) = uint16_t(md->opt2);
        WBUFW(buf, 12) = uint16_t(md->option);
        WBUFW(buf, 20) = md->mob_class;
        WBUFPOS(buf, 36, md->bl.x, md->bl.y);
        clif_send(buf, packet_len_table[0x7c], &md->bl, AREA);
    }

    len = clif_mob0078(md, buf);
    clif_send(buf, len, &md->bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_servertick(struct map_session_data *sd)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x7f;
    WFIFOL(fd, 2) = sd->server_tick;
    WFIFOSET(fd, packet_len_table[0x7f]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok(struct map_session_data *sd)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x87;
    WFIFOL(fd, 2) = gettick();;
    WFIFOPOS2(fd, 6, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
    WFIFOB(fd, 11) = 0;
    WFIFOSET(fd, packet_len_table[0x87]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar(struct map_session_data *sd)
{
    int len;
    unsigned char buf[256];

    nullpo_ret(sd);

    len = clif_set007b(sd, buf);

    clif_send(buf, len, &sd->bl, AREA_WOS);

    if (battle_config.save_clothcolor == 1 && sd->status.clothes_color > 0)
        clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR,
                         sd->status.clothes_color);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_quitsave(int, struct map_session_data *sd)
{
    map_quit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_waitclose(timer_id, tick_t, custom_id_t id, custom_data_t)
{
    if (session[id])
        session[id]->eof = 1;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose(int fd)
{
    add_timer(gettick() + 5000, clif_waitclose, fd, 0);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemap(struct map_session_data *sd, const char *mapname, int x, int y)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;

    WFIFOW(fd, 0) = 0x91;
    memcpy(WFIFOP(fd, 2), mapname, 16);
    WFIFOW(fd, 18) = x;
    WFIFOW(fd, 20) = y;
    WFIFOSET(fd, packet_len_table[0x91]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapserver(struct map_session_data *sd, const char *mapname, int x,
                          int y, struct in_addr ip, int port)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x92;
    memcpy(WFIFOP(fd, 2), mapname, 16);
    WFIFOW(fd, 18) = x;
    WFIFOW(fd, 20) = y;
    WFIFOL(fd, 22) = ip.s_addr;
    WFIFOW(fd, 26) = port;
    WFIFOSET(fd, packet_len_table[0x92]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpos(struct block_list *bl)
{
    uint8_t buf[16];

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x88;
    WBUFL(buf, 2) = bl->id;
    WBUFW(buf, 6) = bl->x;
    WBUFW(buf, 8) = bl->y;

    clif_send(buf, packet_len_table[0x88], bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npcbuysell(struct map_session_data *sd, int id)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xc4;
    WFIFOL(fd, 2) = id;
    WFIFOSET(fd, packet_len_table[0xc4]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_buylist(struct map_session_data *sd, struct npc_data *nd)
{
    struct item_data *id;
    int fd, i, val;

    nullpo_ret(sd);
    nullpo_ret(nd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xc6;
    for (i = 0; nd->u.shop_item[i].nameid > 0; i++)
    {
        id = itemdb_search(nd->u.shop_item[i].nameid);
        val = nd->u.shop_item[i].value;
        WFIFOL(fd, 4 + i * 11) = val; // base price
        WFIFOL(fd, 8 + i * 11) = val; // actual price
        WFIFOB(fd, 12 + i * 11) = uint8_t(id->type);
        if (id->view_id > 0)
            WFIFOW(fd, 13 + i * 11) = id->view_id;
        else
            WFIFOW(fd, 13 + i * 11) = nd->u.shop_item[i].nameid;
    }
    WFIFOW(fd, 2) = i * 11 + 4;
    WFIFOSET(fd, WFIFOW(fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_selllist(struct map_session_data *sd)
{
    int fd, i, c = 0, val;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xc7;
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid > 0 && sd->inventory_data[i])
        {
            val = sd->inventory_data[i]->value_sell;
            if (val < 0)
                continue;
            WFIFOW(fd, 4 + c * 10) = i + 2;
            WFIFOL(fd, 6 + c * 10) = val; // base price
            WFIFOL(fd, 10 + c * 10) = val; // actual price
            c++;
        }
    }
    WFIFOW(fd, 2) = c * 10 + 4;
    WFIFOSET(fd, WFIFOW(fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmes(struct map_session_data *sd, int npcid, const char *mes)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xb4;
    WFIFOW(fd, 2) = strlen(mes) + 9;
    WFIFOL(fd, 4) = npcid;
    strcpy((char *)WFIFOP(fd, 8), mes);
    WFIFOSET(fd, WFIFOW(fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptnext(struct map_session_data *sd, int npcid)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xb5;
    WFIFOL(fd, 2) = npcid;
    WFIFOSET(fd, packet_len_table[0xb5]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptclose(struct map_session_data *sd, int npcid)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xb6;
    WFIFOL(fd, 2) = npcid;
    WFIFOSET(fd, packet_len_table[0xb6]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmenu(struct map_session_data *sd, int npcid, const char *mes)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xb7;
    WFIFOW(fd, 2) = strlen(mes) + 8;
    WFIFOL(fd, 4) = npcid;
    strcpy((char *)WFIFOP(fd, 8), mes);
    WFIFOSET(fd, WFIFOW(fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinput(struct map_session_data *sd, int npcid)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x142;
    WFIFOL(fd, 2) = npcid;
    WFIFOSET(fd, packet_len_table[0x142]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinputstr(struct map_session_data *sd, int npcid)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x1d4;
    WFIFOL(fd, 2) = npcid;
    WFIFOSET(fd, packet_len_table[0x1d4]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_viewpoint(struct map_session_data *sd, int npc_id, int type, int x,
                    int y, int id, int color)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x144;
    WFIFOL(fd, 2) = npc_id;
    WFIFOL(fd, 6) = type;
    WFIFOL(fd, 10) = x;
    WFIFOL(fd, 14) = y;
    WFIFOB(fd, 18) = id;
    WFIFOL(fd, 19) = color;
    WFIFOSET(fd, packet_len_table[0x144]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_cutin(struct map_session_data *sd, const char *image, int type)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x1b3;
    memcpy(WFIFOP(fd, 2), image, 64);
    WFIFOB(fd, 66) = type;
    WFIFOSET(fd, packet_len_table[0x1b3]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem(struct map_session_data *sd, int n, int amount, PickupFail fail)
{
    nullpo_ret(sd);

    int fd = sd->fd;
    if (fail != PickupFail::OKAY)
    {
        WFIFOW(fd, 0) = 0xa0;
        WFIFOW(fd, 2) = n + 2;
        WFIFOW(fd, 4) = amount;
        WFIFOW(fd, 6) = 0;
        WFIFOB(fd, 8) = 0;
        WFIFOB(fd, 9) = 0;
        WFIFOB(fd, 10) = 0;
        WFIFOW(fd, 11) = 0;
        WFIFOW(fd, 13) = 0;
        WFIFOW(fd, 15) = 0;
        WFIFOW(fd, 17) = 0;
        WFIFOW(fd, 19) = 0;
        WFIFOB(fd, 21) = 0;
        WFIFOB(fd, 22) = uint8_t(fail);
    }
    else
    {
        if (n < 0 || n >= MAX_INVENTORY || sd->status.inventory[n].nameid <= 0
            || sd->inventory_data[n] == NULL)
            return 1;

        WFIFOW(fd, 0) = 0xa0;
        WFIFOW(fd, 2) = n + 2;
        WFIFOW(fd, 4) = amount;
        if (sd->inventory_data[n]->view_id > 0)
            WFIFOW(fd, 6) = sd->inventory_data[n]->view_id;
        else
            WFIFOW(fd, 6) = sd->status.inventory[n].nameid;
        WFIFOB(fd, 8) = sd->status.inventory[n].identify;
        if (sd->status.inventory[n].broken == 1)
            WFIFOB(fd, 9) = 1; // is weapon broken [Valaris]
        else
            WFIFOB(fd, 9) = sd->status.inventory[n].attribute;
        WFIFOB(fd, 10) = sd->status.inventory[n].refine;
        if (sd->status.inventory[n].card[0] == 0x00ff
            || sd->status.inventory[n].card[0] == 0x00fe
            || sd->status.inventory[n].card[0] == (short) 0xff00)
        {
            WFIFOW(fd, 11) = sd->status.inventory[n].card[0];
            WFIFOW(fd, 13) = sd->status.inventory[n].card[1];
            WFIFOW(fd, 15) = sd->status.inventory[n].card[2];
            WFIFOW(fd, 17) = sd->status.inventory[n].card[3];
        }
        else
        {
            int j;
            if (sd->status.inventory[n].card[0] > 0
                && (j = itemdb_viewid(sd->status.inventory[n].card[0])) > 0)
                WFIFOW(fd, 11) = j;
            else
                WFIFOW(fd, 11) = sd->status.inventory[n].card[0];
            if (sd->status.inventory[n].card[1] > 0
                && (j = itemdb_viewid(sd->status.inventory[n].card[1])) > 0)
                WFIFOW(fd, 13) = j;
            else
                WFIFOW(fd, 13) = sd->status.inventory[n].card[1];
            if (sd->status.inventory[n].card[2] > 0
                && (j = itemdb_viewid(sd->status.inventory[n].card[2])) > 0)
                WFIFOW(fd, 15) = j;
            else
                WFIFOW(fd, 15) = sd->status.inventory[n].card[2];
            if (sd->status.inventory[n].card[3] > 0
                && (j = itemdb_viewid(sd->status.inventory[n].card[3])) > 0)
                WFIFOW(fd, 17) = j;
            else
                WFIFOW(fd, 17) = sd->status.inventory[n].card[3];
        }
        WFIFOW(fd, 19) = uint16_t(pc_equippoint(sd, n));
        WFIFOB(fd, 21) = uint8_t(sd->inventory_data[n]->type == ItemType::_7
            ? ItemType::WEAPON
            : sd->inventory_data[n]->type);
        WFIFOB(fd, 22) = uint8_t(fail);
    }

    WFIFOSET(fd, packet_len_table[0xa0]);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_delitem(struct map_session_data *sd, int n, int amount)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xaf;
    WFIFOW(fd, 2) = n + 2;
    WFIFOW(fd, 4) = amount;

    WFIFOSET(fd, packet_len_table[0xaf]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_itemlist(struct map_session_data *sd)
{
    nullpo_ret(sd);

    int n = 0;
    int arrow = -1;
    int fd = sd->fd;
    WFIFOW(fd, 0) = 0x1ee;
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid <= 0
            || sd->inventory_data[i] == NULL
            || itemdb_isequip2(sd->inventory_data[i]))
            continue;
        WFIFOW(fd, n * 18 + 4) = i + 2;
        if (sd->inventory_data[i]->view_id > 0)
            WFIFOW(fd, n * 18 + 6) = sd->inventory_data[i]->view_id;
        else
            WFIFOW(fd, n * 18 + 6) = sd->status.inventory[i].nameid;
        WFIFOB(fd, n * 18 + 8) = uint8_t(sd->inventory_data[i]->type);
        WFIFOB(fd, n * 18 + 9) = sd->status.inventory[i].identify;
        WFIFOW(fd, n * 18 + 10) = sd->status.inventory[i].amount;
        if (sd->inventory_data[i]->equip == EPOS::ARROW)
        {
            WFIFOW(fd, n * 18 + 12) = uint16_t(EPOS::ARROW);
            if (bool(sd->status.inventory[i].equip))
                arrow = i;      // ついでに矢装備チェック
        }
        else
            WFIFOW(fd, n * 18 + 12) = uint16_t(EPOS::ZERO);
        WFIFOW(fd, n * 18 + 14) = sd->status.inventory[i].card[0];
        WFIFOW(fd, n * 18 + 16) = sd->status.inventory[i].card[1];
        WFIFOW(fd, n * 18 + 18) = sd->status.inventory[i].card[2];
        WFIFOW(fd, n * 18 + 20) = sd->status.inventory[i].card[3];
        n++;
    }
    if (n)
    {
        WFIFOW(fd, 2) = 4 + n * 18;
        WFIFOSET(fd, WFIFOW(fd, 2));
    }
    if (arrow >= 0)
        clif_arrowequip(sd, arrow);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equiplist(struct map_session_data *sd)
{
    nullpo_ret(sd);

    int fd = sd->fd;
    WFIFOW(fd, 0) = 0xa4;
    int n = 0;
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid <= 0
            || sd->inventory_data[i] == NULL
            || !itemdb_isequip2(sd->inventory_data[i]))
            continue;
        WFIFOW(fd, n * 20 + 4) = i + 2;
        if (sd->inventory_data[i]->view_id > 0)
            WFIFOW(fd, n * 20 + 6) = sd->inventory_data[i]->view_id;
        else
            WFIFOW(fd, n * 20 + 6) = sd->status.inventory[i].nameid;
        WFIFOB(fd, n * 20 + 8) = uint8_t(
                sd->inventory_data[i]->type == ItemType::_7
                ? ItemType::WEAPON
                : sd->inventory_data[i]->type);
        WFIFOB(fd, n * 20 + 9) = sd->status.inventory[i].identify;
        WFIFOW(fd, n * 20 + 10) = uint16_t(pc_equippoint(sd, i));
        WFIFOW(fd, n * 20 + 12) = uint16_t(sd->status.inventory[i].equip);
        if (sd->status.inventory[i].broken == 1)
            WFIFOB(fd, n * 20 + 14) = 1;   // is weapon broken [Valaris]
        else
            WFIFOB(fd, n * 20 + 14) = sd->status.inventory[i].attribute;
        WFIFOB(fd, n * 20 + 15) = sd->status.inventory[i].refine;
        if (sd->status.inventory[i].card[0] == 0x00ff
            || sd->status.inventory[i].card[0] == 0x00fe
            || sd->status.inventory[i].card[0] == (short) 0xff00)
        {
            WFIFOW(fd, n * 20 + 16) = sd->status.inventory[i].card[0];
            WFIFOW(fd, n * 20 + 18) = sd->status.inventory[i].card[1];
            WFIFOW(fd, n * 20 + 20) = sd->status.inventory[i].card[2];
            WFIFOW(fd, n * 20 + 22) = sd->status.inventory[i].card[3];
        }
        else
        {
            int j;
            if (sd->status.inventory[i].card[0] > 0
                && (j = itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
                WFIFOW(fd, n * 20 + 16) = j;
            else
                WFIFOW(fd, n * 20 + 16) = sd->status.inventory[i].card[0];
            if (sd->status.inventory[i].card[1] > 0
                && (j = itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
                WFIFOW(fd, n * 20 + 18) = j;
            else
                WFIFOW(fd, n * 20 + 18) = sd->status.inventory[i].card[1];
            if (sd->status.inventory[i].card[2] > 0
                && (j = itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
                WFIFOW(fd, n * 20 + 20) = j;
            else
                WFIFOW(fd, n * 20 + 20) = sd->status.inventory[i].card[2];
            if (sd->status.inventory[i].card[3] > 0
                && (j = itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
                WFIFOW(fd, n * 20 + 22) = j;
            else
                WFIFOW(fd, n * 20 + 22) = sd->status.inventory[i].card[3];
        }
        n++;
    }
    if (n)
    {
        WFIFOW(fd, 2) = 4 + n * 20;
        WFIFOSET(fd, WFIFOW(fd, 2));
    }
    return 0;
}

/*==========================================
 * カプラさんに預けてある消耗品&収集品リスト
 *------------------------------------------
 */
int clif_storageitemlist(struct map_session_data *sd, struct storage *stor)
{
    nullpo_ret(sd);
    nullpo_ret(stor);

    int fd = sd->fd;
    WFIFOW(fd, 0) = 0x1f0;
    int n = 0;
    for (int i = 0; i < MAX_STORAGE; i++)
    {
        if (stor->storage_[i].nameid <= 0)
            continue;

        struct item_data *id;
        id = itemdb_search(stor->storage_[i].nameid);
        nullpo_ret(id);
        if (itemdb_isequip2(id))
            continue;

        WFIFOW(fd, n * 18 + 4) = i + 1;
        if (id->view_id > 0)
            WFIFOW(fd, n * 18 + 6) = id->view_id;
        else
            WFIFOW(fd, n * 18 + 6) = stor->storage_[i].nameid;
        WFIFOB(fd, n * 18 + 8) = uint8_t(id->type);
        WFIFOB(fd, n * 18 + 9) = stor->storage_[i].identify;
        WFIFOW(fd, n * 18 + 10) = stor->storage_[i].amount;
        WFIFOW(fd, n * 18 + 12) = 0;
        WFIFOW(fd, n * 18 + 14) = stor->storage_[i].card[0];
        WFIFOW(fd, n * 18 + 16) = stor->storage_[i].card[1];
        WFIFOW(fd, n * 18 + 18) = stor->storage_[i].card[2];
        WFIFOW(fd, n * 18 + 20) = stor->storage_[i].card[3];
        n++;
    }
    if (n)
    {
        WFIFOW(fd, 2) = 4 + n * 18;
        WFIFOSET(fd, WFIFOW(fd, 2));
    }
    return 0;
}

/*==========================================
 * カプラさんに預けてある装備リスト
 *------------------------------------------
 */
int clif_storageequiplist(struct map_session_data *sd, struct storage *stor)
{
    nullpo_ret(sd);
    nullpo_ret(stor);

    int fd = sd->fd;
    WFIFOW(fd, 0) = 0xa6;
    int n = 0;
    for (int i = 0; i < MAX_STORAGE; i++)
    {
        if (stor->storage_[i].nameid <= 0)
            continue;

        struct item_data *id;
        id = itemdb_search(stor->storage_[i].nameid);
        nullpo_ret(id);
        if (!itemdb_isequip2(id))
            continue;
        WFIFOW(fd, n * 20 + 4) = i + 1;
        if (id->view_id > 0)
            WFIFOW(fd, n * 20 + 6) = id->view_id;
        else
            WFIFOW(fd, n * 20 + 6) = stor->storage_[i].nameid;
        WFIFOB(fd, n * 20 + 8) = uint8_t(id->type);
        WFIFOB(fd, n * 20 + 9) = stor->storage_[i].identify;
        WFIFOW(fd, n * 20 + 10) = uint16_t(id->equip);
        WFIFOW(fd, n * 20 + 12) = uint16_t(stor->storage_[i].equip);
        if (stor->storage_[i].broken == 1)
            WFIFOB(fd, n * 20 + 14) = 1;   //is weapon broken [Valaris]
        else
            WFIFOB(fd, n * 20 + 14) = stor->storage_[i].attribute;
        WFIFOB(fd, n * 20 + 15) = stor->storage_[i].refine;
        if (stor->storage_[i].card[0] == 0x00ff
            || stor->storage_[i].card[0] == 0x00fe
            || stor->storage_[i].card[0] == (short) 0xff00)
        {
            WFIFOW(fd, n * 20 + 16) = stor->storage_[i].card[0];
            WFIFOW(fd, n * 20 + 18) = stor->storage_[i].card[1];
            WFIFOW(fd, n * 20 + 20) = stor->storage_[i].card[2];
            WFIFOW(fd, n * 20 + 22) = stor->storage_[i].card[3];
        }
        else
        {
            int j;
            if (stor->storage_[i].card[0] > 0
                && (j = itemdb_viewid(stor->storage_[i].card[0])) > 0)
                WFIFOW(fd, n * 20 + 16) = j;
            else
                WFIFOW(fd, n * 20 + 16) = stor->storage_[i].card[0];
            if (stor->storage_[i].card[1] > 0
                && (j = itemdb_viewid(stor->storage_[i].card[1])) > 0)
                WFIFOW(fd, n * 20 + 18) = j;
            else
                WFIFOW(fd, n * 20 + 18) = stor->storage_[i].card[1];
            if (stor->storage_[i].card[2] > 0
                && (j = itemdb_viewid(stor->storage_[i].card[2])) > 0)
                WFIFOW(fd, n * 20 + 20) = j;
            else
                WFIFOW(fd, n * 20 + 20) = stor->storage_[i].card[2];
            if (stor->storage_[i].card[3] > 0
                && (j = itemdb_viewid(stor->storage_[i].card[3])) > 0)
                WFIFOW(fd, n * 20 + 22) = j;
            else
                WFIFOW(fd, n * 20 + 22) = stor->storage_[i].card[3];
        }
        n++;
    }
    if (n)
    {
        WFIFOW(fd, 2) = 4 + n * 20;
        WFIFOSET(fd, WFIFOW(fd, 2));
    }
    return 0;
}

/*==========================================
 * ステータスを送りつける
 * 表示専用数字はこの中で計算して送る
 *------------------------------------------
 */
int clif_updatestatus(struct map_session_data *sd, SP type)
{
    int fd, len = 8;

    nullpo_ret(sd);

    fd = sd->fd;

    WFIFOW(fd, 0) = 0xb0;
    WFIFOW(fd, 2) = uint16_t(type);
    switch (type)
    {
            // 00b0
        case SP_WEIGHT:
            pc_checkweighticon(sd);
            // is this because pc_checkweighticon can send other packets?
            WFIFOW(fd, 0) = 0xb0;
            WFIFOW(fd, 2) = uint16_t(type);
            WFIFOL(fd, 4) = sd->weight;
            break;
        case SP_MAXWEIGHT:
            WFIFOL(fd, 4) = sd->max_weight;
            break;
        case SP_SPEED:
            WFIFOL(fd, 4) = sd->speed;
            break;
        case SP_BASELEVEL:
            WFIFOL(fd, 4) = sd->status.base_level;
            break;
        case SP_JOBLEVEL:
            WFIFOL(fd, 4) = 0;
            break;
        case SP_STATUSPOINT:
            WFIFOL(fd, 4) = sd->status.status_point;
            break;
        case SP_SKILLPOINT:
            WFIFOL(fd, 4) = sd->status.skill_point;
            break;
        case SP_HIT:
            WFIFOL(fd, 4) = sd->hit;
            break;
        case SP_FLEE1:
            WFIFOL(fd, 4) = sd->flee;
            break;
        case SP_FLEE2:
            WFIFOL(fd, 4) = sd->flee2 / 10;
            break;
        case SP_MAXHP:
            WFIFOL(fd, 4) = sd->status.max_hp;
            break;
        case SP_MAXSP:
            WFIFOL(fd, 4) = sd->status.max_sp;
            break;
        case SP_HP:
            WFIFOL(fd, 4) = sd->status.hp;
            break;
        case SP_SP:
            WFIFOL(fd, 4) = sd->status.sp;
            break;
        case SP_ASPD:
            WFIFOL(fd, 4) = sd->aspd;
            break;
        case SP_ATK1:
            WFIFOL(fd, 4) = sd->base_atk + sd->watk;
            break;
        case SP_DEF1:
            WFIFOL(fd, 4) = sd->def;
            break;
        case SP_MDEF1:
            WFIFOL(fd, 4) = sd->mdef;
            break;
        case SP_ATK2:
            WFIFOL(fd, 4) = sd->watk2;
            break;
        case SP_DEF2:
            WFIFOL(fd, 4) = sd->def2;
            break;
        case SP_MDEF2:
            WFIFOL(fd, 4) = sd->mdef2;
            break;
        case SP_CRITICAL:
            WFIFOL(fd, 4) = sd->critical / 10;
            break;
        case SP_MATK1:
            WFIFOL(fd, 4) = sd->matk1;
            break;
        case SP_MATK2:
            WFIFOL(fd, 4) = sd->matk2;
            break;

        case SP_ZENY:
            trade_verifyzeny(sd);
            WFIFOW(fd, 0) = 0xb1;
            if (sd->status.zeny < 0)
                sd->status.zeny = 0;
            WFIFOL(fd, 4) = sd->status.zeny;
            break;
        case SP_BASEEXP:
            WFIFOW(fd, 0) = 0xb1;
            WFIFOL(fd, 4) = sd->status.base_exp;
            break;
        case SP_JOBEXP:
            WFIFOW(fd, 0) = 0xb1;
            WFIFOL(fd, 4) = sd->status.job_exp;
            break;
        case SP_NEXTBASEEXP:
            WFIFOW(fd, 0) = 0xb1;
            WFIFOL(fd, 4) = pc_nextbaseexp(sd);
            break;
        case SP_NEXTJOBEXP:
            WFIFOW(fd, 0) = 0xb1;
            WFIFOL(fd, 4) = pc_nextjobexp(sd);
            break;

            // 00be 終了
        case SP_USTR:
        case SP_UAGI:
        case SP_UVIT:
        case SP_UINT:
        case SP_UDEX:
        case SP_ULUK:
            WFIFOW(fd, 0) = 0xbe;
            WFIFOB(fd, 4) = pc_need_status_point(sd, usp_to_sp(type));
            len = 5;
            break;

            // 013a 終了
        case SP_ATTACKRANGE:
            WFIFOW(fd, 0) = 0x13a;
            WFIFOW(fd, 2) = (sd->attack_spell_override)
                ? sd->attack_spell_range : sd->attackrange;
            len = 4;
            break;

            // 0141 終了
        case SP_STR:
        case SP_AGI:
        case SP_VIT:
        case SP_INT:
        case SP_DEX:
        case SP_LUK:
        {
            ATTR attr = sp_to_attr(type);
            WFIFOW(fd, 0) = 0x141;
            WFIFOL(fd, 2) = uint16_t(type);
            WFIFOL(fd, 6) = sd->status.attrs[attr];
            WFIFOL(fd, 10) = sd->paramb[attr] + sd->parame[attr];
            len = 14;
        }
            break;

        case SP_GM:
            WFIFOL(fd, 4) = pc_isGM(sd);
            break;

        default:
            if (battle_config.error_log)
                PRINTF("clif_updatestatus : make %d routine\n",
                        type);
            return 1;
    }
    WFIFOSET(fd, len);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook(struct block_list *bl, LOOK type, int val)
{
    return clif_changelook_towards(bl, type, val, NULL);
}

int clif_changelook_towards(struct block_list *bl, LOOK type, int val,
                             struct map_session_data *dstsd)
{
    unsigned char buf[32];
    struct map_session_data *sd = NULL;

    nullpo_ret(bl);

    if (bl->type == BL_PC)
        sd = (struct map_session_data *) bl;

    if (sd && bool(sd->status.option & Option::INVISIBILITY))
        return 0;

    if (sd
        && (type == LOOK_WEAPON || type == LOOK_SHIELD || type >= LOOK_SHOES))
    {
        WBUFW(buf, 0) = 0x1d7;
        WBUFL(buf, 2) = bl->id;
        if (type >= LOOK_SHOES)
        {
            EQUIP equip_point = equip_points[type];

            WBUFB(buf, 6) = uint16_t(type);
            if (sd->equip_index[equip_point] >= 0
                && sd->inventory_data[sd->equip_index[equip_point]])
            {
                if (sd->
                    inventory_data[sd->equip_index[equip_point]]->view_id > 0)
                    WBUFW(buf, 7) =
                        sd->inventory_data[sd->
                                           equip_index[equip_point]]->view_id;
                else
                    WBUFW(buf, 7) =
                        sd->status.inventory[sd->
                                             equip_index[equip_point]].nameid;
            }
            else
                WBUFW(buf, 7) = 0;
            WBUFW(buf, 9) = 0;
        }
        else
        {
            WBUFB(buf, 6) = 2;
            if (sd->attack_spell_override)
                WBUFW(buf, 7) = sd->attack_spell_look_override;
            else
            {
                if (sd->equip_index[EQUIP::WEAPON] >= 0
                    && sd->inventory_data[sd->equip_index[EQUIP::WEAPON]])
                {
                    if (sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->view_id > 0)
                        WBUFW(buf, 7) =
                            sd->inventory_data[sd->equip_index[EQUIP::WEAPON]]->view_id;
                    else
                        WBUFW(buf, 7) =
                            sd->status.inventory[sd->equip_index[EQUIP::WEAPON]].nameid;
                }
                else
                    WBUFW(buf, 7) = 0;
            }
            if (sd->equip_index[EQUIP::SHIELD] >= 0
                && sd->equip_index[EQUIP::SHIELD] != sd->equip_index[EQUIP::WEAPON]
                && sd->inventory_data[sd->equip_index[EQUIP::SHIELD]])
            {
                if (sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->view_id > 0)
                    WBUFW(buf, 9) =
                        sd->inventory_data[sd->equip_index[EQUIP::SHIELD]]->view_id;
                else
                    WBUFW(buf, 9) =
                        sd->status.inventory[sd->equip_index[EQUIP::SHIELD]].nameid;
            }
            else
                WBUFW(buf, 9) = 0;
        }
        if (dstsd)
            clif_send(buf, packet_len_table[0x1d7], &dstsd->bl, SELF);
        else
            clif_send(buf, packet_len_table[0x1d7], bl, AREA);
    }
    else
    {
        WBUFW(buf, 0) = 0x1d7;
        WBUFL(buf, 2) = bl->id;
        WBUFB(buf, 6) = uint8_t(type);
        WBUFW(buf, 7) = val;
        WBUFW(buf, 9) = 0;
        if (dstsd)
            clif_send(buf, packet_len_table[0x1d7], &dstsd->bl, SELF);
        else
            clif_send(buf, packet_len_table[0x1d7], bl, AREA);
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_initialstatus(struct map_session_data *sd)
{
    nullpo_ret(sd);

    int fd = sd->fd;

    WFIFOW(fd, 0) = 0xbd;
    WFIFOW(fd, 2) = sd->status.status_point;

    WFIFOB(fd, 4) = min(sd->status.attrs[ATTR::STR], 255);
    WFIFOB(fd, 5) = pc_need_status_point(sd, SP_STR);
    WFIFOB(fd, 6) = min(sd->status.attrs[ATTR::AGI], 255);
    WFIFOB(fd, 7) = pc_need_status_point(sd, SP_AGI);
    WFIFOB(fd, 8) = min(sd->status.attrs[ATTR::VIT], 255);
    WFIFOB(fd, 9) = pc_need_status_point(sd, SP_VIT);
    WFIFOB(fd, 10) = min(sd->status.attrs[ATTR::INT], 255);
    WFIFOB(fd, 11) = pc_need_status_point(sd, SP_INT);
    WFIFOB(fd, 12) = min(sd->status.attrs[ATTR::DEX], 255);
    WFIFOB(fd, 13) = pc_need_status_point(sd, SP_DEX);
    WFIFOB(fd, 14) = min(sd->status.attrs[ATTR::LUK], 255);
    WFIFOB(fd, 15) = pc_need_status_point(sd, SP_LUK);

    WFIFOW(fd, 16) = sd->base_atk + sd->watk;
    WFIFOW(fd, 18) = sd->watk2;    //atk bonus
    WFIFOW(fd, 20) = sd->matk1;
    WFIFOW(fd, 22) = sd->matk2;
    WFIFOW(fd, 24) = sd->def;  // def
    WFIFOW(fd, 26) = sd->def2;
    WFIFOW(fd, 28) = sd->mdef; // mdef
    WFIFOW(fd, 30) = sd->mdef2;
    WFIFOW(fd, 32) = sd->hit;
    WFIFOW(fd, 34) = sd->flee;
    WFIFOW(fd, 36) = sd->flee2 / 10;
    WFIFOW(fd, 38) = sd->critical / 10;
    WFIFOW(fd, 40) = sd->status.karma;
    WFIFOW(fd, 42) = sd->status.manner;

    WFIFOSET(fd, packet_len_table[0xbd]);

    clif_updatestatus(sd, SP_STR);
    clif_updatestatus(sd, SP_AGI);
    clif_updatestatus(sd, SP_VIT);
    clif_updatestatus(sd, SP_INT);
    clif_updatestatus(sd, SP_DEX);
    clif_updatestatus(sd, SP_LUK);

    clif_updatestatus(sd, SP_ATTACKRANGE);
    clif_updatestatus(sd, SP_ASPD);

    return 0;
}

/*==========================================
 *矢装備
 *------------------------------------------
 */
int clif_arrowequip(struct map_session_data *sd, int val)
{
    int fd;

    nullpo_ret(sd);

    if (sd->attacktarget && sd->attacktarget > 0)   // [Valaris]
        sd->attacktarget = 0;

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x013c;
    WFIFOW(fd, 2) = val + 2;   //矢のアイテムID

    WFIFOSET(fd, packet_len_table[0x013c]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_arrow_fail(struct map_session_data *sd, int type)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x013b;
    WFIFOW(fd, 2) = type;

    WFIFOSET(fd, packet_len_table[0x013b]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack(struct map_session_data *sd, SP type, int ok, int val)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xbc;
    WFIFOW(fd, 2) = uint16_t(type);
    WFIFOB(fd, 4) = ok;
    WFIFOB(fd, 5) = val;
    WFIFOSET(fd, packet_len_table[0xbc]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack(struct map_session_data *sd, int n, EPOS pos, int ok)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xaa;
    WFIFOW(fd, 2) = n + 2;
    WFIFOW(fd, 4) = uint16_t(pos);
    WFIFOB(fd, 6) = ok;
    WFIFOSET(fd, packet_len_table[0xaa]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack(struct map_session_data *sd, int n, EPOS pos, int ok)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xac;
    WFIFOW(fd, 2) = n + 2;
    WFIFOW(fd, 4) = uint16_t(pos);
    WFIFOB(fd, 6) = ok;
    WFIFOSET(fd, packet_len_table[0xac]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect(struct block_list *bl, int type)
{
    uint8_t buf[32];

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x19b;
    WBUFL(buf, 2) = bl->id;
    WBUFL(buf, 6) = type;

    clif_send(buf, packet_len_table[0x19b], bl, AREA);

    return 0;
}

/*==========================================
 * 表示オプション変更
 *------------------------------------------
 */
int clif_changeoption(struct block_list *bl)
{
    uint8_t buf[32];
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);

    Option option = *battle_get_option(bl);
    sc_data = battle_get_sc_data(bl);

    WBUFW(buf, 0) = 0x119;
    WBUFL(buf, 2) = bl->id;
    WBUFW(buf, 6) = uint16_t(*battle_get_opt1(bl));
    WBUFW(buf, 8) = uint16_t(*battle_get_opt2(bl));
    WBUFW(buf, 10) = uint16_t(option);
    WBUFB(buf, 12) = 0;        // ??

    clif_send(buf, packet_len_table[0x119], bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack(struct map_session_data *sd, int index, int amount,
                     int ok)
{
    nullpo_ret(sd);

    if (!ok)
    {
        int fd = sd->fd;
        WFIFOW(fd, 0) = 0xa8;
        WFIFOW(fd, 2) = index + 2;
        WFIFOW(fd, 4) = amount;
        WFIFOB(fd, 6) = ok;
        WFIFOSET(fd, packet_len_table[0xa8]);
    }
    else
    {
        uint8_t buf[32];

        WBUFW(buf, 0) = 0x1c8;
        WBUFW(buf, 2) = index + 2;
        if (sd->inventory_data[index]
            && sd->inventory_data[index]->view_id > 0)
            WBUFW(buf, 4) = sd->inventory_data[index]->view_id;
        else
            WBUFW(buf, 4) = sd->status.inventory[index].nameid;
        WBUFL(buf, 6) = sd->bl.id;
        WBUFW(buf, 10) = amount;
        WBUFB(buf, 12) = ok;
        clif_send(buf, packet_len_table[0x1c8], &sd->bl, SELF);
    }

    return 0;
}

/*==========================================
 * 取り引き要請受け
 *------------------------------------------
 */
int clif_traderequest(struct map_session_data *sd, const char *name)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xe5;
    strcpy((char *)WFIFOP(fd, 2), name);
    WFIFOSET(fd, packet_len_table[0xe5]);

    return 0;
}

/*==========================================
 * 取り引き要求応答
 *------------------------------------------
 */
int clif_tradestart(struct map_session_data *sd, int type)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xe7;
    WFIFOB(fd, 2) = type;
    WFIFOSET(fd, packet_len_table[0xe7]);

    return 0;
}

/*==========================================
 * 相手方からのアイテム追加
 *------------------------------------------
 */
int clif_tradeadditem(struct map_session_data *sd,
                       struct map_session_data *tsd, int index, int amount)
{
    int fd, j;

    nullpo_ret(sd);
    nullpo_ret(tsd);

    fd = tsd->fd;
    WFIFOW(fd, 0) = 0xe9;
    WFIFOL(fd, 2) = amount;
    if (index == 0)
    {
        WFIFOW(fd, 6) = 0;     // type id
        WFIFOB(fd, 8) = 0;     //identify flag
        WFIFOB(fd, 9) = 0;     // attribute
        WFIFOB(fd, 10) = 0;    //refine
        WFIFOW(fd, 11) = 0;    //card (4w)
        WFIFOW(fd, 13) = 0;    //card (4w)
        WFIFOW(fd, 15) = 0;    //card (4w)
        WFIFOW(fd, 17) = 0;    //card (4w)
    }
    else
    {
        index -= 2;
        if (sd->inventory_data[index]
            && sd->inventory_data[index]->view_id > 0)
            WFIFOW(fd, 6) = sd->inventory_data[index]->view_id;
        else
            WFIFOW(fd, 6) = sd->status.inventory[index].nameid;    // type id
        WFIFOB(fd, 8) = sd->status.inventory[index].identify;  //identify flag
        if (sd->status.inventory[index].broken == 1)
            WFIFOB(fd, 9) = 1; // is broke weapon [Valaris]
        else
            WFIFOB(fd, 9) = sd->status.inventory[index].attribute; // attribute
        WFIFOB(fd, 10) = sd->status.inventory[index].refine;   //refine
        if (sd->status.inventory[index].card[0] == 0x00ff
            || sd->status.inventory[index].card[0] == 0x00fe
            || sd->status.inventory[index].card[0] == (short) 0xff00)
        {
            WFIFOW(fd, 11) = sd->status.inventory[index].card[0];  //card (4w)
            WFIFOW(fd, 13) = sd->status.inventory[index].card[1];  //card (4w)
            WFIFOW(fd, 15) = sd->status.inventory[index].card[2];  //card (4w)
            WFIFOW(fd, 17) = sd->status.inventory[index].card[3];  //card (4w)
        }
        else
        {
            if (sd->status.inventory[index].card[0] > 0
                && (j =
                    itemdb_viewid(sd->status.inventory[index].card[0])) > 0)
                WFIFOW(fd, 11) = j;
            else
                WFIFOW(fd, 11) = sd->status.inventory[index].card[0];
            if (sd->status.inventory[index].card[1] > 0
                && (j =
                    itemdb_viewid(sd->status.inventory[index].card[1])) > 0)
                WFIFOW(fd, 13) = j;
            else
                WFIFOW(fd, 13) = sd->status.inventory[index].card[1];
            if (sd->status.inventory[index].card[2] > 0
                && (j =
                    itemdb_viewid(sd->status.inventory[index].card[2])) > 0)
                WFIFOW(fd, 15) = j;
            else
                WFIFOW(fd, 15) = sd->status.inventory[index].card[2];
            if (sd->status.inventory[index].card[3] > 0
                && (j =
                    itemdb_viewid(sd->status.inventory[index].card[3])) > 0)
                WFIFOW(fd, 17) = j;
            else
                WFIFOW(fd, 17) = sd->status.inventory[index].card[3];
        }
    }
    WFIFOSET(fd, packet_len_table[0xe9]);

    return 0;
}

/*==========================================
 * アイテム追加成功/失敗
 *------------------------------------------
 */
int clif_tradeitemok(struct map_session_data *sd, int index, int amount,
                      int fail)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x1b1;
    WFIFOW(fd, 2) = index;
    WFIFOW(fd, 4) = amount;
    WFIFOB(fd, 6) = fail;
    WFIFOSET(fd, packet_len_table[0x1b1]);

    return 0;
}

/*==========================================
 * 取り引きok押し
 *------------------------------------------
 */
int clif_tradedeal_lock(struct map_session_data *sd, int fail)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xec;
    WFIFOB(fd, 2) = fail;      // 0=you 1=the other person
    WFIFOSET(fd, packet_len_table[0xec]);

    return 0;
}

/*==========================================
 * 取り引きがキャンセルされました
 *------------------------------------------
 */
int clif_tradecancelled(struct map_session_data *sd)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xee;
    WFIFOSET(fd, packet_len_table[0xee]);

    return 0;
}

/*==========================================
 * 取り引き完了
 *------------------------------------------
 */
int clif_tradecompleted(struct map_session_data *sd, int fail)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xf0;
    WFIFOB(fd, 2) = fail;
    WFIFOSET(fd, packet_len_table[0xf0]);

    return 0;
}

/*==========================================
 * カプラ倉庫のアイテム数を更新
 *------------------------------------------
 */
int clif_updatestorageamount(struct map_session_data *sd,
                              struct storage *stor)
{
    int fd;

    nullpo_ret(sd);
    nullpo_ret(stor);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xf2;      // update storage amount
    WFIFOW(fd, 2) = stor->storage_amount;  //items
    WFIFOW(fd, 4) = MAX_STORAGE;   //items max
    WFIFOSET(fd, packet_len_table[0xf2]);

    return 0;
}

/*==========================================
 * カプラ倉庫にアイテムを追加する
 *------------------------------------------
 */
int clif_storageitemadded(struct map_session_data *sd, struct storage *stor,
                           int index, int amount)
{
    int fd, j;

    nullpo_ret(sd);
    nullpo_ret(stor);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xf4;      // Storage item added
    WFIFOW(fd, 2) = index + 1; // index
    WFIFOL(fd, 4) = amount;    // amount
/*      if ((view = itemdb_viewid(stor->storage_[index].nameid)) > 0)
                WFIFOW(fd,8) =view;
        else*/
    WFIFOW(fd, 8) = stor->storage_[index].nameid;
    WFIFOB(fd, 10) = stor->storage_[index].identify;   //identify flag
    if (stor->storage_[index].broken == 1)
        WFIFOB(fd, 11) = 1;    // is weapon broken [Valaris]
    else
        WFIFOB(fd, 11) = stor->storage_[index].attribute;  // attribute
    WFIFOB(fd, 12) = stor->storage_[index].refine; //refine
    if (stor->storage_[index].card[0] == 0x00ff
        || stor->storage_[index].card[0] == 0x00fe
        || stor->storage_[index].card[0] == (short) 0xff00)
    {
        WFIFOW(fd, 13) = stor->storage_[index].card[0];    //card (4w)
        WFIFOW(fd, 15) = stor->storage_[index].card[1];    //card (4w)
        WFIFOW(fd, 17) = stor->storage_[index].card[2];    //card (4w)
        WFIFOW(fd, 19) = stor->storage_[index].card[3];    //card (4w)
    }
    else
    {
        if (stor->storage_[index].card[0] > 0
            && (j = itemdb_viewid(stor->storage_[index].card[0])) > 0)
            WFIFOW(fd, 13) = j;
        else
            WFIFOW(fd, 13) = stor->storage_[index].card[0];
        if (stor->storage_[index].card[1] > 0
            && (j = itemdb_viewid(stor->storage_[index].card[1])) > 0)
            WFIFOW(fd, 15) = j;
        else
            WFIFOW(fd, 15) = stor->storage_[index].card[1];
        if (stor->storage_[index].card[2] > 0
            && (j = itemdb_viewid(stor->storage_[index].card[2])) > 0)
            WFIFOW(fd, 17) = j;
        else
            WFIFOW(fd, 17) = stor->storage_[index].card[2];
        if (stor->storage_[index].card[3] > 0
            && (j = itemdb_viewid(stor->storage_[index].card[3])) > 0)
            WFIFOW(fd, 19) = j;
        else
            WFIFOW(fd, 19) = stor->storage_[index].card[3];
    }
    WFIFOSET(fd, packet_len_table[0xf4]);

    return 0;
}

/*==========================================
 * カプラ倉庫からアイテムを取り去る
 *------------------------------------------
 */
int clif_storageitemremoved(struct map_session_data *sd, int index,
                             int amount)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xf6;      // Storage item removed
    WFIFOW(fd, 2) = index + 1;
    WFIFOL(fd, 4) = amount;
    WFIFOSET(fd, packet_len_table[0xf6]);

    return 0;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
int clif_storageclose(struct map_session_data *sd)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xf8;      // Storage Closed
    WFIFOSET(fd, packet_len_table[0xf8]);

    return 0;
}

void clif_changelook_accessories(struct block_list *bl,
                             struct map_session_data *dest)
{
    for (LOOK i = LOOK_SHOES; i < LOOK::COUNT; i = LOOK(uint8_t(i) + 1))
        clif_changelook_towards(bl, i, 0, dest);
}

//
// callback系 ?
//
/*==========================================
 * PC表示
 *------------------------------------------
 */
static
void clif_getareachar_pc(struct map_session_data *sd,
                          struct map_session_data *dstsd)
{
    int len;

    if (bool(dstsd->status.option & Option::INVISIBILITY))
        return;

    nullpo_retv(sd);
    nullpo_retv(dstsd);

    if (dstsd->walktimer != -1)
    {
        len = clif_set007b(dstsd, static_cast<uint8_t *>(WFIFOP(sd->fd, 0)));
        WFIFOSET(sd->fd, len);
    }
    else
    {
        len = clif_set0078(dstsd, static_cast<uint8_t *>(WFIFOP(sd->fd, 0)));
        WFIFOSET(sd->fd, len);
    }

    if (battle_config.save_clothcolor == 1 && dstsd->status.clothes_color > 0)
        clif_changelook(&dstsd->bl, LOOK_CLOTHES_COLOR,
                         dstsd->status.clothes_color);

    clif_changelook_accessories(&sd->bl, dstsd);
    clif_changelook_accessories(&dstsd->bl, sd);
}

/*==========================================
 * NPC表示
 *------------------------------------------
 */
static
void clif_getareachar_npc(struct map_session_data *sd, struct npc_data *nd)
{
    int len;

    nullpo_retv(sd);
    nullpo_retv(nd);

    if (nd->npc_class < 0 || nd->flag & 1 || nd->npc_class == INVISIBLE_CLASS)
        return;

    len = clif_npc0078(nd, static_cast<uint8_t *>(WFIFOP(sd->fd, 0)));
    WFIFOSET(sd->fd, len);
}

/*==========================================
 * 移動停止
 *------------------------------------------
 */
int clif_movemob(struct mob_data *md)
{
    unsigned char buf[256];
    int len;

    nullpo_ret(md);

    len = clif_mob007b(md, buf);
    clif_send(buf, len, &md->bl, AREA);

    return 0;
}

/*==========================================
 * モンスターの位置修正
 *------------------------------------------
 */
int clif_fixmobpos(struct mob_data *md)
{
    unsigned char buf[256];
    int len;

    nullpo_ret(md);

    if (md->state.state == MS_WALK)
    {
        len = clif_mob007b(md, buf);
        clif_send(buf, len, &md->bl, AREA);
    }
    else
    {
        len = clif_mob0078(md, buf);
        clif_send(buf, len, &md->bl, AREA);
    }

    return 0;
}

/*==========================================
 * PCの位置修正
 *------------------------------------------
 */
int clif_fixpcpos(struct map_session_data *sd)
{
    unsigned char buf[256];
    int len;

    nullpo_ret(sd);

    if (sd->walktimer != -1)
    {
        len = clif_set007b(sd, buf);
        clif_send(buf, len, &sd->bl, AREA);
    }
    else
    {
        len = clif_set0078(sd, buf);
        clif_send(buf, len, &sd->bl, AREA);
    }
    clif_changelook_accessories(&sd->bl, NULL);

    return 0;
}

/*==========================================
 * 通常攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_damage(struct block_list *src, struct block_list *dst,
                 unsigned int tick, int sdelay, int ddelay, int damage,
                 int div, DamageType type, int damage2)
{
    unsigned char buf[256];
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(src);
    nullpo_ret(dst);

    sc_data = battle_get_sc_data(dst);

    WBUFW(buf, 0) = 0x8a;
    WBUFL(buf, 2) = src->id;
    WBUFL(buf, 6) = dst->id;
    WBUFL(buf, 10) = tick;
    WBUFL(buf, 14) = sdelay;
    WBUFL(buf, 18) = ddelay;
    WBUFW(buf, 22) = (damage > 0x7fff) ? 0x7fff : damage;
    WBUFW(buf, 24) = div;
    WBUFB(buf, 26) = static_cast<uint8_t>(type);
    WBUFW(buf, 27) = damage2;
    clif_send(buf, packet_len_table[0x8a], src, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_getareachar_mob(struct map_session_data *sd, struct mob_data *md)
{
    int len;
    nullpo_retv(sd);
    nullpo_retv(md);

    if (md->state.state == MS_WALK)
    {
        len = clif_mob007b(md, static_cast<uint8_t *>(WFIFOP(sd->fd, 0)));
        WFIFOSET(sd->fd, len);
    }
    else
    {
        len = clif_mob0078(md, static_cast<uint8_t *>(WFIFOP(sd->fd, 0)));
        WFIFOSET(sd->fd, len);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_getareachar_item(struct map_session_data *sd,
                            struct flooritem_data *fitem)
{
    int view, fd;

    nullpo_retv(sd);
    nullpo_retv(fitem);

    fd = sd->fd;
    //009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
    WFIFOW(fd, 0) = 0x9d;
    WFIFOL(fd, 2) = fitem->bl.id;
    if ((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
        WFIFOW(fd, 6) = view;
    else
        WFIFOW(fd, 6) = fitem->item_data.nameid;
    WFIFOB(fd, 8) = fitem->item_data.identify;
    WFIFOW(fd, 9) = fitem->bl.x;
    WFIFOW(fd, 11) = fitem->bl.y;
    WFIFOW(fd, 13) = fitem->item_data.amount;
    WFIFOB(fd, 15) = fitem->subx;
    WFIFOB(fd, 16) = fitem->suby;

    WFIFOSET(fd, packet_len_table[0x9d]);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_getareachar(struct block_list *bl, struct map_session_data *sd)
{
    nullpo_retv(bl);

    switch (bl->type)
    {
        case BL_PC:
            if (sd == (struct map_session_data *) bl)
                break;
            clif_getareachar_pc(sd, (struct map_session_data *) bl);
            break;
        case BL_NPC:
            clif_getareachar_npc(sd, (struct npc_data *) bl);
            break;
        case BL_MOB:
            clif_getareachar_mob(sd, (struct mob_data *) bl);
            break;
        case BL_ITEM:
            clif_getareachar_item(sd, (struct flooritem_data *) bl);
            break;
        default:
            if (battle_config.error_log)
                PRINTF("get area char ??? %d\n",
                        bl->type);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pcoutsight(struct block_list *bl, struct map_session_data *sd)
{
    struct map_session_data *dstsd;

    nullpo_retv(bl);
    nullpo_retv(sd);

    switch (bl->type)
    {
        case BL_PC:
            dstsd = (struct map_session_data *) bl;
            if (sd != dstsd)
            {
                clif_clearchar_id(dstsd->bl.id, 0, sd->fd);
                clif_clearchar_id(sd->bl.id, 0, dstsd->fd);
            }
            break;
        case BL_NPC:
            if (((struct npc_data *) bl)->npc_class != INVISIBLE_CLASS)
                clif_clearchar_id(bl->id, 0, sd->fd);
            break;
        case BL_MOB:
            clif_clearchar_id(bl->id, 0, sd->fd);
            break;
        case BL_ITEM:
            clif_clearflooritem((struct flooritem_data *) bl, sd->fd);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pcinsight(struct block_list *bl, struct map_session_data *sd)
{
    struct map_session_data *dstsd;

    nullpo_retv(bl);
    nullpo_retv(sd);

    switch (bl->type)
    {
        case BL_PC:
            dstsd = (struct map_session_data *) bl;
            if (sd != dstsd)
            {
                clif_getareachar_pc(sd, dstsd);
                clif_getareachar_pc(dstsd, sd);
            }
            break;
        case BL_NPC:
            clif_getareachar_npc(sd, (struct npc_data *) bl);
            break;
        case BL_MOB:
            clif_getareachar_mob(sd, (struct mob_data *) bl);
            break;
        case BL_ITEM:
            clif_getareachar_item(sd, (struct flooritem_data *) bl);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_moboutsight(struct block_list *bl, struct mob_data *md)
{
    struct map_session_data *sd;

    nullpo_retv(bl);
    nullpo_retv(md);

    if (bl->type == BL_PC && (sd = (struct map_session_data *) bl))
    {
        clif_clearchar_id(md->bl.id, 0, sd->fd);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_mobinsight(struct block_list *bl, struct mob_data *md)
{
    struct map_session_data *sd;

    nullpo_retv(bl);
    nullpo_retv(md);

    if (bl->type == BL_PC && (sd = (struct map_session_data *) bl))
    {
        clif_getareachar_mob(sd, md);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillinfo(struct map_session_data *sd, SkillID skillid, int type,
                    int range)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    SkillID id = sd->status.skill[skillid].id;
    if (id == SkillID::ZERO || id == SkillID::NEGATIVE)
        return 0;
    WFIFOW(fd, 0) = 0x147;
    WFIFOW(fd, 2) = uint16_t(id);
    if (type < 0)
        WFIFOW(fd, 4) = skill_get_inf(id);
    else
        WFIFOW(fd, 4) = type;
    WFIFOW(fd, 6) = 0;
    WFIFOW(fd, 8) = sd->status.skill[skillid].lv;
    WFIFOW(fd, 10) = skill_get_sp(id, sd->status.skill[skillid].lv);
    if (range < 0)
    {
        range = skill_get_range(id, sd->status.skill[skillid].lv);
        if (range < 0)
            range = battle_get_range(&sd->bl) - (range + 1);
        WFIFOW(fd, 12) = range;
    }
    else
        WFIFOW(fd, 12) = range;
    memset(WFIFOP(fd, 14), 0, 24);
    WFIFOB(fd, 38) =
        (sd->status.skill[skillid].lv < skill_get_max_raise(id)) ? 1 : 0;
    WFIFOSET(fd, packet_len_table[0x147]);

    return 0;
}

/*==========================================
 * スキルリストを送信する
 *------------------------------------------
 */
int clif_skillinfoblock(struct map_session_data *sd)
{
    int fd;
    int len = 4, range;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x10f;
    for (SkillID i : erange(SkillID(), MAX_SKILL))
    {
        SkillID id = sd->status.skill[i].id;
        if (id != SkillID::ZERO && sd->tmw_version >= 1)
        {
            // [Fate] Version 1 and later don't crash because of bad skill IDs anymore
            WFIFOW(fd, len) = uint16_t(id);
            WFIFOW(fd, len + 2) = skill_get_inf(id);
            WFIFOW(fd, len + 4) = uint16_t(
                skill_db[i].poolflags
                | (sd->status.skill[i].flags & SKILL_POOL_ACTIVATED));
            WFIFOW(fd, len + 6) = sd->status.skill[i].lv;
            WFIFOW(fd, len + 8) = skill_get_sp(id, sd->status.skill[i].lv);
            range = skill_get_range(id, sd->status.skill[i].lv);
            if (range < 0)
                range = battle_get_range(&sd->bl) - (range + 1);
            WFIFOW(fd, len + 10) = range;
            memset(WFIFOP(fd, len + 12), 0, 24);
            WFIFOB(fd, len + 36) =
                (sd->status.skill[i].lv < skill_get_max_raise(id)) ? 1 : 0;
            len += 37;
        }
    }
    WFIFOW(fd, 2) = len;
    WFIFOSET(fd, len);

    return 0;
}

/*==========================================
 * スキル割り振り通知
 *------------------------------------------
 */
int clif_skillup(struct map_session_data *sd, SkillID skill_num)
{
    int range, fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x10e;
    WFIFOW(fd, 2) = uint16_t(skill_num);
    WFIFOW(fd, 4) = sd->status.skill[skill_num].lv;
    WFIFOW(fd, 6) = skill_get_sp(skill_num, sd->status.skill[skill_num].lv);
    range = skill_get_range(skill_num, sd->status.skill[skill_num].lv);
    if (range < 0)
        range = battle_get_range(&sd->bl) - (range + 1);
    WFIFOW(fd, 8) = range;
    WFIFOB(fd, 10) =
        (sd->status.skill[skill_num].lv <
         skill_get_max_raise(sd->status.skill[skill_num].id)) ? 1 : 0;
    WFIFOSET(fd, packet_len_table[0x10e]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel(struct block_list *bl)
{
    unsigned char buf[16];

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x1b9;
    WBUFL(buf, 2) = bl->id;
    clif_send(buf, packet_len_table[0x1b9], bl, AREA);

    return 0;
}

/*==========================================
 * スキル詠唱失敗
 *------------------------------------------
 */
int clif_skill_fail(struct map_session_data *sd, SkillID skill_id, int type,
                     int btype)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;

    if (type == 0x4 && battle_config.display_delay_skill_fail == 0)
    {
        return 0;
    }

    WFIFOW(fd, 0) = 0x110;
    WFIFOW(fd, 2) = uint16_t(skill_id);
    WFIFOW(fd, 4) = btype;
    WFIFOW(fd, 6) = 0;
    WFIFOB(fd, 8) = 0;
    WFIFOB(fd, 9) = type;
    WFIFOSET(fd, packet_len_table[0x110]);

    return 0;
}

/*==========================================
 * スキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage(struct block_list *src, struct block_list *dst,
                       unsigned int tick, int sdelay, int ddelay, int damage,
                       int div, SkillID skill_id, int skill_lv, int type)
{
    unsigned char buf[64];
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(src);
    nullpo_ret(dst);

    sc_data = battle_get_sc_data(dst);

    WBUFW(buf, 0) = 0x1de;
    WBUFW(buf, 2) = uint16_t(skill_id);
    WBUFL(buf, 4) = src->id;
    WBUFL(buf, 8) = dst->id;
    WBUFL(buf, 12) = tick;
    WBUFL(buf, 16) = sdelay;
    WBUFL(buf, 20) = ddelay;
    WBUFL(buf, 24) = damage;
    WBUFW(buf, 28) = skill_lv;
    WBUFW(buf, 30) = div;
    WBUFB(buf, 32) = (type > 0) ? type : skill_get_hit(skill_id);
    clif_send(buf, packet_len_table[0x1de], src, AREA);

    return 0;
}

/*==========================================
 * 状態異常アイコン/メッセージ表示
 *------------------------------------------
 */
int clif_status_change(struct block_list *bl, StatusChange type, int flag)
{
    unsigned char buf[16];

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x0196;
    WBUFW(buf, 2) = uint16_t(type);
    WBUFL(buf, 4) = bl->id;
    WBUFB(buf, 8) = flag;
    clif_send(buf, packet_len_table[0x196], bl, AREA);
    return 0;
}

/*==========================================
 * Send message (modified by [Yor])
 *------------------------------------------
 */
void clif_displaymessage(int fd, const_string mes)
{
    if (mes)
    {
        // don't send a void message (it's not displaying on the client chat). @help can send void line.
        WFIFOW(fd, 0) = 0x8e;
        WFIFOW(fd, 2) = 5 + mes.size();   // 4 + len + NULL teminate
        memcpy(WFIFOP(fd, 4), mes.data(), mes.size());
        WFIFOB(fd, 4 + mes.size()) = '\0';
        WFIFOSET(fd, 5 + mes.size());
    }
}

/*==========================================
 * 天の声を送信する
 *------------------------------------------
 */
void clif_GMmessage(struct block_list *bl, const_string mes, int flag)
{
    unsigned char buf[mes.size() + 16];
    int lp = (flag & 0x10) ? 8 : 4;

    WBUFW(buf, 0) = 0x9a;
    WBUFW(buf, 2) = mes.size() + 1 + lp;
    WBUFL(buf, 4) = 0x65756c62;
    memcpy(WBUFP(buf, lp), mes.data(), mes.size());
    WBUFB(buf, lp + mes.size()) = '\0';
    flag &= 0x07;
    clif_send(buf, WBUFW(buf, 2), bl,
               (flag == 1) ? ALL_SAMEMAP :
               (flag == 2) ? AREA :
               (flag == 3) ? SELF :
               ALL_CLIENT);
}

/*==========================================
 * 復活する
 *------------------------------------------
 */
int clif_resurrection(struct block_list *bl, int type)
{
    unsigned char buf[16];

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x148;
    WBUFL(buf, 2) = bl->id;
    WBUFW(buf, 6) = type;

    clif_send(buf, packet_len_table[0x148], bl, type == 1 ? AREA : AREA_WOS);

    return 0;
}

/*==========================================
 * Wisp/page is transmitted to the destination player
 *------------------------------------------
 */
int clif_wis_message(int fd, const char *nick, const char *mes, int mes_len)   // R 0097 <len>.w <nick>.24B <message>.?B
{
    WFIFOW(fd, 0) = 0x97;
    WFIFOW(fd, 2) = mes_len + 24 + 4;
    memcpy(WFIFOP(fd, 4), nick, 24);
    memcpy(WFIFOP(fd, 28), mes, mes_len);
    WFIFOSET(fd, WFIFOW(fd, 2));
    return 0;
}

/*==========================================
 * The transmission result of Wisp/page is transmitted to the source player
 *------------------------------------------
 */
int clif_wis_end(int fd, int flag) // R 0098 <type>.B: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
{
    WFIFOW(fd, 0) = 0x98;
    WFIFOW(fd, 2) = flag;
    WFIFOSET(fd, packet_len_table[0x98]);
    return 0;
}

/*==========================================
 * パーティ作成完了
 * Relay the result of party creation.
 *
 * (R 00fa <flag>.B)
 *
 * flag:
 *  0 The party was created.
 *  1 The party name is invalid/taken.
 *  2 The character is already in a party.
 *------------------------------------------
 */
int clif_party_created(struct map_session_data *sd, int flag)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xfa;
    WFIFOB(fd, 2) = flag;
    WFIFOSET(fd, packet_len_table[0xfa]);
    return 0;
}

/*==========================================
 * パーティ情報送信
 *------------------------------------------
 */
int clif_party_info(struct party *p, int fd)
{
    unsigned char buf[1024];
    int i, c;
    struct map_session_data *sd = NULL;

    nullpo_ret(p);

    WBUFW(buf, 0) = 0xfb;
    memcpy(WBUFP(buf, 4), p->name, 24);
    for (i = c = 0; i < MAX_PARTY; i++)
    {
        struct party_member *m = &p->member[i];
        if (m->account_id > 0)
        {
            if (sd == NULL)
                sd = m->sd;
            WBUFL(buf, 28 + c * 46) = m->account_id;
            memcpy(WBUFP(buf, 28 + c * 46 + 4), m->name, 24);
            memcpy(WBUFP(buf, 28 + c * 46 + 28), m->map, 16);
            WBUFB(buf, 28 + c * 46 + 44) = (m->leader) ? 0 : 1;
            WBUFB(buf, 28 + c * 46 + 45) = (m->online) ? 0 : 1;
            c++;
        }
    }
    WBUFW(buf, 2) = 28 + c * 46;
    if (fd >= 0)
    {                           // fdが設定されてるならそれに送る
        memcpy(WFIFOP(fd, 0), buf, WBUFW(buf, 2));
        WFIFOSET(fd, WFIFOW(fd, 2));
        return 9;
    }
    if (sd != NULL)
        clif_send(buf, WBUFW(buf, 2), &sd->bl, PARTY);
    return 0;
}

/*==========================================
 * パーティ勧誘
 * Relay a party invitation.
 *
 * (R 00fe <sender_ID>.l <party_name>.24B)
 *------------------------------------------
 */
int clif_party_invite(struct map_session_data *sd,
                       struct map_session_data *tsd)
{
    int fd;
    struct party *p;

    nullpo_ret(sd);
    nullpo_ret(tsd);

    fd = tsd->fd;

    if (!(p = party_search(sd->status.party_id)))
        return 0;

    WFIFOW(fd, 0) = 0xfe;
    WFIFOL(fd, 2) = sd->status.account_id;
    memcpy(WFIFOP(fd, 6), p->name, 24);
    WFIFOSET(fd, packet_len_table[0xfe]);
    return 0;
}

/*==========================================
 * パーティ勧誘結果
 * Relay the response to a party invitation.
 *
 * (R 00fd <name>.24B <flag>.B)
 *
 * flag:
 *  0 The character is already in a party.
 *  1 The invitation was rejected.
 *  2 The invitation was accepted.
 *  3 The party is full.
 *  4 The character is in the same party.
 *------------------------------------------
 */
int clif_party_inviteack(struct map_session_data *sd, const char *nick, int flag)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xfd;
    memcpy(WFIFOP(fd, 2), nick, 24);
    WFIFOB(fd, 26) = flag;
    WFIFOSET(fd, packet_len_table[0xfd]);
    return 0;
}

/*==========================================
 * パーティ設定送信
 * flag & 0x001=exp変更ミス
 *        0x010=item変更ミス
 *        0x100=一人にのみ送信
 *------------------------------------------
 */
int clif_party_option(struct party *p, struct map_session_data *sd, int flag)
{
    unsigned char buf[16];

    nullpo_ret(p);

//  if(battle_config.etc_log)
//      PRINTF("clif_party_option: %d %d %d\n",p->exp,p->item,flag);
    if (sd == NULL && flag == 0)
    {
        int i;
        for (i = 0; i < MAX_PARTY; i++)
            if ((sd = map_id2sd(p->member[i].account_id)) != NULL)
                break;
    }
    if (sd == NULL)
        return 0;
    WBUFW(buf, 0) = 0x101;
    WBUFW(buf, 2) = ((flag & 0x01) ? 2 : p->exp);
    WBUFW(buf, 4) = ((flag & 0x10) ? 2 : p->item);
    if (flag == 0)
        clif_send(buf, packet_len_table[0x101], &sd->bl, PARTY);
    else
    {
        memcpy(WFIFOP(sd->fd, 0), buf, packet_len_table[0x101]);
        WFIFOSET(sd->fd, packet_len_table[0x101]);
    }
    return 0;
}

/*==========================================
 * パーティ脱退（脱退前に呼ぶこと）
 *------------------------------------------
 */
int clif_party_leaved(struct party *p, struct map_session_data *sd,
                       int account_id, const char *name, int flag)
{
    unsigned char buf[64];
    int i;

    nullpo_ret(p);

    WBUFW(buf, 0) = 0x105;
    WBUFL(buf, 2) = account_id;
    memcpy(WBUFP(buf, 6), name, 24);
    WBUFB(buf, 30) = flag & 0x0f;

    if ((flag & 0xf0) == 0)
    {
        if (sd == NULL)
            for (i = 0; i < MAX_PARTY; i++)
                if ((sd = p->member[i].sd) != NULL)
                    break;
        if (sd != NULL)
            clif_send(buf, packet_len_table[0x105], &sd->bl, PARTY);
    }
    else if (sd != NULL)
    {
        memcpy(WFIFOP(sd->fd, 0), buf, packet_len_table[0x105]);
        WFIFOSET(sd->fd, packet_len_table[0x105]);
    }
    return 0;
}

/*==========================================
 * パーティメッセージ送信
 *------------------------------------------
 */
int clif_party_message(struct party *p, int account_id, const char *mes, int len)
{
    // always set, but clang is not smart enough
    struct map_session_data *sd = nullptr;
    int i;

    nullpo_ret(p);

    for (i = 0; i < MAX_PARTY; i++)
    {
        if ((sd = p->member[i].sd) != NULL)
            break;
    }
    if (sd != NULL)
    {
        unsigned char buf[1024];
        WBUFW(buf, 0) = 0x109;
        WBUFW(buf, 2) = len + 8;
        WBUFL(buf, 4) = account_id;
        memcpy(WBUFP(buf, 8), mes, len);
        clif_send(buf, len + 8, &sd->bl, PARTY);
    }
    return 0;
}

/*==========================================
 * パーティ座標通知
 *------------------------------------------
 */
int clif_party_xy(struct party *, struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_ret(sd);

    WBUFW(buf, 0) = 0x107;
    WBUFL(buf, 2) = sd->status.account_id;
    WBUFW(buf, 6) = sd->bl.x;
    WBUFW(buf, 8) = sd->bl.y;
    clif_send(buf, packet_len_table[0x107], &sd->bl, PARTY_SAMEMAP_WOS);
//  if(battle_config.etc_log)
//      PRINTF("clif_party_xy %d\n",sd->status.account_id);
    return 0;
}

/*==========================================
 * パーティHP通知
 *------------------------------------------
 */
int clif_party_hp(struct party *, struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_ret(sd);

    WBUFW(buf, 0) = 0x106;
    WBUFL(buf, 2) = sd->status.account_id;
    WBUFW(buf, 6) = (sd->status.hp > 0x7fff) ? 0x7fff : sd->status.hp;
    WBUFW(buf, 8) =
        (sd->status.max_hp > 0x7fff) ? 0x7fff : sd->status.max_hp;
    clif_send(buf, packet_len_table[0x106], &sd->bl, PARTY_AREA_WOS);
//  if(battle_config.etc_log)
//      PRINTF("clif_party_hp %d\n",sd->status.account_id);
    return 0;
}

/*==========================================
 * 攻撃するために移動が必要
 *------------------------------------------
 */
int clif_movetoattack(struct map_session_data *sd, struct block_list *bl)
{
    int fd;

    nullpo_ret(sd);
    nullpo_ret(bl);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0x139;
    WFIFOL(fd, 2) = bl->id;
    WFIFOW(fd, 6) = bl->x;
    WFIFOW(fd, 8) = bl->y;
    WFIFOW(fd, 10) = sd->bl.x;
    WFIFOW(fd, 12) = sd->bl.y;
    WFIFOW(fd, 14) = sd->attackrange;
    WFIFOSET(fd, packet_len_table[0x139]);
    return 0;
}

/*==========================================
 * MVPエフェクト
 *------------------------------------------
 */
int clif_mvp_effect(struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_ret(sd);

    WBUFW(buf, 0) = 0x10c;
    WBUFL(buf, 2) = sd->bl.id;
    clif_send(buf, packet_len_table[0x10c], &sd->bl, AREA);
    return 0;
}

/*==========================================
 * エモーション
 *------------------------------------------
 */
void clif_emotion(struct block_list *bl, int type)
{
    unsigned char buf[8];

    nullpo_retv(bl);

    WBUFW(buf, 0) = 0xc0;
    WBUFL(buf, 2) = bl->id;
    WBUFB(buf, 6) = type;
    clif_send(buf, packet_len_table[0xc0], bl, AREA);
}

static
void clif_emotion_towards(struct block_list *bl,
                                  struct block_list *target, int type)
{
    unsigned char buf[8];
    int len = packet_len_table[0xc0];
    struct map_session_data *sd = (struct map_session_data *) target;

    nullpo_retv(bl);
    nullpo_retv(target);

    if (target->type != BL_PC)
        return;

    WBUFW(buf, 0) = 0xc0;
    WBUFL(buf, 2) = bl->id;
    WBUFB(buf, 6) = type;

    memcpy(WFIFOP(sd->fd, 0), buf, len);
    WFIFOSET(sd->fd, len);
}

/*==========================================
 * 座る
 *------------------------------------------
 */
void clif_sitting(int, struct map_session_data *sd)
{
    unsigned char buf[64];

    nullpo_retv(sd);

    WBUFW(buf, 0) = 0x8a;
    WBUFL(buf, 2) = sd->bl.id;
    WBUFB(buf, 26) = 2;
    clif_send(buf, packet_len_table[0x8a], &sd->bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_GM_kickack(struct map_session_data *sd, int id)
{
    int fd;

    nullpo_ret(sd);

    fd = sd->fd;
    WFIFOW(fd, 0) = 0xcd;
    WFIFOL(fd, 2) = id;
    WFIFOSET(fd, packet_len_table[0xcd]);
    return 0;
}

static
void clif_parse_QuitGame(int fd, struct map_session_data *sd);

int clif_GM_kick(struct map_session_data *sd, struct map_session_data *tsd,
                  int type)
{
    nullpo_ret(tsd);

    if (type)
        clif_GM_kickack(sd, tsd->status.account_id);
    tsd->opt1 = Opt1::ZERO;
    tsd->opt2 = Opt2::ZERO;
    clif_parse_QuitGame(tsd->fd, tsd);

    return 0;
}

// displaying special effects (npcs, weather, etc) [Valaris]
int clif_specialeffect(struct block_list *bl, int type, int flag)
{
    unsigned char buf[24];

    nullpo_ret(bl);

    memset(buf, 0, packet_len_table[0x19b]);

    WBUFW(buf, 0) = 0x19b;
    WBUFL(buf, 2) = bl->id;
    WBUFL(buf, 6) = type;

    if (flag == 2)
    {
        struct map_session_data *sd = NULL;
        int i;
        for (i = 0; i < fd_max; i++)
        {
            if (session[i] && (sd = (struct map_session_data *)session[i]->session_data) != NULL
                && sd->state.auth && sd->bl.m == bl->m)
                clif_specialeffect(&sd->bl, type, 1);
        }
    }

    else if (flag == 1)
        clif_send(buf, packet_len_table[0x19b], bl, SELF);
    else if (!flag)
        clif_send(buf, packet_len_table[0x19b], bl, AREA);

    return 0;

}

// ------------
// clif_parse_*
// ------------
// パケット読み取って色々操作
/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_WantToConnection(int fd, struct map_session_data *sd)
{
    struct map_session_data *old_sd;
    int account_id;            // account_id in the packet

    if (sd)
    {
        if (battle_config.error_log)
            PRINTF("clif_parse_WantToConnection : invalid request?\n");
        return;
    }

    if (RFIFOW(fd, 0) == 0x72)
    {
        account_id = RFIFOL(fd, 2);
    }
    else
        return;                 // Not the auth packet

    WFIFOL(fd, 0) = account_id;
    WFIFOSET(fd, 4);

    // if same account already connected, we disconnect the 2 sessions
    if ((old_sd = map_id2sd(account_id)) != NULL)
    {
        clif_authfail_fd(fd, 2);   // same id
        clif_authfail_fd(old_sd->fd, 2);   // same id
        PRINTF("clif_parse_WantToConnection: Double connection for account %d (sessions: #%d (new) and #%d (old)).\n",
             account_id, fd, old_sd->fd);
    }
    else
    {
        CREATE(sd, struct map_session_data, 1);
        session[fd]->session_data = sd;
        sd->fd = fd;

        pc_setnewpc(sd, account_id, RFIFOL(fd, 6), RFIFOL(fd, 10),
                     RFIFOL(fd, 14), RFIFOB(fd, 18), fd);

        map_addiddb(&sd->bl);

        chrif_authreq(sd);
    }

    return;
}

/*==========================================
 * 007d クライアント側マップ読み込み完了
 * map侵入時に必要なデータを全て送りつける
 *------------------------------------------
 */
static
void clif_parse_LoadEndAck(int, struct map_session_data *sd)
{
//  struct item_data* item;
    int i;
    nullpo_retv(sd);

    if (sd->bl.prev != NULL)
        return;

    // 接続ok時
    //clif_authok();
    if (sd->npc_id)
        npc_event_dequeue(sd);
    clif_skillinfoblock(sd);
    pc_checkitem(sd);
    //guild_info();

    // loadendack時
    // next exp
    clif_updatestatus(sd, SP_NEXTBASEEXP);
    clif_updatestatus(sd, SP_NEXTJOBEXP);
    // skill point
    clif_updatestatus(sd, SP_SKILLPOINT);
    // item
    clif_itemlist(sd);
    clif_equiplist(sd);
    // param all
    clif_initialstatus(sd);
    // party
    party_send_movemap(sd);
    // 119
    // 78

    if (battle_config.pc_invincible_time > 0)
    {
        pc_setinvincibletimer(sd, battle_config.pc_invincible_time);
    }

    map_addblock(&sd->bl);     // ブロック登録
    clif_spawnpc(sd);          // spawn

    // weight max , now
    clif_updatestatus(sd, SP_MAXWEIGHT);
    clif_updatestatus(sd, SP_WEIGHT);

    // pvp
    if (sd->pvp_timer != -1 && !battle_config.pk_mode)
        delete_timer(sd->pvp_timer, pc_calc_pvprank_timer);
    if (map[sd->bl.m].flag.pvp)
    {
        if (!battle_config.pk_mode)
        {                       // remove pvp stuff for pk_mode [Valaris]
            sd->pvp_timer =
                add_timer(gettick() + 200, pc_calc_pvprank_timer, sd->bl.id,
                           0);
            sd->pvp_rank = 0;
            sd->pvp_lastusers = 0;
            sd->pvp_point = 5;
        }
    }
    else
    {
        sd->pvp_timer = -1;
    }

    sd->state.connect_new = 0;

    // view equipment item
    clif_changelook(&sd->bl, LOOK_WEAPON, 0);
    if (battle_config.save_clothcolor == 1 && sd->status.clothes_color > 0)
        clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR,
                         sd->status.clothes_color);

    if (battle_config.muting_players && sd->status.manner < 0)
        skill_status_change_start(&sd->bl, SC_NOCHAT, 0, 0, 0, 0, 0, 0);

    // option
    clif_changeoption(&sd->bl);
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (bool(sd->status.inventory[i].equip)
            && bool(sd->status.inventory[i].equip & EPOS::WEAPON)
            && sd->status.inventory[i].broken == 1)
            skill_status_change_start(&sd->bl, SC_BROKNWEAPON, 0, 0, 0, 0, 0,
                                       0);
        if (bool(sd->status.inventory[i].equip)
            && bool(sd->status.inventory[i].equip & EPOS::MISC1)
            && sd->status.inventory[i].broken == 1)
            skill_status_change_start(&sd->bl, SC_BROKNARMOR, 0, 0, 0, 0, 0,
                                       0);
    }

//        clif_changelook_accessories(sd, NULL);

    map_foreachinarea(std::bind(clif_getareachar, ph::_1, sd), sd->bl.m, sd->bl.x - AREA_SIZE,
                       sd->bl.y - AREA_SIZE, sd->bl.x + AREA_SIZE,
                       sd->bl.y + AREA_SIZE, BL_NUL);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_TickSend(int fd, struct map_session_data *sd)
{
    nullpo_retv(sd);

    sd->client_tick = RFIFOL(fd, 2);
    sd->server_tick = gettick();
    clif_servertick(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_WalkToXY(int fd, struct map_session_data *sd)
{
    int x, y;

    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }

    if (sd->npc_id != 0 || sd->state.storage_open)
        return;

    if (sd->chatID)
        return;

    if (sd->canmove_tick > gettick())
        return;

    // ステータス異常やハイディング中(トンネルドライブ無)で動けない
    if ((bool(sd->opt1) && sd->opt1 != (Opt1::_stone6))
        || sd->sc_data[SC_ANKLE].timer != -1 //アンクルスネア
        )
        return;

    if (sd->invincible_timer != -1)
        pc_delinvincibletimer(sd);

    pc_stopattack(sd);

    x = RFIFOB(fd, 2) * 4 + (RFIFOB(fd, 3) >> 6);
    y = ((RFIFOB(fd, 3) & 0x3f) << 4) + (RFIFOB(fd, 4) >> 4);
    pc_walktoxy(sd, x, y);

}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_QuitGame(int fd, struct map_session_data *sd)
{
    unsigned int tick = gettick();

    nullpo_retv(sd);

    WFIFOW(fd, 0) = 0x18b;
    if ((!pc_isdead(sd)
         && (sd->opt1 != Opt1::ZERO
             || (sd->opt2 != Opt2::ZERO
                 && !(night_flag == 1 && sd->opt2 == Opt2::BLIND))))
        || (DIFF_TICK(tick, sd->canact_tick) < 0)
        )
    {
        WFIFOW(fd, 2) = 1;
        WFIFOSET(fd, packet_len_table[0x18b]);
        return;
    }

    /*  Rovert's prevent logout option fixed [Valaris]  */
    if ((battle_config.prevent_logout
         && (gettick() - sd->canlog_tick) >= 10000)
        || (!battle_config.prevent_logout))
    {
        clif_setwaitclose(fd);
        WFIFOW(fd, 2) = 0;
    }
    else
    {
        WFIFOW(fd, 2) = 1;
    }
    WFIFOSET(fd, packet_len_table[0x18b]);

}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_GetCharNameRequest(int fd, struct map_session_data *sd)
{
    struct block_list *bl;
    int account_id;

    account_id = RFIFOL(fd, 2);
    bl = map_id2bl(account_id);
    if (bl == NULL)
        return;

    WFIFOW(fd, 0) = 0x95;
    WFIFOL(fd, 2) = account_id;

    switch (bl->type)
    {
        case BL_PC:
        {
            struct map_session_data *ssd = (struct map_session_data *) bl;

            nullpo_retv(ssd);

            if (ssd->state.shroud_active)
                memset(WFIFOP(fd, 6), 0, 24);
            else
                memcpy(WFIFOP(fd, 6), ssd->status.name, 24);
            WFIFOSET(fd, packet_len_table[0x95]);

            struct party *p = NULL;

            const char *party_name = "";

            int send = 0;

            if (ssd->status.party_id > 0 && (p = party_search(ssd->status.party_id)) != NULL)
            {
                party_name = p->name;
                send = 1;
            }

            if (send)
            {
                WFIFOW(fd, 0) = 0x195;
                WFIFOL(fd, 2) = account_id;
                memcpy(WFIFOP(fd, 6), party_name, 24);
                memcpy(WFIFOP(fd, 30), "", 24);
                memcpy(WFIFOP(fd, 54), "", 24);
                memcpy(WFIFOP(fd, 78), "", 24); // We send this value twice because the client expects it
                WFIFOSET(fd, packet_len_table[0x195]);

            }

            if (pc_isGM(sd) >= battle_config.hack_info_GM_level)
            {
                struct in_addr ip = ssd->ip;
                WFIFOW(fd, 0) = 0x20C;

                // Mask the IP using the char-server password
                if (battle_config.mask_ip_gms)
                    ip = MD5_ip(chrif_getpasswd(), ssd->ip);

                WFIFOL(fd, 2) = account_id;
                WFIFOL(fd, 6) = ip.s_addr;
                WFIFOSET(fd, packet_len_table[0x20C]);
             }

        }
            break;
        case BL_NPC:
            memcpy(WFIFOP(fd, 6), ((struct npc_data *) bl)->name, 24);
            {
                char *start = (char *)WFIFOP(fd, 6);
                char *end = strchr(start, '#');    // [fate] elim hashed out/invisible names for the client
                if (end)
                    while (*end)
                        *end++ = 0;

                // [fate] Elim preceding underscores for (hackish) name position fine-tuning
                while (*start == '_')
                    *start++ = ' ';
            }
            WFIFOSET(fd, packet_len_table[0x95]);
            break;
        case BL_MOB:
        {
            struct mob_data *md = (struct mob_data *) bl;

            nullpo_retv(md);

            memcpy(WFIFOP(fd, 6), md->name, 24);
            WFIFOSET(fd, packet_len_table[0x95]);
        }
            break;
        default:
            if (battle_config.error_log)
                PRINTF("clif_parse_GetCharNameRequest : bad type %d (%d)\n",
                        bl->type, account_id);
            break;
    }
}

/*==========================================
 * Validate and process transmission of a
 * global/public message.
 *
 * (S 008c <len>.w <message>.?B)
 *------------------------------------------
 */
static
void clif_parse_GlobalMessage(int fd, struct map_session_data *sd)
{
    int msg_len = RFIFOW(fd, 2) - 4; /* Header(2) + length(2). */
    size_t message_len = 0;
    // sometimes uint8_t
    char *buf = NULL;
    const char *message = NULL;   /* The message text only. */

    nullpo_retv(sd);

    if (!(buf = clif_validate_chat(sd, 2, &message, &message_len)))
    {
        clif_displaymessage(fd, "Your message could not be sent.");
        return;
    }

    if (is_atcommand(fd, sd, message, 0)
            || (sd->sc_data[SC_NOCHAT].timer != -1)) //チャット禁止
    {
        free(buf);
        return;
    }

    if (!magic_message(sd, buf, msg_len))
    {
        /* Don't send chat that results in an automatic ban. */
        if (tmw_CheckChatSpam(sd, message))
        {
            free(buf);
            clif_displaymessage(fd, "Your message could not be sent.");
            return;
        }

        /* It's not a spell/magic message, so send the message to others. */
        WBUFW(reinterpret_cast<uint8_t *>(buf), 0) = 0x8d;
        WBUFW(reinterpret_cast<uint8_t *>(buf), 2) = msg_len + 8;   /* Header(2) + length(2) + ID(4). */
        WBUFL(reinterpret_cast<uint8_t *>(buf), 4) = sd->bl.id;

        // evil multiuse buffer!
        clif_send((const uint8_t *)buf, msg_len + 8, &sd->bl,
                   sd->chatID ? CHAT_WOS : AREA_CHAT_WOC);
    }

    /* Send the message back to the speaker. */
    memcpy(WFIFOP(fd, 0), RFIFOP(fd, 0), RFIFOW(fd, 2));
    WFIFOW(fd, 0) = 0x8e;
    WFIFOSET(fd, WFIFOW(fd, 2));

    free(buf);
    return;
}

int clif_message(struct block_list *bl, const char *msg)
{
    unsigned short msg_len = strlen(msg) + 1;
    unsigned char buf[512];

    if (msg_len + 16 > 512)
        return 0;

    nullpo_ret(bl);

    WBUFW(buf, 0) = 0x8d;
    WBUFW(buf, 2) = msg_len + 8;
    WBUFL(buf, 4) = bl->id;
    memcpy(WBUFP(buf, 8), msg, msg_len);

    clif_send(buf, WBUFW(buf, 2), bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_ChangeDir(int fd, struct map_session_data *sd)
{
    unsigned char buf[64];

    nullpo_retv(sd);

    // RFIFOW(fd, 2) is always 0
    DIR dir = static_cast<DIR>(RFIFOB(fd, 4));
    if (dir >= DIR::COUNT)
        return;

    if (dir == sd->dir)
        return;

    pc_setdir(sd, dir);

    WBUFW(buf, 0) = 0x9c;
    WBUFL(buf, 2) = sd->bl.id;
    WBUFW(buf, 6) = 0;
    WBUFB(buf, 8) = static_cast<uint8_t>(dir);

    clif_send(buf, packet_len_table[0x9c], &sd->bl, AREA_WOS);

}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_Emotion(int fd, struct map_session_data *sd)
{
    unsigned char buf[64];

    nullpo_retv(sd);

    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, NV_EMOTE) >= 1)
    {
        WBUFW(buf, 0) = 0xc0;
        WBUFL(buf, 2) = sd->bl.id;
        WBUFB(buf, 6) = RFIFOB(fd, 2);
        clif_send(buf, packet_len_table[0xc0], &sd->bl, AREA);
    }
    else
        clif_skill_fail(sd, SkillID::ONE, 0, 1);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_HowManyConnections(int fd, struct map_session_data *)
{
    WFIFOW(fd, 0) = 0xc2;
    WFIFOL(fd, 2) = map_getusers();
    WFIFOSET(fd, packet_len_table[0xc2]);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_ActionRequest(int fd, struct map_session_data *sd)
{
    unsigned int tick;
    unsigned char buf[64];
    int action_type, target_id;

    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0
        || bool(sd->opt1)
        || sd->state.storage_open)
        return;

    tick = gettick();

    pc_stop_walking(sd, 0);
    pc_stopattack(sd);

    target_id = RFIFOL(fd, 2);
    action_type = RFIFOB(fd, 6);

    switch (action_type)
    {
        case 0x00:             // once attack
        case 0x07:             // continuous attack
            if (bool(sd->status.option & Option::HIDE))
                return;
            if (!battle_config.sdelay_attack_enable)
            {
                if (DIFF_TICK(tick, sd->canact_tick) < 0)
                {
                    clif_skill_fail(sd, SkillID::ONE, 4, 0);
                    return;
                }
            }
            if (sd->invincible_timer != -1)
                pc_delinvincibletimer(sd);
            if (sd->attacktarget > 0)   // [Valaris]
                sd->attacktarget = 0;
            pc_attack(sd, target_id, action_type != 0);
            break;
        case 0x02:             // sitdown
            pc_stop_walking(sd, 1);
            skill_gangsterparadise(sd, 1); // ギャングスターパラダイス設定
            pc_setsit(sd);
            clif_sitting(fd, sd);
            break;
        case 0x03:             // standup
            skill_gangsterparadise(sd, 0); // ギャングスターパラダイス解除
            pc_setstand(sd);
            WBUFW(buf, 0) = 0x8a;
            WBUFL(buf, 2) = sd->bl.id;
            WBUFB(buf, 26) = 3;
            clif_send(buf, packet_len_table[0x8a], &sd->bl, AREA);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_Restart(int fd, struct map_session_data *sd)
{
    nullpo_retv(sd);

    switch (RFIFOB(fd, 2))
    {
        case 0x00:
            if (pc_isdead(sd))
            {
                pc_setstand(sd);
                pc_setrestartvalue(sd, 3);
                pc_setpos(sd, sd->status.save_point.map,
                           sd->status.save_point.x, sd->status.save_point.y,
                           2);
            }
            break;
        case 0x01:
            /*if (!pc_isdead(sd) && (sd->opt1 || (sd->opt2 && !(night_flag == 1 && sd->opt2 == Opt2::BLIND))))
             * return; */

            /*  Rovert's Prevent logout option - Fixed [Valaris]    */
            if ((battle_config.prevent_logout
                 && (gettick() - sd->canlog_tick) >= 10000)
                || (!battle_config.prevent_logout))
            {
                chrif_charselectreq(sd);
            }
            else
            {
                WFIFOW(fd, 0) = 0x18b;
                WFIFOW(fd, 2) = 1;

                WFIFOSET(fd, packet_len_table[0x018b]);
            }
            break;
    }
}

/*==========================================
 * Validate and process transmission of a
 * whisper/private message.
 *
 * (S 0096 <len>.w <nick>.24B <message>.?B)
 *
 * rewritten by [Yor], then partially by
 * [remoitnane]
 *------------------------------------------
 */
static
void clif_parse_Wis(int fd, struct map_session_data *sd)
{
    size_t message_len = 0;
    char *buf = NULL;
    const char *message = NULL;   /* The message text only. */
    struct map_session_data *dstsd = NULL;

    nullpo_retv(sd);

    if (!(buf = clif_validate_chat(sd, 1, &message, &message_len)))
    {
        clif_displaymessage(fd, "Your message could not be sent.");
        return;
    }

    if (is_atcommand(fd, sd, message, 0)
            || (sd->sc_data[SC_NOCHAT].timer != -1))
    {
        free(buf);
        return;
    }

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam(sd, message))
    {
        free(buf);
        clif_displaymessage(fd, "Your message could not be sent.");
        return;
    }

    /*
     * The player is not on this server. Only send the whisper if the name is
     * exactly the same, because if there are multiple map-servers and a name
     * conflict (for instance, "Test" versus "test"), the char-server must
     * settle the discrepancy.
     */
    if (!(dstsd = map_nick2sd((const char *)RFIFOP(fd, 4)))
            || strcmp(dstsd->status.name, (const char *)RFIFOP(fd, 4)) != 0)
        intif_wis_message(sd, (const char *)RFIFOP(fd, 4), message,  RFIFOW(fd, 2) - 28);
    else
    {
        /* Refuse messages addressed to self. */
        if (dstsd->fd == fd)
        {
            const char *mes = "You cannot page yourself.";
            clif_wis_message(fd, wisp_server_name, mes, strlen(mes) + 1);
        }
        else
        {
            /* The target is ignoring all whispers. */
            if (dstsd->ignoreAll == 1)
                /* Ignored by target. */
                clif_wis_end(fd, 2);
            else
            {
                int i;
                size_t end = sizeof(dstsd->ignore) / sizeof(dstsd->ignore[0]);

                /* See if the source player is being ignored. */
                for (i = 0; i < end; ++i)
                    if (strcmp(dstsd->ignore[i].name, sd->status.name) == 0)
                    {
                        /* Ignored by target. */
                        clif_wis_end(fd, 2);
                        break;
                    }

                /* The player is not being ignored. */
                if (i == end)
                {
                    clif_wis_message(dstsd->fd, sd->status.name, message,
                                      RFIFOW(fd, 2) - 28);
                    /* The whisper was sent successfully. */
                    clif_wis_end(fd, 0);
                }
            }
        }
    }

    free(buf);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_TakeItem(int fd, struct map_session_data *sd)
{
    struct flooritem_data *fitem;
    int map_object_id;

    nullpo_retv(sd);

    map_object_id = RFIFOL(fd, 2);
    fitem = (struct flooritem_data *) map_id2bl(map_object_id);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }

    if (sd->npc_id != 0
        || sd->opt1 != Opt1::ZERO
        || sd->sc_data[SC_NOCHAT].timer != -1)   //会話禁止
        return;

    if (fitem == NULL || fitem->bl.m != sd->bl.m)
        return;

    if (abs(sd->bl.x - fitem->bl.x) >= 2
        || abs(sd->bl.y - fitem->bl.y) >= 2)
        return;                 // too far away to pick up

    if (sd->state.shroud_active && sd->state.shroud_disappears_on_pickup)
        magic_unshroud(sd);

    pc_takeitem(sd, fitem);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_DropItem(int fd, struct map_session_data *sd)
{
    int item_index, item_amount;

    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }
    if (map[sd->bl.m].flag.no_player_drops)
    {
        clif_displaymessage(sd->fd, "Can't drop items here.");
        return;
    }
    if (sd->npc_id != 0
        || sd->opt1 != Opt1::ZERO)
    {
        clif_displaymessage(sd->fd, "Can't drop items right now.");
        return;
    }

    item_index = RFIFOW(fd, 2) - 2;
    item_amount = RFIFOW(fd, 4);

    pc_dropitem(sd, item_index, item_amount);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_UseItem(int fd, struct map_session_data *sd)
{
    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0
        || sd->opt1 != Opt1::ZERO
        || sd->sc_data[SC_NOCHAT].timer != -1)   //会話禁止
        return;

    if (sd->invincible_timer != -1)
        pc_delinvincibletimer(sd);

    pc_useitem(sd, RFIFOW(fd, 2) - 2);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_EquipItem(int fd, struct map_session_data *sd)
{
    int index;

    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }
    index = RFIFOW(fd, 2) - 2;
    if (sd->npc_id != 0)
        return;

    if (sd->status.inventory[index].identify != 1)
    {                           // 未鑑定
        // Bjorn: Auto-identify items when equipping them as there
        //  is no nice way to do this in the client yet.
        sd->status.inventory[index].identify = 1;
        //clif_equipitemack(sd,index,0,0);  // fail
        //return;
    }
    //ペット用装備であるかないか
    if (sd->inventory_data[index])
    {
        EPOS epos = EPOS(RFIFOW(fd, 4));
        if (sd->inventory_data[index]->type == ItemType::ARROW)
            // 矢を無理やり装備できるように（−−；
            epos = EPOS::ARROW;

        // Note: the EPOS argument to pc_equipitem is actually ignored
        pc_equipitem(sd, index, epos);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_UnequipItem(int fd, struct map_session_data *sd)
{
    int index;

    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }
    index = RFIFOW(fd, 2) - 2;
    if (sd->status.inventory[index].broken == 1 && sd->sc_data[SC_BROKNWEAPON].timer != -1)
        skill_status_change_end(&sd->bl, SC_BROKNWEAPON, -1);
    if (sd->status.inventory[index].broken == 1 && sd->sc_data[SC_BROKNARMOR].timer != -1)
        skill_status_change_end(&sd->bl, SC_BROKNARMOR, -1);

    if (sd->npc_id != 0
        || sd->opt1 != Opt1::ZERO)
        return;
    pc_unequipitem(sd, index, CalcStatus::NOW);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcClicked(int fd, struct map_session_data *sd)
{
    nullpo_retv(sd);

    if (pc_isdead(sd))
    {
        clif_clearchar_area(&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0)
        return;
    npc_click(sd, RFIFOL(fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcBuySellSelected(int fd, struct map_session_data *sd)
{
    npc_buysellsel(sd, RFIFOL(fd, 2), RFIFOB(fd, 6));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcBuyListSend(int fd, struct map_session_data *sd)
{
    int n = (RFIFOW(fd, 2) - 4) / 4;
    // really an array of pairs of uint16_t
    const uint16_t *item_list = static_cast<const uint16_t *>(RFIFOP(fd, 4));

    int fail = npc_buylist(sd, n, item_list);

    WFIFOW(fd, 0) = 0xca;
    WFIFOB(fd, 2) = fail;
    WFIFOSET(fd, packet_len_table[0xca]);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcSellListSend(int fd, struct map_session_data *sd)
{
    int n = (RFIFOW(fd, 2) - 4) / 4;
    // really an array of pairs of uint16_t
    const uint16_t *item_list = static_cast<const uint16_t *>(RFIFOP(fd, 4));

    int fail = npc_selllist(sd, n, item_list);

    WFIFOW(fd, 0) = 0xcb;
    WFIFOB(fd, 2) = fail;
    WFIFOSET(fd, packet_len_table[0xcb]);
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
static
void clif_parse_TradeRequest(int, struct map_session_data *sd)
{
    nullpo_retv(sd);

    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, NV_TRADE) >= 1)
    {
        trade_traderequest(sd, RFIFOL(sd->fd, 2));
    }
    else
        clif_skill_fail(sd, SkillID::ONE, 0, 0);
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
static
void clif_parse_TradeAck(int, struct map_session_data *sd)
{
    nullpo_retv(sd);

    trade_tradeack(sd, RFIFOB(sd->fd, 2));
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
static
void clif_parse_TradeAddItem(int, struct map_session_data *sd)
{
    nullpo_retv(sd);

    trade_tradeadditem(sd, RFIFOW(sd->fd, 2), RFIFOL(sd->fd, 4));
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
static
void clif_parse_TradeOk(int, struct map_session_data *sd)
{
    trade_tradeok(sd);
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
static
void clif_parse_TradeCansel(int, struct map_session_data *sd)
{
    trade_tradecancel(sd);
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
static
void clif_parse_TradeCommit(int, struct map_session_data *sd)
{
    trade_tradecommit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_StopAttack(int, struct map_session_data *sd)
{
    pc_stopattack(sd);
}

/*==========================================
 * ステータスアップ
 *------------------------------------------
 */
static
void clif_parse_StatusUp(int fd, struct map_session_data *sd)
{
    pc_statusup(sd, SP(RFIFOW(fd, 2)));
}

/*==========================================
 * スキルレベルアップ
 *------------------------------------------
 */
static
void clif_parse_SkillUp(int fd, struct map_session_data *sd)
{
    pc_skillup(sd, SkillID(RFIFOW(fd, 2)));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcSelectMenu(int fd, struct map_session_data *sd)
{
    nullpo_retv(sd);

    sd->npc_menu = RFIFOB(fd, 6);
    map_scriptcont(sd, RFIFOL(fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcNextClicked(int fd, struct map_session_data *sd)
{
    map_scriptcont(sd, RFIFOL(fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcAmountInput(int fd, struct map_session_data *sd)
{
    nullpo_retv(sd);

    sd->npc_amount = RFIFOL(fd, 6);
    map_scriptcont(sd, RFIFOL(fd, 2));
}

/*==========================================
 * Process string-based input for an NPC.
 *
 * (S 01d5 <len>.w <npc_ID>.l <message>.?B)
 *------------------------------------------
 */
static
void clif_parse_NpcStringInput(int fd, struct map_session_data *sd)
{
    int len;
    nullpo_retv(sd);

    len = RFIFOW(fd, 2) - 8;

    /*
     * If we check for equal to 0, too, we'll freeze clients that send (or
     * claim to have sent) an "empty" message.
     */
    if (len < 0)
        return;

    if (len >= sizeof(sd->npc_str) - 1)
    {
        PRINTF("clif_parse_NpcStringInput(): Input string too long!\n");
        len = sizeof(sd->npc_str) - 1;
    }

    if (len > 0)
        strncpy(sd->npc_str, (const char *)RFIFOP(fd, 8), len);
    sd->npc_str[len] = '\0';

    map_scriptcont(sd, RFIFOL(fd, 4));
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_parse_NpcCloseClicked(int fd, struct map_session_data *sd)
{
    map_scriptcont(sd, RFIFOL(fd, 2));
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
static
void clif_parse_MoveToKafra(int fd, struct map_session_data *sd)
{
    int item_index, item_amount;

    nullpo_retv(sd);

    item_index = RFIFOW(fd, 2) - 2;
    item_amount = RFIFOL(fd, 4);

    if ((sd->npc_id != 0 && !sd->npc_flags.storage) || sd->trade_partner != 0
        || !sd->state.storage_open)
        return;

    if (sd->state.storage_open)
        storage_storageadd(sd, item_index, item_amount);
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
static
void clif_parse_MoveFromKafra(int fd, struct map_session_data *sd)
{
    int item_index, item_amount;

    nullpo_retv(sd);

    item_index = RFIFOW(fd, 2) - 1;
    item_amount = RFIFOL(fd, 4);

    if ((sd->npc_id != 0 && !sd->npc_flags.storage) || sd->trade_partner != 0
        || !sd->state.storage_open)
        return;

    if (sd->state.storage_open)
        storage_storageget(sd, item_index, item_amount);
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
static
void clif_parse_CloseKafra(int, struct map_session_data *sd)
{
    nullpo_retv(sd);

    if (sd->state.storage_open)
        storage_storageclose(sd);
}

/*==========================================
 * パーティを作る
 * Process request to create a party.
 *
 * (S 00f9 <party_name>.24B)
 *------------------------------------------
 */
static
void clif_parse_CreateParty(int fd, struct map_session_data *sd)
{
    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, NV_PARTY) >= 2)
    {
        party_create(sd, (const char *)RFIFOP(fd, 2));
    }
    else
        clif_skill_fail(sd, SkillID::ONE, 0, 4);
}

/*==========================================
 * パーティに勧誘
 * Process invitation to join a party.
 *
 * (S 00fc <account_ID>.l)
 *------------------------------------------
 */
static
void clif_parse_PartyInvite(int fd, struct map_session_data *sd)
{
    party_invite(sd, RFIFOL(fd, 2));
}

/*==========================================
 * パーティ勧誘返答
 * Process reply to party invitation.
 *
 * (S 00ff <account_ID>.l <flag>.l)
 *------------------------------------------
 */
static
void clif_parse_ReplyPartyInvite(int fd, struct map_session_data *sd)
{
    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, NV_PARTY) >= 1)
    {
        party_reply_invite(sd, RFIFOL(fd, 2), RFIFOL(fd, 6));
    }
    else
    {
        party_reply_invite(sd, RFIFOL(fd, 2), 0);
        clif_skill_fail(sd, SkillID::ONE, 0, 4);
    }
}

/*==========================================
 * パーティ脱退要求
 *------------------------------------------
 */
static
void clif_parse_LeaveParty(int, struct map_session_data *sd)
{
    party_leave(sd);
}

/*==========================================
 * パーティ除名要求
 *------------------------------------------
 */
static
void clif_parse_RemovePartyMember(int fd, struct map_session_data *sd)
{
    party_removemember(sd, RFIFOL(fd, 2), (const char *)RFIFOP(fd, 6));
}

/*==========================================
 * パーティ設定変更要求
 *------------------------------------------
 */
static
void clif_parse_PartyChangeOption(int fd, struct map_session_data *sd)
{
    party_changeoption(sd, RFIFOW(fd, 2), RFIFOW(fd, 4));
}

/*==========================================
 * パーティメッセージ送信要求
 * Validate and process transmission of a
 * party message.
 *
 * (S 0108 <len>.w <message>.?B)
 *------------------------------------------
 */
static
void clif_parse_PartyMessage(int fd, struct map_session_data *sd)
{
    size_t message_len = 0;
    char *buf = NULL;
    const char *message = NULL;   /* The message text only. */

    nullpo_retv(sd);

    if (!(buf = clif_validate_chat(sd, 0, &message, &message_len)))
    {
        clif_displaymessage(fd, "Your message could not be sent.");
        return;
    }

    if (is_atcommand(fd, sd, message, 0)
            || (sd->sc_data[SC_NOCHAT].timer != -1)) //チャット禁止
    {
        free(buf);
        return;
    }

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam(sd, message))
    {
        free(buf);
        clif_displaymessage(fd, "Your message could not be sent.");
        return;
    }

    party_send_message(sd, message, RFIFOW(fd, 2) - 4);
    free(buf);
}

// 4144 wants this, but I don't like it ...
static
void clif_parse_PMIgnoreAll(int fd, struct map_session_data *sd)
{                               // Rewritten by [Yor]
    //PRINTF("Ignore all: state: %d\n", RFIFOB(fd,2));
    if (RFIFOB(fd, 2) == 0)
    {                           // S 00d0 <type>len.B: 00 (/exall) deny all speech, 01 (/inall) allow all speech
        WFIFOW(fd, 0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
        WFIFOB(fd, 2) = 0;
        if (sd->ignoreAll == 0)
        {
            sd->ignoreAll = 1;
            WFIFOB(fd, 3) = 0; // success
            WFIFOSET(fd, packet_len_table[0x0d2]);
        }
        else
        {
            WFIFOB(fd, 3) = 1; // fail
            WFIFOSET(fd, packet_len_table[0x0d2]);
            clif_wis_message(fd, wisp_server_name,
                              "You already block everyone.",
                              strlen("You already block everyone.") + 1);
        }
    }
    else
    {
        WFIFOW(fd, 0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
        WFIFOB(fd, 2) = 1;
        if (sd->ignoreAll == 1)
        {
            sd->ignoreAll = 0;
            WFIFOB(fd, 3) = 0; // success
            WFIFOSET(fd, packet_len_table[0x0d2]);
        }
        else
        {
            WFIFOB(fd, 3) = 1; // fail
            WFIFOSET(fd, packet_len_table[0x0d2]);
            clif_wis_message(fd, wisp_server_name,
                              "You already allow everyone.",
                              strlen("You already allow everyone.") + 1);
        }
    }

    return;
}

// functions list. Rate is how many milliseconds are required between
// calls. Packets exceeding this rate will be dropped. flood_rates in
// map.h must be the same length as this table. rate 0 is default
// rate -1 is unlimited

// TODO: merge this with the packet length table, and put the pointer last
typedef struct func_table
{
        void(*func)(int fd, struct map_session_data *sd);
        int rate;
} func_table;
// *INDENT-OFF*
static
func_table clif_parse_func_table[0x220] =
{
        { NULL,                                 0       },      // 0
        { NULL,                                 0       },      // 1
        { NULL,                                 0       },      // 2
        { NULL,                                 0       },      // 3
        { NULL,                                 0       },      // 4
        { NULL,                                 0       },      // 5
        { NULL,                                 0       },      // 6
        { NULL,                                 0       },      // 7
        { NULL,                                 0       },      // 8
        { NULL,                                 0       },      // 9
        { NULL,                                 0       },      // a
        { NULL,                                 0       },      // b
        { NULL,                                 0       },      // c
        { NULL,                                 0       },      // d
        { NULL,                                 0       },      // e
        { NULL,                                 0       },      // f
        { NULL,                                 0       },      // 10
        { NULL,                                 0       },      // 11
        { NULL,                                 0       },      // 12
        { NULL,                                 0       },      // 13
        { NULL,                                 0       },      // 14
        { NULL,                                 0       },      // 15
        { NULL,                                 0       },      // 16
        { NULL,                                 0       },      // 17
        { NULL,                                 0       },      // 18
        { NULL,                                 0       },      // 19
        { NULL,                                 0       },      // 1a
        { NULL,                                 0       },      // 1b
        { NULL,                                 0       },      // 1c
        { NULL,                                 0       },      // 1d
        { NULL,                                 0       },      // 1e
        { NULL,                                 0       },      // 1f
        { NULL,                                 0       },      // 20
        { NULL,                                 0       },      // 21
        { NULL,                                 0       },      // 22
        { NULL,                                 0       },      // 23
        { NULL,                                 0       },      // 24
        { NULL,                                 0       },      // 25
        { NULL,                                 0       },      // 26
        { NULL,                                 0       },      // 27
        { NULL,                                 0       },      // 28
        { NULL,                                 0       },      // 29
        { NULL,                                 0       },      // 2a
        { NULL,                                 0       },      // 2b
        { NULL,                                 0       },      // 2c
        { NULL,                                 0       },      // 2d
        { NULL,                                 0       },      // 2e
        { NULL,                                 0       },      // 2f
        { NULL,                                 0       },      // 30
        { NULL,                                 0       },      // 31
        { NULL,                                 0       },      // 32
        { NULL,                                 0       },      // 33
        { NULL,                                 0       },      // 34
        { NULL,                                 0       },      // 35
        { NULL,                                 0       },      // 36
        { NULL,                                 0       },      // 37
        { NULL,                                 0       },      // 38
        { NULL,                                 0       },      // 39
        { NULL,                                 0       },      // 3a
        { NULL,                                 0       },      // 3b
        { NULL,                                 0       },      // 3c
        { NULL,                                 0       },      // 3d
        { NULL,                                 0       },      // 3e
        { NULL,                                 0       },      // 3f
        { NULL,                                 0       },      // 40
        { NULL,                                 0       },      // 41
        { NULL,                                 0       },      // 42
        { NULL,                                 0       },      // 43
        { NULL,                                 0       },      // 44
        { NULL,                                 0       },      // 45
        { NULL,                                 0       },      // 46
        { NULL,                                 0       },      // 47
        { NULL,                                 0       },      // 48
        { NULL,                                 0       },      // 49
        { NULL,                                 0       },      // 4a
        { NULL,                                 0       },      // 4b
        { NULL,                                 0       },      // 4c
        { NULL,                                 0       },      // 4d
        { NULL,                                 0       },      // 4e
        { NULL,                                 0       },      // 4f
        { NULL,                                 0       },      // 50
        { NULL,                                 0       },      // 51
        { NULL,                                 0       },      // 52
        { NULL,                                 0       },      // 53
        { NULL,                                 0       },      // 54
        { NULL,                                 0       },      // 55
        { NULL,                                 0       },      // 56
        { NULL,                                 0       },      // 57
        { NULL,                                 0       },      // 58
        { NULL,                                 0       },      // 59
        { NULL,                                 0       },      // 5a
        { NULL,                                 0       },      // 5b
        { NULL,                                 0       },      // 5c
        { NULL,                                 0       },      // 5d
        { NULL,                                 0       },      // 5e
        { NULL,                                 0       },      // 5f
        { NULL,                                 0       },      // 60
        { NULL,                                 0       },      // 61
        { NULL,                                 0       },      // 62
        { NULL,                                 0       },      // 63
        { NULL,                                 0       },      // 64
        { NULL,                                 0       },      // 65
        { NULL,                                 0       },      // 66
        { NULL,                                 0       },      // 67
        { NULL,                                 0       },      // 68
        { NULL,                                 0       },      // 69
        { NULL,                                 0       },      // 6a
        { NULL,                                 0       },      // 6b
        { NULL,                                 0       },      // 6c
        { NULL,                                 0       },      // 6d
        { NULL,                                 0       },      // 6e
        { NULL,                                 0       },      // 6f
        { NULL,                                 0       },      // 70
        { NULL,                                 0       },      // 71
        { clif_parse_WantToConnection,          0       },      // 72
        { NULL,                                 0       },      // 73
        { NULL,                                 0       },      // 74
        { NULL,                                 0       },      // 75
        { NULL,                                 0       },      // 76
        { NULL,                                 0       },      // 77
        { NULL,                                 0       },      // 78
        { NULL,                                 0       },      // 79
        { NULL,                                 0       },      // 7a
        { NULL,                                 0       },      // 7b
        { NULL,                                 0       },      // 7c
        { clif_parse_LoadEndAck,                -1      },      // 7d
        { clif_parse_TickSend,                  0       },      // 7e
        { NULL,                                 0       },      // 7f
        { NULL,                                 0       },      // 80
        { NULL,                                 0       },      // 81
        { NULL,                                 0       },      // 82
        { NULL,                                 0       },      // 83
        { NULL,                                 0       },      // 84
        { clif_parse_WalkToXY,                  -1      },      // 85 Walk code limits this on it's own
        { NULL,                                 0       },      // 86
        { NULL,                                 0       },      // 87
        { NULL,                                 0       },      // 88
        { clif_parse_ActionRequest,             1000    },      // 89 Special case - see below
        { NULL,                                 0       },      // 8a
        { NULL,                                 0       },      // 8b
        { clif_parse_GlobalMessage,             300     },      // 8c
        { NULL,                                 0       },      // 8d
        { NULL,                                 0       },      // 8e
        { NULL,                                 0       },      // 8f
        { clif_parse_NpcClicked,                500     },      // 90
        { NULL,                                 0       },      // 91
        { NULL,                                 0       },      // 92
        { NULL,                                 0       },      // 93
        { clif_parse_GetCharNameRequest,        -1      },      // 94
        { NULL,                                 0       },      // 95
        { clif_parse_Wis,                       300     },      // 96
        { NULL,                                 0       },      // 97
        { NULL,                                 0       },      // 98
        { NULL,                                 300     },      // 99
        { NULL,                                 0       },      // 9a
        { clif_parse_ChangeDir,                 -1      },      // 9b
        { NULL,                                 0       },      // 9c
        { NULL,                                 0       },      // 9d
        { NULL,                                 0       },      // 9e
        { clif_parse_TakeItem,                  400     },      // 9f
        { NULL,                                 0       },      // a0
        { NULL,                                 0       },      // a1
        { clif_parse_DropItem,                  50      },      // a2
        { NULL,                                 0       },      // a3
        { NULL,                                 0       },      // a4
        { NULL,                                 0       },      // a5
        { NULL,                                 0       },      // a6
        { clif_parse_UseItem,                   0       },      // a7
        { NULL,                                 0       },      // a8
        { clif_parse_EquipItem,                 -1      },      // a9 Special case - outfit window (not implemented yet - needs to allow bursts)
        { NULL,                                 0       },      // aa
        { clif_parse_UnequipItem,               -1      },      // ab Special case - outfit window (not implemented yet - needs to allow bursts)
        { NULL,                                 0       },      // ac
        { NULL,                                 0       },      // ad
        { NULL,                                 0       },      // ae
        { NULL,                                 0       },      // af
        { NULL,                                 0       },      // b0
        { NULL,                                 0       },      // b1
        { clif_parse_Restart,                   0       },      // b2
        { NULL,                                 0       },      // b3
        { NULL,                                 0       },      // b4
        { NULL,                                 0       },      // b5
        { NULL,                                 0       },      // b6
        { NULL,                                 0       },      // b7
        { clif_parse_NpcSelectMenu,             0       },      // b8
        { clif_parse_NpcNextClicked,            -1      },      // b9
        { NULL,                                 0       },      // ba
        { clif_parse_StatusUp,                  -1      },      // bb People click this very quickly
        { NULL,                                 0       },      // bc
        { NULL,                                 0       },      // bd
        { NULL,                                 0       },      // be
        { clif_parse_Emotion,                   1000    },      // bf
        { NULL,                                 0       },      // c0
        { clif_parse_HowManyConnections,        0       },      // c1
        { NULL,                                 0       },      // c2
        { NULL,                                 0       },      // c3
        { NULL,                                 0       },      // c4
        { clif_parse_NpcBuySellSelected,        0       },      // c5
        { NULL,                                 0       },      // c6
        { NULL,                                 0       },      // c7
        { clif_parse_NpcBuyListSend,            -1      },      // c8
        { clif_parse_NpcSellListSend,           -1      },      // c9 Selling multiple 1-slot items
        { NULL,                                 0       },      // ca
        { NULL,                                 0       },      // cb
        { NULL,                                 0       },      // cc
        { NULL,                                 0       },      // cd
        { NULL,                                 0       },      // ce
        { NULL,                                 0       },      // cf
        { clif_parse_PMIgnoreAll,               0       },      // d0
        { NULL,                                 0       },      // d1
        { NULL,                                 0       },      // d2
        { NULL,                                 0       },      // d3
        { NULL,                                 0       },      // d4
        { NULL,                                 0       },      // d5
        { NULL,                                 0       },      // d6
        { NULL,                                 0       },      // d7
        { NULL,                                 0       },      // d8
        { NULL,                                 0       },      // d9
        { NULL,                                 0       },      // da
        { NULL,                                 0       },      // db
        { NULL,                                 0       },      // dc
        { NULL,                                 0       },      // dd
        { NULL,                                 0       },      // de
        { NULL,                                 0       },      // df
        { NULL,                                 0       },      // e0
        { NULL,                                 0       },      // e1
        { NULL,                                 0       },      // e2
        { NULL,                                 0       },      // e3
        { clif_parse_TradeRequest,              2000    },      // e4
        { NULL,                                 0       },      // e5
        { clif_parse_TradeAck,                  0       },      // e6
        { NULL,                                 0       },      // e7
        { clif_parse_TradeAddItem,              0       },      // e8
        { NULL,                                 0       },      // e9
        { NULL,                                 0       },      // ea
        { clif_parse_TradeOk,                   0       },      // eb
        { NULL,                                 0       },      // ec
        { clif_parse_TradeCansel,               0       },      // ed
        { NULL,                                 0       },      // ee
        { clif_parse_TradeCommit,               0       },      // ef
        { NULL,                                 0       },      // f0
        { NULL,                                 0       },      // f1
        { NULL,                                 0       },      // f2
        { clif_parse_MoveToKafra,               -1      },      // f3
        { NULL,                                 0       },      // f4
        { clif_parse_MoveFromKafra,             -1      },      // f5
        { NULL,                                 0       },      // f6
        { clif_parse_CloseKafra,                0       },      // f7
        { NULL,                                 0       },      // f8
        { clif_parse_CreateParty,               2000    },      // f9
        { NULL,                                 0       },      // fa
        { NULL,                                 0       },      // fb
        { clif_parse_PartyInvite,               2000    },      // fc
        { NULL,                                 0       },      // fd
        { NULL,                                 0       },      // fe
        { clif_parse_ReplyPartyInvite,          0       },      // ff
        { clif_parse_LeaveParty,                0       },      // 100
        { NULL,                                 0       },      // 101
        { clif_parse_PartyChangeOption,         0       },      // 102
        { clif_parse_RemovePartyMember,         0       },      // 103
        { NULL,                                 0       },      // 104
        { NULL,                                 0       },      // 105
        { NULL,                                 0       },      // 106
        { NULL,                                 0       },      // 107
        { clif_parse_PartyMessage,              300     },      // 108
        { NULL,                                 0       },      // 109
        { NULL,                                 0       },      // 10a
        { NULL,                                 0       },      // 10b
        { NULL,                                 0       },      // 10c
        { NULL,                                 0       },      // 10d
        { NULL,                                 0       },      // 10e
        { NULL,                                 0       },      // 10f
        { NULL,                                 0       },      // 110
        { NULL,                                 0       },      // 111
        { clif_parse_SkillUp,                   -1      },      // 112
        { NULL,                                 0       },      // 113
        { NULL,                                 0       },      // 114
        { NULL,                                 0       },      // 115
        { NULL,                                 0       },      // 116
        { NULL,                                 0       },      // 117
        { clif_parse_StopAttack,                0       },      // 118
        { NULL,                                 0       },      // 119
        { NULL,                                 0       },      // 11a
        { NULL,                                 0       },      // 11b
        { NULL,                                 0       },      // 11c
        { NULL,                                 0       },      // 11d
        { NULL,                                 0       },      // 11e
        { NULL,                                 0       },      // 11f
        { NULL,                                 0       },      // 120
        { NULL,                                 0       },      // 121
        { NULL,                                 0       },      // 122
        { NULL,                                 0       },      // 123
        { NULL,                                 0       },      // 124
        { NULL,                                 0       },      // 125
        { NULL,                                 0       },      // 126
        { NULL,                                 0       },      // 127
        { NULL,                                 0       },      // 128
        { NULL,                                 0       },      // 129
        { NULL,                                 0       },      // 12a
        { NULL,                                 0       },      // 12b
        { NULL,                                 0       },      // 12c
        { NULL,                                 0       },      // 12d
        { NULL,                                 0       },      // 12e
        { NULL,                                 0       },      // 12f
        { NULL,                                 0       },      // 130
        { NULL,                                 0       },      // 131
        { NULL,                                 0       },      // 132
        { NULL,                                 0       },      // 133
        { NULL,                                 0       },      // 134
        { NULL,                                 0       },      // 135
        { NULL,                                 0       },      // 136
        { NULL,                                 0       },      // 137
        { NULL,                                 0       },      // 138
        { NULL,                                 0       },      // 139
        { NULL,                                 0       },      // 13a
        { NULL,                                 0       },      // 13b
        { NULL,                                 0       },      // 13c
        { NULL,                                 0       },      // 13d
        { NULL,                                 0       },      // 13e
        { NULL,                                 0       },      // 13f
        { NULL,                                 0       },      // 140
        { NULL,                                 0       },      // 141
        { NULL,                                 0       },      // 142
        { clif_parse_NpcAmountInput,            300     },      // 143
        { NULL,                                 0       },      // 144
        { NULL,                                 0       },      // 145
        { clif_parse_NpcCloseClicked,           300     },      // 146
        { NULL,                                 0       },      // 147
        { NULL,                                 0       },      // 148
        { NULL,                                 0       },      // 149
        { NULL,                                 0       },      // 14a
        { NULL,                                 0       },      // 14b
        { NULL,                                 0       },      // 14c
        { NULL,                                 0       },      // 14d
        { NULL,                                 0       },      // 14e
        { NULL,                                 0       },      // 14f
        { NULL,                                 0       },      // 150
        { NULL,                                 0       },      // 151
        { NULL,                                 0       },      // 152
        { NULL,                                 0       },      // 153
        { NULL,                                 0       },      // 154
        { NULL,                                 0       },      // 155
        { NULL,                                 0       },      // 156
        { NULL,                                 0       },      // 157
        { NULL,                                 0       },      // 158
        { NULL,                                 0       },      // 159
        { NULL,                                 0       },      // 15a
        { NULL,                                 0       },      // 15b
        { NULL,                                 0       },      // 15c
        { NULL,                                 0       },      // 15d
        { NULL,                                 0       },      // 15e
        { NULL,                                 0       },      // 15f
        { NULL,                                 0       },      // 160
        { NULL,                                 0       },      // 161
        { NULL,                                 0       },      // 162
        { NULL,                                 0       },      // 163
        { NULL,                                 0       },      // 164
        { NULL,                                 0       },      // 165
        { NULL,                                 0       },      // 166
        { NULL,                                 0       },      // 167
        { NULL,                                 0       },      // 168
        { NULL,                                 0       },      // 169
        { NULL,                                 0       },      // 16a
        { NULL,                                 0       },      // 16b
        { NULL,                                 0       },      // 16c
        { NULL,                                 0       },      // 16d
        { NULL,                                 0       },      // 16e
        { NULL,                                 0       },      // 16f
        { NULL,                                 0       },      // 170
        { NULL,                                 0       },      // 171
        { NULL,                                 0       },      // 172
        { NULL,                                 0       },      // 173
        { NULL,                                 0       },      // 174
        { NULL,                                 0       },      // 175
        { NULL,                                 0       },      // 176
        { NULL,                                 0       },      // 177
        { NULL,                                 0       },      // 178
        { NULL,                                 0       },      // 179
        { NULL,                                 0       },      // 17a
        { NULL,                                 0       },      // 17b
        { NULL,                                 0       },      // 17c
        { NULL,                                 0       },      // 17d
        { NULL,                                 0       },      // 17e
        { NULL,                                 0       },      // 17f
        { NULL,                                 0       },      // 180
        { NULL,                                 0       },      // 181
        { NULL,                                 0       },      // 182
        { NULL,                                 0       },      // 183
        { NULL,                                 0       },      // 184
        { NULL,                                 0       },      // 185
        { NULL,                                 0       },      // 186
        { NULL,                                 0       },      // 187
        { NULL,                                 0       },      // 188
        { NULL,                                 0       },      // 189
        { clif_parse_QuitGame,                  0       },      // 18a
        { NULL,                                 0       },      // 18b
        { NULL,                                 0       },      // 18c
        { NULL,                                 0       },      // 18d
        { NULL,                                 0       },      // 18e
        { NULL,                                 0       },      // 18f
        { NULL,                                 0       },      // 190
        { NULL,                                 0       },      // 191
        { NULL,                                 0       },      // 192
        { NULL,                                 0       },      // 193
        { NULL,                                 0       },      // 194
        { NULL,                                 0       },      // 195
        { NULL,                                 0       },      // 196
        { NULL,                                 0       },      // 197
        { NULL,                                 0       },      // 198
        { NULL,                                 0       },      // 199
        { NULL,                                 0       },      // 19a
        { NULL,                                 0       },      // 19b
        { NULL,                                 0       },      // 19c
        { NULL,                                 300     },      // 19d
        { NULL,                                 0       },      // 19e
        { NULL,                                 0       },      // 19f
        { NULL,                                 0       },      // 1a0
        { NULL,                                 0       },      // 1a1
        { NULL,                                 0       },      // 1a2
        { NULL,                                 0       },      // 1a3
        { NULL,                                 0       },      // 1a4
        { NULL,                                 0       },      // 1a5
        { NULL,                                 0       },      // 1a6
        { NULL,                                 0       },      // 1a7
        { NULL,                                 0       },      // 1a8
        { NULL,                                 0       },      // 1a9
        { NULL,                                 0       },      // 1aa
        { NULL,                                 0       },      // 1ab
        { NULL,                                 0       },      // 1ac
        { NULL,                                 0       },      // 1ad
        { NULL,                                 0       },      // 1ae
        { NULL,                                 0       },      // 1af
        { NULL,                                 0       },      // 1b0
        { NULL,                                 0       },      // 1b1
        { NULL,                                 0       },      // 1b2
        { NULL,                                 0       },      // 1b3
        { NULL,                                 0       },      // 1b4
        { NULL,                                 0       },      // 1b5
        { NULL,                                 0       },      // 1b6
        { NULL,                                 0       },      // 1b7
        { NULL,                                 0       },      // 1b8
        { NULL,                                 0       },      // 1b9
        { NULL,                                 0       },      // 1ba
        { NULL,                                 0       },      // 1bb
        { NULL,                                 0       },      // 1bc
        { NULL,                                 0       },      // 1bd
        { NULL,                                 0       },      // 1be
        { NULL,                                 0       },      // 1bf
        { NULL,                                 0       },      // 1c0
        { NULL,                                 0       },      // 1c1
        { NULL,                                 0       },      // 1c2
        { NULL,                                 0       },      // 1c3
        { NULL,                                 0       },      // 1c4
        { NULL,                                 0       },      // 1c5
        { NULL,                                 0       },      // 1c6
        { NULL,                                 0       },      // 1c7
        { NULL,                                 0       },      // 1c8
        { NULL,                                 0       },      // 1c9
        { NULL,                                 0       },      // 1ca
        { NULL,                                 0       },      // 1cb
        { NULL,                                 0       },      // 1cc
        { NULL,                                 0       },      // 1cd
        { NULL,                                 0       },      // 1ce
        { NULL,                                 0       },      // 1cf
        { NULL,                                 0       },      // 1d0
        { NULL,                                 0       },      // 1d1
        { NULL,                                 0       },      // 1d2
        { NULL,                                 0       },      // 1d3
        { NULL,                                 0       },      // 1d4
        { clif_parse_NpcStringInput,            300     },      // 1d5
        { NULL,                                 0       },      // 1d6
        { NULL,                                 0       },      // 1d7
        { NULL,                                 0       },      // 1d8
        { NULL,                                 0       },      // 1d9
        { NULL,                                 0       },      // 1da
        { NULL,                                 0       },      // 1db
        { NULL,                                 0       },      // 1dc
        { NULL,                                 0       },      // 1dd
        { NULL,                                 0       },      // 1de
        { NULL,                                 0       },      // 1df
        { NULL,                                 0       },      // 1e0
        { NULL,                                 0       },      // 1e1
        { NULL,                                 0       },      // 1e2
        { NULL,                                 0       },      // 1e3
        { NULL,                                 0       },      // 1e4
        { NULL,                                 0       },      // 1e5
        { NULL,                                 0       },      // 1e6
        { NULL,                                 0       },      // 1e7
        { NULL,                                 0       },      // 1e8
        { NULL,                                 0       },      // 1e9
        { NULL,                                 0       },      // 1ea
        { NULL,                                 0       },      // 1eb
        { NULL,                                 0       },      // 1ec
        { NULL,                                 0       },      // 1ed
        { NULL,                                 0       },      // 1ee
        { NULL,                                 0       },      // 1ef
        { NULL,                                 0       },      // 1f0
        { NULL,                                 0       },      // 1f1
        { NULL,                                 0       },      // 1f2
        { NULL,                                 0       },      // 1f3
        { NULL,                                 0       },      // 1f4
        { NULL,                                 0       },      // 1f5
        { NULL,                                 0       },      // 1f6
        { NULL,                                 0       },      // 1f7
        { NULL,                                 0       },      // 1f8
        { NULL,                                 0       },      // 1f9
        { NULL,                                 0       },      // 1fa
        { NULL,                                 0       },      // 1fb
        { NULL,                                 0       },      // 1fc
        { NULL,                                 0       },      // 1fd
        { NULL,                                 0       },      // 1fe
        { NULL,                                 0       },      // 1ff
        { NULL,                                 0       },      // 200
        { NULL,                                 0       },      // 201
        { NULL,                                 0       },      // 202
        { NULL,                                 0       },      // 203
        { NULL,                                 0       },      // 204
        { NULL,                                 0       },      // 205
        { NULL,                                 0       },      // 206
        { NULL,                                 0       },      // 207
        { NULL,                                 0       },      // 208
        { NULL,                                 0       },      // 209
        { NULL,                                 0       },      // 20a
        { NULL,                                 0       },      // 20b
        { NULL,                                 0       },      // 20c
        { NULL,                                 0       },      // 20d
        { NULL,                                 0       },      // 20e
        { NULL,                                 0       },      // 20f
        { NULL,                                 0       },      // 210
        { NULL,                                 0       },      // 211
        { NULL,                                 0       },      // 212
        { NULL,                                 0       },      // 213
        { NULL,                                 0       },      // 214
        { NULL,                                 0       },      // 215
        { NULL,                                 0       },      // 216
        { NULL,                                 0       },      // 217
        { NULL,                                 0       },      // 218
        { NULL,                                 0       },      // 219
        { NULL,                                 0       },      // 21a
        { NULL,                                 0       },      // 21b
        { NULL,                                 0       },      // 21c
        { NULL,                                 0       },      // 21d
        { NULL,                                 0       },      // 21e
        { NULL,                                 0       },      // 21f
};
// *INDENT-ON*

// Checks for packet flooding
static
int clif_check_packet_flood(int fd, int cmd)
{
    struct map_session_data *sd = (struct map_session_data *)session[fd]->session_data;
    unsigned int rate, tick = gettick();

    // sd will not be set if the client hasn't requested
    // WantToConnection yet. Do not apply flood logic to GMs
    // as approved bots (GMlvl1) should not have to work around
    // flood logic.
    if (!sd || pc_isGM(sd) || clif_parse_func_table[cmd].rate == -1)
        return 0;

    // Timer has wrapped
    if (tick < sd->flood_rates[cmd])
    {
        sd->flood_rates[cmd] = tick;
        return 0;
    }

    // Default rate is 100ms
    if ((rate = clif_parse_func_table[cmd].rate) == 0)
        rate = 100;

    // ActionRequest - attacks are allowed a faster rate than sit/stand
    if (cmd == 0x89)
    {
        int action_type = RFIFOB(fd, 6);
        if (action_type == 0x00 || action_type == 0x07)
            rate = 20;
        else
            rate = 1000;
    }

// Restore this code when mana1.0 is released
#if 0
    // ChangeDir - only apply limit if not walking
    if (cmd == 0x9b)
    {
        // .29 clients spam this packet when walking into a blocked tile
        if (RFIFOB(fd, 4) == sd->dir || sd->walktimer != -1)
            return 0;

        rate = 500;
    }
#endif

    // They are flooding
    if (tick < sd->flood_rates[cmd] + rate)
    {
        time_t now = time(NULL);

        // If it's a nasty flood we log and possibly kick
        if (now > sd->packet_flood_reset_due)
        {
            sd->packet_flood_reset_due = now + battle_config.packet_spam_threshold;
            sd->packet_flood_in = 0;
        }

        sd->packet_flood_in++;

        if (sd->packet_flood_in >= battle_config.packet_spam_flood)
        {
            PRINTF("packet flood detected from %s [0x%x]\n", sd->status.name, cmd);
            if (battle_config.packet_spam_kick)
            {
                session[fd]->eof = 1; // Kick
                return 1;
            }
            sd->packet_flood_in = 0;
        }

        return 1;
    }

    sd->flood_rates[cmd] = tick;
    return 0;
}

#define WARN_MALFORMED_MSG(sd, msg)                             \
    PRINTF("clif_validate_chat(): %s (ID %d) sent a malformed" \
            " message: %s.\n", sd->status.name, sd->status.account_id, msg)
/**
 * Validate message integrity (inspired by upstream source (eAthena)).
 *
 * @param sd active session data
 * @param type message type:
 *  0 for when the sender's name is not included (party chat)
 *  1 for when the target's name is included (whisper chat)
 *  2 for when the sender's name is given ("sender : text", public/guild chat)
 * @param[out] message the message text (pointing within return value, or NULL)
 * @param[out] message_len the length of the actual text, excluding NUL
 * @return a dynamically allocated copy of the message, or NULL upon failure
 */
static
char *clif_validate_chat(struct map_session_data *sd, int type,
        const char **message, size_t *message_len)
{
    int fd;
    unsigned int buf_len;       /* Actual message length. */
    unsigned int msg_len;       /* Reported message length. */
    unsigned int min_len;       /* Minimum message length. */
    size_t name_len;            /* Sender's name length. */
    char *buf = NULL;           /* Copy of actual message data. */

    *message = NULL;
    *message_len = 0;

    nullpo_retr(NULL, sd);
    /*
     * Don't send chat in the period between the ban and the connection's
     * closure.
     */
    if (type < 0 || type > 2 || sd->auto_ban_info.in_progress)
        return NULL;

    fd = sd->fd;
    msg_len = RFIFOW(fd, 2) - 4;
    name_len = strlen(sd->status.name);
    /*
     * At least one character is required in all instances.
     * Notes for length checks:
     *
     * For all types, header (2) + length (2) is considered empty.
     * For type 1, the message must be longer than the maximum name length (24)
     *      to be valid.
     * For type 2, the message must be longer than the sender's name length
     *      plus the length of the separator (" : ").
     */
    min_len = (type == 1) ? 24 : (type == 2) ? name_len + 3 : 0;

    /* The player just sent the header (2) and length (2) words. */
    if (!msg_len)
    {
        WARN_MALFORMED_MSG(sd, "no message sent");
        return NULL;
    }

    /* The client sent (or claims to have sent) an empty message. */
    if (msg_len == min_len)
    {
        WARN_MALFORMED_MSG(sd, "empty message");
        return NULL;
    }

    /* The protocol specifies that the target must be 24 bytes long. */
    if (type == 1 && msg_len < min_len)
    {
        /* Disallow malformed messages. */
        clif_setwaitclose(fd);
        WARN_MALFORMED_MSG(sd, "illegal target name");
        return NULL;
    }

    const char *p = static_cast<const char *>((type != 1) ? RFIFOP(fd, 4) : RFIFOP(fd, 28));
    buf_len = (type == 1) ? msg_len - min_len: msg_len;

    /*
     * The client attempted to exceed the maximum message length.
     *
     * The conf suggests up to chat_maxline characters, after which the message
     * is truncated. But the previous behavior was to drop the message, so
     * we'll do that, too.
     */
    if (buf_len >= battle_config.chat_maxline)
    {
        WARN_MALFORMED_MSG(sd, "exceeded maximum message length");
        return NULL;
    }

    /* We're leaving an extra eight bytes for public/global chat, 1 for NUL. */
    buf_len += (type == 2) ? 8 + 1 : 1;

    buf = (char *) malloc(buf_len);
    memcpy((type != 2) ? buf : buf + 8, p,
            (type != 2) ? buf_len - 1 : buf_len - 8 - 1);
    buf[buf_len - 1] = '\0';
    p = (type != 2) ? buf : buf + 8;

    if (type != 2)
    {
        *message = buf;
        /* Don't count the NUL. */
        *message_len = buf_len - 1;
    }
    else
    {
        const char *pos = NULL;
        if (!(pos = strstr(p, " : "))
                || strncmp(p, sd->status.name, name_len)
                || pos - p != name_len)
        {
            free(buf);
            /* Disallow malformed/spoofed messages. */
            clif_setwaitclose(fd);
            WARN_MALFORMED_MSG(sd, "spoofed name/invalid format");
            return NULL;
        }
        /* Step beyond the separator. */
        *message = pos + 3;
        /* Don't count the sender's name, the extra eight bytes, or the NUL. */
        *message_len = buf_len - min_len - 8 - 1;
    }

    return buf;
}

/*==========================================
 * クライアントからのパケット解析
 * socket.cのdo_parsepacketから呼び出される
 *------------------------------------------
 */
static
void clif_parse(int fd)
{
    int packet_len = 0, cmd = 0;
    struct map_session_data *sd = (struct map_session_data *)session[fd]->session_data;

    if (!sd || (sd && !sd->state.auth))
    {
        if (RFIFOREST(fd) < 2)
        {                       // too small a packet disconnect
            session[fd]->eof = 1;
        }
        if (RFIFOW(fd, 0) != 0x72)
        {                       // first packet not auth, disconnect
            session[fd]->eof = 1;
        }
    }

    // 接続が切れてるので後始末
    if (!chrif_isconnect() || session[fd]->eof)
    {                           // char鯖に繋がってない間は接続禁止 (!chrif_isconnect())
        if (sd && sd->state.auth)
        {
            pc_logout(sd);
            clif_quitsave(fd, sd);
            if (sd->status.name != NULL)
                PRINTF("Player [%s] has logged off your server.\n", sd->status.name);  // Player logout display [Valaris]
            else
                PRINTF("Player with account [%d] has logged off your server.\n", sd->bl.id);   // Player logout display [Yor]
        }
        else if (sd)
        {                       // not authentified! (refused by char-server or disconnect before to be authentified)
            PRINTF("Player with account [%d] has logged off your server (not auth account).\n", sd->bl.id);    // Player logout display [Yor]
            map_deliddb(&sd->bl);  // account_id has been included in the DB before auth answer
        }
        if (fd)
            delete_session(fd);
        return;
    }

    if (RFIFOREST(fd) < 2)
        return;               // Too small (no packet number)

    cmd = RFIFOW(fd, 0);

    // 管理用パケット処理
    if (cmd >= 30000)
    {
        switch (cmd)
        {
            case 0x7530:       // Athena情報所得
                WFIFOW(fd, 0) = 0x7531;
                WFIFOB(fd, 2) = ATHENA_MAJOR_VERSION;
                WFIFOB(fd, 3) = ATHENA_MINOR_VERSION;
                WFIFOB(fd, 4) = ATHENA_REVISION;
                WFIFOB(fd, 5) = ATHENA_RELEASE_FLAG;
                WFIFOB(fd, 6) = ATHENA_OFFICIAL_FLAG;
                WFIFOB(fd, 7) = ATHENA_SERVER_MAP;
                WFIFOW(fd, 8) = ATHENA_MOD_VERSION;
                WFIFOSET(fd, 10);
                RFIFOSKIP(fd, 2);
                break;
            case 0x7532:       // 接続の切断
                session[fd]->eof = 1;
                break;
        }
        return;
    }
    else if (cmd >= 0x200)
        return;

    // パケット長を計算
    packet_len = packet_len_table[cmd];
    if (packet_len == -1)
    {
        if (RFIFOREST(fd) < 4)
        {
            return;           // Runt packet (variable length without a length sent)
        }
        packet_len = RFIFOW(fd, 2);
        if (packet_len < 4 || packet_len > 32768)
        {
            session[fd]->eof = 1;
            return;           // Runt packet (variable out of bounds)
        }
    }

    if (RFIFOREST(fd) < packet_len)
    {
        return;               // Runt packet (sent legnth is too small)
    }

    if (sd && sd->state.auth == 1 && sd->state.waitingdisconnect == 1)
    {                           // 切断待ちの場合パケットを処理しない

    }
    else if (clif_parse_func_table[cmd].func)
    {
        if (clif_check_packet_flood(fd, cmd))
        {
            // Flood triggered. Skip packet.
            RFIFOSKIP(sd->fd, packet_len);
            return;
        }

        clif_parse_func_table[cmd].func(fd, sd);
    }
    else
    {
        // 不明なパケット
        if (battle_config.error_log)
        {
            if (fd)
                PRINTF("\nclif_parse: session #%d, packet 0x%x, lenght %d\n",
                        fd, cmd, packet_len);
#ifdef DUMP_UNKNOWN_PACKET
            {
                int i;
                FILE *fp;
                char packet_txt[256] = "save/packet.txt";
                time_t now;
                PRINTF("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
                for (i = 0; i < packet_len; i++)
                {
                    if ((i & 15) == 0)
                        PRINTF("\n%04X ", i);
                    PRINTF("%02X ", RFIFOB(fd, i));
                }
                if (sd && sd->state.auth)
                {
                    if (sd->status.name != NULL)
                        PRINTF("\nAccount ID %d, character ID %d, player name %s.\n",
                             sd->status.account_id, sd->status.char_id,
                             sd->status.name);
                    else
                        PRINTF("\nAccount ID %d.\n", sd->bl.id);
                }
                else if (sd)    // not authentified! (refused by char-server or disconnect before to be authentified)
                    PRINTF("\nAccount ID %d.\n", sd->bl.id);

                if ((fp = fopen_(packet_txt, "a")) == NULL)
                {
                    PRINTF("clif.c: cant write [%s] !!! data is lost !!!\n",
                            packet_txt);
                    return;
                }
                else
                {
                    time(&now);
                    if (sd && sd->state.auth)
                    {
                        if (sd->status.name != NULL)
                            FPRINTF(fp,
                                     "%sPlayer with account ID %d (character ID %d, player name %s) sent wrong packet:\n",
                                     asctime(gmtime(&now)),
                                     sd->status.account_id,
                                     sd->status.char_id, sd->status.name);
                        else
                            FPRINTF(fp,
                                     "%sPlayer with account ID %d sent wrong packet:\n",
                                     asctime(gmtime(&now)), sd->bl.id);
                    }
                    else if (sd)    // not authentified! (refused by char-server or disconnect before to be authentified)
                        FPRINTF(fp,
                                 "%sPlayer with account ID %d sent wrong packet:\n",
                                 asctime(gmtime(&now)), sd->bl.id);

                    FPRINTF(fp,
                             "\t---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
                    for (i = 0; i < packet_len; i++)
                    {
                        if ((i & 15) == 0)
                            FPRINTF(fp, "\n\t%04X ", i);
                        FPRINTF(fp, "%02X ", RFIFOB(fd, i));
                    }
                    FPRINTF(fp, "\n\n");
                    fclose_(fp);
                }
            }
#endif
        }
    }
    RFIFOSKIP(fd, packet_len);
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_clif (void)
{
    set_defaultparse(clif_parse);
    make_listen_port(map_port);

    return 0;
}
