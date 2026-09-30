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
use super::state::{State, enc, send_bytes};
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
    let (tx, mut rx) = mpsc::channel::<Vec<u8>>(512);
    // writer task
    let wh = tokio::spawn(async move {
        let mut w = wr;
        use tokio::io::AsyncWriteExt;
        while let Some(buf) = rx.recv().await {
            if w.write_all(&buf).await.is_err() {
                break;
            }
            let _ = w.flush().await;
        }
    });

    let mut fr = PacketFramer::new(rd);
    let mut map_id: Option<usize> = None;

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
                    send_bytes(&tx, enc(move |v| p.encode(v)));
                    tracing::warn!("maplink: bad map auth from {ip}");
                    break 'auth false;
                }
                p.code = 0;
                send_bytes(&tx, enc(move |v| p.encode(v)));
                let id = st.map_register(tx.clone(), u32::from_le_bytes(fixed.ip.0), fixed.port);
                tracing::warn!(
                    "maplink: map server {id} registered from {ip} \
                     (client port {}:{})",
                    Ipv4Addr::from(fixed.ip.0),
                    fixed.port
                );
                map_id = Some(id);
                // 0x2b15 GM list on connect
                let gm = st.gm.lock().unwrap();
                let repeat: Vec<P2B15Repeat> = gm
                    .iter()
                    .map(|(&aid, &lv)| P2B15Repeat {
                        account_id: AccountId(aid),
                        gm_level: GmLevel(lv),
                    })
                    .collect();
                drop(gm);
                let p15 = P2B15 { repeat };
                send_bytes(&tx, enc(move |v| p15.encode(v)));
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

    // ---- main loop ----
    loop {
        let pkt = match fr.next().await {
            Ok(Some(p)) => p,
            _ => break,
        };
        let r = handle(&st, &tx, map_id, pkt.id, &pkt.bytes).await;
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
            send_bytes(tx, enc(move |v| p.encode(v)));
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
                send_bytes(tx, enc(|v| head.encode(v)));
            }
            Ok(())
        }
        0x2afc => {
            let Ok(fixed) = P2AFC::decode(bytes) else {
                return Err(());
            };
            // tmwa compares afi.ip to the ip the map reports; through
            // the relay that's the gate's upstream source address,
            // recorded at relay time. Fall back to the client ip.
            let entry = st.take_map_auth(
                fixed.account_id.0,
                fixed.char_id.0,
                fixed.login_id1,
                fixed.login_id2,
                u32::from_le_bytes(fixed.ip.0),
            );
            let Some(e) = entry else {
                tracing::warn!(
                    "maplink: REJECTED 0x2afc account {} char {}",
                    fixed.account_id.0,
                    fixed.char_id.0
                );
                let mut p = P2AFE::default();
                p.account_id = fixed.account_id;
                send_bytes(tx, enc(move |v| p.encode(v)));
                return Ok(());
            };
            // load CharKey + CharData (full, incl. account_reg* + vars)
            let cid = e.char_id;
            let res = {
                let st2 = st.clone();
                tokio::task::spawn_blocking(move || st2.db.load_character(cid as i64)).await
            };
            let Ok(Ok((key, cd))) = res else {
                let mut p = P2AFE::default();
                p.account_id = fixed.account_id;
                send_bytes(tx, enc(move |v| p.encode(v)));
                return Ok(());
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
            let mut p = P2AFD::default();
            p.account_id = fixed.account_id;
            p.login_id2 = e.login_id2;
            p.client_protocol_version = ClientVersion(e.client_version);
            p.char_key = key;
            p.char_data = cd;
            send_bytes(tx, enc(move |v| p.encode(v)));
            tracing::info!(map_id, char_id = cid, "authenticated char for map");
            Ok(())
        }
        0x2aff => {
            let Ok(p) = P2AFF::decode(bytes) else {
                return Err(());
            };
            let chars: Vec<u32> = p.repeat.iter().map(|r| r.char_id.0).collect();
            on_user_list(st, map_id, p.users, chars);
            Ok(())
        }
        0x2b01 => {
            // a drain waiter may be listening
            {
                let Ok(p) = P2B01::decode(bytes) else {
                    return Err(());
                };
                st.drain_pending.lock().unwrap().remove(&p.char_id.0);
            }
            let Ok(p) = P2B01::decode(bytes) else {
                return Err(());
            };
            // update cache + persist (0x2b01 does not touch
            // account vars; see save_character)
            {
                let mut chars = st.chars.lock().unwrap();
                if let Some(c) = chars.get_mut(&p.char_id.0) {
                    c.key = p.char_key;
                    c.data = p.char_data;
                }
            }
            let st2 = st.clone();
            let _ = tokio::task::spawn_blocking(move || {
                st2.db.save_character(&p.char_key, &p.char_data)
            })
            .await;
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
            let mut p = P2B03::default();
            p.account_id = fixed.account_id;
            p.unknown = 0;
            send_bytes(tx, enc(move |v| p.encode(v)));
            Ok(())
        }
        0x2b05 => {
            let Ok(fixed) = P2B05::decode(bytes) else {
                return Err(());
            };
            // map-to-map move: create a map-stage entry so the target
            // map server's 0x2afc can match (multi-map splice of the
            // client connection is a later phase).
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
            send_bytes(tx, enc(move |v| p.encode(v)));
            Ok(())
        }
        0x2b0c => {
            let Ok(fixed) = P2B0C::decode(bytes) else {
                return Err(());
            };
            let old = fixed.old_email.to_string_lossy();
            let new = fixed.new_email.to_string_lossy();
            let aid = fixed.account_id.0 as i64;
            let old2 = old.clone();
            let cur = st
                .db
                .blocking(move |db| db.account_email(aid))
                .await
                .ok()
                .flatten()
                .unwrap_or_default();
            if cur == old2 {
                let _ = st
                    .db
                    .blocking(move |db| db.set_email(aid, Some(&new)))
                    .await;
            }
            Ok(())
        }
        0x2b0e => {
            let Ok(fixed) = P2B0E::decode(bytes) else {
                return Err(());
            };
            handle_named_op(st, tx, fixed).await
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
            let st2 = st.clone();
            let _ = tokio::task::spawn_blocking(move || {
                // tmwa replaces the whole scope with the incoming list
                let _ = st2.db.with_conn(|conn| {
                    conn.execute(
                        "DELETE FROM account_vars WHERE account_id=?1 AND scope=2",
                        [aid as i64],
                    )
                });
                for (n, v) in regs {
                    let _ = st2.db.set_account_var(aid as i64, 2, &n, v);
                }
            })
            .await;
            Ok(())
        }
        0x2b16 => {
            let Ok(fixed) = P2B16::decode(bytes) else {
                return Err(());
            };
            let cid = fixed.char_id.0;
            let partner = {
                let st2 = st.clone();
                tokio::task::spawn_blocking(move || st2.db.divorce(cid as i64))
                    .await
                    .ok()
                    .and_then(|r| r.ok())
                    .flatten()
            };
            {
                let mut chars = st.chars.lock().unwrap();
                if let Some(c) = chars.get_mut(&cid) {
                    c.data.partner_id = CharId(0);
                }
                if let Some(pid) = partner {
                    if let Some(c) = chars.get_mut(&(pid as u32)) {
                        c.data.partner_id = CharId(0);
                    }
                }
            }
            let mut p = P2B12::default();
            p.char_id = fixed.char_id;
            p.partner_id = CharId(partner.unwrap_or(0) as u32);
            st.map_broadcast(&enc(move |v| p.encode(v)));
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
            let send71 = {
                let mut pend = st.pending_sel.lock().unwrap();
                if let Some(ps) = pend.get_mut(&key) {
                    if ps.login_id1 == fixed.login_id1 && ps.login_id2 == fixed.login_id2 {
                        ps.seen += 1;
                        if ps.seen >= ps.needed {
                            Some(pend.remove(&key).unwrap())
                        } else {
                            None
                        }
                    } else {
                        None
                    }
                } else {
                    None
                }
            };
            if let Some(ps) = send71 {
                let mut p = P0071::default();
                p.char_id = CharId(ps.char_id);
                p.map_name = FixedStr::<16>::try_from_str(&ps.map_name).unwrap_or_default();
                p.ip = {
                    let ip: Ipv4Addr = st.cfg.gate.public_ip.parse().unwrap_or(Ipv4Addr::LOCALHOST);
                    Ip4Address(u32::from_le_bytes(ip.octets()).to_le_bytes())
                };
                p.port = st.cfg.gate.public_port;
                send_bytes(&ps.client_tx, enc(move |v| p.encode(v)));
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
            let from = p.from_char_name.to_string_lossy();
            let mut to = p.to_char_name.to_string_lossy();
            let target = st.char_by_name(&to).await;
            let target_map = target.and_then(|cid| st.online.lock().unwrap().get(&cid).copied());
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
                    p2.sender_char_name = FixedStr::<24>::try_from_str(&from).unwrap_or_default();
                    p2.flag = 1;
                    send_bytes(tx, enc(move |v| p2.encode(v)));
                }
            }
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
            let st2 = st.clone();
            let _ = tokio::task::spawn_blocking(move || {
                let _ = st2.db.with_conn(|conn| {
                    conn.execute(
                        "DELETE FROM account_vars WHERE account_id=?1 AND scope=1",
                        [aid as i64],
                    )
                });
                for (n, v) in regs2 {
                    let _ = st2.db.set_account_var(aid as i64, 1, &n, v);
                }
            })
            .await;
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
            // accreg request -> 0x3804 to requester
            let Ok(fixed) = P3005::decode(bytes) else {
                return Err(());
            };
            let aid = fixed.account_id.0 as i64;
            let vars = st
                .db
                .blocking(move |db| db.get_account_vars(aid, 1))
                .await
                .unwrap_or_default();
            let mut p = P3804::default();
            p.account_id = fixed.account_id;
            p.repeat = vars
                .iter()
                .map(|(n, v)| P3804Repeat {
                    name: FixedStr::<32>::try_from_str(n).unwrap_or_default(),
                    value: *v as u32,
                })
                .collect();
            send_bytes(tx, enc(move |v| p.encode(v)));
            Ok(())
        }
        0x3010 => {
            // storage load -> 0x3810
            let Ok(fixed) = P3010::decode(bytes) else {
                return Err(());
            };
            let aid = fixed.account_id.0 as i64;
            let st2 = st.clone();
            let items = tokio::task::spawn_blocking(move || st2.db.load_storage(aid))
                .await
                .ok()
                .and_then(|r| r.ok())
                .unwrap_or_default();
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
            send_bytes(tx, enc(move |v| p.encode(v)));
            Ok(())
        }
        0x3011 => {
            // storage save -> DB + 0x3811 ack
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
            let st2 = st.clone();
            let _ = tokio::task::spawn_blocking(move || st2.db.save_storage(aid, &items)).await;
            let mut ack = P3811::default();
            ack.account_id = p.account_id;
            ack.unknown = 0;
            send_bytes(tx, enc(move |v| ack.encode(v)));
            Ok(())
        }

        // ---- parties ----
        0x3020 => party_create(st, tx, bytes).await,
        0x3021 => party_info(st, tx, bytes).await,
        0x3022 => party_add(st, tx, bytes).await,
        0x3023 => party_option(st, tx, bytes).await,
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

