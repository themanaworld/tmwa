//! SQLite storage, mirroring DESIGN.md's Storage section.
//!
//! Schema versions are tracked with `PRAGMA user_version`; MIGRATIONS
//! are applied in order inside one transaction each.

use std::path::Path;
use std::sync::Mutex;

use rusqlite::{Connection, OptionalExtension, params};

use crate::auth::password::Scheme;
use crate::proto::types::FixedStr;
use crate::proto::{
    CharData, CharKey, Epos, GlobalReg, Item, ItemNameId, Sex, SkillFlags, SkillValue,
};

const MIGRATIONS: &[&str] = &[
    // v1
    "
CREATE TABLE accounts (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    password_scheme TEXT NOT NULL CHECK (password_scheme IN ('argon2id','argon2id-md5')),
    legacy_salt TEXT,
    email TEXT,
    state INTEGER NOT NULL DEFAULT 0,
    error_message TEXT,
    ban_until INTEGER NOT NULL DEFAULT 0,
    memo TEXT NOT NULL DEFAULT '',
    last_login INTEGER,
    login_count INTEGER NOT NULL DEFAULT 0,
    last_ip TEXT,
    created_at INTEGER NOT NULL
);
CREATE TABLE account_vars (
    account_id INTEGER NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    scope INTEGER NOT NULL CHECK (scope IN (1,2)),
    name TEXT NOT NULL,
    value INTEGER NOT NULL,
    PRIMARY KEY (account_id, scope, name)
);
CREATE TABLE characters (
    id INTEGER PRIMARY KEY,
    account_id INTEGER NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    slot INTEGER NOT NULL,
    name TEXT NOT NULL UNIQUE,
    sex INTEGER NOT NULL,
    species INTEGER NOT NULL,
    base_level INTEGER NOT NULL,
    job_level INTEGER NOT NULL,
    base_exp INTEGER NOT NULL,
    job_exp INTEGER NOT NULL,
    zeny INTEGER NOT NULL,
    hp INTEGER NOT NULL,
    max_hp INTEGER NOT NULL,
    sp INTEGER NOT NULL,
    max_sp INTEGER NOT NULL,
    attr_str INTEGER NOT NULL,
    attr_agi INTEGER NOT NULL,
    attr_vit INTEGER NOT NULL,
    attr_int INTEGER NOT NULL,
    attr_dex INTEGER NOT NULL,
    attr_luk INTEGER NOT NULL,
    status_point INTEGER NOT NULL,
    skill_point INTEGER NOT NULL,
    option_ INTEGER NOT NULL,
    karma INTEGER NOT NULL,
    manner INTEGER NOT NULL,
    party_id INTEGER NOT NULL,
    hair INTEGER NOT NULL,
    hair_color INTEGER NOT NULL,
    clothes_color INTEGER NOT NULL,
    weapon INTEGER NOT NULL,
    shield INTEGER NOT NULL,
    head_top INTEGER NOT NULL,
    head_mid INTEGER NOT NULL,
    head_bottom INTEGER NOT NULL,
    last_map TEXT NOT NULL,
    last_x INTEGER NOT NULL,
    last_y INTEGER NOT NULL,
    save_map TEXT NOT NULL,
    save_x INTEGER NOT NULL,
    save_y INTEGER NOT NULL,
    partner_id INTEGER NOT NULL,
    UNIQUE (account_id, slot)
);
CREATE TABLE character_items (
    char_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    idx INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    equip INTEGER NOT NULL,
    PRIMARY KEY (char_id, idx)
);
CREATE TABLE character_skills (
    char_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    skill_id INTEGER NOT NULL,
    level INTEGER NOT NULL,
    flags INTEGER NOT NULL,
    PRIMARY KEY (char_id, skill_id)
);
CREATE TABLE character_vars (
    char_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    value INTEGER NOT NULL,
    PRIMARY KEY (char_id, name)
);
CREATE TABLE parties (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    exp_share INTEGER NOT NULL,
    item_share INTEGER NOT NULL
);
CREATE TABLE party_members (
    party_id INTEGER NOT NULL REFERENCES parties(id) ON DELETE CASCADE,
    account_id INTEGER NOT NULL,
    char_name TEXT NOT NULL,
    leader INTEGER NOT NULL,
    PRIMARY KEY (party_id, account_id)
);
CREATE TABLE storage_items (
    account_id INTEGER NOT NULL,
    idx INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    equip INTEGER NOT NULL,
    PRIMARY KEY (account_id, idx)
);
CREATE TABLE password_resets (
    code TEXT PRIMARY KEY,
    account_id INTEGER NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
    expires_at INTEGER NOT NULL
);
CREATE TABLE meta (
    key TEXT PRIMARY KEY,
    value INTEGER NOT NULL
);
",
];

