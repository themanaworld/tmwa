#ifndef TMWA_IO_FD_HPP
#define TMWA_IO_FD_HPP
//    io/fd.hpp - typesafe (but not scopesafe) file descriptors
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

# include <sys/select.h>
# include <sys/socket.h>

# include <unistd.h>

# include "../strings/fwd.hpp"


namespace io
{
    class FD
    {
    private:
        int fd;

        explicit
        FD(int f)
        : fd(f)
        {}
    public:
        FD()
        : fd(-1)
        {}

        int uncast_dammit() { return fd; }
        static
        FD cast_dammit(int f) { return FD(f); }

        static
        FD stdin() { return FD(0); }
        static
        FD stdout() { return FD(1); }
        static
        FD stderr() { return FD(2); }
        static
        FD open(ZString path, int flags, int mode=0666);
        static
        FD socket(int domain, int type, int protocol);
        FD accept(struct sockaddr *addr, socklen_t *addrlen);
        static
        int pipe(FD& r, FD& w);
        static
        int pipe2(FD& r, FD& w, int flags);
        static
        FD sysconf_SC_OPEN_MAX();

        FD next() { return FD(fd + 1); }
        FD prev() { return FD(fd - 1); }

        ssize_t read(void *buf, size_t count);
        ssize_t write(const void *buf, size_t count);
        ssize_t pread(void *buf, size_t count, off_t offset);
        ssize_t pwrite(const void *buf, size_t count, off_t offset);
        ssize_t readv(const struct iovec *iov, int iovcnt);
        ssize_t writev(const struct iovec *iov, int iovcnt);
        ssize_t preadv(const struct iovec *iov, int iovcnt, off_t offset);
        ssize_t pwritev(const struct iovec *iov, int iovcnt, off_t offset);

        int close();
        int shutdown(int);
        int getsockopt(int level, int optname, void *optval, socklen_t *optlen);
        int setsockopt(int level, int optname, const void *optval, socklen_t optlen);
        // ...
        int fcntl(int cmd);
        int fcntl(int cmd, int arg);
        int fcntl(int cmd, void *arg);
        int listen(int backlog);
        int bind(const struct sockaddr *addr, socklen_t addrlen);
        int connect(const struct sockaddr *addr, socklen_t addrlen);
        FD dup();
        FD dup2(FD newfd);
        FD dup3(FD newfd, int flags);


        friend
        bool operator == (FD l, FD r)
        {
            return l.fd == r.fd;
        }
        friend
        bool operator != (FD l, FD r)
        {
            return l.fd != r.fd;
        }
        friend
        bool operator < (FD l, FD r)
        {
            return l.fd < r.fd;
        }
        friend
        bool operator <= (FD l, FD r)
        {
            return l.fd <= r.fd;
        }
        friend
        bool operator > (FD l, FD r)
        {
            return l.fd > r.fd;
        }
        friend
        bool operator >= (FD l, FD r)
        {
            return l.fd >= r.fd;
        }
    };

    class FD_Set
    {
    private:
        fd_set fds;
    public:
        FD_Set()
        {
            FD_ZERO(&fds);
        }
        void clr(FD fd)
        {
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
            FD_CLR(fd.uncast_dammit(), &fds);
# pragma GCC diagnostic pop
        }
        bool isset(FD fd)
        {
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
            return FD_ISSET(fd.uncast_dammit(), &fds);
# pragma GCC diagnostic pop
        }
        void set(FD fd)
        {
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wold-style-cast"
            FD_SET(fd.uncast_dammit(), &fds);
# pragma GCC diagnostic pop
        }

        static
        int select(int nfds, FD_Set *readfds, FD_Set *writefds, FD_Set *exceptfds, struct timeval *timeout);
        static
        int pselect(int nfds, FD_Set *readfds, FD_Set *writefds, FD_Set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);
    };
} // namespace io

#endif // TMWA_IO_FD_HPP
