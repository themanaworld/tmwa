# tmwa-gate

tmwa-gate is a Rust server that replaces `tmwa-login`, `tmwa-char`,
`tmwa-admin` and the TMWA part of tmw-api. It is the login and character
server, the inter-server router, and the WebSocket transport. TCP
clients connect to `tmwa-map` directly once their character is picked,
so a map server can be replaced under them (blue-green) without the gate
ever carrying the traffic itself.

Status: implemented. The parts that are not (multiple simultaneous map
worlds, the HTTP API surface) say so below.

## Overview

```
  Mana / ManaPlus / Manaverse (TCP)      Mana wasm build (WebSocket)
       |                                        |
       | login + char select                    | login + char + map
       |                                        | (relay)
       v                                        v
  +-------------------------------------------------+
  |                   tmwa-gate                     |
  |  login + char screens          char server for  |   tmwa-gate admin ...
  |  SQLite                        tmwa-map, HTTP:  | <- (CLI over Unix socket)
  |                                /api/tmwa, WS    |
  +--------+-------------------------+--------------+
           |                         |
           | 0x0071 advertises       | inter-server link
           | the map's address       v
           +--------------->  tmwa-map [tmwa-map ...]
                (client opens a direct TCP connection)
```

- TCP clients connect to the gate for login and character select. On
  select the gate's `0x0071` carries the registered address of the
  `tmwa-map` instance that owns the map, and the client opens a direct
  TCP connection to it. The gate never sees game traffic on this path.
- WebSocket clients keep the old shape: browsers can't open raw TCP
  sockets, so the gate relays the map stage per player over a separate
  upstream connection to `tmwa-map`. On this path the gate only frames
  packets; it parses the few it needs (see "Map restarts").
- Towards `tmwa-map` the gate plays the char server role on the existing
  inter-server protocol (`0x2af8`...`0x3830`, defined in
  `tools/protocol.py`). `tmwa-map` does not know it is not talking to
  `tmwa-char`, apart from the small changes listed under "Changes to
  tmwa-map".

## Client connections

- **One TCP port** for login and char. The purpose of a connection
  follows from its first packet: `0x0064` (login), `0x0065` (char),
  optionally preceded by `0x7530` (version). A `0x0072` (map enter) on
  this listener is a leftover of the old relay topology: the gate logs
  once and closes.
- **One WebSocket endpoint** (for example `wss://server.themanaworld.org/tmwa`)
  for the wasm client. Binary frames carry the same byte stream as TCP;
  frame boundaries are not significant. The `binary` subprotocol is
  accepted. The Mana client connects to the configured URL as is (it
  currently appends `/<host>/<port>`, which is dropped). The map stage
  is relayed (see above).
- Limits: maximum connections, idle timeout for connections that have not
  logged in, per-IP login throttling.
- **Real client IP:** taken from the socket, or from `X-Forwarded-For`
  when the WebSocket request comes from a trusted proxy (Caddy on
  loopback). It is passed to `tmwa-map` in `0x3829` (see below), which
  the map trusts only from `trusted_proxy_ip`.

## Login and characters

Ported from `src/login/login.cpp` and `src/char/char.cpp`, without the
login/char link (`parse_fromchar`, `parse_tologin`), which disappears.

- Login: password check, `_M`/`_F` registration suffix (registers when
  `new_account` is enabled; the letter is ignored), bans and blocks,
  `update_host` (`0x0063`), GM level.
- There is no account sex. The sex byte in `0x0069` is sent as a fixed
  value. Characters keep their own sex.
- Characters: list, create, delete, select, as the char server does now,
  including name rules and starting values from the current config.
- Parties, storage and account variables (`#` and `##`), ported from
  `src/char/int_party.cpp`, `int_storage.cpp` and `inter.cpp`.
- Whisper and GM broadcast routing and the online list (`0x2aff`).
- `online.txt` and `online.html` keep being written to the public dir,
  in the current format; `tmw-online-exporter` and the public player list
  on server.themanaworld.org read them.

## Map restarts

The gate owns map authentication: it pushes `0x3829` (pre-auth) to
`tmwa-map` whenever it wants a player to be accepted, so it can log a
player in again at any time without the client.

What happens on a restart depends on the transport and on whether the
restart is planned:

