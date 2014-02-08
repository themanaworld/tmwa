#ifndef TMWA_IO_TTY_HPP
#define TMWA_IO_TTY_HPP
//    io/tty.hpp - terminal escape sequences
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

# include "../sanity.hpp"


# define SGR_BLACK "\e[30m"
# define SGR_RED "\e[31m"
# define SGR_GREEN "\e[32m"
# define SGR_YELLOW "\e[33m"
# define SGR_BLUE "\e[34m"
# define SGR_MAGENTA "\e[35m"
# define SGR_CYAN "\e[36m"
# define SGR_WHITE "\e[37m"

# define SGR_BOLD "\e[1m"

# define SGR_RESET "\e[0m"

#endif //TMWA_IO_TTY_HPP
