// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! Shared runtime state for `tmwa-gate serve`.

use std::collections::HashMap;
use std::sync::Mutex;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

use tokio::sync::mpsc;

use crate::config::Config;
use crate::proto::PartyMost;

/// delflag values mirroring tmwa's auth_fifo semantics:
/// 2 = login->char stage pending, 3 = char->map stage pending,
/// 1 = consumed, 0 = consumed (map-to-map).
pub struct AuthEntry {
    pub account_id: u32,
    pub char_id: u32,
    pub login_id1: u32,
    pub login_id2: u32,
    /// Client IP as seen by the gate at the stage that created the
    /// entry.
    pub ip: u32,
    pub client_version: u32,
    /// Which map server holds this character (char->map stage).
    pub map_id: Option<usize>,
    /// Set by the relay when the upstream connection is created: the
    /// local address tmwa-map sees for this client (and reports back
    /// in 0x2afc).
    pub upstream_ip: Option<u32>,
    pub delflag: u8,
    pub created: Instant,
}

const AUTH_TTL: Duration = Duration::from_secs(300);

/// A client connection being relayed to a map server. The relay
/// keeps this record so a crashed/restarting map can be held and
/// rejoined without dropping the client.
pub struct PlayerSession {
    pub account_id: u32,
    pub char_id: u32,
    pub sex: u8,
    pub login_id1: u32,
    pub login_id2: u32,
    /// Real client IP (the socket's peer address).
    pub client_ip: u32,
    /// Last server tick seen on 0x007f replies (used to keep
    /// answering client pings while the map is down).
    pub server_tick: u32,
    pub server_tick_at: Instant,
    /// Map server slot this player is/was on.
    pub map_id: usize,
    /// The map name the character is on (for rejoin matching).
    pub map_name: String,
    /// Set when the upstream goes away: the saved map may have
    /// changed (warp + shutdown save), so the next hold reloads it.
    pub map_name_stale: bool,
    /// Client-side UI state rebuilt by watching S->C traffic.
    pub npc_id: u32,
    pub trade_open: bool,
    pub storage_open: bool,
    /// Client asked to quit (0x00b2): an upstream close is then a
    /// normal logout, not a crash.
    pub quitting: bool,
    /// The map sent 0x0073 on the current upstream connection: it
    /// accepted the player, so a later close is a per-player kick,
    /// not a refusal.
    pub saw_0073: bool,
    /// The map sent a 0x0092 handoff: the client reconnects with a
    /// fresh 0x0072, so this session ends without hold.
    pub transferring: bool,
    /// Upstream gone; waiting for the map to come back.
    pub held: bool,
    /// Set by `drain` to force the relay into hold mode.
    pub hold_signal: Option<std::sync::Arc<tokio::sync::Notify>>,
    /// Admin kicked: the relay closes the client connection.
    pub kicked: bool,
    /// When the player was put on hold (for the timeout).
    pub held_since: Option<Instant>,
}

/// One connected tmwa-map session.
pub struct MapHandle {
    pub id: usize,
    /// Packet sender into the map session writer task.
    pub tx: mpsc::Sender<Vec<u8>>,
    /// Address advertised in the 0x2af8 login packet (client port of
    /// the map server, where the gate's relay connects).
    pub ip: u32,
    pub port: u16,
    /// Maps this server loaded (from 0x2afa), in file order.
    pub maps: Vec<String>,
    /// Users as last reported by 0x2aff.
    pub users: u16,
    /// Marked by `drain`: no new players are sent here.
    pub draining: bool,
    /// Set when the map sent 0x2b17 (term_func): the link will drop
    /// shortly and every player on it needs holding.
    pub shutting_down: bool,
}

/// A map-link writer queue below this many free slots counts as
/// congested: new char-selects and map joins aimed at it are
/// refused instead of queueing behind a saturated link.
const MAP_TX_LOW_WATER: usize = 64;

impl MapHandle {
    /// Non-critical send (broadcasts, notifications): dropped with a
    /// warning when the writer queue is full. Returns whether the
    /// packet was queued.
    pub fn send(&self, bytes: Vec<u8>) -> bool {
        match self.tx.try_send(bytes) {
            Ok(()) => true,
            Err(e) => {
                tracing::warn!("map {id}: queue full/dropped: {e}", id = self.id);
                false
            }
        }
    }

