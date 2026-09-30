// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! Shared runtime state for `tmwa-gate serve`.

use std::collections::HashMap;
use std::sync::Mutex;
use std::sync::atomic::AtomicU64;
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
}

impl MapHandle {
    pub fn send(&self, bytes: Vec<u8>) {
        if let Err(e) = self.tx.try_send(bytes) {
            tracing::warn!("map {id}: queue full/dropped: {e}", id = self.id);
        }
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
    /// Number of map servers that must 0x3830 before the client is
    /// told (tmwa counts all connected map servers).
    pub needed: usize,
    pub seen: usize,
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
}

impl State {
    pub fn new(cfg: Config, db: std::sync::Arc<crate::db::Db>) -> State {
        let next_party_id = db.meta("next_party_id").ok().flatten().unwrap_or(0) as u64;
        State {
            cfg,
            db,
            auth: Mutex::new(HashMap::new()),
            pending_sel: Mutex::new(HashMap::new()),
            map_servers: Mutex::new(Vec::new()),
            online: Mutex::new(HashMap::new()),
            chars: Mutex::new(HashMap::new()),
            char_names: Mutex::new(HashMap::new()),
            char_sessions: Mutex::new(HashMap::new()),
            gm: Mutex::new(HashMap::new()),
            gm_mtime: Mutex::new(None),
            parties: Mutex::new(HashMap::new()),
            next_party_id: AtomicU64::new(next_party_id),
            recent_logins: Mutex::new(HashMap::new()),
            online_notify: tokio::sync::Notify::new(),
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
        }));
        id
    }

    pub fn map_unregister(&self, id: usize) {
        let mut ms = self.map_servers.lock().unwrap();
        if let Some(slot) = ms.get_mut(id) {
            *slot = None;
        }
        // drop online marks for that map
        self.online.lock().unwrap().retain(|_, v| *v != id);
        self.online_notify.notify_waiters();
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
            h.send(bytes);
            true
        } else {
            false
        }
    }

    /// Map server slot that serves `map`, or the first slot with maps.
    /// On fallback, `rewritten` is set to the server's first map.
    pub fn map_for(&self, map: &str) -> (Option<usize>, Option<String>) {
        let ms = self.map_servers.lock().unwrap();
        for (i, slot) in ms.iter().enumerate() {
            if let Some(h) = slot {
                if h.maps.iter().any(|m| m == map) {
                    return (Some(i), None);
                }
            }
        }
        for (i, slot) in ms.iter().enumerate() {
            if let Some(h) = slot {
                if let Some(first) = h.maps.first() {
                    return (Some(i), Some(first.clone()));
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

    // ---- characters / online ----

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
