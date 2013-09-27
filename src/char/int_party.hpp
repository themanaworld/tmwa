#ifndef INT_PARTY_HPP
#define INT_PARTY_HPP

#include "../strings/fwd.hpp"

int inter_party_init(void);
int inter_party_save(void);

int inter_party_parse_frommap(int fd);

void inter_party_leave(int party_id, int account_id);

extern FString party_txt;

#endif // INT_PARTY_HPP
