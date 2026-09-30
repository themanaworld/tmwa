// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! Client-facing session: dispatches on the first packet.
//!
//! 0x7530 version (then keep waiting), 0x0064 login, 0x0065 char
//! screen, 0x0072 map relay; anything else drops the connection.

use std::net::Ipv4Addr;
use std::sync::Arc;
use std::time::{Duration, Instant};

use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;

/// Client-side halves, generic over the transport (TCP or WS).
type Rd<S> = tokio::io::ReadHalf<S>;
type Wr<S> = tokio::io::WriteHalf<S>;
use tokio::sync::mpsc;

use super::state::{AuthEntry, PendingSel, State, enc, send_bytes};
use crate::db::Db;
use crate::net::framing::PacketFramer;
use crate::proto::types::{FixedStr, Ip4Address, TickT};
use crate::proto::*;

const VERSION_2_UPDATEHOST: u8 = 1;
const MIN_CLIENT_VERSION: u32 = 6;
const DEFAULT_WALK_SPEED: u16 = 150;

/// The gate's own "server version" (crate version + LOGIN flag).
fn gate_version(flags: u8) -> Version {
    Version {
        major: 0,
        minor: 1,
        patch: 0,
        devel: 0,
        flags: 0,
        which: flags,
        vend: 0,
    }
}

/// Per-connection writer: a task that owns the write half; handlers
/// push complete packet buffers.
fn spawn_writer<W: tokio::io::AsyncWrite + Unpin + Send + 'static>(
    w: W,
) -> (mpsc::Sender<Vec<u8>>, tokio::task::JoinHandle<()>) {
    let (tx, mut rx) = mpsc::channel::<Vec<u8>>(256);
    let h = tokio::spawn(async move {
        let mut w = w;
        while let Some(buf) = rx.recv().await {
            if w.write_all(&buf).await.is_err() {
                break;
            }
            let _ = w.flush().await;
        }
        let _ = w.shutdown().await;
    });
    (tx, h)
}

pub async fn run<S>(st: Arc<State>, sock: S, ip4: Ipv4Addr)
where
    S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static,
{
    let ip = u32::from_le_bytes(ip4.octets());
    let (rd, wr) = tokio::io::split(sock);
    let (tx, wh) = spawn_writer(wr);
    let mut fr = PacketFramer::new(rd);

    // no valid reason to stay on the login port forever
    let deadline = Duration::from_secs(90);
    loop {
        let pkt = match tokio::time::timeout(deadline, fr.next()).await {
            Ok(Ok(Some(p))) => p,
            Ok(Ok(None)) | Err(_) => break, // eof / timeout
            Ok(Err(e)) => {
                tracing::warn!(
                    "client {}: frame error {e}",
                    Ipv4Addr::from(ip.to_le_bytes())
                );
                break;
            }
        };
        match pkt.id {
            0x7530 => {
                let mut p = P7531::default();
                p.version = gate_version(0x01); // TMWA_SERVER_LOGIN
                send_bytes(&tx, enc(|v| p.encode(v)));
            }
            0x7532 => break,
            0x0064 => {
                handle_login(&st, &tx, &pkt.bytes, ip).await;
                // tmwa's login session is one-shot: after the reply
                // the client disconnects and reconnects to the char
                // role. Keeping the socket open here made the
                // framer die on bytes the client pipelined; close
                // like tmwa does after 0x0069.
                break;
            }
            0x0065 => {
                char_session(st.clone(), &tx, &mut fr, ip, &pkt.bytes).await;
                break;
            }
            0x0072 => {
                relay(st.clone(), tx, wh, fr, ip, pkt.bytes).await;
                return;
            }
            id => {
                tracing::warn!(
                    "client {}: unknown first packet 0x{id:04x}",
                    Ipv4Addr::from(ip.to_le_bytes())
                );
                break;
            }
        }
    }
    drop(tx);
    let _ = wh.await;
}

fn ip4(v: u32) -> Ip4Address {
    Ip4Address(v.to_le_bytes())
}

fn pub_ip(st: &State) -> Ip4Address {
    let ip: Ipv4Addr = st.cfg.gate.public_ip.parse().unwrap_or(Ipv4Addr::LOCALHOST);
    Ip4Address(u32::from_le_bytes(ip.octets()).to_le_bytes())
}

fn stamp_seconds(secs: u64) -> FixedStr<20> {
    FixedStr::<20>::try_from_str(&super::format_time(secs)).unwrap_or_default()
}

// ------------------------------------------------------------------
// login (0x0064)
// ------------------------------------------------------------------

