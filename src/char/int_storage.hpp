#ifndef INT_STORAGE_HPP
#define INT_STORAGE_HPP

#include "../strings/fwd.hpp"

int inter_storage_init(void);
int inter_storage_save(void);
void inter_storage_delete(int account_id);
struct storage *account2storage(int account_id);

int inter_storage_parse_frommap(int fd);

extern FString storage_txt;

#endif // INT_STORAGE_HPP
