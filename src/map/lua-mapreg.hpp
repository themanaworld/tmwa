#pragma once
//    lua-mapreg.hpp - persistent server-wide variables ($ mapreg).
//
//    Copyright © 2026 The Mana World Development Team
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

// Storage interns the full old-format name ("NAME" int, "NAME$" string) so
// mapreg.txt round-trips byte-identically ("name[,idx]\tvalue" lines). The
// world / world.str / world.array proxies (lua-libs.hpp,
// lua_register_lib_world) sit on top of these accessors.

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../strings/rstring.hpp"
#include "../strings/xstring.hpp"


namespace tmwa
{
namespace map
{
// Load mapreg_txt and start the 10 s autosave timer (was do_init_script).
void mapreg_init();
// Save if dirty (shutdown, before lua_final).
void mapreg_final();

// name is the old spelling without the leading '$': trailing '$' selects the
// string namespace. idx must be 0..255 (callers validate; out of range is a
// programming error here).
int lua_mapreg_get_int(XString name, int idx);
RString lua_mapreg_get_str(XString name, int idx);
// writing 0 / "" deletes the entry, as the old engine did
void lua_mapreg_set_int(XString name, int idx, int val);
void lua_mapreg_set_str(XString name, int idx, XString val);
} // namespace map
} // namespace tmwa