/// Returns true when the connection stays usable (always; failures
/// reply and return true too — tmwa keeps the socket until the client
/// leaves or 90 s).
async fn handle_login(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    pkt: &[u8],
    ip: u32,
) -> Option<()> {
    let Ok(fixed) = P0064::decode(pkt) else {
        return None;
    };
    let name0 = fixed.account_name.to_string_lossy();
    let pass0 = fixed.account_pass.to_string_lossy();

    // IP ACL (tmwa check_ip: order deny_allow/allow_deny, both empty
    // = allow)
    if !ip_allowed(st, ip) {
        send_bytes(tx, {
            let mut p = P006A::default();
            p.error_code = 0x03;
            enc(move |v| p.encode(v))
        });
        return Some(());
    }
    // flood protection
    if st.cfg.login.conn_limit_enable {
        let mut rl = st.recent_logins.lock().unwrap();
        let int = Duration::from_secs(st.cfg.login.conn_limit_interval);
        rl.retain(|_, t| t.elapsed() < int);
        if let Some(t) = rl.get(&ip) {
            if t.elapsed() < int {
                let mut p = P0081::default();
                p.error_code = 2;
                send_bytes(tx, enc(move |v| p.encode(v)));
                return Some(());
            }
        }
        rl.insert(ip, Instant::now());
    }

    // _M/_F registration
    let mut name = name0.clone();
    let mut new_sex = 0u8;
    if st.cfg.login.new_account {
        if name.len() >= 6 && pass0.len() >= 4 {
            match name.as_bytes()[name.len() - 1] {
                b'M' => new_sex = b'M',
                b'F' => new_sex = b'F',
                _ => {}
            }
            if new_sex != 0 {
                name.pop();
                name.pop();
            }
        }
    }

    let db = st.db.clone();
    let db_name = name.clone();
    let row = db
        .blocking(move |db| db.account_auth_row(&db_name))
        .await
        .ok()
        .flatten();

    // resolve account: existing (verify + checks) or create
    let account_id: u32;
    match &row {
        Some(a) => {
            let (id, hash, scheme, salt, state, errmsg, ban) = (
                a.id,
                &a.password_hash,
                &a.password_scheme,
                &a.legacy_salt,
                a.state,
                &a.error_message,
                a.ban_until,
            );
            if new_sex != 0 {
                send_6a(st, tx, 9, 0, None); // account already exists
                return Some(());
            }
            // password verify (argon2 in spawn_blocking)
            let (hash, scheme, salt, pass) =
                (hash.clone(), scheme.clone(), salt.clone(), pass0.clone());
            let v = tokio::task::spawn_blocking(move || {
                crate::auth::password::verify(&scheme, &hash, salt.as_deref(), pass.as_bytes())
            })
            .await;
            let v = match v {
                Ok(Ok(x)) => x,
                _ => crate::auth::password::Verify::Fail,
            };
            if v == crate::auth::password::Verify::Fail {
                send_6a(st, tx, 1, 0, None); // incorrect password
                return Some(());
            }
            if state != 0 {
                // packet 0x006a value + 1
                let code = match state {
                    1..=8 | 100 => (state - 1) as u16,
                    _ => 99,
                };
                send_6a(st, tx, code, ban, errmsg.clone());
                return Some(());
            }
            let now = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs();
            if ban != 0 {
                if ban as u64 > now {
                    send_6a(st, tx, 6, ban, errmsg.clone());
                    return Some(());
                }
                let _ = db.blocking(move |db| db.set_account_ban(id, 0)).await;
            }
            if v == crate::auth::password::Verify::OkNeedsRehash {
                let pass2 = pass0.clone();
                let h = tokio::task::spawn_blocking(move || {
                    crate::auth::password::hash_argon2id(pass2.as_bytes())
                })
                .await;
                if let Ok(Ok(h)) = h {
                    let _ = db
                        .blocking(move |db| db.set_password(id, &h, "argon2id", None))
                        .await;
                }
            }
            account_id = id as u32;
        }
        None => {
            if new_sex == 0 {
                send_6a(st, tx, 0, 0, None); // unregistered
                return Some(());
            }
            let pass = pass0.clone();
            let h = tokio::task::spawn_blocking(move || {
                crate::auth::password::hash_argon2id(pass.as_bytes())
            })
            .await;
            let Ok(Ok(hash)) = h else {
                send_6a(st, tx, 3, 0, None);
                return Some(());
            };
            let name2 = name.clone();
            let id = st
                .db
                .blocking(move |db| {
                    db.with_conn(move |conn| {
                        Db::create_account(conn, &name2, &hash, "").map_err(|e| {
                            rusqlite::Error::ToSqlConversionFailure(e.to_string().into())
                        })
                    })
                })
                .await;
            match id {
                Ok(i) => account_id = i as u32,
                Err(e) => {
                    tracing::warn!("create account: {e}");
                    send_6a(st, tx, 3, 0, None);
                    return Some(());
                }
            }
        }
    }

    // client too old
    if fixed.client_protocol_version.0 < MIN_CLIENT_VERSION {
        send_6a(st, tx, 5, 0, None);
        return Some(());
    }
    // min GM level
    if crate::serve::is_gm(st, account_id) < st.cfg.login.min_level_to_connect {
        let mut p = P0081::default();
        p.error_code = 1;
        send_bytes(tx, enc(move |v| p.encode(v)));
        return Some(());
    }

    let Some(login_id1) = State::random_u32() else {
        tracing::error!("getrandom failed; refusing login");
        return Some(());
    };
    let Some(login_id2) = State::random_u32() else {
        tracing::error!("getrandom failed; refusing login");
        return Some(());
    };
    let now_ms = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis() as i64;
    let prev = st
        .db
        .record_login(
            account_id as i64,
            now_ms,
            &Ipv4Addr::from(ip.to_le_bytes()).to_string(),
        )
        .ok()
        .flatten();

    st.push_auth(AuthEntry {
        account_id,
        char_id: 0,
        login_id1,
        login_id2,
        ip,
        client_version: fixed.client_protocol_version.0,
        map_id: None,
        upstream_ip: None,
        delflag: 2,
        created: Instant::now(),
    });

    // update host (0x0063)
    if fixed.flags & VERSION_2_UPDATEHOST != 0 && !st.cfg.login.update_host.is_empty() {
        let bytes = st.cfg.login.update_host.as_bytes();
        let mut p = P0063::default();
        p.repeat = bytes.iter().map(|&c| P0063Repeat { c }).collect();
        send_bytes(tx, enc(move |v| p.encode(v)));
    }

    // 0x0069 char server list: the gate is the only entry
    let mut head = P0069::default();
    head.login_id1 = login_id1;
    head.account_id = AccountId(account_id);
    head.login_id2 = login_id2;
    head.last_login_string = match prev {
        Some(ms) => stamp_millis(ms),
        None => FixedStr::<24>::try_from_str("-").unwrap_or_default(),
    };
    head.sex = Sex(2); // UNSPECIFIED
    let users = st.count_users() as u16;
    let rep = P0069Repeat {
        ip: pub_ip(st),
        port: st.cfg.gate.public_port,
        server_name: FixedStr::<20>::try_from_str(&st.cfg.char_.server_name).unwrap_or_default(),
        users,
        maintenance: 0,
        is_new: 0,
    };
    head.repeat = vec![rep];
    send_bytes(tx, enc(move |v| head.encode(v)));
    Some(())
}

/// 0x006a account login error; for code 6 the message is the ban
/// timestamp or the account's error_message.
fn send_6a(
    st: &State,
    tx: &mpsc::Sender<Vec<u8>>,
    code: u16,
    ban_until: i64,
    errmsg: Option<String>,
) {
    let mut p = P006A::default();
    p.error_code = code as u8;
    if code == 6 {
        if ban_until != 0 {
            p.error_message = stamp_seconds(ban_until as u64);
        } else if let Some(m) = errmsg {
            p.error_message = FixedStr::<20>::try_from_str(&m).unwrap_or_default();
        }
    }
    let _ = st; // message already resolved by caller
    send_bytes(tx, enc(move |v| p.encode(v)));
}

fn stamp_millis(ms: i64) -> FixedStr<24> {
    let secs = ms / 1000;
    let frac = ms % 1000;
    FixedStr::<24>::try_from_str(&format!("{}.{frac:03}", super::format_time(secs as u64)))
        .unwrap_or_default()
}

