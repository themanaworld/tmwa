#include "int_guild.hpp"
//    int_guild.cpp - Internal guild handling.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 Freeyorp
//    Copyright © 2015 Rawng
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

#include "../ints/udl.hpp"

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/write.hpp"

#include "../proto2/char-map.hpp"

#include "../mmo/ids.hpp"

#include "../high/extract_mmo.hpp"
#include "../high/mmo.hpp"

#include "../wire/packets.hpp"

#include "char.hpp"
#include "globals.hpp"
#include "inter.hpp"
#include "inter_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{

static
bool impl_extract(XString str, GuildPair *gp)
{
    GuildPair& g = *gp;

    // not compatible with the normal extract()ors since it has
    // a variable-size element that uses the same separator
    std::vector<XString> bits;
    if (!extract(str, vrec<'\t'>(&bits)))
        return false;
    auto begin = bits.begin();
    auto end = bits.end();
    if (begin == end || !extract(*begin, &g.guild_id))
        return false;
    ++begin;
    if (begin == end || !extract(*begin, &g.guild_most->name))
        return false;
    ++begin;
    if (begin == end || !extract(*begin, &g.guild_most->exp))
        return false;
    ++begin;

    for (int i = 0; begin != end && i < MAX_GUILD; ++i)
    {
        GuildMember *m = &g->member[i];

        if (begin == end || !extract(*begin, &m->account_id))
            return false;
        ++begin;
        if (begin == end || !extract(*begin, &m->position))
            return false;
        ++begin;
        if (begin == end || !extract(*begin, &m->name))
            return false;
        ++begin;
        if (!m->account_id)
            --i;
    }
    return true;
}

namespace char_
{

static
int guild_check_empty(GuildPair gp);

/*========================================
 *
 *----------------------------------------
 */
static
void guild_check_deleted_init(GuildPair gp)
{
    for (int i = 0; i < MAX_GUILD; ++i)
    {
        if (!gp->member[i].account_id)
            continue;
        const CharPair *c = search_character(gp->member[i].name);
        if (!c || c->key.account_id != gp->member[i].account_id)
        {
            CHAR_LOG("WARNING: deleting obsolete guild member %d %s of %d %s\n"_fmt,
                    gp->member[i].account_id, gp->member[i].name,
                    gp.guild_id, gp->name);
            PRINTF("WARNING: deleting obsolete guild member %d %s of %d %s\n"_fmt,
                    gp->member[i].account_id, gp->member[i].name,
                    gp.guild_id, gp->name);
            gp->member[i] = GuildMember{};
        }
    }
}

/*========================================
 * Initialize guilds from db.
 *----------------------------------------
 */
void inter_guild_init(void)
{
    io::ReadFile in(inter_conf.guild_txt);
    if (!in.is_open())
        return;

    // TODO: convert to use char_id
    AString line;
    int c = 0;
    while (in.getline(line))
    {
        GuildId i;
        if (extract(line, record<'\t'>(&i, "%newid%"_s))
            && guild_newid < i)
        {
            guild_newid = i;
            continue;
        }

        GuildMost gm;
        GuildPair gp{GuildId(), borrow(gm)};
        if (extract(line, &gp) && gp.guild_id)
        {
            if (guild_newid < next(gp.guild_id))
                guild_newid = next(gp.guild_id);
            guild_check_deleted_init(gp);
            guild_db.insert(gp.guild_id, gm);
            guild_check_empty(gp);
            PRINTF("int_guild: loaded %d guilds from db.\n"_fmt, c + 1);
        }
        else
        {
            PRINTF("int_guild: broken data [%s] line %d\n"_fmt, inter_conf.guild_txt, c + 1);
        }
        c++;
    }
}

/*========================================
 *
 *----------------------------------------
 */
static
AString inter_guild_tostr(GuildPair g)
{
    MString str;
    str += STRPRINTF(
            "%d\t"
            "%s\t"
            "%d\t"_fmt,
            g.guild_id,
            g.guild_most->name,
            g.guild_most->exp);
    for (int i = 0; i < MAX_GUILD; ++i)
    {
        GuildMember *m = &g->member[i];
        if (!m->account_id)
            continue;
        str += STRPRINTF(
                "%d\t"
                "%d\t"
                "%s\t"_fmt,
                m->account_id,
                m->position,
                m->name);
    }

    return AString(str);
}

/*========================================
 *
 *----------------------------------------
 */
int guild_check_empty(const GuildPair gp)
{
    int i;

    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].account_id)
        {
            return 0;
        }
    }
    // The guild is empty, so break it and erase it from the db.
    //mapif_guild_broken(gp.guild_id, 0);
    guild_db.erase(gp.guild_id);

    return 1;
}

