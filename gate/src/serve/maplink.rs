// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! The map link: tmwa-map connects to `map.listen`.
//!
//! Ports the char server side of the `char_map` channel (char.cpp
//! parse_frommap / inter_parse_frommap) and the `inter` handlers
//! (inter.cpp, int_party.cpp, int_storage.cpp) for one map server,
//! structured to scale to several.

use std::net::Ipv4Addr;
use std::sync::Arc;

use tokio::net::TcpStream;
use tokio::sync::mpsc;

use super::is_gm;
use super::state::{State, enc, send_must};
use crate::net::framing::PacketFramer;
use crate::proto::types::{FixedStr, GmLevel, Ip4Address};
use crate::proto::*;

const MAX_PARTY: usize = 120;
const ACCOUNT_REG2_NUM: usize = 16;
const ACCOUNT_REG_NUM: usize = 16;

type Fr = PacketFramer<tokio::net::tcp::OwnedReadHalf>;

pub async fn run(st: Arc<State>, sock: TcpStream, ip: Ipv4Addr) {
    let _ip32 = u32::from_le_bytes(ip.octets());
    let (rd, wr) = sock.into_split();
    // two writer queues sharing one socket writer: `rx` is bulk
    // (broadcasts, floods), `rx_prio` is critical replies and is
    // always drained first — a bulk backlog can never starve an
    // answer the map is blocked on.
    let (tx, mut rx) = mpsc::channel::<Vec<u8>>(2048);
    let (tx_prio, mut rx_prio) = mpsc::channel::<Vec<u8>>(2048);
    // writer task
    let wh = tokio::spawn(async move {
        let mut w = wr;
        use tokio::io::AsyncWriteExt;
        loop {
            let buf = tokio::select! {
                biased;
                b = rx_prio.recv() => match b { Some(b) => b, None => break },
                b = rx.recv() => match b { Some(b) => b, None => break },
            };
            if w.write_all(&buf).await.is_err() {
                break;
            }
            let _ = w.flush().await;
        }
    });

    let mut fr = PacketFramer::new(rd);
    let mut map_id: Option<usize> = None;
    let mut map_kill_notify: Option<std::sync::Arc<tokio::sync::Notify>> = None;

    // ---- 0x2af8 login ----
    let authed = 'auth: {
        match fr.next().await {
            Ok(Some(pkt)) if pkt.id == 0x2af8 => {
                let Ok(fixed) = P2AF8::decode(&pkt.bytes) else {
                    break 'auth false;
                };
                let user = fixed.account_name.to_string_lossy();
                let pass = fixed.account_pass.to_string_lossy();
                let mut p = P2AF9::default();
                if st.map_count() >= 32
                    || user != st.cfg.map.userid.as_str()
                    || pass != st.cfg.map.password.as_str()
                {
                    p.code = 3;
                    send_must(&st, usize::MAX, &tx_prio, enc(move |v| p.encode(v))).await;
                    tracing::warn!("maplink: bad map auth from {ip}");
                    break 'auth false;
                }
                p.code = 0;
                send_must(&st, usize::MAX, &tx_prio, enc(move |v| p.encode(v))).await;
                let (id, kill) = st.map_register(
                    tx.clone(),
                    tx_prio.clone(),
                    u32::from_le_bytes(fixed.ip.0),
                    fixed.port,
                );
                tracing::info!(
                    "maplink: map server {id} registered from {ip} \
                     (client port {}:{})",
                    Ipv4Addr::from(fixed.ip.0),
                    fixed.port
                );
                map_id = Some(id);
                // 0x2b15 GM list on connect
                let p15 = {
                    let gm = st.gm.lock().unwrap();
                    let repeat: Vec<P2B15Repeat> = gm
                        .iter()
                        .map(|(&aid, &lv)| P2B15Repeat {
                            account_id: AccountId(aid),
                            gm_level: GmLevel(lv),
                        })
                        .collect();
                    P2B15 { repeat }
                };
                send_must(&st, id, &tx_prio, enc(move |v| p15.encode(v))).await;
                map_kill_notify = Some(kill);
                break 'auth true;
            }
            Ok(Some(pkt)) => {
                tracing::warn!(
                    "maplink: first packet 0x{:04x} from {ip}, expected 0x2af8",
                    pkt.id
                );
                break 'auth false;
            }
            _ => break 'auth false,
        }
    };
    if !authed {
        return;
    }
    let map_id = map_id.unwrap();
    let kill = map_kill_notify.unwrap();

    // ---- main loop ----
    // `handle` never waits on SQLite (map-link DB work goes through
    // the `db_jobs` writer or a spawned task), so the socket drains
    // at wire speed; the only stalls left are a genuinely wedged
    // writer queue or a map that stopped reading.
    loop {
        let pkt = tokio::select! {
            p = fr.next() => match p {
                Ok(Some(p)) => p,
                _ => break,
            },
            _ = kill.notified() => {
                tracing::warn!(map_id, "map link killed: wedged on critical reply");
                break;
            }
        };
        let r = handle(&st, &tx_prio, map_id, pkt.id, &pkt.bytes).await;
        if r.is_err() {
            tracing::warn!("maplink: error handling 0x{:04x} from map {map_id}", pkt.id);
            break;
        }
    }

    st.map_unregister(map_id);
    drop(tx);
    let _ = wh.await;
    tracing::info!(map_id, "map server disconnected");
}

/// Update the online set from a 0x2aff list (port of char.cpp
/// parse_frommap 0x2aff).
fn on_user_list(st: &Arc<State>, map_id: usize, head_users: u16, chars: Vec<u32>) {
    {
        let mut online = st.online.lock().unwrap();
        online.retain(|_, v| *v != map_id);
        for cid in chars {
            online.insert(cid, map_id);
        }
    }
    {
        let mut ms = st.map_servers.lock().unwrap();
        if let Some(Some(h)) = ms.get_mut(map_id) {
            h.users = head_users;
        }
    }
    // keep the char cache's online_map in sync
    let online = st.online.lock().unwrap();
    let mut chars = st.chars.lock().unwrap();
    for c in chars.values_mut() {
        c.online_map = online.get(&c.key.char_id.0).copied();
    }
    drop(chars);
    drop(online);
    st.online_notify.notify_one();
    // 0x2b00 user count to all map servers
    let users = st.count_users() as u32;
    let mut p = P2B00::default();
    p.users = users;
    st.map_broadcast(&enc(move |v| p.encode(v)));
}

type HResult = Result<(), ()>;

