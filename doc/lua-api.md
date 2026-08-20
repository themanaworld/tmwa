# TMWA Lua scripting API reference

This is the complete, binding reference for the script-facing API of the TMWA map server's
Lua 5.4 engine. It replaces the eAthena script language documented nowhere. Every entry gives
the signature, parameter types, return value, error behaviour, semantics, and the old builtin
or construct it replaces ("Old:"). The porting guide (`lua-porting-guide.md`) contains the
inverse mapping (old -> new) as translation tables.

Scripts target the portable Lua subset (see section 3): no `goto`, no native bitwise
operators, no `//`, no `<const>`/`<close>`, no `require`/`load`/`io`, `os` limited to
`time`/`clock`/`date`. The provided `bit32` library and integer helpers cover the gaps.

---------------------------------------------------------------------------------------------------

## 1. Conventions and types

### 1.1 Handles

Game objects are Lua tables with a metatable carrying the integer id; the engine re-resolves
the id on every call. There is no `__gc`. Kinds:

* **player handle** (`p`): one per login session, identity-stable (`p == p2` holds for the
  same session). Obtained as a handler argument or from `players.*`.
* **NPC handle** (`self`): one per NPC (map-placed, floating, puppet). Obtained as a handler
  argument or from `npc.get(name)` / `npc.byid(id)`.
* **being handle** (`being(id)`): any block (player, mob, NPC, floor item). Player and NPC
  handles are also beings (shared metatable base).

A handle whose object no longer exists (player logged out, NPC destroyed, mob dead) makes
every method return `nil`/`false`/`0` after logging one warning, and every property read
return `nil`; property writes are ignored with a warning. `h:exists()` tells. Writing an
unknown field on a handle raises (`player handle has no field 'foo' (use p.tmp for script
data)`). Do not store handles in persistent or long-lived tables; store ids and re-resolve.

### 1.2 Integers

All numbers crossing the C++ boundary are C `int` (32-bit). Argument checks accept Lua
integers and integral floats; fractional floats and non-numbers raise
`bad argument #n to 'f' (integer expected)`. One wrapping rule everywhere: an integral value
out of int32 range wraps to int32 at every write boundary (persistent storage, params,
packet fields), mirroring C `int` assignment; the `int32(x)` global exists as documentation
sugar for intentional overflow (the `CASTS` idiom). Division and modulo of ported code use
`idiv`/`imod` (C truncation semantics; raise on zero divisor). Numeric strings are NOT
coerced (`"5"` is not an integer argument; use `atoi`).

### 1.3 Strings

String arguments accept Lua strings. Numbers are accepted where display text is expected
(`p:mes(5)` prints "5", formatted `%d`) but nowhere else. Embedded NUL bytes are rejected
(wire formats are NUL-terminated).

### 1.4 Booleans

Functions whose old 0/1 return was only ever used as a condition now return `false`/`true`:
`isdead`, `isloggedin`, `isin`, `isat`, `sc_check`, `checkweight`, `mapexists`,
`iscollision`, `issummon`, `marriage`, `divorce`, `getpvpflag(1)`. Everything that returned
a count or id keeps returning an integer.

### 1.5 Errors

* Argument errors (wrong type, wrong arity, extra arguments, out-of-range constants) raise a
  Lua error: the traceback is logged, the handler is aborted, an open dialog is closed
  cleanly. The old engine silently ignored extra arguments; raising catches translation
  mistakes.
* Game-state failures (unknown item, offline player, missing NPC, unknown map) do NOT raise:
  they log a warning with NPC/event context and return `nil`/`false`/`0`, exactly as the old
  engine logged and continued.
* At load time (while content files are loading, including `on_init`), any error is fatal:
  the server refuses to start (or `--check-scripts` exits 1).

### 1.6 Yielding (dialog) functions

`p:next`, `p:menu`, `p:input`, `p:input_str`, `p:close`, `p:close2`, `p:openstorage`,
`p:requestitem`, `p:requestlang`, `p:shop` suspend the running handler until the client
answers. They may only be called from a dialog-capable handler for that same player: NPC
click, touch, targeted events with a player (timers, per-spawn mob death, queued events,
`npc.event(ev, p)`), command handlers, and item use scripts. Calling one from a synchronous
handler (OnInit, NPC timers, `OnPC*Event`, `OnMobKillEvent`, `foreach` callbacks, equip
scripts, `overrideattack` handlers) raises `dialog primitive outside a dialog context`.

### 1.7 Handler signature

Every NPC handler is `function(self, p, args)`: `self` is the NPC handle, `p` the player
handle or nil (OnInit, NPC timers, global events without a player), `args` an event-specific
table, or the argument string for commands, or nil. Plain Lua functions replacing
`function|script|Name` take whatever parameters the porter gives them; the convention is
`(self, p, ...)`.

Old: the implicit attached rid/oid pair. There is no implicit attachment; every operation
names its object through a handle.

---------------------------------------------------------------------------------------------------

## 2. Global functions

Available in every chunk. All are provided by the engine (C++ or the built-in prelude).

| name | signature | returns | semantics | Old |
|---|---|---|---|---|
| `rand` | `rand(n)` / `rand(a, b)` | int | `[0,n)` (0 if n <= 0); inclusive `[a,b]`, swapped if reversed | `rand` |
| `idiv` | `idiv(a, b)` | int | C truncating division; raises on `b == 0` | `/` operator |
| `imod` | `imod(a, b)` | int | C `%` (sign of dividend); raises on `b == 0` | `%` operator |
| `int32` | `int32(x)` | int | wraps to int32 like a C assignment | overflow idiom (`CASTS`) |
| `atoi` | `atoi(s)` | int | old `conv_num` rule: leading decimal digits with optional sign, else 0; numbers pass through | implicit string->int coercion |
| `tostr` | `tostr(v)` | string | old `conv_str`: ints via `%d`, strings unchanged, other types raise | implicit int->string coercion |
| `chr` | `chr(n)` | string | 1-char string | `chr` |
| `ord` | `ord(s)` | int | code of the first char, 0 for `""` | `ord` |
| `l` | `l(s, ...)` | string | returns `s` unchanged (translation placeholder) | `l` |
| `if_then_else` | `if_then_else(c, a, b)` | a or b | old ternary; `c` is a number (nonzero = true) or boolean; both branches are evaluated before the call (Lua call semantics, same as old) | `if_then_else` |
| `min`, `max` | `min(a, b, ...)` / `max(a, b, ...)` | int | integer extremum of 2+ args | `min`, `max` (2+ arg form) |
| `average` | `average(a, b, ...)` | int | integer mean of 2+ args | `average` |
| `sqrt`, `cbrt`, `pow` | `sqrt(n)`, `cbrt(n)`, `pow(a, b)` | int | truncated-to-int libm results (old semantics); `math.sqrt` also exists and returns a float | `sqrt`, `cbrt`, `pow` |
| `setarray` | `setarray(t, start, v1, v2, ...)` | none | writes `t[start], t[start+1], ...`; indices clamp at 255 as before; `t` is a plain table or a `world.array` proxy | `setarray` |
| `cleararray` | `cleararray(t, start, value, count)` | none | `t[start .. start+count-1] = value` (clamped at 255) | `cleararray` |
| `getarraysize` | `getarraysize(t)` | int | index of the last element that is non-nil, non-0, non-`""` plus 1, scanning 0..255; **0 for an empty array** (old returned 1; audit note in the guide); accepts a non-table (returns 0) so `getarraysize(p.tmp.never_set)` is safe | `getarraysize` |
| `array_search` | `array_search(needle, t [, start])` | int | first index >= start (default 0) with `t[i] == needle`, else -1 | `array_search` |
| `explode` | `explode(s, sep)` | table | splits on the **first character** of `sep` into a **0-based** array of strings (max 256 pieces) | `explode` (string array) |
| `explode_int` | `explode_int(s, sep)` | table | same, each piece through `atoi` | `explode` (int array) |
| `arrmin`, `arrmax` | `arrmin(t [, start])` / `arrmax(t [, start])` | int | old one-argument `min(arr)`/`max(arr)`: scan `[start, 256)`, initial values -10 / 0 exactly as before | `min(arr)`, `max(arr)` |
| `array` | `array(scope, name)` | table | returns `scope[name]` if it is a table, else creates a 0-defaulting int array table, stores it under `scope[name]`, returns it; `scope` is `p.tmp`, `self.vars`, `worldtmp` or any plain table | `@arr[i]`, `.arr[i]`, `$@arr[i]` |
| `arraystr` | `arraystr(scope, name)` | table | same with `""` default (for `$`-suffixed array names) | `@arr$[i]` |
| `being` | `being(id)` | handle or nil | block_list by id (player, mob, NPC, floor item); see section 6 | `BL_ID` targets |
| `stop` | `stop()` | never returns | terminates the current handler from any nesting depth without an error log; if a dialog line was shown since the last prompt the client gets the Close button | `end` at depth |
| `import` | `import(path)` | none | loads another Lua content file once (path relative to the map server working directory, like conf paths); fatal at startup on error; raises if called after startup | conf `import:` (script layer) |
| `print` | `print(...)` | none | goes to the server log, prefixed with the current NPC/event context | (new) |

Notes:
* `distance`, `target`, `injure` exist as being methods and as id-taking functions in
  `server` (section 9.5).
