#include "tmw.hpp"
//    tmw.cpp - Some random functions added by TMW.
//
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

#include "../compat/nullpo.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/literal.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/human_time_diff.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "map.hpp"
#include "pc.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
void tmw_AutoBan(dumb_ptr<map_session_data> sd, ZString reason, int length);
static
bool tmw_CheckChatLameness(dumb_ptr<map_session_data> sd, XString message);


int tmw_CheckChatSpam(dumb_ptr<map_session_data> sd, XString message)
{
    nullpo_retr(1, sd);
    TimeT now = TimeT::now();

    if (pc_isGM(sd))
        return 0;

    if (now > sd->chat_reset_due)
    {
        sd->chat_reset_due = static_cast<time_t>(now) + battle_config.chat_spam_threshold;
        sd->chat_lines_in = 0;
    }

    if (now > sd->chat_repeat_reset_due)
    {
        sd->chat_repeat_reset_due =
            static_cast<time_t>(now) + (battle_config.chat_spam_threshold * 60);
        sd->chat_total_repeats = 0;
    }

    sd->chat_lines_in++;

    // Penalty for repeats.
    if (sd->chat_lastmsg.startswith(message) || message.startswith(sd->chat_lastmsg))
    {
        sd->chat_lines_in += battle_config.chat_lame_penalty;
        sd->chat_total_repeats++;
    }
    else
    {
        sd->chat_total_repeats = 0;
    }

    // Penalty for lame, it can stack on top of the repeat penalty.
    if (tmw_CheckChatLameness(sd, message))
        sd->chat_lines_in += battle_config.chat_lame_penalty;

    sd->chat_lastmsg = message;

    if (sd->chat_lines_in >= battle_config.chat_spam_flood
        || sd->chat_total_repeats >= battle_config.chat_spam_flood)
    {
        sd->chat_lines_in = sd->chat_total_repeats = 0;

        tmw_AutoBan(sd, "chat"_s, battle_config.chat_spam_ban);

        return 1;
    }

    if (battle_config.chat_spam_ban &&
        (sd->chat_lines_in >= battle_config.chat_spam_warn
         || sd->chat_total_repeats >= battle_config.chat_spam_warn))
    {
        clif_displaymessage(sd->sess, "WARNING: You are about to be automatically banned for spam!"_s);
        clif_displaymessage(sd->sess, "WARNING: Please slow down, do not repeat, and do not SHOUT!"_s);
    }

    return 0;
}

void tmw_AutoBan(dumb_ptr<map_session_data> sd, ZString reason, int length)
{
    if (length == 0 || sd->auto_ban_info.in_progress)
        return;

    sd->auto_ban_info.in_progress = 1;

    AString hack_msg = STRPRINTF("[GM] %s has been autobanned for %s spam"_fmt,
            sd->status_key.name,
            reason);
    tmw_GmHackMsg(hack_msg);

    AString fake_command = STRPRINTF("@autoban %s %dh (%s spam)"_fmt,
            sd->status_key.name, length, reason);
    log_atcommand(sd, fake_command);

    AString anotherbuf = STRPRINTF("You have been banned for %s spamming. Please do not spam."_fmt,
            reason);

    clif_displaymessage(sd->sess, anotherbuf);
    /* type: 2 - ban(year, month, day, hour, minute, second) */
    HumanTimeDiff ban_len {};
    ban_len.hour = length;
    chrif_char_ask_name(AccountId(), sd->status_key.name, 2, ban_len);
    clif_setwaitclose(sd->sess);
}

// Returns true if more than 50% of input message is caps or punctuation
bool tmw_CheckChatLameness(dumb_ptr<map_session_data>, XString message)
{
    int lame = 0;

    for (char c : message)
    {
        if (c <= ' ')
            continue;
        if (c > '~')
            continue;
        if ('0' <= c && c <= '9')
            continue;
        if ('a' <= c && c <= 'z')
            continue;
        lame++;
    }

    return message.size() > 7 && lame > message.size() / 2;
}

// Sends a whisper to all GMs
void tmw_GmHackMsg(ZString line)
{
    intif_wis_message_to_gm(WISP_SERVER_NAME,
            GmLevel::from(static_cast<uint32_t>(battle_config.hack_info_GM_level)),
            line);
}
} // namespace tmwa