async fn handle(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    id: u16,
    bytes: &[u8],
) -> HResult {
    tracing::debug!(map_id, "maplink rx 0x{id:04x} ({})", bytes.len());
    match id {
        0x2afa => {
            let Ok(p) = P2AFA::decode(bytes) else {
                return Err(());
            };
            let maps: Vec<String> = p
                .repeat
                .iter()
                .map(|r| r.map_name.to_string_lossy())
                .collect();
            {
                let mut ms = st.map_servers.lock().unwrap();
                if let Some(Some(h)) = ms.get_mut(map_id) {
                    h.maps = maps.clone();
                }
            }
            tracing::info!(map_id, maps = maps.len(), "map list received");
            let p = P2AFB::default();
            send_must(st, map_id, tx, enc(move |v| p.encode(v))).await;
            // 0x2b04: tell the others about this server; tell this
            // server about the others.
            let (ip, port) = st.map_addr(map_id).unwrap_or((0, 0));
            let others: Vec<(usize, u32, u16, Vec<String>)> = {
                let ms = st.map_servers.lock().unwrap();
                ms.iter()
                    .enumerate()
                    .filter_map(|(i, s)| {
                        s.as_ref()
                            .and_then(|h| (i != map_id).then(|| (i, h.ip, h.port, h.maps.clone())))
                    })
                    .collect()
            };
            if !maps.is_empty() {
                let mut head = P2B04::default();
                head.ip = ip4(ip);
                head.port = port;
                head.repeat = maps
                    .iter()
                    .map(|m| P2B04Repeat {
                        map_name: FixedStr::<16>::try_from_str(m).unwrap_or_default(),
                    })
                    .collect();
                for (oid, ..) in &others {
                    st.map_send(*oid, enc(|v| head.encode(v)));
                }
            }
            for (_, oip, oport, omaps) in others {
                if omaps.is_empty() {
                    continue;
                }
                let mut head = P2B04::default();
                head.ip = ip4(oip);
                head.port = oport;
                head.repeat = omaps
                    .iter()
                    .map(|m| P2B04Repeat {
                        map_name: FixedStr::<16>::try_from_str(m).unwrap_or_default(),
                    })
                    .collect();
                send_must(st, map_id, tx, enc(|v| head.encode(v))).await;
            }
            // Pre-auth: every player online elsewhere gets a 0x3829
            // on this new map, so it can accept transfers and
            // (re)logins without waiting for the gate. Replies
            // (0x3830) for unknown pending selects are ignored.
            for e in st.online_auths() {
                let mut p = P3829::default();
                p.account_id = AccountId(e.account_id);
                p.char_id = CharId(e.char_id);
                p.login_id1 = e.login_id1;
                p.login_id2 = e.login_id2;
                p.ip = ip4(e.ip);
                send_must(st, map_id, tx, enc(move |v| p.encode(v))).await;
            }
            Ok(())
        }
        0x2afc => {
            let Ok(fixed) = P2AFC::decode(bytes) else {
                return Err(());
            };
            // waits on in-flight saves and reads the DB: keep the
            // read loop hot by finishing in a task. The link's
            // registered client address is captured here: the slot
            // may be gone by the time the spawned task runs, and
            // the auth reservation keys on it.
            let st = st.clone();
            let tx = tx.clone();
            let (map_ip, map_port) = st.map_addr(map_id).unwrap_or((0, 0));
            tokio::spawn(async move {
                handle_auth_request(&st, &tx, map_id, map_ip, map_port, fixed).await;
            });
            Ok(())
        }
        0x2aff => {
            let Ok(p) = P2AFF::decode(bytes) else {
                return Err(());
            };
            let chars: Vec<u32> = p.repeat.iter().map(|r| r.char_id.0).collect();
            on_user_list(st, map_id, p.users, chars.clone());
            st.reconcile_online_auth(map_id, &chars);
            Ok(())
        }
        0x2b01 => {
            let Ok(p) = P2B01::decode(bytes) else {
                return Err(());
            };
            // update cache + persist (0x2b01 does not touch
            // account vars; see save_character). The write goes to
            // the serialized DB writer, which batches consecutive
            // saves into one transaction.
            {
                let mut chars = st.chars.lock().unwrap();
                if let Some(c) = chars.get_mut(&p.char_id.0) {
                    c.key = p.char_key;
                    c.data = p.char_data;
                }
            }
            // marks the write in-flight so a transferring char's
            // 0x2afc on another link can wait for it
            st.queue_save(p.char_id.0, p.char_key, p.char_data);
            Ok(())
        }
        0x2b02 => {
            let Ok(fixed) = P2B02::decode(bytes) else {
                return Err(());
            };
            // back to char select: fresh login ids -> stage-2 entry
            st.push_auth(super::state::AuthEntry {
                account_id: fixed.account_id.0,
                char_id: 0,
                login_id1: fixed.login_id1,
                login_id2: fixed.login_id2,
                ip: u32::from_le_bytes(fixed.ip.0),
                client_version: fixed.client_protocol_version.0,
                map_id: None,
                upstream_ip: None,
                delflag: 2,
                created: std::time::Instant::now(),
            });
            // the char is no longer online on that map
            {
                let mut chars = st.chars.lock().unwrap();
                for c in chars.values_mut() {
                    if c.key.account_id.0 == fixed.account_id.0 {
                        c.online_map = None;
                    }
                }
            }
            {
                let mut online = st.online.lock().unwrap();
                online.retain(|_, v| *v != map_id);
            }
            st.online_notify.notify_one();
            st.drop_account_online_auth(fixed.account_id.0);
            let mut p = P2B03::default();
            p.account_id = fixed.account_id;
            p.unknown = 0;
            send_must(st, map_id, tx, enc(move |v| p.encode(v))).await;
            Ok(())
        }
        0x2b05 => {
            let Ok(fixed) = P2B05::decode(bytes) else {
                return Err(());
            };
            // map-to-map move: the source just sent 0x2b01 on this
            // link and the client now heads to another server. Mark
            // the transfer so that link's 0x2afc waits for the save,
            // and create a map-stage entry so it can match.
            st.transfer_mark(fixed.account_id.0, fixed.char_id.0);
            st.set_online_auth(super::state::OnlineAuth {
                account_id: fixed.account_id.0,
                char_id: fixed.char_id.0,
                login_id1: fixed.login_id1,
                login_id2: fixed.login_id2,
                ip: u32::from_le_bytes(fixed.client_ip.0),
                server: map_id,
            });
            let ok = st
                .load_char(fixed.char_id.0)
                .await
                .map(|r| r.key.account_id.0 == fixed.account_id.0)
                .unwrap_or(false);
            if ok {
                st.push_auth(super::state::AuthEntry {
                    account_id: fixed.account_id.0,
                    char_id: fixed.char_id.0,
                    login_id1: fixed.login_id1,
                    login_id2: fixed.login_id2,
                    ip: u32::from_le_bytes(fixed.client_ip.0),
                    client_version: 0,
                    map_id: None,
                    upstream_ip: None,
                    delflag: 3,
                    created: std::time::Instant::now(),
                });
                // make sure the destination map holds a fresh
                // pre-auth entry: the one it got at registration
                // may have been evicted (bounded auth_fifo) or
                // never sent. Without it the client's 0x0072 is
                // rejected as "not auth account". Spawned: a wedged
                // destination link must not stall this reader, and
                // the client takes longer to reconnect than this
                // push takes anyway; a miss falls back to 0x2afc.
                let dest_ip = u32::from_le_bytes(fixed.map_ip.0);
                match st.map_by_addr(dest_ip, fixed.map_port) {
                    Some(dest) => {
                        let st2 = st.clone();
                        tokio::spawn(async move {
                            let Some(dtx) = st2.map_prio_tx(dest) else {
                                return;
                            };
                            let mut p29 = P3829::default();
                            p29.account_id = fixed.account_id;
                            p29.char_id = fixed.char_id;
                            p29.login_id1 = fixed.login_id1;
                            p29.login_id2 = fixed.login_id2;
                            p29.ip = fixed.client_ip;
                            send_must(&st2, dest, &dtx, enc(move |v| p29.encode(v))).await;
                        });
                    }
                    None => {
                        tracing::warn!(
                            map_id,
                            "0x2b05 names unknown destination {}:{}",
                            Ipv4Addr::from(fixed.map_ip.0),
                            fixed.map_port
                        );
                    }
                }
            }
            let mut p = P2B06::default();
            p.account_id = fixed.account_id;
            p.error = if ok { 0 } else { 1 };
            p.unknown = 0;
            p.char_id = fixed.char_id;
            p.map_name = fixed.map_name;
            p.x = fixed.x;
            p.y = fixed.y;
            p.map_ip = fixed.map_ip;
            p.map_port = fixed.map_port;
            send_must(st, map_id, tx, enc(move |v| p.encode(v))).await;
            Ok(())
        }
        0x2b0c => {
            let Ok(fixed) = P2B0C::decode(bytes) else {
                return Err(());
            };
            let old = fixed.old_email.to_string_lossy();
            let new = fixed.new_email.to_string_lossy();
            let aid = fixed.account_id.0 as i64;
            st.queue_db_op(move |conn| {
                let cur: Option<String> = conn
                    .query_row("SELECT email FROM accounts WHERE id=?1", [aid], |r| {
                        r.get::<_, Option<String>>(0)
                    })
                    .ok()
                    .flatten();
                if cur.unwrap_or_default() == old {
                    let _ = conn.execute(
                        "UPDATE accounts SET email=?2 WHERE id=?1",
                        rusqlite::params![aid, new],
                    );
                }
                super::state::DbOpResult::none()
            });
            Ok(())
        }
        0x2b0e => {
            let Ok(fixed) = P2B0E::decode(bytes) else {
                return Err(());
            };
            // char name lookup may hit SQLite
            let st = st.clone();
            let tx = tx.clone();
            tokio::spawn(async move {
                let _ = handle_named_op(&st, &tx, map_id, fixed).await;
            });
            Ok(())
        }
        0x2b10 => {
            let Ok(p) = P2B10::decode(bytes) else {
                return Err(());
            };
            // ## vars: in-memory CharData + DB scope 2
            let aid = p.account_id.0;
            let regs: Vec<(String, i64)> = p
                .repeat
                .iter()
                .map(|r| (r.name.to_string_lossy(), r.value as i64))
                .collect();
            {
                // update cached CharData account_reg2 (set_account_reg2)
                let mut chars = st.chars.lock().unwrap();
                for c in chars.values_mut() {
                    if c.key.account_id.0 == aid {
                        c.data.account_reg2_num = regs.len().min(ACCOUNT_REG2_NUM) as i32;
                        for (i, (n, v)) in regs.iter().enumerate().take(ACCOUNT_REG2_NUM) {
                            c.data.account_reg2[i] = GlobalReg {
                                str: FixedStr::<32>::try_from_str(n).unwrap_or_default(),
                                value: *v as i32,
                            };
                        }
                    }
                }
            }
            st.queue_db_op(move |conn| {
                // tmwa replaces the whole scope with the incoming list
                let _ = conn.execute(
                    "DELETE FROM account_vars WHERE account_id=?1 AND scope=2",
                    [aid as i64],
                );
                let _ = crate::db::set_account_vars(conn, aid as i64, 2, &regs);
                super::state::DbOpResult::none()
            });
            Ok(())
        }
        0x2b16 => {
            let Ok(fixed) = P2B16::decode(bytes) else {
                return Err(());
            };
            let cid = fixed.char_id.0;
            let st2 = st.clone();
            st.queue_db_op(move |conn| {
                let partner = crate::db::divorce_conn(conn, cid as i64)
                    .ok()
                    .flatten()
                    .unwrap_or(0);
                let mut p = P2B12::default();
                p.char_id = fixed.char_id;
                p.partner_id = CharId(partner as u32);
                super::state::DbOpResult {
                    reply: super::state::LinkReply::Broadcast,
                    bytes: enc(move |v| p.encode(v)),
                    after: Some(Box::new(move || {
                        let mut chars = st2.chars.lock().unwrap();
                        if let Some(c) = chars.get_mut(&cid) {
                            c.data.partner_id = CharId(0);
                        }
                        if let Some(c) = chars.get_mut(&(partner as u32)) {
                            c.data.partner_id = CharId(0);
                        }
                    })),
                }
            });
            Ok(())
        }
        0x2b17 => {
            // tmwa-map is in term_func: the link will drop shortly
            // and every player on it needs holding.
            st.map_set_shutting_down(map_id);
            tracing::info!(map_id, "map announced shutdown");
            Ok(())
        }
        0x3830 => {
            let Ok(fixed) = P3830::decode(bytes) else {
                return Err(());
            };
            let key = (fixed.account_id.0, fixed.char_id.0);
            // a rejoining player's relay is waiting on this
            if let Some(notify) = st.rejoin_notify.lock().unwrap().remove(&key) {
                let _ = notify.send(());
                return Ok(());
            }
            if let Some(ps) = st.sel_waiting_done(key, map_id, fixed.login_id1, fixed.login_id2) {
                st.send_pending_sel(ps);
            }
            Ok(())
        }

        // ---- inter channel ----
        0x3000 => {
            // GM broadcast -> 0x3800 to all map servers
            let Ok(p) = P3000::decode(bytes) else {
                return Err(());
            };
            let mut p2 = P3800::default();
            p2.repeat = p.repeat.iter().map(|r| P3800Repeat { c: r.c }).collect();
            st.map_broadcast(&enc(move |v| p2.encode(v)));
            Ok(())
        }
        0x3001 => {
            // whisper request: forward to the map holding the target
            let Ok(p) = P3001::decode(bytes) else {
                return Err(());
            };
            // char_by_name can hit SQLite on a cache miss
            let st = st.clone();
            let tx = tx.clone();
            tokio::spawn(async move {
                let from = p.from_char_name.to_string_lossy();
                let mut to = p.to_char_name.to_string_lossy();
                let target = st.char_by_name(&to).await;
                let target_map =
                    target.and_then(|cid| st.online.lock().unwrap().get(&cid).copied());
                match (target, target_map) {
                    (Some(cid), Some(tmid)) if from != to => {
                        // rewrite to canonical name
                        let cname = {
                            let chars = st.chars.lock().unwrap();
                            chars.get(&cid).map(|c| c.key.name.to_string_lossy())
                        };
                        if let Some(n) = cname {
                            to = n;
                        }
                        let mut h = P3801::default();
                        h.whisper_id = CharId(cid);
                        h.src_char_name = FixedStr::<24>::try_from_str(&from).unwrap_or_default();
                        h.dst_char_name = FixedStr::<24>::try_from_str(&to).unwrap_or_default();
                        h.repeat = p.repeat.iter().map(|r| P3801Repeat { c: r.c }).collect();
                        st.map_send(tmid, enc(move |v| h.encode(v)));
                    }
                    _ => {
                        let mut p2 = P3802::default();
                        p2.sender_char_name =
                            FixedStr::<24>::try_from_str(&from).unwrap_or_default();
                        p2.flag = 1;
                        send_must(&st, map_id, &tx, enc(move |v| p2.encode(v))).await;
                    }
                }
            });
            Ok(())
        }
        0x3002 => {
            // whisper result -> 0x3802 to the *sender's* map
            let Ok(fixed) = P3002::decode(bytes) else {
                return Err(());
            };
            let cid = fixed.char_id.0;
            let sender_map = st.online.lock().unwrap().get(&cid).copied();
            if let Some(smap) = sender_map {
                let name = {
                    let chars = st.chars.lock().unwrap();
                    chars.get(&cid).map(|c| c.key.name.to_string_lossy())
                };
                if let Some(n) = name {
                    let mut p = P3802::default();
                    p.sender_char_name = FixedStr::<24>::try_from_str(&n).unwrap_or_default();
                    p.flag = fixed.flag;
                    st.map_send(smap, enc(move |v| p.encode(v)));
                }
            }
            Ok(())
        }
        0x3003 => {
            let Ok(p) = P3003::decode(bytes) else {
                return Err(());
            };
            let mut p2 = P3803::default();
            p2.char_name = p.char_name;
            p2.min_gm_level = p.min_gm_level;
            p2.repeat = p.repeat.iter().map(|r| P3803Repeat { c: r.c }).collect();
            st.map_broadcast(&enc(move |v| p2.encode(v)));
            Ok(())
        }
        0x3004 => {
            // # vars save: DB scope 1 + 0x3804 to the other maps
            let Ok(p) = P3004::decode(bytes) else {
                return Err(());
            };
            let aid = p.account_id.0;
            let regs: Vec<(String, i64)> = p
                .repeat
                .iter()
                .map(|r| (r.name.to_string_lossy(), r.value as i64))
                .collect();
            {
                let mut chars = st.chars.lock().unwrap();
                for c in chars.values_mut() {
                    if c.key.account_id.0 == aid {
                        c.data.account_reg_num = regs.len().min(ACCOUNT_REG_NUM) as i32;
                        for (i, (n, v)) in regs.iter().enumerate().take(ACCOUNT_REG_NUM) {
                            c.data.account_reg[i] = GlobalReg {
                                str: FixedStr::<32>::try_from_str(n).unwrap_or_default(),
                                value: *v as i32,
                            };
                        }
                    }
                }
            }
            let regs2 = regs.clone();
            st.queue_db_op(move |conn| {
                let _ = conn.execute(
                    "DELETE FROM account_vars WHERE account_id=?1 AND scope=1",
                    [aid as i64],
                );
                let _ = crate::db::set_account_vars(conn, aid as i64, 1, &regs2);
                super::state::DbOpResult::none()
            });
            // 0x3804 to all OTHER map servers
            let mut h = P3804::default();
            h.account_id = p.account_id;
            h.repeat = regs
                .iter()
                .map(|(n, v)| P3804Repeat {
                    name: FixedStr::<32>::try_from_str(n).unwrap_or_default(),
                    value: *v as u32,
                })
                .collect();
            let others: Vec<usize> = {
                let ms = st.map_servers.lock().unwrap();
                ms.iter()
                    .enumerate()
                    .filter_map(|(i, s)| s.as_ref().and_then(|_| (i != map_id).then_some(i)))
                    .collect()
            };
            for oid in others {
                st.map_send(oid, enc(|v| h.encode(v)));
            }
            Ok(())
        }
        0x3005 => {
            // accreg request -> 0x3804 to requester after commit;
            // deduped: the map re-requests on reply timeout and each
            // re-request was a full accreg reply
            let Ok(fixed) = P3005::decode(bytes) else {
                return Err(());
            };
            let aid = fixed.account_id.0 as i64;
            st.queue_dedup_op(DEDUP_ACCREG, aid, move |conn| {
                let vars = crate::db::get_account_vars_conn(conn, aid, 1).unwrap_or_default();
                let mut p = P3804::default();
                p.account_id = fixed.account_id;
                p.repeat = vars
                    .iter()
                    .map(|(n, v)| P3804Repeat {
                        name: FixedStr::<32>::try_from_str(n).unwrap_or_default(),
                        value: *v as u32,
                    })
                    .collect();
                super::state::DbOpResult::reply(map_id, enc(move |v| p.encode(v)))
            });
            Ok(())
        }
        0x3010 => {
            // storage load -> 0x3810 after commit; going through the
            // DB queue keeps it ordered behind a queued 0x3011 save
            // for the same account.
            let Ok(fixed) = P3010::decode(bytes) else {
                return Err(());
            };
            let aid = fixed.account_id.0 as i64;
            st.queue_dedup_op(DEDUP_STORAGE, aid, move |conn| {
                let items = crate::db::load_storage_conn(conn, aid).unwrap_or_default();
                let mut storage = Storage::default();
                storage.account_id = fixed.account_id;
                let mut n = 0;
                for (_, item_id, amount, equip) in items {
                    if n < storage.storage_.len() {
                        storage.storage_[n] = Item {
                            nameid: ItemNameId(item_id as u32),
                            amount: amount as i16,
                            equip: Epos(equip as u16),
                        };
                        n += 1;
                    }
                }
                storage.storage_amount = n as i16;
                let mut p = P3810::default();
                p.account_id = fixed.account_id;
                p.storage = storage;
                super::state::DbOpResult::reply(map_id, enc(move |v| p.encode(v)))
            });
            Ok(())
        }
        0x3011 => {
            // storage save -> DB + 0x3811 ack after commit
            let Ok(p) = P3011::decode(bytes) else {
                return Err(());
            };
            let aid = p.account_id.0 as i64;
            let items: Vec<(i64, i64, i64)> = p
                .storage
                .storage_
                .iter()
                .take(p.storage.storage_amount.max(0) as usize)
                .filter(|it| it.nameid.0 != 0 && it.amount != 0)
                .map(|it| (it.nameid.0 as i64, it.amount as i64, it.equip.0 as i64))
                .collect();
            st.queue_db_op(move |conn| {
                let _ = crate::db::save_storage_conn(conn, aid, &items);
                let mut ack = P3811::default();
                ack.account_id = p.account_id;
                ack.unknown = 0;
                super::state::DbOpResult::reply(map_id, enc(move |v| ack.encode(v)))
            });
            Ok(())
        }

        // ---- parties ----
        0x3020 => party_create(st, tx, map_id, bytes).await,
        0x3021 => party_info(st, tx, map_id, bytes).await,
        0x3022 => party_add(st, tx, map_id, bytes).await,
        0x3023 => party_option(st, tx, map_id, bytes).await,
        0x3024 => party_leave(st, bytes).await,
        0x3025 => party_map_change(st, tx, bytes).await,
        0x3026 => party_leader(st, bytes).await,
        0x3027 => party_message(st, bytes).await,
        0x3028 => party_check(st, bytes).await,

        _ => {
            tracing::debug!(map_id, "unknown packet 0x{id:04x} from map");
            Err(())
        }
    }
}

