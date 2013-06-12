#ifndef NPC_HPP
#define NPC_HPP

#include <cstddef>
#include <cstdint>

#include "../common/timer.t.hpp"

#include "map.hpp"

constexpr int START_NPC_NUM = 110000000;

constexpr int WARP_CLASS = 45;
constexpr int WARP_DEBUG_CLASS = 722;
constexpr int INVISIBLE_CLASS = 32767;

int npc_event_dequeue(dumb_ptr<map_session_data> sd);
int npc_event(dumb_ptr<map_session_data> sd, const char *npcname, int);
int npc_timer_event(const char *eventname);   // Added by RoVeRT
int npc_command(dumb_ptr<map_session_data> sd, const char *npcname, const char *command);
int npc_touch_areanpc(dumb_ptr<map_session_data>, map_local *, int, int);
int npc_click(dumb_ptr<map_session_data>, int);
int npc_scriptcont(dumb_ptr<map_session_data>, int);
int npc_buysellsel(dumb_ptr<map_session_data>, int, int);
int npc_buylist(dumb_ptr<map_session_data>, int, const uint16_t *);
int npc_selllist(dumb_ptr<map_session_data>, int, const uint16_t *);
int npc_parse_warp(const char *w1, const char *w2, const char *w3, const char *w4);

int npc_enable(const char *name, bool flag);
dumb_ptr<npc_data> npc_name2id(const char *name);

int npc_get_new_npc_id(void);

/**
 * Spawns and installs a talk-only NPC
 *
 * \param message The message to speak.  If message is NULL, the NPC will not do anything at all.
 */
dumb_ptr<npc_data> npc_spawn_text(map_local *m, int x, int y,
        int class_, const char *name, const char *message);    // message is strdup'd within

/**
 * Uninstalls and frees an NPC
 */
void npc_free(dumb_ptr<npc_data> npc);

void npc_addsrcfile(const char *);
void npc_delsrcfile(const char *);
int do_init_npc(void);
int npc_event_do_oninit(void);

struct argrec;
int npc_event_doall_l(const char *name, int rid,
        int argc, struct argrec *argv);
int npc_event_do_l(const char *name, int rid,
        int argc, struct argrec *argv);
inline
int npc_event_doall(const char *name)
{
    return npc_event_doall_l(name, 0, 0, NULL);
}
inline
int npc_event_do(const char *name)
{
    return npc_event_do_l(name, 0, 0, NULL);
}

void npc_timerevent_start(dumb_ptr<npc_data_script> nd);
void npc_timerevent_stop(dumb_ptr<npc_data_script> nd);
interval_t npc_gettimerevent_tick(dumb_ptr<npc_data_script> nd);
void npc_settimerevent_tick(dumb_ptr<npc_data_script> nd, interval_t newtimer);
int npc_delete(dumb_ptr<npc_data> nd);

#endif // NPC_HPP
