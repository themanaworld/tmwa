#ifndef TMWA_IO_WRITE_HPP
#define TMWA_IO_WRITE_HPP
//    io/write.hpp - Output to files.
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

# include "../sanity.hpp"

# include <cstdarg>

# include "../strings/fwd.hpp"

# include "fd.hpp"

namespace io
{
    class WriteFile
    {
    private:
        FD fd;
        bool lb;
        unsigned short buflen;
        char buf[4096];
    public:
        explicit
        WriteFile(FD fd, bool linebuffered=false);
        explicit
        WriteFile(ZString name, bool linebuffered=false);
        WriteFile(WriteFile&&) = delete;
        WriteFile& operator = (WriteFile&&) = delete;
        ~WriteFile();

        void put(char);
        void really_put(const char *dat, size_t len);
        void put_line(XString);

        __attribute__((warn_unused_result))
        bool close();
        bool is_open();
    };

    class AppendFile : public WriteFile
    {
    public:
        explicit
        AppendFile(ZString name, bool linebuffered=false);
    };

    __attribute__((format(printf, 2, 0)))
    int do_vprint(WriteFile& out, const char *fmt, va_list ap);
} // namespace io

#endif // TMWA_IO_WRITE_HPP
