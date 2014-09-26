
"""The PacketBuffer class has been adapted from source originally released by gnufrk"""

import struct

packet_lengths = [
   10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
#0x0040
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  50,  3, -1, 55, 17,  3, 37, 46, -1, 23, -1,  3,108,  3,  2,
    3, 28, 19, 11,  3, -1,  9,  5, 54, 53, 58, 60, 41,  2,  6,  6,
#0x0080
    7,  3,  2,  2,  2,  5, 16, 12, 10,  7, 29, 23, -1, -1, -1,  0,
    7, 22, 28,  2,  6, 30, -1, -1,  3, -1, -1,  5,  9, 17, 17,  6,
   23,  6,  6, -1, -1, -1, -1,  8,  7,  6,  7,  4,  7,  0, -1,  6,
    8,  8,  3,  3, -1,  6,  6, -1,  7,  6,  2,  5,  6, 44,  5,  3,
#0x00C0
    7,  2,  6,  8,  6,  7, -1, -1, -1, -1,  3,  3,  6,  6,  2, 27,
    3,  4,  4,  2, -1, -1,  3, -1,  6, 14,  3, -1, 28, 29, -1, -1,
   30, 30, 26,  2,  6, 26,  3,  3,  8, 19,  5,  2,  3,  2,  2,  2,
    3,  2,  6,  8, 21,  8,  8,  2,  2, 26,  3, -1,  6, 27, 30, 10,
#0x0100
    2,  6,  6, 30, 79, 31, 10, 10, -1, -1,  4,  6,  6,  2, 11, -1,
   10, 39,  4, 10, 31, 35, 10, 18,  2, 13, 15, 20, 68,  2,  3, 16,
    6, 14, -1, -1, 21,  8,  8,  8,  8,  8,  2,  2,  3,  4,  2, -1,
    6, 86,  6, -1, -1,  7, -1,  6,  3, 16,  4,  4,  4,  6, 24, 26,
#0x0140
   22, 14,  6, 10, 23, 19,  6, 39,  8,  9,  6, 27, -1,  2,  6,  6,
  110,  6, -1, -1, -1, -1, -1,  6, -1, 54, 66, 54, 90, 42,  6, 42,
   -1, -1, -1, -1, -1, 30, -1,  3, 14,  3, 30, 10, 43, 14,186,182,
   14, 30, 10,  3, -1,  6,106, -1,  4,  5,  4, -1,  6,  7, -1, -1,
#0x0180
    6,  3,106, 10, 10, 34,  0,  6,  8,  4,  4,  4, 29, -1, 10,  6,
   90, 86, 24,  6, 30,102,  9,  4,  8,  4, 14, 10,  4,  6,  2,  6,
    3,  3, 35,  5, 11, 26, -1,  4,  4,  6, 10, 12,  6, -1,  4,  4,
   11,  7, -1, 67, 12, 18,114,  6,  3,  6, 26, 26, 26, 26,  2,  3,
#0x01C0
    2, 14, 10, -1, 22, 22,  4,  2, 13, 97,  0,  9,  9, 29,  6, 28,
    8, 14, 10, 35,  6,  8,  4, 11, 54, 53, 60,  2, -1, 47, 33,  6,
   30,  8, 34, 14,  2,  6, 26,  2, 28, 81,  6, 10, 26,  2, -1, -1,
   -1, -1, 20, 10, 32,  9, 34, 14,  2,  6, 48, 56, -1,  4,  5, 10,
#0x2000
   26,  0,  0,  0, 18,  0,  0,  0,  0,  0,  0, 19,  10,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
]

