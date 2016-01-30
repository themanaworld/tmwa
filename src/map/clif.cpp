#include "clif.hpp"
//    clif.cpp - Network interface to the client.
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
#include <ctime>

#include <algorithm>

#include "../compat/attr.hpp"
#include "../compat/fun.hpp"
#include "../compat/nullpo.hpp"

#include "../ints/cmp.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/write.hpp"

#include "../net/ip.hpp"
#include "../net/socket.hpp"
#include "../net/timer.hpp"
#include "../net/timestamp-utils.hpp"

#include "../proto2/any-user.hpp"
#include "../proto2/char-map.hpp"
#include "../proto2/map-user.hpp"

#include "../mmo/cxxstdio_enums.hpp"
#include "../mmo/version.hpp"

#include "../high/md5more.hpp"

#include "../wire/packets.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "battle_conf.hpp"
#include "chrif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "magic.hpp"
#include "magic-stmt.hpp"
#include "map.hpp"
#include "map_conf.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "tmw.hpp"
#include "trade.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
constexpr int EMOTE_IGNORED = 0x0e;

// functions list. Rate is how many milliseconds are required between
// calls. Packets exceeding this rate will be dropped. flood_rates in
// map.h must be the same length as this table. rate 0 is default
// rate -1 is unlimited

typedef RecvResult (*clif_func)(Session *s, dumb_ptr<map_session_data> sd);
struct func_table
{
    interval_t rate;
    uint16_t len_unused;
    clif_func func;

    // ctor exists because interval_t must be explicit
    func_table(int r, uint16_t l, clif_func f)
    : rate(r), len_unused(l), func(f)
    {}
};

constexpr uint16_t VAR = 1;

extern // not really - defined below
func_table clif_parse_func_table[0x0220];


// local define
enum class SendWho
{
    ALL_CLIENT,
    ALL_SAMEMAP,
    AREA,
    AREA_WOS,
    AREA_CHAT_WOC,
    PARTY,
    PARTY_WOS,
    PARTY_SAMEMAP,
    PARTY_SAMEMAP_WOS,
    PARTY_AREA,
    PARTY_AREA_WOS,
    SELF,
};

static
int clif_changelook_towards(dumb_ptr<block_list> bl, LOOK type, int val,
                             dumb_ptr<map_session_data> dstsd);
static
void clif_quitsave(Session *, dumb_ptr<map_session_data> sd);

static
void clif_sitnpc_sub(Buffer& buf, dumb_ptr<npc_data> nd, DamageType dmg);

static
void clif_delete(Session *s)
{
    assert (s != char_session);

    dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
    if (sd && sd->state.auth)
    {
        pc_logout(sd);
        clif_quitsave(s, sd);

        PRINTF("Player [%s] has logged off your server.\n"_fmt, sd->status_key.name);  // Player logout display [Valaris]
    }
    else if (sd)
    {                       // not authentified! (refused by char-server or disconnect before to be authentified)
        PRINTF("Player with account [%d] has logged off your server (not auth account).\n"_fmt, sd->bl_id);    // Player logout display [Yor]
        map_deliddb(sd);  // account_id has been included in the DB before auth answer
    }
}


/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void)
{
    int users = 0;

    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
        if (sd && sd->state.auth && !sd->state.connect_new && !(battle_config.hide_GM_session && pc_isGM(sd)))
            users++;
    }
    return users;
}

/*==========================================
 * 全てのclientに対してfunc()実行
 *------------------------------------------
 */
int clif_foreachclient(std::function<void (dumb_ptr<map_session_data>)> func)
{
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
        if (sd && sd->state.auth && !sd->state.connect_new)
            func(sd);
    }
    return 0;
}

static
int is_deaf(dumb_ptr<block_list> bl)
{
    if (!bl || bl->bl_type != BL::PC)
        return 0;
    dumb_ptr<map_session_data> sd = bl->is_player();
    return sd->special_state.deaf;
}

enum class ChatType
{
    Party,
    Whisper,
    Global,
};

static
AString clif_validate_chat(dumb_ptr<map_session_data> sd, ChatType type, XString buf);

/*==========================================
 * clif_sendでSendWho::AREA*指定時用
 *------------------------------------------
 */
