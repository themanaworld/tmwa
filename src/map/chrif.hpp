#ifndef CHRIF_HPP
#define CHRIF_HPP

#include "../common/dumb_ptr.hpp"
#include "../common/human_time_diff.hpp"
#include "../common/ip.hpp"

#include "map.hpp"

void chrif_setuserid(AccountName);
void chrif_setpasswd(AccountPass);
AccountPass chrif_getpasswd(void);

void chrif_setip(IP4Address);
void chrif_setport(int);

int chrif_isconnect(void);

int chrif_authreq(dumb_ptr<map_session_data>);
int chrif_save(dumb_ptr<map_session_data>);
int chrif_charselectreq(dumb_ptr<map_session_data>);

int chrif_changemapserver(dumb_ptr<map_session_data> sd,
        MapName name, int x, int y,
        IP4Address ip, short port);

void chrif_changegm(int id, ZString pass);
void chrif_changeemail(int id, AccountEmail actual_email, AccountEmail new_email);
void chrif_char_ask_name(int id, CharName character_name, short operation_type,
        HumanTimeDiff modif);
int chrif_saveaccountreg2(dumb_ptr<map_session_data> sd);
int chrif_reloadGMdb(void);
int chrif_send_divorce(int char_id);

int do_init_chrif (void);

// only used by intif.cpp
extern int char_fd;

#endif // CHRIF_HPP
