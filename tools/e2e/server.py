#!/usr/bin/env python3
# server.py - orchestrates a throwaway tmwa world: generates a temp data
# directory (conf, dbs, a tiny walkable map, the fixture Lua content) and
# runs tmwa-login / tmwa-char / tmwa-map against it.

import os
import shutil
import signal
import socket
import struct
import subprocess
import time

E2E_DIR = os.path.dirname(os.path.abspath(__file__))

MAP_NAME = 'test'
MAP_W = 60
MAP_H = 60

LOGIN_MASTER_CONF = '''\
login_conf: conf/login.conf
login_lan_conf: conf/lan.conf
'''

LOGIN_CONF = '''\
login_port: {login_port}
new_account: yes
account_filename: save/account.txt
gm_account_filename: save/gm_account.txt
login_log_filename: log/login.log
userid: s1
passwd: p1
main_server: E2E Server
conn_limit_enable: no
'''

LAN_CONF = '''\
lan_char_ip: 127.0.0.1
lan_subnet: 127.0.0.1
'''

CHAR_MASTER_CONF = '''\
char_conf: conf/char.conf
char_lan_conf: conf/lan.conf
inter_conf: conf/inter.conf
'''

CHAR_CONF = '''\
userid: s1
passwd: p1
server_name: E2E Server
login_ip: 127.0.0.1
login_port: {login_port}
char_ip: 127.0.0.1
char_port: {char_port}
char_txt: save/athena.txt
char_log_filename: log/char.log
start_point: test.gat,30,30
char_name_letters: abcdefghijklmnopqrstuvwxyz
char_name_letters: ABCDEFGHIJKLMNOPQRSTUVWXYZ
char_name_letters: 0123456789
min_name_length: 4
char_slots: 9
max_hair_style: 20
max_hair_color: 13
min_stat_value: 1
max_stat_value: 10
total_stat_sum: 30
online_txt_filename: online.txt
online_html_filename: online.html
autosave_time: 30
'''

CHAR_LAN_CONF = '''\
lan_map_ip: 127.0.0.1
lan_subnet: 127.0.0.1
'''

INTER_CONF = '''\
storage_txt: save/storage.txt
party_txt: save/party.txt
accreg_txt: save/accreg.txt
party_share_level: 10
'''

MAP_MASTER_CONF = '''\
map_conf: conf/map.conf
const_db: db/params.txt
item_db: db/item_db.txt
mob_db: db/mob_db.txt
resnametable: data/resnametable.txt
'''

MAP_CONF = '''\
userid: s1
passwd: p1
char_ip: 127.0.0.1
char_port: {char_port}
map_ip: 127.0.0.1
map_port: {map_port}
autosave_time: 30
mapreg_txt: save/mapreg.txt
map: test
npc: npc/e2e.lua
'''


def make_wlk(width, height):
    """A .wlk: u16 xs, u16 ys, then xs*ys MapCell bytes (bit 0 = unwalkable).
    Walkable interior, one-cell unwalkable border."""
    cells = bytearray(width * height)
    for x in range(width):
        cells[x] = 1
        cells[(height - 1) * width + x] = 1
    for y in range(height):
        cells[y * width] = 1
        cells[y * width + width - 1] = 1
    return struct.pack('<HH', width, height) + bytes(cells)


