#include "guild.hpp"
//    guild.cpp - Large groups of long term allies.
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

#include "../compat/nullpo.hpp"

#include "../strings/xstring.hpp"

#include "../generic/db.hpp"

#include "../io/cxxstdio.hpp"

#include "../net/timer.hpp"

#include "../mmo/ids.hpp"
#include "../high/mmo.hpp"

#include "battle.hpp"
#include "battle_conf.hpp"
#include "clif.hpp"
#include "globals.hpp"
#include "intif.hpp"
#include "map.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{

/*========================================
 *
 *----------------------------------------
 */
Option<GuildPair> guild_search(GuildId guild_id)
{
    Option<P<GuildMost>> guild_most_ = guild_db.search(guild_id);
    return guild_most_.map([guild_id](P<GuildMost> guild_most)
            {
                return GuildPair{guild_id, guild_most};
            });
}

/*========================================
 *
 *----------------------------------------
 */
static
void guild_searchname_sub(GuildPair tmp, GuildName str, Borrowed<Option<GuildPair>> dst)
{
    if (tmp->name == str)
            *dst = Some(tmp);
}

/*========================================
 * search for a guild by name
 *----------------------------------------
 */
Option<GuildPair> guild_searchname(GuildName str)
{
    Option<GuildPair> gp = None;

    for (auto& pair : guild_db)
    {
        GuildPair tmp{pair.first, borrow(pair.second)};
        guild_searchname_sub(tmp, str, borrow(gp));
    }

    return gp;
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
    guild_db.erase(gp.guild_id);

    return 1;
}

XString guild_get_position(GuildPair gp, AccountId acct)
{
    int i;
    XString position_name;

    for (i = 1; i < MAX_GUILD; ++i)
    {
        if (gp.guild_most->member[i].account_id == acct)
        {
            switch (gp.guild_most->member[i].position)
            {
                case 1:
                {
                    position_name = "Guild Master"_s;
                    return position_name;
                }
                case 2:
                {
                    position_name = "Executor"_s;
                    return position_name;
                }
                case 3:
                {
                    position_name = "Officer"_s;
                    return position_name;
                }
                case 4:
                {
                    position_name = "Member"_s;
                    return position_name;
                }
                default:
                {
                    position_name = ""_s;
                    return position_name;
                }
            }
        }
    }
    position_name = ""_s;
    return position_name;
}

/*
 *========================================
 * Remove a guild from the guild DB
 *----------------------------------------
 */
void guild_break(GuildId guild_id)
{
    Option<GuildPair> gp_ = guild_search(guild_id);
    GuildPair gp = TRY_UNWRAP(gp_,
            {
                return;
            });

    AString broken_mes = "Guild broken"_s;

    int i;
    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp.guild_most->member[i].name != CharName())
        {
            if (gp.guild_most->member[i].sd != nullptr)
            {
                guild_leave(dumb_ptr<map_session_data>(gp.guild_most->member[i].sd), guild_id, gp.guild_most->member[i].account_id, broken_mes);
                guild_member_left(gp.guild_id, gp.guild_most->member[i].account_id, 0, broken_mes, gp.guild_most->member[i].name);
            }

            gp.guild_most->member[i].account_id = AccountId();
            gp.guild_most->member[i].lv = 0;
            gp.guild_most->member[i].name = CharName();
            gp.guild_most->member[i].online = 0;
            gp.guild_most->member[i].position = 0;
            gp.guild_most->member[i].sd = nullptr;
        }
    }

    guild_db.erase(guild_id);
    return;
}

/*
 *========================================
 * Send guild creation request to char server
 *----------------------------------------
 */
void guild_create(dumb_ptr<map_session_data> sd, GuildName guild_name)
{
    nullpo_retv(sd);

    guild_name = stringish<GuildName>(guild_name.strip());

    // The guild name is empty/invalid.
    if (!guild_name)
    {
        clif_guild_created(sd, 1);
        return;
    }

    // Make sure the character isn't already in a guild.
    if (!sd->status.guild_id)
    {
        intif_guild_create(sd, guild_name);
    }
    else
    {
        clif_guild_created(sd, 2);
    }

    return;
}

