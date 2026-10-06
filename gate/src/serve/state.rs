// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! Shared runtime state for `tmwa-gate serve`.
//!
//! `State` owns the shared tables plus their accessors. The
//! serialized DB writer lives in `dbq.rs` (an `impl State` block
//! there keeps the queue invariants in one file), and the
//! pending-select reply (0x0071/0x0081) is built by
//! `client::send_pending_sel` while the `pending_sel` bookkeeping
//! stays here.

use std::collections::{HashMap, HashSet};
use std::sync::Mutex;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

use bytes::Bytes;
use tokio::sync::mpsc;

use super::dbq::{DB_JOBS_LIMIT, DbJob, DirtyStorage};
use crate::config::Config;
use crate::proto::{CharData, PartyMost};

/// `AuthEntry::delflag` (the name mirrors tmwa's auth_fifo flag):
/// login stage done, waiting on the char stage (0x0065 connect or
/// 0x2b02's return to char select).
pub const DELFLAG_CHAR: u8 = 2;
/// `AuthEntry::delflag`: char stage done, waiting on a map
/// server's 0x2afc.
pub const DELFLAG_MAP: u8 = 3;

/// A pending login handoff. Unlike tmwa's auth_fifo, entries are
/// removed (not flagged) on consume; a stale entry expires after
/// AUTH_TTL.
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
    /// Stage the entry waits on (`DELFLAG_*`).
    pub delflag: u8,
    pub created: Instant,
}

const AUTH_TTL: Duration = Duration::from_secs(300);

impl AuthEntry {
    /// Within its TTL? Matched entries are checked individually
    /// because the table sweep is throttled, not run per access.
    pub fn fresh(&self, now: Instant) -> bool {
        now.duration_since(self.created) < AUTH_TTL
    }
}

/// Minimum interval between full expiry sweeps of an auth table.
/// The sweep is O(n), so it is throttled instead of running per
/// access; entries are TTL-checked at match time, so a skipped
/// sweep only delays reclamation.
const SWEEP_INTERVAL: Duration = Duration::from_secs(1);

/// A `HashMap` of timestamped entries swept for expiry at most
/// once per `SWEEP_INTERVAL` (`sweep`); callers still check each
/// matched entry's own TTL.
pub struct SweptMap<V> {
    /// The entries, keyed by account_id.
    pub map: HashMap<u32, V>,
    last_sweep: Instant,
}

impl<V> SweptMap<V> {
    fn new() -> SweptMap<V> {
        SweptMap {
            map: HashMap::new(),
            last_sweep: Instant::now(),
        }
    }

    /// Drop entries whose timestamp (`at`) is older than `ttl`;
    /// no-op when less than `SWEEP_INTERVAL` passed since the last
    /// sweep.
    fn sweep(&mut self, now: Instant, ttl: Duration, at: impl Fn(&V) -> Instant) {
        if now.duration_since(self.last_sweep) < SWEEP_INTERVAL {
            return;
        }
        self.last_sweep = now;
        self.map.retain(|_, v| now.duration_since(at(v)) < ttl);
    }
}

/// Reply state of a served map-auth reservation: Pending until the
/// first 0x2afc finishes building its answer, then Ready with the
/// exact 0x2afd bytes or Failed when the serve aborted (repeats
/// just re-reject).
#[derive(Clone)]
pub enum ServedReply {
    Pending,
    Ready(Vec<u8>),
    Failed,
}

/// A 0x2afc the gate answered (or is answering) with 0x2afd, kept
/// briefly because the map re-pushes the request when the link
/// flaps and the original answer may never have arrived. Matched
/// like a stage-3 `AuthEntry`; re-served only to the map server
/// (registered client address) the answer went to, so a different
/// server cannot claim the login.
pub struct ServedAuth {
    pub account_id: u32,
    pub char_id: u32,
    pub login_id1: u32,
    pub login_id2: u32,
    pub ip: u32,
    pub upstream_ip: Option<u32>,
    /// Registered client address of the map link the reply went to.
    pub map_ip: u32,
    pub map_port: u16,
    /// Resolves the reservation for waiting repeats.
    pub reply: tokio::sync::watch::Sender<ServedReply>,
    pub served: Instant,
}

/// Grace window for re-serving an answered 0x2afc: comfortably
/// longer than a map link's reconnect-and-repush cycle.
const SERVED_TTL: Duration = Duration::from_secs(60);

impl ServedAuth {
    /// Within its TTL? Checked at match time like `AuthEntry`.
    fn fresh(&self, now: Instant) -> bool {
        now.duration_since(self.served) < SERVED_TTL
    }
}

/// The credential tuple a 0x2afc request carries: what a pending
/// stage-3 entry is matched on and what a served reservation
/// re-matches.
#[derive(Clone, Copy)]
pub struct MapAuthReq {
    pub account_id: u32,
    pub char_id: u32,
    pub login_id1: u32,
    pub login_id2: u32,
    /// The client address the map reports (the relay's upstream
    /// source address when set).
    pub ip: u32,
}