/*========================================
 *
 *----------------------------------------
 */
static
void search_guildname_sub(GuildPair tmp, GuildName str, Borrowed<Option<GuildPair>> dst)
{
    if (tmp->name == str)
            *dst = Some(tmp);
}

/*========================================
 *
 *----------------------------------------
 */
static
Option<GuildPair> search_guildname(GuildName str)
{
    Option<GuildPair> gp = None;

    for (auto& pair : guild_db)
    {
        GuildPair tmp{pair.first, borrow(pair.second)};
        search_guildname_sub(tmp, str, borrow(gp));
    }

    return gp;
}

/*========================================
 * Send guild info to map server.
 *----------------------------------------
 */
static
void mapif_guild_info(Session* s, AccountId account_id, GuildPair gp)
{
    Packet_Head<0x3831> head_31;
    Packet_Option<0x3831> option_31;

    head_31.flag = 1;
    head_31.guild_id = gp.guild_id;
    head_31.account_id = account_id;

    option_31.guild_most = *gp.guild_most;

    send_opacket<0x3831, 13, sizeof(NetPacket_Option<0x3831>)>(s, head_31, true, option_31);
}

/*========================================
 * Send guild creation result to map server.
 *----------------------------------------
 */
static
void mapif_guild_created(Session* s, AccountId account_id, Option<GuildPair> gp_)
{
    Packet_Fixed<0x3830> fixed_30;
    fixed_30.account_id = account_id;

    OMATCH_BEGIN (gp_)
    {
        OMATCH_CASE_SOME (gp)
        {
            fixed_30.error = 0;
            fixed_30.guild_id = gp.guild_id;
            fixed_30.guild_name = gp->name;
        }
        OMATCH_CASE_NONE ()
        {
            fixed_30.error = 1;
            fixed_30.guild_id = GuildId();
            fixed_30.guild_name = stringish<GuildName>("error"_s);
        }
    }
    OMATCH_END ();

    send_fpacket<0x3830, 35>(s, fixed_30);
}

/*========================================
 * Relay to maper server the result of
 * adding character to guild.
 * flag:
 *          0 : success
 *          1 : error
 *----------------------------------------
 */
