#ifndef TMW_HPP
#define TMW_HPP

#include "map.hpp"

int tmw_CheckChatSpam(struct map_session_data *sd, const char *message);
__attribute__((format(printf, 1, 2)))
void tmw_GmHackMsg(const char *fmt, ...);
void tmw_TrimStr(char *str);

#endif // TMW_HPP