    /// The link is congested when its writer backlog is deep: shed
    /// new work (char-selects, joins) rather than add to it.
    pub fn congested(&self) -> bool {
        self.tx.capacity() < MAP_TX_LOW_WATER
    }
}

/// A client's char-stage session waiting on 0x3830.
pub struct PendingSel {
    pub client_tx: mpsc::Sender<Vec<u8>>,
    pub account_id: u32,
    pub char_id: u32,
    pub login_id1: u32,
    pub login_id2: u32,
    pub map_name: String,
    /// Map server slot the player will connect to (carries the
    /// advertised address into the 0x0071 reply).
    pub map_id: usize,
    /// Map server slots that still owe a 0x3830 (tmwa waits for all
    /// connected map servers). A congested link that dropped the
    /// 0x3829 is not in here, so a saturating server can't stall
    /// the select.
    pub waiting: std::collections::HashSet<usize>,
}

/// What the gate must remember about an online player to pre-auth
/// (0x3829) them on a map server that registers late.
#[derive(Clone)]
pub struct OnlineAuth {
    pub account_id: u32,
    pub char_id: u32,
    pub login_id1: u32,
    pub login_id2: u32,
    /// Real client IP, as seen by the gate at the stage that
    /// created the entry.
    pub ip: u32,
    /// Map slot the player is on; mid-transfer this is the source
    /// until the destination's 0x2afc or 0x2aff corrects it.
    pub server: usize,
}

/// A cached character (mirrors tmwa's in-memory char_db).
pub struct CharRecord {
    pub key: crate::proto::CharKey,
    pub data: crate::proto::CharData,
    /// Map server the character is online on (from 0x2aff), or None.
    pub online_map: Option<usize>,
}

pub struct State {
    pub cfg: Config,
    pub db: std::sync::Arc<crate::db::Db>,
    /// account_id -> pending auth entry (a new login replaces the
    /// account's previous entry; entries expire after AUTH_TTL).
    pub auth: Mutex<HashMap<u32, AuthEntry>>,
    /// (account_id, char_id) of clients waiting on 0x3830.
    pub pending_sel: Mutex<HashMap<(u32, u32), PendingSel>>,
    /// char_id -> auth material of each online player, so a map
    /// server that registers late can accept transfers and logins
    /// immediately (gate pushes 0x3829 right after 0x2afa).
    pub online_auth: Mutex<HashMap<u32, OnlineAuth>>,
    /// (account_id, char_id) mid map-to-map transfer: the source map
    /// sent 0x2b01 right before the 0x2b05 that set the mark. A
    /// 0x2afc arriving on another link for a marked char waits for
    /// in-flight saves to commit first (bounded).
    pub transfer_pending: Mutex<HashMap<(u32, u32), Instant>>,
    /// char_id -> number of 0x2b01 saves received but not yet
    /// committed to the database.
    pub saves_in_flight: Mutex<HashMap<u32, u32>>,
    /// Fired whenever an in-flight save commits.
    pub save_notify: tokio::sync::Notify,
    /// Map server slots; None = free.
    pub map_servers: Mutex<Vec<Option<MapHandle>>>,
    /// char_id -> map server slot (online in game).
    pub online: Mutex<HashMap<u32, usize>>,
    /// Character cache.
    pub chars: Mutex<HashMap<u32, CharRecord>>,
    /// name -> char_id
    pub char_names: Mutex<HashMap<String, u32>>,
    /// account_id -> char session sender, while the client is on the
    /// char screen (for disconnect_player / kicked-by-other-login).
    pub char_sessions: Mutex<HashMap<u32, mpsc::Sender<Vec<u8>>>>,
    /// account_id -> gm level
    pub gm: Mutex<HashMap<u32, u32>>,
    pub gm_mtime: Mutex<Option<std::time::SystemTime>>,
    pub parties: Mutex<HashMap<u32, PartyMost>>,
    pub next_party_id: AtomicU64,
    /// login flood protection: ip -> last attempt. Entries older
    /// than the configured interval are pruned on each insert.
    pub recent_logins: Mutex<HashMap<u32, Instant>>,
    /// notify the online-file writer to refresh
    pub online_notify: tokio::sync::Notify,
    /// char_id -> live player relay session.
    pub player_sessions: Mutex<HashMap<u32, std::sync::Arc<Mutex<PlayerSession>>>>,
    /// (account, char) -> oneshot fired when the map answers 0x3830
    /// for a rejoin.
    pub rejoin_notify: Mutex<HashMap<(u32, u32), tokio::sync::oneshot::Sender<()>>>,
    /// TCP+WS client connections currently open.
    pub conn_count: AtomicU64,
}

