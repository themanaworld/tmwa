//! SQLite storage, mirroring DESIGN.md's Storage section.
//!
//! Schema versions are tracked with `PRAGMA user_version`; MIGRATIONS
//! are applied in order inside one transaction each.

use std::path::Path;
use std::sync::Mutex;

use rusqlite::{Connection, OptionalExtension, Transaction, params};

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
    #[error("string too long")]
    StrTooLong,
}

pub type Result<T> = std::result::Result<T, DbError>;

/// Account auth fields: (id, password_hash, password_scheme,
/// legacy_salt).
pub type AuthRow = (i64, String, String, Option<String>);

/// The database. Callers that need async should go through
/// `tokio::task::spawn_blocking`.
pub struct Db {
    conn: Mutex<Connection>,
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
        let conn = self.conn.lock().unwrap();
        let n: i64 = conn.query_row("SELECT count(*) FROM accounts", [], |r| r.get(0))?;
        Ok(n == 0)
    }

    /// Meta key as i64, e.g. next_account_id.
    pub fn meta(&self, key: &str) -> Result<Option<i64>> {
        let conn = self.conn.lock().unwrap();
        Ok(conn
            .query_row("SELECT value FROM meta WHERE key=?1", [key], |r| r.get(0))
            .optional()?)
    }

    pub fn set_meta(&self, key: &str, value: i64) -> Result<()> {
        let conn = self.conn.lock().unwrap();
        conn.execute(
            "INSERT INTO meta(key,value) VALUES(?1,?2)
             ON CONFLICT(key) DO UPDATE SET value=excluded.value",
            params![key, value],
        )?;
        Ok(())
    }

    /// Run `f` with the connection under the lock.
    pub fn with_conn<R>(
        &self,
        f: impl FnOnce(&mut Connection) -> rusqlite::Result<R>,
    ) -> Result<R> {
        let mut conn = self.conn.lock().unwrap();
        f(&mut conn).map_err(DbError::from)
    }

    /// Insert an account row (used by the importer).
    #[allow(clippy::too_many_arguments)]
    pub fn insert_account(
        tx: &Transaction<'_>,
        id: i64,
        name: &str,
        password_hash: &str,
        password_scheme: &str,
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
        tx.execute(
            "INSERT INTO accounts(id,name,password_hash,password_scheme,
             legacy_salt,email,state,error_message,ban_until,memo,
             last_login,login_count,last_ip,created_at)
             VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)",
            params![
                id,
                name,
                password_hash,
                password_scheme,
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

    /// Fetch account row fields needed for auth.
    pub fn find_account_by_name(&self, name: &str) -> Result<Option<AuthRow>> {
        let conn = self.conn.lock().unwrap();
        Ok(conn
            .query_row(
                "SELECT id,password_hash,password_scheme,legacy_salt
                 FROM accounts WHERE name=?1",
                [name],
                |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?, r.get(3)?)),
            )
            .optional()?)
    }

    // ---- characters ----

    /// Load a character as generated CharKey + CharData. Fills
    /// account_reg (scope 1) and account_reg2 (scope 2) from
    /// account_vars, char vars from character_vars.
    pub fn load_character(&self, char_id: i64) -> Result<(CharKey, CharData)> {
        self.with_conn(|conn| load_character_conn(conn, char_id))
    }

    /// Save a character: one transaction replaces the scalar row and
    /// all items/skills/vars.
    pub fn save_character(&self, key: &CharKey, data: &CharData) -> Result<()> {
        self.with_conn(|conn| {
            let tx = conn.transaction()?;
            save_character_tx(&tx, key, data)?;
            tx.commit()
        })
    }

    /// Character keys for one account, slot order.
    pub fn list_characters(&self, account_id: i64) -> Result<Vec<CharKey>> {
        let conn = self.conn.lock().unwrap();
        let mut st =
            conn.prepare("SELECT id,name,slot FROM characters WHERE account_id=?1 ORDER BY slot")?;
        let rows = st
            .query_map([account_id], |r| {
                let id: i64 = r.get(0)?;
                let name: String = r.get(1)?;
                let slot: i64 = r.get(2)?;
                Ok((id, name, slot))
            })?
            .collect::<rusqlite::Result<Vec<_>>>()?;
        Ok(rows
            .into_iter()
            .map(|(id, name, slot)| CharKey {
                name: FixedStr::<24>::try_from_str(&name).unwrap_or_default(),
                account_id: crate::proto::AccountId(account_id as u32),
                char_id: crate::proto::CharId(id as u32),
                char_num: slot as u8,
            })
            .collect())
    }
}

fn fixed_str<const N: usize>(s: &str) -> Result<FixedStr<N>> {
    FixedStr::try_from_str(s).map_err(|_| DbError::StrTooLong)
}

