#ifndef TMWA_MAP_MAGIC_V2_HPP
#define TMWA_MAP_MAGIC_V2_HPP
//    magic-v2.hpp - second generation magic parser
//
//    Copyright © 2014 Ben Longbons <b.r.longbons@gmail.com>
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

# include "../strings/zstring.hpp"

bool magic_init0();
// must be called after itemdb initialization
bool load_magic_file_v2(ZString filename);

#endif // TMWA_MAP_MAGIC_V2_HPP
