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

/// Client-side read half, generic over the transport (TCP or WS).
type Rd<S> = tokio::io::ReadHalf<S>;
use tokio::sync::mpsc;

use super::state::{AuthEntry, DELFLAG_CHAR, DELFLAG_MAP, PendingSel, State};
use super::state::PlayerSession;
use crate::db::Db;
use crate::net::framing::PacketFramer;
use crate::proto::types::{FixedStr, Ip4Address, TickT};
use crate::proto::*;

const VERSION_2_UPDATEHOST: u8 = 1;
const MIN_CLIENT_VERSION: u32 = 6;
const DEFAULT_WALK_SPEED: u16 = 150;

/// The gate's own "server version" for 0x7531: fixed at 0.1.0;
/// `flags` carries the role bits (LOGIN here, CHAR|INTER on the
/// char screen).
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

/// `allow_relay`: only the WebSocket transport relays map traffic;
/// a TCP client that sends 0x0072 to the gate is a leftover of the
/// old topology (or an honest mistake) — log once and drop it.
pub async fn run<S>(st: Arc<State>, sock: S, ip4: Ipv4Addr, allow_relay: bool)
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
                // routine disconnects (tcp rst, ws reset without a
                // close handshake) — not worth a warn
                tracing::debug!(
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
                if !allow_relay {
                    // TCP clients connect to the map server named in
                    // 0x0071 directly; only the WS transport relays.
                    static WARNED: std::sync::atomic::AtomicBool =
                        std::sync::atomic::AtomicBool::new(false);
                    if !WARNED.swap(true, std::sync::atomic::Ordering::Relaxed) {
                        tracing::warn!(
                            "client {}: 0x0072 on the TCP listener; \
                             map-stage relay is WS-only now, closing",
                            Ipv4Addr::from(ip.to_le_bytes())
                        );
                    }
                    break;
                }
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
    Ip4Address(ip.octets())
}

fn stamp_seconds(secs: u64) -> FixedStr<20> {
    FixedStr::<20>::from_str_truncate(&super::format_time(secs))
}

/// Send raw bytes to a client session's writer task.
fn send_bytes(tx: &mpsc::Sender<Vec<u8>>, v: Vec<u8>) {
    let _ = tx.try_send(v);
}

// ------------------------------------------------------------------
// login (0x0064)
// ------------------------------------------------------------------

/// 0x0064 login: verify (or register) the account, push the stage-2
/// auth entry and answer with 0x0069 or an error packet. The caller
/// closes the socket either way.
async fn handle_login(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, pkt: &[u8], ip: u32) {
    let Ok(fixed) = P0064::decode(pkt) else {
        return;
    };
    let name0 = fixed.account_name.to_string_lossy();
    let pass0 = fixed.account_pass.to_string_lossy();

    // IP ACL (tmwa check_ip: order deny_allow/allow_deny, both empty
    // = allow)
    if !ip_allowed(st, ip) {
        send_6a(tx, 3, 0, None);
        return;
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
                return;
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
                send_6a(tx, 9, 0, None); // account already exists
                return;
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
                send_6a(tx, 1, 0, None); // incorrect password
                return;
            }
            if state != 0 {
                // packet 0x006a value + 1
                let code = match state {
                    1..=8 | 100 => (state - 1) as u16,
                    _ => 99,
                };
                send_6a(tx, code, ban, errmsg.clone());
                return;
            }
            let now = std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_secs();
            if ban != 0 {
                if ban as u64 > now {
                    send_6a(tx, 6, ban, errmsg.clone());
                    return;
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
                        .blocking(move |db| {
                            db.set_password(id, &h, crate::auth::password::Scheme::Argon2id, None)
                        })
                        .await;
                }
            }
            account_id = id as u32;
        }
        None => {
            if new_sex == 0 {
                send_6a(tx, 0, 0, None); // unregistered
                return;
            }
            let pass = pass0.clone();
            let h = tokio::task::spawn_blocking(move || {
                crate::auth::password::hash_argon2id(pass.as_bytes())
            })
            .await;
            let Ok(Ok(hash)) = h else {
                send_6a(tx, 3, 0, None);
                return;
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
                    send_6a(tx, 3, 0, None);
                    return;
                }
            }
        }
    }

    // client too old
    if fixed.client_protocol_version.0 < MIN_CLIENT_VERSION {
        send_6a(tx, 5, 0, None);
        return;
    }
    // min GM level
    if crate::serve::is_gm(st, account_id) < st.cfg.login.min_level_to_connect {
        send_server_closed(tx);
        return;
    }

    let Some(login_id1) = State::random_u32() else {
        tracing::error!("getrandom failed; refusing login");
        return;
    };
    let Some(login_id2) = State::random_u32() else {
        tracing::error!("getrandom failed; refusing login");
        return;
    };
    let now_ms = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis() as i64;
    let ip_str = Ipv4Addr::from(ip.to_le_bytes()).to_string();
    let prev = st
        .db
        .blocking(move |db| db.record_login(account_id as i64, now_ms, &ip_str))
        .await
        .ok()
        .flatten();

    st.push_auth(char_auth(
        account_id,
        login_id1,
        login_id2,
        ip,
        fixed.client_protocol_version.0,
    ));

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
        None => FixedStr::<24>::from_str_truncate("-"),
    };
    head.sex = Sex(2); // UNSPECIFIED
    let users = st.count_users() as u16;
    let rep = P0069Repeat {
        ip: pub_ip(st),
        port: st.cfg.gate.public_port,
        server_name: FixedStr::<20>::from_str_truncate(&st.cfg.char_.server_name),
        users,
        maintenance: 0,
        is_new: 0,
    };
    head.repeat = vec![rep];
    send_bytes(tx, enc(move |v| head.encode(v)));
}

/// 0x0081 code 1 ("No servers available."): the connection ends.
fn send_server_closed(tx: &mpsc::Sender<Vec<u8>>) {
    let mut p = P0081::default();
    p.error_code = 1;
    send_bytes(tx, enc(move |v| p.encode(v)));
}

