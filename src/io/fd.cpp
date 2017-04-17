#include "fd.hpp"
//    io/fd.cpp - typesafe (but not scopesafe) file descriptors
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
#include <unistd.h>

#include "../strings/zstring.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace io
{
    FD FD::open(ZString path, int flags, int mode)
    {
        return FD(::open(path.c_str(), flags, mode));
    }
    FD FD::openat(FD dirfd, ZString path, int flags, int mode)
    {
        return FD(::openat(dirfd.fd, path.c_str(), flags, mode));
    }
    FD FD::socket(int domain, int type, int protocol)
    {
        return FD(::socket(domain, type, protocol));
    }
    FD FD::accept(struct sockaddr *addr, socklen_t *addrlen)
    {
        return FD(::accept(fd, addr, addrlen));
    }
    int FD::pipe(FD& r, FD& w)
    {
        int tmp[2] = {-1, -1};
        int rv = ::pipe(tmp);
        r = FD(tmp[0]);
        w = FD(tmp[1]);
        return rv;
    }
    int FD::pipe2(FD& r, FD& w, int flags)
    {
        int tmp[2] = {-1, -1};
        int rv = ::pipe2(tmp, flags);
        r = FD(tmp[0]);
        w = FD(tmp[1]);
        return rv;
    }
    FD FD::sysconf_SC_OPEN_MAX()
    {
        return FD(::sysconf(_SC_OPEN_MAX));
    }

    ssize_t FD::read(void *buf, size_t count)
    {
        return ::read(fd, buf, count);
    }
    ssize_t FD::write(const void *buf, size_t count)
    {
        return ::write(fd, buf, count);
    }
    ssize_t FD::send(const void *buf, size_t count, int flags)
    {
        return ::send(fd, buf, count, flags);
    }
    ssize_t FD::sendmsg(const struct msghdr *msg, int flags)
    {
        return ::sendmsg(fd, msg, flags);
    }
    int FD::sendmmsg(struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags)
    {
		#if defined (__FreeBSD__)
		return 0; // Not implemented on FreeBSD
		#else
        return ::sendmmsg(fd, msgvec, vlen, flags);
		#endif
    }
    ssize_t FD::sendto(const void *buf, size_t count, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen)
    {
        return ::sendto(fd, buf, count, flags, dest_addr, addrlen);
    }
    ssize_t FD::recv(void *buf, size_t count, int flags)
    {
        return ::recv(fd, buf, count, flags);
    }
    ssize_t FD::recvfrom(void *buf, size_t count, int flags,
             struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return ::recvfrom(fd, buf, count, flags, src_addr, addrlen);
    }
    ssize_t FD::recvmsg(struct msghdr *msg, int flags)
    {
        return ::recvmsg(fd, msg, flags);
    }
    ssize_t FD::pread(void *buf, size_t count, off_t offset)
    {
        return ::pread(fd, buf, count, offset);
    }
    ssize_t FD::pwrite(const void *buf, size_t count, off_t offset)
    {
        return ::pwrite(fd, buf, count, offset);
    }
    ssize_t FD::readv(const struct iovec *iov, int iovcnt)
    {
        return ::readv(fd, iov, iovcnt);
    }
    ssize_t FD::writev(const struct iovec *iov, int iovcnt)
    {
        return ::writev(fd, iov, iovcnt);
    }
    ssize_t FD::preadv(const struct iovec *iov, int iovcnt, off_t offset)
    {
        return ::preadv(fd, iov, iovcnt, offset);
    }
    ssize_t FD::pwritev(const struct iovec *iov, int iovcnt, off_t offset)
    {
        return ::pwritev(fd, iov, iovcnt, offset);
    }

    int FD::close()
    {
        if (fd == -1)
        {
            errno = EBADF;
            return -1;
        }
        return ::close(fd);
    }
    int FD::shutdown(int how)
    {
        return ::shutdown(fd, how);
    }
    int FD::getsockopt(int level, int optname, void *optval, socklen_t *optlen)
    {
        return ::getsockopt(fd, level, optname, optval, optlen);
    }
    int FD::setsockopt(int level, int optname, const void *optval, socklen_t optlen)
    {
        return ::setsockopt(fd, level, optname, optval, optlen);
    }
    int FD::fcntl(int cmd)
    {
        return ::fcntl(fd, cmd);
    }
    int FD::fcntl(int cmd, int arg)
    {
        return ::fcntl(fd, cmd, arg);
    }
    int FD::fcntl(int cmd, void *arg)
    {
        return ::fcntl(fd, cmd, arg);
    }
    int FD::listen(int backlog)
    {
        return ::listen(fd, backlog);
    }
    int FD::bind(const struct sockaddr *addr, socklen_t addrlen)
    {
        return ::bind(fd, addr, addrlen);
    }
    int FD::connect(const struct sockaddr *addr, socklen_t addrlen)
    {
        return ::connect(fd, addr, addrlen);
    }
    FD FD::dup()
    {
        return FD(::dup(fd));
    }
    FD FD::dup2(FD newfd)
    {
        return FD(::dup2(fd, newfd.fd));
    }
    FD FD::dup3(FD newfd, int flags)
    {
        return FD(::dup3(fd, newfd.fd, flags));
    }

    int FD_Set::select(int nfds, FD_Set *readfds, FD_Set *writefds, FD_Set *exceptfds, struct timeval *timeout)
    {
        return ::select(nfds,
                readfds ? &readfds->fds : nullptr,
                writefds ? &writefds->fds : nullptr,
                exceptfds ? &exceptfds->fds : nullptr,
                timeout);
    }
    int FD_Set::pselect(int nfds, FD_Set *readfds, FD_Set *writefds, FD_Set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask)
    {
        return ::pselect(nfds,
                readfds ? &readfds->fds : nullptr,
                writefds ? &writefds->fds : nullptr,
                exceptfds ? &exceptfds->fds : nullptr,
                timeout, sigmask);
    }
} // namespace io
} // namespace tmwa