class PacketOut:
    def __init__(self, out):
        self.buff = ""
        self.write_int16(out)

    def __str__(self):
        return self.buff

    def write_string(self, string_val, length):
        self.buff += string_val.ljust(length, '\0')

    def write_int8(self, value):
        self.buff += struct.pack("<B", value)

    def write_int16(self, value):
        self.buff += struct.pack("<H", value)

    def write_int32(self, value):
        self.buff += struct.pack("<L", value)

    def write_coords(self, x, y, direction):
        tmp = x
        tmp <<= 6
        d_0 = 0
        d_1 = 1
        d_2 = 2
        d_0 = (tmp >> 8) % 256
        d_1 = (tmp) % 256
        tmp = y
        tmp <<= 4
        d_1 |= (tmp >> 8) % 256
        d_2 = tmp % 256
        d_2 |= direction
        self.buff += chr(d_0) + chr(d_1) + chr(d_2)

class PacketIn:
    def __init__(self, set_data, pkt_type):
        self.data = set_data
        self.pkttype = pkt_type
        self.pos = 0

    def is_type(self, pkt_type):
        return self.pkttype == pkt_type

    def get_type(self):
        return self.pkttype

    def read_string(self, length):
        msg = self.data[self.pos:self.pos + length]
        self.pos = self.pos + length
        return msg[:msg.find('\0')]

    def read_raw_string(self, length):
        msg = self.data[self.pos:self.pos + length]
        self.pos = self.pos + length
        return msg

    def read_int8(self):
        int_value = struct.unpack("<B", self.data[self.pos:self.pos + 1])[0]
        self.pos = self.pos + 1
        return int_value

    def make_word(self, low, high):
        return (low | (high << 8))

    def read_coord_pair(self):
        cdata = self.data[self.pos:self.pos + 5]
        dst_x = (self.make_word(struct.unpack("<B", cdata[3])[0], struct.unpack("<B", cdata[2])[0] & 0x000f) >> 2)
        dst_y = self.make_word(struct.unpack("<B", cdata[4])[0], struct.unpack("<B", cdata[3])[0] & 0x0003)

        src_x = (self.make_word(struct.unpack("<B", cdata[1])[0], struct.unpack("<B", cdata[0])[0]) >> 6)
        src_y = (self.make_word(struct.unpack("<B", cdata[2])[0], struct.unpack("<B", cdata[1])[0] & 0x003f) >> 4)
        self.pos = self.pos + 5
        return src_x, src_y, dst_x, dst_y

    def read_coord_dir(self):
        cdata = self.data[self.pos:self.pos + 3]
        x = (self.make_word(struct.unpack("<B", cdata[1])[0] & 0x00c0, struct.unpack("<B", cdata[0])[0] & 0x00ff) >> 6) % 255
        y = (self.make_word(struct.unpack("<B", cdata[2])[0] & 0x00f0, struct.unpack("<B", cdata[1])[0] & 0x003f) >> 4) % 255
        dir = struct.unpack("<B", cdata[2])[0] & 0x000f
        self.pos = self.pos + 3
        return x, y, dir

    def read_int16(self):
        int_value = struct.unpack("<H", self.data[self.pos:self.pos + 2])[0]
        self.pos = self.pos + 2
        return int_value

    def read_int32(self):
        int_value = struct.unpack("<L", self.data[self.pos:self.pos + 4])[0]
        self.pos = self.pos + 4
        return int_value

    def skip(self, count):
        self.pos = self.pos + count

class PacketBuffer:
    def __init__(self):
        self.buff = ""

    def feed(self, data):
        self.buff += data

    def drop(self, count):
        self.buff = self.buff[count:]

    def __iter__(self):
        return self

    def next(self):
        if len(self.buff) < 2:
            raise StopIteration

        pktlen = 0
        pkttype = struct.unpack("<H", self.buff[:2])[0]
        assert pkttype < len(packet_lengths)
        assert packet_lengths[pkttype] != 0

        if packet_lengths[pkttype] < 0:
            if len(self.buff) < 4:
                raise StopIteration
            pktlen = struct.unpack("<H", self.buff[2:4])[0]
            assert pktlen >= 4
        else:
            pktlen = packet_lengths[pkttype]

        if len(self.buff) < pktlen:
            raise StopIteration

        packet = PacketIn(self.buff[2:pktlen], pkttype)
        self.buff = self.buff[pktlen:]
        return packet
