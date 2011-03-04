// $Id: clif.c 164 2004-10-01 16:46:58Z $

#define DUMP_UNKNOWN_PACKET	1

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef LCCWIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <time.h>

#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "version.h"
#include "nullpo.h"
#include "md5calc.h"

#include "atcommand.h"
#include "battle.h"
#include "chat.h"
#include "chrif.h"
#include "clif.h"
#include "guild.h"
#include "intif.h"
#include "itemdb.h"
#include "magic.h"
#include "map.h"
#include "mob.h"
#include "npc.h"
#include "party.h"
#include "pc.h"
#include "script.h"
#include "skill.h"
#include "storage.h"
#include "tmw.h"
#include "trade.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define STATE_BLIND 0x10
#define EMOTE_IGNORED 0x0e

static const int packet_len_table[0x220] = {
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
enum
{
    ALL_CLIENT,
    ALL_SAMEMAP,
    AREA,
    AREA_WOS,
    AREA_WOC,
    AREA_WOSC,
    AREA_CHAT_WOC,
    CHAT,
    CHAT_WOS,
    PARTY,
    PARTY_WOS,
    PARTY_SAMEMAP,
    PARTY_SAMEMAP_WOS,
    PARTY_AREA,
    PARTY_AREA_WOS,
    GUILD,
    GUILD_WOS,
    GUILD_SAMEMAP,              // [Valaris]
    GUILD_SAMEMAP_WOS,
    GUILD_AREA,
    GUILD_AREA_WOS,             // end additions [Valaris]
    SELF
};

#define WBUFPOS(p,pos,x,y) { unsigned char *__p = (p); __p+=(pos); __p[0] = (x)>>2; __p[1] = ((x)<<6) | (((y)>>4)&0x3f); __p[2] = (y)<<4; }
#define WBUFPOS2(p,pos,x0,y0,x1,y1) { unsigned char *__p = (p); __p+=(pos); __p[0] = (x0)>>2; __p[1] = ((x0)<<6) | (((y0)>>4)&0x3f); __p[2] = ((y0)<<4) | (((x1)>>6)&0x0f); __p[3]=((x1)<<2) | (((y1)>>8)&0x03); __p[4]=(y1); }

#define WFIFOPOS(fd,pos,x,y) { WBUFPOS (WFIFOP(fd,pos),0,x,y); }
#define WFIFOPOS2(fd,pos,x0,y0,x1,y1) { WBUFPOS2(WFIFOP(fd,pos),0,x0,y0,x1,y1); }

static char map_ip_str[16];
static in_addr_t map_ip;
static int map_port = 5121;
int  map_fd;
char talkie_mes[80];

/*==========================================
 * map鯖のip設定
 *------------------------------------------
 */
void clif_setip (char *ip)
{
    memcpy (map_ip_str, ip, 16);
    map_ip = inet_addr (map_ip_str);
}

/*==========================================
 * map鯖のport設定
 *------------------------------------------
 */
void clif_setport (int port)
{
    map_port = port;
}

/*==========================================
 * map鯖のip読み出し
 *------------------------------------------
 */
in_addr_t clif_getip (void)
{
    return map_ip;
}

/*==========================================
 * map鯖のport読み出し
 *------------------------------------------
 */
int clif_getport (void)
{
    return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers (void)
{
    int  users = 0, i;
    struct map_session_data *sd;

    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = session[i]->session_data) && sd
            && sd->state.auth && !(battle_config.hide_GM_session
                                   && pc_isGM (sd)))
            users++;
    }
    return users;
}

/*==========================================
 * 全てのclientに対してfunc()実行
 *------------------------------------------
 */
int clif_foreachclient (int (*func) (struct map_session_data *, va_list), ...)
{
    int  i;
    va_list ap;
    struct map_session_data *sd;

    va_start (ap, func);
    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = session[i]->session_data) && sd
            && sd->state.auth)
            func (sd, ap);
    }
    va_end (ap);
    return 0;
}

static int is_deaf (struct block_list *bl)
{
    struct map_session_data *sd = (struct map_session_data *) bl;
    if (!bl || bl->type != BL_PC)
        return 0;
    return sd->special_state.deaf;
}

static void clif_emotion_towards (struct block_list *bl,
                                  struct block_list *target, int type);

static char *clif_validate_chat (struct map_session_data *sd, int type,
                                 char **message, size_t *message_len);

/*==========================================
 * clif_sendでAREA*指定時用
 *------------------------------------------
 */