static
void mapif_guild_memberadded(Session* s, GuildId guild_id, AccountId account_id, int flag)
{
    Packet_Fixed<0x3832> fixed_32;
    fixed_32.guild_id = guild_id;
    fixed_32.account_id = account_id;
    fixed_32.unused_char_id = CharId();
    fixed_32.flag = flag;

    send_fpacket<0x3832, 15>(s, fixed_32);
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_guild_addmember(Session* s, GuildId guild_id, AccountId account_id, int i, CharName name, int lv)
{
    Option<P<GuildMost>> ogm_ = guild_db.search(guild_id);
    OMATCH_BEGIN (ogm_)
    {
        OMATCH_CASE_SOME (ogm)
        {
            ogm->member[i].account_id = account_id;
            ogm->member[i].name = name;
            ogm->member[i].position = 4;
            ogm->member[i].online = 1;
            ogm->member[i].lv = lv;
            mapif_guild_memberadded(s, guild_id, account_id, 0);

            GuildPair gp = {guild_id, ogm};
            mapif_guild_info(s, account_id, gp);
        }
        OMATCH_CASE_NONE ()
        {
            {
                PRINTF("    Fatal error, char server could not find guild when adding member.\n"_fmt);
                mapif_guild_memberadded(s, guild_id, account_id, 1);
                return;
            }
        }
    }
    OMATCH_END ();
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_guild_left(Session* s, GuildId guild_id, AccountId account_id, int flag, CharName name, AString mes)
{
    Packet_Fixed<0x3834> fixed_834;
    fixed_834.guild_id = guild_id;
    fixed_834.account_id = account_id;
    fixed_834.char_id = CharId();
    fixed_834.flag = flag;
    fixed_834.mes = mes;
    fixed_834.char_name = name;

    send_fpacket<0x3834, 79>(s, fixed_834);
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_guild_message(AccountId account_id, GuildId guild_id, XString mes)
{
    Packet_Head<0x3837> head_37;
    XString repeat_37 = mes;
    head_37.account_id = account_id;
    head_37.guild_id = guild_id;
    for (Session *ss : iter_map_sessions())
    {
        send_vpacket<0x3837, 12, 1>(ss, head_37, repeat_37);
    }
}

/*========================================
 * Notify map servers of guild expulsion.
 *----------------------------------------
 */
static
void mapif_guild_expulsion(AccountId kicked_id, AString reason, AccountId kicker_id)
{
    Packet_Fixed<0x3838> fixed_38;
    fixed_38.kicked_id = kicked_id;
    fixed_38.reason = reason;
    fixed_38.kicker_id = kicker_id;
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x3838, 50>(ss, fixed_38);
    }
}

/*========================================
 * Notify map servers of guild expulsion.
 *----------------------------------------
 */
static
void mapif_guild_changeposition(GuildId guild_id, AccountId account_id, int position)
{
    Packet_Fixed<0x3839> fixed_39;
    fixed_39.guild_id = guild_id;
    fixed_39.account_id = account_id;
    //fixed_39.char_id = char_id;
    fixed_39.position = position;
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x3839, 14>(ss,fixed_39);
    }
}

/*========================================
 * Handle guild create request.
 *----------------------------------------
 */
static
void mapif_parse_CreateGuild(Session *s, AccountId account_id, GuildName guild_name, CharName char_name, int lv)
{
    // name is invalid
    if (!guild_name.is_print())
    {
        mapif_guild_created(s, account_id, None);
        return;
    }

    // guild name already exists
    if (search_guildname(guild_name).is_some())
    {
        mapif_guild_created(s, account_id, None);
        return;
    }

    GuildMost g;
    guild_newid = next(guild_newid);
    GuildPair gp{guild_newid, borrow(g)};
    g.name = guild_name;
    g.exp = 0;
    g.member[0].account_id = account_id;
    g.member[0].name = char_name;
    g.member[0].position = 1;
    g.member[0].online = 1;
    g.member[0].lv = lv;

    guild_db.insert(gp.guild_id, g);

    PRINTF("int_guild: created! %d %s\n"_fmt, gp.guild_id, gp.guild_most->name);

    mapif_guild_created(s, account_id, Some(gp));
    mapif_guild_info(s, account_id, gp);
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_parse_GuildInfo(Session *s, GuildId guild_id, AccountId account_id)
{
    Option<P<GuildMost>> maybe_guild_most = guild_db.search(guild_id);
    OMATCH_BEGIN (maybe_guild_most)
    {
        OMATCH_CASE_SOME (guild_most)
        {
            int i;
            for (i = 0; i < MAX_GUILD; ++i)
            {
                if (guild_most->member[i].account_id == account_id)
                {
                    guild_most->member[i].online = 1;
                    break;
                }
            }
            mapif_guild_info(s, account_id, GuildPair{guild_id, guild_most});
        }
        OMATCH_CASE_NONE ()
        {
            //mapif_guild_noinfo(s, guild_id);
        }
    }
    OMATCH_END ();
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_parse_GuildAddMember(Session *s, GuildId guild_id, int i, AccountId account_id, CharName name, int lv)
{
    mapif_guild_addmember(s, guild_id, account_id, i, name, lv);
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_parse_GuildLeave(Session* s, GuildId guild_id, AccountId account_id, int flag, AString mes)
{
    Option<P<GuildMost>> maybe_guild_most = guild_db.search(guild_id);
    P<GuildMost> gm = TRY_UNWRAP(maybe_guild_most,
            {
                    PRINTF("    Error: could not find guild.\n"_fmt);
                    PRINTF("        given guild_id : %d\n"_fmt, guild_id);
                    return;
            }
    );

    int i;
    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gm->member[i].account_id == account_id)
        {
            if (flag)
            {
                // expulsion stuff
                PRINTF("    Error in mapif_parse_GuildLeave: flag should be false (for now).\n"_fmt);
            }

            mapif_guild_left(s, guild_id, account_id, flag, gm->member[i].name, mes);

            gm->member[i].account_id = AccountId();
            gm->member[i].name = CharName();
            gm->member[i].position = 0;
            gm->member[i].online = 0;
            gm->member[i].lv = 0;

            GuildPair gp{guild_id, gm};

            if (guild_check_empty(gp) == 0)
            {
                mapif_guild_info (s, account_id, gp );
            }

            break;
        }
    }
}

/*========================================
 *
 *----------------------------------------
 */
static
void mapif_parse_GuildMessage(/*Session* s, */AccountId account_id, GuildId guild_id, XString mes)
{
    mapif_guild_message(account_id, guild_id, mes);
}

/*========================================
 * Process guild expulsion notification
 *----------------------------------------
 */
static
void mapif_parse_GuildExpulsion(/*Session *s,*/ GuildId guild_id, AccountId kicked_id, AString reason, AccountId kicker_id)
{
    Option<P<GuildMost>> maybe_guild_most = guild_db.search(guild_id);
    P<GuildMost> gm = TRY_UNWRAP(maybe_guild_most,
            {
                    return;
            }
    );

    int i;
    for (i = 0; i <= MAX_GUILD; i++)
    {
        if (gm->member[i].account_id == kicked_id)
        {
            gm->member[i].account_id = AccountId();
            gm->member[i].name = CharName();
            gm->member[i].position = 0;
            gm->member[i].online = 0;
            gm->member[i].lv = 0;
            break;
        }
    }

    mapif_guild_expulsion(kicked_id, reason, kicker_id);
}

/*========================================
 * Process guild position change notification
 *----------------------------------------
 */
static
void mapif_parse_GuildChangePos(GuildId guild_id, AccountId account_id, int position)
{
    Option<P<GuildMost>> maybe_guild_most = guild_db.search(guild_id);
    OMATCH_BEGIN (maybe_guild_most)
    {
        OMATCH_CASE_SOME (guild_most)
        {
            int i;
            for (i = 0; i < MAX_GUILD; ++i)
            {
                if (guild_most->member[i].account_id == account_id)
                {
                    guild_most->member[i].position = position;
                    mapif_guild_changeposition(guild_id, account_id, position);
                    break;
                }
            }
        }
        OMATCH_CASE_NONE ()
        {
            PRINTF("Error: mapif_parse_GuildChangePos -- guild not found\n"_fmt);
        }
    }
    OMATCH_END ();
}

/*========================================
 *
 *----------------------------------------
 */
static
void inter_guild_save_sub(GuildPair data, io::WriteFile& gp)
{
    AString line = inter_guild_tostr(data);
    gp.put_line(line);
}

/*========================================
 *
 *----------------------------------------
 */
int inter_guild_save(void)
{
    io::WriteLock fg(inter_conf.guild_txt);
    if (!fg.is_open())
    {
        PRINTF("int_guild: cant write [%s] !!! data is lost !!!\n"_fmt, inter_conf.guild_txt);
        return 1;
    }
    for (auto& pair : guild_db)
    {
        GuildPair tmp{pair.first, borrow(pair.second)};
        inter_guild_save_sub(tmp, fg);
    }

    return 0;
}

/*========================================
 * Parse guild packet from map server.
 *----------------------------------------
 */
RecvResult inter_guild_parse_frommap(Session *ms, uint16_t packet_id)
{
    RecvResult rv = RecvResult::Error;
    switch (packet_id)
    {
        case 0x3030:
        {
            Packet_Fixed<0x3030> fixed;
            rv = recv_fpacket<0x3030, 56>(ms, fixed);
            if (rv != RecvResult::Complete)
                    break;

            AccountId account_id = fixed.account_id;
            GuildName guild_name = fixed.guild_name;
            CharName char_name = fixed.char_name;
            int lv = fixed.level;

            mapif_parse_CreateGuild(ms,
                                    account_id,
                                    guild_name,
                                    char_name,
                                    lv);
            break;
        }
        case 0x3031:
        {
            Packet_Fixed<0x3031> fixed;
            rv = recv_fpacket<0x3031, 10>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            GuildId guild_id = fixed.guild_id;
            AccountId account_id = fixed.account_id;
            mapif_parse_GuildInfo(ms, guild_id, account_id);
            break;
        }
        case 0x3032:
        {
            Packet_Fixed<0x3032> fixed;
            rv = recv_fpacket<0x3032, 40>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            GuildId guild_id = fixed.guild_id;
            AccountId account_id = fixed.account_id;
            CharName name = fixed.name;
            int i = fixed.position;
            int lv = fixed.lv;
            mapif_parse_GuildAddMember(ms, guild_id, i, account_id, name, lv);
            break;
        }
        case 0x3034:
        {
            Packet_Fixed<0x3034> fixed;
            rv = recv_fpacket<0x3034, 55>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            GuildId guild_id = fixed.guild_id;
            AccountId account_id = fixed.account_id;
            int flag = fixed.flag;
            AString mes = fixed.mes;

            mapif_parse_GuildLeave(ms, guild_id, account_id, flag, mes);
            break;
        }
        case 0x3037:
        {
            Packet_Head<0x3037> head_37;
            AString repeat_37;
            rv = recv_vpacket<0x3037, 12, 1>(ms, head_37, repeat_37);
            if (rv != RecvResult::Complete)
                break;

            AccountId account_id = head_37.account_id;
            GuildId guild_id = head_37.guild_id;
            AString& mes = repeat_37;
            mapif_parse_GuildMessage(/*ms,*/account_id, guild_id, mes);
            break;
        }
        case 0x3038:
        {
            Packet_Fixed<0x3038> fixed;
            rv = recv_fpacket<0x3038, 54>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            GuildId guild_id = fixed.guild_id;
            AccountId kicked_id = fixed.kicked_id;
            AString reason = fixed.reason;
            AccountId kicker_id = fixed.kicker_id;

            mapif_parse_GuildExpulsion(/*ms,*/ guild_id, kicked_id, reason, kicker_id);
            break;
        }
        case 0x3039:
        {
            Packet_Fixed<0x3039> fixed;
            rv = recv_fpacket<0x3039, 14>(ms, fixed);
            if (rv != RecvResult::Complete)
                break;

            GuildId guild_id = fixed.guild_id;
            AccountId account_id = fixed.account_id;
            //CharId char_id = fixed.char_id;
            int position = fixed.position;

            mapif_parse_GuildChangePos(guild_id, account_id, position);
            break;
        }
        default:
            return RecvResult::Error;
    }

    return rv;
}

} // namespace char_
} // namespace tmwa
