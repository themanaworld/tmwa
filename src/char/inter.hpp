#ifndef INTER_HPP
#define INTER_HPP

#include "../strings/fwd.hpp"

void inter_init(ZString file);
void inter_save(void);
int inter_parse_frommap(int fd);

int inter_check_length(int fd, int length);

#define inter_cfgName "conf/inter_athena.conf"

extern int party_share_level;

#endif // INTER_HPP
