# Working notes for tmwa-gate

Decisions and findings from standing this branch up locally against a real
serverdata dataset (112k accounts imported) and load-testing it with
protocol-level bots. This file was updated after testing !374 head
`6d6720d1` (prio/bulk link queues, db_jobs cap+dedup, mimalloc).

## Round 5 test results (round 4 + event-loop link priority, gate10)

One more map-side change (`d0a09eda`, `fix/map-link-prio`, all in
src/net/socket.cpp): server-link sessions are serviced first in every
`do_sendrecv` epoll batch and every `do_parsepacket` pass, and when a
link's `wdata_size` exceeds `WFIFO_MAX_SERVERLINK / 8` (4 MiB), client
`func_recv`/`func_parse` (including accepts) are skipped for the pass.
Clients shed first; the link is protected. Link sessions are identified
by `max_rdata >= FIFOSIZE_SERVERLINK`, matching the wdata cap test in
`wire/packets.cpp`.

900-bot run (results/gate10): connected min/avg/max 456/526/536,
46% timeouts, move p50 ~465 ms. Map ~98% CPU, ~836 MB; gate ~24% CPU,
~543 MB peak.

The flap loop is dead: **zero** link wdata-cap hits (was ~18k in gate8),
**one** link disconnect in the whole run (was 5 in gate8, 30 in gate9).
The failure mode changed to the intended one: ~170 map-connect and ~168
login-connect failures, i.e., connections the server shed at TCP level
while busy instead of letting them half-login and die on a dead link.
A flap no longer manufactures a storm; the map slows down and sheds.

What this means for the design question: the remaining ceiling is simply
the map's single core (~530 players sustained with this bot workload at
~46% action timeouts, p50 ~465 ms). Further gains need real CPU work
(login dump encode, broadcast loops, mob AI), not congestion plumbing.
The linked fixes together removed every multiplier that made overload
worse than its CPU cost.

Additional changes on the same branch (`681f0b15`):

- `waitingdisconnect` sessions now self-close 10 s after `pc_changeserver`
  (reuses `Session::timed_close`). A stuck client can no longer keep a
  draining map's `users > 0` until the drain deadline.

Round 6 (same branch, `b19d9873` + `e1137188`): the 0x2b01 flood's root
cause is resolved. Per-callsite save counters showed the generator was
`pc_setparam(SP::SEX)` at ~22-44k/s: the SP::SEX case ran an unequip
rescan, pc_calcstatus, and a forced save unconditionally, and serverdata
scripts (Sorfina's proximity dialog in npc/029-2, and the `set Sex,Sex`
fixpos trick elsewhere) drive it constantly while players are around.
Unchanged-sex calls now just send clif_fixpcpos. 900-bot run gate12:
connected 614/659/670, map CPU 76% (was 98%), gate 1%/354MB, link stable,
0x2b01 on the wire dropped 7.9M to 11k per run. Remaining failures are
TCP-level connect shedding during the ramp, by design.

Also on the branch from the same investigation: a `waitingdisconnect`
re-entry guard in `pc_changeserver`/`pc_setpos`/`pc_evacuate` (a second
handoff during an in-flight one emitted a duplicate save + 0x2b05), and
the SaveSrc per-callsite save counters, which are diagnostic scaffolding
to strip before upstreaming.

Round 7 (gate branch, local merges pending push): the three durability
and slot hazards from rounds 4-6 are fixed on this branch.

- `9f7a3de8` replay dropped saves: `push_db_job` can return Full, and a
  dropped char save (0x2b01) or storage save (0x3011) now marks the
  identity in bounded dirty sets (`SAVE_DIRTY_LIMIT` 16384). `db_writer`
  replays dirty marks from the live `st.chars` cache once `db_jobs_depth`
  falls below 8192. `wait_saves` also checks the dirty sets, closing the
  stale-read window for a transfer racing a dropped save, and batch commit
  failures re-mark char saves. `admin status` gains `db_dirty`. Remaining
  gaps (deliberate): dropped 0x3004/0x2b10 account-var ops, party ops,
  email/divorce are still lost at the cap; they are rarer and need a
  payload store per kind to replay.
