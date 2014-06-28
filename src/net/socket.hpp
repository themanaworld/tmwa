#pragma once
//    socket.hpp - Network event system.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
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

#include <algorithm>

#include <sys/select.h>

#include <memory>

#include "../compat/iter.hpp"
#include "../compat/rawmem.hpp"
#include "../compat/time_t.hpp"

#include "../strings/astring.hpp"
#include "../strings/vstring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/dumb_ptr.hpp"

#include "../io/fd.hpp"

#include "ip.hpp"
#include "timer.t.hpp"


namespace tmwa
{
struct SessionData
{
};
struct SessionDeleter
{
    // defined per-server
    void operator()(SessionData *sd);
};

struct SessionIO
{
    void (*func_recv)(Session *);
    void (*func_send)(Session *);
};

struct SessionParsers
{
    void (*func_parse)(Session *);
    void (*func_delete)(Session *);
};

struct Session
{
    Session(SessionIO, SessionParsers);
    Session(Session&&) = delete;
    Session& operator = (Session&&) = delete;

    void set_io(SessionIO);
    void set_parsers(SessionParsers);

    /// Checks whether a newly-connected socket actually does anything
    TimeT created;
    bool connected;

private:
    /// Flag needed since structure must be freed in a server-dependent manner
    bool eof;
public:
    void set_eof() { eof = true; }

    /// Currently used by clif_setwaitclose
    Timer timed_close;

    /// Since this is a single-threaded application, it can't block
    /// These are the read/write queues
    dumb_ptr<uint8_t[]> rdata, wdata;
    size_t max_rdata, max_wdata;
    /// How much is actually in the queue
    size_t rdata_size, wdata_size;
    /// How much has already been read from the queue
    /// Note that there is no need for a wdata_pos
    size_t rdata_pos;

    IP4Address client_ip;

private:
    /// Send or recieve
    /// Only called when select() indicates the socket is ready
    /// If, after that, nothing is read, it sets eof
    // These could probably be hard-coded with a little work
    void (*func_recv)(Session *);
    void (*func_send)(Session *);
    /// This is the important one
    /// Set to different functions depending on whether the connection
    /// is a player or a server/ladmin
    void (*func_parse)(Session *);
    /// Cleanup function since we're not fully RAII yet
    void (*func_delete)(Session *);

public:
    // this really ought to be part of session_data, once that gets sane
    SessionParsers for_inferior;

    /// Server-specific data type
    // (this really should include the deleter, but ...)
    std::unique_ptr<SessionData, SessionDeleter> session_data;

    io::FD fd;

    friend void do_sendrecv(interval_t next);
    friend void do_parsepacket(void);
    friend void delete_session(Session *);
};

inline
int convert_for_printf(Session *s)
{
    return s->fd.uncast_dammit();
}

// save file descriptors for important stuff
constexpr int SOFT_LIMIT = FD_SETSIZE - 50;

// socket timeout to establish a full connection in seconds
constexpr int CONNECT_TIMEOUT = 15;


void set_session(io::FD fd, std::unique_ptr<Session> sess);
Session *get_session(io::FD fd);
void reset_session(io::FD fd);
int get_fd_max();

class IncrFD
{
public:
    static
    io::FD inced(io::FD v)
    {
        return io::FD::cast_dammit(v.uncast_dammit() + 1);
    }
};
IteratorPair<ValueIterator<io::FD, IncrFD>> iter_fds();


/// open a socket, bind, and listen. Return an fd, or -1 if socket() fails,
/// but exit if bind() or listen() fails
Session *make_listen_port(uint16_t port, SessionParsers inferior);
/// Connect to an address, return a connected socket or -1
// FIXME - this is IPv4 only!
Session *make_connection(IP4Address ip, uint16_t port, SessionParsers);
/// free() the structure and close() the fd
void delete_session(Session *);
/// Make a the internal queues bigger
void realloc_fifo(Session *s, size_t rfifo_size, size_t wfifo_size);
/// Update all sockets that can be read/written from the queues
void do_sendrecv(interval_t next);
/// Call the parser function for every socket that has read data
void do_parsepacket(void);
} // namespace tmwa
