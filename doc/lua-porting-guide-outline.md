# TMWA Lua porting guide: outline and translation tables

This is the binding outline of `doc/lua-porting-guide.md`, the document the AI porting
agents follow when translating the 85k lines of serverdata NPC content to Lua. The full
guide is written after the engine is implemented; the translation tables below are complete
now and are normative. The API semantics live in `lua-scripting.md`; this guide is the
old -> new direction.

Planned chapter structure of the full guide:

0. Workflow (section 15 below)
1. File layout and import order (section 1)
2. Variable scope mapping (section 3)
3. Builtin mapping, all 175 (section 2)
4. Event label mapping (section 4)
5. Control-flow restructuring (section 5)
6. Calling conventions: callfunc/call/callsub/getarg (section 6)
7. Arrays (section 7)
8. Strings (section 8)
9. Operators, precedence, short-circuit (section 9)
10. Numbers and integer semantics (section 10)
11. Constants and params (section 11)
12. Item scripts (section 12)
13. Idiom table, census idioms 1-33 (section 13)
14. Behaviour-change audit list (section 14)
15. Verification workflow (section 15)
16. Appendix: callfunc signature registry (`funcdefs.tsv`, seeded by the converter, filled
    in as porting proceeds)

---------------------------------------------------------------------------------------------------

## 1. File layout and import order

The old tree maps 1:1; `.txt` NPC files become `.lua` with the same basenames. `conf/` and
`db/` are unchanged (item_db `{...}` columns now carry Lua).

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
* Load order (conf order) is preserved; `functions/` loads first, as today. Function lookup
  happens at call time, so only load-time calls need ordering; top-level chunk code must not
  call functions from later files.
* `import(path)` exists for intra-content includes; every file loads once.
* File-local helpers are `local function`s; cross-file functions are globals.
* Old `function|script|Name` names with spaces are mangled to underscores
  (`Easter Debug` -> `Easter_Debug`); the converter emits the mapping table.

## 2. Builtin mapping: all 175

Legend: "(lang)" = handled by Lua syntax, see sections 5/6. Any player-context builtin that
took an optional target (char name / block id / char id) becomes a call on the target's
handle (`players.byname(n)`, `players.byid(id)`, `players.bycharid(c)`, `npc.get(n)`,
`being(id)`).

