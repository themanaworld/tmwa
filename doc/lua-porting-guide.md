# TMWA Lua porting guide

The manual for porting the old eAthena-dialect NPC content (the ~85k lines under
`serverdata/world/map/npc` plus the item-script columns of `db/item_db_*.txt`) to the map
server's Lua 5.4 engine. The primary audience is AI porting agents working file by file;
section 2 is for humans running the toolchain.

This guide is self-sufficient for porting: an agent with this guide, the API reference
`doc/lua-api.md`, and the converter output can port any file without reading the C++
engine. Authority order when documents disagree:

1. The engine implementation (`src/map/lua-*.cpp`); this guide and the API reference were
   verified against it at commit `cde139d3`.
2. `doc/lua-api.md`: binding signatures and semantics of every new API entry.
3. This guide: the old -> new direction, restructuring recipes, and idiom translations.

Engine internals are documented in `doc/lua-engine.md`; porting agents do not need it.

Scripts target the portable Lua subset (`lua-api.md` section 3): no `goto`, no native
bitwise operators, no `//`, no `require`/`load`/`io`, `os` limited to `time`/`clock`/
`date`. `bit32.*` and the integer helpers (`idiv`, `imod`, `int32`) cover the gaps.
Reading an undefined global raises at first use; creating a new global is only allowed
while content files load and during `on_init`.

---------------------------------------------------------------------------------------------------

## 1. Chapter map

| # | chapter | what it answers |
|---|---|---|
| 2 | Running the toolchain | how a human builds and runs everything |
| 3 | The porting workflow | the agent's per-file loop |
| 4 | File layout and load order | where each `.lua` file goes, who loads first |
| 5 | Top-level constructs | `script`/`warp`/`shop`/`monster`/`mapflag` headers |
| 6 | Variable scopes | `@x`, `.@x`, `.x`, `$x`, `$@x`, `#x`, `##x`, plain, params |
| 7 | Events and labels | `OnX` labels -> handler fields, dialog capability |
| 8 | Builtin translation table | all 175 old builtins, old -> new |
| 9 | Control-flow restructuring | `goto`, `menu` labels, `callsub`, `end`, `close` |
| 10 | Calling conventions | `callfunc`, `call`, `getarg`, the two-pass rule |
| 11 | Menus | variadic vs table form, dynamic menus, cancel |
| 12 | Arrays | 0-based helpers, `$` persisted arrays |
| 13 | Strings | concatenation, comparisons, escapes |
| 14 | Operators | the old non-C precedence, `&&`/`||`, `/` and `%` |
| 15 | Numbers | int32, overflow, hex |
| 16 | Constants and params | const_db globals, `p.Zeny`, `b*` SP ids |
| 17 | Timers | player timers, NPC timers, areatimer, clock labels |
| 18 | Puppets, renames, destroy | `puppet`, `fakenpcname`, `destroy` |
| 19 | Spells and GM commands | the magic system and `registercmd` |
| 20 | Item scripts | the `{...}` columns of item_db |
| 21 | Idiom recipes | the 33 census idioms, before/after each |
| 22 | Behaviour changes: audit list | every deliberate deviation and what to check |
| 23 | Per-file verification checklist | what "done" means for one file |
| 24-26 | Worked examples | three complete real files, old and new |
| A | funcdefs.tsv | the callfunc signature registry |

---------------------------------------------------------------------------------------------------

## 2. Running the toolchain (for humans)

Everything below runs on a normal Linux box with CMake and Lua 5.4 development headers.

**Build the server** (out of tree):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

The binary that matters for porting is `tmwa-map`. `ctest --test-dir build` runs the unit
tests (engine invariants, array/menu/timer semantics, the item brace scanner).

**Convert the old tree** (once per old-content update; deterministic and idempotent):

```
tools/lua-port/convert-npc-data.py <serverdata>/world/map
```

The converter rewrites `scripts.conf` and the `_import.txt` files to point at `.lua`
files, fully converts the data entries (`warp`, `shop`, `monster`, `mapflag`), and turns
every `script`/`function` body into a bootable stub: `PORTME()` calls with the original
source embedded in a `--[==[ ... ]==]` comment block. Re-running it only rewrites stubs
still marked PORTME, so ported files are never clobbered. It also seeds `funcdefs.tsv`
(appendix A) and emits `-- AUDIT:` comments at every site on the behaviour-change list
(section 22). `PORTME` is a global defined by the converter's support file; it raises when
called, so a half-ported tree still loads.

(The converter and lint live in `tools/lua-port/`; if your checkout predates them, their
specification is `doc/lua-engine.md` section 15.)

**Lint one file or the whole tree**:

```
tools/lua-port/lint.py world/map/npc/001-1/gossip.lua
```

Checks: Lua syntax (`luac -p`), the forbidden-subset token scan, undefined globals against
the engine's API surface (`tmwa-map --dump-lua-api`) plus const_db names plus
content-defined globals, a PORTME count, and heuristic warnings (tables indexed with both
0 and 1 in one file, side-effecting operands of `and`/`or`, int/string `==` mixes).

**Check the whole tree with the real engine** (from the serverdata `world/map` directory,
because the conf chain uses relative paths):

```
cd <serverdata>/world/map
tmwa-map --check-scripts        # full: loads everything and runs every on_init
tmwa-map --check-scripts=load   # fast: skips on_init (use while stubs remain)
```

Exit 0 means clean; exit 1 prints every error. Full `--check-scripts` runs all `on_init`
handlers, so it fails on files whose `on_init` still contains `PORTME()`; use the `=load`
form for the inner loop until the on_init stubs in the tree are ported. Load-time errors
(syntax, unknown map/item/species, duplicate NPC name, unknown constructor key) are always
fatal, which is exactly what makes this check strong.

**Play-test**: run the normal server trio (`tmwa-login`, `tmwa-char`, `tmwa-map`) against
a client, or use the packet-level e2e harness (`tools/e2e/`, pytest) for scripted dialog
scenarios.

---------------------------------------------------------------------------------------------------

## 3. The porting workflow (for agents)

Per file, in order:

1. **Pick one converted file** containing `PORTME()` stubs. Read the embedded original
   source in the `--[==[ ... ]==]` block. Read any `-- AUDIT:` comments the converter left.
2. **Translate stub by stub** using sections 5-21. The converter has already split labels
   into the right handler fields (`on_click`, `on_touch`, `on_init`, `on_timer[N]`,
   `events.OnX`), so your job is the bodies. Port mechanically: keep numbers, strings,
   variable names, and quirky comparisons verbatim; restructure only control flow and
   calling conventions as this guide directs. Do not "improve" logic in the same pass.
3. **Delete the embedded original-source comment block** when the file is fully ported
   (no PORTME left in the file).
4. **Lint**: `tools/lua-port/lint.py <file>` must be clean.
5. **Engine check**: from the serverdata `world/map` directory run
   `tmwa-map --check-scripts=load` (or the full `--check-scripts` once the tree's on_init
   paths are ported). Exit 0 required.
6. **Resolve every `-- AUDIT:` comment** in the file: either the site is safe (delete the
   comment, mention it in the commit message) or it needs the documented adjustment
   (section 22).
7. **E2E where applicable**: for dialog-bearing NPCs, run the e2e scenario or a manual
   client session covering the mes/next/menu/input/close paths you touched.
8. **One file per commit** on the serverdata `lua-scripting` branch. The commit message
   names the old file and records each AUDIT decision.

Functions with callers in other files follow the two-pass rule of section 10: the first
pass keeps the `@`-variable protocol through `p.tmp`, so single-file commits stay safe.

---------------------------------------------------------------------------------------------------

## 4. File layout and load order

The old tree maps 1:1; `.txt` NPC files become `.lua` with the same basenames. `conf/` and
`db/` are unchanged (the item_db `{...}` columns now carry Lua, section 20).

```
world/map/
  conf/...                        -- unchanged; `import:` conf chain unchanged
  db/...                          -- unchanged formats
  npc/
    scripts.conf                  -- `npc:` lines now name .lua files, order preserved
    functions/*.lua               -- global Lua functions (Banker, PCtoNPCRange, ...), loaded first
    items/*.lua                   -- item-script helper functions + helper NPCs
    mobs/*.lua                    -- MobPoints etc.
    magic/*.lua                   -- spells (floating NPCs + registercmd)
    commands/*.lua                -- GM commands
    annuals/**.lua
    001-1/_import.txt             -- conf role unchanged; entries now .lua
    001-1/_mobs.lua               -- generated npc.monster{...} lines
    001-1/_warps.lua              -- generated npc.warp{...} lines
    001-1/mapflags.lua
    001-1/<hand-written>.lua
```

Rules:

* One old file -> one Lua file. The converter produces the skeleton; the porter fills the
  PORTME stubs in place.
* Load order (conf order) is preserved; `functions/` loads first, as today. Function
  lookup happens at call time, so only load-time calls need ordering: code that runs while
  a chunk loads (top-level statements) must not call functions defined in later files.
  Calls from handlers (including `on_init`, which runs after all files loaded) are safe in
  any order.
* `import(path)` exists for intra-content includes (path relative to the map server
  working directory, like conf paths); every file loads once; calling it after startup
  raises.
* File-local helpers are `local function`s; cross-file functions are globals.
* Old `function|script|Name` names with spaces or other non-identifier characters are
  mangled to underscores (`Easter Debug` -> `Easter_Debug`); the converter emits the
  mapping table. NPC names (strings) are never mangled: `"Magic Timer::OnClear"` stays as
  it is.
* Creating a global is allowed at load time and during `on_init` only; after that a new
  global assignment warns at runtime and is an error under `--check-scripts` and lint.

---------------------------------------------------------------------------------------------------

## 5. Top-level constructs

| old header | new construct |
|---|---|
| `map,x,y,d\|script\|Name\|sprite[,xs,ys]{...}` | `npc.script{ name=, map=, x=, y=, dir=, sprite=, xs=, ys=, ... }` |
| `-\|script\|Name\|32767{...}` (floating) | `npc.script{ name=, ... }` without `map`/`x`/`y` |
| `function\|script\|Name{...}` | `function Name(self, p, ...) ... end` (a plain global function) |
| `map,x,y\|warp\|name\|xs,ys,to_map,to_x,to_y` | `npc.warp{ map=, x=, y=, xs=, ys=, to_map=, to_x=, to_y= }` |
| `map,x,y,d\|shop\|Name\|sprite,item:price,...` | `npc.shop{ name=, map=, x=, y=, dir=, sprite=, items={ {item, price}, ... } }` |
| `map,x,y,xs,ys\|monster\|Name\|species,amount,delay1,delay2[,event]` | `npc.monster{ map=, x=, y=, xs=, ys=, name=, species=, amount=, delay1=, delay2=[, event=] }` |
| `map\|mapflag\|flag[\|args]` | `npc.mapflag{ map=, flag= [, to=, x=, y=] [, mask=] }` |
| conf `import:` | unchanged; `npc:` lines now name `.lua` files |

Notes (all verified against the constructors in `src/map/lua-npc.cpp`):

* Every constructor takes one table; an unknown key raises at load (typo guard). Unknown
  maps, items, species, or mapflags are fatal at load. A duplicate NPC name is a fatal
  load error.
* `npc.script`: `name` required (<= 23 bytes); `sprite` defaults to 32767 (INVISIBLE);
  `dir`, `xs`, `ys` default 0. `xs`/`ys` are the old file numbers (touch radius in cells;
  the engine stores 2n+1 as before). Handler fields: `on_click`, `on_touch`, `on_init`,
  `on_timer = { [N] = fn }` (N > 0), `events = { OnX = fn, ... }`; see section 7.
* `npc.shop` prices: an int is absolute; `"*N"` means `value_buy * N` (old syntax
  verbatim). Items may be named by string, const-aegis global, or numeric id.
* `npc.monster`: `species` is an int (a const-aegis mob name global evaluates to its id);
  `name` may be `"--en--"`/`"--ja--"`; amount is scaled by `mob_count_rate`; `event` is
  optional (`"Npc::OnX"` string, `"~Tag"` phony tag, or a function).
* `npc.mapflag`: `to`/`x`/`y` for `nosave`, `mask` for `mask`.

---------------------------------------------------------------------------------------------------

## 6. Variable scopes