/// tmwa check_ip: allow/deny lists + order (deny_allow / allow_deny).
fn ip_allowed(st: &State, ip: u32) -> bool {
    let ip = Ipv4Addr::from(ip.to_le_bytes());
    let in_list = |list: &[String]| -> Option<bool> {
        for net in list {
            if let Some((addr, bits)) = parse_cidr(net) {
                if cidr_covers(ip, addr, bits) {
                    return Some(true);
                }
            } else if let Ok(a) = net.parse::<Ipv4Addr>() {
                if a == ip {
                    return Some(true);
                }
            }
        }
        None
    };
    // tmwa: order deny_allow means deny is checked first, then allow;
    // empty allow = allow all not denied.
    let denied = in_list(&st.cfg.login.deny).unwrap_or(false);
    let allowed = in_list(&st.cfg.login.allow);
    if st.cfg.login.order == "allow_deny" {
        if allowed == Some(true) {
            return true;
        }
        return !denied && st.cfg.login.allow.is_empty();
    }
    // deny_allow (default)
    if denied {
        return false;
    }
    match allowed {
        Some(a) => a,
        None => st.cfg.login.allow.is_empty(),
    }
}

fn parse_cidr(s: &str) -> Option<(Ipv4Addr, u8)> {
    let (a, b) = s.split_once('/')?;
    Some((a.parse().ok()?, b.parse().ok()?))
}

fn cidr_covers(ip: Ipv4Addr, net: Ipv4Addr, bits: u8) -> bool {
    let ip = u32::from(ip);
    let net = u32::from(net);
    let shift = 32u8.saturating_sub(bits);
    (ip >> shift) == (net >> shift)
}

// ------------------------------------------------------------------
// char screen (0x0065 onward)
// ------------------------------------------------------------------

async fn char_session<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    st: Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    fr: &mut PacketFramer<Rd<S>>,
    ip: u32,
    first: &[u8],
) {
    // 0x8000 magic first, like tmwa-char
    send_bytes(tx, {
        let p = P8000::default();
        enc(move |v| p.encode(v))
    });

    let Ok(fixed) = P0065::decode(first) else {
        return;
    };
    let account_id = fixed.account_id.0;
    let entry = st.take_char_auth(account_id, fixed.login_id1, fixed.login_id2, ip);
    let Some(entry) = entry else {
        tracing::warn!("char: unauthenticated 0x0065 for account {account_id} from {ip:#x}");
        return;
    };
    if st.cfg.char_.max_connect_user > 0 && st.count_users() as i32 >= st.cfg.char_.max_connect_user
    {
        let mut p = P006C::default();
        p.code = 0;
        send_bytes(tx, enc(move |v| p.encode(v)));
        return;
    }

    let gm = crate::serve::is_gm(&st, account_id);
    if gm != 0 {
        tracing::info!(account_id, "character account logged on (gm={gm})");
    }

    // register session (for disconnect_player on ban/delete)
    st.char_sessions
        .lock()
        .unwrap()
        .insert(account_id, tx.clone());
    let sd = CharSd {
        account_id,
        login_id1: fixed.login_id1,
        login_id2: fixed.login_id2,
        sex: fixed.sex,
        client_version: entry.client_version,
    };

    send_char_list(&st, tx, &sd).await;

    let deadline = Duration::from_secs(90 * 60); // generous on char screen
    loop {
        let pkt = match tokio::time::timeout(deadline, fr.next()).await {
            Ok(Ok(Some(p))) => p,
            Ok(Ok(None)) => {
                tracing::debug!(account_id, "char: client closed");
                break;
            }
            Ok(Err(e)) => {
                tracing::warn!(account_id, "char: frame error {e}");
                break;
            }
            Err(_) => break,
        };
        match pkt.id {
            0x7530 => {
                let mut p = P7531::default();
                p.version = gate_version(0x02 | 0x04); // CHAR|INTER
                send_bytes(tx, enc(move |v| p.encode(v)));
            }
            0x0061 => {
                handle_change_pass(&st, tx, &sd, &pkt.bytes).await;
            }
            0x0066 => {
                if handle_char_select(&st, tx, &sd, ip, &pkt.bytes).await {
                    // 0x0071 was sent (or is pending); keep serving
                    // until the client disconnects
                }
            }
            0x0067 => {
                handle_char_create(&st, tx, &sd, &pkt.bytes).await;
            }
            0x0068 => {
                handle_char_delete(&st, tx, &sd, &pkt.bytes).await;
            }
            0x00b2 => {
                // logout (handled at map; ignore here)
            }
            id => {
                tracing::debug!(
                    ip = format_args!("{ip:#x}"),
                    "char: unknown packet 0x{id:04x}"
                );
            }
        }
    }
    st.char_sessions.lock().unwrap().remove(&account_id);
}

struct CharSd {
    account_id: u32,
    login_id1: u32,
    login_id2: u32,
    sex: Sex,
    client_version: u32,
}

async fn send_char_list(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, sd: &CharSd) {
    let ids = st
        .db
        .char_ids_of_account(sd.account_id as i64)
        .unwrap_or_default();
    let mut repeat = Vec::new();
    for cid in ids.iter().take(9) {
        if let Some(rec) = st.load_char(*cid as u32).await {
            let k = &rec.key;
            let p = &rec.data;
            let sel = CharSelect {
                char_id: k.char_id,
                base_exp: p.base_exp as u32,
                zeny: p.zeny as u32,
                job_exp: p.job_exp as u32,
                job_level: p.job_level as u32,
                shoes: equip_view(p, 0x40),  // EPOS::SHOES
                gloves: equip_view(p, 0x80), // EPOS::GLOVES
                cape: equip_view(p, 0x20),   // EPOS::CAPE
                misc1: equip_view(p, 0x100), // EPOS::MISC1
                option: p.option,
                unused: 0,
                karma: p.karma as u32,
                manner: p.manner as u32,
                status_point: p.status_point as u16,
                hp: p.hp.min(0x7fff) as u16,
                max_hp: p.max_hp.min(0x7fff) as u16,
                sp: p.sp.min(0x7fff) as u16,
                max_sp: p.max_sp.min(0x7fff) as u16,
                speed: DEFAULT_WALK_SPEED,
                species: p.species,
                hair_style: p.hair as u16,
                weapon: 0,
                base_level: p.base_level as u16,
                skill_point: p.skill_point as u16,
                head_bottom: p.head_bottom,
                shield: p.shield,
                head_top: p.head_top,
                head_mid: p.head_mid,
                hair_color: p.hair_color as u16,
                misc2: equip_view(p, 0x200), // EPOS::MISC2
                char_name: k.name,
                stats: Stats6 {
                    str: sat8(p.attrs[0]),
                    agi: sat8(p.attrs[1]),
                    vit: sat8(p.attrs[2]),
                    int_: sat8(p.attrs[3]),
                    dex: sat8(p.attrs[4]),
                    luk: sat8(p.attrs[5]),
                },
                char_num: k.char_num,
                sex: if p.sex.0 == 2 { sd.sex } else { p.sex },
            };
            repeat.push(P006BRepeat { char_select: sel });
        }
    }
    let mut p = P006B::default();
    p.repeat = repeat;
    send_bytes(tx, enc(move |v| p.encode(v)));
}

