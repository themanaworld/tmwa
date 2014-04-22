#ifndef TMWA_CHAR_CHAR_HPP
#define TMWA_CHAR_CHAR_HPP
//    char.hpp - Character server.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

# include "fwd.hpp"

# include "../strings/fwd.hpp"

# include "../mmo/ip.hpp"
# include "../mmo/mmo.hpp"

struct Session;

constexpr int MAX_MAP_SERVERS = 30;

struct mmo_map_server
{
    IP4Address ip;
    short port;
    int users;
    Array<MapName, MAX_MAP_PER_SERVER> maps;
};

const CharPair *search_character(CharName character_name);
const CharPair *search_character_id(CharId char_id);
Session *server_for(const CharPair *mcs);

int mapif_sendall(const uint8_t *buf, unsigned int len);
int mapif_sendallwos(Session *s, const uint8_t *buf, unsigned int len);
int mapif_send(Session *s, const uint8_t *buf, unsigned int len);

void char_log(XString line);

# define CHAR_LOG(fmt, ...) \
    char_log(STRPRINTF(fmt, ## __VA_ARGS__))

#endif // TMWA_CHAR_CHAR_HPP
