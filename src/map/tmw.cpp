#include "tmw.hpp"

#include <cctype>
#include <cstring>

#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../common/cxxstdio.hpp"
#include "../common/nullpo.hpp"

#include "atcommand.hpp"
#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "map.hpp"
#include "pc.hpp"

#include "../poison.hpp"

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

        tmw_AutoBan(sd, "chat", battle_config.chat_spam_ban);

        return 1;
    }

    if (battle_config.chat_spam_ban &&
        (sd->chat_lines_in >= battle_config.chat_spam_warn
         || sd->chat_total_repeats >= battle_config.chat_spam_warn))
    {
        clif_displaymessage(sd->fd, "WARNING: You are about to be automatically banned for spam!");
        clif_displaymessage(sd->fd, "WARNING: Please slow down, do not repeat, and do not SHOUT!");
    }

    return 0;
}

void tmw_AutoBan(dumb_ptr<map_session_data> sd, ZString reason, int length)
{
    if (length == 0 || sd->auto_ban_info.in_progress)
        return;

    sd->auto_ban_info.in_progress = 1;

    FString hack_msg = STRPRINTF("[GM] %s has been autobanned for %s spam",
            sd->status.name,
            reason);
    tmw_GmHackMsg(hack_msg);

    FString fake_command = STRPRINTF("@autoban %s %dh (%s spam)",
            sd->status.name, length, reason);
    log_atcommand(sd, fake_command);

    FString anotherbuf = STRPRINTF("You have been banned for %s spamming. Please do not spam.",
            reason);

    clif_displaymessage(sd->fd, anotherbuf);
    /* type: 2 - ban(year, month, day, hour, minute, second) */
    HumanTimeDiff ban_len {};
    ban_len.hour = length;
    chrif_char_ask_name(-1, sd->status.name, 2, ban_len);
    clif_setwaitclose(sd->fd);
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
    intif_wis_message_to_gm(wisp_server_name,
            battle_config.hack_info_GM_level,
            line);
}