fn sat8(v: i16) -> u8 {
    v.clamp(0, 255) as u8
}

/// View an equipped item's nameid for an EPOS bit.
fn equip_view(p: &CharData, bit: u16) -> ItemNameId {
    for it in p.inventory.iter() {
        if it.nameid.0 != 0 && it.equip.0 & bit != 0 {
            return it.nameid;
        }
    }
    ItemNameId(0)
}

async fn handle_change_pass(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    sd: &CharSd,
    bytes: &[u8],
) {
    let Ok(fixed) = P0061::decode(bytes) else {
        return;
    };
    let old = fixed.old_pass.to_string_lossy();
    let new = fixed.new_pass.to_string_lossy();
    let aid = sd.account_id as i64;
    let db = &st.db;
    let row = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT password_hash,password_scheme,legacy_salt FROM accounts WHERE id=?1",
                [aid],
                |r| {
                    Ok((
                        r.get::<usize, String>(0)?,
                        r.get::<usize, String>(1)?,
                        r.get::<usize, Option<String>>(2)?,
                    ))
                },
            )
        })
        .ok();
    let Some((hash, scheme, salt)) = row else {
        return;
    };
    let v = tokio::task::spawn_blocking(move || {
        crate::auth::password::verify(&scheme, &hash, salt.as_deref(), old.as_bytes())
    })
    .await;
    let ok = matches!(
        v,
        Ok(Ok(crate::auth::password::Verify::Ok))
            | Ok(Ok(crate::auth::password::Verify::OkNeedsRehash))
    );
    let mut code = 1u8;
    if ok && new.len() >= 4 {
        let new2 = new.clone();
        if let Ok(Ok(h)) = tokio::task::spawn_blocking(move || {
            crate::auth::password::hash_argon2id(new2.as_bytes())
        })
        .await
        {
            if db
                .blocking(move |db| db.set_password(aid, &h, "argon2id", None))
                .await
                .is_ok()
            {
                code = 0;
            }
        }
    }
    let mut p = P0062::default();
    p.status = code;
    send_bytes(tx, enc(move |v| p.encode(v)));
}

/// 0x0066: select -> 0x3829 to the map(s) -> 0x0071 on 0x3830.
async fn handle_char_select(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    sd: &CharSd,
    ip: u32,
    bytes: &[u8],
) -> bool {
    let Ok(fixed) = P0066::decode(bytes) else {
        return false;
    };
    let slot = fixed.code;
    // find the char in account+slot
    let ids = st
        .db
        .char_ids_of_account(sd.account_id as i64)
        .unwrap_or_default();
    let mut found: Option<(crate::proto::CharKey, CharData)> = None;
    for cid in ids {
        if let Some(rec) = st.load_char(cid as u32).await {
            if rec.key.char_num == slot && rec.key.account_id.0 == sd.account_id {
                found = Some((rec.key, rec.data));
                break;
            }
        }
    }
    let Some((ck, mut cd)) = found else {
        return false;
    };

    // pick map server: one holding last_point.map, else first with
    // maps (and rewrite the last map like tmwa does)
    let (map_id, rewrite) = st.map_for(&cd.last_point.map_.to_string_lossy());
    let Some(map_id) = map_id else {
        let mut p = P0081::default();
        p.error_code = 1; // server closed
        send_bytes(tx, enc(move |v| p.encode(v)));
        return false;
    };
    if let Some(m) = rewrite {
        cd.last_point.map_ = FixedStr::<16>::try_from_str(&m).unwrap_or_default();
        // update cache so the later load in 0x2afc sees the rewrite
        let mut chars = st.chars.lock().unwrap();
        if let Some(c) = chars.get_mut(&ck.char_id.0) {
            c.data.last_point.map_ = cd.last_point.map_;
        }
    }

    // char->map auth entry
    st.push_auth(AuthEntry {
        account_id: sd.account_id,
        char_id: ck.char_id.0,
        login_id1: sd.login_id1,
        login_id2: sd.login_id2,
        ip,
        client_version: sd.client_version,
        map_id: Some(map_id),
        upstream_ip: None,
        delflag: 3,
        created: Instant::now(),
    });

    let needed = st.map_count();
    st.pending_sel.lock().unwrap().insert(
        (sd.account_id, ck.char_id.0),
        PendingSel {
            client_tx: tx.clone(),
            account_id: sd.account_id,
            char_id: ck.char_id.0,
            login_id1: sd.login_id1,
            login_id2: sd.login_id2,
            map_name: cd.last_point.map_.to_string_lossy(),
            needed,
            seen: 0,
        },
    );

    // 0x3829 to all map servers; ip = the real client IP. The map
    // trusts it because its map_conf lists us as trusted_proxy_ip.
    let mut p = P3829::default();
    p.account_id = AccountId(sd.account_id);
    p.char_id = ck.char_id;
    p.login_id1 = sd.login_id1;
    p.login_id2 = sd.login_id2;
    p.ip = ip4(ip);
    st.map_broadcast(&enc(move |v| p.encode(v)));
    true
}

