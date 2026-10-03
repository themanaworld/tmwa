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
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>

#include <fcntl.h>

#include <cstdint>
#include <cstdlib>

#include <vector>

#include "../compat/memory.hpp"

#include "../io/cxxstdio.hpp"

#include "../mmo/consts.hpp"

#include "../proto2/map-user.hpp"

#include "../wire/packets.hpp"

#include "timer.hpp"

#include "../poison.hpp"


namespace tmwa
{
static
int fd_max;

static
const uint32_t RFIFO_SIZE = 65536;
static
const uint32_t WFIFO_SIZE = 65536;

/// Hard bound on the number of fds usable for sessions.
/// The session table is sized per fd, so do not make this huge.
static
const int SESSION_FD_CAP = 1 << 16;
/// Fds reserved below the process limit for non-session use
/// (files, the epoll set itself), same idea as the old SOFT_LIMIT.
static
const int SESSION_FD_RESERVE = 50;

/// The epoll set holding all session fds.
/// All sessions are registered for EPOLLIN, plus EPOLLOUT while
/// the session has data queued for sending.
static
int epoll_fd = -1;
/// fds at or above this are too precious to give to clients;
/// computed by socket_init() from RLIMIT_NOFILE.
static
int fd_soft_limit;

/// session indexed by fd, sparse over all open fds
static
std::vector<std::unique_ptr<Session>> session;
/// fds that currently have sessions, in no particular order
static
std::vector<int> live_fds;
/// index into live_fds per fd, or -1
static
std::vector<int> live_pos;
/// whether EPOLLOUT is currently registered, per fd
static
std::vector<char> epoll_out;

/// One-time setup of the epoll set and the fd soft limit.
static
void socket_init()
{
    if (epoll_fd != -1)
        return;

    fd_soft_limit = FD_SETSIZE - SESSION_FD_RESERVE;
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
    {
        // Raise the soft fd limit so the cap actually improves on the old
        // FD_SETSIZE bound; never above what the hard limit allows.
        rlim_t want = rl.rlim_max;
        rlim_t need = static_cast<rlim_t>(SESSION_FD_CAP) + SESSION_FD_RESERVE;
        if (want == RLIM_INFINITY || want > need)
            want = need;
        if (rl.rlim_cur < want)
        {
            rl.rlim_cur = want;
            if (setrlimit(RLIMIT_NOFILE, &rl) != 0)
                perror("setrlimit(RLIMIT_NOFILE)");
        }
        rlim_t cur = rl.rlim_cur;
        if (cur > need)
            cur = need;
        fd_soft_limit = static_cast<int>(cur) - SESSION_FD_RESERVE;
        if (fd_soft_limit < FD_SETSIZE - SESSION_FD_RESERVE)
            fd_soft_limit = FD_SETSIZE - SESSION_FD_RESERVE;
    }

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(1);
    }
}

static
void epoll_add(io::FD fd)
{
    socket_init();
    struct epoll_event ev {};
    ev.events = EPOLLIN;
    ev.data.fd = fd.uncast_dammit();
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev))
    {
        perror("epoll_ctl(EPOLL_CTL_ADD)");
        exit(1);
    }
}

static
void epoll_del(io::FD fd)
{
    // close() would remove it anyway; be explicit
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd.uncast_dammit(), nullptr);
}

/// Toggle EPOLLOUT interest for a session's fd.
static
void epoll_write(Session *s, bool want)
{
    int f = s->fd.uncast_dammit();
    // sessions that never went through the fd table (e.g. stack
    // sessions in tests) have no epoll state to update
    if (f < 0 || static_cast<size_t>(f) >= session.size()
            || session[f].get() != s)
        return;
    assert (0 <= f && static_cast<size_t>(f) < epoll_out.size());
    if (!epoll_out[f] == !want)
        return;
    epoll_out[f] = want;
    struct epoll_event ev {};
    ev.events = EPOLLIN | (want ? EPOLLOUT : 0);
    ev.data.fd = f;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, f, &ev))
    {
        perror("epoll_ctl(EPOLL_CTL_MOD)");
        exit(1);
    }
}

/// Make room in the fd-indexed tables.
static
void session_table_grow(int f)
{
    if (static_cast<size_t>(f) < session.size())
        return;
    size_t n = session.size() * 2;
    if (n < 1024)
        n = 1024;
    while (static_cast<size_t>(f) >= n)
        n *= 2;
    session.resize(n);
    live_pos.resize(n, -1);
    epoll_out.resize(n, 0);
}