#[derive(Debug, thiserror::Error)]
pub enum DbError {
    #[error("sqlite: {0}")]
    Sqlite(#[from] rusqlite::Error),
    #[error("{0}")]
    Msg(String),
    #[error("account name taken")]
    NameTaken,
}

pub type Result<T> = std::result::Result<T, DbError>;

/// The accounts row the login path needs.
pub struct LoginRow {
    pub id: i64,
    pub password_hash: String,
    pub password_scheme: String,
    pub legacy_salt: Option<String>,
    pub state: i64,
    pub error_message: Option<String>,
    pub ban_until: i64,
}

/// The database. Held as `Arc<Db>`; async callers go through
/// [`Db::blocking`] (spawn_blocking) instead of touching `conn`.
pub struct Db {
    conn: Mutex<Connection>,
}

impl Db {
    /// Run `f` on a blocking thread (for async callers).
    pub async fn blocking<F, T>(self: &std::sync::Arc<Self>, f: F) -> T
    where
        F: FnOnce(&Db) -> T + Send + 'static,
        T: Send + 'static,
    {
        let db = std::sync::Arc::clone(self);
        tokio::task::spawn_blocking(move || f(&db))
            .await
            .expect("blocking db task panicked")
    }

    /// Run `f` on a blocking thread with the locked `&mut
    /// Connection`, for the admin/http paths that compose several
    /// ad-hoc statements in one section.
    pub async fn blocking_conn<F, R>(self: &std::sync::Arc<Self>, f: F) -> R
    where
        F: FnOnce(&mut Connection) -> R + Send + 'static,
        R: Send + 'static,
    {
        let db = std::sync::Arc::clone(self);
        tokio::task::spawn_blocking(move || f(&mut db.lock()))
            .await
            .expect("blocking db task panicked")
    }
}

impl Db {
    /// Open (and create if needed) the database, run pending
    /// migrations, switch to WAL and enable foreign keys.
    pub fn open(path: &Path) -> Result<Db> {
        let conn = Connection::open(path)?;
        Self::init(conn)
    }

    /// In-memory database, for tests.
    pub fn open_memory() -> Result<Db> {
        Self::init(Connection::open_in_memory()?)
    }

    fn init(mut conn: Connection) -> Result<Db> {
        conn.pragma_update(None, "journal_mode", "WAL")?;
        // Under WAL, NORMAL cannot corrupt the database; a power
        // loss may roll back the most recent commits, which FULL
        // would fsync and preserve. Saves are idempotent upserts,
        // so skip the per-commit fsync.
        conn.pragma_update(None, "synchronous", "NORMAL")?;
        conn.pragma_update(None, "foreign_keys", "ON")?;
        let version: i64 = conn.pragma_query_value(None, "user_version", |r| r.get(0))?;
        if version < MIGRATIONS.len() as i64 {
            let tx = conn.transaction()?;
            for (i, sql) in MIGRATIONS[version as usize..].iter().enumerate() {
                tx.execute_batch(sql)?;
                tx.pragma_update(None, "user_version", version + i as i64 + 1)?;
            }
            tx.commit()?;
        }
        Ok(Db {
            conn: Mutex::new(conn),
        })
    }

    /// True when the accounts table is empty (fresh DB).
    pub fn is_empty(&self) -> Result<bool> {
        let conn = self.lock();
        let n: i64 = conn.query_row("SELECT count(*) FROM accounts", [], |r| r.get(0))?;
        Ok(n == 0)
    }

    /// Meta key as i64, e.g. next_account_id.
    pub fn meta(&self, key: &str) -> Result<Option<i64>> {
        let conn = self.lock();
        Ok(conn
            .query_row("SELECT value FROM meta WHERE key=?1", [key], |r| r.get(0))
            .optional()?)
    }

    /// Allocate an id from meta (next_account_id/next_char_id/...),
    /// bumping the counter inside the caller's transaction.
    pub fn alloc_meta_id(conn: &Connection, key: &str) -> rusqlite::Result<i64> {
        let id: i64 = conn.query_row("SELECT value FROM meta WHERE key=?1", [key], |r| r.get(0))?;
        conn.execute(
            "UPDATE meta SET value=?1 WHERE key=?2",
            params![id + 1, key],
        )?;
        Ok(id)
    }

    /// Create an account row inside one transaction: uniqueness
    /// check, next_account_id allocation, default columns. Used by
    /// the _M/_F registration, the admin create command and the
    /// HTTP API so all paths allocate ids the same way.
    pub fn create_account(
        conn: &mut Connection,
        name: &str,
        password_hash: &str,
        email: &str,
    ) -> Result<i64> {
        let tx = conn.transaction()?;
        let dupe: i64 = tx
            .query_row("SELECT COUNT(*) FROM accounts WHERE name=?1", [name], |r| {
                r.get(0)
            })
            .unwrap_or(0);
        if dupe > 0 {
            return Err(DbError::NameTaken);
        }
        let id = Self::alloc_meta_id(&tx, "next_account_id")?;
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_millis() as i64;
        Self::insert_account(
            &tx,
            id,
            name,
            password_hash,
            Scheme::Argon2id,
            None,
            Some(email),
            0,
            None,
            0,
            "",
            None,
            0,
            None,
            now,
        )?;
        tx.commit()?;
        Ok(id)
    }

