# tmwa end-to-end test harness

Boots a complete throwaway tmwa world (tmwa-login, tmwa-char, tmwa-map) with
generated configuration, a tiny 60x60 walkable map, minimal item/mob/param
dbs, and the fixture Lua content in `data/npc/e2e.lua`; then drives a fake
client through the old eAthena-derived wire protocol and asserts on the reply
packets. Python 3, stdlib only. See doc/lua-engine.md section 14.2.

## Running

    tools/e2e/run-e2e.py

Assumes already-built binaries (default `build-release/` at the repo root;
override with `--build-dir`). Never builds anything itself. Prints a
scenario-by-scenario PASS/FAIL list and exits 0 only if everything passed.

Options:

* `--base-port N` - servers listen on N+1 (login), N+2 (char), N+3 (map);
  default 16900, or the `E2E_BASE_PORT` environment variable.
* `--world-dir DIR` - where to generate the world (default: a fresh temp
  dir, removed afterwards unless `--keep`).
* `--keep` - keep the world dir, including the three server stdout logs.
* `--verbose` - log every packet sent/received.
* `--only NAME` - run a single scenario (plus the login).

Server processes are killed (SIGTERM, then SIGKILL) on the way out, also on
harness errors.

## Files

* `run-e2e.py` - the runner and the scenario list.
* `client.py` - fake client: login -> char select/create -> map connect ->
  LoadEndAck, dialog verbs (click/next/menu/input/input_str/close), shop,
  storage, item and chat packets. Paces its sends to stay under the map
  server's per-command flood limits (`SEND_SPACING`).
* `packets.py` - packet ids and lengths (generated from
  src/proto2/client-enum.hpp + client-packet-info.cpp), pos1 coding,
  a small unpack helper.
* `server.py` - world generator/orchestrator; conf templates live here.
* `data/npc/e2e.lua` - the fixture content (one NPC per feature; marker
  strings `E2E_...` are what the runner asserts on).
* `data/db/` - minimal item_db/mob_db/params fixtures.

## Scenarios

| name | exercises |
|---|---|
| login-hook | `OnPCLoginEvent` broadcast label (0x008e marker message) |
| oninit-npctimer | `on_init` at boot + `initnpctimer`/`OnTimerN` re-arm (0x009a announces) |
| dialog | `mes`/`next`/`menu`/`input`/`input_str`/`close` (0x00b4/b5/b7/0142/01d4/b6) |
| setup-items | `p.Zeny` write (0x00b1 sp 20) and `getitem` (0x00a0) |
| shop | `npc.shop` buy list/purchase/sell (0x00c4/c6/ca/a0, c7/cb/af) |
| storage | `close2` + `openstorage` incl. the async first-fetch, resume on close (0x00f2/f8) |
| item-use-dialog | use script opening a dialog on the `#itemdialog` NPC, consumption via 0x01c8 |
| equip-bonus | equip script `p:bonus(bStr, 5)` visible in 0x0141, gone on unequip |
| registercmd | `server.registercmd` word + argstring dispatch |
| mob-death-event | `mob.monster` runtime spawn with a death-event function, killer attached |
| persist-store | writing `p.vars.*` through a dialog `input` |
| warp | `npc.warp`: walking onto the tile yields 0x0091 |
| relog-persistence | `p.vars` values and the login counter survive a full relog |

## Known server bugs

Scenarios expected to fail because of a server bug (not workaroundable
without a rebuild) are marked in `XFAIL` in run-e2e.py with the reason.
Currently none; one real bug found by this harness (NPC handles carry `id`
where check_npc expects `__id`, breaking every NPC self-method) is worked
around in `data/npc/e2e.lua` with a `rawset(self, "__id", self.id)` stamp;
see the comment there. Remove the workaround once the engine is fixed.