/// Stage-2 auth entry pushed by a successful login: the account's
/// next step is the char screen (0x0065).
fn char_auth(account_id: u32, id1: u32, id2: u32, ip: u32, client_version: u32) -> AuthEntry {
    AuthEntry {
        account_id,
        char_id: 0,
        login_id1: id1,
        login_id2: id2,
        ip,
        client_version,
        map_id: None,
        upstream_ip: None,
        delflag: DELFLAG_CHAR,
        created: Instant::now(),
    }
}

/// Stage-3 auth entry pushed on char select / map rejoin: the map
/// server authenticates the char with a 0x2afc.
fn map_auth(
    account_id: u32,
    char_id: u32,
    id1: u32,
    id2: u32,
    ip: u32,
    client_version: u32,
    map_id: usize,
) -> AuthEntry {
    AuthEntry {
        account_id,
        char_id,
        login_id1: id1,
        login_id2: id2,
        ip,
        client_version,
        map_id: Some(map_id),
        upstream_ip: None,
        delflag: DELFLAG_MAP,
        created: Instant::now(),
    }
}

/// 0x006a account login error; for code 6 the message is the ban
/// timestamp or the account's error_message.
fn send_6a(tx: &mpsc::Sender<Vec<u8>>, code: u16, ban_until: i64, errmsg: Option<String>) {
    let mut p = P006A::default();
    p.error_code = code as u8;
    if code == 6 {
        if ban_until != 0 {
            p.error_message = stamp_seconds(ban_until as u64);
        } else if let Some(m) = errmsg {
            p.error_message = FixedStr::<20>::from_str_truncate(&m);
        }
    }
    send_bytes(tx, enc(move |v| p.encode(v)));
}

fn stamp_millis(ms: i64) -> FixedStr<24> {
    let secs = ms / 1000;
    let frac = ms % 1000;
    FixedStr::<24>::from_str_truncate(&format!("{}.{frac:03}", super::format_time(secs as u64)))
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
                tracing::debug!(account_id, "char: frame error {e}");
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
                handle_char_select(&st, tx, &sd, ip, &pkt.bytes).await;
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
}

struct CharSd {
    account_id: u32,
    login_id1: u32,
    login_id2: u32,
    sex: Sex,
    client_version: u32,
}

async fn send_char_list(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, sd: &CharSd) {
    let aid = sd.account_id as i64;
    let ids = st
        .db
        .blocking(move |db| db.char_ids_of_account(aid))
        .await
        .unwrap_or_default();
    let mut repeat = Vec::new();
    for cid in ids.iter().take(st.cfg.char_.char_slots as usize) {
        if let Some(rec) = st.load_char(*cid as u32).await {
            let sel = char_select(&rec.key, &rec.data, sd.sex);
            repeat.push(P006BRepeat { char_select: sel });
        }
    }
    let mut p = P006B::default();
    p.repeat = repeat;
    send_bytes(tx, enc(move |v| p.encode(v)));
}

