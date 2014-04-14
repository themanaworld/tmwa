#ifndef TMWA_MMO_SOCKET_HPP
#define TMWA_MMO_SOCKET_HPP
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

# include "../sanity.hpp"

# include <netinet/in.h>

# include <cstdio>

# include <array>

# include "../compat/rawmem.hpp"

# include "../strings/astring.hpp"
# include "../strings/vstring.hpp"
# include "../strings/xstring.hpp"

# include "../io/fd.hpp"

# include "dumb_ptr.hpp"
# include "ip.hpp"
# include "utils.hpp"
# include "timer.t.hpp"

struct SessionData
{
};
struct SessionDeleter
{
    // defined per-server
    void operator()(SessionData *sd);
};

// Struct declaration

struct Session
{
    /// Checks whether a newly-connected socket actually does anything
    TimeT created;
    bool connected;

    /// Flag needed since structure must be freed in a server-dependent manner
    bool eof;
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

    /// Send or recieve
    /// Only called when select() indicates the socket is ready
    /// If, after that, nothing is read, it sets eof
    // These could probably be hard-coded with a little work
    void (*func_recv)(Session *);
    void (*func_send)(Session *);
    /// This is the important one
    /// Set to different functions depending on whether the connection
    /// is a player or a server/ladmin
    /// Can be set explicitly or via set_defaultparse
    void (*func_parse)(Session *);
    /// Server-specific data type
    std::unique_ptr<SessionData, SessionDeleter> session_data;