impl State {
    pub fn new(cfg: Config, db: std::sync::Arc<crate::db::Db>) -> State {
        let next_party_id = db.meta("next_party_id").ok().flatten().unwrap_or(0) as u64;
        State {
            cfg,
            db,
            auth: Mutex::new(HashMap::new()),
            pending_sel: Mutex::new(HashMap::new()),
            online_auth: Mutex::new(HashMap::new()),
            transfer_pending: Mutex::new(HashMap::new()),
            saves_in_flight: Mutex::new(HashMap::new()),
            save_notify: tokio::sync::Notify::new(),
            map_servers: Mutex::new(Vec::new()),
            online: Mutex::new(HashMap::new()),
            chars: Mutex::new(HashMap::new()),
            char_names: Mutex::new(HashMap::new()),
            char_sessions: Mutex::new(HashMap::new()),
            gm: Mutex::new(HashMap::new()),
            gm_mtime: Mutex::new(None),
            parties: Mutex::new(HashMap::new()),
            next_party_id: AtomicU64::new(next_party_id),
            conn_count: AtomicU64::new(0),
            recent_logins: Mutex::new(HashMap::new()),
            online_notify: tokio::sync::Notify::new(),
            player_sessions: Mutex::new(HashMap::new()),
            rejoin_notify: Mutex::new(HashMap::new()),
        }
    }

    /// Mark a map server draining / clear the flag.
    pub fn map_set_draining(&self, id: usize, draining: bool) {
        let mut ms = self.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get_mut(id) {
            h.draining = draining;
        }
    }

    /// Cryptographic random; callers must refuse the action on
    /// error, never fall back to a predictable value.
    pub fn random_u32() -> Option<u32> {
        let mut b = [0u8; 4];
        getrandom::fill(&mut b).ok()?;
        Some(u32::from_le_bytes(b))
    }

    pub fn push_auth(&self, e: AuthEntry) {
        let mut a = self.auth.lock().unwrap();
        let now = Instant::now();
        a.retain(|_, x| x.delflag != 1 && now.duration_since(x.created) < AUTH_TTL);
        a.insert(e.account_id, e);
    }

    /// Take a pending auth entry (marks it consumed). `stage` is the
    /// expected delflag.
    pub fn take_auth<F: FnMut(&AuthEntry) -> bool>(
        &self,
        stage: u8,
        mut pred: F,
    ) -> Option<AuthEntry> {
        let mut a = self.auth.lock().unwrap();
        let now = Instant::now();
        a.retain(|_, x| now.duration_since(x.created) < AUTH_TTL);
        let key = a
            .iter()
            .find(|(_, e)| e.delflag == stage && pred(e))
            .map(|(k, _)| *k)?;
        let mut e = a.remove(&key)?;
        e.delflag = 1;
        Some(e)
    }

    /// Find without consuming (relay uses the same entry for the
    /// upcoming 0x2afc).
    pub fn find_auth<F: FnMut(&AuthEntry) -> bool>(&self, stage: u8, mut pred: F) -> bool {
        let mut a = self.auth.lock().unwrap();
        let now = Instant::now();
        a.retain(|_, x| now.duration_since(x.created) < AUTH_TTL);
        a.values().any(|e| e.delflag == stage && pred(e))
    }