/**
 *========================================
 * Send guild create response to client
 *----------------------------------------
**/
void guild_created(AccountId account_id, int fail, GuildId guild_id, GuildName name)
{
    dumb_ptr<map_session_data> sd;
    sd = map_id2sd(account_to_block(account_id));

    nullpo_retv(sd); // if this happens, guild exists for char but not map.

    if (!fail)
    {

        sd->status.guild_id = guild_id;

        // this would also be very bad
        if (guild_search(guild_id).is_some())
        {
            PRINTF("guild_created(): ID already exists!\n"_fmt);
            exit(1);
        }

        Borrowed<GuildMost> gm = guild_db.init(guild_id);
        gm->name = name;

        clif_guild_created(sd, 0);
    }
    else
        clif_guild_created(sd, 1);
}

/*========================================
 *
 *----------------------------------------
 */
void guild_request_info(GuildId guild_id, AccountId account_id)
{
    intif_request_guildinfo(guild_id, account_id);
}

/*========================================
 *
 *----------------------------------------
 */
void guild_makemember(dumb_ptr<map_session_data> sd, int i, GuildPair gp)
{
    nullpo_retv(sd);

    gp->member[i].account_id = sd->status_key.account_id;
    gp->member[i].name = sd->status_key.name;
    gp->member[i].position = 4;
    gp->member[i].online = 1;
    gp->member[i].lv = sd->status.base_level;

    return;
}

/*========================================
 *
 *----------------------------------------
 */
/*void guild_recv_noinfo(GuildPair gp)
{

}*/


/*========================================
 *
 *----------------------------------------
 */
static
GuildPair handle_info(const GuildPair gp)
{
    Option<GuildPair> g_ = guild_search(gp.guild_id);
    OMATCH_BEGIN_SOME (g, g_)
    {
        *g.guild_most = *gp.guild_most;
        return g;
    }
    OMATCH_END ();
    {
        GuildPair g{gp.guild_id, guild_db.init(gp.guild_id)};

        *g.guild_most = *gp.guild_most;
        //guild_check_member(g);
        return g;
    }
}

/*========================================
 * Send guild information to cilent.
 *----------------------------------------
 */
void guild_recv_info(GuildPair gp)
{
    GuildPair g = handle_info(gp);

    int i;
    for (i = 0; i < MAX_GUILD; i++)
    {
        dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(g->member[i].account_id));
        g->member[i].sd = (sd != nullptr
                           && sd->status.guild_id == g.guild_id) ? sd.operator->() : nullptr;
        if (g->member[i].sd != nullptr)
        {
            g->member[i].sd->guild_sended = 0;
        }
    }

    CharName leader;

    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp.guild_most->member[i].position == 1)
        {
            leader = gp.guild_most->member[i].name;
            break;
        }
    }

    for (i = 0; i < MAX_GUILD; i++)
    {
        dumb_ptr<map_session_data> sd = dumb_ptr<map_session_data>(g->member[i].sd);
        if (sd != nullptr && sd->guild_sended == 0)
        {
            clif_guild_basicinfo(sd, gp.guild_id, gp.guild_most->name, leader);
            //clif_guild_emblem();
            clif_guild_memberlist(sd, *gp.guild_most);
            //clif_guild_skillinfo();
            clif_guild_belonginfo(sd, gp, g->member[i].position);
            //clif_guild_notice();
            sd->guild_sended = 1;
        }
    }
}

/*========================================
 * Parse response from char server about
 * addint guild member.
 * flags:
 *          0 : success
 *          1 : error
 *----------------------------------------
 */
void guild_memberadded(GuildId guild_id, AccountId account_id, int flag)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(account_id));
    Option<GuildPair> guild_pair_maybe = guild_search(guild_id);

    GuildPair gp = TRY_UNWRAP(guild_pair_maybe,
            {
                PRINTF("    Error in guid_memberadded: guild not found.\n"_fmt);
                return;
            });

    // invitee accepted but they're offline or guidl_invite wasn't set
    if ((sd == NULL || !sd->guild_invite) && flag == 0)
    {
        PRINTF("    Error in guild_memberadded: invitee accepted but they're offline or not set to join this guild.\n"_fmt);
        // issue leave guild packet
        return;
    }

    // why was this above the next line?
    /*sd->guild_invite = GuildId();
    sd->guild_invite_account = AccountId();*/

    // the player that invited this one
    dumb_ptr<map_session_data> tsd = map_id2sd(account_to_block(sd->guild_invite_account));

    sd->guild_invite = GuildId();
    sd->guild_invite_account = AccountId();

    // if the invitee rejected
    if (flag == 1)
    {
        if (tsd != NULL)
        {
            // was sending 3 (full) before?
            clif_guild_inviteack(tsd, 1);
        }
    }

    sd->guild_sended = 0;
    sd->status.guild_id = guild_id;

    if (tsd != NULL)
    {
        // the invitee has accepted and joined
        clif_guild_inviteack(tsd, 2);
        clif_guild_memberlist(tsd, *gp.guild_most);
    }

    // implement later.. if something is wrong why isn't it caught by now?
    //clif_guild_checkconflict(sd);

    return;
}