class World(object):
    """A generated tmwa world in a temp dir plus its three server processes."""

    def __init__(self, build_dir, world_dir, base_port=16900, verbose=False):
        self.build_dir = os.path.abspath(build_dir)
        self.world_dir = os.path.abspath(world_dir)
        self.login_port = base_port + 1
        self.char_port = base_port + 2
        self.map_port = base_port + 3
        self.verbose = verbose
        self.procs = []
        self.logs = []

    # ----- filesystem -----------------------------------------------------

    def generate(self):
        w = self.world_dir
        if os.path.exists(w):
            shutil.rmtree(w)
        ports = {'login_port': self.login_port,
                 'char_port': self.char_port,
                 'map_port': self.map_port}

        login = os.path.join(w, 'login')
        for d in ('conf', 'save', 'log'):
            os.makedirs(os.path.join(login, d))
        self._write(login, 'conf/tmwa-login.conf', LOGIN_MASTER_CONF, ports)
        self._write(login, 'conf/login.conf', LOGIN_CONF, ports)
        self._write(login, 'conf/lan.conf', LAN_CONF, ports)

        world = os.path.join(w, 'world')
        for d in ('conf', 'save', 'log'):
            os.makedirs(os.path.join(world, d))
        self._write(world, 'conf/tmwa-char.conf', CHAR_MASTER_CONF, ports)
        self._write(world, 'conf/char.conf', CHAR_CONF, ports)
        self._write(world, 'conf/lan.conf', CHAR_LAN_CONF, ports)
        self._write(world, 'conf/inter.conf', INTER_CONF, ports)
        open(os.path.join(world, 'save/athena.txt'), 'w').close()

        mapd = os.path.join(w, 'world', 'map')
        for d in ('conf', 'save', 'log', 'data', 'db', 'npc'):
            os.makedirs(os.path.join(mapd, d))
        self._write(mapd, 'conf/tmwa-map.conf', MAP_MASTER_CONF, ports)
        self._write(mapd, 'conf/map.conf', MAP_CONF, ports)
        with open(os.path.join(mapd, 'data/resnametable.txt'), 'w') as f:
            f.write('%s#%s.wlk#\n' % (MAP_NAME, MAP_NAME))
        with open(os.path.join(mapd, 'data/%s.wlk' % MAP_NAME), 'wb') as f:
            f.write(make_wlk(MAP_W, MAP_H))
        for db in ('params.txt', 'item_db.txt', 'mob_db.txt'):
            shutil.copy(os.path.join(E2E_DIR, 'data', 'db', db),
                        os.path.join(mapd, 'db', db))
        shutil.copy(os.path.join(E2E_DIR, 'data', 'npc', 'e2e.lua'),
                    os.path.join(mapd, 'npc', 'e2e.lua'))

    def _write(self, base, rel, template, ports):
        with open(os.path.join(base, rel), 'w') as f:
            f.write(template.format(**ports))

    # ----- processes ------------------------------------------------------

    def _spawn(self, binary, cwd, logname):
        env = dict(os.environ)
        env['LD_LIBRARY_PATH'] = (self.build_dir + ':'
                                  + env.get('LD_LIBRARY_PATH', ''))
        logpath = os.path.join(self.world_dir, logname)
        logf = open(logpath, 'ab')
        proc = subprocess.Popen(
            [os.path.join(self.build_dir, binary)],
            cwd=cwd, env=env,
            stdout=logf, stderr=subprocess.STDOUT,
            start_new_session=True)
        self.procs.append(proc)
        self.logs.append(logpath)
        return proc

    def start(self):
        # fail fast on a port collision: a foreign listener would satisfy
        # _wait_port while our server exits with 'bind: Address already in
        # use', and two concurrent runs would cross-talk
        for port in (self.login_port, self.char_port, self.map_port):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # match the servers' own SO_REUSEADDR so lingering TIME_WAIT
            # connections from a previous run do not fail the probe
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                s.bind(('127.0.0.1', port))
            except OSError:
                raise RuntimeError('port %d already in use (set E2E_BASE_PORT '
                                   'to pick a free range)' % port)
            finally:
                s.close()
        w = self.world_dir
        self.login = self._spawn('tmwa-login', os.path.join(w, 'login'),
                                 'login.stdout.log')
        self._wait_port(self.login_port, 'tmwa-login', self.login)
        self.char = self._spawn('tmwa-char', os.path.join(w, 'world'),
                                'char.stdout.log')
        self._wait_port(self.char_port, 'tmwa-char', self.char)
        self.map = self._spawn('tmwa-map', os.path.join(w, 'world', 'map'),
                               'map.stdout.log')
        self._wait_port(self.map_port, 'tmwa-map', self.map)

    def _wait_port(self, port, name, proc, timeout=30.0):
        deadline = time.time() + timeout
        while time.time() < deadline:
            if proc.poll() is not None:
                raise RuntimeError('%s exited with code %d (see %s)'
                                   % (name, proc.returncode, self.logs[-1]))
            try:
                s = socket.create_connection(('127.0.0.1', port), timeout=0.5)
                s.close()
                # a bind failure can race the connect (something else may
                # own the port); re-check that the server is still alive
                time.sleep(0.3)
                if proc.poll() is not None:
                    raise RuntimeError('%s exited with code %d (see %s)'
                                       % (name, proc.returncode,
                                          self.logs[-1]))
                return
            except OSError:
                time.sleep(0.1)
        raise RuntimeError('%s did not start listening on port %d'
                           % (name, port))

    def alive(self):
        return all(p.poll() is None for p in self.procs)

    def stop(self):
        for p in reversed(self.procs):
            if p.poll() is None:
                try:
                    os.killpg(p.pid, signal.SIGTERM)
                except OSError:
                    pass
        deadline = time.time() + 5
        for p in self.procs:
            while p.poll() is None and time.time() < deadline:
                time.sleep(0.05)
            if p.poll() is None:
                try:
                    os.killpg(p.pid, signal.SIGKILL)
                except OSError:
                    pass
                p.wait()
        self.procs = []

    def dump_logs(self, tail=40):
        for path in self.logs:
            print('----- %s (last %d lines) -----' % (path, tail))
            try:
                with open(path, 'r', errors='replace') as f:
                    for line in f.readlines()[-tail:]:
                        print('    ' + line.rstrip())
            except OSError as e:
                print('    <%s>' % e)
