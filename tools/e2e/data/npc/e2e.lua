-- e2e.lua - fixture content for the tmwa end-to-end harness (tools/e2e).
-- One small map ("test", 60x60) and a handful of NPCs, each exercising one
-- engine feature. The runner (run-e2e.py) drives these by speaking the
-- client protocol and asserts on the packets they emit. Marker strings
-- ("E2E_...") are what the runner greps for; keep them in sync.

-- 1. Dialog NPC: mes/next/menu/input/input_str/close
npc.script{
    name = "Greeter", map = "test", x = 32, y = 30, dir = 0, sprite = 102,
    on_click = function(self, p)
        p:mes("[Greeter]", "E2E hello")
        p:next()
        local c = p:menu("alpha", "beta", "gamma")
        p:mes("E2E choice=" .. c)
        p:next()
        local n = p:input()
        p:mes("E2E number=" .. n)
        p:next()
        local s = p:input_str()
        p:mes("E2E text=" .. s)
        p:close()
    end,
}

-- 2. Setup NPC: grants zeny and the fixture items (drives stat update and
--    inventory add packets)
npc.script{
    name = "Setup", map = "test", x = 28, y = 30, dir = 0, sprite = 103,
    on_click = function(self, p)
        p.Zeny = 5000
        p:getitem("E2ePotion", 3)
        p:getitem("E2eScroll", 2)
        p:getitem("E2eRing", 1)
        p:mes("E2E setup done")
        p:close()
    end,
}

-- 3. close2 + openstorage. The first open triggers the async storage fetch
--    from the char server and cannot succeed yet; the runner retries.
npc.script{
    name = "Banker", map = "test", x = 30, y = 28, dir = 0, sprite = 104,
    on_click = function(self, p)
        p:mes("E2E bank")
        p:close2()
        if p:openstorage() == false then
            p:message("E2E_STORAGE_RETRY")
            return
        end
        p:message("E2E_STORAGE_CLOSED")
    end,
}

-- 4. on_init + NPC timer. on_init proves itself through the announced
--    boot marker; the timer re-arms itself and announces a bounded number
--    of ticks (0x009a to every client on the server). A puppet shares the
--    on_timer handler but runs its own independent counter (boot=9 ticks).
npc.script{
    name = "Ticker", map = "test", x = 26, y = 30, dir = 0, sprite = 105,
    on_init = function(self)
        self.vars.boot = 7
        self.vars.ticks = 0
        self:initnpctimer()
        local pup = self:puppet("test", 26, 31, "TickerPup", 105)
        pup.vars.boot = 9
        pup.vars.ticks = 0
        pup:initnpctimer()
    end,
    on_timer = {
        [1500] = function(self)
            self:setnpctimer(0)
            if self.vars.ticks < 400 then
                self.vars.ticks = self.vars.ticks + 1
                map.mapannounce("test",
                    "E2E_TICK boot=" .. self.vars.boot
                    .. " n=" .. self.vars.ticks, 0)
            end
        end,
    },
}

-- 5. A warp: stepping on (33,33) moves the player to (20,20) on the same
--    map (0x0091 change map notify).
npc.warp{ map = "test", x = 33, y = 33, xs = 0, ys = 0,
          to_map = "test", to_x = 20, to_y = 20 }

-- 6. A shop.
npc.shop{
    name = "Trader", map = "test", x = 30, y = 32, dir = 0, sprite = 106,
    items = { {"E2ePotion", 50} },
}

-- 7. Commands (server.registercmd) and the mob spawn/death event.
npc.script{
    name = "MobMaster",
    on_init = function(self)
        server.registercmd("@e2espawn", self.name)
        server.registercmd("@e2echo", "MobMaster::OnEcho")
    end,
    -- click body doubles as the @e2espawn handler
    on_click = function(self, p, args)
        mob.monster("test", 40, 40, "--en--", 1002, 1, function(mself, killer)
            killer:getitem("E2ePotion", 1)
            killer:message("E2E_MOB_DEAD")
        end)
        -- regression: args passed to npc.event from inside a dialog
        -- coroutine must reach the handler (cross-thread push)
        npc.event("MobMaster::OnArgs", nil, {n = 5})
        p:message("E2E_MOB_SPAWNED")
        map.foreach(2, "test", 38, 38, 42, 42, function(fself, caller, fargs)
            caller:injure(being(fargs.target_id), 100000)
        end, p)
    end,
    events = {
        OnEcho = function(self, p, args)
            p:message("E2E_ECHO:" .. args)
        end,
        OnArgs = function(self, p, args)
            map.mapannounce("test", "E2E_ARGS n=" .. args.n, 0)
        end,
    },
}

-- 8. Persistent character variable across relog.
npc.script{
    name = "Recorder", map = "test", x = 34, y = 30, dir = 0, sprite = 107,
    on_click = function(self, p)
        if p.vars.E2E_SAVED ~= 0 then
            p:mes("E2E saved=" .. p.vars.E2E_SAVED)
            p:close()
        end
        local n = p:input()
        p.vars.E2E_SAVED = n
        p:mes("E2E stored")
        p:close()
    end,
}

-- 9. Global label hook: OnPCLoginEvent (floating NPC, broadcast label).
npc.script{
    name = "LoginHook",
    events = {
        OnPCLoginEvent = function(self, p)
            p.vars.E2E_LOGINS = p.vars.E2E_LOGINS + 1
            p:message("E2E_LOGIN #" .. p.vars.E2E_LOGINS)
        end,
    },
}

-- 10. Item-use dialog body (called from E2eScroll's use script column).
function UseE2eScroll(p)
    p:mesn("E2e Scroll")
    p:mes("E2E scroll speaks")
    p:next()
    local c = p:menu("Red pill", "Blue pill")
    p:mes("E2E pill=" .. c)
    p:close()
end