| old | Lua | default | notes |
|---|---|---|---|
| `.@x`, `.@x$` | `local x = 0` / `local x = ""` | porter initialises | Lua locals; they now survive `next`/`menu` (coroutine) |
| `@x` | `p.tmp.x` | 0 | session lifetime, shared across all scripts (calling conventions keep working) |
| `@x$` | `p.tmpstr.x` | `""` | separate table, so `@x` and `@x$` never collide |
| `@arr[i]` | `array(p.tmp, "arr")[i]` | 0 | 0-based, indices unchanged |
| `@arr$[i]` | `arraystr(p.tmpstr, "arr")[i]` | `""` | |
| `.x` / `.x$` | `self.vars.x` / `self.varstr.x` | 0 / `""` | per-NPC; cross-NPC: `npc.get("Name").vars.x` |
| `.arr[i]` | `array(self.vars, "arr")[i]` | 0 | |
| `$@x` / `$@x$` | `worldtmp.x` / `worldtmpstr.x` | 0 / `""` | server-wide temporaries, shared across files by name, not persisted |
| `$x` / `$x$` | `world.x` / `world.str.x` | 0 / `""` | persisted (`mapreg.txt`, byte-identical format) |
| `$x[i]` / `$x$[i]` | `world.array("x")[i]` / `world.array("x$")[i]` | 0 / `""` | index 0..255; or `world.get(name, i)` / `world.set(name, i, v)` |
| `x` (plain bareword, incl. `QL_*`) | `p.vars.x` | 0 | permanent char variable: int only, names <= 31 chars, 96-entry cap |
| `#x` | `p.acc.x` | 0 | account variable, 16-entry cap |
| `##x` | `p.acc2.x` | 0 | login-server account variable, 16-entry cap; digit-leading names via `p.acc2["00_INFO"]` |
| params (`Zeny`, `Hp`, ...) | `p.Zeny` etc. | | exact params.txt case; on other blocks: `being(id).Hp` |
| `@menu` | the `p:menu` return value | | assign to a local at the menu (section 11) |
| `@args$` | the `argstring` handler parameter | | command handlers, section 19 |
| `@mobID`/`@mobX`/`@mobY` | `args.mobID`/`args.mobX`/`args.mobY` | | OnMobKillEvent handler |
| `@victimrid` | `args.victimrid` | | OnPCKillEvent handler |
| `@target_id` | `args.target_id` | | `map.foreach` and `overrideattack` handlers only |
| `@slotId`/`@itemId` | `args.slotId`/`args.itemId` | | item equip/use scripts |
| `@inventorylist_*`, `@skilllist_*`, `$@MobDrop_*` | return values of `p:getinventorylist()`, `p:get*poolskilllist()`, `mob.getmobdrops()` | | never `p.tmp` |

Rules:

* Lua keywords used as names (`end`, `and`, ...) and non-identifier names use bracket
  syntax: `p.tmp["end"]`, `p.acc2["00_INFO"]`.
* `p.tmp`/`p.tmpstr`/`self.vars`/`self.varstr`/`worldtmp`/`worldtmpstr` are plain tables
  with defaulting reads (0 or `""`); they may hold any Lua value, but do not store handles
  in them long-term (store ids, re-resolve with `players.byid`/`being`).
* Writing 0 (int scopes) or `""`/nil deletes an entry in the persisted scopes
  (`p.vars`, `p.acc`, `p.acc2`, `world`, `world.str`).
* `.@` locals: the old engine scoped them per script run; a Lua `local` in the handler is
  the same thing, except it survives dialog yields (`next`/`menu`), which only removes an
  old foot-gun. Initialise explicitly (`local i = 0`), because Lua locals start as nil,
  not 0.
* Old code sometimes reads a variable that was never set; defaulting reads make that 0 or
  `""` exactly as before.

---------------------------------------------------------------------------------------------------

## 7. Events and labels

| old label / mechanism | Lua handler | signature |
|---|---|---|
| script body (bytecode position 0) | `on_click` | `(self, p [, argstring])` |
| `OnTouch` | `on_touch` | `(self, p)` |
| `OnInit` | `on_init` | `(self)` |
| `OnTimerNNNN` | `on_timer = { [NNNN] = fn }` | `(self)` |
| `OnMinuteMM`, `OnClockHHMM`, `OnHourHH`, `OnDayMMDD` | `events.<same name>` | `(self)` |
| `OnPCLoginEvent` | `events.OnPCLoginEvent` | `(self, p)` |
| `OnPCLogoutEvent` | `events.OnPCLogoutEvent` | `(self, p)` |
| `OnPCDieEvent` | `events.OnPCDieEvent` | `(self, p)` |
| `OnPCKillEvent` | `events.OnPCKillEvent` | `(self, p, args)`, `args.victimrid` |
| `OnMobKillEvent` | `events.OnMobKillEvent` | `(self, p, args)`, `args.mobID/mobX/mobY` |
| any other `OnX` label | `events.OnX` | `(self, p, args)` as delivered |
| mob death event string (`monster ... "N::OnX"`) | `events.OnX` on N | `(self, p)` (killer attached) |
| `~Tag` phony mob tags | unchanged strings | n/a (mobcount/killmonster keys) |
| `addtimer` / `areatimer` target | `events.OnX` or a function | `(self, p)` |
| `addnpctimer` target | `events.OnX` or a function | `(self)` |
| `foreach` target | `events.OnX` or a function | `(self, caller, args)`, `args.target_id` |
| `overrideattack` target | `events.OnX` or a function | `(self, p, args)`, `args.target_id` |
| `registercmd` target | NPC name (= click body) or `"N::OnX"` | `(self, p, argstring)` |
| `goto` into a label (e.g. `goto OnNoRid`) | a plain function call | restructure per section 9 |

`on_click`/`on_touch`/`on_init`/`on_timer[N]` are sugar for `events[""]`,
`events.OnTouch`, `events.OnInit`, `events["OnTimer<N>"]`; giving both forms for one label
raises at load. The engine recognizes the broadcast labels (`OnPC*Event`,
`OnMobKillEvent`) and the clock patterns inside `events` and indexes them for dispatch;
every other `events.OnX` is reachable by name (`"Npc::OnX"`).

Old event strings keep working everywhere an event is accepted: `"Npc::OnX"`, `"Npc::"`
(the click body), `"::OnX"` (broadcast to every NPC defining that label; puppets skipped).
Every such place also accepts a Lua function directly. `npc.event(ev [, p [, args]])`
replaces `donpcevent`; `self:event(label [, p [, args]])` is the
`donpcevent strnpcinfo(0)+"::OnX"` shorthand; `p:event(ev [, args])` runs an event with
that player attached.

**Dialog capability** (engine-enforced; `lua-api.md` 1.6): dialog primitives (`mes`,
`next`, `menu`, `input`, `close`, `close2`, `openstorage`, `requestitem`, `requestlang`,
`shop`) may be called from: click body, OnTouch, targeted events with a player (player
timers, per-spawn mob death events, queued events, `npc.event(ev, p)`), command handlers,
and item use scripts. They raise in synchronous handlers: OnInit, NPC timers
(`on_timer`, `addnpctimer`), OnClock/Minute/Hour/Day, `OnPC*Event`, `OnMobKillEvent`,
`foreach` callbacks, `overrideattack` handlers, and equip scripts. The sanctioned bridge
from a synchronous handler to a dialog is `p:addtimer(0, "Npc::OnX")` (the existing
serverdata idiom, section 21 idiom 33).

---------------------------------------------------------------------------------------------------

## 8. The builtin translation table (all 175)

Legend: "(lang)" = handled by Lua syntax, see sections 9/10. Any player-context builtin
that took an optional target (char name / block id / char id) becomes a call on the
target's handle (`players.byname(n)`, `players.byid(id)`, `players.bycharid(c)`,
`npc.get(n)`, `being(id)`). Full signatures and semantics: `doc/lua-api.md`.

