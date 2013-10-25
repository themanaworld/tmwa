#ifndef CHAR_HPP
#define CHAR_HPP

#include "../strings/fwd.hpp"

#include "../common/const_array.hpp"
#include "../common/ip.hpp"
#include "../common/mmo.hpp"

constexpr int MAX_MAP_SERVERS = 30;

#define CHAR_CONF_NAME  "conf/char_athena.conf"

#define LOGIN_LAN_CONF_NAME     "conf/lan_support.conf"

struct mmo_map_server
{
    IP4Address ip;
    short port;
    int users;
    MapName maps[MAX_MAP_PER_SERVER];
};

const mmo_charstatus *search_character(CharName character_name);

int mapif_sendall(const uint8_t *buf, unsigned int len);
int mapif_sendallwos(int fd, const uint8_t *buf, unsigned int len);
int mapif_send(int fd, const uint8_t *buf, unsigned int len);

void char_log(XString line);

#define CHAR_LOG(fmt, ...)  \
    char_log(STRPRINTF(fmt, ## __VA_ARGS__))

#endif // CHAR_HPP
