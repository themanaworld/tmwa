//    Copyright 2026  Thorbjørn Lindeijer
//
//    This file is part of TMWA.
//
//    TMWA is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    TMWA is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with TMWA.  If not, see <http://www.gnu.org/licenses/>.

// Weak fallback definitions for the per-server hooks referenced by the
// shared objects. Each daemon provides its own strong definitions, so
// these only satisfy non-server binaries (like tests) that link
// tmwa-shared without needing the hooks.

#include "../strings/zstring.hpp"
#include "../high/core.hpp"

namespace tmwa
{

__attribute__((weak))
int do_init(Slice<ZString>)
{
    return 0;
}

__attribute__((weak))
void term_func(void)
{
}

} // namespace tmwa