    /// Mark upstream_ip on a matching entry (relay learned the
    /// address tmwa-map will see).
    pub fn set_auth_upstream_ip(&self, account_id: u32, char_id: u32, login_id1: u32, ip: u32) {
        let mut a = self.auth.lock().unwrap();
        if let Some(e) = a.get_mut(&account_id) {
            if e.delflag == 3 && e.char_id == char_id && e.login_id1 == login_id1 {
                e.upstream_ip = Some(ip);
            }
        }
    }

    /// Consume a stage-3 entry (0x2afc).
    pub fn take_map_auth(
        &self,
        account_id: u32,
        char_id: u32,
        login_id1: u32,
        login_id2: u32,
        ip: u32,
    ) -> Option<AuthEntry> {
        self.take_auth(3, |e| {
            e.account_id == account_id
                && e.char_id == char_id
                && e.login_id1 == login_id1
                && (e.login_id2 == login_id2 || login_id2 == 0)
                && (e.upstream_ip.unwrap_or(e.ip) == ip || e.ip == ip)
        })
    }

    // ---- transfer / save ordering ----

    /// 0x2b05 named this char: a map-to-map transfer started, and the
    /// source link's save may still be committing when the target
    /// link's 0x2afc arrives.
    pub fn transfer_mark(&self, account_id: u32, char_id: u32) {
        let mut tp = self.transfer_pending.lock().unwrap();
        let now = Instant::now();
        tp.retain(|_, t| now.duration_since(*t) < Duration::from_secs(60));
        tp.insert((account_id, char_id), now);
    }

    /// Take the transfer mark for a char (0x2afc uses it to decide
    /// whether to wait on in-flight saves).
    pub fn transfer_take(&self, account_id: u32, char_id: u32) -> bool {
        self.transfer_pending
            .lock()
            .unwrap()
            .remove(&(account_id, char_id))
            .is_some()
    }

    /// A 0x2b01 for this char was received; the DB write is queued.
    pub fn save_begin(&self, char_id: u32) {
        *self
            .saves_in_flight
            .lock()
            .unwrap()
            .entry(char_id)
            .or_insert(0) += 1;
    }

    /// The queued save for this char committed (or failed).
    pub fn save_done(&self, char_id: u32) {
        let mut s = self.saves_in_flight.lock().unwrap();
        if let Some(n) = s.get_mut(&char_id) {
            *n -= 1;
            if *n == 0 {
                s.remove(&char_id);
            }
        }
        drop(s);
        self.save_notify.notify_waiters();
    }

    /// Wait until no 0x2b01 save for `char_id` is in flight.
    /// Returns false on timeout.
    pub async fn wait_saves(&self, char_id: u32, dur: Duration) -> bool {
        let deadline = Instant::now() + dur;
        loop {
            // register the waiter before checking the count, so a
            // commit landing in between can't be missed
            let notified = self.save_notify.notified();
            tokio::pin!(notified);
            notified.as_mut().enable();
            if self
                .saves_in_flight
                .lock()
                .unwrap()
                .get(&char_id)
                .copied()
                .unwrap_or(0)
                == 0
            {
                return true;
            }
            let left = deadline.saturating_duration_since(Instant::now());
            if left.is_zero() {
                return false;
            }
            if tokio::time::timeout(left, notified).await.is_err() {
                return false;
            }
        }
    }

    // ---- online auth (pre-auth for late-registered maps) ----

    /// Remember auth material for a player going in game, so a map
    /// server registering later can be pre-authed (0x3829) for
    /// transfers/logins.
    pub fn set_online_auth(&self, e: OnlineAuth) {
        self.online_auth.lock().unwrap().insert(e.char_id, e);
    }

    /// Forget auth material for an account's chars (0x2b02).
    pub fn drop_account_online_auth(&self, account_id: u32) {
        self.online_auth
            .lock()
            .unwrap()
            .retain(|_, e| e.account_id != account_id);
    }

    /// A fresh copy of every online player's auth material (for
    /// pushing 0x3829 to a late-registered map).
    pub fn online_auths(&self) -> Vec<OnlineAuth> {
        self.online_auth.lock().unwrap().values().cloned().collect()
    }

