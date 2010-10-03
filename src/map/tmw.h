//
#ifndef _TMW_H_
#define _TMW_H_

#include "map.h"

int  tmw_CheckChatSpam (struct map_session_data *sd, char *message);
int  tmw_ShorterStrlen (char *s1, char *s2);
int  tmw_CheckChatLameness (struct map_session_data *sd, char *message);
void tmw_GmHackMsg (const char *fmt, ...);
void tmw_AutoBan (struct map_session_data *sd, char *reason, int length);
void tmw_TrimStr (char *str);

#endif /* _TMW_H_ */
