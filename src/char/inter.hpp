#ifndef TMWA_CHAR_INTER_HPP
#define TMWA_CHAR_INTER_HPP

# include "../strings/fwd.hpp"

class Session;

bool inter_config(XString key, ZString value);
void inter_init2();
void inter_save(void);
int inter_parse_frommap(Session *ms);

int inter_check_length(Session *ms, int length);

extern int party_share_level;

#endif // TMWA_CHAR_INTER_HPP
