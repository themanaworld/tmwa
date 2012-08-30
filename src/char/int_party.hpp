// $Id: int_party.h,v 1.1.1.1 2004/09/10 17:26:51 MagicalTux Exp $
#ifndef INT_PARTY_HPP
#define INT_PARTY_HPP

int  inter_party_init (void);
int  inter_party_save (void);

int  inter_party_parse_frommap (int fd);

int  inter_party_leave (int party_id, int account_id);

extern char party_txt[1024];

#endif