int clif_send_sub (struct block_list *bl, va_list ap)
{
    unsigned char *buf;
    int  len;
    struct block_list *src_bl;
    int  type;
    struct map_session_data *sd;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);
    nullpo_retr (0, sd = (struct map_session_data *) bl);

    buf = va_arg (ap, unsigned char *);
    len = va_arg (ap, int);
    nullpo_retr (0, src_bl = va_arg (ap, struct block_list *));
    type = va_arg (ap, int);

    switch (type)
    {
        case AREA_WOS:
            if (bl && bl == src_bl)
                return 0;
            break;

        case AREA_CHAT_WOC:
            if (is_deaf (bl)
                && !(bl->type == BL_PC
                     && pc_isGM ((struct map_session_data *) src_bl)))
            {
                clif_emotion_towards (src_bl, bl, EMOTE_IGNORED);
                return 0;
            }
            /* fall through... */
        case AREA_WOC:
            if ((sd && sd->chatID) || (bl && bl == src_bl))
                return 0;

            break;
        case AREA_WOSC:
            if ((sd) && sd->chatID
                && sd->chatID == ((struct map_session_data *) src_bl)->chatID)
                return 0;
            break;
    }

    if (session[sd->fd] != NULL)
    {
        if (WFIFOP (sd->fd, 0) == buf)
        {
            printf ("WARNING: Invalid use of clif_send function\n");
            printf
                ("         Packet x%4x use a WFIFO of a player instead of to use a buffer.\n",
                 WBUFW (buf, 0));
            printf ("         Please correct your code.\n");
            // don't send to not move the pointer of the packet for next sessions in the loop
        }
        else
        {
            if (packet_len_table[RBUFW (buf, 0)])
            {                   // packet must exist
                memcpy (WFIFOP (sd->fd, 0), buf, len);
                WFIFOSET (sd->fd, len);
            }
        }
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send (unsigned char *buf, int len, struct block_list *bl, int type)
{
    int  i;
    struct map_session_data *sd;
    struct chat_data *cd;
    struct party *p = NULL;
    struct guild *g = NULL;
    int  x0 = 0, x1 = 0, y0 = 0, y1 = 0;

    if (type != ALL_CLIENT)
    {
        nullpo_retr (0, bl);

        if (bl->type == BL_PC)
        {
            struct map_session_data *sd = (struct map_session_data *) bl;
            if (sd->status.option & OPTION_INVISIBILITY)
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
                if (session[i] && (sd = session[i]->session_data) != NULL
                    && sd->state.auth)
                {
                    if (packet_len_table[RBUFW (buf, 0)])
                    {           // packet must exist
                        memcpy (WFIFOP (i, 0), buf, len);
                        WFIFOSET (i, len);
                    }
                }
            }
            break;
        case ALL_SAMEMAP:      // 同じマップの全クライアントに送信
            for (i = 0; i < fd_max; i++)
            {
                if (session[i] && (sd = session[i]->session_data) != NULL
                    && sd->state.auth && sd->bl.m == bl->m)
                {
                    if (packet_len_table[RBUFW (buf, 0)])
                    {           // packet must exist
                        memcpy (WFIFOP (i, 0), buf, len);
                        WFIFOSET (i, len);
                    }
                }
            }
            break;
        case AREA:
        case AREA_WOS:
        case AREA_WOC:
        case AREA_WOSC:
            map_foreachinarea (clif_send_sub, bl->m, bl->x - AREA_SIZE,
                               bl->y - AREA_SIZE, bl->x + AREA_SIZE,
                               bl->y + AREA_SIZE, BL_PC, buf, len, bl, type);
            break;
        case AREA_CHAT_WOC:
            map_foreachinarea (clif_send_sub, bl->m, bl->x - (AREA_SIZE - 5),
                               bl->y - (AREA_SIZE - 5),
                               bl->x + (AREA_SIZE - 5),
                               bl->y + (AREA_SIZE - 5), BL_PC, buf, len, bl,
                               AREA_CHAT_WOC);
            break;
        case CHAT:
        case CHAT_WOS:
            cd = (struct chat_data *) bl;
            if (bl->type == BL_PC)
            {
                sd = (struct map_session_data *) bl;
                cd = (struct chat_data *) map_id2bl (sd->chatID);
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
                if (packet_len_table[RBUFW (buf, 0)])
                {               // packet must exist
                    memcpy (WFIFOP (cd->usersd[i]->fd, 0), buf, len);
                    WFIFOSET (cd->usersd[i]->fd, len);
                }
            }
            break;

        case PARTY_AREA:       // 同じ画面内の全パーティーメンバに送信
        case PARTY_AREA_WOS:   // 自分以外の同じ画面内の全パーティーメンバに送信
            x0 = bl->x - AREA_SIZE;
            y0 = bl->y - AREA_SIZE;
            x1 = bl->x + AREA_SIZE;
            y1 = bl->y + AREA_SIZE;
        case PARTY:            // 全パーティーメンバに送信
        case PARTY_WOS:        // 自分以外の全パーティーメンバに送信
        case PARTY_SAMEMAP:    // 同じマップの全パーティーメンバに送信
        case PARTY_SAMEMAP_WOS:    // 自分以外の同じマップの全パーティーメンバに送信
            if (bl->type == BL_PC)
            {
                sd = (struct map_session_data *) bl;
                if (sd->partyspy > 0)
                {
                    p = party_search (sd->partyspy);
                }
                else
                {
                    if (sd->status.party_id > 0)
                        p = party_search (sd->status.party_id);
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
                        if (packet_len_table[RBUFW (buf, 0)])
                        {       // packet must exist
                            memcpy (WFIFOP (sd->fd, 0), buf, len);
                            WFIFOSET (sd->fd, len);
                        }
                    }
                }
                for (i = 0; i < fd_max; i++)
                {
                    if (session[i] && (sd = session[i]->session_data) != NULL
                        && sd->state.auth)
                    {
                        if (sd->partyspy == p->party_id)
                        {
                            if (packet_len_table[RBUFW (buf, 0)])
                            {   // packet must exist
                                memcpy (WFIFOP (sd->fd, 0), buf, len);
                                WFIFOSET (sd->fd, len);
                            }
                        }
                    }
                }
            }
            break;
        case SELF:
            sd = (struct map_session_data *) bl;
            if (packet_len_table[RBUFW (buf, 0)])
            {                   // packet must exist
                memcpy (WFIFOP (sd->fd, 0), buf, len);
                WFIFOSET (sd->fd, len);
            }
            break;

/* New definitions for guilds [Valaris]	*/

        case GUILD_AREA:
        case GUILD_AREA_WOS:
            x0 = bl->x - AREA_SIZE;
            y0 = bl->y - AREA_SIZE;
            x1 = bl->x + AREA_SIZE;
            y1 = bl->y + AREA_SIZE;
        case GUILD:
        case GUILD_WOS:
            if (bl && bl->type == BL_PC)
            {                   // guildspy [Syrus22]
                sd = (struct map_session_data *) bl;
                if (sd->guildspy > 0)
                {
                    g = guild_search (sd->guildspy);
                }
                else
                {
                    if (sd->status.guild_id > 0)
                        g = guild_search (sd->status.guild_id);
                }
            }
            if (g)
            {
                for (i = 0; i < g->max_member; i++)
                {
                    if ((sd = g->member[i].sd) != NULL)
                    {
                        if (type == GUILD_WOS && sd->bl.id == bl->id)
                            continue;
                        if (packet_len_table[RBUFW (buf, 0)])
                        {       // packet must exist
                            memcpy (WFIFOP (sd->fd, 0), buf, len);
                            WFIFOSET (sd->fd, len);
                        }
                    }
                }
                for (i = 0; i < fd_max; i++)
                {
                    if (session[i] && (sd = session[i]->session_data) != NULL
                        && sd->state.auth)
                    {
                        if (sd->guildspy == g->guild_id)
                        {
                            if (packet_len_table[RBUFW (buf, 0)])
                            {   // packet must exist
                                memcpy (WFIFOP (sd->fd, 0), buf, len);
                                WFIFOSET (sd->fd, len);
                            }
                        }
                    }
                }
            }
            break;
        case GUILD_SAMEMAP:
        case GUILD_SAMEMAP_WOS:
            if (bl->type == BL_PC)
            {
                sd = (struct map_session_data *) bl;
                if (sd->status.guild_id > 0)
                    g = guild_search (sd->status.guild_id);
            }
            if (g)
            {
                for (i = 0; i < g->max_member; i++)
                {
                    if ((sd = g->member[i].sd) != NULL)
                    {
                        if (sd->bl.id == bl->id && (type == GUILD_WOS ||
                                                    type == GUILD_SAMEMAP_WOS
                                                    || type ==
                                                    GUILD_AREA_WOS))
                            continue;
                        if (type != GUILD && type != GUILD_WOS && bl->m != sd->bl.m)    // マップチェック
                            continue;
                        if ((type == GUILD_AREA || type == GUILD_AREA_WOS) &&
                            (sd->bl.x < x0 || sd->bl.y < y0 ||
                             sd->bl.x > x1 || sd->bl.y > y1))
                            continue;
                        if (packet_len_table[RBUFW (buf, 0)])
                        {       // packet must exist
                            memcpy (WFIFOP (sd->fd, 0), buf, len);
                            WFIFOSET (sd->fd, len);
                        }
                    }
                }
            }
            break;

        default:
            if (battle_config.error_log)
                printf ("clif_send まだ作ってないよー\n");
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
int clif_authok (struct map_session_data *sd)
{
    int  fd;

    nullpo_retr (0, sd);

    if (!sd)
        return 0;

    if (!sd->fd)
        return 0;

    fd = sd->fd;

    WFIFOW (fd, 0) = 0x73;
    WFIFOL (fd, 2) = gettick ();
    WFIFOPOS (fd, 6, sd->bl.x, sd->bl.y);
    WFIFOB (fd, 9) = 5;
    WFIFOB (fd, 10) = 5;
    WFIFOSET (fd, packet_len_table[0x73]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_authfail_fd (int fd, int type)
{
    if (!fd || !session[fd])
        return 0;

    WFIFOW (fd, 0) = 0x81;
    WFIFOL (fd, 2) = type;
    WFIFOSET (fd, packet_len_table[0x81]);

    clif_setwaitclose (fd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok (int id)
{
    struct map_session_data *sd;
    int  fd;

    if ((sd = map_id2sd (id)) == NULL)
        return 1;

    if (!sd->fd)
        return 1;

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xb3;
    WFIFOB (fd, 2) = 1;
    WFIFOSET (fd, packet_len_table[0xb3]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set009e (struct flooritem_data *fitem, char *buf)
{
    int  view;

    nullpo_retr (0, fitem);

    //009e <ID>.l <name ID>.w <identify flag>.B <X>.w <Y>.w <subX>.B <subY>.B <amount>.w
    WBUFW (buf, 0) = 0x9e;
    WBUFL (buf, 2) = fitem->bl.id;
    if ((view = itemdb_viewid (fitem->item_data.nameid)) > 0)
        WBUFW (buf, 6) = view;
    else
        WBUFW (buf, 6) = fitem->item_data.nameid;
    WBUFB (buf, 8) = fitem->item_data.identify;
    WBUFW (buf, 9) = fitem->bl.x;
    WBUFW (buf, 11) = fitem->bl.y;
    WBUFB (buf, 13) = fitem->subx;
    WBUFB (buf, 14) = fitem->suby;
    WBUFW (buf, 15) = fitem->item_data.amount;

    return packet_len_table[0x9e];
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem (struct flooritem_data *fitem)
{
    char buf[64];

    nullpo_retr (0, fitem);

    if (fitem->item_data.nameid <= 0)
        return 0;
    clif_set009e (fitem, buf);
    clif_send (buf, packet_len_table[0x9e], &fitem->bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearflooritem (struct flooritem_data *fitem, int fd)
{
    unsigned char buf[16];

    nullpo_retr (0, fitem);

    WBUFW (buf, 0) = 0xa1;
    WBUFL (buf, 2) = fitem->bl.id;

    if (fd == 0)
    {
        clif_send (buf, packet_len_table[0xa1], &fitem->bl, AREA);
    }
    else
    {
        memcpy (WFIFOP (fd, 0), buf, 6);
        WFIFOSET (fd, packet_len_table[0xa1]);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar (struct block_list *bl, int type)
{
    unsigned char buf[16];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x80;
    WBUFL (buf, 2) = bl->id;
    if (type == 9)
    {
        WBUFB (buf, 6) = 0;
        clif_send (buf, packet_len_table[0x80], bl, AREA);
    }
    else
    {
        WBUFB (buf, 6) = type;
        clif_send (buf, packet_len_table[0x80], bl,
                   type == 1 ? AREA : AREA_WOS);
    }

    return 0;
}

static int clif_clearchar_delay_sub (int tid, unsigned int tick, int id,
                                     int data)
{
    struct block_list *bl = (struct block_list *) id;

    clif_clearchar (bl, data);
    map_freeblock (bl);

    return 0;
}

int clif_clearchar_delay (unsigned int tick, struct block_list *bl, int type)
{
    struct block_list *tmpbl = calloc (sizeof (struct block_list), 1);
    if (tmpbl == NULL)
    {
        printf ("clif_clearchar_delay: out of memory !\n");
        exit (1);
    }
    memcpy (tmpbl, bl, sizeof (struct block_list));
    add_timer (tick, clif_clearchar_delay_sub, (int) tmpbl, type);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar_id (int id, int type, int fd)
{
    unsigned char buf[16];

    WBUFW (buf, 0) = 0x80;
    WBUFL (buf, 2) = id;
    WBUFB (buf, 6) = type;
    memcpy (WFIFOP (fd, 0), buf, 7);
    WFIFOSET (fd, packet_len_table[0x80]);

    return 0;
}

/*
static int current_weapon(struct map_session_data *sd)
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
static int clif_set0078 (struct map_session_data *sd, unsigned char *buf)
{
    int  level = 0;

    nullpo_retr (0, sd);

    if (sd->disguise > 23 && sd->disguise < 4001)
    {                           // mob disguises [Valaris]
        WBUFW (buf, 0) = 0x78;
        WBUFL (buf, 2) = sd->bl.id;
        WBUFW (buf, 6) = battle_get_speed (&sd->bl);
        WBUFW (buf, 8) = sd->opt1;
        WBUFW (buf, 10) = sd->opt2;
        WBUFW (buf, 12) = sd->status.option;
        WBUFW (buf, 14) = sd->disguise;
        WBUFW (buf, 42) = 0;
        WBUFB (buf, 44) = 0;
        WBUFPOS (buf, 46, sd->bl.x, sd->bl.y);
        WBUFB (buf, 48) |= sd->dir & 0x0f;
        WBUFB (buf, 49) = 5;
        WBUFB (buf, 50) = 5;
        WBUFB (buf, 51) = 0;
        WBUFW (buf, 52) =
            ((level =
              battle_get_lv (&sd->bl)) >
             battle_config.max_lv) ? battle_config.max_lv : level;

        return packet_len_table[0x78];
    }

    WBUFW (buf, 0) = 0x1d8;
    WBUFL (buf, 2) = sd->bl.id;
    WBUFW (buf, 6) = sd->speed;
    WBUFW (buf, 8) = sd->opt1;
    WBUFW (buf, 10) = sd->opt2;
    WBUFW (buf, 12) = sd->status.option;
    WBUFW (buf, 14) = sd->view_class;
    WBUFW (buf, 16) = sd->status.hair;
    if (sd->attack_spell_override)
        WBUFB (buf, 18) = sd->attack_spell_look_override;
    else
    {
        if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]]
            && sd->view_class != 22)
        {
            if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
                WBUFW (buf, 18) =
                    sd->inventory_data[sd->equip_index[9]]->view_id;
            else
                WBUFW (buf, 18) =
                    sd->status.inventory[sd->equip_index[9]].nameid;
        }
        else
            WBUFW (buf, 18) = 0;
    }
    if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9]
        && sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22)
    {
        if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
            WBUFW (buf, 20) = sd->inventory_data[sd->equip_index[8]]->view_id;
        else
            WBUFW (buf, 20) = sd->status.inventory[sd->equip_index[8]].nameid;
    }
    else
        WBUFW (buf, 20) = 0;
    WBUFW (buf, 22) = sd->status.head_bottom;
    WBUFW (buf, 24) = sd->status.head_top;
    WBUFW (buf, 26) = sd->status.head_mid;
    WBUFW (buf, 28) = sd->status.hair_color;
    WBUFW (buf, 30) = sd->status.clothes_color;
    WBUFW (buf, 32) = sd->head_dir;
    WBUFL (buf, 34) = sd->status.guild_id;
    WBUFW (buf, 38) = sd->guild_emblem_id;
    WBUFW (buf, 40) = sd->status.manner;
    WBUFW (buf, 42) = sd->opt3;
    WBUFB (buf, 44) = sd->status.karma;
    WBUFB (buf, 45) = sd->sex;
    WBUFPOS (buf, 46, sd->bl.x, sd->bl.y);
    WBUFB (buf, 48) |= sd->dir & 0x0f;
    WBUFW (buf, 49) = (pc_isGM (sd) == 60 || pc_isGM (sd) == 99) ? 0x80 : 0;
    WBUFB (buf, 51) = sd->state.dead_sit;
    WBUFW (buf, 52) = 0;

    return packet_len_table[0x1d8];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set007b (struct map_session_data *sd, unsigned char *buf)
{
    int  level = 0;
    nullpo_retr (0, sd);

    if (sd->disguise > 23 && sd->disguise < 4001)
    {                           // mob disguises [Valaris]
        WBUFW (buf, 0) = 0x7b;
        WBUFL (buf, 2) = sd->bl.id;
        WBUFW (buf, 6) = battle_get_speed (&sd->bl);
        WBUFW (buf, 8) = sd->opt1;
        WBUFW (buf, 10) = sd->opt2;
        WBUFW (buf, 12) = sd->status.option;
        WBUFW (buf, 14) = sd->disguise;
        WBUFL (buf, 22) = gettick ();
        WBUFW (buf, 46) = 0;
        WBUFB (buf, 48) = 0;
        WBUFPOS2 (buf, 50, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
        WBUFB (buf, 55) = 0;
        WBUFB (buf, 56) = 5;
        WBUFB (buf, 57) = 5;
        WBUFW (buf, 58) =
            ((level =
              battle_get_lv (&sd->bl)) >
             battle_config.max_lv) ? battle_config.max_lv : level;

        return packet_len_table[0x7b];
    }

    WBUFW (buf, 0) = 0x1da;
    WBUFL (buf, 2) = sd->bl.id;
    WBUFW (buf, 6) = sd->speed;
    WBUFW (buf, 8) = sd->opt1;
    WBUFW (buf, 10) = sd->opt2;
    WBUFW (buf, 12) = sd->status.option;
    WBUFW (buf, 14) = sd->view_class;
    WBUFW (buf, 16) = sd->status.hair;
    if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]]
        && sd->view_class != 22)
    {
        if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
            WBUFW (buf, 18) = sd->inventory_data[sd->equip_index[9]]->view_id;
        else
            WBUFW (buf, 18) = sd->status.inventory[sd->equip_index[9]].nameid;
    }
    else
        WBUFW (buf, 18) = 0;
    if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9]
        && sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22)
    {
        if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
            WBUFW (buf, 20) = sd->inventory_data[sd->equip_index[8]]->view_id;
        else
            WBUFW (buf, 20) = sd->status.inventory[sd->equip_index[8]].nameid;
    }
    else
        WBUFW (buf, 20) = 0;
    WBUFW (buf, 22) = sd->status.head_bottom;
    WBUFL (buf, 24) = gettick ();
    WBUFW (buf, 28) = sd->status.head_top;
    WBUFW (buf, 30) = sd->status.head_mid;
    WBUFW (buf, 32) = sd->status.hair_color;
    WBUFW (buf, 34) = sd->status.clothes_color;
    WBUFW (buf, 36) = sd->head_dir;
    WBUFL (buf, 38) = sd->status.guild_id;
    WBUFW (buf, 42) = sd->guild_emblem_id;
    WBUFW (buf, 44) = sd->status.manner;
    WBUFW (buf, 46) = sd->opt3;
    WBUFB (buf, 48) = sd->status.karma;
    WBUFB (buf, 49) = sd->sex;
    WBUFPOS2 (buf, 50, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
    WBUFW (buf, 55) = pc_isGM (sd) == 60 ? 0x80 : 0;
    WBUFB (buf, 57) = 5;
    WBUFW (buf, 58) = 0;

    return packet_len_table[0x1da];
}

/*==========================================
 * クラスチェンジ typeはMobの場合は1で他は0？
 *------------------------------------------
 */
int clif_class_change (struct block_list *bl, int class, int type)
{
    char buf[16];

    nullpo_retr (0, bl);

    if (class >= MAX_PC_CLASS)
    {
        WBUFW (buf, 0) = 0x1b0;
        WBUFL (buf, 2) = bl->id;
        WBUFB (buf, 6) = type;
        WBUFL (buf, 7) = class;

        clif_send (buf, packet_len_table[0x1b0], bl, AREA);
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mob_class_change (struct mob_data *md, int class)
{
    char buf[16];
    int  view = mob_get_viewclass (class);

    nullpo_retr (0, md);

    if (view >= MAX_PC_CLASS)
    {
        WBUFW (buf, 0) = 0x1b0;
        WBUFL (buf, 2) = md->bl.id;
        WBUFB (buf, 6) = 1;
        WBUFL (buf, 7) = view;

        clif_send (buf, packet_len_table[0x1b0], &md->bl, AREA);
    }
    return 0;
}

// mob equipment [Valaris]

int clif_mob_equip (struct mob_data *md, int nameid)
{
    unsigned char buf[16];

    nullpo_retr (0, md);

    memset (buf, 0, packet_len_table[0x1a4]);

    WBUFW (buf, 0) = 0x1a4;
    WBUFB (buf, 2) = 3;
    WBUFL (buf, 3) = md->bl.id;
    WBUFL (buf, 7) = nameid;

    clif_send (buf, packet_len_table[0x1a4], &md->bl, AREA);

    return 0;
}

/*==========================================
 * MOB表示1
 *------------------------------------------
 */
static int clif_mob0078 (struct mob_data *md, unsigned char *buf)
{
    int  level;

    memset (buf, 0, packet_len_table[0x78]);

    nullpo_retr (0, md);

    WBUFW (buf, 0) = 0x78;
    WBUFL (buf, 2) = md->bl.id;
    WBUFW (buf, 6) = battle_get_speed (&md->bl);
    WBUFW (buf, 8) = md->opt1;
    WBUFW (buf, 10) = md->opt2;
    WBUFW (buf, 12) = md->option;
    WBUFW (buf, 14) = mob_get_viewclass (md->class);
    if ((mob_get_viewclass (md->class) <= 23)
        || (mob_get_viewclass (md->class) == 812)
        || (mob_get_viewclass (md->class) >= 4001))
    {
        WBUFW (buf, 12) |= mob_db[md->class].option;
        WBUFW (buf, 16) = mob_get_hair (md->class);
        WBUFW (buf, 18) = mob_get_weapon (md->class);
        WBUFW (buf, 20) = mob_get_head_buttom (md->class);
        WBUFW (buf, 22) = mob_get_shield (md->class);
        WBUFW (buf, 24) = mob_get_head_top (md->class);
        WBUFW (buf, 26) = mob_get_head_mid (md->class);
        WBUFW (buf, 28) = mob_get_hair_color (md->class);
        WBUFW (buf, 30) = mob_get_clothes_color (md->class);    //Add for player monster dye - Valaris
        WBUFB (buf, 45) = mob_get_sex (md->class);
    }

    if (md->class >= 1285 && md->class <= 1287)
    {                           // Added guardian emblems [Valaris]
        struct guild *g;
        struct guild_castle *gc = guild_mapname2gc (map[md->bl.m].name);
        if (gc && gc->guild_id > 0)
        {
            g = guild_search (gc->guild_id);
            if (g)
            {
                WBUFL (buf, 26) = gc->guild_id;
                WBUFL (buf, 22) = g->emblem_id;
            }
        }
    }                           // End addition

    WBUFPOS (buf, 46, md->bl.x, md->bl.y);
    WBUFB (buf, 48) |= md->dir & 0x0f;
    WBUFB (buf, 49) = 5;
    WBUFB (buf, 50) = 5;
    WBUFW (buf, 52) =
        ((level =
          battle_get_lv (&md->bl)) >
         battle_config.max_lv) ? battle_config.max_lv : level;

    return packet_len_table[0x78];
}

/*==========================================
 * MOB表示2
 *------------------------------------------
 */
static int clif_mob007b (struct mob_data *md, unsigned char *buf)
{
    int  level;

    memset (buf, 0, packet_len_table[0x7b]);

    nullpo_retr (0, md);

    WBUFW (buf, 0) = 0x7b;
    WBUFL (buf, 2) = md->bl.id;
    WBUFW (buf, 6) = battle_get_speed (&md->bl);
    WBUFW (buf, 8) = md->opt1;
    WBUFW (buf, 10) = md->opt2;
    WBUFW (buf, 12) = md->option;
    WBUFW (buf, 14) = mob_get_viewclass (md->class);
    if ((mob_get_viewclass (md->class) < 24)
        || (mob_get_viewclass (md->class) > 4000))
    {
        WBUFW (buf, 12) |= mob_db[md->class].option;
        WBUFW (buf, 16) = mob_get_hair (md->class);
        WBUFW (buf, 18) = mob_get_weapon (md->class);
        WBUFW (buf, 20) = mob_get_head_buttom (md->class);
        WBUFL (buf, 22) = gettick ();
        WBUFW (buf, 26) = mob_get_shield (md->class);
        WBUFW (buf, 28) = mob_get_head_top (md->class);
        WBUFW (buf, 30) = mob_get_head_mid (md->class);
        WBUFW (buf, 32) = mob_get_hair_color (md->class);
        WBUFW (buf, 34) = mob_get_clothes_color (md->class);    //Add for player monster dye - Valaris
        WBUFB (buf, 49) = mob_get_sex (md->class);
    }
    else
        WBUFL (buf, 22) = gettick ();

    if (md->class >= 1285 && md->class <= 1287)
    {                           // Added guardian emblems [Valaris]
        struct guild *g;
        struct guild_castle *gc = guild_mapname2gc (map[md->bl.m].name);
        if (gc && gc->guild_id > 0)
        {
            g = guild_search (gc->guild_id);
            if (g)
            {
                WBUFL (buf, 28) = gc->guild_id;
                WBUFL (buf, 24) = g->emblem_id;
            }
        }
    }                           // End addition

    WBUFPOS2 (buf, 50, md->bl.x, md->bl.y, md->to_x, md->to_y);
    WBUFB (buf, 56) = 5;
    WBUFB (buf, 57) = 5;
    WBUFW (buf, 58) =
        ((level =
          battle_get_lv (&md->bl)) >
         battle_config.max_lv) ? battle_config.max_lv : level;

    return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_npc0078 (struct npc_data *nd, unsigned char *buf)
{
    struct guild *g;

    nullpo_retr (0, nd);

    memset (buf, 0, packet_len_table[0x78]);

    WBUFW (buf, 0) = 0x78;
    WBUFL (buf, 2) = nd->bl.id;
    WBUFW (buf, 6) = nd->speed;
    WBUFW (buf, 14) = nd->class;
    if ((nd->class == 722) && (nd->u.scr.guild_id > 0)
        && ((g = guild_search (nd->u.scr.guild_id)) != NULL))
    {
        WBUFL (buf, 22) = g->emblem_id;
        WBUFL (buf, 26) = g->guild_id;
    }
    WBUFPOS (buf, 46, nd->bl.x, nd->bl.y);
    WBUFB (buf, 48) |= nd->dir & 0x0f;
    WBUFB (buf, 49) = 5;
    WBUFB (buf, 50) = 5;

    return packet_len_table[0x78];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set01e1 (struct map_session_data *sd, unsigned char *buf)
{
    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x1e1;
    WBUFL (buf, 2) = sd->bl.id;
    WBUFW (buf, 6) = sd->spiritball;

    return packet_len_table[0x1e1];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0192 (int fd, int m, int x, int y, int type)
{
    WFIFOW (fd, 0) = 0x192;
    WFIFOW (fd, 2) = x;
    WFIFOW (fd, 4) = y;
    WFIFOW (fd, 6) = type;
    memcpy (WFIFOP (fd, 8), map[m].name, 16);
    WFIFOSET (fd, packet_len_table[0x192]);

    return 0;
}

/* These indices are derived from equip_pos in pc.c and some guesswork */
static int equip_points[LOOK_LAST + 1] = {
    -1,                         /* 0: base */
    -1,                         /* 1: hair */
    9,                          /* 2: weapon */
    4,                          /* 3: head botom -- leg armour */
    6,                          /* 4: head top -- hat */
    5,                          /* 5: head mid -- torso armour */
    -1,                         /* 6: hair colour */
    -1,                         /* 6: clothes colour */
    8,                          /* 6: shield */
    2,                          /* 9: shoes */
    3,                          /* gloves */
    1,                          /* cape */
    7,                          /* misc1 */
    0,                          /* misc2 */
};

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpc (struct map_session_data *sd)
{
    unsigned char buf[128];

    nullpo_retr (0, sd);

    if (sd->disguise > 23 && sd->disguise < 4001)
    {                           // mob disguises [Valaris]
        clif_clearchar (&sd->bl, 9);

        memset (buf, 0, packet_len_table[0x119]);

        WBUFW (buf, 0) = 0x119;
        WBUFL (buf, 2) = sd->bl.id;
        WBUFW (buf, 6) = 0;
        WBUFW (buf, 8) = 0;
        WBUFW (buf, 10) = 0x40;
        WBUFB (buf, 12) = 0;

        clif_send (buf, packet_len_table[0x119], &sd->bl, SELF);

        memset (buf, 0, packet_len_table[0x7c]);

        WBUFW (buf, 0) = 0x7c;
        WBUFL (buf, 2) = sd->bl.id;
        WBUFW (buf, 6) = sd->speed;
        WBUFW (buf, 8) = sd->opt1;
        WBUFW (buf, 10) = sd->opt2;
        WBUFW (buf, 12) = sd->status.option;
        WBUFW (buf, 20) = sd->disguise;
        WBUFPOS (buf, 36, sd->bl.x, sd->bl.y);
        clif_send (buf, packet_len_table[0x7c], &sd->bl, AREA);
    }

    clif_set0078 (sd, buf);

    WBUFW (buf, 0) = 0x1d9;
    WBUFW (buf, 51) = 0;
    clif_send (buf, packet_len_table[0x1d9], &sd->bl, AREA_WOS);

    if (sd->spiritball > 0)
        clif_spiritball (sd);

    if (sd->status.guild_id > 0)
    {                           // force display of guild emblem [Valaris]
        struct guild *g = guild_search (sd->status.guild_id);
        if (g)
            clif_guild_emblem (sd, g);
    }                           // end addition [Valaris]

    if (sd->status.class == 13 || sd->status.class == 21
        || sd->status.class == 4014 || sd->status.class == 4022)
        pc_setoption (sd, sd->status.option | 0x0020);  // [Valaris]

    if ((pc_isriding (sd) && pc_checkskill (sd, KN_RIDING) > 0)
        && (sd->status.class == 7 || sd->status.class == 14
            || sd->status.class == 4008 || sd->status.class == 4015))
        pc_setriding (sd);      // update peco riders for people upgrading athena [Valaris]

    if (map[sd->bl.m].flag.snow)
        clif_specialeffect (&sd->bl, 162, 1);
    if (map[sd->bl.m].flag.fog)
        clif_specialeffect (&sd->bl, 233, 1);
    if (map[sd->bl.m].flag.sakura)
        clif_specialeffect (&sd->bl, 163, 1);
    if (map[sd->bl.m].flag.leaves)
        clif_specialeffect (&sd->bl, 333, 1);
    if (map[sd->bl.m].flag.rain)
        clif_specialeffect (&sd->bl, 161, 1);

//        clif_changelook_accessories(&sd->bl, NULL);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc (struct npc_data *nd)
{
    unsigned char buf[64];
    int  len;

    nullpo_retr (0, nd);

    if (nd->class < 0 || nd->flag & 1 || nd->class == INVISIBLE_CLASS)
        return 0;

    memset (buf, 0, packet_len_table[0x7c]);

    WBUFW (buf, 0) = 0x7c;
    WBUFL (buf, 2) = nd->bl.id;
    WBUFW (buf, 6) = nd->speed;
    WBUFW (buf, 20) = nd->class;
    WBUFPOS (buf, 36, nd->bl.x, nd->bl.y);

    clif_send (buf, packet_len_table[0x7c], &nd->bl, AREA);

    len = clif_npc0078 (nd, buf);
    clif_send (buf, len, &nd->bl, AREA);

    return 0;
}

int
clif_spawn_fake_npc_for_player (struct map_session_data *sd, int fake_npc_id)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    if (!fd)
        return 0;

    WFIFOW (fd, 0) = 0x7c;
    WFIFOL (fd, 2) = fake_npc_id;
    WFIFOW (fd, 6) = 0;
    WFIFOW (fd, 8) = 0;
    WFIFOW (fd, 10) = 0;
    WFIFOW (fd, 12) = 0;
    WFIFOW (fd, 20) = 127;
    WFIFOPOS (fd, 36, sd->bl.x, sd->bl.y);
    WFIFOSET (fd, packet_len_table[0x7c]);

    WFIFOW (fd, 0) = 0x78;
    WFIFOL (fd, 2) = fake_npc_id;
    WFIFOW (fd, 6) = 0;
    WFIFOW (fd, 8) = 0;
    WFIFOW (fd, 10) = 0;
    WFIFOW (fd, 12) = 0;
    WFIFOW (fd, 14) = 127;      // identifies as NPC
    WFIFOW (fd, 20) = 127;
    WFIFOPOS (fd, 46, sd->bl.x, sd->bl.y);
    WFIFOPOS (fd, 36, sd->bl.x, sd->bl.y);
    WFIFOB (fd, 49) = 5;
    WFIFOB (fd, 50) = 5;
    WFIFOSET (fd, packet_len_table[0x78]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob (struct mob_data *md)
{
    unsigned char buf[64];
    int  len;

    nullpo_retr (0, md);

    if (mob_get_viewclass (md->class) > 23)
    {
        memset (buf, 0, packet_len_table[0x7c]);

        WBUFW (buf, 0) = 0x7c;
        WBUFL (buf, 2) = md->bl.id;
        WBUFW (buf, 6) = md->stats[MOB_SPEED];
        WBUFW (buf, 8) = md->opt1;
        WBUFW (buf, 10) = md->opt2;
        WBUFW (buf, 12) = md->option;
        WBUFW (buf, 20) = mob_get_viewclass (md->class);
        WBUFPOS (buf, 36, md->bl.x, md->bl.y);
        clif_send (buf, packet_len_table[0x7c], &md->bl, AREA);
    }

    len = clif_mob0078 (md, buf);
    clif_send (buf, len, &md->bl, AREA);

    if (mob_get_equip (md->class) > 0)  // mob equipment [Valaris]
        clif_mob_equip (md, mob_get_equip (md->class));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_servertick (struct map_session_data *sd)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x7f;
    WFIFOL (fd, 2) = sd->server_tick;
    WFIFOSET (fd, packet_len_table[0x7f]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok (struct map_session_data *sd)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x87;
    WFIFOL (fd, 2) = gettick ();;
    WFIFOPOS2 (fd, 6, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
    WFIFOB (fd, 11) = 0;
    WFIFOSET (fd, packet_len_table[0x87]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar (struct map_session_data *sd)
{
    int  fd;
    int  len;
    unsigned char buf[256];

    nullpo_retr (0, sd);

    fd = sd->fd;

    len = clif_set007b (sd, buf);

    if (sd->disguise > 23 && sd->disguise < 4001)
    {
        clif_send (buf, len, &sd->bl, AREA);
        return 0;
    }
    else
        clif_send (buf, len, &sd->bl, AREA_WOS);

    if (battle_config.save_clothcolor == 1 && sd->status.clothes_color > 0)
        clif_changelook (&sd->bl, LOOK_CLOTHES_COLOR,
                         sd->status.clothes_color);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_quitsave (int fd, struct map_session_data *sd)
{
    map_quit (sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_waitclose (int tid, unsigned int tick, int id, int data)
{
    if (session[id])
        session[id]->eof = 1;

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose (int fd)
{
    add_timer (gettick () + 5000, clif_waitclose, fd, 0);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemap (struct map_session_data *sd, char *mapname, int x, int y)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0x91;
    memcpy (WFIFOP (fd, 2), mapname, 16);
    WFIFOW (fd, 18) = x;
    WFIFOW (fd, 20) = y;
    WFIFOSET (fd, packet_len_table[0x91]);

    if (sd->disguise > 23 && sd->disguise < 4001)   // mob disguises [Valaris]
        clif_spawnpc (sd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapserver (struct map_session_data *sd, char *mapname, int x,
                          int y, int ip, int port)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x92;
    memcpy (WFIFOP (fd, 2), mapname, 16);
    WFIFOW (fd, 18) = x;
    WFIFOW (fd, 20) = y;
    WFIFOL (fd, 22) = ip;
    WFIFOW (fd, 26) = port;
    WFIFOSET (fd, packet_len_table[0x92]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpos (struct block_list *bl)
{
    char buf[16];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x88;
    WBUFL (buf, 2) = bl->id;
    WBUFW (buf, 6) = bl->x;
    WBUFW (buf, 8) = bl->y;

    clif_send (buf, packet_len_table[0x88], bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npcbuysell (struct map_session_data *sd, int id)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xc4;
    WFIFOL (fd, 2) = id;
    WFIFOSET (fd, packet_len_table[0xc4]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_buylist (struct map_session_data *sd, struct npc_data *nd)
{
    struct item_data *id;
    int  fd, i, val;

    nullpo_retr (0, sd);
    nullpo_retr (0, nd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xc6;
    for (i = 0; nd->u.shop_item[i].nameid > 0; i++)
    {
        id = itemdb_search (nd->u.shop_item[i].nameid);
        val = nd->u.shop_item[i].value;
        WFIFOL (fd, 4 + i * 11) = val;
        if (!id->flag.value_notdc)
            val = pc_modifybuyvalue (sd, val);
        WFIFOL (fd, 8 + i * 11) = val;
        WFIFOB (fd, 12 + i * 11) = id->type;
        if (id->view_id > 0)
            WFIFOW (fd, 13 + i * 11) = id->view_id;
        else
            WFIFOW (fd, 13 + i * 11) = nd->u.shop_item[i].nameid;
    }
    WFIFOW (fd, 2) = i * 11 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_selllist (struct map_session_data *sd)
{
    int  fd, i, c = 0, val;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xc7;
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid > 0 && sd->inventory_data[i])
        {
            val = sd->inventory_data[i]->value_sell;
            if (val < 0)
                continue;
            WFIFOW (fd, 4 + c * 10) = i + 2;
            WFIFOL (fd, 6 + c * 10) = val;
            if (!sd->inventory_data[i]->flag.value_notoc)
                val = pc_modifysellvalue (sd, val);
            WFIFOL (fd, 10 + c * 10) = val;
            c++;
        }
    }
    WFIFOW (fd, 2) = c * 10 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmes (struct map_session_data *sd, int npcid, char *mes)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xb4;
    WFIFOW (fd, 2) = strlen (mes) + 9;
    WFIFOL (fd, 4) = npcid;
    strcpy (WFIFOP (fd, 8), mes);
    WFIFOSET (fd, WFIFOW (fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptnext (struct map_session_data *sd, int npcid)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xb5;
    WFIFOL (fd, 2) = npcid;
    WFIFOSET (fd, packet_len_table[0xb5]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptclose (struct map_session_data *sd, int npcid)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xb6;
    WFIFOL (fd, 2) = npcid;
    WFIFOSET (fd, packet_len_table[0xb6]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmenu (struct map_session_data *sd, int npcid, char *mes)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xb7;
    WFIFOW (fd, 2) = strlen (mes) + 8;
    WFIFOL (fd, 4) = npcid;
    strcpy (WFIFOP (fd, 8), mes);
    WFIFOSET (fd, WFIFOW (fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinput (struct map_session_data *sd, int npcid)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x142;
    WFIFOL (fd, 2) = npcid;
    WFIFOSET (fd, packet_len_table[0x142]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinputstr (struct map_session_data *sd, int npcid)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x1d4;
    WFIFOL (fd, 2) = npcid;
    WFIFOSET (fd, packet_len_table[0x1d4]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_viewpoint (struct map_session_data *sd, int npc_id, int type, int x,
                    int y, int id, int color)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x144;
    WFIFOL (fd, 2) = npc_id;
    WFIFOL (fd, 6) = type;
    WFIFOL (fd, 10) = x;
    WFIFOL (fd, 14) = y;
    WFIFOB (fd, 18) = id;
    WFIFOL (fd, 19) = color;
    WFIFOSET (fd, packet_len_table[0x144]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_cutin (struct map_session_data *sd, char *image, int type)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x1b3;
    memcpy (WFIFOP (fd, 2), image, 64);
    WFIFOB (fd, 66) = type;
    WFIFOSET (fd, packet_len_table[0x1b3]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem (struct map_session_data *sd, int n, int amount, int fail)
{
    int  fd, j;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    if (fail)
    {
        WBUFW (buf, 0) = 0xa0;
        WBUFW (buf, 2) = n + 2;
        WBUFW (buf, 4) = amount;
        WBUFW (buf, 6) = 0;
        WBUFB (buf, 8) = 0;
        WBUFB (buf, 9) = 0;
        WBUFB (buf, 10) = 0;
        WBUFW (buf, 11) = 0;
        WBUFW (buf, 13) = 0;
        WBUFW (buf, 15) = 0;
        WBUFW (buf, 17) = 0;
        WBUFW (buf, 19) = 0;
        WBUFB (buf, 21) = 0;
        WBUFB (buf, 22) = fail;
    }
    else
    {
        if (n < 0 || n >= MAX_INVENTORY || sd->status.inventory[n].nameid <= 0
            || sd->inventory_data[n] == NULL)
            return 1;

        WBUFW (buf, 0) = 0xa0;
        WBUFW (buf, 2) = n + 2;
        WBUFW (buf, 4) = amount;
        if (sd->inventory_data[n]->view_id > 0)
            WBUFW (buf, 6) = sd->inventory_data[n]->view_id;
        else
            WBUFW (buf, 6) = sd->status.inventory[n].nameid;
        WBUFB (buf, 8) = sd->status.inventory[n].identify;
        if (sd->status.inventory[n].broken == 1)
            WBUFB (buf, 9) = 1; // is weapon broken [Valaris]
        else
            WBUFB (buf, 9) = sd->status.inventory[n].attribute;
        WBUFB (buf, 10) = sd->status.inventory[n].refine;
        if (sd->status.inventory[n].card[0] == 0x00ff
            || sd->status.inventory[n].card[0] == 0x00fe
            || sd->status.inventory[n].card[0] == (short) 0xff00)
        {
            WBUFW (buf, 11) = sd->status.inventory[n].card[0];
            WBUFW (buf, 13) = sd->status.inventory[n].card[1];
            WBUFW (buf, 15) = sd->status.inventory[n].card[2];
            WBUFW (buf, 17) = sd->status.inventory[n].card[3];
        }
        else
        {
            if (sd->status.inventory[n].card[0] > 0
                && (j = itemdb_viewid (sd->status.inventory[n].card[0])) > 0)
                WBUFW (buf, 11) = j;
            else
                WBUFW (buf, 11) = sd->status.inventory[n].card[0];
            if (sd->status.inventory[n].card[1] > 0
                && (j = itemdb_viewid (sd->status.inventory[n].card[1])) > 0)
                WBUFW (buf, 13) = j;
            else
                WBUFW (buf, 13) = sd->status.inventory[n].card[1];
            if (sd->status.inventory[n].card[2] > 0
                && (j = itemdb_viewid (sd->status.inventory[n].card[2])) > 0)
                WBUFW (buf, 15) = j;
            else
                WBUFW (buf, 15) = sd->status.inventory[n].card[2];
            if (sd->status.inventory[n].card[3] > 0
                && (j = itemdb_viewid (sd->status.inventory[n].card[3])) > 0)
                WBUFW (buf, 17) = j;
            else
                WBUFW (buf, 17) = sd->status.inventory[n].card[3];
        }
        WBUFW (buf, 19) = pc_equippoint (sd, n);
        WBUFB (buf, 21) =
            (sd->inventory_data[n]->type ==
             7) ? 4 : sd->inventory_data[n]->type;
        WBUFB (buf, 22) = fail;
    }

    WFIFOSET (fd, packet_len_table[0xa0]);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_delitem (struct map_session_data *sd, int n, int amount)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xaf;
    WFIFOW (fd, 2) = n + 2;
    WFIFOW (fd, 4) = amount;

    WFIFOSET (fd, packet_len_table[0xaf]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_itemlist (struct map_session_data *sd)
{
    int  i, n, fd, arrow = -1;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    WBUFW (buf, 0) = 0x1ee;
    for (i = 0, n = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid <= 0
            || sd->inventory_data[i] == NULL
            || itemdb_isequip2 (sd->inventory_data[i]))
            continue;
        WBUFW (buf, n * 18 + 4) = i + 2;
        if (sd->inventory_data[i]->view_id > 0)
            WBUFW (buf, n * 18 + 6) = sd->inventory_data[i]->view_id;
        else
            WBUFW (buf, n * 18 + 6) = sd->status.inventory[i].nameid;
        WBUFB (buf, n * 18 + 8) = sd->inventory_data[i]->type;
        WBUFB (buf, n * 18 + 9) = sd->status.inventory[i].identify;
        WBUFW (buf, n * 18 + 10) = sd->status.inventory[i].amount;
        if (sd->inventory_data[i]->equip == 0x8000)
        {
            WBUFW (buf, n * 18 + 12) = 0x8000;
            if (sd->status.inventory[i].equip)
                arrow = i;      // ついでに矢装備チェック
        }
        else
            WBUFW (buf, n * 18 + 12) = 0;
        WBUFW (buf, n * 18 + 14) = sd->status.inventory[i].card[0];
        WBUFW (buf, n * 18 + 16) = sd->status.inventory[i].card[1];
        WBUFW (buf, n * 18 + 18) = sd->status.inventory[i].card[2];
        WBUFW (buf, n * 18 + 20) = sd->status.inventory[i].card[3];
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 18;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    if (arrow >= 0)
        clif_arrowequip (sd, arrow);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equiplist (struct map_session_data *sd)
{
    int  i, j, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    WBUFW (buf, 0) = 0xa4;
    for (i = 0, n = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid <= 0
            || sd->inventory_data[i] == NULL
            || !itemdb_isequip2 (sd->inventory_data[i]))
            continue;
        WBUFW (buf, n * 20 + 4) = i + 2;
        if (sd->inventory_data[i]->view_id > 0)
            WBUFW (buf, n * 20 + 6) = sd->inventory_data[i]->view_id;
        else
            WBUFW (buf, n * 20 + 6) = sd->status.inventory[i].nameid;
        WBUFB (buf, n * 20 + 8) =
            (sd->inventory_data[i]->type ==
             7) ? 4 : sd->inventory_data[i]->type;
        WBUFB (buf, n * 20 + 9) = sd->status.inventory[i].identify;
        WBUFW (buf, n * 20 + 10) = pc_equippoint (sd, i);
        WBUFW (buf, n * 20 + 12) = sd->status.inventory[i].equip;
        if (sd->status.inventory[i].broken == 1)
            WBUFB (buf, n * 20 + 14) = 1;   // is weapon broken [Valaris]
        else
            WBUFB (buf, n * 20 + 14) = sd->status.inventory[i].attribute;
        WBUFB (buf, n * 20 + 15) = sd->status.inventory[i].refine;
        if (sd->status.inventory[i].card[0] == 0x00ff
            || sd->status.inventory[i].card[0] == 0x00fe
            || sd->status.inventory[i].card[0] == (short) 0xff00)
        {
            WBUFW (buf, n * 20 + 16) = sd->status.inventory[i].card[0];
            WBUFW (buf, n * 20 + 18) = sd->status.inventory[i].card[1];
            WBUFW (buf, n * 20 + 20) = sd->status.inventory[i].card[2];
            WBUFW (buf, n * 20 + 22) = sd->status.inventory[i].card[3];
        }
        else
        {
            if (sd->status.inventory[i].card[0] > 0
                && (j = itemdb_viewid (sd->status.inventory[i].card[0])) > 0)
                WBUFW (buf, n * 20 + 16) = j;
            else
                WBUFW (buf, n * 20 + 16) = sd->status.inventory[i].card[0];
            if (sd->status.inventory[i].card[1] > 0
                && (j = itemdb_viewid (sd->status.inventory[i].card[1])) > 0)
                WBUFW (buf, n * 20 + 18) = j;
            else
                WBUFW (buf, n * 20 + 18) = sd->status.inventory[i].card[1];
            if (sd->status.inventory[i].card[2] > 0
                && (j = itemdb_viewid (sd->status.inventory[i].card[2])) > 0)
                WBUFW (buf, n * 20 + 20) = j;
            else
                WBUFW (buf, n * 20 + 20) = sd->status.inventory[i].card[2];
            if (sd->status.inventory[i].card[3] > 0
                && (j = itemdb_viewid (sd->status.inventory[i].card[3])) > 0)
                WBUFW (buf, n * 20 + 22) = j;
            else
                WBUFW (buf, n * 20 + 22) = sd->status.inventory[i].card[3];
        }
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 20;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 * カプラさんに預けてある消耗品&収集品リスト
 *------------------------------------------
 */
int clif_storageitemlist (struct map_session_data *sd, struct storage *stor)
{
    struct item_data *id;
    int  i, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    WBUFW (buf, 0) = 0x1f0;
    for (i = 0, n = 0; i < MAX_STORAGE; i++)
    {
        if (stor->storage_[i].nameid <= 0)
            continue;
        nullpo_retr (0, id = itemdb_search (stor->storage_[i].nameid));
        if (itemdb_isequip2 (id))
            continue;

        WBUFW (buf, n * 18 + 4) = i + 1;
        if (id->view_id > 0)
            WBUFW (buf, n * 18 + 6) = id->view_id;
        else
            WBUFW (buf, n * 18 + 6) = stor->storage_[i].nameid;
        WBUFB (buf, n * 18 + 8) = id->type;;
        WBUFB (buf, n * 18 + 9) = stor->storage_[i].identify;
        WBUFW (buf, n * 18 + 10) = stor->storage_[i].amount;
        WBUFW (buf, n * 18 + 12) = 0;
        WBUFW (buf, n * 18 + 14) = stor->storage_[i].card[0];
        WBUFW (buf, n * 18 + 16) = stor->storage_[i].card[1];
        WBUFW (buf, n * 18 + 18) = stor->storage_[i].card[2];
        WBUFW (buf, n * 18 + 20) = stor->storage_[i].card[3];
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 18;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 * カプラさんに預けてある装備リスト
 *------------------------------------------
 */
int clif_storageequiplist (struct map_session_data *sd, struct storage *stor)
{
    struct item_data *id;
    int  i, j, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    WBUFW (buf, 0) = 0xa6;
    for (i = 0, n = 0; i < MAX_STORAGE; i++)
    {
        if (stor->storage_[i].nameid <= 0)
            continue;
        nullpo_retr (0, id = itemdb_search (stor->storage_[i].nameid));
        if (!itemdb_isequip2 (id))
            continue;
        WBUFW (buf, n * 20 + 4) = i + 1;
        if (id->view_id > 0)
            WBUFW (buf, n * 20 + 6) = id->view_id;
        else
            WBUFW (buf, n * 20 + 6) = stor->storage_[i].nameid;
        WBUFB (buf, n * 20 + 8) = id->type;
        WBUFB (buf, n * 20 + 9) = stor->storage_[i].identify;
        WBUFW (buf, n * 20 + 10) = id->equip;
        WBUFW (buf, n * 20 + 12) = stor->storage_[i].equip;
        if (stor->storage_[i].broken == 1)
            WBUFB (buf, n * 20 + 14) = 1;   //is weapon broken [Valaris]
        else
            WBUFB (buf, n * 20 + 14) = stor->storage_[i].attribute;
        WBUFB (buf, n * 20 + 15) = stor->storage_[i].refine;
        if (stor->storage_[i].card[0] == 0x00ff
            || stor->storage_[i].card[0] == 0x00fe
            || stor->storage_[i].card[0] == (short) 0xff00)
        {
            WBUFW (buf, n * 20 + 16) = stor->storage_[i].card[0];
            WBUFW (buf, n * 20 + 18) = stor->storage_[i].card[1];
            WBUFW (buf, n * 20 + 20) = stor->storage_[i].card[2];
            WBUFW (buf, n * 20 + 22) = stor->storage_[i].card[3];
        }
        else
        {
            if (stor->storage_[i].card[0] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[0])) > 0)
                WBUFW (buf, n * 20 + 16) = j;
            else
                WBUFW (buf, n * 20 + 16) = stor->storage_[i].card[0];
            if (stor->storage_[i].card[1] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[1])) > 0)
                WBUFW (buf, n * 20 + 18) = j;
            else
                WBUFW (buf, n * 20 + 18) = stor->storage_[i].card[1];
            if (stor->storage_[i].card[2] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[2])) > 0)
                WBUFW (buf, n * 20 + 20) = j;
            else
                WBUFW (buf, n * 20 + 20) = stor->storage_[i].card[2];
            if (stor->storage_[i].card[3] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[3])) > 0)
                WBUFW (buf, n * 20 + 22) = j;
            else
                WBUFW (buf, n * 20 + 22) = stor->storage_[i].card[3];
        }
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 20;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageitemlist (struct map_session_data *sd,
                               struct guild_storage *stor)
{
    struct item_data *id;
    int  i, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);

    WBUFW (buf, 0) = 0x1f0;
    for (i = 0, n = 0; i < MAX_GUILD_STORAGE; i++)
    {
        if (stor->storage_[i].nameid <= 0)
            continue;
        nullpo_retr (0, id = itemdb_search (stor->storage_[i].nameid));
        if (itemdb_isequip2 (id))
            continue;

        WBUFW (buf, n * 18 + 4) = i + 1;
        if (id->view_id > 0)
            WBUFW (buf, n * 18 + 6) = id->view_id;
        else
            WBUFW (buf, n * 18 + 6) = stor->storage_[i].nameid;
        WBUFB (buf, n * 18 + 8) = id->type;;
        WBUFB (buf, n * 18 + 9) = stor->storage_[i].identify;
        WBUFW (buf, n * 18 + 10) = stor->storage_[i].amount;
        WBUFW (buf, n * 18 + 12) = 0;
        WBUFW (buf, n * 18 + 14) = stor->storage_[i].card[0];
        WBUFW (buf, n * 18 + 16) = stor->storage_[i].card[1];
        WBUFW (buf, n * 18 + 18) = stor->storage_[i].card[2];
        WBUFW (buf, n * 18 + 20) = stor->storage_[i].card[3];
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 18;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageequiplist (struct map_session_data *sd,
                                struct guild_storage *stor)
{
    struct item_data *id;
    int  i, j, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);

    WBUFW (buf, 0) = 0xa6;
    for (i = 0, n = 0; i < MAX_GUILD_STORAGE; i++)
    {
        if (stor->storage_[i].nameid <= 0)
            continue;
        nullpo_retr (0, id = itemdb_search (stor->storage_[i].nameid));
        if (!itemdb_isequip2 (id))
            continue;
        WBUFW (buf, n * 20 + 4) = i + 1;
        if (id->view_id > 0)
            WBUFW (buf, n * 20 + 6) = id->view_id;
        else
            WBUFW (buf, n * 20 + 6) = stor->storage_[i].nameid;
        WBUFB (buf, n * 20 + 8) = id->type;
        WBUFB (buf, n * 20 + 9) = stor->storage_[i].identify;
        WBUFW (buf, n * 20 + 10) = id->equip;
        WBUFW (buf, n * 20 + 12) = stor->storage_[i].equip;
        if (stor->storage_[i].broken == 1)
            WBUFB (buf, n * 20 + 14) = 1;   // is weapon broken [Valaris]
        else
            WBUFB (buf, n * 20 + 14) = stor->storage_[i].attribute;
        WBUFB (buf, n * 20 + 15) = stor->storage_[i].refine;
        if (stor->storage_[i].card[0] == 0x00ff
            || stor->storage_[i].card[0] == 0x00fe
            || stor->storage_[i].card[0] == (short) 0xff00)
        {
            WBUFW (buf, n * 20 + 16) = stor->storage_[i].card[0];
            WBUFW (buf, n * 20 + 18) = stor->storage_[i].card[1];
            WBUFW (buf, n * 20 + 20) = stor->storage_[i].card[2];
            WBUFW (buf, n * 20 + 22) = stor->storage_[i].card[3];
        }
        else
        {
            if (stor->storage_[i].card[0] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[0])) > 0)
                WBUFW (buf, n * 20 + 16) = j;
            else
                WBUFW (buf, n * 20 + 16) = stor->storage_[i].card[0];
            if (stor->storage_[i].card[1] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[1])) > 0)
                WBUFW (buf, n * 20 + 18) = j;
            else
                WBUFW (buf, n * 20 + 18) = stor->storage_[i].card[1];
            if (stor->storage_[i].card[2] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[2])) > 0)
                WBUFW (buf, n * 20 + 20) = j;
            else
                WBUFW (buf, n * 20 + 20) = stor->storage_[i].card[2];
            if (stor->storage_[i].card[3] > 0
                && (j = itemdb_viewid (stor->storage_[i].card[3])) > 0)
                WBUFW (buf, n * 20 + 22) = j;
            else
                WBUFW (buf, n * 20 + 22) = stor->storage_[i].card[3];
        }
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 20;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 * ステータスを送りつける
 * 表示専用数字はこの中で計算して送る
 *------------------------------------------
 */
int clif_updatestatus (struct map_session_data *sd, int type)
{
    int  fd, len = 8;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0xb0;
    WFIFOW (fd, 2) = type;
    switch (type)
    {
            // 00b0
        case SP_WEIGHT:
            pc_checkweighticon (sd);
            WFIFOW (fd, 0) = 0xb0;
            WFIFOW (fd, 2) = type;
            WFIFOL (fd, 4) = sd->weight;
            break;
        case SP_MAXWEIGHT:
            WFIFOL (fd, 4) = sd->max_weight;
            break;
        case SP_SPEED:
            WFIFOL (fd, 4) = sd->speed;
            break;
        case SP_BASELEVEL:
            WFIFOL (fd, 4) = sd->status.base_level;
            break;
        case SP_JOBLEVEL:
            WFIFOL (fd, 4) = 0;
            break;
        case SP_MANNER:
            WFIFOL (fd, 4) = sd->status.manner;
            clif_changestatus (&sd->bl, SP_MANNER, sd->status.manner);
            break;
        case SP_STATUSPOINT:
            WFIFOL (fd, 4) = sd->status.status_point;
            break;
        case SP_SKILLPOINT:
            WFIFOL (fd, 4) = sd->status.skill_point;
            break;
        case SP_HIT:
            WFIFOL (fd, 4) = sd->hit;
            break;
        case SP_FLEE1:
            WFIFOL (fd, 4) = sd->flee;
            break;
        case SP_FLEE2:
            WFIFOL (fd, 4) = sd->flee2 / 10;
            break;
        case SP_MAXHP:
            WFIFOL (fd, 4) = sd->status.max_hp;
            break;
        case SP_MAXSP:
            WFIFOL (fd, 4) = sd->status.max_sp;
            break;
        case SP_HP:
            WFIFOL (fd, 4) = sd->status.hp;
            break;
        case SP_SP:
            WFIFOL (fd, 4) = sd->status.sp;
            break;
        case SP_ASPD:
            WFIFOL (fd, 4) = sd->aspd;
            break;
        case SP_ATK1:
            WFIFOL (fd, 4) = sd->base_atk + sd->watk;
            break;
        case SP_DEF1:
            WFIFOL (fd, 4) = sd->def;
            break;
        case SP_MDEF1:
            WFIFOL (fd, 4) = sd->mdef;
            break;
        case SP_ATK2:
            WFIFOL (fd, 4) = sd->watk2;
            break;
        case SP_DEF2:
            WFIFOL (fd, 4) = sd->def2;
            break;
        case SP_MDEF2:
            WFIFOL (fd, 4) = sd->mdef2;
            break;
        case SP_CRITICAL:
            WFIFOL (fd, 4) = sd->critical / 10;
            break;
        case SP_MATK1:
            WFIFOL (fd, 4) = sd->matk1;
            break;
        case SP_MATK2:
            WFIFOL (fd, 4) = sd->matk2;
            break;

        case SP_ZENY:
            trade_verifyzeny (sd);
            WFIFOW (fd, 0) = 0xb1;
            if (sd->status.zeny < 0)
                sd->status.zeny = 0;
            WFIFOL (fd, 4) = sd->status.zeny;
            break;
        case SP_BASEEXP:
            WFIFOW (fd, 0) = 0xb1;
            WFIFOL (fd, 4) = sd->status.base_exp;
            break;
        case SP_JOBEXP:
            WFIFOW (fd, 0) = 0xb1;
            WFIFOL (fd, 4) = sd->status.job_exp;
            break;
        case SP_NEXTBASEEXP:
            WFIFOW (fd, 0) = 0xb1;
            WFIFOL (fd, 4) = pc_nextbaseexp (sd);
            break;
        case SP_NEXTJOBEXP:
            WFIFOW (fd, 0) = 0xb1;
            WFIFOL (fd, 4) = pc_nextjobexp (sd);
            break;

            // 00be 終了
        case SP_USTR:
        case SP_UAGI:
        case SP_UVIT:
        case SP_UINT:
        case SP_UDEX:
        case SP_ULUK:
            WFIFOW (fd, 0) = 0xbe;
            WFIFOB (fd, 4) =
                pc_need_status_point (sd, type - SP_USTR + SP_STR);
            len = 5;
            break;

            // 013a 終了
        case SP_ATTACKRANGE:
            WFIFOW (fd, 0) = 0x13a;
            WFIFOW (fd, 2) = (sd->attack_spell_override)
                ? sd->attack_spell_range : sd->attackrange;
            len = 4;
            break;

            // 0141 終了
        case SP_STR:
            WFIFOW (fd, 0) = 0x141;
            WFIFOL (fd, 2) = type;
            WFIFOL (fd, 6) = sd->status.str;
            WFIFOL (fd, 10) = sd->paramb[0] + sd->parame[0];
            len = 14;
            break;
        case SP_AGI:
            WFIFOW (fd, 0) = 0x141;
            WFIFOL (fd, 2) = type;
            WFIFOL (fd, 6) = sd->status.agi;
            WFIFOL (fd, 10) = sd->paramb[1] + sd->parame[1];
            len = 14;
            break;
        case SP_VIT:
            WFIFOW (fd, 0) = 0x141;
            WFIFOL (fd, 2) = type;
            WFIFOL (fd, 6) = sd->status.vit;
            WFIFOL (fd, 10) = sd->paramb[2] + sd->parame[2];
            len = 14;
            break;
        case SP_INT:
            WFIFOW (fd, 0) = 0x141;
            WFIFOL (fd, 2) = type;
            WFIFOL (fd, 6) = sd->status.int_;
            WFIFOL (fd, 10) = sd->paramb[3] + sd->parame[3];
            len = 14;
            break;
        case SP_DEX:
            WFIFOW (fd, 0) = 0x141;
            WFIFOL (fd, 2) = type;
            WFIFOL (fd, 6) = sd->status.dex;
            WFIFOL (fd, 10) = sd->paramb[4] + sd->parame[4];
            len = 14;
            break;
        case SP_LUK:
            WFIFOW (fd, 0) = 0x141;
            WFIFOL (fd, 2) = type;
            WFIFOL (fd, 6) = sd->status.luk;
            WFIFOL (fd, 10) = sd->paramb[5] + sd->parame[5];
            len = 14;
            break;

        case SP_CARTINFO:
            WFIFOW (fd, 0) = 0x121;
            WFIFOW (fd, 2) = sd->cart_num;
            WFIFOW (fd, 4) = sd->cart_max_num;
            WFIFOL (fd, 6) = sd->cart_weight;
            WFIFOL (fd, 10) = sd->cart_max_weight;
            len = 14;
            break;

        case SP_GM:
            WFIFOL (fd, 4) = pc_isGM (sd);
            break;

        default:
            if (battle_config.error_log)
                printf ("clif_updatestatus : make %d routine\n", type);
            return 1;
    }
    WFIFOSET (fd, len);

    return 0;
}

int clif_changestatus (struct block_list *bl, int type, int val)
{
    unsigned char buf[12];
    struct map_session_data *sd = NULL;

    nullpo_retr (0, bl);

    if (bl->type == BL_PC)
        sd = (struct map_session_data *) bl;

//printf("clif_changestatus id:%d type:%d val:%d\n",bl->id,type,val);
    if (sd)
    {
        WBUFW (buf, 0) = 0x1ab;
        WBUFL (buf, 2) = bl->id;
        WBUFW (buf, 6) = type;
        switch (type)
        {
            case SP_MANNER:
                WBUFL (buf, 8) = val;
                break;
            default:
                if (battle_config.error_log)
                    printf ("clif_changestatus : make %d routine\n", type);
                return 1;
        }
        clif_send (buf, packet_len_table[0x1ab], bl, AREA_WOS);
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook (struct block_list *bl, int type, int val)
{
    return clif_changelook_towards (bl, type, val, NULL);
}

int clif_changelook_towards (struct block_list *bl, int type, int val,
                             struct map_session_data *dstsd)
{
    unsigned char rbuf[32];
    unsigned char *buf = dstsd ? WFIFOP (dstsd->fd, 0) : rbuf;  // pick target buffer or general-purpose one
    struct map_session_data *sd = NULL;

    nullpo_retr (0, bl);

    if (bl->type == BL_PC)
        sd = (struct map_session_data *) bl;

    if (sd && sd->disguise > 23 && sd->disguise < 4001) // mob disguises [Valaris]
        return 0;

    if (sd && sd->status.option & OPTION_INVISIBILITY)
        return 0;

    if (sd
        && (type == LOOK_WEAPON || type == LOOK_SHIELD || type >= LOOK_SHOES))
    {
        WBUFW (buf, 0) = 0x1d7;
        WBUFL (buf, 2) = bl->id;
        if (type >= LOOK_SHOES)
        {
            int  equip_point = equip_points[type];

            WBUFB (buf, 6) = type;
            if (sd->equip_index[equip_point] >= 0
                && sd->inventory_data[sd->equip_index[2]])
            {
                if (sd->
                    inventory_data[sd->equip_index[equip_point]]->view_id > 0)
                    WBUFW (buf, 7) =
                        sd->inventory_data[sd->
                                           equip_index[equip_point]]->view_id;
                else
                    WBUFW (buf, 7) =
                        sd->status.inventory[sd->
                                             equip_index[equip_point]].nameid;
            }
            else
                WBUFW (buf, 7) = 0;
            WBUFW (buf, 9) = 0;
        }
        else
        {
            WBUFB (buf, 6) = 2;
            if (sd->attack_spell_override)
                WBUFW (buf, 7) = sd->attack_spell_look_override;
            else
            {
                if (sd->equip_index[9] >= 0
                    && sd->inventory_data[sd->equip_index[9]]
                    && sd->view_class != 22)
                {
                    if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
                        WBUFW (buf, 7) =
                            sd->inventory_data[sd->equip_index[9]]->view_id;
                    else
                        WBUFW (buf, 7) =
                            sd->status.inventory[sd->equip_index[9]].nameid;
                }
                else
                    WBUFW (buf, 7) = 0;
            }
            if (sd->equip_index[8] >= 0
                && sd->equip_index[8] != sd->equip_index[9]
                && sd->inventory_data[sd->equip_index[8]]
                && sd->view_class != 22)
            {
                if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
                    WBUFW (buf, 9) =
                        sd->inventory_data[sd->equip_index[8]]->view_id;
                else
                    WBUFW (buf, 9) =
                        sd->status.inventory[sd->equip_index[8]].nameid;
            }
            else
                WBUFW (buf, 9) = 0;
        }
        if (dstsd)
            WFIFOSET (dstsd->fd, packet_len_table[0x1d7]);
        else
            clif_send (buf, packet_len_table[0x1d7], bl, AREA);
    }
    else
    {
        WBUFW (buf, 0) = 0x1d7;
        WBUFL (buf, 2) = bl->id;
        WBUFB (buf, 6) = type;
        WBUFW (buf, 7) = val;
        WBUFW (buf, 9) = 0;
        if (dstsd)
            WFIFOSET (dstsd->fd, packet_len_table[0x1d7]);
        else
            clif_send (buf, packet_len_table[0x1d7], bl, AREA);
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_initialstatus (struct map_session_data *sd)
{
    int  fd;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);

    WBUFW (buf, 0) = 0xbd;
    WBUFW (buf, 2) = sd->status.status_point;
    WBUFB (buf, 4) = (sd->status.str > 255) ? 255 : sd->status.str;
    WBUFB (buf, 5) = pc_need_status_point (sd, SP_STR);
    WBUFB (buf, 6) = (sd->status.agi > 255) ? 255 : sd->status.agi;
    WBUFB (buf, 7) = pc_need_status_point (sd, SP_AGI);
    WBUFB (buf, 8) = (sd->status.vit > 255) ? 255 : sd->status.vit;
    WBUFB (buf, 9) = pc_need_status_point (sd, SP_VIT);
    WBUFB (buf, 10) = (sd->status.int_ > 255) ? 255 : sd->status.int_;
    WBUFB (buf, 11) = pc_need_status_point (sd, SP_INT);
    WBUFB (buf, 12) = (sd->status.dex > 255) ? 255 : sd->status.dex;
    WBUFB (buf, 13) = pc_need_status_point (sd, SP_DEX);
    WBUFB (buf, 14) = (sd->status.luk > 255) ? 255 : sd->status.luk;
    WBUFB (buf, 15) = pc_need_status_point (sd, SP_LUK);

    WBUFW (buf, 16) = sd->base_atk + sd->watk;
    WBUFW (buf, 18) = sd->watk2;    //atk bonus
    WBUFW (buf, 20) = sd->matk1;
    WBUFW (buf, 22) = sd->matk2;
    WBUFW (buf, 24) = sd->def;  // def
    WBUFW (buf, 26) = sd->def2;
    WBUFW (buf, 28) = sd->mdef; // mdef
    WBUFW (buf, 30) = sd->mdef2;
    WBUFW (buf, 32) = sd->hit;
    WBUFW (buf, 34) = sd->flee;
    WBUFW (buf, 36) = sd->flee2 / 10;
    WBUFW (buf, 38) = sd->critical / 10;
    WBUFW (buf, 40) = sd->status.karma;
    WBUFW (buf, 42) = sd->status.manner;

    WFIFOSET (fd, packet_len_table[0xbd]);

    clif_updatestatus (sd, SP_STR);
    clif_updatestatus (sd, SP_AGI);
    clif_updatestatus (sd, SP_VIT);
    clif_updatestatus (sd, SP_INT);
    clif_updatestatus (sd, SP_DEX);
    clif_updatestatus (sd, SP_LUK);

    clif_updatestatus (sd, SP_ATTACKRANGE);
    clif_updatestatus (sd, SP_ASPD);

    return 0;
}

/*==========================================
 *矢装備
 *------------------------------------------
 */
int clif_arrowequip (struct map_session_data *sd, int val)
{
    int  fd;

    nullpo_retr (0, sd);

    if (sd->attacktarget && sd->attacktarget > 0)   // [Valaris]
        sd->attacktarget = 0;

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x013c;
    WFIFOW (fd, 2) = val + 2;   //矢のアイテムID

    WFIFOSET (fd, packet_len_table[0x013c]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_arrow_fail (struct map_session_data *sd, int type)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x013b;
    WFIFOW (fd, 2) = type;

    WFIFOSET (fd, packet_len_table[0x013b]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack (struct map_session_data *sd, int type, int ok, int val)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xbc;
    WFIFOW (fd, 2) = type;
    WFIFOB (fd, 4) = ok;
    WFIFOB (fd, 5) = val;
    WFIFOSET (fd, packet_len_table[0xbc]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack (struct map_session_data *sd, int n, int pos, int ok)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xaa;
    WFIFOW (fd, 2) = n + 2;
    WFIFOW (fd, 4) = pos;
    WFIFOB (fd, 6) = ok;
    WFIFOSET (fd, packet_len_table[0xaa]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack (struct map_session_data *sd, int n, int pos, int ok)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xac;
    WFIFOW (fd, 2) = n + 2;
    WFIFOW (fd, 4) = pos;
    WFIFOB (fd, 6) = ok;
    WFIFOSET (fd, packet_len_table[0xac]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect (struct block_list *bl, int type)
{
    char buf[32];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x19b;
    WBUFL (buf, 2) = bl->id;
    WBUFL (buf, 6) = type;

    clif_send (buf, packet_len_table[0x19b], bl, AREA);

    return 0;
}

/*==========================================
 * 表示オプション変更
 *------------------------------------------
 */
int clif_changeoption (struct block_list *bl)
{
    char buf[32];
    short option;
    struct status_change *sc_data;
    static const int omask[] = { 0x10, 0x20 };
    static const int scnum[] = { SC_FALCON, SC_RIDING };
    int  i;

    nullpo_retr (0, bl);

    option = *battle_get_option (bl);
    sc_data = battle_get_sc_data (bl);

    WBUFW (buf, 0) = 0x119;
    WBUFL (buf, 2) = bl->id;
    WBUFW (buf, 6) = *battle_get_opt1 (bl);
    WBUFW (buf, 8) = *battle_get_opt2 (bl);
    WBUFW (buf, 10) = option;
    WBUFB (buf, 12) = 0;        // ??

    if (bl->type == BL_PC)
    {                           // disguises [Valaris]
        struct map_session_data *sd = ((struct map_session_data *) bl);
        if (sd && sd->disguise > 23 && sd->disguise < 4001)
        {
            clif_send (buf, packet_len_table[0x119], bl, AREA_WOS);
            clif_spawnpc (sd);
        }
        else
            clif_send (buf, packet_len_table[0x119], bl, AREA);
    }
    else
        clif_send (buf, packet_len_table[0x119], bl, AREA);

    // アイコンの表示
    for (i = 0; i < sizeof (omask) / sizeof (omask[0]); i++)
    {
        if (option & omask[i])
        {
            if (sc_data[scnum[i]].timer == -1)
                skill_status_change_start (bl, scnum[i], 0, 0, 0, 0, 0, 0);
        }
        else
        {
            skill_status_change_end (bl, scnum[i], -1);
        }
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack (struct map_session_data *sd, int index, int amount,
                     int ok)
{
    nullpo_retr (0, sd);

    if (!ok)
    {
        int  fd = sd->fd;
        WFIFOW (fd, 0) = 0xa8;
        WFIFOW (fd, 2) = index + 2;
        WFIFOW (fd, 4) = amount;
        WFIFOB (fd, 6) = ok;
        WFIFOSET (fd, packet_len_table[0xa8]);
    }
    else
    {
        char buf[32];

        WBUFW (buf, 0) = 0x1c8;
        WBUFW (buf, 2) = index + 2;
        if (sd->inventory_data[index]
            && sd->inventory_data[index]->view_id > 0)
            WBUFW (buf, 4) = sd->inventory_data[index]->view_id;
        else
            WBUFW (buf, 4) = sd->status.inventory[index].nameid;
        WBUFL (buf, 6) = sd->bl.id;
        WBUFW (buf, 10) = amount;
        WBUFB (buf, 12) = ok;
        clif_send (buf, packet_len_table[0x1c8], &sd->bl, SELF);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_createchat (struct map_session_data *sd, int fail)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xd6;
    WFIFOB (fd, 2) = fail;
    WFIFOSET (fd, packet_len_table[0xd6]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dispchat (struct chat_data *cd, int fd)
{
    char buf[128];              // 最大title(60バイト)+17

    if (cd == NULL || *cd->owner == NULL)
        return 1;

    WBUFW (buf, 0) = 0xd7;
    WBUFW (buf, 2) = strlen (cd->title) + 17;
    WBUFL (buf, 4) = (*cd->owner)->id;
    WBUFL (buf, 8) = cd->bl.id;
    WBUFW (buf, 12) = cd->limit;
    WBUFW (buf, 14) = cd->users;
    WBUFB (buf, 16) = cd->pub;
    strcpy (WBUFP (buf, 17), cd->title);
    if (fd)
    {
        memcpy (WFIFOP (fd, 0), buf, WBUFW (buf, 2));
        WFIFOSET (fd, WBUFW (buf, 2));
    }
    else
    {
        clif_send (buf, WBUFW (buf, 2), *cd->owner, AREA_WOSC);
    }

    return 0;
}

/*==========================================
 * chatの状態変更成功
 * 外部の人用と命令コード(d7->df)が違うだけ
 *------------------------------------------
 */
int clif_changechatstatus (struct chat_data *cd)
{
    char buf[128];              // 最大title(60バイト)+17

    if (cd == NULL || cd->usersd[0] == NULL)
        return 1;

    WBUFW (buf, 0) = 0xdf;
    WBUFW (buf, 2) = strlen (cd->title) + 17;
    WBUFL (buf, 4) = cd->usersd[0]->bl.id;
    WBUFL (buf, 8) = cd->bl.id;
    WBUFW (buf, 12) = cd->limit;
    WBUFW (buf, 14) = cd->users;
    WBUFB (buf, 16) = cd->pub;
    strcpy (WBUFP (buf, 17), cd->title);
    clif_send (buf, WBUFW (buf, 2), &cd->usersd[0]->bl, CHAT);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchat (struct chat_data *cd, int fd)
{
    char buf[32];

    nullpo_retr (0, cd);

    WBUFW (buf, 0) = 0xd8;
    WBUFL (buf, 2) = cd->bl.id;
    if (fd)
    {
        memcpy (WFIFOP (fd, 0), buf, packet_len_table[0xd8]);
        WFIFOSET (fd, packet_len_table[0xd8]);
    }
    else
    {
        clif_send (buf, packet_len_table[0xd8], *cd->owner, AREA_WOSC);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatfail (struct map_session_data *sd, int fail)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0xda;
    WFIFOB (fd, 2) = fail;
    WFIFOSET (fd, packet_len_table[0xda]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatok (struct map_session_data *sd, struct chat_data *cd)
{
    int  fd;
    int  i;

    nullpo_retr (0, sd);
    nullpo_retr (0, cd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xdb;
    WFIFOW (fd, 2) = 8 + (28 * cd->users);
    WFIFOL (fd, 4) = cd->bl.id;
    for (i = 0; i < cd->users; i++)
    {
        WFIFOL (fd, 8 + i * 28) = (i != 0) || ((*cd->owner)->type == BL_NPC);
        memcpy (WFIFOP (fd, 8 + i * 28 + 4), cd->usersd[i]->status.name, 24);
    }
    WFIFOSET (fd, WFIFOW (fd, 2));

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_addchat (struct chat_data *cd, struct map_session_data *sd)
{
    char buf[32];

    nullpo_retr (0, sd);
    nullpo_retr (0, cd);

    WBUFW (buf, 0) = 0x0dc;
    WBUFW (buf, 2) = cd->users;
    memcpy (WBUFP (buf, 4), sd->status.name, 24);
    clif_send (buf, packet_len_table[0xdc], &sd->bl, CHAT_WOS);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changechatowner (struct chat_data *cd, struct map_session_data *sd)
{
    char buf[64];

    nullpo_retr (0, sd);
    nullpo_retr (0, cd);

    WBUFW (buf, 0) = 0xe1;
    WBUFL (buf, 2) = 1;
    memcpy (WBUFP (buf, 6), cd->usersd[0]->status.name, 24);
    WBUFW (buf, 30) = 0xe1;
    WBUFL (buf, 32) = 0;
    memcpy (WBUFP (buf, 36), sd->status.name, 24);

    clif_send (buf, packet_len_table[0xe1] * 2, &sd->bl, CHAT);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_leavechat (struct chat_data *cd, struct map_session_data *sd)
{
    char buf[32];

    nullpo_retr (0, sd);
    nullpo_retr (0, cd);

    WBUFW (buf, 0) = 0xdd;
    WBUFW (buf, 2) = cd->users - 1;
    memcpy (WBUFP (buf, 4), sd->status.name, 24);
    WBUFB (buf, 28) = 0;

    clif_send (buf, packet_len_table[0xdd], &sd->bl, CHAT);

    return 0;
}

/*==========================================
 * 取り引き要請受け
 *------------------------------------------
 */
int clif_traderequest (struct map_session_data *sd, char *name)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xe5;
    strcpy (WFIFOP (fd, 2), name);
    WFIFOSET (fd, packet_len_table[0xe5]);

    return 0;
}

/*==========================================
 * 取り引き要求応答
 *------------------------------------------
 */
int clif_tradestart (struct map_session_data *sd, int type)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xe7;
    WFIFOB (fd, 2) = type;
    WFIFOSET (fd, packet_len_table[0xe7]);

    return 0;
}

/*==========================================
 * 相手方からのアイテム追加
 *------------------------------------------
 */
int clif_tradeadditem (struct map_session_data *sd,
                       struct map_session_data *tsd, int index, int amount)
{
    int  fd, j;

    nullpo_retr (0, sd);
    nullpo_retr (0, tsd);

    fd = tsd->fd;
    WFIFOW (fd, 0) = 0xe9;
    WFIFOL (fd, 2) = amount;
    if (index == 0)
    {
        WFIFOW (fd, 6) = 0;     // type id
        WFIFOB (fd, 8) = 0;     //identify flag
        WFIFOB (fd, 9) = 0;     // attribute
        WFIFOB (fd, 10) = 0;    //refine
        WFIFOW (fd, 11) = 0;    //card (4w)
        WFIFOW (fd, 13) = 0;    //card (4w)
        WFIFOW (fd, 15) = 0;    //card (4w)
        WFIFOW (fd, 17) = 0;    //card (4w)
    }
    else
    {
        index -= 2;
        if (sd->inventory_data[index]
            && sd->inventory_data[index]->view_id > 0)
            WFIFOW (fd, 6) = sd->inventory_data[index]->view_id;
        else
            WFIFOW (fd, 6) = sd->status.inventory[index].nameid;    // type id
        WFIFOB (fd, 8) = sd->status.inventory[index].identify;  //identify flag
        if (sd->status.inventory[index].broken == 1)
            WFIFOB (fd, 9) = 1; // is broke weapon [Valaris]
        else
            WFIFOB (fd, 9) = sd->status.inventory[index].attribute; // attribute
        WFIFOB (fd, 10) = sd->status.inventory[index].refine;   //refine
        if (sd->status.inventory[index].card[0] == 0x00ff
            || sd->status.inventory[index].card[0] == 0x00fe
            || sd->status.inventory[index].card[0] == (short) 0xff00)
        {
            WFIFOW (fd, 11) = sd->status.inventory[index].card[0];  //card (4w)
            WFIFOW (fd, 13) = sd->status.inventory[index].card[1];  //card (4w)
            WFIFOW (fd, 15) = sd->status.inventory[index].card[2];  //card (4w)
            WFIFOW (fd, 17) = sd->status.inventory[index].card[3];  //card (4w)
        }
        else
        {
            if (sd->status.inventory[index].card[0] > 0
                && (j =
                    itemdb_viewid (sd->status.inventory[index].card[0])) > 0)
                WFIFOW (fd, 11) = j;
            else
                WFIFOW (fd, 11) = sd->status.inventory[index].card[0];
            if (sd->status.inventory[index].card[1] > 0
                && (j =
                    itemdb_viewid (sd->status.inventory[index].card[1])) > 0)
                WFIFOW (fd, 13) = j;
            else
                WFIFOW (fd, 13) = sd->status.inventory[index].card[1];
            if (sd->status.inventory[index].card[2] > 0
                && (j =
                    itemdb_viewid (sd->status.inventory[index].card[2])) > 0)
                WFIFOW (fd, 15) = j;
            else
                WFIFOW (fd, 15) = sd->status.inventory[index].card[2];
            if (sd->status.inventory[index].card[3] > 0
                && (j =
                    itemdb_viewid (sd->status.inventory[index].card[3])) > 0)
                WFIFOW (fd, 17) = j;
            else
                WFIFOW (fd, 17) = sd->status.inventory[index].card[3];
        }
    }
    WFIFOSET (fd, packet_len_table[0xe9]);

    return 0;
}

/*==========================================
 * アイテム追加成功/失敗
 *------------------------------------------
 */
int clif_tradeitemok (struct map_session_data *sd, int index, int amount,
                      int fail)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x1b1;
    //WFIFOW(fd,0)=0xea;
    WFIFOW (fd, 2) = index;
    WFIFOW (fd, 4) = amount;
    WFIFOB (fd, 6) = fail;
    WFIFOSET (fd, packet_len_table[0x1b1]);

    return 0;
}

/*==========================================
 * 取り引きok押し
 *------------------------------------------
 */
int clif_tradedeal_lock (struct map_session_data *sd, int fail)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xec;
    WFIFOB (fd, 2) = fail;      // 0=you 1=the other person
    WFIFOSET (fd, packet_len_table[0xec]);

    return 0;
}

/*==========================================
 * 取り引きがキャンセルされました
 *------------------------------------------
 */
int clif_tradecancelled (struct map_session_data *sd)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xee;
    WFIFOSET (fd, packet_len_table[0xee]);

    return 0;
}

/*==========================================
 * 取り引き完了
 *------------------------------------------
 */
int clif_tradecompleted (struct map_session_data *sd, int fail)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf0;
    WFIFOB (fd, 2) = fail;
    WFIFOSET (fd, packet_len_table[0xf0]);

    return 0;
}

/*==========================================
 * カプラ倉庫のアイテム数を更新
 *------------------------------------------
 */
int clif_updatestorageamount (struct map_session_data *sd,
                              struct storage *stor)
{
    int  fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf2;      // update storage amount
    WFIFOW (fd, 2) = stor->storage_amount;  //items
    WFIFOW (fd, 4) = MAX_STORAGE;   //items max
    WFIFOSET (fd, packet_len_table[0xf2]);

    return 0;
}

/*==========================================
 * カプラ倉庫にアイテムを追加する
 *------------------------------------------
 */
int clif_storageitemadded (struct map_session_data *sd, struct storage *stor,
                           int index, int amount)
{
    int  fd, j;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf4;      // Storage item added
    WFIFOW (fd, 2) = index + 1; // index
    WFIFOL (fd, 4) = amount;    // amount
/*	if((view = itemdb_viewid(stor->storage_[index].nameid)) > 0)
		WFIFOW(fd,8) =view;
	else*/
    WFIFOW (fd, 8) = stor->storage_[index].nameid;
    WFIFOB (fd, 10) = stor->storage_[index].identify;   //identify flag
    if (stor->storage_[index].broken == 1)
        WFIFOB (fd, 11) = 1;    // is weapon broken [Valaris]
    else
        WFIFOB (fd, 11) = stor->storage_[index].attribute;  // attribute
    WFIFOB (fd, 12) = stor->storage_[index].refine; //refine
    if (stor->storage_[index].card[0] == 0x00ff
        || stor->storage_[index].card[0] == 0x00fe
        || stor->storage_[index].card[0] == (short) 0xff00)
    {
        WFIFOW (fd, 13) = stor->storage_[index].card[0];    //card (4w)
        WFIFOW (fd, 15) = stor->storage_[index].card[1];    //card (4w)
        WFIFOW (fd, 17) = stor->storage_[index].card[2];    //card (4w)
        WFIFOW (fd, 19) = stor->storage_[index].card[3];    //card (4w)
    }
    else
    {
        if (stor->storage_[index].card[0] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[0])) > 0)
            WFIFOW (fd, 13) = j;
        else
            WFIFOW (fd, 13) = stor->storage_[index].card[0];
        if (stor->storage_[index].card[1] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[1])) > 0)
            WFIFOW (fd, 15) = j;
        else
            WFIFOW (fd, 15) = stor->storage_[index].card[1];
        if (stor->storage_[index].card[2] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[2])) > 0)
            WFIFOW (fd, 17) = j;
        else
            WFIFOW (fd, 17) = stor->storage_[index].card[2];
        if (stor->storage_[index].card[3] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[3])) > 0)
            WFIFOW (fd, 19) = j;
        else
            WFIFOW (fd, 19) = stor->storage_[index].card[3];
    }
    WFIFOSET (fd, packet_len_table[0xf4]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_updateguildstorageamount (struct map_session_data *sd,
                                   struct guild_storage *stor)
{
    int  fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf2;      // update storage amount
    WFIFOW (fd, 2) = stor->storage_amount;  //items
    WFIFOW (fd, 4) = MAX_GUILD_STORAGE; //items max
    WFIFOSET (fd, packet_len_table[0xf2]);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageitemadded (struct map_session_data *sd,
                                struct guild_storage *stor, int index,
                                int amount)
{
    int  view, fd, j;

    nullpo_retr (0, sd);
    nullpo_retr (0, stor);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf4;      // Storage item added
    WFIFOW (fd, 2) = index + 1; // index
    WFIFOL (fd, 4) = amount;    // amount
    if ((view = itemdb_viewid (stor->storage_[index].nameid)) > 0)
        WFIFOW (fd, 8) = view;
    else
        WFIFOW (fd, 8) = stor->storage_[index].nameid;  // id
    WFIFOB (fd, 10) = stor->storage_[index].identify;   //identify flag
    if (stor->storage_[index].broken == 1)
        WFIFOB (fd, 11) = 1;    // is weapon broken [Valaris]
    else
        WFIFOB (fd, 11) = stor->storage_[index].attribute;  // attribute
    WFIFOB (fd, 12) = stor->storage_[index].refine; //refine
    if (stor->storage_[index].card[0] == 0x00ff
        || stor->storage_[index].card[0] == 0x00fe
        || stor->storage_[index].card[0] == (short) 0xff00)
    {
        WFIFOW (fd, 13) = stor->storage_[index].card[0];    //card (4w)
        WFIFOW (fd, 15) = stor->storage_[index].card[1];    //card (4w)
        WFIFOW (fd, 17) = stor->storage_[index].card[2];    //card (4w)
        WFIFOW (fd, 19) = stor->storage_[index].card[3];    //card (4w)
    }
    else
    {
        if (stor->storage_[index].card[0] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[0])) > 0)
            WFIFOW (fd, 13) = j;
        else
            WFIFOW (fd, 13) = stor->storage_[index].card[0];
        if (stor->storage_[index].card[1] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[1])) > 0)
            WFIFOW (fd, 15) = j;
        else
            WFIFOW (fd, 15) = stor->storage_[index].card[1];
        if (stor->storage_[index].card[2] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[2])) > 0)
            WFIFOW (fd, 17) = j;
        else
            WFIFOW (fd, 17) = stor->storage_[index].card[2];
        if (stor->storage_[index].card[3] > 0
            && (j = itemdb_viewid (stor->storage_[index].card[3])) > 0)
            WFIFOW (fd, 19) = j;
        else
            WFIFOW (fd, 19) = stor->storage_[index].card[3];
    }
    WFIFOSET (fd, packet_len_table[0xf4]);

    return 0;
}

/*==========================================
 * カプラ倉庫からアイテムを取り去る
 *------------------------------------------
 */
int clif_storageitemremoved (struct map_session_data *sd, int index,
                             int amount)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf6;      // Storage item removed
    WFIFOW (fd, 2) = index + 1;
    WFIFOL (fd, 4) = amount;
    WFIFOSET (fd, packet_len_table[0xf6]);

    return 0;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
int clif_storageclose (struct map_session_data *sd)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xf8;      // Storage Closed
    WFIFOSET (fd, packet_len_table[0xf8]);

    return 0;
}

void
clif_changelook_accessories (struct block_list *bl,
                             struct map_session_data *dest)
{
    int  i;

    for (i = LOOK_SHOES; i <= LOOK_LAST; i++)
        clif_changelook_towards (bl, i, 0, dest);
}

//
// callback系 ?
//
/*==========================================
 * PC表示
 *------------------------------------------
 */
void clif_getareachar_pc (struct map_session_data *sd,
                          struct map_session_data *dstsd)
{
    int  len;

    if (dstsd->status.option & OPTION_INVISIBILITY)
        return;

    nullpo_retv (sd);
    nullpo_retv (dstsd);

    if (dstsd->walktimer != -1)
    {
        len = clif_set007b (dstsd, WFIFOP (sd->fd, 0));
        WFIFOSET (sd->fd, len);
    }
    else
    {
        len = clif_set0078 (dstsd, WFIFOP (sd->fd, 0));
        WFIFOSET (sd->fd, len);
    }

    if (dstsd->chatID)
    {
        struct chat_data *cd;
        cd = (struct chat_data *) map_id2bl (dstsd->chatID);
        if (cd->usersd[0] == dstsd)
            clif_dispchat (cd, sd->fd);
    }
    if (dstsd->spiritball > 0)
    {
        clif_set01e1 (dstsd, WFIFOP (sd->fd, 0));
        WFIFOSET (sd->fd, packet_len_table[0x1e1]);
    }
    if (battle_config.save_clothcolor == 1 && dstsd->status.clothes_color > 0)
        clif_changelook (&dstsd->bl, LOOK_CLOTHES_COLOR,
                         dstsd->status.clothes_color);

    if (sd->status.manner < 0)
        clif_changestatus (&sd->bl, SP_MANNER, sd->status.manner);

    clif_changelook_accessories (&sd->bl, dstsd);
    clif_changelook_accessories (&dstsd->bl, sd);
}

/*==========================================
 * NPC表示
 *------------------------------------------
 */
void clif_getareachar_npc (struct map_session_data *sd, struct npc_data *nd)
{
    int  len;

    nullpo_retv (sd);
    nullpo_retv (nd);

    if (nd->class < 0 || nd->flag & 1 || nd->class == INVISIBLE_CLASS)
        return;

    len = clif_npc0078 (nd, WFIFOP (sd->fd, 0));
    WFIFOSET (sd->fd, len);

    if (nd->chat_id)
    {
        clif_dispchat ((struct chat_data *) map_id2bl (nd->chat_id), sd->fd);
    }

}

/*==========================================
 * 移動停止
 *------------------------------------------
 */
int clif_movemob (struct mob_data *md)
{
    unsigned char buf[256];
    int  len;

    nullpo_retr (0, md);

    len = clif_mob007b (md, buf);
    clif_send (buf, len, &md->bl, AREA);

    if (mob_get_equip (md->class) > 0)  // mob equipment [Valaris]
        clif_mob_equip (md, mob_get_equip (md->class));

    return 0;
}

/*==========================================
 * モンスターの位置修正
 *------------------------------------------
 */
int clif_fixmobpos (struct mob_data *md)
{
    unsigned char buf[256];
    int  len;

    nullpo_retr (0, md);

    if (md->state.state == MS_WALK)
    {
        len = clif_mob007b (md, buf);
        clif_send (buf, len, &md->bl, AREA);
    }
    else
    {
        len = clif_mob0078 (md, buf);
        clif_send (buf, len, &md->bl, AREA);
    }

    return 0;
}

/*==========================================
 * PCの位置修正
 *------------------------------------------
 */
int clif_fixpcpos (struct map_session_data *sd)
{
    unsigned char buf[256];
    int  len;

    nullpo_retr (0, sd);

    if (sd->walktimer != -1)
    {
        len = clif_set007b (sd, buf);
        clif_send (buf, len, &sd->bl, AREA);
    }
    else
    {
        len = clif_set0078 (sd, buf);
        clif_send (buf, len, &sd->bl, AREA);
    }
    clif_changelook_accessories (&sd->bl, NULL);

    return 0;
}

/*==========================================
 * 通常攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_damage (struct block_list *src, struct block_list *dst,
                 unsigned int tick, int sdelay, int ddelay, int damage,
                 int div, int type, int damage2)
{
    unsigned char buf[256];
    struct status_change *sc_data;

    nullpo_retr (0, src);
    nullpo_retr (0, dst);

    sc_data = battle_get_sc_data (dst);

    if (type != 4 && dst->type == BL_PC
        && ((struct map_session_data *) dst)->special_state.infinite_endure)
        type = 9;
    if (sc_data)
    {
        if (type != 4 && sc_data[SC_ENDURE].timer != -1)
            type = 9;
        if (sc_data[SC_HALLUCINATION].timer != -1)
        {
            if (damage > 0)
                damage =
                    damage * (5 + sc_data[SC_HALLUCINATION].val1) +
                    MRAND (100);
            if (damage2 > 0)
                damage2 =
                    damage2 * (5 + sc_data[SC_HALLUCINATION].val1) +
                    MRAND (100);
        }
    }

    WBUFW (buf, 0) = 0x8a;
    WBUFL (buf, 2) = src->id;
    WBUFL (buf, 6) = dst->id;
    WBUFL (buf, 10) = tick;
    WBUFL (buf, 14) = sdelay;
    WBUFL (buf, 18) = ddelay;
    WBUFW (buf, 22) = (damage > 0x7fff) ? 0x7fff : damage;
    WBUFW (buf, 24) = div;
    WBUFB (buf, 26) = type;
    WBUFW (buf, 27) = damage2;
    clif_send (buf, packet_len_table[0x8a], src, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_mob (struct map_session_data *sd, struct mob_data *md)
{
    int  len;
    nullpo_retv (sd);
    nullpo_retv (md);

    if (md->state.state == MS_WALK)
    {
        len = clif_mob007b (md, WFIFOP (sd->fd, 0));
        WFIFOSET (sd->fd, len);
    }
    else
    {
        len = clif_mob0078 (md, WFIFOP (sd->fd, 0));
        WFIFOSET (sd->fd, len);
    }

    if (mob_get_equip (md->class) > 0)  // mob equipment [Valaris]
        clif_mob_equip (md, mob_get_equip (md->class));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_item (struct map_session_data *sd,
                            struct flooritem_data *fitem)
{
    int  view, fd;

    nullpo_retv (sd);
    nullpo_retv (fitem);

    fd = sd->fd;
    //009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
    WFIFOW (fd, 0) = 0x9d;
    WFIFOL (fd, 2) = fitem->bl.id;
    if ((view = itemdb_viewid (fitem->item_data.nameid)) > 0)
        WFIFOW (fd, 6) = view;
    else
        WFIFOW (fd, 6) = fitem->item_data.nameid;
    WFIFOB (fd, 8) = fitem->item_data.identify;
    WFIFOW (fd, 9) = fitem->bl.x;
    WFIFOW (fd, 11) = fitem->bl.y;
    WFIFOW (fd, 13) = fitem->item_data.amount;
    WFIFOB (fd, 15) = fitem->subx;
    WFIFOB (fd, 16) = fitem->suby;

    WFIFOSET (fd, packet_len_table[0x9d]);
}

/*==========================================
 * 場所スキルエフェクトが視界に入る
 *------------------------------------------
 */
int clif_getareachar_skillunit (struct map_session_data *sd,
                                struct skill_unit *unit)
{
    int  fd;
    struct block_list *bl;

    nullpo_retr (0, unit);

    fd = sd->fd;
    bl = map_id2bl (unit->group->src_id);
    memset (WFIFOP (fd, 0), 0, packet_len_table[0x1c9]);
    WFIFOW (fd, 0) = 0x1c9;
    WFIFOL (fd, 2) = unit->bl.id;
    WFIFOL (fd, 6) = unit->group->src_id;
    WFIFOW (fd, 10) = unit->bl.x;
    WFIFOW (fd, 12) = unit->bl.y;
    WFIFOB (fd, 14) = unit->group->unit_id;
    WFIFOB (fd, 15) = 1;
    WFIFOL (fd, 15 + 1) = 0;    //1-4調べた限り固定
    WFIFOL (fd, 15 + 5) = 0;    //5-8調べた限り固定
    //9-12マップごとで一定の77-80とはまた違う4バイトのかなり大きな数字
    WFIFOL (fd, 15 + 13) = unit->bl.y - 0x12;   //13-16ユニットのY座標-18っぽい(Y:17でFF FF FF FF)
    WFIFOL (fd, 15 + 17) = 0x004f37dd;  //17-20調べた限り固定
    WFIFOL (fd, 15 + 21) = 0x0012f674;  //21-24調べた限り固定
    WFIFOL (fd, 15 + 25) = 0x0012f664;  //25-28調べた限り固定
    WFIFOL (fd, 15 + 29) = 0x0012f654;  //29-32調べた限り固定
    WFIFOL (fd, 15 + 33) = 0x77527bbc;  //33-36調べた限り固定
    //37-39
    WFIFOB (fd, 15 + 40) = 0x2d;    //40調べた限り固定
    WFIFOL (fd, 15 + 41) = 0;   //41-44調べた限り0固定
    WFIFOL (fd, 15 + 45) = 0;   //45-48調べた限り0固定
    WFIFOL (fd, 15 + 49) = 0;   //49-52調べた限り0固定
    WFIFOL (fd, 15 + 53) = 0x0048d919;  //53-56調べた限り固定
    WFIFOL (fd, 15 + 57) = 0x0000003e;  //57-60調べた限り固定
    WFIFOL (fd, 15 + 61) = 0x0012f66c;  //61-64調べた限り固定
    //65-68
    //69-72
    if (bl)
        WFIFOL (fd, 15 + 73) = bl->y;   //73-76術者のY座標
    WFIFOL (fd, 15 + 77) = unit->bl.m;  //77-80マップIDかなぁ？かなり2バイトで足りそうな数字
    WFIFOB (fd, 15 + 81) = 0xaa;    //81終端文字0xaa

    /*  Graffiti [Valaris]  */
    if (unit->group->unit_id == 0xb0)
    {
        WFIFOL (fd, 15) = 1;
        WFIFOL (fd, 16) = 1;
        memcpy (WFIFOP (fd, 17), unit->group->valstr, 80);
    }

    WFIFOSET (fd, packet_len_table[0x1c9]);
    if (unit->group->skill_id == WZ_ICEWALL)
        clif_set0192 (fd, unit->bl.m, unit->bl.x, unit->bl.y, 5);

    return 0;
}

/*==========================================
 * 場所スキルエフェクトが視界から消える
 *------------------------------------------
 */
int clif_clearchar_skillunit (struct skill_unit *unit, int fd)
{
    nullpo_retr (0, unit);

    WFIFOW (fd, 0) = 0x120;
    WFIFOL (fd, 2) = unit->bl.id;
    WFIFOSET (fd, packet_len_table[0x120]);
    if (unit->group->skill_id == WZ_ICEWALL)
        clif_set0192 (fd, unit->bl.m, unit->bl.x, unit->bl.y, unit->val2);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_01ac (struct block_list *bl)
{
    char buf[32];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x1ac;
    WBUFL (buf, 2) = bl->id;

    clif_send (buf, packet_len_table[0x1ac], bl, AREA);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_getareachar (struct block_list *bl, va_list ap)
{
    struct map_session_data *sd;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);

    sd = va_arg (ap, struct map_session_data *);

    switch (bl->type)
    {
        case BL_PC:
            if (sd == (struct map_session_data *) bl)
                break;
            clif_getareachar_pc (sd, (struct map_session_data *) bl);
            break;
        case BL_NPC:
            clif_getareachar_npc (sd, (struct npc_data *) bl);
            break;
        case BL_MOB:
            clif_getareachar_mob (sd, (struct mob_data *) bl);
            break;
        case BL_ITEM:
            clif_getareachar_item (sd, (struct flooritem_data *) bl);
            break;
        case BL_SKILL:
            clif_getareachar_skillunit (sd, (struct skill_unit *) bl);
            break;
        default:
            if (battle_config.error_log)
                printf ("get area char ??? %d\n", bl->type);
            break;
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcoutsight (struct block_list *bl, va_list ap)
{
    struct map_session_data *sd, *dstsd;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);
    nullpo_retr (0, sd = va_arg (ap, struct map_session_data *));

    switch (bl->type)
    {
        case BL_PC:
            dstsd = (struct map_session_data *) bl;
            if (sd != dstsd)
            {
                clif_clearchar_id (dstsd->bl.id, 0, sd->fd);
                clif_clearchar_id (sd->bl.id, 0, dstsd->fd);
                if (dstsd->chatID)
                {
                    struct chat_data *cd;
                    cd = (struct chat_data *) map_id2bl (dstsd->chatID);
                    if (cd->usersd[0] == dstsd)
                        clif_dispchat (cd, sd->fd);
                }
            }
            break;
        case BL_NPC:
            if (((struct npc_data *) bl)->class != INVISIBLE_CLASS)
                clif_clearchar_id (bl->id, 0, sd->fd);
            break;
        case BL_MOB:
            clif_clearchar_id (bl->id, 0, sd->fd);
            break;
        case BL_ITEM:
            clif_clearflooritem ((struct flooritem_data *) bl, sd->fd);
            break;
        case BL_SKILL:
            clif_clearchar_skillunit ((struct skill_unit *) bl, sd->fd);
            break;
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcinsight (struct block_list *bl, va_list ap)
{
    struct map_session_data *sd, *dstsd;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);
    nullpo_retr (0, sd = va_arg (ap, struct map_session_data *));

    switch (bl->type)
    {
        case BL_PC:
            dstsd = (struct map_session_data *) bl;
            if (sd != dstsd)
            {
                clif_getareachar_pc (sd, dstsd);
                clif_getareachar_pc (dstsd, sd);
            }
            break;
        case BL_NPC:
            clif_getareachar_npc (sd, (struct npc_data *) bl);
            break;
        case BL_MOB:
            clif_getareachar_mob (sd, (struct mob_data *) bl);
            break;
        case BL_ITEM:
            clif_getareachar_item (sd, (struct flooritem_data *) bl);
            break;
        case BL_SKILL:
            clif_getareachar_skillunit (sd, (struct skill_unit *) bl);
            break;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_moboutsight (struct block_list *bl, va_list ap)
{
    struct map_session_data *sd;
    struct mob_data *md;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);
    nullpo_retr (0, md = va_arg (ap, struct mob_data *));

    if (bl->type == BL_PC && (sd = (struct map_session_data *) bl))
    {
        clif_clearchar_id (md->bl.id, 0, sd->fd);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mobinsight (struct block_list *bl, va_list ap)
{
    struct map_session_data *sd;
    struct mob_data *md;

    nullpo_retr (0, bl);
    nullpo_retr (0, ap);

    md = va_arg (ap, struct mob_data *);
    if (bl->type == BL_PC && (sd = (struct map_session_data *) bl))
    {
        clif_getareachar_mob (sd, md);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillinfo (struct map_session_data *sd, int skillid, int type,
                    int range)
{
    int  fd, id;

    nullpo_retr (0, sd);

    fd = sd->fd;
    if ((id = sd->status.skill[skillid].id) <= 0)
        return 0;
    WFIFOW (fd, 0) = 0x147;
    WFIFOW (fd, 2) = id;
    if (type < 0)
        WFIFOW (fd, 4) = skill_get_inf (id);
    else
        WFIFOW (fd, 4) = type;
    WFIFOW (fd, 6) = 0;
    WFIFOW (fd, 8) = sd->status.skill[skillid].lv;
    WFIFOW (fd, 10) = skill_get_sp (id, sd->status.skill[skillid].lv);
    if (range < 0)
    {
        range = skill_get_range (id, sd->status.skill[skillid].lv);
        if (range < 0)
            range = battle_get_range (&sd->bl) - (range + 1);
        WFIFOW (fd, 12) = range;
    }
    else
        WFIFOW (fd, 12) = range;
    memset (WFIFOP (fd, 14), 0, 24);
    WFIFOB (fd, 38) =
        (sd->status.skill[skillid].lv < skill_get_max_raise (id)) ? 1 : 0;
    WFIFOSET (fd, packet_len_table[0x147]);

    return 0;
}

/*==========================================
 * スキルリストを送信する
 *------------------------------------------
 */
int clif_skillinfoblock (struct map_session_data *sd)
{
    int  fd;
    int  i, c, len = 4, id, range;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x10f;
    for (i = c = 0; i < MAX_SKILL; i++)
    {
        if ((id = sd->status.skill[i].id) != 0 && (sd->tmw_version >= 1))
        {                       // [Fate] Version 1 and later don't crash because of bad skill IDs anymore
            WFIFOW (fd, len) = id;
            WFIFOW (fd, len + 2) = skill_get_inf (id);
            WFIFOW (fd, len + 4) =
                skill_db[i].poolflags | (sd->status.
                                         skill[i].flags &
                                         (SKILL_POOL_ACTIVATED));
            WFIFOW (fd, len + 6) = sd->status.skill[i].lv;
            WFIFOW (fd, len + 8) = skill_get_sp (id, sd->status.skill[i].lv);
            range = skill_get_range (id, sd->status.skill[i].lv);
            if (range < 0)
                range = battle_get_range (&sd->bl) - (range + 1);
            WFIFOW (fd, len + 10) = range;
            memset (WFIFOP (fd, len + 12), 0, 24);
            WFIFOB (fd, len + 36) =
                (sd->status.skill[i].lv < skill_get_max_raise (id)) ? 1 : 0;
            len += 37;
            c++;
        }
    }
    WFIFOW (fd, 2) = len;
    WFIFOSET (fd, len);

    return 0;
}

/*==========================================
 * スキル割り振り通知
 *------------------------------------------
 */
int clif_skillup (struct map_session_data *sd, int skill_num)
{
    int  range, fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x10e;
    WFIFOW (fd, 2) = skill_num;
    WFIFOW (fd, 4) = sd->status.skill[skill_num].lv;
    WFIFOW (fd, 6) = skill_get_sp (skill_num, sd->status.skill[skill_num].lv);
    range = skill_get_range (skill_num, sd->status.skill[skill_num].lv);
    if (range < 0)
        range = battle_get_range (&sd->bl) - (range + 1);
    WFIFOW (fd, 8) = range;
    WFIFOB (fd, 10) =
        (sd->status.skill[skill_num].lv <
         skill_get_max_raise (sd->status.skill[skill_num].id)) ? 1 : 0;
    WFIFOSET (fd, packet_len_table[0x10e]);

    return 0;
}

/*==========================================
 * スキル詠唱エフェクトを送信する
 *------------------------------------------
 */
int clif_skillcasting (struct block_list *bl,
                       int src_id, int dst_id, int dst_x, int dst_y,
                       int skill_num, int casttime)
{
    unsigned char buf[32];
    WBUFW (buf, 0) = 0x13e;
    WBUFL (buf, 2) = src_id;
    WBUFL (buf, 6) = dst_id;
    WBUFW (buf, 10) = dst_x;
    WBUFW (buf, 12) = dst_y;
    WBUFW (buf, 14) = skill_num;    //魔法詠唱スキル
    WBUFL (buf, 16) = skill_get_pl (skill_num); //属性
    WBUFL (buf, 20) = casttime; //skill詠唱時間
    clif_send (buf, packet_len_table[0x13e], bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel (struct block_list *bl)
{
    unsigned char buf[16];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x1b9;
    WBUFL (buf, 2) = bl->id;
    clif_send (buf, packet_len_table[0x1b9], bl, AREA);

    return 0;
}

/*==========================================
 * スキル詠唱失敗
 *------------------------------------------
 */
int clif_skill_fail (struct map_session_data *sd, int skill_id, int type,
                     int btype)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    if (type == 0x4 && battle_config.display_delay_skill_fail == 0)
    {
        return 0;
    }

    WFIFOW (fd, 0) = 0x110;
    WFIFOW (fd, 2) = skill_id;
    WFIFOW (fd, 4) = btype;
    WFIFOW (fd, 6) = 0;
    WFIFOB (fd, 8) = 0;
    WFIFOB (fd, 9) = type;
    WFIFOSET (fd, packet_len_table[0x110]);

    return 0;
}

/*==========================================
 * スキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage (struct block_list *src, struct block_list *dst,
                       unsigned int tick, int sdelay, int ddelay, int damage,
                       int div, int skill_id, int skill_lv, int type)
{
    unsigned char buf[64];
    struct status_change *sc_data;

    nullpo_retr (0, src);
    nullpo_retr (0, dst);

    sc_data = battle_get_sc_data (dst);

    if (type != 5 && dst->type == BL_PC
        && ((struct map_session_data *) dst)->special_state.infinite_endure)
        type = 9;
    if (sc_data)
    {
        if (type != 5 && sc_data[SC_ENDURE].timer != -1)
            type = 9;
        if (sc_data[SC_HALLUCINATION].timer != -1 && damage > 0)
            damage =
                damage * (5 + sc_data[SC_HALLUCINATION].val1) + MRAND (100);
    }

    WBUFW (buf, 0) = 0x1de;
    WBUFW (buf, 2) = skill_id;
    WBUFL (buf, 4) = src->id;
    WBUFL (buf, 8) = dst->id;
    WBUFL (buf, 12) = tick;
    WBUFL (buf, 16) = sdelay;
    WBUFL (buf, 20) = ddelay;
    WBUFL (buf, 24) = damage;
    WBUFW (buf, 28) = skill_lv;
    WBUFW (buf, 30) = div;
    WBUFB (buf, 32) = (type > 0) ? type : skill_get_hit (skill_id);
    clif_send (buf, packet_len_table[0x1de], src, AREA);

    return 0;
}

/*==========================================
 * 吹き飛ばしスキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage2 (struct block_list *src, struct block_list *dst,
                        unsigned int tick, int sdelay, int ddelay, int damage,
                        int div, int skill_id, int skill_lv, int type)
{
    unsigned char buf[64];
    struct status_change *sc_data;

    nullpo_retr (0, src);
    nullpo_retr (0, dst);

    sc_data = battle_get_sc_data (dst);

    if (type != 5 && dst->type == BL_PC
        && ((struct map_session_data *) dst)->special_state.infinite_endure)
        type = 9;
    if (sc_data)
    {
        if (type != 5 && sc_data[SC_ENDURE].timer != -1)
            type = 9;
        if (sc_data[SC_HALLUCINATION].timer != -1 && damage > 0)
            damage =
                damage * (5 + sc_data[SC_HALLUCINATION].val1) + MRAND (100);
    }

    WBUFW (buf, 0) = 0x115;
    WBUFW (buf, 2) = skill_id;
    WBUFL (buf, 4) = src->id;
    WBUFL (buf, 8) = dst->id;
    WBUFL (buf, 12) = tick;
    WBUFL (buf, 16) = sdelay;
    WBUFL (buf, 20) = ddelay;
    WBUFW (buf, 24) = dst->x;
    WBUFW (buf, 26) = dst->y;
    WBUFW (buf, 28) = damage;
    WBUFW (buf, 30) = skill_lv;
    WBUFW (buf, 32) = div;
    WBUFB (buf, 34) = (type > 0) ? type : skill_get_hit (skill_id);
    clif_send (buf, packet_len_table[0x115], src, AREA);

    return 0;
}

/*==========================================
 * 支援/回復スキルエフェクト
 *------------------------------------------
 */
int clif_skill_nodamage (struct block_list *src, struct block_list *dst,
                         int skill_id, int heal, int fail)
{
    unsigned char buf[32];

    nullpo_retr (0, src);
    nullpo_retr (0, dst);

    WBUFW (buf, 0) = 0x11a;
    WBUFW (buf, 2) = skill_id;
    WBUFW (buf, 4) = (heal > 0x7fff) ? 0x7fff : heal;
    WBUFL (buf, 6) = dst->id;
    WBUFL (buf, 10) = src->id;
    WBUFB (buf, 14) = fail;
    clif_send (buf, packet_len_table[0x11a], src, AREA);

    return 0;
}

/*==========================================
 * 場所スキルエフェクト
 *------------------------------------------
 */
int clif_skill_poseffect (struct block_list *src, int skill_id, int val,
                          int x, int y, int tick)
{
    unsigned char buf[32];

    nullpo_retr (0, src);

    WBUFW (buf, 0) = 0x117;
    WBUFW (buf, 2) = skill_id;
    WBUFL (buf, 4) = src->id;
    WBUFW (buf, 8) = val;
    WBUFW (buf, 10) = x;
    WBUFW (buf, 12) = y;
    WBUFL (buf, 14) = tick;
    clif_send (buf, packet_len_table[0x117], src, AREA);

    return 0;
}

/*==========================================
 * 場所スキルエフェクト表示
 *------------------------------------------
 */
int clif_skill_setunit (struct skill_unit *unit)
{
    unsigned char buf[128];
    struct block_list *bl;

    nullpo_retr (0, unit);

    bl = map_id2bl (unit->group->src_id);

    memset (WBUFP (buf, 0), 0, packet_len_table[0x1c9]);
    WBUFW (buf, 0) = 0x1c9;
    WBUFL (buf, 2) = unit->bl.id;
    WBUFL (buf, 6) = unit->group->src_id;
    WBUFW (buf, 10) = unit->bl.x;
    WBUFW (buf, 12) = unit->bl.y;
    WBUFB (buf, 14) = unit->group->unit_id;
    WBUFB (buf, 15) = 1;
    WBUFL (buf, 15 + 1) = 0;    //1-4調べた限り固定
    WBUFL (buf, 15 + 5) = 0;    //5-8調べた限り固定
    //9-12マップごとで一定の77-80とはまた違う4バイトのかなり大きな数字
    WBUFL (buf, 15 + 13) = unit->bl.y - 0x12;   //13-16ユニットのY座標-18っぽい(Y:17でFF FF FF FF)
    WBUFL (buf, 15 + 17) = 0x004f37dd;  //17-20調べた限り固定(0x1b2で0x004fdbddだった)
    WBUFL (buf, 15 + 21) = 0x0012f674;  //21-24調べた限り固定
    WBUFL (buf, 15 + 25) = 0x0012f664;  //25-28調べた限り固定
    WBUFL (buf, 15 + 29) = 0x0012f654;  //29-32調べた限り固定
    WBUFL (buf, 15 + 33) = 0x77527bbc;  //33-36調べた限り固定
    //37-39
    WBUFB (buf, 15 + 40) = 0x2d;    //40調べた限り固定
    WBUFL (buf, 15 + 41) = 0;   //41-44調べた限り0固定
    WBUFL (buf, 15 + 45) = 0;   //45-48調べた限り0固定
    WBUFL (buf, 15 + 49) = 0;   //49-52調べた限り0固定
    WBUFL (buf, 15 + 53) = 0x0048d919;  //53-56調べた限り固定(0x01b2で0x00495119だった)
    WBUFL (buf, 15 + 57) = 0x0000003e;  //57-60調べた限り固定
    WBUFL (buf, 15 + 61) = 0x0012f66c;  //61-64調べた限り固定
    //65-68
    //69-72
    if (bl)
        WBUFL (buf, 15 + 73) = bl->y;   //73-76術者のY座標
    WBUFL (buf, 15 + 77) = unit->bl.m;  //77-80マップIDかなぁ？かなり2バイトで足りそうな数字
    WBUFB (buf, 15 + 81) = 0xaa;    //81終端文字0xaa

    /*      Graffiti [Valaris]      */
    if (unit->group->unit_id == 0xb0)
    {
        WBUFL (buf, 15) = 1;
        WBUFL (buf, 16) = 1;
        memcpy (WBUFP (buf, 17), unit->group->valstr, 80);
    }

    clif_send (buf, packet_len_table[0x1c9], &unit->bl, AREA);
    return 0;
}

/*==========================================
 * 場所スキルエフェクト削除
 *------------------------------------------
 */
int clif_skill_delunit (struct skill_unit *unit)
{
    unsigned char buf[16];

    nullpo_retr (0, unit);

    WBUFW (buf, 0) = 0x120;
    WBUFL (buf, 2) = unit->bl.id;
    clif_send (buf, packet_len_table[0x120], &unit->bl, AREA);
    return 0;
}

/*==========================================
 * ワープ場所選択
 *------------------------------------------
 */
int clif_skill_warppoint (struct map_session_data *sd, int skill_num,
                          const char *map1, const char *map2,
                          const char *map3, const char *map4)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x11c;
    WFIFOW (fd, 2) = skill_num;
    memcpy (WFIFOP (fd, 4), map1, 16);
    memcpy (WFIFOP (fd, 20), map2, 16);
    memcpy (WFIFOP (fd, 36), map3, 16);
    memcpy (WFIFOP (fd, 52), map4, 16);
    WFIFOSET (fd, packet_len_table[0x11c]);
    return 0;
}

/*==========================================
 * メモ応答
 *------------------------------------------
 */
int clif_skill_memo (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0x11e;
    WFIFOB (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x11e]);
    return 0;
}

int clif_skill_teleportmessage (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x189;
    WFIFOW (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x189]);
    return 0;
}

/*==========================================
 * モンスター情報
 *------------------------------------------
 */
int clif_skill_estimation (struct map_session_data *sd,
                           struct block_list *dst)
{
    struct mob_data *md;
    unsigned char buf[64];
    int  i;

    nullpo_retr (0, sd);
    nullpo_retr (0, dst);

    if (dst->type != BL_MOB)
        return 0;
    if ((md = (struct mob_data *) dst) == NULL)
        return 0;

    WBUFW (buf, 0) = 0x18c;
    WBUFW (buf, 2) = mob_get_viewclass (md->class);
    WBUFW (buf, 4) = mob_db[md->class].lv;
    WBUFW (buf, 6) = mob_db[md->class].size;
    WBUFL (buf, 8) = md->hp;
    WBUFW (buf, 12) = battle_get_def2 (&md->bl);
    WBUFW (buf, 14) = mob_db[md->class].race;
    WBUFW (buf, 16) =
        battle_get_mdef2 (&md->bl) - (mob_db[md->class].vit >> 1);
    WBUFW (buf, 18) = battle_get_elem_type (&md->bl);
    for (i = 0; i < 9; i++)
        WBUFB (buf, 20 + i) = battle_attr_fix (100, i + 1, md->def_ele);

    if (sd->status.party_id > 0)
        clif_send (buf, packet_len_table[0x18c], &sd->bl, PARTY_AREA);
    else
    {
        memcpy (WFIFOP (sd->fd, 0), buf, packet_len_table[0x18c]);
        WFIFOSET (sd->fd, packet_len_table[0x18c]);
    }
    return 0;
}

/*==========================================
 * 状態異常アイコン/メッセージ表示
 *------------------------------------------
 */
int clif_status_change (struct block_list *bl, int type, int flag)
{
    unsigned char buf[16];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x0196;
    WBUFW (buf, 2) = type;
    WBUFL (buf, 4) = bl->id;
    WBUFB (buf, 8) = flag;
    clif_send (buf, packet_len_table[0x196], bl, AREA);
    return 0;
}

/*==========================================
 * Send message (modified by [Yor])
 *------------------------------------------
 */
int clif_displaymessage (const int fd, char *mes)
{
    int  len_mes = strlen (mes);

    if (len_mes > 0)
    {                           // don't send a void message (it's not displaying on the client chat). @help can send void line.
        WFIFOW (fd, 0) = 0x8e;
        WFIFOW (fd, 2) = 5 + len_mes;   // 4 + len + NULL teminate
        memcpy (WFIFOP (fd, 4), mes, len_mes + 1);
        WFIFOSET (fd, 5 + len_mes);
    }

    return 0;
}

/*==========================================
 * 天の声を送信する
 *------------------------------------------
 */
int clif_GMmessage (struct block_list *bl, char *mes, int len, int flag)
{
    unsigned char lbuf[255];
    unsigned char *buf =
        ((len + 16) >= sizeof (lbuf)) ? malloc (len + 16) : lbuf;
    int  lp = (flag & 0x10) ? 8 : 4;

    WBUFW (buf, 0) = 0x9a;
    WBUFW (buf, 2) = len + lp;
    WBUFL (buf, 4) = 0x65756c62;
    memcpy (WBUFP (buf, lp), mes, len);
    flag &= 0x07;
    clif_send (buf, WBUFW (buf, 2), bl,
               (flag == 1) ? ALL_SAMEMAP :
               (flag == 2) ? AREA : (flag == 3) ? SELF : ALL_CLIENT);
    if (buf != lbuf)
        free (buf);
    return 0;
}

/*==========================================
 * HPSP回復エフェクトを送信する
 *------------------------------------------
 */
int clif_heal (int fd, int type, int val)
{
    WFIFOW (fd, 0) = 0x13d;
    WFIFOW (fd, 2) = type;
    WFIFOW (fd, 4) = val;
    WFIFOSET (fd, packet_len_table[0x13d]);

    return 0;
}

/*==========================================
 * 復活する
 *------------------------------------------
 */
int clif_resurrection (struct block_list *bl, int type)
{
    unsigned char buf[16];

    nullpo_retr (0, bl);

    if (bl->type == BL_PC)
    {                           // disguises [Valaris]
        struct map_session_data *sd = ((struct map_session_data *) bl);
        if (sd && sd->disguise > 23 && sd->disguise < 4001)
            clif_spawnpc (sd);
    }

    WBUFW (buf, 0) = 0x148;
    WBUFL (buf, 2) = bl->id;
    WBUFW (buf, 6) = type;

    clif_send (buf, packet_len_table[0x148], bl, type == 1 ? AREA : AREA_WOS);

    return 0;
}

/*==========================================
 * PVP実装？（仮）
 *------------------------------------------
 */
int clif_set0199 (int fd, int type)
{
    WFIFOW (fd, 0) = 0x199;
    WFIFOW (fd, 2) = type;
    WFIFOSET (fd, packet_len_table[0x199]);

    return 0;
}

/*==========================================
 * PVP実装？(仮)
 *------------------------------------------
 */
int clif_pvpset (struct map_session_data *sd, int pvprank, int pvpnum,
                 int type)
{
    nullpo_retr (0, sd);

    if (map[sd->bl.m].flag.nopvp)
        return 0;

    if (type == 2)
    {
        WFIFOW (sd->fd, 0) = 0x19a;
        WFIFOL (sd->fd, 2) = sd->bl.id;
        if (pvprank <= 0)
            pc_calc_pvprank (sd);
        WFIFOL (sd->fd, 6) = pvprank;
        WFIFOL (sd->fd, 10) = pvpnum;
        WFIFOSET (sd->fd, packet_len_table[0x19a]);
    }
    else
    {
        char buf[32];

        WBUFW (buf, 0) = 0x19a;
        WBUFL (buf, 2) = sd->bl.id;
        if (sd->status.option & 0x46)
            WBUFL (buf, 6) = -1;
        else if (pvprank <= 0)
            pc_calc_pvprank (sd);
        WBUFL (buf, 6) = pvprank;
        WBUFL (buf, 10) = pvpnum;
        if (!type)
            clif_send (buf, packet_len_table[0x19a], &sd->bl, AREA);
        else
            clif_send (buf, packet_len_table[0x19a], &sd->bl, ALL_SAMEMAP);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send0199 (int map, int type)
{
    struct block_list bl;
    char buf[16];

    bl.m = map;
    WBUFW (buf, 0) = 0x199;
    WBUFW (buf, 2) = type;
    clif_send (buf, packet_len_table[0x199], &bl, ALL_SAMEMAP);

    return 0;
}

/*==========================================
 * 精錬エフェクトを送信する
 *------------------------------------------
 */
int clif_refine (int fd, struct map_session_data *sd, int fail, int index,
                 int val)
{
    WFIFOW (fd, 0) = 0x188;
    WFIFOW (fd, 2) = fail;
    WFIFOW (fd, 4) = index + 2;
    WFIFOW (fd, 6) = val;
    WFIFOSET (fd, packet_len_table[0x188]);

    return 0;
}

/*==========================================
 * Wisp/page is transmitted to the destination player
 *------------------------------------------
 */
int clif_wis_message (int fd, char *nick, char *mes, int mes_len)   // R 0097 <len>.w <nick>.24B <message>.?B
{
    WFIFOW (fd, 0) = 0x97;
    WFIFOW (fd, 2) = mes_len + 24 + 4;
    memcpy (WFIFOP (fd, 4), nick, 24);
    memcpy (WFIFOP (fd, 28), mes, mes_len);
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * The transmission result of Wisp/page is transmitted to the source player
 *------------------------------------------
 */
int clif_wis_end (int fd, int flag) // R 0098 <type>.B: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
{
    WFIFOW (fd, 0) = 0x98;
    WFIFOW (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x98]);
    return 0;
}

/*==========================================
 * キャラID名前引き結果を送信する
 *------------------------------------------
 */
int clif_solved_charname (struct map_session_data *sd, int char_id)
{
    char *p = map_charid2nick (char_id);
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    if (p != NULL)
    {
        WFIFOW (fd, 0) = 0x194;
        WFIFOL (fd, 2) = char_id;
        memcpy (WFIFOP (fd, 6), p, 24);
        WFIFOSET (fd, packet_len_table[0x194]);
    }
    else
    {
        map_reqchariddb (sd, char_id);
        chrif_searchcharid (char_id);
    }
    return 0;
}

/*==========================================
 * カードの挿入可能リストを返す
 *------------------------------------------
 */
int clif_use_card (struct map_session_data *sd, int idx)
{
    nullpo_retr (0, sd);

    if (sd->inventory_data[idx])
    {
        int  i, c;
        int  ep = sd->inventory_data[idx]->equip;
        int  fd = sd->fd;
        WFIFOW (fd, 0) = 0x017b;

        for (i = c = 0; i < MAX_INVENTORY; i++)
        {
            int  j;

            if (sd->inventory_data[i] == NULL)
                continue;
            if (sd->inventory_data[i]->type != 4 && sd->inventory_data[i]->type != 5)   // 武器防具じゃない
                continue;
            if (sd->status.inventory[i].card[0] == 0x00ff)  // 製造武器
                continue;
            if (sd->status.inventory[i].card[0] == (short) 0xff00
                || sd->status.inventory[i].card[0] == 0x00fe)
                continue;
            if (sd->status.inventory[i].identify == 0)  // 未鑑定
                continue;

            if ((sd->inventory_data[i]->equip & ep) == 0)   // 装備個所が違う
                continue;
            if (sd->inventory_data[i]->type == 4 && ep == 32)   // 盾カードと両手武器
                continue;

            for (j = 0; j < sd->inventory_data[i]->slot; j++)
            {
                if (sd->status.inventory[i].card[j] == 0)
                    break;
            }
            if (j == sd->inventory_data[i]->slot)   // すでにカードが一杯
                continue;

            WFIFOW (fd, 4 + c * 2) = i + 2;
            c++;
        }
        WFIFOW (fd, 2) = 4 + c * 2;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }

    return 0;
}

/*==========================================
 * カードの挿入終了
 *------------------------------------------
 */
int clif_insert_card (struct map_session_data *sd, int idx_equip,
                      int idx_card, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x17d;
    WFIFOW (fd, 2) = idx_equip + 2;
    WFIFOW (fd, 4) = idx_card + 2;
    WFIFOB (fd, 6) = flag;
    WFIFOSET (fd, packet_len_table[0x17d]);
    return 0;
}

/*==========================================
 * 鑑定可能アイテムリスト送信
 *------------------------------------------
 */
int clif_item_identify_list (struct map_session_data *sd)
{
    int  i, c;
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0x177;
    for (i = c = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid > 0
            && sd->status.inventory[i].identify != 1)
        {
            WFIFOW (fd, c * 2 + 4) = i + 2;
            c++;
        }
    }
    if (c > 0)
    {
        WFIFOW (fd, 2) = c * 2 + 4;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 * 鑑定結果
 *------------------------------------------
 */
int clif_item_identified (struct map_session_data *sd, int idx, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x179;
    WFIFOW (fd, 2) = idx + 2;
    WFIFOB (fd, 4) = flag;
    WFIFOSET (fd, packet_len_table[0x179]);
    return 0;
}

/*==========================================
 * 修理可能アイテムリスト送信
 * ※実際のパケットがわからないので動作しません
 *------------------------------------------
 */
int clif_item_repair_list (struct map_session_data *sd)
{
    int  i, c;
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0x0;
    for (i = c = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].nameid > 0
            && sd->status.inventory[i].broken != 0)
        {
            WFIFOW (fd, c * 2 + 4) = i + 2;
            c++;
        }
    }
    if (c > 0)
    {
        WFIFOW (fd, 2) = c * 2 + 4;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 * アイテムによる一時的なスキル効果
 *------------------------------------------
 */
int clif_item_skill (struct map_session_data *sd, int skillid, int skilllv,
                     const char *name)
{
    int  range, fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x147;
    WFIFOW (fd, 2) = skillid;
    WFIFOW (fd, 4) = skill_get_inf (skillid);
    WFIFOW (fd, 6) = 0;
    WFIFOW (fd, 8) = skilllv;
    WFIFOW (fd, 10) = skill_get_sp (skillid, skilllv);
    range = skill_get_range (skillid, skilllv);
    if (range < 0)
        range = battle_get_range (&sd->bl) - (range + 1);
    WFIFOW (fd, 12) = range;
    memcpy (WFIFOP (fd, 14), name, 24);
    WFIFOB (fd, 38) = 0;
    WFIFOSET (fd, packet_len_table[0x147]);
    return 0;
}

/*==========================================
 * カートにアイテム追加
 *------------------------------------------
 */
int clif_cart_additem (struct map_session_data *sd, int n, int amount,
                       int fail)
{
    int  view, j, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    if (n < 0 || n >= MAX_CART || sd->status.cart[n].nameid <= 0)
        return 1;

    WBUFW (buf, 0) = 0x124;
    WBUFW (buf, 2) = n + 2;
    WBUFL (buf, 4) = amount;
    if ((view = itemdb_viewid (sd->status.cart[n].nameid)) > 0)
        WBUFW (buf, 8) = view;
    else
        WBUFW (buf, 8) = sd->status.cart[n].nameid;
    WBUFB (buf, 10) = sd->status.cart[n].identify;
    if (sd->status.cart[n].broken == 1) //is weapon broken [Valaris]
        WBUFB (buf, 11) = 1;
    else
        WBUFB (buf, 11) = sd->status.cart[n].attribute;
    WBUFB (buf, 12) = sd->status.cart[n].refine;
    if (sd->status.cart[n].card[0] == 0x00ff
        || sd->status.cart[n].card[0] == 0x00fe
        || sd->status.cart[n].card[0] == (short) 0xff00)
    {
        WBUFW (buf, 13) = sd->status.cart[n].card[0];
        WBUFW (buf, 15) = sd->status.cart[n].card[1];
        WBUFW (buf, 17) = sd->status.cart[n].card[2];
        WBUFW (buf, 19) = sd->status.cart[n].card[3];
    }
    else
    {
        if (sd->status.cart[n].card[0] > 0
            && (j = itemdb_viewid (sd->status.cart[n].card[0])) > 0)
            WBUFW (buf, 13) = j;
        else
            WBUFW (buf, 13) = sd->status.cart[n].card[0];
        if (sd->status.cart[n].card[1] > 0
            && (j = itemdb_viewid (sd->status.cart[n].card[1])) > 0)
            WBUFW (buf, 15) = j;
        else
            WBUFW (buf, 15) = sd->status.cart[n].card[1];
        if (sd->status.cart[n].card[2] > 0
            && (j = itemdb_viewid (sd->status.cart[n].card[2])) > 0)
            WBUFW (buf, 17) = j;
        else
            WBUFW (buf, 17) = sd->status.cart[n].card[2];
        if (sd->status.cart[n].card[3] > 0
            && (j = itemdb_viewid (sd->status.cart[n].card[3])) > 0)
            WBUFW (buf, 19) = j;
        else
            WBUFW (buf, 19) = sd->status.cart[n].card[3];
    }
    WFIFOSET (fd, packet_len_table[0x124]);
    return 0;
}

/*==========================================
 * カートからアイテム削除
 *------------------------------------------
 */
int clif_cart_delitem (struct map_session_data *sd, int n, int amount)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;

    WFIFOW (fd, 0) = 0x125;
    WFIFOW (fd, 2) = n + 2;
    WFIFOL (fd, 4) = amount;

    WFIFOSET (fd, packet_len_table[0x125]);

    return 0;
}

/*==========================================
 * カートのアイテムリスト
 *------------------------------------------
 */
int clif_cart_itemlist (struct map_session_data *sd)
{
    struct item_data *id;
    int  i, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);
    WBUFW (buf, 0) = 0x1ef;
    for (i = 0, n = 0; i < MAX_CART; i++)
    {
        if (sd->status.cart[i].nameid <= 0)
            continue;
        id = itemdb_search (sd->status.cart[i].nameid);
        if (itemdb_isequip2 (id))
            continue;
        WBUFW (buf, n * 18 + 4) = i + 2;
        if (id->view_id > 0)
            WBUFW (buf, n * 18 + 6) = id->view_id;
        else
            WBUFW (buf, n * 18 + 6) = sd->status.cart[i].nameid;
        WBUFB (buf, n * 18 + 8) = id->type;
        WBUFB (buf, n * 18 + 9) = sd->status.cart[i].identify;
        WBUFW (buf, n * 18 + 10) = sd->status.cart[i].amount;
        WBUFW (buf, n * 18 + 12) = 0;
        WBUFW (buf, n * 18 + 14) = sd->status.cart[i].card[0];
        WBUFW (buf, n * 18 + 16) = sd->status.cart[i].card[1];
        WBUFW (buf, n * 18 + 18) = sd->status.cart[i].card[2];
        WBUFW (buf, n * 18 + 20) = sd->status.cart[i].card[3];
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 18;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
    return 0;
}

/*==========================================
 * カートの装備品リスト
 *------------------------------------------
 */
int clif_cart_equiplist (struct map_session_data *sd)
{
    struct item_data *id;
    int  i, j, n, fd;
    unsigned char *buf;

    nullpo_retr (0, sd);

    fd = sd->fd;
    buf = WFIFOP (fd, 0);

    WBUFW (buf, 0) = 0x122;
    for (i = 0, n = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.cart[i].nameid <= 0)
            continue;
        id = itemdb_search (sd->status.cart[i].nameid);
        if (!itemdb_isequip2 (id))
            continue;
        WBUFW (buf, n * 20 + 4) = i + 2;
        if (id->view_id > 0)
            WBUFW (buf, n * 20 + 6) = id->view_id;
        else
            WBUFW (buf, n * 20 + 6) = sd->status.cart[i].nameid;
        WBUFB (buf, n * 20 + 8) = id->type;
        WBUFB (buf, n * 20 + 9) = sd->status.cart[i].identify;
        WBUFW (buf, n * 20 + 10) = id->equip;
        WBUFW (buf, n * 20 + 12) = sd->status.cart[i].equip;
        if (sd->status.cart[i].broken == 1)
            WBUFB (buf, n * 20 + 14) = 1;   //is weapon broken [Valaris]
        else
            WBUFB (buf, n * 20 + 14) = sd->status.cart[i].attribute;
        WBUFB (buf, n * 20 + 15) = sd->status.cart[i].refine;
        if (sd->status.cart[i].card[0] == 0x00ff
            || sd->status.cart[i].card[0] == 0x00fe
            || sd->status.cart[i].card[0] == (short) 0xff00)
        {
            WBUFW (buf, n * 20 + 16) = sd->status.cart[i].card[0];
            WBUFW (buf, n * 20 + 18) = sd->status.cart[i].card[1];
            WBUFW (buf, n * 20 + 20) = sd->status.cart[i].card[2];
            WBUFW (buf, n * 20 + 22) = sd->status.cart[i].card[3];
        }
        else
        {
            if (sd->status.cart[i].card[0] > 0
                && (j = itemdb_viewid (sd->status.cart[i].card[0])) > 0)
                WBUFW (buf, n * 20 + 16) = j;
            else
                WBUFW (buf, n * 20 + 16) = sd->status.cart[i].card[0];
            if (sd->status.cart[i].card[1] > 0
                && (j = itemdb_viewid (sd->status.cart[i].card[1])) > 0)
                WBUFW (buf, n * 20 + 18) = j;
            else
                WBUFW (buf, n * 20 + 18) = sd->status.cart[i].card[1];
            if (sd->status.cart[i].card[2] > 0
                && (j = itemdb_viewid (sd->status.cart[i].card[2])) > 0)
                WBUFW (buf, n * 20 + 20) = j;
            else
                WBUFW (buf, n * 20 + 20) = sd->status.cart[i].card[2];
            if (sd->status.cart[i].card[3] > 0
                && (j = itemdb_viewid (sd->status.cart[i].card[3])) > 0)
                WBUFW (buf, n * 20 + 22) = j;
            else
                WBUFW (buf, n * 20 + 22) = sd->status.cart[i].card[3];
        }
        n++;
    }
    if (n)
    {
        WBUFW (buf, 2) = 4 + n * 20;
        WFIFOSET (fd, WFIFOW (fd, 2));
    }
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
int clif_party_created (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xfa;
    WFIFOB (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0xfa]);
    return 0;
}

/*==========================================
 * パーティ情報送信
 *------------------------------------------
 */
int clif_party_info (struct party *p, int fd)
{
    unsigned char buf[1024];
    int  i, c;
    struct map_session_data *sd = NULL;

    nullpo_retr (0, p);

    WBUFW (buf, 0) = 0xfb;
    memcpy (WBUFP (buf, 4), p->name, 24);
    for (i = c = 0; i < MAX_PARTY; i++)
    {
        struct party_member *m = &p->member[i];
        if (m->account_id > 0)
        {
            if (sd == NULL)
                sd = m->sd;
            WBUFL (buf, 28 + c * 46) = m->account_id;
            memcpy (WBUFP (buf, 28 + c * 46 + 4), m->name, 24);
            memcpy (WBUFP (buf, 28 + c * 46 + 28), m->map, 16);
            WBUFB (buf, 28 + c * 46 + 44) = (m->leader) ? 0 : 1;
            WBUFB (buf, 28 + c * 46 + 45) = (m->online) ? 0 : 1;
            c++;
        }
    }
    WBUFW (buf, 2) = 28 + c * 46;
    if (fd >= 0)
    {                           // fdが設定されてるならそれに送る
        memcpy (WFIFOP (fd, 0), buf, WBUFW (buf, 2));
        WFIFOSET (fd, WFIFOW (fd, 2));
        return 9;
    }
    if (sd != NULL)
        clif_send (buf, WBUFW (buf, 2), &sd->bl, PARTY);
    return 0;
}

/*==========================================
 * パーティ勧誘
 * Relay a party invitation.
 *
 * (R 00fe <sender_ID>.l <party_name>.24B)
 *------------------------------------------
 */
int clif_party_invite (struct map_session_data *sd,
                       struct map_session_data *tsd)
{
    int  fd;
    struct party *p;

    nullpo_retr (0, sd);
    nullpo_retr (0, tsd);

    fd = tsd->fd;

    if (!(p = party_search (sd->status.party_id)))
        return 0;

    WFIFOW (fd, 0) = 0xfe;
    WFIFOL (fd, 2) = sd->status.account_id;
    memcpy (WFIFOP (fd, 6), p->name, 24);
    WFIFOSET (fd, packet_len_table[0xfe]);
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
int clif_party_inviteack (struct map_session_data *sd, char *nick, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xfd;
    memcpy (WFIFOP (fd, 2), nick, 24);
    WFIFOB (fd, 26) = flag;
    WFIFOSET (fd, packet_len_table[0xfd]);
    return 0;
}

/*==========================================
 * パーティ設定送信
 * flag & 0x001=exp変更ミス
 *        0x010=item変更ミス
 *        0x100=一人にのみ送信
 *------------------------------------------
 */
int clif_party_option (struct party *p, struct map_session_data *sd, int flag)
{
    unsigned char buf[16];

    nullpo_retr (0, p);

//  if(battle_config.etc_log)
//      printf("clif_party_option: %d %d %d\n",p->exp,p->item,flag);
    if (sd == NULL && flag == 0)
    {
        int  i;
        for (i = 0; i < MAX_PARTY; i++)
            if ((sd = map_id2sd (p->member[i].account_id)) != NULL)
                break;
    }
    if (sd == NULL)
        return 0;
    WBUFW (buf, 0) = 0x101;
    WBUFW (buf, 2) = ((flag & 0x01) ? 2 : p->exp);
    WBUFW (buf, 4) = ((flag & 0x10) ? 2 : p->item);
    if (flag == 0)
        clif_send (buf, packet_len_table[0x101], &sd->bl, PARTY);
    else
    {
        memcpy (WFIFOP (sd->fd, 0), buf, packet_len_table[0x101]);
        WFIFOSET (sd->fd, packet_len_table[0x101]);
    }
    return 0;
}

/*==========================================
 * パーティ脱退（脱退前に呼ぶこと）
 *------------------------------------------
 */
int clif_party_leaved (struct party *p, struct map_session_data *sd,
                       int account_id, char *name, int flag)
{
    unsigned char buf[64];
    int  i;

    nullpo_retr (0, p);

    WBUFW (buf, 0) = 0x105;
    WBUFL (buf, 2) = account_id;
    memcpy (WBUFP (buf, 6), name, 24);
    WBUFB (buf, 30) = flag & 0x0f;

    if ((flag & 0xf0) == 0)
    {
        if (sd == NULL)
            for (i = 0; i < MAX_PARTY; i++)
                if ((sd = p->member[i].sd) != NULL)
                    break;
        if (sd != NULL)
            clif_send (buf, packet_len_table[0x105], &sd->bl, PARTY);
    }
    else if (sd != NULL)
    {
        memcpy (WFIFOP (sd->fd, 0), buf, packet_len_table[0x105]);
        WFIFOSET (sd->fd, packet_len_table[0x105]);
    }
    return 0;
}

/*==========================================
 * パーティメッセージ送信
 *------------------------------------------
 */
int clif_party_message (struct party *p, int account_id, char *mes, int len)
{
    struct map_session_data *sd;
    int  i;

    nullpo_retr (0, p);

    for (i = 0; i < MAX_PARTY; i++)
    {
        if ((sd = p->member[i].sd) != NULL)
            break;
    }
    if (sd != NULL)
    {
        unsigned char buf[1024];
        WBUFW (buf, 0) = 0x109;
        WBUFW (buf, 2) = len + 8;
        WBUFL (buf, 4) = account_id;
        memcpy (WBUFP (buf, 8), mes, len);
        clif_send (buf, len + 8, &sd->bl, PARTY);
    }
    return 0;
}

/*==========================================
 * パーティ座標通知
 *------------------------------------------
 */
int clif_party_xy (struct party *p, struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x107;
    WBUFL (buf, 2) = sd->status.account_id;
    WBUFW (buf, 6) = sd->bl.x;
    WBUFW (buf, 8) = sd->bl.y;
    clif_send (buf, packet_len_table[0x107], &sd->bl, PARTY_SAMEMAP_WOS);
//  if(battle_config.etc_log)
//      printf("clif_party_xy %d\n",sd->status.account_id);
    return 0;
}

/*==========================================
 * パーティHP通知
 *------------------------------------------
 */
int clif_party_hp (struct party *p, struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x106;
    WBUFL (buf, 2) = sd->status.account_id;
    WBUFW (buf, 6) = (sd->status.hp > 0x7fff) ? 0x7fff : sd->status.hp;
    WBUFW (buf, 8) =
        (sd->status.max_hp > 0x7fff) ? 0x7fff : sd->status.max_hp;
    clif_send (buf, packet_len_table[0x106], &sd->bl, PARTY_AREA_WOS);
//  if(battle_config.etc_log)
//      printf("clif_party_hp %d\n",sd->status.account_id);
    return 0;
}

/*==========================================
 * パーティ場所移動（未使用）
 *------------------------------------------
 */
int clif_party_move (struct party *p, struct map_session_data *sd, int online)
{
    unsigned char buf[128];

    nullpo_retr (0, sd);
    nullpo_retr (0, p);

    WBUFW (buf, 0) = 0x104;
    WBUFL (buf, 2) = sd->status.account_id;
    WBUFL (buf, 6) = 0;
    WBUFW (buf, 10) = sd->bl.x;
    WBUFW (buf, 12) = sd->bl.y;
    WBUFB (buf, 14) = !online;
    memcpy (WBUFP (buf, 15), p->name, 24);
    memcpy (WBUFP (buf, 39), sd->status.name, 24);
    memcpy (WBUFP (buf, 63), map[sd->bl.m].name, 16);
    clif_send (buf, packet_len_table[0x104], &sd->bl, PARTY);
    return 0;
}

/*==========================================
 * 攻撃するために移動が必要
 *------------------------------------------
 */
int clif_movetoattack (struct map_session_data *sd, struct block_list *bl)
{
    int  fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, bl);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x139;
    WFIFOL (fd, 2) = bl->id;
    WFIFOW (fd, 6) = bl->x;
    WFIFOW (fd, 8) = bl->y;
    WFIFOW (fd, 10) = sd->bl.x;
    WFIFOW (fd, 12) = sd->bl.y;
    WFIFOW (fd, 14) = sd->attackrange;
    WFIFOSET (fd, packet_len_table[0x139]);
    return 0;
}

/*==========================================
 * 製造エフェクト
 *------------------------------------------
 */
int clif_produceeffect (struct map_session_data *sd, int flag, int nameid)
{
    int  view, fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    // 名前の登録と送信を先にしておく
    if (map_charid2nick (sd->status.char_id) == NULL)
        map_addchariddb (sd->status.char_id, sd->status.name);
    clif_solved_charname (sd, sd->status.char_id);

    WFIFOW (fd, 0) = 0x18f;
    WFIFOW (fd, 2) = flag;
    if ((view = itemdb_viewid (nameid)) > 0)
        WFIFOW (fd, 4) = view;
    else
        WFIFOW (fd, 4) = nameid;
    WFIFOSET (fd, packet_len_table[0x18f]);
    return 0;
}

/*==========================================
 * オートスペル リスト送信
 *------------------------------------------
 */
int clif_autospell (struct map_session_data *sd, int skilllv)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x1cd;

    if (skilllv > 0 && pc_checkskill (sd, MG_NAPALMBEAT) > 0)
        WFIFOL (fd, 2) = MG_NAPALMBEAT;
    else
        WFIFOL (fd, 2) = 0x00000000;
    if (skilllv > 1 && pc_checkskill (sd, MG_COLDBOLT) > 0)
        WFIFOL (fd, 6) = MG_COLDBOLT;
    else
        WFIFOL (fd, 6) = 0x00000000;
    if (skilllv > 1 && pc_checkskill (sd, MG_FIREBOLT) > 0)
        WFIFOL (fd, 10) = MG_FIREBOLT;
    else
        WFIFOL (fd, 10) = 0x00000000;
    if (skilllv > 1 && pc_checkskill (sd, MG_LIGHTNINGBOLT) > 0)
        WFIFOL (fd, 14) = MG_LIGHTNINGBOLT;
    else
        WFIFOL (fd, 14) = 0x00000000;
    if (skilllv > 4 && pc_checkskill (sd, MG_SOULSTRIKE) > 0)
        WFIFOL (fd, 18) = MG_SOULSTRIKE;
    else
        WFIFOL (fd, 18) = 0x00000000;
    if (skilllv > 7 && pc_checkskill (sd, MG_FIREBALL) > 0)
        WFIFOL (fd, 22) = MG_FIREBALL;
    else
        WFIFOL (fd, 22) = 0x00000000;
    if (skilllv > 9 && pc_checkskill (sd, MG_FROSTDIVER) > 0)
        WFIFOL (fd, 26) = MG_FROSTDIVER;
    else
        WFIFOL (fd, 26) = 0x00000000;

    WFIFOSET (fd, packet_len_table[0x1cd]);
    return 0;
}

/*==========================================
 * ディボーションの青い糸
 *------------------------------------------
 */
int clif_devotion (struct map_session_data *sd, int target)
{
    unsigned char buf[56];
    int  n;

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x1cf;
    WBUFL (buf, 2) = sd->bl.id;
//  WBUFL(buf,6)=target;
    for (n = 0; n < 5; n++)
        WBUFL (buf, 6 + 4 * n) = sd->dev.val2[n];
//      WBUFL(buf,10+4*n)=0;
    WBUFB (buf, 26) = 8;
    WBUFB (buf, 27) = 0;

    clif_send (buf, packet_len_table[0x1cf], &sd->bl, AREA);
    return 0;
}

/*==========================================
 * 氣球
 *------------------------------------------
 */
int clif_spiritball (struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x1d0;
    WBUFL (buf, 2) = sd->bl.id;
    WBUFW (buf, 6) = sd->spiritball;
    clif_send (buf, packet_len_table[0x1d0], &sd->bl, AREA);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_combo_delay (struct block_list *bl, int wait)
{
    unsigned char buf[32];

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x1d2;
    WBUFL (buf, 2) = bl->id;
    WBUFL (buf, 6) = wait;
    clif_send (buf, packet_len_table[0x1d2], bl, AREA);

    return 0;
}

/*==========================================
 *白刃取り
 *------------------------------------------
 */
int clif_bladestop (struct block_list *src, struct block_list *dst, int bool)
{
    unsigned char buf[32];

    nullpo_retr (0, src);
    nullpo_retr (0, dst);

    WBUFW (buf, 0) = 0x1d1;
    WBUFL (buf, 2) = src->id;
    WBUFL (buf, 6) = dst->id;
    WBUFL (buf, 10) = bool;

    clif_send (buf, packet_len_table[0x1d1], src, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapcell (int m, int x, int y, int cell_type, int type)
{
    struct block_list bl;
    char buf[32];

    bl.m = m;
    bl.x = x;
    bl.y = y;
    WBUFW (buf, 0) = 0x192;
    WBUFW (buf, 2) = x;
    WBUFW (buf, 4) = y;
    WBUFW (buf, 6) = cell_type;
    memcpy (WBUFP (buf, 8), map[m].name, 16);
    if (!type)
        clif_send (buf, packet_len_table[0x192], &bl, AREA);
    else
        clif_send (buf, packet_len_table[0x192], &bl, ALL_SAMEMAP);

    return 0;
}

/*==========================================
 * MVPエフェクト
 *------------------------------------------
 */
int clif_mvp_effect (struct map_session_data *sd)
{
    unsigned char buf[16];

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x10c;
    WBUFL (buf, 2) = sd->bl.id;
    clif_send (buf, packet_len_table[0x10c], &sd->bl, AREA);
    return 0;
}

/*==========================================
 * MVPアイテム所得
 *------------------------------------------
 */
int clif_mvp_item (struct map_session_data *sd, int nameid)
{
    int  view, fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x10a;
    if ((view = itemdb_viewid (nameid)) > 0)
        WFIFOW (fd, 2) = view;
    else
        WFIFOW (fd, 2) = nameid;
    WFIFOSET (fd, packet_len_table[0x10a]);
    return 0;
}

/*==========================================
 * MVP経験値所得
 *------------------------------------------
 */
int clif_mvp_exp (struct map_session_data *sd, int exp)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x10b;
    WFIFOL (fd, 2) = exp;
    WFIFOSET (fd, packet_len_table[0x10b]);
    return 0;
}

/*==========================================
 * ギルド作成可否通知
 * Relay the result of guild creation.
 *
 * (R 0167 <flag>.B)
 *
 * flag:
 *  0 The guild was created.
 *  1 The character is already in a guild.
 *  2 The guild name is invalid/taken.
 *  3 The Emperium item is required.
 *------------------------------------------
 */
int clif_guild_created (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x167;
    WFIFOB (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x167]);
    return 0;
}

/*==========================================
 * ギルド所属通知
 *------------------------------------------
 */
int clif_guild_belonginfo (struct map_session_data *sd, struct guild *g)
{
    int  ps, fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, g);

    fd = sd->fd;
    ps = guild_getposition (sd, g);

    memset (WFIFOP (fd, 0), 0, packet_len_table[0x16c]);
    WFIFOW (fd, 0) = 0x16c;
    WFIFOL (fd, 2) = g->guild_id;
    WFIFOL (fd, 6) = g->emblem_id;
    WFIFOL (fd, 10) = g->position[ps].mode;
    memcpy (WFIFOP (fd, 19), g->name, 24);
    WFIFOSET (fd, packet_len_table[0x16c]);
    return 0;
}

/*==========================================
 * ギルドメンバログイン通知
 *------------------------------------------
 */
int clif_guild_memberlogin_notice (struct guild *g, int idx, int flag)
{
    unsigned char buf[64];

    nullpo_retr (0, g);

    WBUFW (buf, 0) = 0x16d;
    WBUFL (buf, 2) = g->member[idx].account_id;
    WBUFL (buf, 6) = 0;
    WBUFL (buf, 10) = flag;
    if (g->member[idx].sd == NULL)
    {
        struct map_session_data *sd = guild_getavailablesd (g);
        if (sd != NULL)
            clif_send (buf, packet_len_table[0x16d], &sd->bl, GUILD);
    }
    else
        clif_send (buf, packet_len_table[0x16d], &g->member[idx].sd->bl,
                   GUILD_WOS);
    return 0;
}

/*==========================================
 * ギルドマスター通知(14dへの応答)
 *------------------------------------------
 */
int clif_guild_masterormember (struct map_session_data *sd)
{
    int  type = 0x57, fd;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g != NULL && strcmp (g->master, sd->status.name) == 0)
        type = 0xd7;
    WFIFOW (fd, 0) = 0x14e;
    WFIFOL (fd, 2) = type;
    WFIFOSET (fd, packet_len_table[0x14e]);
    return 0;
}

/*==========================================
 * Basic Info (Territories [Valaris])
 *------------------------------------------
 */
int clif_guild_basicinfo (struct map_session_data *sd)
{
    int  fd, i, t = 0;
    struct guild *g;
    struct guild_castle *gc = NULL;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;

    WFIFOW (fd, 0) = 0x1b6;     //0x150;
    WFIFOL (fd, 2) = g->guild_id;
    WFIFOL (fd, 6) = g->guild_lv;
    WFIFOL (fd, 10) = g->connect_member;
    WFIFOL (fd, 14) = g->max_member;
    WFIFOL (fd, 18) = g->average_lv;
    WFIFOL (fd, 22) = g->exp;
    WFIFOL (fd, 26) = g->next_exp;
    WFIFOL (fd, 30) = 0;        // 上納
    WFIFOL (fd, 34) = 0;        // VW（性格の悪さ？：性向グラフ左右）
    WFIFOL (fd, 38) = 0;        // RF（正義の度合い？：性向グラフ上下）
    WFIFOL (fd, 42) = 0;        // 人数？
    memcpy (WFIFOP (fd, 46), g->name, 24);
    memcpy (WFIFOP (fd, 70), g->master, 24);

    for (i = 0; i < MAX_GUILDCASTLE; i++)
    {
        gc = guild_castle_search (i);
        if (!gc)
            continue;
        if (g->guild_id == gc->guild_id)
            t++;
    }

    if (t == 1)
        memcpy (WFIFOP (fd, 94), "One Castle", 20);
    else if (t == 2)
        memcpy (WFIFOP (fd, 94), "Two Castles", 20);
    else if (t == 3)
        memcpy (WFIFOP (fd, 94), "Three Castles", 20);
    else if (t == 4)
        memcpy (WFIFOP (fd, 94), "Four Castles", 20);
    else if (t == 5)
        memcpy (WFIFOP (fd, 94), "Five Castles", 20);
    else if (t == 6)
        memcpy (WFIFOP (fd, 94), "Six Castles", 20);
    else if (t == 7)
        memcpy (WFIFOP (fd, 94), "Seven Castles", 20);
    else if (t == 8)
        memcpy (WFIFOP (fd, 94), "Eight Castles", 20);
    else if (t == 9)
        memcpy (WFIFOP (fd, 94), "Nine Castles", 20);
    else if (t == 10)
        memcpy (WFIFOP (fd, 94), "Ten Castles", 20);
    else if (t == 11)
        memcpy (WFIFOP (fd, 94), "Eleven Castles", 20);
    else if (t == 12)
        memcpy (WFIFOP (fd, 94), "Twelve Castles", 20);
    else if (t == 13)
        memcpy (WFIFOP (fd, 94), "Thirteen Castles", 20);
    else if (t == 14)
        memcpy (WFIFOP (fd, 94), "Fourteen Castles", 20);
    else if (t == 15)
        memcpy (WFIFOP (fd, 94), "Fifteen Castles", 20);
    else if (t == 16)
        memcpy (WFIFOP (fd, 94), "Sixteen Castles", 20);
    else if (t == 17)
        memcpy (WFIFOP (fd, 94), "Seventeen Castles", 20);
    else if (t == 18)
        memcpy (WFIFOP (fd, 94), "Eighteen Castles", 20);
    else if (t == 19)
        memcpy (WFIFOP (fd, 94), "Nineteen Castles", 20);
    else if (t == 20)
        memcpy (WFIFOP (fd, 94), "Twenty Castles", 20);
    else if (t == 21)
        memcpy (WFIFOP (fd, 94), "Twenty One Castles", 20);
    else if (t == 22)
        memcpy (WFIFOP (fd, 94), "Twenty Two Castles", 20);
    else if (t == 23)
        memcpy (WFIFOP (fd, 94), "Twenty Three Castles", 20);
    else if (t == 24)
        memcpy (WFIFOP (fd, 94), "Twenty Four Castles", 20);
    else if (t == MAX_GUILDCASTLE)
        memcpy (WFIFOP (fd, 94), "Total Domination", 20);
    else
        memcpy (WFIFOP (fd, 94), "None Taken", 20);

    WFIFOSET (fd, packet_len_table[WFIFOW (fd, 0)]);
    clif_guild_emblem (sd, g);  // Guild emblem vanish fix [Valaris]
    return 0;
}

/*==========================================
 * ギルド同盟/敵対情報
 *------------------------------------------
 */
int clif_guild_allianceinfo (struct map_session_data *sd)
{
    int  fd, i, c;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;
    WFIFOW (fd, 0) = 0x14c;
    for (i = c = 0; i < MAX_GUILDALLIANCE; i++)
    {
        struct guild_alliance *a = &g->alliance[i];
        if (a->guild_id > 0)
        {
            WFIFOL (fd, c * 32 + 4) = a->opposition;
            WFIFOL (fd, c * 32 + 8) = a->guild_id;
            memcpy (WFIFOP (fd, c * 32 + 12), a->name, 24);
            c++;
        }
    }
    WFIFOW (fd, 2) = c * 32 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルドメンバーリスト
 *------------------------------------------
 */
int clif_guild_memberlist (struct map_session_data *sd)
{
    int  fd;
    int  i, c;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;

    WFIFOW (fd, 0) = 0x154;
    for (i = 0, c = 0; i < g->max_member; i++)
    {
        struct guild_member *m = &g->member[i];
        if (m->account_id == 0)
            continue;
        WFIFOL (fd, c * 104 + 4) = m->account_id;
        WFIFOL (fd, c * 104 + 8) = 0;
        WFIFOW (fd, c * 104 + 12) = m->hair;
        WFIFOW (fd, c * 104 + 14) = m->hair_color;
        WFIFOW (fd, c * 104 + 16) = m->gender;
        WFIFOW (fd, c * 104 + 18) = m->class;
        WFIFOW (fd, c * 104 + 20) = m->lv;
        WFIFOL (fd, c * 104 + 22) = m->exp;
        WFIFOL (fd, c * 104 + 26) = m->online;
        WFIFOL (fd, c * 104 + 30) = m->position;
        memset (WFIFOP (fd, c * 104 + 34), 0, 50);  // メモ？
        memcpy (WFIFOP (fd, c * 104 + 84), m->name, 24);
        c++;
    }
    WFIFOW (fd, 2) = c * 104 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルド役職名リスト
 *------------------------------------------
 */
int clif_guild_positionnamelist (struct map_session_data *sd)
{
    int  i, fd;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;
    WFIFOW (fd, 0) = 0x166;
    for (i = 0; i < MAX_GUILDPOSITION; i++)
    {
        WFIFOL (fd, i * 28 + 4) = i;
        memcpy (WFIFOP (fd, i * 28 + 8), g->position[i].name, 24);
    }
    WFIFOW (fd, 2) = i * 28 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルド役職情報リスト
 *------------------------------------------
 */
int clif_guild_positioninfolist (struct map_session_data *sd)
{
    int  i, fd;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;
    WFIFOW (fd, 0) = 0x160;
    for (i = 0; i < MAX_GUILDPOSITION; i++)
    {
        struct guild_position *p = &g->position[i];
        WFIFOL (fd, i * 16 + 4) = i;
        WFIFOL (fd, i * 16 + 8) = p->mode;
        WFIFOL (fd, i * 16 + 12) = i;
        WFIFOL (fd, i * 16 + 16) = p->exp_mode;
    }
    WFIFOW (fd, 2) = i * 16 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルド役職変更通知
 *------------------------------------------
 */
int clif_guild_positionchanged (struct guild *g, int idx)
{
    struct map_session_data *sd;
    unsigned char buf[128];

    nullpo_retr (0, g);

    WBUFW (buf, 0) = 0x174;
    WBUFW (buf, 2) = 44;
    WBUFL (buf, 4) = idx;
    WBUFL (buf, 8) = g->position[idx].mode;
    WBUFL (buf, 12) = idx;
    WBUFL (buf, 16) = g->position[idx].exp_mode;
    memcpy (WBUFP (buf, 20), g->position[idx].name, 24);
    if ((sd = guild_getavailablesd (g)) != NULL)
        clif_send (buf, WBUFW (buf, 2), &sd->bl, GUILD);
    return 0;
}

/*==========================================
 * ギルドメンバ変更通知
 *------------------------------------------
 */
int clif_guild_memberpositionchanged (struct guild *g, int idx)
{
    struct map_session_data *sd;
    unsigned char buf[64];

    nullpo_retr (0, g);

    WBUFW (buf, 0) = 0x156;
    WBUFW (buf, 2) = 16;
    WBUFL (buf, 4) = g->member[idx].account_id;
    WBUFL (buf, 8) = 0;
    WBUFL (buf, 12) = g->member[idx].position;
    if ((sd = guild_getavailablesd (g)) != NULL)
        clif_send (buf, WBUFW (buf, 2), &sd->bl, GUILD);
    return 0;
}

/*==========================================
 * ギルドエンブレム送信
 *------------------------------------------
 */
int clif_guild_emblem (struct map_session_data *sd, struct guild *g)
{
    int  fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, g);

    fd = sd->fd;

    if (g->emblem_len <= 0)
        return 0;
    WFIFOW (fd, 0) = 0x152;
    WFIFOW (fd, 2) = g->emblem_len + 12;
    WFIFOL (fd, 4) = g->guild_id;
    WFIFOL (fd, 8) = g->emblem_id;
    memcpy (WFIFOP (fd, 12), g->emblem_data, g->emblem_len);
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルドスキル送信
 *------------------------------------------
 */
int clif_guild_skillinfo (struct map_session_data *sd)
{
    int  fd;
    int  i, id, c;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;
    WFIFOW (fd, 0) = 0x0162;
    WFIFOW (fd, 4) = g->skill_point;
    for (i = c = 0; i < MAX_GUILDSKILL; i++)
    {
        if (g->skill[i].id > 0)
        {
            WFIFOW (fd, c * 37 + 6) = id = g->skill[i].id;
            WFIFOW (fd, c * 37 + 8) = guild_skill_get_inf (id);
            WFIFOW (fd, c * 37 + 10) = 0;
            WFIFOW (fd, c * 37 + 12) = g->skill[i].lv;
            WFIFOW (fd, c * 37 + 14) =
                guild_skill_get_sp (id, g->skill[i].lv);
            WFIFOW (fd, c * 37 + 16) = guild_skill_get_range (id);
            memset (WFIFOP (fd, c * 37 + 18), 0, 24);
            WFIFOB (fd, c * 37 + 42) =  //up;
                (g->skill[i].lv < guild_skill_get_max (id)) ? 1 : 0;
            c++;
        }
    }
    WFIFOW (fd, 2) = c * 37 + 6;
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルド告知送信
 *------------------------------------------
 */
int clif_guild_notice (struct map_session_data *sd, struct guild *g)
{
    int  fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, g);

    fd = sd->fd;
    if (*g->mes1 == 0 && *g->mes2 == 0)
        return 0;
    WFIFOW (fd, 0) = 0x16f;
    memcpy (WFIFOP (fd, 2), g->mes1, 60);
    memcpy (WFIFOP (fd, 62), g->mes2, 120);
    WFIFOSET (fd, packet_len_table[0x16f]);
    return 0;
}

/*==========================================
 * ギルドメンバ勧誘
 *------------------------------------------
 */
int clif_guild_invite (struct map_session_data *sd, struct guild *g)
{
    int  fd;

    nullpo_retr (0, sd);
    nullpo_retr (0, g);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x16a;
    WFIFOL (fd, 2) = g->guild_id;
    memcpy (WFIFOP (fd, 6), g->name, 24);
    WFIFOSET (fd, packet_len_table[0x16a]);
    return 0;
}

/*==========================================
 * ギルドメンバ勧誘結果
 *------------------------------------------
 */
int clif_guild_inviteack (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x169;
    WFIFOB (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x169]);
    return 0;
}

/*==========================================
 * ギルドメンバ脱退通知
 *------------------------------------------
 */
int clif_guild_leave (struct map_session_data *sd, const char *name,
                      const char *mes)
{
    unsigned char buf[128];

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x15a;
    memcpy (WBUFP (buf, 2), name, 24);
    memcpy (WBUFP (buf, 26), mes, 40);
    clif_send (buf, packet_len_table[0x15a], &sd->bl, GUILD);
    return 0;
}

/*==========================================
 * ギルドメンバ追放通知
 *------------------------------------------
 */
int clif_guild_explusion (struct map_session_data *sd, const char *name,
                          const char *mes, int account_id)
{
    unsigned char buf[128];

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x15c;
    memcpy (WBUFP (buf, 2), name, 24);
    memcpy (WBUFP (buf, 26), mes, 40);
    memcpy (WBUFP (buf, 66), "dummy", 24);
    clif_send (buf, packet_len_table[0x15c], &sd->bl, GUILD);
    return 0;
}

/*==========================================
 * ギルド追放メンバリスト
 *------------------------------------------
 */
int clif_guild_explusionlist (struct map_session_data *sd)
{
    int  fd;
    int  i, c;
    struct guild *g;

    nullpo_retr (0, sd);

    fd = sd->fd;
    g = guild_search (sd->status.guild_id);
    if (g == NULL)
        return 0;
    WFIFOW (fd, 0) = 0x163;
    for (i = c = 0; i < MAX_GUILDEXPLUSION; i++)
    {
        struct guild_explusion *e = &g->explusion[i];
        if (e->account_id > 0)
        {
            memcpy (WFIFOP (fd, c * 88 + 4), e->name, 24);
            memcpy (WFIFOP (fd, c * 88 + 28), e->acc, 24);
            memcpy (WFIFOP (fd, c * 88 + 52), e->mes, 44);
            c++;
        }
    }
    WFIFOW (fd, 2) = c * 88 + 4;
    WFIFOSET (fd, WFIFOW (fd, 2));
    return 0;
}

/*==========================================
 * ギルド会話
 *------------------------------------------
 */
int clif_guild_message (struct guild *g, int account_id, const char *mes,
                        int len)
{
    struct map_session_data *sd;
    unsigned char lbuf[255];
    unsigned char *buf = lbuf;
    if (len + 32 >= sizeof (lbuf))
        buf = malloc (len + 32);
    WBUFW (buf, 0) = 0x17f;
    WBUFW (buf, 2) = len + 4;
    memcpy (WBUFP (buf, 4), mes, len);

    if ((sd = guild_getavailablesd (g)) != NULL)
        clif_send (buf, WBUFW (buf, 2), &sd->bl, GUILD);
    if (buf != lbuf)
        free (buf);
    return 0;
}

/*==========================================
 * ギルドスキル割り振り通知
 *------------------------------------------
 */
int clif_guild_skillup (struct map_session_data *sd, int skill_num, int lv)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x10e;
    WFIFOW (fd, 2) = skill_num;
    WFIFOW (fd, 4) = lv;
    WFIFOW (fd, 6) = guild_skill_get_sp (skill_num, lv);
    WFIFOW (fd, 8) = guild_skill_get_range (skill_num);
    WFIFOB (fd, 10) = 1;
    WFIFOSET (fd, 11);
    return 0;
}

/*==========================================
 * ギルド同盟要請
 *------------------------------------------
 */
int clif_guild_reqalliance (struct map_session_data *sd, int account_id,
                            const char *name)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x171;
    WFIFOL (fd, 2) = account_id;
    memcpy (WFIFOP (fd, 6), name, 24);
    WFIFOSET (fd, packet_len_table[0x171]);
    return 0;
}

/*==========================================
 * ギルド同盟結果
 *------------------------------------------
 */
int clif_guild_allianceack (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x173;
    WFIFOL (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x173]);
    return 0;
}

/*==========================================
 * ギルド関係解消通知
 *------------------------------------------
 */
int clif_guild_delalliance (struct map_session_data *sd, int guild_id,
                            int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x184;
    WFIFOL (fd, 2) = guild_id;
    WFIFOL (fd, 6) = flag;
    WFIFOSET (fd, packet_len_table[0x184]);
    return 0;
}

/*==========================================
 * ギルド敵対結果
 *------------------------------------------
 */
int clif_guild_oppositionack (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x181;
    WFIFOB (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x181]);
    return 0;
}

/*==========================================
 * ギルド関係追加
 *------------------------------------------
 */
/*int clif_guild_allianceadded(struct guild *g,int idx)
{
	unsigned char buf[64];
	WBUFW(fd,0)=0x185;
	WBUFL(fd,2)=g->alliance[idx].opposition;
	WBUFL(fd,6)=g->alliance[idx].guild_id;
	memcpy(WBUFP(fd,10),g->alliance[idx].name,24);
	clif_send(buf,packet_len_table[0x185],guild_getavailablesd(g),GUILD);
	return 0;
}*/

/*==========================================
 * ギルド解散通知
 *------------------------------------------
 */
int clif_guild_broken (struct map_session_data *sd, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x15e;
    WFIFOL (fd, 2) = flag;
    WFIFOSET (fd, packet_len_table[0x15e]);
    return 0;
}

/*==========================================
 * エモーション
 *------------------------------------------
 */
void clif_emotion (struct block_list *bl, int type)
{
    unsigned char buf[8];

    nullpo_retv (bl);

    WBUFW (buf, 0) = 0xc0;
    WBUFL (buf, 2) = bl->id;
    WBUFB (buf, 6) = type;
    clif_send (buf, packet_len_table[0xc0], bl, AREA);
}

static void clif_emotion_towards (struct block_list *bl,
                                  struct block_list *target, int type)
{
    unsigned char buf[8];
    int  len = packet_len_table[0xc0];
    struct map_session_data *sd = (struct map_session_data *) target;

    nullpo_retv (bl);
    nullpo_retv (target);

    if (target->type != BL_PC)
        return;

    WBUFW (buf, 0) = 0xc0;
    WBUFL (buf, 2) = bl->id;
    WBUFB (buf, 6) = type;

    memcpy (WFIFOP (sd->fd, 0), buf, len);
    WFIFOSET (sd->fd, len);
}

/*==========================================
 * トーキーボックス
 *------------------------------------------
 */
void clif_talkiebox (struct block_list *bl, char *talkie)
{
    unsigned char buf[86];

    nullpo_retv (bl);

    WBUFW (buf, 0) = 0x191;
    WBUFL (buf, 2) = bl->id;
    memcpy (WBUFP (buf, 6), talkie, 80);
    clif_send (buf, packet_len_table[0x191], bl, AREA);
}

/*==========================================
 * 結婚エフェクト
 *------------------------------------------
 */
void clif_wedding_effect (struct block_list *bl)
{
    unsigned char buf[6];

    nullpo_retv (bl);

    WBUFW (buf, 0) = 0x1ea;
    WBUFL (buf, 2) = bl->id;
    clif_send (buf, packet_len_table[0x1ea], bl, AREA);
}

/*==========================================
 * あなたに逢いたい使用時名前叫び
 *------------------------------------------

void clif_callpartner(struct map_session_data *sd)
{
	unsigned char buf[26];
	char *p;

	nullpo_retv(sd);

	if(sd->status.partner_id){
		WBUFW(buf,0)=0x1e6;
		p = map_charid2nick(sd->status.partner_id);
		if(p){
			memcpy(WBUFP(buf,2),p,24);
		}else{
			map_reqchariddb(sd,sd->status.partner_id);
			chrif_searchcharid(sd->status.partner_id);
			WBUFB(buf,2) = 0;
		}
		clif_send(buf,packet_len_table[0x1e6]&sd->bl,AREA);
	}
	return;
}
*/
/*==========================================
 * 座る
 *------------------------------------------
 */
void clif_sitting (int fd, struct map_session_data *sd)
{
    unsigned char buf[64];

    nullpo_retv (sd);

    WBUFW (buf, 0) = 0x8a;
    WBUFL (buf, 2) = sd->bl.id;
    WBUFB (buf, 26) = 2;
    clif_send (buf, packet_len_table[0x8a], &sd->bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_disp_onlyself (struct map_session_data *sd, char *mes, int len)
{
    unsigned char lbuf[255];
    unsigned char *buf =
        (len + 32 >= sizeof (lbuf)) ? malloc (len + 32) : lbuf;

    nullpo_retr (0, sd);

    WBUFW (buf, 0) = 0x17f;
    WBUFW (buf, 2) = len + 8;
    memcpy (WBUFP (buf, 4), mes, len + 4);

    clif_send (buf, WBUFW (buf, 2), &sd->bl, SELF);

    if (buf != lbuf)
        free (buf);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */

int clif_GM_kickack (struct map_session_data *sd, int id)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xcd;
    WFIFOL (fd, 2) = id;
    WFIFOSET (fd, packet_len_table[0xcd]);
    return 0;
}

void clif_parse_QuitGame (int fd, struct map_session_data *sd);

int clif_GM_kick (struct map_session_data *sd, struct map_session_data *tsd,
                  int type)
{
    nullpo_retr (0, tsd);

    if (type)
        clif_GM_kickack (sd, tsd->status.account_id);
    tsd->opt1 = tsd->opt2 = 0;
    clif_parse_QuitGame (tsd->fd, tsd);

    return 0;
}

/*==========================================
 * Wis拒否許可応答
 *------------------------------------------
 */
int clif_wisexin (struct map_session_data *sd, int type, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xd1;
    WFIFOB (fd, 2) = type;
    WFIFOB (fd, 3) = flag;
    WFIFOSET (fd, packet_len_table[0xd1]);

    return 0;
}

/*==========================================
 * Wis全拒否許可応答
 *------------------------------------------
 */
int clif_wisall (struct map_session_data *sd, int type, int flag)
{
    int  fd;

    nullpo_retr (0, sd);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0xd2;
    WFIFOB (fd, 2) = type;
    WFIFOB (fd, 3) = flag;
    WFIFOSET (fd, packet_len_table[0xd2]);

    return 0;
}

/*==========================================
 * サウンドエフェクト
 *------------------------------------------
 */
void clif_soundeffect (struct map_session_data *sd, struct block_list *bl,
                       char *name, int type)
{
    int  fd;

    nullpo_retv (sd);
    nullpo_retv (bl);

    fd = sd->fd;
    WFIFOW (fd, 0) = 0x1d3;
    memcpy (WFIFOP (fd, 2), name, 24);
    WFIFOB (fd, 26) = type;
    WFIFOL (fd, 27) = 0;
    WFIFOL (fd, 31) = bl->id;
    WFIFOSET (fd, packet_len_table[0x1d3]);

    return;
}

// displaying special effects (npcs, weather, etc) [Valaris]
int clif_specialeffect (struct block_list *bl, int type, int flag)
{
    unsigned char buf[24];

    nullpo_retr (0, bl);

    memset (buf, 0, packet_len_table[0x19b]);

    WBUFW (buf, 0) = 0x19b;
    WBUFL (buf, 2) = bl->id;
    WBUFL (buf, 6) = type;

    if (flag == 2)
    {
        struct map_session_data *sd = NULL;
        int  i;
        for (i = 0; i < fd_max; i++)
        {
            if (session[i] && (sd = session[i]->session_data) != NULL
                && sd->state.auth && sd->bl.m == bl->m)
                clif_specialeffect (&sd->bl, type, 1);
        }
    }

    else if (flag == 1)
        clif_send (buf, packet_len_table[0x19b], bl, SELF);
    else if (!flag)
        clif_send (buf, packet_len_table[0x19b], bl, AREA);

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
void clif_parse_WantToConnection (int fd, struct map_session_data *sd)
{
    struct map_session_data *old_sd;
    int  account_id;            // account_id in the packet

    if (sd)
    {
        if (battle_config.error_log)
            printf ("clif_parse_WantToConnection : invalid request?\n");
        return;
    }

    if (RFIFOW (fd, 0) == 0x72)
    {
        account_id = RFIFOL (fd, 2);
    }
    else
        return;                 // Not the auth packet

    WFIFOL (fd, 0) = account_id;
    WFIFOSET (fd, 4);

    // if same account already connected, we disconnect the 2 sessions
    if ((old_sd = map_id2sd (account_id)) != NULL)
    {
        clif_authfail_fd (fd, 2);   // same id
        clif_authfail_fd (old_sd->fd, 2);   // same id
        printf
            ("clif_parse_WantToConnection: Double connection for account %d (sessions: #%d (new) and #%d (old)).\n",
             account_id, fd, old_sd->fd);
    }
    else
    {
        sd = session[fd]->session_data = calloc (sizeof (*sd), 1);
        if (sd == NULL)
        {
            printf ("out of memory : clif_parse_WantToConnection\n");
            exit (1);
        }
        sd->fd = fd;

        pc_setnewpc (sd, account_id, RFIFOL (fd, 6), RFIFOL (fd, 10),
                     RFIFOL (fd, 14), RFIFOB (fd, 18), fd);

        map_addiddb (&sd->bl);

        chrif_authreq (sd);
    }

    return;
}

/*==========================================
 * 007d クライアント側マップ読み込み完了
 * map侵入時に必要なデータを全て送りつける
 *------------------------------------------
 */
void clif_parse_LoadEndAck (int fd, struct map_session_data *sd)
{
//  struct item_data* item;
    int  i;
    nullpo_retv (sd);

    if (sd->bl.prev != NULL)
        return;

    // 接続ok時
    //clif_authok();
    if (sd->npc_id)
        npc_event_dequeue (sd);
    clif_skillinfoblock (sd);
    pc_checkitem (sd);
    //guild_info();

    // loadendack時
    // next exp
    clif_updatestatus (sd, SP_NEXTBASEEXP);
    clif_updatestatus (sd, SP_NEXTJOBEXP);
    // skill point
    clif_updatestatus (sd, SP_SKILLPOINT);
    // item
    clif_itemlist (sd);
    clif_equiplist (sd);
    // cart
    if (pc_iscarton (sd))
    {
        clif_cart_itemlist (sd);
        clif_cart_equiplist (sd);
        clif_updatestatus (sd, SP_CARTINFO);
    }
    // param all
    clif_initialstatus (sd);
    // party
    party_send_movemap (sd);
    // guild
    guild_send_memberinfoshort (sd, 1);
    // 119
    // 78

    if (battle_config.pc_invincible_time > 0)
    {
        if (map[sd->bl.m].flag.gvg)
            pc_setinvincibletimer (sd, battle_config.pc_invincible_time << 1);
        else
            pc_setinvincibletimer (sd, battle_config.pc_invincible_time);
    }

    map_addblock (&sd->bl);     // ブロック登録
    clif_spawnpc (sd);          // spawn

    // weight max , now
    clif_updatestatus (sd, SP_MAXWEIGHT);
    clif_updatestatus (sd, SP_WEIGHT);

    // pvp
    if (sd->pvp_timer != -1 && !battle_config.pk_mode)
        delete_timer (sd->pvp_timer, pc_calc_pvprank_timer);
    if (map[sd->bl.m].flag.pvp)
    {
        if (!battle_config.pk_mode)
        {                       // remove pvp stuff for pk_mode [Valaris]
            sd->pvp_timer =
                add_timer (gettick () + 200, pc_calc_pvprank_timer, sd->bl.id,
                           0);
            sd->pvp_rank = 0;
            sd->pvp_lastusers = 0;
            sd->pvp_point = 5;
        }
        clif_set0199 (sd->fd, 1);
    }
    else
    {
        sd->pvp_timer = -1;
    }
    if (map[sd->bl.m].flag.gvg)
    {
        clif_set0199 (sd->fd, 3);
    }

    if (sd->state.connect_new)
    {
        sd->state.connect_new = 0;
        if (sd->status.class != sd->view_class)
            clif_changelook (&sd->bl, LOOK_BASE, sd->view_class);

/*						Stop players from spawning inside castles [Valaris]					*/

        {
            struct guild_castle *gc = guild_mapname2gc (map[sd->bl.m].name);
            if (gc)
                pc_setpos (sd, sd->status.save_point.map,
                           sd->status.save_point.x, sd->status.save_point.y,
                           2);
        }

/*						End Addition [Valaris]			*/

    }

    // view equipment item
    clif_changelook (&sd->bl, LOOK_WEAPON, 0);
    if (battle_config.save_clothcolor == 1 && sd->status.clothes_color > 0)
        clif_changelook (&sd->bl, LOOK_CLOTHES_COLOR,
                         sd->status.clothes_color);

    if (sd->status.hp < sd->status.max_hp >> 2
        && pc_checkskill (sd, SM_AUTOBERSERK) > 0
        && (sd->sc_data[SC_PROVOKE].timer == -1
            || sd->sc_data[SC_PROVOKE].val2 == 0))
        // オートバーサーク発動
        skill_status_change_start (&sd->bl, SC_PROVOKE, 10, 1, 0, 0, 0, 0);

//  if(time(&timer) < ((weddingtime=pc_readglobalreg(sd,"PC_WEDDING_TIME")) + 3600))
//      skill_status_change_start(&sd->bl,SC_WEDDING,0,weddingtime,0,0,36000,0);

    if (battle_config.muting_players && sd->status.manner < 0)
        skill_status_change_start (&sd->bl, SC_NOCHAT, 0, 0, 0, 0, 0, 0);

    // option
    clif_changeoption (&sd->bl);
    if (sd->sc_data[SC_TRICKDEAD].timer != -1)
        skill_status_change_end (&sd->bl, SC_TRICKDEAD, -1);
    if (sd->sc_data[SC_SIGNUMCRUCIS].timer != -1
        && !battle_check_undead (7, sd->def_ele))
        skill_status_change_end (&sd->bl, SC_SIGNUMCRUCIS, -1);
    if (sd->special_state.infinite_endure
        && sd->sc_data[SC_ENDURE].timer == -1)
        skill_status_change_start (&sd->bl, SC_ENDURE, 10, 1, 0, 0, 0, 0);
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].equip
            && sd->status.inventory[i].equip & 0x0002
            && sd->status.inventory[i].broken == 1)
            skill_status_change_start (&sd->bl, SC_BROKNWEAPON, 0, 0, 0, 0, 0,
                                       0);
        if (sd->status.inventory[i].equip
            && sd->status.inventory[i].equip & 0x0010
            && sd->status.inventory[i].broken == 1)
            skill_status_change_start (&sd->bl, SC_BROKNARMOR, 0, 0, 0, 0, 0,
                                       0);
    }

//        clif_changelook_accessories(sd, NULL);

    map_foreachinarea (clif_getareachar, sd->bl.m, sd->bl.x - AREA_SIZE,
                       sd->bl.y - AREA_SIZE, sd->bl.x + AREA_SIZE,
                       sd->bl.y + AREA_SIZE, 0, sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TickSend (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    sd->client_tick = RFIFOL (fd, 2);
    sd->server_tick = gettick ();
    clif_servertick (sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WalkToXY (int fd, struct map_session_data *sd)
{
    int  x, y;

    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }

    if (sd->npc_id != 0 || sd->state.storage_flag)
        return;

    if (sd->skilltimer != -1 && pc_checkskill (sd, SA_FREECAST) <= 0)   // フリーキャスト
        return;

    if (sd->chatID)
        return;

    if (sd->canmove_tick > gettick ())
        return;

    // ステータス異常やハイディング中(トンネルドライブ無)で動けない
    if ((sd->opt1 > 0 && sd->opt1 != 6) || sd->sc_data[SC_ANKLE].timer != -1 || //アンクルスネア
        sd->sc_data[SC_AUTOCOUNTER].timer != -1 ||  //オートカウンター
        sd->sc_data[SC_TRICKDEAD].timer != -1 ||    //死んだふり
        sd->sc_data[SC_BLADESTOP].timer != -1 ||    //白刃取り
        sd->sc_data[SC_SPIDERWEB].timer != -1 ||    //スパイダーウェッブ
        (sd->sc_data[SC_DANCING].timer != -1 && sd->sc_data[SC_DANCING].val4))  //合奏スキル演奏中は動けない
        return;
    if ((sd->status.option & 2) && pc_checkskill (sd, RG_TUNNELDRIVE) <= 0)
        return;

    if (sd->invincible_timer != -1)
        pc_delinvincibletimer (sd);

    pc_stopattack (sd);

    x = RFIFOB (fd, 2) * 4 + (RFIFOB (fd, 3) >> 6);
    y = ((RFIFOB (fd, 3) & 0x3f) << 4) + (RFIFOB (fd, 4) >> 4);
    pc_walktoxy (sd, x, y);

}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_QuitGame (int fd, struct map_session_data *sd)
{
    unsigned int tick = gettick ();
    struct skill_unit_group *sg;

    nullpo_retv (sd);

    WFIFOW (fd, 0) = 0x18b;
    if ((!pc_isdead (sd)
         && (sd->opt1
             || (sd->opt2 && !(night_flag == 1 && sd->opt2 == STATE_BLIND))))
        || sd->skilltimer != -1 || (DIFF_TICK (tick, sd->canact_tick) < 0)
        || (sd->sc_data && sd->sc_data[SC_DANCING].timer != -1
            && sd->sc_data[SC_DANCING].val4
            && (sg = (struct skill_unit_group *) sd->sc_data[SC_DANCING].val2)
            && sg->src_id == sd->bl.id))
    {
        WFIFOW (fd, 2) = 1;
        WFIFOSET (fd, packet_len_table[0x18b]);
        return;
    }

    /*  Rovert's prevent logout option fixed [Valaris]  */
    if ((battle_config.prevent_logout
         && (gettick () - sd->canlog_tick) >= 10000)
        || (!battle_config.prevent_logout))
    {
        clif_setwaitclose (fd);
        WFIFOW (fd, 2) = 0;
    }
    else
    {
        WFIFOW (fd, 2) = 1;
    }
    WFIFOSET (fd, packet_len_table[0x18b]);

}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GetCharNameRequest (int fd, struct map_session_data *sd)
{
    struct block_list *bl;
    int  account_id;

    account_id = RFIFOL (fd, 2);
    bl = map_id2bl (account_id);
    if (bl == NULL)
        return;

    WFIFOW (fd, 0) = 0x95;
    WFIFOL (fd, 2) = account_id;

    switch (bl->type)
    {
        case BL_PC:
        {
            struct map_session_data *ssd = (struct map_session_data *) bl;

            nullpo_retv (ssd);

            if (ssd->state.shroud_active)
                memset (WFIFOP (fd, 6), 0, 24);
            else
                memcpy (WFIFOP (fd, 6), ssd->status.name, 24);
            WFIFOSET (fd, packet_len_table[0x95]);

            struct guild *g = NULL;
            struct party *p = NULL;

            char *guild_name = "", *guild_pos = "", *party_name = "";

            int send = 0;

            if (ssd->status.guild_id > 0 && (g = guild_search (ssd->status.guild_id)) != NULL)
            {
                // there used to be a comment near here, but the code has changed slightly
                // ギルド所属ならパケット0195を返す
                // google says that means: 0195 return if the packet belongs Guild
                int  i, ps = -1;
                for (i = 0; i < g->max_member; i++)
                {
                    if (g->member[i].account_id == ssd->status.account_id)
                        ps = g->member[i].position;
                }
                if (ps >= 0 && ps < MAX_GUILDPOSITION)
                {
                    guild_name = g->name;
                    guild_pos = g->position[ps].name;
                    send = 1;
                }
            }
            if (ssd->status.party_id > 0 && (p = party_search (ssd->status.party_id)) != NULL)
            {
                party_name = p->name;
                send = 1;
            }

            if (send)
            {
                WFIFOW (fd, 0) = 0x195;
                WFIFOL (fd, 2) = account_id;
                memcpy (WFIFOP (fd, 6), party_name, 24);
                memcpy (WFIFOP (fd, 30), guild_name, 24);
                memcpy (WFIFOP (fd, 54), guild_pos, 24);
                memcpy (WFIFOP (fd, 78), guild_pos, 24); // We send this value twice because the client expects it
                WFIFOSET (fd, packet_len_table[0x195]);

            }

            if (pc_isGM(sd) >= battle_config.hack_info_GM_level)
            {
                in_addr_t ip = ssd->ip;
                WFIFOW (fd, 0) = 0x20C;

                // Mask the IP using the char-server password
                if (battle_config.mask_ip_gms)
                    ip = MD5_ip(chrif_getpasswd (), ssd->ip);

                WFIFOL (fd, 2) = account_id;
                WFIFOL (fd, 6) = ip;
                WFIFOSET (fd, packet_len_table[0x20C]);
             }

        }
            break;
        case BL_NPC:
            memcpy (WFIFOP (fd, 6), ((struct npc_data *) bl)->name, 24);
            {
                char *start = WFIFOP (fd, 6);
                char *end = strchr (start, '#');    // [fate] elim hashed out/invisible names for the client
                if (end)
                    while (*end)
                        *end++ = 0;

                // [fate] Elim preceding underscores for (hackish) name position fine-tuning
                while (*start == '_')
                    *start++ = ' ';
            }
            WFIFOSET (fd, packet_len_table[0x95]);
            break;
        case BL_MOB:
        {
            struct mob_data *md = (struct mob_data *) bl;

            nullpo_retv (md);

            memcpy (WFIFOP (fd, 6), md->name, 24);
            WFIFOSET (fd, packet_len_table[0x95]);
        }
            break;
        default:
            if (battle_config.error_log)
                printf ("clif_parse_GetCharNameRequest : bad type %d(%d)\n",
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
void clif_parse_GlobalMessage (int fd, struct map_session_data *sd)
{
    int msg_len = RFIFOW (fd, 2) - 4; /* Header (2) + length (2). */
    size_t message_len = 0;
    char *buf = NULL;
    char *message = NULL;   /* The message text only. */

    nullpo_retv (sd);

    if (!(buf = clif_validate_chat (sd, 2, &message, &message_len)))
    {
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }

    if (is_atcommand (fd, sd, message, 0) != AtCommand_None
            || (sd->sc_data && (sd->sc_data[SC_BERSERK].timer != -1 //バーサーク時は会話も不可
                                || sd->sc_data[SC_NOCHAT].timer != -1)))//チャット禁止
    {
        free (buf);
        return;
    }

    if (!magic_message (sd, buf, msg_len))
    {
        /* Don't send chat that results in an automatic ban. */
        if (tmw_CheckChatSpam (sd, message))
        {
            free (buf);
            /* "Your message could not be sent." */
            clif_displaymessage (fd, msg_txt (505));
            return;
        }

        /* It's not a spell/magic message, so send the message to others. */
        WBUFW (buf, 0) = 0x8d;
        WBUFW (buf, 2) = msg_len + 8;   /* Header (2) + length (2) + ID (4). */
        WBUFL (buf, 4) = sd->bl.id;

        clif_send (buf, msg_len + 8, &sd->bl,
                   sd->chatID ? CHAT_WOS : AREA_CHAT_WOC);
    }

    /* Send the message back to the speaker. */
    memcpy (WFIFOP (fd, 0), RFIFOP (fd, 0), RFIFOW (fd, 2));
    WFIFOW (fd, 0) = 0x8e;
    WFIFOSET (fd, WFIFOW (fd, 2));

    free (buf);
    return;
}

int clif_message (struct block_list *bl, char *msg)
{
    unsigned short msg_len = strlen (msg) + 1;
    unsigned char buf[512];

    if (msg_len + 16 > 512)
        return 0;

    nullpo_retr (0, bl);

    WBUFW (buf, 0) = 0x8d;
    WBUFW (buf, 2) = msg_len + 8;
    WBUFL (buf, 4) = bl->id;
    memcpy (WBUFP (buf, 8), msg, msg_len);

    clif_send (buf, WBUFW (buf, 2), bl, AREA);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_MapMove (int fd, struct map_session_data *sd)
{
// /m /mapmove (as @rura GM command)
    char output[100];
    char map_name[17];

    nullpo_retv (sd);

    memset (output, '\0', sizeof (output));
    memset (map_name, '\0', sizeof (map_name));

    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_MapMove)))
    {
        memcpy (map_name, RFIFOP (fd, 2), 16);
        sprintf (output, "%s %d %d", map_name, RFIFOW (fd, 18),
                 RFIFOW (fd, 20));
        log_atcommand (sd, "@warp %s", output);
        atcommand_warp (fd, sd, "@warp", output);
    }

    return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeDir (int fd, struct map_session_data *sd)
{
    unsigned char buf[64];
    short dir;

    nullpo_retv (sd);

//  RFIFOW(fd,2); // Apparently does nothing?
    dir = RFIFOB (fd, 4);

    if (dir == sd->dir)
        return;

    pc_setdir (sd, dir);

    WBUFW (buf, 0) = 0x9c;
    WBUFL (buf, 2) = sd->bl.id;
    WBUFW (buf, 6) = 0;
    WBUFB (buf, 8) = dir;
    if (sd->disguise > 23 && sd->disguise < 4001)   // mob disguises [Valaris]
        clif_send (buf, packet_len_table[0x9c], &sd->bl, AREA);
    else
        clif_send (buf, packet_len_table[0x9c], &sd->bl, AREA_WOS);

}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Emotion (int fd, struct map_session_data *sd)
{
    unsigned char buf[64];

    nullpo_retv (sd);

    if (battle_config.basic_skill_check == 0
        || pc_checkskill (sd, NV_EMOTE) >= 1)
    {
        WBUFW (buf, 0) = 0xc0;
        WBUFL (buf, 2) = sd->bl.id;
        WBUFB (buf, 6) = RFIFOB (fd, 2);
        clif_send (buf, packet_len_table[0xc0], &sd->bl, AREA);
    }
    else
        clif_skill_fail (sd, 1, 0, 1);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_HowManyConnections (int fd, struct map_session_data *sd)
{
    WFIFOW (fd, 0) = 0xc2;
    WFIFOL (fd, 2) = map_getusers ();
    WFIFOSET (fd, packet_len_table[0xc2]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ActionRequest (int fd, struct map_session_data *sd)
{
    unsigned int tick;
    unsigned char buf[64];
    int  action_type, target_id;

    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0 || sd->opt1 > 0 || sd->status.option & 2 || sd->state.storage_flag ||
            (sd->sc_data && (sd->sc_data[SC_AUTOCOUNTER].timer != -1 ||   //オートカウンター
            sd->sc_data[SC_BLADESTOP].timer != -1 || //白刃取り
            sd->sc_data[SC_DANCING].timer != -1)))
        return;

    tick = gettick ();

    pc_stop_walking (sd, 0);
    pc_stopattack (sd);

    target_id = RFIFOL (fd, 2);
    action_type = RFIFOB (fd, 6);

    switch (action_type)
    {
        case 0x00:             // once attack
        case 0x07:             // continuous attack
            if (sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22
                || sd->status.option & OPTION_HIDE)
                return;
            if (!battle_config.sdelay_attack_enable
                && pc_checkskill (sd, SA_FREECAST) <= 0)
            {
                if (DIFF_TICK (tick, sd->canact_tick) < 0)
                {
                    clif_skill_fail (sd, 1, 4, 0);
                    return;
                }
            }
            if (sd->invincible_timer != -1)
                pc_delinvincibletimer (sd);
            if (sd->attacktarget > 0)   // [Valaris]
                sd->attacktarget = 0;
            pc_attack (sd, target_id, action_type != 0);
            break;
        case 0x02:             // sitdown
            pc_stop_walking (sd, 1);
            skill_gangsterparadise (sd, 1); // ギャングスターパラダイス設定
            pc_setsit (sd);
            clif_sitting (fd, sd);
            break;
        case 0x03:             // standup
            skill_gangsterparadise (sd, 0); // ギャングスターパラダイス解除
            pc_setstand (sd);
            WBUFW (buf, 0) = 0x8a;
            WBUFL (buf, 2) = sd->bl.id;
            WBUFB (buf, 26) = 3;
            clif_send (buf, packet_len_table[0x8a], &sd->bl, AREA);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Restart (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    switch (RFIFOB (fd, 2))
    {
        case 0x00:
            if (pc_isdead (sd))
            {
                pc_setstand (sd);
                pc_setrestartvalue (sd, 3);
                pc_setpos (sd, sd->status.save_point.map,
                           sd->status.save_point.x, sd->status.save_point.y,
                           2);
            }
            break;
        case 0x01:
            /*if(!pc_isdead(sd) && (sd->opt1 || (sd->opt2 && !(night_flag == 1 && sd->opt2 == STATE_BLIND))))
             * return; */

            /*  Rovert's Prevent logout option - Fixed [Valaris]    */
            if ((battle_config.prevent_logout
                 && (gettick () - sd->canlog_tick) >= 10000)
                || (!battle_config.prevent_logout))
            {
                chrif_charselectreq (sd);
            }
            else
            {
                WFIFOW (fd, 0) = 0x18b;
                WFIFOW (fd, 2) = 1;

                WFIFOSET (fd, packet_len_table[0x018b]);
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
void clif_parse_Wis (int fd, struct map_session_data *sd)
{
    size_t message_len = 0;
    char *buf = NULL;
    char *message = NULL;   /* The message text only. */
    struct map_session_data *dstsd = NULL;

    nullpo_retv (sd);

    if (!(buf = clif_validate_chat (sd, 1, &message, &message_len)))
    {
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }

    if (is_atcommand (fd, sd, message, 0) != AtCommand_None
            || (sd->sc_data && (sd->sc_data[SC_BERSERK].timer != -1
                                || sd->sc_data[SC_NOCHAT].timer != -1)))
    {
        free (buf);
        return;
    }

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam (sd, message))
    {
        free (buf);
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }

    /*
     * The player is not on this server. Only send the whisper if the name is
     * exactly the same, because if there are multiple map-servers and a name
     * conflict (for instance, "Test" versus "test"), the char-server must
     * settle the discrepancy.
     */
    if (!(dstsd = map_nick2sd (RFIFOP (fd, 4)))
            || strcmp (dstsd->status.name, RFIFOP (fd, 4)) != 0)
        intif_wis_message (sd, RFIFOP (fd, 4), message,  RFIFOW (fd, 2) - 28);
    else
    {
        /* Refuse messages addressed to self. */
        if (dstsd->fd == fd)
        {
            /* "You cannot page yourself." */
            char *mes = msg_txt (504);
            clif_wis_message (fd, wisp_server_name, mes, strlen (mes) + 1);
        }
        else
        {
            /* The target is ignoring all whispers. */
            if (dstsd->ignoreAll == 1)
                /* Ignored by target. */
                clif_wis_end (fd, 2);
            else
            {
                int i;
                size_t end = sizeof (dstsd->ignore) / sizeof (dstsd->ignore[0]);

                /* See if the source player is being ignored. */
                for (i = 0; i < end; ++i)
                    if (strcmp (dstsd->ignore[i].name, sd->status.name) == 0)
                    {
                        /* Ignored by target. */
                        clif_wis_end (fd, 2);
                        break;
                    }

                /* The player is not being ignored. */
                if (i == end)
                {
                    clif_wis_message (dstsd->fd, sd->status.name, message,
                                      RFIFOW (fd, 2) - 28);
                    /* The whisper was sent successfully. */
                    clif_wis_end (fd, 0);
                }
            }
        }
    }

    free (buf);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GMmessage (int fd, struct map_session_data *sd)
{
    char m[512];
    char output[200];
    nullpo_retv (sd);

    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_Broadcast)))
    {
        strncpy (m, RFIFOP (fd, 4), RFIFOW (fd, 2) - 4);
        m[RFIFOW (fd, 2) - 4] = 0;
        log_atcommand (sd, "/announce %s", m);

        memset (output, '\0', sizeof (output));
        snprintf (output, 199, "%s : %s", sd->status.name, m);

        intif_GMmessage (output, strlen (output) + 1, 0);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TakeItem (int fd, struct map_session_data *sd)
{
    struct flooritem_data *fitem;
    int  map_object_id;

    nullpo_retv (sd);

    map_object_id = RFIFOL (fd, 2);
    fitem = (struct flooritem_data *) map_id2bl (map_object_id);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }

    if (sd->npc_id != 0 || sd->opt1 > 0 || (sd->sc_data &&
            (sd->sc_data[SC_TRICKDEAD].timer != -1 ||    //死んだふり
            sd->sc_data[SC_BLADESTOP].timer != -1 ||    //白刃取り
            sd->sc_data[SC_BERSERK].timer != -1 ||  //バーサーク
            sd->sc_data[SC_NOCHAT].timer != -1)))   //会話禁止
        return;

    if (fitem == NULL || fitem->bl.m != sd->bl.m)
        return;

    if (abs (sd->bl.x - fitem->bl.x) >= 2
        || abs (sd->bl.y - fitem->bl.y) >= 2)
        return;                 // too far away to pick up

    if (sd->state.shroud_active && sd->state.shroud_disappears_on_pickup)
        magic_unshroud (sd);

    pc_takeitem (sd, fitem);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_DropItem (int fd, struct map_session_data *sd)
{
    int  item_index, item_amount;

    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0 || sd->opt1 > 0 || map[sd->bl.m].flag.no_player_drops ||
            (sd->sc_data && (sd->sc_data[SC_AUTOCOUNTER].timer != -1 ||    //オートカウンター
            sd->sc_data[SC_BLADESTOP].timer != -1 ||  //白刃取り
            sd->sc_data[SC_BERSERK].timer != -1)))    //バーサーク
        return;

    item_index = RFIFOW (fd, 2) - 2;
    item_amount = RFIFOW (fd, 4);

    pc_dropitem (sd, item_index, item_amount);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UseItem (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0 || sd->opt1 > 0 || (sd->sc_data &&
            (sd->sc_data[SC_TRICKDEAD].timer != -1 ||    //死んだふり
            sd->sc_data[SC_BLADESTOP].timer != -1 ||    //白刃取り
            sd->sc_data[SC_BERSERK].timer != -1 ||  //バーサーク
            sd->sc_data[SC_NOCHAT].timer != -1)))   //会話禁止
        return;

    if (sd->invincible_timer != -1)
        pc_delinvincibletimer (sd);

    pc_useitem (sd, RFIFOW (fd, 2) - 2);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_EquipItem (int fd, struct map_session_data *sd)
{
    int  index;

    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }
    index = RFIFOW (fd, 2) - 2;
    if (sd->npc_id != 0)
        return;
    if (sd->sc_data
        && (sd->sc_data[SC_BLADESTOP].timer != -1
            || sd->sc_data[SC_BERSERK].timer != -1))
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
        if (sd->inventory_data[index]->type == 10)
            RFIFOW (fd, 4) = 0x8000;    // 矢を無理やり装備できるように（−−；
        pc_equipitem (sd, index, RFIFOW (fd, 4));
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UnequipItem (int fd, struct map_session_data *sd)
{
    int  index;

    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }
    index = RFIFOW (fd, 2) - 2;
    if (sd->status.inventory[index].broken == 1 && sd->sc_data
        && sd->sc_data[SC_BROKNWEAPON].timer != -1)
        skill_status_change_end (&sd->bl, SC_BROKNWEAPON, -1);
    if (sd->status.inventory[index].broken == 1 && sd->sc_data
        && sd->sc_data[SC_BROKNARMOR].timer != -1)
        skill_status_change_end (&sd->bl, SC_BROKNARMOR, -1);
    if (sd->sc_data
        && (sd->sc_data[SC_BLADESTOP].timer != -1
            || sd->sc_data[SC_BERSERK].timer != -1))
        return;

    if (sd->npc_id != 0 || sd->opt1 > 0)
        return;
    pc_unequipitem (sd, index, 0);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcClicked (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (pc_isdead (sd))
    {
        clif_clearchar_area (&sd->bl, 1);
        return;
    }
    if (sd->npc_id != 0)
        return;
    npc_click (sd, RFIFOL (fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuySellSelected (int fd, struct map_session_data *sd)
{
    npc_buysellsel (sd, RFIFOL (fd, 2), RFIFOB (fd, 6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuyListSend (int fd, struct map_session_data *sd)
{
    int  fail = 0, n;
    unsigned short *item_list;

    n = (RFIFOW (fd, 2) - 4) / 4;
    item_list = (unsigned short *) RFIFOP (fd, 4);

    fail = npc_buylist (sd, n, item_list);

    WFIFOW (fd, 0) = 0xca;
    WFIFOB (fd, 2) = fail;
    WFIFOSET (fd, packet_len_table[0xca]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSellListSend (int fd, struct map_session_data *sd)
{
    int  fail = 0, n;
    unsigned short *item_list;

    n = (RFIFOW (fd, 2) - 4) / 4;
    item_list = (unsigned short *) RFIFOP (fd, 4);

    fail = npc_selllist (sd, n, item_list);

    WFIFOW (fd, 0) = 0xcb;
    WFIFOB (fd, 2) = fail;
    WFIFOSET (fd, packet_len_table[0xcb]);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_CreateChatRoom (int fd, struct map_session_data *sd)
{
    chat_createchat (sd, RFIFOW (fd, 4), RFIFOB (fd, 6), RFIFOP (fd, 7),
                     RFIFOP (fd, 15), RFIFOW (fd, 2) - 15);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatAddMember (int fd, struct map_session_data *sd)
{
    chat_joinchat (sd, RFIFOL (fd, 2), RFIFOP (fd, 6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatRoomStatusChange (int fd, struct map_session_data *sd)
{
    chat_changechatstatus (sd, RFIFOW (fd, 4), RFIFOB (fd, 6), RFIFOP (fd, 7),
                           RFIFOP (fd, 15), RFIFOW (fd, 2) - 15);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeChatOwner (int fd, struct map_session_data *sd)
{
    chat_changechatowner (sd, RFIFOP (fd, 6));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_KickFromChat (int fd, struct map_session_data *sd)
{
    chat_kickchat (sd, RFIFOP (fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatLeave (int fd, struct map_session_data *sd)
{
    chat_leavechat (sd);
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void clif_parse_TradeRequest (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (battle_config.basic_skill_check == 0
        || pc_checkskill (sd, NV_TRADE) >= 1)
    {
        trade_traderequest (sd, RFIFOL (sd->fd, 2));
    }
    else
        clif_skill_fail (sd, 1, 0, 0);
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void clif_parse_TradeAck (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    trade_tradeack (sd, RFIFOB (sd->fd, 2));
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
void clif_parse_TradeAddItem (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    trade_tradeadditem (sd, RFIFOW (sd->fd, 2), RFIFOL (sd->fd, 4));
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
void clif_parse_TradeOk (int fd, struct map_session_data *sd)
{
    trade_tradeok (sd);
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void clif_parse_TradeCansel (int fd, struct map_session_data *sd)
{
    trade_tradecancel (sd);
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void clif_parse_TradeCommit (int fd, struct map_session_data *sd)
{
    trade_tradecommit (sd);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_StopAttack (int fd, struct map_session_data *sd)
{
    pc_stopattack (sd);
}

/*==========================================
 * カートへアイテムを移す
 *------------------------------------------
 */
void clif_parse_PutItemToCart (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (sd->npc_id != 0 || sd->trade_partner != 0)
        return;
    pc_putitemtocart (sd, RFIFOW (fd, 2) - 2, RFIFOL (fd, 4));
}

/*==========================================
 * カートからアイテムを出す
 *------------------------------------------
 */
void clif_parse_GetItemFromCart (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (sd->npc_id != 0 || sd->trade_partner != 0)
        return;
    pc_getitemfromcart (sd, RFIFOW (fd, 2) - 2, RFIFOL (fd, 4));
}

/*==========================================
 * 付属品(鷹,ペコ,カート)をはずす
 *------------------------------------------
 */
void clif_parse_RemoveOption (int fd, struct map_session_data *sd)
{
    if (pc_isriding (sd))
    {                           // jobchange when removing peco [Valaris]
        if (sd->status.class == 13)
            sd->status.class = sd->view_class = 7;

        if (sd->status.class == 21)
            sd->status.class = sd->view_class = 14;

        if (sd->status.class == 4014)
            sd->status.class = sd->view_class = 4008;

        if (sd->status.class == 4022)
            sd->status.class = sd->view_class = 4015;
    }

    pc_setoption (sd, 0);
}

/*==========================================
 * チェンジカート
 *------------------------------------------
 */
void clif_parse_ChangeCart (int fd, struct map_session_data *sd)
{
    pc_setcart (sd, RFIFOW (fd, 2));
}

/*==========================================
 * ステータスアップ
 *------------------------------------------
 */
void clif_parse_StatusUp (int fd, struct map_session_data *sd)
{
    pc_statusup (sd, RFIFOW (fd, 2));
}

/*==========================================
 * スキルレベルアップ
 *------------------------------------------
 */
void clif_parse_SkillUp (int fd, struct map_session_data *sd)
{
    pc_skillup (sd, RFIFOW (fd, 2));
}

/*==========================================
 * スキル使用（ID指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToId (int fd, struct map_session_data *sd)
{
    int  skillnum, skilllv, lv, target_id;
    unsigned int tick = gettick ();

    nullpo_retv (sd);

    if (map[sd->bl.m].flag.noskill)
        return;
    if (sd->chatID || sd->npc_id != 0 || sd->state.storage_flag)
        return;

    skilllv = RFIFOW (fd, 2);
    skillnum = RFIFOW (fd, 4);
    target_id = RFIFOL (fd, 6);

    if (sd->skilltimer != -1)
    {
        if (skillnum != SA_CASTCANCEL)
            return;
    }
    else if (DIFF_TICK (tick, sd->canact_tick) < 0)
    {
        clif_skill_fail (sd, skillnum, 4, 0);
        return;
    }

    if ((sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
        sd->sc_data[SC_BERSERK].timer != -1
        || sd->sc_data[SC_NOCHAT].timer != -1
        || sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
        return;
    if (sd->invincible_timer != -1)
        pc_delinvincibletimer (sd);
    if (sd->skillitem >= 0 && sd->skillitem == skillnum)
    {
        if (skilllv != sd->skillitemlv)
            skilllv = sd->skillitemlv;
        skill_use_id (sd, target_id, skillnum, skilllv);
    }
    else
    {
        sd->skillitem = sd->skillitemlv = -1;
        if (skillnum == MO_EXTREMITYFIST)
        {
            if ((sd->sc_data[SC_COMBO].timer == -1
                 || (sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH
                     && sd->sc_data[SC_COMBO].val1 != CH_CHAINCRUSH)))
            {
                if (!sd->state.skill_flag)
                {
                    sd->state.skill_flag = 1;
                    clif_skillinfo (sd, MO_EXTREMITYFIST, 1, -1);
                    return;
                }
                else if (sd->bl.id == target_id)
                {
                    clif_skillinfo (sd, MO_EXTREMITYFIST, 1, -1);
                    return;
                }
            }
        }
        if ((lv = pc_checkskill (sd, skillnum)) > 0)
        {
            if (skilllv > lv)
                skilllv = lv;
            skill_use_id (sd, target_id, skillnum, skilllv);
            if (sd->state.skill_flag)
                sd->state.skill_flag = 0;
        }
    }
}

/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToPos (int fd, struct map_session_data *sd)
{
    int  skillnum, skilllv, lv, x, y;
    unsigned int tick = gettick ();
    int  skillmoreinfo;

    nullpo_retv (sd);

    if (map[sd->bl.m].flag.noskill)
        return;
    if (sd->npc_id != 0 || sd->state.storage_flag)
        return;
    if (sd->chatID)
        return;

    skillmoreinfo = -1;
    skilllv = RFIFOW (fd, 2);
    skillnum = RFIFOW (fd, 4);
    x = RFIFOW (fd, 6);
    y = RFIFOW (fd, 8);
    if (RFIFOW (fd, 0) == 0x190)
        skillmoreinfo = 10;

    if (skillmoreinfo != -1)
    {
        if (pc_issit (sd))
        {
            clif_skill_fail (sd, skillnum, 0, 0);
            return;
        }
        memcpy (talkie_mes, RFIFOP (fd, skillmoreinfo), 80);
    }

    if (sd->skilltimer != -1)
        return;
    else if (DIFF_TICK (tick, sd->canact_tick) < 0)
    {
        clif_skill_fail (sd, skillnum, 4, 0);
        return;
    }

    if ((sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
        sd->sc_data[SC_BERSERK].timer != -1
        || sd->sc_data[SC_NOCHAT].timer != -1
        || sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
        return;
    if (sd->invincible_timer != -1)
        pc_delinvincibletimer (sd);
    if (sd->skillitem >= 0 && sd->skillitem == skillnum)
    {
        if (skilllv != sd->skillitemlv)
            skilllv = sd->skillitemlv;
        skill_use_pos (sd, x, y, skillnum, skilllv);
    }
    else
    {
        sd->skillitem = sd->skillitemlv = -1;
        if ((lv = pc_checkskill (sd, skillnum)) > 0)
        {
            if (skilllv > lv)
                skilllv = lv;
            skill_use_pos (sd, x, y, skillnum, skilllv);
        }
    }
}

/*==========================================
 * スキル使用（map指定）
 *------------------------------------------
 */
void clif_parse_UseSkillMap (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (map[sd->bl.m].flag.noskill)
        return;
    if (sd->chatID)
        return;

    if (sd->npc_id != 0 || (sd->sc_data &&
                            (sd->sc_data[SC_TRICKDEAD].timer != -1 ||
                             sd->sc_data[SC_BERSERK].timer != -1 ||
                             sd->sc_data[SC_NOCHAT].timer != -1 ||
                             sd->sc_data[SC_WEDDING].timer != -1 ||
                             sd->view_class == 22)))
        return;

    if (sd->invincible_timer != -1)
        pc_delinvincibletimer (sd);

    skill_castend_map (sd, RFIFOW (fd, 2), RFIFOP (fd, 4));
}

/*==========================================
 * メモ要求
 *------------------------------------------
 */
void clif_parse_RequestMemo (int fd, struct map_session_data *sd)
{
    pc_memo (sd, -1);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSelectMenu (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    sd->npc_menu = RFIFOB (fd, 6);
    map_scriptcont (sd, RFIFOL (fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcNextClicked (int fd, struct map_session_data *sd)
{
    map_scriptcont (sd, RFIFOL (fd, 2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcAmountInput (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

#define RFIFOL_(fd,pos) (*(int*)(session[fd]->rdata+session[fd]->rdata_pos+(pos)))
    //Input Value overflow Exploit FIX
    sd->npc_amount = RFIFOL_ (fd, 6);   //fixed by Lupus. npc_amount is (int) but was RFIFOL changing it to (unsigned int)

#undef RFIFOL_

    map_scriptcont (sd, RFIFOL (fd, 2));
}

/*==========================================
 * Process string-based input for an NPC.
 *
 * (S 01d5 <len>.w <npc_ID>.l <message>.?B)
 *------------------------------------------
 */
void clif_parse_NpcStringInput (int fd, struct map_session_data *sd)
{
    int  len;
    nullpo_retv (sd);

    len = RFIFOW (fd, 2) - 8;

    /*
     * If we check for equal to 0, too, we'll freeze clients that send (or
     * claim to have sent) an "empty" message.
     */
    if (len < 0)
        return;

    if (len >= sizeof (sd->npc_str) - 1)
    {
        printf ("clif_parse_NpcStringInput(): Input string too long!\n");
        len = sizeof (sd->npc_str) - 1;
    }

    if (len > 0)
        strncpy (sd->npc_str, RFIFOP (fd, 8), len);
    sd->npc_str[len] = '\0';

    map_scriptcont (sd, RFIFOL (fd, 4));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcCloseClicked (int fd, struct map_session_data *sd)
{
    map_scriptcont (sd, RFIFOL (fd, 2));
}

/*==========================================
 * アイテム鑑定
 *------------------------------------------
 */
void clif_parse_ItemIdentify (int fd, struct map_session_data *sd)
{
    pc_item_identify (sd, RFIFOW (fd, 2) - 2);
}

/*==========================================
 * オートスペル受信
 *------------------------------------------
 */
void clif_parse_AutoSpell (int fd, struct map_session_data *sd)
{
    skill_autospell (sd, RFIFOW (fd, 2));
}

/*==========================================
 * カード使用
 *------------------------------------------
 */
void clif_parse_UseCard (int fd, struct map_session_data *sd)
{
    clif_use_card (sd, RFIFOW (fd, 2) - 2);
}

/*==========================================
 * カード挿入装備選択
 *------------------------------------------
 */
void clif_parse_InsertCard (int fd, struct map_session_data *sd)
{
    pc_insert_card (sd, RFIFOW (fd, 2) - 2, RFIFOW (fd, 4) - 2);
}

/*==========================================
 * 0193 キャラID名前引き
 *------------------------------------------
 */
void clif_parse_SolveCharName (int fd, struct map_session_data *sd)
{
    int  char_id;

    char_id = RFIFOL (fd, 2);
    clif_solved_charname (sd, char_id);
}

/*==========================================
 * 0197 /resetskill /resetstate
 *------------------------------------------
 */
void clif_parse_ResetChar (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (battle_config.atc_gmonly == 0 || pc_isGM (sd))
    {
        switch (RFIFOW (fd, 2))
        {
            case 0:
                log_atcommand (sd, "@charstreset %s", sd->status.name);
                if (pc_isGM (sd) >=
                    get_atcommand_level (AtCommand_ResetState))
                    pc_resetstate (sd);
                break;
            case 1:
                log_atcommand (sd, "@charskreset %s", sd->status.name);
                if (pc_isGM (sd) >=
                    get_atcommand_level (AtCommand_ResetState))
                    pc_resetskill (sd);
                break;
        }
    }
}

/*==========================================
 * 019c /lb等
 *------------------------------------------
 */
void clif_parse_LGMmessage (int fd, struct map_session_data *sd)
{
    unsigned char buf[64];

    nullpo_retv (sd);

    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_LocalBroadcast)))
    {
        WBUFW (buf, 0) = 0x9a;
        WBUFW (buf, 2) = RFIFOW (fd, 2);
        memcpy (WBUFP (buf, 4), RFIFOP (fd, 4), RFIFOW (fd, 2) - 4);
        clif_send (buf, RFIFOW (fd, 2), &sd->bl, ALL_SAMEMAP);
    }
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafra (int fd, struct map_session_data *sd)
{
    int  item_index, item_amount;

    nullpo_retv (sd);

    item_index = RFIFOW (fd, 2) - 2;
    item_amount = RFIFOL (fd, 4);

    if ((sd->npc_id != 0 && !sd->npc_flags.storage) || sd->trade_partner != 0
        || !sd->state.storage_flag)
        return;

    if (sd->state.storage_flag == 1)
        storage_storageadd (sd, item_index, item_amount);
    else if (sd->state.storage_flag == 2)
        storage_guild_storageadd (sd, item_index, item_amount);
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafra (int fd, struct map_session_data *sd)
{
    int  item_index, item_amount;

    nullpo_retv (sd);

    item_index = RFIFOW (fd, 2) - 1;
    item_amount = RFIFOL (fd, 4);

    if ((sd->npc_id != 0 && !sd->npc_flags.storage) || sd->trade_partner != 0
        || !sd->state.storage_flag)
        return;

    if (sd->state.storage_flag == 1)
        storage_storageget (sd, item_index, item_amount);
    else if (sd->state.storage_flag == 2)
        storage_guild_storageget (sd, item_index, item_amount);
}

/*==========================================
 * カプラ倉庫へカートから入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafraFromCart (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if ((sd->npc_id != 0 && !sd->npc_flags.storage) || sd->trade_partner != 0
        || !sd->state.storage_flag)
        return;
    if (sd->state.storage_flag == 1)
        storage_storageaddfromcart (sd, RFIFOW (fd, 2) - 2, RFIFOL (fd, 4));
    else if (sd->state.storage_flag == 2)
        storage_guild_storageaddfromcart (sd, RFIFOW (fd, 2) - 2,
                                          RFIFOL (fd, 4));
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafraToCart (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if ((sd->npc_id != 0 && !sd->npc_flags.storage) || sd->trade_partner != 0
        || !sd->state.storage_flag)
        return;
    if (sd->state.storage_flag == 1)
        storage_storagegettocart (sd, RFIFOW (fd, 2) - 1, RFIFOL (fd, 4));
    else if (sd->state.storage_flag == 2)
        storage_guild_storagegettocart (sd, RFIFOW (fd, 2) - 1,
                                        RFIFOL (fd, 4));
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
void clif_parse_CloseKafra (int fd, struct map_session_data *sd)
{
    nullpo_retv (sd);

    if (sd->state.storage_flag == 1)
        storage_storageclose (sd);
    else if (sd->state.storage_flag == 2)
        storage_guild_storageclose (sd);
}

/*==========================================
 * パーティを作る
 * Process request to create a party.
 *
 * (S 00f9 <party_name>.24B)
 *------------------------------------------
 */
void clif_parse_CreateParty (int fd, struct map_session_data *sd)
{
    if (battle_config.basic_skill_check == 0
        || pc_checkskill (sd, NV_PARTY) >= 2)
    {
        party_create (sd, RFIFOP (fd, 2));
    }
    else
        clif_skill_fail (sd, 1, 0, 4);
}

/*==========================================
 * パーティを作る
 * Process request to create a party.
 *
 * (S 01e8 <party_name>.24B <exp>.B <itm>.B)
 *
 * Note: Upstream eAthena uses this to
 *       specify experience/item sharing,
 *       respectively, but it was left
 *       incomplete here.
 *------------------------------------------
 */
void clif_parse_CreateParty2 (int fd, struct map_session_data *sd)
{
    if (battle_config.basic_skill_check == 0
        || pc_checkskill (sd, NV_PARTY) >= 2)
    {
        party_create (sd, RFIFOP (fd, 2));
    }
    else
        clif_skill_fail (sd, 1, 0, 4);
}

/*==========================================
 * パーティに勧誘
 * Process invitation to join a party.
 *
 * (S 00fc <account_ID>.l)
 *------------------------------------------
 */
void clif_parse_PartyInvite (int fd, struct map_session_data *sd)
{
    party_invite (sd, RFIFOL (fd, 2));
}

/*==========================================
 * パーティ勧誘返答
 * Process reply to party invitation.
 *
 * (S 00ff <account_ID>.l <flag>.l)
 *------------------------------------------
 */
void clif_parse_ReplyPartyInvite (int fd, struct map_session_data *sd)
{
    if (battle_config.basic_skill_check == 0
        || pc_checkskill (sd, NV_PARTY) >= 1)
    {
        party_reply_invite (sd, RFIFOL (fd, 2), RFIFOL (fd, 6));
    }
    else
    {
        party_reply_invite (sd, RFIFOL (fd, 2), 0);
        clif_skill_fail (sd, 1, 0, 4);
    }
}

/*==========================================
 * パーティ脱退要求
 *------------------------------------------
 */
void clif_parse_LeaveParty (int fd, struct map_session_data *sd)
{
    party_leave (sd);
}

/*==========================================
 * パーティ除名要求
 *------------------------------------------
 */
void clif_parse_RemovePartyMember (int fd, struct map_session_data *sd)
{
    party_removemember (sd, RFIFOL (fd, 2), RFIFOP (fd, 6));
}

/*==========================================
 * パーティ設定変更要求
 *------------------------------------------
 */
void clif_parse_PartyChangeOption (int fd, struct map_session_data *sd)
{
    party_changeoption (sd, RFIFOW (fd, 2), RFIFOW (fd, 4));
}

/*==========================================
 * パーティメッセージ送信要求
 * Validate and process transmission of a
 * party message.
 *
 * (S 0108 <len>.w <message>.?B)
 *------------------------------------------
 */
void clif_parse_PartyMessage (int fd, struct map_session_data *sd)
{
    size_t message_len = 0;
    char *buf = NULL;
    char *message = NULL;   /* The message text only. */

    nullpo_retv (sd);

    if (!(buf = clif_validate_chat (sd, 0, &message, &message_len)))
    {
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }
    
    if (is_atcommand (fd, sd, message, 0) != AtCommand_None
            || (sd->sc_data && (sd->sc_data[SC_BERSERK].timer != -1 //バーサーク時は会話も不可
                                || sd->sc_data[SC_NOCHAT].timer != -1))) //チャット禁止
    {
        free (buf);
        return;
    }

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam (sd, message))
    {
        free (buf);
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }

    party_send_message (sd, message, RFIFOW (fd, 2) - 4);
    free (buf);
}

/*==========================================
 * /monster /item rewriten by [Yor]
 *------------------------------------------
 */
void clif_parse_GM_Monster_Item (int fd, struct map_session_data *sd)
{
    char monster_item_name[25];

    nullpo_retv (sd);

    memset (monster_item_name, '\0', sizeof (monster_item_name));

    if (battle_config.atc_gmonly == 0 || pc_isGM (sd))
    {
        memcpy (monster_item_name, RFIFOP (fd, 2), 24);

        if (mobdb_searchname (monster_item_name) != 0)
        {
            if (pc_isGM (sd) >= get_atcommand_level (AtCommand_Monster))
            {
                log_atcommand (sd, "@spawn %s", monster_item_name);
                atcommand_spawn (fd, sd, "@spawn", monster_item_name);  // as @spawn
            }
        }
        else if (itemdb_searchname (monster_item_name) != NULL)
        {
            if (pc_isGM (sd) >= get_atcommand_level (AtCommand_Item))
            {
                log_atcommand (sd, "@item %s", monster_item_name);
                atcommand_item (fd, sd, "@item", monster_item_name);    // as @item
            }
        }

    }
}

/*==========================================
 * ギルドを作る
 * Process request to create a guild.
 *
 * (S 0165 <account_ID>.l <guild_name>.24B)
 *
 * Note: The account ID seems to be ignored.
 *------------------------------------------
 */
void clif_parse_CreateGuild (int fd, struct map_session_data *sd)
{
    guild_create (sd, RFIFOP (fd, 6));
}

/*==========================================
 * ギルドマスターかどうか確認
 *------------------------------------------
 */
void clif_parse_GuildCheckMaster (int fd, struct map_session_data *sd)
{
    clif_guild_masterormember (sd);
}

/*==========================================
 * ギルド情報要求
 *------------------------------------------
 */
void clif_parse_GuildReqeustInfo (int fd, struct map_session_data *sd)
{
    switch (RFIFOL (fd, 2))
    {
        case 0:                // ギルド基本情報、同盟敵対情報
            clif_guild_basicinfo (sd);
            clif_guild_allianceinfo (sd);
            break;
        case 1:                // メンバーリスト、役職名リスト
            clif_guild_positionnamelist (sd);
            clif_guild_memberlist (sd);
            break;
        case 2:                // 役職名リスト、役職情報リスト
            clif_guild_positionnamelist (sd);
            clif_guild_positioninfolist (sd);
            break;
        case 3:                // スキルリスト
            clif_guild_skillinfo (sd);
            break;
        case 4:                // 追放リスト
            clif_guild_explusionlist (sd);
            break;
        default:
            if (battle_config.error_log)
                printf ("clif: guild request info: unknown type %d\n",
                        RFIFOL (fd, 2));
            break;
    }
}

/*==========================================
 * ギルド役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangePositionInfo (int fd, struct map_session_data *sd)
{
    struct guild *g;
    int  i, ps;

    nullpo_retv (sd);

    g = guild_search (sd->status.guild_id);

    if (g == NULL)
        return;

    if ((ps = guild_getposition (sd, g)) < 0
        || (!(g->position[ps].mode & 0x0010) && strcmp (g->master, sd->status.name)))
        return;

    for (i = 4; i < RFIFOW (fd, 2); i += 40)
    {
        guild_change_position (sd, RFIFOL (fd, i), RFIFOL (fd, i + 4),
                               RFIFOL (fd, i + 12), RFIFOP (fd, i + 16));
    }
}

/*==========================================
 * ギルドメンバ役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangeMemberPosition (int fd,
                                           struct map_session_data *sd)
{
    struct guild *g;
    int  i, ps;

    nullpo_retv (sd);

    g = guild_search (sd->status.guild_id);

    if (g == NULL)
        return;

    if ((ps = guild_getposition (sd, g)) < 0
        || (!(g->position[ps].mode & 0x0010) && strcmp (g->master, sd->status.name)))
        return;

    for (i = 4; i < RFIFOW (fd, 2); i += 12)
    {
        guild_change_memberposition (sd->status.guild_id,
                                     RFIFOL (fd, i), RFIFOL (fd, i + 4),
                                     RFIFOL (fd, i + 8));
    }
}

/*==========================================
 * ギルドエンブレム要求
 *------------------------------------------
 */
void clif_parse_GuildRequestEmblem (int fd, struct map_session_data *sd)
{
    struct guild *g = guild_search (RFIFOL (fd, 2));
    if (g != NULL)
        clif_guild_emblem (sd, g);
}

/*==========================================
 * ギルドエンブレム変更
 *------------------------------------------
 */
void clif_parse_GuildChangeEmblem (int fd, struct map_session_data *sd)
{
    guild_change_emblem (sd, RFIFOW (fd, 2) - 4, RFIFOP (fd, 4));
}

/*==========================================
 * ギルド告知変更
 *------------------------------------------
 */
void clif_parse_GuildChangeNotice (int fd, struct map_session_data *sd)
{
    guild_change_notice (sd, RFIFOL (fd, 2), RFIFOP (fd, 6), RFIFOP (fd, 66));
}

/*==========================================
 * ギルド勧誘
 *------------------------------------------
 */
void clif_parse_GuildInvite (int fd, struct map_session_data *sd)
{
    guild_invite (sd, RFIFOL (fd, 2));
}

/*==========================================
 * ギルド勧誘返信
 *------------------------------------------
 */
void clif_parse_GuildReplyInvite (int fd, struct map_session_data *sd)
{
    guild_reply_invite (sd, RFIFOL (fd, 2), RFIFOB (fd, 6));
}

/*==========================================
 * ギルド脱退
 *------------------------------------------
 */
void clif_parse_GuildLeave (int fd, struct map_session_data *sd)
{
    guild_leave (sd, RFIFOL (fd, 2), RFIFOL (fd, 6), RFIFOL (fd, 10),
                 RFIFOP (fd, 14));
}

/*==========================================
 * ギルド追放
 *------------------------------------------
 */
void clif_parse_GuildExplusion (int fd, struct map_session_data *sd)
{
    guild_explusion (sd, RFIFOL (fd, 2), RFIFOL (fd, 6), RFIFOL (fd, 10),
                     RFIFOP (fd, 14));
}

/*==========================================
 * ギルド会話
 * Validate and process transmission of a
 * guild message.
 *
 * (S 017e <len>.w <message>.?B)
 *------------------------------------------
 */
void clif_parse_GuildMessage (int fd, struct map_session_data *sd)
{
    size_t message_len = 0;
    char *buf = NULL;
    char *message = NULL;   /* The message text only. */

    nullpo_retv (sd);

    if (!(buf = clif_validate_chat (sd, 2, &message, &message_len)))
    {
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }

    if (is_atcommand (fd, sd, message, 0) != AtCommand_None
            || (sd->sc_data && (sd->sc_data[SC_BERSERK].timer != -1 //バーサーク時は会話も不可
                                || sd->sc_data[SC_NOCHAT].timer != -1))) //チャット禁止
    {
        free (buf);
        return;
    }

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam (sd, message))
    {
        free (buf);
        /* "Your message could not be sent." */
        clif_displaymessage (fd, msg_txt (505));
        return;
    }

    guild_send_message (sd, buf + 8, RFIFOW (fd, 2) - 4);
    free (buf);
}

/*==========================================
 * ギルド同盟要求
 *------------------------------------------
 */
void clif_parse_GuildRequestAlliance (int fd, struct map_session_data *sd)
{
    guild_reqalliance (sd, RFIFOL (fd, 2));
}

/*==========================================
 * ギルド同盟要求返信
 *------------------------------------------
 */
void clif_parse_GuildReplyAlliance (int fd, struct map_session_data *sd)
{
    guild_reply_reqalliance (sd, RFIFOL (fd, 2), RFIFOL (fd, 6));
}

/*==========================================
 * ギルド関係解消
 *------------------------------------------
 */
void clif_parse_GuildDelAlliance (int fd, struct map_session_data *sd)
{
    guild_delalliance (sd, RFIFOL (fd, 2), RFIFOL (fd, 6));
}

/*==========================================
 * ギルド敵対
 *------------------------------------------
 */
void clif_parse_GuildOpposition (int fd, struct map_session_data *sd)
{
    guild_opposition (sd, RFIFOL (fd, 2));
}

/*==========================================
 * ギルド解散
 *------------------------------------------
 */
void clif_parse_GuildBreak (int fd, struct map_session_data *sd)
{
    guild_break (sd, RFIFOP (fd, 2));
}

// Kick (right click menu for GM "(name) force to quit")
void clif_parse_GMKick (int fd, struct map_session_data *sd)
{
    struct block_list *target;
    int  tid = RFIFOL (fd, 2);

    nullpo_retv (sd);

    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_Kick)))
    {
        target = map_id2bl (tid);
        if (target)
        {
            if (target->type == BL_PC)
            {
                struct map_session_data *tsd =
                    (struct map_session_data *) target;
                log_atcommand (sd, "@kick %s", tsd->status.name);
                if (pc_isGM (sd) > pc_isGM (tsd))
                    clif_GM_kick (sd, tsd, 1);
                else
                    clif_GM_kickack (sd, 0);
            }
            else if (target->type == BL_MOB)
            {
                struct mob_data *md = (struct mob_data *) target;
                sd->state.attack_type = 0;
                mob_damage (&sd->bl, md, md->hp, 2);
            }
            else
                clif_GM_kickack (sd, 0);
        }
        else
            clif_GM_kickack (sd, 0);
    }
}

/*==========================================
 * /shift
 *------------------------------------------
 */
void clif_parse_Shift (int fd, struct map_session_data *sd)
{                               // Rewriten by [Yor]
    char player_name[25];

    nullpo_retv (sd);

    memset (player_name, '\0', sizeof (player_name));

    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_Goto)))
    {
        memcpy (player_name, RFIFOP (fd, 2), 24);
        log_atcommand (sd, "@goto %s", player_name);
        atcommand_goto (fd, sd, "@goto", player_name);  // as @jumpto
    }

    return;
}

/*==========================================
 * /recall
 *------------------------------------------
 */
void clif_parse_Recall (int fd, struct map_session_data *sd)
{                               // Added by RoVeRT
    char player_name[25];

    nullpo_retv (sd);

    memset (player_name, '\0', sizeof (player_name));

    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_Recall)))
    {
        memcpy (player_name, RFIFOP (fd, 2), 24);
        log_atcommand (sd, "@recall %s", player_name);
        atcommand_recall (fd, sd, "@recall", player_name);  // as @recall
    }

    return;
}

void clif_parse_GMHide (int fd, struct map_session_data *sd)
{                               // Modified by [Yor]
    nullpo_retv (sd);

    //printf("%2x %2x %2x\n", RFIFOW(fd,0), RFIFOW(fd,2), RFIFOW(fd,4)); // R 019d <Option_value>.2B <flag>.2B
    if ((battle_config.atc_gmonly == 0 || pc_isGM (sd)) &&
        (pc_isGM (sd) >= get_atcommand_level (AtCommand_Hide)))
    {
        log_atcommand (sd, "@hide");
        if (sd->status.option & OPTION_HIDE)
        {                       // OPTION_HIDE = 0x40
            sd->status.option &= ~OPTION_HIDE;  // OPTION_HIDE = 0x40
            /* "Invisible: Off." */
            clif_displaymessage (fd, msg_txt (10));
        }
        else
        {
            sd->status.option |= OPTION_HIDE;   // OPTION_HIDE = 0x40
            /* "Invisible: On." */
            clif_displaymessage (fd, msg_txt (11));
        }
        clif_changeoption (&sd->bl);
    }
}

/*==========================================
 * GMによるチャット禁止時間付与
 *------------------------------------------
 */
void clif_parse_GMReqNoChat (int fd, struct map_session_data *sd)
{
    int  tid = RFIFOL (fd, 2);
    int  type = RFIFOB (fd, 6);
    int  limit = RFIFOW (fd, 7);
    struct block_list *bl = map_id2bl (tid);
    struct map_session_data *dstsd;
    int  dstfd;

    nullpo_retv (sd);

    if (!battle_config.muting_players)
    {
        /* "Muting is disabled." */
        clif_displaymessage (fd, msg_txt (245));
        return;
    }

    if (type == 0)
        limit = 0 - limit;
    if (bl->type == BL_PC && (dstsd = (struct map_session_data *) bl))
    {
        if ((tid == bl->id && type == 2 && !pc_isGM (sd))
            || (pc_isGM (sd) > pc_isGM (dstsd)))
        {
            dstfd = dstsd->fd;
            WFIFOW (dstfd, 0) = 0x14b;
            WFIFOB (dstfd, 2) = (type == 2) ? 1 : type;
            memcpy (WFIFOP (dstfd, 3), sd->status.name, 24);
            WFIFOSET (dstfd, packet_len_table[0x14b]);
            dstsd->status.manner -= limit;
            if (dstsd->status.manner < 0)
                skill_status_change_start (bl, SC_NOCHAT, 0, 0, 0, 0, 0, 0);
            else
            {
                dstsd->status.manner = 0;
                skill_status_change_end (bl, SC_NOCHAT, -1);
            }
            printf ("name:%s type:%d limit:%d manner:%d\n",
                    dstsd->status.name, type, limit, dstsd->status.manner);
        }
    }

    return;
}

/*==========================================
 * GMによるチャット禁止時間参照（？）
 *------------------------------------------
 */
void clif_parse_GMReqNoChatCount (int fd, struct map_session_data *sd)
{
    int  tid = RFIFOL (fd, 2);

    WFIFOW (fd, 0) = 0x1e0;
    WFIFOL (fd, 2) = tid;
    sprintf (WFIFOP (fd, 6), "%d", tid);
//  memcpy(WFIFOP(fd,6),"TESTNAME",24);
    WFIFOSET (fd, packet_len_table[0x1e0]);

    return;
}

void clif_parse_PMIgnore (int fd, struct map_session_data *sd)
{                               // Rewritten by [Yor]
    char output[1024];
    char *nick;                 // S 00cf <nick>.24B <type>.B: 00 (/ex nick) deny speech from nick, 01 (/in nick) allow speech from nick
    int  i;
    int  pos;

    memset (output, '\0', sizeof (output));

    nick = RFIFOP (fd, 2);      // speed up
    //printf("Ignore: char '%s' state: %d\n", nick, RFIFOB(fd,26));
    // we ask for deny (we add nick only if it's not already exist
    if (RFIFOB (fd, 26) == 0)
    {                           // type
        if (strlen (nick) >= 4 && strlen (nick) < 24)
        {                       // do something only if nick can be exist
            pos = -1;
            for (i = 0; i < (sizeof (sd->ignore) / sizeof (sd->ignore[0]));
                 i++)
            {
                if (strcmp (sd->ignore[i].name, nick) == 0)
                    break;
                else if (pos == -1 && sd->ignore[i].name[0] == '\0')
                    pos = i;
            }
            WFIFOW (fd, 0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
            WFIFOB (fd, 2) = 0;
            // if a position is found and name not found, we add it in the list
            if (pos != -1
                && i == (sizeof (sd->ignore) / sizeof (sd->ignore[0])))
            {
                memcpy (sd->ignore[pos].name, nick, 24);
                WFIFOB (fd, 3) = 0; // success
                WFIFOSET (fd, packet_len_table[0x0d1]);
                if (strcmp (wisp_server_name, nick) == 0)
                {               // to found possible bot users that automaticaly ignores people.
                    sprintf (output,
                             "Character '%s' (account: %d) has tried to block wisps from '%s' (wisp name of the server). Bot user?",
                             sd->status.name, sd->status.account_id,
                             wisp_server_name);
                    intif_wis_message_to_gm (wisp_server_name,
                                             battle_config.hack_info_GM_level,
                                             output, strlen (output) + 1);
                    // send something to be inform and force bot to ignore twice... If GM receiving block + block again, it's a bot :)
                    clif_wis_message (fd, wisp_server_name,
                                      "Add me in your ignore list, doesn't block my wisps.",
                                      strlen
                                      ("Add me in your ignore list, doesn't block my wisps.")
                                      + 1);
                }
            }
            else
            {
                WFIFOB (fd, 3) = 1; // fail
                if (i == (sizeof (sd->ignore) / sizeof (sd->ignore[0])))
                {
                    clif_wis_message (fd, wisp_server_name,
                                      "You can not block more people.",
                                      strlen
                                      ("You can not block more people.") + 1);
                    if (strcmp (wisp_server_name, nick) == 0)
                    {           // to found possible bot users that automaticaly ignores people.
                        sprintf (output,
                                 "Character '%s' (account: %d) has tried to block wisps from '%s' (wisp name of the server). Bot user?",
                                 sd->status.name, sd->status.account_id,
                                 wisp_server_name);
                        intif_wis_message_to_gm (wisp_server_name,
                                                 battle_config.hack_info_GM_level,
                                                 output, strlen (output) + 1);
                    }
                }
                else
                {
                    clif_wis_message (fd, wisp_server_name,
                                      "This player is already blocked.",
                                      strlen
                                      ("This player is already blocked.") +
                                      1);
                    if (strcmp (wisp_server_name, nick) == 0)
                    {           // to found possible bot users that automaticaly ignores people.
                        sprintf (output,
                                 "Character '%s' (account: %d) has tried AGAIN to block wisps from '%s' (wisp name of the server). Bot user?",
                                 sd->status.name, sd->status.account_id,
                                 wisp_server_name);
                        intif_wis_message_to_gm (wisp_server_name,
                                                 battle_config.hack_info_GM_level,
                                                 output, strlen (output) + 1);
                    }
                }
            }
        }
        else
            clif_wis_message (fd, wisp_server_name,
                              "It's impossible to block this player.",
                              strlen ("It's impossible to block this player.")
                              + 1);
        // we ask for allow (we remove all same nick if exist)
    }
    else
    {
        if (strlen (nick) >= 4 && strlen (nick) < 24)
        {                       // do something only if nick can be exist
            WFIFOW (fd, 0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
            WFIFOB (fd, 2) = 1;
            for (i = 0; i < (sizeof (sd->ignore) / sizeof (sd->ignore[0]));
                 i++)
                if (strcmp (sd->ignore[i].name, nick) == 0)
                {
                    memset (sd->ignore[i].name, 0,
                            sizeof (sd->ignore[i].name));
                    WFIFOB (fd, 3) = 0; // success
                    WFIFOSET (fd, packet_len_table[0x0d1]);
                    break;
                }
            if (i == (sizeof (sd->ignore) / sizeof (sd->ignore[0])))
            {
                WFIFOB (fd, 3) = 1; // fail
                WFIFOSET (fd, packet_len_table[0x0d1]);
                clif_wis_message (fd, wisp_server_name,
                                  "This player is not blocked by you.",
                                  strlen
                                  ("This player is not blocked by you.") + 1);
            }
        }
        else
            clif_wis_message (fd, wisp_server_name,
                              "It's impossible to unblock this player.",
                              strlen
                              ("It's impossible to unblock this player.") +
                              1);
    }

//  for(i = 0; i < (sizeof(sd->ignore) / sizeof(sd->ignore[0])); i++) // for debug only
//      if (sd->ignore[i].name[0] != '\0')
//          printf("Ignored player: '%s'\n", sd->ignore[i].name);

    return;
}

void clif_parse_PMIgnoreAll (int fd, struct map_session_data *sd)
{                               // Rewritten by [Yor]
    //printf("Ignore all: state: %d\n", RFIFOB(fd,2));
    if (RFIFOB (fd, 2) == 0)
    {                           // S 00d0 <type>len.B: 00 (/exall) deny all speech, 01 (/inall) allow all speech
        WFIFOW (fd, 0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
        WFIFOB (fd, 2) = 0;
        if (sd->ignoreAll == 0)
        {
            sd->ignoreAll = 1;
            WFIFOB (fd, 3) = 0; // success
            WFIFOSET (fd, packet_len_table[0x0d2]);
        }
        else
        {
            WFIFOB (fd, 3) = 1; // fail
            WFIFOSET (fd, packet_len_table[0x0d2]);
            clif_wis_message (fd, wisp_server_name,
                              "You already block everyone.",
                              strlen ("You already block everyone.") + 1);
        }
    }
    else
    {
        WFIFOW (fd, 0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
        WFIFOB (fd, 2) = 1;
        if (sd->ignoreAll == 1)
        {
            sd->ignoreAll = 0;
            WFIFOB (fd, 3) = 0; // success
            WFIFOSET (fd, packet_len_table[0x0d2]);
        }
        else
        {
            WFIFOB (fd, 3) = 1; // fail
            WFIFOSET (fd, packet_len_table[0x0d2]);
            clif_wis_message (fd, wisp_server_name,
                              "You already allow everyone.",
                              strlen ("You already allow everyone.") + 1);
        }
    }

    return;
}

void clif_parse_skillMessage (int fd, struct map_session_data *sd)
{                               // Added by RoVeRT
    int  skillid, skilllv, x, y;
    char *mes;

    skilllv = RFIFOW (fd, 2);
    skillid = RFIFOW (fd, 4);

    y = RFIFOB (fd, 6);
    x = RFIFOB (fd, 8);

    mes = RFIFOP (fd, 10);

    // skill 220 = graffiti
//  printf("skill: %d %d location: %3d %3d message: %s\n", skillid, skilllv, x, y, (char*)mes);
}

int monk (struct map_session_data *sd, struct block_list *target, int type)
{
//R 01d1 <Monk id>L <Target monster id>L <Bool>L
    int  fd = sd->fd;
    WFIFOW (fd, 0) = 0x1d1;
    WFIFOL (fd, 2) = sd->bl.id;
    WFIFOL (fd, 6) = target->id;
    WFIFOL (fd, 10) = type;
    WFIFOSET (fd, packet_len_table[0x1d1]);

    return 0;
}

/*==========================================
 * スパノビの/doridoriによるSPR2倍
 *------------------------------------------
 */
void clif_parse_sn_doridori (int fd, struct map_session_data *sd)
{
    if (sd)
        sd->doridori_counter = 1;

    return;
}

/*==========================================
 * スパノビの爆裂波動
 *------------------------------------------
 */
void clif_parse_sn_explosionspirits (int fd, struct map_session_data *sd)
{
    if (sd)
    {
        int  nextbaseexp = pc_nextbaseexp (sd);
        struct pc_base_job s_class = pc_calc_base_job (sd->status.class);
        if (battle_config.etc_log)
        {
            if (nextbaseexp != 0)
                printf ("SuperNovice explosionspirits!! %d %d %d %d\n",
                        sd->bl.id, s_class.job, sd->status.base_exp,
                        (int) ((double) 1000 * sd->status.base_exp /
                               nextbaseexp));
            else
                printf ("SuperNovice explosionspirits!! %d %d %d 000\n",
                        sd->bl.id, s_class.job, sd->status.base_exp);
        }
        if (s_class.job == 23 && sd->status.base_exp > 0 && nextbaseexp > 0
            && (int) ((double) 1000 * sd->status.base_exp / nextbaseexp) %
            100 == 0)
        {
            clif_skill_nodamage (&sd->bl, &sd->bl, MO_EXPLOSIONSPIRITS, 5, 1);
            skill_status_change_start (&sd->bl,
                                       SkillStatusChangeTable
                                       [MO_EXPLOSIONSPIRITS], 5, 0, 0, 0,
                                       skill_get_time (MO_EXPLOSIONSPIRITS,
                                                       5), 0);
        }
    }
    return;
}

// functions list. Rate is how many milliseconds are required between
// calls. Packets exceeding this rate will be dropped. flood_rates in
// map.h must be the same length as this table. rate 0 is default
// rate -1 is unlimited
typedef struct func_table
{
	void (*func)();
	int rate;
} func_table;
// *INDENT-OFF*
func_table clif_parse_func_table[0x220] =
{
	{ NULL,					0	},	// 0
	{ NULL,					0	},	// 1
	{ NULL,					0	},	// 2
	{ NULL,					0	},	// 3
	{ NULL,					0	},	// 4
	{ NULL,					0	},	// 5
	{ NULL,					0	},	// 6
	{ NULL,					0	},	// 7
	{ NULL,					0	},	// 8
	{ NULL,					0	},	// 9
	{ NULL,					0	},	// a
	{ NULL,					0	},	// b
	{ NULL,					0	},	// c
	{ NULL,					0	},	// d
	{ NULL,					0	},	// e
	{ NULL,					0	},	// f
	{ NULL,					0	},	// 10
	{ NULL,					0	},	// 11
	{ NULL,					0	},	// 12
	{ NULL,					0	},	// 13
	{ NULL,					0	},	// 14
	{ NULL,					0	},	// 15
	{ NULL,					0	},	// 16
	{ NULL,					0	},	// 17
	{ NULL,					0	},	// 18
	{ NULL,					0	},	// 19
	{ NULL,					0	},	// 1a
	{ NULL,					0	},	// 1b
	{ NULL,					0	},	// 1c
	{ NULL,					0	},	// 1d
	{ NULL,					0	},	// 1e
	{ NULL,					0	},	// 1f
	{ NULL,					0	},	// 20
	{ NULL,					0	},	// 21
	{ NULL,					0	},	// 22
	{ NULL,					0	},	// 23
	{ NULL,					0	},	// 24
	{ NULL,					0	},	// 25
	{ NULL,					0	},	// 26
	{ NULL,					0	},	// 27
	{ NULL,					0	},	// 28
	{ NULL,					0	},	// 29
	{ NULL,					0	},	// 2a
	{ NULL,					0	},	// 2b
	{ NULL,					0	},	// 2c
	{ NULL,					0	},	// 2d
	{ NULL,					0	},	// 2e
	{ NULL,					0	},	// 2f
	{ NULL,					0	},	// 30
	{ NULL,					0	},	// 31
	{ NULL,					0	},	// 32
	{ NULL,					0	},	// 33
	{ NULL,					0	},	// 34
	{ NULL,					0	},	// 35
	{ NULL,					0	},	// 36
	{ NULL,					0	},	// 37
	{ NULL,					0	},	// 38
	{ NULL,					0	},	// 39
	{ NULL,					0	},	// 3a
	{ NULL,					0	},	// 3b
	{ NULL,					0	},	// 3c
	{ NULL,					0	},	// 3d
	{ NULL,					0	},	// 3e
	{ NULL,					0	},	// 3f
	{ NULL,					0	},	// 40
	{ NULL,					0	},	// 41
	{ NULL,					0	},	// 42
	{ NULL,					0	},	// 43
	{ NULL,					0	},	// 44
	{ NULL,					0	},	// 45
	{ NULL,					0	},	// 46
	{ NULL,					0	},	// 47
	{ NULL,					0	},	// 48
	{ NULL,					0	},	// 49
	{ NULL,					0	},	// 4a
	{ NULL,					0	},	// 4b
	{ NULL,					0	},	// 4c
	{ NULL,					0	},	// 4d
	{ NULL,					0	},	// 4e
	{ NULL,					0	},	// 4f
	{ NULL,					0	},	// 50
	{ NULL,					0	},	// 51
	{ NULL,					0	},	// 52
	{ NULL,					0	},	// 53
	{ NULL,					0	},	// 54
	{ NULL,					0	},	// 55
	{ NULL,					0	},	// 56
	{ NULL,					0	},	// 57
	{ NULL,					0	},	// 58
	{ NULL,					0	},	// 59
	{ NULL,					0	},	// 5a
	{ NULL,					0	},	// 5b
	{ NULL,					0	},	// 5c
	{ NULL,					0	},	// 5d
	{ NULL,					0	},	// 5e
	{ NULL,					0	},	// 5f
	{ NULL,					0	},	// 60
	{ NULL,					0	},	// 61
	{ NULL,					0	},	// 62
	{ NULL,					0	},	// 63
	{ NULL,					0	},	// 64
	{ NULL,					0	},	// 65
	{ NULL,					0	},	// 66
	{ NULL,					0	},	// 67
	{ NULL,					0	},	// 68
	{ NULL,					0	},	// 69
	{ NULL,					0	},	// 6a
	{ NULL,					0	},	// 6b
	{ NULL,					0	},	// 6c
	{ NULL,					0	},	// 6d
	{ NULL,					0	},	// 6e
	{ NULL,					0	},	// 6f
	{ NULL,					0	},	// 70
	{ NULL,					0	},	// 71
	{ clif_parse_WantToConnection,		0	},	// 72
	{ NULL,					0	},	// 73
	{ NULL,					0	},	// 74
	{ NULL,					0	},	// 75
	{ NULL,					0	},	// 76
	{ NULL,					0	},	// 77
	{ NULL,					0	},	// 78
	{ NULL,					0	},	// 79
	{ NULL,					0	},	// 7a
	{ NULL,					0	},	// 7b
	{ NULL,					0	},	// 7c
	{ clif_parse_LoadEndAck,		-1	},	// 7d
	{ clif_parse_TickSend,			0	},	// 7e
	{ NULL,					0	},	// 7f
	{ NULL,					0	},	// 80
	{ NULL,					0	},	// 81
	{ NULL,					0	},	// 82
	{ NULL,					0	},	// 83
	{ NULL,					0	},	// 84
	{ clif_parse_WalkToXY,			-1	},	// 85 Walk code limits this on it's own
	{ NULL,					0	},	// 86
	{ NULL,					0	},	// 87
	{ NULL,					0	},	// 88
	{ clif_parse_ActionRequest,		1000	},	// 89 Special case - see below
	{ NULL,					0	},	// 8a
	{ NULL,					0	},	// 8b
	{ clif_parse_GlobalMessage,		300	},	// 8c
	{ NULL,					0	},	// 8d
	{ NULL,					0	},	// 8e
	{ NULL,					0	},	// 8f
	{ clif_parse_NpcClicked,		500	},	// 90
	{ NULL,					0	},	// 91
	{ NULL,					0	},	// 92
	{ NULL,					0	},	// 93
	{ clif_parse_GetCharNameRequest,	-1	},	// 94
	{ NULL,					0	},	// 95
	{ clif_parse_Wis,			300	},	// 96
	{ NULL,					0	},	// 97
	{ NULL,					0	},	// 98
	{ clif_parse_GMmessage,			300	},	// 99
	{ NULL,					0	},	// 9a
	{ clif_parse_ChangeDir,			-1	},	// 9b
	{ NULL,					0	},	// 9c
	{ NULL,					0	},	// 9d
	{ NULL,					0	},	// 9e
	{ clif_parse_TakeItem,			400	},	// 9f
	{ NULL,					0	},	// a0
	{ NULL,					0	},	// a1
	{ clif_parse_DropItem,			50	},	// a2
	{ NULL,					0	},	// a3
	{ NULL,					0	},	// a4
	{ NULL,					0	},	// a5
	{ NULL,					0	},	// a6
	{ clif_parse_UseItem,			0	},	// a7
	{ NULL,					0	},	// a8
	{ clif_parse_EquipItem,			-1	},	// a9 Special case - outfit window (not implemented yet - needs to allow bursts)
	{ NULL,					0	},	// aa
	{ clif_parse_UnequipItem,		-1	},	// ab Special case - outfit window (not implemented yet - needs to allow bursts)
	{ NULL,					0	},	// ac
	{ NULL,					0	},	// ad
	{ NULL,					0	},	// ae
	{ NULL,					0	},	// af
	{ NULL,					0	},	// b0
	{ NULL,					0	},	// b1
	{ clif_parse_Restart,			0	},	// b2
	{ NULL,					0	},	// b3
	{ NULL,					0	},	// b4
	{ NULL,					0	},	// b5
	{ NULL,					0	},	// b6
	{ NULL,					0	},	// b7
	{ clif_parse_NpcSelectMenu,		0	},	// b8
	{ clif_parse_NpcNextClicked,		-1	},	// b9
	{ NULL,					0	},	// ba
	{ clif_parse_StatusUp,			-1	},	// bb People click this very quickly
	{ NULL,					0	},	// bc
	{ NULL,					0	},	// bd
	{ NULL,					0	},	// be
	{ clif_parse_Emotion,			1000	},	// bf
	{ NULL,					0	},	// c0
	{ clif_parse_HowManyConnections,	0	},	// c1
	{ NULL,					0	},	// c2
	{ NULL,					0	},	// c3
	{ NULL,					0	},	// c4
	{ clif_parse_NpcBuySellSelected,	0	},	// c5
	{ NULL,					0	},	// c6
	{ NULL,					0	},	// c7
	{ clif_parse_NpcBuyListSend,		-1	},	// c8
	{ clif_parse_NpcSellListSend,		-1	},	// c9 Selling multiple 1-slot items
	{ NULL,					0	},	// ca
	{ NULL,					0	},	// cb
	{ clif_parse_GMKick,			0	},	// cc
	{ NULL,					0	},	// cd
	{ NULL,					0	},	// ce
	{ clif_parse_PMIgnore,			0	},	// cf
	{ clif_parse_PMIgnoreAll,		0	},	// d0
	{ NULL,					0	},	// d1
	{ NULL,					0	},	// d2
	{ NULL,					0	},	// d3
	{ NULL,					0	},	// d4
	{ clif_parse_CreateChatRoom,		1000	},	// d5
	{ NULL,					0	},	// d6
	{ NULL,					0	},	// d7
	{ NULL,					0	},	// d8
	{ clif_parse_ChatAddMember,		0	},	// d9
	{ NULL,					0	},	// da
	{ NULL,					0	},	// db
	{ NULL,					0	},	// dc
	{ NULL,					0	},	// dd
	{ clif_parse_ChatRoomStatusChange,	0	},	// de
	{ NULL,					0	},	// df
	{ clif_parse_ChangeChatOwner,		0	},	// e0
	{ NULL,					0	},	// e1
	{ clif_parse_KickFromChat,		0	},	// e2
	{ clif_parse_ChatLeave,			0	},	// e3
	{ clif_parse_TradeRequest,		2000	},	// e4
	{ NULL,					0	},	// e5
	{ clif_parse_TradeAck,			0	},	// e6
	{ NULL,					0	},	// e7
	{ clif_parse_TradeAddItem,		0	},	// e8
	{ NULL,					0	},	// e9
	{ NULL,					0	},	// ea
	{ clif_parse_TradeOk,			0	},	// eb
	{ NULL,					0	},	// ec
	{ clif_parse_TradeCansel,		0	},	// ed
	{ NULL,					0	},	// ee
	{ clif_parse_TradeCommit,		0	},	// ef
	{ NULL,					0	},	// f0
	{ NULL,					0	},	// f1
	{ NULL,					0	},	// f2
	{ clif_parse_MoveToKafra,		-1	},	// f3
	{ NULL,					0	},	// f4
	{ clif_parse_MoveFromKafra,		-1	},	// f5
	{ NULL,					0	},	// f6
	{ clif_parse_CloseKafra,		0	},	// f7
	{ NULL,					0	},	// f8
	{ clif_parse_CreateParty,		2000	},	// f9
	{ NULL,					0	},	// fa
	{ NULL,					0	},	// fb
	{ clif_parse_PartyInvite,		2000	},	// fc
	{ NULL,					0	},	// fd
	{ NULL,					0	},	// fe
	{ clif_parse_ReplyPartyInvite,		0	},	// ff
	{ clif_parse_LeaveParty,		0	},	// 100
	{ NULL,					0	},	// 101
	{ clif_parse_PartyChangeOption,		0	},	// 102
	{ clif_parse_RemovePartyMember,		0	},	// 103
	{ NULL,					0	},	// 104
	{ NULL,					0	},	// 105
	{ NULL,					0	},	// 106
	{ NULL,					0	},	// 107
	{ clif_parse_PartyMessage,		300	},	// 108
	{ NULL,					0	},	// 109
	{ NULL,					0	},	// 10a
	{ NULL,					0	},	// 10b
	{ NULL,					0	},	// 10c
	{ NULL,					0	},	// 10d
	{ NULL,					0	},	// 10e
	{ NULL,					0	},	// 10f
	{ NULL,					0	},	// 110
	{ NULL,					0	},	// 111
	{ clif_parse_SkillUp,			-1	},	// 112
	{ clif_parse_UseSkillToId,		0	},	// 113
	{ NULL,					0	},	// 114
	{ NULL,					0	},	// 115
	{ clif_parse_UseSkillToPos,		0	},	// 116
	{ NULL,					0	},	// 117
	{ clif_parse_StopAttack,		0	},	// 118
	{ NULL,					0	},	// 119
	{ NULL,					0	},	// 11a
	{ clif_parse_UseSkillMap,		0	},	// 11b
	{ NULL,					0	},	// 11c
	{ clif_parse_RequestMemo,		0	},	// 11d
	{ NULL,					0	},	// 11e
	{ NULL,					0	},	// 11f
	{ NULL,					0	},	// 120
	{ NULL,					0	},	// 121
	{ NULL,					0	},	// 122
	{ NULL,					0	},	// 123
	{ NULL,					0	},	// 124
	{ NULL,					0	},	// 125
	{ clif_parse_PutItemToCart,		0	},	// 126
	{ clif_parse_GetItemFromCart,		0	},	// 127
	{ clif_parse_MoveFromKafraToCart,	0	},	// 128
	{ clif_parse_MoveToKafraFromCart,	0	},	// 129
	{ clif_parse_RemoveOption,		0	},	// 12a
	{ NULL,					0	},	// 12b
	{ NULL,					0	},	// 12c
	{ NULL,					0	},	// 12d
	{ NULL,					0	},	// 12e
	{ NULL,					0	},	// 12f
	{ NULL,					0	},	// 130
	{ NULL,					0	},	// 131
	{ NULL,					0	},	// 132
	{ NULL,					0	},	// 133
	{ NULL,					0	},	// 134
	{ NULL,					0	},	// 135
	{ NULL,					0	},	// 136
	{ NULL,					0	},	// 137
	{ NULL,					0	},	// 138
	{ NULL,					0	},	// 139
	{ NULL,					0	},	// 13a
	{ NULL,					0	},	// 13b
	{ NULL,					0	},	// 13c
	{ NULL,					0	},	// 13d
	{ NULL,					0	},	// 13e
	{ clif_parse_GM_Monster_Item,		0	},	// 13f
	{ clif_parse_MapMove,			0	},	// 140
	{ NULL,					0	},	// 141
	{ NULL,					0	},	// 142
	{ clif_parse_NpcAmountInput,		300	},	// 143
	{ NULL,					0	},	// 144
	{ NULL,					0	},	// 145
	{ clif_parse_NpcCloseClicked,		300	},	// 146
	{ NULL,					0	},	// 147
	{ NULL,					0	},	// 148
	{ clif_parse_GMReqNoChat,		0	},	// 149
	{ NULL,					0	},	// 14a
	{ NULL,					0	},	// 14b
	{ NULL,					0	},	// 14c
	{ clif_parse_GuildCheckMaster,		0	},	// 14d
	{ NULL,					0	},	// 14e
	{ clif_parse_GuildReqeustInfo,		0	},	// 14f
	{ NULL,					0	},	// 150
	{ clif_parse_GuildRequestEmblem,	0	},	// 151
	{ NULL,					0	},	// 152
	{ clif_parse_GuildChangeEmblem,		0	},	// 153
	{ NULL,					0	},	// 154
	{ clif_parse_GuildChangeMemberPosition,	0	},	// 155
	{ NULL,					0	},	// 156
	{ NULL,					0	},	// 157
	{ NULL,					0	},	// 158
	{ clif_parse_GuildLeave,		0	},	// 159
	{ NULL,					0	},	// 15a
	{ clif_parse_GuildExplusion,		0	},	// 15b
	{ NULL,					0	},	// 15c
	{ clif_parse_GuildBreak,		0	},	// 15d
	{ NULL,					0	},	// 15e
	{ NULL,					0	},	// 15f
	{ NULL,					0	},	// 160
	{ clif_parse_GuildChangePositionInfo,	0	},	// 161
	{ NULL,					0	},	// 162
	{ NULL,					0	},	// 163
	{ NULL,					0	},	// 164
	{ clif_parse_CreateGuild,		0	},	// 165
	{ NULL,					0	},	// 166
	{ NULL,					0	},	// 167
	{ clif_parse_GuildInvite,		2000	},	// 168
	{ NULL,					0	},	// 169
	{ NULL,					0	},	// 16a
	{ clif_parse_GuildReplyInvite,		0	},	// 16b
	{ NULL,					0	},	// 16c
	{ NULL,					0	},	// 16d
	{ clif_parse_GuildChangeNotice,		0	},	// 16e
	{ NULL,					0	},	// 16f
	{ clif_parse_GuildRequestAlliance,	0	},	// 170
	{ NULL,					0	},	// 171
	{ clif_parse_GuildReplyAlliance,	0	},	// 172
	{ NULL,					0	},	// 173
	{ NULL,					0	},	// 174
	{ NULL,					0	},	// 175
	{ NULL,					0	},	// 176
	{ NULL,					0	},	// 177
	{ clif_parse_ItemIdentify,		0	},	// 178
	{ NULL,					0	},	// 179
	{ clif_parse_UseCard,			0	},	// 17a
	{ NULL,					0	},	// 17b
	{ clif_parse_InsertCard,		0	},	// 17c
	{ NULL,					0	},	// 17d
	{ clif_parse_GuildMessage,		300	},	// 17e
	{ NULL,					0	},	// 17f
	{ clif_parse_GuildOpposition,		0	},	// 180
	{ NULL,					0	},	// 181
	{ NULL,					0	},	// 182
	{ clif_parse_GuildDelAlliance,		0	},	// 183
	{ NULL,					0	},	// 184
	{ NULL,					0	},	// 185
	{ NULL,					0	},	// 186
	{ NULL,					0	},	// 187
	{ NULL,					0	},	// 188
	{ NULL,					0	},	// 189
	{ clif_parse_QuitGame,			0	},	// 18a
	{ NULL,					0	},	// 18b
	{ NULL,					0	},	// 18c
	{ NULL,					0	},	// 18d
	{ NULL,					0	},	// 18e
	{ NULL,					0	},	// 18f
	{ clif_parse_UseSkillToPos,		0	},	// 190
	{ NULL,					0	},	// 191
	{ NULL,					0	},	// 192
	{ clif_parse_SolveCharName,		0	},	// 193
	{ NULL,					0	},	// 194
	{ NULL,					0	},	// 195
	{ NULL,					0	},	// 196
	{ clif_parse_ResetChar,			0	},	// 197
	{ NULL,					0	},	// 198
	{ NULL,					0	},	// 199
	{ NULL,					0	},	// 19a
	{ NULL,					0	},	// 19b
	{ clif_parse_LGMmessage,		0	},	// 19c
	{ clif_parse_GMHide,			300	},	// 19d
	{ NULL,					0	},	// 19e
	{ NULL,					0	},	// 19f
	{ NULL,					0	},	// 1a0
	{ NULL,					0	},	// 1a1
	{ NULL,					0	},	// 1a2
	{ NULL,					0	},	// 1a3
	{ NULL,					0	},	// 1a4
	{ NULL,					0	},	// 1a5
	{ NULL,					0	},	// 1a6
	{ NULL,					0	},	// 1a7
	{ NULL,					0	},	// 1a8
	{ NULL,					0	},	// 1a9
	{ NULL,					0	},	// 1aa
	{ NULL,					0	},	// 1ab
	{ NULL,					0	},	// 1ac
	{ NULL,					0	},	// 1ad
	{ NULL,					0	},	// 1ae
	{ clif_parse_ChangeCart,		0	},	// 1af
	{ NULL,					0	},	// 1b0
	{ NULL,					0	},	// 1b1
	{ NULL,					0	},	// 1b2
	{ NULL,					0	},	// 1b3
	{ NULL,					0	},	// 1b4
	{ NULL,					0	},	// 1b5
	{ NULL,					0	},	// 1b6
	{ NULL,					0	},	// 1b7
	{ NULL,					0	},	// 1b8
	{ NULL,					0	},	// 1b9
	{ clif_parse_Shift,			300	},	// 1ba
	{ clif_parse_Shift,			300	},	// 1bb
	{ clif_parse_Recall,			300	},	// 1bc
	{ clif_parse_Recall,			300	},	// 1bd
	{ NULL,					0	},	// 1be
	{ NULL,					0	},	// 1bf
	{ NULL,					0	},	// 1c0
	{ NULL,					0	},	// 1c1
	{ NULL,					0	},	// 1c2
	{ NULL,					0	},	// 1c3
	{ NULL,					0	},	// 1c4
	{ NULL,					0	},	// 1c5
	{ NULL,					0	},	// 1c6
	{ NULL,					0	},	// 1c7
	{ NULL,					0	},	// 1c8
	{ NULL,					0	},	// 1c9
	{ NULL,					0	},	// 1ca
	{ NULL,					0	},	// 1cb
	{ NULL,					0	},	// 1cc
	{ NULL,					0	},	// 1cd
	{ clif_parse_AutoSpell,			0	},	// 1ce
	{ NULL,					0	},	// 1cf
	{ NULL,					0	},	// 1d0
	{ NULL,					0	},	// 1d1
	{ NULL,					0	},	// 1d2
	{ NULL,					0	},	// 1d3
	{ NULL,					0	},	// 1d4
	{ clif_parse_NpcStringInput,		300	},	// 1d5
	{ NULL,					0	},	// 1d6
	{ NULL,					0	},	// 1d7
	{ NULL,					0	},	// 1d8
	{ NULL,					0	},	// 1d9
	{ NULL,					0	},	// 1da
	{ NULL,					0	},	// 1db
	{ NULL,					0	},	// 1dc
	{ NULL,					0	},	// 1dd
	{ NULL,					0	},	// 1de
	{ clif_parse_GMReqNoChatCount,		0	},	// 1df
	{ NULL,					0	},	// 1e0
	{ NULL,					0	},	// 1e1
	{ NULL,					0	},	// 1e2
	{ NULL,					0	},	// 1e3
	{ NULL,					0	},	// 1e4
	{ NULL,					0	},	// 1e5
	{ NULL,					0	},	// 1e6
	{ clif_parse_sn_doridori,		0	},	// 1e7
	{ clif_parse_CreateParty2,		1000	},	// 1e8
	{ NULL,					0	},	// 1e9
	{ NULL,					0	},	// 1ea
	{ NULL,					0	},	// 1eb
	{ NULL,					0	},	// 1ec
	{ clif_parse_sn_explosionspirits,	0	},	// 1ed
	{ NULL,					0	},	// 1ee
	{ NULL,					0	},	// 1ef
	{ NULL,					0	},	// 1f0
	{ NULL,					0	},	// 1f1
	{ NULL,					0	},	// 1f2
	{ NULL,					0	},	// 1f3
	{ NULL,					0	},	// 1f4
	{ NULL,					0	},	// 1f5
	{ NULL,					0	},	// 1f6
	{ NULL,					0	},	// 1f7
	{ NULL,					0	},	// 1f8
	{ NULL,					0	},	// 1f9
	{ NULL,					0	},	// 1fa
	{ NULL,					0	},	// 1fb
	{ NULL,					0	},	// 1fc
	{ NULL,					0	},	// 1fd
	{ NULL,					0	},	// 1fe
	{ NULL,					0	},	// 1ff
	{ NULL,					0	},	// 200
	{ NULL,					0	},	// 201
	{ NULL,					0	},	// 202
	{ NULL,					0	},	// 203
	{ NULL,					0	},	// 204
	{ NULL,					0	},	// 205
	{ NULL,					0	},	// 206
	{ NULL,					0	},	// 207
	{ NULL,					0	},	// 208
	{ NULL,					0	},	// 209
	{ NULL,					0	},	// 20a
	{ NULL,					0	},	// 20b
	{ NULL,					0	},	// 20c
	{ NULL,					0	},	// 20d
	{ NULL,					0	},	// 20e
	{ NULL,					0	},	// 20f
	{ NULL,					0	},	// 210
	{ NULL,					0	},	// 211
	{ NULL,					0	},	// 212
	{ NULL,					0	},	// 213
	{ NULL,					0	},	// 214
	{ NULL,					0	},	// 215
	{ NULL,					0	},	// 216
	{ NULL,					0	},	// 217
	{ NULL,					0	},	// 218
	{ NULL,					0	},	// 219
	{ NULL,					0	},	// 21a
	{ NULL,					0	},	// 21b
	{ NULL,					0	},	// 21c
	{ NULL,					0	},	// 21d
	{ NULL,					0	},	// 21e
	{ NULL,					0	},	// 21f
};
// *INDENT-ON*

// Checks for packet flooding
int clif_check_packet_flood(fd, cmd)
{
    struct map_session_data *sd = session[fd]->session_data;
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
        int action_type = RFIFOB (fd, 6);
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
            printf("packet flood detected from %s [0x%x]\n", sd->status.name, cmd);
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
    printf ("clif_validate_chat(): %s (ID %d) sent a malformed" \
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
static char *clif_validate_chat (struct map_session_data *sd, int type,
                                 char **message, size_t *message_len)
{
    int fd;
    unsigned int buf_len;       /* Actual message length. */
    unsigned int msg_len;       /* Reported message length. */
    unsigned int min_len;       /* Minimum message length. */
    size_t name_len;            /* Sender's name length. */
    char *buf = NULL;           /* Copy of actual message data. */
    char *p = NULL;             /* Temporary pointer to message. */

    *message = NULL;
    *message_len = 0;

    nullpo_retr (NULL, sd);
    /*
     * Don't send chat in the period between the ban and the connection's
     * closure.
     */
    if (type < 0 || type > 2 || sd->auto_ban_info.in_progress)
        return NULL;

    fd = sd->fd;
    msg_len = RFIFOW (fd, 2) - 4;
    name_len = strlen (sd->status.name);
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
        WARN_MALFORMED_MSG (sd, "no message sent");
        return NULL;
    }

    /* The client sent (or claims to have sent) an empty message. */
    if (msg_len == min_len)
    {
        WARN_MALFORMED_MSG (sd, "empty message");
        return NULL;
    }

    /* The protocol specifies that the target must be 24 bytes long. */
    if (type == 1 && msg_len < min_len)
    {
        /* Disallow malformed messages. */
        clif_setwaitclose (fd);
        WARN_MALFORMED_MSG (sd, "illegal target name");
        return NULL;
    }

    p = (char *) (type != 1) ? RFIFOP (fd, 4) : RFIFOP (fd, 28);
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
        WARN_MALFORMED_MSG (sd, "exceeded maximum message length");
        return NULL;
    }

    /* We're leaving an extra eight bytes for public/global chat, 1 for NUL. */
    buf_len += (type == 2) ? 8 + 1 : 1;

    buf = (char *) aMalloc (buf_len);
    memcpy ((type != 2) ? buf : buf + 8, p,
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
        char *pos = NULL;
        if (!(pos = strstr(p, " : "))
                || strncmp (p, sd->status.name, name_len)
                || pos - p != name_len)
        {
            free (buf);
            /* Disallow malformed/spoofed messages. */
            clif_setwaitclose (fd);
            WARN_MALFORMED_MSG (sd, "spoofed name/invalid format");
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
static int clif_parse (int fd)
{
    int  packet_len = 0, cmd = 0;
    struct map_session_data *sd = NULL;

    sd = session[fd]->session_data;

    if (!sd || (sd && !sd->state.auth))
    {
        if (RFIFOREST (fd) < 2)
        {                       // too small a packet disconnect
            session[fd]->eof = 1;
        }
        if (RFIFOW (fd, 0) != 0x72)
        {                       // first packet not auth, disconnect
            session[fd]->eof = 1;
        }
    }

    // 接続が切れてるので後始末
    if (!chrif_isconnect () || session[fd]->eof)
    {                           // char鯖に繋がってない間は接続禁止 (!chrif_isconnect())
        if (sd && sd->state.auth)
        {
            pc_logout (sd);
            clif_quitsave (fd, sd);
            if (sd->status.name != NULL)
                printf ("Player [%s] has logged off your server.\n", sd->status.name);  // Player logout display [Valaris]
            else
                printf ("Player with account [%d] has logged off your server.\n", sd->bl.id);   // Player logout display [Yor]
        }
        else if (sd)
        {                       // not authentified! (refused by char-server or disconnect before to be authentified)
            printf ("Player with account [%d] has logged off your server (not auth account).\n", sd->bl.id);    // Player logout display [Yor]
            map_deliddb (&sd->bl);  // account_id has been included in the DB before auth answer
        }
        if (fd)
            close (fd);
        if (fd)
            delete_session (fd);
        return 0;
    }

    if (RFIFOREST (fd) < 2)
        return 0;               // Too small (no packet number)

    cmd = RFIFOW (fd, 0);

    // 管理用パケット処理
    if (cmd >= 30000)
    {
        switch (cmd)
        {
            case 0x7530:       // Athena情報所得
                WFIFOW (fd, 0) = 0x7531;
                WFIFOB (fd, 2) = ATHENA_MAJOR_VERSION;
                WFIFOB (fd, 3) = ATHENA_MINOR_VERSION;
                WFIFOB (fd, 4) = ATHENA_REVISION;
                WFIFOB (fd, 5) = ATHENA_RELEASE_FLAG;
                WFIFOB (fd, 6) = ATHENA_OFFICIAL_FLAG;
                WFIFOB (fd, 7) = ATHENA_SERVER_MAP;
                WFIFOW (fd, 8) = ATHENA_MOD_VERSION;
                WFIFOSET (fd, 10);
                RFIFOSKIP (fd, 2);
                break;
            case 0x7532:       // 接続の切断
                session[fd]->eof = 1;
                break;
        }
        return 0;
    }
    else if (cmd >= 0x200)
        return 0;

    // パケット長を計算
    packet_len = packet_len_table[cmd];
    if (packet_len == -1)
    {
        if (RFIFOREST (fd) < 4)
        {
            return 0;           // Runt packet (variable length without a length sent)
        }
        packet_len = RFIFOW (fd, 2);
        if (packet_len < 4 || packet_len > 32768)
        {
            session[fd]->eof = 1;
            return 0;           // Runt packet (variable out of bounds)
        }
    }

    if (RFIFOREST (fd) < packet_len)
    {
        return 0;               // Runt packet (sent legnth is too small)
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
            return 0;
        }

        clif_parse_func_table[cmd].func (fd, sd);
    }
    else
    {
        // 不明なパケット
        if (battle_config.error_log)
        {
            if (fd)
                printf ("\nclif_parse: session #%d, packet 0x%x, lenght %d\n",
                        fd, cmd, packet_len);
#ifdef DUMP_UNKNOWN_PACKET
            {
                int  i;
                FILE *fp;
                char packet_txt[256] = "save/packet.txt";
                time_t now;
                printf
                    ("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
                for (i = 0; i < packet_len; i++)
                {
                    if ((i & 15) == 0)
                        printf ("\n%04X ", i);
                    printf ("%02X ", RFIFOB (fd, i));
                }
                if (sd && sd->state.auth)
                {
                    if (sd->status.name != NULL)
                        printf
                            ("\nAccount ID %d, character ID %d, player name %s.\n",
                             sd->status.account_id, sd->status.char_id,
                             sd->status.name);
                    else
                        printf ("\nAccount ID %d.\n", sd->bl.id);
                }
                else if (sd)    // not authentified! (refused by char-server or disconnect before to be authentified)
                    printf ("\nAccount ID %d.\n", sd->bl.id);

                if ((fp = fopen_ (packet_txt, "a")) == NULL)
                {
                    printf ("clif.c: cant write [%s] !!! data is lost !!!\n",
                            packet_txt);
                    return 1;
                }
                else
                {
                    time (&now);
                    if (sd && sd->state.auth)
                    {
                        if (sd->status.name != NULL)
                            fprintf (fp,
                                     "%sPlayer with account ID %d (character ID %d, player name %s) sent wrong packet:\n",
                                     asctime (gmtime (&now)),
                                     sd->status.account_id,
                                     sd->status.char_id, sd->status.name);
                        else
                            fprintf (fp,
                                     "%sPlayer with account ID %d sent wrong packet:\n",
                                     asctime (gmtime (&now)), sd->bl.id);
                    }
                    else if (sd)    // not authentified! (refused by char-server or disconnect before to be authentified)
                        fprintf (fp,
                                 "%sPlayer with account ID %d sent wrong packet:\n",
                                 asctime (gmtime (&now)), sd->bl.id);

                    fprintf (fp,
                             "\t---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
                    for (i = 0; i < packet_len; i++)
                    {
                        if ((i & 15) == 0)
                            fprintf (fp, "\n\t%04X ", i);
                        fprintf (fp, "%02X ", RFIFOB (fd, i));
                    }
                    fprintf (fp, "\n\n");
                    fclose_ (fp);
                }
            }
#endif
        }
    }
    RFIFOSKIP (fd, packet_len);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_clif (void)
{
    int  i;

    set_defaultparse (clif_parse);
    for (i = 0; i < 10; i++)
    {
        if (make_listen_port (map_port))
            break;
#ifdef LCCWIN32
        Sleep (20000);
#else
        sleep (20);
#endif
    }
    if (i == 10)
    {
        printf ("cant bind game port\n");
        exit (1);
    }

    add_timer_func_list (clif_waitclose, "clif_waitclose");
    add_timer_func_list (clif_clearchar_delay_sub,
                         "clif_clearchar_delay_sub");

    return 0;
}
