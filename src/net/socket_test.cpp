#include "socket.hpp"
//    socket_test.cpp - Testsuite for the network event system.
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

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "../wire/packets.hpp"

#include "../poison.hpp"


namespace tmwa
{
// defined per-server, so the test provides its own
void SessionDeleter::operator()(SessionData *sd)
{
    really_delete1 sd;
}

static
void test_delete(Session *s)
{
    (void)s;
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
} // namespace tmwa