* `bit32` is a library, see section 3.

---------------------------------------------------------------------------------------------------

## 3. Standard library subset

The sandbox provides exactly (D2):

* Base: `tostring`, `tonumber`, `type`, `pairs`, `ipairs`, `next`, `select`, `unpack`
  (= `table.unpack`), `error`, `assert`, `pcall`, `xpcall`, `rawget`, `rawset`, `rawequal`,
  `rawlen`, `setmetatable`, `getmetatable`, `_VERSION`. `collectgarbage` is restricted to
  `"count"`. `print` is redirected to the server log.
* Libraries: `string`, `table`, `math` (`math.random` is backed by the same engine RNG as
  `rand`), `utf8`, `coroutine` (read-only; scripts must not build their own dialog
  coroutines), `os` limited to `os.time`, `os.clock`, `os.date` (`os.date` is forced to UTC).
* `bit32`: `band`, `bor`, `bxor`, `bnot`, `lshift`, `rshift`, `arshift`, `btest`, `extract`,
  `replace`. Implemented in C++ (Lua 5.4 has no bit32). Results are returned as signed int32
  so `bit32.band(FLAGS, MASK)` compares equal to old values.
  Old: the `&`, `|`, `^`, `~`, `<<`, `>>` script operators (note the old engine's non-C
  precedence; see the porting guide, operators section).

Not available: `goto` (compiles but is forbidden by lint for Luau portability), native
bitwise operators, `//`, `load`, `loadfile`, `loadstring`, `dofile`, `require`, `io`,
`debug`, `package`, `os.*` beyond the three above, `<const>`, `<close>`.

Strict globals: reading an undefined global raises `undefined global 'X'` (catches typos in
function, constant, and API names at first use). Creating a new global is allowed while
content files load and during `on_init`; after `on_init` completes, a new global assignment
logs a warning at runtime and is an error under `--check-scripts` and lint. Reading a global
whose name is a params.txt name gives a targeted message
(`undefined global 'Zeny' (did you mean p.Zeny?)`).

---------------------------------------------------------------------------------------------------

## 4. Constants

All names from the `const_db` files (`const.txt`, `const-magic.txt`, `const-quest.txt`,
`const-mapflags.txt`, `const-debugflag.txt`, `permissions*.txt`, and the item and mob names
from `const-aegis.txt`) are read-only globals with the same names. Assigning to a constant
name raises `attempt to assign constant 'X'`.

Params (`params.txt` entries with the `1` flag: `Zeny`, `Hp`, `BaseLevel`, `Sex`, `CASTS`,
`GM`, ...) are NOT globals: they are properties on player/being handles (`p.Zeny`), spelled
exactly as in params.txt. The `b*` SP constants used by `bonus` (`bStr`, `bInt`, `bMaxHP`,
...) are integer constants (the SP ids), usable with `p:bonus` and `p:param`.

Old: `const_db` constants were script globals; params were magic bareword variables.

---------------------------------------------------------------------------------------------------

## 5. Player handle `p`

Obtained as the second handler argument, from `players.byid(blockid)`,
`players.byname(name)`, `players.bycharid(charid)`, `players.all()`, `players.onmap(map)`,
`players.inarea(map, x0, y0, x1, y1)`, or `being(id)` when the block is a player.

### 5.1 Properties

| property | r/w | type | semantics | Old |
|---|---|---|---|---|
| `p.id` | r | int | account id = block id | `BL_ID`, `getcharid(3)` |
| `p.charid` | r | int | char id | `getcharid(0)`, `CHAR_ID` |
| `p.partyid` | r | int | 0 if none | `getcharid(1)` |
| `p.guildid` | r | int | always 0 | `getcharid(2)` |
| `p.name` | r | string | character name | `strcharinfo(0)` |
| `p.partyname` | r | string | `""` if none | `strcharinfo(1)` |
| `p.map` | r | string | `""` if on no/undefined map | `getmap()` |
| `p.x`, `p.y`, `p.dir` | r | int | position and facing | `getx()`, `gety()`, `getdir()` |
| `p.dead` | r | bool | | `isdead()` |
| `p.gmlevel` | r | int | same value as `p.GM` | `getgmlevel()` |
| `p.partnerid` | r | int | partner char id, 0 unmarried | `getpartnerid2()` |
| `p.version` | r | int | client version | `getversion()` |
| `p.opt2` | r/w | int | write triggers `clif_changeoption` + status recalc when changed | `getopt2()` / `setopt2` |
| `p.pvpchannel` | r/w | int | write clamps at 0 | `getpvpflag(0)` / `setpvpchannel` |
| `p.hidden` | r | bool | Opt0::HIDE | `getpvpflag(1)` |
| `p.mask` | r | int | the player's map mask; -1 if no map | `getmask()` |
| `p.online` | r | bool | false once the session is gone | `isloggedin(id)` |
| `p.type` | r | string | `"player"` | `BL_TYPE` |
| `p.<Param>` | per param | int | every name from `db/params.txt` with the `1` flag, exact case (`p.Zeny`, `p.Hp`, `p.BaseLevel`, `p.CASTS`, `p.MUTE_GLOBAL`, ...). Read = `pc_readparam`; write = `pc_setparam`. Writing a read-only param raises. Values wrap to int32 on write | the bareword params |

The property set is data-driven from params.txt: a new params.txt line is automatically a
new property.

`p:param(sp_id) -> int` reads any param by its numeric SP id (needed where the old code
stored a `b*` constant in a variable, e.g. `RequireStat`). `p:setparam(sp_id, v)` writes
(raises for read-only SPs). Old: `get(Int, id)` with an SP number.

### 5.2 Variable scopes on the player

| accessor | kind | default | semantics | Old |
|---|---|---|---|---|
| `p.vars.NAME` | proxy, int r/w | 0 | permanent char variables (`global_reg`, 96 entries, names <= 31 chars). Write 0 or nil deletes. Name > 31 chars raises. Overflow of the 96-cap logs an error and the write returns normally. Quest-log aliasing (`QL_*` names) keeps working (it lives in `pc_setglobalreg`). `p.vars:names()` returns the sequence of set names (for maintenance scripts) | `NAME` (plain) |
| `p.acc.NAME` | proxy, int r/w | 0 | account variables (16 entries); stored with the `#` prefix on the wire, byte-identical formats | `#NAME` |
| `p.acc2.NAME` | proxy, int r/w | 0 | login-server account variables (16 entries), stored with `##` | `##NAME` |
| `p.tmp.NAME` | plain table | 0 | per-session int temporaries; missing keys read 0; may hold any Lua value (arrays via `array(p.tmp, "name")`); dies at logout | `@NAME` |
| `p.tmpstr.NAME` | plain table | `""` | per-session string temporaries; missing keys read `""` | `@NAME$` |

Names that are not Lua identifiers use bracket syntax: `p.acc2["00_INFO"]`.
`@x` and `@x$` land in different tables, so nothing collides.

### 5.3 Dialog methods

All operate on the NPC the player is currently attached to (`sd->npc_id`). See 1.6 for
where they may be called. A handler that returns with a dialog still open gets an automatic
close (0x00b6 if text was shown, else `npc_action 5`).

| method | returns | semantics | Old |
|---|---|---|---|
| `p:mes(...)` | none | each argument is one dialog line (0x00b4); numbers formatted `%d`; `p:mes()` sends one blank line. Sets the "text shown" flag that selects the close packet | `mes` |
| `p:mesq(text)` | none | the text wrapped in quotes | `mesq` |
| `p:mesn([name])` | none | `[name]`; default is the dialog NPC's basename; inside an item-use dialog the name is required (raises without it) | `mesn` |
| `p:clear()` | none | clears the dialog text (`npc_action 9`) | `clear` |
| `p:next()` | none | yields (0x00b5) until the client clicks Next (0x00b9) | `next` |
| `p:close()` | never returns | sends 0x00b6 (or `npc_action 5` if nothing was shown), then terminates the handler (yield-and-discard). Code after `p:close()` never runs, also when called from a nested Lua function | `close` |
| `p:close2()` | none | same packets, yields until the client clicks Close (0x0146; 0x00b9 is also accepted, as in the old engine), then continues | `close2` |
| `p:menu(s1, s2, ...)` | int | variadic form. Sends `"s1:s2:...:"`; the list stops at the first empty string (empty choices hide the rest, old behaviour); yields; returns the 1-based choice. Entries hidden by the empty-string cut are NOT selectable (old: a crafted client could select them; intentional fix). Client cancel (0xff) terminates the handler like `close` without a packet. An out-of-range answer is ignored (the prompt stays outstanding). `nil` arguments raise | `menu` |
| `p:menu(tbl)` | value, index | table form, for dynamic menus. `tbl` is a 1-based sequence whose entries are strings or pairs `{text, value}` (value = any Lua value, including a function); entries that are `false` or `""` are skipped (not sent, not selectable) but positions keep counting. Returns `value, index` for the pair form, `index` for the string form. Optional keys: `tbl.title = s` calls `p:title(s)` first; `tbl.cancel = v` makes client cancel return `v` instead of terminating the handler. Replaces the 40-slot `@choice_n$` idiom | `menu` (dynamic) |
| `p:input()` | int | yields (0x0142), returns the int (0x0143). A negative answer sends `clif_tradecancelled` plus the close packet and terminates the handler (old behaviour) | `input @x` |
| `p:input_str()` | string | yields (0x01d4), returns the string (0x01d5), possibly empty | `input @x$` |
| `p:requestitem([amount [, names]])` | table | amount clamped 1..16 (default 1); yields; returns a 1-based sequence of item ids present in the inventory (or item names if `names` is true), same filtering as before | `requestitem` |
| `p:requestlang()` | string | yields, returns the language string | `requestlang` |
| `p:title(text)` | none | dialog window title (`clif_npc_send_title`) | `title` |
| `p:shop(npcname)` | never returns | closes the dialog, opens the named shop NPC's buy/sell window, terminates the handler | `shop` |
| `p:openstorage()` | none | opens the storage window and yields until the client closes it; returns false without yielding when storage cannot open (trading, ...) | `openstorage` |
| `p:npcaction(cmd [, id [, x [, y]]])` | none | raw `clif_npc_action`; for cmd 2 `id` may be an NPC name, an NPC handle, or an id | `npcaction` |
| `p:camera()` / `p:camera(x, y)` / `p:camera(actor [, dx, dy])` | none | `actor` is a handle, an id, `"relative"`, `"rid"`/`"player"`, `"oid"`/`"npc"`, or an NPC name | `camera` |