fn ip4(v: u32) -> Ip4Address {
    Ip4Address(v.to_le_bytes())
}

/// `queue_dedup_op` kind for 0x3010 storage requests.
const DEDUP_STORAGE: u8 = 1;
/// `queue_dedup_op` kind for 0x3005 accreg requests.
const DEDUP_ACCREG: u8 = 2;

/// 0x2afc auth request: waits out in-flight saves for the char, then
/// answers with the CharData. Runs as its own task so the link's
/// read loop is never blocked by SQLite. `map_ip`/`map_port` are
/// the requesting link's registered client address: a flap-repeat
/// is only re-served to the same server.
async fn handle_auth_request(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    map_ip: u32,
    map_port: u16,
    fixed: P2AFC,
) {
    // A map-to-map transfer is in flight for this char: the
    // source link's pre-0x2b05 save may still be committing.
    // Any in-flight save for this char is waited out
    // (bounded) so this link's answer carries the fresh
    // CharData; an unmarked quit-save racing a relog is the
    // same hazard.
    let marked = st.transfer_take(fixed.account_id.0, fixed.char_id.0);
    if !st
        .wait_saves(fixed.char_id.0, std::time::Duration::from_secs(2))
        .await
    {
        tracing::warn!(
            map_id,
            char_id = fixed.char_id.0,
            transfer = marked,
            "0x2afc raced a queued save; answering with current data"
        );
    }
    // tmwa compares afi.ip to the ip the map reports; through
    // the relay that's the gate's upstream source address,
    // recorded at relay time. Fall back to the client ip.
    let req = super::state::MapAuthReq {
        account_id: fixed.account_id.0,
        char_id: fixed.char_id.0,
        login_id1: fixed.login_id1,
        login_id2: fixed.login_id2,
        ip: u32::from_le_bytes(fixed.ip.0),
    };
    let entry = st.take_map_auth(req, map_ip, map_port);
    let Some((e, reserve)) = entry else {
        // No pending entry: the map re-pushes pending 0x2afc
        // requests when the link flaps, so this may repeat a
        // request whose 0x2afd never arrived. Re-serving to the
        // same server is safe; anything else is a real reject.
        if reserve_map_auth(st, tx, map_id, map_ip, map_port, &fixed).await {
            return;
        }
        tracing::warn!(
            "maplink: REJECTED 0x2afc account {} char {}",
            fixed.account_id.0,
            fixed.char_id.0
        );
        let mut p = P2AFE::default();
        p.account_id = fixed.account_id;
        send_must(st, map_id, tx, enc(move |v| p.encode(v))).await;
        return;
    };
    // load CharKey + CharData (full, incl. account_reg* + vars)
    let cid = e.char_id;
    let res = {
        let st2 = st.clone();
        tokio::task::spawn_blocking(move || st2.db.load_character(cid as i64)).await
    };
    let Ok(Ok((key, cd))) = res else {
        reserve.send_replace(super::state::ServedReply::Failed);
        let mut p = P2AFE::default();
        p.account_id = fixed.account_id;
        send_must(st, map_id, tx, enc(move |v| p.encode(v))).await;
        return;
    };
    // update cache
    st.chars.lock().unwrap().insert(
        cid,
        super::state::CharRecord {
            key,
            data: cd,
            online_map: Some(map_id),
        },
    );
    // the char is (about to be) online on this map
    st.online.lock().unwrap().insert(cid, map_id);
    st.online_notify.notify_one();
    // a transfer that landed: count it for any drain in flight
    if marked {
        st.drain_track_arrive(map_id, cid);
    }
    // remember the auth material so a map registering later
    // can be pre-authed for this player
    st.set_online_auth(super::state::OnlineAuth {
        account_id: e.account_id,
        char_id: e.char_id,
        login_id1: e.login_id1,
        login_id2: e.login_id2,
        ip: e.ip,
        server: map_id,
    });
    let mut p = P2AFD::default();
    p.account_id = fixed.account_id;
    p.login_id2 = e.login_id2;
    p.client_protocol_version = ClientVersion(e.client_version);
    p.char_key = key;
    p.char_data = cd;
    let bytes = enc(move |v| p.encode(v));
    // resolve the reservation before the send: if the link dies
    // with this reply in flight, the map's re-pushed 0x2afc gets
    // the same answer instead of a reject.
    reserve.send_replace(super::state::ServedReply::Ready(bytes.clone()));
    send_must(st, map_id, tx, bytes).await;
    tracing::info!(map_id, char_id = cid, "authenticated char for map");
}

