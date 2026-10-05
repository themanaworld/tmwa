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
pub mod http;
pub mod maplink;
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

    // serialized DB writer for the map link (state.rs)
    {
        let st = st.clone();
        tokio::spawn(async move {
            state::db_writer(st).await;
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
                write_online_files(&st).await;
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
                    client::run(st.clone(), sock, ip4, false).await;
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
    st.map_broadcast(&state::enc(|v| p.encode(v)));
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

/// Write online.txt / online.html (port of char.cpp
/// create_online_files).
async fn write_online_files(st: &State) {
    use std::fmt::Write as _;
    let cfg = &st.cfg;
    let now = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs();
    let when = format_time(now);
    let server = &cfg.char_.server_name;
    let refresh = cfg.char_.online_refresh_html;
    let min_gm = cfg.char_.online_gm_display_min_level;

    // collect online players (not HIDE-flagged)
    let mut names: Vec<(u32, String, u16)> = Vec::new();
    {
        let online = st.online.lock().unwrap();
        let chars = st.chars.lock().unwrap();
        for &cid in online.keys() {
            if let Some(c) = chars.get(&cid) {
                if c.data.option.0 & 0x0040 != 0 {
                    // Opt0::HIDE
                    continue;
                }
                let gm = is_gm(st, c.key.account_id.0);
                names.push((gm, c.key.name.to_string_lossy(), 0));
            }
        }
    }
    names.sort_by(|a, b| a.1.cmp(&b.1));

    let mut txt = String::new();
    let mut html = String::new();
    let _ = writeln!(html, "<HTML>\n  <HEAD>");
    let _ = writeln!(
        html,
        "    <META http-equiv=\"refresh\" content=\"{refresh}\">"
    );
    let _ = writeln!(html, "    <TITLE>Online Players on {server}</TITLE>");
    let _ = writeln!(html, "  </HEAD>\n  <BODY>");
    let _ = writeln!(html, "    <H3>Online Players on {server} ({when}):</H3>");
    let _ = writeln!(txt, "Online Players on {server} ({when}):\n");
    let _ = writeln!(
        html,
        "    <table border=\"1\" cellspacing=\"1\">\n      <tr>"
    );
    let _ = writeln!(html, "        <th>Name</th>\n      </tr>");
    let _ = writeln!(
        txt,
        "Name                          \n------------------------------"
    );
    let mut players = 0usize;
    for (gm, name, _) in &names {
        players += 1;
        let is_gm_shown = (*gm >= min_gm && gm % 10 == 0) || *gm >= 99;
        if *gm >= min_gm && *gm == 60 || *gm >= 99 {
            let _ = writeln!(txt, "{name:<24} (GM) ");
        } else {
            let _ = writeln!(txt, "{name:<24}      ");
        }
        let _ = write!(html, "      <tr>\n        <td>");
        if is_gm_shown {
            let _ = write!(html, "<b>");
        }
        for c in name.chars() {
            match c {
                '&' => html.push_str("&amp;"),
                '<' => html.push_str("&lt;"),
                '>' => html.push_str("&gt;"),
                c => html.push(c),
            }
        }
        if is_gm_shown {
            let _ = write!(html, "</b>");
            match *gm {
                40 | 80 => html.push_str(" (DEV)"),
                50 => html.push_str(" (EVTC)"),
                60 => html.push_str(" (GM)"),
                99 => html.push_str(" (ADM)"),
                _ => {}
            }
        }
        let _ = writeln!(html, "</td>\n      </tr>");
    }
    let _ = writeln!(html, "    </table>");
    txt.push('\n');
    if players == 0 {
        html.push_str("    <p>No user is online.</p>\n");
        txt.push_str("No user is online.\n");
    } else if players > 1 {
        let _ = writeln!(html, "    <p>{players} users are online.</p>");
        let _ = writeln!(txt, "{players} users are online.");
    }
    html.push_str("  </BODY>\n</HTML>\n");

    let _ = std::fs::write(&cfg.gate.online_txt, &txt);
    let _ = std::fs::write(&cfg.gate.online_html, &html);
}

/// "YYYY-MM-DD HH:MM:SS" for unix seconds (UTC).
pub fn format_time(secs: u64) -> String {
    // civil-from-days (Hinnant)
    let days = (secs / 86400) as i64;
    let rem = secs % 86400;
    let (h, mi, s) = (rem / 3600, rem % 3600 / 60, rem % 60);
    let z = days + 719468;
    let era = if z >= 0 { z } else { z - 146096 } / 146097;
    let doe = z - era * 146097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = if m <= 2 { y + 1 } else { y };
    format!("{y:04}-{m:02}-{d:02} {h:02}:{mi:02}:{s:02}")
}