    /// Reconcile the online-auth table with a map's 0x2aff user
    /// list: entries this map reported keep/get its slot; entries
    /// it owns but no longer lists are dropped (the player left).
    pub fn reconcile_online_auth(&self, map_id: usize, chars: &[u32]) {
        let listed: std::collections::HashSet<u32> = chars.iter().copied().collect();
        self.online_auth.lock().unwrap().retain(|cid, e| {
            if listed.contains(cid) {
                e.server = map_id;
                true
            } else {
                e.server != map_id
            }
        });
    }

    /// Consume a stage-2 entry (0x0065 char connect / 0x2b02 stage).
    pub fn take_char_auth(
        &self,
        account_id: u32,
        id1: u32,
        id2: u32,
        ip: u32,
    ) -> Option<AuthEntry> {
        self.take_auth(2, |e| {
            e.account_id == account_id && e.login_id1 == id1 && e.login_id2 == id2 && e.ip == ip
        })
    }

    // ---- map servers ----

    /// Allocate a map server slot; returns the slot index.
    pub fn map_register(&self, tx: mpsc::Sender<Vec<u8>>, ip: u32, port: u16) -> usize {
        let mut ms = self.map_servers.lock().unwrap();
        for (i, slot) in ms.iter_mut().enumerate() {
            if slot.is_none() {
                let id = i;
                *slot = Some(MapHandle {
                    id,
                    tx,
                    ip,
                    port,
                    maps: vec![],
                    users: 0,
                    draining: false,
                    shutting_down: false,
                });
                return id;
            }
        }
        let id = ms.len();
        ms.push(Some(MapHandle {
            id,
            tx,
            ip,
            port,
            maps: vec![],
            users: 0,
            draining: false,
            shutting_down: false,
        }));
        id
    }

    pub fn map_unregister(&self, id: usize) {
        {
            let mut ms = self.map_servers.lock().unwrap();
            if let Some(slot) = ms.get_mut(id) {
                *slot = None;
            }
        }
        // drop online marks + pre-auth material for that map
        self.online.lock().unwrap().retain(|_, v| *v != id);
        self.online_auth
            .lock()
            .unwrap()
            .retain(|_, e| e.server != id);
        self.online_notify.notify_waiters();
        // resolve char-selects that were waiting on this link's
        // 0x3830 (or that it was the target of)
        let mut done = Vec::new();
        {
            let mut pend = self.pending_sel.lock().unwrap();
            for ps in pend.values_mut() {
                ps.waiting.remove(&id);
            }
            let keys: Vec<(u32, u32)> = pend
                .iter()
                .filter(|(_, ps)| ps.map_id == id || ps.waiting.is_empty())
                .map(|(k, _)| *k)
                .collect();
            for k in keys {
                if let Some(ps) = pend.remove(&k) {
                    done.push(ps);
                }
            }
        }
        for ps in done {
            self.send_pending_sel(ps);
        }
    }

    /// Remove `mid` from a pending select's waiting set (its 0x3830
    /// arrived, or its link went away mid-select). When nothing
    /// else is awaited the entry is popped and returned so the
    /// caller can `send_pending_sel` it.
    pub(crate) fn sel_waiting_done(
        &self,
        key: (u32, u32),
        mid: usize,
        login_id1: u32,
        login_id2: u32,
    ) -> Option<PendingSel> {
        let mut pend = self.pending_sel.lock().unwrap();
        let ps = pend.get_mut(&key)?;
        if ps.login_id1 != login_id1 || ps.login_id2 != login_id2 {
            return None;
        }
        ps.waiting.remove(&mid);
        if ps.waiting.is_empty() {
            pend.remove(&key)
        } else {
            None
        }
    }

    /// Complete a char-select: 0x0071 with the target map server's
    /// registered address, or 0x0081 if it is gone.
    pub(crate) fn send_pending_sel(&self, ps: PendingSel) {
        use crate::proto::types::{FixedStr, Ip4Address};
        use crate::proto::{CharId, P0071, P0081};
        let addr = self.map_addr(ps.map_id);
        match addr {
            Some((ip, port)) => {
                let mut p = P0071::default();
                p.char_id = CharId(ps.char_id);
                p.map_name = FixedStr::<16>::try_from_str(&ps.map_name).unwrap_or_default();
                p.ip = Ip4Address(ip.to_le_bytes());
                p.port = port;
                send_bytes(&ps.client_tx, enc(move |v| p.encode(v)));
            }
            None => {
                let mut p = P0081::default();
                p.error_code = 1;
                send_bytes(&ps.client_tx, enc(move |v| p.encode(v)));
            }
        }
    }