    /// Account id by exact name.
    pub fn account_id_by_name(&self, name: &str) -> Result<Option<i64>> {
        let conn = self.lock();
        Ok(account_id_by_name_conn(&conn, name)?)
    }

    /// Full account row for the login path.
    pub fn account_auth_row(&self, name: &str) -> Result<Option<LoginRow>> {
        let conn = self.lock();
        Ok(conn
            .query_row(
                "SELECT id,password_hash,password_scheme,legacy_salt,
                 state,error_message,ban_until
                 FROM accounts WHERE name=?1",
                [name],
                |r| {
                    Ok(LoginRow {
                        id: r.get(0)?,
                        password_hash: r.get(1)?,
                        password_scheme: r.get(2)?,
                        legacy_salt: r.get(3)?,
                        state: r.get(4)?,
                        error_message: r.get(5)?,
                        ban_until: r.get(6)?,
                    })
                },
            )
            .optional()?)
    }

    /// Record a successful login (stamp + ip + count).
    /// Returns the previous last_login (ms) for the 0x0069 field.
    pub fn record_login(&self, account_id: i64, now_ms: i64, ip: &str) -> Result<Option<i64>> {
        let conn = self.lock();
        let prev: Option<i64> = conn
            .query_row(
                "SELECT last_login FROM accounts WHERE id=?1",
                [account_id],
                |r| r.get(0),
            )
            .optional()?;
        conn.execute(
            "UPDATE accounts SET last_login=?2, last_ip=?3,
             login_count=login_count+1 WHERE id=?1",
            params![account_id, now_ms, ip],
        )?;
        Ok(prev)
    }

    /// Update the password (hash + scheme; clears legacy_salt for
    /// plain argon2id).
    pub fn set_password(
        &self,
        account_id: i64,
        hash: &str,
        scheme: Scheme,
        salt: Option<&str>,
    ) -> Result<()> {
        let conn = self.lock();
        Ok(set_password_conn(&conn, account_id, hash, scheme, salt)?)
    }

    pub fn set_memo(&self, account_id: i64, memo: &str) -> Result<()> {
        let conn = self.lock();
        conn.execute(
            "UPDATE accounts SET memo=?2 WHERE id=?1",
            params![account_id, memo],
        )?;
        Ok(())
    }

    pub fn set_email(&self, account_id: i64, email: Option<&str>) -> Result<()> {
        let conn = self.lock();
        Ok(set_email_conn(&conn, account_id, email)?)
    }

    pub fn set_account_state(&self, account_id: i64, state: i64) -> Result<()> {
        let conn = self.lock();
        Ok(set_account_state_conn(&conn, account_id, state)?)
    }

    /// Set state and the login error message together (the admin
    /// `state` command carries an optional error_message for state 7).
    pub fn set_account_state_msg(
        &self,
        account_id: i64,
        state: i64,
        error_message: &str,
    ) -> Result<()> {
        let conn = self.lock();
        conn.execute(
            "UPDATE accounts SET state=?2, error_message=?3 WHERE id=?1",
            params![account_id, state, error_message],
        )?;
        Ok(())
    }

    /// Current ban_until (unix seconds), if the account exists.
    pub fn account_ban_until(&self, account_id: i64) -> Result<Option<i64>> {
        let conn = self.lock();
        Ok(conn
            .query_row(
                "SELECT ban_until FROM accounts WHERE id=?1",
                [account_id],
                |r| r.get(0),
            )
            .optional()?)
    }

    /// ban_until (unix seconds) or unblock when 0.
    pub fn set_account_ban(&self, account_id: i64, ban_until: i64) -> Result<()> {
        let conn = self.lock();
        Ok(set_account_ban_conn(&conn, account_id, ban_until)?)
    }

    /// Char id by exact name.
    pub fn char_id_by_name(&self, name: &str) -> Result<Option<i64>> {
        let conn = self.lock();
        Ok(conn
            .query_row("SELECT id FROM characters WHERE name=?1", [name], |r| {
                r.get(0)
            })
            .optional()?)
    }

    /// Char ids of one account.
    pub fn char_ids_of_account(&self, account_id: i64) -> Result<Vec<i64>> {
        let conn = self.lock();
        Ok(char_ids_of_account_conn(&conn, account_id)?)
    }

    pub fn delete_character(&self, char_id: i64) -> Result<()> {
        let conn = self.lock();
        conn.execute("DELETE FROM characters WHERE id=?1", [char_id])?;
        Ok(())
    }