/// 0x2b0e named-char ops: block(1)/ban(2)/unblock(3)/unban(4) against
/// a character found by name; operation 5 (changesex) fails since
/// accounts have no sex anymore.
async fn handle_named_op(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, fixed: P2B0E) -> HResult {
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
        send_bytes(tx, enc(move |v| reply.encode(v)));
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
    let db = st.db.clone();
    drop(tokio::task::spawn_blocking(move || {
        db.with_conn(|conn| conn.execute("DELETE FROM parties WHERE id=?1", [party_id as i64]))
    }));
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
    let db = st.db.clone();
    drop(tokio::task::spawn_blocking(move || {
        db.with_conn(|conn| {
            let tx = conn.transaction()?;
            tx.execute(
                "INSERT INTO parties(id,name,exp_share,item_share) VALUES(?1,?2,?3,?4)
             ON CONFLICT(id) DO UPDATE SET name=excluded.name,
             exp_share=excluded.exp_share,item_share=excluded.item_share",
                rusqlite::params![party_id as i64, name, p.exp as i64, p.item as i64],
            )?;
            tx.execute(
                "DELETE FROM party_members WHERE party_id=?1",
                [party_id as i64],
            )?;
            for (a, n, l) in members {
                tx.execute(
                    "INSERT INTO party_members(party_id,account_id,char_name,leader)
                 VALUES(?1,?2,?3,?4)",
                    rusqlite::params![party_id as i64, a, n, l],
                )?;
            }
            tx.commit()
        })
    }));
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

fn party_info_to(st: &State, tx: Option<&mpsc::Sender<Vec<u8>>>, party_id: u32) {
    if let Some(p) = party_get(st, party_id) {
        let mut h = P3821::default();
        h.party_id = PartyId(party_id);
        h.option = Some(P3821Option { party_most: p });
        match tx {
            Some(t) => send_bytes(t, enc(move |v| h.encode(v))),
            None => {
                st.map_broadcast(&enc(move |v| h.encode(v)));
            }
        }
    } else if let Some(t) = tx {
        let mut h = P3821::default();
        h.party_id = PartyId(party_id);
        h.option = None;
        send_bytes(t, enc(move |v| h.encode(v)));
    }
}

async fn party_create(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, bytes: &[u8]) -> HResult {
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
        send_bytes(tx, enc(move |v| p.encode(v)));
    };
    if name.is_empty() || !name.bytes().all(|b| (32..=126).contains(&b)) {
        reply(1, 0, "error".into());
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
        reply(1, 0, "error".into());
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
    let _ = st
        .db
        .blocking(move |db| db.set_meta("next_party_id", pid as i64))
        .await;
    reply(0, pid, name);
    party_info_to(st, Some(tx), pid);
    Ok(())
}

async fn party_info(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3021::decode(bytes) else {
        return Err(());
    };
    party_info_to(st, Some(tx), fixed.party_id.0);
    Ok(())
}

async fn party_add(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3022::decode(bytes) else {
        return Err(());
    };
    let pid = fixed.party_id.0;
    let reply = |flag: u8| {
        let mut p = P3822::default();
        p.party_id = fixed.party_id;
        p.account_id = fixed.account_id;
        p.flag = flag;
        send_bytes(tx, enc(move |v| p.encode(v)));
    };
    let mut p = match party_get(st, pid) {
        Some(p) => p,
        None => {
            reply(1);
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
            reply(0);
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
            party_info_to(st, None, pid);
            return Ok(());
        }
    }
    reply(1);
    Ok(())
}

async fn party_option(st: &Arc<State>, tx: &mpsc::Sender<Vec<u8>>, bytes: &[u8]) -> HResult {
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
        send_bytes(tx, enc(move |v| o.encode(v)));
    }
    party_put(st, pid, p);
    Ok(())
}

async fn party_leave(st: &Arc<State>, bytes: &[u8]) -> HResult {
    let Ok(fixed) = P3024::decode(bytes) else {
        return Err(());
    };
    party_leave_do(st, fixed.party_id.0, fixed.account_id.0);
    Ok(())
}

fn party_leave_do(st: &Arc<State>, pid: u32, account_id: u32) {
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
            party_info_to(st, None, pid);
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
        party_leave_do(st, p, account_id);
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