Protocol rules (engine-enforced, new): the outstanding prompt kind must match the client's
answer packet; mismatched packets are logged and ignored, EXCEPT that a Close click (0x0146)
while any prompt is outstanding abandons the dialog cleanly (the client already closed the
window; the old engine would have resumed the script, which could grant rewards on a closed
window, or soft-locked the player). Menu entry expressions are evaluated once (the old
RERUNLINE mechanism re-ran the whole menu statement on resume; the census found only pure
expressions in menu arguments).

`@menu` is gone: the value the old code read from `@menu` is `p:menu`'s return value.

### 5.4 State, stats, status

| method | returns | semantics | Old |
|---|---|---|---|
| `p:heal(hp, sp [, itemheal])` | none | identical incl. resurrection and invincibility timer; negative damages | `heal` |
| `p:getexp(base, job)` | none | negative ignored | `getexp` |
| `p:setlook(type, val)` / `p:getlook(type)` | none / int | `LOOK_*` constants; getlook returns -1 for unsupported types | `setlook` / `getlook` |
| `p:savepoint(map, x, y)` | none | | `savepoint` |
| `p:getsavepoint()` | map, x, y | three return values | `getsavepoint(0/1/2)` |
| `p:resetstatus()` | none | | `resetstatus` |
| `p:bonus(type, val)` / `p:bonus2(type, t2, val)` | none | `type` = `b*` constant (SP id). Intended for equip scripts (inside `pc_calcstatus`); called elsewhere it logs a warning and still applies (the bonus lasts until the next status recalc, old behaviour; 10 NPC-script call sites exist) | `bonus` / `bonus2` |
| `p:sc_start(type, tick, val1)` | none | keeps the tick heuristic: `tick < 1000` means seconds (multiplied by 1000) unless the type is one of `SC_PHYS_SHIELD`, `SC_PHYS_SHIELD_ITEM`, `SC_MBARRIER`, `SC_COOLDOWN*`, `SC_SLOWMOVE`, `SC_CANTMOVE` (always ms). Do not convert units when porting | `sc_start` (rid form) |
| `p:sc_end(type)` | none | | `sc_end` |
| `p:sc_check(type)` | bool | | `sc_check` |
| `p:marriage(partnername)` | bool | | `marriage` |
| `p:divorce()` | bool | | `divorce` |
| `p:isat(map, x, y)` | bool | | `isat` |
| `p:isin(map, x0, y0, x1, y1)` | bool | inclusive | `isin` |
| `p:warp(map, x, y)` | none | `pc_setpos(GONE)`; no nowarp flag check (as today) | `warp` |
| `p:mapmask(mask [, persist])` | none | sends the mask; with `persist` also stores it on the map | `mapmask` |
| `p:sendcollision(map, mask, x1, y1 [, x2, y2])` | none | the old char-name target form becomes a call on that player's handle | `sendcollision` |
| `p:music(name)` | none | | `music` |
| `p:skill(id, level [, flag])` | none | | `skill` |
| `p:setskill(id, level)` | none | | `setskill` |
| `p:getskilllv(id)` | int | | `getskilllv` |
| `p:getactivatedpoolskilllist()` | table | 1-based sequence of `{id=, lv=, flag=, name=}` | `getactivatedpoolskilllist` (`@skilllist_*`) |
| `p:getunactivatedpoolskilllist()` | table | same shape | `getunactivatedpoolskilllist` |
| `p:poolskill(id)` / `p:unpoolskill(id)` | none | | `poolskill` / `unpoolskill` |
| `p:overrideattack(delay, range, icon, look, event [, charges])` | none | `event` = `"Npc::OnX"` string or function `(self, p, args)` with `args.target_id`; runs synchronously per attack. `p:overrideattack()` discharges | `overrideattack` |

### 5.5 Inventory and equipment

| method | returns | semantics | Old |
|---|---|---|---|
| `p:countitem(item)` | int | `item` = id or exact name; unknown -> 0 plus log | `countitem` |
| `p:checkweight(item, amount)` | bool | | `checkweight` |
| `p:getitem(item, amount)` | none | inventory overflow drops to the floor as before; the old target forms become calls on that player's handle | `getitem` |
| `p:delitem(item, amount)` | none | removes what exists, no error | `delitem` |
| `p:getinventorylist()` | table | 1-based sequence of `{id=, amount=, equip=, index=}`; `#seq` replaces `@inventorylist_count` | `getinventorylist` (`@inventorylist_*`) |
| `p:getequipid(pos)` | int | pos 1..11 (raises outside); returns the item id, 0, or -1 exactly as before | `getequipid` |
| `p:nude()` | none | | `nude` |
| `p:unequipbyid(slot)` | none | slot = EQUIP index (range-checked) | `unequipbyid` |

### 5.6 Messages and effects

| method | semantics | Old |
|---|---|---|
| `p:message(text)` | server-style whisper to this player; for another player: `players.byname(n):message(text)` | `message strcharinfo(0), text` |
| `p:smsg([type,] text)` | | `smsg` |
| `p:remotecmd(cmd)` | | `remotecmd` |
| `p:emotion(type)` | the player emotes | `emotion type, "self"` |
| `p:misceffect(fx)` | effect on the player | `misceffect fx, charname/id` |
| `p:specialeffect(fx)` | | `specialeffect2` |
| `p:announce(text, flag)` | local announce with the player as source (`flag & 8 == 0`); see `server.announce` | `announce` |
| `p:gmlog(text)` | | `gmlog` |

### 5.7 Timers and events on the player

| method | returns | semantics | Old |
|---|---|---|---|
| `p:addtimer(ms, event)` | none | `event` = `"Npc::OnX"` string or function `(self, p)`. 32 pending timers per player; when full the timer is dropped with a log line (old: silent). On fire the event runs through the normal event path with this player attached: queued 100 ms at a time while the player is in a dialog, same-map/in-area gate for map-placed NPCs, disabled NPC drops it. For a function, `self` is the NPC of the handler that called `addtimer` (nil in item scripts) | `addtimer` |
| `p:event(event [, args])` | bool | runs a named event with this player attached; same as `npc.event(event, p, args)` | `npc_event(sd, ev)` |

---------------------------------------------------------------------------------------------------

## 6. Being handle `being(id)`

For any block id. Player and NPC handles are beings too.

| member | returns | semantics | Old |
|---|---|---|---|
| `b.id`, `b.type` | int, string | type is `"player"`, `"mob"`, `"npc"`, `"item"` | `BL_ID`, `BL_TYPE` |
| `b.x`, `b.y`, `b.map` | int, int, string | | `POS_X`, `POS_Y` |
| `b.<Param>` | int | `pc_readparam` works for mobs and NPCs as before (`Hp`, `MaxHp`, `Class`, `BaseLevel`, `Str`..`Luk`, ...); writes go through `pc_setparam` (no-ops for most non-player params, as before) | `get(Hp, id)`, `set(Class, v, id)` |
| `b:exists()` | bool | | `isloggedin(id)` for players |
| `b:sc_start(type, tick, val1)` | none | same tick heuristic as 5.4 | `sc_start type, tick, val, id` |
| `b:sc_end(type)`, `b:sc_check(type)` | none, bool | | `sc_end`, `sc_check` |
| `b:distance(other)` | int | `other` = handle or id; `0x7fffffff` on different maps; invalid id raises (old crashed) | `distance(id1, id2)` |
| `b:target(other, flags)` | int | same bitmask | `target` |
| `b:injure(other, damage)` | none | | `injure` |
| `b:aggravate([target])` | none | `b` must be a mob; `target` handle or id | `aggravate` |
| `b:issummon()` | bool | false for non-mobs | `issummon` |
| `b:misceffect(fx)`, `b:emotion(type)` | none | | `misceffect fx, id` |

Id-taking forms for spell code that passes raw ids: `server.distance(id1, id2)`,
`server.target(src, tgt, flags)`, `server.injure(src, tgt, dmg)` (section 9.5).

---------------------------------------------------------------------------------------------------