impl MapAuthReq {
    /// The tuple check shared by `take_map_auth` and
    /// `served_map_auth`: strict match except `login_id2`, where a
    /// request of 0 is a wildcard.
    fn matches(&self, e: &AuthEntry) -> bool {
        e.delflag == DELFLAG_MAP
            && e.account_id == self.account_id
            && e.char_id == self.char_id
            && e.login_id1 == self.login_id1
            && (e.login_id2 == self.login_id2 || self.login_id2 == 0)
            && (e.upstream_ip.unwrap_or(e.ip) == self.ip || e.ip == self.ip)
    }

    /// Same check against a served reservation (no delflag).
    fn matches_served(&self, r: &ServedAuth) -> bool {
        r.char_id == self.char_id
            && r.login_id1 == self.login_id1
            && (r.login_id2 == self.login_id2 || self.login_id2 == 0)
            && (r.upstream_ip.unwrap_or(r.ip) == self.ip || r.ip == self.ip)
    }
}

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

/// Most fields start zeroed; only the per-connection values (ids,
/// tick, map, hold_signal) are set at construction.
impl Default for PlayerSession {
    fn default() -> Self {
        PlayerSession {
            account_id: 0,
            char_id: 0,
            sex: 0,
            login_id1: 0,
            login_id2: 0,
            client_ip: 0,
            server_tick: 0,
            server_tick_at: Instant::now(),
            map_id: 0,
            map_name: String::new(),
            map_name_stale: false,
            npc_id: 0,
            trade_open: false,
            storage_open: false,
            quitting: false,
            saw_0073: false,
            transferring: false,
            held: false,
            hold_signal: None,
            kicked: false,
            held_since: None,
        }
    }
}

/// One connected tmwa-map session.
pub struct MapHandle {
    pub id: usize,
    /// Bulk queue into the map session writer task: broadcasts,
    /// notifications, pre-auth floods. Dropped-first under load.
    /// `Bytes` so a broadcast is a refcount clone per recipient
    /// rather than a fresh copy.
    pub tx: mpsc::Sender<Bytes>,
    /// Critical queue, drained before `tx`: request replies the map
    /// is blocked waiting on (0x2afd/0x3810/0x2b06/0x382a/...).
    /// `send_must` targets this so bulk floods can't starve answers.
    pub tx_prio: mpsc::Sender<Bytes>,
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
    /// Fired when a critical reply could not be queued: the link is
    /// wedged beyond recovery and its session is asked to die so
    /// the map reconnects with a clean slate.
    pub kill: std::sync::Arc<tokio::sync::Notify>,
}

/// A map-link writer queue below this many free slots counts as
/// congested: new char-selects and map joins aimed at it are
/// refused instead of queueing behind a saturated link.
const MAP_TX_LOW_WATER: usize = 256;

/// Map-slot sentinel for "no owning link": pre-registration
/// replies (before `map_register` hands out a slot) and broadcast
/// paths pass it to `send_must`/`map_broadcast_except`, where it
/// matches nothing (`map_kill` keys on a real slot).
pub const NO_MAP: usize = usize::MAX;

/// Snapshot backing `map_for`: map name to the lowest
/// non-draining slot serving it, plus the first non-draining
/// server with maps and its first map name (the tmwa "unknown
/// map" fallback).
/// Rebuilt by `State::rebuild_map_index`.
#[derive(Default)]
struct MapIndex {
    by_name: HashMap<String, usize>,
    /// (slot, first map name) of the fallback server.
    fallback: Option<(usize, String)>,
}

