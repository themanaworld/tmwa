#ifndef INTER_HPP
#define INTER_HPP

# include "../strings/fwd.hpp"

bool inter_config(XString key, ZString value);
void inter_init2();
void inter_save(void);
int inter_parse_frommap(int fd);

int inter_check_length(int fd, int length);

extern int party_share_level;

#endif // INTER_HPP