- `6ef9f9b7` pin the send_must kill: `map_kill` now only fires when the
  slot's current handle owns the failed channel (same_channel), so a kill
  armed during a wedge, or a late sender holding a dead link's tx, cannot
  kill a fresh link that reused the slot. Two regression tests.
- `2589311d` re-serve consumed map auth: `take_map_auth` now inserts a
  60 s `served_auth` reservation under the auth lock, keyed by the serving
  map's registered ip:port. A repeated 0x2afc after a flap waits up to 5 s
  for the in-flight serve and then gets the identical 0x2afd instead of a
  reject. A different map cannot claim the served login.

cargo build --release clean; cargo test --lib 21/21 pass. The heavier
admin/import fixtures were not re-run after merging (each rehashes ~112k
fixture accounts, several hundred CPU-minutes).

Verification notes for the gate side (checked against this tree's
handle_auth_request):

- A repeated `0x2afc` for a consumed auth entry gets `0x2afe` (reject),
  which the map turns into a clean `0x0081` kick. So the map-side auth
  re-push after a link flap heals requests whose `0x2afc` died on the
  wire, but a processed request whose `0x2afd` reply died still loses the
  player to a relogin. Re-serving a consumed entry briefly (or rebuilding
  the answer from `online_auth` plus the chars cache) would close that;
  `OnlineAuth` currently lacks `client_version`, so it cannot rebuild
  `0x2afd` alone.
- Slot-reuse hazard: `send_must`'s 30 s timeout arm calls
  `map_kill(map_id)`; if the old link died mid-wait and the slot
  re-registered, the kill hits the fresh link's notify. Rare, but real.
- Direct-client `0x0072` with a consumed/missing auth_fifo entry is a
  silent drop with no retry. A flap mid-drain that eats a `0x3829` (or
  lands between consume and `0x2afd`) still loses that evacuee.
- The `0x3010` storage-request flood is fixed (`cf123666`):
  `storage_storageopen` had no in-flight tracking, so any re-trigger while
  a `0x3810` reply was outstanding or lost re-sent the request. One
  outstanding request per account is now deduped with a 5 s resend
  expiry, and `chrif_delete` clears the table. Zero `0x3010`s in the
  latest run.

## Round 4 test results (6d6720d1 + map-side fixes, gate9)

Map-side branch `test/gate-perf` now also carries three fixes that target
the churn loop itself (commits `24b226ce`, `12dae717`, `a6772e67`):

- `chrif_save` coalescing: duplicate `0x2b01`s within 5 s per char are
  dropped; logout/transfer/shutdown saves are forced through.
- Removed the `chrif_state != 2` mass-kick in `clif_parse`: players now
  survive a brief link outage instead of all being force-logged-out (the
  main churn amplifier). Reconnect re-pushes pending `0x2afc` auths and
  retries are jittered 5..15 s.
- O(fd_max) scans removed on hot paths: `map_id2sd`/`map_nick2sd` now use
  `id_db`/`nick_db`, `chrif_*` no longer rescans all fds for `client_ip`,
  a duplicate `pc_calcstatus` per login removed, session fifos skip the
  128 KiB zero-init (and re-zeroing on growth).

900-bot run (results/gate9): connected min/avg/max 524/566/570, 45%
timeouts, move p50 ~658 ms. Map held ~98% of one core; gate 2% / ~250 MB.

What changed vs round 3:

- ~2.4x more players held (230 to 566). The flap cycle still exists (30
  link disconnects during ramp) but clients now survive a flap, so each
  one only costs the select-refusals in its down window, not a mass-kick.
- `0x2b01` volume fell ~7.7x (4.3M to 562k) thanks to coalescing plus not
  kicking clients on flap.
- The map's map list now registers fully (`maps:141`).
- `db_dropped: 39150`: the gate did hit the 64k `db_jobs` cap during ramp
  bursts; those are saves that never reached SQLite (worth noting as data
  loss under sustained overload).

Remaining wall: the map is single-core-bound. It saturates accepting +
logging in a 900-conn ramp while serving ~570 players; during the worst
seconds the link's 512 MiB wdata cap still fires. The relay used to mask
the login-storm CPU cost. Next candidate fixes, in rough order:

- Accept-side throttling: stop polling the client listen fd (or delay new
  session work) while the link wdata or CPU is over a threshold, so the
  map sheds ramp load instead of entering the flap loop.
