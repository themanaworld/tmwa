# tmwa-gate

tmwa-gate is a Rust server that replaces `tmwa-login`, `tmwa-char`,
`tmwa-admin` and the TMWA part of tmw-api. It is the only entry point for
clients and it keeps them connected while `tmwa-map` restarts, so a map
server restart no longer disconnects players.

Status: design. Nothing here is implemented yet.

## Overview

```
  Mana / ManaPlus / Manaverse (TCP)     Mana wasm build (WebSocket)
                 \                         /
                  \                       /   Caddy: TLS, /tmwa, /api/tmwa
                   v                     v
              +-------------------------------+
              |           tmwa-gate           |
              |  login + char screens         |
              |  map traffic relay            |      tmwa-gate admin ...
              |  char server for tmwa-map     | <--- (CLI over Unix socket)
              |  HTTP: /api/tmwa, WebSocket   |
              |  SQLite                       |
              +---------------+---------------+
                              | one TCP connection per player
                              | + one inter-server connection
                              v   per map server
                     tmwa-map [tmwa-map ...]
```

- Clients connect to the gate for everything: login, character select and
  the game itself. The gate advertises its own address as char and map
  server.
- Map traffic is relayed per player over a separate upstream connection to
  `tmwa-map`. On this path the gate only frames packets; it parses the few
  it needs (see "Seamless map restart").
- Towards `tmwa-map` the gate plays the char server role on the existing
  inter-server protocol (`0x2af8`...`0x3830`, defined in
  `tools/protocol.py`). `tmwa-map` does not know it is not talking to
  `tmwa-char`, apart from the small changes listed under "Changes to
  tmwa-map".

## Client connections

- **One TCP port** for login, char and map. The purpose of a connection
  follows from its first packet: `0x0064` (login), `0x0065` (char),
  `0x0072` (map), optionally preceded by `0x7530` (version). The addresses
  the gate sends in `0x0069` and `0x0071` point back to this port.
- **One WebSocket endpoint** (for example `wss://server.themanaworld.org/tmwa`)
  for the wasm client. Binary frames carry the same byte stream as TCP;
  frame boundaries are not significant. The `binary` subprotocol is
  accepted. The Mana client connects to the configured URL as is (it
  currently appends `/<host>/<port>`, which is dropped).
- Limits: maximum connections, idle timeout for connections that have not
  logged in, per-IP login throttling.
- **Real client IP:** taken from the socket, or from `X-Forwarded-For`
  when the WebSocket request comes from a trusted proxy (Caddy on
  loopback). It is passed to `tmwa-map` in `0x3829` (see below).

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

## Seamless map restart

The gate owns map authentication: it pushes `0x3829` (pre-auth) to
`tmwa-map` whenever it wants a player to be accepted, so it can log a
player in again at any time without the client.

1. **Drain.** Before a planned restart (`tmwa-gate admin drain`, or on
   `tmwa-map` shutting down), the gate closes each player's upstream
   connection. `tmwa-map` runs `map_quit` for each and sends the final
   save (`0x2b01`). `tmwa-map` does not save players in `term_func`, so
   without a drain up to one autosave interval (default 1 min) is lost.
2. **Hold.** While `tmwa-map` is down, the gate keeps the client
   connections, answers `0x007e` with `0x007f`, drops other input and
   sends one `0x009a` announcement.
3. **Reconnect.** When `tmwa-map` connects again (`0x2af8`, map list
   `0x2afa`), the gate pushes `0x3829` for each held player and opens a new
   upstream connection with `0x0072`, answering `0x2afc` with the stored
   character (`0x2afd`).
4. **Resync the client.** The gate drops the `0x8000` and `0x0073`
   replies and sends the client `0x0091` (change map) to the same map and
   position. The client clears all beings, reloads the map and sends
   `0x007d`, which the gate forwards. The login burst `tmwa-map` sends
   after auth (stats, inventory, equipment, skills) is forwarded as is.
5. Also sent to the client where needed: trade cancelled (`0x00ee`),
   storage closed (`0x00f8`).

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
again upstream and splices the new map session) confirmed this with the
Mana desktop client: after `0x0091` it reloads the map, drops the old
monsters, takes the new inventory and equipment (sent after `0x007d`),
and walking, chat and NPCs keep working, across repeated restarts with
SIGTERM and SIGKILL. Findings to carry over:

- An NPC dialog that is open during the restart stays open but can't be
  advanced, because the new session has no NPC state; it closes with its
  close button, and the next NPC works. The gate should close it on
  resync.
- Every upstream step of a reconnect needs a timeout; a hung `tmwa-map`
  otherwise leaves players waiting forever.
- `tmwa-map` took about 14 s to accept players again (about 60 s with a
  cold page cache).

ManaPlus, Manaverse and the wasm build still need the same check.

## Multiple map servers

Several `tmwa-map` processes can each serve a different set of maps, to
use more than one CPU. Clients don't notice: they only ever talk to the
gate.

- Each `tmwa-map` connects to the gate and reports its maps (`0x2afa`).
  The gate keeps a map to server table and sends each server the maps of
  the others (`0x2b04`), so `tmwa-map`'s existing remote map handling
  works unchanged.
- **Warping to a map on another server:** `tmwa-map` saves the character
  (`0x2b01`) and asks for a server change (`0x2b05`); the gate answers as
  the char server does (`0x2b06`), after which `tmwa-map` sends the client
  `0x0092` (change map server). The gate intercepts `0x0092`, pushes
  `0x3829` to the target server, opens a new upstream connection there
  and resyncs the client with `0x0091` to the new map, the same way as
  after a restart.
- Whispers, party messages and GM broadcasts are routed between servers
  by the gate, as `tmwa-char` does now (the inter-server packets were made
  for this).
- Restarting one map server only holds the players on its maps.

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
  `@ip` and IP bans.
- On SIGTERM, save all players (`0x2b01`) and flush before exiting.
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
  `status` (map server state, online count), `online`, `kick`, `drain`.

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
