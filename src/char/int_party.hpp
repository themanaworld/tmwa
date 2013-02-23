#ifndef INT_PARTY_HPP
#define INT_PARTY_HPP

int inter_party_init(void);
int inter_party_save(void);

int inter_party_parse_frommap(int fd);

void inter_party_leave(int party_id, int account_id);

extern char party_txt[1024];

#endif // INT_PARTY_HPP