impl MapHandle {
    /// Non-critical send (broadcasts, notifications): dropped with a
    /// warning when the writer queue is full. Returns whether the
    /// packet was queued.
    pub fn send(&self, bytes: Bytes) -> bool {
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

    /// Packets queued but not yet written on the bulk channel.
    pub fn backlog(&self) -> usize {
        self.tx.max_capacity().saturating_sub(self.tx.capacity())
    }

    /// Same, for the critical channel.
    pub fn backlog_prio(&self) -> usize {
        self.tx_prio
            .max_capacity()
            .saturating_sub(self.tx_prio.capacity())
    }
}

/// A client's char-stage session waiting on 0x3830.
pub struct PendingSel {
    pub client_tx: mpsc::Sender<Vec<u8>>,
    pub account_id: u32,
    pub char_id: u32,
    pub login_id1: u32,
    pub login_id2: u32,
    /// Real client IP; decides whether the 0x0071 reply carries the
    /// map's advertised address or the LAN override (lan_subnet).
    pub client_ip: u32,
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

/// A cached character (mirrors tmwa's in-memory char_db). `data`
/// is shared with queued `DbJob::SaveChar`s (and `load_char`
/// returns), so mutation goes through `Arc::make_mut` copy-on-write.
pub struct CharRecord {
    pub key: crate::proto::CharKey,
    pub data: std::sync::Arc<CharData>,
}

/// Arrival bookkeeping for a `drain` in flight.
pub struct DrainTrack {
    /// Chars online on the drained server when the drain began.
    pub expected: HashSet<u32>,
    /// Of those, the ones that have since authenticated (0x2afc) on
    /// a different map server.
    pub arrived: HashSet<u32>,
}

pub struct State {
    pub cfg: Config,
    pub db: std::sync::Arc<crate::db::Db>,
    /// account_id -> pending auth entry (a new login replaces the
    /// account's previous entry; entries expire after AUTH_TTL).
    pub auth: Mutex<SweptMap<AuthEntry>>,
    /// Keyed by account_id: a 0x2afc already answered (or being
    /// answered) with 0x2afd. The map re-pushes pending auth
    /// requests when the link flaps; the reservation lets the
    /// repeat get the same reply instead of a 0x2afe. Expires
    /// after SERVED_TTL.
    pub served_auth: Mutex<SweptMap<ServedAuth>>,
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
    /// char_id whose newest cached CharData is not known to be in
    /// SQLite: its 0x2b01 was dropped at `db_jobs_limit` or lost in
    /// a failed batch. `db_writer` re-queues the save from `chars`
    /// once the backlog drains below `DB_JOBS_REPLAY_LOW`; a fresh
    /// 0x2b01 that gets queued meanwhile supersedes the mark.
    pub save_dirty: Mutex<HashSet<u32>>,
    /// account_id to (map slot, newest items) of a 0x3011 storage
    /// save dropped at `db_jobs_limit`. The payload is kept because
    /// storage contents are not otherwise cached in the gate.
    /// Replayed like `save_dirty`.
    pub storage_dirty: Mutex<HashMap<i64, DirtyStorage>>,
    /// Map server slots; None = free.
    pub map_servers: Mutex<Vec<Option<MapHandle>>>,
    /// Secondary index over `map_servers` for `map_for`: map name
    /// to the lowest non-draining slot serving it, plus the
    /// fallback target. Rebuilt whenever a link's map list or
    /// draining flag changes (`rebuild_map_index`).
    map_index: Mutex<MapIndex>,
    /// char_id -> map server slot (online in game).
    pub online: Mutex<HashMap<u32, usize>>,
    /// Character cache.
    pub chars: Mutex<HashMap<u32, CharRecord>>,
    /// account_id to char ids of cached records. Secondary index
    /// over `chars`, kept in lockstep at every insert/remove
    /// (`cache_put`/`cache_remove`) so per-account scans don't walk
    /// the whole cache.
    pub chars_by_account: Mutex<HashMap<u32, Vec<u32>>>,
    /// name -> char_id
    pub char_names: Mutex<HashMap<String, u32>>,
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
    /// FIFO queue feeding the serialized DB writer
    /// (`dbq::db_writer`). `pub(crate)` for the `dbq` impl block;
    /// every enqueue must still go through `push_db_job`, which
    /// keeps `db_jobs_depth` (and the drop cap) accurate.
    pub(crate) db_jobs: mpsc::UnboundedSender<DbJob>,
    /// Receiver half of `db_jobs`; taken once by `dbq::db_writer`.
    pub(crate) db_jobs_rx: Mutex<Option<mpsc::UnboundedReceiver<DbJob>>>,
    /// Jobs queued but not yet applied (the channel is unbounded;
    /// this counts them so `status` can see the backlog and
    /// `queue_*` can cap it).
    pub db_jobs_depth: std::sync::atomic::AtomicUsize,
    /// Effective cap on `db_jobs_depth`; a field so tests can run
    /// the drop/replay paths without queueing 64k jobs.
    pub(crate) db_jobs_limit: usize,
    /// Jobs refused because the queue was over `db_jobs_limit` or a
    /// request was already pending (deduped reads).
    pub db_dropped: std::sync::atomic::AtomicU64,
    /// Coalescible requests in flight: (kind, account_id). A second
    /// identical request while one is queued is dropped; the
    /// reply is computed at apply time anyway, so it always
    /// carries the newest committed state.
    pub(crate) pending_db_req: std::sync::Arc<Mutex<std::collections::HashSet<(u8, i64)>>>,
    /// map slot -> arrival tracking while a `drain` is in flight.
    pub drains: Mutex<HashMap<usize, DrainTrack>>,
}

impl State {
    pub fn new(cfg: Config, db: std::sync::Arc<crate::db::Db>) -> State {
        let next_party_id = db.meta("next_party_id").ok().flatten().unwrap_or(0) as u64;
        let (db_jobs, db_jobs_rx) = mpsc::unbounded_channel();
        State {
            cfg,
            db,
            auth: Mutex::new(SweptMap::new()),
            served_auth: Mutex::new(SweptMap::new()),
            pending_sel: Mutex::new(HashMap::new()),
            online_auth: Mutex::new(HashMap::new()),
            transfer_pending: Mutex::new(HashMap::new()),
            saves_in_flight: Mutex::new(HashMap::new()),
            save_notify: tokio::sync::Notify::new(),
            save_dirty: Mutex::new(HashSet::new()),
            storage_dirty: Mutex::new(HashMap::new()),
            map_servers: Mutex::new(Vec::new()),
            map_index: Mutex::new(MapIndex::default()),
            online: Mutex::new(HashMap::new()),
            chars: Mutex::new(HashMap::new()),
            chars_by_account: Mutex::new(HashMap::new()),
            char_names: Mutex::new(HashMap::new()),
            gm: Mutex::new(HashMap::new()),
            gm_mtime: Mutex::new(None),
            parties: Mutex::new(HashMap::new()),
            next_party_id: AtomicU64::new(next_party_id),
            conn_count: AtomicU64::new(0),
            recent_logins: Mutex::new(HashMap::new()),
            online_notify: tokio::sync::Notify::new(),
            player_sessions: Mutex::new(HashMap::new()),
            rejoin_notify: Mutex::new(HashMap::new()),
            db_jobs,
            db_jobs_rx: Mutex::new(Some(db_jobs_rx)),
            db_jobs_depth: std::sync::atomic::AtomicUsize::new(0),
            db_jobs_limit: DB_JOBS_LIMIT,
            db_dropped: std::sync::atomic::AtomicU64::new(0),
            pending_db_req: std::sync::Arc::new(Mutex::new(std::collections::HashSet::new())),
            drains: Mutex::new(HashMap::new()),
        }
    }

