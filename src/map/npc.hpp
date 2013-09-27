#ifndef NPC_HPP
#define NPC_HPP

#include <cstddef>
#include <cstdint>

#include "../strings/fwd.hpp"

#include "../common/timer.t.hpp"

#include "map.hpp"

constexpr int START_NPC_NUM = 110000000;

constexpr int WARP_CLASS = 45;
constexpr int WARP_DEBUG_CLASS = 722;
constexpr int INVISIBLE_CLASS = 32767;

int npc_event_dequeue(dumb_ptr<map_session_data> sd);
int npc_event(dumb_ptr<map_session_data> sd, NpcEvent npcname, int);
void npc_timer_event(NpcEvent eventname);   // Added by RoVeRT
int npc_command(dumb_ptr<map_session_data> sd, NpcName npcname, XString command);
int npc_touch_areanpc(dumb_ptr<map_session_data>, map_local *, int, int);
int npc_click(dumb_ptr<map_session_data>, int);
int npc_scriptcont(dumb_ptr<map_session_data>, int);
int npc_buysellsel(dumb_ptr<map_session_data>, int, int);
int npc_buylist(dumb_ptr<map_session_data>, int, const uint16_t *);
int npc_selllist(dumb_ptr<map_session_data>, int, const uint16_t *);
int npc_parse_warp(XString w1, XString, NpcName w3, XString w4);

int npc_enable(NpcName name, bool flag);
dumb_ptr<npc_data> npc_name2id(NpcName name);

int npc_get_new_npc_id(void);

/**
 * Spawns and installs a talk-only NPC
 *
 * \param message The message to speak.  If message is NULL, the NPC will not do anything at all.
 */
dumb_ptr<npc_data> npc_spawn_text(map_local *m, int x, int y,
        int class_, NpcName name, FString message);

/**
 * Uninstalls and frees an NPC
 */
void npc_free(dumb_ptr<npc_data> npc);

void npc_addsrcfile(FString);
void npc_delsrcfile(FString);
int do_init_npc(void);
int npc_event_do_oninit(void);

int npc_event_doall_l(ScriptLabel name, int rid, int argc, struct argrec_t *argv);
int npc_event_do_l(NpcEvent name, int rid, int argc, struct argrec_t *argv);
inline
int npc_event_doall(ScriptLabel name)
{
    return npc_event_doall_l(name, 0, 0, NULL);
}
inline
int npc_event_do(NpcEvent name)
{
    return npc_event_do_l(name, 0, 0, NULL);
}

void npc_timerevent_start(dumb_ptr<npc_data_script> nd);
void npc_timerevent_stop(dumb_ptr<npc_data_script> nd);
interval_t npc_gettimerevent_tick(dumb_ptr<npc_data_script> nd);
void npc_settimerevent_tick(dumb_ptr<npc_data_script> nd, interval_t newtimer);
int npc_delete(dumb_ptr<npc_data> nd);

#endif // NPC_HPP
