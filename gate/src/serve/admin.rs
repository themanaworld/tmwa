//! Admin channel: one JSON object per line over a Unix socket.
//! `{"cmd": "<name>", "args": [...], "password": "..."}` ->
//! `{"ok": bool, "text": "...", ...}` one per line.
//!
//! The command set mirrors tmwa's ladmin where tmwa has an
//! equivalent; the text output matches tmwa-admin's PRINTF format
//! closely enough for mirror-lake's pcmd parser.

use std::sync::Arc;
use std::time::{Duration, Instant};

use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::net::UnixListener;

use serde_json::{Value, json};

use crate::proto::types::{FixedStr, Ip4Address};
use crate::proto::{
    AccountId, P2B04, P2B04Repeat, P2B11Repeat, P382A, P3800Repeat, P3804Repeat, enc,
};

use super::dbq::send_must;
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

pub async fn handle(st: Arc<State>, sock: tokio::net::UnixStream) {
    let (rd, mut wr) = sock.into_split();
    let mut lines = BufReader::new(rd).lines();
    while let Ok(Some(line)) = lines.next_line().await {
        let reply = match serde_json::from_str::<Value>(&line) {
            Ok(req) => {
                let cmd = req
                    .get("cmd")
                    .and_then(|c| c.as_str())
                    .unwrap_or("")
                    .to_string();
                let args: Vec<String> = req
                    .get("args")
                    .and_then(|a| a.as_array())
                    .map(|a| {
                        a.iter()
                            .filter_map(|v| v.as_str().map(|s| s.to_string()))
                            .collect()
                    })
                    .unwrap_or_default();
                let password = req
                    .get("password")
                    .and_then(|p| p.as_str())
                    .map(|p| p.to_string());
                // Each command runs in its own task so a panic
                // kills the request, not the connection.
                let stc = st.clone();
                match tokio::spawn(async move { dispatch(&stc, &cmd, args, password).await }).await
                {
                    Ok(v) => v,
                    Err(e) => {
                        tracing::error!("admin request panicked: {e}");
                        json!({"ok": false, "error": "internal error (see gate log)"})
                    }
                }
            }
            Err(_) => json!({"ok": false, "error": "bad json"}),
        };
        let mut out = reply.to_string();
        out.push('\n');
        if wr.write_all(out.as_bytes()).await.is_err() {
            break;
        }
    }
}

fn ok_text(text: impl Into<String>) -> Value {
    json!({"ok": true, "text": text.into()})
}
fn err_text(text: impl Into<String>) -> Value {
    json!({"ok": false, "error": text.into()})
}

/// Account id by exact name, inside a locked conn section.
fn acct_id_by_name(conn: &rusqlite::Connection, name: &str) -> Option<i64> {
    crate::db::account_id_by_name_conn(conn, name)
        .ok()
        .flatten()
}
fn acct_name_by_id(conn: &rusqlite::Connection, id: i64) -> Option<String> {
    crate::db::account_name_by_id_conn(conn, id).ok().flatten()
}

fn acct_exists(conn: &rusqlite::Connection, id: i64) -> bool {
    crate::db::account_exists_conn(conn, id).unwrap_or(false)
}

/// Flag each session kicked and wake any hold signal; returns the
/// kicked char ids (so callers can drop `online` entries, which are
/// keyed by char_id).
fn kick_sessions(recs: &[Arc<std::sync::Mutex<super::state::PlayerSession>>]) -> Vec<u32> {
    let mut gone = Vec::with_capacity(recs.len());
    for rec in recs {
        let mut r = rec.lock().unwrap();
        r.kicked = true;
        if let Some(sig) = &r.hold_signal {
            sig.notify_one();
        }
        gone.push(r.char_id);
    }
    gone
}

/// Drop char ids from `online` and wake its waiters (drain
/// counting, the online-file writer).
fn forget_online(st: &Arc<State>, cids: Vec<u32>) {
    {
        let mut online = st.online.lock().unwrap();
        for cid in cids {
            online.remove(&cid);
        }
    }
    st.online_notify.notify_waiters();
}

/// Kick (disconnect) a player: with a char id, match exactly that
/// character's session; without one, match the account. A 0 must
/// never match anything.
fn kick_player(st: &Arc<State>, account_id: u32, char_id: u32) {
    let rec = {
        st.player_sessions
            .lock()
            .unwrap()
            .values()
            .find(|r| {
                let r = r.lock().unwrap();
                if char_id != 0 {
                    r.char_id == char_id
                } else if account_id != 0 {
                    r.account_id == account_id
                } else {
                    false
                }
            })
            .cloned()
    };
    if let Some(rec) = rec {
        // `online` is keyed by char_id: drop the kicked char's
        // entry so maps stop seeing the player
        forget_online(st, kick_sessions(&[rec]));
    }
}

fn state_label(state: i64) -> &'static str {
    match state {
        0 => "Account OK",
        1 => "Unregistered ID",
        2 => "Incorrect Password",
        3 => "This ID is expired",
        4 => "Rejected from Server",
        5 => "You have been blocked by the GM Team",
        6 => "Your Game's EXE file is not the latest version",
        7 => "You are Prohibited to log in until...",
        8 => "Server is jammed due to over populated",
        100 => "This ID has been totally erased",
        _ => "No MSG",
    }
}

/// ladmin variable scope: `#name` is scope 1, `##name` (and
/// anything without a single leading `#`) is scope 2.
fn accreg_scope(name: &str) -> i64 {
    if name.starts_with('#') && !name.starts_with("##") {
        1
    } else {
        2
    }
}

/// The ladmin "no such account" line, shared by the accreg
/// commands that take a bare account id.
fn err_no_acct(id: i64) -> String {
    format!("Unable to find the account [id: {id}]. Account doesn't exist.\n")
}

/// `yyyy/mm/dd hh:mm:ss` for unix seconds; empty when out of
/// range (matching the old Option::map/unwrap_or_default shape).
fn fmt_ymd_slash(secs: i64) -> String {
    chrono::DateTime::from_timestamp(secs, 0)
        .map(|t| t.format("%Y/%m/%d %H:%M:%S").to_string())
        .unwrap_or_default()
}

