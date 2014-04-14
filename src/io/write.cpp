#include "write.hpp"
//    io/write.cpp - Output to files
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

#include <sys/uio.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>

#include "../strings/xstring.hpp"

#include "../poison.hpp"


namespace io
{
    WriteFile::WriteFile(FD f, bool linebuffered)
    : fd(f), lb(linebuffered), buflen(0)
    {}
    WriteFile::WriteFile(ZString name, bool linebuffered)
    : fd(FD::open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666)), lb(linebuffered), buflen(0)
    {}
    WriteFile::~WriteFile()
    {
        if (fd != FD())
        {
            if (!close())
            {
                abort();
            }
        }
    }

    void WriteFile::put(char c)
    {
        really_put(&c, 1);
    }
    void WriteFile::really_put(const char *dat, size_t len)
    {
        if (len + buflen <= sizeof(buf))
        {
            std::copy(dat, dat + len, buf + buflen);
            buflen += len;
            goto maybe_linebuffered;
        }

        {
            size_t offset = 0;
            while (offset < buflen)
            {
                // iovec is used for both reading and writing,
                // so it declares the pointer to non-const.
                iovec iov[2] =
                {
                    {buf + offset, buflen - offset},
                    {const_cast<char *>(dat), len},
                };
                ssize_t rv = fd.writev(iov, 2);
                if (rv <= 0)
                    goto write_fail;
                offset += rv;
            }
            offset -= buflen;
            dat += offset;
            len -= offset;
        }

        while (len > sizeof(buf))
        {
            ssize_t rv = fd.write(dat, len);
            if (rv <= 0)
                goto write_fail;
            dat += rv;
            len -= rv;
        }
        buflen = len;

    maybe_linebuffered:
        if (lb)
        {
            auto rbegin = std::reverse_iterator<char *>(buf + buflen);
            auto rend = std::reverse_iterator<char *>(buf);
            auto last_nl = std::find(rbegin, rend, '\n');

            if (last_nl != rend)
            {
                size_t offset = 0;
                size_t remaining = rend - last_nl;
                while (remaining)
                {
                    ssize_t rv = fd.write(buf + offset, remaining);
                    if (rv <= 0)
                        goto write_fail;
                    offset += rv;
                    remaining -= rv;
                }
                std::copy(buf + offset, buf + buflen, buf);
                buflen -= offset;
            }
        }
        return;

    write_fail:
        fd.close();
        fd = FD();
        return;
    }

    void WriteFile::put_line(XString xs)
    {
        really_put(xs.data(), xs.size());
        if (!xs.endswith('\n'))
            put('\n');
    }

    bool WriteFile::close()
    {
        size_t off = 0;
        while (off < buflen)
        {
            ssize_t rv = fd.write(buf + off, buflen - off);
            if (rv <= 0)
            {
                fd.close();
                fd = FD();
                return false;
            }
            off += rv;
        }

        FD f = fd;
        fd = FD();
        return f.close() == 0;
    }

    bool WriteFile::is_open()
    {
        return fd != FD();
    }

    AppendFile::AppendFile(ZString name, bool linebuffered)
    : WriteFile(FD::open(name, O_WRONLY | O_CREAT | O_APPEND, 0666), linebuffered)
    {}

    int do_vprint(WriteFile& out, const char *fmt, va_list ap)
    {
        int len;
        {
            va_list ap2;
            va_copy(ap2, ap);
            len = vsnprintf(nullptr, 0, fmt, ap2);
            va_end(ap2);
        }
        char buffer[len + 1];
        vsnprintf(buffer, len + 1, fmt, ap);

        out.really_put(buffer, len);
        return len;
    }
} // namespace io
