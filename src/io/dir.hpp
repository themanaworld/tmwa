#pragma once
//    io/dir.hpp - rooted file operations
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

#include "fwd.hpp"

#include "fd.hpp"


namespace tmwa
{
namespace io
{
    class DirFd
    {
    private:
        FD dirfd;

    public:
        DirFd();
        explicit
        DirFd(ZString);
        DirFd(const DirFd&, ZString);

        DirFd(const DirFd&) = delete;
        ~DirFd();
        DirFd& operator = (const DirFd&) = delete;

        FD open_fd(ZString name, int flags, int mode=FD::DEFAULT_MODE) const;
    };
} // namespace io
} // namespace tmwa
