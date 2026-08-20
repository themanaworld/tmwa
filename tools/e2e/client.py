#!/usr/bin/env python3
# client.py - a minimal fake TMWA client speaking the old eAthena-derived
# protocol, sufficient to drive the login -> char -> map handshake and the
# NPC dialog / shop / storage / item packets used by the e2e scenarios.

import socket
import struct
import time

from packets import (PACKET_LENGTHS, VAR, Packet, fixed_string, pack_pos1)


class ClientError(Exception):
    pass


class Timeout(ClientError):
    pass


class Disconnected(ClientError):
    pass


# The map server rate-limits client packets per command id (see the
# clif_parse_func_table rates in src/map/clif.cpp) and silently DROPS
# packets that arrive faster; a scripted client easily trips this. Minimum
# spacing enforced between two sends of the same command, in seconds, a
# little above the server's rate:
SEND_SPACING = {
    0x0089: 1.1,    # action
    0x008c: 0.4,    # global chat (server rate 300 ms)
    0x0090: 0.6,    # npc click (server rate 500 ms)
    0x009f: 0.5,    # item pickup
    0x00bf: 1.1,    # emote
    0x0143: 0.4,    # npc int input (server rate 300 ms)
    0x0146: 0.4,    # npc close click (server rate 300 ms)
}
DEFAULT_SEND_SPACING = 0.15   # server default rate is 100 ms


class Connection(object):
    """A framed connection to one of the servers."""

    def __init__(self, host, port, label, log=None):
        self.label = label
        self.log = log
        self.sock = socket.create_connection((host, port), timeout=10)
        self.sock.settimeout(0.1)
        self.buf = b''
        self.queue = []      # parsed but unconsumed packets
        self.last_send = {}  # command id -> time of last send

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass

    def send(self, data):
        cmd = struct.unpack_from('<H', data)[0]
        spacing = SEND_SPACING.get(cmd, DEFAULT_SEND_SPACING)
        wait_until = self.last_send.get(cmd, 0) + spacing
        now = time.time()
        if wait_until > now:
            # keep reading while we pace, so nothing backs up
            end = wait_until
            while time.time() < end:
                self.poll()
        if self.log:
            self.log('%s >> 0x%04x (%d bytes)' % (self.label, cmd, len(data)))
        self.sock.sendall(data)
        self.last_send[cmd] = time.time()

    def read_raw(self, n, timeout=10.0):
        """Read exactly n raw bytes (outside packet framing)."""
        deadline = time.time() + timeout
        while len(self.buf) < n:
            self._fill(deadline)
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def _fill(self, deadline):
        if time.time() > deadline:
            raise Timeout('%s: timed out' % self.label)
        try:
            chunk = self.sock.recv(4096)
        except socket.timeout:
            return
        if not chunk:
            raise Disconnected('%s: server closed connection' % self.label)
        self.buf += chunk

    def _parse_one(self):
        """Parse one packet from self.buf, or return None."""
        if len(self.buf) < 2:
            return None
        pid = struct.unpack_from('<H', self.buf)[0]
        if pid not in PACKET_LENGTHS:
            raise ClientError('%s: unknown packet id 0x%04x (buf %s)'
                              % (self.label, pid, self.buf[:32].hex()))
        length = PACKET_LENGTHS[pid]
        if length == VAR:
            if len(self.buf) < 4:
                return None
            length = struct.unpack_from('<H', self.buf, 2)[0]
            if length < 4:
                raise ClientError('%s: bad var length %d for 0x%04x'
                                  % (self.label, length, pid))
        if len(self.buf) < length:
            return None
        data, self.buf = self.buf[:length], self.buf[length:]
        pkt = Packet(pid, data)
        if self.log:
            self.log('%s << 0x%04x (%d bytes)' % (self.label, pid, length))
        return pkt

    def poll(self):
        """Non-blocking-ish: pull whatever is available into the queue."""
        try:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise Disconnected('%s: server closed connection' % self.label)
            self.buf += chunk
        except socket.timeout:
            pass
        while True:
            pkt = self._parse_one()
            if pkt is None:
                break
            self.queue.append(pkt)

    def wait(self, pred, timeout=10.0, consume_all=False):
        """Return the first packet matching pred(pkt).

        Non-matching packets stay in the queue (they may be asserted on
        later) unless consume_all is set.
        """
        deadline = time.time() + timeout
        scanned = 0
        while True:
            while scanned < len(self.queue):
                pkt = self.queue[scanned]
                if pred(pkt):
                    del self.queue[scanned]
                    return pkt
                if consume_all:
                    del self.queue[scanned]
                else:
                    scanned += 1
            if time.time() > deadline:
                raise Timeout('%s: no matching packet within %.1fs '
                              '(queue: %s)'
                              % (self.label, timeout,
                                 ['0x%04x' % p.id for p in self.queue[-20:]]))
            self._fill(deadline)
            while True:
                pkt = self._parse_one()
                if pkt is None:
                    break
                self.queue.append(pkt)

    def expect(self, packet_id, timeout=10.0):
        return self.wait(lambda p: p.id == packet_id, timeout=timeout)

    def drain(self, duration=0.2):
        """Read packets for a fixed duration; keep them queued."""
        end = time.time() + duration
        while time.time() < end:
            self.poll()

    def flush_queue(self):
        self.queue = []