## 7. NPC handle `self` / `npc.get(name)` / `npc.byid(id)`

| member | r/w | semantics | Old |
|---|---|---|---|
| `self.id` | r | block id | `getnpcid()` |
| `self.name` | r | full name incl. `#suffix` | `strnpcinfo(0)` |
| `self.basename` | r | before the first `#` | `strnpcinfo(1)` |
| `self.suffix` | r | from `#` to end, `""` if none | `strnpcinfo(2)` |
| `self.map` | r | `""` for floating NPCs | `strnpcinfo(3)` |
| `self.x`, `self.y` | r | | `getnpcx()`, `getnpcy()` |
| `self.dir` | r/w | | |
| `self.sprite` | r/w | write = disable + re-enable with the new class (the `fakenpcname X, X, sprite` idiom) | `set Class, v, npcid` |
| `self.sex` | r/w | | `Sex` param on an NPC |
| `self.enabled` | r | bool | `!(flag & 1)` |
| `self.parent` | r | parent NPC handle for puppets, nil otherwise | |
| `self.vars.NAME` | table | per-NPC int vars; missing keys read 0; any Lua value allowed; arrays via `array(self.vars, "x")`; cross-NPC: `npc.get("Name").vars.x` | `.NAME` |
| `self.varstr.NAME` | table | per-NPC string vars; missing keys read `""` | `.NAME$` |

Methods:

| method | returns | semantics | Old |
|---|---|---|---|
| `self:enable()` / `self:disable()` | none | `npc_enable` incl. the OnTouch re-fire for players inside the area | `enablenpc` / `disablenpc` |
| `self:warp(x, y)` | none | same-map move (disable + move + enable) | `npcwarp x, y, name` |
| `self:areawarp(x0, y0, x1, y1 [, avoid_collision])` | none | random reposition in the rectangle | `npcareawarp` |
| `self:setdirection(dir, sit, save [, p])` | none | with `p`: only that player sees it | `setnpcdirection` |
| `self:talk(text [, p])` | none | | `npctalk` |
| `self:emotion(type [, p])` | none | the NPC emotes (towards one player if `p`) | `emotion` |
| `self:misceffect(fx)` | none | oid default form | `misceffect fx` |
| `self:specialeffect(fx)` | none | | `specialeffect` |
| `self:announce(text, flag)` | none | NPC as source (`flag | 8`) | `announce` |
| `self:rename(newname [, sprite])` | none | fixed semantics (design decision): updates `npcs_by_name` and the event registry keys, re-spawns; afterwards the NPC is addressable only by the new name. The old builtin left the NPC unaddressable by either name; census: no content relies on the old half-rename, the 29 uses are audited during porting | `fakenpcname old, new, sprite` |
| `self:initnpctimer()` | none | identical state machine incl. the no-op quirk while the timer is counting toward its first label (kept verbatim: zero audit cost for the 67 uses) | `initnpctimer` |
| `self:startnpctimer()` / `self:stopnpctimer()` | none | | `startnpctimer` / `stopnpctimer` |
| `self:getnpctimer(type)` | int | 0 tick, 1 active, 2 number of timer labels | `getnpctimer` |
| `self:setnpctimer(ms)` | none | | `setnpctimer` |
| `self:addnpctimer(ms, event)` | none | `event` = `"Npc::OnX"` string or function `(self)`; the timer slot lives on `self` (old: on the NPC named in the string; only slot accounting differs) | `addnpctimer` |
| `self:puppet(map, x, y, name, sprite [, xs, ys])` | handle or nil | nil if the name is taken (old returned 0). The puppet shares all handlers of `self` (events, timers, click body), has its own `vars`/`varstr`, timers, id, and name; excluded from broadcasts; freed when the parent is | `puppet` |
| `self:destroy()` | none | schedules `npc_free` (deferred to the next tick, as before), cancels its timers, dequeues an attached player. Does NOT terminate the handler: translate bare `destroy;` as `self:destroy() return` (or `stop()` at depth) | `destroy` |
| `self:event(label [, p [, args]])` | bool | shorthand for `npc.event(self.name .. "::" .. label, p, args)` | `donpcevent name+"::"+label` |
| `self:exists()` | bool | false after destroy | |

---------------------------------------------------------------------------------------------------

## 8. Content constructors and functions

All constructors take one table; unknown keys raise at load (typo guard). `name` values are
NpcName (<= 23 bytes; longer raises). Unknown maps, items, species, or mapflags are fatal at
load. Registering a duplicate NPC name is a fatal load error (design decision: the old
engine warned and replaced; serverdata has no duplicates and silent replacement hides
porting mistakes).

### 8.1 `npc.script{...}` -> NPC handle

```lua
npc.script{
    name    = "Aisha",            -- required, unique
    map     = "001-1", x = 102, y = 27, dir = 0,   -- omit map/x/y for a floating NPC
    sprite  = 108,                -- default 32767 (INVISIBLE); required for visible map NPCs
    xs = 2, ys = 2,               -- touch radius in cells, the old file numbers (engine stores 2n+1)
    on_click = function(self, p) ... end,          -- the old body (label ""); touch runs it when there is no on_touch
    on_touch = function(self, p) ... end,          -- OnTouch
    on_init  = function(self) ... end,             -- OnInit (runs once after all files loaded)
    on_timer = { [5000] = function(self) ... end },-- OnTimerNNNN (N > 0)
    events   = {                                   -- every other label, verbatim old names
        OnBoard = function(self, p) ... end,
        OnPCLoginEvent = function(self, p) ... end,    -- broadcast labels keep their names
        OnClock0000 = function(self) ... end,
        OnCommandTalk = function(self) ... end,
    },
}
```

`on_click`/`on_touch`/`on_init`/`on_timer[N]` are sugar for `events[""]`,
`events.OnTouch`, `events.OnInit`, `events["OnTimer<N>"]`; giving both forms for one label
raises. The engine recognizes the broadcast labels (`OnPCLoginEvent`, `OnPCLogoutEvent`,
`OnPCDieEvent`, `OnPCKillEvent`, `OnMobKillEvent`) and the clock patterns (`OnMinuteMM`,
`OnClockHHMM`, `OnHourHH`, `OnDayMMDD`) inside `events` and indexes them for dispatch.

Old: `map,x,y,d|script|Name|sprite[,xs,ys]{...}` and `-|script|Name|32767{...}`.

### 8.2 `npc.warp{...}`

```lua
npc.warp{ map = "001-1", x = 63, y = 119, xs = 3, ys = -1,
          to_map = "002-1", to_x = 63, to_y = 21 }
```
`xs`/`ys` are exactly the old file numbers (the engine adds 2, as before). The name is
auto-generated as before. Old: `map,x,y|warp|name|xs,ys,to_map,to_x,to_y`.

### 8.3 `npc.shop{...}`

```lua
npc.shop{ name = "Neko", map = "001-1", x = 106, y = 105, dir = 0, sprite = 101,
          items = { {"TonoriDelight", "*10"}, {"CactusDrink", "*1"}, {"Beer", 120} } }
```
Price: an int is absolute; `"*N"` means `value_buy * N` (old syntax verbatim). Unknown item
is fatal. Old: `map,x,y,d|shop|Name|sprite,item:price,...`.

### 8.4 `npc.monster{...}` (permanent spawn)

```lua
npc.monster{ map = "001-1", x = 32, y = 59, xs = 10, ys = 11, name = "GreenSlime",
             species = 1005, amount = 8, delay1 = 100000, delay2 = 30000
             [, event = "Npc::OnX"] }
```
Delays in ms; `name` may be `"--en--"`/`"--ja--"` as before; amount is scaled by
`mob_count_rate`; `species` may be a const-aegis name. Old:
`map,x,y,xs,ys|monster|Name|species,amount,delay1,delay2[,event]`.

### 8.5 `npc.mapflag{...}`

```lua
npc.mapflag{ map = "001-1", flag = "town" }
npc.mapflag{ map = "botcheck", flag = "nosave", to = "001-1", x = 10, y = 10 }
npc.mapflag{ map = "033-1", flag = "mask", mask = 3 }
```
Old: `map|mapflag|flag[|args]`.

### 8.6 Functions

Old `function|script|Name{...}` bodies become plain global Lua functions:
`function Banker(self, p) ... end`, called as `Banker(self, p)`. Old `callfunc "Name"` and
`call("Name", a, b)` become plain calls; see the porting guide for the `@`-variable calling
convention rules. File-local helpers are `local function`s. Names with spaces are mangled
(`Easter Debug` -> `Easter_Debug`; the converter emits the mapping).

---------------------------------------------------------------------------------------------------

## 9. Namespaces

### 9.1 `npc`

