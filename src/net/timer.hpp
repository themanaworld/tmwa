#pragma once
//    timer.hpp - Future event scheduler.
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

#include "timer.t.hpp"

#include "fwd.hpp"

#include "../strings/fwd.hpp"


namespace tmwa
{
// updated automatically when using milli_clock::now()
// which is done only by core.cpp
extern tick_t gettick_cache;

inline
tick_t gettick(void)
{
    return gettick_cache;
}

/// Do all timers scheduled before tick, and return the number of
/// milliseconds until the next timer happens
interval_t do_timer(tick_t tick);

/// Stat a file, and return its modification time, truncated to seconds.
tick_t file_modified(ZString name);

/// Check if there are any events at all scheduled.
bool has_timers();
} // namespace tmwa