fn load_character_conn(
    conn: &mut Connection,
    char_id: i64,
) -> rusqlite::Result<(CharKey, CharData)> {
    let mut cd = CharData::default();
    let mut key = CharKey::default();
    {
        let row = conn.query_row(
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
                let account_id: i64 = r.get(0)?;
                let slot: i64 = r.get(1)?;
                let name: String = r.get(2)?;
                let mut ints = Vec::with_capacity(36);
                // ints in 3..34, text at 34 (last_map), ints 35..37,
                // text at 37 (save_map), ints 38..41.
                for i in 3..34 {
                    ints.push(r.get::<usize, i64>(i)?);
                }
                let last_map: String = r.get(34)?;
                for i in 35..37 {
                    ints.push(r.get::<usize, i64>(i)?);
                }
                let save_map: String = r.get(37)?;
                for i in 38..41 {
                    ints.push(r.get::<usize, i64>(i)?);
                }
                let ck = (char_id, slot, name, account_id);
                Ok((ck, ints, last_map, save_map))
            },
        )?;
        let (ck, vals, last_map, save_map) = row;
        // ints layout: cols 3..34 -> vals[0..31], cols 35..37 ->
        // vals[31..33], cols 38..41 -> vals[33..36].
        let v = |i: usize| match i {
            3..=33 => vals[i - 3],
            35..=36 => vals[i - 4],
            38..=40 => vals[i - 5],
            _ => unreachable!(),
        };
        key.name = FixedStr::<24>::try_from_str(&ck.2).unwrap_or_default();
        key.account_id = crate::proto::AccountId(ck.3 as u32);
        key.char_id = crate::proto::CharId(ck.0 as u32);
        key.char_num = ck.1 as u8;
        cd.sex = Sex(v(3) as u8);
        cd.species = crate::proto::Species(v(4) as u16);
        cd.base_level = v(5) as u8;
        cd.job_level = v(6) as u8;
        cd.base_exp = v(7) as i32;
        cd.job_exp = v(8) as i32;
        cd.zeny = v(9) as i32;
        cd.hp = v(10) as i32;
        cd.max_hp = v(11) as i32;
        cd.sp = v(12) as i32;
        cd.max_sp = v(13) as i32;
        for i in 0..6 {
            cd.attrs[i] = v(14 + i) as i16;
        }
        cd.status_point = v(20) as i16;
        cd.skill_point = v(21) as i16;
        cd.option = crate::proto::Opt0(v(22) as u16);
        cd.karma = v(23) as i16;
        cd.manner = v(24) as i16;
        cd.party_id = crate::proto::PartyId(v(25) as u32);
        cd.hair = v(26) as i16;
        cd.hair_color = v(27) as i16;
        cd.clothes_color = v(28) as i16;
        cd.weapon = crate::proto::ItemLook(v(29) as u16);
        cd.shield = ItemNameId(v(30) as u32);
        cd.head_top = ItemNameId(v(31) as u32);
        cd.head_mid = ItemNameId(v(32) as u32);
        cd.head_bottom = ItemNameId(v(33) as u32);
        cd.last_point = crate::proto::Point {
            map_: FixedStr::<16>::try_from_str(&last_map).unwrap_or_default(),
            x: v(35) as i16,
            y: v(36) as i16,
        };
        cd.save_point = crate::proto::Point {
            map_: FixedStr::<16>::try_from_str(&save_map).unwrap_or_default(),
            x: v(38) as i16,
            y: v(39) as i16,
        };
        cd.partner_id = crate::proto::CharId(v(40) as u32);
    }
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
                    str: FixedStr::<32>::try_from_str(&name).unwrap_or_default(),
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
                str: FixedStr::<32>::try_from_str(&name).unwrap_or_default(),
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

fn save_character_tx(tx: &Transaction<'_>, key: &CharKey, cd: &CharData) -> rusqlite::Result<()> {
    let char_id = key.char_id.0 as i64;
    tx.execute(
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
        params![
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
        ],
    )?;
    tx.execute("DELETE FROM character_items WHERE char_id=?1", [char_id])?;
    {
        let mut st = tx.prepare(
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
    tx.execute("DELETE FROM character_skills WHERE char_id=?1", [char_id])?;
    {
        let mut st = tx.prepare(
            "INSERT INTO character_skills(char_id,skill_id,level,flags)
             VALUES(?1,?2,?3,?4)",
        )?;
        for (i, sk) in cd.skill.iter().enumerate() {
            if sk.lv != 0 {
                st.execute(params![char_id, i as i64, sk.lv as i64, sk.flags.0 as i64])?;
            }
        }
    }
    tx.execute("DELETE FROM character_vars WHERE char_id=?1", [char_id])?;
    {
        let mut st =
            tx.prepare("INSERT INTO character_vars(char_id,name,value) VALUES(?1,?2,?3)")?;
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
    tx.execute(
        "DELETE FROM account_vars WHERE account_id=?1",
        [key.account_id.0 as i64],
    )?;
    {
        let mut st = tx.prepare(
            "INSERT INTO account_vars(account_id,scope,name,value)
             VALUES(?1,?2,?3,?4)",
        )?;
        for (scope, regs, num) in [
            (1i64, &cd.account_reg[..], cd.account_reg_num),
            (2i64, &cd.account_reg2[..], cd.account_reg2_num),
        ] {
            for reg in regs.iter().take(num as usize) {
                if reg.str.as_bytes().is_empty() {
                    continue;
                }
                st.execute(params![
                    key.account_id.0 as i64,
                    scope,
                    reg.str.to_string_lossy(),
                    reg.value as i64
                ])?;
            }
        }
    }
    Ok(())
}