pub async fn dispatch(
    st: &Arc<State>,
    cmd: &str,
    args: Vec<String>,
    password_stdin: Option<String>,
) -> Value {
    match cmd {
        #[cfg(debug_assertions)]
        "__panic" => panic!("test panic"),
        "status" => status(st),
        "online" => online(st),
        "kick" => kick_cmd(st, &args).await,
        "drain" => {
            let wait = args.iter().any(|a| a == "--wait");
            let which = args
                .iter()
                .find(|a| !a.starts_with('-'))
                .and_then(|a| a.parse::<usize>().ok());
            drain(st, wait, which).await
        }
        "find" => find(st, &args).await,
        "chars" => chars_cmd(st, &args).await,
        "getcount" => getcount(st),
        "who" | "info" => who(st, &args).await,
        "id" => id_cmd(st, &args).await,
        "name" => name_cmd(st, &args).await,
        "list" | "ls" | "listban" | "listgm" | "listok" | "search" => {
            list_cmd(st, cmd, &args).await
        }
        "memo" => memo(st, &args).await,
        "email" => email(st, &args).await,
        "block" => state_cmd(st, &args, 5, "block").await,
        "unblock" => state_cmd(st, &args, 0, "unblock").await,
        "state" => state_set(st, &args).await,
        "ban" => ban_cmd(st, &args).await,
        "banset" => banset(st, &args).await,
        "banadd" => banadd(st, &args).await,
        "unban" | "unbanish" => unban(st, &args).await,
        "delete" => delete(st, &args).await,
        "password" => password_cmd(st, &args, password_stdin).await,
        "check" => check_cmd(st, &args, password_stdin).await,
        "create" => create(st, &args, password_stdin).await,
        "add" => add_cmd(st, &args, password_stdin).await,
        "gm" => gm_cmd(st, &args).await,
        "reloadgm" | "reloadGM" => {
            let n = super::load_gm_file(st);
            ok_text(format!("  {n} GM account(s) loaded.\n"))
        }
        "kami" | "kamib" => kami(st, cmd, &args),
        "getallaccreg2" | "getall" | "ga" => getall(st, &args, 2).await,
        "getaccreg2" | "get" | "g" => getaccreg(st, &args).await,
        "setaccreg2" | "set" | "s" => setaccreg(st, &args).await,
        "delaccreg2" | "del" | "d" => delaccreg(st, &args).await,
        "version" => ok_text(format!("tmwa-gate {}\n", env!("CARGO_PKG_VERSION"))),
        "help" | "?" => help(),
        "" => err_text("no command"),
        _ => err_text(format!("Unknown command [{cmd}].")),
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
                    "users": h.users,
                    "draining": h.draining,
                    // writer-queue backlog, bulk and critical
                    "out_queue": h.backlog(),
                    "out_queue_prio": h.backlog_prio(),
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
        "db_queue": st.db_jobs_depth.load(std::sync::atomic::Ordering::Relaxed),
        "db_dropped": st.db_dropped.load(std::sync::atomic::Ordering::Relaxed),
        // chars/accounts owed a replay of a dropped write
        "db_dirty": st.save_dirty.lock().unwrap().len()
            + st.storage_dirty.lock().unwrap().len(),
    })
}

fn online(st: &Arc<State>) -> Value {
    let mut out = String::new();
    let sessions = st.player_sessions.lock().unwrap();
    let chars = st.chars.lock().unwrap();
    let mut rows: Vec<(u32, u32, bool, usize)> = sessions
        .values()
        .map(|s| {
            let r = s.lock().unwrap();
            (r.account_id, r.char_id, r.held, r.map_id)
        })
        .collect();
    rows.sort();
    for (acct, cid, held, mid) in &rows {
        // `chars` is keyed by char_id and carries the name already
        let cname = chars
            .get(cid)
            .map(|r| r.key.name.to_string_lossy())
            .unwrap_or_else(|| "?".to_string());
        out += &format!(
            "{:10} {:10} {:<24} map {:3} {}\n",
            acct,
            cid,
            cname,
            mid,
            if *held { "held" } else { "" }
        );
    }
    out += &format!("  {} player(s) online.\n", rows.len());
    ok_text(out)
}

async fn kick_cmd(st: &Arc<State>, args: &[String]) -> Value {
    let Some(name) = args.first() else {
        return err_text("usage: kick <char name>");
    };
    match st.char_by_name(name).await {
        Some(cid) => {
            kick_player(st, 0, cid);
            ok_text(format!("Character [{name}] kicked.\n"))
        }
        None => err_text(format!("Character [{name}] not found.")),
    }
}

