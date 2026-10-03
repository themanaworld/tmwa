#include "socket.hpp"
//    socket_test.cpp - Testsuite for the socket layer.
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
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "../mmo/consts.hpp"

#include "../wire/packets.hpp"

#include "../poison.hpp"


namespace tmwa
{
// The real deleter is defined per-server; tests never set session_data.
void SessionDeleter::operator()(SessionData *sd)
{
    really_delete1 sd;
}

static
void test_delete(Session *)
{
}

static
size_t parsed_bytes;
static
Session *parsed_session;
static
std::vector<Session *> parse_order;
static
void test_parse(Session *s)
{
    parsed_bytes += packet_avail(s);
    parsed_session = s;
    parse_order.push_back(s);
    packet_discard(s, packet_avail(s));
}

/// Run send/recv and parse until the given session is accepted and has
/// consumed all pending input, or too many iterations pass.
static
void pump(int rounds)
{
    for (int i = 0; i < rounds; i++)
    {
        do_sendrecv(100_ms);
        do_parsepacket();
    }
}

static
int connect_to(Session *ls)
{
    struct sockaddr_in addr {};
    socklen_t alen = sizeof(addr);
    EXPECT_EQ(0, ::getsockname(ls->fd.uncast_dammit(),
            reinterpret_cast<struct sockaddr *>(&addr), &alen));
    int cfd = ::socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_EQ(0, ::connect(cfd,
            reinterpret_cast<struct sockaddr *>(&addr), alen));
    return cfd;
}

TEST(socket, accept_recv_send)
{
    parsed_bytes = 0;
    parsed_session = nullptr;
    Session *ls = make_listen_port(0,
            SessionParsers{.func_parse= test_parse, .func_delete= test_delete});
    ASSERT_NE(ls, nullptr);

    int cfd = connect_to(ls);
    ASSERT_GE(cfd, 0);
    ASSERT_EQ(4, ::send(cfd, "ping", 4, 0));

    // first pump accepts, second pumps the data through recv and parse
    pump(4);
    ASSERT_NE(parsed_session, nullptr);
    EXPECT_EQ(4u, parsed_bytes);

    // queue a reply and check it is flushed back over the wire
    Byte reply[2] = {Byte{0x12}, Byte{0x34}};
    ASSERT_TRUE(packet_send(parsed_session, reply, 2));
    pump(4);
    char buf[8];
    ASSERT_EQ(2, ::recv(cfd, buf, sizeof(buf), 0));
    EXPECT_EQ(buf[0], 0x12);
    EXPECT_EQ(buf[1], 0x34);

    // client close must set eof, and parsepacket must reap the session
    int f = parsed_session->fd.uncast_dammit();
    ::close(cfd);
    pump(4);
    EXPECT_EQ(nullptr, get_session(io::FD::cast_dammit(f)));

    delete_session(ls);
}

TEST(socket, accept_above_fd_setsize)
{
    // skip if the hard fd limit is too low to place fds above FD_SETSIZE
    struct rlimit rl;
    ASSERT_EQ(0, getrlimit(RLIMIT_NOFILE, &rl));
    if (rl.rlim_max != RLIM_INFINITY && rl.rlim_max < FD_SETSIZE + 100)
        GTEST_SKIP() << "hard fd limit too low";

    parsed_bytes = 0;
    parsed_session = nullptr;
    Session *ls = make_listen_port(0,
            SessionParsers{.func_parse= test_parse, .func_delete= test_delete});
    ASSERT_NE(ls, nullptr);

    // push the accept()ed fd above the old FD_SETSIZE bound
    std::vector<int> filler;
    while (true)
    {
        int f = ::open("/dev/null", O_RDONLY);
        if (f < 0)
            break;
        filler.push_back(f);
        if (f >= FD_SETSIZE + 10)
            break;
    }
    ASSERT_GE(filler.back(), FD_SETSIZE + 10);

    int cfd = connect_to(ls);
    ASSERT_GE(cfd, 0);
    ASSERT_EQ(4, ::send(cfd, "ping", 4, 0));
    pump(4);
    ASSERT_NE(parsed_session, nullptr);
    EXPECT_GT(parsed_session->fd.uncast_dammit(), FD_SETSIZE);
    EXPECT_EQ(4u, parsed_bytes);

    delete_session(parsed_session);
    delete_session(ls);
    for (int f : filler)
        ::close(f);
    ::close(cfd);
}

/// pretend each byte is part of a longer stream that must stay ordered
static
uint8_t stream_byte(size_t i)
{
    return static_cast<uint8_t>((i * 2654435761u) >> 24);
}

static
void queue_bytes(Session *s, std::vector<uint8_t>& expected, size_t n)
{
    std::vector<Byte> chunk(n);
    for (size_t i = 0; i < n; i++)
        chunk[i] = Byte{stream_byte(expected.size() + i)};
    ASSERT_TRUE(packet_send(s, chunk.data(), n));
    for (size_t i = 0; i < n; i++)
        expected.push_back(stream_byte(expected.size()));
}

static
void drain_peer(io::FD peer, std::vector<uint8_t>& received)
{
    uint8_t buf[65536];
    ssize_t n;
    while ((n = ::recv(peer.uncast_dammit(), buf, sizeof(buf), 0)) > 0)
        received.insert(received.end(), buf, buf + n);
}

TEST(socket, wdata_ring)
{
    // a real connection, so send_from_fifo runs against a real socket
    io::FD lfd = io::FD::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_TRUE(lfd != io::FD());
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);
    ASSERT_EQ(lfd.bind(reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr)), 0);
    ASSERT_EQ(lfd.listen(1), 0);
    socklen_t alen = sizeof(addr);
    ASSERT_EQ(::getsockname(lfd.uncast_dammit(),
                reinterpret_cast<struct sockaddr *>(&addr), &alen), 0);
    uint16_t port = ntohs(addr.sin_port);

    Session *s = make_connection(IP4_LOCALHOST, port,
            SessionParsers{.func_parse= nullptr, .func_delete= test_delete});
    ASSERT_TRUE(s);

    // completing the accept means the nonblocking connect finished
    io::FD peer = lfd.accept(nullptr, nullptr);
    ASSERT_TRUE(peer != io::FD());
    peer.fcntl(F_SETFL, O_NONBLOCK);

    // small send buffer, so the queue drains only a little at a time
    const int sndbuf = 4096;
    s->fd.setsockopt(SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof sndbuf);

    const size_t capacity = 128 * 1024;
    realloc_fifo(s, 4096, capacity);
    ASSERT_EQ(s->max_wdata, capacity);

    std::vector<uint8_t> expected;
    std::vector<uint8_t> received;

    // fill the queue completely
    while (s->wdata_size < capacity)
        queue_bytes(s, expected,
                std::min(capacity - s->wdata_size, size_t(997)));
    ASSERT_EQ(s->wdata_size, capacity);

    // partial sends advance the head of the ring without moving data
    while (s->wdata_size == capacity)
    {
        ASSERT_TRUE(do_sendrecv(0_ms));
        drain_peer(peer, received);
    }
    ASSERT_GT(s->wdata_pos, 0u);
    ASSERT_GT(s->wdata_size, 0u);

    // refilling past the end wraps the tail of the ring
    size_t gap = s->wdata_pos;
    queue_bytes(s, expected, gap);
    ASSERT_EQ(s->wdata_size, capacity);
    ASSERT_GT(s->wdata_pos + s->wdata_size, s->max_wdata);

    // growing the queue while wrapped must not reorder the data
    realloc_fifo(s, 4096, 2 * capacity);
    ASSERT_EQ(s->max_wdata, 2 * capacity);
    queue_bytes(s, expected, capacity / 2);
    ASSERT_EQ(s->wdata_size, capacity + capacity / 2);

    // drain the rest; progress is guaranteed because the peer reads
    size_t prev = s->wdata_size + 1;
    while (s->wdata_size && s->wdata_size < prev)
    {
        prev = s->wdata_size;
        drain_peer(peer, received);
        ASSERT_TRUE(do_sendrecv(0_ms));
    }
    EXPECT_EQ(s->wdata_size, 0u);
    EXPECT_EQ(s->wdata_pos, 0u);

    // the last send() may still be in flight, keep reading until it lands
    for (int i = 0; i < 100 && received.size() < expected.size(); i++)
    {
        drain_peer(peer, received);
        usleep(1000);
    }

    ASSERT_EQ(received.size(), expected.size());
    EXPECT_EQ(received, expected);

    delete_session(s);
    peer.close();
    lfd.close();
}

