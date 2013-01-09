#ifndef NPC_HPP
#define NPC_HPP

#include "../common/timer.hpp"

#define START_NPC_NUM 110000000

#define WARP_CLASS 45
#define WARP_DEBUG_CLASS 722
#define INVISIBLE_CLASS 32767

int npc_event_dequeue(struct map_session_data *sd);
int npc_event(struct map_session_data *sd, const char *npcname, int);
int npc_timer_event(const char *eventname);   // Added by RoVeRT
int npc_command(struct map_session_data *sd, const char *npcname, const char *command);
int npc_touch_areanpc(struct map_session_data *, int, int, int);
int npc_click(struct map_session_data *, int);
int npc_scriptcont(struct map_session_data *, int);
int npc_buysellsel(struct map_session_data *, int, int);
int npc_buylist(struct map_session_data *, int, const uint16_t *);
int npc_selllist(struct map_session_data *, int, const uint16_t *);
int npc_parse_warp(const char *w1, const char *w2, const char *w3, const char *w4);

int npc_enable(const char *name, int flag);
struct npc_data *npc_name2id(const char *name);

int npc_get_new_npc_id(void);

/**
 * Spawns and installs a talk-only NPC
 *
 * \param message The message to speak.  If message is NULL, the NPC will not do anything at all.
 */
struct npc_data *npc_spawn_text(int m, int x, int y, int class_, const char *name, const char *message);    // message is strdup'd within

/**
 * Uninstalls and frees an NPC
 */
void npc_free(struct npc_data *npc);

void npc_addsrcfile(const char *);
void npc_delsrcfile(const char *);
int do_init_npc(void);
int npc_event_do_oninit(void);
int npc_do_ontimer(int, struct map_session_data *, int);

struct argrec;
int npc_event_doall_l(const char *name, int rid, int argc,
                        struct argrec *argv);
int npc_event_do_l(const char *name, int rid, int argc,
                     struct argrec *argv);
#define npc_event_doall(name) npc_event_doall_l(name, 0, 0, NULL)
#define npc_event_do(name) npc_event_do_l(name, 0, 0, NULL)

int npc_timerevent_start(struct npc_data *nd);
int npc_timerevent_stop(struct npc_data *nd);
int npc_gettimerevent_tick(struct npc_data *nd);
int npc_settimerevent_tick(struct npc_data *nd, int newtimer);
int npc_delete(struct npc_data *nd);

#endif // NPC_HPP