async fn handle_char_create(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    sd: &CharSd,
    bytes: &[u8],
) {
    let Ok(fixed) = P0067::decode(bytes) else {
        return;
    };
    let name = fixed.char_name.to_string_lossy();
    let stats = fixed.stats;
    let slot = fixed.slot;
    let hair_color = fixed.hair_color;
    let hair_style = fixed.hair_style;
    let cfg = &st.cfg.char_;
    tracing::debug!(account = sd.account_id, %name, slot, "char: create request");

    let err = |code: u8| {
        let mut p = P006E::default();
        p.code = code;
        send_bytes(tx, enc(move |v| p.encode(v)));
    };

    // printable + no leading/trailing whitespace
    if name.is_empty() || !name.bytes().all(|b| (32..=126).contains(&b)) || name != name.trim() {
        tracing::debug!(%name, "create: bad name");
        return err(0x02);
    }
    if name.len() < cfg.min_name_length as usize {
        tracing::debug!(%name, "create: too short");
        return err(0x02);
    }
    let letters = st.cfg.name_letters();
    if !name.bytes().all(|b| letters.contains(&b)) {
        tracing::debug!(%name, "create: bad letters");
        return err(0x02);
    }
    let sum: u32 = stats.str as u32
        + stats.agi as u32
        + stats.vit as u32
        + stats.int_ as u32
        + stats.dex as u32
        + stats.luk as u32;
    if sum != cfg.total_stat_sum as u32 {
        tracing::debug!(%name, sum, "create: bad stats");
        return err(0x03);
    }
    if slot >= cfg.char_slots as u8 {
        return err(0x05);
    }
    if hair_style > cfg.max_hair_style || hair_color > cfg.max_hair_color {
        return err(0x04);
    }
    let stats_arr = [
        stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
    ];
    for v in stats_arr {
        if (v as u16) < cfg.min_stat_value || (v as u16) > cfg.max_stat_value {
            return err(0x03);
        }
    }
    if name == "#wisp#" {
        return err(0x01);
    }
    let name2 = name.clone();
    if st
        .db
        .blocking(move |db| db.char_id_by_name(&name2))
        .await
        .ok()
        .flatten()
        .is_some()
    {
        return err(0x01);
    }
    // slot already used?
    for cid in st
        .db
        .char_ids_of_account(sd.account_id as i64)
        .unwrap_or_default()
    {
        if let Some(rec) = st.load_char(cid as u32).await {
            if rec.key.char_num == slot {
                return err(0x05);
            }
        }
    }

    // build CharData with tmwa's starting values
    let mut cd = CharData::default();
    cd.species = Species(0);
    cd.sex = Sex(3); // NEUTRAL
    cd.base_level = 1;
    cd.job_level = 1;
    cd.attrs[0] = stats.str as i16;
    cd.attrs[1] = stats.agi as i16;
    cd.attrs[2] = stats.vit as i16;
    cd.attrs[3] = stats.int_ as i16;
    cd.attrs[4] = stats.dex as i16;
    cd.attrs[5] = stats.luk as i16;
    cd.max_hp = 40 * (100 + cd.attrs[2] as i32) / 100;
    cd.max_sp = 11 * (100 + cd.attrs[3] as i32) / 100;
    cd.hp = cd.max_hp;
    cd.sp = cd.max_sp;
    cd.hair = hair_style as i16;
    cd.hair_color = hair_color as i16;
    cd.weapon = ItemLook(0); // W_FIST
    let (map, x, y) = st.cfg.start_point();
    cd.last_point = Point {
        map_: FixedStr::<16>::try_from_str(&map).unwrap_or_default(),
        x,
        y,
    };
    cd.save_point = cd.last_point;

    let name_s = name.clone();
    let aid = sd.account_id as i64;
    let res = st
        .db
        .blocking(move |db| {
            db.with_conn(move |conn| {
                let tx2 = conn.transaction()?;
                let cid = Db::alloc_meta_id(&tx2, "next_char_id")?;
                tx2.execute(
                    "INSERT INTO characters(id,account_id,slot,name,sex,species,
             base_level,job_level,base_exp,job_exp,zeny,hp,max_hp,sp,max_sp,
             attr_str,attr_agi,attr_vit,attr_int,attr_dex,attr_luk,
             status_point,skill_point,option_,karma,manner,party_id,
             hair,hair_color,clothes_color,weapon,shield,
             head_top,head_mid,head_bottom,
             last_map,last_x,last_y,save_map,save_x,save_y,partner_id)
             VALUES(?1,?2,?3,?4,?5,?6,?7,?8,0,0,0,?9,?10,?11,?12,
             ?13,?14,?15,?16,?17,?18,0,0,0,0,0,0,
             ?19,?20,0,0,0,0,0,0,?21,?22,?23,?24,?25,?26,0)",
                    rusqlite::params![
                        cid,
                        aid,
                        slot as i64,
                        name_s,
                        3i64,
                        0i64,
                        1i64,
                        1i64,
                        cd.hp,
                        cd.max_hp,
                        cd.sp,
                        cd.max_sp,
                        stats_arr[0] as i64,
                        stats_arr[1] as i64,
                        stats_arr[2] as i64,
                        stats_arr[3] as i64,
                        stats_arr[4] as i64,
                        stats_arr[5] as i64,
                        hair_style as i64,
                        hair_color as i64,
                        map,
                        x as i64,
                        y as i64,
                        map,
                        x as i64,
                        y as i64,
                    ],
                )?;
                tx2.commit().map(|_| cid)
            })
        })
        .await;
    let cid = match res {
        Ok(c) => c as u32,
        Err(e) => {
            tracing::warn!("char create db: {e}");
            return err(0x02);
        }
    };

    let key = CharKey {
        name: FixedStr::<24>::try_from_str(&name).unwrap_or_default(),
        account_id: AccountId(sd.account_id),
        char_id: CharId(cid),
        char_num: slot,
    };
    st.chars.lock().unwrap().insert(
        cid,
        super::state::CharRecord {
            key,
            data: cd,
            online_map: None,
        },
    );
    st.char_names.lock().unwrap().insert(name, cid);

    // 0x006d reply (tmwa fills CharSelect like this)
    let sel = CharSelect {
        char_id: CharId(cid),
        base_exp: 0,
        zeny: 0,
        job_exp: 0,
        job_level: 1,
        shoes: ItemNameId(0),
        gloves: ItemNameId(0),
        cape: ItemNameId(0),
        misc1: ItemNameId(0),
        option: Opt0(0),
        unused: 0,
        karma: 0,
        manner: 0,
        status_point: 0x30,
        hp: cd.hp.min(0x7fff) as u16,
        max_hp: cd.max_hp.min(0x7fff) as u16,
        sp: cd.sp.min(0x7fff) as u16,
        max_sp: cd.max_sp.min(0x7fff) as u16,
        speed: DEFAULT_WALK_SPEED,
        species: cd.species,
        hair_style: cd.hair as u16,
        weapon: 0,
        base_level: 1,
        skill_point: 0,
        head_bottom: ItemNameId(0),
        shield: cd.shield,
        head_top: cd.head_top,
        head_mid: cd.head_mid,
        hair_color: cd.hair_color as u16,
        misc2: ItemNameId(0),
        char_name: key.name,
        stats,
        char_num: slot,
        sex: cd.sex,
    };
    let mut p = P006D::default();
    p.char_select = sel;
    send_bytes(tx, enc(move |v| p.encode(v)));
}

async fn handle_char_delete(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    sd: &CharSd,
    bytes: &[u8],
) {
    let Ok(fixed) = P0068::decode(bytes) else {
        return;
    };
    let cid = fixed.char_id.0;
    // must belong to this account (tmwa also verifies)
    let rec = st.load_char(cid).await;
    let owns = rec
        .as_ref()
        .map(|r| r.key.account_id.0 == sd.account_id)
        .unwrap_or(false);
    if owns {
        delete_character(st, cid).await;
        let p = P006F::default();
        send_bytes(tx, enc(move |v| p.encode(v)));
    } else {
        let mut p = P0070::default();
        p.code = 0;
        send_bytes(tx, enc(move |v| p.encode(v)));
    }
}