| function | returns | semantics | Old |
|---|---|---|---|
| `npc.get(name)` | handle or nil | exact full name | `npc_name2id` uses |
| `npc.byid(id)` | handle or nil | | |
| `npc.exists(name)` | bool | | |
| `npc.enable(name)` / `npc.disable(name)` | bool | false plus log if unknown | `enablenpc` / `disablenpc` |
| `npc.event(event [, p [, args]])` | bool | `event` = `"Name::OnX"`, `"Name::"` (click body), or `"::OnX"` (broadcast to all NPCs with that label, puppets skipped). Without `p`: synchronous, no player, disabled flag ignored (old `donpcevent`). With `p`: the player-attached event path (queue-if-busy, same-map/in-area gate for map NPCs, disabled drop, sets `sd->npc_id` during the run). `args` (a table) becomes the handler's third argument; when `args ~= nil` the event is never queued (the old "with args" rule). Called from inside a dialog handler for the same player, the target handler runs inline in the same coroutine and may itself use dialog primitives. Returns true if the handler ran or was queued, false if not found/dropped | `donpcevent`, `npc_event` |
| `npc.event_all(label [, p [, args]])` | none | same as `npc.event("::" .. label, ...)` | `npc_event_doall` |

### 9.2 `map`

| function | returns | semantics | Old |
|---|---|---|---|
| `map.areawarp(map, x0, y0, x1, y1, to_map, to_x, to_y)` | none | | `areawarp` |
| `map.mapwarp(map, to_map, to_x, to_y)` | none | | `mapwarp` |
| `map.getmapusers(map)` | int | -1 unknown map | `getmapusers` |
| `map.getareausers(map, x0, y0, x1, y1 [, living])` | int | | `getareausers` |
| `map.getmapflag(map, flag)` | int | `flag` = `MF_*` constant; -1 unknown map | `getmapflag` |
| `map.setmapflag(map, flag)` / `map.removemapflag(map, flag)` | none | | `setmapflag` / `removemapflag` |
| `map.pvpon(map)` / `map.pvpoff(map)` | none | | `pvpon` / `pvpoff` |
| `map.iscollision(map, x, y)` | bool | raises on unknown map (old aborted the script silently) | `iscollision` |
| `map.getmapmaxx(map)` / `map.getmapmaxy(map)` | int | raise on unknown map (old aborted) | `getmapmaxx` / `getmapmaxy` |
| `map.getmaphash(map)` | int | | `getmaphash` |
| `map.getmapnamefromhash(h)` | string | | `getmapnamefromhash` |
| `map.mapexists(map)` | bool | | `mapexists` |
| `map.numberofmaps()` | int | | `numberofmaps` |
| `map.getmapnamebyindex(i)` | string | | `getmapnamebyindex` |
| `map.mapannounce(map, text, flag)` | none | | `mapannounce` |
| `map.getareadropitem(map, x0, y0, x1, y1, item [, delete])` | int | | `getareadropitem` |
| `map.makeitem(item, amount, map, x, y)` | none | `"this"` is not accepted: pass `p.map` | `makeitem` |
| `map.areatimer(map, x0, y0, x1, y1, ms, event)` | none | one player timer per player in the rectangle; the old leading bltype `0` is dropped (only players were supported); `event` = string or function `(self, p)` | `areatimer` |
| `map.foreach(bltype, map, x0, y0, x1, y1, event [, caller])` | none | bltype 0 PC, 1 NPC, 2 MOB, 3 all; synchronous; handler `(self, caller, args)` with `args.target_id`; `caller` = player handle or nil (old ended the script without a rid; now nil is allowed) | `foreach` |
| `map.mask(map)` / `map.setmask(map, mask)` | int / none | | `getmask`/`mapmask` map fallback |
| `map.distance(map, x0, y0, x1, y1)` | int | euclidean, truncated | |

### 9.3 `mob`

| function | returns | semantics | Old |
|---|---|---|---|
| `mob.monster(map, x, y, name, species, amount [, event])` | eventtag | runtime spawn. `map` must be real (pass `p.map` for the old `"this"`; pass `p.x, p.y` for the old `x,y <= 0` convenience). `event` = `"Npc::OnX"` string, a `"~Tag"` phony tag, or a function `(self, p)`; returns the event string actually stored on the mobs (for a function: a synthetic tag) so `mob.killmonster`/`mob.mobcount` can use it | `monster` |
| `mob.areamonster(map, x0, y0, x1, y1, name, species, amount [, event])` | eventtag | | `areamonster` |
| `mob.summon(map, x, y, owner, name, species, attitude, lifespan_ms [, event])` | eventtag | `owner` = handle or id | `summon` |
| `mob.killmonster(map, event)` | none | `"All"` = every once-spawned mob | `killmonster` |
| `mob.mobcount(map, event)` | int | KEEPS the minus-one: 0 matching mobs returns -1 (98 call sites compare `< 0` / `<= 0`; do not "fix" this when porting) | `mobcount` |
| `mob.mobinfo(species, what)` | int or string | -1 invalid | `mobinfo` |
| `mob.getmobdrops(species)` | table, int | 1-based sequence of `{item=, name=, rate=}` plus the 0/1/2 status | `getmobdrops`, `mobinfo_droparrays` |
| `mob.checkid(species)` | bool | | |

### 9.4 `item`

| function | returns | Old |
|---|---|---|
| `item.getitemlink(item)` | string | `getitemlink` |
| `item.id(name)` | int (0 unknown) | |
| `item.name(id)` | string | |
| `item.exists(item)` | bool | |
| `item.weight(item)` | int | |

### 9.5 `players` and `server`

| function | returns | semantics | Old |
|---|---|---|---|
| `players.byid(id)` | handle or nil | nil when offline | `attachrid`, `map_id2sd` |
| `players.byname(name)` | handle or nil | online only | |
| `players.bycharid(charid)` | handle or nil | the old `>= 150000` char-id rule in `set`/`get` | |
| `players.all()` / `players.onmap(map)` / `players.inarea(map, x0, y0, x1, y1)` | table | 1-based sequences of handles (snapshots) | |
| `players.count()` | int | authed players on this map server | |
| `server.announce(text, flag [, source])` | none | `flag & 0xf == 0` is server-wide via the char server; otherwise `source` (player or NPC handle) is required | `announce` |
| `server.getusers(flag)` | int | 1 = world-wide count | `getusers` |
| `server.gettimetick(type)` | int | 0 ms tick, 1 seconds since UTC midnight, 2 unix seconds | `gettimetick` |
| `server.gettime(type)` | int | UTC, same type numbers | `gettime` |
| `server.tick()` | int | ms tick, same as `gettimetick(0)` | `gettick()` |
| `server.debugmes(text)` | none | log line with npc/player context | `debugmes` |
| `server.wgm(text)` | none | | `wgm` |
| `server.getbattleconfig(key)` | int | -1 unknown | `getbattleconfig` |
| `server.mapexit()` | none | sets runflag = 0; does not terminate the handler (as before) | `mapexit` |
| `server.isloggedin(id)` | bool | | `isloggedin` |
| `server.distance(id1, id2)` / `server.target(src, tgt, flags)` / `server.injure(src, tgt, dmg)` | int / int / none | id-taking forms of the being methods | `distance`, `target`, `injure` |
| `server.registercmd(word, handler)` | none | registers a chat word (GM command or spell invocation). `handler` is an event string: `"Npc::OnX"` or an NPC name (meaning the click body, the old `registercmd word, strnpcinfo(0)` idiom), with handler signature `(self, p, argstring)`; or a plain function `(p, argstring)` (per D6). Ported content uses the NPC/event form only, so exactly one signature appears in ported code. Dispatch: if the player is mid-dialog, the open dialog is abandoned first and the command runs (matches the old clobber path observably, without the stale-stack corruption). The handler is dialog-capable | `registercmd` |

### 9.6 `world`, `worldtmp`, `worldtmpstr` (server-wide variables)

| access | default | semantics | Old |
|---|---|---|---|
| `world.NAME` (r/w int) | 0 | mapreg index 0; write 0 or nil deletes; persisted in `mapreg.txt` as `$NAME` (byte-identical format) | `$NAME` |
| `world.str.NAME` (r/w string) | `""` | stored as `$NAME$`; write `""` or nil deletes | `$NAME$` |
| `world.get(name, idx)` / `world.set(name, idx, value)` | | `name` without the `$` prefix, with the trailing `$` for strings; `idx` 0..255 (raises outside) | `$NAME[idx]`, `$NAME$[idx]` |
| `world.array("NAME")` / `world.array("NAME$")` | | index proxy: `a[i]` get/set (0-based), `a:size()` (getarraysize rule, 0 when empty), `a:clear(start, count)`; accepted by the generic `setarray`/`cleararray`/`getarraysize`/`array_search` helpers (detected via metatable) | `$NAME[i]` arrays |
| `worldtmp.NAME` | 0 | plain Lua table, missing keys read 0; any Lua value allowed; arrays via `array(worldtmp, "NAME")`; not persisted | `$@NAME` |
| `worldtmpstr.NAME` | `""` | plain table, missing keys read `""` | `$@NAME$` |

---------------------------------------------------------------------------------------------------

## 10. Events: kinds, signatures, args