/// Repeated 0x2afc with no pending entry: if the request was
/// already answered on a link that flapped, wait (bounded) for the
/// original serve's reply and send the same bytes to the same
/// server. Returns false when there is no matching reservation or
/// it resolved to failure; the caller then sends a 0x2afe.
async fn reserve_map_auth(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    map_ip: u32,
    map_port: u16,
    fixed: &P2AFC,
) -> bool {
    let req = super::state::MapAuthReq {
        account_id: fixed.account_id.0,
        char_id: fixed.char_id.0,
        login_id1: fixed.login_id1,
        login_id2: fixed.login_id2,
        ip: u32::from_le_bytes(fixed.ip.0),
    };
    let Some((mut rx, auth)) = st.served_map_auth(req, map_id, map_ip, map_port) else {
        return false;
    };
    // the Ref borrows the channel and is not Send: scope it so it
    // is gone before the send_must await below
    let bytes = {
        let resolved = tokio::time::timeout(
            std::time::Duration::from_secs(5),
            rx.wait_for(|r| !matches!(r, super::state::ServedReply::Pending)),
        )
        .await;
        match resolved {
            Ok(Ok(r)) => match &*r {
                super::state::ServedReply::Ready(b) => b.clone(),
                _ => return false,
            },
            _ => return false,
        }
    };
    // the char is on this (new) link now: refresh the online
    // bookkeeping the dead link's unregister dropped
    let cid = fixed.char_id.0;
    st.online.lock().unwrap().insert(cid, map_id);
    st.online_notify.notify_one();
    if let Some(c) = st.chars.lock().unwrap().get_mut(&cid) {
        c.online_map = Some(map_id);
    }
    st.set_online_auth(auth);
    send_must(st, map_id, tx, bytes).await;
    tracing::info!(map_id, char_id = cid, "re-served auth for map");
    true
}

