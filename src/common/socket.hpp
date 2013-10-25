#ifndef SOCKET_HPP
#define SOCKET_HPP

# include "sanity.hpp"

# include <netinet/in.h>

# include <cstdio>

# include <array>

# include "../strings/fstring.hpp"
# include "../strings/vstring.hpp"
# include "../strings/xstring.hpp"

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

struct socket_data
{
    /// Checks whether a newly-connected socket actually does anything
    TimeT created;
    bool connected;

    /// Flag needed since structure must be freed in a server-dependent manner
    bool eof;

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
    void (*func_recv)(int);
    void (*func_send)(int);
    /// This is the important one
    /// Set to different functions depending on whether the connection
    /// is a player or a server/ladmin
    /// Can be set explicitly or via set_defaultparse
    void (*func_parse)(int);
    /// Server-specific data type
    std::unique_ptr<SessionData, SessionDeleter> session_data;
};

// save file descriptors for important stuff
constexpr int SOFT_LIMIT = FD_SETSIZE - 50;

// socket timeout to establish a full connection in seconds
constexpr int CONNECT_TIMEOUT = 15;

/// Everyone who has connected
// note: call delete_session(i) to null out an element
extern std::array<std::unique_ptr<socket_data>, FD_SETSIZE> session;

/// Maximum used FD, +1
extern int fd_max;

/// open a socket, bind, and listen. Return an fd, or -1 if socket() fails,
/// but exit if bind() or listen() fails
int make_listen_port(uint16_t port);
/// Connect to an address, return a connected socket or -1
// FIXME - this is IPv4 only!
int make_connection(IP4Address ip, uint16_t port);
/// free() the structure and close() the fd
void delete_session(int);
/// Make a the internal queues bigger
void realloc_fifo(int fd, size_t rfifo_size, size_t wfifo_size);
/// Update all sockets that can be read/written from the queues
void do_sendrecv(interval_t next);
/// Call the parser function for every socket that has read data
void do_parsepacket(void);

/// An init function
void do_socket(void);

/// Change the default parser for newly connected clients
// typically called once per server, but individual clients may identify
// themselves as servers
void set_defaultparse(void(*defaultparse)(int));

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
size_t RFIFOREST(int fd)
{
    return session[fd]->rdata_size - session[fd]->rdata_pos;
}
/// Read from the queue
inline
const void *RFIFOP(int fd, size_t pos)
{
    return &session[fd]->rdata[session[fd]->rdata_pos + pos];
}
inline
uint8_t RFIFOB(int fd, size_t pos)
{
    return *static_cast<const uint8_t *>(RFIFOP(fd, pos));
}
inline
uint16_t RFIFOW(int fd, size_t pos)
{
    return *static_cast<const uint16_t *>(RFIFOP(fd, pos));
}
inline
uint32_t RFIFOL(int fd, size_t pos)
{
    return *static_cast<const uint32_t *>(RFIFOP(fd, pos));
}
template<class T>
void RFIFO_STRUCT(int fd, size_t pos, T& structure)
{
    really_memcpy(pod_addressof_m(structure), static_cast<const uint8_t *>(RFIFOP(fd, pos)), sizeof(T));
}
inline
IP4Address RFIFOIP(int fd, size_t pos)
{
    IP4Address o;
    RFIFO_STRUCT(fd, pos, o);
    return o;
}
template<uint8_t len>
inline
VString<len-1> RFIFO_STRING(int fd, size_t pos)
{
    const char *const begin = static_cast<const char *>(RFIFOP(fd, pos));
    const char *const end = begin + len-1;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}
inline
FString RFIFO_STRING(int fd, size_t pos, size_t len)
{
    const char *const begin = static_cast<const char *>(RFIFOP(fd, pos));
    const char *const end = begin + len;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}
inline
void RFIFO_BUF_CLONE(int fd, uint8_t *buf, size_t len)
{
    really_memcpy(buf, static_cast<const uint8_t *>(RFIFOP(fd, 0)), len);
}

/// Done reading
void RFIFOSKIP(int fd, size_t len);

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
FString RBUF_STRING(const uint8_t *p, size_t pos, size_t len)
{
    const char *const begin = static_cast<const char *>(RBUFP(p, pos));
    const char *const end = begin + len;
    const char *const mid = std::find(begin, end, '\0');
    return XString(begin, mid, nullptr);
}


/// Unused - check how much data can be written
// the existence of this seems scary
inline
size_t WFIFOSPACE(int fd)
{
    return session[fd]->max_wdata - session[fd]->wdata_size;
}
/// Write to the queue
inline
void *WFIFOP(int fd, size_t pos)
{
    return &session[fd]->wdata[session[fd]->wdata_size + pos];
}
inline
uint8_t& WFIFOB(int fd, size_t pos)
{
    return *static_cast<uint8_t *>(WFIFOP(fd, pos));
}
inline
uint16_t& WFIFOW(int fd, size_t pos)
{
    return *static_cast<uint16_t *>(WFIFOP(fd, pos));
}
inline
uint32_t& WFIFOL(int fd, size_t pos)
{
    return *static_cast<uint32_t *>(WFIFOP(fd, pos));
}
template<class T>
void WFIFO_STRUCT(int fd, size_t pos, T& structure)
{
    really_memcpy(static_cast<uint8_t *>(WFIFOP(fd, pos)), pod_addressof_c(structure), sizeof(T));
}
inline
IP4Address& WFIFOIP(int fd, size_t pos)
{
    static_assert(is_trivially_copyable<IP4Address>::value, "That was the whole point");
    return *static_cast<IP4Address *>(WFIFOP(fd, pos));
}
inline
void WFIFO_STRING(int fd, size_t pos, XString s, size_t len)
{
    char *const begin = static_cast<char *>(WFIFOP(fd, pos));
    char *const end = begin + len;
    char *const mid = std::copy(s.begin(), s.end(), begin);
    std::fill(mid, end, '\0');
}
inline
void WFIFO_ZERO(int fd, size_t pos, size_t len)
{
    uint8_t *b = static_cast<uint8_t *>(WFIFOP(fd, pos));
    uint8_t *e = b + len;
    std::fill(b, e, '\0');
}
inline
void WFIFO_BUF_CLONE(int fd, const uint8_t *buf, size_t len)
{
    really_memcpy(static_cast<uint8_t *>(WFIFOP(fd, 0)), buf, len);
}

/// Finish writing
void WFIFOSET(int fd, size_t len);

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
void RFIFO_WFIFO_CLONE(int rfd, int wfd, size_t len)
{
    really_memcpy(static_cast<uint8_t *>(WFIFOP(wfd, 0)),
            static_cast<const uint8_t *>(RFIFOP(rfd, 0)), len);
}

#endif // SOCKET_HPP
