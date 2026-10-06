// Packet structs are filled field-by-field after `Default::default()` —
// the tmwa handlers set each field explicitly; struct-literal style
// would be unmanageable for these sizes.
#![allow(clippy::collapsible_if)]
#![allow(clippy::field_reassign_with_default)]

//! `tmwa-gate serve`: the client-facing gateway.
//!
//! One client listener (`gate.listen`) dispatches on the first packet:
//! 0x7530 version, 0x0064 login, 0x0065 char screen, 0x0072 map
//! relay. tmwa-map connects to `map.listen` (the old char server's
//! inter port).

pub mod admin;
pub mod client;
pub mod dbq;
pub mod http;
pub mod maplink;
pub mod online_files;
pub mod state;
pub mod ws;

pub use state::State;

use std::sync::Arc;
use std::time::Duration;

use tokio::net::TcpListener;

use crate::config::Config;
use crate::db::Db;

#[derive(Debug, thiserror::Error)]
pub enum ServeError {
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    #[error("db: {0}")]
    Db(#[from] crate::db::DbError),
    #[error("bad listen addr {0:?}: {1}")]
    Addr(String, std::net::AddrParseError),
}

pub async fn run(cfg: Config) -> Result<(), ServeError> {
    // tmwa's lan_check is fatal here; the gate only warns.
    if !cfg.lan.lan_subnet.covers(cfg.lan.lan_map_ip) {
        tracing::warn!(
            "lan: lan_map_ip {} is outside lan_subnet; LAN clients \
             are sent an address they may not reach",
            cfg.lan.lan_map_ip
        );
    }
    let db = std::sync::Arc::new(Db::open(&cfg.gate.db)?);
    let st = Arc::new(State::new(cfg, db));
    {
        let st2 = st.clone();
        tokio::task::spawn_blocking(move || maplink::load_parties(&st2))
            .await
            .expect("load_parties panicked");
    }

    // serialized DB writer for the map link (dbq.rs)
    {
        let st = st.clone();
        tokio::spawn(async move {
            dbq::db_writer(st).await;
        });
    }

    // load GM levels once at startup, then watch the file
    reload_gm(&st);
    {
        let st = st.clone();
        tokio::spawn(async move {
            let secs = st.cfg.login.gm_account_filename_check_timer.max(1);
            let mut iv = tokio::time::interval(Duration::from_secs(secs));
            loop {
                iv.tick().await;
                reload_gm(&st);
            }
        });
    }

    // admin unix socket
    {
        let st = st.clone();
        tokio::spawn(async move {
            if let Err(e) = admin::run(st).await {
                tracing::warn!("admin socket: {e}");
            }
        });
    }

    // prune expired rate limits, bad actors and reset codes
    {
        let st = st.clone();
        tokio::spawn(async move {
            let mut iv = tokio::time::interval(Duration::from_secs(600));
            loop {
                iv.tick().await;
                http::prune(&st).await;
            }
        });
    }

    // online.txt / online.html writer task (tmwa regenerates on each
    // 0x2aff, throttled to every 8 s)
    {
        let st = st.clone();
        tokio::spawn(async move {
            let mut last = std::time::Instant::now() - Duration::from_secs(60);
            loop {
                st.online_notify.notified().await;
                if last.elapsed() < Duration::from_secs(8) {
                    continue;
                }
                last = std::time::Instant::now();
                online_files::write_online_files(&st).await;
            }
        });
    }

    // http + websocket listener
    {
        let st = st.clone();
        tokio::spawn(async move {
            if let Err(e) = http::run(st).await {
                tracing::warn!("http: {e}");
            }
        });
    }

    let client_addr = st
        .cfg
        .listen_addr()
        .map_err(|e| ServeError::Addr(st.cfg.gate.listen.clone(), e))?;
    let map_addr = st
        .cfg
        .map_listen_addr()
        .map_err(|e| ServeError::Addr(st.cfg.map.listen.clone(), e))?;

    let client_listener = TcpListener::bind(client_addr).await?;
    let map_listener = TcpListener::bind(map_addr).await?;
    tracing::info!("gate: clients on {client_addr}, map link on {map_addr}");

    let st2 = st.clone();
    tokio::spawn(async move {
        loop {
            match map_listener.accept().await {
                Ok((sock, peer)) => {
                    let st = st2.clone();
                    tokio::spawn(async move {
                        let _ = sock.set_nodelay(true);
                        if let std::net::IpAddr::V4(v4) = peer.ip() {
                            maplink::run(st, sock, v4).await;
                        } else {
                            tracing::warn!("maplink: non-v4 peer {peer} dropped");
                        }
                    });
                }
                Err(e) => tracing::warn!("map accept: {e}"),
            }
        }
    });

    loop {
        match client_listener.accept().await {
            Ok((sock, peer)) => {
                let st = st.clone();
                tokio::spawn(async move {
                    let _ = sock.set_nodelay(true);
                    if st.conn_count() >= st.cfg.http.max_connections {
                        return; // over the limit: just close
                    }
                    st.conn_inc();
                    let ip4 = crate::net::map_ip(peer.ip());
                    if ip4 == std::net::Ipv4Addr::UNSPECIFIED
                        || !matches!(peer.ip(), std::net::IpAddr::V4(_))
                    {
                        tracing::info!("client {peer} -> pseudo-ipv4 {ip4}");
                    }
                    // TCP clients connect to the map server named in
                    // 0x0071 directly; only the WS transport relays.
                    client::run_tcp(st.clone(), sock, ip4).await;
                    st.conn_dec();
                });
            }
            Err(e) => tracing::warn!("client accept: {e}"),
        }
    }
}

/// Push the current `st.gm` map to every connected map server
/// (0x2b15).
pub fn send_gm_list(st: &State) {
    let gm = st.gm.lock().unwrap();
    let repeat: Vec<crate::proto::P2B15Repeat> = gm
        .iter()
        .map(|(&id, &lv)| crate::proto::P2B15Repeat {
            account_id: crate::proto::AccountId(id),
            gm_level: crate::proto::types::GmLevel(lv),
        })
        .collect();
    drop(gm);
    let p = crate::proto::P2B15 { repeat };
    st.map_broadcast(&p.encoded());
}

/// Parse gm_account.txt ("id level" per line, `//` comments), swap
/// it in atomically and push it to the map servers. Returns the GM
/// count. Unlike `reload_gm` this always re-reads the file; the
/// admin `reloadgm` command uses it.
pub fn load_gm_file(st: &State) -> usize {
    let path = &st.cfg.gate.gm_account_file;
    let mtime = std::fs::metadata(path).and_then(|m| m.modified()).ok();
    let mut map = std::collections::HashMap::new();
    match std::fs::read_to_string(path) {
        Ok(text) => {
            for line in text.lines() {
                let line = line.trim();
                if line.is_empty() || line.starts_with("//") {
                    continue;
                }
                let mut it = line.split_whitespace();
                let (Ok(id), Ok(lv)) = (
                    it.next().unwrap_or("").parse::<u32>(),
                    it.next().unwrap_or("").parse::<u32>(),
                ) else {
                    tracing::warn!("gate: bad gm_account line {line:?}");
                    continue;
                };
                map.insert(id, lv);
            }
            tracing::info!("gate: {} GM account(s) loaded", map.len());
        }
        Err(e) => {
            tracing::warn!("gate: cannot read {}: {e}", path.display());
        }
    }
    let n = map.len();
    *st.gm.lock().unwrap() = map;
    *st.gm_mtime.lock().unwrap() = mtime;
    send_gm_list(st);
    n
}

/// Reload gm_account.txt when its mtime changed (startup + periodic
/// check); the admin command calls `load_gm_file` directly instead.
pub fn reload_gm(st: &State) {
    let path = &st.cfg.gate.gm_account_file;
    let mtime = std::fs::metadata(path).and_then(|m| m.modified()).ok();
    {
        let cur = st.gm_mtime.lock().unwrap();
        if *cur == mtime && cur.is_some() {
            return;
        }
    }
    load_gm_file(st);
}

pub fn is_gm(st: &State, account_id: u32) -> u32 {
    st.gm.lock().unwrap().get(&account_id).copied().unwrap_or(0)
}