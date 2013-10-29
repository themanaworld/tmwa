#ifndef TMWA_IO_READ_HPP
#define TMWA_IO_READ_HPP
//    io/read.hpp - Input from files.
//
//    Copyright © 2013 Ben Longbons <b.r.longbons@gmail.com>
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

#include "../common/sanity.hpp"

#include "../strings/fwd.hpp"


namespace io
{
    class ReadFile
    {
    private:
        int fd;
        unsigned short start, end;
        char buf[4096];
    public:
        explicit
        ReadFile(int fd);
        explicit
        ReadFile(ZString name);
        ReadFile(ReadFile&&) = delete;
        ~ReadFile();

        bool get(char&);
        size_t get(char *buf, size_t len);
        bool getline(FString&);

        bool is_open();
    };
} // namespace io

#endif //TMWA_IO_READ_HPP