    /// Clear partner_id both directions (0x2b16 divorce).
    pub fn divorce(&self, char_id: i64) -> Result<Option<i64>> {
        let conn = self.lock();
        Ok(divorce_conn(&conn, char_id)?)
    }

    /// Lock the connection. A panic while it was held (e.g. in an admin
    /// request) must not take down every later save, and SQLite rolls
    /// back any open transaction, so a poisoned lock is still usable.
    fn lock(&self) -> std::sync::MutexGuard<'_, Connection> {
        self.conn.lock().unwrap_or_else(|e| e.into_inner())
    }

    /// Run `f` with the connection under the lock.
    pub fn with_conn<R>(
        &self,
        f: impl FnOnce(&mut Connection) -> rusqlite::Result<R>,
    ) -> Result<R> {
        let mut conn = self.lock();
        f(&mut conn).map_err(DbError::from)
    }

    /// Insert an account row (used by the importer). Takes
    /// `&Connection` so it composes inside the importer's transaction.
    #[allow(clippy::too_many_arguments)]
    pub fn insert_account(
        conn: &Connection,
        id: i64,
        name: &str,
        password_hash: &str,
        password_scheme: Scheme,
        legacy_salt: Option<&str>,
        email: Option<&str>,
        state: i64,
        error_message: Option<&str>,
        ban_until: i64,
        memo: &str,
        last_login: Option<i64>,
        login_count: i64,
        last_ip: Option<&str>,
        created_at: i64,
    ) -> rusqlite::Result<()> {
        conn.execute(
            "INSERT INTO accounts(id,name,password_hash,password_scheme,
             legacy_salt,email,state,error_message,ban_until,memo,
             last_login,login_count,last_ip,created_at)
             VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)",
            params![
                id,
                name,
                password_hash,
                password_scheme.as_str(),
                legacy_salt,
                email,
                state,
                error_message,
                ban_until,
                memo,
                last_login,
                login_count,
                last_ip,
                created_at,
            ],
        )?;
        Ok(())
    }

    // ---- characters ----

    /// Load a character as generated CharKey + CharData. Fills
    /// account_reg (scope 1) and account_reg2 (scope 2) from
    /// account_vars, char vars from character_vars.
    pub fn load_character(&self, char_id: i64) -> Result<(CharKey, CharData)> {
        self.with_conn(|conn| load_character_conn(conn, char_id))
    }
}

// ---- conn-level helpers ----
//
// The maplink DB writer runs jobs inside one shared batch
// transaction and the importer writes its whole load in one; these
// take `&Connection` (a `&Transaction` derefs to it) so they compose
// there and standalone. Callers must not open a nested transaction
// on it.

/// Upsert a meta row (key/value).
pub fn set_meta_conn(conn: &Connection, key: &str, value: i64) -> rusqlite::Result<()> {
    conn.execute(
        "INSERT INTO meta(key,value) VALUES(?1,?2)
         ON CONFLICT(key) DO UPDATE SET value=excluded.value",
        params![key, value],
    )?;
    Ok(())
}

/// Account id by exact name.
pub fn account_id_by_name_conn(conn: &Connection, name: &str) -> rusqlite::Result<Option<i64>> {
    conn.query_row("SELECT id FROM accounts WHERE name=?1", [name], |r| {
        r.get(0)
    })
    .optional()
}

/// Account name by id.
pub fn account_name_by_id_conn(conn: &Connection, id: i64) -> rusqlite::Result<Option<String>> {
    conn.query_row("SELECT name FROM accounts WHERE id=?1", [id], |r| r.get(0))
        .optional()
}

/// An account with this id exists.
pub fn account_exists_conn(conn: &Connection, id: i64) -> rusqlite::Result<bool> {
    Ok(conn
        .query_row("SELECT 1 FROM accounts WHERE id=?1", [id], |_| Ok(()))
        .optional()?
        .is_some())
}

/// Account email (NULL-able column flattens to Option).
pub fn account_email_conn(conn: &Connection, account_id: i64) -> rusqlite::Result<Option<String>> {
    Ok(conn
        .query_row(
            "SELECT email FROM accounts WHERE id=?1",
            [account_id],
            |r| r.get::<_, Option<String>>(0),
        )
        .optional()?
        .flatten())
}

/// (password_hash, password_scheme, legacy_salt) by account id.
pub fn password_row_conn(
    conn: &Connection,
    account_id: i64,
) -> rusqlite::Result<Option<(String, String, Option<String>)>> {
    conn.query_row(
        "SELECT password_hash,password_scheme,legacy_salt
         FROM accounts WHERE id=?1",
        [account_id],
        |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)),
    )
    .optional()
}

