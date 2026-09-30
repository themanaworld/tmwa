//! `tmwa-gate import`: import tmwa's flat save files into SQLite.
//!
//! Follows the readers in src/login/login.cpp (account.txt),
//! src/char/char.cpp (athena.txt), src/char/int_party.cpp (party.txt),
//! src/char/int_storage.cpp (storage.txt) and src/char/inter.cpp
//! (accreg.txt): where tmwa skips or fixes a line on load, we do the
//! same and count it.

use std::collections::HashSet;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicUsize, Ordering};

use crate::auth::password;
use crate::db::Db;

/// Where each input file lives; all default to `<save_dir>/<name>.txt`.
#[derive(Debug, Clone)]
pub struct ImportFiles {
    pub save_dir: PathBuf,
    pub account_txt: Option<PathBuf>,
    pub athena_txt: Option<PathBuf>,
    pub party_txt: Option<PathBuf>,
    pub storage_txt: Option<PathBuf>,
    pub accreg_txt: Option<PathBuf>,
}

impl ImportFiles {
    fn resolve(&self, name: &str, o: &Option<PathBuf>) -> PathBuf {
        o.clone().unwrap_or_else(|| self.save_dir.join(name))
    }
    pub fn account_txt(&self) -> PathBuf {
        self.resolve("account.txt", &self.account_txt)
    }
    pub fn athena_txt(&self) -> PathBuf {
        self.resolve("athena.txt", &self.athena_txt)
    }
    pub fn party_txt(&self) -> PathBuf {
        self.resolve("party.txt", &self.party_txt)
    }
    pub fn storage_txt(&self) -> PathBuf {
        self.resolve("storage.txt", &self.storage_txt)
    }
    pub fn accreg_txt(&self) -> PathBuf {
        self.resolve("accreg.txt", &self.accreg_txt)
    }
}

#[derive(Debug, Default)]
pub struct ImportSummary {
    pub accounts: usize,
    pub characters: usize,
    pub parties: usize,
    pub storage_entries: usize,
    pub vars: usize,
    pub skipped: Vec<String>,
    pub password_seconds: f64,
}