- **TCP players** hold a direct socket to `tmwa-map`. If the map dies
  they are disconnected, like today — but they land in-game again as
  soon as they log back in, on whatever server their map is on.
- **WebSocket players** are held and rejoined by the gate (the client
  sees a `0x0091` to the same map), because a browser client has no
  way to switch servers.
- **A planned restart** is blue-green: a second `tmwa-map` instance is
  started first, the old one is drained (`tmwa-gate admin drain`), and
  every player is handed over through the ordinary cross-server move,
  so nobody is disconnected at all.

### Hold and rejoin (WebSocket)

1. **Hold.** When the map link drops — or `tmwa-map` announces its
   shutdown on the link (`0x2b17`, sent by `term_func`) — the gate keeps
   the WebSocket connection, answers `0x007e` with `0x007f`, drops other
   input and sends one `0x009a` announcement. A single player's upstream
   close while the map stays up (`@kick`, over the fd softlimit, double
   login) is passed through as a client close instead.
2. **Reconnect.** When `tmwa-map` connects again (`0x2af8`, map list
   `0x2afa`), the gate pushes `0x3829` for each held player and opens a
   new upstream connection with `0x0072`, answering `0x2afc` with the
   stored character (`0x2afd`).
3. **Resync the client.** The gate drops the `0x8000` and `0x0073`
   replies and sends the client `0x0091` (change map) to the saved map
   and position. The client clears all beings, reloads the map and sends
   `0x007d`, which the gate forwards. The login burst `tmwa-map` sends
   after auth (stats, inventory, equipment, skills) is forwarded as is.
4. Also sent to the client where needed: trade cancelled (`0x00ee`),
   storage closed (`0x00f8`).

The hold has a bounded timeout (`hold_timeout` in the config): players
on a map that never comes back are dropped.

### Blue-green drain

`tmwa-gate admin drain <id>` (or `drain` for all) evacuates a running
`tmwa-map`:

1. The slot is marked **draining**. New char selects skip it
   (`map_for` prefers non-draining servers) and in-flight selects that
   targeted it resolve as usual — their `0x3829` still goes out, and if
   the map goes away before answering, the pending select completes
   against whatever remains.
2. The gate re-broadcasts `0x2b04` for every map name the target
   serves, pointing at a surviving server that also serves the name.
   The receiving maps record these as *shadow* announcements
   (`map_shadow_db`): the map keeps its local entry, but
   `map_otheripport` now resolves the name to the survivor. Names with
   no survivor are reported as stragglers.
3. The gate sends `0x382a` (evacuate) to the target. `tmwa-map` walks
   its online players through `pc_evacuate`, which resolves the
   player's current map through the shadow table and runs the ordinary
   `pc_changeserver` path: save (`0x2b01`), `0x2b05` ask, gate answers
   `0x2b06`, map sends the client `0x0092` naming the survivor. The
   gate paces the `0x2b06` replies at `gate.evacuate_per_second`
   (default 50; `admin drain --rate` overrides): each reply releases
   one client's `0x0092`, so the destination sees logins no faster
   than that during a drain. While the map stays draining and
   nonempty the gate re-sends `0x382a` every couple of seconds, so
   players that land after the first pass (in-flight logins, relayed
   rejoins) are evacuated too; map-side, sessions already moving are
   skipped, so re-sweeps only pick up the tail.
4. TCP clients open a direct connection to the survivor; WebSocket
   clients reconnect to the gate, which re-auths them and relays them
   onto the survivor. In both cases the character save is ordered:
   the gate tracks the transfer (`0x2b05`) and the in-flight saves
   (`0x2b01`) so a `0x2afc` on the new link waits briefly for the old
   link's save to commit.
