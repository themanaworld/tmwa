#ifndef TMW_HPP
#define TMW_HPP

#include "../common/const_array.hpp"

int tmw_CheckChatSpam(struct map_session_data *sd, const char *message);
void tmw_GmHackMsg(const_string line);
void tmw_TrimStr(char *str);

#endif // TMW_HPP
