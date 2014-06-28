#ifndef TMWA_TESTS_FDHACK_HPP
#define TMWA_TESTS_FDHACK_HPP
//    fdhack.hpp - Move file descriptors around.
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

# include "fwd.hpp"

# include <cstddef>
# include <fcntl.h>

# include <stdexcept>

# include "../strings/literal.hpp"
# include "../strings/zstring.hpp"

# include "../io/fd.hpp"

namespace tmwa
{
class ReplaceFd
{
    io::FD number, backup;
public:
    ReplaceFd(io::FD num, io::FD handle, bool owned)
    : number(handle.dup2(num)), backup(num.dup())
    {
        if (owned)
            handle.close();
    }
    ~ReplaceFd()
    {
        backup.dup2(number);
        backup.close();
    }
};
class QuietFd : ReplaceFd
{
public:
    QuietFd(io::FD num=io::FD::stderr())
    : ReplaceFd(num, io::FD::open("/dev/null"_s, O_RDONLY), true)
    {}
};
} // namespace tmwa

#endif // TMWA_TESTS_FDHACK_HPP
