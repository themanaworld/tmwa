//
#ifndef TMW_HPP
#define TMW_HPP

#include "map.hpp"

int  tmw_CheckChatSpam (struct map_session_data *sd, const char *message);
int  tmw_ShorterStrlen (const char *s1, const char *s2);
int  tmw_CheckChatLameness (struct map_session_data *sd, const char *message);
__attribute__((format(printf, 1, 2)))
void tmw_GmHackMsg (const char *fmt, ...);
void tmw_AutoBan (struct map_session_data *sd, const char *reason, int length);
void tmw_TrimStr (char *str);

#endif /* TMW_H_ */