/// Drain a map server (blue-green evacuation).
///
/// Marks the server draining (no new logins/warps are sent to it),
/// then re-broadcasts 0x2b04 for every map name it serves: each
/// name that another non-draining server also serves is announced
/// with *that* server's address (map_setipport is last-write-wins,
/// so a single announcement pointing at the survivor is enough).
/// Names with no survivor keep pointing at the drained server, which
/// also clears any shadow entry it had; those players stay and are
/// saved by the shutdown logout path when the server finally stops.
///
/// Each drained server then gets a 0x382a evacuate request, which
/// walks its players through the ordinary cross-server warp
/// (0x2b05/0x2b06 -> client 0x0092) onto the surviving instance.
///
/// With `wait`, returns once the drained servers report zero users
/// (or their links drop), 60 s at most. The reply then counts how
/// many of the players that were on the drained servers actually
/// landed elsewhere (`arrived` is confirmed by the destination
/// link's 0x2afc), how many are still there, and how many left
/// without landing (logged out or failed).
async fn drain(st: &Arc<State>, wait: bool, which: Option<usize>) -> Value {
    // One snapshot drives the whole drain: the map set can't
    // meaningfully change mid-reply anyway (new registrations go
    // through `drain`/`undebug` paths of their own).
    let infos = st.map_infos();
    let targets: Vec<usize> = match which {
        Some(id) => {
            if !infos.iter().any(|(i, ..)| *i == id) {
                return err_text(format!("no map server {id} connected"));
            }
            vec![id]
        }
        None => infos.iter().map(|(id, ..)| *id).collect(),
    };
    for id in &targets {
        st.drain_track_begin(*id, st.online_on(*id));
        st.map_set_draining(*id, true);
    }
    let mut stragglers: Vec<String> = Vec::new();
    for id in &targets {
        let Some((_, self_ip, self_port, maps, _, _)) = infos.iter().find(|(i, ..)| i == id)
        else {
            continue;
        };
        let self_addr = (*self_ip, *self_port);
        let mut kept = 0;
        for name in maps {
            // a non-draining server that also serves this name wins
            // the name outright (the announcement below overwrites
            // every server's remote/shadow entry)
            let winner = infos.iter().find_map(|(i, ip, port, hmaps, draining, _)| {
                (!*draining && !targets.contains(i) && hmaps.iter().any(|m| m == name))
                    .then_some((*ip, *port))
            });
            let (ip, port) = match winner {
                Some(w) => w,
                None => {
                    stragglers.push(name.clone());
                    kept += 1;
                    self_addr
                }
            };
            let mut head = P2B04::default();
            head.ip = Ip4Address::from(ip);
            head.port = port;
            head.repeat = vec![P2B04Repeat {
                map_name: FixedStr::<16>::from_str_truncate(name),
            }];
            let bytes = enc(move |v| head.encode(v));
            st.map_broadcast_except(*id, &bytes);
            // the drained server's own copies go on its priority
            // queue so they strictly precede the 0x382a below: it
            // builds the shadow table the evacuation resolves
            // through
            if let Some(tx) = st.map_prio_tx(*id) {
                send_must(st, *id, &tx, bytes).await;
            }
        }
        if let Some(tx) = st.map_prio_tx(*id) {
            send_must(st, *id, &tx, enc(|v| P382A::default().encode(v))).await;
        }
        tracing::info!(
            "map {id}: draining ({} maps handed over, {kept} with no survivor)",
            maps.len() - kept,
        );
    }
    // Evacuee accounting: `expected` is the set that was online on
    // the drained server when the drain began; `arrived` is how many
    // of those have since authenticated on a different server;
    // `still` are still on it; the rest left (logged out or failed
    // to land). A high `departed` count with emptied==true is the
    // "lost everyone" case the raw user count can't show.
    let collect = |st: &Arc<State>| {
        let (mut expected, mut arrived, mut still) = (0usize, 0usize, 0usize);
        for id in &targets {
            if let Some(t) = st.drain_track_end(*id) {
                expected += t.expected.len();
                arrived += t.arrived.len();
                let on = st.online_on(*id);
                still += t.expected.intersection(&on).count();
            }
        }
        json!({
            "evacuees": expected,
            "arrived": arrived,
            "still_on_source": still,
            "departed": expected.saturating_sub(arrived + still),
        })
    };
    let reply = |emptied: Option<bool>, st: &Arc<State>| {
        let mut v = json!({
            "ok": true,
            "draining": targets,
            "stragglers": stragglers,
            "emptied": emptied,
        });
        if emptied.is_some() {
            v.as_object_mut()
                .unwrap()
                .extend(collect(st).as_object().unwrap().clone());
        }
        v
    };
    if !wait {
        // no --wait: drop the tracking entries now, they would
        // otherwise stay until the next drain of the same slot
        for id in &targets {
            st.drain_track_end(*id);
        }
        return reply(None, st);
    }
    let deadline = Instant::now() + Duration::from_secs(60);
    // phase 1: the drained servers report zero users (or drop)
    let emptied = loop {
        let left = {
            let ms = st.map_servers.lock().unwrap();
            targets
                .iter()
                .filter(|id| matches!(ms.get(**id), Some(Some(h)) if h.users > 0))
                .count()
        };
        if left == 0 {
            break true;
        }
        if Instant::now() >= deadline {
            break false;
        }
        let _ = tokio::time::timeout(Duration::from_millis(200), st.online_notify.notified()).await;
    };
    // phase 2: let in-flight handoffs land or fail so `arrived`
    // isn't undercounted (a reconnect + 0x2afc is sub-second).
    // `unresolved` = evacuees still online somewhere but without a
    // confirmed landing — quit-and-relogin during the drain, or a
    // handoff still in flight. Bounded at 5 s.
    if emptied {
        let grace_end = Instant::now() + Duration::from_millis(1500);
        let settle_end = Instant::now() + Duration::from_secs(5);
        loop {
            let unresolved = {
                let d = st.drains.lock().unwrap();
                let on = st.online.lock().unwrap();
                targets
                    .iter()
                    .flat_map(|id| d.get(id))
                    .map(|t| {
                        t.expected
                            .iter()
                            .filter(|c| on.contains_key(*c))
                            .count()
                            .saturating_sub(t.arrived.len())
                    })
                    .sum::<usize>()
            };
            let now = Instant::now();
            if (unresolved == 0 && now >= grace_end) || now >= settle_end {
                break;
            }
            let _ =
                tokio::time::timeout(Duration::from_millis(300), st.online_notify.notified()).await;
        }
    }
    reply(Some(emptied), st)
}

fn getcount(st: &Arc<State>) -> Value {
    let n = st.count_users();
    let name = &st.cfg.char_.server_name;
    ok_text(format!(
        "  Number of online players (server: number).\n    {name:<20} : {n:5}\n"
    ))
}

async fn who(st: &Arc<State>, args: &[String]) -> Value {
    let Some(v) = args.first() else {
        return err_text("usage: who <account id|name>");
    };
    let v = v.clone();
    st.db.blocking_conn(move |c| {
        let id: i64 = if let Ok(id) = v.parse::<i64>() {
            id
        } else if let Some(id) = acct_id_by_name(c, &v) {
            id
        } else {
            return err_text(format!("Account [{v}] not found."));
        };
        let r = c.query_row(
            "SELECT id,name,email,state,ban_until,memo,last_login,login_count,last_ip FROM accounts WHERE id=?1",
            [id],
            |r| {
                Ok((
                    r.get::<_, i64>(0)?, r.get::<_, String>(1)?,
                    r.get::<_, Option<String>>(2)?.unwrap_or_default(),
                    r.get::<_, i64>(3)?, r.get::<_, i64>(4)?,
                    r.get::<_, Option<String>>(5)?.unwrap_or_default(),
                    r.get::<_, Option<i64>>(6)?.unwrap_or_default(),
                    r.get::<_, i64>(7)?,
                    r.get::<_, Option<String>>(8)?.unwrap_or_default(),
                ))
            },
        );
        let Ok((id, name, email, state, ban, memo, last, cnt, lip)) = r else {
            return err_text(format!("Account [{v}] not found."));
        };
        let banned = if ban > 0 {
            fmt_ymd_slash(ban)
        } else {
            "not banned".to_string()
        };
        let last_s = fmt_ymd_slash(last / 1000);
        let chars: Vec<String> = c
            .prepare("SELECT name FROM characters WHERE account_id=?1")
            .unwrap()
            .query_map([id], |r| r.get(0))
            .unwrap()
            .flatten()
            .collect();
        ok_text(format!(
            "Account [{name}] [id: {id}]\n  state: {} ({state})\n  e-mail: {email}\n  memo: {memo}\n  ban until: {banned}\n  last login: {last_s}\n  login count: {cnt}\n  last ip: {lip}\n  characters: {}\n",
            state_label(state),
            chars.join(", ")
        ))
    })
    .await
}