- `db_dropped` semantics on the gate: the 64k cap converts overload into
  silent save loss; consider whether dropped saves should be retried or at
  least surfaced per-char.
- If more headroom is needed, the remaining CPU is mostly protocol-required
  login dumps and broadcast encode loops, which need interest-management
  work rather than point fixes.

## Round 3 test results (head 6d6720d1)

### What is fixed

- mimalloc works: gate RSS peaked at ~236 MB during the 900-bot run and sat
  at ~135 MB after (was: climb to 12-20 GB and stay).
- Gate-side queues are healthy under the storm: `db_queue` drained back to
  0 between bursts, `db_dropped` 21 total, zero wedge/queue-full events on
  the gate to map direction, zero `send_must` kills of the link.
- Drain is clean: at ~206 players, `drain --wait 0` reported
  `evacuees:188, arrived:188, departed:0, emptied:true`. SIGTERM on the old
  instance left all 206 players playing on the survivor. Evacuee loss went
  100% (round 1) to ~11% (round 2) to 0% (round 3).

### What still fails: map to gate save storm

The 900-bot ramp still collapses to ~230 connected (180 code-1 select
refusals during link-down windows). This round the flaps are map-initiated:
the map's link session hit its 512 MiB wdata cap and disconnected itself
five times.

Gate rx counts over the ~8 min run window:

- `0x2b01` char saves: **4,325,631** (~9k/s during churn, ~66 MB/s of wire)
- `0x3830`: 1,496; `0x2afc`: 1,093; `0x3005`: 1,069; `0x3004`: 1,059
- `0x2aff`: 196; `0x2b05`: 188; `0x3010`: 37

That is ~4,800 saves per player. The save volume itself is the anomaly to
hunt on the map side: something saves the same chars constantly during
churn (each flap kicks sessions, each logout saves, plus whatever periodic
or per-action save triggers exist upstream). The gate's per-char-per-batch
save dedup protects the DB but not the wire; the link cannot carry 9k x
7.3 KB saves/s while the map also fights for CPU.

Two secondary observations:

- `db_dropped` was nonzero early in the ramp (16-21): the 64k db_jobs cap
  did kick in, probably during a save burst.
- The gate→map direction never wedged this run, so `send_must`'s 30 s
  behavior is unverified under this load; the failure moved entirely to
  map→gate.

Suggested next step for the map side (upstream): find why saves fire at
~5 saves/second/player during churn; if it is a retry or a per-event
trigger looping, cap coalesce or rate-limit saves per char. The gate
already dedups DB writes, but the wire flood is generated map-side.

## Round 2 test results (head 0bf1c92e)

### 900-bot load test, direct connect

Still collapses during the ramp: ~323 avg connected, 297 char-select
refusals (code 1 during link-down windows), 3 link disconnects, 24
queue-wedge events, 46% action timeouts.

The db batching worked: the gate is no longer intake-bound (it sits at ~4%
CPU throughout). The bottleneck moved to the map: a single-threaded process
that is CPU-saturated accepting and logging in ~900 direct connections while
also serving players already in game. It starves its own link reads, so the
gate's writer queue fills (2048 deep, ~14 MB of mostly `0x2afd`/`0x3810`
replies), `send_must` wedges and kills the link, and each flap kicks the
pending-auth sessions and feeds more churn into the ramp. In relay mode the
gate's per-client upstream writers absorbed this pacing mismatch; in direct
mode there is no buffer between a slow map and a login storm.

New regression to look at: gate RSS climbed to ~12 GB during the run, kept
growing to ~20 GB afterwards with only 5 s `0x2aff` heartbeats flowing, then
stayed put (all anonymous pages). Previous rounds held ~220 MB. Likely
allocator retention after a huge `db_jobs` backlog, or a structure that
never shrinks; a container restart reclaimed it. Worth reproducing and
profiling.

### Drain test (second instance serving the same 141 maps)

With ~232 players online, `admin drain --wait 0` now reports real
accounting: `evacuees: 226, arrived: 201, departed: 25, emptied: true`.
Then SIGTERM on the old instance: all 219 survivors kept playing on the
new one. The restart flow works end to end.

