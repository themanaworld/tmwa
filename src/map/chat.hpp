// $Id: chat.h,v 1.3 2004/09/25 05:32:18 MouseJstr Exp $
#ifndef CHAT_HPP
#define CHAT_HPP

#include "map.hpp"

int  chat_createchat (struct map_session_data *, int, int, const char *, const char *,
                      int);
int  chat_joinchat (struct map_session_data *, int, const char *);
int  chat_leavechat (struct map_session_data *);
int  chat_changechatowner (struct map_session_data *, const char *);
int  chat_changechatstatus (struct map_session_data *, int, int, const char *,
                            const char *, int);
int  chat_kickchat (struct map_session_data *, const char *);

int  chat_createnpcchat (struct npc_data *nd, int limit, int pub, int trigger,
                         const char *title, int titlelen, const char *ev);
int  chat_deletenpcchat (struct npc_data *nd);
int  chat_enableevent (struct chat_data *cd);
int  chat_disableevent (struct chat_data *cd);
int  chat_npckickall (struct chat_data *cd);

int  do_final_chat (void);

#endif