async fn id_cmd(st: &Arc<State>, args: &[String]) -> Value {
    let Some(v) = args.first() else {
        return err_text("usage: id <account name>");
    };
    let v = v.clone();
    st.db.blocking_conn(move |c| match acct_id_by_name(c, &v) {
        Some(id) => ok_text(format!("{id}\n")),
        None => err_text(format!("Account [{v}] not found.")),
    })
    .await
}

async fn name_cmd(st: &Arc<State>, args: &[String]) -> Value {
    let Some(v) = args.first() else {
        return err_text("usage: name <account id>");
    };
    let id: i64 = match v.parse() {
        Ok(id) => id,
        Err(_) => return err_text("usage: name <account id>"),
    };
    st.db.blocking_conn(move |c| match acct_name_by_id(c, id) {
        Some(n) => ok_text(format!("{n}\n")),
        None => err_text(format!("Account id [{id}] not found.")),
    })
    .await
}

async fn list_cmd(st: &Arc<State>, cmd: &str, args: &[String]) -> Value {
    // list/ls [start_id [end_id]]; listban = non-ok only; listgm =
    // GM only; listok = ok only; search <expression> [-r regex]
    let (start, end, filter) = match cmd {
        "search" => {
            let expr = args
                .iter()
                .find(|a| !a.starts_with('-'))
                .cloned()
                .unwrap_or_default();
            (0i64, i64::MAX, expr)
        }
        _ => {
            let start: i64 = args.first().and_then(|a| a.parse().ok()).unwrap_or(0);
            let end: i64 = args.get(1).and_then(|a| a.parse().ok()).unwrap_or(i64::MAX);
            (start, end, String::new())
        }
    };
    let is_regex = args.iter().any(|a| a == "-r");
    let cmd = cmd.to_string();
    let gm_map = st.gm.lock().unwrap().clone();
    st.db.blocking_conn(move |c| {
        let mut q = c
            .prepare(
                "SELECT a.id,a.name,a.login_count,a.state \
                 FROM accounts a WHERE a.id>=?1 AND a.id<=?2 ORDER BY a.id LIMIT 2000",
            )
            .unwrap();
        // (id, name, gm level, login count, state)
        let rows: Vec<(i64, String, i64, i64, i64)> = q
            .query_map([start, end], |r| {
                let id: i64 = r.get(0)?;
                let name: String = r.get(1)?;
                let count: i64 = r.get(2)?;
                let state: i64 = r.get(3)?;
                let gm = *gm_map.get(&(id as u32)).unwrap_or(&0) as i64;
                Ok((id, name, gm, count, state))
            })
            .unwrap()
            .flatten()
            .collect();
        // search filters on the name; -r treats the expression as a
        // regex (an invalid pattern falls back to a substring match)
        let re = if is_regex && !filter.is_empty() {
            regex::Regex::new(&filter).ok()
        } else {
            None
        };
        let needle = filter.to_lowercase();
        let mut out = String::new();
        let mut n = 0;
        for (id, name, gm, count, state) in &rows {
            let keep = match cmd.as_str() {
                "listban" => *state != 0,
                "listgm" => *gm > 0,
                "listok" => *state == 0,
                "search" => {
                    if filter.is_empty() {
                        true
                    } else if let Some(re) = &re {
                        re.is_match(name)
                    } else {
                        name.to_lowercase().contains(&needle)
                    }
                }
                _ => true,
            };
            if !keep {
                continue;
            }
            n += 1;
            let gm_s = if *gm > 0 {
                format!("{gm:2} ")
            } else {
                "   ".to_string()
            };
            out += &format!(
                "{:10} {}{:<24} {:6} {}\n",
                id,
                gm_s,
                name,
                count,
                state_label(*state)
            );
        }
        if n == 0 {
            return ok_text("No account found.\n".to_string());
        }
        out += &format!("{n} account(s) found.\n");
        ok_text(out)
    })
    .await
}

/// The shared shape of the account-mutation commands: resolve the
/// name, run `update` on the account row, optionally kick the
/// account's sessions, print `Account [name][id: N] <tail>`.
async fn mutate_account(
    st: &Arc<State>,
    name: &str,
    kick: bool,
    update: impl FnOnce(&crate::db::Db, i64) -> crate::db::Result<()> + Send + 'static,
    tail: String,
) -> Value {
    let name = name.to_string();
    let namec = name.clone();
    let r = st
        .db
        .blocking(move |db| -> crate::db::Result<Option<i64>> {
            let Some(id) = db.account_id_by_name(&namec)? else {
                return Ok(None);
            };
            update(db, id)?;
            Ok(Some(id))
        })
        .await;
    match r {
        Ok(Some(id)) => {
            if kick {
                kick_account(st, id as u32);
            }
            ok_text(format!("Account [{name}][id: {id}] {tail}\n"))
        }
        Ok(None) => err_text(format!("Account [{name}] not found.")),
        Err(e) => err_text(format!("db: {e}")),
    }
}

async fn memo(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: memo <account name> <memo>");
    }
    let memo = args[1..].join(" ");
    mutate_account(
        st,
        &args[0],
        false,
        move |db, id| db.set_memo(id, &memo),
        "memo successfully changed.".to_string(),
    )
    .await
}

async fn email(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: email <account name> <email>");
    }
    let email = args[1].clone();
    mutate_account(
        st,
        &args[0],
        false,
        move |db, id| db.set_email(id, Some(&email)),
        "e-mail successfully changed.".to_string(),
    )
    .await
}

async fn state_cmd(st: &Arc<State>, args: &[String], state: i64, verb: &str) -> Value {
    let Some(name) = args.first() else {
        return err_text(format!("usage: {verb} <account name>"));
    };
    mutate_account(
        st,
        name,
        state != 0,
        move |db, id| db.set_account_state(id, state),
        format!("successfully {verb}ed."),
    )
    .await
}

/// Kick every session of an account (in game or char screen).
fn kick_account(st: &Arc<State>, account_id: u32) {
    let recs: Vec<Arc<std::sync::Mutex<super::state::PlayerSession>>> = st
        .player_sessions
        .lock()
        .unwrap()
        .values()
        .filter(|r| r.lock().unwrap().account_id == account_id)
        .cloned()
        .collect();
    // `online` is keyed by char_id, not account_id: drop the kicked
    // chars' entries so maps stop seeing the player
    forget_online(st, kick_sessions(&recs));
}

