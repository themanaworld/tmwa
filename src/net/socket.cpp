#include "socket.hpp"
//    socket.cpp - Network event system.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 MadCamel
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

#include <netinet/tcp.h>
#include <sys/socket.h>

#include <fcntl.h>

#include <cstdlib>

#include <array>

#include "../compat/memory.hpp"

#include "../io/cxxstdio.hpp"

#include "timer.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
io::FD_Set readfds;
static
int fd_max;

static
const uint32_t RFIFO_SIZE = 65536;
static
const uint32_t WFIFO_SIZE = 65536;

DIAG_PUSH();
DIAG_I(old_style_cast);
static
std::array<std::unique_ptr<Session>, FD_SETSIZE> session;
DIAG_POP();

Session::Session(SessionIO io, SessionParsers p)
: created()
, connected()
, eof()
, timed_close()
, rdata(), wdata()
, max_rdata(), max_wdata()
, rdata_size(), wdata_size()
, rdata_pos()
, client_ip()
, func_recv()
, func_send()
, func_parse()
, func_delete()
, for_inferior()
, session_data()
, fd()
{
    set_io(io);
    set_parsers(p);
}
void Session::set_io(SessionIO io)
{
    func_send = io.func_send;
    func_recv = io.func_recv;
}
void Session::set_parsers(SessionParsers p)
{
    func_parse = p.func_parse;
    func_delete = p.func_delete;
}


void set_session(io::FD fd, std::unique_ptr<Session> sess)
{
    int f = fd.uncast_dammit();
    assert (0 <= f && f < FD_SETSIZE);
    session[f] = std::move(sess);
}
Session *get_session(io::FD fd)
{
    int f = fd.uncast_dammit();
    if (0 <= f && f < FD_SETSIZE)
        return session[f].get();
    return nullptr;
}
void reset_session(io::FD fd)
{
    int f = fd.uncast_dammit();
    assert (0 <= f && f < FD_SETSIZE);
    session[f] = nullptr;
}
int get_fd_max() { return fd_max; }
IteratorPair<ValueIterator<io::FD, IncrFD>> iter_fds()
{
    return {io::FD::cast_dammit(0), io::FD::cast_dammit(fd_max)};
}

/// clean up by discarding handled bytes
inline
void RFIFOFLUSH(Session *s)
{
    really_memmove(&s->rdata[0], &s->rdata[s->rdata_pos], s->rdata_size - s->rdata_pos);
    s->rdata_size -= s->rdata_pos;
    s->rdata_pos = 0;
}

/// how much room there is to read more data
inline
size_t RFIFOSPACE(Session *s)
{
    return s->max_rdata - s->rdata_size;
}


/// Read from socket to the queue
static
void recv_to_fifo(Session *s)
{
    ssize_t len = s->fd.read(&s->rdata[s->rdata_size],
                        RFIFOSPACE(s));

    if (len > 0)
    {
        s->rdata_size += len;
        s->connected = 1;
    }
    else
    {
        s->set_eof();
    }
}

static
void send_from_fifo(Session *s)
{
    ssize_t len = s->fd.write(&s->wdata[0], s->wdata_size);

    if (len > 0)
    {
        s->wdata_size -= len;
        if (s->wdata_size)
        {
            really_memmove(&s->wdata[0], &s->wdata[len],
                     s->wdata_size);
        }
        s->connected = 1;
    }
    else
    {
        s->set_eof();
    }
}

static
void nothing_delete(Session *s)
{
    (void)s;
}

