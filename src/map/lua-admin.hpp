#pragma once
//    lua-admin.hpp - @setvar/@getvar/@luastats backends.
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

#include "fwd.hpp"

#include "lua-compat.hpp"

#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"

#include "../generic/dumb_ptr.hpp"


namespace tmwa
{
namespace map
{
// varname keeps the old GM-command spelling: "X" (char), "#X" / "##X"
// (account), "$X" / "$X$" (mapreg, idx for arrays), "@X" / "@X$" (the
// player's p.tmp / p.tmpstr tables). The old "." NPC scope is dropped (D5).
// target may be null for the "$" scopes. On success *out_msg is the feedback
// line for the GM; on failure it is the error text and false is returned.
bool lua_admin_setvar(dumb_ptr<map_session_data> target, ZString varname,
        int idx, ZString value, AString* out_msg);
bool lua_admin_getvar(dumb_ptr<map_session_data> target, ZString varname,
        int idx, AString* out_msg);

// @luastats: memory usage, live callback refs, budget aborts.
AString lua_admin_stats();
} // namespace map
} // namespace tmwa