#[derive(Debug, thiserror::Error)]
pub enum ImportError {
    #[error("io {path}: {err}")]
    Io { path: PathBuf, err: std::io::Error },
    #[error("db: {0}")]
    Db(#[from] crate::db::DbError),
    #[error("database is not empty; refusing to import")]
    NotEmpty,
}

fn is_comment(line: &str) -> bool {
    line.starts_with("//")
}

fn parse_i64(s: &str) -> Option<i64> {
    s.trim().parse().ok()
}

fn parse_u32(s: &str) -> Option<u32> {
    s.trim().parse().ok()
}

fn parse_i32(s: &str) -> Option<i32> {
    s.trim().parse().ok()
}

fn parse_i16(s: &str) -> Option<i16> {
    s.trim().parse().ok()
}

fn parse_u16(s: &str) -> Option<u16> {
    s.trim().parse().ok()
}

fn parse_u8(s: &str) -> Option<u8> {
    s.trim().parse().ok()
}

/// Parse "YYYY-MM-DD HH:MM:SS.mmm" (tmwa writes local time; we read it
/// as UTC, which is correct on hosts running UTC).
fn parse_lastlogin(s: &str) -> Option<i64> {
    if s == "-" {
        return None;
    }
    let (date, time) = s.split_once(' ')?;
    let mut d = date.split('-');
    let y: i64 = parse_i64(d.next()?)?;
    let mo: i64 = parse_i64(d.next()?)?;
    let day: i64 = parse_i64(d.next()?)?;
    let (hms, ms) = time.split_once('.')?;
    let mut t = hms.split(':');
    let h: i64 = parse_i64(t.next()?)?;
    let mi: i64 = parse_i64(t.next()?)?;
    let sec: i64 = parse_i64(t.next()?)?;
    let ms = parse_i64(ms.get(..3)?)?;
    // days-from-civil (Howard Hinnant)
    let y = if mo <= 2 { y - 1 } else { y };
    let era = if y >= 0 { y } else { y - 399 } / 400;
    let yoe = y - era * 400;
    let mp = (mo + 9) % 12;
    let doy = (153 * mp + 2) / 5 + day - 1;
    let doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    let days = era * 146097 + doe - 719468;
    Some(days * 86_400_000 + h * 3_600_000 + mi * 60_000 + sec * 1000 + ms)
}

/// An account's trailing space-separated `name,value` register list.
fn parse_vars(s: &str, max: usize) -> Result<Vec<(String, i32)>, ()> {
    let mut vars = Vec::new();
    if s.trim().is_empty() {
        return Ok(vars);
    }
    for tok in s.split(' ') {
        if tok.is_empty() {
            continue;
        }
        let Some((name, val)) = tok.split_once(',') else {
            return Err(());
        };
        let Some(value) = parse_i32(val) else {
            return Err(());
        };
        vars.push((name.to_string(), value));
    }
    if vars.len() > max {
        return Err(());
    }
    Ok(vars)
}

/// Inventory/storage item record (nameid, amount, equip) from a
/// comma-separated item tuple; the leading 0 and the trailing
/// identify/refine/attribute/cards/broken fields are ignored.
fn parse_item(tok: &str) -> Option<(u32, i16, u16)> {
    let parts: Vec<&str> = tok.split(',').collect();
    if parts.len() != 11 && parts.len() != 12 {
        return None;
    }
    let nameid = parse_u32(parts[1])?;
    let amount = parse_i16(parts[2])?;
    let equip = parse_u16(parts[3])?;
    Some((nameid, amount, equip))
}

fn parse_items(s: &str, max: usize) -> Result<Vec<(u32, i16, u16)>, ()> {
    let mut items = Vec::new();
    if s.trim().is_empty() {
        return Ok(items);
    }
    for tok in s.split(' ') {
        if tok.is_empty() {
            continue;
        }
        let Some(it) = parse_item(tok) else {
            return Err(());
        };
        items.push(it);
    }
    if items.len() > max {
        return Err(());
    }
    Ok(items)
}

/// `id,lv|flags<<16` skill record.
fn parse_skills(s: &str, max_skill: usize) -> Result<Vec<(usize, u16, u16)>, ()> {
    let mut out = Vec::new();
    if s.trim().is_empty() {
        return Ok(out);
    }
    for tok in s.split(' ') {
        if tok.is_empty() {
            continue;
        }
        let Some((id_s, lv_s)) = tok.split_once(',') else {
            return Err(());
        };
        let Some(id) = parse_u32(id_s) else {
            return Err(());
        };
        let Some(fl) = parse_u32(lv_s) else {
            return Err(());
        };
        if id as usize >= max_skill {
            return Err(());
        }
        out.push((id as usize, (fl & 0xffff) as u16, (fl >> 16) as u16));
    }
    Ok(out)
}

fn sex_char(c: Option<&str>) -> Option<u8> {
    // tmwa's sex_from_char: F=0 M=1 N=3, else UNSPECIFIED(2)
    match c {
        Some("F") => Some(0),
        Some("M") => Some(1),
        Some("N") => Some(3),
        Some("S") => Some(2),
        _ => None,
    }
}

fn valid_email(s: &str) -> bool {
    // port of src/high/utils.cpp e_mail_check
    if s.len() < 3 || s.len() > 39 {
        return false;
    }
    let Some(at) = s.find('@') else {
        return false;
    };
    let (user, host) = s.split_at(at);
    let host = &host[1..];
    if user.is_empty() || host.is_empty() {
        return false;
    }
    if host.contains('@') {
        return false;
    }
    if host.starts_with('.') || host.ends_with('.') {
        return false;
    }
    if host.contains("..") {
        return false;
    }
    if s.contains(' ') || s.contains(';') {
        return false;
    }
    s.is_ascii()
}

const DEFAULT_EMAIL: &str = "a@a.com";
const ACCOUNT_REG_NUM: usize = 16;
const ACCOUNT_REG2_NUM: usize = 16;
const GLOBAL_REG_NUM: usize = 96;
const MAX_INVENTORY: usize = 100;
const MAX_STORAGE: usize = 500;
const MAX_PARTY: usize = 120;
const MAX_SKILL: usize = 474;

struct ParsedAccount {
    id: i64,
    name: String,
    pass_raw: String,
    last_login: Option<i64>,
    sex: u8,
    login_count: i64,
    state: i64,
    email: String,
    error_message: Option<String>,
    ip: Option<String>,
    memo: String,
    ban_until: i64,
    reg2: Vec<(String, i32)>,
}

struct ParsedChar {
    id: i64,
    account_id: i64,
    slot: i64,
    name: String,
    sex: u8,
    species: u16,
    base_level: u8,
    job_level: u8,
    base_exp: i32,
    job_exp: i32,
    zeny: i32,
    hp: i32,
    max_hp: i32,
    sp: i32,
    max_sp: i32,
    attrs: [i16; 6],
    status_point: i16,
    skill_point: i16,
    option: u16,
    karma: i16,
    manner: i16,
    party_id: u32,
    hair: i16,
    hair_color: i16,
    clothes_color: i16,
    weapon: u16,
    shield: u32,
    head_top: u32,
    head_mid: u32,
    head_bottom: u32,
    last_map: String,
    last_x: i16,
    last_y: i16,
    save_map: String,
    save_x: i16,
    save_y: i16,
    partner_id: u32,
    items: Vec<(u32, i16, u16)>,
    skills: Vec<(usize, u16, u16)>,
    vars: Vec<(String, i32)>,
}

fn csv(f: &str, n: usize) -> Option<Vec<&str>> {
    let v: Vec<&str> = f.split(',').collect();
    if v.len() == n { Some(v) } else { None }
}

fn parse_account(line: &str, seen: &mut (HashSet<i64>, HashSet<String>)) -> Option<ParsedAccount> {
    let f: Vec<&str> = line.split('\t').collect();
    if f.len() < 14 {
        return None;
    }
    let id = parse_i64(f[0])?;
    let name = f[1].to_string();
    let pass_raw = f[2].to_string();
    let last_login = parse_lastlogin(f[3]);
    let sex = sex_char(f.get(4).copied())?;
    let login_count = parse_i64(f[5])?;
    let state = parse_i64(f[6])?;
    let email = f[7].to_string();
    let error_message = f[8].to_string();
    let _conn_until = parse_i64(f[9]);
    let ip = f[10].to_string();
    let memo = f[11].to_string();
    let ban_until = parse_i64(f[12])?;
    let reg2 = parse_vars(f.get(13).copied().unwrap_or(""), ACCOUNT_REG2_NUM).ok()?;
    // tmwa: duplicate ids/names are skipped
    if !seen.0.insert(id) {
        return None;
    }
    if !seen.1.insert(name.clone()) {
        seen.0.remove(&id);
        return None;
    }
    // tmwa: invalid email -> a@a.com; error_message forced to "-"
    // unless state == 7.
    let email = if valid_email(&email) {
        email
    } else {
        DEFAULT_EMAIL.into()
    };
    let error_message = if state == 7 && !error_message.is_empty() {
        Some(error_message)
    } else {
        None
    };
    let ip = if ip == "-" || ip.is_empty() {
        None
    } else {
        Some(ip)
    };
    Some(ParsedAccount {
        id,
        name,
        pass_raw,
        last_login,
        sex,
        login_count,
        state,
        email,
        error_message,
        ip,
        memo,
        ban_until,
        reg2,
    })
}

fn parse_char(line: &str, seen: &mut (HashSet<i64>, HashSet<String>)) -> Option<ParsedChar> {
    let f: Vec<&str> = line.split('\t').collect();
    if f.len() < 16 {
        return None;
    }
    let id = parse_i64(f[0])?;
    let acct = csv(f[1], 2)?;
    let account_id = parse_i64(acct[0])?;
    let slot = parse_i64(acct[1])?;
    let name = f[2].to_string();
    let spc = csv(f[3], 3)?;
    let (species, base_level, job_level) =
        (parse_u16(spc[0])?, parse_u8(spc[1])?, parse_u8(spc[2])?);
    let exp = csv(f[4], 3)?;
    let (base_exp, job_exp, zeny) = (parse_i32(exp[0])?, parse_i32(exp[1])?, parse_i32(exp[2])?);
    let hpv = csv(f[5], 4)?;
    let (hp, max_hp, sp, max_sp) = (
        parse_i32(hpv[0])?,
        parse_i32(hpv[1])?,
        parse_i32(hpv[2])?,
        parse_i32(hpv[3])?,
    );
    let at = csv(f[6], 6)?;
    let mut attrs = [0i16; 6];
    for i in 0..6 {
        attrs[i] = parse_i16(at[i])?;
    }
    let pts = csv(f[7], 2)?;
    let (status_point, skill_point) = (parse_i16(pts[0])?, parse_i16(pts[1])?);
    let okm = csv(f[8], 3)?;
    let (option, karma, manner) = (parse_u16(okm[0])?, parse_i16(okm[1])?, parse_i16(okm[2])?);
    let pgp = csv(f[9], 3)?;
    let party_id = parse_u32(pgp[0])?;
    let look = csv(f[10], 3)?;
    let hair_style = look[0];
    let (hair_color, clothes_color) = (parse_i16(look[1])?, parse_i16(look[2])?);
    let eq = csv(f[11], 5)?;
    let (weapon, shield, head_top, head_mid, head_bottom) = (
        parse_u16(eq[0])?,
        parse_u32(eq[1])?,
        parse_u32(eq[2])?,
        parse_u32(eq[3])?,
        parse_u32(eq[4])?,
    );
    let last = csv(f[12], 3)?;
    let (last_map, last_x, last_y) = (
        last[0].to_string(),
        parse_i16(last[1])?,
        parse_i16(last[2])?,
    );
    let save = csv(f[13], 4)?;
    let (save_map, save_x, save_y, partner_id) = (
        save[0].to_string(),
        parse_i16(save[1])?,
        parse_i16(save[2])?,
        parse_u32(save[3])?,
    );
    let sex = sex_char(f.get(14).copied());
    let items = parse_items(f.get(15).copied().unwrap_or(""), MAX_INVENTORY).ok()?;
    let _cart = f.get(16);
    let skills = parse_skills(f.get(17).copied().unwrap_or(""), MAX_SKILL).ok()?;
    let vars = parse_vars(f.get(18).copied().unwrap_or(""), GLOBAL_REG_NUM).ok()?;

    // tmwa: leftover Platinum corruption "-1" means hair 0
    let hair = if hair_style == "-1" {
        0
    } else {
        parse_i16(hair_style)?
    };
    // tmwa: WISP_SERVER_NAME is refused; ours is "_Server_"? use wisp
    if name == "#wisp#" {
        return None;
    }
    if !seen.0.insert(id) {
        return None;
    }
    if !seen.1.insert(name.clone()) {
        seen.0.remove(&id);
        return None;
    }
    Some(ParsedChar {
        id,
        account_id,
        slot,
        name,
        sex: sex.unwrap_or(2),
        species,
        base_level,
        job_level,
        base_exp,
        job_exp,
        zeny,
        hp,
        max_hp,
        sp,
        max_sp,
        attrs,
        status_point,
        skill_point,
        option,
        karma,
        manner,
        party_id,
        hair,
        hair_color,
        clothes_color,
        weapon,
        shield,
        head_top,
        head_mid,
        head_bottom,
        last_map,
        last_x,
        last_y,
        save_map,
        save_x,
        save_y,
        partner_id,
        items,
        skills,
        vars,
    })
}

struct ParsedParty {
    id: i64,
    name: String,
    exp: i32,
    item: i32,
    members: Vec<(i64, String, i64)>, // account_id, name, leader
}

fn parse_party(line: &str) -> Option<ParsedParty> {
    let mut bits = line.split('\t');
    let id = parse_i64(bits.next()?)?;
    let name = bits.next()?.to_string();
    let eic = csv(bits.next()?, 2)?;
    let (exp, item) = (parse_i32(eic[0])?, parse_i32(eic[1])?);
    let mut members = Vec::new();
    while let Some(a) = bits.next() {
        // trailing tab produces an empty tail field; treat as end
        if a.is_empty() {
            break;
        }
        let b = bits.next()?;
        let al = csv(a, 2)?;
        let account_id = parse_i64(al[0])?;
        let leader = parse_i64(al[1])?;
        if account_id != 0 {
            members.push((account_id, b.to_string(), leader));
        }
        if members.len() >= MAX_PARTY {
            break;
        }
    }
    Some(ParsedParty {
        id,
        name,
        exp,
        item,
        members,
    })
}

struct ParsedStorage {
    account_id: i64,
    items: Vec<(u32, i16, u16)>,
}

fn parse_storage(line: &str) -> Option<ParsedStorage> {
    let f: Vec<&str> = line.split('\t').collect();
    let head = csv(f[0], 2)?;
    let account_id = parse_i64(head[0])?;
    if account_id == 0 {
        return None;
    }
    let _amount = parse_i64(head[1])?;
    let items = parse_items(f.get(1).copied().unwrap_or(""), MAX_STORAGE).ok()?;
    Some(ParsedStorage { account_id, items })
}

struct ParsedAccreg {
    account_id: i64,
    vars: Vec<(String, i32)>,
}

fn parse_accreg(line: &str) -> Option<ParsedAccreg> {
    let f: Vec<&str> = line.split('\t').collect();
    let account_id = parse_i64(f[0])?;
    if account_id == 0 {
        return None;
    }
    let vars = parse_vars(f.get(1).copied().unwrap_or(""), ACCOUNT_REG_NUM).ok()?;
    Some(ParsedAccreg { account_id, vars })
}

/// `id\t%newid%` marker lines.
fn newid_line(line: &str) -> Option<i64> {
    let f: Vec<&str> = line.split('\t').collect();
    if f.len() == 2 && f[1] == "%newid%" {
        parse_i64(f[0])
    } else {
        None
    }
}

/// Run the import. `progress` is called with human-readable status.
pub fn run(
    files: &ImportFiles,
    db: &Db,
    mut progress: impl FnMut(&str),
) -> Result<ImportSummary, ImportError> {
    if !db.is_empty()? {
        return Err(ImportError::NotEmpty);
    }
    let mut sum = ImportSummary::default();
    let mut skipped = Vec::new();

    let read = |p: &Path| -> Result<String, ImportError> {
        fs::read_to_string(p).map_err(|err| ImportError::Io {
            path: p.to_path_buf(),
            err,
        })
    };

    // ---- accounts ----
    let mut accounts = Vec::new();
    let mut account_sex: std::collections::HashMap<i64, u8> = std::collections::HashMap::new();
    let mut next_account_id = 0i64;
    let mut seen = (HashSet::new(), HashSet::new());
    for (i, line) in read(&files.account_txt())?.lines().enumerate() {
        if is_comment(line) {
            continue;
        }
        if let Some(id) = newid_line(line) {
            next_account_id = next_account_id.max(id);
            continue;
        }
        match parse_account(line, &mut seen) {
            Some(a) => {
                account_sex.insert(a.id, a.sex);
                next_account_id = next_account_id.max(a.id + 1);
                accounts.push(a);
            }
            None => skipped.push(format!("account.txt:{}", i + 1)),
        }
    }

    // ---- characters ----
    let mut chars = Vec::new();
    let mut next_char_id = 0i64;
    let mut seen = (HashSet::new(), HashSet::new());
    for (i, line) in read(&files.athena_txt())?.lines().enumerate() {
        if is_comment(line) {
            continue;
        }
        if let Some(id) = newid_line(line) {
            next_char_id = next_char_id.max(id);
            continue;
        }
        match parse_char(line, &mut seen) {
            Some(mut c) => {
                // account sex fallback (mmo_char_tostr)
                if c.sex == 2
                    && let Some(&s) = account_sex.get(&c.account_id)
                {
                    c.sex = s;
                }
                next_char_id = next_char_id.max(c.id + 1);
                chars.push(c);
            }
            None => skipped.push(format!("athena.txt:{}", i + 1)),
        }
    }

    // ---- parties ----
    let mut parties = Vec::new();
    let mut next_party_id = 0i64;
    for (i, line) in read(&files.party_txt())?.lines().enumerate() {
        if is_comment(line) {
            continue;
        }
        if let Some(id) = newid_line(line) {
            next_party_id = next_party_id.max(id);
            continue;
        }
        match parse_party(line) {
            Some(p) => {
                next_party_id = next_party_id.max(p.id + 1);
                parties.push(p);
            }
            None => skipped.push(format!("party.txt:{}", i + 1)),
        }
    }

    // ---- storage ----
    let mut storage = Vec::new();
    for (i, line) in read(&files.storage_txt())?.lines().enumerate() {
        if is_comment(line) {
            continue;
        }
        match parse_storage(line) {
            Some(s) => storage.push(s),
            None => skipped.push(format!("storage.txt:{}", i + 1)),
        }
    }

    // ---- accreg ----
    let mut accreg = Vec::new();
    for (i, line) in read(&files.accreg_txt())?.lines().enumerate() {
        if is_comment(line) {
            continue;
        }
        match parse_accreg(line) {
            Some(a) => accreg.push(a),
            None => skipped.push(format!("accreg.txt:{}", i + 1)),
        }
    }

    // ---- hash passwords in parallel ----
    progress(&format!("hashing {} passwords...", accounts.len()));
    let t0 = std::time::Instant::now();
    let index = AtomicUsize::new(0);
    type HashOut = Option<(String, &'static str, Option<String>)>;
    let results: Vec<std::sync::Mutex<HashOut>> = (0..accounts.len())
        .map(|_| std::sync::Mutex::new(None))
        .collect();
    let threads = std::thread::available_parallelism()
        .map(|n| n.get())
        .unwrap_or(4)
        .min(16);
    std::thread::scope(|s| {
        for _ in 0..threads {
            s.spawn(|| {
                loop {
                    let i = index.fetch_add(1, Ordering::Relaxed);
                    if i >= accounts.len() {
                        break;
                    }
                    let a = &accounts[i];
                    // tmwa (login.cpp impl_extract): plaintext iff the
                    // pass does not start with '!' AND the memo starts
                    // with '-'; anything else is wrapped like a legacy
                    // entry (and never verifies if it isn't one).
                    let r = if !a.pass_raw.starts_with('!') && a.memo.starts_with('-') {
                        let h = password::hash_argon2id(a.pass_raw.as_bytes())
                            .expect("argon2 hash failed");
                        (h, "argon2id", None)
                    } else {
                        let salt = password::legacy_salt(&a.pass_raw).unwrap_or("").to_string();
                        let h = password::wrap_legacy(&a.pass_raw).expect("argon2 wrap failed");
                        (h, "argon2id-md5", Some(salt))
                    };
                    *results[i].lock().unwrap() = Some(r);
                }
            });
        }
    });
    sum.password_seconds = t0.elapsed().as_secs_f64();
    progress(&format!(
        "hashed {} passwords in {:.2}s",
        accounts.len(),
        sum.password_seconds
    ));

    // ---- write in one transaction ----
    progress("writing database...");
    db.with_conn(|conn| {
        let tx = conn.transaction()?;
        for (i, a) in accounts.iter().enumerate() {
            let (hash, scheme, salt) = results[i].lock().unwrap().take().unwrap();
            let email = if a.email == DEFAULT_EMAIL {
                None
            } else {
                Some(a.email.as_str())
            };
            // plaintext migrations store '!' in memo like tmwa does
            let memo = if scheme == "argon2id" && a.memo.starts_with('-') {
                "!"
            } else {
                &a.memo
            };
            Db::insert_account(
                &tx,
                a.id,
                &a.name,
                &hash,
                scheme,
                salt.as_deref(),
                email,
                a.state,
                a.error_message.as_deref(),
                a.ban_until,
                memo,
                a.last_login,
                a.login_count,
                a.ip.as_deref(),
                0,
            )?;
            for (name, value) in &a.reg2 {
                tx.execute(
                    "INSERT INTO account_vars(account_id,scope,name,value)
                     VALUES(?1,2,?2,?3)",
                    rusqlite::params![a.id, name, value],
                )?;
            }
            sum.accounts += 1;
        }
        for c in &chars {
            write_char(&tx, c)?;
            sum.characters += 1;
        }
        for p in &parties {
            tx.execute(
                "INSERT INTO parties(id,name,exp_share,item_share)
                 VALUES(?1,?2,?3,?4)",
                rusqlite::params![p.id, p.name, p.exp, p.item],
            )?;
            for (acct, name, leader) in &p.members {
                tx.execute(
                    "INSERT INTO party_members(party_id,account_id,char_name,leader)
                     VALUES(?1,?2,?3,?4)",
                    rusqlite::params![p.id, acct, name, leader],
                )?;
            }
            sum.parties += 1;
        }
        for s in &storage {
            for (idx, (item_id, amount, equip)) in s.items.iter().enumerate() {
                tx.execute(
                    "INSERT INTO storage_items(account_id,idx,item_id,amount,equip)
                     VALUES(?1,?2,?3,?4,?5)",
                    rusqlite::params![s.account_id, idx as i64, item_id, amount, equip],
                )?;
            }
            if !s.items.is_empty() {
                sum.storage_entries += s.items.len();
            }
        }
        for a in &accreg {
            for (name, value) in &a.vars {
                tx.execute(
                    "INSERT INTO account_vars(account_id,scope,name,value)
                     VALUES(?1,1,?2,?3)",
                    rusqlite::params![a.account_id, name, value],
                )?;
            }
            sum.vars += a.vars.len();
        }
        tx.execute(
            "INSERT INTO meta(key,value) VALUES('next_account_id',?1)",
            [next_account_id],
        )?;
        tx.execute(
            "INSERT INTO meta(key,value) VALUES('next_char_id',?1)",
            [next_char_id],
        )?;
        tx.execute(
            "INSERT INTO meta(key,value) VALUES('next_party_id',?1)",
            [next_party_id],
        )?;
        tx.commit()
    })?;
    sum.skipped = skipped;
    Ok(sum)
}

fn write_char(tx: &rusqlite::Transaction<'_>, c: &ParsedChar) -> rusqlite::Result<()> {
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
                ?28,?29,?30,?31,?32,?33,?34,?35,?36,?37,?38,?39,?40,?41,?42)",
        rusqlite::params![
            c.id,
            c.account_id,
            c.slot,
            c.name,
            c.sex as i64,
            c.species as i64,
            c.base_level as i64,
            c.job_level as i64,
            c.base_exp,
            c.job_exp,
            c.zeny,
            c.hp,
            c.max_hp,
            c.sp,
            c.max_sp,
            c.attrs[0] as i64,
            c.attrs[1] as i64,
            c.attrs[2] as i64,
            c.attrs[3] as i64,
            c.attrs[4] as i64,
            c.attrs[5] as i64,
            c.status_point as i64,
            c.skill_point as i64,
            c.option as i64,
            c.karma as i64,
            c.manner as i64,
            c.party_id as i64,
            c.hair as i64,
            c.hair_color as i64,
            c.clothes_color as i64,
            c.weapon as i64,
            c.shield as i64,
            c.head_top as i64,
            c.head_mid as i64,
            c.head_bottom as i64,
            c.last_map,
            c.last_x as i64,
            c.last_y as i64,
            c.save_map,
            c.save_x as i64,
            c.save_y as i64,
            c.partner_id as i64,
        ],
    )?;
    {
        let mut st = tx.prepare(
            "INSERT INTO character_items(char_id,idx,item_id,amount,equip)
             VALUES(?1,?2,?3,?4,?5)",
        )?;
        for (idx, (item_id, amount, equip)) in c.items.iter().enumerate() {
            st.execute(rusqlite::params![c.id, idx as i64, item_id, amount, equip])?;
        }
    }
    {
        let mut st = tx.prepare(
            "INSERT INTO character_skills(char_id,skill_id,level,flags)
             VALUES(?1,?2,?3,?4)",
        )?;
        for (id, lv, flags) in &c.skills {
            st.execute(rusqlite::params![
                c.id,
                *id as i64,
                *lv as i64,
                *flags as i64
            ])?;
        }
    }
    {
        let mut st =
            tx.prepare("INSERT INTO character_vars(char_id,name,value) VALUES(?1,?2,?3)")?;
        for (name, value) in &c.vars {
            st.execute(rusqlite::params![c.id, name, value])?;
        }
    }
    Ok(())
}