| # | old | -> Lua |
|---|---|---|
| 1 | `mes` | `p:mes(text)`; bare `mes;` -> `p:mes()` |
| 2 | `mesq` | `p:mesq(text)` |
| 3 | `mesn` | `p:mesn([name])` |
| 4 | `clear` | `p:clear()` |
| 5 | `goto` | (lang) restructure, section 5 |
| 6 | `callfunc` | `Name(self, p)` (global function); `@` inputs/outputs keep working through `p.tmp` (section 6) |
| 7 | `call` | `Name(self, p, a, b, ...)` with a return value; label form -> local function call |
| 8 | `callsub` | call of a local function made from the `S_` block |
| 9 | `getarg` | function parameters; `getarg(n, d)` -> `param or d` (0/"" are truthy in Lua, so only a missing arg takes the default, exactly as before) |
| 10 | `return` | `return [v]` |
| 11 | `void` | (lang) call as a statement |
| 12 | `next` | `p:next()` |
| 13 | `close` | `p:close()` (handler ends; nothing after it runs) |
| 14 | `close2` | `p:close2()` |
| 15 | `menu` | `local c = p:menu(...)` + `if c == 1 then ... elseif ...`; dynamic menus: the table form |
| 16 | `rand` | `rand(n)` / `rand(a, b)` |
| 17 | `isat` | `p:isat(map, x, y)` |
| 18 | `warp` | `p:warp(map, x, y)` |
| 19 | `areawarp` | `map.areawarp(map, x0, y0, x1, y1, to_map, to_x, to_y)` |
| 20 | `mapwarp` | `map.mapwarp(map, to_map, x, y)` |
| 21 | `heal` | `p:heal(hp, sp [, itemheal])` |
| 22 | `injure` | `server.injure(src, tgt, dmg)` or `being(src):injure(tgt, dmg)` |
| 23 | `input` | `local v = p:input()` / `local s = p:input_str()` |
| 24 | `requestitem` | `local ids = p:requestitem(n)`; names into `$` string arrays -> `p:requestitem(n, true)` |
| 25 | `requestlang` | `local s = p:requestlang()` |
| 26 | `if` | (lang) `if cond ~= 0 then ... end` (int conditions need the explicit comparison, section 9) |
| 27 | `elif` | (lang) `elseif` when adjacent to its `if` (always, in content) |
| 28 | `else` | (lang) `else` |
| 29 | `set` | assignment: `p.tmp.x = v`, `p.vars.X = v`, `p.Zeny = v`, `world.X = v`; target form: `players.byid(id).Zeny = v`, `npc.get(n).vars.x = v` |
| 30 | `get` | read on the target's handle: `players.byid(id).Hp`, `npc.get(n).vars.x`; SP-number reads: `p:param(sp)` |
| 31 | `setarray` | `setarray(array(scope, "name"), start, v1, ...)`; append form: `setarray(t, getarraysize(t), ...)` |
| 32 | `cleararray` | `cleararray(t, start, value, count)` |
| 33 | `getarraysize` | `getarraysize(t)` (empty array now 0, was 1; AUDIT) |
| 34 | `getelementofarray` | `t[i]` |
| 35 | `array_search` | `array_search(needle, t [, start])` |
| 36 | `setlook` | `p:setlook(type, val)` |
| 37 | `countitem` | `p:countitem(item)` |
| 38 | `checkweight` | `p:checkweight(item, n)` (boolean) |
| 39 | `getitem` | `p:getitem(item, n)`; target form on the handle |
| 40 | `makeitem` | `map.makeitem(item, n, map, x, y)` (`"this"` -> `p.map`) |
| 41 | `delitem` | `p:delitem(item, n)` |
| 42 | `getcharid` | `p.charid` (0), `p.partyid` (1), `p.guildid` (2), `p.id` (3); name form via `players.byname(n)` + nil check |
| 43 | `getnpcid` | `self.id` / `npc.get(n).id` (nil instead of -1 when missing) |
| 44 | `getversion` | `p.version` |
| 45 | `strcharinfo` | `p.name` (0), `p.partyname` (1), `""` (2) |
| 46 | `getequipid` | `p:getequipid(pos)` |
| 47 | `bonus` | `p:bonus(type, val)` |
| 48 | `bonus2` | `p:bonus2(type, t2, val)` |
| 49 | `skill` | `p:skill(id, lv [, flag])` |
| 50 | `setskill` | `p:setskill(id, lv)` |
| 51 | `getskilllv` | `p:getskilllv(id)` |
| 52 | `overrideattack` | `p:overrideattack(delay, range, icon, look, event [, charges])` / `p:overrideattack()` |
| 53 | `getgmlevel` | `p.gmlevel` (or `p.GM`) |
| 54 | `end` | `return` from the handler; at nesting depth: `stop()`; with an open dialog: `p:close()` |
| 55 | `getopt2` | `p.opt2` |
| 56 | `setopt2` | `p.opt2 = v` |
| 57 | `savepoint` | `p:savepoint(map, x, y)` |
| 58 | `gettimetick` | `server.gettimetick(t)` |
| 59 | `gettime` | `server.gettime(t)` |
| 60 | `openstorage` | `p:openstorage()` |
| 61 | `getexp` | `p:getexp(base, job)` |
| 62 | `mobinfo` | `mob.mobinfo(species, what)` |
| 63 | `mobinfo_droparrays` | `mob.getmobdrops(species)` (records) |
| 64 | `getmobdrops` | `mob.getmobdrops(species)` |
| 65 | `summon` | `mob.summon(map, x, y, owner, name, species, attitude, lifespan [, event])` |
| 66 | `monster` | `mob.monster(map, x, y, name, species, amount [, event])` (`"this"`/`x,y <= 0` conveniences gone: pass `p.map`, `p.x`, `p.y`) |
| 67 | `areamonster` | `mob.areamonster(map, x0, y0, x1, y1, name, species, amount [, event])` |
| 68 | `killmonster` | `mob.killmonster(map, event)` |
| 69 | `donpcevent` | `npc.event(event)` |
| 70 | `addtimer` | `p:addtimer(ms, event)`; id form on the handle |
| 71 | `addnpctimer` | `self:addnpctimer(ms, event)` / `npc.get(n):addnpctimer(...)` |
| 72 | `initnpctimer` | `self:initnpctimer()` (quirk preserved) |
| 73 | `startnpctimer` | `self:startnpctimer()` |
| 74 | `stopnpctimer` | `self:stopnpctimer()` |
| 75 | `getnpctimer` | `self:getnpctimer(type)` |
| 76 | `setnpctimer` | `self:setnpctimer(ms)` |
| 77 | `setnpcdirection` | `self:setdirection(dir, sit, save [, p])` |
| 78 | `npcaction` | `p:npcaction(cmd [, id, x, y])` |
| 79 | `camera` | `p:camera(...)` |
| 80 | `announce` | `server.announce(text, flag [, source])` / `p:announce` / `self:announce` |
| 81 | `mapannounce` | `map.mapannounce(map, text, flag)` |
| 82 | `getusers` | `server.getusers(flag)` |
| 83 | `getmapusers` | `map.getmapusers(map)` |
| 84 | `getareausers` | `map.getareausers(map, x0, y0, x1, y1 [, living])` |
| 85 | `getareadropitem` | `map.getareadropitem(...)` |
| 86 | `enablenpc` | `npc.enable(name)` / `self:enable()` |
| 87 | `disablenpc` | `npc.disable(name)` / `self:disable()` |
| 88 | `sc_start` | `p:sc_start(type, tick, val)` / `being(id):sc_start(...)` (tick heuristic preserved: do NOT convert units) |
| 89 | `sc_end` | `p:sc_end(type)` / `being(id):sc_end(type)` |
| 90 | `sc_check` | `p:sc_check(type)` (boolean) |
| 91 | `debugmes` | `server.debugmes(text)` |
| 92 | `wgm` | `server.wgm(text)` |
| 93 | `gmlog` | `p:gmlog(text)` |
| 94 | `resetstatus` | `p:resetstatus()` |
| 95 | `attachrid` | `p = players.byid(id)`; `if (attachrid(id) == 0)` -> `if not p then` |
| 96 | `detachrid` | nothing (or `p = nil`) |
| 97 | `isloggedin` | `server.isloggedin(id)` (or `players.byid(id) ~= nil`) |
| 98 | `setmapflag` | `map.setmapflag(map, flag)` |
| 99 | `removemapflag` | `map.removemapflag(map, flag)` |
| 100 | `getmapflag` | `map.getmapflag(map, flag)` |
| 101 | `getbattleconfig` | `server.getbattleconfig(key)` |
| 102 | `pvpon` | `map.pvpon(map)` |
| 103 | `pvpoff` | `map.pvpoff(map)` |
| 104 | `setpvpchannel` | `p.pvpchannel = n` |
| 105 | `getpvpflag` | `p.pvpchannel` (0) / `p.hidden` (1) |
| 106 | `emotion` | `self:emotion(type [, p])` (NPC emotes) / `p:emotion(type)` (the `"self"` form) |
| 107 | `mobcount` | `mob.mobcount(map, event)` (minus-one preserved: 0 mobs -> -1; keep `< 0` comparisons verbatim) |
| 108 | `marriage` | `p:marriage(name)` |
| 109 | `divorce` | `p:divorce()` |
| 110 | `getitemlink` | `item.getitemlink(item)` |
| 111 | `getpartnerid2` | `p.partnerid` |
| 112 | `explode` | `local t = explode(s, sep)` (0-based) / `explode_int` for int arrays |
| 113 | `getinventorylist` | `local inv = p:getinventorylist()`; `#inv` replaces `@inventorylist_count` |
| 114 | `getactivatedpoolskilllist` | `local l = p:getactivatedpoolskilllist()` |
| 115 | `getunactivatedpoolskilllist` | `p:getunactivatedpoolskilllist()` |
| 116 | `poolskill` | `p:poolskill(id)` |
| 117 | `unpoolskill` | `p:unpoolskill(id)` |
| 118 | `misceffect` | `self:misceffect(fx)` (oid default) / `p:misceffect(fx)` (charname form) / `being(id):misceffect(fx)` |
| 119 | `specialeffect` | `self:specialeffect(fx)` |
| 120 | `specialeffect2` | `p:specialeffect(fx)` |
| 121 | `nude` | `p:nude()` |
| 122 | `unequipbyid` | `p:unequipbyid(slot)` |
| 123 | `npcwarp` | `self:warp(x, y)` / `npc.get(name):warp(x, y)` |
| 124 | `npcareawarp` | `h:areawarp(x0, y0, x1, y1, avoid)` |
| 125 | `message` | `p:message(text)` / `players.byname(n):message(text)` |
| 126 | `npctalk` | `self:talk(text [, p])` / `npc.get(n):talk(text)` |
| 127 | `registercmd` | `server.registercmd(word, event)`; ported content always uses the event/NPC-name form, handler `(self, p, argstring)` |
| 128 | `title` | `p:title(text)` |
| 129 | `smsg` | `p:smsg([type,] text)` |
| 130 | `remotecmd` | `p:remotecmd(cmd)` |
| 131 | `sendcollision` | `p:sendcollision(map, mask, x1, y1 [, x2, y2])` |
| 132 | `music` | `p:music(name)` |
| 133 | `mapmask` | `p:mapmask(mask [, persist])` |
| 134 | `getmask` | `p.mask` (or `map.mask(self.map)` for the NPC-map fallback) |
| 135 | `getlook` | `p:getlook(type)` |
| 136 | `getsavepoint` | `local m, x, y = p:getsavepoint()` |
| 137 | `areatimer` | `map.areatimer(map, x0, y0, x1, y1, ms, event)` (drop the old leading `0`) |
| 138 | `foreach` | `map.foreach(type, map, x0, y0, x1, y1, event [, caller])`; handler reads `args.target_id` |
| 139 | `isin` | `p:isin(map, x0, y0, x1, y1)` |
| 140 | `iscollision` | `map.iscollision(map, x, y)` |
| 141 | `shop` | `p:shop(name)` (handler ends) |
| 142 | `isdead` | `p.dead` |
| 143 | `aggravate` | `being(mobid):aggravate(target)` |
| 144 | `issummon` | `being(id):issummon()` |
| 145 | `fakenpcname` | `npc.get(old):rename(new, sprite)`; same-name sprite change: `self.sprite = n` (AUDIT: semantics fixed, 29 uses) |
| 146 | `puppet` | `self:puppet(map, x, y, name, sprite [, xs, ys])` (returns handle or nil) |
| 147 | `destroy` | `self:destroy() return` / `npc.byid(id):destroy()` (does not end the handler by itself) |
| 148 | `getx` | `p.x` |
| 149 | `gety` | `p.y` |
| 150 | `getdir` | `p.dir` |
| 151 | `getnpcx` | `self.x` / `npc.get(n).x` |
| 152 | `getnpcy` | `self.y` |
| 153 | `strnpcinfo` | `self.name` (0), `self.basename` (1), `self.suffix` (2), `self.map` (3) |
| 154 | `getmap` | `p.map`; block-id form via `players.byid(id).map` |
| 155 | `getmapmaxx` | `map.getmapmaxx(map)` |
| 156 | `getmapmaxy` | `map.getmapmaxy(map)` |
| 157 | `getmaphash` | `map.getmaphash(map)` |
| 158 | `getmapnamefromhash` | `map.getmapnamefromhash(h)` |
| 159 | `mapexists` | `map.mapexists(map)` |
| 160 | `numberofmaps` | `map.numberofmaps()` |
| 161 | `getmapnamebyindex` | `map.getmapnamebyindex(i)` |
| 162 | `mapexit` | `server.mapexit()` |
| 163 | `freeloop` | dropped (instruction budget is large and per-resume); delete the statement |
| 164 | `if_then_else` | `if_then_else(c, a, b)` (kept as a global; both branches evaluated, same as old) |
| 165 | `max` | `max(a, b, ...)`; one-argument array form -> `arrmax(t)` |
| 166 | `min` | `min(a, b, ...)`; array form -> `arrmin(t)` |
| 167 | `average` | `average(...)` |
| 168 | `sqrt` | `sqrt(n)` (int) |
| 169 | `cbrt` | `cbrt(n)` |
| 170 | `pow` | `pow(a, b)` |
| 171 | `target` | `server.target(src, tgt, flags)` / `being(src):target(tgt, flags)` |
| 172 | `distance` | `server.distance(a, b)` / `being(a):distance(b)` |
| 173 | `chr` | `chr(n)` |
| 174 | `ord` | `ord(s)` |
| 175 | `l` | `l(s, ...)` |

