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

int tmw_CheckChatSpam(struct map_session_data *sd, char* message) {
	nullpo_retr(1, sd);
	time_t now = time(NULL);

	if (pc_isGM(sd)) return 0;

	if (now > sd->chat_reset_due) {
		sd->chat_reset_due = now + battle_config.chat_spam_threshold;
		sd->chat_lines_in = 0;
	}

	sd->chat_lines_in++;

	if (message) {
		// Penalty for repeating
		if (strncmp(sd->chat_lastmsg, message, battle_config.chat_maxline) == 0)
			sd->chat_lines_in += battle_config.chat_lame_penalty;

		// Penalty for lame, it can stack on top of the repeat penalty.
		if (tmw_CheckChatLameness(sd, message))
			sd->chat_lines_in += battle_config.chat_lame_penalty;

		strncpy((char*)sd->chat_lastmsg, message, battle_config.chat_maxline);
	}
	else {
		// No message means we're checking another type of spam.
		// Most other types are pretty lame..
		sd->chat_lines_in += battle_config.chat_lame_penalty;
	}

	if (sd->chat_lines_in >= battle_config.chat_spam_flood) {
		sd->chat_lines_in = 0;
		tmw_GmHackMsg("Spam detected from character '%s' (account: %d)", sd->status.name, sd->status.account_id);

		if (battle_config.chat_spam_ban > 0) {
			clif_displaymessage(sd->fd, "You have been banned for spamming. Please do not spam.");
			tmw_GmHackMsg("This player has been banned for %d hour(s).", battle_config.chat_spam_ban);

			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, battle_config.chat_spam_ban, 0, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			clif_setwaitclose(sd->fd);
		}
	}

	if (battle_config.chat_spam_ban && sd->chat_lines_in >= battle_config.chat_spam_warn) {
		clif_displaymessage(sd->fd, "WARNING : You are about to be automaticly banned for spam!");
		clif_displaymessage(sd->fd, "WARNING : Please slow down, do not repeat, and do not SHOUT!");
	}

	return 0;
}

// Returns true if more than 50% of input message is caps or punctuation
int tmw_CheckChatLameness(struct map_session_data *sd, char *message)
{
	int count, lame;

	// Ignore the name
	message += strlen(sd->status.name);

	for(count = lame = 0; *message; message++,count++)
		if (isupper(*message) || ispunct(*message))
			lame++;

	if (count > 7 && lame > count / 2)
		return(1);

	return(0);
}

// Sends a whisper to all GMs
void tmw_GmHackMsg(const char *fmt, ...) {
	char buf[512];
	va_list ap;

	buf[512] = 0;

	va_start(ap, fmt);
	vsnprintf(buf, 511, fmt, ap);
	va_end(ap);

	intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, buf, strlen(buf) + 1);
}