    /// Mark a map server draining / clear the flag. Draining
    /// changes `map_for`'s answers, so the name index is rebuilt.
    pub fn map_set_draining(&self, id: usize, draining: bool) {
        {
            let mut ms = self.map_servers.lock().unwrap();
            if let Some(Some(h)) = ms.get_mut(id) {
                h.draining = draining;
            }
        }
        self.rebuild_map_index();
    }

    /// Set the map list a link serves (0x2afa) and rebuild the
    /// `map_for` name index.
    pub fn map_set_maps(&self, id: usize, maps: Vec<String>) {
        {
            let mut ms = self.map_servers.lock().unwrap();
            if let Some(Some(h)) = ms.get_mut(id) {
                h.maps = maps;
            }
        }
        self.rebuild_map_index();
    }

    /// Rebuild `map_index` from the live slots: every map name of
    /// a non-draining server maps to its (lowest) slot, and the
    /// fallback is the lowest non-draining server with maps.
    fn rebuild_map_index(&self) {
        let ms = self.map_servers.lock().unwrap();
        let mut idx = MapIndex::default();
        for (i, slot) in ms.iter().enumerate() {
            let Some(h) = slot else { continue };
            if h.draining || h.maps.is_empty() {
                continue;
            }
            if idx.fallback.is_none() {
                idx.fallback = Some((i, h.maps[0].clone()));
            }
            for m in &h.maps {
                idx.by_name.entry(m.clone()).or_insert(i);
            }
        }
        drop(ms);
        *self.map_index.lock().unwrap() = idx;
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
        a.sweep(Instant::now(), AUTH_TTL, |e| e.created);
        a.map.insert(e.account_id, e);
    }

    /// Remove and return the account's pending auth entry when it
    /// is at `stage` (the expected delflag), still fresh, and `pred`
    /// accepts it.
    pub fn take_auth<F: FnMut(&AuthEntry) -> bool>(
        &self,
        account_id: u32,
        stage: u8,
        mut pred: F,
    ) -> Option<AuthEntry> {
        let mut a = self.auth.lock().unwrap();
        let now = Instant::now();
        a.sweep(now, AUTH_TTL, |e| e.created);
        let e = a.map.get(&account_id)?;
        if e.delflag != stage || !e.fresh(now) || !pred(e) {
            return None;
        }
        a.map.remove(&account_id)
    }

    /// Mark upstream_ip on a matching entry (relay learned the
    /// address tmwa-map will see).
    pub fn set_auth_upstream_ip(&self, account_id: u32, char_id: u32, login_id1: u32, ip: u32) {
        let mut a = self.auth.lock().unwrap();
        if let Some(e) = a.map.get_mut(&account_id) {
            if e.delflag == DELFLAG_MAP && e.char_id == char_id && e.login_id1 == login_id1 {
                e.upstream_ip = Some(ip);
            }
        }
    }