/// 0x2b0e named-char ops: block(1)/ban(2)/unblock(3)/unban(4) against
/// a character found by name; operation 5 (changesex) fails since
/// accounts have no sex anymore.
async fn handle_named_op(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    fixed: P2B0E,
) -> HResult {
    let acc = fixed.account_id.0;
    let cname = fixed.char_name.to_string_lossy();
    let op = fixed.operation;
    let mut reply = P2B0F::default();
    reply.account_id = fixed.account_id;
    reply.operation = op;
    let cid = st.char_by_name(&cname).await;
    match cid {
        None => {
            reply.char_name = fixed.char_name;
            reply.error = 1;
        }
        Some(cid) => {
            let rec = st.load_char(cid).await;
            let target_acc = rec.as_ref().map(|r| r.key.account_id.0).unwrap_or(0);
            reply.char_name = rec.as_ref().map(|r| r.key.name).unwrap_or(fixed.char_name);
            // gm-level check: requester must outrank the target
            // (account_id 0 = server-internal, always allowed)
            let requester_gm = if acc == 0 { u32::MAX } else { is_gm(st, acc) };
            let target_gm = is_gm(st, target_acc);
            if requester_gm <= target_gm && acc != 0 {
                reply.error = 2;
            } else {
                reply.error = 0;
                let aid = target_acc as i64;
                match op {
                    1 => {
                        let _ = st.db.blocking(move |db| db.set_account_state(aid, 5)).await;
                        kick_online(st, target_acc, 5, 0);
                    }
                    2 => {
                        // ban_add is a HumanTimeDiff added to now
                        let until = std::time::SystemTime::now()
                            .duration_since(std::time::UNIX_EPOCH)
                            .unwrap_or_default()
                            .as_secs() as i64
                            + htd_seconds(&fixed.ban_add);
                        let _ = st
                            .db
                            .blocking(move |db| db.set_account_ban(aid, until))
                            .await;
                        kick_online(st, target_acc, 1, until);
                    }
                    3 => {
                        let _ = st.db.blocking(move |db| db.set_account_state(aid, 0)).await;
                    }
                    4 => {
                        let _ = st.db.blocking(move |db| db.set_account_ban(aid, 0)).await;
                    }
                    _ => {
                        // changesex etc: no account sex anymore
                        reply.error = 1;
                    }
                }
            }
        }
    }
    // reply only when a player asked (acc != 0)
    if acc != 0 {
        send_must(st, map_id, tx, enc(move |v| reply.encode(v))).await;
    }
    Ok(())
}