// ------------------------------------------------------------------
// map relay (0x0072): client <-> upstream splice with hold/rejoin
// ------------------------------------------------------------------

use super::state::PlayerSession;

/// Delete one character, mirroring tmwa's char_delete: leave its
/// party (0x3824 broadcast), divorce if married, delete the row and
/// the in-memory copies. Callers delete the account row separately.
pub(crate) async fn delete_character(st: &Arc<State>, cid: u32) {
    let rec = st.load_char(cid).await;
    if let Some(r) = rec.as_ref() {
        let pid = r.data.party_id.0;
        if pid != 0 {
            super::maplink::party_leave_do(st, pid, r.key.account_id.0);
        }
        let cid64 = cid as i64;
        let _ = st.db.blocking(move |db| db.divorce(cid64)).await;
    }
    let _ = st
        .db
        .blocking(move |db| db.delete_character(cid as i64))
        .await;
    st.chars.lock().unwrap().remove(&cid);
    if let Some(name) = rec.map(|r| r.key.name.to_string_lossy()) {
        st.char_names.lock().unwrap().remove(&name);
    }
}

/// How a forwarding session ended.
enum FwdEnd {
    /// Client socket closed, or a normal logout/char-select that the
    /// map confirmed by closing.
    ClientGone,
    /// Upstream closed (map crash/restart or drain).
    UpstreamGone,
    /// Something we can't hold for (0x0092 multi-map request).
    Fatal,
}

/// Track client-UI state from an S->C packet (open NPC dialog, trade,
/// storage; also the server tick for hold-mode ping replies).
fn track_sc(rec: &std::sync::Mutex<PlayerSession>, id: u16, bytes: &[u8]) {
    let mut r = rec.lock().unwrap();
    match id {
        0x00b4 | 0x00b5 | 0x00b7 | 0x0142 | 0x01d4 => {
            if bytes.len() >= 8 {
                r.npc_id = u32::from_le_bytes(bytes[4..8].try_into().unwrap());
            }
        }
        0x00b6 => r.npc_id = 0,
        0x00e7 => {
            if bytes.len() >= 3 && bytes[2] == 0 {
                r.trade_open = true;
            }
        }
        0x00ee | 0x00f0 => r.trade_open = false,
        0x00f2 | 0x01f0 | 0x00a6 => r.storage_open = true,
        0x00f8 => r.storage_open = false,
        0x007f if bytes.len() >= 6 => {
            r.server_tick = u32::from_le_bytes(bytes[2..6].try_into().unwrap());
            r.server_tick_at = Instant::now();
        }
        _ => {}
    }
}

/// Send a single 0x009a announcement to the client.
fn announce(tx: &mpsc::Sender<Vec<u8>>, msg: &str) {
    let mut p = P009A::default();
    let mut bytes = msg.as_bytes().to_vec();
    bytes.push(0);
    p.repeat = bytes.iter().map(|&c| P009ARepeat { c }).collect();
    send_bytes(tx, enc(move |v| p.encode(v)));
}

/// Connect upstream and run the client-auth prelude: connect to the
/// map's client port and write 0x0072 (the auth entry from char
/// select is already in place). Returns the split halves.
async fn upstream_open(
    st: &Arc<State>,
    map_id: usize,
    pkt72: Vec<u8>,
    account_id: u32,
    char_id: u32,
    login_id1: u32,
) -> Option<(
    tokio::net::tcp::OwnedReadHalf,
    tokio::net::tcp::OwnedWriteHalf,
)> {
    let Some((mip, mport)) = st.map_addr(map_id) else {
        tracing::warn!("relay: map server {map_id} gone");
        return None;
    };
    let upstream_addr = std::net::SocketAddr::new(
        std::net::IpAddr::V4(Ipv4Addr::from(mip.to_le_bytes())),
        mport,
    );
    let Ok(mut up) = TcpStream::connect(upstream_addr).await else {
        tracing::warn!("relay: cannot connect to map {upstream_addr}");
        return None;
    };
    let _ = up.set_nodelay(true);
    // remember the address tmwa-map sees (it reports it in 0x2afc)
    if let Ok(la) = up.local_addr() {
        if let std::net::IpAddr::V4(v4) = la.ip() {
            st.set_auth_upstream_ip(
                account_id,
                char_id,
                login_id1,
                u32::from_le_bytes(v4.octets()),
            );
        }
    }
    if up.write_all(&pkt72).await.is_err() {
        return None;
    }
    Some(up.into_split())
}

/// Rejoin a map server: push a fresh stage-3 auth, send 0x3829, wait
/// for 0x3830, then run the client-auth prelude (0x0072 -> 0x8000 ->
/// 0x0073). Returns the upstream halves and the 0x0073 position.
async fn upstream_rejoin(
    st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
    map_id: usize,
) -> Option<(
    tokio::net::tcp::OwnedReadHalf,
    tokio::net::tcp::OwnedWriteHalf,
    P0073,
)> {
    let (account_id, char_id, login_id1, login_id2, sex, client_ip) = {
        let r = rec.lock().unwrap();
        (
            r.account_id,
            r.char_id,
            r.login_id1,
            r.login_id2,
            r.sex,
            r.client_ip,
        )
    };
    // refresh the map name from the saved CharData: this is exactly
    // where the map server will place the player after a shutdown
    // save, and it must be what we send in the client's 0x0091.
    if let Some(row) = st.load_char(char_id).await {
        let m = row.data.last_point.map_.to_string_lossy();
        let mut r = rec.lock().unwrap();
        r.map_name = m;
    }
    st.push_auth(AuthEntry {
        account_id,
        char_id,
        login_id1,
        login_id2,
        ip: client_ip,
        client_version: 0,
        map_id: Some(map_id),
        upstream_ip: None,
        delflag: 3,
        created: Instant::now(),
    });
    let mut p29 = P3829::default();
    p29.account_id = AccountId(account_id);
    p29.char_id = CharId(char_id);
    p29.login_id1 = login_id1;
    p29.login_id2 = login_id2;
    p29.ip = ip4(client_ip);
    st.map_send(map_id, enc(move |v| p29.encode(v)));

    let (rtx, rrx) = tokio::sync::oneshot::channel::<()>();
    st.rejoin_notify
        .lock()
        .unwrap()
        .insert((account_id, char_id), rtx);
    if tokio::time::timeout(Duration::from_secs(5), rrx)
        .await
        .is_err()
    {
        st.rejoin_notify
            .lock()
            .unwrap()
            .remove(&(account_id, char_id));
        tracing::warn!(char_id, "rejoin: no 0x3830 from map {map_id}");
        return None;
    }

    let mut pkt72 = Vec::new();
    P0072 {
        account_id: AccountId(account_id),
        char_id: CharId(char_id),
        login_id1,
        client_tick: 0,
        sex: Sex(sex),
    }
    .encode(&mut pkt72);
    let (mut urd, uwr) = upstream_open(st, map_id, pkt72, account_id, char_id, login_id1).await?;
    // swallow the 0x8000 magic and the 0x0073 login reply; the client
    // gets our 0x0091 instead
    let mut ufr = PacketFramer::new(&mut urd);
    let mut p73: Option<P0073> = None;
    for _ in 0..4 {
        match tokio::time::timeout(Duration::from_secs(5), ufr.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x8000 => continue,
            Ok(Ok(Some(p))) if p.id == 0x0073 => {
                p73 = P0073::decode(&p.bytes).ok();
                break;
            }
            Ok(Ok(Some(_))) => continue,
            _ => break,
        }
    }
    drop(ufr);
    p73.map(|p73| (urd, uwr, p73))
}

