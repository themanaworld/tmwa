//

#include "map.h"

int tmw_CheckChatSpam(struct map_session_data *sd, char* message);
int tmw_CheckChatLameness(struct map_session_data *sd, char *message);
void tmw_GmHackMsg(const char *fmt, ...);
int tmw_CheckTradeSpam(struct map_session_data *sd);