fn htd_seconds(h: &HumanTimeDiff) -> i64 {
    h.year as i64 * 31536000
        + h.month as i64 * 2592000
        + h.day as i64 * 86400
        + h.hour as i64 * 3600
        + h.minute as i64 * 60
        + h.second as i64
}

/// 0x2b14 to all maps + drop the char-screen session.
fn kick_online(st: &Arc<State>, account_id: u32, ban_not_status: u8, until: i64) {
    let mut p = P2B14::default();
    p.account_id = AccountId(account_id);
    p.ban_not_status = ban_not_status;
    p.status_or_ban_until = crate::proto::types::TimeT(until);
    st.map_broadcast(&enc(move |v| p.encode(v)));
    if let Some(txs) = st.char_sessions.lock().unwrap().remove(&account_id) {
        drop(txs);
    }
}

// ---------------- parties (int_party.cpp port) ----------------

fn party_get(st: &State, party_id: u32) -> Option<PartyMost> {
    st.parties.lock().unwrap().get(&party_id).copied()
}

fn party_put(st: &std::sync::Arc<State>, party_id: u32, p: PartyMost) {
    st.parties.lock().unwrap().insert(party_id, p);
    persist_party(st, party_id);
}

fn party_del(st: &std::sync::Arc<State>, party_id: u32) {
    st.parties.lock().unwrap().remove(&party_id);
    st.queue_db_op(move |conn| {
        let _ = conn.execute("DELETE FROM parties WHERE id=?1", [party_id as i64]);
        super::state::DbOpResult::none()
    });
}

fn persist_party(st: &std::sync::Arc<State>, party_id: u32) {
    let p = match party_get(st, party_id) {
        Some(p) => p,
        None => return,
    };
    let name = p.name.to_string_lossy();
    let members: Vec<(i64, String, i64)> = p
        .member
        .iter()
        .filter(|m| m.account_id.0 != 0)
        .map(|m| {
            (
                m.account_id.0 as i64,
                m.name.to_string_lossy(),
                m.leader as i64,
            )
        })
        .collect();
    let (exp, item) = (p.exp as i64, p.item as i64);
    st.queue_db_op(move |conn| {
        let _ = conn.execute(
            "INSERT INTO parties(id,name,exp_share,item_share) VALUES(?1,?2,?3,?4)
             ON CONFLICT(id) DO UPDATE SET name=excluded.name,
             exp_share=excluded.exp_share,item_share=excluded.item_share",
            rusqlite::params![party_id as i64, name, exp, item],
        );
        let _ = conn.execute(
            "DELETE FROM party_members WHERE party_id=?1",
            [party_id as i64],
        );
        for (a, n, l) in &members {
            let _ = conn.execute(
                "INSERT INTO party_members(party_id,account_id,char_name,leader)
                 VALUES(?1,?2,?3,?4)",
                rusqlite::params![party_id as i64, a, n, l],
            );
        }
        super::state::DbOpResult::none()
    });
}