async fn state_set(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: state <account name> <new_state> [error_message_#7]");
    }
    let new_state: i64 = match args[1].parse() {
        Ok(v) => v,
        Err(_) => return err_text("bad state"),
    };
    let emsg = args.get(2).cloned().unwrap_or_default();
    mutate_account(
        st,
        &args[0],
        new_state != 0,
        move |db, id| db.set_account_state_msg(id, new_state, &emsg),
        "state successfully changed.".to_string(),
    )
    .await
}

fn parse_ban_date(s: &str) -> Option<i64> {
    // yyyy/mm/dd [hh:mm:ss] or bare 0
    if s == "0" {
        return Some(0);
    }
    for fmt in ["%Y/%m/%d %H:%M:%S", "%Y/%m/%d"] {
        if let Ok(d) = chrono::NaiveDateTime::parse_from_str(s, fmt) {
            return Some(d.and_utc().timestamp());
        }
        if let Ok(d) = chrono::NaiveDate::parse_from_str(s, fmt) {
            return Some(d.and_hms_opt(23, 59, 59)?.and_utc().timestamp());
        }
    }
    None
}

async fn ban_cmd(st: &Arc<State>, args: &[String]) -> Value {
    // ban yyyy/mm/dd hh:mm:ss <account name>
    if args.len() < 3 {
        return err_text("usage: ban yyyy/mm/dd hh:mm:ss <account name>");
    }
    let name = args[2].clone();
    let date = format!("{} {}", args[0], args[1]);
    let Some(ts) = parse_ban_date(&date) else {
        return err_text("bad date");
    };
    set_ban(st, &name, ts).await
}

async fn banset(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: banset <account name> yyyy/mm/dd [hh:mm:ss]");
    }
    let name = args[0].clone();
    let date = args[1..].join(" ");
    let Some(ts) = parse_ban_date(&date) else {
        return err_text("bad date");
    };
    set_ban(st, &name, ts).await
}

async fn set_ban(st: &Arc<State>, name: &str, ts: i64) -> Value {
    mutate_account(
        st,
        name,
        true,
        move |db, id| db.set_account_ban(id, ts),
        "banishment successfully changed.".to_string(),
    )
    .await
}

async fn banadd(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: banadd <account name> <modifier>");
    }
    let name = args[0].clone();
    let m = args[1].clone();
    // modifier: +N<unit> sequences, units a|y,m,j|d,h,mn,s
    let now = chrono::Utc::now().timestamp();
    let mut delta = 0i64;
    let re = regex::Regex::new(r"([+-]?\d+)(a|y|mn|m|j|d|h|s)").unwrap();
    let mut matched = false;
    for cap in re.captures_iter(&m) {
        matched = true;
        let v: i64 = cap[1].parse().unwrap_or(0);
        delta += match &cap[2] {
            "a" | "y" => v * 365 * 86400,
            "m" => v * 30 * 86400,
            "j" | "d" => v * 86400,
            "h" => v * 3600,
            "mn" => v * 60,
            "s" => v,
            _ => 0,
        };
    }
    if !matched {
        return err_text("bad modifier");
    }
    mutate_account(
        st,
        &name,
        false,
        move |db, id| {
            let cur = db.account_ban_until(id)?.unwrap_or(0);
            let new = std::cmp::max(0, if cur > 0 { cur + delta } else { now + delta });
            db.set_account_ban(id, new)
        },
        "banishment successfully changed.".to_string(),
    )
    .await
}

async fn unban(st: &Arc<State>, args: &[String]) -> Value {
    let Some(name) = args.first() else {
        return err_text("usage: unban <account name>");
    };
    set_ban(st, name, 0).await
}

async fn delete(st: &Arc<State>, args: &[String]) -> Value {
    let Some(name) = args.first() else {
        return err_text("usage: delete <account name>");
    };
    let name = name.clone();
    let namec = name.clone();
    let r = st.db.blocking_conn(move |c| acct_id_by_name(c, &namec)).await;
    let Some(id) = r else {
        return err_text(format!("Account [{name}] not found."));
    };
    // kick first and let the map's final 0x2b01 land before the rows
    // disappear
    kick_account(st, id as u32);
    for _ in 0..50 {
        let still = {
            st.player_sessions
                .lock()
                .unwrap()
                .values()
                .any(|r| r.lock().unwrap().account_id == id as u32)
        };
        if !still {
            break;
        }
        tokio::time::sleep(std::time::Duration::from_millis(100)).await;
    }
    // delete each character through the same routine the char screen
    // uses (party leave, divorce)
    let cids: Vec<i64> = st.db.blocking_conn(move |c| {
        let mut q = c
            .prepare("SELECT id FROM characters WHERE account_id=?1")
            .unwrap();
        q.query_map([id], |r| r.get(0)).unwrap().flatten().collect()
    })
    .await;
    for cid in cids {
        super::client::delete_character(st, cid as u32).await;
    }
    st.db.blocking_conn(move |c| {
        c.execute("DELETE FROM accounts WHERE id=?1", [id]).unwrap();
    })
    .await;
    ok_text(format!(
        "Account [{name}][id: {id}] is successfully DELETED.\n"
    ))
}

async fn password_cmd(st: &Arc<State>, args: &[String], password_stdin: Option<String>) -> Value {
    let name = args.first().cloned().unwrap_or_default();
    let newp = password_stdin.or_else(|| args.get(1).cloned());
    let Some(newp) = newp else {
        return err_text("usage: password <account name> <new password>");
    };
    mutate_account(
        st,
        &name,
        false,
        move |db, id| {
            let h = crate::auth::password::hash_argon2id(newp.as_bytes())
                .map_err(|e| crate::db::DbError::Msg(e.to_string()))?;
            db.set_password(id, &h, crate::auth::password::Scheme::Argon2id, None)
        },
        "password successfully changed.".to_string(),
    )
    .await
}

