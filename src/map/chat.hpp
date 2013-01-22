#ifndef CHAT_HPP
#define CHAT_HPP

int chat_leavechat(struct map_session_data *);

int chat_createnpcchat(struct npc_data *nd, int limit, int pub, int trigger,
        const char *title, int titlelen, const char *ev);
int chat_deletenpcchat(struct npc_data *nd);
int chat_enableevent(struct chat_data *cd);
int chat_disableevent(struct chat_data *cd);

#endif // CHAT_HPP