Top-level kinds: `map,x,y,d|script|Name|sprite[,xs,ys]{}` -> `npc.script{...}`;
`-|script|Name|32767{}` -> `npc.script{...}` without map; `function|script|Name{}` ->
`function Name(self, p, ...) end`; `warp` -> `npc.warp{}`; `shop` -> `npc.shop{}`;
`monster` -> `npc.monster{}`; `mapflag` -> `npc.mapflag{}`; conf `import:` unchanged;
`npc:` lines now name `.lua` files.

## 3. Variable scope mapping

| old | Lua | default | notes |
|---|---|---|---|
| `.@x`, `.@x$` | `local x = 0` / `local x = ""` | porter initialises | locals now survive `next`/`menu` |
| `@x` | `p.tmp.x` | 0 | session lifetime, shared across scripts (calling conventions keep working) |
| `@x$` | `p.tmpstr.x` | `""` | |
| `@arr[i]` | `array(p.tmp, "arr")[i]` | 0 | 0-based, indices unchanged; `@arr$[i]` -> `arraystr(p.tmpstr, "arr")[i]` |
| `.x` / `.x$` | `self.vars.x` / `self.varstr.x` | 0 / `""` | cross-NPC: `npc.get("Name").vars.x` |
| `.arr[i]` | `array(self.vars, "arr")[i]` | 0 | |
| `$@x` / `$@x$` | `worldtmp.x` / `worldtmpstr.x` | 0 / `""` | shared across files by name, as before |
| `$x` / `$x$` | `world.x` / `world.str.x` | 0 / `""` | persisted (mapreg.txt unchanged) |
| `$x[i]` / `$x$[i]` | `world.array("x")[i]` / `world.array("x$")[i]` | 0 / `""` | or `world.get/set(name, i)` |
| `x` (plain, incl. `QL_*`) | `p.vars.x` | 0 | int only, <= 31 chars, 96-entry cap |
| `#x` | `p.acc.x` | 0 | 16-entry cap |
| `##x` | `p.acc2.x` | 0 | digit-leading names via `p.acc2["00_INFO"]` |
| params (`Zeny`, `Hp`, ...) | `p.Zeny` etc. | | exact params.txt case; `being(id).Hp` for other blocks |
| `@menu` | the `p:menu` return value | | assign it to a local at the menu |
| `@args$` | the `argstring` parameter | | |
| `@mobID/@mobX/@mobY` | `args.mobID/args.mobX/args.mobY` | | OnMobKillEvent handler |
| `@victimrid` | `args.victimrid` | | OnPCKillEvent handler |
| `@target_id` | `args.target_id` | | foreach / overrideattack handlers |
| `@slotId/@itemId` | `args.slotId/args.itemId` | | equip scripts |
| `@inventorylist_*`, `@skilllist_*`, `$@MobDrop_*` | return values of the methods | | |