/// Held session: keep the client alive, answer pings, wait for a map
/// that serves the player's map to come back (or the drain fallback
/// at half the timeout). Returns the map id to rejoin on, or None if
/// the client is gone / hold timed out.
async fn hold_wait<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    st: &Arc<State>,
    rec: &std::sync::Arc<std::sync::Mutex<PlayerSession>>,
    fr: &mut PacketFramer<Rd<S>>,
    tx: &mpsc::Sender<Vec<u8>>,
) -> Option<usize> {
    let (char_id, deadline, half) = {
        let r = rec.lock().unwrap();
        let t = Duration::from_secs(st.cfg.gate.hold_timeout_secs);
        let since = r.held_since.unwrap_or_else(Instant::now);
        (r.char_id, since + t, t / 2)
    };
    loop {
        let now = Instant::now();
        if now >= deadline {
            tracing::info!(char_id, "hold timed out; closing client");
            return None;
        }
        // a map that serves the player's map, or the old map id once
        // half the timeout passed and nothing happened (drain fallback)
        let target = {
            // refresh once from the DB: the shutdown/drain save can
            // have moved the player to their saved map
            let needs = rec.lock().unwrap().map_name_stale;
            if needs {
                let cid = rec.lock().unwrap().char_id;
                if let Some(row) = st.load_char(cid).await {
                    let m = row.data.last_point.map_.to_string_lossy();
                    let mut r = rec.lock().unwrap();
                    r.map_name = m;
                    r.map_name_stale = false;
                }
            }
            let r = rec.lock().unwrap();
            let (mid, _) = st.map_for(&r.map_name);
            mid.or_else(|| {
                if now - r.held_since.unwrap_or(now) >= half && st.map_addr(r.map_id).is_some() {
                    Some(r.map_id)
                } else {
                    None
                }
            })
        };
        if let Some(m) = target {
            return Some(m);
        }
        // still nothing: read client input while we wait
        let wait = (deadline - now).min(Duration::from_secs(1));
        match tokio::time::timeout(wait, fr.next()).await {
            Ok(Ok(Some(p))) => {
                if p.id == 0x007e {
                    let (tick, at) = {
                        let r = rec.lock().unwrap();
                        (r.server_tick, r.server_tick_at)
                    };
                    let mut rep = P007F::default();
                    rep.tick = TickT(tick + at.elapsed().as_millis() as u32);
                    send_bytes(tx, enc(move |v| rep.encode(v)));
                }
                // all other packets are dropped while held
            }
            Ok(Ok(None)) | Ok(Err(_)) => return None,
            Err(_) => {} // timeout, re-evaluate
        }
    }
}

/// Forward packets in both directions until either side ends or a
/// drain asks us to hold. Returns how it ended.
async fn forward_phase<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    _st: &Arc<State>,
    rec: &std::sync::Arc<std::sync::Mutex<PlayerSession>>,
    fr: &mut PacketFramer<Rd<S>>,
    tx: &mpsc::Sender<Vec<u8>>,
    mut up: tokio::net::tcp::OwnedWriteHalf,
    mut urd: tokio::net::tcp::OwnedReadHalf,
    hold_signal: &std::sync::Arc<tokio::sync::Notify>,
) -> FwdEnd {
    let mut ufr = PacketFramer::new(&mut urd);
    loop {
        tokio::select! {
            p = fr.next() => {
                match p {
                    Ok(Some(p)) => {
                        if p.id == 0x00b2 {
                            rec.lock().unwrap().quitting = true;
                        }
                        if p.id == 0x0146 {
                            rec.lock().unwrap().npc_id = 0;
                        }
                        if up.write_all(&p.bytes).await.is_err() {
                            return FwdEnd::UpstreamGone;
                        }
                    }
                    Ok(None) => return FwdEnd::ClientGone,
                    Err(e) => {
                        tracing::warn!("relay: client frame error {e}");
                        return FwdEnd::ClientGone;
                    }
                }
            }
            p = ufr.next() => {
                match p {
                    Ok(Some(p)) => {
                        if p.id == 0x0092 {
                            tracing::warn!(
                                "relay: map requests map change; multi-map splicing not implemented; closing"
                            );
                            return FwdEnd::Fatal;
                        }
                        track_sc(rec, p.id, &p.bytes);
                        if tx.send(p.bytes).await.is_err() {
                            return FwdEnd::ClientGone;
                        }
                    }
                    Ok(None) => {
                        return if rec.lock().unwrap().quitting {
                            FwdEnd::ClientGone
                        } else {
                            FwdEnd::UpstreamGone
                        };
                    }
                    Err(e) => {
                        tracing::warn!("relay: upstream frame error {e}");
                        return FwdEnd::UpstreamGone;
                    }
                }
            }
            _ = hold_signal.notified() => {
                // drain: close upstream so tmwa-map runs map_quit
                // (which sends its 0x2b01 save), then stop
                let _ = up.shutdown().await;
                return FwdEnd::UpstreamGone;
            }
        }
    }
}

