//

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
#include "tmw.h"
#include "trade.h"

#include <string.h>

int tmw_CheckChatSpam(struct map_session_data *sd, char* message) {
	unsigned int tick,elapsed = 0;
	nullpo_retr(1, sd);

	if (pc_isGM(sd)) return 0;

	tick = gettick();
	elapsed = tick - sd->chat_lastmsg_time;
	sd->chat_lastmsg_time = tick;

	if (elapsed < battle_config.spam_time)
		sd->chat_threshold++;

	sd->chat_threshold -= (int)(elapsed / (battle_config.spam_time/2));

	if (sd->chat_threshold < 0)
		sd->chat_threshold = 0;

	if (strncmp(sd->chat_lastmsg, message, battle_config.chat_maxline) == 0)
		sd->chat_repeatmsg++;
	else
		sd->chat_repeatmsg--;

	if (sd->chat_repeatmsg < 0)
		sd->chat_repeatmsg = 0;

	strncpy((char*)sd->chat_lastmsg, message, battle_config.chat_maxline);

	if (sd->chat_threshold > battle_config.spam_threshold || sd->chat_repeatmsg > battle_config.spam_threshold) {
		sprintf(message, "Spam detected from character '%s' (account: %d), threshold was exceeded.", sd->status.name, sd->status.account_id);
		printf("%s\n", message);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message, strlen(message) + 1);

		if (battle_config.spam_ban > 0)
			sprintf(message, "This player has been banned for %d hours(s).", battle_config.spam_ban);
		else
			sprintf(message, "This player hasn't been banned (Ban option is disabled).");
		printf("%s\n", message);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message, strlen(message) + 1);

		if (battle_config.spam_ban > 0)
		{
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, battle_config.spam_ban, 0, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			return 2; // forced to disconnect
		}
		else
			return 1; // just ignore, dont ban.
	}

	if (strlen(message) >= battle_config.chat_maxline)
		return 1; // ignore lines exceeding the max length in config.

	return 0;
}