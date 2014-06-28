#pragma once
//    magic.hpp - Entry to the magic system.
//
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

#include "../strings/fwd.hpp"

#include "../generic/fwd.hpp"

#include "map.t.hpp"
#include "skill.t.hpp"


namespace tmwa
{
/**
 * Try to cast magic.
 *
 * As an intended side effect, the magic message may be distorted (text only).
 * No, it can't. Thank God.
 *
 * \param caster Player attempting to cast magic
 * \param source_invocation The prospective incantation
 * \return 1 or -1 if the input message was magic and was handled by this function, 0 otherwise.  -1 is returned when the
 *         message should not be repeated.
 */
int magic_message(dumb_ptr<map_session_data> caster, XString source_invocation);
} // namespace tmwa