TEST(socket, server_link_priority)
{
    parsed_bytes = 0;
    parsed_session = nullptr;
    parse_order.clear();
    Session *ls = make_listen_port(0,
            SessionParsers{.func_parse= test_parse, .func_delete= test_delete});
    ASSERT_NE(ls, nullptr);

    // accept the client first so it sits ahead of the link in live_fds
    int cfd_client = connect_to(ls);
    ASSERT_GE(cfd_client, 0);
    ASSERT_EQ(2, ::send(cfd_client, "cc", 2, 0));
    pump(4);
    Session *client_s = parsed_session;
    ASSERT_NE(client_s, nullptr);

    int cfd_link = connect_to(ls);
    ASSERT_GE(cfd_link, 0);
    ASSERT_EQ(2, ::send(cfd_link, "ll", 2, 0));
    pump(4);
    Session *link_s = parsed_session;
    ASSERT_NE(link_s, nullptr);
    ASSERT_NE(link_s, client_s);
    // same convention as the servers: an authenticated server peer
    // gets its fifos raised, which is what marks it as a link
    realloc_fifo(link_s, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

    // with nothing congested the link parses before the older client
    parse_order.clear();
    ASSERT_EQ(1, ::send(cfd_client, "c", 1, 0));
    ASSERT_EQ(1, ::send(cfd_link, "l", 1, 0));
    pump(4);
    ASSERT_EQ(2u, parse_order.size());
    EXPECT_EQ(link_s, parse_order[0]);
    EXPECT_EQ(client_s, parse_order[1]);

    // push the link's outbound queue well past the throttle depth,
    // leaving enough margin that the kernel send buffer absorbing a
    // few MiB cannot drop it back below; the peer does not read, so
    // it stays deep across the pumps below
    std::vector<Byte> chunk(65536, Byte{0x5a});
    while (link_s->wdata_size <= WFIFO_MAX_SERVERLINK / 2)
        ASSERT_TRUE(packet_send(link_s, chunk.data(), chunk.size()));

    // while the link is deep the client is neither read nor parsed,
    // even though input is waiting on it
    parse_order.clear();
    ASSERT_EQ(4, ::send(cfd_client, "zzzz", 4, 0));
    pump(4);
    EXPECT_TRUE(parse_order.empty());
    EXPECT_EQ(0u, client_s->rdata_size);

    // once the peer drains the link, the client gets serviced again
    ASSERT_EQ(0, fcntl(cfd_link, F_SETFL, O_NONBLOCK));
    uint8_t buf[65536];
    for (int i = 0; i < 1000 && link_s->wdata_size; i++)
    {
        ASSERT_TRUE(do_sendrecv(0_ms));
        while (::recv(cfd_link, buf, sizeof(buf), 0) > 0)
            ;
        // give the kernel a moment to ack what the peer just read
        usleep(1000);
    }
    EXPECT_EQ(0u, link_s->wdata_size);
    pump(4);
    ASSERT_FALSE(parse_order.empty());
    EXPECT_EQ(client_s, parse_order.back());

    delete_session(client_s);
    delete_session(link_s);
    delete_session(ls);
    ::close(cfd_client);
    ::close(cfd_link);
}
} // namespace tmwa