    /// Send to every connected map server; returns count.
    pub fn map_broadcast(&self, bytes: &[u8]) -> usize {
        let ms = self.map_servers.lock().unwrap();
        let mut n = 0;
        for slot in ms.iter().flatten() {
            slot.send(bytes.to_vec());
            n += 1;
        }
        n
    }

    /// Send to one map server by slot.
    pub fn map_send(&self, id: usize, bytes: Vec<u8>) -> bool {
        let ms = self.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get(id) {
            h.send(bytes)
        } else {
            false
        }
    }

    /// The writer queue of map `id`'s link is nearly full.
    pub fn map_congested(&self, id: usize) -> bool {
        let ms = self.map_servers.lock().unwrap();
        matches!(ms.get(id), Some(Some(h)) if h.congested())
    }

    /// Whether map `id` exists and is marked draining.
    pub fn map_draining(&self, id: usize) -> bool {
        let ms = self.map_servers.lock().unwrap();
        matches!(ms.get(id), Some(Some(h)) if h.draining)
    }

    /// (slot, sender) pairs for every connected map server.
    pub fn map_senders(&self) -> Vec<(usize, mpsc::Sender<Vec<u8>>)> {
        let ms = self.map_servers.lock().unwrap();
        ms.iter()
            .enumerate()
            .filter_map(|(i, s)| s.as_ref().map(|h| (i, h.tx.clone())))
            .collect()
    }

    /// The sender for map `id`'s link, for must-not-drop replies.
    pub fn map_tx(&self, id: usize) -> Option<mpsc::Sender<Vec<u8>>> {
        let ms = self.map_servers.lock().unwrap();
        ms.get(id).and_then(|s| s.as_ref().map(|h| h.tx.clone()))
    }

    /// Map slot -> (registered client address, served maps, draining,
    /// user count) for all connected servers.
    pub fn map_infos(&self) -> Vec<(usize, u32, u16, Vec<String>, bool, u16)> {
        let ms = self.map_servers.lock().unwrap();
        ms.iter()
            .enumerate()
            .filter_map(|(i, s)| {
                s.as_ref()
                    .map(|h| (i, h.ip, h.port, h.maps.clone(), h.draining, h.users))
            })
            .collect()
    }

    /// Map server slot that serves `map`, or the first slot with maps.
    /// On fallback, `rewritten` is set to the server's first map.
    pub fn map_for(&self, map: &str) -> (Option<usize>, Option<String>) {
        let ms = self.map_servers.lock().unwrap();
        for (i, slot) in ms.iter().enumerate() {
            if let Some(h) = slot {
                if !h.draining && h.maps.iter().any(|m| m == map) {
                    return (Some(i), None);
                }
            }
        }
        for (i, slot) in ms.iter().enumerate() {
            if let Some(h) = slot {
                if !h.draining {
                    if let Some(first) = h.maps.first() {
                        return (Some(i), Some(first.clone()));
                    }
                }
            }
        }
        (None, None)
    }

    pub fn map_count(&self) -> usize {
        self.map_servers.lock().unwrap().iter().flatten().count()
    }

    /// (ip, port) the relay should connect to for map `id`.
    pub fn map_addr(&self, id: usize) -> Option<(u32, u16)> {
        let ms = self.map_servers.lock().unwrap();
        ms.get(id).and_then(|s| s.as_ref().map(|h| (h.ip, h.port)))
    }

    /// The map link is down or the map announced shutdown: players
    /// on it need holding, not per-player disconnects.
    pub fn map_gone(&self, id: usize) -> bool {
        let ms = self.map_servers.lock().unwrap();
        match ms.get(id) {
            Some(Some(h)) => h.shutting_down,
            _ => true,
        }
    }
    pub fn map_set_shutting_down(&self, id: usize) {
        let mut ms = self.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get_mut(id) {
            h.shutting_down = true;
        }
    }

