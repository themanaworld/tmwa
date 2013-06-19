#ifndef TMW_HPP
#define TMW_HPP

#include "../common/const_array.hpp"
#include "../common/dumb_ptr.hpp"

#include "map.hpp"

int tmw_CheckChatSpam(dumb_ptr<map_session_data> sd, const char *message);
void tmw_GmHackMsg(const char *line);
void tmw_TrimStr(char *str);

#endif // TMW_HPP
