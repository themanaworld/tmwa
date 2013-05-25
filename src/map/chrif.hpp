#ifndef CHRIF_HPP
#define CHRIF_HPP

#include "../common/dumb_ptr.hpp"

#include "map.hpp"

void chrif_setuserid(const char *);
void chrif_setpasswd(const char *);
char *chrif_getpasswd(void);

void chrif_setip(const char *);
void chrif_setport(int);

int chrif_isconnect(void);

int chrif_authreq(dumb_ptr<map_session_data>);
int chrif_save(dumb_ptr<map_session_data>);
int chrif_charselectreq(dumb_ptr<map_session_data>);

int chrif_changemapserver(dumb_ptr<map_session_data> sd,
        char *name, int x, int y,
        struct in_addr ip, short port);

int chrif_searchcharid(int char_id);
int chrif_changegm(int id, const char *pass, int len);
int chrif_changeemail(int id, const char *actual_email,
        const char *new_email);
int chrif_char_ask_name(int id, char *character_name, short operation_type,
        int year, int month, int day, int hour, int minute, int second);
int chrif_saveaccountreg2(dumb_ptr<map_session_data> sd);
int chrif_reloadGMdb(void);
int chrif_send_divorce(int char_id);

int do_init_chrif (void);

// only used by intif.cpp
extern int char_fd;

#endif // CHRIF_HPP