static
void clif_send_sub(dumb_ptr<block_list> bl, const Buffer& buf,
        dumb_ptr<block_list> src_bl, SendWho type, short min_version)
{
    nullpo_retv(bl);
    dumb_ptr<map_session_data> sd = bl->is_player();

    switch (type)
    {
        case SendWho::AREA_WOS:
            if (bl && bl == src_bl)
                return;
            break;

        case SendWho::AREA_CHAT_WOC:
            if (is_deaf(bl)
                && !(bl->bl_type == BL::PC
                     && pc_isGM(src_bl->is_player())))
            {
                clif_emotion_towards(src_bl, bl, EMOTE_IGNORED);
                return;
            }
            if (bl && bl == src_bl)
                return;

            break;
    }

    if (sd->sess != nullptr)
    {
        {
            if (sd->client_version >= min_version)
            {
                send_buffer(sd->sess, buf);
            }
        }
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_send(const Buffer& buf, dumb_ptr<block_list> bl, SendWho type, short min_version)
{
    int x0 = 0, x1 = 0, y0 = 0, y1 = 0;

    if (type != SendWho::ALL_CLIENT)
    {
        nullpo_retz(bl);

        if (bl->bl_type == BL::PC)
        {
            dumb_ptr<map_session_data> sd2 = bl->is_player();
            if (bool(sd2->status.option & Opt0::INVISIBILITY))
            {
                // Obscure hidden GMs

                switch (type)
                {
                    case SendWho::AREA:
                        type = SendWho::SELF;
                        break;

                    case SendWho::AREA_WOS:
                        return 1;

                    default:
                        break;
                }
            }
        }
    }

    switch (type)
    {
        case SendWho::ALL_CLIENT:       // 全クライアントに送信
            for (io::FD i : iter_fds())
            {
                Session *s = get_session(i);
                if (!s)
                    continue;
                dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
                if (sd && sd->state.auth && !sd->state.connect_new)
                {
                    {
                        send_buffer(s, buf);
                    }
                }
            }
            break;
        case SendWho::ALL_SAMEMAP:      // 同じマップの全クライアントに送信
            for (io::FD i : iter_fds())
            {
                Session *s = get_session(i);
                if (!s)
                    continue;
                dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
                if (sd && sd->state.auth && !sd->state.connect_new && sd->bl_m == bl->bl_m)
                {
                    {
                        send_buffer(s, buf);
                    }
                }
            }
            break;
        case SendWho::AREA:
        case SendWho::AREA_WOS:
        {
            if (bl->bl_m != borrow(undefined_gat))
            {
                map_foreachinarea(std::bind(clif_send_sub, ph::_1, buf, bl, type, min_version),
                        bl->bl_m,
                        bl->bl_x - AREA_SIZE, bl->bl_y - AREA_SIZE,
                        bl->bl_x + AREA_SIZE, bl->bl_y + AREA_SIZE,
                        BL::PC);
            }
        }
            break;
        case SendWho::AREA_CHAT_WOC:
        {
            if (bl->bl_m != borrow(undefined_gat))
            {
                map_foreachinarea(std::bind(clif_send_sub, ph::_1, buf, bl, SendWho::AREA_CHAT_WOC, min_version),
                        bl->bl_m,
                        bl->bl_x - (AREA_SIZE), bl->bl_y - (AREA_SIZE),
                        bl->bl_x + (AREA_SIZE), bl->bl_y + (AREA_SIZE),
                        BL::PC);
            }
        }
            break;

        case SendWho::PARTY_AREA:       // 同じ画面内の全パーティーメンバに送信
        case SendWho::PARTY_AREA_WOS:   // 自分以外の同じ画面内の全パーティーメンバに送信
            x0 = bl->bl_x - AREA_SIZE;
            y0 = bl->bl_y - AREA_SIZE;
            x1 = bl->bl_x + AREA_SIZE;
            y1 = bl->bl_y + AREA_SIZE;
            FALLTHROUGH;
        case SendWho::PARTY:            // 全パーティーメンバに送信
        case SendWho::PARTY_WOS:        // 自分以外の全パーティーメンバに送信
        case SendWho::PARTY_SAMEMAP:    // 同じマップの全パーティーメンバに送信
        case SendWho::PARTY_SAMEMAP_WOS:    // 自分以外の同じマップの全パーティーメンバに送信
        {
            Option<PartyPair> p_ = None;
            if (bl->bl_type == BL::PC)
            {
                dumb_ptr<map_session_data> sd = bl->is_player();
                if (sd->partyspy)
                {
                    p_ = party_search(sd->partyspy);
                }
                else
                {
                    if (sd->status.party_id)
                        p_ = party_search(sd->status.party_id);
                }
            }
            OMATCH_BEGIN_SOME (p, p_)
            {
                for (int i = 0; i < MAX_PARTY; i++)
                {
                    dumb_ptr<map_session_data> sd =  dumb_ptr<map_session_data>(p->member[i].sd);
                    if (sd)
                    {
                        if (sd->bl_id == bl->bl_id && (type == SendWho::PARTY_WOS ||
                                                    type == SendWho::PARTY_SAMEMAP_WOS
                                                    || type == SendWho::PARTY_AREA_WOS))
                            continue;
                        if (type != SendWho::PARTY && type != SendWho::PARTY_WOS && bl->bl_m != sd->bl_m)    // マップチェック
                            continue;
                        if ((type == SendWho::PARTY_AREA || type == SendWho::PARTY_AREA_WOS) &&
                            (sd->bl_x < x0 || sd->bl_y < y0 ||
                             sd->bl_x > x1 || sd->bl_y > y1))
                            continue;
                        if (sd->client_version >= min_version)
                        {
                            send_buffer(sd->sess, buf);
                        }
                    }
                }
                for (io::FD i : iter_fds())
                {
                    Session *s = get_session(i);
                    if (!s)
                        continue;
                    dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
                    if (sd && sd->state.auth && !sd->state.connect_new)
                    {
                        if (sd->partyspy == p.party_id)
                        {
                            if (sd->client_version >= min_version)
                            {
                                send_buffer(sd->sess, buf);
                            }
                        }
                    }
                }
            }
            OMATCH_END ();
        }
            break;
        case SendWho::SELF:
        {
            dumb_ptr<map_session_data> sd = bl->is_player();

            {
                if (sd->client_version >= min_version)
                    send_buffer(sd->sess, buf);
            }
        }
            break;

        default:
            if (battle_config.error_log)
                PRINTF("clif_send まだ作ってないよー\n"_fmt);
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
int clif_authok(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (!sd)
        return 0;

    if (!sd->sess)
        return 0;

    Session *s = sd->sess;

    Packet_Fixed<0x0073> fixed_73;
    fixed_73.tick = gettick();
    fixed_73.pos = Position1{static_cast<uint16_t>(sd->bl_x), static_cast<uint16_t>(sd->bl_y), DIR::S};
    fixed_73.five1 = 5;
    fixed_73.five2 = 5;
    send_fpacket<0x0073, 11>(s, fixed_73);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_authfail_fd(Session *s, int type)
{
    if (!s)
        return 0;

    Packet_Fixed<0x0081> fixed_81;
    fixed_81.error_code = type;
    send_fpacket<0x0081, 3>(s, fixed_81);

    clif_setwaitclose(s);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok(BlockId id)
{
    dumb_ptr<map_session_data> sd;

    if ((sd = map_id2sd(id)) == nullptr)
        return 1;

    if (!sd->sess)
        return 1;

    Session *s = sd->sess;
    Packet_Fixed<0x00b3> fixed_b3;
    fixed_b3.one = 1;
    send_fpacket<0x00b3, 3>(s, fixed_b3);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_set009e(dumb_ptr<flooritem_data> fitem, Buffer& buf)
{
    nullpo_retv(fitem);

    Packet_Fixed<0x009e> fixed_9e;
    fixed_9e.block_id = fitem->bl_id;
    fixed_9e.name_id = fitem->item_data.nameid;
    fixed_9e.identify = 1;
    fixed_9e.x = fitem->bl_x;
    fixed_9e.y = fitem->bl_y;
    fixed_9e.subx = fitem->subx;
    fixed_9e.suby = fitem->suby;
    fixed_9e.amount = fitem->item_data.amount;

    buf = create_fpacket<0x009e, 17>(fixed_9e);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem(dumb_ptr<flooritem_data> fitem)
{
    nullpo_retz(fitem);

    if (!fitem->item_data.nameid)
        return 0;

    Buffer buf;
    clif_set009e(fitem, buf);
    clif_send(buf, fitem, SendWho::AREA, MIN_CLIENT_VERSION);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearflooritem(dumb_ptr<flooritem_data> fitem, Session *s)
{
    nullpo_retz(fitem);

    Packet_Fixed<0x00a1> fixed_a1;
    fixed_a1.block_id = fitem->bl_id;

    if (!s)
    {
        Buffer buf = create_fpacket<0x00a1, 6>(fixed_a1);
        clif_send(buf, fitem, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    else
    {
        send_fpacket<0x00a1, 6>(s, fixed_a1);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar(dumb_ptr<block_list> bl, BeingRemoveWhy type)
{
    nullpo_retz(bl);

    Packet_Fixed<0x0080> fixed_80;
    fixed_80.block_id = bl->bl_id;

    if (type == BeingRemoveWhy::DISGUISE)
    {
        fixed_80.type = BeingRemoveWhy::GONE;
        Buffer buf = create_fpacket<0x0080, 7>(fixed_80);
        clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    else
    {
        fixed_80.type = type;
        Buffer buf = create_fpacket<0x0080, 7>(fixed_80);
        clif_send(buf, bl,
                   type == BeingRemoveWhy::DEAD ? SendWho::AREA : SendWho::AREA_WOS, MIN_CLIENT_VERSION);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_clearchar_id(BlockId id, BeingRemoveWhy type, Session *s)
{
    Packet_Fixed<0x0080> fixed_80;
    fixed_80.block_id = id;
    fixed_80.type = type;
    send_fpacket<0x0080, 7>(s, fixed_80);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_set0078_main_1d8(dumb_ptr<map_session_data> sd, Buffer& buf)
{
    nullpo_retv(sd);

    Packet_Fixed<0x01d8> fixed_1d8;
    fixed_1d8.block_id = sd->bl_id;
    fixed_1d8.speed = sd->speed;
    fixed_1d8.opt1 = sd->opt1;
    fixed_1d8.opt2 = sd->opt2;
    fixed_1d8.option = sd->status.option;
    fixed_1d8.species = sd->status.species;
    fixed_1d8.hair_style = sd->status.hair;

    IOff0 widx = sd->equip_index_maybe[EQUIP::WEAPON];
    IOff0 sidx = sd->equip_index_maybe[EQUIP::SHIELD];
    if (sd->attack_spell_override)
        fixed_1d8.weapon = sd->attack_spell_look_override;
    else
    {
        if (widx.ok() && sd->inventory_data[widx].is_some())
        {
            fixed_1d8.weapon = sd->status.inventory[widx].nameid;
        }
        else
            fixed_1d8.weapon = ItemNameId();
    }
    if (sidx.ok() && sidx != widx && sd->inventory_data[sidx].is_some())
    {
        fixed_1d8.shield = sd->status.inventory[sidx].nameid;
    }
    else
        fixed_1d8.shield = ItemNameId();
    fixed_1d8.head_bottom = sd->status.head_bottom;
    fixed_1d8.head_top = sd->status.head_top;
    fixed_1d8.head_mid = sd->status.head_mid;
    fixed_1d8.hair_color = sd->status.hair_color;
    fixed_1d8.clothes_color = sd->status.clothes_color;
    fixed_1d8.head_dir = sd->head_dir;
    fixed_1d8.guild_id = 0;
    fixed_1d8.guild_emblem_id = 0;
    fixed_1d8.manner = sd->status.manner;
    fixed_1d8.opt3 = sd->opt3;
    fixed_1d8.karma = sd->status.karma;
    fixed_1d8.sex = sd->status.sex;
    fixed_1d8.pos.x = sd->bl_x;
    fixed_1d8.pos.y = sd->bl_y;
    fixed_1d8.pos.dir = sd->dir;
    fixed_1d8.gm_bits = pc_isGM(sd).get_public_word();
    fixed_1d8.dead_sit = sd->state.dead_sit;
    fixed_1d8.unused = 0;

    buf = create_fpacket<0x01d8, 54>(fixed_1d8);
}
static
void clif_set0078_alt_1d9(dumb_ptr<map_session_data> sd, Buffer& buf)
{
    nullpo_retv(sd);

    Packet_Fixed<0x01d9> fixed_1d8; // LIES
    fixed_1d8.block_id = sd->bl_id;
    fixed_1d8.speed = sd->speed;
    fixed_1d8.opt1 = sd->opt1;
    fixed_1d8.opt2 = sd->opt2;
    fixed_1d8.option = sd->status.option;
    fixed_1d8.species = sd->status.species;
    fixed_1d8.hair_style = sd->status.hair;

    IOff0 widx = sd->equip_index_maybe[EQUIP::WEAPON];
    IOff0 sidx = sd->equip_index_maybe[EQUIP::SHIELD];
    if (sd->attack_spell_override)
        fixed_1d8.weapon = sd->attack_spell_look_override;
    else
    {
        if (widx.ok() && sd->inventory_data[widx].is_some())
        {
            fixed_1d8.weapon = sd->status.inventory[widx].nameid;
        }
        else
            fixed_1d8.weapon = ItemNameId();
    }
    if (sidx.ok() && sidx != widx && sd->inventory_data[sidx].is_some())
    {
        fixed_1d8.shield = sd->status.inventory[sidx].nameid;
    }
    else
        fixed_1d8.shield = ItemNameId();
    fixed_1d8.head_bottom = sd->status.head_bottom;
    fixed_1d8.head_top = sd->status.head_top;
    fixed_1d8.head_mid = sd->status.head_mid;
    fixed_1d8.hair_color = sd->status.hair_color;
    fixed_1d8.clothes_color = sd->status.clothes_color;
    fixed_1d8.head_dir = sd->head_dir;
    fixed_1d8.guild_id = 0;
    fixed_1d8.guild_emblem_id = 0;
    fixed_1d8.manner = sd->status.manner;
    fixed_1d8.opt3 = sd->opt3;
    fixed_1d8.karma = sd->status.karma;
    fixed_1d8.sex = sd->status.sex;
    fixed_1d8.pos.x = sd->bl_x;
    fixed_1d8.pos.y = sd->bl_y;
    fixed_1d8.pos.dir = sd->dir;
    fixed_1d8.gm_bits = pc_isGM(sd).get_public_word();
    fixed_1d8.unused = 0;

    buf = create_fpacket<0x01d9, 53>(fixed_1d8);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_set007b(dumb_ptr<map_session_data> sd, Buffer& buf)
{
    nullpo_retv(sd);

    Packet_Fixed<0x01da> fixed_1da;
    fixed_1da.block_id = sd->bl_id;
    fixed_1da.speed = sd->speed;
    fixed_1da.opt1 = sd->opt1;
    fixed_1da.opt2 = sd->opt2;
    fixed_1da.option = sd->status.option;
    fixed_1da.species = sd->status.species;
    fixed_1da.hair_style = sd->status.hair;
    IOff0 widx = sd->equip_index_maybe[EQUIP::WEAPON];
    IOff0 sidx = sd->equip_index_maybe[EQUIP::SHIELD];
    if (widx.ok() && sd->inventory_data[widx].is_some())
    {
        fixed_1da.weapon = sd->status.inventory[widx].nameid;
    }
    else
        fixed_1da.weapon = ItemNameId();
    if (sidx.ok() && sidx != widx && sd->inventory_data[sidx].is_some())
    {
        fixed_1da.shield = sd->status.inventory[sidx].nameid;
    }
    else
        fixed_1da.shield = ItemNameId();
    fixed_1da.head_bottom = sd->status.head_bottom;
    fixed_1da.tick = gettick();
    fixed_1da.head_top = sd->status.head_top;
    fixed_1da.head_mid = sd->status.head_mid;
    fixed_1da.hair_color = sd->status.hair_color;
    fixed_1da.clothes_color = sd->status.clothes_color;
    fixed_1da.head_dir = sd->head_dir;
    fixed_1da.guild_id = 0;
    fixed_1da.guild_emblem_id = 0;
    fixed_1da.manner = sd->status.manner;
    fixed_1da.opt3 = sd->opt3;
    fixed_1da.karma = sd->status.karma;
    fixed_1da.sex = sd->status.sex;
    fixed_1da.pos2.x0 = sd->bl_x;
    fixed_1da.pos2.y0 = sd->bl_y;
    fixed_1da.pos2.x1 = sd->to_x;
    fixed_1da.pos2.y1 = sd->to_y;
    fixed_1da.gm_bits = pc_isGM(sd).get_public_word();
    fixed_1da.five = 5;
    fixed_1da.unused = 0;

    buf = create_fpacket<0x01da, 60>(fixed_1da);
}

/*==========================================
 * MOB表示1
 *------------------------------------------
 */
static
void clif_mob0078(dumb_ptr<mob_data> md, Buffer& buf)
{
    nullpo_retv(md);
    int max_hp = md->stats[mob_stat::MAX_HP];
    int hp = md->hp;

    Packet_Fixed<0x0078> fixed_78;
    fixed_78.block_id = md->bl_id;
    fixed_78.speed = battle_get_speed(md);
    fixed_78.opt1 = md->opt1;
    fixed_78.opt2 = md->opt2;
    fixed_78.option = md->option;
    fixed_78.species = md->mob_class;
    // snip: stuff do do with disguise as a PC
    fixed_78.pos.x = md->bl_x;
    fixed_78.pos.y = md->bl_y;
    fixed_78.pos.dir = md->dir;

    fixed_78.gloves_or_part_of_hp = static_cast<short>(hp & 0xffff);
    fixed_78.part_of_guild_id_or_part_of_hp = static_cast<short>(hp >> 16);
    fixed_78.part_of_guild_id_or_part_of_max_hp = static_cast<short>(max_hp & 0xffff);
    fixed_78.guild_emblem_or_part_of_max_hp = static_cast<short>(max_hp >> 16);
    fixed_78.karma_or_attack_range = battle_get_range(md);

    buf = create_fpacket<0x0078, 54>(fixed_78);
}

void clif_npc_action(dumb_ptr<map_session_data> sd, BlockId npcid,
        short command, int id, short x, short y)
{
    nullpo_retv(sd);
    if(sd->client_version < 2)
        return;

    Packet_Fixed<0x0212> fixed_212;
    fixed_212.npc_id = npcid;
    fixed_212.command = command;
    fixed_212.id = id;
    fixed_212.x = x;
    fixed_212.y = y;

    Buffer buf = create_fpacket<0x0212, 16>(fixed_212);
    send_buffer(sd->sess, buf);
}

/*==========================================
 * MOB表示2
 *------------------------------------------
 */
static
void clif_mob007b(dumb_ptr<mob_data> md, Buffer& buf)
{
    nullpo_retv(md);
    int max_hp = md->stats[mob_stat::MAX_HP];
    int hp = md->hp;

    Packet_Fixed<0x007b> fixed_7b;
    fixed_7b.block_id = md->bl_id;
    fixed_7b.speed = battle_get_speed(md);
    fixed_7b.opt1 = md->opt1;
    fixed_7b.opt2 = md->opt2;
    fixed_7b.option = md->option;
    fixed_7b.mob_class = md->mob_class;
    // snip: stuff for monsters disguised as PCs
    fixed_7b.tick = gettick();

    fixed_7b.pos2.x0 = md->bl_x;
    fixed_7b.pos2.y0 = md->bl_y;
    fixed_7b.pos2.x1 = md->to_x;
    fixed_7b.pos2.y1 = md->to_y;

    fixed_7b.gloves_or_part_of_hp = static_cast<short>(hp & 0xffff);
    fixed_7b.part_of_guild_id_or_part_of_hp = static_cast<short>(hp >> 16);
    fixed_7b.part_of_guild_id_or_part_of_max_hp = static_cast<short>(max_hp & 0xffff);
    fixed_7b.guild_emblem_or_part_of_max_hp = static_cast<short>(max_hp >> 16);
    fixed_7b.karma_or_attack_range = battle_get_range(md);

    buf = create_fpacket<0x007b, 60>(fixed_7b);
}
/*==========================================
 * Packet to send server's mob walkpath data
 *------------------------------------------
 */
static
int clif_0225_being_move3(dumb_ptr<mob_data> md, Buffer& buf)
{
    Packet_Head<0x0225> head_225;
    std::vector<Packet_Repeat<0x0225>> repeat_225;

    head_225.magic_packet_length = md->walkpath.path_len + 14;
    head_225.id = md->bl_id;
    head_225.speed = battle_get_speed(md);
    head_225.x_position = md->bl_x;
    head_225.y_position = md->bl_y;
    for (int i = 0; i < md->walkpath.path_len; i++)
    {
        Packet_Repeat<0x0225> move_225;
        move_225.move = md->walkpath.path[i];
        repeat_225.push_back(move_225);
    }

    buf = create_vpacket<0x0225, 14, 1>(head_225, repeat_225);
    return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
static
void clif_npc0078(dumb_ptr<npc_data> nd, Buffer& buf)
{
    nullpo_retv(nd);

    Packet_Fixed<0x0078> fixed_78;
    fixed_78.block_id = nd->bl_id;
    fixed_78.speed = nd->speed;
    fixed_78.species = nd->npc_class;
    fixed_78.pos.x = nd->bl_x;
    fixed_78.pos.y = nd->bl_y;
    fixed_78.pos.dir = nd->dir;
    fixed_78.sex = nd->sex;
    buf = create_fpacket<0x0078, 54>(fixed_78);
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
int clif_spawnpc(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Buffer buf;
    clif_set0078_alt_1d9(sd, buf);

    clif_send(buf, sd, SendWho::AREA_WOS, MIN_CLIENT_VERSION);

    clif_pvpstatus(sd);

    if (sd->bl_m->flag.get(MapFlag::SNOW))
        clif_specialeffect(sd, 162, 1);
    if (sd->bl_m->flag.get(MapFlag::FOG))
        clif_specialeffect(sd, 233, 1);
    if (sd->bl_m->flag.get(MapFlag::SAKURA))
        clif_specialeffect(sd, 163, 1);
    if (sd->bl_m->flag.get(MapFlag::LEAVES))
        clif_specialeffect(sd, 333, 1);
    if (sd->bl_m->flag.get(MapFlag::RAIN))
        clif_specialeffect(sd, 161, 1);

//        clif_changelook_accessories(sd, nullptr);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc(dumb_ptr<npc_data> nd)
{
    nullpo_retz(nd);

    if (nd->flag & 1 || nd->npc_class == INVISIBLE_CLASS)
        return 0;
    /* manaplus is skipping this packet
    Packet_Fixed<0x007c> fixed_7c;
    fixed_7c.block_id = nd->bl_id;
    fixed_7c.speed = nd->speed;
    fixed_7c.species = nd->npc_class;
    fixed_7c.pos.x = nd->bl_x;
    fixed_7c.pos.y = nd->bl_y;

    Buffer buf = create_fpacket<0x007c, 41>(fixed_7c);
    clif_send(buf, nd, SendWho::AREA, MIN_CLIENT_VERSION);
    */
    Buffer buf;
    clif_npc0078(nd, buf);
    clif_send(buf, nd, SendWho::AREA, MIN_CLIENT_VERSION);

    if(nd->sit == DamageType::SIT)
    {
        Buffer buff;
        clif_sitnpc_sub(buff, nd, nd->sit);
        clif_send(buff, nd, SendWho::AREA, MIN_CLIENT_VERSION);
    }

    return 0;
}

int clif_spawn_fake_npc_for_player(dumb_ptr<map_session_data> sd, BlockId fake_npc_id)
{
    nullpo_retz(sd);

    Session *s = sd->sess;

    if (!s)
        return 0;

    Packet_Fixed<0x007c> fixed_7c;
    fixed_7c.block_id = fake_npc_id;
    fixed_7c.speed = interval_t();
    fixed_7c.opt1 = Opt1::ZERO;
    fixed_7c.opt2 = Opt2::ZERO;
    fixed_7c.option = Opt0::ZERO;
    fixed_7c.species = FAKE_NPC_CLASS;
    fixed_7c.pos.x = sd->bl_x;
    fixed_7c.pos.y = sd->bl_y;
    send_fpacket<0x007c, 41>(s, fixed_7c);

    Packet_Fixed<0x0078> fixed_78;
    fixed_78.block_id = fake_npc_id;
    fixed_78.speed = interval_t();
    fixed_78.opt1 = Opt1::ZERO;
    fixed_78.opt2 = Opt2::ZERO;
    fixed_78.option = Opt0::ZERO;
    fixed_78.species = FAKE_NPC_CLASS;
    fixed_78.pos.x = sd->bl_x;
    fixed_78.pos.y = sd->bl_y;
    send_fpacket<0x0078, 54>(s, fixed_78);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob(dumb_ptr<mob_data> md)
{
    nullpo_retz(md);

    {
        Packet_Fixed<0x007c> fixed_7c;
        fixed_7c.block_id = md->bl_id;
        fixed_7c.speed = interval_t(md->stats[mob_stat::SPEED]);
        fixed_7c.opt1 = md->opt1;
        fixed_7c.opt2 = md->opt2;
        fixed_7c.option = md->option;
        fixed_7c.species = md->mob_class;
        fixed_7c.pos.x = md->bl_x;
        fixed_7c.pos.y = md->bl_y;
        Buffer buf = create_fpacket<0x007c, 41>(fixed_7c);
        clif_send(buf, md, SendWho::AREA, MIN_CLIENT_VERSION);
    }

    Buffer buf;
    clif_mob0078(md, buf);
    clif_send(buf, md, SendWho::AREA, MIN_CLIENT_VERSION);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_servertick(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x007f> fixed_7f;
    fixed_7f.tick = gettick();
    send_fpacket<0x007f, 6>(s, fixed_7f);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x0087> fixed_87;
    fixed_87.tick = gettick();
    fixed_87.pos2.x0 = sd->bl_x;
    fixed_87.pos2.y0 = sd->bl_y;
    fixed_87.pos2.x1 = sd->to_x;
    fixed_87.pos2.y1 = sd->to_y;
    fixed_87.zero = 0;
    send_fpacket<0x0087, 12>(s, fixed_87);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Buffer buf;
    clif_set007b(sd, buf);

    clif_send(buf, sd, SendWho::AREA_WOS, MIN_CLIENT_VERSION);

    if (battle_config.save_clothcolor == 1 && sd->status.clothes_color > 0)
        clif_changelook(sd, LOOK::CLOTHES_COLOR,
                         sd->status.clothes_color);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_quitsave(Session *, dumb_ptr<map_session_data> sd)
{
    map_quit(sd);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_waitclose(TimerData *, tick_t, Session *s)
{
    if (s)
        s->set_eof();
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose(Session *s)
{
    s->timed_close = Timer(gettick() + 5_s,
            std::bind(clif_waitclose, ph::_1, ph::_2,
                s)
    );
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changemap(dumb_ptr<map_session_data> sd, MapName mapname, int x, int y)
{
    nullpo_retv(sd);

    Session *s = sd->sess;

    Packet_Fixed<0x0091> fixed_91;
    fixed_91.map_name = mapname;
    fixed_91.x = x;
    fixed_91.y = y;
    send_fpacket<0x0091, 22>(s, fixed_91);

    if(sd->bl_m->mask > 0)
        clif_send_mask(sd, sd->bl_m->mask);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changemapserver(dumb_ptr<map_session_data> sd,
        MapName mapname, int x, int y, IP4Address ip, int port)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x0092> fixed_92;
    fixed_92.map_name = mapname;
    fixed_92.x = x;
    fixed_92.y = y;
    fixed_92.ip = ip;
    fixed_92.port = port;
    send_fpacket<0x0092, 28>(s, fixed_92);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_fixpos(dumb_ptr<block_list> bl)
{
    nullpo_retv(bl);

    Packet_Fixed<0x0088> fixed_88;
    fixed_88.block_id = bl->bl_id;
    fixed_88.x = bl->bl_x;
    fixed_88.y = bl->bl_y;

    Buffer buf = create_fpacket<0x0088, 10>(fixed_88);
    clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npcbuysell(dumb_ptr<map_session_data> sd, BlockId id)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00c4> fixed_c4;
    fixed_c4.block_id = id;
    send_fpacket<0x00c4, 6>(s, fixed_c4);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_buylist(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data_shop> nd)
{
    int i, val;

    nullpo_retz(sd);
    nullpo_retz(nd);

    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x00c6>> repeat_c6(nd->shop_items.size());
    for (i = 0; i < nd->shop_items.size(); i++)
    {
        P<struct item_data> id = itemdb_search(nd->shop_items[i].nameid);
        val = nd->shop_items[i].value;
        repeat_c6[i].base_price = val; // base price
        repeat_c6[i].actual_price = val; // actual price
        repeat_c6[i].type = id->type;
        repeat_c6[i].name_id = nd->shop_items[i].nameid;
    }
    send_packet_repeatonly<0x00c6, 4, 11>(s, repeat_c6);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_selllist(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x00c7>> repeat_c7;
    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid)
            continue;
        OMATCH_BEGIN_SOME (sdidi, sd->inventory_data[i])
        {
            int val = sdidi->value_sell;
            if (val < 0)
                continue;
            Packet_Repeat<0x00c7> info;
            info.ioff2 = i.shift();
            info.base_price = val;
            info.actual_price = val;
            repeat_c7.push_back(info);
        }
        OMATCH_END ();
    }
    send_packet_repeatonly<0x00c7, 4, 10>(s, repeat_c7);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptmes(dumb_ptr<map_session_data> sd, BlockId npcid, XString mes)
{
    nullpo_retv(sd);

    Session *s = sd->sess;

    Packet_Head<0x00b4> head_b4;
    head_b4.block_id = npcid;
    send_vpacket<0x00b4, 8, 1>(s, head_b4, mes);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptnext(dumb_ptr<map_session_data> sd, BlockId npcid)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00b5> fixed_b5;
    fixed_b5.block_id = npcid;
    send_fpacket<0x00b5, 6>(s, fixed_b5);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptclose(dumb_ptr<map_session_data> sd, BlockId npcid)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00b6> fixed_b6;
    fixed_b6.block_id = npcid;
    send_fpacket<0x00b6, 6>(s, fixed_b6);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptmenu(dumb_ptr<map_session_data> sd, BlockId npcid, XString mes)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Head<0x00b7> head_b7;
    head_b7.block_id = npcid;
    send_vpacket<0x00b7, 8, 1>(s, head_b7, mes);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptinput(dumb_ptr<map_session_data> sd, BlockId npcid)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x0142> fixed_142;
    fixed_142.block_id = npcid;
    send_fpacket<0x0142, 6>(s, fixed_142);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptinputstr(dumb_ptr<map_session_data> sd, BlockId npcid)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x01d4> fixed_1d4;
    fixed_1d4.block_id = npcid;
    send_fpacket<0x01d4, 6>(s, fixed_1d4);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem(dumb_ptr<map_session_data> sd, IOff0 n, int amount, PickupFail fail)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00a0> fixed_a0;
    if (fail != PickupFail::OKAY)
    {
        fixed_a0.ioff2 = n.shift();
        fixed_a0.amount = amount;
        fixed_a0.name_id = ItemNameId();
        fixed_a0.pickup_fail = fail;
    }
    else
    {
        if (!n.ok() || !sd->status.inventory[n].nameid)
            return 1;
        auto sdidn = TRY_UNWRAP(sd->inventory_data[n], return 1);

        fixed_a0.ioff2 = n.shift();
        fixed_a0.amount = amount;
        fixed_a0.name_id = sd->status.inventory[n].nameid;
        fixed_a0.identify = 1;
        fixed_a0.broken_or_attribute = 0;
        fixed_a0.refine = 0;
        {
            fixed_a0.card0 = 0;
            fixed_a0.card1 = 0;
            fixed_a0.card2 = 0;
            fixed_a0.card3 = 0;
        }
        fixed_a0.epos = pc_equippoint(sd, n);
        fixed_a0.item_type = (sdidn->type == ItemType::_7
            ? ItemType::WEAPON
            : sdidn->type);
        fixed_a0.pickup_fail = fail;
    }

    send_fpacket<0x00a0, 23>(s, fixed_a0);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_delitem(dumb_ptr<map_session_data> sd, IOff0 n, int amount)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00af> fixed_af;
    fixed_af.ioff2 = n.shift();
    fixed_af.amount = amount;

    send_fpacket<0x00af, 6>(s, fixed_af);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_itemlist(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    IOff0 arrow = IOff0::from(-1);
    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x01ee>> repeat_1ee;
    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid)
            continue;
        auto sdidi = TRY_UNWRAP(sd->inventory_data[i], continue);
        if (itemdb_isequip2(sdidi))
            continue;
        Packet_Repeat<0x01ee> info;
        info.ioff2 = i.shift();
        info.name_id = sd->status.inventory[i].nameid;
        info.item_type = sdidi->type;
        info.identify = 1;
        info.amount = sd->status.inventory[i].amount;
        if (sdidi->equip == EPOS::ARROW)
        {
            info.epos = EPOS::ARROW;
            if (bool(sd->status.inventory[i].equip))
                arrow = i;
        }
        else
            info.epos = EPOS::ZERO;
        info.card0 = 0;
        info.card1 = 0;
        info.card2 = 0;
        info.card3 = 0;
        repeat_1ee.push_back(info);
    }
    if (!repeat_1ee.empty())
    {
        send_packet_repeatonly<0x01ee, 4, 18>(s, repeat_1ee);
    }
    if (arrow.ok())
        clif_arrowequip(sd, arrow);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_equiplist(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x00a4>> repeat_a4;
    for (IOff0 i : IOff0::iter())
    {
        if (!sd->status.inventory[i].nameid)
            continue;
        P<struct item_data> sdidi = TRY_UNWRAP(sd->inventory_data[i], continue);
        if (!itemdb_isequip2(sdidi))
            continue;
        Packet_Repeat<0x00a4> info;
        info.ioff2 = i.shift();
        info.name_id = sd->status.inventory[i].nameid;
        info.item_type = (
                sdidi->type == ItemType::_7
                ? ItemType::WEAPON
                : sdidi->type);
        info.identify = 0;
        info.epos_pc = pc_equippoint(sd, i);
        info.epos_inv = sd->status.inventory[i].equip;
        info.broken_or_attribute = 0;
        info.refine = 0;
        {
            info.card0 = 0;
            info.card1 = 0;
            info.card2 = 0;
            info.card3 = 0;
        }
        repeat_a4.push_back(info);
    }
    if (!repeat_a4.empty())
    {
        send_packet_repeatonly<0x00a4, 4, 20>(s, repeat_a4);
    }
}

/*==========================================
 * カプラさんに預けてある消耗品&収集品リスト
 *------------------------------------------
 */
int clif_storageitemlist(dumb_ptr<map_session_data> sd, Borrowed<Storage> stor)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x01f0>> repeat_1f0;
    for (SOff0 i : SOff0::iter())
    {
        if (!stor->storage_[i].nameid)
            continue;

        P<struct item_data> id = itemdb_search(stor->storage_[i].nameid);
        if (itemdb_isequip2(id))
            continue;

        Packet_Repeat<0x01f0> info;
        info.soff1 = i.shift();
        info.name_id = stor->storage_[i].nameid;
        info.item_type = id->type;
        info.identify = 0;
        info.amount = stor->storage_[i].amount;
        info.epos_zero = EPOS::ZERO;
        info.card0 = 0;
        info.card1 = 0;
        info.card2 = 0;
        info.card3 = 0;
        repeat_1f0.push_back(info);
    }
    if (!repeat_1f0.empty())
    {
        send_packet_repeatonly<0x01f0, 4, 18>(s, repeat_1f0);
    }
    return 0;
}

/*==========================================
 * カプラさんに預けてある装備リスト
 *------------------------------------------
 */
int clif_storageequiplist(dumb_ptr<map_session_data> sd, Borrowed<Storage> stor)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x00a6>> repeat_a6;
    for (SOff0 i : SOff0::iter())
    {
        if (!stor->storage_[i].nameid)
            continue;

        P<struct item_data> id = itemdb_search(stor->storage_[i].nameid);
        if (!itemdb_isequip2(id))
            continue;
        Packet_Repeat<0x00a6> info;
        info.soff1 = i.shift();
        info.name_id = stor->storage_[i].nameid;
        info.item_type = id->type;
        info.identify = 0;
        info.epos_id = id->equip;
        info.epos_stor = stor->storage_[i].equip;
        info.broken_or_attribute = 0;
        info.refine = 0;
        {
            info.card0 = 0;
            info.card1 = 0;
            info.card2 = 0;
            info.card3 = 0;
        }
        repeat_a6.push_back(info);
    }
    if (!repeat_a6.empty())
    {
        send_packet_repeatonly<0x00a6, 4, 20>(s, repeat_a6);
    }
    return 0;
}

/*==========================================
 * ステータスを送りつける
 * 表示専用数字はこの中で計算して送る
 *------------------------------------------
 */
int clif_updatestatus(dumb_ptr<map_session_data> sd, SP type)
{
    nullpo_retz(sd);

    Session *s = sd->sess;

    {
        Packet_Fixed<0x00b0> fixed_b0;
        fixed_b0.sp_type = type;
        switch (type)
        {
        case SP::WEIGHT:
            pc_checkweighticon(sd);
            fixed_b0.value = sd->weight;
            break;
        case SP::MAXWEIGHT:
            fixed_b0.value = sd->max_weight;
            break;
        case SP::SPEED:
            // 'speed' is actually delay, in milliseconds
            fixed_b0.value = sd->speed.count();
            break;
        case SP::BASELEVEL:
            fixed_b0.value = sd->status.base_level;
            break;
        case SP::JOBLEVEL:
            fixed_b0.value = sd->status.job_level;
            break;
        case SP::STATUSPOINT:
            fixed_b0.value = sd->status.status_point;
            break;
        case SP::SKILLPOINT:
            fixed_b0.value = sd->status.skill_point;
            break;
        case SP::HIT:
            fixed_b0.value = sd->hit;
            break;
        case SP::FLEE1:
            fixed_b0.value = sd->flee;
            break;
        case SP::FLEE2:
            fixed_b0.value = sd->flee2 / 10;
            break;
        case SP::MAXHP:
            fixed_b0.value = sd->status.max_hp;
            break;
        case SP::MAXSP:
            fixed_b0.value = sd->status.max_sp;
            break;
        case SP::HP:
            fixed_b0.value = sd->status.hp;
            break;
        case SP::SP:
            fixed_b0.value = sd->status.sp;
            break;
        case SP::ASPD:
            fixed_b0.value = sd->aspd.count();
            break;
        case SP::ATK1:
            fixed_b0.value = sd->base_atk + sd->watk;
            break;
        case SP::DEF1:
            fixed_b0.value = sd->def;
            break;
        case SP::MDEF1:
            fixed_b0.value = sd->mdef;
            break;
        case SP::ATK2:
            fixed_b0.value = sd->watk2;
            break;
        case SP::DEF2:
            fixed_b0.value = sd->def2;
            break;
        case SP::MDEF2:
            fixed_b0.value = sd->mdef2;
            break;
        case SP::CRITICAL:
            fixed_b0.value = sd->critical / 10;
            break;
        case SP::MATK1:
            fixed_b0.value = sd->matk1;
            break;
        case SP::MATK2:
            fixed_b0.value = sd->matk2;
            break;
        case SP::GM:
            fixed_b0.value = pc_isGM(sd).get_all_bits();
            break;

        default:
            goto not_b0;
        }

        send_fpacket<0x00b0, 8>(s, fixed_b0);
        return 0;
    }
not_b0:

    {
        Packet_Fixed<0x00b1> fixed_b1;
        fixed_b1.sp_type = type;
        switch (type)
        {
        case SP::ZENY:
            trade_verifyzeny(sd);
            if (sd->status.zeny < 0)
                sd->status.zeny = 0;
            fixed_b1.value = sd->status.zeny;
            break;

        case SP::BASEEXP:
            fixed_b1.value = sd->status.base_exp;
            break;
        case SP::JOBEXP:
            fixed_b1.value = sd->status.job_exp;
            break;
        case SP::NEXTBASEEXP:
            fixed_b1.value = pc_nextbaseexp(sd);
            break;
        case SP::NEXTJOBEXP:
            fixed_b1.value = pc_nextjobexp(sd);
            break;

        default:
            goto not_b1;
        }

        send_fpacket<0x00b1, 8>(s, fixed_b1);
        return 0;
    }
not_b1:

    {
        Packet_Fixed<0x00be> fixed_be;
        fixed_be.sp_type = type;
        switch (type)
        {
        case SP::USTR:
        case SP::UAGI:
        case SP::UVIT:
        case SP::UINT:
        case SP::UDEX:
        case SP::ULUK:
            fixed_be.value = pc_need_status_point(sd, usp_to_sp(type));
            break;

        default:
            goto not_be;
        }

        send_fpacket<0x00be, 5>(s, fixed_be);
        return 0;
    }
not_be:

    {
        Packet_Fixed<0x013a> fixed_13a;
        switch (type)
        {
        case SP::ATTACKRANGE:
            fixed_13a.attack_range = (sd->attack_spell_override
                    ? sd->attack_spell_range
                    : sd->attackrange);
            break;

        default:
            goto not_13a;
        }

        send_fpacket<0x013a, 4>(s, fixed_13a);
        return 0;
    }
not_13a:

    {
        Packet_Fixed<0x00141> fixed_141;
        fixed_141.sp_type = type;
        switch (type)
        {
        case SP::STR:
        case SP::AGI:
        case SP::VIT:
        case SP::INT:
        case SP::DEX:
        case SP::LUK:
        {
            ATTR attr = sp_to_attr(type);
            fixed_141.value_status = sd->status.attrs[attr];
            fixed_141.value_b_e = sd->paramb[attr] + sd->parame[attr];
        }
            break;

        default:
            goto not_141;
        }

        send_fpacket<0x0141, 14>(s, fixed_141);
        return 0;
    }

not_141:
    {
        {
            if (battle_config.error_log)
                PRINTF("clif_updatestatus : make %d routine\n"_fmt,
                        type);
            return 1;
        }
    }
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook(dumb_ptr<block_list> bl, LOOK type, int val)
{
    return clif_changelook_towards(bl, type, val, nullptr);
}

int clif_changelook_towards(dumb_ptr<block_list> bl, LOOK type, int val,
                             dumb_ptr<map_session_data> dstsd)
{
    dumb_ptr<map_session_data> sd = nullptr;

    nullpo_retz(bl);

    if (bl->bl_type == BL::PC)
        sd = bl->is_player();

    if (sd && bool(sd->status.option & Opt0::INVISIBILITY))
        return 0;

    if (sd
        && (type == LOOK::WEAPON || type == LOOK::SHIELD || type >= LOOK::SHOES))
    {
        Packet_Fixed<0x01d7> fixed_1d7;
        fixed_1d7.block_id = bl->bl_id;
        if (type >= LOOK::SHOES)
        {
            EQUIP equip_point = equip_points[type];

            fixed_1d7.look_type = type;
            IOff0 idx = sd->equip_index_maybe[equip_point];
            if (idx.ok() && sd->inventory_data[idx].is_some())
            {
                fixed_1d7.weapon_or_name_id_or_value = unwrap<ItemNameId>(sd->status.inventory[idx].nameid);
            }
            else
                fixed_1d7.weapon_or_name_id_or_value = unwrap<ItemNameId>(ItemNameId());
            fixed_1d7.shield = ItemNameId();
        }
        else
        {
            fixed_1d7.look_type = LOOK::WEAPON;
            IOff0 widx = sd->equip_index_maybe[EQUIP::WEAPON];
            IOff0 sidx = sd->equip_index_maybe[EQUIP::SHIELD];
            if (sd->attack_spell_override)
                fixed_1d7.weapon_or_name_id_or_value = unwrap<ItemNameId>(sd->attack_spell_look_override);
            else
            {
                if (widx.ok() && sd->inventory_data[widx].is_some())
                {
                    fixed_1d7.weapon_or_name_id_or_value = unwrap<ItemNameId>(sd->status.inventory[widx].nameid);
                }
                else
                    fixed_1d7.weapon_or_name_id_or_value = unwrap<ItemNameId>(ItemNameId());
            }
            if (sidx.ok() && sidx != widx && sd->inventory_data[sidx].is_some())
            {
                fixed_1d7.shield = sd->status.inventory[sidx].nameid;
            }
            else
                fixed_1d7.shield = ItemNameId();
        }

        Buffer buf = create_fpacket<0x01d7, 11>(fixed_1d7);
        if (dstsd)
            clif_send(buf, dstsd, SendWho::SELF, MIN_CLIENT_VERSION);
        else
            clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    else
    {
        Packet_Fixed<0x01d7> fixed_1d7;
        fixed_1d7.block_id = bl->bl_id;
        fixed_1d7.look_type = type;
        fixed_1d7.weapon_or_name_id_or_value = val;
        fixed_1d7.shield = ItemNameId();

        Buffer buf = create_fpacket<0x01d7, 11>(fixed_1d7);
        if (dstsd)
            clif_send(buf, dstsd, SendWho::SELF, MIN_CLIENT_VERSION);
        else
            clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_initialstatus(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Session *s = sd->sess;

    Packet_Fixed<0x00bd> fixed_bd;
    fixed_bd.status_point = sd->status.status_point;

    fixed_bd.str_attr = saturate<uint8_t>(sd->status.attrs[ATTR::STR]);
    fixed_bd.str_upd = pc_need_status_point(sd, SP::STR);
    fixed_bd.agi_attr = saturate<uint8_t>(sd->status.attrs[ATTR::AGI]);
    fixed_bd.agi_upd = pc_need_status_point(sd, SP::AGI);
    fixed_bd.vit_attr = saturate<uint8_t>(sd->status.attrs[ATTR::VIT]);
    fixed_bd.vit_upd = pc_need_status_point(sd, SP::VIT);
    fixed_bd.int_attr = saturate<uint8_t>(sd->status.attrs[ATTR::INT]);
    fixed_bd.int_upd = pc_need_status_point(sd, SP::INT);
    fixed_bd.dex_attr = saturate<uint8_t>(sd->status.attrs[ATTR::DEX]);
    fixed_bd.dex_upd = pc_need_status_point(sd, SP::DEX);
    fixed_bd.luk_attr = saturate<uint8_t>(sd->status.attrs[ATTR::LUK]);
    fixed_bd.luk_upd = pc_need_status_point(sd, SP::LUK);

    fixed_bd.atk_sum = sd->base_atk + sd->watk;
    fixed_bd.watk2 = sd->watk2;    //atk bonus
    fixed_bd.matk1 = sd->matk1;
    fixed_bd.matk2 = sd->matk2;
    fixed_bd.def = sd->def;
    fixed_bd.def2 = sd->def2;
    fixed_bd.mdef = sd->mdef;
    fixed_bd.mdef2 = sd->mdef2;
    fixed_bd.hit = sd->hit;
    fixed_bd.flee = sd->flee;
    fixed_bd.flee2 = sd->flee2 / 10;
    fixed_bd.critical = sd->critical / 10;
    fixed_bd.karma = sd->status.karma;
    fixed_bd.manner = sd->status.manner;

    send_fpacket<0x00bd, 44>(s, fixed_bd);

    clif_updatestatus(sd, SP::STR);
    clif_updatestatus(sd, SP::AGI);
    clif_updatestatus(sd, SP::VIT);
    clif_updatestatus(sd, SP::INT);
    clif_updatestatus(sd, SP::DEX);
    clif_updatestatus(sd, SP::LUK);

    clif_updatestatus(sd, SP::ATTACKRANGE);
    clif_updatestatus(sd, SP::ASPD);

    return 0;
}

/*==========================================
 *矢装備
 *------------------------------------------
 */
int clif_arrowequip(dumb_ptr<map_session_data> sd, IOff0 val)
{
    nullpo_retz(sd);

    sd->attacktarget = BlockId();

    Session *s = sd->sess;
    Packet_Fixed<0x013c> fixed_13c;
    fixed_13c.ioff2 = val.shift();
    send_fpacket<0x013c, 4>(s, fixed_13c);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_arrow_fail(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_retz(sd);

    Session *s = sd->sess;

    Packet_Fixed<0x013b> fixed_13b;
    fixed_13b.type = type;
    send_fpacket<0x013b, 4>(s, fixed_13b);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack(dumb_ptr<map_session_data> sd, SP type, int ok, int val)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00bc> fixed_bc;
    fixed_bc.sp_type = type;
    fixed_bc.ok = ok;
    fixed_bc.val = val;
    send_fpacket<0x00bc, 6>(s, fixed_bc);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack(dumb_ptr<map_session_data> sd, IOff0 n, EPOS pos, int ok)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00aa> fixed_aa;
    fixed_aa.ioff2 = n.shift();
    fixed_aa.epos = pos;
    fixed_aa.ok = ok;
    send_fpacket<0x00aa, 7>(s, fixed_aa);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack(dumb_ptr<map_session_data> sd, IOff0 n, EPOS pos, int ok)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00ac> fixed_ac;
    fixed_ac.ioff2 = n.shift();
    fixed_ac.epos = pos;
    fixed_ac.ok = ok;
    send_fpacket<0x00ac, 7>(s, fixed_ac);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect(dumb_ptr<block_list> bl, int type)
{
    nullpo_retz(bl);

    Packet_Fixed<0x019b> fixed_19b;
    fixed_19b.block_id = bl->bl_id;
    fixed_19b.type = type;
    Buffer buf = create_fpacket<0x019b, 10>(fixed_19b);

    clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);

    return 0;
}

void clif_map_pvp(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    Packet_Fixed<0x0199> fixed_199;
    fixed_199.status = sd->bl_m->flag.get(MapFlag::PVP)? 1: 0;
    Buffer buf = create_fpacket<0x0199, 4>(fixed_199);

    clif_send(buf, sd, SendWho::SELF, 2);
}

static
void clif_pvpstatus_towards(Buffer& buf, dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    Packet_Fixed<0x019a> fixed_19a;
    fixed_19a.block_id = sd->bl_id;
    fixed_19a.rank = sd->state.pvp_rank;
    fixed_19a.channel = sd->state.pvpchannel;
    buf = create_fpacket<0x019a, 14>(fixed_19a);
}

void clif_pvpstatus(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);
    Buffer buf;
    clif_pvpstatus_towards(buf, sd);
    clif_send(buf, sd, SendWho::AREA, 2);
}

/*==========================================
 * 表示オプション変更
 *------------------------------------------
 */
int clif_changeoption(dumb_ptr<block_list> bl)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(bl);

    Opt0 option = *battle_get_option(bl);
    sc_data = battle_get_sc_data(bl);

    Packet_Fixed<0x0119> fixed_119;
    fixed_119.block_id = bl->bl_id;
    fixed_119.opt1 = *battle_get_opt1(bl);
    fixed_119.opt2 = *battle_get_opt2(bl);
    fixed_119.option = option;
    fixed_119.zero = 0;
    Buffer buf = create_fpacket<0x0119, 13>(fixed_119);

    clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack(dumb_ptr<map_session_data> sd, IOff0 index, int amount,
                     int ok)
{
    nullpo_retz(sd);

    if (!ok)
    {
        Session *s = sd->sess;
        Packet_Fixed<0x00a8> fixed_a8;
        fixed_a8.ioff2 = index.shift();
        fixed_a8.amount = amount;
        fixed_a8.ok = ok;
        send_fpacket<0x00a8, 7>(s, fixed_a8);
    }
    else
    {
        Packet_Fixed<0x01c8> fixed_1c8;
        fixed_1c8.ioff2 = index.shift();
        fixed_1c8.name_id = sd->status.inventory[index].nameid;
        fixed_1c8.block_id = sd->bl_id;
        fixed_1c8.amount = amount;
        fixed_1c8.ok = ok;
        Buffer buf = create_fpacket<0x01c8, 13>(fixed_1c8);
        clif_send(buf, sd, SendWho::SELF, MIN_CLIENT_VERSION);
    }

    return 0;
}

/*==========================================
 * 取り引き要請受け
 *------------------------------------------
 */
void clif_traderequest(dumb_ptr<map_session_data> sd, CharName name)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00e5> fixed_e5;
    fixed_e5.char_name = name;
    send_fpacket<0x00e5, 26>(s, fixed_e5);
}

/*==========================================
 * 取り引き要求応答
 *------------------------------------------
 */
void clif_tradestart(dumb_ptr<map_session_data> sd, int type)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00e7> fixed_e7;
    fixed_e7.type = type;
    send_fpacket<0x00e7, 3>(s, fixed_e7);
}

/*==========================================
 * 相手方からのアイテム追加
 *------------------------------------------
 */
void clif_tradeadditem(dumb_ptr<map_session_data> sd,
        dumb_ptr<map_session_data> tsd, IOff2 index2, int amount)
{
    nullpo_retv(sd);
    nullpo_retv(tsd);

    Session *s = tsd->sess;
    Packet_Fixed<0x00e9> fixed_e9;
    fixed_e9.amount = amount;
    if (!index2.ok())
    {
        fixed_e9.name_id = ItemNameId();
        fixed_e9.identify = 0;
        fixed_e9.broken_or_attribute = 0;
        fixed_e9.refine = 0;
        fixed_e9.card0 = 0;
        fixed_e9.card1 = 0;
        fixed_e9.card2 = 0;
        fixed_e9.card3 = 0;
    }
    else
    {
        IOff0 index0 = index2.unshift();
        fixed_e9.name_id = sd->status.inventory[index0].nameid;
        fixed_e9.identify = 0;
        fixed_e9.broken_or_attribute = 0;
        fixed_e9.refine = 0;
        {
            fixed_e9.card0 = 0;
            fixed_e9.card1 = 0;
            fixed_e9.card2 = 0;
            fixed_e9.card3 = 0;
        }
    }
    send_fpacket<0x00e9, 19>(s, fixed_e9);
}

/*==========================================
 * アイテム追加成功/失敗
 *------------------------------------------
 */
int clif_tradeitemok(dumb_ptr<map_session_data> sd, IOff2 index2, int amount,
                      int fail)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x01b1> fixed_1b1;
    fixed_1b1.ioff2 = index2;
    fixed_1b1.amount = amount;
    fixed_1b1.fail = fail;
    send_fpacket<0x01b1, 7>(s, fixed_1b1);

    return 0;
}

/*==========================================
 * 取り引きok押し
 *------------------------------------------
 */
int clif_tradedeal_lock(dumb_ptr<map_session_data> sd, int fail)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00ec> fixed_ec;
    fixed_ec.fail = fail;      // 0=you 1=the other person
    send_fpacket<0x00ec, 3>(s, fixed_ec);

    return 0;
}

/*==========================================
 * 取り引きがキャンセルされました
 *------------------------------------------
 */
int clif_tradecancelled(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00ee> fixed_ee;
    send_fpacket<0x00ee, 2>(s, fixed_ee);

    return 0;
}

/*==========================================
 * 取り引き完了
 *------------------------------------------
 */
int clif_tradecompleted(dumb_ptr<map_session_data> sd, int fail)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00f0> fixed_f0;
    fixed_f0.fail = fail;
    send_fpacket<0x00f0, 3>(s, fixed_f0);

    return 0;
}

/*==========================================
 * カプラ倉庫のアイテム数を更新
 *------------------------------------------
 */
int clif_updatestorageamount(dumb_ptr<map_session_data> sd,
        Borrowed<Storage> stor)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00f2> fixed_f2;
    fixed_f2.current_slots = stor->storage_amount;  //items
    fixed_f2.max_slots = MAX_STORAGE;   //items max
    send_fpacket<0x00f2, 6>(s, fixed_f2);

    return 0;
}

/*==========================================
 * カプラ倉庫にアイテムを追加する
 *------------------------------------------
 */
int clif_storageitemadded(dumb_ptr<map_session_data> sd, Borrowed<Storage> stor,
        SOff0 index, int amount)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00f4> fixed_f4;
    fixed_f4.soff1 = index.shift();
    fixed_f4.amount = amount;
    fixed_f4.name_id = stor->storage_[index].nameid;
    fixed_f4.identify = 0;
    fixed_f4.broken_or_attribute = 0;
    fixed_f4.refine = 0;
    {
        fixed_f4.card0 = 0;
        fixed_f4.card1 = 0;
        fixed_f4.card2 = 0;
        fixed_f4.card3 = 0;
    }
    send_fpacket<0x00f4, 21>(s, fixed_f4);

    return 0;
}

/*==========================================
 * カプラ倉庫からアイテムを取り去る
 *------------------------------------------
 */
int clif_storageitemremoved(dumb_ptr<map_session_data> sd, SOff0 index,
                             int amount)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00f6> fixed_f6;
    fixed_f6.soff1 = index.shift();
    fixed_f6.amount = amount;
    send_fpacket<0x00f6, 8>(s, fixed_f6);

    return 0;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
int clif_storageclose(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00f8> fixed_f8;
    send_fpacket<0x00f8, 2>(s, fixed_f8);

    return 0;
}

void clif_changelook_accessories(dumb_ptr<block_list> bl,
                             dumb_ptr<map_session_data> dest)
{
    for (LOOK i = LOOK::SHOES; i < LOOK::COUNT; i = LOOK(static_cast<uint8_t>(i) + 1))
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
void clif_getareachar_pc(dumb_ptr<map_session_data> sd,
                          dumb_ptr<map_session_data> dstsd)
{
    if (bool(dstsd->status.option & Opt0::INVISIBILITY))
        return;

    nullpo_retv(sd);
    nullpo_retv(dstsd);

    Buffer buf;
    if (dstsd->walktimer)
    {
        clif_set007b(dstsd, buf);
    }
    else
    {
        clif_set0078_main_1d8(dstsd, buf);
    }
    send_buffer(sd->sess, buf);

    Buffer buff;
    clif_pvpstatus_towards(buff, dstsd);
    clif_send(buff, sd, SendWho::SELF, 2);

    if (battle_config.save_clothcolor == 1 && dstsd->status.clothes_color > 0)
        clif_changelook(dstsd, LOOK::CLOTHES_COLOR,
                         dstsd->status.clothes_color);

    clif_changelook_accessories(sd, dstsd);
    clif_changelook_accessories(dstsd, sd);
}

/*==========================================
 * NPC表示
 *------------------------------------------
 */
static
void clif_getareachar_npc(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd)
{
    nullpo_retv(sd);
    nullpo_retv(nd);

    if (nd->flag & 1 || nd->npc_class == INVISIBLE_CLASS)
        return;

    Buffer buf;
    clif_npc0078(nd, buf);
    send_buffer(sd->sess, buf);

    if(nd->sit == DamageType::SIT)
    {
        Buffer buff;
        clif_sitnpc_sub(buff, nd, nd->sit);
        send_buffer(sd->sess, buff);
    }
}

/*==========================================
 * 移動停止
 *------------------------------------------
 */

static
void clif_movemob_sub(dumb_ptr<block_list> sd_bl, dumb_ptr<mob_data> md)
{
    nullpo_retv(sd_bl);
    nullpo_retv(md);
    dumb_ptr<map_session_data> sd = sd_bl->is_player();
    Buffer buf;

    if (sd->client_version < 3 || sd->client_version >= 4)
        clif_mob007b(md, buf); // backward compatibility for old clients
    else
    {
        Buffer buf2;
        clif_mob0078(md, buf2);
        clif_send(buf2, sd, SendWho::SELF, MIN_CLIENT_VERSION);

        clif_0225_being_move3(md, buf);
    }

    clif_send(buf, sd, SendWho::SELF, MIN_CLIENT_VERSION);
}

int clif_movemob(dumb_ptr<mob_data> md)
{
    nullpo_retz(md);

    map_foreachinarea(std::bind(clif_movemob_sub, ph::_1, md),
            md->bl_m,
            md->bl_x - AREA_SIZE, md->bl_y - AREA_SIZE,
            md->bl_x + AREA_SIZE, md->bl_y + AREA_SIZE,
            BL::PC);

    return 0;
}

/*==========================================
 * モンスターの位置修正
 *------------------------------------------
 */
int clif_fixmobpos(dumb_ptr<mob_data> md)
{
    nullpo_retz(md);

    if (md->state.state == MS::WALK)
    {
        clif_movemob(md);
    }
    else
    {
        Buffer buf;
        clif_mob0078(md, buf);
        clif_send(buf, md, SendWho::AREA, MIN_CLIENT_VERSION);
    }

    return 0;
}

/*==========================================
 * PCの位置修正
 *------------------------------------------
 */
int clif_fixpcpos(dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    if (sd->walktimer)
    {
        Buffer buf;
        clif_set007b(sd, buf);
        clif_send(buf, sd, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    else
    {
        Buffer buf;
        clif_set0078_main_1d8(sd, buf);
        clif_send(buf, sd, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    clif_changelook_accessories(sd, nullptr);

    return 0;
}

/*==========================================
 * 通常攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_damage(dumb_ptr<block_list> src, dumb_ptr<block_list> dst,
        tick_t tick, interval_t sdelay, interval_t ddelay, int damage,
        int div, DamageType type)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(src);
    nullpo_retz(dst);

    sc_data = battle_get_sc_data(dst);

    Packet_Fixed<0x008a> fixed_8a;
    fixed_8a.src_id = src->bl_id;
    fixed_8a.dst_id = dst->bl_id;
    fixed_8a.tick = tick;
    fixed_8a.sdelay = sdelay;
    fixed_8a.ddelay = ddelay;
    fixed_8a.damage = (damage > 0x7fff) ? 0x7fff : damage;
    fixed_8a.div = div;
    fixed_8a.damage_type = type;
    fixed_8a.damage2 = 0;
    Buffer buf = create_fpacket<0x008a, 29>(fixed_8a);
    clif_send(buf, src, SendWho::AREA, MIN_CLIENT_VERSION);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_getareachar_mob(dumb_ptr<map_session_data> sd, dumb_ptr<mob_data> md)
{
    nullpo_retv(sd);
    nullpo_retv(md);

    if (md->state.state == MS::WALK)
    {
        clif_movemob_sub(sd, md);
    }
    else
    {
        Buffer buf;
        clif_mob0078(md, buf);
        send_buffer(sd->sess, buf);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_getareachar_item(dumb_ptr<map_session_data> sd,
                            dumb_ptr<flooritem_data> fitem)
{
    nullpo_retv(sd);
    nullpo_retv(fitem);

    Session *s = sd->sess;
    Packet_Fixed<0x009d> fixed_9d;
    fixed_9d.block_id = fitem->bl_id;
    fixed_9d.name_id = fitem->item_data.nameid;
    fixed_9d.identify = 0;
    fixed_9d.x = fitem->bl_x;
    fixed_9d.y = fitem->bl_y;
    fixed_9d.amount = fitem->item_data.amount;
    fixed_9d.subx = fitem->subx;
    fixed_9d.suby = fitem->suby;
    send_fpacket<0x009d, 17>(s, fixed_9d);
}

/*==========================================
 *
 *------------------------------------------
 */
static
void clif_getareachar(dumb_ptr<block_list> bl, dumb_ptr<map_session_data> sd)
{
    nullpo_retv(bl);

    switch (bl->bl_type)
    {
        case BL::PC:
            if (sd == bl->is_player())
                break;
            clif_getareachar_pc(sd, bl->is_player());
            break;
        case BL::NPC:
            clif_getareachar_npc(sd, bl->is_npc());
            break;
        case BL::MOB:
            clif_getareachar_mob(sd, bl->is_mob());
            break;
        case BL::ITEM:
            clif_getareachar_item(sd, bl->is_item());
            break;
        case BL::SPELL:
            // spell objects are not visible
            // (at least, I *think* that's what this code is for)
            // in any case, this is not a behavior change, just silencing
            // the below warning
            break;
        default:
            if (battle_config.error_log)
                PRINTF("get area char ??? %d\n"_fmt,
                        bl->bl_type);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pcoutsight(dumb_ptr<block_list> bl, dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> dstsd;

    nullpo_retv(bl);
    nullpo_retv(sd);

    switch (bl->bl_type)
    {
        case BL::PC:
            dstsd = bl->is_player();
            if (sd != dstsd)
            {
                clif_clearchar_id(dstsd->bl_id, BeingRemoveWhy::GONE, sd->sess);
                clif_clearchar_id(sd->bl_id, BeingRemoveWhy::GONE, dstsd->sess);
            }
            break;
        case BL::NPC:
            if (bl->is_npc()->npc_class != INVISIBLE_CLASS)
                clif_clearchar_id(bl->bl_id, BeingRemoveWhy::GONE, sd->sess);
            break;
        case BL::MOB:
            clif_clearchar_id(bl->bl_id, BeingRemoveWhy::GONE, sd->sess);
            break;
        case BL::ITEM:
            clif_clearflooritem(bl->is_item(), sd->sess);
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pcinsight(dumb_ptr<block_list> bl, dumb_ptr<map_session_data> sd)
{
    dumb_ptr<map_session_data> dstsd;

    nullpo_retv(bl);
    nullpo_retv(sd);

    switch (bl->bl_type)
    {
        case BL::PC:
            dstsd = bl->is_player();
            if (sd != dstsd)
            {
                clif_getareachar_pc(sd, dstsd);
                clif_getareachar_pc(dstsd, sd);
            }
            break;
        case BL::NPC:
            clif_getareachar_npc(sd, bl->is_npc());
            break;
        case BL::MOB:
            clif_getareachar_mob(sd, bl->is_mob());
            break;
        case BL::ITEM:
            clif_getareachar_item(sd, bl->is_item());
            break;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_moboutsight(dumb_ptr<block_list> bl, dumb_ptr<mob_data> md)
{
    dumb_ptr<map_session_data> sd;

    nullpo_retv(bl);
    nullpo_retv(md);

    if (bl->bl_type == BL::PC)
    {
        sd = bl->is_player();
        clif_clearchar_id(md->bl_id, BeingRemoveWhy::GONE, sd->sess);
    }
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_mobinsight(dumb_ptr<block_list> bl, dumb_ptr<mob_data> md)
{
    dumb_ptr<map_session_data> sd;

    nullpo_retv(bl);
    nullpo_retv(md);

    if (bl->bl_type == BL::PC)
    {
        sd = bl->is_player();
        clif_getareachar_mob(sd, md);
    }
}

/*==========================================
 * スキルリストを送信する
 *------------------------------------------
 */
void clif_skillinfoblock(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    std::vector<Packet_Repeat<0x010f>> repeat_10f;
    for (SkillID i : erange(SkillID(), MAX_SKILL))
    {
        if (sd->status.skill[i].lv)
        {
            Packet_Repeat<0x010f> info;
            // [Fate] Version 1 and later don't crash because of bad skill IDs anymore
            info.info.skill_id = i;
            info.info.type_or_inf = skill_get_inf(i);
            info.info.flags = (
                skill_db[i].poolflags
                | (sd->status.skill[i].flags & SkillFlags::POOL_ACTIVATED));
            info.info.level = sd->status.skill[i].lv;
            info.info.sp = skill_get_sp(i, sd->status.skill[i].lv);
            int range = skill_get_range(i, sd->status.skill[i].lv);
            if (range < 0)
                range = battle_get_range(sd) - (range + 1);
            info.info.range = range;
            info.info.unused = ""_s;
            info.info.can_raise = sd->status.skill[i].lv < skill_get_max_raise(i);

            repeat_10f.push_back(info);
        }
    }
    send_packet_repeatonly<0x010f, 4, 37>(s, repeat_10f);
}

/*==========================================
 * スキル割り振り通知
 *------------------------------------------
 */
int clif_skillup(dumb_ptr<map_session_data> sd, SkillID skill_num)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x010e> fixed_10e;
    fixed_10e.skill_id = skill_num;
    fixed_10e.level = sd->status.skill[skill_num].lv;
    fixed_10e.sp = skill_get_sp(skill_num, sd->status.skill[skill_num].lv);
    int range = skill_get_range(skill_num, sd->status.skill[skill_num].lv);
    if (range < 0)
        range = battle_get_range(sd) - (range + 1);
    fixed_10e.range = range;
    fixed_10e.can_raise = sd->status.skill[skill_num].lv < skill_get_max_raise(skill_num);
    send_fpacket<0x010e, 11>(s, fixed_10e);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel(dumb_ptr<block_list> bl)
{
    // packet 0x01b9 was being sent with length 0,
    // even though there were 6 bytes involved
    // and the client ignores it anyway

    (void)bl;
    return 0;
}

/*==========================================
 * スキル詠唱失敗
 *------------------------------------------
 */
int clif_skill_fail(dumb_ptr<map_session_data> sd, SkillID skill_id, int type,
                     int btype)
{
    nullpo_retz(sd);

    Session *s = sd->sess;

    if (type == 0x4 && battle_config.display_delay_skill_fail == 0)
    {
        return 0;
    }

    Packet_Fixed<0x0110> fixed_110;
    fixed_110.skill_id = skill_id;
    fixed_110.btype = btype;
    fixed_110.zero1 = 0;
    fixed_110.zero2 = 0;
    fixed_110.type = type;
    send_fpacket<0x0110, 10>(s, fixed_110);

    return 0;
}

/*==========================================
 * スキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage(dumb_ptr<block_list> src, dumb_ptr<block_list> dst,
        tick_t tick, interval_t sdelay, interval_t ddelay, int damage,
        int div, SkillID skill_id, int skill_lv, int type)
{
    eptr<struct status_change, StatusChange, StatusChange::MAX_STATUSCHANGE> sc_data;

    nullpo_retz(src);
    nullpo_retz(dst);

    sc_data = battle_get_sc_data(dst);

    Packet_Fixed<0x01de> fixed_1de;
    fixed_1de.skill_id = skill_id;
    fixed_1de.src_id = src->bl_id;
    fixed_1de.dst_id = dst->bl_id;
    fixed_1de.tick = tick;
    fixed_1de.sdelay = sdelay;
    fixed_1de.ddelay = ddelay;
    fixed_1de.damage = damage;
    fixed_1de.skill_level = skill_lv;
    fixed_1de.div = div;
    fixed_1de.type_or_hit = (type > 0) ? type : skill_get_hit(skill_id);
    Buffer buf = create_fpacket<0x01de, 33>(fixed_1de);
    clif_send(buf, src, SendWho::AREA, MIN_CLIENT_VERSION);

    return 0;
}

/*==========================================
 * 状態異常アイコン/メッセージ表示
 *------------------------------------------
 */
int clif_status_change(dumb_ptr<block_list> bl, StatusChange type, int flag)
{
    nullpo_retz(bl);

    Packet_Fixed<0x0196> fixed_196;
    fixed_196.sc_type = type;
    fixed_196.block_id = bl->bl_id;
    fixed_196.flag = flag;
    Buffer buf = create_fpacket<0x0196, 9>(fixed_196);
    clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
    return 0;
}

/*==========================================
 * Send message (modified by [Yor])
 *------------------------------------------
 */
void clif_displaymessage(Session *s, XString mes)
{
    if (mes)
    {
        // don't send a void message (it's not displaying on the client chat). @help can send void line.
        // This is untrue now ^
        send_packet_repeatonly<0x008e, 4, 1>(s, mes);
    }
}

/*==========================================
 * 天の声を送信する
 *------------------------------------------
 */
void clif_GMmessage(dumb_ptr<block_list> bl, XString mes, int flag)
{
    Buffer buf = create_packet_repeatonly<0x009a, 4, 1>(mes);
    flag &= 0x07;
    clif_send(buf, bl,
               (flag == 1) ? SendWho::ALL_SAMEMAP :
               (flag == 2) ? SendWho::AREA :
               (flag == 3) ? SendWho::SELF :
               SendWho::ALL_CLIENT, MIN_CLIENT_VERSION);
}

/*==========================================
 * 復活する
 *------------------------------------------
 */
void clif_resurrection(dumb_ptr<block_list> bl, int type)
{
    nullpo_retv(bl);

    Packet_Fixed<0x0148> fixed_148;
    fixed_148.block_id = bl->bl_id;
    fixed_148.type = type;
    Buffer buf = create_fpacket<0x0148, 8>(fixed_148);

    clif_send(buf, bl,
            type == 1 ? SendWho::AREA : SendWho::AREA_WOS, MIN_CLIENT_VERSION);
}

/*==========================================
 * Wisp/page is transmitted to the destination player
 *------------------------------------------
 */
void clif_wis_message(Session *s, CharName nick, XString mes)   // R 0097 <len>.w <nick>.24B <message>.?B
{
    Packet_Head<0x0097> head_97;
    head_97.char_name = nick;
    send_vpacket<0x0097, 28, 1>(s, head_97, mes);
}

/*==========================================
 * The transmission result of Wisp/page is transmitted to the source player
 *------------------------------------------
 */
void clif_wis_end(Session *s, int flag) // R 0098 <type>.B: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
{
    Packet_Fixed<0x0098> fixed_98;
    fixed_98.flag = flag;
    send_fpacket<0x0098, 3>(s, fixed_98);
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
int clif_party_created(dumb_ptr<map_session_data> sd, int flag)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00fa> fixed_fa;
    fixed_fa.flag = flag;
    send_fpacket<0x00fa, 3>(s, fixed_fa);
    return 0;
}

/*==========================================
 * パーティ情報送信
 *------------------------------------------
 */
int clif_party_info(PartyPair p, Session *s)
{
    int i;
    dumb_ptr<map_session_data> sd = nullptr;

    Packet_Head<0x00fb> head_fb;
    std::vector<Packet_Repeat<0x00fb>> repeat_fb;
    head_fb.party_name = p->name;
    for (i = 0; i < MAX_PARTY; i++)
    {
        PartyMember *m = &p->member[i];
        if (m->account_id)
        {
            Packet_Repeat<0x00fb> info;
            if (sd == nullptr)
                sd = dumb_ptr<map_session_data>(m->sd);

            info.account_id = m->account_id;
            info.char_name = m->name;
            info.map_name = m->map;
            info.leader = (m->leader) ? 0 : 1;
            info.online = (m->online) ? 0 : 1;
            repeat_fb.push_back(info);
        }
    }
    if (s)
    {
        // If set, send only to fd.
        send_vpacket<0x00fb, 28, 46>(s, head_fb, repeat_fb);
        return 9;
    }
    // else, send it to all the party, if they exist.
    if (sd != nullptr)
    {
        Buffer buf = create_vpacket<0x00fb, 28, 46>(head_fb, repeat_fb);
        clif_send(buf, sd, SendWho::PARTY, MIN_CLIENT_VERSION);
    }
    return 0;
}

/*==========================================
 * パーティ勧誘
 * Relay a party invitation.
 *
 * (R 00fe <sender_ID>.l <party_name>.24B)
 *------------------------------------------
 */
void clif_party_invite(dumb_ptr<map_session_data> sd,
                       dumb_ptr<map_session_data> tsd)
{
    nullpo_retv(sd);
    nullpo_retv(tsd);

    Session *s = tsd->sess;

    PartyPair p = TRY_UNWRAP(party_search(sd->status.party_id), return);

    Packet_Fixed<0x00fe> fixed_fe;
    fixed_fe.account_id = sd->status_key.account_id;
    fixed_fe.party_name = p->name;
    send_fpacket<0x00fe, 30>(s, fixed_fe);
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
void clif_party_inviteack(dumb_ptr<map_session_data> sd, CharName nick, int flag)
{
    nullpo_retv(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00fd> fixed_fd;
    fixed_fd.char_name = nick;
    fixed_fd.flag = flag;
    send_fpacket<0x00fd, 27>(s, fixed_fd);
}

/*==========================================
 * パーティ設定送信
 * flag & 0x001=exp変更ミス
 *        0x010=item変更ミス
 *        0x100=一人にのみ送信
 *------------------------------------------
 */
void clif_party_option(PartyPair p, dumb_ptr<map_session_data> sd, int flag)
{
    if (sd == nullptr && flag == 0)
    {
        int i;
        for (i = 0; i < MAX_PARTY; i++)
            if ((sd = map_id2sd(account_to_block(p->member[i].account_id))) != nullptr)
                break;
    }
    if (sd == nullptr)
        return;
    Packet_Fixed<0x0101> fixed_101;
    fixed_101.exp = ((flag & 0x01) ? 2 : p->exp);
    fixed_101.item = ((flag & 0x10) ? 2 : p->item);
    if (flag == 0)
    {
        Buffer buf = create_fpacket<0x0101, 6>(fixed_101);
        clif_send(buf, sd, SendWho::PARTY, MIN_CLIENT_VERSION);
    }
    else
    {
        send_fpacket<0x0101, 6>(sd->sess, fixed_101);
    }
}

/*==========================================
 * パーティ脱退（脱退前に呼ぶこと）
 *------------------------------------------
 */
void clif_party_leaved(PartyPair p, dumb_ptr<map_session_data> sd,
        AccountId account_id, CharName name, int flag)
{
    int i;

    Packet_Fixed<0x0105> fixed_105;
    fixed_105.account_id = account_id;
    fixed_105.char_name = name;
    fixed_105.flag = flag & 0x0f;

    if ((flag & 0xf0) == 0)
    {
        if (sd == nullptr)
            for (i = 0; i < MAX_PARTY; i++)
            {
                sd = dumb_ptr<map_session_data>(p->member[i].sd);
                if (sd != nullptr)
                    break;
            }
        if (sd != nullptr)
        {
            Buffer buf = create_fpacket<0x0105, 31>(fixed_105);
            clif_send(buf, sd, SendWho::PARTY, MIN_CLIENT_VERSION);
        }
    }
    else if (sd != nullptr)
    {
        send_fpacket<0x0105, 31>(sd->sess, fixed_105);
    }
}

/*==========================================
 * パーティメッセージ送信
 *------------------------------------------
 */
void clif_party_message(PartyPair p, AccountId account_id, XString mes)
{
    // always set, but clang is not smart enough
    dumb_ptr<map_session_data> sd = nullptr;
    int i;

    for (i = 0; i < MAX_PARTY; i++)
    {
        sd = dumb_ptr<map_session_data>(p->member[i].sd);
        if (sd != nullptr)
            break;
    }
    if (sd != nullptr)
    {
        Packet_Head<0x0109> head_109;
        head_109.account_id = account_id;
        Buffer buf = create_vpacket<0x0109, 8, 1>(head_109, mes);
        clif_send(buf, sd, SendWho::PARTY, MIN_CLIENT_VERSION);
    }
}

/*==========================================
 * パーティ座標通知
 *------------------------------------------
 */
int clif_party_xy(PartyPair , dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Packet_Fixed<0x0107> fixed_107;
    fixed_107.account_id = sd->status_key.account_id;
    fixed_107.x = sd->bl_x;
    fixed_107.y = sd->bl_y;
    Buffer buf = create_fpacket<0x0107, 10>(fixed_107);
    clif_send(buf, sd, SendWho::PARTY_SAMEMAP_WOS, MIN_CLIENT_VERSION);
    return 0;
}

/*==========================================
 * パーティHP通知
 *------------------------------------------
 */
int clif_party_hp(PartyPair , dumb_ptr<map_session_data> sd)
{
    nullpo_retz(sd);

    Packet_Fixed<0x0106> fixed_106;
    fixed_106.account_id = sd->status_key.account_id;
    fixed_106.hp = (sd->status.hp > 0x7fff) ? 0x7fff : sd->status.hp;
    fixed_106.max_hp =
        (sd->status.max_hp > 0x7fff) ? 0x7fff : sd->status.max_hp;
    Buffer buf = create_fpacket<0x0106, 10>(fixed_106);
    clif_send(buf, sd, SendWho::PARTY_AREA_WOS, MIN_CLIENT_VERSION);
    return 0;
}

/*==========================================
 * 攻撃するために移動が必要
 *------------------------------------------
 */
int clif_movetoattack(dumb_ptr<map_session_data> sd, dumb_ptr<block_list> bl)
{
    nullpo_retz(sd);
    nullpo_retz(bl);

    Session *s = sd->sess;
    Packet_Fixed<0x0139> fixed_139;
    fixed_139.block_id = bl->bl_id;
    fixed_139.bl_x = bl->bl_x;
    fixed_139.bl_y = bl->bl_y;
    fixed_139.sd_x = sd->bl_x;
    fixed_139.sd_y = sd->bl_y;
    fixed_139.range = sd->attackrange;
    send_fpacket<0x0139, 16>(s, fixed_139);
    return 0;
}

/*==========================================
 * エモーション
 *------------------------------------------
 */
void clif_emotion(dumb_ptr<block_list> bl, int type)
{
    nullpo_retv(bl);

    Packet_Fixed<0x00c0> fixed_c0;
    fixed_c0.block_id = bl->bl_id;
    fixed_c0.type = type;
    Buffer buf = create_fpacket<0x00c0, 7>(fixed_c0);
    clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
}

void clif_emotion_towards(dumb_ptr<block_list> bl,
                                  dumb_ptr<block_list> target, int type)
{
    dumb_ptr<map_session_data> sd = target->is_player();

    nullpo_retv(bl);
    nullpo_retv(target);

    if (target->bl_type != BL::PC)
        return;

    Packet_Fixed<0x00c0> fixed_c0;
    fixed_c0.block_id = bl->bl_id;
    fixed_c0.type = type;

    send_fpacket<0x00c0, 7>(sd->sess, fixed_c0);
}

/*==========================================
 * 座る
 *------------------------------------------
 */
void clif_sitting(Session *, dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);

    Packet_Fixed<0x008a> fixed_8a;
    fixed_8a.src_id = sd->bl_id;
    fixed_8a.damage_type = DamageType::SIT;
    Buffer buf = create_fpacket<0x008a, 29>(fixed_8a);
    clif_send(buf, sd, SendWho::AREA, MIN_CLIENT_VERSION);
}

static
void clif_sitnpc_sub(Buffer& buf, dumb_ptr<npc_data> nd, DamageType dmg)
{
    nullpo_retv(nd);

    Packet_Fixed<0x008a> fixed_8a;
    fixed_8a.src_id = nd->bl_id;
    fixed_8a.damage_type = dmg;
    buf = create_fpacket<0x008a, 29>(fixed_8a);
}

void clif_sitnpc_towards(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd, DamageType dmg)
{
    nullpo_retv(nd);
    nullpo_retv(sd);

    if(!sd)
        return;

    Buffer buf;
    clif_sitnpc_sub(buf, nd, dmg);
    clif_send(buf, sd, SendWho::SELF, MIN_CLIENT_VERSION);
}

void clif_sitnpc(dumb_ptr<npc_data> nd, DamageType dmg)
{
    nullpo_retv(nd);

    Buffer buf;
    clif_sitnpc_sub(buf, nd, dmg);
    clif_send(buf, nd, SendWho::AREA, MIN_CLIENT_VERSION);
}

static
void clif_setnpcdirection_sub(Buffer& buf, dumb_ptr<npc_data> nd, DIR direction)
{
    nullpo_retv(nd);
    short dir = 1 | 0;

    switch (direction)
    {
        case DIR::S:  dir = 1 | 0; break; // down
        case DIR::SW: dir = 1 | 2; break;
        case DIR::W:  dir = 0 | 2; break; // left
        case DIR::NW: dir = 4 | 2; break;
        case DIR::N:  dir = 4 | 0; break; // up
        case DIR::NE: dir = 4 | 8; break;
        case DIR::E:  dir = 0 | 8; break; // right
        case DIR::SE: dir = 1 | 8; break;
    }

    Packet_Fixed<0x009c> fixed_9c;
    fixed_9c.block_id = nd->bl_id;
    fixed_9c.client_dir = dir;
    buf = create_fpacket<0x009c, 9>(fixed_9c);
}

void clif_setnpcdirection_towards(dumb_ptr<map_session_data> sd, dumb_ptr<npc_data> nd, DIR direction)
{
    nullpo_retv(nd);
    nullpo_retv(sd);

    if(!sd)
        return;

    Buffer buf;
    clif_setnpcdirection_sub(buf, nd, direction);
    clif_send(buf, sd, SendWho::SELF, MIN_CLIENT_VERSION);
}

void clif_setnpcdirection(dumb_ptr<npc_data> nd, DIR direction)
{
    nullpo_retv(nd);

    Buffer buf;
    clif_setnpcdirection_sub(buf, nd, direction);
    clif_send(buf, nd, SendWho::AREA, MIN_CLIENT_VERSION);
}

/*==========================================
 *
 *------------------------------------------
 */
static
int clif_GM_kickack(dumb_ptr<map_session_data> sd, AccountId id)
{
    nullpo_retz(sd);

    Session *s = sd->sess;
    Packet_Fixed<0x00cd> fixed_cd;
    fixed_cd.account_id = id;
    send_fpacket<0x00cd, 6>(s, fixed_cd);
    return 0;
}

static
void clif_do_quit_game(Session *s, dumb_ptr<map_session_data> sd);

int clif_GM_kick(dumb_ptr<map_session_data> sd, dumb_ptr<map_session_data> tsd,
                  int type)
{
    nullpo_retz(tsd);

    if (type)
        clif_GM_kickack(sd, tsd->status_key.account_id);
    tsd->opt1 = Opt1::ZERO;
    tsd->opt2 = Opt2::ZERO;
    clif_do_quit_game(tsd->sess, tsd);

    return 0;
}

// displaying special effects (npcs, weather, etc) [Valaris]
int clif_specialeffect(dumb_ptr<block_list> bl, int type, int flag)
{
    nullpo_retz(bl);

    Packet_Fixed<0x019b> fixed_19b;
    fixed_19b.block_id = bl->bl_id;
    fixed_19b.type = type;
    Buffer buf = create_fpacket<0x019b, 10>(fixed_19b);

    if (flag == 2)
    {
        for (io::FD i : iter_fds())
        {
            Session *s = get_session(i);
            if (!s)
                continue;
            dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
            if (sd && sd->state.auth && !sd->state.connect_new && sd->bl_m == bl->bl_m)
                clif_specialeffect(sd, type, 1);
        }
    }
    else if (flag == 1)
        clif_send(buf, bl, SendWho::SELF, MIN_CLIENT_VERSION);
    else if (!flag)
        clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);

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
RecvResult clif_parse_WantToConnection(Session *s, dumb_ptr<map_session_data> sd)
{
    AccountId account_id;            // account_id in the packet

    if (sd)
    {
        if (battle_config.error_log)
            PRINTF("clif_parse_WantToConnection : invalid request?\n"_fmt);
        return RecvResult::Error;
    }

    Packet_Fixed<0x0072> fixed;
    RecvResult rv = recv_fpacket<0x0072, 19>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    {
        account_id = fixed.account_id;
    }

    // formerly: account id
    Packet_Payload<0x8000> special;
    special.magic_packet_length = 4;
    send_ppacket<0x8000>(s, special);

    // if same account already connected, we disconnect the 2 sessions
    dumb_ptr<map_session_data> old_sd = map_id2sd(account_to_block(account_id));
    if (old_sd)
    {
        clif_authfail_fd(s, 2);   // same id
        clif_authfail_fd(old_sd->sess, 2);   // same id
        PRINTF("clif_parse_WantToConnection: Double connection for account %d (sessions: #%d (new) and #%d (old)).\n"_fmt,
                account_id, s, old_sd->sess);
    }
    else
    {
        sd.new_();
        s->session_data.reset(sd.operator->());
        sd->sess = s;

        pc_setnewpc(sd, account_id, fixed.char_id, fixed.login_id1,
                fixed.client_tick,
                fixed.sex);

        map_addiddb(sd);

        chrif_authreq(sd);
    }

    return RecvResult::Complete;
}

/*==========================================
 * 007d クライアント側マップ読み込み完了
 * map侵入時に必要なデータを全て送りつける
 *------------------------------------------
 */
static
RecvResult clif_parse_LoadEndAck(Session *s, dumb_ptr<map_session_data> sd)
{
    if (sd->bl_prev != nullptr)
        return RecvResult::Error;

    Packet_Fixed<0x007d> fixed;
    RecvResult rv = recv_fpacket<0x007d, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    // 接続ok時
    //clif_authok();
    if (sd->npc_id)
        npc_event_dequeue(sd);
    clif_skillinfoblock(sd);
    pc_checkitem(sd);
    //guild_info();

    // loadendack時
    // next exp
    clif_updatestatus(sd, SP::NEXTBASEEXP);
    clif_updatestatus(sd, SP::NEXTJOBEXP);
    // skill point
    clif_updatestatus(sd, SP::SKILLPOINT);
    // item
    clif_itemlist(sd);
    clif_equiplist(sd);
    // param all
    clif_initialstatus(sd);
    // party
    party_send_movemap(sd);
    // 119
    // 78

    if (battle_config.player_invincible_time > interval_t::zero())
    {
        pc_setinvincibletimer(sd, battle_config.player_invincible_time);
    }

    map_addblock(sd);     // ブロック登録
    clif_spawnpc(sd);          // spawn

    clif_map_pvp(sd); // send map pvp status

    // weight max , now
    clif_updatestatus(sd, SP::MAXWEIGHT);
    clif_updatestatus(sd, SP::WEIGHT);

    // pvp
    if (!battle_config.pk_mode)
        sd->pvp_timer.cancel();

    if (sd->bl_m->flag.get(MapFlag::PVP))
    {
        if (!battle_config.pk_mode)
        {
            // remove pvp stuff for pk_mode [Valaris]
            sd->pvp_timer = Timer(gettick() + 200_ms,
                    std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2,
                        sd->bl_id));
            sd->pvp_rank = 0;
            sd->pvp_point = 5;
        }
    }
    else
    {
        // sd->pvp_timer = nullptr;
    }

    sd->state.connect_new = 0;

    // view equipment item
    clif_changelook(sd, LOOK::WEAPON, static_cast<uint16_t>(ItemLook::NONE));
    if (battle_config.save_clothcolor == 1 && sd->status.clothes_color > 0)
        clif_changelook(sd, LOOK::CLOTHES_COLOR,
                         sd->status.clothes_color);

    // option
    clif_changeoption(sd);
    // broken equipment

//        clif_changelook_accessories(sd, nullptr);

    map_foreachinarea(std::bind(clif_getareachar, ph::_1, sd),
            sd->bl_m,
            sd->bl_x - AREA_SIZE, sd->bl_y - AREA_SIZE,
            sd->bl_x + AREA_SIZE, sd->bl_y + AREA_SIZE,
            BL::NUL);

    if (!sd->state.seen_motd)
        pc_show_motd(sd);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_TickSend(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x007e> fixed;
    RecvResult rv = recv_fpacket<0x007e, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    uint32_t client_tick = fixed.client_tick;
    (void)client_tick;
    clif_servertick(sd);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_WalkToXY(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0085> fixed;
    RecvResult rv = recv_fpacket<0x0085, 5>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }

    if (sd->npc_id || sd->state.storage_open)
        return rv;

    if (sd->canmove_tick > gettick())
        return rv;

    // ステータス異常やハイディング中(トンネルドライブ無)で動けない
    if (bool(sd->opt1) && sd->opt1 != (Opt1::_stone6))
        return rv;

    if (sd->invincible_timer)
        pc_delinvincibletimer(sd);

    pc_stopattack(sd);

    int x = fixed.pos.x;
    int y = fixed.pos.y;
    pc_walktoxy(sd, x, y);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_QuitGame(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x018a> fixed;
    RecvResult rv = recv_fpacket<0x018a, 4>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    clif_do_quit_game(s, sd);
    return rv;
}

void clif_do_quit_game(Session *s, dumb_ptr<map_session_data> sd)
{

    tick_t tick = gettick();

    Packet_Fixed<0x018b> fixed_18b;
    if ((!pc_isdead(sd) && (sd->opt1 != Opt1::ZERO || sd->opt2 != Opt2::ZERO))
        || (tick < sd->canact_tick))
    {
        fixed_18b.okay = 1;
        send_fpacket<0x018b, 4>(s, fixed_18b);
        return;
    }

    /*  Rovert's prevent logout option fixed [Valaris]  */
    if (!battle_config.prevent_logout
        || tick >= sd->canlog_tick + 10_s)
    {
        clif_setwaitclose(s);
        fixed_18b.okay = 0;
    }
    else
    {
        fixed_18b.okay = 1;
    }
    send_fpacket<0x018b, 4>(s, fixed_18b);
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_GetCharNameRequest(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0094> fixed;
    RecvResult rv = recv_fpacket<0x0094, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    dumb_ptr<block_list> bl;
    BlockId account_id;

    account_id = fixed.block_id;
    bl = map_id2bl(account_id);
    if (bl == nullptr)
        return rv;

    Packet_Fixed<0x0095> fixed_95;
    fixed_95.block_id = account_id;

    switch (bl->bl_type)
    {
        case BL::PC:
        {
            dumb_ptr<map_session_data> ssd = bl->is_player();

            nullpo_retr(rv, ssd);

            if (ssd->state.shroud_active)
                fixed_95.char_name = CharName();
            else
                fixed_95.char_name = ssd->status_key.name;
            send_fpacket<0x0095, 30>(s, fixed_95);

            PartyName party_name;

            int send = 0;

            if (ssd->status.party_id)
            {
                Option<PartyPair> p_ = party_search(ssd->status.party_id);

                OMATCH_BEGIN_SOME (p, p_)
                {
                    party_name = p->name;
                    send = 1;
                }
                OMATCH_END ();
            }

            if (send)
            {
                Packet_Fixed<0x0195> fixed_195;
                fixed_195.block_id = account_id;
                fixed_195.party_name = party_name;
                fixed_195.guild_name = ""_s;
                fixed_195.guild_pos = ""_s;
                fixed_195.guild_pos = ""_s; // We send this value twice because the client expects it
                send_fpacket<0x0195, 102>(s, fixed_195);
            }

            if (pc_isGM(sd).satisfies(battle_config.hack_info_GM_level))
            {
                IP4Address ip = ssd->get_ip();
                Packet_Fixed<0x020c> fixed_20c;

                // Mask the IP using the char-server password
                if (battle_config.mask_ip_gms)
                    ip = MD5_ip(ip);

                fixed_20c.block_id = account_id;
                fixed_20c.ip = ip;
                send_fpacket<0x020c, 10>(s, fixed_20c);
             }
        }
            break;
        case BL::NPC:
        {
            NpcName name = bl->is_npc()->name;
            // [fate] elim hashed out/invisible names for the client
            auto it = std::find(name.begin(), name.end(), '#');
            fixed_95.char_name = stringish<CharName>(name.xislice_h(it));
            send_fpacket<0x0095, 30>(s, fixed_95);
        }
            break;
        case BL::MOB:
        {
            dumb_ptr<mob_data> md = bl->is_mob();

            nullpo_retr(rv, md);

            fixed_95.char_name = stringish<CharName>(md->name);
            send_fpacket<0x0095, 30>(s, fixed_95);
        }
            break;
        // case BL::SPELL
        default:
            if (battle_config.error_log)
                PRINTF("clif_parse_GetCharNameRequest : bad type %d (%d)\n"_fmt,
                        bl->bl_type, account_id);
            break;
    }

    return rv;
}

/*==========================================
 * Validate and process transmission of a
 * global/public message.
 *
 * (S 008c <len>.w <message>.?B)
 *------------------------------------------
 */
static
RecvResult clif_parse_GlobalMessage(Session *s, dumb_ptr<map_session_data> sd)
{
    AString repeat;
    RecvResult rv = recv_packet_repeatonly<0x008c, 4, 1>(s, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    AString mbuf = clif_validate_chat(sd, ChatType::Global, repeat);
    if (!mbuf)
    {
        clif_displaymessage(s, "Your message could not be sent."_s);
        return rv;
    }

    if (is_atcommand(s, sd, mbuf, GmLevel()))
        return rv;

    if (!magic::magic_message(sd, mbuf))
    {
        /* Don't send chat that results in an automatic ban. */
        if (tmw_CheckChatSpam(sd, mbuf))
        {
            clif_displaymessage(s, "Your message could not be sent."_s);
            return rv;
        }

        /* It's not a spell/magic message, so send the message to others. */
        Packet_Head<0x008d> head_8d;
        head_8d.block_id = sd->bl_id;
        XString repeat_8d = mbuf;
        Buffer sendbuf = create_vpacket<0x008d, 8, 1>(head_8d, repeat_8d);

        clif_send(sendbuf, sd, SendWho::AREA_CHAT_WOC, MIN_CLIENT_VERSION);
    }

    /* Send the message back to the speaker. */
    send_packet_repeatonly<0x008e, 4, 1>(s, repeat);

    return rv;
}

static
void clif_message_sub(Buffer& buf, dumb_ptr<block_list> bl, XString msg)
{
    size_t msg_len = msg.size() + 1;
    if (msg_len + 16 > 512)
        return;

    Packet_Head<0x008d> head_8d;
    head_8d.block_id = bl->bl_id;
    buf = create_vpacket<0x008d, 8, 1>(head_8d, msg);
}

void clif_npc_send_title(Session *s, BlockId npcid, XString msg)
{
    size_t msg_len = msg.size() + 1;
    if (msg_len > 50)
        return;

    Packet_Head<0x0228> head_228;
    head_228.npc_id = npcid;
    head_228.string_length = msg_len;
    Buffer buf = create_vpacket<0x0228, 10, 1>(head_228, msg);

    send_buffer(s, buf);
}

void clif_change_music(dumb_ptr<map_session_data> sd, XString music)
{
    nullpo_retv(sd);
    if(sd->client_version < 2)
        return;

    size_t msg_len = music.size();
    if (msg_len > 128)
        return;

    Packet_Head<0x0227> head_227;
    Buffer buf = create_vpacket<0x0227, 4, 1>(head_227, music);

    send_buffer(sd->sess, buf);
}

void clif_message_towards(dumb_ptr<map_session_data> sd, dumb_ptr<block_list> bl, XString msg)
{
    nullpo_retv(bl);
    nullpo_retv(sd);

    if(!sd)
        return;

    Buffer buf;
    clif_message_sub(buf, bl, msg);
    clif_send(buf, sd, SendWho::SELF, MIN_CLIENT_VERSION);
}

void clif_message(dumb_ptr<block_list> bl, XString msg)
{
    nullpo_retv(bl);

    Buffer buf;
    clif_message_sub(buf, bl, msg);
    clif_send(buf, bl, SendWho::AREA, MIN_CLIENT_VERSION);
}

void clif_send_mask(dumb_ptr<map_session_data> sd, int map_mask)
{
    nullpo_retv(sd);
    if(sd->client_version < 2)
        return;

    Packet_Fixed<0x0226> fixed_226;
    fixed_226.mask = map_mask;

    Buffer buf = create_fpacket<0x0226, 10>(fixed_226);
    send_buffer(sd->sess, buf);
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_ChangeDir(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x009b> fixed;
    RecvResult rv = recv_fpacket<0x009b, 5>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    // RFIFOW(fd,2) and WBUFW(buf,6) are always 0
    // TODO perhaps we could use that to remove this hack?
    DIR dir;
    uint8_t client_dir = fixed.client_dir;
    // the client uses a diffenent direction enum ... ugh
    switch (client_dir)
    {
    case 1 | 0: dir = DIR::S; break; // down
    case 1 | 2: dir = DIR::SW; break;
    case 0 | 2: dir = DIR::W; break; // left
    case 4 | 2: dir = DIR::NW; break;
    case 4 | 0: dir = DIR::N; break; // up
    case 4 | 8: dir = DIR::NE; break;
    case 0 | 8: dir = DIR::E; break; // right
    case 1 | 8: dir = DIR::SE; break;
    default:
        return rv;
    }

    if (dir == sd->dir)
        return rv;

    pc_setdir(sd, dir);

    Packet_Fixed<0x009c> fixed_9c;
    fixed_9c.block_id = sd->bl_id;
    fixed_9c.client_dir = client_dir;
    Buffer buf = create_fpacket<0x009c, 9>(fixed_9c);

    clif_send(buf, sd, SendWho::AREA_WOS, MIN_CLIENT_VERSION);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_Emotion(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00bf> fixed;
    RecvResult rv = recv_fpacket<0x00bf, 3>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, SkillID::NV_EMOTE) >= 1)
    {
        uint8_t emote = fixed.emote;
        Packet_Fixed<0x00c0> fixed_c0;
        fixed_c0.block_id = sd->bl_id;
        fixed_c0.type = emote;
        Buffer buf = create_fpacket<0x00c0, 7>(fixed_c0);
        clif_send(buf, sd, SendWho::AREA, MIN_CLIENT_VERSION);
    }
    else
        clif_skill_fail(sd, SkillID::ONE, 0, 1);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_ActionRequest(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0089> fixed;
    RecvResult rv = recv_fpacket<0x0089, 7>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    DamageType action_type;
    BlockId target_id;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }
    if (sd->npc_id
        || bool(sd->opt1)
        || sd->state.storage_open)
        return rv;

    tick_t tick = gettick();

    pc_stop_walking(sd, 0);
    pc_stopattack(sd);

    target_id = fixed.target_id;
    action_type = fixed.action;

    switch (action_type)
    {
        case DamageType::NORMAL:
        case DamageType::CONTINUOUS:
            if (bool(sd->status.option & Opt0::HIDE))
                return rv;
            if (!battle_config.skill_delay_attack_enable)
            {
                if (tick < sd->canact_tick)
                {
                    clif_skill_fail(sd, SkillID::ONE, 4, 0);
                    return rv;
                }
            }
            if (sd->invincible_timer)
                pc_delinvincibletimer(sd);
            sd->attacktarget = BlockId();
            pc_attack(sd, target_id, action_type != DamageType::NORMAL);
            break;
        case DamageType::SIT:
            pc_stop_walking(sd, 1);
            pc_setsit(sd);
            clif_sitting(s, sd);
            break;
        case DamageType::STAND:
            pc_setstand(sd);
            Packet_Fixed<0x008a> fixed_8a;
            fixed_8a.src_id = sd->bl_id;
            fixed_8a.damage_type = DamageType::STAND;
            Buffer buf = create_fpacket<0x008a, 29>(fixed_8a);
            clif_send(buf, sd, SendWho::AREA, MIN_CLIENT_VERSION);
            break;
    }

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_Restart(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00b2> fixed;
    RecvResult rv = recv_fpacket<0x00b2, 3>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    switch (fixed.flag)
    {
        case 0x00:
            if (pc_isdead(sd))
            {
                pc_setstand(sd);
                pc_setrestartvalue(sd, 3);
                skill_status_change_clear(sd, 0);
                if (sd->bl_m->flag.get(MapFlag::RESAVE))
                {
                    pc_setpos(sd, sd->bl_m->resave.map_,
                            sd->bl_m->resave.x, sd->bl_m->resave.y,
                            BeingRemoveWhy::QUIT);
                }
                else
                {
                    pc_setpos(sd, sd->status.save_point.map_,
                            sd->status.save_point.x, sd->status.save_point.y,
                            BeingRemoveWhy::QUIT);
                }
            }
            break;
        case 0x01:
            /*  Rovert's Prevent logout option - Fixed [Valaris]    */
            if (!battle_config.prevent_logout
                || gettick() >= sd->canlog_tick + 10_s)
            {
                chrif_charselectreq(sd);
            }
            else
            {
                Packet_Fixed<0x018b> fixed_18b;
                fixed_18b.okay = 1;

                send_fpacket<0x018b, 4>(s, fixed_18b);
            }
            break;
    }

    return rv;
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
RecvResult clif_parse_Wis(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Head<0x0096> head;
    AString repeat;
    RecvResult rv = recv_vpacket<0x0096, 28, 1>(s, head, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    dumb_ptr<map_session_data> dstsd = nullptr;

    AString mbuf = clif_validate_chat(sd, ChatType::Whisper, repeat);
    if (!mbuf)
    {
        clif_displaymessage(s, "Your message could not be sent."_s);
        return rv;
    }

    if (is_atcommand(s, sd, mbuf, GmLevel()))
    {
        return rv;
    }

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam(sd, mbuf))
    {
        clif_displaymessage(s, "Your message could not be sent."_s);
        return rv;
    }

    /*
     * The player is not on this server. Only send the whisper if the name is
     * exactly the same, because if there are multiple map-servers and a name
     * conflict (for instance, "Test" versus "test"), the char-server must
     * settle the discrepancy.
     */
    CharName tname = head.target_name;
    if (!(dstsd = map_nick2sd(tname))
            || dstsd->status_key.name != tname)
        intif_wis_message(sd, tname, mbuf);
    else
    {
        /* Refuse messages addressed to self. */
        if (dstsd->sess == s)
        {
            ZString mes = "You cannot page yourself."_s;
            clif_wis_message(s, WISP_SERVER_NAME, mes);
        }
        else
        {
            {
                /* The player is not being ignored. */
                {
                    clif_wis_message(dstsd->sess, sd->status_key.name, mbuf);
                    /* The whisper was sent successfully. */
                    clif_wis_end(s, 0);
                }
            }
        }
    }

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_TakeItem(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x009f> fixed;
    RecvResult rv = recv_fpacket<0x009f, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    dumb_ptr<flooritem_data> fitem;

    BlockId map_object_id = fixed.object_id;
    fitem = map_id_is_item(map_object_id);

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }

    if (sd->npc_id
        || sd->opt1 != Opt1::ZERO)   //会話禁止
        return rv;

    if (fitem == nullptr || fitem->bl_m != sd->bl_m)
        return rv;

    if (abs(sd->bl_x - fitem->bl_x) >= 2
        || abs(sd->bl_y - fitem->bl_y) >= 2)
        return rv;                 // too far away to pick up

    if (sd->state.shroud_active && sd->state.shroud_disappears_on_pickup)
        magic::magic_unshroud(sd);

    pc_takeitem(sd, fitem);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_DropItem(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00a2> fixed;
    RecvResult rv = recv_fpacket<0x00a2, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }
    if (sd->bl_m->flag.get(MapFlag::NO_PLAYER_DROPS))
    {
        clif_displaymessage(sd->sess, "Can't drop items here."_s);
        return rv;
    }
    if (sd->npc_id
        || sd->opt1 != Opt1::ZERO)
    {
        clif_displaymessage(sd->sess, "Can't drop items right now."_s);
        return rv;
    }

    if (!fixed.ioff2.ok())
        return RecvResult::Error;
    IOff0 item_index = fixed.ioff2.unshift();
    int item_amount = fixed.amount;

    pc_dropitem(sd, item_index, item_amount);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_UseItem(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00a7> fixed;
    RecvResult rv = recv_fpacket<0x00a7, 8>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }
    if (sd->npc_id
        || sd->opt1 != Opt1::ZERO)   //会話禁止
        return rv;

    if (sd->invincible_timer)
        pc_delinvincibletimer(sd);

    if (!fixed.ioff2.ok())
        return RecvResult::Error;
    pc_useitem(sd, fixed.ioff2.unshift());

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_EquipItem(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00a9> fixed;
    RecvResult rv = recv_fpacket<0x00a9, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }
    if (!fixed.ioff2.ok())
        return RecvResult::Error;
    IOff0 index = fixed.ioff2.unshift();
    if (sd->npc_id)
        return rv;

    OMATCH_BEGIN_SOME (sdidi, sd->inventory_data[index])
    {
        EPOS epos = fixed.epos_ignored;
        if (sdidi->type == ItemType::ARROW)
            epos = EPOS::ARROW;

        // Note: the EPOS argument to pc_equipitem is actually ignored
        pc_equipitem(sd, index, epos);
    }
    OMATCH_END ();

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_UnequipItem(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00ab> fixed;
    RecvResult rv = recv_fpacket<0x00ab, 4>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }
    if (!fixed.ioff2.ok())
        return RecvResult::Error;
    IOff0 index = fixed.ioff2.unshift();

    if (sd->npc_id
        || sd->opt1 != Opt1::ZERO)
        return rv;
    pc_unequipitem(sd, index, CalcStatus::NOW);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcClicked(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0090> fixed;
    RecvResult rv = recv_fpacket<0x0090, 7>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (pc_isdead(sd))
    {
        clif_clearchar(sd, BeingRemoveWhy::DEAD);
        return rv;
    }
    if (sd->npc_id)
        return rv;
    npc_click(sd, fixed.block_id);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcBuySellSelected(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00c5> fixed;
    RecvResult rv = recv_fpacket<0x00c5, 7>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    npc_buysellsel(sd, fixed.block_id, fixed.type);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcBuyListSend(Session *s, dumb_ptr<map_session_data> sd)
{
    std::vector<Packet_Repeat<0x00c8>> repeat;
    RecvResult rv = recv_packet_repeatonly<0x00c8, 4, 4>(s, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    int fail = npc_buylist(sd, repeat);

    Packet_Fixed<0x00ca> fixed_ca;
    fixed_ca.fail = fail;
    send_fpacket<0x00ca, 3>(s, fixed_ca);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcSellListSend(Session *s, dumb_ptr<map_session_data> sd)
{
    std::vector<Packet_Repeat<0x00c9>> repeat;
    RecvResult rv = recv_packet_repeatonly<0x00c9, 4, 4>(s, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    int fail = npc_selllist(sd, repeat);

    Packet_Fixed<0x00cb> fixed_cb;
    fixed_cb.fail = fail;
    send_fpacket<0x00cb, 3>(s, fixed_cb);

    return rv;
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
static
RecvResult clif_parse_TradeRequest(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00e4> fixed;
    RecvResult rv = recv_fpacket<0x00e4, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, SkillID::NV_TRADE) >= 1)
    {
        trade_traderequest(sd, fixed.block_id);
    }
    else
        clif_skill_fail(sd, SkillID::ONE, 0, 0);

    return rv;
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
static
RecvResult clif_parse_TradeAck(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00e6> fixed;
    RecvResult rv = recv_fpacket<0x00e6, 3>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    trade_tradeack(sd, fixed.type);

    return rv;
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
static
RecvResult clif_parse_TradeAddItem(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00e8> fixed;
    RecvResult rv = recv_fpacket<0x00e8, 8>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (fixed.zeny_or_ioff2.index != 0 && !fixed.zeny_or_ioff2.ok())
        return RecvResult::Error;
    trade_tradeadditem(sd, fixed.zeny_or_ioff2, fixed.amount);

    return rv;
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
static
RecvResult clif_parse_TradeOk(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00eb> fixed;
    RecvResult rv = recv_fpacket<0x00eb, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    trade_tradeok(sd);

    return rv;
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
static
RecvResult clif_parse_TradeCansel(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00ed> fixed;
    RecvResult rv = recv_fpacket<0x00ed, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    trade_tradecancel(sd);

    return rv;
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
static
RecvResult clif_parse_TradeCommit(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00ef> fixed;
    RecvResult rv = recv_fpacket<0x00ef, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    trade_tradecommit(sd);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_StopAttack(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0118> fixed;
    RecvResult rv = recv_fpacket<0x0118, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    pc_stopattack(sd);

    return rv;
}

/*==========================================
 * ステータスアップ
 *------------------------------------------
 */
static
RecvResult clif_parse_StatusUp(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00bb> fixed;
    RecvResult rv = recv_fpacket<0x00bb, 5>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    pc_statusup(sd, fixed.asp);

    return rv;
}

/*==========================================
 * スキルレベルアップ
 *------------------------------------------
 */
static
RecvResult clif_parse_SkillUp(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0112> fixed;
    RecvResult rv = recv_fpacket<0x0112, 4>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    pc_skillup(sd, fixed.skill_id);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcSelectMenu(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00b8> fixed;
    RecvResult rv = recv_fpacket<0x00b8, 7>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    sd->npc_menu = fixed.menu_entry;
    map_scriptcont(sd, fixed.npc_id);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcNextClicked(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00b9> fixed;
    RecvResult rv = recv_fpacket<0x00b9, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    map_scriptcont(sd, fixed.npc_id);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcAmountInput(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0143> fixed;
    RecvResult rv = recv_fpacket<0x0143, 10>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    sd->npc_amount = fixed.input_int_value;
    map_scriptcont(sd, fixed.block_id);

    return rv;
}

/*==========================================
 * Process string-based input for an NPC.
 *
 * (S 01d5 <len>.w <npc_ID>.l <message>.?B)
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcStringInput(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Head<0x01d5> head;
    AString repeat;
    RecvResult rv = recv_vpacket<0x01d5, 8, 1>(s, head, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    sd->npc_str = repeat;

    map_scriptcont(sd, head.block_id);

    return rv;
}

/*==========================================
 *
 *------------------------------------------
 */
static
RecvResult clif_parse_NpcCloseClicked(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0146> fixed;
    RecvResult rv = recv_fpacket<0x0146, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    map_scriptcont(sd, fixed.block_id);

    return rv;
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
static
RecvResult clif_parse_MoveToKafra(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00f3> fixed;
    RecvResult rv = recv_fpacket<0x00f3, 8>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (!fixed.ioff2.ok())
        return RecvResult::Error;
    IOff0 item_index = fixed.ioff2.unshift();
    int item_amount = fixed.amount;

    if ((sd->npc_id && !sd->npc_flags.storage) || sd->trade_partner
        || !sd->state.storage_open)
        return rv;

    if (sd->state.storage_open)
        storage_storageadd(sd, item_index, item_amount);

    return rv;
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
static
RecvResult clif_parse_MoveFromKafra(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00f5> fixed;
    RecvResult rv = recv_fpacket<0x00f5, 8>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (!fixed.soff1.ok())
        return RecvResult::Error;
    SOff0 item_index = fixed.soff1.unshift();
    int item_amount = fixed.amount;

    if ((sd->npc_id && !sd->npc_flags.storage) || sd->trade_partner
        || !sd->state.storage_open)
        return rv;

    if (sd->state.storage_open)
        storage_storageget(sd, item_index, item_amount);

    return rv;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
static
RecvResult clif_parse_CloseKafra(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00f7> fixed;
    RecvResult rv = recv_fpacket<0x00f7, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (sd->state.storage_open)
        storage_storageclose(sd);

    return rv;
}

/*==========================================
 * パーティを作る
 * Process request to create a party.
 *
 * (S 00f9 <party_name>.24B)
 *------------------------------------------
 */
static
RecvResult clif_parse_CreateParty(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00f9> fixed;
    RecvResult rv = recv_fpacket<0x00f9, 26>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, SkillID::NV_PARTY) >= 2)
    {
        PartyName name = fixed.party_name;
        party_create(sd, name);
    }
    else
        clif_skill_fail(sd, SkillID::ONE, 0, 4);

    return rv;
}

/*==========================================
 * パーティに勧誘
 * Process invitation to join a party.
 *
 * (S 00fc <account_ID>.l)
 *------------------------------------------
 */
static
RecvResult clif_parse_PartyInvite(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00fc> fixed;
    RecvResult rv = recv_fpacket<0x00fc, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    party_invite(sd, fixed.account_id);

    return rv;
}

/*==========================================
 * パーティ勧誘返答
 * Process reply to party invitation.
 *
 * (S 00ff <account_ID>.l <flag>.l)
 *------------------------------------------
 */
static
RecvResult clif_parse_ReplyPartyInvite(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x00ff> fixed;
    RecvResult rv = recv_fpacket<0x00ff, 10>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    if (battle_config.basic_skill_check == 0
        || pc_checkskill(sd, SkillID::NV_PARTY) >= 1)
    {
        party_reply_invite(sd, fixed.account_id, fixed.flag);
    }
    else
    {
        party_reply_invite(sd, fixed.account_id, 0);
        clif_skill_fail(sd, SkillID::ONE, 0, 4);
    }

    return rv;
}

/*==========================================
 * パーティ脱退要求
 *------------------------------------------
 */
static
RecvResult clif_parse_LeaveParty(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0100> fixed;
    RecvResult rv = recv_fpacket<0x0100, 2>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    party_leave(sd);

    return rv;
}

/*==========================================
 * パーティ除名要求
 *------------------------------------------
 */
static
RecvResult clif_parse_RemovePartyMember(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0103> fixed;
    RecvResult rv = recv_fpacket<0x0103, 30>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    AccountId account_id = fixed.account_id;
    // unused fixed.unusedchar_name;
    party_removemember(sd, account_id);

    return rv;
}

/*==========================================
 * パーティ設定変更要求
 *------------------------------------------
 */
static
RecvResult clif_parse_PartyChangeOption(Session *s, dumb_ptr<map_session_data> sd)
{
    Packet_Fixed<0x0102> fixed;
    RecvResult rv = recv_fpacket<0x0102, 6>(s, fixed);
    if (rv != RecvResult::Complete)
        return rv;

    party_changeoption(sd, fixed.exp, fixed.item);

    return rv;
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
RecvResult clif_parse_PartyMessage(Session *s, dumb_ptr<map_session_data> sd)
{
    AString repeat;
    RecvResult rv = recv_packet_repeatonly<0x0108, 4, 1>(s, repeat);
    if (rv != RecvResult::Complete)
        return rv;

    AString mbuf = clif_validate_chat(sd, ChatType::Party, repeat);
    if (!mbuf)
    {
        clif_displaymessage(s, "Your message could not be sent."_s);
        return rv;
    }

    if (is_atcommand(s, sd, mbuf, GmLevel()))
        return rv;

    /* Don't send chat that results in an automatic ban. */
    if (tmw_CheckChatSpam(sd, mbuf))
    {
        clif_displaymessage(s, "Your message could not be sent."_s);
        return rv;
    }

    party_send_message(sd, mbuf);

    return rv;
}

void clif_sendallquest(dumb_ptr<map_session_data> sd)
{
    int i;
    QuestId questid;
    if (!sd)
        return;

    if (!sd->sess)
        return;

    if(sd->client_version < 2) // require 1.5.5.9 or above
        return;

    Session *s = sd->sess;
    Packet_Head<0x0215> head_215;
    std::vector<Packet_Repeat<0x0215>> repeat_215;

    assert (sd->status.global_reg_num < GLOBAL_REG_NUM);
    for (QuestId q = wrap<QuestId>(0); q < wrap<QuestId>(-1); q = next(q))
    {
        P<struct quest_data> quest_data_ = TRY_UNWRAP(questdb_exists(q), continue);
        for (i = 0; i < sd->status.global_reg_num; i++)
        {
            if (sd->status.global_reg[i].str == quest_data_->quest_vr)
            {
                int val = ((sd->status.global_reg[i].value & (((1 << quest_data_->quest_mask) - 1) << (quest_data_->quest_shift * quest_data_->quest_mask))) >> (quest_data_->quest_shift * quest_data_->quest_mask));
                Packet_Repeat<0x0215> info;
                info.variable = unwrap<QuestId>(quest_data_->questid);
                info.value = val;
                repeat_215.push_back(info);
                break;
            }
        }
    }

    send_vpacket<0x0215, 4, 6>(s, head_215, repeat_215);
    return;
}

void clif_sendquest(dumb_ptr<map_session_data> sd, QuestId questid, int value)
{
    if (!sd)
        return;

    if (!sd->sess)
        return;

    if(sd->client_version < 2) // require 1.5.5.9 or above
        return;

    Session *s = sd->sess;

    Packet_Fixed<0x0214> fixed;
    fixed.variable = unwrap<QuestId>(questid);
    fixed.value = value;
    send_fpacket<0x0214, 8>(s, fixed);
    return;
}


func_table clif_parse_func_table[0x0220] =
{
    {0,     10, nullptr,                        },  // 0x0000
    {0,     0,  nullptr,                        },  // 0x0001
    {0,     0,  nullptr,                        },  // 0x0002
    {0,     0,  nullptr,                        },  // 0x0003
    {0,     0,  nullptr,                        },  // 0x0004
    {0,     0,  nullptr,                        },  // 0x0005
    {0,     0,  nullptr,                        },  // 0x0006
    {0,     0,  nullptr,                        },  // 0x0007
    {0,     0,  nullptr,                        },  // 0x0008
    {0,     0,  nullptr,                        },  // 0x0009
    {0,     0,  nullptr,                        },  // 0x000a
    {0,     0,  nullptr,                        },  // 0x000b
    {0,     0,  nullptr,                        },  // 0x000c
    {0,     0,  nullptr,                        },  // 0x000d
    {0,     0,  nullptr,                        },  // 0x000e
    {0,     0,  nullptr,                        },  // 0x000f
    {0,     0,  nullptr,                        },  // 0x0010
    {0,     0,  nullptr,                        },  // 0x0011
    {0,     0,  nullptr,                        },  // 0x0012
    {0,     0,  nullptr,                        },  // 0x0013
    {0,     0,  nullptr,                        },  // 0x0014
    {0,     0,  nullptr,                        },  // 0x0015
    {0,     0,  nullptr,                        },  // 0x0016
    {0,     0,  nullptr,                        },  // 0x0017
    {0,     0,  nullptr,                        },  // 0x0018
    {0,     0,  nullptr,                        },  // 0x0019
    {0,     0,  nullptr,                        },  // 0x001a
    {0,     0,  nullptr,                        },  // 0x001b
    {0,     0,  nullptr,                        },  // 0x001c
    {0,     0,  nullptr,                        },  // 0x001d
    {0,     0,  nullptr,                        },  // 0x001e
    {0,     0,  nullptr,                        },  // 0x001f
    {0,     0,  nullptr,                        },  // 0x0020
    {0,     0,  nullptr,                        },  // 0x0021
    {0,     0,  nullptr,                        },  // 0x0022
    {0,     0,  nullptr,                        },  // 0x0023
    {0,     0,  nullptr,                        },  // 0x0024
    {0,     0,  nullptr,                        },  // 0x0025
    {0,     0,  nullptr,                        },  // 0x0026
    {0,     0,  nullptr,                        },  // 0x0027
    {0,     0,  nullptr,                        },  // 0x0028
    {0,     0,  nullptr,                        },  // 0x0029
    {0,     0,  nullptr,                        },  // 0x002a
    {0,     0,  nullptr,                        },  // 0x002b
    {0,     0,  nullptr,                        },  // 0x002c
    {0,     0,  nullptr,                        },  // 0x002d
    {0,     0,  nullptr,                        },  // 0x002e
    {0,     0,  nullptr,                        },  // 0x002f
    {0,     0,  nullptr,                        },  // 0x0030
    {0,     0,  nullptr,                        },  // 0x0031
    {0,     0,  nullptr,                        },  // 0x0032
    {0,     0,  nullptr,                        },  // 0x0033
    {0,     0,  nullptr,                        },  // 0x0034
    {0,     0,  nullptr,                        },  // 0x0035
    {0,     0,  nullptr,                        },  // 0x0036
    {0,     0,  nullptr,                        },  // 0x0037
    {0,     0,  nullptr,                        },  // 0x0038
    {0,     0,  nullptr,                        },  // 0x0039
    {0,     0,  nullptr,                        },  // 0x003a
    {0,     0,  nullptr,                        },  // 0x003b
    {0,     0,  nullptr,                        },  // 0x003c
    {0,     0,  nullptr,                        },  // 0x003d
    {0,     0,  nullptr,                        },  // 0x003e
    {0,     0,  nullptr,                        },  // 0x003f
    {0,     0,  nullptr,                        },  // 0x0040
    {0,     0,  nullptr,                        },  // 0x0041
    {0,     0,  nullptr,                        },  // 0x0042
    {0,     0,  nullptr,                        },  // 0x0043
    {0,     0,  nullptr,                        },  // 0x0044
    {0,     0,  nullptr,                        },  // 0x0045
    {0,     0,  nullptr,                        },  // 0x0046
    {0,     0,  nullptr,                        },  // 0x0047
    {0,     0,  nullptr,                        },  // 0x0048
    {0,     0,  nullptr,                        },  // 0x0049
    {0,     0,  nullptr,                        },  // 0x004a
    {0,     0,  nullptr,                        },  // 0x004b
    {0,     0,  nullptr,                        },  // 0x004c
    {0,     0,  nullptr,                        },  // 0x004d
    {0,     0,  nullptr,                        },  // 0x004e
    {0,     0,  nullptr,                        },  // 0x004f
    {0,     0,  nullptr,                        },  // 0x0050
    {0,     0,  nullptr,                        },  // 0x0051
    {0,     0,  nullptr,                        },  // 0x0052
    {0,     0,  nullptr,                        },  // 0x0053
    {0,     0,  nullptr,                        },  // 0x0054
    {0,     0,  nullptr,                        },  // 0x0055
    {0,     0,  nullptr,                        },  // 0x0056
    {0,     0,  nullptr,                        },  // 0x0057
    {0,     0,  nullptr,                        },  // 0x0058
    {0,     0,  nullptr,                        },  // 0x0059
    {0,     0,  nullptr,                        },  // 0x005a
    {0,     0,  nullptr,                        },  // 0x005b
    {0,     0,  nullptr,                        },  // 0x005c
    {0,     0,  nullptr,                        },  // 0x005d
    {0,     0,  nullptr,                        },  // 0x005e
    {0,     0,  nullptr,                        },  // 0x005f
    {0,     0,  nullptr,                        },  // 0x0060
    {0,     0,  nullptr,                        },  // 0x0061
    {0,     0,  nullptr,                        },  // 0x0062
    {0,     VAR,nullptr,                        },  // 0x0063
    {0,     55, nullptr,                        },  // 0x0064
    {0,     17, nullptr,                        },  // 0x0065
    {0,     3,  nullptr,                        },  // 0x0066
    {0,     37, nullptr,                        },  // 0x0067
    {0,     46, nullptr,                        },  // 0x0068
    {0,     VAR,nullptr,                        },  // 0x0069
    {0,     23, nullptr,                        },  // 0x006a
    {0,     VAR,nullptr,                        },  // 0x006b
    {0,     3,  nullptr,                        },  // 0x006c
    {0,     108,nullptr,                        },  // 0x006d
    {0,     3,  nullptr,                        },  // 0x006e
    {0,     2,  nullptr,                        },  // 0x006f
    {0,     3,  nullptr,                        },  // 0x0070
    {0,     28, nullptr,                        },  // 0x0071
    {0,     19, clif_parse_WantToConnection,    },  // 0x0072
    {0,     11, nullptr,                        },  // 0x0073
    {0,     3,  nullptr,                        },  // 0x0074
    {0,     VAR,nullptr,                        },  // 0x0075
    {0,     9,  nullptr,                        },  // 0x0076
    {0,     5,  nullptr,                        },  // 0x0077
    {0,     54, nullptr,                        },  // 0x0078
    {0,     53, nullptr,                        },  // 0x0079
    {0,     58, nullptr,                        },  // 0x007a
    {0,     60, nullptr,                        },  // 0x007b
    {0,     41, nullptr,                        },  // 0x007c
    {-1,    2,  clif_parse_LoadEndAck,          },  // 0x007d
    {0,     6,  clif_parse_TickSend,            },  // 0x007e
    {0,     6,  nullptr,                        },  // 0x007f
    {0,     7,  nullptr,                        },  // 0x0080
    {0,     3,  nullptr,                        },  // 0x0081
    {0,     2,  nullptr,                        },  // 0x0082
    {0,     2,  nullptr,                        },  // 0x0083
    {0,     2,  nullptr,                        },  // 0x0084
    {-1,    5,  clif_parse_WalkToXY,            },  // 0x0085 Walk code limits this on it's own
    {0,     16, nullptr,                        },  // 0x0086
    {0,     12, nullptr,                        },  // 0x0087
    {0,     10, nullptr,                        },  // 0x0088
    {1000,  7,  clif_parse_ActionRequest,       },  // 0x0089 Special case - see below
    {0,     29, nullptr,                        },  // 0x008a
    {0,     23, nullptr,                        },  // 0x008b unknown... size 2 or 23?
    {300,   VAR,clif_parse_GlobalMessage,       },  // 0x008c
    {0,     VAR,nullptr,                        },  // 0x008d
    {0,     VAR,nullptr,                        },  // 0x008e
    {0,     0,  nullptr,                        },  // 0x008f
    {500,   7,  clif_parse_NpcClicked,          },  // 0x0090
    {0,     22, nullptr,                        },  // 0x0091
    {0,     28, nullptr,                        },  // 0x0092
    {0,     2,  nullptr,                        },  // 0x0093
    {-1,    6,  clif_parse_GetCharNameRequest,  },  // 0x0094
    {0,     30, nullptr,                        },  // 0x0095
    {300,   VAR,clif_parse_Wis,                 },  // 0x0096
    {0,     VAR,nullptr,                        },  // 0x0097
    {0,     3,  nullptr,                        },  // 0x0098
    {300,   VAR,nullptr,                        },  // 0x0099
    {0,     VAR,nullptr,                        },  // 0x009a
    {-1,    5,  clif_parse_ChangeDir,           },  // 0x009b
    {0,     9,  nullptr,                        },  // 0x009c
    {0,     17, nullptr,                        },  // 0x009d
    {0,     17, nullptr,                        },  // 0x009e
    {400,   6,  clif_parse_TakeItem,            },  // 0x009f
    {0,     23, nullptr,                        },  // 0x00a0
    {0,     6,  nullptr,                        },  // 0x00a1
    {50,    6,  clif_parse_DropItem,            },  // 0x00a2
    {0,     VAR,nullptr,                        },  // 0x00a3
    {0,     VAR,nullptr,                        },  // 0x00a4
    {0,     VAR,nullptr,                        },  // 0x00a5
    {0,     VAR,nullptr,                        },  // 0x00a6
    {0,     8,  clif_parse_UseItem,             },  // 0x00a7
    {0,     7,  nullptr,                        },  // 0x00a8
    {-1,    6,  clif_parse_EquipItem,           },  // 0x00a9 Special case - outfit window (not implemented yet - needs to allow bursts)
    {0,     7,  nullptr,                        },  // 0x00aa
    {-1,    4,  clif_parse_UnequipItem,         },  // 0x00ab Special case - outfit window (not implemented yet - needs to allow bursts)
    {0,     7,  nullptr,                        },  // 0x00ac
    {0,     0,  nullptr,                        },  // 0x00ad
    {0,     VAR,nullptr,                        },  // 0x00ae
    {0,     6,  nullptr,                        },  // 0x00af
    {0,     8,  nullptr,                        },  // 0x00b0
    {0,     8,  nullptr,                        },  // 0x00b1
    {0,     3,  clif_parse_Restart,             },  // 0x00b2
    {0,     3,  nullptr,                        },  // 0x00b3
    {0,     VAR,nullptr,                        },  // 0x00b4
    {0,     6,  nullptr,                        },  // 0x00b5
    {0,     6,  nullptr,                        },  // 0x00b6
    {0,     VAR,nullptr,                        },  // 0x00b7
    {0,     7,  clif_parse_NpcSelectMenu,       },  // 0x00b8
    {-1,    6,  clif_parse_NpcNextClicked,      },  // 0x00b9
    {0,     2,  nullptr,                        },  // 0x00ba
    {-1,    5,  clif_parse_StatusUp,            },  // 0x00bb People click this very quickly
    {0,     6,  nullptr,                        },  // 0x00bc
    {0,     44, nullptr,                        },  // 0x00bd
    {0,     5,  nullptr,                        },  // 0x00be
    {1000,  3,  clif_parse_Emotion,             },  // 0x00bf
    {0,     7,  nullptr,                        },  // 0x00c0
    {0,     2,  nullptr,                        },  // 0x00c1
    {0,     6,  nullptr,                        },  // 0x00c2
    {0,     8,  nullptr,                        },  // 0x00c3
    {0,     6,  nullptr,                        },  // 0x00c4
    {0,     7,  clif_parse_NpcBuySellSelected,  },  // 0x00c5
    {0,     VAR,nullptr,                        },  // 0x00c6
    {0,     VAR,nullptr,                        },  // 0x00c7
    {-1,    VAR,clif_parse_NpcBuyListSend,      },  // 0x00c8
    {-1,    VAR,clif_parse_NpcSellListSend,     },  // 0x00c9 Selling multiple 1-slot items
    {0,     3,  nullptr,                        },  // 0x00ca
    {0,     3,  nullptr,                        },  // 0x00cb
    {0,     6,  nullptr,                        },  // 0x00cc
    {0,     6,  nullptr,                        },  // 0x00cd
    {0,     2,  nullptr,                        },  // 0x00ce
    {0,     27, nullptr,                        },  // 0x00cf
    {0,     3,  nullptr,                        },  // 0x00d0
    {0,     4,  nullptr,                        },  // 0x00d1
    {0,     4,  nullptr,                        },  // 0x00d2
    {0,     2,  nullptr,                        },  // 0x00d3
    {0,     VAR,nullptr,                        },  // 0x00d4
    {0,     VAR,nullptr,                        },  // 0x00d5
    {0,     3,  nullptr,                        },  // 0x00d6
    {0,     VAR,nullptr,                        },  // 0x00d7
    {0,     6,  nullptr,                        },  // 0x00d8
    {0,     14, nullptr,                        },  // 0x00d9
    {0,     3,  nullptr,                        },  // 0x00da
    {0,     VAR,nullptr,                        },  // 0x00db
    {0,     28, nullptr,                        },  // 0x00dc
    {0,     29, nullptr,                        },  // 0x00dd
    {0,     VAR,nullptr,                        },  // 0x00de
    {0,     VAR,nullptr,                        },  // 0x00df
    {0,     30, nullptr,                        },  // 0x00e0
    {0,     30, nullptr,                        },  // 0x00e1
    {0,     26, nullptr,                        },  // 0x00e2
    {0,     2,  nullptr,                        },  // 0x00e3
    {2000,  6,  clif_parse_TradeRequest,        },  // 0x00e4
    {0,     26, nullptr,                        },  // 0x00e5
    {0,     3,  clif_parse_TradeAck,            },  // 0x00e6
    {0,     3,  nullptr,                        },  // 0x00e7
    {0,     8,  clif_parse_TradeAddItem,        },  // 0x00e8
    {0,     19, nullptr,                        },  // 0x00e9
    {0,     5,  nullptr,                        },  // 0x00ea
    {0,     2,  clif_parse_TradeOk,             },  // 0x00eb
    {0,     3,  nullptr,                        },  // 0x00ec
    {0,     2,  clif_parse_TradeCansel,         },  // 0x00ed
    {0,     2,  nullptr,                        },  // 0x00ee
    {0,     2,  clif_parse_TradeCommit,         },  // 0x00ef
    {0,     3,  nullptr,                        },  // 0x00f0
    {0,     2,  nullptr,                        },  // 0x00f1
    {0,     6,  nullptr,                        },  // 0x00f2
    {-1,    8,  clif_parse_MoveToKafra,         },  // 0x00f3
    {0,     21, nullptr,                        },  // 0x00f4
    {-1,    8,  clif_parse_MoveFromKafra,       },  // 0x00f5
    {0,     8,  nullptr,                        },  // 0x00f6
    {0,     2,  clif_parse_CloseKafra,          },  // 0x00f7
    {0,     2,  nullptr,                        },  // 0x00f8
    {2000,  26, clif_parse_CreateParty,         },  // 0x00f9
    {0,     3,  nullptr,                        },  // 0x00fa
    {0,     VAR,nullptr,                        },  // 0x00fb
    {2000,  6,  clif_parse_PartyInvite,         },  // 0x00fc
    {0,     27, nullptr,                        },  // 0x00fd
    {0,     30, nullptr,                        },  // 0x00fe
    {0,     10, clif_parse_ReplyPartyInvite,    },  // 0x00ff
    {0,     2,  clif_parse_LeaveParty,          },  // 0x0100
    {0,     6,  nullptr,                        },  // 0x0101
    {0,     6,  clif_parse_PartyChangeOption,   },  // 0x0102
    {0,     30, clif_parse_RemovePartyMember,   },  // 0x0103
    {0,     79, nullptr,                        },  // 0x0104
    {0,     31, nullptr,                        },  // 0x0105
    {0,     10, nullptr,                        },  // 0x0106
    {0,     10, nullptr,                        },  // 0x0107
    {300,   VAR,clif_parse_PartyMessage,        },  // 0x0108
    {0,     VAR,nullptr,                        },  // 0x0109
    {0,     4,  nullptr,                        },  // 0x010a
    {0,     6,  nullptr,                        },  // 0x010b
    {0,     6,  nullptr,                        },  // 0x010c
    {0,     2,  nullptr,                        },  // 0x010d
    {0,     11, nullptr,                        },  // 0x010e
    {0,     VAR,nullptr,                        },  // 0x010f
    {0,     10, nullptr,                        },  // 0x0110
    {0,     39, nullptr,                        },  // 0x0111
    {-1,    4,  clif_parse_SkillUp,             },  // 0x0112
    {0,     10, nullptr,                        },  // 0x0113
    {0,     31, nullptr,                        },  // 0x0114
    {0,     35, nullptr,                        },  // 0x0115
    {0,     10, nullptr,                        },  // 0x0116
    {0,     18, nullptr,                        },  // 0x0117
    {0,     2,  clif_parse_StopAttack,          },  // 0x0118
    {0,     13, nullptr,                        },  // 0x0119
    {0,     15, nullptr,                        },  // 0x011a
    {0,     20, nullptr,                        },  // 0x011b
    {0,     68, nullptr,                        },  // 0x011c
    {0,     2,  nullptr,                        },  // 0x011d
    {0,     3,  nullptr,                        },  // 0x011e
    {0,     16, nullptr,                        },  // 0x011f
    {0,     6,  nullptr,                        },  // 0x0120
    {0,     14, nullptr,                        },  // 0x0121
    {0,     VAR,nullptr,                        },  // 0x0122
    {0,     VAR,nullptr,                        },  // 0x0123
    {0,     21, nullptr,                        },  // 0x0124
    {0,     8,  nullptr,                        },  // 0x0125
    {0,     8,  nullptr,                        },  // 0x0126
    {0,     8,  nullptr,                        },  // 0x0127
    {0,     8,  nullptr,                        },  // 0x0128
    {0,     8,  nullptr,                        },  // 0x0129
    {0,     2,  nullptr,                        },  // 0x012a
    {0,     2,  nullptr,                        },  // 0x012b
    {0,     3,  nullptr,                        },  // 0x012c
    {0,     4,  nullptr,                        },  // 0x012d
    {0,     2,  nullptr,                        },  // 0x012e
    {0,     VAR,nullptr,                        },  // 0x012f
    {0,     6,  nullptr,                        },  // 0x0130
    {0,     86, nullptr,                        },  // 0x0131
    {0,     6,  nullptr,                        },  // 0x0132
    {0,     VAR,nullptr,                        },  // 0x0133
    {0,     VAR,nullptr,                        },  // 0x0134
    {0,     7,  nullptr,                        },  // 0x0135
    {0,     VAR,nullptr,                        },  // 0x0136
    {0,     6,  nullptr,                        },  // 0x0137
    {0,     3,  nullptr,                        },  // 0x0138
    {0,     16, nullptr,                        },  // 0x0139
    {0,     4,  nullptr,                        },  // 0x013a
    {0,     4,  nullptr,                        },  // 0x013b
    {0,     4,  nullptr,                        },  // 0x013c
    {0,     6,  nullptr,                        },  // 0x013d
    {0,     24, nullptr,                        },  // 0x013e
    {0,     26, nullptr,                        },  // 0x013f
    {0,     22, nullptr,                        },  // 0x0140
    {0,     14, nullptr,                        },  // 0x0141
    {0,     6,  nullptr,                        },  // 0x0142
    {300,   10, clif_parse_NpcAmountInput,      },  // 0x0143
    {0,     23, nullptr,                        },  // 0x0144
    {0,     19, nullptr,                        },  // 0x0145
    {300,   6,  clif_parse_NpcCloseClicked,     },  // 0x0146
    {0,     39, nullptr,                        },  // 0x0147
    {0,     8,  nullptr,                        },  // 0x0148
    {0,     9,  nullptr,                        },  // 0x0149
    {0,     6,  nullptr,                        },  // 0x014a
    {0,     27, nullptr,                        },  // 0x014b
    {0,     VAR,nullptr,                        },  // 0x014c
    {0,     2,  nullptr,                        },  // 0x014d
    {0,     6,  nullptr,                        },  // 0x014e
    {0,     6,  nullptr,                        },  // 0x014f
    {0,     110,nullptr,                        },  // 0x0150
    {0,     6,  nullptr,                        },  // 0x0151
    {0,     VAR,nullptr,                        },  // 0x0152
    {0,     VAR,nullptr,                        },  // 0x0153
    {0,     VAR,nullptr,                        },  // 0x0154
    {0,     VAR,nullptr,                        },  // 0x0155
    {0,     VAR,nullptr,                        },  // 0x0156
    {0,     6,  nullptr,                        },  // 0x0157
    {0,     VAR,nullptr,                        },  // 0x0158
    {0,     54, nullptr,                        },  // 0x0159
    {0,     66, nullptr,                        },  // 0x015a
    {0,     54, nullptr,                        },  // 0x015b
    {0,     90, nullptr,                        },  // 0x015c
    {0,     42, nullptr,                        },  // 0x015d
    {0,     6,  nullptr,                        },  // 0x015e
    {0,     42, nullptr,                        },  // 0x015f
    {0,     VAR,nullptr,                        },  // 0x0160
    {0,     VAR,nullptr,                        },  // 0x0161
    {0,     VAR,nullptr,                        },  // 0x0162
    {0,     VAR,nullptr,                        },  // 0x0163
    {0,     VAR,nullptr,                        },  // 0x0164
    {0,     30, nullptr,                        },  // 0x0165
    {0,     VAR,nullptr,                        },  // 0x0166
    {0,     3,  nullptr,                        },  // 0x0167
    {0,     14, nullptr,                        },  // 0x0168
    {0,     3,  nullptr,                        },  // 0x0169
    {0,     30, nullptr,                        },  // 0x016a
    {0,     10, nullptr,                        },  // 0x016b
    {0,     43, nullptr,                        },  // 0x016c
    {0,     14, nullptr,                        },  // 0x016d
    {0,     186,nullptr,                        },  // 0x016e
    {0,     182,nullptr,                        },  // 0x016f
    {0,     14, nullptr,                        },  // 0x0170
    {0,     30, nullptr,                        },  // 0x0171
    {0,     10, nullptr,                        },  // 0x0172
    {0,     3,  nullptr,                        },  // 0x0173
    {0,     VAR,nullptr,                        },  // 0x0174
    {0,     6,  nullptr,                        },  // 0x0175
    {0,     106,nullptr,                        },  // 0x0176
    {0,     VAR,nullptr,                        },  // 0x0177
    {0,     4,  nullptr,                        },  // 0x0178
    {0,     5,  nullptr,                        },  // 0x0179
    {0,     4,  nullptr,                        },  // 0x017a
    {0,     VAR,nullptr,                        },  // 0x017b
    {0,     6,  nullptr,                        },  // 0x017c
    {0,     7,  nullptr,                        },  // 0x017d
    {0,     VAR,nullptr,                        },  // 0x017e
    {0,     VAR,nullptr,                        },  // 0x017f
    {0,     6,  nullptr,                        },  // 0x0180
    {0,     3,  nullptr,                        },  // 0x0181
    {0,     106,nullptr,                        },  // 0x0182
    {0,     10, nullptr,                        },  // 0x0183
    {0,     10, nullptr,                        },  // 0x0184
    {0,     34, nullptr,                        },  // 0x0185
    {0,     0,  nullptr,                        },  // 0x0186
    {0,     6,  nullptr,                        },  // 0x0187
    {0,     8,  nullptr,                        },  // 0x0188
    {0,     4,  nullptr,                        },  // 0x0189
    {0,     4,  clif_parse_QuitGame,            },  // 0x018a
    {0,     4,  nullptr,                        },  // 0x018b
    {0,     29, nullptr,                        },  // 0x018c
    {0,     VAR,nullptr,                        },  // 0x018d
    {0,     10, nullptr,                        },  // 0x018e
    {0,     6,  nullptr,                        },  // 0x018f
    {0,     90, nullptr,                        },  // 0x0190
    {0,     86, nullptr,                        },  // 0x0191
    {0,     24, nullptr,                        },  // 0x0192
    {0,     6,  nullptr,                        },  // 0x0193
    {0,     30, nullptr,                        },  // 0x0194
    {0,     102,nullptr,                        },  // 0x0195
    {0,     9,  nullptr,                        },  // 0x0196
    {0,     4,  nullptr,                        },  // 0x0197
    {0,     8,  nullptr,                        },  // 0x0198
    {0,     4,  nullptr,                        },  // 0x0199
    {0,     14, nullptr,                        },  // 0x019a
    {0,     10, nullptr,                        },  // 0x019b
    {0,     VAR,nullptr,                        },  // 0x019c
    {300,   6,  nullptr,                        },  // 0x019d
    {0,     2,  nullptr,                        },  // 0x019e
    {0,     6,  nullptr,                        },  // 0x019f
    {0,     3,  nullptr,                        },  // 0x01a0
    {0,     3,  nullptr,                        },  // 0x01a1
    {0,     35, nullptr,                        },  // 0x01a2
    {0,     5,  nullptr,                        },  // 0x01a3
    {0,     11, nullptr,                        },  // 0x01a4
    {0,     26, nullptr,                        },  // 0x01a5
    {0,     VAR,nullptr,                        },  // 0x01a6
    {0,     4,  nullptr,                        },  // 0x01a7
    {0,     4,  nullptr,                        },  // 0x01a8
    {0,     6,  nullptr,                        },  // 0x01a9
    {0,     10, nullptr,                        },  // 0x01aa
    {0,     12, nullptr,                        },  // 0x01ab
    {0,     6,  nullptr,                        },  // 0x01ac
    {0,     VAR,nullptr,                        },  // 0x01ad
    {0,     4,  nullptr,                        },  // 0x01ae
    {0,     4,  nullptr,                        },  // 0x01af
    {0,     11, nullptr,                        },  // 0x01b0
    {0,     7,  nullptr,                        },  // 0x01b1
    {0,     VAR,nullptr,                        },  // 0x01b2
    {0,     67, nullptr,                        },  // 0x01b3
    {0,     12, nullptr,                        },  // 0x01b4
    {0,     18, nullptr,                        },  // 0x01b5
    {0,     114,nullptr,                        },  // 0x01b6
    {0,     6,  nullptr,                        },  // 0x01b7
    {0,     3,  nullptr,                        },  // 0x01b8
    {0,     6,  nullptr,                        },  // 0x01b9
    {0,     26, nullptr,                        },  // 0x01ba
    {0,     26, nullptr,                        },  // 0x01bb
    {0,     26, nullptr,                        },  // 0x01bc
    {0,     26, nullptr,                        },  // 0x01bd
    {0,     2,  nullptr,                        },  // 0x01be
    {0,     3,  nullptr,                        },  // 0x01bf
    {0,     2,  nullptr,                        },  // 0x01c0
    {0,     14, nullptr,                        },  // 0x01c1
    {0,     10, nullptr,                        },  // 0x01c2
    {0,     VAR,nullptr,                        },  // 0x01c3
    {0,     22, nullptr,                        },  // 0x01c4
    {0,     22, nullptr,                        },  // 0x01c5
    {0,     4,  nullptr,                        },  // 0x01c6
    {0,     2,  nullptr,                        },  // 0x01c7
    {0,     13, nullptr,                        },  // 0x01c8
    {0,     97, nullptr,                        },  // 0x01c9
    {0,     0,  nullptr,                        },  // 0x01ca
    {0,     9,  nullptr,                        },  // 0x01cb
    {0,     9,  nullptr,                        },  // 0x01cc
    {0,     30, nullptr,                        },  // 0x01cd
    {0,     6,  nullptr,                        },  // 0x01ce
    {0,     28, nullptr,                        },  // 0x01cf
    {0,     8,  nullptr,                        },  // 0x01d0
    {0,     14, nullptr,                        },  // 0x01d1
    {0,     10, nullptr,                        },  // 0x01d2
    {0,     35, nullptr,                        },  // 0x01d3
    {0,     6,  nullptr,                        },  // 0x01d4
    {300,   VAR,clif_parse_NpcStringInput,      },  // 0x01d5 - set to -1
    {0,     4,  nullptr,                        },  // 0x01d6
    {0,     11, nullptr,                        },  // 0x01d7
    {0,     54, nullptr,                        },  // 0x01d8
    {0,     53, nullptr,                        },  // 0x01d9
    {0,     60, nullptr,                        },  // 0x01da
    {0,     2,  nullptr,                        },  // 0x01db
    {0,     VAR,nullptr,                        },  // 0x01dc
    {0,     47, nullptr,                        },  // 0x01dd
    {0,     33, nullptr,                        },  // 0x01de
    {0,     6,  nullptr,                        },  // 0x01df
    {0,     30, nullptr,                        },  // 0x01e0
    {0,     8,  nullptr,                        },  // 0x01e1
    {0,     34, nullptr,                        },  // 0x01e2
    {0,     14, nullptr,                        },  // 0x01e3
    {0,     2,  nullptr,                        },  // 0x01e4
    {0,     6,  nullptr,                        },  // 0x01e5
    {0,     26, nullptr,                        },  // 0x01e6
    {0,     2,  nullptr,                        },  // 0x01e7
    {0,     28, nullptr,                        },  // 0x01e8
    {0,     81, nullptr,                        },  // 0x01e9
    {0,     6,  nullptr,                        },  // 0x01ea
    {0,     10, nullptr,                        },  // 0x01eb
    {0,     26, nullptr,                        },  // 0x01ec
    {0,     2,  nullptr,                        },  // 0x01ed
    {0,     VAR,nullptr,                        },  // 0x01ee
    {0,     VAR,nullptr,                        },  // 0x01ef
    {0,     VAR,nullptr,                        },  // 0x01f0
    {0,     VAR,nullptr,                        },  // 0x01f1
    {0,     20, nullptr,                        },  // 0x01f2
    {0,     10, nullptr,                        },  // 0x01f3
    {0,     32, nullptr,                        },  // 0x01f4
    {0,     9,  nullptr,                        },  // 0x01f5
    {0,     34, nullptr,                        },  // 0x01f6
    {0,     14, nullptr,                        },  // 0x01f7
    {0,     2,  nullptr,                        },  // 0x01f8
    {0,     6,  nullptr,                        },  // 0x01f9
    {0,     48, nullptr,                        },  // 0x01fa
    {0,     56, nullptr,                        },  // 0x01fb
    {0,     VAR,nullptr,                        },  // 0x01fc
    {0,     4,  nullptr,                        },  // 0x01fd
    {0,     5,  nullptr,                        },  // 0x01fe
    {0,     10, nullptr,                        },  // 0x01ff
    {0,     26, nullptr,                        },  // 0x0200
    {0,     VAR,nullptr,                        },  // 0x0201
    {0,     26, nullptr,                        },  // 0x0202
    {0,     10, nullptr,                        },  // 0x0203
    {0,     18, nullptr,                        },  // 0x0204
    {0,     26, nullptr,                        },  // 0x0205
    {0,     11, nullptr,                        },  // 0x0206
    {0,     34, nullptr,                        },  // 0x0207
    {0,     14, nullptr,                        },  // 0x0208
    {0,     36, nullptr,                        },  // 0x0209
    {0,     10, nullptr,                        },  // 0x020a
    {0,     19, nullptr,                        },  // 0x020b
    {0,     10, nullptr,                        },  // 0x020c
    {0,     VAR,nullptr,                        },  // 0x020d
    {0,     24, nullptr,                        },  // 0x020e
    {0,     0,  nullptr,                        },  // 0x020f
    {0,     0,  nullptr,                        },  // 0x0210
    {0,     0,  nullptr,                        },  // 0x0211
    {0,     0,  nullptr,                        },  // 0x0212
    {0,     0,  nullptr,                        },  // 0x0213
    {0,     0,  nullptr,                        },  // 0x0214
    {0,     0,  nullptr,                        },  // 0x0215
    {0,     0,  nullptr,                        },  // 0x0216
    {0,     0,  nullptr,                        },  // 0x0217
    {0,     0,  nullptr,                        },  // 0x0218
    {0,     0,  nullptr,                        },  // 0x0219
    {0,     0,  nullptr,                        },  // 0x021a
    {0,     0,  nullptr,                        },  // 0x021b
    {0,     0,  nullptr,                        },  // 0x021c
    {0,     0,  nullptr,                        },  // 0x021d
    {0,     0,  nullptr,                        },  // 0x021e
    {0,     0,  nullptr,                        },  // 0x021f
};

// Checks for packet flooding
static
uint16_t clif_check_packet_flood(Session *s, int cmd)
{
    uint16_t len = clif_parse_func_table[cmd].len_unused;
    if (len == VAR)
    {
        Little16 netlen;
        if (!packet_fetch(s, 2, reinterpret_cast<Byte *>(&netlen), 2))
        {
            return 0;
        }
        if (!network_to_native(&len, netlen))
        {
            s->set_eof();
            return 0;
        }
    }
    if (packet_avail(s) < len)
        return 0;

    dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));
    tick_t tick = gettick();

    // sd will not be set if the client hasn't requested
    // WantToConnection yet. Do not apply flood logic to GMs
    // as approved bots (GMlvl1) should not have to work around
    // flood logic.
    if (!sd || pc_isGM(sd) || clif_parse_func_table[cmd].rate == static_cast<interval_t>(-1))
        return 0;

    // Timer has wrapped
    if (tick < sd->flood_rates[cmd])
    {
        sd->flood_rates[cmd] = tick;
        return 0;
    }

    // Default rate is 100ms
    interval_t rate = clif_parse_func_table[cmd].rate;
    if (rate == interval_t::zero())
        rate = 100_ms;

    // ActionRequest - attacks are allowed a faster rate than sit/stand
    if (cmd == 0x89)
    {
        DamageType damage_type;
        Byte action_type;
        if (!packet_fetch(s, 6, &action_type, 1))
            return 0;
        if (!network_to_native(&damage_type, action_type))
        {
            s->set_eof();
            return 0;
        }
        if (damage_type == DamageType::NORMAL || damage_type == DamageType::CONTINUOUS)
            rate = 20_ms;
        else
            rate = 1_s;
    }

    // Restore this code when mana1.0 is released
    // nope, nuh-uh

    // They are flooding
    if (tick < sd->flood_rates[cmd] + rate)
    {
        // If it's a nasty flood we log and possibly kick
        if (tick > sd->packet_flood_reset_due)
        {
            sd->packet_flood_reset_due = tick + battle_config.packet_spam_threshold;
            sd->packet_flood_in = 0;
        }

        sd->packet_flood_in++;

        if (sd->packet_flood_in >= battle_config.packet_spam_flood)
        {
            PRINTF("packet flood detected from %s [0x%x]\n"_fmt, sd->status_key.name, cmd);
            if (battle_config.packet_spam_kick)
            {
                s->set_eof();
                return len;
            }
            sd->packet_flood_in = 0;
        }

        return len;
    }

    sd->flood_rates[cmd] = tick;
    return 0;
}

inline
void WARN_MALFORMED_MSG(dumb_ptr<map_session_data> sd, ZString msg)
{
    PRINTF("clif_validate_chat(): %s (ID %d) sent a malformed message: %s.\n"_fmt,
            sd->status_key.name, sd->status_key.account_id, msg);
}
/**
 * Validate message integrity (inspired by upstream source (eAthena)).
 *
 * @param sd active session data
 * @param type message type:
 *  0 for when the sender's name is not included (party chat)
 *  1 for when the target's name is included (whisper chat)
 *  2 for when the sender's name is given ("sender : text", public/guild chat)
 * @return a dynamically allocated copy of the message, or empty string upon failure
 */
static
AString clif_validate_chat(dumb_ptr<map_session_data> sd, ChatType type, XString buf)
{
    nullpo_retr(AString(), sd);
    /*
     * Don't send chat in the period between the ban and the connection's
     * closure.
     */
    if (sd->auto_ban_info.in_progress)
        return AString();

    Session *s = sd->sess;
    size_t name_len = sd->status_key.name.to__actual().size();
    XString pbuf = buf;

    /*
     * The client attempted to exceed the maximum message length.
     *
     * The conf suggests up to chat_maxline characters, after which the message
     * is truncated. But the previous behavior was to drop the message, so
     * we'll do that, too.
     */
    // TODO this cuts global chat short by (name_length + 3)
    if (buf.size() >= battle_config.chat_maxline)
    {
        WARN_MALFORMED_MSG(sd, "exceeded maximum message length"_s);
        return AString();
    }

    if (type == ChatType::Global)
    {
        XString p = pbuf;
        if (!(p.startswith(sd->status_key.name.to__actual()) && p.xslice_t(name_len).startswith(" : "_s)))
        {
            /* Disallow malformed/spoofed messages. */
            clif_setwaitclose(s);
            WARN_MALFORMED_MSG(sd, "spoofed name/invalid format"_s);
            return AString();
        }
        /* Step beyond the separator. */
        XString xs = p.xslice_t(name_len + 3);
        return xs;
    }
    return pbuf;
}

/*==========================================
 * クライアントからのパケット解析
 * socket.cのdo_parsepacketから呼び出される
 *------------------------------------------
 */
static
void clif_parse(Session *s)
{
    // old code:
    //  no while loop (can hang if more than one packet)
    //  handles 0x7530 and 0x7532 specially, also 0x0072
    //  checks packet length table
    //  checks rate limiter
    //  dispatches to actual function, unchecked
    // interstitial code:
    //  introduces while loop
    //  checks rate limiter
    //  dispatches to actual function
    //   if incomplete, unchecks rate limiter
    //   if error, close socket
    // future code:
    //  hoists while loop
    //  treats all packets as variable-length, except a hard-coded list
    //  reads packet of that length unconditionally into a buffer
    //  does rate-limit check (hoisted?)
    //  dispatches to actual function
    //   if error, close socket

    dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(s->session_data.get()));

    if (!sd || (sd && !sd->state.auth))
    {
        uint16_t packet_id;
        if (!packet_peek_id(s, &packet_id))
            return;

        if (packet_id != 0x0072 && packet_id != 0x7530)
        {
            // first packet must be auth or finger
            s->set_eof();
            return;
        }
    }

    if (!chrif_isconnect())
    {
        s->set_eof();
        return;
    }
    if (sd && sd->state.auth == 1 && sd->state.waitingdisconnect == 1)
    {
        packet_discard(s, packet_avail(s));
        return;
    }

    uint16_t packet_id;
    RecvResult rv = RecvResult::Complete;
    while (rv == RecvResult::Complete && packet_peek_id(s, &packet_id))
    {
        switch (packet_id)
        {
            case 0x7530:
            {
                Packet_Fixed<0x7530> fixed;
                rv = recv_fpacket<0x7530, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7531> fixed_31;
                fixed_31.version = CURRENT_MAP_SERVER_VERSION;
                send_fpacket<0x7531, 10>(s, fixed_31);
                break;
            }
            case 0x7532:
            {
                Packet_Fixed<0x7532> fixed;
                rv = recv_fpacket<0x7532, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                s->set_eof();
                break;
            }
        }
        if (packet_id < 0x0220)
        {
            if (uint16_t len = clif_check_packet_flood(s, packet_id))
            {
                // Packet flood: skip packet
                packet_discard(s, len);
                rv = RecvResult::Complete;
            }
            else
            {
                clif_func func = clif_parse_func_table[packet_id].func;
                if (!func)
                    goto unknown_packet;
                rv = func(s, sd);
            }
        }
        else
            goto unknown_packet;
    }

    if (rv == RecvResult::Error)
        s->set_eof();
    return;

unknown_packet:
    {
        if (battle_config.error_log)
        {
            if (s)
                PRINTF("\nclif_parse: session #%d, packet 0x%x, lenght %zu\n"_fmt,
                        s, packet_id, packet_avail(s));
            {
                if (sd && sd->state.auth)
                {
                    PRINTF("Unknown packet: Account ID %d, character ID %d, player name %s.\n"_fmt,
                            sd->status_key.account_id, sd->status_key.char_id,
                            sd->status_key.name);
                }
                else if (sd)    // not authentified! (refused by char-server or disconnect before to be authentified)
                    PRINTF("Unkonwn packet (unauthenticated): Account ID %d.\n"_fmt, sd->bl_id);
                else
                    PRINTF("Unknown packet (unknown)\n"_fmt);

                {
                    timestamp_seconds_buffer now;
                    stamp_time(now);
                    if (sd && sd->state.auth)
                    {
                        FPRINTF(stderr,
                                "%s\nPlayer with account ID %d (character ID %d, player name %s) sent wrong packet:\n"_fmt,
                                now,
                                sd->status_key.account_id,
                                sd->status_key.char_id, sd->status_key.name);
                    }
                    else if (sd)    // not authentified! (refused by char-server or disconnect before to be authentified)
                        FPRINTF(stderr,
                                "%s\nUnauthenticated player with account ID %d sent wrong packet:\n"_fmt,
                                now, sd->bl_id);
                    else
                        FPRINTF(stderr,
                                "%s\nUnknown connection sent wrong packet:\n"_fmt,
                                now);

                    packet_dump(s);
                }
            }
        }
    }
}

void do_init_clif(void)
{
    make_listen_port(map_conf.map_port, SessionParsers{.func_parse= clif_parse, .func_delete= clif_delete});
}
} // namespace map
} // namespace tmwa