async fn check_cmd(st: &Arc<State>, args: &[String], password_stdin: Option<String>) -> Value {
    let name = args.first().cloned().unwrap_or_default();
    let pw = password_stdin.or_else(|| args.get(1).cloned());
    let Some(pw) = pw else {
        return err_text("usage: check <account name> <password>");
    };
    st.db.blocking_conn(move |c| match acct_id_by_name(c, &name) {
        Some(id) => {
            let Some((h, scheme, salt)) =
                crate::db::password_row_conn(c, id).ok().flatten()
            else {
                return err_text(format!("Account [{name}] not found."));
            };
            match crate::auth::password::verify(&scheme, &h, salt.as_deref(), pw.as_bytes()) {
                Ok(crate::auth::password::Verify::Ok)
                | Ok(crate::auth::password::Verify::OkNeedsRehash) => ok_text(format!(
                    "The proposed password is correct for the account [{name}][id: {id}].\n"
                )),
                _ => ok_text(format!(
                    "The proposed password is INCORRECT for the account [{name}][id: {id}].\n"
                )),
            }
        }
        None => err_text(format!("Account [{name}] not found.")),
    })
    .await
}

async fn create(st: &Arc<State>, args: &[String], password_stdin: Option<String>) -> Value {
    // create <name> <email> <password>; the tmwa-admin form
    // `create <name> <M|F|N> <email> <password>` is also accepted
    // (the sex letter is ignored)
    let args = strip_sex_arg(args);
    let pw = password_stdin.or_else(|| args.get(2).cloned());
    if args.len() < 2 || pw.is_none() {
        return err_text("usage: create <name> <email> <password>");
    }
    let name = args[0].clone();
    let email = args[1].clone();
    let pw = pw.unwrap();
    let Ok(h) = crate::auth::password::hash_argon2id(pw.as_bytes()) else {
        return err_text("hashing failed");
    };
    st.db.blocking_conn(move |c| {
        if acct_id_by_name(c, &name).is_some() {
            return err_text(format!("Account [{name}] already exists."));
        }
        match crate::db::Db::create_account(c, &name, &h, &email) {
            Ok(id) => {
                let mut v = ok_text(format!(
                    "Account [{name}] is successfully created [id: {id}].\n"
                ));
                v["id"] = serde_json::json!(id);
                v
            }
            Err(crate::db::DbError::NameTaken) => {
                err_text(format!("Account [{name}] already exists."))
            }
            Err(e) => err_text(format!("create failed: {e}")),
        }
    })
    .await
}

/// tmwa-admin puts a sex letter between the name and the rest in
/// `create`/`add`; drop it so both the old (`<name> M ...`) and the
/// new argument lists work.
fn strip_sex_arg(args: &[String]) -> Vec<String> {
    let mut v = args.to_vec();
    if v.len() >= 3 && v[1].len() == 1 && "mfn".contains(v[1].to_ascii_lowercase().as_str()) {
        v.remove(1);
    }
    v
}

async fn add_cmd(st: &Arc<State>, args: &[String], password_stdin: Option<String>) -> Value {
    // add <name> <password>; tmwa-admin's `add <name> <sex>
    // <password>` is accepted too (sex ignored). Default email
    // a@a.com like tmwa's add.
    let args = strip_sex_arg(args);
    let pw = password_stdin.or_else(|| args.get(1).cloned());
    if args.is_empty() || pw.is_none() {
        return err_text("usage: add <name> <password>");
    }
    create(st, &[args[0].clone(), "a@a.com".to_string()], pw).await
}

async fn gm_cmd(st: &Arc<State>, args: &[String]) -> Value {
    if args.is_empty() {
        return err_text("usage: gm <account name> [GM level]");
    }
    let name = args[0].clone();
    let level: u32 = args.get(1).and_then(|a| a.parse().ok()).unwrap_or(0);
    let namec = name.clone();
    let r = st.db.blocking_conn(move |c| acct_id_by_name(c, &namec)).await;
    match r {
        Some(id) => {
            {
                let mut gm = st.gm.lock().unwrap();
                if level > 0 {
                    gm.insert(id as u32, level);
                } else {
                    gm.remove(&(id as u32));
                }
            }
            write_gm_file(st);
            super::send_gm_list(st);
            ok_text(format!(
                "Account [{name}][id: {id}] GM level successfully changed.\n"
            ))
        }
        None => err_text(format!("Account [{name}] not found.")),
    }
}

fn write_gm_file(st: &Arc<State>) {
    let gm = st.gm.lock().unwrap().clone();
    // sort by account id so the file is stable across reloads
    let mut rows: Vec<(u32, u32)> = gm.iter().map(|(i, l)| (*i, *l)).collect();
    rows.sort();
    let mut txt = String::new();
    for (id, level) in rows {
        txt += &format!("{id} {level}\n");
    }
    let path = &st.cfg.gate.gm_account_file;
    let _ = std::fs::write(path, txt);
}

fn kami(st: &Arc<State>, _cmd: &str, args: &[String]) -> Value {
    let msg = args.join(" ");
    if msg.is_empty() {
        return err_text("usage: kami|kamib <message>");
    }
    // kami/kamib share the same broadcast packet in tmwa; the two
    // names only differed in the admin log line
    let mut p = crate::proto::P3800::default();
    p.repeat = msg.bytes().map(|c| P3800Repeat { c }).collect();
    st.map_broadcast(&enc(move |v| p.encode(v)));
    ok_text(
        "Message sent to all map-server.
"
        .to_string(),
    )
}

async fn getall(st: &Arc<State>, args: &[String], scope: i64) -> Value {
    let Some(id) = args.first().and_then(|a| a.parse::<i64>().ok()) else {
        return err_text("usage: getall <account id> [--scope 1|2|all]");
    };
    let scope = args
        .iter()
        .position(|a| a == "--scope")
        .and_then(|i| args.get(i + 1))
        .and_then(|s| match s.as_str() {
            "all" => Some(0),
            _ => s.parse::<i64>().ok(),
        })
        .unwrap_or(scope);
    st.db.blocking_conn(move |c| {
        if !acct_exists(c, id) {
            return err_text(err_no_acct(id));
        }
        // mirror ladmin's 0x7957 reply text
        let mut vars = Vec::new();
        let mut out = String::new();
        let scopes: Vec<i64> = if scope == 0 { vec![1, 2] } else { vec![scope] };
        for sc in scopes {
            let rows = crate::db::get_account_vars_conn(c, id, sc).unwrap();
            for (n, v) in rows {
                let prefix = if sc == 2 { "##" } else { "#" };
                let full = format!("{prefix}{n}");
                out += &format!("Variable {full} == `{v}`\n");
                vars.push(serde_json::json!({"name": full, "value": v}));
            }
        }
        out = format!("Variables {} of 16 used.\n{out}", vars.len());
        let mut v = ok_text(out);
        v["vars"] = serde_json::json!(vars);
        v
    })
    .await
}