Evacuee loss is ~11% (was ~100% under congestion before the `0x3829`
pre-auth push + auth_fifo 4096). The remaining losses are `0x0072` re-logins
that die before `0x0073` when the survivor's link is mid-burst; plausible
candidates are map-side auth timeouts on congested links.

### Note on the round-1 maplink diagnosis

The earlier report attributed maplink congestion to the sequential DB
awaits. That was one half; the other half (now visible) is that the map
itself cannot drain the link while CPU-bound. Fixing gate intake alone does
not prevent flaps, it only moves where the backlog piles up.

## Status of the earlier feedback

The previously reported issues are addressed in !374:

- TCP clients now connect straight to tmwa-map; the gate keeps the relay
  only for WebSocket clients.
- `admin drain` implements the blue-green restart: mark draining, re-announce
  maps at the survivor (`0x2b04`), push pre-auth, `0x382a` evacuates through
  the normal `0x0092` warp path. Verified working end to end (see below).
- The importer now warns and skips the bad-UTF-8 party line and the orphan
  `accreg.txt` rows found in this dataset.
- `TMWA_ALLOW_ROOT=1` covers the rootless-Docker geteuid problem; the
  `libfakeeuid.so` shim can be dropped from the image.

## Load test results

All runs: ramp 45 s, warmup 45 s, 300 s steady, bots unpinned (cpuset is not
delegated to the user slice), ~10 configured maps, actions
move/attack/pickup/chat.

| | gate relay + vanilla map | gate relay + patched map | direct + patched map |
|---|---|---|---|
| connected (of 900) | 900 | 899 | ~260 |
| action timeout share | 40% | 23% | ~41% |
| move latency p50/p99 | 492/1029 ms | 41/867 ms | 41/82 ms |
| gate CPU / RSS | ~5.7 cores / 220 MB | 51% / 220 MB | ~4% / 130 MB |
| map CPU / RSS | ~98% / 843 MB | 37% / 343 MB | ~14-63% / ~260 MB |

The patched map is vanilla master plus three socket-layer fixes (branch
`perf/socket-fixes` in the tmwa repo): a 4 MiB per-client wdata cap
(512 MiB serverlink cap for the test run), a ring-buffer drain (no quadratic
memmove), and an epoll loop that lifts the ~974 fd ceiling.

An 1800-bot run (relay + patched map) reached ~904 connected, 28% timeouts,
p50 41 ms. It exposed the failure below.

## Blocking issue: the map link saturates and flaps

Every failure in the direct-connect runs traces to the same mechanism.

The maplink reader awaits a blocking SQLite op per packet
(`spawn_blocking(...).await` inside the sequential `handle()` loop,
`src/serve/maplink.rs`). Throughput is latency-bound at roughly 1k packets/s
regardless of how much CPU is idle (the gate sat at ~2-4% during failures).

Measured inbound mix during a ramp with ~300 players online:

- ~630 x `0x3010`/s (storage loads, 6 B requests; replies are large). Storage
  is only requested on open, so this rate implies repeated re-requests, likely
  when `0x3810` replies are late or dropped.
- ~150 x `0x2b01`/s (char saves, ~7.3 KB each).
- plus `0x2afc`, `0x3004`/`0x3005`, `0x2aff`, `0x2b05`.

Failure chain:

1. The map produces link traffic faster than the gate's latency-bound reader
   drains it, so the map's link-session wdata grows until it hits the cap
   (32 MiB; even 512 MiB is not enough during churn) and the session is
   disconnected.
2. While the link is down or reconnecting, `map_for` returns no server and
   char selects are refused (`0x0081` code 1): ~320-780 of 900 bots die at
   this point depending on when the flap hits.
3. In the other direction, gate to map replies fill the bounded writer
   channel and `send_must` waits 5 s then drops: `map N: dropping critical
   reply, link wedged`. The dropped packets include `0x2afd` auth answers, so
   players mid-login or mid-evacuation get disconnected.
4. Two link-loss loops feed each other: capped/closed client sessions log
   out, each logout is another `0x2b01`, which deepens the congestion.

Map-side detail worth fixing upstream: `recv_to_fifo` calls
`read(fd, buf, RFIFOSPACE)`. When the link's rdata is full that reads 0 bytes
and `len <= 0` is treated as EOF, so the map drops its own char link purely
because inbound data arrived faster than it could parse it.