/// party_check_exp_share: exp share legal when online levels are
/// within party_share_level of each other (or nobody online).
fn party_check_exp_share(st: &State, p: &PartyMost) -> bool {
    let mut maxlv = 0i32;
    let mut minlv = i32::MAX;
    for m in p.member.iter() {
        if m.online != 0 {
            if m.lv < minlv {
                minlv = m.lv;
            }
            if m.lv > maxlv {
                maxlv = m.lv;
            }
        }
    }
    maxlv == 0 || maxlv - minlv <= st.cfg.inter.party_share_level as i32
}

fn party_check_empty(st: &std::sync::Arc<State>, party_id: u32) -> bool {
    if let Some(p) = party_get(st, party_id) {
        if p.member.iter().any(|m| m.account_id.0 != 0) {
            return false;
        }
    }
    // empty -> disband
    let mut p = P3826::default();
    p.party_id = PartyId(party_id);
    p.flag = 0;
    st.map_broadcast(&enc(move |v| p.encode(v)));
    party_del(st, party_id);
    true
}

async fn party_info_to(
    st: &State,
    tx: Option<&mpsc::Sender<Vec<u8>>>,
    map_id: usize,
    party_id: u32,
) {
    if let Some(p) = party_get(st, party_id) {
        let mut h = P3821::default();
        h.party_id = PartyId(party_id);
        h.option = Some(P3821Option { party_most: p });
        match tx {
            Some(t) => {
                send_must(st, map_id, t, enc(move |v| h.encode(v))).await;
            }
            None => {
                st.map_broadcast(&enc(move |v| h.encode(v)));
            }
        }
    } else if let Some(t) = tx {
        let mut h = P3821::default();
        h.party_id = PartyId(party_id);
        h.option = None;
        send_must(st, map_id, t, enc(move |v| h.encode(v))).await;
    }
}

async fn party_create(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    bytes: &[u8],
) -> HResult {
    let Ok(fixed) = P3020::decode(bytes) else {
        return Err(());
    };
    let name = fixed.party_name.to_string_lossy();
    let reply = |error: u8, pid: u32, pname: String| {
        let mut p = P3820::default();
        p.account_id = fixed.account_id;
        p.error = error;
        p.party_id = PartyId(pid);
        p.party_name = FixedStr::<24>::try_from_str(&pname).unwrap_or_default();
        enc(move |v| p.encode(v))
    };
    if name.is_empty() || !name.bytes().all(|b| (32..=126).contains(&b)) {
        send_must(st, map_id, tx, reply(1, 0, "error".into())).await;
        return Ok(());
    }
    // name collision
    if st
        .parties
        .lock()
        .unwrap()
        .values()
        .any(|p| p.name.to_string_lossy() == name)
    {
        send_must(st, map_id, tx, reply(1, 0, "error".into())).await;
        return Ok(());
    }
    // tmwa's party_newid starts at 0 and is pre-incremented: the
    // first party gets id 1 (id 0 means "no party" to the map).
    let pid = st
        .next_party_id
        .fetch_add(1, std::sync::atomic::Ordering::SeqCst) as u32
        + 1;
    let mut p = PartyMost::default();
    p.name = FixedStr::<24>::try_from_str(&name).unwrap_or_default();
    p.exp = 0;
    p.item = 0;
    p.member[0] = PartyMember {
        account_id: fixed.account_id,
        name: fixed.char_name,
        map: fixed.map_name,
        leader: 1,
        online: 1,
        lv: fixed.level as i32,
    };
    party_put(st, pid, p);
    st.queue_db_op(move |conn| {
        let _ = conn.execute(
            "INSERT INTO meta(key,value) VALUES('next_party_id',?1)
             ON CONFLICT(key) DO UPDATE SET value=excluded.value",
            [pid as i64],
        );
        super::state::DbOpResult::none()
    });
    send_must(st, map_id, tx, reply(0, pid, name)).await;
    party_info_to(st, Some(tx), map_id, pid).await;
    Ok(())
}

async fn party_info(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    bytes: &[u8],
) -> HResult {
    let Ok(fixed) = P3021::decode(bytes) else {
        return Err(());
    };
    party_info_to(st, Some(tx), map_id, fixed.party_id.0).await;
    Ok(())
}

async fn party_add(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    bytes: &[u8],
) -> HResult {
    let Ok(fixed) = P3022::decode(bytes) else {
        return Err(());
    };
    let pid = fixed.party_id.0;
    let reply = |flag: u8| {
        let mut p = P3822::default();
        p.party_id = fixed.party_id;
        p.account_id = fixed.account_id;
        p.flag = flag;
        enc(move |v| p.encode(v))
    };
    let mut p = match party_get(st, pid) {
        Some(p) => p,
        None => {
            send_must(st, map_id, tx, reply(1)).await;
            return Ok(());
        }
    };
    for i in 0..MAX_PARTY {
        if p.member[i].account_id.0 == 0 {
            p.member[i] = PartyMember {
                account_id: fixed.account_id,
                name: fixed.char_name,
                map: fixed.map_name,
                leader: 0,
                online: 1,
                lv: fixed.level as i32,
            };
            send_must(st, map_id, tx, reply(0)).await;
            let mut flag = 0u8;
            if p.exp > 0 && !party_check_exp_share(st, &p) {
                p.exp = 0;
                flag = 0x01;
            }
            if flag != 0 {
                let mut o = P3823::default();
                o.party_id = fixed.party_id;
                o.account_id = AccountId(0);
                o.exp = p.exp as u16;
                o.item = p.item as u16;
                o.flag = flag;
                st.map_broadcast(&enc(move |v| o.encode(v)));
            }
            party_put(st, pid, p);
            // broadcast AFTER the update, or the member table the
            // maps learn is missing the new member
            party_info_to(st, None, map_id, pid).await;
            return Ok(());
        }
    }
    send_must(st, map_id, tx, reply(1)).await;
    Ok(())
}