async fn getaccreg(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: get <account id> <variable>");
    }
    let id: i64 = args[0].parse().unwrap_or(-1);
    let name = args[1].clone();
    st.db.blocking_conn(move |c| {
        if !acct_exists(c, id) {
            return err_text(err_no_acct(id));
        }
        let scope = accreg_scope(&name);
        let n = name.trim_start_matches('#');
        let v: Option<i64> = c
            .query_row(
                "SELECT value FROM account_vars WHERE account_id=?1 AND scope=?2 AND name=?3",
                rusqlite::params![id, scope, n],
                |r| r.get(0),
            )
            .ok();
        match v {
            Some(v) => ok_text(format!("Variable {name} == `{v}`\n")),
            None => ok_text("Variable not found.\n".to_string()),
        }
    })
    .await
}

async fn setaccreg(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 3 {
        return err_text("usage: set <account id> <variable> <value>");
    }
    let id: i64 = args[0].parse().unwrap_or(-1);
    let name = args[1].clone();
    let value: i64 = args[2].parse().unwrap_or(0);
    let stc = st.clone();
    let r = st.db.blocking_conn(move |c| {
        if !acct_exists(c, id) {
            return (String::new(), 0, false, Some(err_no_acct(id)));
        }
        let scope = accreg_scope(&name);
        let n = name.trim_start_matches('#').to_string();
        let existed: bool = c
            .query_row(
                "SELECT 1 FROM account_vars WHERE account_id=?1 AND scope=?2 AND name=?3",
                rusqlite::params![id, scope, n],
                |_| Ok(true),
            )
            .unwrap_or(false);
        crate::db::set_account_vars(c, id, scope, &[(n.clone(), value)]).unwrap();
        (n, scope, existed, None)
    })
    .await;
    let (r, scope, existed, missing) = r;
    if let Some(m) = missing {
        return err_text(m);
    }
    // Notify the maps of any of this account's online chars (##
    // becomes 0x2b11, # becomes 0x3804). `online` is keyed by
    // char_id, so find the account's chars through their sessions
    // (the same match kick_account does).
    let cids: Vec<u32> = stc
        .player_sessions
        .lock()
        .unwrap()
        .iter()
        .filter(|(_, rec)| rec.lock().unwrap().account_id == id as u32)
        .map(|(cid, _)| *cid)
        .collect();
    let mids: Vec<usize> = {
        let online = stc.online.lock().unwrap();
        cids.iter().filter_map(|c| online.get(c)).copied().collect()
    };
    let n = FixedStr::<32>::from_str_truncate(&r);
    for mid in mids {
        if scope == 2 {
            let mut p = crate::proto::P2B11::default();
            p.account_id = AccountId(id as u32);
            p.repeat = vec![P2B11Repeat {
                name: n,
                value: value as u32,
            }];
            stc.map_send(mid, enc(move |v| p.encode(v)));
        } else {
            let mut p = crate::proto::P3804::default();
            p.account_id = AccountId(id as u32);
            p.repeat = vec![P3804Repeat {
                name: n,
                value: value as u32,
            }];
            stc.map_send(mid, enc(move |v| p.encode(v)));
        }
    }
    if existed {
        ok_text("Variable changed.\n".to_string())
    } else {
        ok_text("New Variable created.\n".to_string())
    }
}

async fn delaccreg(st: &Arc<State>, args: &[String]) -> Value {
    if args.len() < 2 {
        return err_text("usage: del <account id> <variable>");
    }
    let id: i64 = args[0].parse().unwrap_or(-1);
    let name = args[1].clone();
    st.db.blocking_conn(move |c| {
        if !acct_exists(c, id) {
            return err_text(err_no_acct(id));
        }
        let scope = accreg_scope(&name);
        let n = name.trim_start_matches('#');
        let rows = c
            .execute(
                "DELETE FROM account_vars WHERE account_id=?1 AND scope=?2 AND name=?3",
                rusqlite::params![id, scope, n],
            )
            .unwrap();
        if rows > 0 {
            ok_text("Variable deleted.\n".to_string())
        } else {
            ok_text("Variable not found.\n".to_string())
        }
    })
    .await
}

async fn find(st: &Arc<State>, args: &[String]) -> Value {
    // find --id|--name|--email|--memo <v> — exact matches
    let field = args
        .iter()
        .find(|a| a.starts_with("--"))
        .map(|a| a.trim_start_matches('-').to_string())
        .unwrap_or_else(|| "name".to_string());
    let val = args
        .iter()
        .find(|a| !a.starts_with("--"))
        .cloned()
        .unwrap_or_default();
    let col = match field.as_str() {
        "id" => "id",
        "name" => "name",
        "email" => "email",
        "memo" => "memo",
        _ => return err_text("find: unknown field"),
    };
    st.db.blocking_conn(move |c| {
        let sql = format!(
            "SELECT id,name,state,email,last_login,login_count,last_ip,memo \
             FROM accounts WHERE {col} = ?1 LIMIT 50"
        );
        let mut q = c.prepare(&sql).unwrap();
        // (id, name, state, email, last_login ms, login_count, last_ip, memo)
        let rows: Vec<_> = q
            .query_map([val.clone()], |r| {
                Ok((
                    r.get::<_, i64>(0)?,
                    r.get::<_, String>(1)?,
                    r.get::<_, i64>(2)?,
                    r.get::<_, Option<String>>(3)?.unwrap_or_default(),
                    r.get::<_, Option<i64>>(4)?.unwrap_or(0),
                    r.get::<_, Option<i64>>(5)?.unwrap_or(0),
                    r.get::<_, Option<String>>(6)?.unwrap_or_default(),
                    r.get::<_, String>(7).unwrap_or_default(),
                ))
            })
            .unwrap()
            .flatten()
            .collect();
        let mut lines = Vec::new();
        let mut accounts = Vec::new();
        for (id, name, state, email, last, cnt, ip, memo) in &rows {
            let last_s = fmt_ymd_slash(last / 1000);
            lines.push(format!(
                "{id:10} {name:<24} st={state} email={email} last={last_s} logins={cnt} ip={ip} memo={memo}"
            ));
            accounts.push(serde_json::json!({
                "id": id, "name": name, "state": state,
                "email": email, "last_login": last,
                "login_count": cnt, "last_ip": ip, "memo": memo,
            }));
        }
        let mut v = if lines.is_empty() {
            ok_text("No account found.\n".to_string())
        } else {
            ok_text(lines.join("\n") + "\n")
        };
        v["accounts"] = serde_json::json!(accounts);
        v
    })
    .await
}

