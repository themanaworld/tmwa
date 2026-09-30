//! Minimal admin channel: one JSON object per line over a Unix
//! socket. The full CLI grows in phase 4; only `status` and
//! `drain` exist now.

use std::sync::Arc;
use std::time::{Duration, Instant};

use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::net::UnixListener;

use serde_json::{Value, json};

use super::state::State;

/// Start the admin listener; returns when the listener errors.
pub async fn run(st: Arc<State>) -> std::io::Result<()> {
    let path = &st.cfg.gate.admin_socket;
    let _ = std::fs::remove_file(path);
    let lis = UnixListener::bind(path)?;
    // readable/writable by owner + group only
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let _ = std::fs::set_permissions(path, std::fs::Permissions::from_mode(0o660));
    }
    tracing::info!("admin socket on {}", path.display());
    loop {
        match lis.accept().await {
            Ok((sock, _)) => {
                let st = st.clone();
                tokio::spawn(async move { handle(st, sock).await });
            }
            Err(e) => {
                tracing::warn!("admin accept: {e}");
            }
        }
    }
}

async fn handle(st: Arc<State>, sock: tokio::net::UnixStream) {
    let (rd, mut wr) = sock.into_split();
    let mut lines = BufReader::new(rd).lines();
    while let Ok(Some(line)) = lines.next_line().await {
        let Ok(req) = serde_json::from_str::<Value>(&line) else {
            let _ = wr
                .write_all(b"{\"ok\":false,\"error\":\"bad json\"}\n")
                .await;
            continue;
        };
        let cmd = req.get("cmd").and_then(|c| c.as_str()).unwrap_or("");
        let reply = match cmd {
            "status" => status(&st),
            "drain" => {
                let wait = req.get("wait").and_then(|w| w.as_bool()).unwrap_or(false);
                drain(&st, wait).await
            }
            _ => json!({"ok": false, "error": "unknown cmd"}),
        };
        let mut out = reply.to_string();
        out.push('\n');
        if wr.write_all(out.as_bytes()).await.is_err() {
            break;
        }
    }
}

fn status(st: &Arc<State>) -> Value {
    let maps: Vec<Value> = {
        let ms = st.map_servers.lock().unwrap();
        ms.iter()
            .flatten()
            .map(|h| {
                json!({
                    "id": h.id,
                    "addr": format!("{}.{}.{}.{}:{}",
                        h.ip.to_le_bytes()[0], h.ip.to_le_bytes()[1],
                        h.ip.to_le_bytes()[2], h.ip.to_le_bytes()[3], h.port),
                    "maps": h.maps.len(),
                    "draining": h.draining,
                })
            })
            .collect()
    };
    let sessions = st.player_sessions.lock().unwrap();
    let held = sessions.values().filter(|s| s.lock().unwrap().held).count();
    json!({
        "ok": true,
        "map_servers": maps,
        "players_online": st.count_users(),
        "players_held": held,
    })
}

/// Drain: close every relayed player's upstream (map_quit -> 0x2b01)
/// and mark the map links draining so no new players are sent.
/// With `wait`, returns after every drained char's 0x2b01 arrived or
/// 15 s.
async fn drain(st: &Arc<State>, wait: bool) -> Value {
    // mark map servers draining + collect target char ids
    let mut pending = std::collections::HashSet::new();
    {
        let mut ms = st.map_servers.lock().unwrap();
        for h in ms.iter_mut().flatten() {
            h.draining = true;
        }
    }
    let sessions: Vec<Arc<std::sync::Mutex<super::state::PlayerSession>>> = st
        .player_sessions
        .lock()
        .unwrap()
        .values()
        .cloned()
        .collect();
    for rec in &sessions {
        let mut r = rec.lock().unwrap();
        if !r.held {
            r.held = true;
            r.held_since = Some(Instant::now());
            pending.insert(r.char_id);
        }
        if let Some(sig) = &r.hold_signal {
            sig.notify_one();
        }
    }
    if !pending.is_empty() {
        st.drain_pending.lock().unwrap().extend(&pending);
    }
    if !wait {
        return json!({"ok": true, "draining": pending.len()});
    }
    let deadline = Instant::now() + Duration::from_secs(15);
    loop {
        let left: Vec<u32> = st.drain_pending.lock().unwrap().iter().copied().collect();
        if left.is_empty() {
            return json!({"ok": true, "unsaved": []});
        }
        if Instant::now() >= deadline {
            return json!({"ok": true, "unsaved": left});
        }
        tokio::time::sleep(Duration::from_millis(100)).await;
    }
}