/*========================================
 *
 *----------------------------------------
 */
void guild_invite(dumb_ptr<map_session_data> sd, AccountId account_id)
{
    dumb_ptr<map_session_data> tsd;
    Option<GuildPair> ogp = guild_search(sd->status.guild_id);

    GuildPair gp = TRY_UNWRAP(ogp, {
            PRINTF("Error: map server did not find the guild of the inviting player.\n"_fmt);
            return;
    });

    int i;
    // Can the inviting player invite players?
    bool can_invite = false;
    GmLevel is_gm = pc_isGM(sd);

    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].account_id == sd->status_key.account_id
            && (gp->member[i].position == 1
             || gp->member[i].position == 2
             || gp->member[i].position == 3
             || is_gm != GmLevel()))
        {
            can_invite = true;
            continue;
        }

        //clif_guild_inviteack(sd, 0);
        //return;
    }

    if (!can_invite)
    {
        clif_guild_inviteack(sd, 0);
        return;
    }

    // Set tsd to the invited players session.
    tsd = map_id2sd(account_to_block(account_id));

    if (   tsd == NULL          || !gp.guild_id
        || tsd->party_invite    || tsd->trade_partner
        || tsd->status.guild_id || tsd->guild_invite)
    {
        clif_guild_inviteack(sd, 0);
        return;
    }

    // Find out whether or not there is room in the guild for a new member.
    for (i = 0; i < MAX_GUILD; ++i)
        if(gp->member[i].account_id == wrap<AccountId>(0_u32))
            break;
    if(i == MAX_GUILD)
    {
        clif_guild_inviteack(sd, 3);
    }

    tsd->guild_invite = gp.guild_id;
    tsd->guild_invite_account = sd->status_key.account_id;

    clif_guild_invite(tsd, gp.guild_id, gp->name);
}

/*========================================
 *
 *----------------------------------------
 */
void guild_reply_invite(dumb_ptr<map_session_data> sd, GuildId guild_id, int flag)
{
    dumb_ptr<map_session_data> tsd;

    tsd = map_id2sd(account_to_block(sd->guild_invite_account));
    nullpo_retv(tsd);
    nullpo_retv(sd);

    // if the given guild id is not the guild we were previously invited to
    if (sd->guild_invite != guild_id)
    {
        return;
    }

    // the invitee accepted
    if (flag==1)
    {
        Option<GuildPair> ogp = guild_search (tsd->status.guild_id);

        // if the inviting party is not in a guild
        GuildPair gp = TRY_UNWRAP(ogp,
            {
                sd->guild_invite = GuildId();
                sd->guild_invite_account = AccountId();
                return;
            }
        );

        int i;

        // find the next empty guild member slot
        for (i = 0; i < MAX_GUILD; ++i)
        {
            if (!gp->member[i].account_id)
                break;
        }

        // if it is past the max guild limit
        if (i == MAX_GUILD)
        {
            sd->guild_invite = GuildId();
            sd->guild_invite_account = AccountId();
            clif_guild_inviteack(tsd, 3);
            return;
        }

        guild_makemember (sd, i, gp);
        intif_guild_addmember (sd->guild_invite, sd->status_key.account_id, i, sd->status.base_level, sd->status_key.name);
        return;
    }

    // the invitee refused to join
    else
    {
        sd->guild_invite = GuildId();
        sd->guild_invite_account = AccountId();
        if (tsd == NULL)
        {
            // the inviter is offline, so don't bother notifying them
            return;
        }
        clif_guild_inviteack(tsd, 1);
    }
    return;
}

/*========================================
 *
 *----------------------------------------
 */
