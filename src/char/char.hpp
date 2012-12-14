// $Id: char.h,v 1.1.1.1 2004/09/10 17:26:50 MagicalTux Exp $
#ifndef CHAR_HPP
#define CHAR_HPP

#define MAX_MAP_SERVERS 30

#define CHAR_CONF_NAME  "conf/char_athena.conf"

#define LOGIN_LAN_CONF_NAME     "conf/lan_support.conf"

#define DEFAULT_AUTOSAVE_INTERVAL 300*1000

struct mmo_map_server
{
    long ip;
    short port;
    int users;
    char map[MAX_MAP_PER_SERVER][16];
};

int search_character_index(const char *character_name);
char *search_character_name(int index);

int mapif_sendall(const uint8_t *buf, unsigned int len);
int mapif_sendallwos(int fd, const uint8_t *buf, unsigned int len);
int mapif_send(int fd, const uint8_t *buf, unsigned int len);

__attribute__((format(printf, 1, 2)))
int char_log(const char *fmt, ...);

extern int autosave_interval;

#endif