/// Build the CharSelect wire view of a cached char: the 0x006b list
/// and the 0x006d create reply share this layout (the create reply
/// overrides `status_point` afterwards).
fn char_select(k: &CharKey, p: &CharData, account_sex: Sex) -> CharSelect {
    CharSelect {
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
        sex: if p.sex.0 == 2 { account_sex } else { p.sex },
    }
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
    let row = st
        .db
        .blocking(move |db| db.with_conn(|conn| crate::db::password_row_conn(conn, aid)))
        .await
        .ok()
        .flatten();
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
            if st
                .db
                .blocking(move |db| {
                    db.set_password(aid, &h, crate::auth::password::Scheme::Argon2id, None)
                })
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

/// Complete a char-select: 0x0071 with the target map server's
/// registered address, or 0x0081 if it is gone. The wire reply is
/// built here (client side); the `pending_sel` bookkeeping it
/// resolves lives on `State` (`sel_waiting_done`, `map_unregister`).
pub(crate) fn send_pending_sel(st: &State, ps: PendingSel) {
    let addr = st.map_addr(ps.map_id);
    match addr {
        Some((ip, port)) => {
            // tmwa lan_support.conf (char.cpp lan_ip_check): a
            // client inside lan_subnet is pointed at lan_map_ip
            // instead of the map's advertised address; the
            // registered port stays.
            let client = Ipv4Addr::from(ps.client_ip.to_le_bytes());
            let ip = if st.cfg.lan.lan_subnet.covers(client) {
                st.cfg.lan.lan_map_ip
            } else {
                Ipv4Addr::from(ip.to_le_bytes())
            };
            let mut p = P0071::default();
            p.char_id = CharId(ps.char_id);
            p.map_name = FixedStr::<16>::from_str_truncate(&ps.map_name);
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

/// 0x0066 select: 0x3829 to the map(s), then 0x0071 on 0x3830.
/// The 0x0071 may go out later via `pending_sel`; either way the
/// session keeps serving until the client disconnects.
async fn handle_char_select(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    sd: &CharSd,
    ip: u32,
    bytes: &[u8],
) {
    let Ok(fixed) = P0066::decode(bytes) else {
        return;
    };
    let slot = fixed.code;
    // find the char in account+slot
    let aid = sd.account_id as i64;
    let ids = st
        .db
        .blocking(move |db| db.char_ids_of_account(aid))
        .await
        .unwrap_or_default();
    let mut found: Option<(crate::proto::CharKey, CharData)> = None;
    for cid in ids {
        if let Some(rec) = st.load_char(cid as u32).await {
            if rec.key.char_num == slot && rec.key.account_id.0 == sd.account_id {
                // select rewrites last_point below, so an owned copy
                found = Some((rec.key, *rec.data));
                break;
            }
        }
    }
    let Some((ck, mut cd)) = found else {
        return;
    };

    // pick map server: one holding last_point.map, else first with
    // maps (and rewrite the last map like tmwa does)
    let (map_id, rewrite) = st.map_for(&cd.last_point.map_.to_string_lossy());
    let Some(map_id) = map_id else {
        send_server_closed(tx);
        return;
    };
    // a saturated map link can't answer auth requests in time:
    // refuse the select rather than queue behind it
    if st.map_congested(map_id) {
        send_server_closed(tx);
        return;
    }
    if let Some(m) = rewrite {
        cd.last_point.map_ = FixedStr::<16>::from_str_truncate(&m);
        // update cache so the later load in 0x2afc sees the rewrite
        let mut chars = st.chars.lock().unwrap();
        if let Some(c) = chars.get_mut(&ck.char_id.0) {
            Arc::make_mut(&mut c.data).last_point.map_ = cd.last_point.map_;
        }
    }

    // char->map auth entry
    st.push_auth(map_auth(
        sd.account_id,
        ck.char_id.0,
        sd.login_id1,
        sd.login_id2,
        ip,
        sd.client_version,
        map_id,
    ));

    // 0x3829 to all map servers; ip = the real client IP. The map
    // trusts it because its map_conf lists us as trusted_proxy_ip.
    // The pending select is registered BEFORE the packets go out:
    // a fast map can ack (0x3830) before send_must returns, and a
    // map unregistered mid-select must still resolve the entry.
    let mut p = P3829::default();
    p.account_id = AccountId(sd.account_id);
    p.char_id = ck.char_id;
    p.login_id1 = sd.login_id1;
    p.login_id2 = sd.login_id2;
    p.ip = ip4(ip);
    let Some(ttx) = st.map_prio_tx(map_id) else {
        send_server_closed(tx);
        return;
    };
    let key = (sd.account_id, ck.char_id.0);
    let senders = st.map_senders();
    let mut waiting: std::collections::HashSet<usize> =
        senders.iter().map(|(mid, _)| *mid).collect();
    waiting.insert(map_id);
    st.pending_sel.lock().unwrap().insert(
        key,
        PendingSel {
            client_tx: tx.clone(),
            account_id: sd.account_id,
            char_id: ck.char_id.0,
            login_id1: sd.login_id1,
            login_id2: sd.login_id2,
            client_ip: ip,
            map_name: cd.last_point.map_.to_string_lossy(),
            map_id,
            waiting,
        },
    );
    // a map that unregistered between map_for and this insert left
    // the entry orphaned (its cleanup already ran): re-check and
    // resolve it like a missing 0x3830.
    if st.map_prio_tx(map_id).is_none()
        && let Some(ps) = st.sel_waiting_done(key, map_id, sd.login_id1, sd.login_id2)
    {
        send_pending_sel(st, ps);
    }
    for (mid, mtx) in &senders {
        if *mid == map_id {
            continue; // the target goes through send_must below
        }
        if mtx.try_send(enc(|v| p.encode(v)).into()).is_err() {
            tracing::warn!("map {mid}: dropped select pre-auth (link congested)");
            if let Some(ps) = st.sel_waiting_done(key, *mid, sd.login_id1, sd.login_id2) {
                send_pending_sel(st, ps);
            }
        }
    }
    if !super::dbq::send_must(st, map_id, &ttx, enc(|v| p.encode(v))).await
        && let Some(ps) = st.sel_waiting_done(key, map_id, sd.login_id1, sd.login_id2)
    {
        // the target never got the pre-auth; it will authenticate
        // the client through 0x2afc instead, so still answer 0x0071
        send_pending_sel(st, ps);
    }
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
    let aid = sd.account_id as i64;
    for cid in st
        .db
        .blocking(move |db| db.char_ids_of_account(aid))
        .await
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
        map_: FixedStr::<16>::from_str_truncate(&map),
        x,
        y,
    };
    cd.save_point = cd.last_point;

    let name_s = name.clone();
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
        name: FixedStr::<24>::from_str_truncate(&name),
        account_id: AccountId(sd.account_id),
        char_id: CharId(cid),
        char_num: slot,
    };
    st.cache_put(
        cid,
        super::state::CharRecord {
            key,
            data: Arc::new(cd),
        },
    );
    st.char_names.lock().unwrap().insert(name, cid);

    // 0x006d reply (tmwa fills CharSelect like this)
    let mut sel = char_select(&key, &cd, sd.sex);
    // a fresh char shows the creation points, not the stored 0
    sel.status_point = 0x30;
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

/// Delete one character, mirroring tmwa's char_delete: leave its
/// party (0x3824 broadcast), divorce if married, delete the row and
/// the in-memory copies. Callers delete the account row separately.
pub(crate) async fn delete_character(st: &Arc<State>, cid: u32) {
    let rec = st.load_char(cid).await;
    if let Some(r) = rec.as_ref() {
        let pid = r.data.party_id.0;
        if pid != 0 {
            super::maplink::party_leave_do(st, pid, r.key.account_id.0).await;
        }
        let cid64 = cid as i64;
        let _ = st.db.blocking(move |db| db.divorce(cid64)).await;
    }
    let _ = st
        .db
        .blocking(move |db| db.delete_character(cid as i64))
        .await;
    st.cache_remove(cid);
    // owed-save marks must not resurrect the deleted rows
    st.save_dirty.lock().unwrap().remove(&cid);
    if let Some(a) = rec.as_ref().map(|r| r.key.account_id.0) {
        st.storage_dirty.lock().unwrap().remove(&(a as i64));
    }
    if let Some(name) = rec.map(|r| r.key.name.to_string_lossy()) {
        st.char_names.lock().unwrap().remove(&name);
    }
}

// ------------------------------------------------------------------
// map relay (0x0072): client and upstream splice with hold/rejoin
// ------------------------------------------------------------------

/// How a forwarding session ended.
enum FwdEnd {
    /// Client socket closed, or a normal logout/char-select that the
    /// map confirmed by closing.
    ClientGone,
    /// Upstream closed; whether to hold or drop is decided by
    /// `resolve_upstream_eof`.
    UpstreamGone,
    /// The drain signal fired: hold unconditionally.
    Hold,
    /// An admin kick fired the same signal: upstream was closed so
    /// the map saves, but the client disconnects instead of holding.
    Kicked,
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
        0x0073 => r.saw_0073 = true,
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

/// Answer a client 0x007e ping with 0x007f, carrying the last server
/// tick plus the time elapsed since we saw it.
fn answer_tick(rec: &std::sync::Mutex<PlayerSession>, tx: &mpsc::Sender<Vec<u8>>) {
    let (tick, at) = {
        let r = rec.lock().unwrap();
        (r.server_tick, r.server_tick_at)
    };
    let mut rep = P007F::default();
    rep.tick = TickT(tick + at.elapsed().as_millis() as u32);
    send_bytes(tx, enc(move |v| rep.encode(v)));
}

/// Serve the client for up to `wait` while the session is held or
/// the upstream's fate is undecided: answer 0x007e pings, drop
/// everything else. Returns false on EOF or a frame error (the
/// client is gone).
async fn drain_client_input<
    S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static,
>(
    fr: &mut PacketFramer<Rd<S>>,
    rec: &std::sync::Mutex<PlayerSession>,
    tx: &mpsc::Sender<Vec<u8>>,
    wait: Duration,
) -> bool {
    match tokio::time::timeout(wait, fr.next()).await {
        Ok(Ok(Some(p))) => {
            if p.id == 0x007e {
                answer_tick(rec, tx);
            }
            true
        }
        Ok(Ok(None)) | Ok(Err(_)) => false,
        Err(_) => true, // timeout: nothing arrived
    }
}

/// What an upstream EOF resolves to once the map-link state is known.
enum UpGone {
    /// The map itself is going away: hold the client.
    Hold,
    /// A per-player disconnect (kick, double login, softlimit): the
    /// client is closed; if the map never accepted it (no 0x0073)
    /// it first gets 0x0081 code 1 ("No servers available.").
    Close,
    /// The client left while we were looking at the link.
    ClientGone,
}

/// How long to watch the map link after an upstream EOF before
/// calling it a per-player disconnect.
const UPSTREAM_GONE_GRACE: Duration = Duration::from_secs(3);

/// Upstream closed while the session wasn't quitting, kicked, or
/// drained. If the map link is down or the map announced shutdown
/// (0x2b17) this is a map restart: hold. Otherwise give the link up
/// to UPSTREAM_GONE_GRACE to drop or announce (SIGKILL drops the
/// kernel sockets together), still serving the client meanwhile.
/// Nothing is announced until hold is decided.
async fn resolve_upstream_eof<
    S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static,
>(
    st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
    map_id: usize,
    fr: &mut PacketFramer<Rd<S>>,
    tx: &mpsc::Sender<Vec<u8>>,
) -> UpGone {
    if st.map_gone(map_id) {
        return UpGone::Hold;
    }
    let deadline = Instant::now() + UPSTREAM_GONE_GRACE;
    loop {
        let left = deadline.saturating_duration_since(Instant::now());
        if left.is_zero() {
            break;
        }
        let wait = left.min(Duration::from_millis(200));
        if !drain_client_input(fr, rec, tx, wait).await {
            return UpGone::ClientGone;
        }
        if st.map_gone(map_id) {
            return UpGone::Hold;
        }
    }
    // link still up and no notice: this disconnect concerned this
    // player alone. If the map never accepted it, tell the client
    // why; otherwise it already got whatever the map sent.
    if !rec.lock().unwrap().saw_0073 {
        send_server_closed(tx);
    }
    UpGone::Close
}

/// Connect upstream and run the client-auth prelude: connect to the
/// map's client port and write 0x0072 (the auth entry from char
/// select is already in place). Returns the split halves.
async fn upstream_open(
    st: &Arc<State>,
    map_id: usize,
    pkt72: bytes::Bytes,
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

/// How one rejoin attempt ended.
enum Rejoin {
    /// Upstream open and the map sent 0x0073.
    Joined(
        tokio::net::tcp::OwnedReadHalf,
        tokio::net::tcp::OwnedWriteHalf,
        P0073,
    ),
    /// The map isn't back yet; keep retrying until the deadline.
    Retry,
    /// The link is up, no shutdown notice, and the map closed the
    /// 0x0072 connection before 0x0073: the map is back but full.
    MapFull,
}

/// Push a fresh stage-3 auth entry and 0x3829 to `map_id`, then wait
/// (bounded) for its 0x3830. Shared by the hold-rejoin and the
/// 0x0092-handoff fresh-auth retry.
async fn push_reauth(
    st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
    map_id: usize,
) -> bool {
    let (account_id, char_id, login_id1, login_id2, client_ip) = {
        let r = rec.lock().unwrap();
        (
            r.account_id,
            r.char_id,
            r.login_id1,
            r.login_id2,
            r.client_ip,
        )
    };
    st.push_auth(map_auth(
        account_id,
        char_id,
        login_id1,
        login_id2,
        client_ip,
        0,
        map_id,
    ));
    let mut p29 = P3829::default();
    p29.account_id = AccountId(account_id);
    p29.char_id = CharId(char_id);
    p29.login_id1 = login_id1;
    p29.login_id2 = login_id2;
    p29.ip = ip4(client_ip);
    let Some(mtx) = st.map_prio_tx(map_id) else {
        return false;
    };
    // register the waiter BEFORE the 0x3829 goes out, like the
    // pending select in handle_char_select: a fast map can answer
    // 0x3830 before send_must returns, and an ack that finds no
    // waiter is dropped.
    let (rtx, rrx) = tokio::sync::oneshot::channel::<()>();
    st.rejoin_notify
        .lock()
        .unwrap()
        .insert((account_id, char_id), rtx);
    if !super::dbq::send_must(st, map_id, &mtx, enc(move |v| p29.encode(v))).await {
        // wedged link: fail now instead of waiting out the timeout
        st.rejoin_notify
            .lock()
            .unwrap()
            .remove(&(account_id, char_id));
        return false;
    }
    if tokio::time::timeout(Duration::from_secs(5), rrx)
        .await
        .is_err()
    {
        st.rejoin_notify
            .lock()
            .unwrap()
            .remove(&(account_id, char_id));
        tracing::debug!(char_id, "rejoin: no 0x3830 from map {map_id}");
        return false;
    }
    true
}

/// Fresh-auth retry for a 0x0092 handoff reconnect that reached the
/// target map before our 0x3829 did: the client already sent its
/// 0x0072, so reopen the upstream and replay it.
async fn transfer_reopen(
    st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
    map_id: usize,
    pkt72: bytes::Bytes,
) -> Option<(
    tokio::net::tcp::OwnedReadHalf,
    tokio::net::tcp::OwnedWriteHalf,
)> {
    if !push_reauth(st, rec, map_id).await {
        return None;
    }
    let (account_id, char_id, login_id1) = {
        let r = rec.lock().unwrap();
        (r.account_id, r.char_id, r.login_id1)
    };
    upstream_open(st, map_id, pkt72, account_id, char_id, login_id1).await
}

/// Rejoin a map server: push a fresh stage-3 auth, send 0x3829, wait
/// for 0x3830, then run the client-auth prelude (0x0072 -> 0x8000 ->
/// 0x0073).
async fn upstream_rejoin(
    st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
    map_id: usize,
) -> Rejoin {
    let (account_id, char_id, login_id1, sex) = {
        let r = rec.lock().unwrap();
        (r.account_id, r.char_id, r.login_id1, r.sex)
    };
    // refresh the map name from the saved CharData: this is exactly
    // where the map server will place the player after a shutdown
    // save, and it must be what we send in the client's 0x0091.
    if let Some(row) = st.load_char(char_id).await {
        let m = row.data.last_point.map_.to_string_lossy();
        let mut r = rec.lock().unwrap();
        r.map_name = m;
    }
    if !push_reauth(st, rec, map_id).await {
        return Rejoin::Retry;
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
    let Some((mut urd, uwr)) =
        upstream_open(st, map_id, pkt72.into(), account_id, char_id, login_id1).await
    else {
        return Rejoin::Retry;
    };
    // swallow the 0x8000 magic and the 0x0073 login reply; the client
    // gets our 0x0091 instead
    let mut ufr = PacketFramer::new(&mut urd);
    let mut p73: Option<P0073> = None;
    let mut upstream_eof = false;
    // bounded in count and time: expect 0x8000, maybe a stray packet
    // or two, then 0x0073; a map that never answers hits the timeout
    for _ in 0..4 {
        match tokio::time::timeout(Duration::from_secs(5), ufr.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x8000 => continue,
            Ok(Ok(Some(p))) if p.id == 0x0073 => {
                p73 = P0073::decode(&p.bytes).ok();
                break;
            }
            Ok(Ok(Some(_))) => continue,
            Ok(Ok(None)) | Ok(Err(_)) => {
                upstream_eof = true;
                break;
            }
            Err(_) => break,
        }
    }
    drop(ufr);
    match p73 {
        Some(p73) => Rejoin::Joined(urd, uwr, p73),
        None if upstream_eof && !st.map_gone(map_id) => Rejoin::MapFull,
        None => Rejoin::Retry,
    }
}

/// Held session: keep the client alive, answer pings, wait for a map
/// that serves the player's map to come back (or the drain fallback
/// at half the timeout). Returns the map id to rejoin on, or None if
/// the client is gone / hold timed out.
async fn hold_wait<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
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
        // a kick while held stores its notify permit; the flag is
        // what matters, so check it here too
        if rec.lock().unwrap().kicked {
            return None;
        }
        if now >= deadline {
            tracing::warn!(char_id, "hold timed out; closing client");
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
        if !drain_client_input(fr, rec, tx, wait).await {
            return None;
        }
    }
}

/// Forward packets in both directions until either side ends or a
/// drain asks us to hold. Returns how it ended.
async fn forward_phase<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    _st: &Arc<State>,
    rec: &std::sync::Mutex<PlayerSession>,
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
                        tracing::debug!("relay: client frame error {e}");
                        return FwdEnd::ClientGone;
                    }
                }
            }
            p = ufr.next() => {
                match p {
                    Ok(Some(p)) => {
                        if p.id == 0x0092 {
                            // drain evacuate / cross-server warp:
                            // the client reconnects to the same WS
                            // endpoint and sends 0x0072 again. Hand
                            // it the packet and keep relaying until
                            // either side closes.
                            rec.lock().unwrap().transferring = true;
                        }
                        track_sc(rec, p.id, &p.bytes);
                        if tx.send(p.bytes.to_vec()).await.is_err() {
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
                // drain or admin kick: close upstream so tmwa-map
                // runs map_quit (which sends its 0x2b01 save). A
                // drain then holds the client; a kick drops it.
                let _ = up.shutdown().await;
                return if rec.lock().unwrap().kicked {
                    FwdEnd::Kicked
                } else {
                    FwdEnd::Hold
                };
            }
        }
    }
}

/// Rejoin loop after a hold: retry `upstream_rejoin` on `map_id`
/// until the hold deadline, still serving the held client meanwhile
/// (pings answered, a client close or kick ends the session). On
/// success the client-side leftovers (open NPC dialog, trade,
/// storage) are closed and the new position is announced with
/// 0x0091. Returns the new upstream halves, or None when the
/// session is over: kick, client gone, map full, or timed out.
async fn rejoin_until<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    st: &Arc<State>,
    rec: &std::sync::Arc<std::sync::Mutex<PlayerSession>>,
    fr: &mut PacketFramer<Rd<S>>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
) -> Option<(
    tokio::net::tcp::OwnedReadHalf,
    tokio::net::tcp::OwnedWriteHalf,
)> {
    let deadline = {
        let r = rec.lock().unwrap();
        r.held_since.unwrap_or_else(Instant::now)
            + Duration::from_secs(st.cfg.gate.hold_timeout_secs)
    };
    let mut joined = None;
    'retry: while Instant::now() < deadline {
        if rec.lock().unwrap().kicked {
            return None;
        }
        // the attempt blocks on map-link round-trips; run it
        // in a task so the held client is still served
        // (pings answered, quit noticed).
        let st2 = st.clone();
        let rec2 = rec.clone();
        let mut att = tokio::spawn(async move { upstream_rejoin(&st2, &rec2, map_id).await });
        'attempt: loop {
            tokio::select! {
                out = &mut att => {
                    match out {
                        Ok(Rejoin::Joined(v0, v1, v2)) => {
                            joined = Some((v0, v1, v2));
                            break 'attempt;
                        }
                        Ok(Rejoin::Retry) => break 'attempt,
                        Ok(Rejoin::MapFull) => {
                            // map is back but refused us
                            // before 0x0073 (full / limit):
                            // tell the client, stop holding
                            send_server_closed(tx);
                            return None;
                        }
                        Err(_) => break 'attempt,
                    }
                }
                p = fr.next() => {
                    match p {
                        Ok(Some(p)) if p.id == 0x007e => {
                            answer_tick(rec, tx);
                        }
                        Ok(Some(_)) => {} // drop held input
                        _ => {
                            att.abort();
                            return None;
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
    let (urd, uwr, p73) = joined?;
    rec.lock().unwrap().saw_0073 = true;
    // client-side session cleanup before the 0x0091:
    // close an open NPC dialog / trade / storage
    let (npc, trade, storage) = {
        let mut r = rec.lock().unwrap();
        r.held = false;
        r.map_id = map_id;
        (r.npc_id, r.trade_open, r.storage_open)
    };
    if npc != 0 {
        let mut p = P00B6::default();
        p.block_id = BlockId(npc);
        send_bytes(tx, enc(move |v| p.encode(v)));
    }
    if trade {
        send_bytes(tx, enc(|v| P00EE::default().encode(v)));
    }
    if storage {
        send_bytes(tx, enc(|v| P00F8::default().encode(v)));
    }
    let mut p91 = P0091::default();
    p91.map_name = FixedStr::<16>::try_from_str(&rec.lock().unwrap().map_name)
        .unwrap_or_default();
    p91.x = p73.pos.x;
    p91.y = p73.pos.y;
    send_bytes(tx, enc(move |v| p91.encode(v)));
    {
        let mut r = rec.lock().unwrap();
        r.server_tick = p73.tick.0;
        r.server_tick_at = Instant::now();
    }
    Some((urd, uwr))
}

async fn relay<S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin + Send + 'static>(
    st: Arc<State>,
    tx: mpsc::Sender<Vec<u8>>,
    wh: tokio::task::JoinHandle<()>,
    mut fr: PacketFramer<Rd<S>>,
    ip: u32,
    first: bytes::Bytes,
) {
    let Ok(fixed) = P0072::decode(&first) else {
        tracing::warn!("relay: bad 0x0072 from {ip:#x}");
        return;
    };
    // match the char-to-map auth entry (keyed by account_id)
    let found = {
        let a = st.auth.lock().unwrap();
        let now = Instant::now();
        a.map.get(&fixed.account_id.0)
            .filter(|e| {
                e.delflag == DELFLAG_MAP
                    && e.fresh(now)
                    && e.char_id == fixed.char_id.0
                    && e.login_id1 == fixed.login_id1
                    && e.ip == ip
            })
            .map(|e| (e.map_id, e.login_id2))
    };
    let Some((entry_map_id, login_id2)) = found else {
        tracing::warn!(
            "relay: no map auth for account {} char {} from {ip:#x}",
            fixed.account_id.0,
            fixed.char_id.0
        );
        return;
    };

    let map_name = st
        .load_char(fixed.char_id.0)
        .await
        .map(|r| r.data.last_point.map_.to_string_lossy())
        .unwrap_or_default();

    // A 0x2b05 handoff entry carries the map the source resolved;
    // the saved last_point is the authoritative target after the
    // transfer save. An entry with no map id, or one pointing at a
    // draining/gone server, resolves through the saved map name
    // (map_for prefers non-draining servers).
    let resolved = match entry_map_id {
        Some(mid) if !st.map_gone(mid) && !st.map_draining(mid) => Some(mid),
        _ => st.map_for(&map_name).0,
    };
    let Some(map_id) = resolved else {
        tracing::warn!(
            "relay: no map serves {} for account {}",
            map_name,
            fixed.account_id.0
        );
        return;
    };
    // this 0x0072 follows a 0x0092 handoff: the target map may not
    // have our 0x3829 yet, so an early upstream close gets one
    // fresh-auth retry
    let from_transfer = entry_map_id.is_none() || entry_map_id != Some(map_id);
    let hold_signal = std::sync::Arc::new(tokio::sync::Notify::new());
    let rec = std::sync::Arc::new(std::sync::Mutex::new(PlayerSession {
        account_id: fixed.account_id.0,
        char_id: fixed.char_id.0,
        sex: fixed.sex.0,
        login_id1: fixed.login_id1,
        login_id2,
        client_ip: ip,
        server_tick: fixed.client_tick,
        map_id,
        map_name,
        map_name_stale: true,
        hold_signal: Some(hold_signal.clone()),
        ..Default::default()
    }));
    st.player_sessions
        .lock()
        .unwrap()
        .insert(fixed.char_id.0, rec.clone());

    let mut cur_map_id = map_id;
    let pkt72 = first;
    let mut first_iter = true;
    // a 0x0092-handoff reconnect gets one fresh-auth retry when the
    // target map closed it unanswered
    let mut retried_transfer = false;

    'life: loop {
        // ---- open upstream ----
        let (mut urd, mut uwr) = if first_iter {
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
            // rejoin: fresh auth, then the 0x3829/0x3830 and
            // 0x0072/0x0073 handshakes
            let Some(v) = rejoin_until(&st, &rec, &mut fr, &tx, cur_map_id).await else {
                break 'life;
            };
            v
        };

        // ---- forward ----
        let mut end;
        loop {
            end = forward_phase(&st, &rec, &mut fr, &tx, uwr, urd, &hold_signal).await;
            // the 0x0072 for a 0x0092 handoff can reach the target
            // map before our 0x3829 did; the map then closed the
            // upstream unanswered. Push a fresh auth and replay the
            // 0x0072 once before giving up on the player.
            if matches!(end, FwdEnd::UpstreamGone)
                && !rec.lock().unwrap().saw_0073
                && from_transfer
                && !retried_transfer
            {
                retried_transfer = true;
                if let Some((urd2, uwr2)) =
                    transfer_reopen(&st, &rec, cur_map_id, pkt72.clone()).await
                {
                    urd = urd2;
                    uwr = uwr2;
                    continue;
                }
            }
            break;
        }
        // The drain signal asks for a hold even while the map is
        // still up; a bare upstream EOF goes through the decision.
        let want_hold = match end {
            FwdEnd::ClientGone | FwdEnd::Kicked => break 'life,
            FwdEnd::Hold => true,
            FwdEnd::UpstreamGone => {
                {
                    let r = rec.lock().unwrap();
                    // a kicked player and a client that got its
                    // 0x0092 both just end here
                    if r.kicked || r.transferring {
                        break 'life;
                    }
                }
                match resolve_upstream_eof(&st, &rec, cur_map_id, &mut fr, &tx).await {
                    UpGone::Hold => true,
                    UpGone::Close | UpGone::ClientGone => break 'life,
                }
            }
        };
        if want_hold {
            // hold: client stays, announce once, wait for rejoin
            {
                let mut r = rec.lock().unwrap();
                r.held = true;
                r.held_since = Some(Instant::now());
            }
            announce(&tx, &st.cfg.gate.hold_message);
            match hold_wait(&st, &rec, &mut fr, &tx).await {
                Some(m) => {
                    cur_map_id = m;
                    continue 'life;
                }
                None => break 'life,
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
}

#[cfg(test)]
mod tests {
    use super::*;

    fn test_state() -> Arc<State> {
        let dir = std::env::temp_dir().join(format!(
            "upgone-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        std::fs::create_dir_all(&dir).unwrap();
        let db = Db::open(&dir.join("gate.db")).unwrap();
        Arc::new(State::new(
            crate::config::Config::default(),
            std::sync::Arc::new(db),
        ))
    }

    fn test_rec() -> std::sync::Arc<std::sync::Mutex<PlayerSession>> {
        std::sync::Arc::new(std::sync::Mutex::new(PlayerSession {
            account_id: 1,
            char_id: 2,
            login_id1: 3,
            login_id2: 4,
            ..Default::default()
        }))
    }

    fn test_conn() -> (
        PacketFramer<Rd<tokio::io::DuplexStream>>,
        tokio::io::DuplexStream,
        mpsc::Receiver<Vec<u8>>,
    ) {
        let (a, b) = tokio::io::duplex(1024);
        let (rd, _wr) = tokio::io::split(a);
        let (tx, rx) = mpsc::channel(8);
        let _ = tx;
        (PacketFramer::new(rd), b, rx)
    }

    /// A dead/absent link holds immediately.
    #[tokio::test]
    async fn upstream_eof_holds_when_map_gone() {
        let st = test_state();
        let rec = test_rec();
        let (mut fr, _peer, _rx) = test_conn();
        let (tx, _rx) = mpsc::channel(8);
        match resolve_upstream_eof(&st, &rec, 0, &mut fr, &tx).await {
            UpGone::Hold => {}
            _ => panic!("absent link must hold"),
        }
    }

    /// A live link with no shutdown notice after the grace window
    /// is a per-player disconnect (and the client hears 0x0081 when
    /// the map never accepted it).
    #[tokio::test]
    async fn upstream_eof_closes_when_map_alive() {
        let st = test_state();
        let (tx, mut wrx) = mpsc::channel::<Vec<u8>>(8);
        let (mtx, _mrx) = mpsc::channel(8);
        let (mid, _kill) = st.map_register(mtx.clone(), mtx.clone(), 0, 0);
        let rec = test_rec();
        let (mut fr, _peer, _rx) = test_conn();
        let t0 = Instant::now();
        let r = resolve_upstream_eof(&st, &rec, mid, &mut fr, &tx).await;
        match r {
            UpGone::Close => {}
            _ => panic!("live link without notice must close"),
        }
        assert!(t0.elapsed() >= UPSTREAM_GONE_GRACE);
        let msg = wrx.recv().await.unwrap();
        assert_eq!(u16::from_le_bytes([msg[0], msg[1]]), 0x0081);
        assert_eq!(msg[2], 1);
    }

    /// A live link but a shutdown notice arrives during the grace
    /// window: hold.
    #[tokio::test]
    async fn upstream_eof_holds_on_notice() {
        let st = test_state();
        let (tx, _rx) = mpsc::channel::<Vec<u8>>(8);
        let (mtx, _mrx) = mpsc::channel(8);
        let (mid, _kill) = st.map_register(mtx.clone(), mtx.clone(), 0, 0);
        let rec = test_rec();
        let (mut fr, _peer, _rx) = test_conn();
        let st2 = st.clone();
        tokio::spawn(async move {
            tokio::time::sleep(Duration::from_millis(300)).await;
            st2.map_set_shutting_down(mid);
        });
        let t0 = Instant::now();
        match resolve_upstream_eof(&st, &rec, mid, &mut fr, &tx).await {
            UpGone::Hold => {}
            _ => panic!("notice during grace must hold"),
        }
        assert!(t0.elapsed() < UPSTREAM_GONE_GRACE);
    }

    /// Client EOF during the grace window ends the session.
    #[tokio::test]
    async fn upstream_eof_client_gone() {
        let st = test_state();
        let (tx, _rx) = mpsc::channel::<Vec<u8>>(8);
        let (mtx, _mrx) = mpsc::channel(8);
        let (mid, _kill) = st.map_register(mtx.clone(), mtx.clone(), 0, 0);
        let rec = test_rec();
        let (mut fr, peer, _rx) = test_conn();
        drop(peer); // client closed
        match resolve_upstream_eof(&st, &rec, mid, &mut fr, &tx).await {
            UpGone::ClientGone => {}
            _ => panic!("client EOF must win"),
        }
    }

    /// The 0x3830 waiter must be registered before the 0x3829 goes
    /// out: a map answering fast enough to beat send_must's return
    /// must still find it.
    #[tokio::test]
    async fn push_reauth_registers_waiter_before_send() {
        let st = test_state();
        let (btx, _brx) = mpsc::channel(8);
        let (ptx, mut prx) = mpsc::channel(8);
        let (mid, _kill) = st.map_register(btx, ptx, 0, 0);
        let rec = test_rec();
        let st2 = st.clone();
        let att = tokio::spawn(async move { push_reauth(&st2, &rec, mid).await });
        // the map sees the 0x3829
        let pkt = prx.recv().await.unwrap();
        assert_eq!(u16::from_le_bytes([pkt[0], pkt[1]]), 0x3829);
        // the waiter is already there; its answer cannot be dropped
        let notify = st
            .rejoin_notify
            .lock()
            .unwrap()
            .remove(&(1, 2))
            .expect("waiter must be registered before the send");
        notify.send(()).unwrap();
        assert!(att.await.unwrap());
    }

    /// A dead link fails the rejoin immediately instead of waiting
    /// out the 5 s ack timeout, and leaves no stale waiter behind.
    #[tokio::test]
    async fn push_reauth_dead_link_fails_fast() {
        let st = test_state();
        let (btx, brx) = mpsc::channel(8);
        let (ptx, prx) = mpsc::channel(8);
        let (mid, _kill) = st.map_register(btx, ptx, 0, 0);
        drop(brx);
        drop(prx);
        let rec = test_rec();
        let t0 = Instant::now();
        assert!(!push_reauth(&st, &rec, mid).await);
        assert!(t0.elapsed() < Duration::from_secs(5));
        assert!(st.rejoin_notify.lock().unwrap().is_empty());
    }

    /// Loopback halves for the upstream side of forward_phase.
    async fn upstream_pair() -> (
        tokio::net::tcp::OwnedReadHalf,
        tokio::net::tcp::OwnedWriteHalf,
        TcpStream,
    ) {
        let l = tokio::net::TcpListener::bind((Ipv4Addr::LOCALHOST, 0))
            .await
            .unwrap();
        let a = l.local_addr().unwrap();
        let c = TcpStream::connect(a).await.unwrap();
        let (s, _) = l.accept().await.unwrap();
        let (urd, uwr) = c.into_split();
        (urd, uwr, s)
    }

    /// An admin kick shares the drain's notify: it must end the
    /// relay, not return a hold that leads to a rejoin.
    #[tokio::test]
    async fn hold_signal_with_kick_ends_relay() {
        let st = test_state();
        let rec = test_rec();
        rec.lock().unwrap().kicked = true;
        let sig = std::sync::Arc::new(tokio::sync::Notify::new());
        let (mut fr, _peer, _rx) = test_conn();
        let (tx, _rx) = mpsc::channel(8);
        let (urd, uwr, _srv) = upstream_pair().await;
        sig.notify_one();
        let end = forward_phase(&st, &rec, &mut fr, &tx, uwr, urd, &sig).await;
        assert!(matches!(end, FwdEnd::Kicked));
    }

    /// The same signal without a kick is a drain hold.
    #[tokio::test]
    async fn hold_signal_without_kick_holds() {
        let st = test_state();
        let rec = test_rec();
        let sig = std::sync::Arc::new(tokio::sync::Notify::new());
        let (mut fr, _peer, _rx) = test_conn();
        let (tx, _rx) = mpsc::channel(8);
        let (urd, uwr, _srv) = upstream_pair().await;
        sig.notify_one();
        let end = forward_phase(&st, &rec, &mut fr, &tx, uwr, urd, &sig).await;
        assert!(matches!(end, FwdEnd::Hold));
    }

    fn sel_state(lan_subnet: &str, lan_map_ip: Ipv4Addr) -> State {
        let mut cfg = crate::config::Config::default();
        cfg.lan.lan_subnet = lan_subnet.parse().unwrap();
        cfg.lan.lan_map_ip = lan_map_ip;
        State::new(cfg, std::sync::Arc::new(Db::open_memory().unwrap()))
    }

    /// Drive one char-select completion; the receiver carries the
    /// 0x0071/0x0081 bytes the client would get.
    fn pending_sel(st: &State, map_id: usize, client_ip: [u8; 4]) -> mpsc::Receiver<Vec<u8>> {
        let (tx, rx) = mpsc::channel(8);
        send_pending_sel(
            st,
            PendingSel {
                client_tx: tx,
                account_id: 1,
                char_id: 100,
                login_id1: 1,
                login_id2: 2,
                client_ip: u32::from_le_bytes(client_ip),
                map_name: "001-1.gat".into(),
                map_id,
                waiting: std::collections::HashSet::new(),
            },
        );
        rx
    }

    #[test]
    fn send_pending_sel_lan_override() {
        use crate::proto::P0071;
        let st = sel_state("10.0.0.0/8", Ipv4Addr::new(192, 168, 1, 10));
        // a map advertising a WAN address
        let (mtx, _mrx) = mpsc::channel(8);
        let (map_id, _kill) = st.map_register(
            mtx.clone(),
            mtx.clone(),
            u32::from_le_bytes([203, 0, 113, 7]),
            5121,
        );

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
    fn send_pending_sel_default_subnet() {
        use crate::proto::P0071;
        // default lan_subnet covers only 127.0.0.1
        let st = sel_state("127.0.0.1", Ipv4Addr::LOCALHOST);
        let (mtx, _mrx) = mpsc::channel(8);
        let (map_id, _kill) = st.map_register(
            mtx.clone(),
            mtx.clone(),
            u32::from_le_bytes([203, 0, 113, 7]),
            5121,
        );

        let mut rx = pending_sel(&st, map_id, [127, 0, 0, 1]);
        let p = P0071::decode(&rx.try_recv().unwrap()).unwrap();
        assert_eq!(p.ip.0, [127, 0, 0, 1]);

        let mut rx = pending_sel(&st, map_id, [203, 0, 113, 9]);
        let p = P0071::decode(&rx.try_recv().unwrap()).unwrap();
        assert_eq!(p.ip.0, [203, 0, 113, 7]);
    }
}