    // ---- characters / online ----

    pub fn conn_inc(&self) {
        self.conn_count.fetch_add(1, Ordering::Relaxed);
    }
    pub fn conn_dec(&self) {
        self.conn_count.fetch_sub(1, Ordering::Relaxed);
    }
    pub fn conn_count(&self) -> usize {
        self.conn_count.load(Ordering::Relaxed) as usize
    }

    pub fn count_users(&self) -> usize {
        self.online.lock().unwrap().len()
    }

    /// char name -> char_id (loads from DB once)
    pub async fn char_by_name(&self, name: &str) -> Option<u32> {
        if let Some(&id) = self.char_names.lock().unwrap().get(name) {
            return Some(id);
        }
        let name2 = name.to_string();
        let id = self
            .db
            .blocking(move |db| db.char_id_by_name(&name2))
            .await
            .ok()??;
        let mut names = self.char_names.lock().unwrap();
        names.insert(name.to_string(), id as u32);
        Some(id as u32)
    }

    /// Reconcile a char's party_id with the live party membership
    /// (tmwa's char_fix_party): a char not in any party has
    /// party_id 0; a member gets the id of the party that lists it.
    fn fix_party_id(&self, key: &crate::proto::CharKey, data: &mut crate::proto::CharData) {
        let parties = self.parties.lock().unwrap();
        let mut found = 0u32;
        for (pid, p) in parties.iter() {
            if p.member.iter().any(|m| {
                m.account_id.0 != 0 && m.account_id == key.account_id && m.name == key.name
            }) {
                found = *pid;
                break;
            }
        }
        if data.party_id.0 != found {
            data.party_id = crate::proto::PartyId(found);
            let cid = key.char_id.0 as i64;
            let db = self.db.clone();
            drop(tokio::task::spawn_blocking(move || {
                db.with_conn(move |conn| {
                    conn.execute(
                        "UPDATE characters SET party_id=?2 WHERE id=?1",
                        rusqlite::params![cid, found as i64],
                    )
                })
            }));
        }
    }

    /// Load a character into the cache (from DB if needed).
    pub async fn load_char(&self, char_id: u32) -> Option<CharRecord> {
        if let Some(c) = self.chars.lock().unwrap().get(&char_id) {
            return Some(CharRecord {
                key: c.key,
                data: c.data,
                online_map: c.online_map,
            });
        }
        let (key, mut data) = self
            .db
            .blocking(move |db| db.load_character(char_id as i64))
            .await
            .ok()?;
        self.fix_party_id(&key, &mut data);
        let mut chars = self.chars.lock().unwrap();
        let rec = CharRecord {
            key,
            data,
            online_map: None,
        };
        chars.insert(
            char_id,
            CharRecord {
                key: rec.key,
                data: rec.data,
                online_map: None,
            },
        );
        Some(rec)
    }
}

/// Send raw bytes to a map link's writer task when the reply must
/// not be dropped (auth answers, request results, saves). A full
/// queue gets a bounded wait; only a wedged link loses the reply.
/// Returns false when the bytes were not queued.
pub async fn send_must(tx: &mpsc::Sender<Vec<u8>>, v: Vec<u8>, map_id: usize) -> bool {
    use tokio::sync::mpsc::error::TrySendError;
    match tx.try_send(v) {
        Ok(()) => true,
        Err(TrySendError::Full(v)) | Err(TrySendError::Closed(v)) => {
            match tokio::time::timeout(Duration::from_secs(5), tx.send(v)).await {
                Ok(Ok(())) => true,
                _ => {
                    tracing::warn!("map {map_id}: dropping critical reply, link wedged");
                    false
                }
            }
        }
    }
}

/// Send raw bytes to a client session's writer task.
pub fn send_bytes(tx: &mpsc::Sender<Vec<u8>>, v: Vec<u8>) {
    let _ = tx.try_send(v);
}

/// Encode helper: `enc(|v| p.encode(v))`.
pub fn enc(f: impl FnOnce(&mut Vec<u8>)) -> Vec<u8> {
    let mut v = Vec::new();
    f(&mut v);
    v
}