static
void connect_client(Session *ls)
{
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);

    io::FD fd = ls->fd.accept(reinterpret_cast<struct sockaddr *>(&client_address), &len);
    if (fd == io::FD())
    {
        perror("accept");
        return;
    }
    if (fd.uncast_dammit() >= SOFT_LIMIT)
    {
        FPRINTF(stderr, "softlimit reached, disconnecting : %d\n"_fmt, fd.uncast_dammit());
        fd.shutdown(SHUT_RDWR);
        fd.close();
        return;
    }
    if (fd_max <= fd.uncast_dammit())
    {
        fd_max = fd.uncast_dammit() + 1;
    }

    const int yes = 1;
    /// Allow to bind() again after the server restarts.
    // Since the socket is still in the TIME_WAIT, there's a possibility
    // that formerly lost packets might be delivered and confuse the server.
    fd.setsockopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    /// Send packets as soon as possible
    /// even if the kernel thinks there is too little for it to be worth it!
    /// Testing shows this is indeed a good idea.
    fd.setsockopt(IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

    // Linux-ism: Set socket options to optimize for thin streams
    // See http://lwn.net/Articles/308919/ and
    // Documentation/networking/tcp-thin.txt .. Kernel 3.2+
#ifdef TCP_THIN_LINEAR_TIMEOUTS
    fd.setsockopt(IPPROTO_TCP, TCP_THIN_LINEAR_TIMEOUTS, &yes, sizeof yes);
#endif
#ifdef TCP_THIN_DUPACK
    fd.setsockopt(IPPROTO_TCP, TCP_THIN_DUPACK, &yes, sizeof yes);
#endif

    readfds.set(fd);

    fd.fcntl(F_SETFL, O_NONBLOCK);

    set_session(fd, make_unique<Session>(
                SessionIO{.func_recv= recv_to_fifo, .func_send= send_from_fifo},
                ls->for_inferior));
    Session *s = get_session(fd);
    s->fd = fd;
    s->rdata.new_(RFIFO_SIZE);
    s->wdata.new_(WFIFO_SIZE);
    s->max_rdata = RFIFO_SIZE;
    s->max_wdata = WFIFO_SIZE;
    s->client_ip = IP4Address(client_address.sin_addr);
    s->created = TimeT::now();
    s->connected = 0;
}

Session *make_listen_port(uint16_t port, SessionParsers inferior)
{
    struct sockaddr_in server_address;
    io::FD fd = io::FD::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == io::FD())
    {
        perror("socket");
        return nullptr;
    }
    if (fd_max <= fd.uncast_dammit())
        fd_max = fd.uncast_dammit() + 1;

    fd.fcntl(F_SETFL, O_NONBLOCK);

    const int yes = 1;
    /// Allow to bind() again after the server restarts.
    // Since the socket is still in the TIME_WAIT, there's a possibility
    // that formerly lost packets might be delivered and confuse the server.
    fd.setsockopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    /// Send packets as soon as possible
    /// even if the kernel thinks there is too little for it to be worth it!
    // I'm not convinced this is a good idea; although in minimizes the
    // latency for an individual write, it increases traffic in general.
    fd.setsockopt(IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

    server_address.sin_family = AF_INET;
    DIAG_PUSH();
    DIAG_I(old_style_cast);
    DIAG_I(useless_cast);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
    DIAG_POP();

    if (fd.bind(reinterpret_cast<struct sockaddr *>(&server_address),
              sizeof(server_address)) == -1)
    {
        perror("bind");
        exit(1);
    }
    if (fd.listen(5) == -1)
    {                           /* error */
        perror("listen");
        exit(1);
    }

    readfds.set(fd);

    set_session(fd, make_unique<Session>(
                SessionIO{.func_recv= connect_client, .func_send= nullptr},
                SessionParsers{.func_parse= nullptr, .func_delete= nothing_delete}));
    Session *s = get_session(fd);
    s->for_inferior = inferior;
    s->fd = fd;

    s->created = TimeT::now();
    s->connected = 1;

    return s;
}