void guild_leave(dumb_ptr<map_session_data> sd, GuildId guild_id, AccountId account_id, AString mes)
{
    Option<GuildPair> ogp = guild_search(guild_id);
    GuildPair gp = TRY_UNWRAP(ogp,
        {
            PRINTF("    could not find guild.\n"_fmt);
            return;
        }
    );

    // you can't make other people leave their guild, sorry.
    if (sd->status_key.account_id != account_id ||
        sd->status.guild_id != guild_id)
    {
        return;
    }

    int i;
    for (i = 0; i <= MAX_GUILD; ++i)
    {
        if (gp->member[i].account_id == sd->status_key.account_id)
        {
            intif_guild_leave(guild_id, account_id, 0, mes);
            return;
        }
    }

    return;
}

/*========================================
 * Process a request to expel a guild member.
 *----------------------------------------
 */
void guild_expulsion(dumb_ptr<map_session_data> sd, GuildId guild_id, AccountId account_id, /*CharId char_id,*/ XString reason)
{
    Option<GuildPair> ogp = guild_search(guild_id);
    GuildPair gp = TRY_UNWRAP(ogp,
        {
            PRINTF("    could not find guild.\n"_fmt);
            return;
        }
    );

    int i;

    // don't kick the guild leader.
    for (i = 0; i <= MAX_GUILD; i++)
    {
        if (gp->member[i].account_id == account_id &&
            gp->member[i].position == 1)
        {
            return;
        }
    }

    bool can_kick = false;
    for (i = 0; i <= MAX_GUILD; i++)
    {
        if (gp->member[i].account_id == sd->status_key.account_id &&
            (gp->member[i].position == 1 || gp->member[i].position == 2))
        {
            can_kick = true;
            break;
        }
    }
    if (!can_kick)
        return;

    intif_guild_expulsion(sd, guild_id, account_id, reason);

    return;
}

/*========================================
 * A guild member has been expelled.
 *----------------------------------------
 */
void guild_expelled(AccountId kicked_id, XString reason, AccountId kicker_id)
{
    dumb_ptr<map_session_data> sd;
    sd = map_id2sd(account_to_block(kicker_id));

    dumb_ptr<map_session_data> tsd;
    tsd = map_id2sd(account_to_block(kicked_id));

    Option<GuildPair> ogp = guild_search(sd->status.guild_id);
    GuildPair gp = TRY_UNWRAP(ogp,
        {
            return;
        }
    );

    CharName kicked_name = tsd->status_key.name;
    CharName kicker_name = sd->status_key.name;

    int i;
    for (i = 0; i < MAX_GUILD; i++)
    {
        if (gp->member[i].online)
        {
            tsd = map_id2sd(account_to_block(gp->member[i].account_id));

            clif_guild_expulsion(tsd, kicked_name, reason, kicker_name);
            guild_request_info(tsd->status.guild_id, tsd->status_key.account_id);
        }
    }

    tsd->status.guild_id = GuildId();

    for(i = 0; i<= MAX_GUILD; i++)
    {
        if (gp->member[i].account_id == kicked_id)
        {
            gp->member[i].account_id = AccountId();
            gp->member[i].name = CharName();
            gp->member[i].position = 0;
            gp->member[i].online = 0;
            gp->member[i].lv = 0;
            return;
        }
    }
}

/*========================================
 *
 *----------------------------------------
 */
