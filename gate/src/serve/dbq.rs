// Packet structs are filled field-by-field after `Default::default()`:
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! The serialized DB writer: every map-link packet that touches
//! SQLite becomes a `DbJob` on the `db_jobs` queue, drained FIFO by
//! the single `db_writer` task and committed in per-batch
//! transactions, so a slow database cannot stall the per-link read
//! loops.
//!
//! Over `DB_JOBS_LIMIT` queued jobs the enqueue drops rather than
//! grow an unbounded backlog; dropped char/storage saves are marked
//! in `State::save_dirty`/`State::storage_dirty` and re-emitted by
//! `replay_dropped` once the backlog drains below
//! `DB_JOBS_REPLAY_LOW`. `State::saves_in_flight`/`save_notify`
//! track queued char saves so a mid-transfer 0x2afc can wait for
//! them (`State::wait_saves`).

use std::collections::HashMap;
use std::time::Duration;

use bytes::Bytes;
use tokio::sync::mpsc;

use super::state::State;
use crate::proto::{CharData, CharKey, enc};

/// Cap on queued map-link DB jobs. Beyond it the queue drops new
/// jobs rather than become an unbounded memory backlog.
pub(crate) const DB_JOBS_LIMIT: usize = 65536;

/// Queue depth below which `db_writer` re-queues writes that were
/// dropped at the cap: the burst has drained, so the memory for
/// them is affordable again.
const DB_JOBS_REPLAY_LOW: usize = 8192;

/// Cap on the sets of chars/accounts owed a write replay (distinct
/// identities, not jobs). Far past any real player count; it exists
/// so a pathological flood cannot grow even the marks without bound.
const SAVE_DIRTY_LIMIT: usize = 16384;

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
pub type DirtyStorage = (usize, Vec<(i64, i64, i64)>);

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
    /// 0x2b01 character save. `data` shares the cache's Arc: the
    /// ~7 KB CharData is allocated once per 0x2b01, not copied into
    /// the job.
    SaveChar {
        char_id: u32,
        key: Box<CharKey>,
        data: std::sync::Arc<CharData>,
    },
    /// Serialized op; runs inside the batch transaction on the
    /// shared `&Connection` (a `Transaction` derefs to it), so
    /// multi-statement jobs are atomic with the rest of the batch.
    /// Must not open a nested transaction.
    Op(Box<dyn FnOnce(&rusqlite::Connection) -> DbOpResult + Send>),
}

impl State {
    /// Queue a 0x2b01 character save. Marks the char as having an
    /// in-flight save BEFORE enqueueing, so a 0x2afc on another
    /// link can wait for it (`wait_saves`). `data` is shared with
    /// the `chars` cache entry rather than copied.
    pub fn queue_save(&self, char_id: u32, key: CharKey, data: std::sync::Arc<CharData>) {
        self.save_begin(char_id);
        let job = DbJob::SaveChar {
            char_id,
            key: Box::new(key),
            data,
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
    /// still queued is dropped; the reply is computed at apply
    /// time, so it carries the newest committed state anyway. The
    /// flag clears when the job runs. Used for 0x3010/0x3005,
    /// which the map re-requests on reply timeout; under
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
                .map(|c| (c.key, c.data.clone()));
            let Some((key, data)) = cur else {
                // the char record is gone (deleted) or was never
                // cached: there is no newer state left to write
                self.save_dirty.lock().unwrap().remove(&cid);
                tracing::warn!("db_writer: dropped save for char {cid} has no cached state");
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
                data,
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
    /// `db_writer` at task start.
    fn take_db_jobs_rx(&self) -> Option<mpsc::UnboundedReceiver<DbJob>> {
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
pub async fn send_must(
    st: &State,
    map_id: usize,
    tx: &mpsc::Sender<Bytes>,
    v: impl Into<Bytes>,
) -> bool {
    use tokio::sync::mpsc::error::TrySendError;
    match tx.try_send(v.into()) {
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
/// drain batch commits in a single transaction; under a save
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
/// for a char is applied; earlier 0x2b01s are superseded.
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
                            crate::db::save_character_conn(&tx, &key, &data)
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::Config;
    use crate::serve::state::CharRecord;
    use std::net::Ipv4Addr;
    use std::sync::Arc;
    use std::sync::atomic::Ordering;

    fn test_state(lan_subnet: &str, lan_map_ip: Ipv4Addr) -> State {
        let mut cfg = Config::default();
        cfg.lan.lan_subnet = lan_subnet.parse().unwrap();
        cfg.lan.lan_map_ip = lan_map_ip;
        State::new(cfg, Arc::new(crate::db::Db::open_memory().unwrap()))
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
        st.cache_put(
            100,
            CharRecord {
                key,
                data: Arc::new(data),
            },
        );
        // six saves for one char; the queue only takes four
        for zeny in 1..=6 {
            data.zeny = zeny;
            // the 0x2b01 handler refreshes the cache before queueing
            st.chars.lock().unwrap().get_mut(&100).unwrap().data = Arc::new(data);
            st.queue_save(100, key, Arc::new(data));
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
        let items = st
            .db
            .with_conn(|conn| crate::db::load_storage_conn(conn, 1))
            .unwrap();
        assert_eq!(items, vec![(0, 501, 3, 0)]);
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
        st.queue_save(7, key, Arc::new(data));
        st.queue_save(7, key, Arc::new(data));
        assert!(st.save_dirty.lock().unwrap().contains(&7));
        st.db_jobs_limit = 64;
        st.queue_save(7, key, Arc::new(data));
        assert!(!st.save_dirty.lock().unwrap().contains(&7));
        assert_eq!(st.saves_in_flight.lock().unwrap()[&7], 2);
    }
}