async fn party_option(
    st: &Arc<State>,
    tx: &mpsc::Sender<Vec<u8>>,
    map_id: usize,
    bytes: &[u8],
) -> HResult {
    let Ok(fixed) = P3023::decode(bytes) else {
        return Err(());
    };
    let pid = fixed.party_id.0;
    let mut p = match party_get(st, pid) {
        Some(p) => p,
        None => return Ok(()),
    };
    p.exp = fixed.exp as i32;
    let mut flag = 0u8;
    if p.exp > 0 && !party_check_exp_share(st, &p) {
        flag |= 0x01;
        p.exp = 0;
    }
    p.item = fixed.item as i32;
    let mut o = P3823::default();
    o.party_id = fixed.party_id;
    o.account_id = fixed.account_id;
    o.exp = p.exp as u16;
    o.item = p.item as u16;
    o.flag = flag;
    if flag == 0 {
        st.map_broadcast(&enc(move |v| o.encode(v)));
    } else {
        send_must(st, map_id, tx, enc(move |v| o.encode(v))).await;
    }
    party_put(st, pid, p);
    Ok(())
}

async fn party_leave(st: &Arc<State>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3024::decode(bytes) else {
        return Err(());
    };
    party_leave_do(st, fixed.party_id.0, fixed.account_id.0).await;
    Ok(())
}

pub(crate) async fn party_leave_do(st: &Arc<State>, pid: u32, account_id: u32) {
    let mut p = match party_get(st, pid) {
        Some(p) => p,
        None => return,
    };
    for i in 0..MAX_PARTY {
        if p.member[i].account_id.0 != account_id {
            continue;
        }
        let name = p.member[i].name;
        let mut n = P3824::default();
        n.party_id = PartyId(pid);
        n.account_id = AccountId(account_id);
        n.char_name = name;
        st.map_broadcast(&enc(move |v| n.encode(v)));
        p.member[i] = PartyMember::default();
        party_put(st, pid, p);
        if !party_check_empty(st, pid) {
            party_info_to(st, None, usize::MAX, pid).await;
        }
        return;
    }
}

async fn party_map_change(st: &Arc<State>, _tx: &mpsc::Sender<Vec<u8>>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3025::decode(bytes) else {
        return Err(());
    };
    let pid = fixed.party_id.0;
    let mut p = match party_get(st, pid) {
        Some(p) => p,
        None => return Ok(()),
    };
    for i in 0..MAX_PARTY {
        if p.member[i].account_id.0 != fixed.account_id.0 {
            continue;
        }
        p.member[i].map = fixed.map_name;
        p.member[i].online = fixed.online as i32;
        p.member[i].lv = fixed.level as i32;
        let m = p.member[i];
        let mut n = P3825::default();
        n.party_id = PartyId(pid);
        n.account_id = m.account_id;
        n.map_name = m.map;
        n.online = m.online as u8;
        n.level = m.lv as u16;
        st.map_broadcast(&enc(move |v| n.encode(v)));
        let mut flag = 0u8;
        if p.exp > 0 && !party_check_exp_share(st, &p) {
            p.exp = 0;
            flag = 1;
        }
        if flag != 0 {
            let mut o = P3823::default();
            o.party_id = PartyId(pid);
            o.account_id = AccountId(0);
            o.exp = p.exp as u16;
            o.item = p.item as u16;
            o.flag = flag;
            st.map_broadcast(&enc(move |v| o.encode(v)));
        }
        party_put(st, pid, p);
        return Ok(());
    }
    Ok(())
}

async fn party_leader(st: &Arc<State>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3026::decode(bytes) else {
        return Err(());
    };
    let pid = fixed.party_id.0;
    let mut p = match party_get(st, pid) {
        Some(p) => p,
        None => return Ok(()),
    };
    for i in 0..MAX_PARTY {
        if p.member[i].account_id.0 != fixed.account_id.0 {
            continue;
        }
        p.member[i].leader = fixed.leader as i32;
        let mut n = P3828::default();
        n.party_id = PartyId(pid);
        n.account_id = fixed.account_id;
        n.leader = fixed.leader;
        st.map_broadcast(&enc(move |v| n.encode(v)));
        party_put(st, pid, p);
        return Ok(());
    }
    Ok(())
}

async fn party_message(st: &Arc<State>, bytes: &[u8]) -> HResult {
    let Ok(p) = P3027::decode(bytes) else {
        tracing::warn!("party_message: decode failed");
        return Err(());
    };

    let mut h = P3827::default();
    h.party_id = p.party_id;
    h.account_id = p.account_id;
    h.repeat = p.repeat.iter().map(|r| P3827Repeat { c: r.c }).collect();
    st.map_broadcast(&enc(move |v| h.encode(v)));
    Ok(())
}

async fn party_check(st: &Arc<State>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3028::decode(bytes) else {
        return Err(());
    };
    // conflict check: if the (account_id, name) pair is in ANOTHER
    // party, remove it from that one.
    let pid = fixed.party_id.0;
    let account_id = fixed.account_id.0;
    let name = fixed.char_name.to_string_lossy();
    let mut to_clean = Vec::new();
    {
        let parties = st.parties.lock().unwrap();
        for (&p, pm) in parties.iter() {
            if p == pid {
                continue;
            }
            if pm
                .member
                .iter()
                .any(|m| m.account_id.0 == account_id && m.name.to_string_lossy() == name)
            {
                to_clean.push(p);
            }
        }
    }
    for p in to_clean {
        party_leave_do(st, p, account_id).await;
    }
    Ok(())
}

/// Load parties from the DB into memory at startup.
pub fn load_parties(st: &State) {
    let rows = st.db.with_conn(|conn| {
        let mut st_ = conn.prepare("SELECT id,name,exp_share,item_share FROM parties")?;
        let parties: Vec<(i64, String, i64, i64)> = st_
            .query_map([], |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?, r.get(3)?)))?
            .collect::<rusqlite::Result<Vec<_>>>()?;
        let mut st2 = conn
            .prepare("SELECT account_id,char_name,leader FROM party_members WHERE party_id=?1")?;
        let mut out = Vec::new();
        for (id, name, e, i) in parties {
            let members: Vec<(i64, String, i64)> = st2
                .query_map([id], |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)))?
                .collect::<rusqlite::Result<Vec<_>>>()?;
            out.push((id, name, e, i, members));
        }
        Ok(out)
    });
    let Ok(rows) = rows else { return };
    let mut parties = st.parties.lock().unwrap();
    for (id, name, exp, item, members) in rows {
        let mut p = PartyMost::default();
        p.name = FixedStr::<24>::try_from_str(&name).unwrap_or_default();
        p.exp = exp as i32;
        p.item = item as i32;
        for (i, (a, n, l)) in members.iter().enumerate().take(MAX_PARTY) {
            p.member[i] = PartyMember {
                account_id: AccountId(*a as u32),
                name: FixedStr::<24>::try_from_str(n).unwrap_or_default(),
                map: FixedStr::<16>::default(),
                leader: *l as i32,
                online: 0,
                lv: 0,
            };
        }
        parties.insert(id as u32, p);
    }
}