async fn relay<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    st: Arc<State>,
    tx: mpsc::Sender<Vec<u8>>,
    wh: tokio::task::JoinHandle<()>,
    mut fr: PacketFramer<Rd<S>>,
    ip: u32,
    first: Vec<u8>,
) {
    let Ok(fixed) = P0072::decode(&first) else {
        tracing::warn!("relay: bad 0x0072 from {ip:#x}");
        return;
    };
    // match the char->map auth entry
    let found = {
        let a = st.auth.lock().unwrap();
        a.values()
            .find(|e| {
                e.delflag == 3
                    && e.account_id == fixed.account_id.0
                    && e.char_id == fixed.char_id.0
                    && e.login_id1 == fixed.login_id1
                    && e.ip == ip
            })
            .map(|e| (e.map_id, e.login_id2))
    };
    let Some((map_id, login_id2)) = found else {
        tracing::warn!(
            "relay: no map auth for account {} char {} from {ip:#x}",
            fixed.account_id.0,
            fixed.char_id.0
        );
        return;
    };
    let Some(map_id) = map_id else {
        tracing::warn!(
            "relay: auth entry has no map for account {}",
            fixed.account_id.0
        );
        return;
    };

    let map_name = st
        .load_char(fixed.char_id.0)
        .await
        .map(|r| r.data.last_point.map_.to_string_lossy())
        .unwrap_or_default();
    let hold_signal = std::sync::Arc::new(tokio::sync::Notify::new());
    let rec = std::sync::Arc::new(std::sync::Mutex::new(PlayerSession {
        account_id: fixed.account_id.0,
        char_id: fixed.char_id.0,
        sex: fixed.sex.0,
        login_id1: fixed.login_id1,
        login_id2,
        client_ip: ip,
        server_tick: fixed.client_tick,
        server_tick_at: Instant::now(),
        map_id,
        map_name,
        map_name_stale: true,
        npc_id: 0,
        trade_open: false,
        storage_open: false,
        quitting: false,
        kicked: false,
        held: false,
        hold_signal: Some(hold_signal.clone()),
        held_since: None,
    }));
    st.player_sessions
        .lock()
        .unwrap()
        .insert(fixed.char_id.0, rec.clone());

    let mut cur_map_id = map_id;
    let pkt72 = first;
    let mut first_iter = true;

    'life: loop {
        // ---- open upstream ----
        let (urd, uwr) = if first_iter {
            first_iter = false;
            match upstream_open(
                &st,
                cur_map_id,
                pkt72.clone(),
                fixed.account_id.0,
                fixed.char_id.0,
                fixed.login_id1,
            )
            .await
            {
                Some(v) => v,
                None => {
                    // can't even open the map once: hold the client
                    // and wait for a map to come back
                    rec.lock().unwrap().held = true;
                    match hold_wait(&st, &rec, &mut fr, &tx).await {
                        Some(m) => {
                            cur_map_id = m;
                            continue 'life;
                        }
                        None => break 'life,
                    }
                }
            }
        } else {
            // rejoin: fresh auth + 0x3829 + 0x3830 + 0x0072 -> 0x0073
            let deadline = {
                let r = rec.lock().unwrap();
                r.held_since.unwrap_or_else(Instant::now)
                    + Duration::from_secs(st.cfg.gate.hold_timeout_secs)
            };
            let mut joined = None;
            'retry: while Instant::now() < deadline {
                // the attempt blocks on map-link round-trips; run it
                // in a task so the held client is still served
                // (pings answered, quit noticed).
                let st2 = st.clone();
                let rec2 = rec.clone();
                let mid = cur_map_id;
                let mut att = tokio::spawn(async move { upstream_rejoin(&st2, &rec2, mid).await });
                'attempt: loop {
                    tokio::select! {
                        out = &mut att => {
                            match out {
                                Ok(Some(v)) => {
                                    joined = Some(v);
                                    break 'attempt;
                                }
                                Ok(None) => break 'attempt,
                                Err(_) => break 'attempt,
                            }
                        }
                        p = fr.next() => {
                            match p {
                                Ok(Some(p)) if p.id == 0x007e => {
                                    let (tick, at) = {
                                        let r = rec.lock().unwrap();
                                        (r.server_tick, r.server_tick_at)
                                    };
                                    let mut rep = P007F::default();
                                    rep.tick = TickT(tick + at.elapsed().as_millis() as u32);
                                    send_bytes(&tx, enc(move |v| rep.encode(v)));
                                }
                                Ok(Some(_)) => {} // drop held input
                                _ => {
                                    att.abort();
                                    break 'life;
                                }
                            }
                        }
                        _ = tokio::time::sleep_until(
                            tokio::time::Instant::from_std(deadline),
                        ) => {
                            att.abort();
                            break 'attempt;
                        }
                    }
                    if Instant::now() >= deadline {
                        break 'attempt;
                    }
                }
                if joined.is_some() || Instant::now() >= deadline {
                    break 'retry;
                }
                tokio::time::sleep(Duration::from_secs(1)).await;
            }
            match joined {
                Some((urd, uwr, p73)) => {
                    // client-side session cleanup before the 0x0091:
                    // close an open NPC dialog / trade / storage
                    let (npc, trade, storage) = {
                        let mut r = rec.lock().unwrap();
                        r.held = false;
                        r.map_id = cur_map_id;
                        (r.npc_id, r.trade_open, r.storage_open)
                    };
                    if npc != 0 {
                        let mut p = P00B6::default();
                        p.block_id = BlockId(npc);
                        send_bytes(&tx, enc(move |v| p.encode(v)));
                    }
                    if trade {
                        send_bytes(&tx, enc(|v| P00EE::default().encode(v)));
                    }
                    if storage {
                        send_bytes(&tx, enc(|v| P00F8::default().encode(v)));
                    }
                    let mut p91 = P0091::default();
                    p91.map_name = FixedStr::<16>::try_from_str(&rec.lock().unwrap().map_name)
                        .unwrap_or_default();
                    p91.x = p73.pos.x;
                    p91.y = p73.pos.y;
                    send_bytes(&tx, enc(move |v| p91.encode(v)));
                    {
                        let mut r = rec.lock().unwrap();
                        r.server_tick = p73.tick.0;
                        r.server_tick_at = Instant::now();
                    }
                    (urd, uwr)
                }
                None => break 'life,
            }
        };

        // ---- forward ----
        let end = forward_phase(&st, &rec, &mut fr, &tx, uwr, urd, &hold_signal).await;
        match end {
            FwdEnd::ClientGone | FwdEnd::Fatal => break 'life,
            FwdEnd::UpstreamGone => {
                if rec.lock().unwrap().kicked {
                    break 'life;
                }
                // hold: client stays, announce once, wait for rejoin
                {
                    let mut r = rec.lock().unwrap();
                    r.held = true;
                    r.held_since = Some(Instant::now());
                }
                announce(&tx, &st.cfg.gate.hold_message.clone());
                match hold_wait(&st, &rec, &mut fr, &tx).await {
                    Some(m) => {
                        cur_map_id = m;
                        continue 'life;
                    }
                    None => break 'life,
                }
            }
        }
    }

    st.player_sessions.lock().unwrap().remove(&fixed.char_id.0);
    // don't let a stale attempt steal the next login's 0x3830
    st.rejoin_notify
        .lock()
        .unwrap()
        .remove(&(fixed.account_id.0, fixed.char_id.0));
    drop(tx);
    let _ = wh.await;
    let _ = pkt72;
}
