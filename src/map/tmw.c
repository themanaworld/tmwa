//

#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "tmw.h"

#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "version.h"
#include "nullpo.h"

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
#include "trade.h"
    
int tmw_CheckChatSpam (struct map_session_data *sd, char *message)
{
    nullpo_retr (1, sd);
    time_t now = time (NULL);

    if (pc_isGM (sd))
        return 0;

    if (now > sd->chat_reset_due)
    {
        sd->chat_reset_due = now + battle_config.chat_spam_threshold;
        sd->chat_lines_in = 0;
    }

    if (now > sd->chat_repeat_reset_due)
    {
        sd->chat_repeat_reset_due =
            now + (battle_config.chat_spam_threshold * 60);
        sd->chat_total_repeats = 0;
    }

    sd->chat_lines_in++;

    // Penalty for repeats.
    if (strncmp
        (sd->chat_lastmsg, message,
         tmw_ShorterStrlen (sd->chat_lastmsg, message)) == 0)
    {
        sd->chat_lines_in += battle_config.chat_lame_penalty;
        sd->chat_total_repeats++;
    }
    else
    {
        sd->chat_total_repeats = 0;
    }

    // Penalty for lame, it can stack on top of the repeat penalty.
    if (tmw_CheckChatLameness (sd, message))
        sd->chat_lines_in += battle_config.chat_lame_penalty;

    strncpy ((char *) sd->chat_lastmsg, message, battle_config.chat_maxline);

    if (sd->chat_lines_in >= battle_config.chat_spam_flood
        || sd->chat_total_repeats >= battle_config.chat_spam_flood)
    {
        sd->chat_lines_in = sd->chat_total_repeats = 0;

        tmw_AutoBan (sd, "chat", battle_config.chat_spam_ban);

        return 1;
    }

    if (battle_config.chat_spam_ban &&
        (sd->chat_lines_in >= battle_config.chat_spam_warn
         || sd->chat_total_repeats >= battle_config.chat_spam_warn))
    {
        /* "WARNING: You are about to be automatically banned for spam!" */
        clif_displaymessage (sd->fd, msg_txt (506));
        /* "WARNING: Please slow down, do not repeat, and do not SHOUT!" */
        clif_displaymessage (sd->fd, msg_txt (507));
    }

    return 0;
}

void tmw_AutoBan(struct map_session_data *sd, char *reason, int length)
{
    char anotherbuf[512];

    if (length == 0 || sd->auto_ban_info.in_progress)
        return;

    sd->auto_ban_info.in_progress = 1;

    tmw_GmHackMsg ("%s has been autobanned for %s spam",
                   sd->status.name, reason);

    gm_log ("%s(%d,%d) Server : @autoban %s %dh (%s spam)",
            map[sd->bl.m].name, sd->bl.x, sd->bl.y,
            sd->status.name, length, reason);

    /* "You have been banned for %s spamming. Please do not spam." */
    snprintf (anotherbuf, 511, msg_txt (508), reason);

    clif_displaymessage (sd->fd, anotherbuf);
    /* type: 2 - ban (year, month, day, hour, minute, second) */
    chrif_char_ask_name (-1, sd->status.name, 2, 0, 0, 0, length, 0, 0);
    clif_setwaitclose (sd->fd);
}

// Compares the length of two strings and returns that of the shorter
int tmw_ShorterStrlen (char *s1, char *s2)
{
    int  s1_len = strlen (s1);
    int  s2_len = strlen (s2);
    return (s2_len >= s1_len ? s1_len : s2_len);
}

// Returns true if more than 50% of input message is caps or punctuation
int tmw_CheckChatLameness (struct map_session_data *sd, char *message)
{
    int  count, lame;

    for (count = lame = 0; *message; message++, count++)
        if (isupper (*message) || ispunct (*message))
            lame++;

    if (count > 7 && lame > count / 2)
        return (1);

    return (0);
}

// Sends a whisper to all GMs
void tmw_GmHackMsg (const char *fmt, ...)
{
    char buf[512];
    va_list ap;

    va_start (ap, fmt);
    vsnprintf (buf, 511, fmt, ap);
    va_end (ap);

    char outbuf[512 + 5];
    strcpy (outbuf, "[GM] ");
    strcat (outbuf, buf);

    intif_wis_message_to_gm (wisp_server_name,
                             battle_config.hack_info_GM_level, outbuf,
                             strlen (outbuf) + 1);
}

/* Remove leading and trailing spaces from a string, modifying in place. */
void tmw_TrimStr (char *str)
{
    char *l;
    char *a;
    char *e;

    if (!*str)
        return;

    e = str + strlen (str) - 1;

    /* Skip all leading spaces. */
    for (l = str; *l && isspace (*l); ++l)
        ;

    /* Find the end of the string, or the start of trailing spaces. */
    for (a = e; *a && a > l && isspace (*a); --a)
        ;

    memmove (str, l, a - l + 1);
    str[a - l + 1] = '\0';
}