    /// Consume a stage-3 entry (0x2afc) and open a re-serve
    /// reservation keyed on the serving map's registered client
    /// address (`map_ip`/`map_port`). The returned sender resolves
    /// the reservation: `ServedReply::Ready` with the 0x2afd bytes
    /// on success, `ServedReply::Failed` when the serve aborts. The
    /// pending take and the reservation happen while the auth lock
    /// is held, so a flap-repeated request can never observe a
    /// missing entry between the two.
    pub fn take_map_auth(
        &self,
        req: MapAuthReq,
        map_ip: u32,
        map_port: u16,
    ) -> Option<(AuthEntry, tokio::sync::watch::Sender<ServedReply>)> {
        let mut a = self.auth.lock().unwrap();
        let now = Instant::now();
        a.sweep(now, AUTH_TTL, |e| e.created);
        let e = a.map.get(&req.account_id)?;
        if !e.fresh(now) || !req.matches(e) {
            return None;
        }
        let e = a.map.remove(&req.account_id)?;
        let (tx, _rx) = tokio::sync::watch::channel(ServedReply::Pending);
        let mut s = self.served_auth.lock().unwrap();
        s.sweep(now, SERVED_TTL, |r| r.served);
        s.map.insert(
            req.account_id,
            ServedAuth {
                account_id: req.account_id,
                char_id: req.char_id,
                login_id1: req.login_id1,
                login_id2: req.login_id2,
                ip: e.ip,
                upstream_ip: e.upstream_ip,
                map_ip,
                map_port,
                reply: tx.clone(),
                served: now,
            },
        );
        Some((e, tx))
    }

