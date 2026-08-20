#!/usr/bin/env python3
# run-e2e.py - end-to-end test runner for the tmwa Lua engine.
#
# Boots a throwaway world (tmwa-login/char/map from an existing build) with
# generated conf and the fixture content in data/, then drives a fake client
# through the scenarios and asserts on the reply packets.
#
# Usage:
#   tools/e2e/run-e2e.py [--build-dir DIR] [--base-port N] [--world-dir DIR]
#                        [--keep] [--verbose] [--only NAME]
#
# Exit status: 0 if every scenario passed (or was an expected failure),
# 1 otherwise.

import argparse
import os
import sys
import tempfile
import time
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from client import TmwaClient, ClientError, Timeout, Disconnected  # noqa: E402
from packets import SP_STR, SP_ZENY  # noqa: E402
from server import World  # noqa: E402

E2E_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_BUILD = os.path.join(E2E_DIR, '..', '..', 'build-release')

ACCOUNT = 'e2etest'
PASSWORD = 'e2epass'
CHARNAME = 'E2eTester'

# scenario name -> reason, for known server bugs that cannot be worked
# around without rebuilding (documented in the analysis scratchpad).
XFAIL = {
}


class Ctx(object):
    """State shared between scenarios (inventory indexes and the like)."""

    def __init__(self):
        self.inv = {}      # item id -> 0-based inventory index
        self.mob_id = None


# ---------------------------------------------------------------------------
# scenario helpers

def do_login(world, verbose, timeout=40.0):
    """Full login with account auto-creation and boot-race retries."""
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        client = TmwaClient('127.0.0.1', world.login_port, verbose=verbose)
        for username in (ACCOUNT, ACCOUNT + '_M'):
            try:
                client.full_login(username, PASSWORD, CHARNAME)
                return client
            except (ClientError, Timeout, Disconnected, OSError) as e:
                last = e
                client.disconnect()
        time.sleep(1.0)
    raise RuntimeError('could not log in: %s' % last)


def dialog_expect_close(c, npc_id, timeout=10.0):
    c.map.wait(lambda p: p.id == 0x00b6 and p.u32(2) == npc_id,
               timeout=timeout)
    c.close_dialog(npc_id)


# ---------------------------------------------------------------------------
# scenarios

def s01_login_hook(c, ctx, world):
    """OnPCLoginEvent broadcast label fires on login."""
    c.wait_chat('E2E_LOGIN #1')


def s02_oninit_npctimer(c, ctx, world):
    """on_init ran at boot; the NPC timer fires and re-arms."""
    pkt = c.wait_gm_chat('E2E_TICK boot=7', timeout=15.0)
    text = pkt.tail(4).rstrip(b'\0').decode('utf-8', 'replace')
    assert 'n=' in text, text


def s03_dialog(c, ctx, world):
    """mes / next / menu / input / input_str / close against Greeter."""
    npc = c.find_npc('Greeter')
    c.click_npc(npc)
    c.wait_npc_mes('E2E hello', npc)
    c.map.wait(lambda p: p.id == 0x00b5 and p.u32(2) == npc)
    c.next(npc)
    menu = c.map.wait(lambda p: p.id == 0x00b7 and p.u32(4) == npc)
    text = menu.tail(8).rstrip(b'\0').decode('utf-8')
    assert text == 'alpha:beta:gamma:', repr(text)
    c.menu(npc, 2)
    c.wait_npc_mes('E2E choice=2', npc)
    c.map.wait(lambda p: p.id == 0x00b5 and p.u32(2) == npc)
    c.next(npc)
    c.map.wait(lambda p: p.id == 0x0142 and p.u32(2) == npc)
    c.input_int(npc, 777)
    c.wait_npc_mes('E2E number=777', npc)
    c.map.wait(lambda p: p.id == 0x00b5 and p.u32(2) == npc)
    c.next(npc)
    c.map.wait(lambda p: p.id == 0x01d4 and p.u32(2) == npc)
    c.input_str(npc, 'hello world')
    c.wait_npc_mes('E2E text=hello world', npc)
    dialog_expect_close(c, npc)


