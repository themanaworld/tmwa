#include "socket.hpp"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
//#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

#include "cxxstdio.hpp"
//#include "mmo.hpp"
#include "utils.hpp"

#include "../poison.hpp"

static
fd_set readfds;
int fd_max;

static
const uint32_t RFIFO_SIZE = 65536;
static
const uint32_t WFIFO_SIZE = 65536;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
std::array<std::unique_ptr<struct socket_data>, FD_SETSIZE> session;
#pragma GCC diagnostic pop

/// clean up by discarding handled bytes
inline
void RFIFOFLUSH(int fd)
{
    really_memmove(&session[fd]->rdata[0], &session[fd]->rdata[session[fd]->rdata_pos], RFIFOREST(fd));
    session[fd]->rdata_size = RFIFOREST(fd);
    session[fd]->rdata_pos = 0;
}

/// how much room there is to read more data
inline
size_t RFIFOSPACE(int fd)
{
    return session[fd]->max_rdata - session[fd]->rdata_size;
}


/// Discard all input
static
void null_parse(int fd);
/// Default parser for new connections
static
void (*default_func_parse)(int) = null_parse;

void set_defaultparse(void (*defaultparse)(int))
{
    default_func_parse = defaultparse;
}

/// Read from socket to the queue
static
void recv_to_fifo(int fd)
{
    if (session[fd]->eof)
        return;

    ssize_t len = read(fd, &session[fd]->rdata[session[fd]->rdata_size],
                        RFIFOSPACE(fd));

    if (len > 0)
    {
        session[fd]->rdata_size += len;
        session[fd]->connected = 1;
    }
    else
    {
        session[fd]->eof = 1;
    }
}

static
void send_from_fifo(int fd)
{
    if (session[fd]->eof)
        return;

    ssize_t len = write(fd, &session[fd]->wdata[0], session[fd]->wdata_size);

    if (len > 0)
    {
        session[fd]->wdata_size -= len;
        if (session[fd]->wdata_size)
        {
            really_memmove(&session[fd]->wdata[0], &session[fd]->wdata[len],
                     session[fd]->wdata_size);
        }
        session[fd]->connected = 1;
    }
    else
    {
        session[fd]->eof = 1;
    }
}

static
void null_parse(int fd)
{
    PRINTF("null_parse : %d\n", fd);
    RFIFOSKIP(fd, RFIFOREST(fd));
}


static
void connect_client(int listen_fd)
{
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);

    int fd = accept(listen_fd, reinterpret_cast<struct sockaddr *>(&client_address), &len);
    if (fd == -1)
    {
        perror("accept");
        return;
    }
    if (fd >= SOFT_LIMIT)
    {
        FPRINTF(stderr, "softlimit reached, disconnecting : %d\n", fd);
        shutdown(fd, SHUT_RDWR);
        close(fd);
        return;
    }
    if (fd_max <= fd)
    {
        fd_max = fd + 1;
    }

    const int yes = 1;
    /// Allow to bind() again after the server restarts.
    // Since the socket is still in the TIME_WAIT, there's a possibility
    // that formerly lost packets might be delivered and confuse the server.
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    /// Send packets as soon as possible
    /// even if the kernel thinks there is too little for it to be worth it!
    /// Testing shows this is indeed a good idea.
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

    // Linux-ism: Set socket options to optimize for thin streams
    // See http://lwn.net/Articles/308919/ and
    // Documentation/networking/tcp-thin.txt .. Kernel 3.2+
#ifdef TCP_THIN_LINEAR_TIMEOUTS
    setsockopt(fd, IPPROTO_TCP, TCP_THIN_LINEAR_TIMEOUTS, &yes, sizeof yes);
#endif
#ifdef TCP_THIN_DUPACK
    setsockopt(fd, IPPROTO_TCP, TCP_THIN_DUPACK, &yes, sizeof yes);
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    FD_SET(fd, &readfds);
#pragma GCC diagnostic pop

    fcntl(fd, F_SETFL, O_NONBLOCK);

    session[fd] = make_unique<socket_data>();
    session[fd]->rdata.new_(RFIFO_SIZE);
    session[fd]->wdata.new_(WFIFO_SIZE);

    session[fd]->max_rdata = RFIFO_SIZE;
    session[fd]->max_wdata = WFIFO_SIZE;
    session[fd]->func_recv = recv_to_fifo;
    session[fd]->func_send = send_from_fifo;
    session[fd]->func_parse = default_func_parse;
    session[fd]->client_addr = client_address;
    session[fd]->created = TimeT::now();
    session[fd]->connected = 0;
}

int make_listen_port(uint16_t port)
{
    struct sockaddr_in server_address;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }
    if (fd_max <= fd)
        fd_max = fd + 1;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    const int yes = 1;
    /// Allow to bind() again after the server restarts.
    // Since the socket is still in the TIME_WAIT, there's a possibility
    // that formerly lost packets might be delivered and confuse the server.
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    /// Send packets as soon as possible
    /// even if the kernel thinks there is too little for it to be worth it!
    // I'm not convinced this is a good idea; although in minimizes the
    // latency for an individual write, it increases traffic in general.
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

    server_address.sin_family = AF_INET;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
