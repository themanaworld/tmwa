#pragma once
//    core.hpp - The main loop.
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

#include "../range/slice.hpp"


namespace tmwa
{
/// core.c contains a server-independent main() function
/// and then runs a do_sendrecv loop

/// When this is cleared, the server exits gracefully.
extern volatile bool runflag;

/// This is an external function defined by each server
/// This function must register stuff for the parse loop
extern int do_init(Slice<ZString>);

/// Cleanup function called whenever a signal kills us
/// or when if we manage to exit() gracefully.
extern void term_func(void);
} // namespace tmwa

/// grumble grumble stupid intertwined includes mumble mumble
__attribute__((warn_unused_result))
extern int tmwa_main(int argc, char **argv);