class TmwaClient(object):
    """Fake client. Holds the state of one logged-in character session."""

    CLIENT_VERSION = 6      # MIN_CLIENT_VERSION; >= 6 means no name prefix
                            # on global chat
    VERSION_2_FLAGS = 0x03  # accepts update host + server order

    def __init__(self, host, login_port, verbose=False):
        self.host = host
        self.login_port = login_port
        self.verbose = verbose
        self.account_id = None
        self.login_id1 = None
        self.login_id2 = None
        self.sex = None
        self.char_id = None
        self.char_name = None
        self.map_name = None
        self.x = self.y = 0
        self.map = None          # map server Connection
        self.npcs = {}           # name -> block id (learned via 0x0095)
        self.beings = {}         # block id -> dict from 0x0078/0x007c

    def log(self, msg):
        if self.verbose:
            print('    | %s' % msg)

    # ----- login server ---------------------------------------------------

    def login(self, username, password, timeout=10.0):
        """Authenticate; fills account ids and char server address."""
        conn = Connection(self.host, self.login_port, 'login', self.log)
        try:
            pkt = struct.pack('<HI', 0x0064, self.CLIENT_VERSION)
            pkt += fixed_string(username, 24)
            pkt += fixed_string(password, 24)
            pkt += struct.pack('<B', self.VERSION_2_FLAGS)
            conn.send(pkt)
            reply = conn.wait(lambda p: p.id in (0x0069, 0x006a, 0x0081),
                              timeout=timeout)
            if reply.id == 0x006a:
                raise ClientError('login refused: error code %d'
                                  % reply.u8(2))
            if reply.id == 0x0081:
                raise ClientError('login connection problem: code %d'
                                  % reply.u8(2))
            # 0x0069: head 47 bytes, then 32-byte char server entries
            self.login_id1 = reply.u32(4)
            self.account_id = reply.u32(8)
            self.login_id2 = reply.u32(12)
            self.sex = reply.u8(46)
            entry = reply.data[47:47 + 32]
            if len(entry) < 32:
                raise ClientError('no char server advertised')
            self.char_ip = socket.inet_ntoa(entry[0:4])
            self.char_port = struct.unpack_from('<H', entry, 4)[0]
        finally:
            conn.close()

    # ----- char server ----------------------------------------------------

    def char_login(self, char_name, timeout=10.0):
        """Connect to the char server; create the char if needed; select it.

        Fills char_id and the map server address.
        """
        conn = Connection('127.0.0.1', self.char_port, 'char', self.log)
        try:
            pkt = struct.pack('<HIIIHB', 0x0065, self.account_id,
                              self.login_id1, self.login_id2,
                              self.CLIENT_VERSION, self.sex)
            conn.send(pkt)
            # (the char server sends a 4-byte 0x8000 marker first; the
            # framing layer skips it via PACKET_LENGTHS)
            reply = conn.wait(lambda p: p.id in (0x006b, 0x006c),
                              timeout=timeout)
            if reply.id == 0x006c:
                raise ClientError('char server refused: code %d' % reply.u8(2))
            # 0x006b: head 24, then 106-byte char entries
            chars = []
            off = 24
            while off + 106 <= len(reply.data):
                entry = reply.data[off:off + 106]
                cid = struct.unpack_from('<I', entry, 0)[0]
                name = entry[74:98].split(b'\0')[0].decode('utf-8', 'replace')
                slot = entry[104]
                chars.append((cid, name, slot))
                off += 106
            slot = None
            for cid, name, cslot in chars:
                if name == char_name:
                    self.char_id = cid
                    slot = cslot
                    break
            if slot is None:
                slot = self._create_char(conn, char_name, timeout)
            # select
            conn.send(struct.pack('<HB', 0x0066, slot))
            reply = conn.wait(lambda p: p.id in (0x0071, 0x0081),
                              timeout=timeout)
            if reply.id == 0x0081:
                raise ClientError('char select refused: code %d'
                                  % reply.u8(2))
            self.char_id = reply.u32(2)
            self.map_name = reply.string(6, 16)
            self.map_ip = socket.inet_ntoa(reply.data[22:26])
            self.map_port = reply.u16(26)
            self.char_name = char_name
        finally:
            conn.close()

    def _create_char(self, conn, char_name, timeout):
        pkt = struct.pack('<H', 0x0067)
        pkt += fixed_string(char_name, 24)
        pkt += bytes([5, 5, 5, 5, 5, 5])          # str agi vit int dex luk
        pkt += struct.pack('<BHH', 0, 1, 1)       # slot, hair color, style
        conn.send(pkt)
        reply = conn.wait(lambda p: p.id in (0x006d, 0x006e), timeout=timeout)
        if reply.id == 0x006e:
            raise ClientError('char creation failed: code %d' % reply.u8(2))
        self.char_id = reply.u32(2)
        return 0

    # ----- map server -----------------------------------------------------

    def map_login(self, timeout=10.0):
        """Connect to the map server and finish loading the map."""
        self.map = Connection('127.0.0.1', self.map_port, 'map', self.log)
        pkt = struct.pack('<HIIIIB', 0x0072, self.account_id, self.char_id,
                          self.login_id1, int(time.time() * 1000) & 0xffffffff,
                          self.sex)
        self.map.send(pkt)
        reply = self.map.wait(lambda p: p.id in (0x0073, 0x0081),
                              timeout=timeout)
        if reply.id == 0x0081:
            raise ClientError('map connect refused: code %d' % reply.u8(2))
        self.x, self.y, _d = reply.pos1(6)
        self.map_loaded()

    def map_loaded(self):
        """Send the LoadEndAck (also required after every same-server warp)."""
        self.map.send(struct.pack('<H', 0x007d))

    def full_login(self, username, password, char_name, timeout=10.0):
        self.login(username, password, timeout=timeout)
        self.char_login(char_name, timeout=timeout)
        self.map_login(timeout=timeout)

    def disconnect(self):
        if self.map is not None:
            self.map.close()
            self.map = None

    # ----- being bookkeeping ---------------------------------------------

    def scan_beings(self, duration=0.5):
        """Record all beings seen via 0x0078/0x007c so far (plus a drain)."""
        self.map.drain(duration)
        for pkt in self.map.queue:
            if pkt.id == 0x0078:
                bid = pkt.u32(2)
                self.beings[bid] = {'species': pkt.u16(14),
                                    'x': pkt.pos1(46)[0],
                                    'y': pkt.pos1(46)[1]}
            elif pkt.id == 0x007c:
                bid = pkt.u32(2)
                self.beings[bid] = {'species': pkt.u16(20),
                                    'x': pkt.pos1(36)[0],
                                    'y': pkt.pos1(36)[1]}

    def resolve_name(self, block_id, timeout=5.0):
        self.map.send(struct.pack('<HI', 0x0094, block_id))
        pkt = self.map.wait(
            lambda p: p.id == 0x0095 and p.u32(2) == block_id,
            timeout=timeout)
        return pkt.string(6, 24)

    def find_npc(self, name, timeout=5.0):
        """Find an NPC's block id by resolving names of the beings seen."""
        if name in self.npcs:
            return self.npcs[name]
        self.scan_beings(0.3)
        for bid in sorted(self.beings):
            if bid in self.npcs.values():
                continue
            try:
                bname = self.resolve_name(bid, timeout=2.0)
            except Timeout:
                continue
            self.npcs[bname] = bid
            if bname == name:
                return bid
        raise ClientError('NPC %r not found (beings: %s)'
                          % (name, self.beings))

    # ----- dialog verbs ---------------------------------------------------

    def click_npc(self, npc_id):
        self.map.send(struct.pack('<HIB', 0x0090, npc_id, 0))

    def next(self, npc_id):
        self.map.send(struct.pack('<HI', 0x00b9, npc_id))

    def menu(self, npc_id, choice):
        self.map.send(struct.pack('<HIB', 0x00b8, npc_id, choice))

    def input_int(self, npc_id, value):
        self.map.send(struct.pack('<HIi', 0x0143, npc_id, value))

    def input_str(self, npc_id, text):
        b = text.encode('utf-8') + b'\0'
        self.map.send(struct.pack('<HHI', 0x01d5, 8 + len(b), npc_id) + b)

    def close_dialog(self, npc_id):
        self.map.send(struct.pack('<HI', 0x0146, npc_id))

    # ----- other client actions -------------------------------------------

    def say(self, text):
        b = text.encode('utf-8')
        self.map.send(struct.pack('<HH', 0x008c, 4 + len(b)) + b)

    def walk_to(self, x, y):
        self.map.send(struct.pack('<H', 0x0085) + pack_pos1(x, y, 0))

    def buy_sell(self, npc_id, buy):
        self.map.send(struct.pack('<HIB', 0x00c5, npc_id, 0 if buy else 1))

    def buy(self, items):
        """items: list of (count, item_id)."""
        body = b''.join(struct.pack('<HH', c, i) for c, i in items)
        self.map.send(struct.pack('<HH', 0x00c8, 4 + len(body)) + body)

    def sell(self, items):
        """items: list of (inventory_index, count); index is the raw 0-based
        index (ioff2 conversion applied here)."""
        body = b''.join(struct.pack('<HH', idx + 2, c) for idx, c in items)
        self.map.send(struct.pack('<HH', 0x00c9, 4 + len(body)) + body)

    def use_item(self, index):
        self.map.send(struct.pack('<HHI', 0x00a7, index + 2, 0))

    def equip_item(self, index, epos=0):
        self.map.send(struct.pack('<HHH', 0x00a9, index + 2, epos))

    def unequip_item(self, index):
        self.map.send(struct.pack('<HH', 0x00ab, index + 2))

    def storage_close(self):
        self.map.send(struct.pack('<H', 0x00f7))

    # ----- assertions helpers ---------------------------------------------

    def wait_npc_mes(self, text, npc_id=None, timeout=10.0):
        """Wait for an 0x00b4 whose text contains `text`."""
        def pred(p):
            if p.id != 0x00b4:
                return False
            if npc_id is not None and p.u32(4) != npc_id:
                return False
            return text in p.tail(8).rstrip(b'\0').decode('utf-8', 'replace')
        return self.map.wait(pred, timeout=timeout)

    def wait_chat(self, text, timeout=10.0):
        """Wait for a 0x008e (self chat / server message) containing text."""
        def pred(p):
            return (p.id == 0x008e
                    and text in p.tail(4).rstrip(b'\0').decode('utf-8',
                                                               'replace'))
        return self.map.wait(pred, timeout=timeout)

    def wait_gm_chat(self, text, timeout=10.0):
        def pred(p):
            return (p.id == 0x009a
                    and text in p.tail(4).rstrip(b'\0').decode('utf-8',
                                                               'replace'))
        return self.map.wait(pred, timeout=timeout)
