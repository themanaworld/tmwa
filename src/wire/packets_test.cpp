#include "packets.hpp"
//    packets_test.cpp - Testsuite for socket buffer accessors
//
//    Copyright © 2025 The Mana World Development Team
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

#include <gtest/gtest.h>

#include "../mmo/consts.hpp"

#include "../poison.hpp"


namespace tmwa
{
// sessions in this test carry no SessionData
void SessionDeleter::operator()(SessionData *)
{
}

static
void init_fifo(Session& s, size_t fifo_size)
{
    s.wdata.new_(fifo_size);
    s.max_rdata = fifo_size;
    s.max_wdata = fifo_size;
}

TEST(packets, send_grows_fifo)
{
    Session s(SessionIO{nullptr, nullptr}, SessionParsers{nullptr, nullptr});
    init_fifo(s, 64 * 1024);

    Byte data[4096] {};
    // 160 KiB total, forces the fifo to grow past its initial size
    for (int i = 0; i < 40; i++)
        EXPECT_TRUE(packet_send(&s, data, sizeof(data)));
    EXPECT_EQ(s.wdata_size, 40 * sizeof(data));
    EXPECT_GE(s.max_wdata, s.wdata_size);
    EXPECT_FALSE(s.is_eof());
    s.wdata.delete_();
}

TEST(packets, send_caps_client_fifo)
{
    Session s(SessionIO{nullptr, nullptr}, SessionParsers{nullptr, nullptr});
    init_fifo(s, 64 * 1024);

    Byte data[65536] {};
    // 8 MiB exceeds WFIFO_MAX, so this must be refused
    bool ok = true;
    for (int i = 0; i < 128 && ok; i++)
        ok = packet_send(&s, data, sizeof(data));
    EXPECT_FALSE(ok);
    EXPECT_TRUE(s.is_eof());
    EXPECT_EQ(s.max_wdata, WFIFO_MAX);
    EXPECT_LE(s.wdata_size, WFIFO_MAX);
    s.wdata.delete_();
}

TEST(packets, send_caps_serverlink_fifo)
{
    Session s(SessionIO{nullptr, nullptr}, SessionParsers{nullptr, nullptr});
    init_fifo(s, FIFOSIZE_SERVERLINK);

    Byte data[65536] {};
    // server links may exceed the client cap ...
    for (int i = 0; i < 96; i++)
        ASSERT_TRUE(packet_send(&s, data, sizeof(data)));
    EXPECT_EQ(s.wdata_size, 96 * sizeof(data));
    EXPECT_FALSE(s.is_eof());
    // ... but still bounded at WFIFO_MAX_SERVERLINK (32 MiB)
    bool ok = true;
    for (int i = 0; i < 512 && ok; i++)
        ok = packet_send(&s, data, sizeof(data));
    EXPECT_FALSE(ok);
    EXPECT_TRUE(s.is_eof());
    EXPECT_EQ(s.max_wdata, WFIFO_MAX_SERVERLINK);
    EXPECT_LE(s.wdata_size, WFIFO_MAX_SERVERLINK);
    s.wdata.delete_();
}
} // namespace tmwa
