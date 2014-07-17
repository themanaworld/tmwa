#include "dir.hpp"
//    io/dir.cpp - rooted file operations
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

#include <fcntl.h>

#include "../strings/zstring.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace io
{
    DirFd::DirFd()
    : dirfd(FD::cast_dammit(AT_FDCWD))
    {}

    DirFd::DirFd(ZString path)
    : dirfd(FD::open(path, O_DIRECTORY | O_RDONLY, 0))
    {}

    DirFd::DirFd(const DirFd& root, ZString path)
    : dirfd(FD::openat(root.dirfd, path, O_DIRECTORY | O_RDONLY, 0))
    {}

    DirFd::~DirFd()
    {
        dirfd.close();
    }

    FD DirFd::open_fd(ZString name, int flags, int mode) const
    {
        return FD::openat(dirfd, name, flags, mode);
    }
} // namespace io
} // namespace tmwa