Session::Session(SessionIO io, SessionParsers p)
: created()
, last_tick()
, connected()
, timed_close()
, rdata(), wdata()
, max_rdata(), max_wdata()
, rdata_size(), wdata_size()
, rdata_pos(), wdata_pos()
, client_ip()
, func_recv()
, func_send()
, func_parse()
, func_delete()
, for_inferior()
, session_data()
, fd()
{
    flag.eof = 0;
    flag.server = 0;
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
    assert (0 <= f);
    session_table_grow(f);
    assert (!session[f]);
    live_pos[f] = live_fds.size();
    live_fds.push_back(f);
    session[f] = std::move(sess);
}
Session *get_session(io::FD fd)
{
    int f = fd.uncast_dammit();
    if (0 <= f && static_cast<size_t>(f) < session.size())
        return session[f].get();
    return nullptr;
}
void reset_session(io::FD fd)
{
    int f = fd.uncast_dammit();
    assert (0 <= f && static_cast<size_t>(f) < session.size());
    int pos = live_pos[f];
    if (pos >= 0)
    {
        int last = live_fds.back();
        live_fds[pos] = last;
        live_pos[last] = pos;
        live_fds.pop_back();
        live_pos[f] = -1;
    }
    epoll_out[f] = 0;
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
        s->last_tick = TimeT::now();
    }
    else
    {
        s->set_eof();
    }
}

static
void send_from_fifo(Session *s)
{
    if (!s->wdata_size)
    {
        // stale EPOLLOUT event: nothing queued, drop write interest
        epoll_write(s, false);
        return;
    }
    // pending data starts at wdata_pos and may wrap around the end,
    // so only the contiguous head run can be sent in one call
    ssize_t len = s->fd.send(&s->wdata[s->wdata_pos],
                        std::min(s->wdata_size, s->max_wdata - s->wdata_pos),
                        MSG_NOSIGNAL);

    if (len > 0)
    {
        s->wdata_pos += len;
        s->wdata_size -= len;
        if (s->wdata_pos >= s->max_wdata || !s->wdata_size)
        {
            s->wdata_pos = 0;
        }
        if (!s->wdata_size)
        {
            // queue drained; wait for more data before polling for writable
            epoll_write(s, false);
        }
        s->connected = 1;
        s->last_tick = TimeT::now();
    }
    else
    {
        s->set_eof();
    }
}

/// Called when data was queued in s->wdata; registers write interest.
/// packet_send is the only place wdata grows.
void session_want_write(Session *s)
{
    epoll_write(s, true);
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
    socket_init();
    if (fd.uncast_dammit() >= fd_soft_limit)
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

    fd.fcntl(F_SETFL, O_NONBLOCK);

    epoll_add(fd);

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

    epoll_add(fd);

    set_session(fd, make_unique<Session>(
                SessionIO{.func_recv= connect_client, .func_send= nullptr},
                SessionParsers{.func_parse= nullptr, .func_delete= nothing_delete}));
    Session *s = get_session(fd);
    s->for_inferior = inferior;
    s->fd = fd;

    s->created = TimeT::now();
    s->connected = 1;
    s->set_server();

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
    /// Let the main epoll loop detect when we know the state
    fd.connect(reinterpret_cast<struct sockaddr *>(&server_address),
             sizeof(struct sockaddr_in));

    epoll_add(fd);

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
    s->set_server();

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
    epoll_del(fd);
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
        dumb_ptr<uint8_t[]> nd;
        nd.new_(wfifo_size);
        if (s->wdata_size)
        {
            // pending data is a ring starting at wdata_pos, un-wrap it
            size_t first = std::min(s->wdata_size,
                    s->max_wdata - s->wdata_pos);
            really_memcpy(&nd[0], &s->wdata[s->wdata_pos], first);
            really_memcpy(&nd[first], &s->wdata[0], s->wdata_size - first);
        }
        s->wdata.delete_();
        s->wdata = nd;
        s->wdata_pos = 0;
        s->max_wdata = wfifo_size;
    }
}

/// Server links are the sessions whose fifos were raised to
/// FIFOSIZE_SERVERLINK once the peer authenticated, matching the rule
/// packet_send uses for the wdata cap. flag.server is not set on
/// accepted links, so the fifo size is the reliable test. Listeners
/// have no fifo and sort with the clients.
static
bool is_server_link(Session *s)
{
    return s->max_rdata >= FIFOSIZE_SERVERLINK;
}

/// Outbound queue depth at which a link counts as congested: past
/// this it is heading for the hard cap, while still cheap to drain.
static
const size_t LINK_THROTTLE_WDATA = WFIFO_MAX_SERVERLINK / 8;