def s04_setup_items(c, ctx, world):
    """NPC grants zeny (stat update packet) and items (inventory add)."""
    npc = c.find_npc('Setup')
    c.click_npc(npc)
    zeny = c.map.wait(lambda p: p.id in (0x00b0, 0x00b1)
                      and p.u16(2) == SP_ZENY and p.u32(4) == 5000)
    assert zeny is not None
    for item_id, amount in ((501, 3), (502, 2), (601, 1)):
        pkt = c.map.wait(lambda p, i=item_id, a=amount:
                         p.id == 0x00a0 and p.u16(6) == i and p.u16(4) == a)
        assert pkt.u8(22) == 0, 'pickup fail flag set'
        ctx.inv[item_id] = pkt.u16(2) - 2
    c.wait_npc_mes('E2E setup done', npc)
    dialog_expect_close(c, npc)


def s05_shop(c, ctx, world):
    """Shop NPC: buy list, a purchase, sell list, a sale."""
    npc = c.find_npc('Trader')
    c.click_npc(npc)
    c.map.wait(lambda p: p.id == 0x00c4 and p.u32(2) == npc)
    c.buy_sell(npc, buy=True)
    buylist = c.map.expect(0x00c6)
    entries = [(buylist.u32(4 + i * 11), buylist.u16(4 + i * 11 + 9))
               for i in range((len(buylist.data) - 4) // 11)]
    assert (50, 501) in entries, entries
    c.buy([(2, 501)])
    resp = c.map.expect(0x00ca)
    assert resp.u8(2) == 0, 'buy failed'
    add = c.map.wait(lambda p: p.id == 0x00a0 and p.u16(6) == 501
                     and p.u16(4) == 2)
    ctx.inv[501] = add.u16(2) - 2
    c.map.wait(lambda p: p.id in (0x00b0, 0x00b1) and p.u16(2) == SP_ZENY
               and p.u32(4) == 4900)

    # sell one back
    c.click_npc(npc)
    c.map.wait(lambda p: p.id == 0x00c4 and p.u32(2) == npc)
    c.buy_sell(npc, buy=False)
    selllist = c.map.expect(0x00c7)
    indexes = [selllist.u16(4 + i * 10) - 2
               for i in range((len(selllist.data) - 4) // 10)]
    assert ctx.inv[501] in indexes, (ctx.inv, indexes)
    c.sell([(ctx.inv[501], 1)])
    resp = c.map.expect(0x00cb)
    assert resp.u8(2) == 0, 'sell failed'
    c.map.wait(lambda p: p.id == 0x00af and p.u16(2) - 2 == ctx.inv[501]
               and p.u16(4) == 1)


def s06_storage(c, ctx, world):
    """close2 + openstorage: dialog closes, storage opens, script resumes
    after the client closes the storage window."""
    npc = c.find_npc('Banker')
    for attempt in range(5):
        c.click_npc(npc)
        c.wait_npc_mes('E2E bank', npc)
        c.map.wait(lambda p: p.id == 0x00b6 and p.u32(2) == npc)
        c.close_dialog(npc)
        pkt = c.map.wait(lambda p:
                         p.id in (0x00f2, 0x01f0, 0x00a6)
                         or (p.id == 0x008e
                             and b'E2E_STORAGE_RETRY' in p.tail(4)))
        if pkt.id != 0x008e:
            break
        # First open: the map server had to fetch the storage from the char
        # server; the script saw a refusal, and when the data arrives the
        # server force-opens the storage window with no script attached
        # (old engine behaviour). Wait for that async open, close it, and
        # click again; the next open succeeds synchronously.
        c.map.wait(lambda p: p.id in (0x00f2, 0x01f0, 0x00a6), timeout=5.0)
        c.map.drain(0.2)
        c.storage_close()
        c.map.expect(0x00f8)
    else:
        raise AssertionError('storage never opened')
    c.map.drain(0.2)
    c.storage_close()
    c.map.expect(0x00f8)
    c.wait_chat('E2E_STORAGE_CLOSED')


def s07_item_use_dialog(c, ctx, world):
    """A use script that opens a dialog (engine-owned #itemdialog NPC)."""
    idx = ctx.inv[502]
    c.use_item(idx)
    mes = c.wait_npc_mes('E2E scroll speaks')
    npc = mes.u32(4)     # the #itemdialog NPC's id
    c.wait_npc_mes('E2e Scroll', npc, timeout=1.0)  # the mesn() header line
    c.map.wait(lambda p: p.id == 0x00b5 and p.u32(2) == npc)
    c.next(npc)
    menu = c.map.wait(lambda p: p.id == 0x00b7 and p.u32(4) == npc)
    assert menu.tail(8).rstrip(b'\0') == b'Red pill:Blue pill:'
    c.menu(npc, 1)
    c.wait_npc_mes('E2E pill=1', npc)
    dialog_expect_close(c, npc)
    # the scroll was consumed: 0x01c8 use-item ack with the reduced amount
    # (the server deletes silently, no 0x00af on the use path)
    ack = c.map.wait(lambda p: p.id == 0x01c8 and p.u16(2) - 2 == idx
                     and p.u16(4) == 502)
    assert ack.u8(12) == 1, 'use refused'
    assert ack.u16(10) == 1, 'amount not reduced'


def s08_equip_bonus(c, ctx, world):
    """Equipping the ring runs its equip script: bonus bStr 5 shows up in
    the stat update."""
    idx = ctx.inv[601]
    c.equip_item(idx)
    ack = c.map.wait(lambda p: p.id == 0x00aa and p.u16(2) - 2 == idx)
    assert ack.u8(6) == 1, 'equip refused'
    pkt = c.map.wait(lambda p: p.id == 0x0141 and p.u16(2) == SP_STR
                     and p.u32(10) == 5)
    assert pkt.u32(6) == 5, 'base str changed unexpectedly'
    # and it goes away on unequip
    c.unequip_item(idx)
    c.map.wait(lambda p: p.id == 0x0141 and p.u16(2) == SP_STR
               and p.u32(10) == 0)


def s09_registercmd(c, ctx, world):
    """A chat word registered with server.registercmd reaches its handler
    with the argument string."""
    c.say('@e2echo hello world')
    c.wait_chat('E2E_ECHO:hello world')


def s10_mob_death_event(c, ctx, world):
    """Runtime mob spawn with a death event; the event fires with the
    killer attached."""
    c.say('@e2espawn')
    c.wait_chat('E2E_MOB_SPAWNED')
    spawn = c.map.wait(lambda p:
                       (p.id == 0x0078 and p.u16(14) == 1002)
                       or (p.id == 0x007c and p.u16(20) == 1002))
    mob_id = spawn.u32(2)
    c.wait_chat('E2E_MOB_DEAD')
    c.map.wait(lambda p: p.id == 0x00a0 and p.u16(6) == 501
               and p.u16(4) == 1)
    c.map.wait(lambda p: p.id == 0x0080 and p.u32(2) == mob_id)


def s11_persist_store(c, ctx, world):
    """Store a permanent character variable through a dialog."""
    npc = c.find_npc('Recorder')
    c.click_npc(npc)
    c.map.wait(lambda p: p.id == 0x0142 and p.u32(2) == npc)
    c.input_int(npc, 4242)
    c.wait_npc_mes('E2E stored', npc)
    dialog_expect_close(c, npc)


def s12_warp(c, ctx, world):
    """Walking onto a warp tile teleports (0x0091 change map notify)."""
    c.walk_to(33, 33)
    c.map.expect(0x0087)
    warp = c.map.expect(0x0091, timeout=15.0)
    assert warp.string(2, 16).split('.')[0] == 'test', warp.string(2, 16)
    assert (warp.u16(18), warp.u16(20)) == (20, 20), \
        (warp.u16(18), warp.u16(20))
    c.map_loaded()
    c.map.drain(0.3)


def s13_relog_persistence(c, ctx, world):
    """Variables written through p.vars survive a relog; the login hook
    fires again with the persisted counter."""
    c.disconnect()
    time.sleep(1.0)
    c2 = do_login(world, c.verbose)
    # adopt the new session in place so later cleanup closes the right one
    c.__dict__.update(c2.__dict__)
    c.wait_chat('E2E_LOGIN #2')
    npc = c.find_npc('Recorder')
    c.click_npc(npc)
    c.wait_npc_mes('E2E saved=4242', npc)
    dialog_expect_close(c, npc)


SCENARIOS = [
    ('login-hook', s01_login_hook),
    ('oninit-npctimer', s02_oninit_npctimer),
    ('dialog', s03_dialog),
    ('setup-items', s04_setup_items),
    ('shop', s05_shop),
    ('storage', s06_storage),
    ('item-use-dialog', s07_item_use_dialog),
    ('equip-bonus', s08_equip_bonus),
    ('registercmd', s09_registercmd),
    ('mob-death-event', s10_mob_death_event),
    ('persist-store', s11_persist_store),
    ('warp', s12_warp),
    ('relog-persistence', s13_relog_persistence),
]


# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description='tmwa end-to-end test harness')
    ap.add_argument('--build-dir', default=DEFAULT_BUILD,
                    help='directory with the tmwa binaries (default: '
                         'build-release)')
    ap.add_argument('--base-port',
                    default=int(os.environ.get('E2E_BASE_PORT', 16900)),
                    type=int,
                    help='base port; login/char/map use base+1..base+3')
    ap.add_argument('--world-dir', default=None,
                    help='where to generate the world (default: a temp dir)')
    ap.add_argument('--keep', action='store_true',
                    help='keep the world dir and logs after the run')
    ap.add_argument('--verbose', action='store_true',
                    help='log every packet')
    ap.add_argument('--only', default=None,
                    help='run only the scenario with this name '
                         '(plus login)')
    args = ap.parse_args()

    world_dir = args.world_dir or tempfile.mkdtemp(prefix='tmwa-e2e-')
    world = World(args.build_dir, world_dir, base_port=args.base_port,
                  verbose=args.verbose)

    print('== tmwa e2e: world in %s, ports %d-%d' %
          (world_dir, world.login_port, world.map_port))

    results = []
    client = None
    try:
        world.generate()
        world.start()
        client = do_login(world, args.verbose)
        ctx = Ctx()
        for name, fn in SCENARIOS:
            if args.only and name != args.only:
                continue
            xreason = XFAIL.get(name)
            try:
                fn(client, ctx, world)
            except Exception as e:
                if xreason:
                    results.append((name, 'XFAIL', xreason))
                    print('XFAIL %-20s (%s)' % (name, xreason))
                else:
                    results.append((name, 'FAIL', '%s: %s'
                                    % (type(e).__name__, e)))
                    print('FAIL  %-20s %s: %s' % (name, type(e).__name__, e))
                    if args.verbose:
                        traceback.print_exc()
                if not world.alive():
                    print('!! a server process died; aborting')
                    break
            else:
                if xreason:
                    results.append((name, 'XPASS', xreason))
                    print('XPASS %-20s (expected to fail: %s)'
                          % (name, xreason))
                else:
                    results.append((name, 'PASS', ''))
                    print('PASS  %-20s' % name)
    except Exception as e:
        print('!! harness error: %s: %s' % (type(e).__name__, e))
        traceback.print_exc()
        results.append(('<harness>', 'FAIL', str(e)))
        world.dump_logs()
    finally:
        if client is not None:
            client.disconnect()
        world.stop()
        if args.keep:
            print('== world kept in %s' % world_dir)
        elif args.world_dir is None:
            import shutil
            shutil.rmtree(world_dir, ignore_errors=True)

    npass = sum(1 for _, s, _ in results if s in ('PASS', 'XFAIL'))
    nfail = len(results) - npass
    print('== %d ok, %d failing' % (npass, nfail))
    return 1 if nfail else 0


if __name__ == '__main__':
    sys.exit(main())