void guild_member_left(GuildId guild_id, AccountId account_id, int flag, AString mes, CharName name)
{
    dumb_ptr<map_session_data> sd = map_id2sd(account_to_block(account_id));
    Option<GuildPair> maybe_guild_pair = guild_search(guild_id);

    GuildPair gp = TRY_UNWRAP(maybe_guild_pair,
        {
            return;
        }
    );

    int i;
    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].account_id == account_id)
        {
                if (flag == 0)
                {
                    clif_guild_leave(sd, name, mes);
                    sd->status.guild_id = wrap<GuildId>(0_u32);

                    gp->member[i].account_id = AccountId();
                    gp->member[i].name = CharName();
                    gp->member[i].position = 0;
                    gp->member[i].lv = 0;
                    gp->member[i].online = 0;
                    break;
                }
                else
                {
                    PRINTF("    Error guild_member_left: flag should be 0 (for now)\n"_fmt);
                    //clif_guild_expulsion(sd2, name, mes, account_id);
                    sd->status.guild_id = GuildId();

                    gp->member[i].account_id = AccountId();
                    gp->member[i].name = CharName();
                    gp->member[i].position = 0;
                    gp->member[i].lv = 0;
                    gp->member[i].online = 0;
                    break;
                }

                /*dumb_ptr<map_session_data> lsd = map_id2sd(account_to_block(gp->member[i].account_id));

                lsd->status.guild_id = GuildId();

                gp->member[i].account_id = AccountId();
                gp->member[i].name = CharName();
                gp->member[i].leader = 0;
                gp->member[i].lv = 0;
                gp->member[i].online = 0;
                continue;*/
        }
    }

    // check if the guild is empty. if so, delete it
    if (guild_check_empty(gp) == 1)
    {
        return;
    }

    dumb_ptr<map_session_data> tsd = nullptr;

    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].account_id)
        {
            tsd = map_id2sd(account_to_block(gp->member[i].account_id));
            clif_guild_leave(tsd, name, mes);
            clif_guild_memberlist(tsd, *gp);
        }
    }

    /*if (sd->state.storage_flag == 2)
     *      storage_guild_storageclose (sd);
     */
    //sd->status.guild_id = GuildId();
    sd->guild_sended = 0;

    return;
}

/*========================================
 *
 *----------------------------------------
 */
void guild_send_logout(dumb_ptr<map_session_data> sd)
{
    nullpo_retv(sd);
    if (sd->status.guild_id)
    {
        GuildPair g = TRY_UNWRAP(guild_search(sd->status.guild_id), return);
        {
            int i;
            for (i = 0; i < MAX_PARTY; i++)
                if (dumb_ptr<map_session_data>(g->member[i].sd) == sd)
                {
                    g->member[i].online = 0;
                    guild_recv_info(g);
                    g->member[i].sd = nullptr;
                }
        }
    }
    return;
}

/*========================================
 *
 *----------------------------------------
 */
void guild_send_message(dumb_ptr<map_session_data> sd, AString mbuf)
{
    nullpo_retv(sd);

    if (sd->status.guild_id == GuildId())
        return;
    intif_guild_message(sd->status.guild_id, sd->status_key.account_id, mbuf);

    return;
}

/*========================================
 *
 *----------------------------------------
 */
void guild_recv_message(GuildId guild_id, XString mes)
{
    GuildPair g = TRY_UNWRAP(guild_search(guild_id), return);
    clif_guild_message(g, mes);
}

/*========================================
 *
 *----------------------------------------
 */
void guild_changememberpos(GuildId guild_id, AccountId acct_id, int position)
{
    Option<GuildPair> ogp = guild_search(guild_id);

    GuildPair gp = TRY_UNWRAP(ogp,
            {
                PRINTF("Error: guild_changememberos -- guild not found\n"_fmt);
                return;
            });

    int i;

    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].sd == nullptr)
        {
            continue;
        }
        if (gp->member[i].sd->status_key.account_id == acct_id
         && gp->member[i].position != 1)
        {
            gp->member[i].position = position;
            break;
        }
    }

    guild_recv_info(gp);

    return;
}

/*========================================
 *
 *----------------------------------------
 */
void guild_changememberpos_request(dumb_ptr<map_session_data> sd, AccountId acct_id, int position)
{
    nullpo_retv(sd);

    Option<GuildPair> ogp = guild_search(sd->status.guild_id);

    GuildPair gp = TRY_UNWRAP(ogp,
            {
                PRINTF("Error: guild_changememberpos_request -- could not find guild\n"_fmt);
                return;
            });

    int member_key = (MAX_GUILD + 1);
    int i;

    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].sd == nullptr)
        {
            continue;
        }
        if (gp->member[i].sd->status_key.account_id == acct_id
         && gp->member[i].position != 1)
        {
            member_key = i;
            break;
        }
    }

    if (member_key == (MAX_GUILD + 1))
        return;

    GmLevel is_gm = pc_isGM(sd);
    for (i = 0; i < MAX_GUILD; ++i)
    {
        if (gp->member[i].sd == nullptr)
        {
            continue;
        }
        if ((gp->member[i].sd->status_key.account_id == sd->status_key.account_id
            && ((gp->member[i].position == 1 || gp->member[i].position == 2)))
              || is_gm != GmLevel())
        {
            intif_guild_poschanged(sd->status.guild_id, acct_id, position);
            break;
        }
    }

    return;
}

} // namespace map
} // namespace tmwa