/// Update the password (hash + scheme; clears legacy_salt for plain
/// argon2id).
pub fn set_password_conn(
    conn: &Connection,
    account_id: i64,
    hash: &str,
    scheme: Scheme,
    salt: Option<&str>,
) -> rusqlite::Result<()> {
    conn.execute(
        "UPDATE accounts SET password_hash=?2, password_scheme=?3,
         legacy_salt=?4 WHERE id=?1",
        params![account_id, hash, scheme.as_str(), salt],
    )?;
    Ok(())
}

pub fn set_email_conn(
    conn: &Connection,
    account_id: i64,
    email: Option<&str>,
) -> rusqlite::Result<()> {
    conn.execute(
        "UPDATE accounts SET email=?2 WHERE id=?1",
        params![account_id, email],
    )?;
    Ok(())
}

pub fn set_account_state_conn(
    conn: &Connection,
    account_id: i64,
    state: i64,
) -> rusqlite::Result<()> {
    conn.execute(
        "UPDATE accounts SET state=?2 WHERE id=?1",
        params![account_id, state],
    )?;
    Ok(())
}

/// ban_until (unix seconds) or unblock when 0.
pub fn set_account_ban_conn(
    conn: &Connection,
    account_id: i64,
    ban_until: i64,
) -> rusqlite::Result<()> {
    conn.execute(
        "UPDATE accounts SET ban_until=?2 WHERE id=?1",
        params![account_id, ban_until],
    )?;
    Ok(())
}

/// Insert or replace a party and its member list. `members` is
/// (account_id, char_name, leader).
pub fn upsert_party_conn(
    conn: &Connection,
    party_id: i64,
    name: &str,
    exp_share: i64,
    item_share: i64,
    members: &[(i64, String, i64)],
) -> rusqlite::Result<()> {
    conn.execute(
        "INSERT INTO parties(id,name,exp_share,item_share) VALUES(?1,?2,?3,?4)
         ON CONFLICT(id) DO UPDATE SET name=excluded.name,
         exp_share=excluded.exp_share,item_share=excluded.item_share",
        params![party_id, name, exp_share, item_share],
    )?;
    conn.execute("DELETE FROM party_members WHERE party_id=?1", [party_id])?;
    let mut st = conn.prepare(
        "INSERT INTO party_members(party_id,account_id,char_name,leader)
         VALUES(?1,?2,?3,?4)",
    )?;
    for (acct, char_name, leader) in members {
        st.execute(params![party_id, acct, char_name, leader])?;
    }
    Ok(())
}

/// Storage items for an account, slot order.
pub fn load_storage_conn(
    conn: &Connection,
    account_id: i64,
) -> rusqlite::Result<Vec<(i64, i64, i64, i64)>> {
    let mut st = conn.prepare_cached(
        "SELECT idx,item_id,amount,equip FROM storage_items
         WHERE account_id=?1 ORDER BY idx",
    )?;
    st.query_map([account_id], |r| {
        Ok((r.get(0)?, r.get(1)?, r.get(2)?, r.get(3)?))
    })?
    .collect::<rusqlite::Result<Vec<_>>>()
}

/// Replace a whole storage (0x3011 semantics): items are
/// (item_id, amount, equip) in slot order.
pub fn save_storage_conn(
    conn: &Connection,
    account_id: i64,
    items: &[(i64, i64, i64)],
) -> rusqlite::Result<()> {
    conn.prepare_cached("DELETE FROM storage_items WHERE account_id=?1")?
        .execute([account_id])?;
    let mut st = conn.prepare_cached(
        "INSERT INTO storage_items(account_id,idx,item_id,amount,equip)
         VALUES(?1,?2,?3,?4,?5)",
    )?;
    for (idx, item) in items.iter().enumerate() {
        st.execute(params![account_id, idx as i64, item.0, item.1, item.2])?;
    }
    Ok(())
}

/// All vars for an account/scope, name order.
pub fn get_account_vars_conn(
    conn: &Connection,
    account_id: i64,
    scope: i64,
) -> rusqlite::Result<Vec<(String, i64)>> {
    let mut st = conn.prepare_cached(
        "SELECT name,value FROM account_vars
         WHERE account_id=?1 AND scope=?2 ORDER BY name",
    )?;
    st.query_map(params![account_id, scope], |r| Ok((r.get(0)?, r.get(1)?)))?
        .collect::<rusqlite::Result<Vec<_>>>()
}

/// Insert-or-update each (name, value) pair in `vars` at `scope`.
/// Does not delete other names; callers that want the tmwa
/// "replace the whole scope" semantics DELETE first.
pub fn set_account_vars(
    conn: &Connection,
    account_id: i64,
    scope: i64,
    vars: &[(String, i64)],
) -> rusqlite::Result<()> {
    let mut st = conn.prepare_cached(
        "INSERT INTO account_vars(account_id,scope,name,value)
         VALUES(?1,?2,?3,?4)
         ON CONFLICT(account_id,scope,name) DO UPDATE SET
         value=excluded.value",
    )?;
    for (name, value) in vars {
        st.execute(params![account_id, scope, name, value])?;
    }
    Ok(())
}