Suggested fixes, roughly in impact order:

- Get DB work off the maplink read loop: decode + enqueue, let a separate
  task apply writes (the `saves_in_flight`/`wait_saves` tracking already
  keeps ordering for transfers). Batch `0x2b01` saves into transactions.
- Check whether `0x3010` re-requests are being amplified on timeout.
- `0x2afd` (auth answer) and `0x2b06` (warp ack) probably should not be
  droppable: a 5 s `send_must` timeout is not enough headroom under load.
  If a reply must be dropped, the corresponding client conn should be
  refused early rather than left to hit the map-side auth timeout.

## Blue-green drain: verified with caveats

Setup used: a second map instance from the same tree with its own `conf/`
and `save/` dirs (symlink `db/`, `npc/`, `data/`, `langs/`). Gotcha: the
instance needs `conf/permissions.txt` copied too, or it exits with "Fatal
error during startup" right after printing "ready". Two instances serving
the same 141 maps coexist fine; the gate registered them as map_id 0/1 and
spread new logins across both (239/16 observed).

At ~255 players online, `admin drain --wait 0`:

- 141 maps re-announced at the survivor, `0x382a` processed, map0 went from
  239 to 2 users (the 60 s wait timed out with `emptied: false`).
- ~205 evacuees landed on the survivor and resumed playing; the old instance
  was then SIGTERM'd and players continued uninterrupted. The restart flow
  works.
- ~14% (34) of evacuees were lost: `0x0092` arrived, they connected to the
  survivor and sent `0x0072`, then the conn was closed before `0x0073`
  (map log: "logged off your server (not auth account)"). The gate's
  `0x2afc` handler logged no REJECTED lines, so the failures look like
  map-side auth timeouts while its link was congested, or dropped `0x2afd`
  answers (there was one "link wedged" burst on the survivor's link earlier).

At ~290 players during a congested window the same drain lost ~100% of
evacuees the same way.

Also: `emptied` reflects the *source* server's user count, not confirmed
landings. It reported `emptied: true` in a window where every evacuee
failed to land. For an ops signal it should probably track confirmed
arrivals on the destination.

## Ops / config notes

- `gate.toml` needs `[lan]` `lan_subnet`/`lan_map_ip` when clients and the
  map share an address space: the map's `map_ip`/`map_port` from
  `conf/map_local.conf` is sent verbatim in `0x0071`/`0x0092`, so LAN clients
  need the substituted LAN address.
- `conn_limit_enable` must be off for load tests: one login per 5 s per IP
  serializes a ramp from a single address.
- `http.max_connections` (default 1000) caps all TCP+WS sockets; 900 players
  plus in-flight handshakes exceed it. Used 5000.
- Map names are matched exactly: this serverdata stores them without `.gat`.
  `start_point = "029-2,22,24"`, not `"029-2.gat,22,24"`.
- The runtime image still needs `ca-certificates` or the HTTP/reqwest init
  panics.
- GM accounts: `login/save/gm_account.txt` is reloaded on mtime and pushed to
  map servers via `0x2b15`. Load-test accounts `lt000000`+ need GM 60 for
  `@warp`; provision by reading ids from `characters.name` in `gate.db`.
- The map↔gate link's wdata cap (upstream socket code, new in
  `perf/socket-fixes`) is what turns congestion into flapping. 32 MiB is
  enough for steady state, not for a ramp; the test ran with 512 MiB and
  still overflowed it. Buffering alone will not fix this, the drain rate is
  the problem.

## Load test assets

`/home/bjorn/playground/tmw-stdb-server/loadtest`: `tmwa_bot` speaks the
client protocol and now handles `0x0092` (ChangeMapServer in
`tmwa-proto/src/proto.rs`, reconnect+relogin in `tmwa_bot/src/bot.rs`), so it
follows drain evacuations. `harness/run.sh tmwa --no-server-setup` drives a
run; cgroup samplers are `harness/sample-cgroup.sh`.

Results: `results/gate1` (relay+vanilla: run1 clean, run2 the 36 GB
collapse), `gate2` (relay+patched, 900 clean), `gate3` (same at 1800),
`gate4`/`gate5`/`gate6` (direct+patched, maplink congestion).