| event | fires when | handler signature | `args` | dialog-capable |
|---|---|---|---|---|
| click body (`on_click`, label `""`) | NPC click, attack-click, touch without on_touch, `npc.event("Name::", p)`, registered command word | `(self, p [, args])` | nil; the argument string for commands | yes |
| `on_touch` / `OnTouch` | stepping into the xs/ys area; enable re-fire | `(self, p)` | nil | yes |
| `on_init` / `OnInit` | once after all files load, before the clock starts | `(self)` | nil | no |
| `on_timer[N]` / `OnTimerN` | NPC timer reaching N ms | `(self)` | nil | no |
| `OnMinuteMM`, `OnClockHHMM`, `OnHourHH`, `OnDayMMDD` | clock timer (UTC), broadcast | `(self)` | nil | no |
| `OnPCLoginEvent` | once per login, broadcast | `(self, p)` | nil | no |
| `OnPCLogoutEvent` | logout, broadcast | `(self, p)` | nil | no |
| `OnPCDieEvent` | player death, broadcast | `(self, p)` | nil | no |
| `OnPCKillEvent` | player killed a player, broadcast, killer attached | `(self, p, args)` | `{victimrid = id}` | no |
| `OnMobKillEvent` | player killed a mob, broadcast, killer attached | `(self, p, args)` | `{mobID = species, mobX = x, mobY = y}` | no |
| per-spawn mob death event | mob with an event tag dies with a player killer; queued if the killer is mid-dialog | `(self, p)` | nil | yes |
| `p:addtimer` / `map.areatimer` events | timer fires; queued if the player is mid-dialog | `(self, p)` | nil | yes |
| `self:addnpctimer` events | timer fires, no player | `(self)` | nil | no |
| `map.foreach` callback | per found block, synchronous, nested | `(self, caller, args)` | `{target_id = id}` | no |
| `p:overrideattack` handler | per attack, synchronous | `(self, p, args)` | `{target_id = id}` | no |
| registered command (`server.registercmd`) | chat word; abandons an open dialog first, then runs | `(self, p, argstring)` (event form) / `(p, argstring)` (function form) | the rest of the chat line | yes |
| `npc.event(ev, p, args)` | explicit | `(self, p, args)` | the given table | yes (inline when called from the same player's dialog) |
| item use script | item used | chunk body with `p`, `args` | `{itemId = id}` | yes (see section 11) |
| item equip script | inside `pc_calcstatus` | chunk body with `p`, `args` | `{itemId = id, slotId = slot}` | no |

Old labels map verbatim: the porting guide's label table is 1:1.

Dialog context rule (preserved): a handler may use dialog primitives iff it started with a
player through the click/touch/event/timer/command/item-use paths marked dialog-capable.
To open a dialog from a synchronous handler use the `p:addtimer(0, "Npc::OnX")` hop (the
existing serverdata idiom) or `p:addtimer(0, fn)`.

---------------------------------------------------------------------------------------------------

## 11. Item scripts

`item_db_*.txt` keeps its columns; the two `{...}` columns contain Lua. The text between
the outer braces is compiled as a chunk with the prelude `local p, args = ...`. The engine's
brace scanner ignores braces inside Lua short strings (`"..."`, `'...'`, with backslash
escapes); `--` comments and long brackets are NOT recognised inside item columns (an item
line is one physical line; do not use comments there). Compile errors are fatal at startup.

* **Use scripts** run in a dialog coroutine and MAY open dialogs (new capability, per D1):
  while a dialog is open, the player is attached to the engine-owned invisible NPC
  `#itemdialog`, so all walk/attack/item gates behave as for any dialog. `p:mesn()` requires
  an explicit name there. `args = { itemId = <id> }`. The old `addtimer 0, "Npc::OnUse"` hop
  keeps working unchanged.
  Old: the use-script column; dialogs in use scripts were silently broken before.
* **Equip scripts** run synchronously inside `pc_calcstatus` with
  `args = { itemId = <id>, slotId = <slot> }` and must only call `bonus`-style functions,
  read state, or schedule timers; dialog primitives raise. `@slotId`/`@itemId` map to
  `args.slotId`/`args.itemId`.
  Old: the equip-script column.

---------------------------------------------------------------------------------------------------

## 12. Variable scope mapping (summary)

| old | Lua | default | notes |
|---|---|---|---|
| `.@x`, `.@x$` | `local x` | porter initialises (`= 0` / `= ""`) | locals survive `next`/`menu` now (they are Lua locals in a coroutine) |
| `@x` | `p.tmp.x` | 0 | session lifetime; shared across all scripts (callfunc conventions keep working) |
| `@x$` | `p.tmpstr.x` | `""` | |
| `@arr[i]` | `array(p.tmp, "arr")[i]` | 0 | 0-based, indices unchanged |
| `.x` / `.x$` | `self.vars.x` / `self.varstr.x` | 0 / `""` | cross-NPC: `npc.get("Name").vars.x` |
| `$@x` / `$@x$` | `worldtmp.x` / `worldtmpstr.x` | 0 / `""` | |
| `$x` / `$x$` / `$x[i]` | `world.x` / `world.str.x` / `world.array("x")[i]` | 0 / `""` | persisted, format unchanged |
| `x` (char permanent, incl. `QL_*`) | `p.vars.x` | 0 | int only, 31-char names, 96-entry cap |
| `#x` | `p.acc.x` | 0 | 16-entry cap |
| `##x` | `p.acc2.x` | 0 | 16-entry cap |
| params (`Zeny`, ...) | `p.Zeny` | | exact params.txt spelling |
| `@menu` | `p:menu` return value | | |
| `@args$` | the `argstring` handler parameter | | |
| `@mobID/@mobX/@mobY`, `@victimrid`, `@target_id`, `@slotId/@itemId` | the `args` table | | not mirrored into `p.tmp` |
| `@inventorylist_*`, `@skilllist_*`, `$@MobDrop_*` | return values of the respective methods | | |

---------------------------------------------------------------------------------------------------

## 13. Deliberate semantic deviations

Everything not listed here is preserved, including the quirks: `mob.mobcount` minus-one,
the `initnpctimer` no-op quirk, the `sc_start` seconds heuristic, menu stop-at-first-empty,
`input` negative termination, shop `"*N"` prices.

1. `getarraysize` of an empty array returns 0 (was 1). No census site depends on 1.
2. `and`/`or` short-circuit, and `if` evaluates its condition before the guarded statement's
   arguments (Lua semantics). The census found no content depending on the old order.
3. `menu` cancel or out-of-range no longer falls through: cancel ends the handler (as the
   dominant old path), out-of-range is ignored; entries hidden past an empty string are not
   selectable (old: crafted clients could select them).
4. Menu entry expressions are evaluated once (old RERUNLINE re-evaluated the statement).
5. A Close click (0x0146) while a Next/Menu/Input prompt is outstanding abandons the dialog
   (old: resumed the script as if answered; unsafe).
6. Strict argument checking raises instead of coercing; extra arguments raise.
7. `fakenpcname` -> `self:rename` really renames (updates registries).
8. `get(PLAINVAR, target)` semantics: `target.vars.X` reads the permanent variable (the old
   builtin read temp regs: a bug).
9. `.@` locals survive `next`/`menu`.
10. `self:addnpctimer` registers the slot on the calling handle.
11. `map.foreach` without a caller runs with `p == nil` instead of ending the script.
12. Booleans replace 0/1 where the value was only ever a condition (1.4).
13. Unknown map in `getmapmaxx/y`, `iscollision`, `getmaphash` raises (was a silent abort).
14. `mob.monster` `"this"` / `x,y <= 0` conveniences are gone: pass `p.map`, `p.x`, `p.y`.
15. Duplicate NPC names are a fatal load error (old: warn and replace).
16. A registered command received while the player is mid-dialog abandons that dialog, then
    runs (old: clobbered the dialog state buffer in place; observably similar, no
    corruption).
17. `@menu` and the `@inventorylist_*`/`@skilllist_*`/`$@MobDrop_*` result registers are
    replaced by return values.
18. `p:bonus` outside `pc_calcstatus` logs a warning (still applies, as before).
19. Timer slots exhausted (`p:addtimer` beyond 32) logs (old: silent drop).

---------------------------------------------------------------------------------------------------

## 14. Worked examples

### Example 1: dialogue NPC with menu, labels, goto (`001-1/children.txt` Aisha)

Old:
```
001-1,102,27,0|script|Aisha|108
{
    set @TEMP,rand(10);
    if(@TEMP == 1) goto L_1;
    ...
L_9:
    mes "\"I know a very bad word. But I can't say it because monsters will come and get me if I do!\"";
    next;
    menu
        "A bad word?", L_tell,
        "Oh. You better keep it to yourself then.", L_Close;
L_tell:
    ...
L_Close:
    set @TEMP, 0;
    close;
}
```
New (`npc/001-1/children.lua`):
```lua
npc.script{
    name = "Aisha", map = "001-1", x = 102, y = 27, dir = 0, sprite = 108,
    on_click = function(self, p)
        local lines = {
            [1] = "\"Maggots are so slimey!\" %%^",
            [2] = "\"Want to play ball with me?\"",
            [3] = "\"There are so many monsters! I hate scorpions!\" %%3",
            [4] = "\"When I grow up, I want to be strong enough to kill a scorpion!\" %%=",
            [5] = "\"Mommy told me that you can sell the things that monsters drop.\"",
            [6] = "\"Have you tried to eat a roasted maggot? They're sooo yummy!\" %%8",
            [7] = "\"I want to be a Doctor when I grow up!\"",
            [8] = "\"That earthquake was sooo scary! But now they've rebuilt everything.\"",
        }
        local temp = rand(10)
        if temp == 9 then
            p:mes("\"I know a very bad word. But I can't say it because monsters will come and get me if I do!\"")
            p:next()
            local c = p:menu("A bad word?", "Oh. You better keep it to yourself then.")
            if c == 1 then
                p:mes("[Aisha]")
                p:mes("\"I heard my mother say it once, and she made me promise her to never say it. ##BNever##b!\"")
                p:next()
                c = p:menu("If I promise to never tell anyone, can you tell me the word?",
                           "I understand. You don't need to tell me...",
                           "Goodbye!")
                if c == 1 then
                    p:mes("[Aisha]")
                    p:mes("\"No.\"")
                elseif c == 2 then
                    p:mes("[Aisha]")
                    p:mes("Aisha looks around as she leans in and whispers to you:")
                    p:mes("\"The bad word is '" .. npc.get("spell-aggravate").varstr.invocation .. ".'\"")
                    p:next()
                    p:mes("[Aisha]")
                    p:mes("\"But you can't tell anyone!\" %%>")
                end
            end
        else
            p:mes("[Aisha]")
            p:mes(lines[temp] or lines[1])   -- rand(10) gives 0..9; 0 falls back to L_1 like the old goto chain
        end
        p.tmp.TEMP = 0                       -- kept: other scripts might read @TEMP (mechanical safety)
        p:close()
    end,
}
```

### Example 2: callfunc function with callsub, close2 + openstorage, input, dynamic menu (`functions/banker.txt`)

```lua
-- npc/functions/banker.lua
local DEPOSITS = { 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000 }

function Banker(self, p)
    if p.vars.BankAccount ~= 0 then
        -- S_MoveAccount (callsub became inline code; a reused callsub becomes a local function)
        p.acc.BankAccount = p.acc.BankAccount + p.vars.BankAccount
        p.vars.BankAccount = 0
    end
    local function npcname()
        if p.tmpstr.npcname == "" then p.tmpstr.npcname = self.basename end
        return "[" .. p.tmpstr.npcname .. "]"
    end
    local function leave()               -- L_Return
        p.tmpstr.npcname = ""
    end
    while true do                        -- L_Start (backward gotos become a loop)
        p:mes(npcname())
        p:mes("\"Welcome to the bank!")
        p:mes("How can I help you?\"")
        p:next()
        local c = p:menu("Open my storage", "Deposit", "Withdraw", "Check my balance",
                         "Change Bank Options", "Nevermind")
        if c == 1 then                                             -- L_Storage
            if bit32.band(p.acc.BankOptions, OPT_STORAGE_CLOSE) ~= 0 then p:close2() end
            p:openstorage()
            if bit32.band(p.acc.BankOptions, OPT_STORAGE_CLOSE) ~= 0 then return leave() end
        elseif c == 2 then                                         -- L_Dep
            p:mes(npcname())
            p:mes("\"How much would you like to deposit?\"")
            p:next()
            local d = p:menu("Other", "5,000 GP", "10,000 GP", "25,000 GP", "50,000 GP",
                             "100,000 GP", "250,000 GP", "500,000 GP", "1,000,000 GP",
                             "All of my money", "I've changed my mind", "Quit")
            local amount = nil
            if d == 1 then                                         -- L_Dep_Input
                while true do
                    local a = p:input()
                    if a >= 0 then amount = a break end
                    p:mes(npcname())
                    p:mes("\"I need a positive amount. What would you like to do?\"")
                    local e = p:menu("Go back", "Try again", "Deposit all", "Nevermind")
                    if e == 1 then break
                    elseif e == 3 then amount = p.Zeny break
                    elseif e == 4 then
                        p:mes(npcname()) p:mes("\"Goodbye then.\"") return
                    end
                end
            elseif d >= 2 and d <= 9 then
                amount = DEPOSITS[d - 1]
            elseif d == 10 then
                amount = p.Zeny
            elseif d == 12 then
                return leave()
            end
            if amount then                                         -- L_Dep_Continue
                if p.Zeny < amount or amount < 1 then              -- L_NoMoney
                    p:mes(npcname())
                    p:mes("\"Oh dear, it seems that you don't have enough money.\"")
                else
                    p.Zeny = p.Zeny - amount
                    p.acc.BankAccount = p.acc.BankAccount + amount
                    -- L_Balance ... (continues as in the original)
                end
            end
        elseif c == 5 then                                         -- L_Change (dynamic menu)
            local items = { "Keep the current settings",
                            "Close NPC dialog after selecting storage option",
                            "Close NPC dialog after checking your balance" }
            if bit32.band(p.acc.BankOptions, OPT_STORAGE_CLOSE) ~= 0 then
                items[2] = "Return to main menu after leaving storage" end
            if bit32.band(p.acc.BankOptions, OPT_BANK_CLOSE) ~= 0 then
                items[3] = "Return to main menu after leaving bank" end
            local k = p:menu(items)
            if k == 2 then p.acc.BankOptions = bit32.bxor(p.acc.BankOptions, OPT_STORAGE_CLOSE)
            elseif k == 3 then p.acc.BankOptions = bit32.bxor(p.acc.BankOptions, OPT_BANK_CLOSE) end
        elseif c == 6 then                                         -- L_Nev
            p:mes(npcname())
            p:mes("\"Goodbye then.\"")
            return
        end
        -- every other branch loops back to L_Start
    end
end
```
```lua
-- npc/001-2/bank.lua
npc.script{ name = "Hydusun", map = "001-2", x = 63, y = 17, dir = 0, sprite = 149,
    on_click = function(self, p)
        p.tmpstr.npcname = "Hydusun"       -- old: set @npcname$, "Hydusun"
        Banker(self, p)
        p:close()
    end }
```

### Example 3: OnInit + OnTimer NPC (`029-3/parua.txt` excerpt)

```lua
local function cleanup(self)                       -- L_CleanUp (shared by OnInit and OnTimer5000)
    self:talk("Game Over")
    worldtmpstr.candor_npctalk = "The dungeon is now ready for its next victims."
    npc.event("#CandorAnnouncer::OnCommandTalk")
    map.areatimer("029-3", 20, 20, 70, 60, 10, "Parua::OnReward")
    worldtmp.FIGHT_CAVE_STATUS = 0
    mob.killmonster("029-3", "Parua::OnPetDeath")
end

npc.script{
    name = "Parua", map = "029-3", x = 63, y = 73, dir = 0, sprite = 155,
    on_init = function(self)
        if debug >= 2 then return end
        self:initnpctimer()
        self:stopnpctimer()
        cleanup(self)
    end,
    on_timer = {
        [5000] = function(self)
            self:setnpctimer(0)                    -- re-arm idiom, unchanged
            if worldtmp.FIGHT_CAVE_STATUS == 1 then
                -- L_CaveLogic ...
            elseif worldtmp.FIGHT_CAVE_STATUS >= 2 then
                -- L_GlobalAnnounce ...
            else                                   -- L_Return_1
                worldtmp.FIGHT_CAVE_PLAYER_COUNT = 0
                map.areatimer("029-3", 20, 20, 70, 60, 10, "Parua::OnTick")
            end
        end,
    },
    events = {
        OnPetDeath = function(self, p) end,        -- tag for mobcount/killmonster
        OnReward = function(self, p) --[[ ... ]] end,
        OnTick = function(self, p) --[[ ... ]] end,
    },
    on_click = function(self, p) --[[ the dialogue ]] end,
}
```

### Example 4: touch-area NPC and a real OnTouch (`001-1/sewer_east.txt`, `001-1/dock.txt`)

```lua
-- xs = 0, ys = 0 is a 1x1 touch area; stepping on it runs on_click (no on_touch), as before
npc.script{ name = "#tulimsharsewer2", map = "001-1", x = 117, y = 110, dir = 0,
            sprite = 45, xs = 0, ys = 0,
    on_click = function(self, p)
        p:mes("Descend into the sewers?")
        p:next()
        if p:menu("Yes.", "Nevermind.") == 1 then
            p:warp("021-3", 143, 129)
        end
        p:close()
    end }

npc.script{ name = "Tulimshar Koga", map = "001-1", x = 75, y = 70, dir = 0,
            sprite = 395, xs = 9, ys = 4,
    on_click = function(self, p)
        p.tmp.npc_distance = 10                     -- first-pass mechanical callfunc protocol
        PCtoNPCRange(self, p)
        if p.tmp.npc_check ~= 0 then return end
        BoardFerry(self, p)
    end,
    on_touch = function(self, p)
        p:addtimer(npc.get("#FerryConfig").vars.warp_delay, self.name .. "::OnBoard")
    end,
    events = {
        OnBoard = function(self, p) BoardFerry(self, p) end,
    } }
```

### Example 5: callfunc function (first pass, mechanical) and a call() function with getarg

```lua
-- npc/functions/default_npc_checks.lua
-- Mechanical first pass: the @-variable calling convention is kept, so all 70 callers
-- translate as: p.tmp.npc_distance = N; PCtoNPCRange(self, p); if p.tmp.npc_check ~= 0 then return end
function PCtoNPCRange(self, p)
    p.tmp.npc_check = 0
    p.tmpstr.Nmap = self.map
    if p.tmp.npc_distance == 0 then p.tmp.npc_distance = 4 end
    if p.tmp.npc_distance == (1 - 2) then p.tmp.npc_distance = p.ATTACKRANGE end
    local loc = array(p.tmp, "npc_loc")
    cleararray(loc, 0, 0, 3)
    setarray(loc, 0, self.x, self.y, p.tmp.npc_distance)
    p.tmp.Nx1 = loc[0] - loc[2]
    p.tmp.Ny1 = loc[1] - loc[2]
    p.tmp.Nx2 = loc[0] + loc[2]
    p.tmp.Ny2 = loc[1] + loc[2]
    if not p:isin(p.tmpstr.Nmap, p.tmp.Nx1, p.tmp.Ny1, p.tmp.Nx2, p.tmp.Ny2) then
        p.tmp.npc_check = 1
        if p.tmp.distance_handler == 0 then
            p.tmpstr.dnpc_name = self.basename
            if p.tmpstr.dnpc_name ~= "" then
                p:message(p.tmpstr.dnpc_name .. " : ##BPlease move closer.")
            else
                p:message("Server : ##BYou need to move closer to interact with this npc.")
            end
        end
    end
    -- L_Return
    p.tmpstr.dnpc_name = ""
    p.tmp.distance_handler = 0
    p.tmp.npc_distance = 0
    cleararray(loc, 0, 0, 3)
end
-- Optional second pass (all callers at once): PCtoNPCRange(self, p, distance) -> bool

-- npc/functions/spawns_on_mobkill.lua: a call() function; getarg(n, d) -> parameter or default
function spawn_mobs_around(self, p, mapname, mobX, mobY, mobID, mobQTY)
    mapname = mapname or ""
    mobX = mobX or -1
    mobY = mobY or -1
    mobID = mobID or -1
    mobQTY = mobQTY or -1
    if mapname == "" or mobX < 1 or mobY < 1 or mobID < 1002
       or not map.mapexists(mapname)      -- guard added: getmapmaxx raises on unknown maps
       or mobX > map.getmapmaxx(mapname) or mobY > map.getmapmaxy(mapname)
       or mobQTY < 1 then
        server.debugmes("spawn_mob_around: invalid args! Map=" .. mapname .. " x=" .. mobX
                        .. " y=" .. mobY .. " mobID=" .. mobID .. " mobQTY=" .. mobQTY)
        return
    end
    if mobX > 1 and mobY > 1 and mobX < map.getmapmaxx(mapname)
       and mobY < map.getmapmaxy(mapname) and mobQTY > 1 then
        mob.areamonster(mapname, mobX - 1, mobY - 1, mobX + 1, mobY + 1, "", mobID, mobQTY)
    else
        mob.monster(mapname, mobX, mobY, "", mobID, mobQTY)
    end
end
-- caller inside an OnMobKillEvent handler:
--   spawn_mobs_around(self, p, p.map, args.mobX, args.mobY, AngrySeaSlime, rand(8, 16))
```

### Example 6: monster spawn with a death event (`051-3/reinforcements.txt` "Door")

```lua
npc.script{ name = "Door", map = "051-3", x = 40, y = 25, dir = 0, sprite = 32767,
    events = {
        OnRnfrcmts = function(self)
            worldtmp.illia_level_2_progress = 3
            mob.areamonster("051-3", 29, 25, 48, 39, "", 1064, 18, "Door::OnB")
            mob.areamonster("051-3", 29, 25, 48, 39, "", 1065, 3, "Door::OnB")
            self:initnpctimer()
            map.mapannounce("051-3", "Bandit Lords : Do not let them escape!!", 0)
            map.areatimer("051-3", 25, 20, 80, 85, 10, "Door::OnDRnfrcmts")
        end,
        OnB = function(self, p) end,       -- tag; runs per kill with the killer attached
        OnDRnfrcmts = function(self, p)
            p:message("Oh no, reinforcements! We must kill them all!")
        end,
    },
    on_timer = {
        [2000] = function(self)
            self:setnpctimer(0)
            if worldtmp.illia_level_2_progress ~= 3 or worldtmp.illia_progress ~= 2 then return end
            if mob.mobcount("051-3", "Door::OnB") < 0 then    -- minus-one kept: < 0 means none left
                -- L_OpenDoor ...
            end
        end,
    },
    on_click = function(self, p) --[[ ... ]] end,
}
```
A generated `_mobs.txt` line becomes:
```lua
npc.monster{ map = "001-1", x = 32, y = 59, xs = 10, ys = 11, name = "GreenSlime",
             species = 1005, amount = 8, delay1 = 100000, delay2 = 30000 }
```

### Example 7: registercmd GM command (`commands/zeny.txt`)

```lua
-- npc/commands/zeny.lua
npc.script{ name = "@zeny",
    on_init = function(self)
        self.vars.max_zeny = 1000000000
        self.vars.max_int  = 2147483647
        server.registercmd(chr(ATCMD_SYMBOL) .. "zeny", self.name)      -- NPC name = the click body
        server.registercmd(chr(ATCMD_SYMBOL) .. "charzeny", self.name)
    end,
    on_click = function(self, p, args)      -- command handlers get the argument string
        local argv = argv_splitter(args)    -- ported helper: 0-based array of strings (old @argv$)
        local n = if_then_else(argv[1] ~= "", "char", "") .. "zeny"
        local function gm_error()
            p:message(n .. " : GM command is level "
                      .. if_then_else(argv[1] ~= "", CMD_CHARZENY, CMD_ZENY)
                      .. ", but you are level " .. p.GM)
        end
        if p.GM < CMD_ZENY and p.GM < G_SYSOP then return gm_error() end
        local target = p
        if argv[1] ~= "" then
            target = players.byname(argv[1])
            if not target then
                p:message(n .. " : Impossible to attach to the target player.")
                return
            end
            if p.GM < CMD_CHARZENY and p.GM < G_SYSOP then return gm_error() end
        end
        local function success()
            p:gmlog("@zeny " .. args)
            p:message(n .. " : The operation succeeded.")
        end
        if argv[0] == "--" then target.Zeny = 0 return success() end
        if argv[0] == "++" then target.Zeny = self.vars.max_zeny return success() end
        local delta = atoi(argv[0])
        local new_zeny = target.Zeny + delta
        if new_zeny < 0 then
            local bank = target.acc.BankAccount
            if bank + new_zeny < 0 then
                p:message(n .. " : Impossible to proceed! This would cause less than 0 zeny.")
                return
            end
            target.Zeny = 0
            target.acc.BankAccount = bank + new_zeny
        else
            target.Zeny = new_zeny
        end
        return success()
    end }
```
(`set Zeny, v, .@target_id` became `target.Zeny = v` on the handle; `isloggedin` became a nil
check on `players.byname`.)

### Example 8: item scripts (use, equip, and a dialog-opening use script)

```
501,  CactusDrink, ...,  {p:heal(15, 0, 1)},                                                {}
675,  GraduationCap, ..., {},                                                               {p:bonus(bInt, 1)}
5253, MagicRing, ...,     {},   {RequireStat(p, args.slotId, bInt, 80) MagicRingItem(p, args) p:bonus(bInt, 1)}
<anchor stone>,           {UseAnchorStone(p, "AnchorStone")},                               {}
```
```lua
-- npc/items/require_stat.lua
-- old: set @bStat, bInt; set @minbStatVal, 80; callfunc "RequireStat";  (reads @slotId)
-- bInt is the SP id constant; the old code compared the *player's* stat value, read via p:param.
function RequireStat(p, slotId, stat_sp, minval)
    if p:param(stat_sp) < minval then UnequipLater(p, slotId) end
end

function UnequipLater(p, slotId)
    if p.tmp.unequip_slot ~= 0 then return end
    p.tmp.unequip_slot = slotId + 1
    p:addtimer(0, "UnequipCB::OnUnequip")           -- equip scripts cannot unequip inline
end
npc.script{ name = "UnequipCB", events = { OnUnequip = function(self, p)
    p:unequipbyid(p.tmp.unequip_slot - 1)
    p.tmp.unequip_slot = 0
end } }

-- npc/items/anchor_stone.lua: a use script that opens a dialog directly (new capability;
-- the old addtimer-0 hop also still works)
function UseAnchorStone(p, stone)
    p:mesn("Anchor Stone")                          -- item dialogs must pass a name
    p:mes("\"You really want to bind the [@@" .. AnchorStone .. "|@@] to this place?\"")
    if p:menu{ {"No", false}, {"Yes", true} } then
        p.vars.AnchorStoneDest = map.getmaphash(p.map)
        p:delitem(AnchorStone, 1)
        p:getitem(AnchoredAnchorStone, 1)
    end
    p:close()
end
```

---------------------------------------------------------------------------------------------------

## 15. Builtin coverage

All **175** old builtins are covered by this API. 174 have a direct replacement (a method,
namespace function, property, constructor field, handler argument, or a Lua language
construct); the complete old -> new table is in the porting guide.

Deliberately dropped (1):

| builtin | reason | replacement |
|---|---|---|
| `freeloop` | the instruction budget is large (default 20M VM instructions, a conf key) and resets on every resume, so no content needs an opt-out; keeping it would sanction unbounded handlers | none needed; long-running work belongs in timers |

Builtins absorbed by the Lua language rather than an API entry (still counted as covered):
`goto`, `if`, `elif`, `else`, `set`, `get`, `end`, `return`, `void`, `call`, `callfunc`,
`callsub`, `getarg`, `getelementofarray`, `attachrid`, `detachrid`. Their translations are
specified in the porting guide's control-flow and calling-convention sections.