    io::FD fd;
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
Session *make_listen_port(uint16_t port);
/// Connect to an address, return a connected socket or -1
// FIXME - this is IPv4 only!
Session *make_connection(IP4Address ip, uint16_t port);
/// free() the structure and close() the fd
void delete_session(Session *);
/// Make a the internal queues bigger
void realloc_fifo(Session *s, size_t rfifo_size, size_t wfifo_size);
/// Update all sockets that can be read/written from the queues
void do_sendrecv(interval_t next);
/// Call the parser function for every socket that has read data
void do_parsepacket(void);

/// Change the default parser for newly connected clients
// typically called once per server, but individual clients may identify
// themselves as servers
void set_defaultparse(void(*defaultparse)(Session *));

template<class T>
uint8_t *pod_addressof_m(T& structure)
{
    static_assert(is_trivially_copyable<T>::value, "Can only byte-copy POD-ish structs");
    return &reinterpret_cast<uint8_t&>(structure);
}

template<class T>
const uint8_t *pod_addressof_c(const T& structure)
{
    static_assert(is_trivially_copyable<T>::value, "Can only byte-copy POD-ish structs");
    return &reinterpret_cast<const uint8_t&>(structure);
}


/// Check how much can be read
inline
size_t RFIFOREST(Session *s)
{
    return s->rdata_size - s->rdata_pos;
}
/// Read from the queue
inline
const void *RFIFOP(Session *s, size_t pos)
{
    return &s->rdata[s->rdata_pos + pos];
}
inline
uint8_t RFIFOB(Session *s, size_t pos)
{
    return *static_cast<const uint8_t *>(RFIFOP(s, pos));
}
inline
uint16_t RFIFOW(Session *s, size_t pos)
{
    return *static_cast<const uint16_t *>(RFIFOP(s, pos));
}
inline
uint32_t RFIFOL(Session *s, size_t pos)
{
    return *static_cast<const uint32_t *>(RFIFOP(s, pos));
}
template<class T>
void RFIFO_STRUCT(Session *s, size_t pos, T& structure)
{
    really_memcpy(pod_addressof_m(structure), static_cast<const uint8_t *>(RFIFOP(s, pos)), sizeof(T));
}
inline
IP4Address RFIFOIP(Session *s, size_t pos)
{
    IP4Address o;
    RFIFO_STRUCT(s, pos, o);
    return o;
}
template<uint8_t len>
inline
VString<len-1> RFIFO_STRING(Session *s, size_t pos)
{
    const char *const begin = static_cast<const char *>(RFIFOP(s, pos));
    const char *const end = begin + len-1;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}
inline
AString RFIFO_STRING(Session *s, size_t pos, size_t len)
{
    const char *const begin = static_cast<const char *>(RFIFOP(s, pos));
    const char *const end = begin + len;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}
inline
void RFIFO_BUF_CLONE(Session *s, uint8_t *buf, size_t len)
{
    really_memcpy(buf, static_cast<const uint8_t *>(RFIFOP(s, 0)), len);
}

/// Done reading
void RFIFOSKIP(Session *s, size_t len);

/// Read from an arbitrary buffer
inline
const void *RBUFP(const uint8_t *p, size_t pos)
{
    return p + pos;
}
inline
uint8_t RBUFB(const uint8_t *p, size_t pos)
{
    return *static_cast<const uint8_t *>(RBUFP(p, pos));
}
inline
uint16_t RBUFW(const uint8_t *p, size_t pos)
{
    return *static_cast<const uint16_t *>(RBUFP(p, pos));
}
inline
uint32_t RBUFL(const uint8_t *p, size_t pos)
{
    return *static_cast<const uint32_t *>(RBUFP(p, pos));
}
template<class T>
void RBUF_STRUCT(const uint8_t *p, size_t pos, T& structure)
{
    really_memcpy(pod_addressof_m(structure), p + pos, sizeof(T));
}
inline
IP4Address RBUFIP(const uint8_t *p, size_t pos)
{
    IP4Address o;
    RBUF_STRUCT(p, pos, o);
    return o;
}
template<uint8_t len>
inline
VString<len-1> RBUF_STRING(const uint8_t *p, size_t pos)
{
    const char *const begin = static_cast<const char *>(RBUFP(p, pos));
    const char *const end = begin + len-1;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}
inline
AString RBUF_STRING(const uint8_t *p, size_t pos, size_t len)
{
    const char *const begin = static_cast<const char *>(RBUFP(p, pos));
    const char *const end = begin + len;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}


/// Unused - check how much data can be written
// the existence of this seems scary
inline
size_t WFIFOSPACE(Session *s)
{
    return s->max_wdata - s->wdata_size;
}
/// Write to the queue
inline
void *WFIFOP(Session *s, size_t pos)
{
    return &s->wdata[s->wdata_size + pos];
}
inline
uint8_t& WFIFOB(Session *s, size_t pos)
{
    return *static_cast<uint8_t *>(WFIFOP(s, pos));
}
inline
uint16_t& WFIFOW(Session *s, size_t pos)
{
    return *static_cast<uint16_t *>(WFIFOP(s, pos));
}
inline
uint32_t& WFIFOL(Session *s, size_t pos)
{
    return *static_cast<uint32_t *>(WFIFOP(s, pos));
}
template<class T>
void WFIFO_STRUCT(Session *s, size_t pos, T& structure)
{
    really_memcpy(static_cast<uint8_t *>(WFIFOP(s, pos)), pod_addressof_c(structure), sizeof(T));
}
inline
IP4Address& WFIFOIP(Session *s, size_t pos)
{
    static_assert(is_trivially_copyable<IP4Address>::value, "That was the whole point");
    return *static_cast<IP4Address *>(WFIFOP(s, pos));
}
inline
void WFIFO_STRING(Session *s, size_t pos, XString str, size_t len)
{
    char *const begin = static_cast<char *>(WFIFOP(s, pos));
    char *const end = begin + len;
    char *const mid = std::copy(str.begin(), str.end(), begin);
    std::fill(mid, end, '\0');
}
inline
void WFIFO_ZERO(Session *s, size_t pos, size_t len)
{
    uint8_t *b = static_cast<uint8_t *>(WFIFOP(s, pos));
    uint8_t *e = b + len;
    std::fill(b, e, '\0');
}
inline
void WFIFO_BUF_CLONE(Session *s, const uint8_t *buf, size_t len)
{
    really_memcpy(static_cast<uint8_t *>(WFIFOP(s, 0)), buf, len);
}

/// Finish writing
void WFIFOSET(Session *s, size_t len);

/// Write to an arbitrary buffer
inline
void *WBUFP(uint8_t *p, size_t pos)
{
    return p + pos;
}
inline
uint8_t& WBUFB(uint8_t *p, size_t pos)
{
    return *static_cast<uint8_t *>(WBUFP(p, pos));
}
inline
uint16_t& WBUFW(uint8_t *p, size_t pos)
{
    return *static_cast<uint16_t *>(WBUFP(p, pos));
}
inline
uint32_t& WBUFL(uint8_t *p, size_t pos)
{
    return *static_cast<uint32_t *>(WBUFP(p, pos));
}
template<class T>
void WBUF_STRUCT(uint8_t *p, size_t pos, T& structure)
{
    really_memcpy(p + pos, pod_addressof_c(structure), sizeof(T));
}
inline
IP4Address& WBUFIP(uint8_t *p, size_t pos)
{
    return *static_cast<IP4Address *>(WBUFP(p, pos));
}
inline
void WBUF_STRING(uint8_t *p, size_t pos, XString s, size_t len)
{
    char *const begin = static_cast<char *>(WBUFP(p, pos));
    char *const end = begin + len;
    char *const mid = std::copy(s.begin(), s.end(), begin);
    std::fill(mid, end, '\0');
}
inline
void WBUF_ZERO(uint8_t *p, size_t pos, size_t len)
{
    uint8_t *b = static_cast<uint8_t *>(WBUFP(p, pos));
    uint8_t *e = b + len;
    std::fill(b, e, '\0');
}

inline
void RFIFO_WFIFO_CLONE(Session *rs, Session *ws, size_t len)
{
    really_memcpy(static_cast<uint8_t *>(WFIFOP(ws, 0)),
            static_cast<const uint8_t *>(RFIFOP(rs, 0)), len);
}

#endif // TMWA_MMO_SOCKET_HPP