Local deployment: `/home/bjorn/projects/tmw/serverdata`
(`docker-compose.yml`, `gate/gate.toml`, `gate/gate.db`), image `tmwa:gate`
built from `Dockerfile.gate` in the worktree root. Second map instance dir:
`world/map2/` (port 5123), launched with the same binary under a
`tmwa-map2-perf.scope` systemd scope.

## Round 8: full code review, fix and cleanup passes

Six parallel review agents audited all of `gate/src` (~12k lines) and the
map-side MR branches; four fix agents and two restructure agents landed the
findings. All merged onto this branch; `cargo build` clean, `cargo test
--lib` 36/36.

Functional fixes worth knowing about:

- `0x2b02` used to `retain` every char's online mark for the map instead of
  only the departing account's; `0x2b14` sent ban/status fields swapped for
  block ops; `0x3823` broadcast flag=1 where upstream sends 0 (stale party
  exp-share on other maps); `handle_auth_request` read `db.load_character`
  directly and could serve stale CharData past the `wait_saves` machinery;
  `handle_named_op` ran account 0 ops on a failed load; `0x2b05` awaited
  SQLite inside the link read loop.
- `push_reauth` registered its `rejoin_notify` waiter after `send_must`,
  dropping fast `0x3830` acks; `admin kick` on a relayed player held and
  rejoined them instead of disconnecting (`FwdEnd::Kicked` added);
  `kick_account`/`setaccreg` looked up `st.online` by account_id (it is
  keyed by char_id); `drain` without `--wait` leaked `DrainTrack` entries;
  `char_sessions` was write-only machinery that could never disconnect,
  deleted.
- HTTP: `create_account` allocated ids from `MAX(id)+1` instead of the
  `next_account_id` meta counter (eventual PK collision deadlock);
  `client_ip` trusted the leftmost (client-supplied) XFF entry, so HTTP
  rate limits were spoofable (now `net::forwarded_for`, right-to-left);
  captcha used a no-timeout `reqwest::get`; `uuid()` fell back to 0 on
  getrandom failure, violating `random_u32`'s contract.

Efficiency fixes: `prepare_cached` on the per-save helpers (was ~7 fresh
prepares per save on the serialized writer), `PRAGMA synchronous=NORMAL`
under WAL, client.rs DB reads moved off the executor (`db.blocking`),
`CharRecord.data`/`DbJob::SaveChar` are `Arc<CharData>` (save path went
from two 7.3 KB memcpys to a refcount bump), auth lookups are O(1) `get`
plus 1 s-throttled TTL sweeps (`SweptMap`), `chars_by_account` index
replaces three O(all-chars) scans, `map_for` uses a name-to-slot index,
map-link channels carry `Bytes` (broadcasts are refcount clones),
`Packet::bytes` is zero-copy via `BytesMut::split_to`.

Structure: the DB writer lives in `serve/dbq.rs` (state.rs is now ~1400
lines of shared state), online-file rendering in `serve/online_files.rs`,
`send_pending_sel` in client.rs, `enc` in proto, `send_server_closed`
shared from state.rs, generated packets gained `.encoded()` (call sites
swapped), admin commands go through `db/` conn helpers via
`mutate_account`, import reuses the same `db/` helpers instead of a second
copy of the schema SQL, and the blanket `#[allow(dead_code)]` attributes
are gone.

Behavior notes: `search`/`list` match on the account name only (not the
rendered line); a malformed HTTP body still costs a cooldown but an
over-limit body returns 413; `password` propagates a hashing error rather
than writing an empty hash; unknown-but-frameable map-link packet ids now
log a warning instead of killing the link (matches upstream); the WS
relay path drops packets on a full 256-deep queue like TCP does instead
of killing the session; `admin quit`/`exit` over a raw socket returns
Unknown command (stdin mode still honors them).

Still deliberately open: dropped non-save db ops (account vars, party
ops, email/divorce) are not replayed; `gate/tests/` heavyweight fixtures
(argon2-hashing ~112k accounts) were not re-run; `State` pub fields are
still reachable directly in places (~50 raw lock sites), which is the
main remaining structural simplification.