    /// Match a recently served auth (a repeated 0x2afc after a
    /// link flap). Same tuple check as `take_map_auth`, plus the
    /// requester's registered client address must be the map the
    /// reply went to. On a hit, returns a receiver that resolves
    /// to the served reply (Pending means the first request is
    /// still building its answer) and the auth material to refresh
    /// online-auth bookkeeping with (`server` set to `map_id`).
    pub fn served_map_auth(
        &self,
        req: MapAuthReq,
        map_id: usize,
        map_ip: u32,
        map_port: u16,
    ) -> Option<(tokio::sync::watch::Receiver<ServedReply>, OnlineAuth)> {
        let mut s = self.served_auth.lock().unwrap();
        let now = Instant::now();
        s.sweep(now, SERVED_TTL, |r| r.served);
        let r = s.map.get(&req.account_id)?;
        if r.fresh(now) && req.matches_served(r) && r.map_ip == map_ip && r.map_port == map_port {
            let auth = OnlineAuth {
                account_id: r.account_id,
                char_id: r.char_id,
                login_id1: r.login_id1,
                login_id2: r.login_id2,
                ip: r.ip,
                server: map_id,
            };
            Some((r.reply.subscribe(), auth))
        } else {
            None
        }
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

    /// Wait until no 0x2b01 save for `char_id` is in flight and no
    /// dropped write for the char (or its account's storage) is
    /// still owed a replay. Returns false on timeout.
    pub async fn wait_saves(&self, char_id: u32, account_id: i64, dur: Duration) -> bool {
        let deadline = Instant::now() + dur;
        loop {
            // register the waiter before checking the count, so a
            // commit landing in between can't be missed
            let notified = self.save_notify.notified();
            tokio::pin!(notified);
            notified.as_mut().enable();
            // read all three under one section, in this order: a
            // pending write moves between the in-flight count and
            // the dirty sets (drop marks before save_done, replay
            // counts before unmarking), so they must be observed
            // together or a write could slip between the checks
            {
                let s = self.saves_in_flight.lock().unwrap();
                let d = self.save_dirty.lock().unwrap();
                let sd = self.storage_dirty.lock().unwrap();
                if s.get(&char_id).copied().unwrap_or(0) == 0
                    && !d.contains(&char_id)
                    && !sd.contains_key(&account_id)
                {
                    return true;
                }
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
        self.take_auth(account_id, DELFLAG_CHAR, |e| {
            e.login_id1 == id1 && e.login_id2 == id2 && e.ip == ip
        })
    }

    // ---- map servers ----

    /// Allocate a map server slot; returns (slot index, kill
    /// notify). `kill` fires when the link must die. `tx` is the
    /// bulk queue, `tx_prio` the critical queue (both into the same
    /// socket writer, which drains prio first).
    pub fn map_register(
        &self,
        tx: mpsc::Sender<Bytes>,
        tx_prio: mpsc::Sender<Bytes>,
        ip: u32,
        port: u16,
    ) -> (usize, std::sync::Arc<tokio::sync::Notify>) {
        let kill = std::sync::Arc::new(tokio::sync::Notify::new());
        let new = || MapHandle {
            id: 0,
            tx: tx.clone(),
            tx_prio: tx_prio.clone(),
            ip,
            port,
            maps: vec![],
            users: 0,
            draining: false,
            shutting_down: false,
            kill: kill.clone(),
        };
        let mut ms = self.map_servers.lock().unwrap();
        for (i, slot) in ms.iter_mut().enumerate() {
            if slot.is_none() {
                let mut h = new();
                h.id = i;
                *slot = Some(h);
                return (i, kill);
            }
        }
        let id = ms.len();
        let mut h = new();
        h.id = id;
        ms.push(Some(h));
        (id, kill)
    }

    /// The link is wedged beyond recovery (a critical reply could
    /// not be queued): ask its session to die so the map
    /// reconnects with a clean slate. The player side is handled
    /// the same as a link drop.
    ///
    /// `tx` pins the target to a link, not a slot: the kill fires
    /// only while `id` still holds the link whose writer queue
    /// failed. Slots are reused on re-register, so a `send_must`
    /// that armed its wait on a dead link (or runs late from a
    /// dead link's spawned task) must not kill the fresh link
    /// that took its place.
    pub fn map_kill(&self, id: usize, tx: &mpsc::Sender<Bytes>) {
        let ms = self.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get(id)
            && (h.tx_prio.same_channel(tx) || h.tx.same_channel(tx))
        {
            h.kill.notify_one();
        }
    }

    /// The critical-reply queue for map `id` (0x2afd/0x3810/0x382a
    /// and friends). `send_must` and request replies go here.
    pub fn map_prio_tx(&self, id: usize) -> Option<mpsc::Sender<Bytes>> {
        let ms = self.map_servers.lock().unwrap();
        ms.get(id)
            .and_then(|s| s.as_ref())
            .map(|h| h.tx_prio.clone())
    }

    /// Map slot registered with client address (ip, port), for
    /// resolving the destination server a 0x2b05 names.
    pub fn map_by_addr(&self, ip: u32, port: u16) -> Option<usize> {
        let ms = self.map_servers.lock().unwrap();
        ms.iter().enumerate().find_map(|(i, s)| {
            s.as_ref()
                .and_then(|h| (h.ip == ip && h.port == port).then_some(i))
        })
    }

    pub fn map_unregister(&self, id: usize) {
        {
            let mut ms = self.map_servers.lock().unwrap();
            if let Some(slot) = ms.get_mut(id) {
                *slot = None;
            }
        }
        self.rebuild_map_index();
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
            super::client::send_pending_sel(self, ps);
        }
    }

    /// Remove `mid` from a pending select's waiting set (its 0x3830
    /// arrived, or its link went away mid-select). When nothing
    /// else is awaited the entry is popped and returned so the
    /// caller can hand it to `client::send_pending_sel`.
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

    /// Send to every connected map server.
    pub fn map_broadcast(&self, bytes: &[u8]) {
        self.map_broadcast_except(NO_MAP, bytes)
    }

    /// Send to every connected map server except `skip` (which may
    /// need the same bytes on its priority queue instead: ordering
    /// vs a following send_must). The payload is shared per
    /// recipient through `Bytes`, so this is refcount clones, not
    /// one copy per map.
    pub fn map_broadcast_except(&self, skip: usize, bytes: &[u8]) {
        let bytes = Bytes::copy_from_slice(bytes);
        let ms = self.map_servers.lock().unwrap();
        for (i, slot) in ms.iter().enumerate() {
            if i == skip {
                continue;
            }
            if let Some(h) = slot {
                h.send(bytes.clone());
            }
        }
    }

    /// Send to one map server by slot.
    pub fn map_send(&self, id: usize, bytes: impl Into<Bytes>) -> bool {
        let ms = self.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get(id) {
            h.send(bytes.into())
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
    pub fn map_senders(&self) -> Vec<(usize, mpsc::Sender<Bytes>)> {
        let ms = self.map_servers.lock().unwrap();
        ms.iter()
            .enumerate()
            .filter_map(|(i, s)| s.as_ref().map(|h| (i, h.tx.clone())))
            .collect()
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

    // ---- drain tracking ----

    /// Chars currently recorded as online on map server `id`.
    pub fn online_on(&self, id: usize) -> HashSet<u32> {
        self.online
            .lock()
            .unwrap()
            .iter()
            .filter_map(|(c, m)| (*m == id).then_some(*c))
            .collect()
    }

    /// Begin tracking evacuee arrivals for a drain of `id`.
    pub fn drain_track_begin(&self, id: usize, expected: HashSet<u32>) {
        self.drains.lock().unwrap().insert(
            id,
            DrainTrack {
                expected,
                arrived: HashSet::new(),
            },
        );
    }

    /// A transfer-marked char authenticated on map `to`: count it
    /// as an arrival for every drain that expected it elsewhere.
    pub fn drain_track_arrive(&self, to: usize, char_id: u32) {
        let mut d = self.drains.lock().unwrap();
        for (target, t) in d.iter_mut() {
            if *target != to && t.expected.contains(&char_id) {
                t.arrived.insert(char_id);
            }
        }
    }

    /// End tracking for `id`; returns (expected, arrived, the
    /// expected set) so the caller can count who is still there.
    pub fn drain_track_end(&self, id: usize) -> Option<DrainTrack> {
        self.drains.lock().unwrap().remove(&id)
    }

    /// Map server slot that serves `map`, or the fallback target
    /// (first non-draining slot with maps). On fallback, `rewritten`
    /// is set to the server's first map. Reads the index maintained
    /// by `rebuild_map_index` instead of scanning the slots.
    pub fn map_for(&self, map: &str) -> (Option<usize>, Option<String>) {
        let idx = self.map_index.lock().unwrap();
        if let Some(&i) = idx.by_name.get(map) {
            return (Some(i), None);
        }
        match &idx.fallback {
            Some((i, first)) => (Some(*i), Some(first.clone())),
            None => (None, None),
        }
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

    /// Load a character into the cache (from DB if needed). The
    /// returned record shares its `data` Arc with the cache entry;
    /// a cache hit is a refcount bump, not a copy.
    pub async fn load_char(&self, char_id: u32) -> Option<CharRecord> {
        if let Some(c) = self.chars.lock().unwrap().get(&char_id) {
            return Some(CharRecord {
                key: c.key,
                data: c.data.clone(),
            });
        }
        let (key, mut data) = self
            .db
            .blocking(move |db| db.load_character(char_id as i64))
            .await
            .ok()?;
        self.fix_party_id(&key, &mut data);
        let data = std::sync::Arc::new(data);
        self.cache_put(
            char_id,
            CharRecord {
                key,
                data: data.clone(),
            },
        );
        Some(CharRecord { key, data })
    }

    /// Insert/replace a cached char record and maintain the
    /// account index (`chars_by_account`).
    pub fn cache_put(&self, char_id: u32, rec: CharRecord) {
        let aid = rec.key.account_id.0;
        self.chars.lock().unwrap().insert(char_id, rec);
        let mut by = self.chars_by_account.lock().unwrap();
        let v = by.entry(aid).or_default();
        if !v.contains(&char_id) {
            v.push(char_id);
        }
    }

    /// Remove a cached char record and its account-index entry.
    /// Returns the removed record, if any.
    pub fn cache_remove(&self, char_id: u32) -> Option<CharRecord> {
        let rec = self.chars.lock().unwrap().remove(&char_id)?;
        let mut by = self.chars_by_account.lock().unwrap();
        if let Some(v) = by.get_mut(&rec.key.account_id.0) {
            v.retain(|&c| c != char_id);
            if v.is_empty() {
                by.remove(&rec.key.account_id.0);
            }
        }
        Some(rec)
    }

    /// Char ids of the account's cached records (empty when none).
    pub fn chars_of_account(&self, account_id: u32) -> Vec<u32> {
        self.chars_by_account
            .lock()
            .unwrap()
            .get(&account_id)
            .cloned()
            .unwrap_or_default()
    }

    /// Replace the account_reg (scope 1) or account_reg2 (scope 2)
    /// snapshot in every cached CharData of the account (tmwa's
    /// set_account_reg / set_account_reg2). Copy-on-write: a record
    /// shared with a queued save keeps its old snapshot there.
    pub fn cache_account_regs(&self, account_id: u32, regs: &[(String, i64)], scope: i64) {
        use crate::proto::types::FixedStr;
        let cids = self.chars_of_account(account_id);
        if cids.is_empty() {
            return;
        }
        let mut chars = self.chars.lock().unwrap();
        for cid in cids {
            let Some(c) = chars.get_mut(&cid) else { continue };
            let d = std::sync::Arc::make_mut(&mut c.data);
            let (num, arr) = if scope == 2 {
                (&mut d.account_reg2_num, &mut d.account_reg2)
            } else {
                (&mut d.account_reg_num, &mut d.account_reg)
            };
            *num = regs.len().min(arr.len()) as i32;
            for (i, (name, v)) in regs.iter().enumerate().take(arr.len()) {
                arr[i] = crate::proto::GlobalReg {
                    str: FixedStr::<32>::from_str_truncate(name),
                    value: *v as i32,
                };
            }
        }
    }

    /// Mark `cid` online on `map_id` and wake the online-file
    /// writer and drain waiters.
    pub fn mark_online(&self, cid: u32, map_id: usize) {
        self.online.lock().unwrap().insert(cid, map_id);
        self.online_notify.notify_waiters();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::Config;
    use std::net::Ipv4Addr;
    use std::sync::Arc;

    fn test_state(lan_subnet: &str, lan_map_ip: Ipv4Addr) -> State {
        let mut cfg = Config::default();
        cfg.lan.lan_subnet = lan_subnet.parse().unwrap();
        cfg.lan.lan_map_ip = lan_map_ip;
        State::new(cfg, Arc::new(crate::db::Db::open_memory().unwrap()))
    }

    #[test]
    fn drain_track_counts_only_other_servers() {
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        st.drain_track_begin(0, [7, 8, 9].into_iter().collect());
        // 7 and 8 land on server 1; 8 again on server 2 (a second
        // arrival must not double count... one char, one arrival)
        st.drain_track_arrive(1, 7);
        st.drain_track_arrive(1, 8);
        st.drain_track_arrive(2, 8);
        // a landing back on the drained server doesn't count
        st.drain_track_arrive(0, 9);
        // nor does an unexpected char
        st.drain_track_arrive(1, 999);
        let t = st.drain_track_end(0).unwrap();
        assert_eq!(t.expected.len(), 3);
        assert_eq!(t.arrived.len(), 2);
    }

    fn push_stage3(
        st: &State,
        account_id: u32,
        char_id: u32,
        login_id1: u32,
        login_id2: u32,
        client_ip: [u8; 4],
    ) {
        st.push_auth(AuthEntry {
            account_id,
            char_id,
            login_id1,
            login_id2,
            ip: u32::from_le_bytes(client_ip),
            client_version: 0,
            map_id: None,
            upstream_ip: None,
            delflag: DELFLAG_MAP,
            created: Instant::now(),
        });
    }

    fn auth_req(account_id: u32, char_id: u32, login_id1: u32, login_id2: u32) -> MapAuthReq {
        MapAuthReq {
            account_id,
            char_id,
            login_id1,
            login_id2,
            ip: u32::from_le_bytes([1, 2, 3, 4]),
        }
    }

    #[test]
    fn take_map_auth_reserves_for_same_server() {
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        let mip = u32::from_le_bytes([203, 0, 113, 7]);
        push_stage3(&st, 10, 100, 111, 222, [1, 2, 3, 4]);
        let req = auth_req(10, 100, 111, 222);
        let (e, tx) = st.take_map_auth(req, mip, 5121).unwrap();
        assert_eq!(e.char_id, 100);
        // the pending entry is consumed
        assert!(st.take_map_auth(req, mip, 5121).is_none());
        // a repeat matches only on the served map's address
        assert!(st.served_map_auth(req, 1, mip, 5121).is_some());
        assert!(st.served_map_auth(req, 1, mip, 5122).is_none());
        let other_ip = u32::from_le_bytes([203, 0, 113, 9]);
        assert!(st.served_map_auth(req, 1, other_ip, 5121).is_none());
        // login_id2 0 is a wildcard, like the pending match
        assert!(
            st.served_map_auth(auth_req(10, 100, 111, 0), 1, mip, 5121)
                .is_some()
        );
        // wrong credentials never match
        assert!(
            st.served_map_auth(auth_req(10, 100, 999, 222), 1, mip, 5121)
                .is_none()
        );
        assert!(
            st.served_map_auth(auth_req(10, 101, 111, 222), 1, mip, 5121)
                .is_none()
        );
        // resolving the reservation hands out the exact reply and
        // the auth material for bookkeeping
        let (rx, auth) = st.served_map_auth(req, 1, mip, 5121).unwrap();
        assert_eq!(auth.server, 1);
        assert_eq!(auth.login_id2, 222);
        tx.send_replace(ServedReply::Ready(b"reply".to_vec()));
        assert!(matches!(&*rx.borrow(), ServedReply::Ready(b) if b.as_slice() == b"reply"));
    }

    #[tokio::test]
    async fn served_map_auth_pending_resolves() {
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        let mip = u32::from_le_bytes([203, 0, 113, 7]);
        push_stage3(&st, 11, 100, 111, 222, [1, 2, 3, 4]);
        let req = auth_req(11, 100, 111, 222);
        let (_e, tx) = st.take_map_auth(req, mip, 5121).unwrap();
        // subscribed while the first serve is still building
        let (mut rx, _auth) = st.served_map_auth(req, 2, mip, 5121).unwrap();
        tx.send_replace(ServedReply::Ready(b"r".to_vec()));
        let r = rx
            .wait_for(|v| !matches!(v, ServedReply::Pending))
            .await
            .unwrap();
        assert!(matches!(&*r, ServedReply::Ready(b) if b.as_slice() == b"r"));
        drop(r);
        // a Failed resolution wakes waiters to reject
        push_stage3(&st, 12, 100, 111, 222, [1, 2, 3, 4]);
        let req = auth_req(12, 100, 111, 222);
        let (_e, tx) = st.take_map_auth(req, mip, 5121).unwrap();
        let (mut rx, _auth) = st.served_map_auth(req, 2, mip, 5121).unwrap();
        tx.send_replace(ServedReply::Failed);
        let r = rx
            .wait_for(|v| !matches!(v, ServedReply::Pending))
            .await
            .unwrap();
        assert!(matches!(&*r, ServedReply::Failed));
    }
}
