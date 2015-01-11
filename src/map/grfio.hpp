#pragma once
//    grfio.hpp - Don't read GRF files, just name-map .wlk files.
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

#include <cstdint>

#include <vector>


namespace tmwa
{
namespace map
{
bool load_resnametable(ZString filename);

/// Load a resource into memory, subject to data/resnametable.txt.
/// Normally, resourcename is xxx-y.gat and the file is xxx-y.wlk.
/// Currently there is exactly one .wlk per .gat, but multiples are fine.
std::vector<uint8_t> grfio_reads(MapName resourcename);
} // namespace map
} // namespace tmwa
