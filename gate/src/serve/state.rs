// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! Shared runtime state for `tmwa-gate serve`.

use std::collections::{HashMap, HashSet};
use std::sync::Mutex;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

use tokio::sync::mpsc;

use crate::config::Config;
use crate::proto::{CharData, CharKey, PartyMost};

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

/// Drop expired entries from the auth table; runs before each
/// mutation/lookup so stale handoffs cannot pile up.
fn prune_auth(a: &mut HashMap<u32, AuthEntry>, now: Instant) {
    a.retain(|_, e| now.duration_since(e.created) < AUTH_TTL);
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

/// Drop expired reservations from the served-auth table.
fn prune_served(s: &mut HashMap<u32, ServedAuth>, now: Instant) {
    s.retain(|_, r| now.duration_since(r.served) < SERVED_TTL);
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

/// One connected tmwa-map session.
pub struct MapHandle {
    pub id: usize,
    /// Bulk queue into the map session writer task: broadcasts,
    /// notifications, pre-auth floods. Dropped-first under load.
    pub tx: mpsc::Sender<Vec<u8>>,
    /// Critical queue, drained before `tx`: request replies the map
    /// is blocked waiting on (0x2afd/0x3810/0x2b06/0x382a/...).
    /// `send_must` targets this so bulk floods can't starve answers.
    pub tx_prio: mpsc::Sender<Vec<u8>>,
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

/// Cap on queued map-link DB jobs. Beyond it the queue drops new
/// jobs rather than become an unbounded memory backlog.
const DB_JOBS_LIMIT: usize = 65536;

/// Queue depth below which `db_writer` re-queues writes that were
/// dropped at the cap: the burst has drained, so the memory for
/// them is affordable again.
const DB_JOBS_REPLAY_LOW: usize = 8192;

/// Cap on the sets of chars/accounts owed a write replay (distinct
/// identities, not jobs). Far past any real player count; it exists
/// so a pathological flood cannot grow even the marks without bound.
const SAVE_DIRTY_LIMIT: usize = 16384;

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

/// A cached character (mirrors tmwa's in-memory char_db).
pub struct CharRecord {
    pub key: crate::proto::CharKey,
    pub data: crate::proto::CharData,
}

// ---- serialized DB writer ----

/// Where a queued DB op's reply packet goes once the batch commits.
pub enum LinkReply {
    None,
    /// send_must to this map server's link.
    Map(usize),
    /// map_broadcast to all map servers.
    Broadcast,
}

/// Outcome of `push_db_job`: why a job did or did not get queued.
enum JobPush {
    /// The job is in the queue.
    Queued,
    /// The queue was over `db_jobs_limit`; the job was dropped and
    /// counted in `db_dropped`.
    Full,
    /// The writer task is gone; the job was dropped.
    Closed,
}

/// Kept payload of a dropped 0x3011 storage save: the requesting
/// map's slot plus the newest (item_id, amount, equip) list.
type DirtyStorage = (usize, Vec<(i64, i64, i64)>);

/// The 0x3011 job: replace the account's storage inside the batch
/// transaction, ack 0x3811 to the requesting map after commit.
fn storage_save_op(
    map_id: usize,
    account_id: crate::proto::AccountId,
    items: Vec<(i64, i64, i64)>,
) -> DbJob {
    let aid = account_id.0 as i64;
    DbJob::Op(Box::new(move |conn| {
        let _ = crate::db::save_storage_conn(conn, aid, &items);
        let mut ack = crate::proto::P3811::default();
        ack.account_id = account_id;
        ack.unknown = 0;
        DbOpResult::reply(map_id, enc(move |v| ack.encode(v)))
    }))
}

/// What a queued DB op produced inside the batch transaction.
pub struct DbOpResult {
    pub reply: LinkReply,
    /// Encoded reply packet; sent only after the batch commits.
    pub bytes: Vec<u8>,
    /// Post-commit bookkeeping (cache updates, oneshot results).
    /// Runs only when the batch committed; on rollback it is
    /// dropped, releasing any captured oneshot sender so the
    /// requester observes failure.
    pub after: Option<Box<dyn FnOnce() + Send>>,
}

impl DbOpResult {
    /// No reply, no post-commit action.
    pub fn none() -> DbOpResult {
        DbOpResult {
            reply: LinkReply::None,
            bytes: Vec::new(),
            after: None,
        }
    }

    /// `bytes` to `map_id` after commit.
    pub fn reply(map_id: usize, bytes: Vec<u8>) -> DbOpResult {
        DbOpResult {
            reply: LinkReply::Map(map_id),
            bytes,
            after: None,
        }
    }
}

/// One unit of work for the serialized DB writer. Every map-link
/// packet that touches SQLite goes through this queue so a slow
/// database cannot stall the per-link read loop; consecutive jobs
/// are committed in one transaction.
pub enum DbJob {
    /// 0x2b01 character save.
    SaveChar {
        char_id: u32,
        key: Box<CharKey>,
        data: Box<CharData>,
    },
    /// Serialized op; runs inside the batch transaction on the
    /// shared `&Connection` (a `Transaction` derefs to it), so
    /// multi-statement jobs are atomic with the rest of the batch.
    /// Must not open a nested transaction.
    Op(Box<dyn FnOnce(&rusqlite::Connection) -> DbOpResult + Send>),
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
    pub auth: Mutex<HashMap<u32, AuthEntry>>,
    /// Keyed by account_id: a 0x2afc already answered (or being
    /// answered) with 0x2afd. The map re-pushes pending auth
    /// requests when the link flaps; the reservation lets the
    /// repeat get the same reply instead of a 0x2afe. Expires
    /// after SERVED_TTL.
    pub served_auth: Mutex<HashMap<u32, ServedAuth>>,
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
    /// FIFO queue feeding the serialized DB writer (`db_writer`).
    /// Private so every enqueue goes through `push_db_job`, which
    /// keeps `db_jobs_depth` (and the drop cap) accurate.
    db_jobs: mpsc::UnboundedSender<DbJob>,
    /// Receiver half of `db_jobs`; taken once by `db_writer`.
    db_jobs_rx: Mutex<Option<mpsc::UnboundedReceiver<DbJob>>>,
    /// Jobs queued but not yet applied (the channel is unbounded;
    /// this counts them so `status` can see the backlog and
    /// `queue_*` can cap it).
    pub db_jobs_depth: std::sync::atomic::AtomicUsize,
    /// Effective cap on `db_jobs_depth`; a field so tests can run
    /// the drop/replay paths without queueing 64k jobs.
    db_jobs_limit: usize,
    /// Jobs refused because the queue was over `db_jobs_limit` or a
    /// request was already pending (deduped reads).
    pub db_dropped: std::sync::atomic::AtomicU64,
    /// Coalescible requests in flight: (kind, account_id). A second
    /// identical request while one is queued is dropped — the
    /// reply is computed at apply time anyway, so it always
    /// carries the newest committed state.
    pending_db_req: std::sync::Arc<Mutex<std::collections::HashSet<(u8, i64)>>>,
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
            auth: Mutex::new(HashMap::new()),
            served_auth: Mutex::new(HashMap::new()),
            pending_sel: Mutex::new(HashMap::new()),
            online_auth: Mutex::new(HashMap::new()),
            transfer_pending: Mutex::new(HashMap::new()),
            saves_in_flight: Mutex::new(HashMap::new()),
            save_notify: tokio::sync::Notify::new(),
            save_dirty: Mutex::new(HashSet::new()),
            storage_dirty: Mutex::new(HashMap::new()),
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
            db_jobs,
            db_jobs_rx: Mutex::new(Some(db_jobs_rx)),
            db_jobs_depth: std::sync::atomic::AtomicUsize::new(0),
            db_jobs_limit: DB_JOBS_LIMIT,
            db_dropped: std::sync::atomic::AtomicU64::new(0),
            pending_db_req: std::sync::Arc::new(Mutex::new(std::collections::HashSet::new())),
            drains: Mutex::new(HashMap::new()),
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
        prune_auth(&mut a, Instant::now());
        a.insert(e.account_id, e);
    }

    /// Remove and return a pending auth entry. `stage` is the
    /// expected delflag.
    pub fn take_auth<F: FnMut(&AuthEntry) -> bool>(
        &self,
        stage: u8,
        mut pred: F,
    ) -> Option<AuthEntry> {
        let mut a = self.auth.lock().unwrap();
        prune_auth(&mut a, Instant::now());
        let key = a
            .iter()
            .find(|(_, e)| e.delflag == stage && pred(e))
            .map(|(k, _)| *k)?;
        a.remove(&key)
    }

    /// Mark upstream_ip on a matching entry (relay learned the
    /// address tmwa-map will see).
    pub fn set_auth_upstream_ip(&self, account_id: u32, char_id: u32, login_id1: u32, ip: u32) {
        let mut a = self.auth.lock().unwrap();
        if let Some(e) = a.get_mut(&account_id) {
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
        prune_auth(&mut a, now);
        let key = a.iter().find(|(_, e)| req.matches(e)).map(|(k, _)| *k)?;
        let e = a.remove(&key)?;
        let (tx, _rx) = tokio::sync::watch::channel(ServedReply::Pending);
        let mut s = self.served_auth.lock().unwrap();
        prune_served(&mut s, now);
        s.insert(
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
        prune_served(&mut s, Instant::now());
        let r = s.get(&req.account_id)?;
        if req.matches_served(r) && r.map_ip == map_ip && r.map_port == map_port {
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
        self.take_auth(2, |e| {
            e.account_id == account_id && e.login_id1 == id1 && e.login_id2 == id2 && e.ip == ip
        })
    }

    // ---- map servers ----

    /// Allocate a map server slot; returns (slot index, kill
    /// notify). `kill` fires when the link must die. `tx` is the
    /// bulk queue, `tx_prio` the critical queue (both into the same
    /// socket writer, which drains prio first).
    pub fn map_register(
        &self,
        tx: mpsc::Sender<Vec<u8>>,
        tx_prio: mpsc::Sender<Vec<u8>>,
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
    pub fn map_kill(&self, id: usize, tx: &mpsc::Sender<Vec<u8>>) {
        let ms = self.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get(id)
            && (h.tx_prio.same_channel(tx) || h.tx.same_channel(tx))
        {
            h.kill.notify_one();
        }
    }

    /// The critical-reply queue for map `id` (0x2afd/0x3810/0x382a
    /// and friends). `send_must` and request replies go here.
    pub fn map_prio_tx(&self, id: usize) -> Option<mpsc::Sender<Vec<u8>>> {
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
                // tmwa lan_support.conf (char.cpp lan_ip_check): a
                // client inside lan_subnet is pointed at lan_map_ip
                // instead of the map's advertised address; the
                // registered port stays.
                let client = std::net::Ipv4Addr::from(ps.client_ip.to_le_bytes());
                let ip = if self.cfg.lan.lan_subnet.covers(client) {
                    self.cfg.lan.lan_map_ip
                } else {
                    std::net::Ipv4Addr::from(ip.to_le_bytes())
                };
                let mut p = P0071::default();
                p.char_id = CharId(ps.char_id);
                p.map_name = FixedStr::<16>::try_from_str(&ps.map_name).unwrap_or_default();
                p.ip = Ip4Address(ip.octets());
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

    /// Send to every connected map server.
    pub fn map_broadcast(&self, bytes: &[u8]) {
        self.map_broadcast_except(NO_MAP, bytes)
    }

    /// Send to every connected map server except `skip` (which may
    /// need the same bytes on its priority queue instead: ordering
    /// vs a following send_must).
    pub fn map_broadcast_except(&self, skip: usize, bytes: &[u8]) {
        let ms = self.map_servers.lock().unwrap();
        for (i, slot) in ms.iter().enumerate() {
            if i == skip {
                continue;
            }
            if let Some(h) = slot {
                h.send(bytes.to_vec());
            }
        }
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

    // ---- serialized DB writer ----

    /// Queue a 0x2b01 character save. Marks the char as having an
    /// in-flight save BEFORE enqueueing, so a 0x2afc on another
    /// link can wait for it (`wait_saves`).
    pub fn queue_save(&self, char_id: u32, key: CharKey, data: CharData) {
        self.save_begin(char_id);
        let job = DbJob::SaveChar {
            char_id,
            key: Box::new(key),
            data: Box::new(data),
        };
        match self.push_db_job(job) {
            JobPush::Queued => {
                // a queued save carries the newest known state and
                // will commit: it supersedes an earlier drop for
                // this char. Clearing only after save_begin keeps
                // `wait_saves` covered by the in-flight count.
                self.save_dirty.lock().unwrap().remove(&char_id);
            }
            JobPush::Full => {
                // mark before save_done: pending state stays covered
                // (in-flight count or dirty mark) the whole time a
                // `wait_saves` caller might look. The newest state
                // is in `chars` already; `db_writer` re-queues it
                // once the backlog drains.
                self.mark_save_dirty(char_id);
                self.save_done(char_id);
            }
            JobPush::Closed => {
                // writer gone: no replay will come
                self.save_done(char_id);
            }
        }
    }

    /// Record that `char_id`'s newest cached CharData is not in
    /// SQLite: its save was dropped at the queue cap (or lost to a
    /// failed batch). `db_writer` replays it from `chars` once the
    /// backlog drains.
    fn mark_save_dirty(&self, char_id: u32) {
        let mut d = self.save_dirty.lock().unwrap();
        if d.len() < SAVE_DIRTY_LIMIT || d.contains(&char_id) {
            d.insert(char_id);
        }
    }

    /// Queue a 0x3011 storage save plus the 0x3811 ack for the
    /// requesting map. Over `db_jobs_limit` the newest payload is
    /// kept per account in `storage_dirty` and replayed once the
    /// backlog drains: a dropped storage save is a silent item
    /// loss, and the map only re-sends on the next autosave (or
    /// never, once the storage is closed).
    pub fn queue_storage_save(
        &self,
        map_id: usize,
        account_id: crate::proto::AccountId,
        items: Vec<(i64, i64, i64)>,
    ) {
        let aid = account_id.0 as i64;
        // the lock spans the push so the mark/unmark decision is
        // atomic with the enqueue: a marked payload is always the
        // newest one that is not queued
        let mut d = self.storage_dirty.lock().unwrap();
        match self.push_db_job(storage_save_op(map_id, account_id, items.clone())) {
            JobPush::Queued => {
                // a queued save carries the newest payload and will
                // commit: it supersedes an earlier drop
                d.remove(&aid);
                drop(d);
                self.save_notify.notify_waiters();
            }
            JobPush::Full => {
                if d.len() < SAVE_DIRTY_LIMIT || d.contains_key(&aid) {
                    d.insert(aid, (map_id, items));
                }
            }
            JobPush::Closed => {}
        }
    }

    /// Queue a serialized DB op. `f` runs inside the writer's batch
    /// transaction; the reply it produces is sent only after the
    /// batch commits.
    pub fn queue_db_op(
        &self,
        f: impl FnOnce(&rusqlite::Connection) -> DbOpResult + Send + 'static,
    ) {
        self.push_db_job(DbJob::Op(Box::new(f)));
    }

    /// Queue an idempotent read-reply op, coalescing duplicates:
    /// a second request for the same (kind, account) while one is
    /// still queued is dropped — the reply is computed at apply
    /// time, so it carries the newest committed state anyway. The
    /// flag clears when the job runs. Used for 0x3010/0x3005,
    /// which the map re-requests on reply timeout — under
    /// congestion each re-request was one more full storage reply.
    pub fn queue_dedup_op(
        &self,
        kind: u8,
        account_id: i64,
        f: impl FnOnce(&rusqlite::Connection) -> DbOpResult + Send + 'static,
    ) {
        {
            let mut p = self.pending_db_req.lock().unwrap();
            if !p.insert((kind, account_id)) {
                self.db_dropped
                    .fetch_add(1, std::sync::atomic::Ordering::Relaxed);
                return;
            }
        }
        let pending = self.pending_db_req.clone();
        let job = DbJob::Op(Box::new(move |conn| {
            pending.lock().unwrap().remove(&(kind, account_id));
            f(conn)
        }));
        if !matches!(self.push_db_job(job), JobPush::Queued) {
            self.pending_db_req
                .lock()
                .unwrap()
                .remove(&(kind, account_id));
        }
    }

    /// Enqueue a job, capped at `db_jobs_limit` queued. Over the
    /// limit the job is dropped and counted: the alternative is an
    /// unbounded memory backlog, which is what the round-2 load
    /// test measured at ~20 GB. Dropped saves are marked dirty and
    /// re-emitted by `replay_dropped` once the backlog drains; a
    /// dropped request is re-requested by the map on reply timeout.
    fn push_db_job(&self, job: DbJob) -> JobPush {
        let limit = self.db_jobs_limit;
        let depth = self
            .db_jobs_depth
            .fetch_add(1, std::sync::atomic::Ordering::Relaxed);
        if depth >= limit {
            self.db_jobs_depth
                .fetch_sub(1, std::sync::atomic::Ordering::Relaxed);
            let drops = self
                .db_dropped
                .fetch_add(1, std::sync::atomic::Ordering::Relaxed)
                + 1;
            if drops.is_power_of_two() || drops == 1 {
                tracing::warn!("db job queue over {limit}, dropping ({drops} total)");
            }
            return JobPush::Full;
        }
        if self.db_jobs.send(job).is_err() {
            self.db_jobs_depth
                .fetch_sub(1, std::sync::atomic::Ordering::Relaxed);
            return JobPush::Closed;
        }
        JobPush::Queued
    }

    /// Re-queue writes that were dropped at `db_jobs_limit`, at most
    /// `DB_JOBS_REPLAY_LOW - depth` jobs per pass. Runs on
    /// `db_writer` before each batch. Char saves replay the live
    /// `chars` cache, so they always carry the newest received
    /// state; storage saves replay the payload kept at drop time.
    fn replay_dropped(&self) {
        let depth = self
            .db_jobs_depth
            .load(std::sync::atomic::Ordering::Relaxed);
        if depth >= DB_JOBS_REPLAY_LOW {
            return;
        }
        let room = DB_JOBS_REPLAY_LOW - depth;
        let chars: Vec<u32> = {
            let d = self.save_dirty.lock().unwrap();
            d.iter().copied().take(room).collect()
        };
        let accounts: Vec<i64> = {
            let d = self.storage_dirty.lock().unwrap();
            d.keys().copied().take(room).collect()
        };
        if chars.is_empty() && accounts.is_empty() {
            return;
        }
        let mut n = 0usize;
        for cid in chars {
            let cur = self
                .chars
                .lock()
                .unwrap()
                .get(&cid)
                .map(|c| (c.key, c.data));
            let Some((key, data)) = cur else {
                // the char record is gone (deleted) or was never
                // cached: there is no newer state left to write
                self.save_dirty.lock().unwrap().remove(&cid);
                tracing::warn!(
                    "db_writer: dropped save for char {cid} has no cached state"
                );
                continue;
            };
            // count the replay in-flight before dropping the dirty
            // mark, so a `wait_saves` caller never observes a false
            // all-committed in between
            self.save_begin(cid);
            self.save_dirty.lock().unwrap().remove(&cid);
            let job = DbJob::SaveChar {
                char_id: cid,
                key: Box::new(key),
                data: Box::new(data),
            };
            match self.push_db_job(job) {
                JobPush::Queued => n += 1,
                _ => {
                    // refilled past the limit (or writer gone):
                    // keep the mark for the next pass
                    self.mark_save_dirty(cid);
                    self.save_done(cid);
                }
            }
        }
        for aid in accounts {
            let mut d = self.storage_dirty.lock().unwrap();
            let Some((mid, items)) = d.get(&aid).cloned() else {
                continue;
            };
            // the lock spans the push: a queued replay clears its
            // mark atomically, a failed one keeps it for next time
            if matches!(
                self.push_db_job(storage_save_op(
                    mid,
                    crate::proto::AccountId(aid as u32),
                    items
                )),
                JobPush::Queued
            ) {
                d.remove(&aid);
                n += 1;
            }
        }
        if n > 0 {
            tracing::info!("db_writer: re-queued {n} dropped saves");
        }
        // wake `wait_saves` callers: marks may have cleared without
        // a commit (uncached char) or moved to the in-flight count
        self.save_notify.notify_waiters();
    }

    /// Take the receiver half of the job queue. Called once by
    /// `serve::run` to start `db_writer`.
    pub fn take_db_jobs_rx(&self) -> Option<mpsc::UnboundedReceiver<DbJob>> {
        self.db_jobs_rx.lock().unwrap().take()
    }

    /// A oneshot that fires after every job queued so far has
    /// committed (or the writer is gone). Lets tests observe the
    /// queue draining.
    pub fn db_barrier(&self) -> tokio::sync::oneshot::Receiver<()> {
        let (tx, rx) = tokio::sync::oneshot::channel();
        self.queue_db_op(move |_| DbOpResult {
            reply: LinkReply::None,
            bytes: Vec::new(),
            after: Some(Box::new(move || {
                let _ = tx.send(());
            })),
        });
        rx
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
            });
        }
        let (key, mut data) = self
            .db
            .blocking(move |db| db.load_character(char_id as i64))
            .await
            .ok()?;
        self.fix_party_id(&key, &mut data);
        let mut chars = self.chars.lock().unwrap();
        let rec = CharRecord { key, data };
        chars.insert(char_id, CharRecord { key, data });
        Some(rec)
    }
}

/// Send raw bytes to a map link's *priority* writer queue when the
/// reply must not be dropped (auth answers, request results, save
/// acks). A full queue gets a bounded wait; a link that stays
/// wedged is killed, since dropping the reply would leave the map
/// waiting on an answer that never comes (map-side auth stalls for
/// minutes). The kill only lands if `map_id`'s slot still holds
/// the link `tx` belongs to (`map_kill` checks channel identity),
/// so a stale wait can't kill a fresh link that reused the slot.
/// Returns false when the bytes were not queued.
pub async fn send_must(st: &State, map_id: usize, tx: &mpsc::Sender<Vec<u8>>, v: Vec<u8>) -> bool {
    use tokio::sync::mpsc::error::TrySendError;
    match tx.try_send(v) {
        Ok(()) => true,
        Err(TrySendError::Full(v)) | Err(TrySendError::Closed(v)) => {
            match tokio::time::timeout(Duration::from_secs(30), tx.send(v)).await {
                Ok(Ok(())) => true,
                _ => {
                    tracing::warn!("map {map_id}: link wedged on a critical reply, dropping link");
                    st.map_kill(map_id, tx);
                    false
                }
            }
        }
    }
}

/// The serialized DB writer: drains `db_jobs` in FIFO order, one
/// task, so the per-link read loops never wait on SQLite. Each
/// drain batch commits in a single transaction — under a save
/// burst this is the difference between ~150 commits/s and one
/// commit per batch.
pub async fn db_writer(st: std::sync::Arc<State>) {
    let Some(mut rx) = st.take_db_jobs_rx() else {
        tracing::warn!("db_writer: job queue already taken");
        return;
    };
    const BATCH: usize = 512;
    loop {
        // backlog drained low: re-emit the writes that were dropped
        // at the cap. Bounded by `DB_JOBS_REPLAY_LOW` so the memory
        // cap still holds through the recovery.
        st.replay_dropped();
        let Some(first) = rx.recv().await else {
            break;
        };
        let mut jobs = Vec::with_capacity(64);
        jobs.push(first);
        while jobs.len() < BATCH {
            match rx.try_recv() {
                Ok(j) => jobs.push(j),
                Err(_) => break,
            }
        }
        let njobs = jobs.len();
        // char ids owed a save_done no matter how the batch ends
        let save_ids: Vec<u32> = jobs
            .iter()
            .filter_map(|j| match j {
                DbJob::SaveChar { char_id, .. } => Some(*char_id),
                _ => None,
            })
            .collect();
        st.db_jobs_depth
            .fetch_sub(njobs, std::sync::atomic::Ordering::Relaxed);
        let db = st.db.clone();
        let (committed, posts) =
            match tokio::task::spawn_blocking(move || apply_db_jobs(&db, jobs)).await {
                Ok(r) => r,
                Err(e) => {
                    tracing::error!("db_writer: batch of {njobs} panicked: {e}");
                    (false, Vec::new())
                }
            };
        if !committed {
            tracing::error!("db_writer: batch of {njobs} failed to commit, jobs dropped");
            // the char saves in this batch never landed: mark them
            // dirty BEFORE the in-flight counts drop, so they are
            // rewritten once the database works again
            for cid in &save_ids {
                st.mark_save_dirty(*cid);
            }
            for cid in save_ids {
                st.save_done(cid);
            }
            // a wedged database would otherwise spin replays
            tokio::time::sleep(Duration::from_millis(200)).await;
            continue;
        }
        for cid in save_ids {
            st.save_done(cid);
        }
        for post in posts {
            match post.reply {
                LinkReply::Map(mid) => {
                    if let Some(tx) = st.map_prio_tx(mid) {
                        send_must(&st, mid, &tx, post.bytes).await;
                    }
                }
                LinkReply::Broadcast => {
                    st.map_broadcast(&post.bytes);
                }
                LinkReply::None => {}
            }
            if let Some(after) = post.after {
                after();
            }
        }
    }
}

/// Apply one drained batch of jobs inside a single transaction.
/// A panicking or failing job is logged and skipped (a statement
/// error does not poison the transaction); the batch only fails
/// when the commit itself does. Within a batch only the LAST save
/// for a char is applied — earlier 0x2b01s are superseded.
fn apply_db_jobs(db: &crate::db::Db, jobs: Vec<DbJob>) -> (bool, Vec<DbOpResult>) {
    // last batch index of a save for each char id
    let mut last_save: HashMap<u32, usize> = HashMap::new();
    for (i, job) in jobs.iter().enumerate() {
        if let DbJob::SaveChar { char_id, .. } = job {
            last_save.insert(*char_id, i);
        }
    }
    let mut posts = Vec::new();
    let committed = db
        .with_conn(|conn| {
            let tx = conn.transaction()?;
            for (i, job) in jobs.into_iter().enumerate() {
                match job {
                    DbJob::SaveChar { char_id, key, data } => {
                        if last_save.get(&char_id) != Some(&i) {
                            continue;
                        }
                        let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                            crate::db::save_character_tx(&tx, &key, &data)
                        }));
                        match r {
                            Ok(Ok(())) => {}
                            Ok(Err(e)) => {
                                tracing::warn!("db_writer: save_character {char_id}: {e}")
                            }
                            Err(_) => {
                                tracing::warn!("db_writer: save_character {char_id} panicked")
                            }
                        }
                    }
                    DbJob::Op(f) => {
                        let r = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| f(&tx)));
                        match r {
                            Ok(res) => posts.push(res),
                            Err(_) => tracing::warn!("db_writer: op panicked"),
                        }
                    }
                }
            }
            tx.commit()
        })
        .is_ok();
    (committed, posts)
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

    /// Drive one char-select completion; the receiver carries the
    /// 0x0071/0x0081 bytes the client would get.
    fn pending_sel(st: &State, map_id: usize, client_ip: [u8; 4]) -> mpsc::Receiver<Vec<u8>> {
        let (tx, rx) = mpsc::channel(8);
        st.send_pending_sel(PendingSel {
            client_tx: tx,
            account_id: 1,
            char_id: 100,
            login_id1: 1,
            login_id2: 2,
            client_ip: u32::from_le_bytes(client_ip),
            map_name: "001-1.gat".into(),
            map_id,
            waiting: std::collections::HashSet::new(),
        });
        rx
    }

    #[test]
    fn send_pending_sel_lan_override() {
        use crate::proto::P0071;
        let st = test_state("10.0.0.0/8", Ipv4Addr::new(192, 168, 1, 10));
        // a map advertising a WAN address
        let (mtx, _mrx) = mpsc::channel(8);
        let (map_id, _kill) =
            st.map_register(mtx.clone(), mtx.clone(), u32::from_le_bytes([203, 0, 113, 7]), 5121);

        // LAN client: gets lan_map_ip, keeps the map's port
        let mut rx = pending_sel(&st, map_id, [10, 1, 2, 3]);
        let p = P0071::decode(&rx.try_recv().unwrap()).unwrap();
        assert_eq!(p.ip.0, [192, 168, 1, 10]);
        assert_eq!(p.port, 5121);

        // WAN client: gets the advertised address
        let mut rx = pending_sel(&st, map_id, [1, 2, 3, 4]);
        let p = P0071::decode(&rx.try_recv().unwrap()).unwrap();
        assert_eq!(p.ip.0, [203, 0, 113, 7]);
        assert_eq!(p.port, 5121);
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

    /// One character row (and its account) for the save-replay
    /// tests: the writer's save is an upsert but `account_id` is a
    /// FK, so the account must exist.
    fn seed_char(db: &crate::db::Db, account_id: i64, char_id: i64) {
        db.with_conn(|conn| {
            conn.execute(
                "INSERT INTO accounts(id,name,password_hash,password_scheme,created_at)
                 VALUES(?1,'acct','x','argon2id',0)",
                [account_id],
            )?;
            conn.execute(
                "INSERT INTO characters(id,account_id,slot,name,sex,species,
                     base_level,job_level,base_exp,job_exp,zeny,hp,max_hp,sp,max_sp,
                     attr_str,attr_agi,attr_vit,attr_int,attr_dex,attr_luk,
                     status_point,skill_point,option_,karma,manner,party_id,
                     hair,hair_color,clothes_color,weapon,shield,
                     head_top,head_mid,head_bottom,
                     last_map,last_x,last_y,save_map,save_x,save_y,partner_id)
                 VALUES(?1,?2,0,'char',0,0,1,1,0,0,0,1,1,0,0,
                        1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                        'map',0,0,'map',0,0,0)",
                rusqlite::params![char_id, account_id],
            )?;
            Ok(())
        })
        .unwrap();
    }

    fn char_key(char_id: u32, account_id: u32) -> CharKey {
        CharKey {
            char_id: crate::proto::CharId(char_id),
            account_id: crate::proto::AccountId(account_id),
            name: crate::proto::types::FixedStr::<24>::try_from_str("char").unwrap(),
            char_num: 0,
        }
    }

    /// Queue depth and owed-write marks all settled. Polls so the
    /// test does not depend on batch boundaries.
    async fn writes_settled(st: &State) -> bool {
        for _ in 0..2000 {
            let pending = st.db_jobs_depth.load(Ordering::Relaxed) != 0
                || !st.saves_in_flight.lock().unwrap().is_empty()
                || !st.save_dirty.lock().unwrap().is_empty()
                || !st.storage_dirty.lock().unwrap().is_empty();
            if !pending {
                return true;
            }
            tokio::time::sleep(Duration::from_millis(1)).await;
        }
        false
    }

    #[tokio::test]
    async fn dropped_char_save_replays_newest_state() {
        let mut st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        st.db_jobs_limit = 4;
        seed_char(&st.db, 1, 100);
        let key = char_key(100, 1);
        let mut data = CharData::default();
        data.zeny = 1;
        st.chars
            .lock()
            .unwrap()
            .insert(100, CharRecord { key, data });
        // six saves for one char; the queue only takes four
        for zeny in 1..=6 {
            data.zeny = zeny;
            // the 0x2b01 handler refreshes the cache before queueing
            st.chars.lock().unwrap().get_mut(&100).unwrap().data = data;
            st.queue_save(100, key, data);
        }
        assert_eq!(st.db_dropped.load(Ordering::Relaxed), 2);
        assert!(st.save_dirty.lock().unwrap().contains(&100));

        // headroom again, then the writer drains and replays
        st.db_jobs_limit = 64;
        let st = Arc::new(st);
        let st2 = st.clone();
        tokio::spawn(async move { db_writer(st2).await });
        assert!(writes_settled(&st).await);

        let (_, cd) = st.db.load_character(100).unwrap();
        assert_eq!(cd.zeny, 6, "replay must write the newest cached state");
        assert!(st.save_dirty.lock().unwrap().is_empty());
    }

    #[tokio::test]
    async fn dropped_storage_save_replays_kept_payload() {
        let mut st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        st.db_jobs_limit = 2;
        // two filler ops occupy the queue; the storage save is dropped
        st.queue_db_op(|_| DbOpResult::none());
        st.queue_db_op(|_| DbOpResult::none());
        st.queue_storage_save(0, crate::proto::AccountId(1), vec![(501, 3, 0)]);
        assert_eq!(st.db_dropped.load(Ordering::Relaxed), 1);
        assert!(st.storage_dirty.lock().unwrap().contains_key(&1));

        st.db_jobs_limit = 64;
        let st = Arc::new(st);
        let st2 = st.clone();
        tokio::spawn(async move { db_writer(st2).await });
        assert!(writes_settled(&st).await);
        assert_eq!(st.db.load_storage(1).unwrap(), vec![(0, 501, 3, 0)]);
    }

    #[tokio::test]
    async fn wait_saves_covers_dirty_marks() {
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        st.mark_save_dirty(9);
        assert!(!st.wait_saves(9, 1, Duration::from_millis(20)).await);
        st.storage_dirty
            .lock()
            .unwrap()
            .insert(1, (0, vec![(7, 1, 0)]));
        st.save_dirty.lock().unwrap().remove(&9);
        // the account's storage mark still blocks the transfer wait
        assert!(!st.wait_saves(9, 1, Duration::from_millis(20)).await);
        st.storage_dirty.lock().unwrap().remove(&1);
        assert!(st.wait_saves(9, 1, Duration::from_millis(20)).await);
    }

    #[test]
    fn queued_save_supersedes_dropped_one() {
        let mut st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        st.db_jobs_limit = 1;
        let key = char_key(7, 1);
        let data = CharData::default();
        st.queue_save(7, key, data);
        st.queue_save(7, key, data);
        assert!(st.save_dirty.lock().unwrap().contains(&7));
        st.db_jobs_limit = 64;
        st.queue_save(7, key, data);
        assert!(!st.save_dirty.lock().unwrap().contains(&7));
        assert_eq!(st.saves_in_flight.lock().unwrap()[&7], 2);
    }

    #[test]
    fn send_pending_sel_default_subnet() {
        use crate::proto::P0071;
        // default lan_subnet covers only 127.0.0.1
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        let (mtx, _mrx) = mpsc::channel(8);
        let (map_id, _kill) =
            st.map_register(mtx.clone(), mtx.clone(), u32::from_le_bytes([203, 0, 113, 7]), 5121);

        let mut rx = pending_sel(&st, map_id, [127, 0, 0, 1]);
        let p = P0071::decode(&rx.try_recv().unwrap()).unwrap();
        assert_eq!(p.ip.0, [127, 0, 0, 1]);

        let mut rx = pending_sel(&st, map_id, [203, 0, 113, 9]);
        let p = P0071::decode(&rx.try_recv().unwrap()).unwrap();
        assert_eq!(p.ip.0, [203, 0, 113, 7]);
    }

    /// A `send_must` that fails on a dead link's channel must not
    /// kill the fresh link that reused its slot.
    #[tokio::test]
    async fn send_must_stale_tx_does_not_kill_new_link() {
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        // link A holds slot 0, then dies and unregisters
        let (txa, rxa) = mpsc::channel(4);
        let (ida, _ka) = st.map_register(txa.clone(), txa.clone(), 0, 0);
        st.map_unregister(ida);
        drop(rxa);
        // a fresh link re-registers into the same slot
        let (txb, _rxb) = mpsc::channel(4);
        let (idb, killb) = st.map_register(txb.clone(), txb.clone(), 0, 0);
        assert_eq!(ida, idb);
        // A's stale prio sender fails instantly (channel closed);
        // the kill aimed at A's slot must not land on B.
        assert!(!send_must(&st, ida, &txa, vec![1, 2, 3]).await);
        let fired = tokio::time::timeout(Duration::from_millis(50), killb.notified()).await;
        assert!(fired.is_err());
    }

    /// While the slot still holds the failing link, the kill fires.
    #[tokio::test]
    async fn send_must_kills_owning_link() {
        let st = test_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        let (txa, rxa) = mpsc::channel(4);
        let (id, kill) = st.map_register(txa.clone(), txa.clone(), 0, 0);
        // the link's writer went away but its session has not
        // unregistered yet: the slot still holds its handle
        drop(rxa);
        assert!(!send_must(&st, id, &txa, vec![1]).await);
        tokio::time::timeout(Duration::from_secs(1), kill.notified())
            .await
            .unwrap();
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