| # | old | -> Lua |
|---|---|---|
| 1 | `mes` | `p:mes(text)`; bare `mes;` -> `p:mes()` (one blank line) |
| 2 | `mesq` | `p:mesq(text)` (text wrapped in quotes) |
| 3 | `mesn` | `p:mesn([name])`; default is the dialog NPC's basename; inside an item-use dialog the name is required |
| 4 | `clear` | `p:clear()` |
| 5 | `goto` | (lang) restructure, section 9 |
| 6 | `callfunc` | `Name(self, p)` (global function); `@` inputs/outputs keep working through `p.tmp` (section 10) |
| 7 | `call` | `Name(self, p, a, b, ...)` with a return value; label form -> local function call |
| 8 | `callsub` | call of a local function made from the `S_` block (section 9) |
| 9 | `getarg` | function parameters; `getarg(n, d)` -> `param or d` (0/"" are truthy in Lua, so only a missing arg takes the default, exactly as before) |
| 10 | `return` | `return [v]` |
| 11 | `void` | (lang) call the function as a statement |
| 12 | `next` | `p:next()` |
| 13 | `close` | `p:close()` (never returns; nothing after it runs) |
| 14 | `close2` | `p:close2()` (yields until the client clicks Close, then continues) |
| 15 | `menu` | `local c = p:menu(...)` + `if c == 1 then ... elseif ...`; dynamic menus: the table form (section 11) |
| 16 | `rand` | `rand(n)` (0..n-1) / `rand(a, b)` (inclusive) |
| 17 | `isat` | `p:isat(map, x, y)` (boolean) |
| 18 | `warp` | `p:warp(map, x, y)` |
| 19 | `areawarp` | `map.areawarp(map, x0, y0, x1, y1, to_map, to_x, to_y)` |
| 20 | `mapwarp` | `map.mapwarp(map, to_map, x, y)` |
| 21 | `heal` | `p:heal(hp, sp [, itemheal])` |
| 22 | `injure` | `server.injure(src, tgt, dmg)` or `being(src):injure(tgt, dmg)` |
| 23 | `input` | `local v = p:input()` / `local s = p:input_str()`; negative int answer terminates the handler (old behaviour kept) |
| 24 | `requestitem` | `local ids = p:requestitem(n)`; name form: `p:requestitem(n, true)` returns names |
| 25 | `requestlang` | `local s = p:requestlang()` |
| 26 | `if` | (lang) `if cond ~= 0 then ... end` (int conditions need the explicit comparison, section 14) |
| 27 | `elif` | (lang) `elseif` merged into the adjacent `if` chain (always adjacent in content) |
| 28 | `else` | (lang) `else` |
| 29 | `set` | assignment: `p.tmp.x = v`, `p.vars.X = v`, `p.Zeny = v`, `world.X = v`; target form: `players.byid(id).Zeny = v`, `npc.get(n).vars.x = v` |
| 30 | `get` | read on the target's handle: `players.byid(id).Hp`, `npc.get(n).vars.x`; SP-number reads: `p:param(sp)` |
| 31 | `setarray` | `setarray(array(scope, "name"), start, v1, ...)`; append form: `setarray(t, getarraysize(t), ...)` |
| 32 | `cleararray` | `cleararray(t, start, value, count)` |
| 33 | `getarraysize` | `getarraysize(t)` (empty array now 0, was 1; AUDIT, section 22) |
| 34 | `getelementofarray` | `t[i]` |
| 35 | `array_search` | `array_search(needle, t [, start])` (-1 when not found, as before) |
| 36 | `setlook` | `p:setlook(type, val)` |
| 37 | `countitem` | `p:countitem(item)` |
| 38 | `checkweight` | `p:checkweight(item, n)` (boolean) |
| 39 | `getitem` | `p:getitem(item, n)`; target forms on the target's handle |
| 40 | `makeitem` | `map.makeitem(item, n, map, x, y)` (`"this"` no longer accepted: pass `p.map`) |
| 41 | `delitem` | `p:delitem(item, n)` |
| 42 | `getcharid` | `p.charid` (0), `p.partyid` (1), `p.guildid` (2), `p.id` (3); name form via `players.byname(n)` + nil check |
| 43 | `getnpcid` | `self.id` / `npc.get(n).id` (`npc.get` returns nil instead of -1 when missing: guard) |
| 44 | `getversion` | `p.version` |
| 45 | `strcharinfo` | `p.name` (0), `p.partyname` (1), `""` (2) |
| 46 | `getequipid` | `p:getequipid(pos)` (pos 1..11; returns id, 0, or -1 as before) |
| 47 | `bonus` | `p:bonus(type, val)` (`b*` constant) |
| 48 | `bonus2` | `p:bonus2(type, t2, val)` |
| 49 | `skill` | `p:skill(id, lv [, flag])` |
| 50 | `setskill` | `p:setskill(id, lv)` |
| 51 | `getskilllv` | `p:getskilllv(id)` |
| 52 | `overrideattack` | `p:overrideattack(delay, range, icon, look, event [, charges])`; bare `overrideattack;` (discharge) -> `p:overrideattack()` |
| 53 | `getgmlevel` | `p.gmlevel` (same value as the `p.GM` param) |
| 54 | `end` | `return` from the handler; at nesting depth: `stop()`; with an open dialog: `p:close()` (section 9) |
| 55 | `getopt2` | `p.opt2` |
| 56 | `setopt2` | `p.opt2 = v` |
| 57 | `savepoint` | `p:savepoint(map, x, y)` |
| 58 | `gettimetick` | `server.gettimetick(t)` (0 ms tick, 1 seconds since UTC midnight, 2 unix seconds) |
| 59 | `gettime` | `server.gettime(t)` (UTC) |
| 60 | `openstorage` | `p:openstorage()` (yields; returns false without yielding when storage cannot open) |
| 61 | `getexp` | `p:getexp(base, job)` |
| 62 | `mobinfo` | `mob.mobinfo(species, what)` |
| 63 | `mobinfo_droparrays` | `mob.getmobdrops(species)` (returns records; the `$@MobDrop_*` arrays are gone) |
| 64 | `getmobdrops` | `mob.getmobdrops(species)` |
| 65 | `summon` | `mob.summon(map, x, y, owner, name, species, attitude, lifespan_ms [, event])` |
| 66 | `monster` | `mob.monster(map, x, y, name, species, amount [, event])` (`"this"`/`x,y <= 0` conveniences gone: pass `p.map`, `p.x`, `p.y`) |
| 67 | `areamonster` | `mob.areamonster(map, x0, y0, x1, y1, name, species, amount [, event])` |
| 68 | `killmonster` | `mob.killmonster(map, event)` (`"All"` = every once-spawned mob) |
| 69 | `donpcevent` | `npc.event(event)`; same-NPC form: `self:event("OnX")` |
| 70 | `addtimer` | `p:addtimer(ms, event)`; id/target form on that player's handle |
| 71 | `addnpctimer` | `self:addnpctimer(ms, event)` / `npc.get(n):addnpctimer(...)` |
| 72 | `initnpctimer` | `self:initnpctimer()` (quirk preserved verbatim) |
| 73 | `startnpctimer` | `self:startnpctimer()` |
| 74 | `stopnpctimer` | `self:stopnpctimer()` |
| 75 | `getnpctimer` | `self:getnpctimer(type)` (0 tick, 1 active, 2 label count) |
| 76 | `setnpctimer` | `self:setnpctimer(ms)` |
| 77 | `setnpcdirection` | `self:setdirection(dir, sit, save [, p])` |
| 78 | `npcaction` | `p:npcaction(cmd [, id, x, y])` |
| 79 | `camera` | `p:camera(...)` (see `lua-api.md` 5.3 for the argument forms) |
| 80 | `announce` | `server.announce(text, flag [, source])`; `p:announce(text, flag)` (player source); `self:announce(text, flag)` (NPC source) |
| 81 | `mapannounce` | `map.mapannounce(map, text, flag)` |
| 82 | `getusers` | `server.getusers(flag)` |
| 83 | `getmapusers` | `map.getmapusers(map)` (-1 unknown map) |
| 84 | `getareausers` | `map.getareausers(map, x0, y0, x1, y1 [, living])` |
| 85 | `getareadropitem` | `map.getareadropitem(map, x0, y0, x1, y1, item [, delete])` |
| 86 | `enablenpc` | `npc.enable(name)` / `self:enable()` |
| 87 | `disablenpc` | `npc.disable(name)` / `self:disable()` |
| 88 | `sc_start` | `p:sc_start(type, tick, val)` / `being(id):sc_start(...)` (tick heuristic preserved: do NOT convert units) |
| 89 | `sc_end` | `p:sc_end(type)` / `being(id):sc_end(type)` |
| 90 | `sc_check` | `p:sc_check(type)` (boolean now) |
| 91 | `debugmes` | `server.debugmes(text)` |
| 92 | `wgm` | `server.wgm(text)` |
| 93 | `gmlog` | `p:gmlog(text)` |
| 94 | `resetstatus` | `p:resetstatus()` |
| 95 | `attachrid` | `p = players.byid(id)`; `if (attachrid(id) == 0)` -> `if not p then` |
| 96 | `detachrid` | nothing (or `p = nil` if a local held the handle) |
| 97 | `isloggedin` | `server.isloggedin(id)` (or `players.byid(id) ~= nil`) |
| 98 | `setmapflag` | `map.setmapflag(map, flag)` (`MF_*` constant) |
| 99 | `removemapflag` | `map.removemapflag(map, flag)` |
| 100 | `getmapflag` | `map.getmapflag(map, flag)` |
| 101 | `getbattleconfig` | `server.getbattleconfig(key)` (-1 unknown) |
| 102 | `pvpon` | `map.pvpon(map)` |
| 103 | `pvpoff` | `map.pvpoff(map)` |
| 104 | `setpvpchannel` | `p.pvpchannel = n` (write clamps at 0) |
| 105 | `getpvpflag` | `p.pvpchannel` (0) / `p.hidden` (1, boolean now) |
| 106 | `emotion` | `self:emotion(type [, p])` (NPC emotes) / `p:emotion(type)` (the old `"self"` form: the player emotes) |
| 107 | `mobcount` | `mob.mobcount(map, event)` (minus-one preserved: 0 mobs -> -1; keep `< 0` comparisons verbatim) |
| 108 | `marriage` | `p:marriage(name)` (boolean) |
| 109 | `divorce` | `p:divorce()` (boolean) |
| 110 | `getitemlink` | `item.getitemlink(item)` |
| 111 | `getpartnerid2` | `p.partnerid` |
| 112 | `explode` | `local t = explode(s, sep)` (0-based table) / `explode_int(s, sep)` for int arrays |
| 113 | `getinventorylist` | `local inv = p:getinventorylist()`; `#inv` replaces `@inventorylist_count`; entries `{id=, amount=, equip=, index=}` |
| 114 | `getactivatedpoolskilllist` | `local l = p:getactivatedpoolskilllist()` (entries `{id=, lv=, flag=, name=}`) |
| 115 | `getunactivatedpoolskilllist` | `p:getunactivatedpoolskilllist()` |
| 116 | `poolskill` | `p:poolskill(id)` |
| 117 | `unpoolskill` | `p:unpoolskill(id)` |
| 118 | `misceffect` | `self:misceffect(fx)` (oid default) / `p:misceffect(fx)` (charname form) / `being(id):misceffect(fx)` |
| 119 | `specialeffect` | `self:specialeffect(fx)` |
| 120 | `specialeffect2` | `p:specialeffect(fx)` |
| 121 | `nude` | `p:nude()` |
| 122 | `unequipbyid` | `p:unequipbyid(slot)` |
| 123 | `npcwarp` | `self:warp(x, y)` / `npc.get(name):warp(x, y)` |
| 124 | `npcareawarp` | `self:areawarp(x0, y0, x1, y1 [, avoid_collision])` (or on `npc.get(name)`) |
| 125 | `message` | `p:message(text)` / `players.byname(n):message(text)` |
| 126 | `npctalk` | `self:talk(text [, p])` / `npc.get(n):talk(text)` |
| 127 | `registercmd` | `server.registercmd(word, event)`; ported content always uses the event/NPC-name form, handler `(self, p, argstring)` |
| 128 | `title` | `p:title(text)` |
| 129 | `smsg` | `p:smsg([type,] text)` |
| 130 | `remotecmd` | `p:remotecmd(cmd)` |
| 131 | `sendcollision` | `p:sendcollision(map, mask, x1, y1 [, x2, y2])` (char-name target form: on that player's handle) |
| 132 | `music` | `p:music(name)` |
| 133 | `mapmask` | `p:mapmask(mask [, persist])`; map-wide: `map.setmask(map, mask)` |
| 134 | `getmask` | `p.mask` (or `map.mask(self.map)` for the NPC-map fallback) |
| 135 | `getlook` | `p:getlook(type)` |
| 136 | `getsavepoint` | `local m, x, y = p:getsavepoint()` (three return values) |
| 137 | `areatimer` | `map.areatimer(map, x0, y0, x1, y1, ms, event)` (the old leading bltype `0` argument is dropped) |
| 138 | `foreach` | `map.foreach(bltype, map, x0, y0, x1, y1, event [, caller])`; handler `(self, caller, args)` reads `args.target_id` |
| 139 | `isin` | `p:isin(map, x0, y0, x1, y1)` (boolean, inclusive) |
| 140 | `iscollision` | `map.iscollision(map, x, y)` (boolean; raises on unknown map) |
| 141 | `shop` | `p:shop(name)` (never returns; the handler ends) |
| 142 | `isdead` | `p.dead` (boolean) |
| 143 | `aggravate` | `being(mobid):aggravate([target])` |
| 144 | `issummon` | `being(id):issummon()` (boolean) |
| 145 | `fakenpcname` | `npc.get(old):rename(new [, sprite])`; same-name sprite change: `self.sprite = n` (AUDIT: semantics fixed, section 22) |
| 146 | `puppet` | `self:puppet(map, x, y, name, sprite [, xs, ys])` (returns handle or nil, was id or 0) |
| 147 | `destroy` | `self:destroy() return` / `npc.byid(id):destroy()` (does not end the handler by itself) |
| 148 | `getx` | `p.x` |
| 149 | `gety` | `p.y` |
| 150 | `getdir` | `p.dir` |
| 151 | `getnpcx` | `self.x` / `npc.get(n).x` |
| 152 | `getnpcy` | `self.y` |
| 153 | `strnpcinfo` | `self.name` (0), `self.basename` (1), `self.suffix` (2), `self.map` (3) |
| 154 | `getmap` | `p.map`; block-id form via `players.byid(id).map` / `being(id).map` |
| 155 | `getmapmaxx` | `map.getmapmaxx(map)` (raises on unknown map: guard with `map.mapexists`) |
| 156 | `getmapmaxy` | `map.getmapmaxy(map)` |
| 157 | `getmaphash` | `map.getmaphash(map)` |
| 158 | `getmapnamefromhash` | `map.getmapnamefromhash(h)` |
| 159 | `mapexists` | `map.mapexists(map)` (boolean) |
| 160 | `numberofmaps` | `map.numberofmaps()` |
| 161 | `getmapnamebyindex` | `map.getmapnamebyindex(i)` |
| 162 | `mapexit` | `server.mapexit()` (does not terminate the handler, as before) |
| 163 | `freeloop` | dropped (the instruction budget is large and per-resume); delete the statement |
| 164 | `if_then_else` | `if_then_else(c, a, b)` (kept as a global; both branches evaluated, same as old) |
| 165 | `max` | `max(a, b, ...)`; one-argument array form -> `arrmax(t)` |
| 166 | `min` | `min(a, b, ...)`; array form -> `arrmin(t)` |
| 167 | `average` | `average(a, b, ...)` |
| 168 | `sqrt` | `sqrt(n)` (truncated int) |
| 169 | `cbrt` | `cbrt(n)` |
| 170 | `pow` | `pow(a, b)` |
| 171 | `target` | `server.target(src, tgt, flags)` / `being(src):target(tgt, flags)` |
| 172 | `distance` | `server.distance(id1, id2)` / `being(a):distance(b)` |
| 173 | `chr` | `chr(n)` |
| 174 | `ord` | `ord(s)` |
| 175 | `l` | `l(s, ...)` (translation placeholder, returns `s`) |

---------------------------------------------------------------------------------------------------

## 9. Control-flow restructuring

The old language has no structured loops; everything is `goto`, `menu` labels, and
`callsub`. Five patterns cover all 7636 gotos in the content census. Translate by
intended meaning, never token by token.

### 9.1 Epilogue labels (`L_Close`, `L_End`, `L_NoMoney`, ...; 1387 gotos)

The label block runs cleanup and `close`. When the epilogue is just `close;`, replace
`goto L_Close` with `p:close()` directly. When it has a body, make it a local function
ending in `p:close()` and replace `goto L_Close` with `return close_()` (the `return`
matters: it ends the enclosing function even though `p:close()` already never returns,
and it keeps the code shape honest at nesting depth).

Old (`001-1/weellos.txt`):

```
    mes "\"Due to its historical significance, part of it has been turned into a museum.\"";
    goto L_BeforeClose;
L_In:
    mes "[Weellos]";
    mes "\"What did you think? Isn't the building intriguing?\"";
    goto L_BeforeClose;
L_BeforeClose:
    if (QL_KYLIAN != 4)
        goto L_Close;
    next;
    mes "You wonder if Kylian would be interested in seeing this historic landmark...";
    goto L_Close;
L_Close:
    close;
```

New:

```lua
local function before_close(p)             -- L_BeforeClose (two gotos target it)
    if p.vars.QL_KYLIAN ~= 4 then p:close() end
    p:next()
    p:mes("You wonder if Kylian would be interested in seeing this historic landmark...")
    p:close()                              -- L_Close
end
-- in the handler:
        p:mes("\"Due to its historical significance, part of it has been turned into a museum.\"")
        return before_close(p)
```

### 9.2 Menu dispatch

`menu "a",L_A,"b",L_B;` becomes `local c = p:menu("a", "b")` plus an
`if c == 1 then <L_A block> elseif c == 2 then <L_B block> end` chain. Shared tails become
local functions; a label targeted by several menus becomes one local function. The menu
entry order defines the numbering; entries pointing at the same label get the same branch
(`if c == 2 or c == 5 then`). Section 24 shows a complete file. Menus whose entries are
built at runtime use the table form (section 11).

### 9.3 Loops (1149 backward gotos)

Any backward goto is a loop: wrap the region from the label to the goto in
`while true do ... end`; a forward goto out of the region becomes `break` (when the target
is right after the loop) or `return f()` for far targets. Counted loops become `for` or
`while` with locals.

Old (`functions/item_menu.txt`):

```
L_pick_choice_loop:
    if (@c >= @items_nr)
        goto L_choice_init_done;
    set @choice_v[@c], @items[@c];
    set @choice_n$[@c], @item_names$[@c];
    set @choice_i[@c], @c;
    set @c, @c + 1;
    goto L_pick_choice_loop;
L_choice_init_done:
```

New (mechanical):

```lua
local c = 0
while true do
    if c >= items_nr then break end       -- goto L_choice_init_done
    choice_v[c] = items[c]
    choice_n[c] = item_names[c]
    choice_i[c] = c
    c = c + 1
end
```

(or, since this is a plain counted loop, `for c = 0, items_nr - 1 do ... end`; both are
acceptable, the `while` form is the safe mechanical default because the loop variable may
be read after the loop.)

### 9.4 Fall-through chains

Ladders of small label blocks that each set a value and jump to a common join
(`L_Dep_5k: set @amount, 5000; goto L_Dep_Continue;`) either get data-driven (a table of
amounts indexed by the menu choice) or the join point becomes a local function taking the
value (`dep_continue(amount)`). See the Banker worked example in `lua-api.md` section 14.

### 9.5 if/elif/goto ladders

Adjacent `if (c1) goto A; if (c2) goto B; goto C;` become
`if c1 then return a_() elseif c2 then return b_() else return c_() end` once the targets
are local functions, or plain inline blocks when each target has one use.

### 9.6 callsub

`callsub S_X` becomes a call of a local function made from the `S_` block. Local
functions close over the handler's locals, which replaces the old shared-`@` communication
for free. `callsub` with arguments maps like `call` (section 10). A goto from inside an
`S_` block to an outer `L_` label (55 cases in content) cannot become a Lua goto: give the
closure a return value that the caller switches on:

```lua
local function s_array()                   -- S_Array; old body could `goto L_Fail`
    ...
    if bad then return "fail" end
    ...
end
if s_array() == "fail" then return fail_() end
```

### 9.7 `end`, `close`, `destroy`, conditions

* `end;` at the top level of a handler -> `return`. Inside nested local functions where a
  plain `return` would only leave the inner function -> `stop()` (terminates the whole
  handler from any depth, no error log). With an open dialog (text shown, no terminator
  yet), prefer `p:close()`, which is what the old `end` did visually anyway via the
  auto-close.
* `if (...) end;` -> `if ... then return end`.
* A trailing `close;` maps to `p:close()`. A handler that falls off the end with an open
  dialog gets an automatic close from the engine, but keep the explicit `p:close()` for
  clarity.
* `destroy;` -> `self:destroy() return` (`self:destroy()` does not end the handler by
  itself; the old builtin did).
* Conditions: old int conditions get an explicit comparison (`if cond ~= 0 then`); `!x` ->
  `x == 0` (int) or `not x` (boolean); `&&`/`||` -> `and`/`or` (see section 14 for the
  evaluation-order caveat). Functions that now return booleans (section 22 item 12) drop
  the comparison: `if (isdead())` -> `if p.dead then`, `if (sc_check(x) == 0)` ->
  `if not p:sc_check(x) then`.
* Free-standing `elif`/`else` statements merge into one `if/elseif/else` chain (they are
  always adjacent to their `if` in content).

---------------------------------------------------------------------------------------------------

## 10. Calling conventions

Three old call mechanisms exist; each maps differently.

### 10.1 `call("f", a, b)`: explicit arguments, may return a value

Plain Lua parameters after `(self, p)`. `getarg(n)` -> the n-th parameter.
`getarg(n, default)` -> `param = param or default` at the top of the function: in Lua,
`0` and `""` are truthy, so only a genuinely missing argument (nil) takes the default,
exactly matching the old arity check. `return v;` -> `return v`.
`void call("f", ...)` -> a plain call statement. `set .@x, call("f", ...)` ->
`local x = f(self, p, ...)`.

```lua
-- old: function|script|get_byte  ... set .@v, getarg(0); set .@id, getarg(1);
function get_byte(self, p, v, id)
    ...
end
-- old: set .@gto, call("get_byte", ##00_INFO, 3);
local gto = get_byte(self, p, p.acc2["00_INFO"], 3)
```

### 10.2 `callfunc "F"`: the implicit `@`-variable protocol. TWO PASSES.

* **FIRST PASS (mechanical, the default)**: keep the `p.tmp` protocol exactly. Callers
  set `p.tmp.x = v` before the call; the function reads and writes `p.tmp.*` /
  `p.tmpstr.*`. This is safe because callers live in other files and `@` variables are
  session-wide by name; no cross-file reasoning is needed, and each file can be committed
  alone. Document the protocol in a comment above the function (copy the old header
  comment's Input:/Return: lines).
* **SECOND PASS (optional, per function, only with ALL callers in the same change)**:
  convert the documented Input:/Return: convention to parameters and return values. The
  `funcdefs.tsv` registry (appendix A) lists, per callfunc function, its `@` inputs and
  outputs (seeded by the converter) and, once chosen, the final Lua signature. Never mix:
  a function is either fully on the `@` protocol or fully on parameters.

Convention: every ported function takes `(self, p, ...)` even when it uses neither, so
call sites are uniform (`Inn(self, p)`).

### 10.3 `callsub`

A local function (section 9.6). Arguments and `return` map as in 10.1, but the function
is local to the handler and closes over its locals.

### 10.4 Engine-supplied `@` values

`@menu`, `@mobID`, `@victimrid`, `@target_id`, `@args$`, `@slotId`/`@itemId`,
`@inventorylist_*`, `@skilllist_*`, `$@MobDrop_*` are handler parameters, `args` fields,
or method return values (section 6 table); the engine never mirrors them into `p.tmp`.
Passing them onward to a callfunc-protocol function means writing them to `p.tmp`
explicitly at the call site (that is what the old engine effectively did).

### 10.5 Cross-handler state on the same player

When one handler stores and another reads later (e.g. `@StoneName$` set by an item script
and read by the `addtimer 0` hop target; `@flarspell[]` written by a spell's cast body and
read by its OnAttack), the state must stay in `p.tmp`/`p.tmpstr`: the handlers are
separate Lua functions with no shared locals. The converter greps for `@` names written in
one handler and read in another and marks them; do NOT demote such names to locals.

### 10.6 `attachrid` / `detachrid`

```lua
-- old: if (attachrid(getcharid(3, $@cave1fighter$)) == 0) goto OnNoRid;
local target = players.byname(worldtmpstr.cave1fighter)
if not target then return on_no_rid() end
-- every subsequent player operation goes through `target` instead of the implicit rid
target:warp("025-1", 71, 20)
```

`detachrid` becomes nothing (drop it), or `target = nil` when a local held the handle.

---------------------------------------------------------------------------------------------------

## 11. Menus

### 11.1 Static menus: the variadic form

`local c = p:menu("a", "b", "c")` sends the entries, yields, and returns the 1-based
choice. Preserved old semantics: the list stops rendering at the first empty string
(`""`), and entries hidden by that cut are not selectable (the old engine let crafted
clients select them; fixed). Client cancel terminates the handler like `p:close()`
without a packet; an out-of-range answer is ignored (the prompt stays outstanding).
`nil` arguments raise. Menu entries may be built expressions
(`"Hello!  My name is " .. p.name .. "."`); they are evaluated once (section 22 item 4).

### 11.2 Dynamic menus: the table form

`p:menu(tbl)` takes a 1-based sequence whose entries are strings or pairs
`{text, value}` (any Lua value, including a function). Entries that are `false` or `""`
are skipped (not sent, not selectable) but positions keep counting. Returns
`value, index` for the pair form, `index` for the string form. Optional keys:
`tbl.title = s` shows a dialog title first; `tbl.cancel = v` makes client cancel return
`v` instead of terminating the handler.

### 11.3 The 40-slot `@choice_n$` emulation -> table form

The old engine could not build menus at runtime, so content pre-filled 40-slot arrays,
passed all 40 `@choice_n$[i]` to `menu`, and decoded `@menu - 1` back to an index
(`functions/item_menu.txt`, `functions/dynamic_menu.txt`). All of that machinery is
replaced by the table form; delete the padding arrays, the 10/20/30/40 menu ladders, and
the `@menu - 1` decode.

Old (`functions/item_menu.txt`, abridged):

```
    setarray @choice_n$, "", "", ... ;              // 40 slots
L_pick_choice_loop:
    ...
    set @choice_v[@c], @items[@c];
    set @choice_n$[@c], @item_names$[@c];
    ...
L_choice_init_done:
    set @choice_v[@c], 0;
    set @choice_n$[@c], @default_choice$;
    if (@c < 10) menu @choice_n$[0], L_MenuItems, ..., @choice_n$[9], L_MenuItems;
    ...
L_choice_join:
    set @menu, @menu - 1;
    set @item, @choice_v[@menu];
```

New:

```lua
-- Input/Return protocol kept (first pass): reads array(p.tmp,"items"),
-- arraystr(p.tmpstr,"item_names"), p.tmpstr.default_choice; writes p.tmp.item.
function ItemMenu(self, p)
    local items = array(p.tmp, "items")
    local names = arraystr(p.tmpstr, "item_names")
    local n = getarraysize(items)
    if n ~= getarraysize(names) then
        server.debugmes("ItemMenu: array length mismatch")
        return
    end
    local tbl = {}
    for i = 0, n - 1 do
        tbl[#tbl + 1] = { names[i], items[i] }
    end
    local default_choice = p.tmpstr.default_choice
    if default_choice == "" then default_choice = "Never mind." end
    tbl[#tbl + 1] = { default_choice, 0 }
    p.tmp.item = (p:menu(tbl))            -- pair form returns value, index; keep the value
    p.tmpstr.default_choice = ""
end
```

### 11.4 Cancel-path audit

The dominant old cancel behaviour (fall out of the script) matches the new default
(handler terminated). Menus that must survive cancel (rare) use `tbl.cancel`:

```lua
local v = p:menu{ "a", "b", cancel = 0 }
if v == 0 then ... end
```

---------------------------------------------------------------------------------------------------

## 12. Arrays

* Ported arrays stay **0-based** with indices unchanged: `@arr[i]` ->
  `array(p.tmp, "arr")[i]`; loop bounds via `getarraysize`; index arithmetic ports
  literally. Only hand-cleanup passes may shift a self-contained loop to 1-based Lua
  sequences; never in the mechanical pass. Cache the array table in a local at the top of
  the handler (`local arr = array(p.tmp, "arr")`).
* `setarray @a[s], v1, v2` -> `setarray(a, s, v1, v2)`; `cleararray @a[s], v, n` ->
  `cleararray(a, s, v, n)`; `getelementofarray(@a, i)` -> `a[i]`;
  `array_search(v, @a)` -> `array_search(v, a)` (returns -1 when absent, as before).
* `getarraysize(t)`: index of the last non-nil, non-0, non-`""` element plus 1, scanning
  0..255. An empty array returns 0 (old: 1). It accepts a non-table (returns 0), so
  `getarraysize(p.tmp.never_set)` is safe. Every use is AUDIT-flagged (section 22 item 1).
* `explode(s, sep)` splits on the first character of `sep` into a 0-based table (max 256
  pieces); `explode_int` runs each piece through `atoi`.
* `$` persisted arrays: `world.array("name")` / `world.array("name$")` returns an index
  proxy (`a[i]` get/set, `a:size()`, `a:clear(start, count)`), indices 0..255 (part of the
  save format; out of range raises). The generic helpers (`setarray`, `cleararray`,
  `getarraysize`, `array_search`) accept the proxy.
* New API return values (`p:getinventorylist`, `p:requestitem`, `mob.getmobdrops`,
  `players.all`, ...) are 1-based Lua sequences; use `#t` and `ipairs` on them. Never mix
  the two conventions in one table.
* The 256-element cap applies only to `world` arrays (storage format); plain Lua tables
  are unlimited (the helpers still clamp their scan at index 255).

---------------------------------------------------------------------------------------------------

## 13. Strings

* Concatenation `+` -> `..`. Int-plus-int stays `+`. Lua's `..` accepts numbers, so
  `(@loop + 1) + " - " + $name$[@loop]` ports as `(loop + 1) .. " - " .. names[loop]`.
  When one operand could be a string-typed variable holding digits and the old code meant
  arithmetic, use `atoi`; when the old code meant concatenation of an int, `tostr` exists
  for explicitness. The census found no digit-string arithmetic site; the lint flags mixed
  int/string `==` comparisons (constant-false in the old engine) for manual review.
* String comparisons: `==`/`~=` work on strings directly. (The old engine supported
  ordering operators on strings; content never used them.)
* `chr(3)` NUL-sentinel tricks keep working (`chr`/`ord` are provided;
  `commands/_procedures.txt` argument parsing).
* Escapes: old `\"` inside script strings is the same in Lua; `%%` client emote markup,
  `@@` link markup, and `##` color codes pass through unchanged inside string literals.
* Case-sensitivity of names, labels, and variables is unchanged.
* Numbers are accepted where display text is expected (`p:mes(5)`) but nowhere else;
  numeric strings are NOT coerced to ints (use `atoi`).

---------------------------------------------------------------------------------------------------

## 14. Operators

### 14.1 Precedence: the old engine is NOT C

The old parser's precedence (low to high): `||` < `&&` < `== != >= > <= <` < `^` < `|` <
`& << >>` < `+ -` < `* / %`, all left-associative, unary `- ! ~` tightest. Two famous
consequences: `&` binds TIGHTER than `==`, and `| ^ << >>` chains group left across what C
would split. Do not translate token by token: rewrite by intended meaning and
parenthesize. The three precedence-dependent expression shapes in content, with canonical
translations:

| old | means | Lua |
|---|---|---|
| `$SANGUINE & $@SV_BMDBit != 0` | `($SANGUINE & $@SV_BMDBit) != 0` | `bit32.band(world.SANGUINE, worldtmp.SV_BMDBit) ~= 0` |
| `A & M >> S` | `(A & M) >> S` | `bit32.rshift(bit32.band(A, M), S)` |
| `F \| K ^ a ^ b` | `((F \| K) ^ a) ^ b` | `bit32.bxor(bit32.bxor(bit32.bor(F, K), a), b)` |

### 14.2 Bitwise

`& | ^ ~ << >>` -> `bit32.band` / `bor` / `bxor` / `bnot` / `lshift` / `rshift` (never
Lua 5.4 native operators: forbidden subset, lint rejects them). Results are returned as
signed int32, so comparisons against old values hold. Compound flag updates read
naturally:

```lua
-- old: set QUEST_BlueSage, (QUEST_BlueSage & ~($@Q_MASK) | (@pages << $@Q_SHIFT));
p.vars.QUEST_BlueSage = bit32.bor(
    bit32.band(p.vars.QUEST_BlueSage, bit32.bnot(worldtmp.Q_BlueSageBookPages_MASK)),
    bit32.lshift(pages, worldtmp.Q_BlueSageBookPages_SHIFT))
```

(check the old grouping first against the table above; `A & ~M | B << S` groups as
`((A & ~M) | (B << S))` under old precedence because `&`, `<<` bind tighter than `|`,
which happens to match the intent everywhere in content).

### 14.3 `&&` / `||` did not short-circuit

The old engine evaluated both sides of `&&`/`||` always. Lua `and`/`or` short-circuit.
The census found only pure right-hand sides in content, so `and`/`or` is the standard
translation; the lint flags side-effecting right operands (function calls outside the
known-pure list) for manual review. If a right operand has a needed side effect, hoist it:
`local b = f(...) if a ~= 0 and b ~= 0 then`.

Related: the old engine evaluated an `if`-guarded statement's arguments BEFORE the
condition. Lua evaluates the condition first. No content depends on the old order (census
verified); do not try to preserve it.

### 14.4 Ternary

`if_then_else(c, a, b)` stays as a global; both branches are evaluated before the call
(Lua call semantics, same as the old builtin). `c` may be a number (nonzero = true) or a
boolean. Only rewrite to `c and a or b` in hand cleanup when the middle operand can never
be false/nil.

### 14.5 Division and modulo

`/` -> `idiv(a, b)`, `%` -> `imod(a, b)`: C truncation toward zero, sign of the dividend,
raise on zero divisor. Lua's `/` produces floats and `%`/`//` are floor-based: never use
them in ported integer code.

---------------------------------------------------------------------------------------------------

## 15. Numbers

* All script integers are C int32 at the API boundary. Fractional values raise
  (`integer expected`).
* One wrapping rule: an integral value out of int32 range wraps to int32 at every write
  boundary (params, persisted variables, packet fields), mirroring C assignment. The
  intentional-overflow idiom (`CASTS`) therefore still works through a param write; write
  `p.CASTS = int32(p.CASTS + 1)` to document the intent (`int32()` is sugar, the wrap
  happens anyway).
* Hex literals (`0x...`, 78 uses) port unchanged. Content has no octal literals (the old
  parser would have read a leading `0` as octal; Lua would not: if one ever appears,
  convert the value).
* `sqrt`/`cbrt`/`pow` return truncated ints, as before. `math.*` returns floats: do not
  mix `math.sqrt` etc. into ported arithmetic.
* `rand(n)` is `[0, n)` (0 if n <= 0); `rand(a, b)` is inclusive, swapped if reversed.

---------------------------------------------------------------------------------------------------

## 16. Constants and params

* Every `const_db` name (`const.txt`, the const-aegis item and mob names,
  `const-quest.txt`, `const-magic.txt`, `const-mapflags.txt`, `const-debugflag.txt`,
  `permissions*.txt`) is a read-only global with the same spelling: port uses verbatim
  (`SkirtLength`, `FLAG_ROSSI_COMPLETED`, `MF_NOSAVE`, `CMD_ZENY`, `ATCMD_SYMBOL`,
  `MAGIC_SYMBOL`, `debug`, ...). Assigning to one raises.
* Params are handle properties with the exact `params.txt` spelling: bareword `Zeny` ->
  `p.Zeny`, `set Zeny, v` -> `p.Zeny = v`. Reading the bareword as a global raises with a
  hint (`undefined global 'Zeny' (did you mean p.Zeny?)`). Writing a read-only param
  raises. The property set is data-driven from params.txt.
* The `b*` SP constants (`bStr`, `bInt`, `bMaxHP`, ...) are integer globals (SP ids) for
  `p:bonus`/`p:bonus2`/`p:param`/`p:setparam`. Where old code stored an SP id in a
  variable and later read the stat (`set @bStat, bInt` ... `get(Int, ...)` style), the new
  code reads `p:param(stat_sp)`.
* `debug` gates port verbatim: `if debug >= 2 then return end`.
* Items may be referenced by name string (`"LeatherSuitcase"`), const-aegis global
  (`WhiteCake`), or numeric id everywhere an item is expected; monsters by numeric id or
  const-aegis name. Unchanged from old content practice.

---------------------------------------------------------------------------------------------------

## 17. Timers

### 17.1 Player timers

`addtimer ms, "Npc::OnX"` -> `p:addtimer(ms, event)` where `event` is an event string or
a function `(self, p)` (for a function, `self` is the NPC of the calling handler, nil in
item scripts). 32 pending timers per player; overflow drops the timer with a log line.
On fire, the event runs through the normal event path with the player attached: queued
100 ms at a time while the player is in a dialog, same-map/in-area gate for map-placed
NPCs, disabled NPC drops it. The handler is dialog-capable: `p:addtimer(0, ev)` is the
sanctioned "continue outside the current context" hop (idiom 33).

### 17.2 NPC timers

Verbatim methods with the old state machine, including the `initnpctimer` quirk (a second
`initnpctimer` while the timer is counting toward its first label is a no-op):

```lua
on_init = function(self)
    self:initnpctimer()
    self:stopnpctimer()
end,
on_timer = {
    [5000] = function(self)
        self:setnpctimer(0)               -- the re-arm idiom, unchanged
        ...
    end,
},
```

`addnpctimer ms, "Npc::OnX"` -> `self:addnpctimer(ms, event)` (event string or function
`(self)`); the timer slot lives on the calling handle. NPC timer handlers are
synchronous: no dialog primitives.

### 17.3 areatimer

`areatimer 0, map, x0, y0, x1, y1, ms, event` -> `map.areatimer(map, x0, y0, x1, y1, ms,
event)`: one player timer per player in the rectangle; the old leading bltype `0` is
dropped (only players were ever supported). The handler `(self, p)` is dialog-capable and
does NOT receive `@target_id` (it never did; the player is attached).

### 17.4 Clock labels

`OnMinuteMM`, `OnClockHHMM`, `OnHourHH`, `OnDayMMDD` keep their exact names under
`events`, fire on the UTC clock, signature `(self)`, synchronous.

---------------------------------------------------------------------------------------------------

## 18. Puppets, renames, destroy

* `puppet("map", x, y, "Name", sprite)` -> `self:puppet(map, x, y, name, sprite [, xs,
  ys])`, returning a handle or nil when the name is taken (old returned an id or 0):
  `if (puppet(...) < 1) mapexit;` -> `if not self:puppet(...) then server.mapexit() end`.
  The puppet shares all handlers of `self` (events, timers, click body), has its own
  `vars`/`varstr`, its own timers, id, and name; it is excluded from broadcast events and
  freed when the parent is. Inside a shared handler, `self` is the puppet that was
  clicked; `self.parent` is the template NPC (nil for non-puppets), so per-instance state
  lives in `self.vars` and shared config in `self.parent.vars` (old code used
  `strnpcinfo` tricks for this).
* `fakenpcname "Old", "New", sprite` -> `npc.get("Old"):rename("New", sprite)`. AUDIT:
  the new call REALLY renames (updates the name registry and event keys; afterwards the
  NPC is addressable only by the new name), where the old builtin left the NPC
  unaddressable by either name. The dominant same-name use `fakenpcname "X", "X", sprite`
  (sprite swap) becomes `self.sprite = n` (or `npc.get("X").sprite = n`). For real
  renames (the doomsday files), check every follow-up `npc.get`/event string for which
  name it must use now (section 22 item 7).
* `destroy;` -> `self:destroy() return`. `destroy(id)` -> `npc.byid(id):destroy()`.
  `self:destroy()` schedules the free for the next tick (as before), cancels the NPC's
  timers, dequeues an attached player, and does NOT terminate the handler: always add the
  `return` (or `stop()` at depth) that the old builtin implied.

---------------------------------------------------------------------------------------------------

## 19. Spells and GM commands

### 19.1 registercmd

`registercmd word, target` -> `server.registercmd(word, target)` where `target` is an
event string (`"Npc::OnX"`) or an NPC name (meaning that NPC's click body, the old
`registercmd ..., strnpcinfo(0)` idiom). Handler signature `(self, p, argstring)`;
`argstring` is the rest of the chat line (old `@args$`). The handler is dialog-capable.
If the player is mid-dialog when the word arrives, the open dialog is abandoned first,
then the command runs (section 22 item 16). A plain Lua function `(p, argstring)` is also
accepted, but ported content always uses the NPC/event form.

### 19.2 GM commands

The pattern (`commands/*.txt`): a floating NPC registers `chr(ATCMD_SYMBOL) .. "word"` in
`on_init`, parses `argstring` with the ported `argv_splitter` helper (returns a 0-based
table of strings, old `@argv$[]`; its numeric companion via `atoi`), checks `p.GM`
against the `CMD_*` levels from `conf/permissions.txt`, acts on `p` or on
`players.byname(argv[1])`, and reports via `p:message`, `p:gmlog`, `server.wgm`.
A complete translation is `lua-api.md` section 14 Example 7 (`commands/zeny.txt`).

### 19.3 The magic system

Each spell is a floating NPC (`magic/*.txt`): the click body is the cast (players "click"
it by saying the invocation word, which `magic_register` wires through
`server.registercmd`), `OnAttack`/`OnDischarge`/`OnSetRecast` labels drive the attack
override, and `on_init` sets `self.vars.school`, `self.varstr.invocation`
(`chr(MAGIC_SYMBOL) .. "flar"`), then calls the ported `magic_register(self)`.

Structure ports verbatim:

* Cast guards: `if(call("magic_checks")) end;` -> `if magic_checks(self, p) ~= 0 then
  return end` (signature per funcdefs.tsv).
* `overrideattack delay, range, icon, look, strnpcinfo(0)+"::OnAttack", charges` ->
  `p:overrideattack(delay, range, icon, look, self.name .. "::OnAttack", charges)`.
  The OnAttack handler `(self, p, args)` runs synchronously per attack and reads
  `args.target_id`; bare `overrideattack;` (discharge) -> `p:overrideattack()`.
* Spell state read across handlers (`@spellpower`, `@flarspell[]`, `@_M_BLOCK`) stays in
  `p.tmp` (section 10.5).
* `target(BL_ID, @target_id, 50)` -> `p:target(args.target_id, 50)`;
  raw-id call sites keep `server.target(src, tgt, flags)`.
* Dialogue NPCs read a spell's invocation via
  `npc.get("spell-name").varstr.invocation` (old `get(.invocation$, "spell-name")`).

Section 26 is a complete spell port.

---------------------------------------------------------------------------------------------------

## 20. Item scripts

* `item_db_*.txt` columns are unchanged; the two `{...}` columns hold Lua statements
  compiled with the prelude `local p, args = ...`. One physical line; no `--` comments and
  no long brackets inside item columns; `}` inside quoted strings is fine (the scanner
  understands quotes and backslash escapes). Compile errors are fatal at startup.
* **Use scripts**: `args = { itemId = id }`. They run in a dialog coroutine and MAY open
  dialogs directly (new capability; the player is attached to the engine's invisible
  `#itemdialog` NPC while it is open, and `p:mesn()` there requires an explicit name).
  The old `addtimer 0, "Npc::OnUse"` hop also keeps working unchanged and remains the
  right pattern when the intent is "after this handler ends" (idiom 33).
* **Equip scripts**: `args = { itemId = id, slotId = slot }`, run synchronously inside
  `pc_calcstatus`: only `bonus`-family calls, state reads, and timer scheduling; dialog
  primitives raise. The old `set @slotId`-based helper protocols become explicit
  arguments: `{RequireStat(p, args.slotId, bInt, 80)}`. Old `set @bStat, bInt` stored the
  SP id; the ported helper reads the live stat via `p:param(stat_sp)`.
* `p:bonus` outside an equip script logs a warning and still applies until the next
  status recalc (old behaviour; 10 such call sites exist in NPC scripts).

---------------------------------------------------------------------------------------------------

## 21. Idiom recipes (census idioms 1-33)

One recipe per idiom found by the content census, each with a real occurrence and
before/after code. Cross-references go to the chapter with the full rules.

### Idiom 1: rand-dispatch if-chains (`001-1/children.txt`)

```
set @TEMP,rand(10);
if(@TEMP == 1) goto L_1;
if(@TEMP == 2) goto L_2;
...
goto L_1;
```
```lua
local temp = rand(10)
if temp == 9 then
    ...                                   -- the one branch with real logic
else
    p:mes(lines[temp] or lines[1])        -- a table of lines; 0 and 1 both fall to L_1
end
```
When every branch is one line, use a table indexed by the random value; otherwise an
`if/elseif` chain. See `lua-api.md` section 14 Example 1 for the full file.

### Idiom 2: label fall-through

Does not occur: the old parser rejected fall-through, every label block is closed by
goto/close/end/return. Nothing to do.

### Idiom 3: goto-loops (`functions/item_menu.txt`, section 9.3)

Backward goto = `while true do ... end`; forward goto out = `break` or `return f()`;
counted loops become `for`/`while`. Loops inside `S_` subroutines that escape to outer
labels: the closure returns a tag the caller switches on (section 9.6).

### Idiom 4: `getarg` with defaults (`functions/spawns_on_mobkill.txt`)

```
set .@map$,   getarg(0, ""); // map where to spawn
set .@mobX,   getarg(1, -1); // X coord
```
```lua
function spawn_mobs_around(self, p, mapname, mobX, mobY, mobID, mobQTY)
    mapname = mapname or ""
    mobX = mobX or -1
```
Only a missing argument (nil) takes the default; 0 and "" are truthy in Lua, matching the
old arity-based rule exactly (section 10.1).

### Idiom 5: string concatenation with `+` (`033-1/kimarr.txt`)

```
mes (@loop + 1) + " - " + $Record_Fluffy_Name$[@loop] + " - " + $Record_Fluffy_Kills[@loop] + " Fluffies killed";
```
```lua
p:mes((loop + 1) .. " - " .. fluffy_names[loop] .. " - " .. fluffy_kills[loop] .. " Fluffies killed")
```
`..` accepts numbers; `tostr` only where explicitness helps; `atoi` where the old code
meant arithmetic on a digit string (section 13).

### Idiom 6: `set` on params (`014-1/wedding-officiator.txt`)

```
set Zeny, Zeny - WEDDING_FEE;
```
```lua
p.Zeny = p.Zeny - WEDDING_FEE
```
Read-only params raise on write; content never writes them (the engine catches porter
errors). Target form: `players.byid(id).Zeny = v`.

### Idiom 7: `close2` + `openstorage` (`functions/banker.txt`)

```
if (#BankOptions & OPT_STORAGE_CLOSE) close2;
openstorage;
```
```lua
if bit32.band(p.acc.BankOptions, OPT_STORAGE_CLOSE) ~= 0 then p:close2() end
p:openstorage()
```
Verbatim sequence; `p:close2()` yields until the Close click, `p:openstorage()` yields
until the storage window closes.

### Idiom 8: `if (...) end;` guards (`magic/level1-flare-dart.txt`)

```
if (Sp < 10) end;
```
```lua
if p.Sp < 10 then return end
```
`stop()` instead of `return` when inside a nested local function (section 9.7).

### Idiom 9: value-returning `call()` functions (`functions/bitwise.txt`)

```
set .@gto, call("get_byte", ##00_INFO, 3);
void call("set_byte", ##00_INFO, 3, 0);
```
```lua
local gto = get_byte(self, p, p.acc2["00_INFO"], 3)
set_byte(self, p, p.acc2["00_INFO"], 3, 0)
```
Explicit arguments become parameters, `return` stays `return` (section 10.1).

### Idiom 10: `input` with strings (`002-1/luca.txt`)

```
input @answer$;
if (@answer$ == "Kalmurk") goto L_Right;
```
```lua
local answer = p:input_str()
if answer == "Kalmurk" then return right_() end
```
Int form: `local v = p:input()`; a negative answer terminates the handler (old behaviour
kept, do not add guards).

### Idiom 11: 40-slot `@choice_n$` dynamic menus (`functions/item_menu.txt`)

Replaced wholesale by the `p:menu(tbl)` table form; the `@menu - 1` decode becomes the
returned `value`; delete the padding arrays and menu ladders. Full before/after in
section 11.3.

### Idiom 12: nested callsub (`functions/DyeConfig.txt`)

`S_Array` calling `S_Color` in a loop becomes nested local functions (closures over the
handler's locals); escapes to outer labels return tags (section 9.6).

### Idiom 13: NPC timers (`029-3/parua.txt`, section 17.2)

```
OnInit:
    initnpctimer;
    stopnpctimer;
    ...
OnTimer5000:
    setnpctimer 0;
```
```lua
on_init = function(self)
    self:initnpctimer()
    self:stopnpctimer()
end,
on_timer = { [5000] = function(self)
    self:setnpctimer(0)
    ...
end },
```
The initnpctimer quirk is preserved, so no site needs audit.

### Idiom 14: areatimer / foreach (`001-1/dock.txt`, `009-8/celestia.txt`)

```
areatimer 0, "001-1", 66, 71, 77, 73, get(.warp_delay, "#FerryConfig"), strnpcinfo(0)+"::OnAreaWarp";
foreach 0, getmap(), .x1, .y1, .x2, .y2, "CelestiaCrcAux::OnNthPlayer";
```
```lua
map.areatimer("001-1", 66, 71, 77, 73, npc.get("#FerryConfig").vars.warp_delay,
              self.name .. "::OnAreaWarp")
map.foreach(0, p.map, self.vars.x1, self.vars.y1, self.vars.x2, self.vars.y2,
            "CelestiaCrcAux::OnNthPlayer", p)
```
areatimer drops the leading `0` and its handler `(self, p)` runs with the player
attached (no `@target_id`; it never had one). foreach handlers are `(self, caller, args)`
with `args.target_id`, synchronous, no dialogs; pass the current player as `caller` when
the old code ran with a rid attached (sections 17.3, 7).

### Idiom 15: puppets / fakenpcname / destroy (`001-2/wizards.txt`, `functions/doomsday.txt`)

```
if (puppet("001-2", 104, 19, "Desert Mana Seed", 166) < 1) mapexit;
if (rand(1, 20) != 3) destroy;
fakenpcname "Constable Bob", "Constable Bob#_D", 421;
```
```lua
if not self:puppet("001-2", 104, 19, "Desert Mana Seed", 166) then server.mapexit() end
if rand(1, 20) ~= 3 then self:destroy() return end
npc.get("Constable Bob"):rename("Constable Bob#_D", 421)   -- AUDIT: real rename now
```
Section 18 has the rules and the audit note.

### Idiom 16: sc_start (`magic/level1-flare-dart.txt`, `029-3/parua.txt`)

```
sc_start SC_COOLDOWN, 500, 0, BL_ID;
sc_start SC_POISON, 1, @candor_idle_counter*25;
```
```lua
p:sc_start(SC_COOLDOWN, 500, 0)
p:sc_start(SC_POISON, 1, p.tmp.candor_idle_counter * 25)
```
The seconds heuristic is preserved (`tick < 1000` means seconds except for the always-ms
types): keep the numbers, never convert units.

### Idiom 17: getmapusers polling (`029-3/parua.txt`)

```
if (getmapusers("029-3") < 5) goto L_NotEnough;
```
```lua
if map.getmapusers("029-3") < 5 then return not_enough() end
```

### Idiom 18: bit ops on quest flags (`048-2/bluesageConfig.txt`, section 14)

`bit32.*` with the precedence rewrite rules; every `& | ^ << >>` expression is
AUDIT-flagged by the converter, resolve each against the section 14.1 table.

### Idiom 19: if_then_else (`commands/marry.txt`)

```
mes "##3##BYou"+ if_then_else(PARTNER, " are", "r partner is") +" already married.";
```
```lua
p:mes("##3##BYou" .. if_then_else(p.PARTNER, " are", "r partner is") .. " already married.")
```
Keep `if_then_else` (both branches evaluated, as before); `c and a or b` only in hand
cleanup with proven-safe operands (section 14.4).

### Idiom 20: int math (`025-4/battlecaves.txt`, `commands/gm.txt`)

```
heal ((MaxHp/100) * -10), 0;
set GM, (GM - (GM % 10)) + 1;
```
```lua
p:heal(idiv(p.MaxHp, 100) * -10, 0)
p.GM = (p.GM - imod(p.GM, 10)) + 1
```
`idiv`/`imod` everywhere old `/`/`%` appear; `int32()` where overflow is intended
(section 15).

### Idiom 21: string compares and `chr(3)` sentinels (`commands/_procedures.txt`)

```
set .@NULL$, chr(3);
if (.@check$[.@i] == .@NULL$) goto L_ParseDone;
```
```lua
local NULL = chr(3)
if check[i] == NULL then break end
```

### Idiom 22: free-standing `elif`/`else` (`magic/level1-flare-dart.txt`)

```
if (@level <= 2 && countitem("SulphurPowder") >= 1) delitem "SulphurPowder", 1;
elif (@level <= 2) end;
```
```lua
if p.tmp.level <= 2 and p:countitem("SulphurPowder") >= 1 then
    p:delitem("SulphurPowder", 1)
elseif p.tmp.level <= 2 then
    return
end
```
Always adjacent in content; merge into one chain.

### Idiom 23: cross-NPC state (`001-1/ched.txt`, `#FerryConfig`)

```
set @invocation$, get(.invocation$, "detect-magic");
set .warp_delay, 20000, "#FerryConfig";
setarray .arr, "ConfigNpc", 1, 2, 3;
```
```lua
p.tmpstr.invocation = npc.get("detect-magic").varstr.invocation
npc.get("#FerryConfig").vars.warp_delay = 20000
setarray(array(npc.get("ConfigNpc").vars, "arr"), 0, 1, 2, 3)
```
Append form: `local t = array(npc.get("N").vars, "arr"); setarray(t, getarraysize(t), ...)`.

### Idiom 24: GM commands (`commands/mute.txt`, section 19.2)

```
registercmd chr(ATCMD_SYMBOL) + "mute", strnpcinfo(0);
registercmd chr(ATCMD_SYMBOL) + "stfu", strnpcinfo(0) + "::OnSTFU";
```
```lua
server.registercmd(chr(ATCMD_SYMBOL) .. "mute", self.name)          -- click body
server.registercmd(chr(ATCMD_SYMBOL) .. "stfu", self.name .. "::OnSTFU")
```
Handler `(self, p, argstring)`; `argv_splitter(argstring)` (ported helper) returns a
0-based table; permission checks against `p.GM` and `CMD_*` constants.

### Idiom 25: the spell system (`magic/level1-flare-dart.txt`)

Verbatim structure: `on_init` sets `self.vars.school` / `self.varstr.invocation` and
calls `magic_register(self)`; the click body is the cast; the attack handler reads
`args.target_id`; `p:overrideattack(..., self.name .. "::OnAttack", n)`. Complete port in
section 26.

### Idiom 26: attachrid/detachrid (`025-3/barriers.txt`, section 10.6)

```
if (attachrid(getcharid(3,$@cave1fighter$)) == 0) goto OnNoRid;
```
```lua
local fighter = players.byname(worldtmpstr.cave1fighter)
if not fighter then return on_no_rid(self) end
```

### Idiom 27: `@inventorylist_*` (`001-1/adrian.txt`)

```
getinventorylist;
if ((checkweight("LeatherSuitcase", 1) == 0) || (@inventorylist_count == 100))
    goto L_Inventory;
```
```lua
local inv = p:getinventorylist()
if not p:checkweight("LeatherSuitcase", 1) or #inv == 100 then
    return inventory_full()
end
```
Entries are `{id=, amount=, equip=, index=}` in a 1-based sequence.

### Idiom 28: items by name/const/id (`functions/vault.txt`)

```
getitem "LeatherSuitcase", 1;
getitem WhiteCake, 1;
getitem 501, 3;
```
```lua
p:getitem("LeatherSuitcase", 1)
p:getitem(WhiteCake, 1)                   -- const-aegis global, unchanged spelling
p:getitem(501, 3)
```
All three forms remain valid everywhere an item is expected.

### Idiom 29: quest-log views (`001-1/adrian.txt`)

```
if (QL_KYLIAN == 1) goto L_Started;
set QL_KYLIAN, 2;
```
```lua
if p.vars.QL_KYLIAN == 1 then return started() end
p.vars.QL_KYLIAN = 2
```
The `QL_*` aliasing into the packed `STARTAREA` field lives below the variable layer and
keeps working verbatim.

### Idiom 30: `debug` constant gate (`029-3/parua.txt`)

```
if (debug >= 2) end;
```
```lua
if debug >= 2 then return end
```

### Idiom 31: `##` vault (`functions/vault.txt`)

```
set ##00_INFO, call("set_byte", ##00_INFO, 3, 0);
```
```lua
p.acc2["00_INFO"] = set_byte(self, p, p.acc2["00_INFO"], 3, 0)
```
Digit-leading `##` names always use bracket syntax on `p.acc2`.

### Idiom 32: statement `void call(...)` (`functions/spawns_on_mobkill.txt`)

```
void call("spawn_mobs_around", getmap(), @mobX, @mobY, AngrySeaSlime, rand(8, 16));
```
```lua
spawn_mobs_around(self, p, p.map, args.mobX, args.mobY, AngrySeaSlime, rand(8, 16))
```
A plain call statement (Lua discards results of statement calls).

### Idiom 33: item-use dialog hop (`npc/items/anchor_stone.txt`)

```
function|script|useAnchorStone
{
    addtimer 0, "AS_Core::OnUse";
    return;
}
```
```lua
function useAnchorStone(self, p)
    p:addtimer(0, "AS_Core::OnUse")       -- @StoneName$ style args stay in p.tmpstr
end
```
Mechanical pass: keep the hop verbatim (the event is queued while the player is in a
dialog, exactly as before; the hop target reads its `p.tmpstr` arguments). Hand cleanup
may instead open the dialog directly in the use script, which is now allowed
(section 20); keep the hop whenever the intent is "after this handler ends".

---------------------------------------------------------------------------------------------------

## 22. Behaviour changes: the audit list

Everything the port deliberately changes, and what to do at each occurrence. The
converter emits `-- AUDIT:` comments at each detected site; every one must be resolved
(fix or justify) before the file's commit. Everything NOT listed here is preserved,
including the quirks: `mob.mobcount` minus-one, the `initnpctimer` no-op quirk, the
`sc_start` seconds heuristic, menu stop-at-first-empty, `input` negative termination,
shop `"*N"` prices. Keep old numbers and comparisons verbatim; do not "fix" quirks.

| # | change | porter action at each occurrence |
|---|---|---|
| 1 | `getarraysize` of an empty array returns 0 (was 1) | verify the use is a loop bound or append index (census: all are); a site that stored/compared the old minimum 1 would need `max(1, ...)`, none exist |
| 2 | `and`/`or` short-circuit; `if` evaluates its condition before the guarded call's arguments | check the right operand (and the guarded arguments) for side effects; hoist into a local if any (census: none) |
| 3 | menu cancel terminates the handler; out-of-range ignored; entries past an empty string not selectable | none (matches the dominant old path); menus needing a cancel value use `tbl.cancel` |
| 4 | menu entry expressions evaluated once (old RERUNLINE re-ran the statement on resume) | verify entries are pure expressions (census: all are) |
| 5 | a Close click while any prompt is outstanding abandons the dialog | none (old could resume a closed window; unsafe) |
| 6 | strict argument checking: wrong type/arity/extra args raise | fix at lint/`--check-scripts` time; the raise is the point |
| 7 | `rename` really renames (old `fakenpcname` left the NPC unaddressable) | at each of the 29 uses check follow-up `npc.get`/event strings for which name they must use (doomsday files) |
| 8 | `get(PLAINVAR, target)` reads the permanent variable via `target.vars.X` (old read temp regs: a bug) | verify the site wanted the permanent var (they all did) |
| 9 | `.@` locals survive `next`/`menu` | none (strictly less surprising) |
| 10 | `self:addnpctimer` registers the slot on the calling handle (old: on the NPC named in the event string) | none (only slot accounting differs) |
| 11 | `map.foreach` without a caller runs the handler with `caller == nil` (old ended the script) | guard `caller` uses with a nil check if the old code relied on the silent stop |
| 12 | booleans replace 0/1 for condition-only returns (`isdead`, `isloggedin`, `isin`, `isat`, `sc_check`, `checkweight`, `mapexists`, `iscollision`, `issummon`, `marriage`, `divorce`, `getpvpflag(1)`) | rewrite comparisons: `== 0` -> `not`, `!= 0`/`== 1` -> the bare value; never compare a boolean to a number |
| 13 | unknown map raises in `map.getmapmaxx/y`, `map.iscollision`, `map.getmaphash` (was a silent script abort) | add a `map.mapexists` guard where old code relied on the abort (e.g. validating user input) |
| 14 | `mob.monster` `"this"` map and `x,y <= 0` conveniences are gone | pass `p.map`, `p.x`, `p.y` explicitly |
| 15 | duplicate NPC names are a fatal load error (old: warn and replace) | none in serverdata; a porting collision shows up in `--check-scripts` |
| 16 | a registered command mid-dialog abandons the dialog, then runs | play-test spells cast mid-dialog; observable behaviour matches old minus the corruption |
| 17 | `@menu` and the `@inventorylist_*`/`@skilllist_*`/`$@MobDrop_*` registers replaced by return values | mechanical (sections 8, 11); nothing survives in `p.tmp` |
| 18 | `p:bonus` outside `pc_calcstatus` logs a warning (still applies) | leave the 10 NPC-script call sites; the log is informational |
| 19 | player timer slots exhausted (33rd pending timer) logs a line (old: silent drop) | none |
| 20 | `freeloop` dropped | delete the statement (instruction budget is per-resume and large) |
| 21 | `p:requestitem` returns a sequence (names with the `true` flag) instead of filling `$` arrays | mechanical rewrite at each site |
| 22 | `puppet` returns a handle or nil (was id or 0) | `< 1` checks become `not` checks (idiom 15) |

---------------------------------------------------------------------------------------------------

## 23. Per-file verification checklist

A file is done when every box ticks:

1. No `PORTME(` left; the embedded original-source `--[==[ ]==]` block is deleted.
2. No `-- AUDIT:` comment left; each was resolved and the decision is in the commit
   message.
3. `tools/lua-port/lint.py <file>` is clean: syntax, forbidden subset (no `goto`, native
   bitwise ops, `//`, `require`/`load`/`io`/`debug`, `os.*` beyond time/clock/date),
   no undefined globals, no new-global writes outside load/on_init.
4. `tmwa-map --check-scripts=load` passes from `world/map` (full `--check-scripts` once
   the tree's on_init chain is ported; final trees must pass the full check).
5. Every old `/` and `%` on ints became `idiv`/`imod`; every `& | ^ ~ << >>` became
   `bit32.*` with grouping checked against section 14.1; every string `+` became `..`.
6. Every int condition has its explicit comparison (`~= 0` etc.) unless the callee now
   returns a boolean (section 22 item 12).
7. `@` variables: locals only where the census/converter shows no cross-handler or
   cross-file readers; everything else in `p.tmp`/`p.tmpstr` with names unchanged.
8. Every handler that showed dialog text ends via `p:close()`, `p:close2()`, `p:shop`,
   or a documented fall-off; `self:destroy()` is followed by `return`.
9. Event strings (`"Npc::OnX"`) reference names that exist in the tree
   (`--check-scripts` verifies label references it can see; grep for dynamic ones).
10. For dialog-bearing NPCs: an e2e scenario or manual client session covered the
    mes/next/menu/input/close paths touched by the port.
11. One old file -> one commit; the message names the old file and the AUDIT decisions.

---------------------------------------------------------------------------------------------------

## 24. Worked example A: a simple dialogue NPC (`001-1/gossip.txt`)

A menu-dispatch NPC: one menu, seven label targets, one shared epilogue. This is the
bread-and-butter shape of most dialogue files.

Old (`world/map/npc/001-1/gossip.txt`, complete):

```
// NPC to provide hints on progression of linear quest chain in Tulimshar

001-1,66,104,0|script|Gladys|154
{
    mes "[Gladys]";
    mes "\"Hello deary! I hear some of the most fascinating rumors!\"";
    next;
    mes "\"Would you like me to let you in on the good stuff?\"";
    menu
        "Tell me about Lt. Dausen.", L_Intro,
        "Tell me about Fieri.", L_Cook,
        "Tell me about Sarah.", L_Sarah,
        "Tell me about Sandra.", L_Sandra,
        "Tell me about the Desert Mine.", L_Desert,
        "Tell me about the Ferry.", L_Ferry,
        "No thanks.", L_No;

L_No:
    mes "[Gladys]";
    mes "\"Very well. Come back later if you want to hear some juicy news!\"";
    goto L_Close;

L_Intro:
    mes "[Gladys]";
    mes "\"Have you met our lovely guard captain yet? He usually keeps post just outside the gates to keep an eye on things. He likes to talk to new adventurers too.\"";
    goto L_Close;

L_Cook:
    mes "[Gladys]";
    mes "\"I hear the kitchen at the Magic Institute of Tulimshar is in need of some help.\"";
    next;
    mes "\"The only thing bigger then a wizard's ego is their waistline. Hehe.\" %%8";
    goto L_Close;

L_Sarah:
    mes "[Gladys]";
    mes "\"Do you believe what parents will let their kids do these days?! I heard about this girl in the southeast side of town who does nothing but eat sweets all day! Honestly, what is this world coming to?\"";
    goto L_Close;

L_Sandra:
    mes "[Gladys]";
    mes "\"The students and wizards at the Magic Institute of Tulimshar are always looking for people to gather the reagents they need for spells as they are far too busy studying to gather the materials themselves.\"";
    goto L_Close;

L_Desert:
    mes "[Gladys]";
    mes "\"If you're still looking for adventure, I would suggest talking to some of the guards. I hear there's a sizable monster threat outside the city walls!\"";
    goto L_Close;

L_Ferry:
    mes "[Gladys]";
    mes "\"There's a ferry in the northern part of town that takes travelers to exotic new places!\"";
    next;
    mes "\"I haven't been on it myself, but I hear it travels all over the world!\"";
    goto L_Close;

L_Close:
    close;
}
```

New (`world/map/npc/001-1/gossip.lua`, complete):

```lua
-- NPC to provide hints on progression of linear quest chain in Tulimshar

npc.script{
    name = "Gladys", map = "001-1", x = 66, y = 104, dir = 0, sprite = 154,
    on_click = function(self, p)
        p:mes("[Gladys]")
        p:mes("\"Hello deary! I hear some of the most fascinating rumors!\"")
        p:next()
        p:mes("\"Would you like me to let you in on the good stuff?\"")
        local c = p:menu("Tell me about Lt. Dausen.",
                         "Tell me about Fieri.",
                         "Tell me about Sarah.",
                         "Tell me about Sandra.",
                         "Tell me about the Desert Mine.",
                         "Tell me about the Ferry.",
                         "No thanks.")
        if c == 1 then                    -- L_Intro
            p:mes("[Gladys]")
            p:mes("\"Have you met our lovely guard captain yet? He usually keeps post just outside the gates to keep an eye on things. He likes to talk to new adventurers too.\"")
        elseif c == 2 then                -- L_Cook
            p:mes("[Gladys]")
            p:mes("\"I hear the kitchen at the Magic Institute of Tulimshar is in need of some help.\"")
            p:next()
            p:mes("\"The only thing bigger then a wizard's ego is their waistline. Hehe.\" %%8")
        elseif c == 3 then                -- L_Sarah
            p:mes("[Gladys]")
            p:mes("\"Do you believe what parents will let their kids do these days?! I heard about this girl in the southeast side of town who does nothing but eat sweets all day! Honestly, what is this world coming to?\"")
        elseif c == 4 then                -- L_Sandra
            p:mes("[Gladys]")
            p:mes("\"The students and wizards at the Magic Institute of Tulimshar are always looking for people to gather the reagents they need for spells as they are far too busy studying to gather the materials themselves.\"")
        elseif c == 5 then                -- L_Desert
            p:mes("[Gladys]")
            p:mes("\"If you're still looking for adventure, I would suggest talking to some of the guards. I hear there's a sizable monster threat outside the city walls!\"")
        elseif c == 6 then                -- L_Ferry
            p:mes("[Gladys]")
            p:mes("\"There's a ferry in the northern part of town that takes travelers to exotic new places!\"")
            p:next()
            p:mes("\"I haven't been on it myself, but I hear it travels all over the world!\"")
        elseif c == 7 then                -- L_No
            p:mes("[Gladys]")
            p:mes("\"Very well. Come back later if you want to hear some juicy news!\"")
        end
        p:close()                         -- L_Close
    end,
}
```

What happened: the header line became `npc.script{...}` fields; the menu labels became
branch numbers in entry order; seven `goto L_Close` collapsed into the shared `p:close()`
after the chain; text, markup (`%%8`), and escapes are byte-identical.

---------------------------------------------------------------------------------------------------

## 25. Worked example B: a callfunc function and its caller (`functions/inn.txt`, `036-2/shops.txt`)

The `Inn` function uses the classic `@`-variable protocol: callers set `@npcname$` and
`@cost`, then `callfunc "Inn"`. First pass keeps that protocol through `p.tmp`/`p.tmpstr`
so every caller file can be ported independently.

Old (`world/map/npc/functions/inn.txt`, complete):

```
// INN

function|script|Inn
{
    if(@npcname$ == "") set @npcname$, strnpcinfo(1);
    mes "[" + @npcname$ + "]";
    mes "\"Would you like to rest? It's only " + @cost + " gp.\"";
    next;
    menu
        "Yes", L_Next,
        "No", L_Close;

L_Next:
    if (Zeny < @cost)
        goto L_NoMoney;
    set Zeny, Zeny - @cost;
    heal 10000, 10000;

    mes "[" + @npcname$ + "]";
    mes "\"Sleep well!\"";
    next;
    goto L_Close;

L_Close:
    mes "[" + @npcname$ + "]";
    mes "\"See you.\"";
    set @npcname$, "";
    close2;
    return;

L_NoMoney:
    mes "[" + @npcname$ + "]";
    mes "\"You don't have enough money to stay here.\"";
    next;
    goto L_Close;
}
```

New (`world/map/npc/functions/inn.lua`, complete):

```lua
-- INN
-- callfunc protocol (first pass, kept):
--   Input:  @npcname$ (optional; defaults to the calling NPC's basename), @cost
--   Return: none; clears @npcname$
-- Second-pass candidate signature (all callers at once): Inn(self, p, cost [, npcname])

function Inn(self, p)
    if p.tmpstr.npcname == "" then p.tmpstr.npcname = self.basename end
    local function finish()               -- L_Close
        p:mes("[" .. p.tmpstr.npcname .. "]")
        p:mes("\"See you.\"")
        p.tmpstr.npcname = ""
        p:close2()
    end
    p:mes("[" .. p.tmpstr.npcname .. "]")
    p:mes("\"Would you like to rest? It's only " .. p.tmp.cost .. " gp.\"")
    p:next()
    local c = p:menu("Yes", "No")
    if c == 2 then return finish() end
    -- L_Next
    if p.Zeny < p.tmp.cost then           -- L_NoMoney
        p:mes("[" .. p.tmpstr.npcname .. "]")
        p:mes("\"You don't have enough money to stay here.\"")
        p:next()
        return finish()
    end
    p.Zeny = p.Zeny - p.tmp.cost
    p:heal(10000, 10000)
    p:mes("[" .. p.tmpstr.npcname .. "]")
    p:mes("\"Sleep well!\"")
    p:next()
    return finish()
end
```

Old caller (`world/map/npc/036-2/shops.txt`, complete):

```
//

036-2,23,38,0|shop|Chef Armand|211,Beer:*1,Steak:*2,CasinoCoins:*1
036-2,35,22,0|shop|Gunney|138,Arrow:*4,IronArrow:*2,SilverArrow:*1

036-2,23,35,0|script|Bunkmaster Daban|212
{
    set @npcname$, "Bunkmaster Daban";
    set @cost, 100;
    callfunc "Inn";
    set @npcname$, "";
    set @cost, 0;
    close;
}
```

New (`world/map/npc/036-2/shops.lua`, complete):

```lua
npc.shop{ name = "Chef Armand", map = "036-2", x = 23, y = 38, dir = 0, sprite = 211,
          items = { {"Beer", "*1"}, {"Steak", "*2"}, {"CasinoCoins", "*1"} } }
npc.shop{ name = "Gunney", map = "036-2", x = 35, y = 22, dir = 0, sprite = 138,
          items = { {"Arrow", "*4"}, {"IronArrow", "*2"}, {"SilverArrow", "*1"} } }

npc.script{ name = "Bunkmaster Daban", map = "036-2", x = 23, y = 35, dir = 0, sprite = 212,
    on_click = function(self, p)
        p.tmpstr.npcname = "Bunkmaster Daban"
        p.tmp.cost = 100
        Inn(self, p)
        p.tmpstr.npcname = ""
        p.tmp.cost = 0
        p:close()
    end }
```

What happened: `@npcname$`/`@cost` stayed session variables with unchanged names
(first-pass protocol), so the seven other Inn caller files translate the same way without
touching this one. The `L_Close` epilogue (targeted from three places) became the local
`finish()` closure. The old caller sequence `close2; return;` then `close;` in the caller
is preserved exactly (close2 yields until the Close click, then the caller's `p:close()`
ends the handler, matching old packet order). The shop lines became `npc.shop`
constructors with the `"*N"` price syntax verbatim.

---------------------------------------------------------------------------------------------------

## 26. Worked example C: a magic spell (`magic/level1-flare-dart.txt`)

The full spell shape: guarded cast in the click body, attack override with per-attack
handler, recast scheduling through a 0 ms player timer, cross-handler spell state, and
`on_init` registration.

Old (`world/map/npc/magic/level1-flare-dart.txt`, complete):

```
-|script|flare-dart|32767
{
    if(call("magic_checks")) end;
    if (Sp < 10) end;
    set @level, getskilllv(.school);
    if (getskilllv(SKILL_MAGIC) < .level) end;
    if (@level <= 2 && countitem("SulphurPowder") >= 1) delitem "SulphurPowder", 1;
    elif (@level <= 2) end;
    set @_M_BLOCK, 1; // block casting, until the timer clears it
    addtimer 500, "Magic Timer::OnClear"; // set the new debuff
    sc_start SC_COOLDOWN, 500, 0, BL_ID;
    callfunc "adjust_spellpower";
    set Sp, Sp - 10;
    set CASTS, CASTS + 1;
    if (CASTS < 0) set CASTS, 1; // overflow
    misceffect FX_MAGIC_DART_CAST, strcharinfo(0);
    setarray @flarspell[0],
        sqrt(@spellpower) * 5, //dmg
        (BaseLevel/3) + 5, // dmg bonus
        (@spellpower/50) + 3, // charges
        (((200 - (Agi+Agi2)) * 1200) / 200); // delay
    callfunc "magic_exp";
    goto L_FreeRecast;

OnAttack:
    if (target(BL_ID, @target_id, 50) != 50) goto L_FreeRecast; // 0x20 | 0x02 | 0x10
    void call("elt_damage", @flarspell[0], @flarspell[1], ELT_WATER, ELT_FIRE, FX_NONE);
    set @flarspell[2], @flarspell[2] - 1;
    goto L_FreeRecast;

L_FreeRecast:
    if (@flarspell[2] > 0)
        addtimer 0, strnpcinfo(0) + "::OnSetRecast";
    end;

OnDischarge:
    if (@flarspell[2] < 1) end;
    set @flarspell[2], 0;
    misceffect FX_MAGIC_DISCHARGE, strcharinfo(0);
    overrideattack;
    end;

OnSetRecast:
    overrideattack @flarspell[3], 4, ATTACK_ICON_GENERIC, OVERRIDE_DART, strnpcinfo(0)+"::OnAttack", @flarspell[2];
    end;

OnInit:
    set .school, SKILL_MAGIC_WAR;
    set .invocation$, chr(MAGIC_SYMBOL) + "flar"; // used in npcs that refer to this spell
    void call("magic_register");
    set .level, 1;
    set .exp_gain, 1;
    end;
}
```

New (`world/map/npc/magic/level1-flare-dart.lua`, complete):

```lua
-- @flarspell[] is written by the cast and read by OnAttack/OnDischarge/OnSetRecast:
-- cross-handler state, stays in p.tmp (guide section 10.5). Indices unchanged:
-- [0] dmg, [1] dmg bonus, [2] charges, [3] delay.

local function free_recast(self, p)       -- L_FreeRecast (shared by cast and OnAttack)
    local spell = array(p.tmp, "flarspell")
    if spell[2] > 0 then
        p:addtimer(0, self.name .. "::OnSetRecast")
    end
end

npc.script{
    name = "flare-dart",                  -- floating NPC; sprite 32767 is the default
    on_click = function(self, p)          -- the cast: run when the invocation is spoken
        if magic_checks(self, p) ~= 0 then return end
        if p.Sp < 10 then return end
        p.tmp.level = p:getskilllv(self.vars.school)
        if p:getskilllv(SKILL_MAGIC) < self.vars.level then return end
        if p.tmp.level <= 2 and p:countitem("SulphurPowder") >= 1 then
            p:delitem("SulphurPowder", 1)
        elseif p.tmp.level <= 2 then
            return
        end
        p.tmp._M_BLOCK = 1                -- block casting, until the timer clears it
        p:addtimer(500, "Magic Timer::OnClear")   -- set the new debuff
        p:sc_start(SC_COOLDOWN, 500, 0)   -- 500 is one of the always-ms types; verbatim
        adjust_spellpower(self, p)        -- callfunc protocol: reads @level, writes @spellpower
        p.Sp = p.Sp - 10
        p.CASTS = int32(p.CASTS + 1)      -- param write wraps to int32; int32() documents it
        if p.CASTS < 0 then p.CASTS = 1 end       -- overflow
        p:misceffect(FX_MAGIC_DART_CAST)
        local spell = array(p.tmp, "flarspell")
        setarray(spell, 0,
            sqrt(p.tmp.spellpower) * 5,                   -- dmg
            idiv(p.BaseLevel, 3) + 5,                     -- dmg bonus
            idiv(p.tmp.spellpower, 50) + 3,               -- charges
            idiv((200 - (p.Agi + p.Agi2)) * 1200, 200))   -- delay
        magic_exp(self, p)
        return free_recast(self, p)
    end,
    events = {
        OnAttack = function(self, p, args)        -- overrideattack handler: synchronous
            local spell = array(p.tmp, "flarspell")
            if p:target(args.target_id, 50) ~= 50 then    -- 0x20 | 0x02 | 0x10
                return free_recast(self, p)
            end
            elt_damage(self, p, spell[0], spell[1], ELT_WATER, ELT_FIRE, FX_NONE)
            spell[2] = spell[2] - 1
            return free_recast(self, p)
        end,
        OnDischarge = function(self, p)           -- player timer target: (self, p)
            local spell = array(p.tmp, "flarspell")
            if spell[2] < 1 then return end
            spell[2] = 0
            p:misceffect(FX_MAGIC_DISCHARGE)
            p:overrideattack()                    -- bare overrideattack; = discharge
        end,
        OnSetRecast = function(self, p)
            local spell = array(p.tmp, "flarspell")
            p:overrideattack(spell[3], 4, ATTACK_ICON_GENERIC, OVERRIDE_DART,
                             self.name .. "::OnAttack", spell[2])
        end,
    },
    on_init = function(self)
        self.vars.school = SKILL_MAGIC_WAR
        self.varstr.invocation = chr(MAGIC_SYMBOL) .. "flar"  -- read by npcs that refer to this spell
        magic_register(self)              -- registers the invocation word for this NPC
        self.vars.level = 1
        self.vars.exp_gain = 1
    end,
}
```

What happened: the four labels became handler fields; the shared `L_FreeRecast` became a
file-local function used by two handlers; `@flarspell` stayed a 0-based `p.tmp` array
with unchanged indices because three later handlers read it; `@level`/`@spellpower`
stayed in `p.tmp` because `adjust_spellpower`/`magic_exp` are callfunc-protocol functions
(their files port independently); `/` became `idiv`; `target(BL_ID, ...)` became
`p:target(...)`; `misceffect FX, strcharinfo(0)` became `p:misceffect(FX)`; the
`.school`/`.invocation$` NPC variables became `self.vars`/`self.varstr` (dialogue NPCs
read the invocation via `npc.get("flare-dart").varstr.invocation`); every `end;` became
`return` (or fall-off at the end of a handler); the CASTS overflow check is verbatim, it
still works because the param write wraps to int32.

---------------------------------------------------------------------------------------------------

## Appendix A: the callfunc signature registry (`funcdefs.tsv`)

Seeded by the converter, maintained by the porting agents, one row per
`function|script|Name`. Tab-separated columns:

| column | content |
|---|---|
| `name` | the (mangled) Lua global name |
| `kind` | `callfunc` (implicit `@` protocol), `call` (explicit args), `both` |
| `reads` | `@`/`@$` names the body reads before writing (the protocol inputs) |
| `writes` | `@`/`@$` names the body writes (outputs and scratch) |
| `signature` | the current Lua signature, e.g. `Inn(self, p)` for first-pass protocol functions, `get_byte(self, p, v, id) -> int` for converted ones |
| `status` | `stub`, `ported-pass1`, `ported-pass2` |

Rules: `call`-kind functions get their real parameter list immediately (the old call
sites pass explicit arguments, section 10.1). `callfunc`-kind functions stay
`(self, p)` with the `@` protocol until a second pass converts the function AND all its
callers in one change; the chosen signature is recorded here first, so two agents never
convert the same function to different signatures. Before porting any call site, look the
function up here; if its row says `stub`, port against the documented old protocol (the
Input:/Return: header comment) and keep the `@` names.
