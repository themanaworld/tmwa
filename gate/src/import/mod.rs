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

use crate::auth::password::{self, Scheme};
use crate::db::Db;
use crate::proto::types::FixedStr;
use crate::proto::{
    AccountId, CharData, CharId, CharKey, Epos, GlobalReg, Item, ItemLook, ItemNameId, Opt0,
    PartyId, Point, Sex, SkillFlags, SkillValue, Species,
};

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

fn parse<T: std::str::FromStr>(s: &str) -> Option<T> {
    s.trim().parse().ok()
}

/// Parse "YYYY-MM-DD HH:MM:SS.mmm" (tmwa writes local time; we read it
/// as UTC, which is correct on hosts running UTC).
fn parse_lastlogin(s: &str) -> Option<i64> {
    if s == "-" {
        return None;
    }
    let t = chrono::NaiveDateTime::parse_from_str(s, "%Y-%m-%d %H:%M:%S%.3f").ok()?;
    Some(t.and_utc().timestamp_millis())
}

/// Split a space-separated field into parsed tokens, skipping
/// empties; fails on a bad token or more than `max` entries.
fn parse_list<T>(s: &str, max: usize, mut f: impl FnMut(&str) -> Option<T>) -> Result<Vec<T>, ()> {
    let mut out = Vec::new();
    if s.trim().is_empty() {
        return Ok(out);
    }
    for tok in s.split(' ') {
        if tok.is_empty() {
            continue;
        }
        let Some(v) = f(tok) else {
            return Err(());
        };
        out.push(v);
    }
    if out.len() > max {
        return Err(());
    }
    Ok(out)
}

/// An account's trailing space-separated `name,value` register list.
fn parse_vars(s: &str, max: usize) -> Result<Vec<(String, i32)>, ()> {
    parse_list(s, max, |tok| {
        let (name, val) = tok.split_once(',')?;
        let value = parse::<i32>(val)?;
        Some((name.to_string(), value))
    })
}

/// Inventory/storage item record (nameid, amount, equip) from a
/// comma-separated item tuple; the leading 0 and the trailing
/// identify/refine/attribute/cards/broken fields are ignored.
fn parse_item(tok: &str) -> Option<(u32, i16, u16)> {
    let parts: Vec<&str> = tok.split(',').collect();
    if parts.len() != 11 && parts.len() != 12 {
        return None;
    }
    let nameid = parse::<u32>(parts[1])?;
    let amount = parse::<i16>(parts[2])?;
    let equip = parse::<u16>(parts[3])?;
    Some((nameid, amount, equip))
}

fn parse_items(s: &str, max: usize) -> Result<Vec<(u32, i16, u16)>, ()> {
    parse_list(s, max, parse_item)
}