/// Clear partner_id both directions (0x2b16 divorce); returns the
/// former partner's char id.
pub fn divorce_conn(conn: &Connection, char_id: i64) -> rusqlite::Result<Option<i64>> {
    let partner: Option<i64> = conn
        .query_row(
            "SELECT partner_id FROM characters WHERE id=?1",
            [char_id],
            |r| r.get(0),
        )
        .optional()?;
    if let Some(pid) = partner
        && pid != 0
    {
        conn.execute(
            "UPDATE characters SET partner_id=0 WHERE id IN (?1,?2)",
            params![char_id, pid],
        )?;
    }
    Ok(partner.filter(|p| *p != 0))
}

/// Char ids of one account, slot order.
pub fn char_ids_of_account_conn(conn: &Connection, account_id: i64) -> rusqlite::Result<Vec<i64>> {
    let mut st = conn.prepare("SELECT id FROM characters WHERE account_id=?1 ORDER BY slot")?;
    st.query_map([account_id], |r| r.get(0))?
        .collect::<rusqlite::Result<Vec<i64>>>()
}

fn load_character_conn(conn: &Connection, char_id: i64) -> rusqlite::Result<(CharKey, CharData)> {
    let (key, mut cd) = conn.query_row(
        "SELECT account_id,slot,name,sex,species,base_level,job_level,
         base_exp,job_exp,zeny,hp,max_hp,sp,max_sp,
         attr_str,attr_agi,attr_vit,attr_int,attr_dex,attr_luk,
         status_point,skill_point,option_,karma,manner,party_id,
         hair,hair_color,clothes_color,weapon,shield,
         head_top,head_mid,head_bottom,
         last_map,last_x,last_y,save_map,save_x,save_y,partner_id
         FROM characters WHERE id=?1",
        [char_id],
        |r| {
            let mut key = CharKey::default();
            let mut cd = CharData::default();
            key.account_id = crate::proto::AccountId(r.get::<usize, i64>(0)? as u32);
            key.char_id = crate::proto::CharId(char_id as u32);
            key.char_num = r.get::<usize, i64>(1)? as u8;
            key.name = FixedStr::<24>::from_str_truncate(&r.get::<usize, String>(2)?);
            cd.sex = Sex(r.get::<usize, i64>(3)? as u8);
            cd.species = crate::proto::Species(r.get::<usize, i64>(4)? as u16);
            cd.base_level = r.get::<usize, i64>(5)? as u8;
            cd.job_level = r.get::<usize, i64>(6)? as u8;
            cd.base_exp = r.get::<usize, i64>(7)? as i32;
            cd.job_exp = r.get::<usize, i64>(8)? as i32;
            cd.zeny = r.get::<usize, i64>(9)? as i32;
            cd.hp = r.get::<usize, i64>(10)? as i32;
            cd.max_hp = r.get::<usize, i64>(11)? as i32;
            cd.sp = r.get::<usize, i64>(12)? as i32;
            cd.max_sp = r.get::<usize, i64>(13)? as i32;
            for (i, a) in cd.attrs.iter_mut().enumerate() {
                *a = r.get::<usize, i64>(14 + i)? as i16;
            }
            cd.status_point = r.get::<usize, i64>(20)? as i16;
            cd.skill_point = r.get::<usize, i64>(21)? as i16;
            cd.option = crate::proto::Opt0(r.get::<usize, i64>(22)? as u16);
            cd.karma = r.get::<usize, i64>(23)? as i16;
            cd.manner = r.get::<usize, i64>(24)? as i16;
            cd.party_id = crate::proto::PartyId(r.get::<usize, i64>(25)? as u32);
            cd.hair = r.get::<usize, i64>(26)? as i16;
            cd.hair_color = r.get::<usize, i64>(27)? as i16;
            cd.clothes_color = r.get::<usize, i64>(28)? as i16;
            cd.weapon = crate::proto::ItemLook(r.get::<usize, i64>(29)? as u16);
            cd.shield = ItemNameId(r.get::<usize, i64>(30)? as u32);
            cd.head_top = ItemNameId(r.get::<usize, i64>(31)? as u32);
            cd.head_mid = ItemNameId(r.get::<usize, i64>(32)? as u32);
            cd.head_bottom = ItemNameId(r.get::<usize, i64>(33)? as u32);
            cd.last_point = crate::proto::Point {
                map_: FixedStr::<16>::from_str_truncate(&r.get::<usize, String>(34)?),
                x: r.get::<usize, i64>(35)? as i16,
                y: r.get::<usize, i64>(36)? as i16,
            };
            cd.save_point = crate::proto::Point {
                map_: FixedStr::<16>::from_str_truncate(&r.get::<usize, String>(37)?),
                x: r.get::<usize, i64>(38)? as i16,
                y: r.get::<usize, i64>(39)? as i16,
            };
            cd.partner_id = crate::proto::CharId(r.get::<usize, i64>(40)? as u32);
            Ok((key, cd))
        },
    )?;
    {
        let mut st =
            conn.prepare("SELECT idx,item_id,amount,equip FROM character_items WHERE char_id=?1")?;
        let rows = st.query_map([char_id], |r| {
            Ok((
                r.get::<usize, i64>(0)? as usize,
                r.get::<usize, i64>(1)? as u32,
                r.get::<usize, i64>(2)? as i16,
                r.get::<usize, i64>(3)? as u16,
            ))
        })?;
        for row in rows {
            let (idx, item_id, amount, equip) = row?;
            if idx < cd.inventory.len() {
                cd.inventory[idx] = Item {
                    nameid: ItemNameId(item_id),
                    amount,
                    equip: Epos(equip),
                };
            }
        }
    }
    {
        let mut st =
            conn.prepare("SELECT skill_id,level,flags FROM character_skills WHERE char_id=?1")?;
        let rows = st.query_map([char_id], |r| {
            Ok((
                r.get::<usize, i64>(0)? as usize,
                r.get::<usize, i64>(1)? as u16,
                r.get::<usize, i64>(2)? as u16,
            ))
        })?;
        for row in rows {
            let (id, lv, flags) = row?;
            if id < cd.skill.len() {
                cd.skill[id] = SkillValue {
                    lv,
                    flags: SkillFlags(flags),
                };
            }
        }
    }
    {
        let mut st =
            conn.prepare("SELECT name,value FROM character_vars WHERE char_id=?1 ORDER BY name")?;
        let rows = st.query_map([char_id], |r| {
            Ok((r.get::<usize, String>(0)?, r.get::<usize, i64>(1)?))
        })?;
        for row in rows {
            let (name, value) = row?;
            let i = cd.global_reg_num as usize;
            if i < cd.global_reg.len() {
                cd.global_reg[i] = GlobalReg {
                    str: FixedStr::<32>::from_str_truncate(&name),
                    value: value as i32,
                };
                cd.global_reg_num += 1;
            }
        }
    }
    {
        let account_id = key.account_id.0 as i64;
        let mut st = conn.prepare(
            "SELECT scope,name,value FROM account_vars WHERE account_id=?1 ORDER BY scope,name",
        )?;
        let rows = st.query_map([account_id], |r| {
            Ok((
                r.get::<usize, i64>(0)?,
                r.get::<usize, String>(1)?,
                r.get::<usize, i64>(2)?,
            ))
        })?;
        for row in rows {
            let (scope, name, value) = row?;
            let reg = GlobalReg {
                str: FixedStr::<32>::from_str_truncate(&name),
                value: value as i32,
            };
            if scope == 1 {
                let i = cd.account_reg_num as usize;
                if i < cd.account_reg.len() {
                    cd.account_reg[i] = reg;
                    cd.account_reg_num += 1;
                }
            } else {
                let i = cd.account_reg2_num as usize;
                if i < cd.account_reg2.len() {
                    cd.account_reg2[i] = reg;
                    cd.account_reg2_num += 1;
                }
            }
        }
    }
    Ok((key, cd))
}

