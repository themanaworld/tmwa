#include "packets.hpp"
//    packets.cpp - palatable socket buffer accessors
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

#include "../io/cxxstdio.hpp"
#include "../io/write.hpp"

#include "../poison.hpp"


namespace tmwa
{
size_t packet_avail(Session *s)
{
    return s->rdata_size - s->rdata_pos;
}

bool packet_fetch(Session *s, size_t offset, Byte *data, size_t sz)
{
    if (packet_avail(s) < offset + sz)
        return false;
    const Byte *start = reinterpret_cast<const Byte *>(&s->rdata[s->rdata_pos + offset]);
    const Byte *end = start + sz;
    std::copy(start, end, data);
    return true;
}
void packet_discard(Session *s, size_t sz)
{
    s->rdata_pos += sz;

    assert (s->rdata_size >= s->rdata_pos);
}
bool packet_send(Session *s, const Byte *data, size_t sz)
{
    if (s->wdata_size + sz > s->max_wdata)
    {
        realloc_fifo(s, s->max_rdata, s->max_wdata << 1);
        PRINTF("socket: %d wdata expanded to %zu bytes.\n"_fmt, s, s->max_wdata);
    }
    if (!s->max_wdata || !s->wdata)
    {
        return false;
    }
    s->wdata_size += sz;

    Byte *end = reinterpret_cast<Byte *>(&s->wdata[s->wdata_size + 0]);
    Byte *start = end - sz;
    std::copy(data, data + sz, start);
    return true;
}

void packet_dump(Session *s)
{
    FPRINTF(stderr,
            "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F\n"_fmt);
    char tmpstr[16 + 1] {};
    int i;
    for (i = 0; i < packet_avail(s); i++)
    {
        if ((i & 15) == 0)
            FPRINTF(stderr, "%04X "_fmt, i);
        Byte rfifob_ib;
        packet_fetch(s, i, &rfifob_ib, 1);
        uint8_t rfifob_i = rfifob_ib.value;
        FPRINTF(stderr, "%02x "_fmt, rfifob_i);
        if (rfifob_i > 0x1f)
            tmpstr[i % 16] = rfifob_i;
        else
            tmpstr[i % 16] = '.';
        if ((i - 7) % 16 == 0)  // -8 + 1
            FPRINTF(stderr, " "_fmt);
        else if ((i + 1) % 16 == 0)
        {
            FPRINTF(stderr, " %s\n"_fmt, tmpstr);
            std::fill(tmpstr + 0, tmpstr + 17, '\0');
        }
    }
    if (i % 16 != 0)
    {
        for (int j = i; j % 16 != 0; j++)
        {
            FPRINTF(stderr, "   "_fmt);
            if ((j - 7) % 16 == 0)  // -8 + 1
                FPRINTF(stderr, " "_fmt);
        }
        FPRINTF(stderr, " %s\n"_fmt, tmpstr);
    }
    FPRINTF(stderr, "\n"_fmt);
}
} // namespace tmwa
