#include "tmw.hpp"

#include <cctype>
#include <cstring>

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
void tmw_AutoBan(dumb_ptr<map_session_data> sd, const char *reason, int length);
static
int tmw_CheckChatLameness(dumb_ptr<map_session_data> sd, const char *message);
static
int tmw_ShorterStrlen(const char *s1, const char *s2);


int tmw_CheckChatSpam(dumb_ptr<map_session_data> sd, const char *message)
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
    if (strncmp(sd->chat_lastmsg, message,
         tmw_ShorterStrlen(sd->chat_lastmsg, message)) == 0)
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

    strzcpy(sd->chat_lastmsg, message, battle_config.chat_maxline);

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

void tmw_AutoBan(dumb_ptr<map_session_data> sd, const char *reason, int length)
{
    if (length == 0 || sd->auto_ban_info.in_progress)
        return;

    sd->auto_ban_info.in_progress = 1;

    std::string hack_msg = STRPRINTF("[GM] %s has been autobanned for %s spam",
            sd->status.name,
            reason);
    tmw_GmHackMsg(hack_msg.c_str());

    std::string fake_command = STRPRINTF("@autoban %s %dh (%s spam)",
            sd->status.name, length, reason);
    log_atcommand(sd, fake_command);

    std::string anotherbuf = STRPRINTF("You have been banned for %s spamming. Please do not spam.",
            reason);

    clif_displaymessage(sd->fd, anotherbuf);
    /* type: 2 - ban(year, month, day, hour, minute, second) */
    chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, length, 0, 0);
    clif_setwaitclose(sd->fd);
}

// Compares the length of two strings and returns that of the shorter
int tmw_ShorterStrlen(const char *s1, const char *s2)
{
    int s1_len = strlen(s1);
    int s2_len = strlen(s2);
    return (s2_len >= s1_len ? s1_len : s2_len);
}

// Returns true if more than 50% of input message is caps or punctuation
int tmw_CheckChatLameness(dumb_ptr<map_session_data>, const char *message)
{
    int count, lame;

    for (count = lame = 0; *message; message++, count++)
        if (isupper(*message) || ispunct(*message))
            lame++;

    if (count > 7 && lame > count / 2)
        return (1);

    return (0);
}

// Sends a whisper to all GMs
void tmw_GmHackMsg(const char *line)
{
    intif_wis_message_to_gm(wisp_server_name,
                             battle_config.hack_info_GM_level,
                             line);
}

/* Remove leading and trailing spaces from a string, modifying in place. */
void tmw_TrimStr(char *const ob)
{
    char *const oe = ob + strlen(ob);
    char *nb = ob;
    while (*nb && isspace(*nb))
        nb++;
    char *ne = oe;
    while (ne != nb && isspace(ne[-1]))
        ne--;
    // not like memcpy - allowed to overlap one way
    char *zb = std::copy(nb, ne, ob);
    std::fill(zb, oe, '\0');
}