5. `drain --wait` blocks until the target reports zero users on the
   link or the link drops (bounded at 60 s plus the paced drain
   time, so a slower `--rate` does not truncate the wait), then
   waits up to ~5 s more for in-flight handoffs to resolve. The reply counts the
   players that were on the target when the drain began
   (`evacuees`), how many of those then authenticated on another
   server (`arrived`, confirmed by the destination link's `0x2afc`),
   how many are still there (`still_on_source`), and the rest
   (`departed` — logged out or failed to land). `emptied` only
   means the source is empty; a large `departed` means the evacuees
   did not land.

Lost across a restart, by nature: floor items, monster positions,
temporary `@` variables, `addtimer` timers, open NPC dialogs, open trades.
`OnPCLoginEvent` runs again for every player (MOTD, broadcast,
`VaultLogin`/`VaultLogout`, magic timer, birthday); the scripts in
`serverdata/world/map/npc/functions/global_event_handler.txt` need a
review, or a variable telling them it is a reconnect.

Restarting the gate itself still disconnects everyone. Handing live
sockets over to a new gate process is possible later, but not planned for
the first version.

A throwaway spike (a proxy in front of unmodified tmwa that logs in
again upstream and splices the new map session) confirmed the resync
with the Mana desktop client: after `0x0091` it reloads the map, drops
the old monsters, takes the new inventory and equipment (sent after
`0x007d`), and walking, chat and NPCs keep working, across repeated
restarts with SIGTERM and SIGKILL. Findings to carry over:

- An NPC dialog that is open during the restart stays open but can't be
  advanced, because the new session has no NPC state; it closes with its
  close button, and the next NPC works. The gate closes it on resync.
- Every upstream step of a reconnect needs a timeout; a hung `tmwa-map`
  otherwise leaves players waiting forever.
- `tmwa-map` took about 14 s to accept players again (about 60 s with a
  cold page cache).

## Multiple map servers

Several `tmwa-map` processes can serve the same world, or split it.
Two ways this is used:

- **Blue-green replacement:** a second instance loads the same maps;
  `drain` hands the players over and the old one exits. This is how
  `tmwa-map` restarts without disconnecting players.
- **Sharding** (not production yet): each process serves a different
  set of maps, to use more than one CPU.

Routing:

- Each `tmwa-map` connects to the gate and reports its maps (`0x2afa`).
  The gate keeps a map name to server table — the last registered
  server wins for a duplicated name — and sends each server the maps of
  the others (`0x2b04`), so `tmwa-map`'s existing remote map handling
  works unchanged.
- **Warping to a map on another server:** `tmwa-map` saves the
  character (`0x2b01`) and asks for a server change (`0x2b05`); the
  gate answers as the char server does (`0x2b06`), after which
  `tmwa-map` sends the client `0x0092` (change map server). A TCP
  client connects to the address `0x0092` names. A WebSocket client is
  relayed: the gate intercepts `0x0092`, opens a new upstream
  connection to the target server (re-pushing `0x3829` if the entry
  was already consumed), and the client reconnects with `0x0072`
  through the same WebSocket. If the target map's `0x0072` lands
  before the pre-auth it is retried once with a fresh `0x3829`.
- Whispers, party messages and GM broadcasts are routed between servers
  by the gate, as `tmwa-char` does now (the inter-server packets were
  made for this).
- A char select waits for every connected server's `0x3830` like
  `tmwa-char` does (the map list can be rewritten between them), but
  only for links the pre-auth actually reached; a congested or dead
  link stops blocking the select instead of hanging it.

The limits are in `tmwa-map` and serverdata, not in the gate. State that
is global today becomes per process:

- `$` variables (each process has its own `mapreg.txt`).
- Global NPC events and timers (`OnInit`, `OnClock`/`OnDay`, floating
  NPCs) run on every server.
- Lookups by player name (`map_nick2sd`: `isloggedin`, `@recall`, `@kick`,
  `@where` and similar) and the online list only see local players.
- NPCs on a map the process doesn't load are an error ("Map not found"),
  so the script list must be split per server, or `tmwa-map` must skip
  them.

So running more than one map server in production needs work in
`tmwa-map` and serverdata first. Before that, measure whether `tmwa-map`
is actually CPU bound under load. The gate supports several map servers
from the start, since it only adds a routing table.

## Changes to tmwa-map

Small, and compatible with `tmwa-char` where possible:

- `0x3829` carries the real client IP, and `clif_parse_WantToConnection`
  uses it instead of the socket address for the auth check, logging,
  `@ip` and IP bans, gated on `trusted_proxy_ip`.
- `0x2b04` announcements for a map the server hosts itself used to be
  an error; they now populate `map_shadow_db` so `map_otheripport`
  resolves them for a drain. An announcement pointing back at the
  server itself clears the shadow entry.
- `0x382a` asks the server to evacuate: `map_evacuate` walks all
  authenticated sessions and `pc_evacuate` hands each to the server the
  shadow table names, through the ordinary `pc_changeserver` path
  (shared with `pc_setpos`).
- On SIGTERM, save all players (`0x2b01`) and flush before exiting, and
  announce the shutdown (`0x2b17`) so the gate can hold WS players
  immediately.
- `TMWA_ALLOW_ROOT=1` lets the binary run as uid 0 for rootless test
  deploys where the image happens to be root.
- Optionally, a flag in `0x2afd` that exposes "reconnect" to scripts.

## Storage

SQLite from the start (WAL mode), one database file, accessed with
rusqlite (bundled SQLite) from blocking tasks. Schema versions via
`PRAGMA user_version` and embedded migrations. Ids are kept from tmwa
(account ids, char ids, party ids), so logs and GM habits stay valid.

- `accounts`: name, password hash and scheme, email, state, error
  message, ban end, memo, last login, login count, last IP.
- `account_vars`: `#` and `##` variables, per account.
- `characters`: one row per character with the scalar fields of
  `CharKey`/`CharData` as columns (so admins can query them), plus
  `character_items`, `character_skills` and `character_vars`.
- `parties` and `party_members`; `storage_items`.
- `password_resets`: code, account, expiry.

GM levels stay in `gm_account.txt`, which Ansible renders on aurora: the
gate reads it at startup and when it changes (as `tmwa-login` checks it
every 15 s), `reloadgm` rereads it, and `gm` rewrites it like
`tmwa-login` does.

A one-time importer (`tmwa-gate import`) reads the tmwa flat files:
`account.txt`, `athena.txt`, `party.txt`, `storage.txt`, `accreg.txt`.
Rollback means going back to the pre-import files.

**The map link never waits on SQLite.** The per-link reader is a hot
loop — every `0x2b01`/`0x3010`/`0x2afc` that waited inline on a
blocking DB call capped the link at roughly 1/latency packets per
second, which under a few hundred players backs the map's send buffer
up until the link flaps. Writes and mixed read/write work go to a
single serialized job queue instead: a `db_writer` task drains up to
512 jobs into one batch transaction and emits the queued replies
(storage answers, `0x3811` acks) only after the commit. Pure reads
(`0x2afc` auth, `0x2b0e` name ops, whisper targets) run on spawned
tasks.

The queue is capped (`DB_JOBS_LIMIT`, 64k jobs): past it new jobs are
dropped and counted (`status`'s `db_dropped`) rather than become an
unbounded memory backlog — a dropped save is retried by the map's
next autosave. Duplicate in-flight read requests are coalesced per
account (`0x3010` storage, `0x3005` accreg): the map re-requests on
reply timeout, and under congestion each re-request was one more
full storage reply the already-busy map had to parse. Within one
batch only the last save per char runs.

**Two writer queues per link.** Broadcasts, notifications and
pre-auth floods go on the bulk queue (dropped first under load);
request replies the map is blocked on (`0x2afd`, `0x2b06`, `0x3810`,
`0x3811`, `0x382a`, select-time `0x3829`s) go on the prio queue,
which the socket writer drains first — a bulk backlog can no longer
starve an auth answer. `send_must` gives a wedged prio queue 30 s,
then kills the link (`map_kill`): the map reconnects cleanly instead
of waiting minutes on an answer that never comes. The kill is pinned
to the failed writer channel, not the slot, so a wait that armed on
a dead link cannot kill a fresh link that re-registered into the
same slot while the wait was still running.

**Auth answers survive a flap.** Serving `0x2afd` consumes the staged
auth entry, so a `0x2afc` repeated after a link flap (the map re-pushes
pending requests on reconnect, since a reply lost with the old socket
used to burn the entry and kick the player with `0x2afe`) used to be a
spurious reject. The take therefore also opens a 60 s reservation
holding the exact `0x2afd` bytes; a repeat on the same registered map
address is re-served verbatim, or waits (bounded) for an in-flight
first serve to resolve. Re-requests from a different map address or
with different credentials still reject.

**mimalloc, not glibc malloc.** The job-queue backlog is a large
transient (~1 GB per ~120k queued saves); glibc's per-thread arenas
never return it — load testing measured ~20 GB of permanently
retained anonymous RSS. mimalloc hands freed memory back to the OS
(measured: ~960 MB peak, ~27 MB after drain).

## Passwords

- argon2id.
- On import, each legacy hash is wrapped: argon2id(legacy hash), marked as
  wrapped. Every stored hash is strong from the start.
- On the next successful login the password is rehashed from the
  plaintext (`0x0064` carries it), and the wrapped marker is dropped.
- Over TCP the password still travels in plaintext; over `wss://` it is
  encrypted.

## Admin CLI

`tmwa-gate admin <command> [args]`, talking to the running gate over a
Unix socket (access by file permissions, no admin password). Without a
command it reads commands from stdin, one per line, like `tmwa-admin`.

- All `tmwa-admin` commands except `sex`. `create` becomes
  `create <name> <email> <password>`.
- `--json` on every command, with a meaningful exit code.
- In stdin mode, `search`, `create`, `getcount` and `ga` print what
  `tmwa-admin` printed, so mirror-lake's parser keeps working.
- Passwords for `create`, `password` and `check` can be read from stdin
  instead of argv, so they don't show in `ps`.
- Added: `find --id/--name/--email/--memo`, `chars --account/--name/--id`
  (level, exp, zeny, equipped items), `#` variables in `get`/`getall`,
  `status` (map server state, per-server user counts, draining flag),
  `online`, `kick`, `drain [<id>] [--wait]` (blue-green evacuation, see
  "Map restarts").

## HTTP API

Served by the gate on loopback, behind Caddy, together with the
WebSocket endpoint. Replaces tmw-api's `/api/tmwa` with the same paths,
JSON and status codes:

- `GET /api/tmwa/server`: schema.org GameServer (name, url,
  `playersOnline`, `serverStatus`), from the gate's own state.
- `POST /api/tmwa/account`: registration. reCAPTCHA check (POST to
  `siteverify`), confirmation email through `sendmail`.
- `PUT /api/tmwa/account`: password reset by email, with a one-time code
  valid for an hour, stored in SQLite.
- Per-IP cooldowns as in tmw-api.

tmw-api is retired afterwards; its `/api/vault` router does not run in
production (no `SQL__*` settings on aurora).

## Other consumers

- **mirror-lake** (Vault's TMWA bridge, `tmwa.py`): `TMWA_EXEC` points to
  `tmwa-gate admin`; its reads of `account.txt`, `athena.txt` and
  `accreg.txt` become `find`, `chars` and `getall --json`. `mapreg.txt`
  is written by `tmwa-map` and stays.
- **tmw-online-exporter** and the public player list: unchanged, they
  keep reading `online.txt` and `online.html`.

## Code layout

A Cargo crate in `gate/`, GPL-3.0-or-later, producing the `tmwa-gate`
binary. Packet definitions are generated from `tools/protocol.py` by a
Rust backend added to it, so the gate follows protocol changes. CI builds
and tests the crate on the amd64 runners.

## Plan

1. Resync spike: a throwaway proxy in front of unmodified tmwa that
   restarts the map session under a real client.
2. Gate with login, char, map relay and SQLite, plus the importer. Test
   against `tmwa-map` with the existing tests and a load bot.
3. Seamless restart: drain, hold, reconnect, resync; the `tmwa-map`
   changes.
4. Admin CLI, HTTP API, WebSocket.
5. Test deployment on sauna, then aurora (Ansible role replacing
   `tmwa-login`, `tmwa-char` and `tmw-api`; Caddy routes; mirror-lake
   changes).
6. Multiple map servers, once load tests show `tmwa-map` needs more than
   one CPU, together with the `tmwa-map` and serverdata changes above.

### IPv6 clients

tmwa-map's client auth carries an IPv4 address, so the gate maps
every client address to an IPv4. IPv4 passes through; an IPv6
address becomes a stable pseudo-IPv4 in 240.0.0.0/4, hashed (FNV-1a)
from the /64 prefix so privacy-extension host bits do not matter.
The mapped address goes everywhere an IPv4 is required — map auth
(0x3829), last_ip, IP ACLs and rate limits, online files — and the
gate logs the real IPv6 next to it at connect so operators can
correlate.