#pragma GCC diagnostic pop

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&server_address),
              sizeof(server_address)) == -1)
    {
        perror("bind");
        exit(1);
    }
    if (listen(fd, 5) == -1)
    {                           /* error */
        perror("listen");
        exit(1);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    FD_SET(fd, &readfds);
#pragma GCC diagnostic pop

    session[fd] = make_unique<socket_data>();

    session[fd]->func_recv = connect_client;
    session[fd]->created = TimeT::now();
    session[fd]->connected = 1;

    return fd;
}

int make_connection(uint32_t ip, uint16_t port)
{
    struct sockaddr_in server_address;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }
    if (fd_max <= fd)
        fd_max = fd + 1;

    const int yes = 1;
    /// Allow to bind() again after the server restarts.
    // Since the socket is still in the TIME_WAIT, there's a possibility
    // that formerly lost packets might be delivered and confuse the server.
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    /// Send packets as soon as possible
    /// even if the kernel thinks there is too little for it to be worth it!
    // I'm not convinced this is a good idea; although in minimizes the
    // latency for an individual write, it increases traffic in general.
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = ip;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
    server_address.sin_port = htons(port);
#pragma GCC diagnostic pop

    fcntl(fd, F_SETFL, O_NONBLOCK);

    /// Errors not caught - we must not block
    /// Let the main select() loop detect when we know the state
    connect(fd, reinterpret_cast<struct sockaddr *>(&server_address),
             sizeof(struct sockaddr_in));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    FD_SET(fd, &readfds);
#pragma GCC diagnostic pop

    session[fd] = make_unique<socket_data>();
    session[fd]->rdata.new_(RFIFO_SIZE);
    session[fd]->wdata.new_(WFIFO_SIZE);

    session[fd]->max_rdata = RFIFO_SIZE;
    session[fd]->max_wdata = WFIFO_SIZE;
    session[fd]->func_recv = recv_to_fifo;
    session[fd]->func_send = send_from_fifo;
    session[fd]->func_parse = default_func_parse;
    session[fd]->created = TimeT::now();
    session[fd]->connected = 1;

    return fd;
}

void delete_session(int fd)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    if (fd < 0 || fd >= FD_SETSIZE)
#pragma GCC diagnostic pop
        return;
    // If this was the highest fd, decrease it
    // We could add a loop to decrement fd_max further for every null session,
    // but this is cheap and good enough for the typical case
    if (fd == fd_max - 1)
        fd_max--;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    FD_CLR(fd, &readfds);
#pragma GCC diagnostic pop
    if (session[fd])
    {
        session[fd]->rdata.delete_();
        session[fd]->wdata.delete_();
        session[fd]->session_data.reset();
        session[fd].reset();
    }

    // just close() would try to keep sending buffers
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void realloc_fifo(int fd, size_t rfifo_size, size_t wfifo_size)
{
    const std::unique_ptr<socket_data>& s = session[fd];
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

void WFIFOSET(int fd, size_t len)
{
    std::unique_ptr<socket_data>& s = session[fd];
    if (s->wdata_size + len + 16384 > s->max_wdata)
    {
        realloc_fifo(fd, s->max_rdata, s->max_wdata << 1);
        PRINTF("socket: %d wdata expanded to %zu bytes.\n", fd, s->max_wdata);
    }
    if (s->wdata_size + len + 2048 < s->max_wdata)
        s->wdata_size += len;
    else
        FPRINTF(stderr, "socket: %d wdata lost !!\n", fd), abort();
}

void do_sendrecv(interval_t next_ms)
{
    fd_set rfd = readfds, wfd;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    FD_ZERO(&wfd);
#pragma GCC diagnostic pop
    for (int i = 0; i < fd_max; i++)
    {
        if (session[i] && session[i]->wdata_size)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            FD_SET(i, &wfd);
#pragma GCC diagnostic pop
    }
    struct timeval timeout;
    {
        std::chrono::seconds next_s = std::chrono::duration_cast<std::chrono::seconds>(next_ms);
        std::chrono::microseconds next_us = next_ms - next_s;
        timeout.tv_sec = next_s.count();
        timeout.tv_usec = next_us.count();
    }
    if (select(fd_max, &rfd, &wfd, NULL, &timeout) <= 0)
        return;
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (FD_ISSET(i, &wfd))
#pragma GCC diagnostic pop
        {
            if (session[i]->func_send)
                //send_from_fifo(i);
                session[i]->func_send(i);
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (FD_ISSET(i, &rfd))
#pragma GCC diagnostic pop
        {
            if (session[i]->func_recv)
                //recv_to_fifo(i);
                //or connect_client(i);
                session[i]->func_recv(i);
        }
    }
}

void do_parsepacket(void)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        if (!session[i]->connected
            && static_cast<time_t>(TimeT::now()) - static_cast<time_t>(session[i]->created) > CONNECT_TIMEOUT)
        {
            PRINTF("Session #%d timed out\n", i);
            session[i]->eof = 1;
        }
        if (!session[i]->rdata_size && !session[i]->eof)
            continue;
        if (session[i]->func_parse)
        {
            session[i]->func_parse(i);
            /// some func_parse may call delete_session
            if (!session[i])
                continue;
        }
        /// Reclaim buffer space for what was read
        RFIFOFLUSH(i);
    }
}

void do_socket(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    FD_ZERO(&readfds);
#pragma GCC diagnostic pop
}

void RFIFOSKIP(int fd, size_t len)
{
    std::unique_ptr<socket_data>& s = session[fd];
    s->rdata_pos += len;

    if (s->rdata_size < s->rdata_pos)
    {
        FPRINTF(stderr, "too many skip\n");
        abort();
    }
}