Lua keywords used as names (`end`, `and`, ...) and digit-leading names use bracket syntax.
When one script uses both `@x` and `@x$`, they land in different tables; nothing collides.

## 4. Event label -> handler mapping

| old label / mechanism | Lua handler | signature |
|---|---|---|
| script body (position 0) | `on_click` | `(self, p [, argstring])` |
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
| mob death event string (`monster ... "N::OnX"`) | `events.OnX` on N | `(self, p)` |
| `~Tag` phony mob tags | unchanged strings | n/a |
| `addtimer` / `areatimer` target | `events.OnX` or a function | `(self, p)` |
| `addnpctimer` target | `events.OnX` or a function | `(self)` |
| `foreach` target | `events.OnX` or a function | `(self, caller, args)`, `args.target_id` |
| `overrideattack` target | `events.OnX` or a function | `(self, p, args)`, `args.target_id` |
| `registercmd` target | NPC name (click body) or `"N::OnX"` | `(self, p, argstring)` |
| `goto` into a label (`goto OnNoRid`) | function call | restructure per section 5 |

Old event strings (`"Npc::OnX"`, `"Npc::"`, `"::OnX"`) keep working everywhere an event is
accepted, and every such place also accepts a Lua function directly.

Dialog capability: click body, OnTouch, targeted events with a player (timers, mob death,
queued events, commands, `npc.event(ev, p)`), and item use scripts may open dialogs.
OnInit, OnTimer, OnClock/Minute/Hour/Day, OnPC*Event, OnMobKillEvent, foreach and
overrideattack handlers, and equip scripts are synchronous: dialog primitives raise there.
The `p:addtimer(0, "Npc::OnX")` hop is the sanctioned bridge (existing serverdata idiom).