Session *make_connection(IP4Address ip, uint16_t port, SessionParsers parsers)
{
    struct sockaddr_in server_address;
    io::FD fd = io::FD::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == io::FD())
    {
        perror("socket");
        return nullptr;
    }
    if (fd_max <= fd.uncast_dammit())
        fd_max = fd.uncast_dammit() + 1;

    const int yes = 1;
    /// Allow to bind() again after the server restarts.
    // Since the socket is still in the TIME_WAIT, there's a possibility
    // that formerly lost packets might be delivered and confuse the server.
    fd.setsockopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    /// Send packets as soon as possible
    /// even if the kernel thinks there is too little for it to be worth it!
    // I'm not convinced this is a good idea; although in minimizes the
    // latency for an individual write, it increases traffic in general.
    fd.setsockopt(IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

    server_address.sin_family = AF_INET;
    server_address.sin_addr = in_addr(ip);
    DIAG_PUSH();
    DIAG_I(old_style_cast);
    DIAG_I(useless_cast);
    server_address.sin_port = htons(port);
    DIAG_POP();

    fd.fcntl(F_SETFL, O_NONBLOCK);

    /// Errors not caught - we must not block
    /// Let the main select() loop detect when we know the state
    fd.connect(reinterpret_cast<struct sockaddr *>(&server_address),
             sizeof(struct sockaddr_in));

    readfds.set(fd);

    set_session(fd, make_unique<Session>(
                SessionIO{.func_recv= recv_to_fifo, .func_send= send_from_fifo},
                parsers));
    Session *s = get_session(fd);
    s->fd = fd;
    s->rdata.new_(RFIFO_SIZE);
    s->wdata.new_(WFIFO_SIZE);

    s->max_rdata = RFIFO_SIZE;
    s->max_wdata = WFIFO_SIZE;
    s->created = TimeT::now();
    s->connected = 1;

    return s;
}

void delete_session(Session *s)
{
    if (!s)
        return;
    // this needs to be before the fd_max--
    s->func_delete(s);

    io::FD fd = s->fd;
    // If this was the highest fd, decrease it
    // We could add a loop to decrement fd_max further for every null session,
    // but this is cheap and good enough for the typical case
    if (fd.uncast_dammit() == fd_max - 1)
        fd_max--;
    readfds.clr(fd);
    {
        s->rdata.delete_();
        s->wdata.delete_();
        s->session_data.reset();
        reset_session(fd);
    }

    // just close() would try to keep sending buffers
    fd.shutdown(SHUT_RDWR);
    fd.close();
}

void realloc_fifo(Session *s, size_t rfifo_size, size_t wfifo_size)
{
    if (s->max_rdata != rfifo_size && s->rdata_size < rfifo_size)
    {
        s->rdata.resize(rfifo_size);
        s->max_rdata = rfifo_size;
    }
    if (s->max_wdata != wfifo_size && s->wdata_size < wfifo_size)
    {
        s->wdata.resize(wfifo_size);
        s->max_wdata = wfifo_size;
    }
}

bool do_sendrecv(interval_t next_ms)
{
    bool any = false;
    io::FD_Set rfd = readfds, wfd;
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (s)
        {
            any = true;
            if (s->wdata_size)
                wfd.set(i);
        }
    }
    if (!any)
    {
        if (!has_timers())
        {
            PRINTF("Shutting down - nothing to do\n"_fmt);
            // TODO hoist this
            return false;
        }
        return true;
    }
    struct timeval timeout;
    {
        std::chrono::seconds next_s = std::chrono::duration_cast<std::chrono::seconds>(next_ms);
        std::chrono::microseconds next_us = next_ms - next_s;
        timeout.tv_sec = next_s.count();
        timeout.tv_usec = next_us.count();
    }
    if (io::FD_Set::select(fd_max, &rfd, &wfd, nullptr, &timeout) <= 0)
        return true;
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        if (wfd.isset(i) && !s->eof)
        {
            if (s->func_send)
                //send_from_fifo(i);
                s->func_send(s);
        }
        if (rfd.isset(i) && !s->eof)
        {
            if (s->func_recv)
                //recv_to_fifo(i);
                //or connect_client(i);
                s->func_recv(s);
        }
    }
    return true;
}

bool do_parsepacket(void)
{
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        if (!s->connected
            && static_cast<time_t>(TimeT::now()) - static_cast<time_t>(s->created) > CONNECT_TIMEOUT)
        {
            PRINTF("Session #%d timed out\n"_fmt, s);
            s->set_eof();
        }
        if (s->rdata_size && !s->eof && s->func_parse)
        {
            s->func_parse(s);
            /// some func_parse may call delete_session
            // (that's kind of evil)
            s = get_session(i);
            if (!s)
                continue;
        }
        if (s->eof)
        {
            delete_session(s);
            continue;
        }
        /// Reclaim buffer space for what was read
        RFIFOFLUSH(s);
    }
    return true;
}
} // namespace tmwa