/// Replace a character's scalar row plus items/skills/vars. NOT
/// `INSERT OR REPLACE`: REPLACE resolves every unique constraint by
/// deleting the conflicting row, so a name or (account_id, slot)
/// collision with a *different* character would silently delete that
/// character (and cascade its items/skills/vars). The `ON CONFLICT(id)
/// DO UPDATE` form errors instead.
pub fn save_character_conn(
    conn: &Connection,
    key: &CharKey,
    cd: &CharData,
) -> rusqlite::Result<()> {
    let char_id = key.char_id.0 as i64;
    conn.prepare_cached(
        "INSERT INTO characters(
             id,account_id,slot,name,sex,species,base_level,job_level,
             base_exp,job_exp,zeny,hp,max_hp,sp,max_sp,
             attr_str,attr_agi,attr_vit,attr_int,attr_dex,attr_luk,
             status_point,skill_point,option_,karma,manner,party_id,
             hair,hair_color,clothes_color,weapon,shield,
             head_top,head_mid,head_bottom,
             last_map,last_x,last_y,save_map,save_x,save_y,partner_id)
         VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,
                ?16,?17,?18,?19,?20,?21,?22,?23,?24,?25,?26,?27,
                ?28,?29,?30,?31,?32,?33,?34,?35,?36,?37,?38,?39,?40,?41,?42)
         ON CONFLICT(id) DO UPDATE SET
             account_id=excluded.account_id, slot=excluded.slot,
             name=excluded.name, sex=excluded.sex, species=excluded.species,
             base_level=excluded.base_level, job_level=excluded.job_level,
             base_exp=excluded.base_exp, job_exp=excluded.job_exp,
             zeny=excluded.zeny, hp=excluded.hp, max_hp=excluded.max_hp,
             sp=excluded.sp, max_sp=excluded.max_sp,
             attr_str=excluded.attr_str, attr_agi=excluded.attr_agi,
             attr_vit=excluded.attr_vit, attr_int=excluded.attr_int,
             attr_dex=excluded.attr_dex, attr_luk=excluded.attr_luk,
             status_point=excluded.status_point, skill_point=excluded.skill_point,
             option_=excluded.option_, karma=excluded.karma, manner=excluded.manner,
             party_id=excluded.party_id,
             hair=excluded.hair, hair_color=excluded.hair_color,
             clothes_color=excluded.clothes_color,
             weapon=excluded.weapon, shield=excluded.shield,
             head_top=excluded.head_top, head_mid=excluded.head_mid,
             head_bottom=excluded.head_bottom,
             last_map=excluded.last_map, last_x=excluded.last_x, last_y=excluded.last_y,
             save_map=excluded.save_map, save_x=excluded.save_x, save_y=excluded.save_y,
             partner_id=excluded.partner_id",
    )?
    .execute(params![
        char_id,
        key.account_id.0 as i64,
        key.char_num as i64,
        key.name.to_string_lossy(),
        cd.sex.0 as i64,
        cd.species.0 as i64,
        cd.base_level as i64,
        cd.job_level as i64,
        cd.base_exp as i64,
        cd.job_exp as i64,
        cd.zeny as i64,
        cd.hp as i64,
        cd.max_hp as i64,
        cd.sp as i64,
        cd.max_sp as i64,
        cd.attrs[0] as i64,
        cd.attrs[1] as i64,
        cd.attrs[2] as i64,
        cd.attrs[3] as i64,
        cd.attrs[4] as i64,
        cd.attrs[5] as i64,
        cd.status_point as i64,
        cd.skill_point as i64,
        cd.option.0 as i64,
        cd.karma as i64,
        cd.manner as i64,
        cd.party_id.0 as i64,
        cd.hair as i64,
        cd.hair_color as i64,
        cd.clothes_color as i64,
        cd.weapon.0 as i64,
        cd.shield.0 as i64,
        cd.head_top.0 as i64,
        cd.head_mid.0 as i64,
        cd.head_bottom.0 as i64,
        cd.last_point.map_.to_string_lossy(),
        cd.last_point.x as i64,
        cd.last_point.y as i64,
        cd.save_point.map_.to_string_lossy(),
        cd.save_point.x as i64,
        cd.save_point.y as i64,
        cd.partner_id.0 as i64,
    ])?;
    conn.prepare_cached("DELETE FROM character_items WHERE char_id=?1")?
        .execute([char_id])?;
    {
        let mut st = conn.prepare_cached(
            "INSERT INTO character_items(char_id,idx,item_id,amount,equip)
             VALUES(?1,?2,?3,?4,?5)",
        )?;
        for (i, item) in cd.inventory.iter().enumerate() {
            if item.nameid.0 != 0 {
                st.execute(params![
                    char_id,
                    i as i64,
                    item.nameid.0 as i64,
                    item.amount as i64,
                    item.equip.0 as i64
                ])?;
            }
        }
    }
    conn.prepare_cached("DELETE FROM character_skills WHERE char_id=?1")?
        .execute([char_id])?;
    {
        let mut st = conn.prepare_cached(
            "INSERT INTO character_skills(char_id,skill_id,level,flags)
             VALUES(?1,?2,?3,?4)",
        )?;
        for (i, sk) in cd.skill.iter().enumerate() {
            if sk.lv != 0 {
                st.execute(params![char_id, i as i64, sk.lv as i64, sk.flags.0 as i64])?;
            }
        }
    }
    conn.prepare_cached("DELETE FROM character_vars WHERE char_id=?1")?
        .execute([char_id])?;
    {
        let mut st =
            conn.prepare_cached("INSERT INTO character_vars(char_id,name,value) VALUES(?1,?2,?3)")?;
        for reg in cd.global_reg.iter().take(cd.global_reg_num as usize) {
            if reg.str.as_bytes().is_empty() {
                continue;
            }
            st.execute(params![
                char_id,
                reg.str.to_string_lossy(),
                reg.value as i64
            ])?;
        }
    }
    // NOTE: account_reg (#) and account_reg2 (##) are NOT saved
    // here. In tmwa, athena.txt never persists them: # vars are
    // written only by the 0x3004 handler (inter.cpp) and ## vars only
    // by 0x2b10 (login). A 0x2b01 save carries a stale snapshot in
    // CharData and must not overwrite newer values.
    Ok(())
}