async fn chars_cmd(st: &Arc<State>, args: &[String]) -> Value {
    let flag = args
        .iter()
        .find(|a| a.starts_with("--"))
        .cloned()
        .unwrap_or_default();
    let val = args
        .iter()
        .find(|a| !a.starts_with("--"))
        .cloned()
        .unwrap_or_default();
    st.db.blocking_conn(move |c| {
        let sql = match flag.as_str() {
            "--account" => "SELECT c.id,c.account_id,c.name,c.slot,c.base_level,c.base_exp,c.zeny,c.last_map FROM characters c JOIN accounts a ON a.id=c.account_id WHERE a.name=?1 OR a.id=CAST(?1 AS INT)",
            "--name" | "--id" | "" => "SELECT c.id,c.account_id,c.name,c.slot,c.base_level,c.base_exp,c.zeny,c.last_map FROM characters c WHERE c.name=?1 OR c.id=CAST(?1 AS INT)",
            _ => return err_text("chars: unknown option"),
        };
        let sql = format!("{sql} ORDER BY c.id");
        let mut q = c.prepare(&sql).unwrap();
        let mut rows = Vec::new();
        let mut lines = Vec::new();
        let res = q
            .query_map([val.clone()], |r| {
                Ok((
                    r.get::<_, i64>(0)?,
                    r.get::<_, i64>(1)?,
                    r.get::<_, String>(2)?,
                    r.get::<_, i64>(3)?,
                    r.get::<_, i64>(4)?,
                    r.get::<_, i64>(5)?,
                    r.get::<_, i64>(6)?,
                    r.get::<_, String>(7)?,
                ))
            })
            .unwrap();
        for r in res.flatten() {
            let (id, account_id, name, slot, lvl, exp, zeny, map) = r;
            // equipped item ids: inventory entries with a nonzero
            // equip field, in slot order
            let mut iq = c
                .prepare("SELECT item_id FROM character_items WHERE char_id=?1 AND equip!=0 ORDER BY idx")
                .unwrap();
            let equipped: Vec<i64> = iq
                .query_map(rusqlite::params![id], |r| r.get(0))
                .unwrap()
                .flatten()
                .collect();
            lines.push(format!(
                "{id} {account_id} {name:<24} slot={slot} lvl={lvl} exp={exp} zeny={zeny} map={map}"
            ));
            rows.push(serde_json::json!({
                "id": id, "account_id": account_id, "name": name,
                "slot": slot, "base_level": lvl, "base_exp": exp,
                "zeny": zeny, "map": map, "equipped": equipped,
            }));
        }
        let mut v = if lines.is_empty() {
            ok_text("No character found.\n".to_string())
        } else {
            ok_text(lines.join("\n") + "\n")
        };
        v["chars"] = serde_json::json!(rows);
        v
    })
    .await
}

fn help() -> Value {
    ok_text(
        " help/?                          -- Display this help\n \
         add <name> <sex> <password>   -- Create an account (default email)\n \
         ban <date> <time> <name>     -- Set ban end\n \
         banadd <name> <modifier>    -- Add/subtract ban time\n \
         banset <name> <date>        -- Set ban end\n \
         block/unblock <name>        -- Set state 5/0\n \
         check <name> <password>     -- Check a password\n \
         create <name> <email> <pw>  -- Create an account\n \
         delete <name>               -- Delete an account\n \
         drain [--wait]              -- Hold players and wait for saves\n \
         email <name> <email>        -- Change e-mail\n \
         find --id|--name|--email|--memo <v> -- Search accounts\n \
         get/g <id> <var>            -- Show a ## (or #) variable\n \
         getall/ga <id> [--scope n]  -- Show all ## variables\n \
         getcount                    -- Number of online players\n \
         gm <name> [level]           -- Set GM level\n \
         id <name> / name <id>       -- Account id/name lookup\n \
         info/who <id|name>          -- Account information\n \
         kami/kamib <message>        -- Broadcast (yellow/blue)\n \
         kick <name>                 -- Disconnect a player\n \
         list/ls [a b]               -- List accounts\n \
         listban/listgm/listok       -- Filtered lists\n \
         memo <name> <memo>          -- Change memo\n \
         online                      -- Who is online\n \
         password <name> <pw>        -- Change password\n \
         reloadGM                    -- Reload GM file\n \
         search <expr> [-r]          -- Search accounts\n \
         set/s <id> <var> <value>    -- Set a ## variable\n \
         del/d <id> <var>            -- Delete a ## variable\n \
         state <name> <n> [msg]      -- Change state\n \
         status                      -- Gate status\n \
         unban <name>                -- Remove a ban\n \
         version                     -- Gate version\n \
         quit/exit/end/q             -- End stdin session\n",
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::Config;
    use std::sync::Mutex;

    fn test_state() -> Arc<State> {
        Arc::new(State::new(
            Config::default(),
            Arc::new(crate::db::Db::open_memory().unwrap()),
        ))
    }

    fn session(account_id: u32, char_id: u32) -> Arc<Mutex<crate::serve::state::PlayerSession>> {
        Arc::new(Mutex::new(crate::serve::state::PlayerSession {
            account_id,
            char_id,
            sex: 0,
            login_id1: 0,
            login_id2: 0,
            client_ip: 0,
            server_tick: 0,
            server_tick_at: Instant::now(),
            map_id: 0,
            map_name: String::new(),
            map_name_stale: false,
            npc_id: 0,
            trade_open: false,
            storage_open: false,
            quitting: false,
            saw_0073: false,
            transferring: false,
            held: false,
            hold_signal: None,
            kicked: false,
            held_since: None,
        }))
    }

    /// `online` is keyed by char_id: kicking an account must drop
    /// its chars' entries, and a char_id equal to some account_id
    /// must never remove an unrelated session's entry.
    #[test]
    fn kick_account_removes_char_entries() {
        let st = test_state();
        let kicked = session(100, 2000007);
        // unrelated session whose char_id equals account 100
        let other = session(55, 100);
        {
            let mut ps = st.player_sessions.lock().unwrap();
            ps.insert(2000007, kicked.clone());
            ps.insert(100, other.clone());
            let mut on = st.online.lock().unwrap();
            on.insert(2000007, 0);
            on.insert(100, 0);
        }
        kick_account(&st, 100);
        assert!(kicked.lock().unwrap().kicked);
        assert!(!other.lock().unwrap().kicked);
        let on = st.online.lock().unwrap();
        assert!(!on.contains_key(&2000007));
        assert!(on.contains_key(&100));
    }
}
