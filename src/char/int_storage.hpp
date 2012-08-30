// $Id: int_storage.h,v 1.1.1.1 2004/09/10 17:26:51 MagicalTux Exp $
#ifndef INT_STORAGE_HPP
#define INT_STORAGE_HPP

int  inter_storage_init (void);
void inter_storage_final (void);
int  inter_storage_save (void);
int  inter_guild_storage_save (void);
int  inter_storage_delete (int account_id);
int  inter_guild_storage_delete (int guild_id);
struct storage *account2storage (int account_id);

int  inter_storage_parse_frommap (int fd);

extern char storage_txt[1024];
extern char guild_storage_txt[1024];

#endif
