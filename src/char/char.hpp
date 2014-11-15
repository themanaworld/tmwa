#pragma once
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

#include "fwd.hpp"

#include "../generic/array.hpp"

#include "../net/ip.hpp"

#include "../high/mmo.hpp"


namespace tmwa
{
constexpr int MAX_MAP_SERVERS = 30;
constexpr
std::chrono::seconds DEFAULT_AUTOSAVE_INTERVAL = 5_min;


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

auto iter_map_sessions() -> decltype(filter_iterator<Session *>(std::declval<Array<Session *, MAX_MAP_SERVERS> *>()));

void char_log(XString line);

#define CHAR_LOG(fmt, ...)  \
    char_log(STRPRINTF(fmt, ## __VA_ARGS__))
} // namespace tmwa