## 5. Control-flow restructuring

The five goto patterns (they cover all 7636 gotos in the census):

1. **Epilogue labels** (`goto L_Close`, `L_End`, `L_NoMoney`, 1387 gotos): the label block
   runs cleanup and `close`. Make it a local function ending in `p:close()`;
   `goto L_Close` -> `return close_()`. When the epilogue is just `close;`,
   `goto L_Close` -> `p:close()` directly.
2. **Menu dispatch**: `menu "a",L_A,"b",L_B;` -> `local c = p:menu("a", "b")` plus
   `if c == 1 then <L_A block> elseif c == 2 then <L_B block> end`. Shared tails become
   local functions; a label targeted by several menus becomes one local function.
3. **Loops**: any backward goto = `while true do ... end` around the region from the label
   to the goto; a forward goto inside becomes `break` (target right after the loop) or
   `return f()` for far targets. Counted loops (`set @c, @c+1; goto L_loop`) become
   `for`/`while` with locals.
4. **Fall-through chains** (`L_Dep_5k: ... goto L_Dep_Continue;` ladders): data-drive them
   (a table of amounts) or make the join point a local function (`dep_continue(amount)`).
5. **if/elif/goto ladders**: adjacent `if (c1) goto A; if (c2) goto B; goto C;` become
   `if c1 then A() elseif c2 then B() else C() end` once the targets are functions or
   inline blocks.