/// `id,lv|flags<<16` skill record. `max_skill` bounds the skill id,
/// not the list length.
fn parse_skills(s: &str, max_skill: usize) -> Result<Vec<(usize, u16, u16)>, ()> {
    parse_list(s, usize::MAX, |tok| {
        let (id_s, lv_s) = tok.split_once(',')?;
        let id = parse::<u32>(id_s)?;
        let fl = parse::<u32>(lv_s)?;
        if id as usize >= max_skill {
            return None;
        }
        Some((id as usize, (fl & 0xffff) as u16, (fl >> 16) as u16))
    })
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

impl ParsedChar {
    /// Build the generated wire types, so the DB write goes through
    /// `db::save_character_conn` like a live 0x2b01 save.
    fn to_proto(&self) -> (CharKey, CharData) {
        let key = CharKey {
            name: FixedStr::<24>::from_str_truncate(&self.name),
            account_id: AccountId(self.account_id as u32),
            char_id: CharId(self.id as u32),
            char_num: self.slot as u8,
        };
        let mut cd = CharData {
            sex: Sex(self.sex),
            species: Species(self.species),
            base_level: self.base_level,
            job_level: self.job_level,
            base_exp: self.base_exp,
            job_exp: self.job_exp,
            zeny: self.zeny,
            hp: self.hp,
            max_hp: self.max_hp,
            sp: self.sp,
            max_sp: self.max_sp,
            attrs: self.attrs,
            status_point: self.status_point,
            skill_point: self.skill_point,
            option: Opt0(self.option),
            karma: self.karma,
            manner: self.manner,
            party_id: PartyId(self.party_id),
            hair: self.hair,
            hair_color: self.hair_color,
            clothes_color: self.clothes_color,
            weapon: ItemLook(self.weapon),
            shield: ItemNameId(self.shield),
            head_top: ItemNameId(self.head_top),
            head_mid: ItemNameId(self.head_mid),
            head_bottom: ItemNameId(self.head_bottom),
            last_point: Point {
                map_: FixedStr::<16>::from_str_truncate(&self.last_map),
                x: self.last_x,
                y: self.last_y,
            },
            save_point: Point {
                map_: FixedStr::<16>::from_str_truncate(&self.save_map),
                x: self.save_x,
                y: self.save_y,
            },
            partner_id: CharId(self.partner_id),
            ..Default::default()
        };
        for (slot, &(nameid, amount, equip)) in self.items.iter().enumerate() {
            if slot < cd.inventory.len() {
                cd.inventory[slot] = Item {
                    nameid: ItemNameId(nameid),
                    amount,
                    equip: Epos(equip),
                };
            }
        }
        for &(id, lv, flags) in &self.skills {
            if id < cd.skill.len() {
                cd.skill[id] = SkillValue {
                    lv,
                    flags: SkillFlags(flags),
                };
            }
        }
        // character_vars is keyed on (char_id, name): keep the first
        // occurrence of a duplicated name
        let mut seen = HashSet::new();
        for (name, value) in &self.vars {
            if !seen.insert(name) {
                continue;
            }
            let i = cd.global_reg_num as usize;
            if i < cd.global_reg.len() {
                cd.global_reg[i] = GlobalReg {
                    str: FixedStr::<32>::from_str_truncate(name),
                    value: *value,
                };
                cd.global_reg_num += 1;
            }
        }
        (key, cd)
    }
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
    let id = parse::<i64>(f[0])?;
    let name = f[1].to_string();
    let pass_raw = f[2].to_string();
    let last_login = parse_lastlogin(f[3]);
    let sex = sex_char(f.get(4).copied())?;
    let login_count = parse::<i64>(f[5])?;
    let state = parse::<i64>(f[6])?;
    let email = f[7].to_string();
    let error_message = f[8].to_string();
    let _conn_until = parse::<i64>(f[9]);
    let ip = f[10].to_string();
    let memo = f[11].to_string();
    let ban_until = parse::<i64>(f[12])?;
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

fn parse_char(
    line: &str,
    seen: &mut (HashSet<i64>, HashSet<String>, HashSet<(i64, i64)>),
) -> Option<ParsedChar> {
    let f: Vec<&str> = line.split('\t').collect();
    if f.len() < 16 {
        return None;
    }
    let id = parse::<i64>(f[0])?;
    let acct = csv(f[1], 2)?;
    let account_id = parse::<i64>(acct[0])?;
    let slot = parse::<i64>(acct[1])?;
    let name = f[2].to_string();
    let spc = csv(f[3], 3)?;
    let (species, base_level, job_level) = (
        parse::<u16>(spc[0])?,
        parse::<u8>(spc[1])?,
        parse::<u8>(spc[2])?,
    );
    let exp = csv(f[4], 3)?;
    let (base_exp, job_exp, zeny) = (
        parse::<i32>(exp[0])?,
        parse::<i32>(exp[1])?,
        parse::<i32>(exp[2])?,
    );
    let hpv = csv(f[5], 4)?;
    let (hp, max_hp, sp, max_sp) = (
        parse::<i32>(hpv[0])?,
        parse::<i32>(hpv[1])?,
        parse::<i32>(hpv[2])?,
        parse::<i32>(hpv[3])?,
    );
    let at = csv(f[6], 6)?;
    let mut attrs = [0i16; 6];
    for i in 0..6 {
        attrs[i] = parse::<i16>(at[i])?;
    }
    let pts = csv(f[7], 2)?;
    let (status_point, skill_point) = (parse::<i16>(pts[0])?, parse::<i16>(pts[1])?);
    let okm = csv(f[8], 3)?;
    let (option, karma, manner) = (
        parse::<u16>(okm[0])?,
        parse::<i16>(okm[1])?,
        parse::<i16>(okm[2])?,
    );
    let pgp = csv(f[9], 3)?;
    let party_id = parse::<u32>(pgp[0])?;
    let look = csv(f[10], 3)?;
    let hair_style = look[0];
    let (hair_color, clothes_color) = (parse::<i16>(look[1])?, parse::<i16>(look[2])?);
    let eq = csv(f[11], 5)?;
    let (weapon, shield, head_top, head_mid, head_bottom) = (
        parse::<u16>(eq[0])?,
        parse::<u32>(eq[1])?,
        parse::<u32>(eq[2])?,
        parse::<u32>(eq[3])?,
        parse::<u32>(eq[4])?,
    );
    let last = csv(f[12], 3)?;
    let (last_map, last_x, last_y) = (
        last[0].to_string(),
        parse::<i16>(last[1])?,
        parse::<i16>(last[2])?,
    );
    let save = csv(f[13], 4)?;
    let (save_map, save_x, save_y, partner_id) = (
        save[0].to_string(),
        parse::<i16>(save[1])?,
        parse::<i16>(save[2])?,
        parse::<u32>(save[3])?,
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
        parse::<i16>(hair_style)?
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
    // the UNIQUE(account_id, slot) column pair: a second char on the
    // same slot is malformed; drop it like the dup id/name cases
    if !seen.2.insert((account_id, slot)) {
        seen.0.remove(&id);
        seen.1.remove(&name);
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
    let id = parse::<i64>(bits.next()?)?;
    let name = bits.next()?.to_string();
    let eic = csv(bits.next()?, 2)?;
    let (exp, item) = (parse::<i32>(eic[0])?, parse::<i32>(eic[1])?);
    let mut members = Vec::new();
    let mut member_accts = HashSet::new();
    while let Some(a) = bits.next() {
        // trailing tab produces an empty tail field; treat as end
        if a.is_empty() {
            break;
        }
        let b = bits.next()?;
        let al = csv(a, 2)?;
        let account_id = parse::<i64>(al[0])?;
        let leader = parse::<i64>(al[1])?;
        // party_members is keyed on (party_id, account_id); a dup
        // member in the file keeps the first occurrence
        if account_id != 0 && member_accts.insert(account_id) {
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
    let account_id = parse::<i64>(head[0])?;
    if account_id == 0 {
        return None;
    }
    let _amount = parse::<i64>(head[1])?;
    let items = parse_items(f.get(1).copied().unwrap_or(""), MAX_STORAGE).ok()?;
    Some(ParsedStorage { account_id, items })
}

struct ParsedAccreg {
    account_id: i64,
    vars: Vec<(String, i32)>,
}

fn parse_accreg(line: &str) -> Option<ParsedAccreg> {
    let f: Vec<&str> = line.split('\t').collect();
    let account_id = parse::<i64>(f[0])?;
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
        parse::<i64>(f[0])
    } else {
        None
    }
}

/// Parse a "one line per account" file (storage.txt, accreg.txt):
/// comments skipped, bad lines and duplicate account ids counted
/// in `skipped`.
fn parse_per_account<T>(
    lines: &[String],
    file: &str,
    skipped: &mut Vec<String>,
    parse_line: impl Fn(&str) -> Option<T>,
    account_id: impl Fn(&T) -> i64,
) -> Vec<T> {
    let mut seen = HashSet::new();
    let mut out = Vec::new();
    for (i, line) in lines.iter().enumerate() {
        if is_comment(line) {
            continue;
        }
        match parse_line(line) {
            Some(v) => {
                if !seen.insert(account_id(&v)) {
                    skipped.push(format!("{file}:{} (duplicate account)", i + 1));
                    continue;
                }
                out.push(v);
            }
            None => skipped.push(format!("{file}:{}", i + 1)),
        }
    }
    out
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

    // Real save files accumulate non-UTF-8 bytes (e.g. a party name
    // truncated in the middle of a multi-byte character); bad lines
    // are skipped with a warning instead of failing the import.
    let read_lines = |p: &Path, skipped: &mut Vec<String>| -> Result<Vec<String>, ImportError> {
        let bytes = fs::read(p).map_err(|err| ImportError::Io {
            path: p.to_path_buf(),
            err,
        })?;
        let name = p
            .file_name()
            .and_then(|n| n.to_str())
            .unwrap_or("?")
            .to_string();
        let mut out = Vec::new();
        let mut raw_lines: Vec<&[u8]> = bytes.split(|&b| b == b'\n').collect();
        // str::lines() doesn't yield a trailing empty element
        if raw_lines.last().is_some_and(|l| l.is_empty()) {
            raw_lines.pop();
        }
        for (i, raw) in raw_lines.iter().enumerate() {
            match String::from_utf8(raw.to_vec()) {
                Ok(mut l) => {
                    if l.ends_with('\r') {
                        l.pop();
                    }
                    out.push(l);
                }
                Err(_) => skipped.push(format!("{name}:{} (invalid utf-8)", i + 1)),
            }
        }
        Ok(out)
    };

    // ---- accounts ----
    let mut accounts = Vec::new();
    let mut account_sex: std::collections::HashMap<i64, u8> = std::collections::HashMap::new();
    let mut next_account_id = 0i64;
    let mut seen = (HashSet::new(), HashSet::new());
    for (i, line) in read_lines(&files.account_txt(), &mut skipped)?
        .iter()
        .enumerate()
    {
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
    let mut seen = (HashSet::new(), HashSet::new(), HashSet::new());
    for (i, line) in read_lines(&files.athena_txt(), &mut skipped)?
        .iter()
        .enumerate()
    {
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
    // One line per party; a repeated id or name would hit the PK or
    // the UNIQUE(name) column, so the second line is skipped.
    let mut parties = Vec::new();
    let mut next_party_id = 0i64;
    let mut seen_parties = (HashSet::new(), HashSet::new());
    for (i, line) in read_lines(&files.party_txt(), &mut skipped)?
        .iter()
        .enumerate()
    {
        if is_comment(line) {
            continue;
        }
        if let Some(id) = newid_line(line) {
            next_party_id = next_party_id.max(id);
            continue;
        }
        match parse_party(line) {
            Some(p) => {
                if !seen_parties.0.insert(p.id) || !seen_parties.1.insert(p.name.clone()) {
                    skipped.push(format!("party.txt:{} (duplicate id or name)", i + 1));
                    continue;
                }
                next_party_id = next_party_id.max(p.id);
                parties.push(p);
            }
            None => skipped.push(format!("party.txt:{}", i + 1)),
        }
    }

    // ---- storage ----
    // One line per account; a second line for the same account is
    // skipped (the PK on (account_id, idx) would otherwise abort the
    // whole transaction).
    let storage = {
        let lines = read_lines(&files.storage_txt(), &mut skipped)?;
        parse_per_account(&lines, "storage.txt", &mut skipped, parse_storage, |s| {
            s.account_id
        })
    };

    // ---- accreg ----
    // One line per account, same as storage.txt.
    let mut accreg = {
        let lines = read_lines(&files.accreg_txt(), &mut skipped)?;
        parse_per_account(&lines, "accreg.txt", &mut skipped, parse_accreg, |a| {
            a.account_id
        })
    };

    // Orphans: real saves can reference accounts that are not in
    // account.txt anymore (the row was deleted upstream, or the
    // files fell out of sync). The account_vars / characters foreign
    // keys would fail the whole import; warn and drop them instead.
    {
        let known: HashSet<i64> = accounts.iter().map(|a| a.id).collect();
        accreg.retain(|a| {
            if known.contains(&a.account_id) {
                true
            } else {
                skipped.push(format!(
                    "accreg.txt: account {} has no account row",
                    a.account_id
                ));
                false
            }
        });
        chars.retain(|c| {
            if known.contains(&c.account_id) {
                true
            } else {
                skipped.push(format!(
                    "athena.txt: char {} belongs to missing account {}",
                    c.id, c.account_id
                ));
                false
            }
        });
    }

    // ---- hash passwords in parallel ----
    progress(&format!("hashing {} passwords...", accounts.len()));
    let t0 = std::time::Instant::now();
    type HashOut = (String, Scheme, Option<String>);
    let threads = std::thread::available_parallelism()
        .map(|n| n.get())
        .unwrap_or(4)
        .min(16)
        .min(accounts.len().max(1));
    let chunk = accounts.len().div_ceil(threads).max(1);
    let results: Vec<HashOut> = std::thread::scope(|s| {
        let handles: Vec<_> = accounts
            .chunks(chunk)
            .map(|group| {
                s.spawn(move || {
                    group
                        .iter()
                        .map(|a| {
                            // tmwa (login.cpp impl_extract): plaintext
                            // iff the pass does not start with '!' AND
                            // the memo starts with '-'; anything else is
                            // wrapped like a legacy entry (and never
                            // verifies if it isn't one).
                            if !a.pass_raw.starts_with('!') && a.memo.starts_with('-') {
                                let h = password::hash_argon2id(a.pass_raw.as_bytes())
                                    .expect("argon2 hash failed");
                                (h, Scheme::Argon2id, None)
                            } else {
                                let salt =
                                    password::legacy_salt(&a.pass_raw).unwrap_or("").to_string();
                                let h =
                                    password::wrap_legacy(&a.pass_raw).expect("argon2 wrap failed");
                                (h, Scheme::Argon2idMd5, Some(salt))
                            }
                        })
                        .collect::<Vec<HashOut>>()
                })
            })
            .collect();
        handles
            .into_iter()
            .flat_map(|h| h.join().expect("hash thread panicked"))
            .collect()
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
        for (a, (hash, scheme, salt)) in accounts.iter().zip(results.iter()) {
            let email = if a.email == DEFAULT_EMAIL {
                None
            } else {
                Some(a.email.as_str())
            };
            // plaintext migrations store '!' in memo like tmwa does
            let memo = if *scheme == Scheme::Argon2id && a.memo.starts_with('-') {
                "!"
            } else {
                &a.memo
            };
            Db::insert_account(
                &tx,
                a.id,
                &a.name,
                hash,
                *scheme,
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
            let vars: Vec<(String, i64)> =
                a.reg2.iter().map(|(n, v)| (n.clone(), *v as i64)).collect();
            crate::db::set_account_vars(&tx, a.id, 2, &vars)?;
            sum.accounts += 1;
        }
        for c in &chars {
            let (key, data) = c.to_proto();
            crate::db::save_character_conn(&tx, &key, &data)?;
            sum.characters += 1;
        }
        for p in &parties {
            crate::db::upsert_party_conn(
                &tx,
                p.id,
                &p.name,
                p.exp as i64,
                p.item as i64,
                &p.members,
            )?;
            sum.parties += 1;
        }
        for s in &storage {
            let items: Vec<(i64, i64, i64)> = s
                .items
                .iter()
                .map(|&(id, n, e)| (id as i64, n as i64, e as i64))
                .collect();
            crate::db::save_storage_conn(&tx, s.account_id, &items)?;
            sum.storage_entries += s.items.len();
        }
        for a in &accreg {
            let vars: Vec<(String, i64)> =
                a.vars.iter().map(|(n, v)| (n.clone(), *v as i64)).collect();
            crate::db::set_account_vars(&tx, a.account_id, 1, &vars)?;
            sum.vars += a.vars.len();
        }
        crate::db::set_meta_conn(&tx, "next_account_id", next_account_id)?;
        crate::db::set_meta_conn(&tx, "next_char_id", next_char_id)?;
        crate::db::set_meta_conn(&tx, "next_party_id", next_party_id)?;
        tx.commit()
    })?;
    sum.skipped = skipped;
    Ok(sum)
}
