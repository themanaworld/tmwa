#ifndef CHAR_HPP
#define CHAR_HPP

# include "../strings/fwd.hpp"

# include "../common/const_array.hpp"
# include "../common/ip.hpp"
# include "../common/mmo.hpp"

class Session;

constexpr int MAX_MAP_SERVERS = 30;

struct mmo_map_server
{
    IP4Address ip;
    short port;
    int users;
    MapName maps[MAX_MAP_PER_SERVER];
};

const CharPair *search_character(CharName character_name);
const CharPair *search_character_id(int char_id);
Session *server_for(const CharPair *mcs);

int mapif_sendall(const uint8_t *buf, unsigned int len);
int mapif_sendallwos(Session *s, const uint8_t *buf, unsigned int len);
int mapif_send(Session *s, const uint8_t *buf, unsigned int len);

void char_log(XString line);

# define CHAR_LOG(fmt, ...) \
    char_log(STRPRINTF(fmt, ## __VA_ARGS__))

#endif // CHAR_HPP