Related rules:
* `callsub S_X` -> a local function (closure over the handler's locals). A goto from inside
  an `S_` block to an outer `L_` label (55 cases): the closure returns a tag the caller
  switches on.
* Conditions: old int conditions get an explicit comparison (`if cond ~= 0 then`); `!x` ->
  `x == 0`; `&&`/`||` -> `and`/`or` (short-circuit differences audited by the census: safe).
* `end;` -> `return` (top level of the handler) or `stop()` (at nesting depth);
  `if (...) end;` -> `if ... then return end`.
* A trailing `close;` maps to `p:close()`; a handler falling off the end with an open
  dialog gets an automatic close, but keep `p:close()` for clarity.
* `destroy;` -> `self:destroy() return`.
* Free-standing `elif`/`else`: merge into one `if/elseif/else` chain (always adjacent in
  content).

## 6. Calling conventions

* **`call("f", a, b)` functions** (explicit arguments): plain parameters after `(self, p)`;
  `getarg(n, d)` -> `param or d`; `return v` -> `return v`; `void call(...)` -> a call
  statement.
* **`callfunc "F"` functions** (implicit `@`-variable protocol): TWO PASSES.
  * FIRST PASS (mechanical, default): keep the `p.tmp` protocol exactly. Callers set
    `p.tmp.x = v` before the call; the function reads and writes `p.tmp.*`. This is safe
    because callers live in other files; no cross-file reasoning is needed. See the
    PCtoNPCRange example in the API reference.
  * SECOND PASS (optional, per function, only with ALL callers in the same change): convert
    the documented Input:/Return: conventions to parameters and return values. The
    `funcdefs.tsv` appendix lists, per callfunc function, its `@` inputs and outputs
    (seeded by the converter) and, once chosen, the final Lua signature.
* **Engine-supplied `@` values** (`@menu`, `@mobID`, `@victimrid`, `@target_id`, `@args$`,
  `@slotId/@itemId`, `@inventorylist_*`, `@skilllist_*`): handler parameters, `args`
  fields, or method return values; never `p.tmp` (section 3 table).
* **Cross-handler state on the same player** (one handler stores, another reads later, e.g.
  `@StoneName$` set by an item script and read by the timer hop target): stays in
  `p.tmp`/`p.tmpstr`. The converter greps for `@` names written in one handler and read in
  another and marks them.
* `attachrid(id)` loops -> `local target = players.byid(id); if not target then ... end`;
  all subsequent operations on `target`.

## 7. Arrays

* Ported arrays stay **0-based** with indices unchanged: `@arr[i]` ->
  `array(p.tmp, "arr")[i]`; loop bounds via `getarraysize`; index arithmetic ports
  literally. Only hand-cleanup passes may shift a self-contained loop to 1-based Lua
  sequences; never in the mechanical pass.
* `setarray @a[s], v1, v2` -> `setarray(a, s, v1, v2)`; `cleararray @a[s], v, n` ->
  `cleararray(a, s, v, n)`; `getelementofarray(a, i)` -> `a[i]`;
  `array_search(v, @a)` -> `array_search(v, a)`.
* `getarraysize` of an empty array returns 0 (was 1). Census: only used as loop bounds, no
  site depends on 1; the converter still AUDIT-flags each use.
* `explode` returns a 0-based table; `explode_int` for int arrays.
* `$` persisted arrays: `world.array("name")` proxy, indices 0..255 (part of the save
  format); the generic helpers accept the proxy.
* New API return values (`p:getinventorylist`, `p:requestitem`, `mob.getmobdrops`,
  `players.all`) are 1-based Lua sequences; use `#t` and `ipairs`.
* The 256-element cap applies only to `world` arrays (storage format); Lua tables are
  unlimited.

## 8. Strings

* Concatenation `+` -> `..`. Int-plus-int stays `+`. When one operand can be a
  string-typed variable holding digits, use `tostr()`; the census found no such site, but
  the lint flags mixed `==` comparisons between int and string expressions (constant-false
  in the old engine) for manual review.
* String comparisons: `==`/`~=` work on strings directly.
* `chr(3)` NUL-sentinel tricks keep working (`chr`/`ord` are provided).
* Escapes: old `\"` inside script strings is the same in Lua; `%%` client markup and `##`
  color codes pass through unchanged.
* Case-sensitivity of names and labels is unchanged.

## 9. Operators

* **Precedence warning**: the old engine's operator precedence is NOT C's. In particular
  `&` binds tighter than `==` in old scripts (`$SANGUINE & $@SV_BMDBit != 0` means
  `($SANGUINE & $@SV_BMDBit) != 0`), and shift/or chains group left. Do not translate
  token-by-token: rewrite by intended meaning. The three precedence-dependent census
  expressions and their canonical translations:
  * `$SANGUINE & $@SV_BMDBit != 0` -> `bit32.band(world.SANGUINE, worldtmp.SV_BMDBit) ~= 0`
  * `A & M >> S` -> `bit32.rshift(bit32.band(A, M), S)`
  * `F | K ^ a ^ b` -> `bit32.bxor(bit32.bxor(bit32.bor(F, K), a), b)`
* **Bitwise**: `& | ^ ~ << >>` -> `bit32.band/bor/bxor/bnot/lshift/rshift` (never Lua 5.4
  native operators: forbidden subset). Results are signed int32, so comparisons against old
  values hold.
* **Short-circuit warning**: old `&&`/`||` evaluated both sides; Lua `and`/`or`
  short-circuit. The census found only pure expressions on right-hand sides, so `and`/`or`
  is the standard translation; the lint flags side-effecting right operands (function calls
  other than the known-pure list) for review.
* **Ternary**: `if_then_else(c, a, b)` stays (both branches evaluated, like old). Only
  rewrite to `c and a or b` in hand cleanup when the middle operand can never be
  false/nil.
* **Division/modulo**: `/` -> `idiv(a, b)`, `%` -> `imod(a, b)` (C truncation and sign;
  Lua's `/` produces floats and `%`/`//` are floor-based: never use them in ported code).
* Comparison of function results that are now booleans (`isdead()`, `sc_check`, ...):
  `if (isdead())` -> `if p.dead then`; `if (sc_check(x) == 0)` -> `if not p:sc_check(x)
  then`.

## 10. Numbers

* All script integers are C int32. Fractional values raise at the API boundary.
* Intentional overflow (the `CASTS` counter idiom): write `p.CASTS = int32(p.CASTS + 1)`;
  writes to params/storage wrap to int32 anyway, `int32()` documents the intent.
* Hex literals port unchanged; the content has no octal.
* `sqrt/cbrt/pow` return truncated ints, as before. `math.*` returns floats: do not mix
  into ported arithmetic.

## 11. Constants and params

* Every `const_db` name (const.txt, const-aegis item and mob names, quest, magic, mapflags,
  debugflag, permissions) is a read-only global with the same spelling: port uses verbatim.
* Params are handle properties with the exact params.txt spelling: bareword `Zeny` ->
  `p.Zeny`. Reading the bareword as a global raises with a hint.
* The `b*` SP constants (`bStr`, `bInt`, ...) are integer globals for `p:bonus`/`p:param`.
* `debug` gates port verbatim: `if debug >= 2 then return end`.

## 12. Item scripts

* `item_db_*.txt` columns unchanged; the `{...}` columns hold Lua statements with `p` and
  `args` in scope (`args.itemId`; equip scripts also `args.slotId`).
* One physical line; no comments or long strings inside item columns; `}` inside quoted
  strings is fine (the scanner understands quotes).
* Use scripts may open dialogs directly (new); the old `addtimer 0, "Npc::OnUse"` hop also
  keeps working and remains the right pattern when the intent is "after this handler".
* Equip scripts: `bonus`-family calls, reads, and timer scheduling only; dialog primitives
  raise. `set @slotId`-based helper protocols become explicit arguments
  (`RequireStat(p, args.slotId, bInt, 80)`).
* Old `set @bStat, Int` stored the SP id; the helper reads the live stat via
  `p:param(stat_sp)`.

## 13. Idiom translation table (census idioms 1-33)

| # | idiom | translation |
|---|---|---|
| 1 | rand-dispatch if-chains (`if(@TEMP == 1) goto L_1; ...`) | a table of branches indexed by the random value, or `if/elseif`; API reference Example 1 |
| 2 | label fall-through | does not occur (every label block is closed); nothing to do |
| 3 | goto-loops (backward goto) | `while true do ... break ... end`; counted loops -> `for`/`while`; S_-block escapes -> closure returns a tag |
| 4 | `getarg(n, default)` | parameter with `x = x or default` (only a missing arg takes the default, exactly as before) |
| 5 | string concatenation with `+` | `..`; `tostr()` only when a value can be a digit-string; mixed int/string `==` flagged by lint |
| 6 | `set` on params | `p.Zeny = ...`; read-only params are never written by content (engine raises if the porter errs) |
| 7 | `close2; openstorage;` | `p:close2() p:openstorage()` verbatim |
| 8 | `if(...) end;` guards | `if ... then return end` |
| 9 | value-returning `call()` functions | Lua functions with `return`; `void call(...)` -> call statement |
| 10 | `input @x$` | `local s = p:input_str()` |
| 11 | 40-slot `@choice_n$` dynamic menus | build a Lua sequence, use the `p:menu(tbl)` pair form; the old `@menu - 1` indexing becomes the returned `value`; delete the padding arrays |
| 12 | nested callsub | nested local functions (closures) |
| 13 | NPC timers (`initnpctimer/stopnpctimer` in OnInit, `setnpctimer 0` re-arm) | verbatim methods; `on_timer = {[N] = fn}`; the initnpctimer quirk is preserved so no site needs audit |
| 14 | areatimer / foreach | `map.areatimer(map, ..., ms, event)` (leading 0 dropped); `map.foreach(type, map, ..., event, caller)`; handler reads `args.target_id` |
| 15 | puppets / fakenpcname / destroy | `self:puppet(...)` handle; `npc.get(x):rename(y, sprite)` (AUDIT: real rename now); sprite-only change `self.sprite = n`; `destroy;` -> `self:destroy() return` |
| 16 | sc_start | `p:sc_start(...)` / `being(id):sc_start(...)`; the seconds heuristic is preserved: keep the numbers, never convert units |
| 17 | getmapusers polling | `map.getmapusers(m)` |
| 18 | bit ops on quest flags | `bit32.*` with the precedence rewrite rules of section 9 |
| 19 | if_then_else | keep `if_then_else(c, a, b)`; `and/or` only in hand cleanup with proven-safe operands |
| 20 | int math | `idiv`/`imod`; `int32()` where overflow is intended |
| 21 | string compares, `chr(3)` sentinels | `==`/`~=`; `chr(3)` still available |
| 22 | free-standing elif/else | merge into one `if/elseif/else` chain (always adjacent in content) |
| 23 | cross-NPC state (`get(.x, "N")`, `set .x, v, "N"`, setarray on another NPC) | `npc.get("N").vars.x` read/write; append form: `local t = array(npc.get("N").vars, "arr"); setarray(t, getarraysize(t), ...)` |
| 24 | GM commands (registercmd, `@args$`, argv_splitter, CMD_* levels) | API reference Example 7; `argv_splitter(args)` ported to return a 0-based table |
| 25 | spell system (floating NPC, invocation, overrideattack, magic_register) | verbatim structure: `on_init` sets `self.varstr.invocation` and registers the invocation word; the attack handler reads `args.target_id`; `p:overrideattack(..., self.name .. "::OnAttack", n)` |
| 26 | attachrid/detachrid | `players.byid/byname` handles with nil checks |
| 27 | `@inventorylist_*` | `local inv = p:getinventorylist()`; `#inv`; fields `.id/.amount/.equip/.index` |
| 28 | items by name/const/id | unchanged: strings, const-aegis globals, and numbers are all accepted where an item is expected |
| 29 | quest-log views (`QL_*`) | `p.vars.QL_X` verbatim |
| 30 | `debug` constant gate | `if debug >= 2 then return end` |
| 31 | `##` vault (`##00_INFO`) | `p.acc2["00_INFO"]` |
| 32 | `void call(...)` | plain call statement |
| 33 | item-use dialog hop (`addtimer 0, "Npc::OnUse"` with `@StoneName$` args) | either keep the hop verbatim (`p.tmpstr.StoneName = ...; p:addtimer(0, "AS_Core::OnUse")`) or open the dialog directly in the use script (now allowed); keep the hop in the mechanical pass, simplify in cleanup |

## 14. Behaviour-change audit list

Everything the mechanical port must watch for; the converter emits `-- AUDIT:` comments at
each detected site. This list is also the release-notes delta.

| change | sites | porter action |
|---|---|---|
| `getarraysize` empty -> 0 (was 1) | all uses flagged | verify the use is a loop bound (census: all are) |
| menu cancel/out-of-range no longer falls through; hidden entries unselectable | dynamic menus | none (safe); use the table form for dynamic menus |
| 0x0146 during other prompts abandons the dialog | none in content | none |
| `fakenpcname` really renames | 29 uses | check follow-up references to the old name (doomsday files) |
| duplicate NPC names fatal | none in serverdata | none |
| strict argument checking | everywhere | fix at `--check-scripts` time |
| unknown map raises in `getmapmaxx/y`, `iscollision`, `getmaphash` | few | add `map.mapexists` guards where the old code relied on silent abort |
| `mob.monster` `"this"`/`x,y <= 0` conveniences gone | flagged | pass `p.map`, `p.x`, `p.y` |
| booleans replace 0/1 conditions | flagged builtins | use `if x then` / `if not x then` |
| `foreach` without caller runs with `p == nil` | rare | none (old ended silently) |
| menu arguments evaluated once (not re-run on resume) | none (pure expressions only) | none |
| commands mid-dialog abandon the dialog first | spells cast mid-dialog | play-test pass |
| NOT changed (do not "fix"): `mobcount` minus-one, `initnpctimer` quirk, `sc_start` seconds heuristic, `input` negative termination, `"*N"` shop prices | | keep old numbers and comparisons verbatim |

## 15. Verification workflow (the porting agents' loop)

1. Pick one converted file with PORTME stubs; read the embedded original source.
2. Translate stub by stub using sections 2-13; delete the original-source comment block
   when the file is done.
3. `tools/lua-port/lint.py <file>`: syntax, forbidden subset, unknown globals.
4. `tmwa-map --check-scripts` from the `world/map` directory: loads every file, runs
   constructors and `on_init`, exit 0 required. (`--check-scripts=load` for a fast pass
   while iterating.)
5. For dialog-bearing NPCs: the e2e harness scenario or a manual client session
   (mes/next/menu/input/close paths).
6. Grep for remaining `-- AUDIT:` comments in the file; resolve or justify each in the
   commit message.
7. One file per commit, on the `lua-scripting` serverdata branch; the commit message names
   the old file and any AUDIT decisions.