/// Whether any link session still holds a deep outbound queue.
static
bool server_link_congested()
{
    for (int f : live_fds)
    {
        Session *s = get_session(io::FD::cast_dammit(f));
        if (s && is_server_link(s) && s->wdata_size > LINK_THROTTLE_WDATA)
            return true;
    }
    return false;
}

/// How many events are dispatched per epoll_wait call.
/// Level-triggered sockets just report again next time if there are more.
static
const int EPOLL_MAX_EVENTS = 512;

bool do_sendrecv(interval_t next_ms)
{
    socket_init();
    if (live_fds.empty())
    {
        if (!has_timers())
        {
            PRINTF("Shutting down - nothing to do\n"_fmt);
            // TODO hoist this
            return false;
        }
        return true;
    }
    // epoll_wait takes a timeout in whole milliseconds
    int64_t ms = next_ms.count();
    int timeout = ms < 0 ? 0 : ms > INT32_MAX ? INT32_MAX : static_cast<int>(ms);
    struct epoll_event events[EPOLL_MAX_EVENTS];
    int n = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, timeout);
    if (n <= 0)
        return true;
    // Two passes over the batch: server links are serviced before the
    // bulk client sessions, in both directions. The map<->char link
    // shares this loop with every client, and starving it lets its
    // wdata hit the cap and kills a healthy link.
    for (int pass = 0; pass < 2; pass++)
    {
        bool want_links = pass == 0;
        // The backlog is checked only after the links were serviced
        // this pass. While one is still deep, no new client input is
        // ingested, so production falls below drain and the queue
        // clears; queued client output keeps flushing either way.
        bool want_recv = want_links || !server_link_congested();
        for (int i = 0; i < n; i++)
        {
            Session *s = get_session(io::FD::cast_dammit(events[i].data.fd));
            if (!s || is_server_link(s) != want_links)
                continue;
            uint32_t ev = events[i].events;
            if ((ev & EPOLLOUT) && s->flag.eof != 1)
            {
                if (s->func_send)
                    //send_from_fifo(i);
                    s->func_send(s);
            }
            if (want_recv
                    && (ev & (EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP))
                    && s->flag.eof != 1)
            {
                if (s->func_recv)
                    //recv_to_fifo(i);
                    //or connect_client(i);
                    s->func_recv(s);
            }
        }
    }
    return true;
}

bool do_parsepacket(void)
{
    // Iterating by index, since func_parse may delete sessions:
    // live_fds can shrink under us, so entries are re-fetched each time.
    // A session swapped into an already-visited slot is parsed next call.
    // Server links are parsed before client sessions, matching
    // do_sendrecv. While a link's outbound queue is still deep after
    // its pass, client func_parse is skipped for this pass; the
    // timeout/eof bookkeeping and fifo reclaim still run for clients,
    // so dead sessions do not pile up behind the congestion.
    for (int pass = 0; pass < 2; pass++)
    {
        bool want_links = pass == 0;
        bool allow_parse = want_links || !server_link_congested();
        for (size_t i = 0; i < live_fds.size(); i++)
        {
            io::FD fd = io::FD::cast_dammit(live_fds[i]);
            Session *s = get_session(fd);
            if (!s || is_server_link(s) != want_links)
                continue;
            if (s->connected && s->flag.server != 1
            && static_cast<time_t>(TimeT::now()) - static_cast<time_t>(s->last_tick) > STALL_TIMEOUT / 2)
            {
                // send a keepalive packet
                Packet_Fixed<0x007f> fixed_7f;
                fixed_7f.tick = gettick();
                send_fpacket<0x007f, 6>(s, fixed_7f);
                // if this fails it will auto-eof
            }
            if ((!s->connected
                && static_cast<time_t>(TimeT::now()) - static_cast<time_t>(s->created) > CONNECT_TIMEOUT) ||
                (s->connected && s->flag.server != 1
                && static_cast<time_t>(TimeT::now()) - static_cast<time_t>(s->last_tick) > STALL_TIMEOUT))
            {
                PRINTF("Session #%d timed out\n"_fmt, s);
                s->set_eof();
            }
            if (allow_parse && s->rdata_size && s->flag.eof != 1 && s->func_parse)
            {
                s->func_parse(s);
                /// some func_parse may call delete_session
                // (that's kind of evil)
                s = get_session(fd);
                if (!s)
                    continue;
            }
            if (s->flag.eof == 1)
            {
                delete_session(s);
                continue;
            }
            /// Reclaim buffer space for what was read
            RFIFOFLUSH(s);
        }
    }
    return true;
}
} // namespace tmwa
