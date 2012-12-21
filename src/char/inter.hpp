#ifndef INTER_HPP
#define INTER_HPP

int inter_init(const char *file);
int inter_save(void);
int inter_parse_frommap(int fd);

int inter_check_length(int fd, int length);

#define inter_cfgName "conf/inter_athena.conf"

extern int party_share_level;
extern char inter_log_filename[1024];

#endif // INTER_HPP
