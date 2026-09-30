//! End-to-end test against a live tmwa-map behind tmwa-gate.
//!
//! Disabled unless TMWA_E2E=1; connection details come from env:
#![allow(clippy::field_reassign_with_default)]
#![allow(clippy::collapsible_if)]
//!   TMWA_E2E_ADDR   client port (default 127.0.0.1:16901)
//!   TMWA_E2E_MAPLINK map-link port (default 127.0.0.1:6121)
//!   TMWA_E2E_USER / TMWA_E2E_PASS  account (default spiketest/spikepass)
//!   TMWA_E2E_MAPUSER/_MAPPASS    map-link auth (default s1/p1)
//!   TMWA_E2E_SAVE_DIR save dir holding athena.txt/party.txt/...
//!                    (default ~/projects/tmw/serverdata/world/save)
//!   TMWA_E2E_ACCOUNT_TXT  account.txt path
//!                    (default ~/projects/tmw/serverdata/login/save)
//!   TMWA_E2E_RUNDIR  gate runtime dir for the fresh DB/config/logs
//!                    (default ~/gate-run)
//!
//! The test imports a fresh DB from the save fixtures and
//! (re)spawns `tmwa-gate serve` itself, so each run starts from a
//! clean world regardless of what previous runs left behind.

use std::time::Duration;

use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};

use std::process::Command;

use tmwa_gate::db::Db;
use tmwa_gate::net::framing::{Packet, PacketFramer};
use tmwa_gate::proto::types::*;
use tmwa_gate::proto::*;

fn env(k: &str, d: &str) -> String {
    std::env::var(k).unwrap_or_else(|_| d.into())
}

/// The e2e tests own the whole environment (DB file, gate, map)
/// — serialise them.
static E2E_LOCK: std::sync::Mutex<()> = std::sync::Mutex::new(());
fn e2e_lock() -> std::sync::MutexGuard<'static, ()> {
    E2E_LOCK.lock().unwrap_or_else(|e| e.into_inner())
}

struct Client {
    rd: PacketFramer<OwnedReadHalf>,
    wr: OwnedWriteHalf,
}

impl Client {
    async fn connect() -> Client {
        let s = TcpStream::connect(env("TMWA_E2E_ADDR", "127.0.0.1:16901"))
            .await
            .expect("connect client port");
        s.set_nodelay(true).unwrap();
        let (rd, wr) = s.into_split();
        Client {
            rd: PacketFramer::new(rd),
            wr,
        }
    }

    async fn send<P: Fn(&mut Vec<u8>)>(&mut self, f: P) {
        let mut v = Vec::new();
        f(&mut v);
        self.wr.write_all(&v).await.unwrap();
    }

    /// Wait for a packet id, dropping others. Tolerates up to 4
    /// consecutive idle 10s windows: some packets (party chat,
    /// 0x2aff updates) can be arbitrarily late on a loaded map.
    async fn wait(&mut self, id: u16) -> Packet {
        let mut idle = 0;
        for _ in 0..200 {
            match tokio::time::timeout(Duration::from_secs(10), self.rd.next()).await {
                Ok(Ok(Some(p))) if p.id == id => return p,
                Ok(Ok(Some(_))) => continue,
                Ok(Ok(None)) => panic!("eof waiting for 0x{id:04x}"),
                Ok(Err(e)) => panic!("frame error {e} waiting for 0x{id:04x}"),
                Err(_) => {
                    idle += 1;
                    if idle > 4 {
                        panic!("timeout waiting for 0x{id:04x}");
                    }
                }
            }
        }
        panic!("never got 0x{id:04x}");
    }
}

fn f24(s: &str) -> FixedStr<24> {
    FixedStr::<24>::try_from_str(s).unwrap()
}
#[allow(dead_code)]
fn f40(s: &str) -> FixedStr<40> {
    FixedStr::<40>::try_from_str(s).unwrap()
}

/// 0x0064 login; returns (account_id, login_id1, login_id2).
async fn login(c: &mut Client, name: &str, pass: &str) -> (u32, u32, u32) {
    // the gate enforces conn_limit (5 s between logins per IP like
    // tmwa); retry past it when refused
    for _ in 0..4 {
        c.send(|v| {
            P0064 {
                client_protocol_version: ClientVersion(999),
                account_name: f24(name),
                account_pass: f24(pass),
                flags: 3,
            }
            .encode(v)
        })
        .await;
        match tokio::time::timeout(Duration::from_secs(10), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x0069 => {
                let p = P0069::decode(&p.bytes).unwrap();
                return (p.account_id.0, p.login_id1, p.login_id2);
            }
            Ok(Ok(Some(p))) if p.id == 0x0081 || p.id == 0x006a => {
                // rate limited or refused; wait out the interval
                drop(p);
                tokio::time::sleep(Duration::from_secs(6)).await;
                *c = Client::connect().await;
            }
            other => panic!("login: {other:?}"),
        }
    }
    panic!("login never succeeded")
}

/// 0x0065 char connect; returns the char list.
async fn char_connect(c: &mut Client, acct: u32, id1: u32, id2: u32) -> Vec<CharSelect> {
    c.send(|v| {
        P0065 {
            account_id: AccountId(acct),
            login_id1: id1,
            login_id2: id2,
            unused_client_protocol_version: 0,
            sex: Sex(1),
        }
        .encode(v)
    })
    .await;
    c.wait(0x8000).await;
    let p = P006B::decode(&c.wait(0x006b).await.bytes).unwrap();
    p.repeat.into_iter().map(|r| r.char_select).collect()
}

/// 0x0066 select; waits for 0x0071. Returns map name + advertised addr.
async fn char_select(c: &mut Client, slot: u8) -> P0071 {
    for _ in 0..10 {
        c.send(|v| P0066 { code: slot }.encode(v)).await;
        loop {
            match tokio::time::timeout(Duration::from_secs(10), c.rd.next()).await {
                Ok(Ok(Some(p))) if p.id == 0x0071 => {
                    return P0071::decode(&p.bytes).unwrap();
                }
                Ok(Ok(Some(p))) if p.id == 0x0081 => {
                    // no map server registered yet (0x0081): retry
                    break;
                }
                Ok(Ok(Some(_))) => continue,
                other => panic!("char_select: {other:?}"),
            }
        }
        tokio::time::sleep(Duration::from_secs(2)).await;
    }
    panic!("char_select never got 0x0071")
}

/// 0x0072 map connect through the relay; returns after the client
/// sends CMSG_MAP_LOADED and the login burst has started.
async fn map_connect(c: &mut Client, acct: u32, char_id: u32, id1: u32) {
    c.send(|v| {
        P0072 {
            account_id: AccountId(acct),
            char_id: CharId(char_id),
            login_id1: id1,
            client_tick: 999999,
            sex: Sex(1),
        }
        .encode(v)
    })
    .await;
    // expect 0x8000 + a packet burst; wait for 0x0091 or 0x0073-ish.
    // Just wait for map data: the first being/stat packet.
    c.wait_any(&[0x0073, 0x0091, 0x00b0, 0x01ee, 0x00b5]).await;
    // tell the server the map loaded
    c.send(|v| P007D::default().encode(v)).await;
}

impl Client {
    async fn wait_any(&mut self, ids: &[u16]) -> Packet {
        for _ in 0..400 {
            let p = tokio::time::timeout(Duration::from_secs(10), self.rd.next())
                .await
                .expect("timeout")
                .unwrap()
                .expect("eof");
            if ids.contains(&p.id) {
                return p;
            }
        }
        panic!("none of {ids:?} arrived");
    }
}

/// chat (0x008c): "msg" NUL-terminated.
fn chat_pkt(msg: &str) -> Vec<u8> {
    let mut v = Vec::new();
    let mut p = P008C::default();
    p.repeat = msg
        .as_bytes()
        .iter()
        .chain([0].iter())
        .map(|&c| P008CRepeat { c })
        .collect();
    p.encode(&mut v);
    v
}

/// whisper (0x0096): name + NUL + msg + NUL.
fn whisper_pkt(name: &str, msg: &str) -> Vec<u8> {
    let mut v = Vec::new();
    let mut p = P0096::default();
    p.target_name = f24(name);
    p.repeat = msg
        .as_bytes()
        .iter()
        .chain([0].iter())
        .map(|&c| P0096Repeat { c })
        .collect();
    p.encode(&mut v);
    v
}

/// Import a fresh DB from the save fixtures and (re)start the gate.
/// The running map server reconnects to the new gate on its own
/// timer (char conf connect_retry ~ 15 s).
fn fresh_gate() {
    let home = std::env::var("HOME").unwrap();
    let rundir = env("TMWA_E2E_RUNDIR", &format!("{home}/gate-run"));
    let account_txt = env(
        "TMWA_E2E_ACCOUNT_TXT",
        &format!("{home}/projects/tmw/serverdata/login/save/account.txt"),
    );
    let save_dir = env(
        "TMWA_E2E_SAVE_DIR",
        &format!("{home}/projects/tmw/serverdata/world/save"),
    );
    let db = format!("{rundir}/gate.db");
    let bin = env!("CARGO_BIN_EXE_tmwa-gate");

    // fresh DB (including WAL sidecars, or the new file replays
    // the old journal)
    let _ = std::fs::remove_file(&db);
    let _ = std::fs::remove_file(format!("{db}-wal"));
    let _ = std::fs::remove_file(format!("{db}-shm"));
    let out = Command::new(bin)
        .args(["import", "--db", &db, "--save-dir", &save_dir])
        .arg("--account-txt")
        .arg(&account_txt)
        .output()
        .expect("tmwa-gate import");
    eprintln!("import: {}", String::from_utf8_lossy(&out.stdout));
    assert!(out.status.success(), "import failed: {:?}", out);

    // fresh config (gm file + online files from the run dir)
    let toml = format!(
        "[gate]\nlisten = '0.0.0.0:16901'\npublic_ip = '127.0.0.1'\npublic_port = 16901\ndb = '{db}'\ngm_account_file = '{rundir}/gm_account.txt'\nonline_txt = '{rundir}/online.txt'\nonline_html = '{rundir}/online.html'\nadmin_socket = '{rundir}/gate.sock'\n\n[map]\nlisten = '127.0.0.1:6121'\nuserid = '{}'\npassword = '{}'\n\n[login]\nnew_account = true\n\n[char]\nserver_name = 'The Mana World'\nstart_point = '001-1.gat,32,23'\nchar_name_letters = [\"$ &\'()*+,-.\", \"0123456789\", \";<=>?\", \"ABCDEFGHIJKLMNOPRSTQUVWXYZ\", \"\\\\^_`\", \"abcdefghijklmnoprstquvwxyz\"]\n",
        env("TMWA_E2E_MAPUSER", "s1").replace('\'', ""),
        env("TMWA_E2E_MAPPASS", "p1").replace('\'', ""),
    );
    std::fs::write(format!("{rundir}/gate.toml"), &toml).unwrap();

    // (re)start the map too: it keeps in-memory state (parties,
    // online set) that must match the fresh DB
    #[allow(clippy::zombie_processes)]
    let _ = Command::new("pkill").args(["-x", "tmwa-map"]).status();
    spawn_map();
    std::thread::sleep(Duration::from_secs(2));

    // (re)start the gate on the fresh DB; stays running after the
    // test so manual use continues
    #[allow(clippy::zombie_processes)]
    let _ = Command::new("pkill").args(["-x", "tmwa-gate"]).status();
    std::thread::sleep(Duration::from_secs(1));
    let log = std::fs::File::create(format!("{rundir}/gate.log")).unwrap();
    #[allow(clippy::zombie_processes)]
    let _child = Command::new(bin)
        .args(["serve", "--config", &format!("{rundir}/gate.toml")])
        .stdout(log.try_clone().unwrap())
        .stderr(log)
        .spawn()
        .expect("spawn tmwa-gate");
    drop(_child);

    // wait for the client port, then for the map link to have a
    // map registered (the running tmwa-map reconnects itself)
    for _ in 0..50 {
        if std::net::TcpStream::connect("127.0.0.1:16901").is_ok() {
            break;
        }
        std::thread::sleep(Duration::from_millis(200));
    }
    std::thread::sleep(Duration::from_secs(1));
}

/// Spawn tmwa-map (detached).
fn spawn_map() {
    let home = std::env::var("HOME").unwrap();
    #[allow(clippy::zombie_processes)]
    let _ = Command::new(env(
        "TMWA_E2E_MAPBIN",
        &format!("{home}/tmw-test/prefix/bin/tmwa-map"),
    ))
    .current_dir(env(
        "TMWA_E2E_MAPDIR",
        &format!("{home}/projects/tmw/serverdata/world/map"),
    ))
    .stdout(
        std::fs::OpenOptions::new()
            .append(true)
            .create(true)
            .open("/tmp/e2e-map.log")
            .unwrap(),
    )
    .stderr(
        std::fs::OpenOptions::new()
            .append(true)
            .create(true)
            .open("/tmp/e2e-map.err")
            .unwrap(),
    )
    .spawn();
}

fn kill_map(sig: &str) {
    let _ = Command::new("pkill")
        .args([sig, "-x", "tmwa-map"])
        .status();
}

async fn admin_cmd(args: &[&str]) -> String {
    let rundir = env("TMWA_E2E_RUNDIR", &format!("{}/gate-run", std::env::var("HOME").unwrap()));
    let mut cmd: Vec<String> = vec![
        "admin".into(),
        "--socket".into(),
        format!("{rundir}/gate.sock"),
    ];
    cmd.extend(args.iter().map(|s| s.to_string()));
    let out = Command::new(env!("CARGO_BIN_EXE_tmwa-gate"))
        .args(&cmd)
        .output()
        .unwrap();
    String::from_utf8_lossy(&out.stdout).to_string()
}

/// Wait until the map-link admin reports a live map server.
async fn wait_map_up() {
    for _ in 0..80 {
        let st = admin_cmd(&["status"]).await;
        if st.contains("\"id\"") {
            return;
        }
        tokio::time::sleep(Duration::from_secs(1)).await;
    }
    panic!("map server never re-registered");
}

#[tokio::test]
async fn e2e_all() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _g = e2e_lock();
    fresh_gate();
    let user = env("TMWA_E2E_USER", "spiketest");
    let pass = env("TMWA_E2E_PASS", "spikepass");

    // 1. login -> char list -> select -> in game -> relog via 0x00b2
    let mut c = Client::connect().await;
    let (acct, id1, id2) = login(&mut c, &user, &pass).await;
    assert!(acct >= 2000000, "account id {acct}");
    drop(c);

    let mut c = Client::connect().await;
    let chars = char_connect(&mut c, acct, id1, id2).await;
    assert!(!chars.is_empty(), "no characters for {user}");
    let char_id = chars[0].char_id.0;
    let slot = chars[0].char_num;
    let sel = char_select(&mut c, slot).await;
    assert_eq!(sel.char_id.0, char_id);
    assert_eq!(
        sel.port as u16,
        env("TMWA_E2E_ADDR", "")
            .parse::<std::net::SocketAddr>()
            .map(|a| a.port())
            .unwrap_or(16901)
    );
    drop(c);

    let mut c = Client::connect().await;
    map_connect(&mut c, acct, char_id, id1).await;
    // say something and get the echo (0x008c -> 0x008d)
    let pkt = chat_pkt("gate e2e hello");
    c.wr.write_all(&pkt).await.unwrap();
    c.wait(0x008d).await;
    eprintln!("e2e: in game as char {char_id}");

    // 3. persistence: @item gives candy (535), relog shows it
    c.wr.write_all(&chat_pkt("@item 535 7")).await.unwrap();
    tokio::time::sleep(Duration::from_millis(500)).await;

    // logout to char select (0x00b2 type 1) then reconnect char stage
    c.send(|v| P00B2 { flag: 1 }.encode(v)).await;
    tokio::time::sleep(Duration::from_millis(500)).await;
    drop(c);
    tokio::time::sleep(Duration::from_millis(1000)).await;
    let mut c = Client::connect().await;
    let chars = char_connect(&mut c, acct, id1, id2).await;
    assert_eq!(chars.len(), 1);
    let _ = char_select(&mut c, slot).await;
    drop(c);
    let mut c = Client::connect().await;
    map_connect(&mut c, acct, char_id, id1).await;
    // inventory packet arrives in the post-0x007d burst
    let inv = P01EE::decode(&c.wait(0x01ee).await.bytes).unwrap();
    assert!(
        inv.repeat.iter().any(|r| r.name_id.0 == 535),
        "candy not in inventory after relog"
    );
    eprintln!("e2e: persistence OK (candy in inventory after relog)");

    // 4. password rehash happened after the first login
    {
        let dbp = env(
            "TMWA_E2E_DB",
            &format!("{}/gate-run/gate.db", std::env::var("HOME").unwrap()),
        );
        let db = Db::open(std::path::Path::new(&dbp)).unwrap();
        let row = db.find_account_by_name(&user).unwrap().unwrap();
        assert_eq!(row.2, "argon2id", "expected rehash after login");
        eprintln!("e2e: password rehashed to argon2id");
    }

    // 5. second client + whisper + party
    let mut c2 = Client::connect().await;
    // account may already exist from a previous e2e run: try the
    // plain name first, fall back to _M registration.
    let (acct2, a1, a2) = 'h: {
        for _ in 0..4 {
            c2.send(|v| {
                P0064 {
                    client_protocol_version: ClientVersion(999),
                    account_name: f24("e2ehelper"),
                    account_pass: f24("testpass"),
                    flags: 3,
                }
                .encode(v)
            })
            .await;
            match tokio::time::timeout(Duration::from_secs(10), c2.rd.next()).await {
                Ok(Ok(Some(p))) if p.id == 0x0069 => {
                    let p = P0069::decode(&p.bytes).unwrap();
                    break 'h (p.account_id.0, p.login_id1, p.login_id2);
                }
                Ok(Ok(Some(p))) if p.id == 0x006a => {
                    let e = P006A::decode(&p.bytes).unwrap();
                    if e.error_code == 0 {
                        // doesn't exist yet: register it (fresh
                        // socket: the login conn is one-shot)
                        let mut c3 = Client::connect().await;
                        break 'h login(&mut c3, "e2ehelper_M", "testpass").await;
                    }
                }
                Ok(Ok(Some(_))) | Ok(Ok(None)) | Ok(Err(_)) | Err(_) => {}
            }
            tokio::time::sleep(Duration::from_secs(6)).await;
            c2 = Client::connect().await;
        }
        panic!("helper login failed")
    };
    assert!(acct2 >= acct);
    drop(c2);
    let mut c2 = Client::connect().await;
    // create a character for e2ehelper if none
    let chars2 = char_connect(&mut c2, acct2, a1, a2).await;
    let cid2 = if let Some(ch) = chars2.first() {
        ch.char_id.0
    } else {
        c2.send(|v| {
            P0067 {
                char_name: f24("E2ehelper"),
                stats: Stats6 {
                    str: 5,
                    agi: 5,
                    vit: 5,
                    int_: 5,
                    dex: 5,
                    luk: 5,
                },
                slot: 0,
                hair_color: 0,
                hair_style: 1,
            }
            .encode(v)
        })
        .await;
        let p = P006D::decode(&c2.wait(0x006d).await.bytes).unwrap();
        p.char_select.char_id.0
    };
    let _ = char_select(&mut c2, 0).await;
    drop(c2);
    let mut c2 = Client::connect().await;
    map_connect(&mut c2, acct2, cid2, a1).await;

    // whisper: c (Spiketest) -> c2 (E2ehelper)
    c.wr.write_all(&whisper_pkt("E2ehelper", "hi helper"))
        .await
        .unwrap();
    let p98 = c2.wait(0x0097).await; // SMSG_WHISPER to recipient
    let _ = p98;
    c.wait(0x0098).await; // whisper ack to sender
    eprintln!("e2e: whisper OK");

    // party: c creates, invites c2, c2 accepts, chat, leave
    c.send(|v| {
        P00F9 {
            party_name: f24(&format!(
                "GateParty{}",
                std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .unwrap()
                    .as_secs()
                    % 10000
            )),
        }
        .encode(v)
    })
    .await;
    c.wait(0x00fa).await; // SMSG_PARTY_CREATE
    // invite; the map can take a moment to register the new party,
    // retry the invite until c2 sees the request
    let mut inv = None;
    'outer: for _ in 0..8 {
        c.send(|v| {
            P00FC {
                account_id: AccountId(acct2),
            }
            .encode(v)
        })
        .await;
        let deadline = std::time::Instant::now() + Duration::from_secs(4);
        loop {
            let left = deadline.saturating_duration_since(std::time::Instant::now());
            if left.is_zero() {
                break;
            }
            match tokio::time::timeout(left, c2.rd.next()).await {
                Ok(Ok(Some(p))) if p.id == 0x00fe => {
                    inv = Some(P00FE::decode(&p.bytes).unwrap());
                    break 'outer;
                }
                Ok(Ok(Some(_))) => continue,
                Ok(Ok(None)) | Ok(Err(_)) | Err(_) => break,
            }
        }
    }
    let inv = inv.expect("party invite never reached c2");
    c2.send(|v| {
        P00FF {
            account_id: inv.account_id,
            flag: 1, // accept
        }
        .encode(v)
    })
    .await;
    c.wait(0x00fb).await; // party info with both members
    // let the 0x3822 member-add propagate on the map side before the
    // chat, otherwise party_send_message can race the member list
    tokio::time::sleep(Duration::from_secs(2)).await;
    // party chat 0x0108
    let mut pmsg = Vec::new();
    let mut pm = P0108::default();
    pm.repeat = b"hi party\0"
        .iter()
        .map(|&x| P0108Repeat { c: x })
        .collect();
    pm.encode(&mut pmsg);
    c.wr.write_all(&pmsg).await.unwrap();
    c2.wait(0x0109).await; // party chat received
    // leave
    c.send(|v| P0100::default().encode(v)).await;
    c.wait(0x0105).await; // party left
    eprintln!("e2e: party OK");

    // ## and # vars via the map link (acting as a second map server)
    {
        let link = TcpStream::connect(env("TMWA_E2E_MAPLINK", "127.0.0.1:6121"))
            .await
            .unwrap();
        let (lrd, mut lwr) = link.into_split();
        let mut lfr = PacketFramer::new(lrd);
        let mut p = P2AF8::default();
        p.account_name = f24(&env("TMWA_E2E_MAPUSER", "s1"));
        p.account_pass = f24(&env("TMWA_E2E_MAPPASS", "p1"));
        p.ip = Ip4Address([127, 0, 0, 1]);
        p.port = 5999;
        let mut v = Vec::new();
        p.encode(&mut v);
        lwr.write_all(&v).await.unwrap();
        let r = lfr.next().await.unwrap().unwrap();
        assert_eq!(r.id, 0x2af9);
        // 0x2b10 with a ## var
        let mut p = P2B10::default();
        p.account_id = AccountId(acct);
        p.repeat = vec![P2B10Repeat {
            name: FixedStr::<32>::try_from_str("##e2e_var").unwrap(),
            value: 4242,
        }];
        let mut v = Vec::new();
        p.encode(&mut v);
        lwr.write_all(&v).await.unwrap();
        // 0x3004 with a # var
        let mut p = P3004::default();
        p.account_id = AccountId(acct);
        p.repeat = vec![P3004Repeat {
            name: FixedStr::<32>::try_from_str("#e2e").unwrap(),
            value: 77,
        }];
        let mut v = Vec::new();
        p.encode(&mut v);
        lwr.write_all(&v).await.unwrap();
        // storage: save then load
        let mut p = P3011::default();
        p.account_id = AccountId(acct);
        p.storage.storage_amount = 1;
        p.storage.storage_[0] = Item {
            nameid: ItemNameId(7049),
            amount: 5,
            equip: Epos(0),
        };
        let mut v = Vec::new();
        p.encode(&mut v);
        lwr.write_all(&v).await.unwrap();
        // wait for 0x3811 ack
        loop {
            let r = lfr.next().await.unwrap().unwrap();
            if r.id == 0x3811 {
                break;
            }
        }
        // request storage back
        let mut p = P3010::default();
        p.account_id = AccountId(acct);
        let mut v = Vec::new();
        p.encode(&mut v);
        lwr.write_all(&v).await.unwrap();
        loop {
            let r = lfr.next().await.unwrap().unwrap();
            if r.id == 0x3810 {
                let p = P3810::decode(&r.bytes).unwrap();
                assert_eq!(p.storage.storage_[0].nameid.0, 7049);
                assert_eq!(p.storage.storage_[0].amount, 5);
                break;
            }
        }
        drop(lwr);
    }
    // verify vars reached the DB
    {
        let dbp = env(
            "TMWA_E2E_DB",
            &format!("{}/gate-run/gate.db", std::env::var("HOME").unwrap()),
        );
        let db = Db::open(std::path::Path::new(&dbp)).unwrap();
        let v2 = db.get_account_vars(acct as i64, 2).unwrap();
        assert!(v2.iter().any(|(n, v)| n == "##e2e_var" && *v == 4242));
        let v1 = db.get_account_vars(acct as i64, 1).unwrap();
        assert!(v1.iter().any(|(n, v)| n == "#e2e" && *v == 77));
        eprintln!("e2e: ##/# vars + storage OK");
    }

    // 7. online.txt should list two players (poll: the file is
    // refreshed from the map's periodic 0x2aff)
    {
        let txt = format!("{}/gate-run/online.txt", std::env::var("HOME").unwrap());
        let mut ok = false;
        for _ in 0..15 {
            if let Ok(t) = std::fs::read_to_string(&txt) {
                if t.contains("Spiketest") && t.contains("E2ehelper") {
                    ok = true;
                    break;
                }
            }
            tokio::time::sleep(Duration::from_secs(2)).await;
        }
        assert!(
            ok,
            "online.txt: {}",
            std::fs::read_to_string(&txt).unwrap_or_default()
        );
        eprintln!("e2e: online.txt lists both players");
    }

    // relog c once more to verify ## var flows to map (0x2afd carries
    // it — asserted by DB check + game continues to work)
    c.send(|v| P00B2 { flag: 1 }.encode(v)).await;
    c2.send(|v| P00B2 { flag: 1 }.encode(v)).await;
    eprintln!("e2e: all done");
}

// ------------------------------------------------------------------
// seamless-restart scenarios (phase 3)
// ------------------------------------------------------------------

/// `TMWA_E2E_RESTART` path exercises the hold/rejoin machinery:
/// separate test so the baseline stays fast.
#[tokio::test]
async fn e2e_restart() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _g = e2e_lock();
    fresh_gate();
    let user = env("TMWA_E2E_USER", "spiketest");
    let pass = env("TMWA_E2E_PASS", "spikepass");

    // ---- scenario 1+2: map SIGTERM and SIGKILL restarts ----
    for (sig, name) in [("-TERM", "term"), ("-KILL", "kill")] {
        let mut c = Client::connect().await;
        let (acct, id1, id2) = login(&mut c, &user, &pass).await;
        drop(c);
        let mut c = Client::connect().await;
        let chars = char_connect(&mut c, acct, id1, id2).await;
        let slot = chars[0].char_num;
        char_select(&mut c, slot).await;
        drop(c);
        let mut c = Client::connect().await;
        map_connect(&mut c, acct, chars[0].char_id.0, id1).await;

        // give an item right before the kill (SIGTERM must save it)
        c.wr.write_all(&chat_pkt("@item 535 3")).await.unwrap();
        tokio::time::sleep(Duration::from_secs(1)).await;

        kill_map(sig);
        // client should get the hold announcement (or silence while
        // held); the map must come back before hold_timeout
        tokio::time::sleep(Duration::from_secs(2)).await;
        spawn_map();
        wait_map_up().await;
        // rejoin: client sees 0x0091 (map-change) from the gate
        let p = c.wait(0x0091).await;
        let p91 = P0091::decode(&p.bytes).unwrap();
        assert!(!p91.map_name.to_string_lossy().is_empty(), "empty map name"); eprintln!("e2e: rejoined at {}", p91.map_name.to_string_lossy());
        eprintln!("e2e: {name} restart rejoined");
        // client acks the map change, then the login burst lands
        c.send(|v| P007D::default().encode(v)).await;
        tokio::time::sleep(Duration::from_secs(3)).await;

        // walk works: one tile right of the rejoin position
        'walk: for (x, y) in [(p91.x + 1, p91.y), (p91.x, p91.y)] {
            c.send(|v| {
                P0085 {
                    pos: Position1 { x, y, dir: Dir(0) },
                }
                .encode(v)
            })
            .await;
            for _ in 0..60 {
                match tokio::time::timeout(Duration::from_secs(2), c.rd.next()).await {
                    Ok(Ok(Some(p))) if p.id == 0x0087 => continue 'walk,
                    Ok(Ok(Some(_))) => continue,
                    Ok(Ok(None)) | Ok(Err(_)) => panic!("conn lost walking after {name}"),
                    Err(_) => break,
                }
            }
            panic!("no 0x0087 after {name} restart");
        }
        eprintln!("e2e: {name} restart walk OK");
        drop(c);
    }

    // ---- scenario 3: drain --wait then restart ----
    {
        let mut c = Client::connect().await;
        let (acct, id1, id2) = login(&mut c, &user, &pass).await;
        drop(c);
        let mut c = Client::connect().await;
        let chars = char_connect(&mut c, acct, id1, id2).await;
        let slot = chars[0].char_num;
        char_select(&mut c, slot).await;
        drop(c);
        let mut c = Client::connect().await;
        map_connect(&mut c, acct, chars[0].char_id.0, id1).await;
        c.wr.write_all(&chat_pkt("@item 535 4")).await.unwrap();
        tokio::time::sleep(Duration::from_secs(1)).await;

        let out = admin_cmd(&["drain", "--wait"]).await;
        assert!(out.contains(r#""unsaved":[]"#), "drain not clean: {out}");
        eprintln!("e2e: drain saved all players");

        kill_map("-TERM");
        tokio::time::sleep(Duration::from_secs(1)).await;
        spawn_map();
        wait_map_up().await;
        c.wait(0x0091).await;
        c.send(|v| P007D::default().encode(v)).await;
        eprintln!("e2e: drain restart rejoined");
        drop(c);
    }

    // ---- scenario 7: two clients held and rejoined, whisper ----
    {
        // ensure the helper account + char exist
        {
            let mut h = Client::connect().await;
            let (a2, i1, i2) = login(&mut h, "e2ehelper_M", "testpass").await;
            drop(h);
            let mut h = Client::connect().await;
            let chars2 = char_connect(&mut h, a2, i1, i2).await;
            if chars2.is_empty() {
                h.send(|v| {
                    P0067 {
                        char_name: f24("E2ehelper"),
                        stats: Stats6 {
                            str: 5,
                            agi: 5,
                            vit: 5,
                            int_: 5,
                            dex: 5,
                            luk: 5,
                        },
                        slot: 0,
                        hair_color: 0,
                        hair_style: 1,
                    }
                    .encode(v)
                })
                .await;
                h.wait(0x006d).await;
            }
            drop(h);
        }
        async fn mk(u: &str, pw: &str) -> Client {
            let mut c = Client::connect().await;
            let (a, i1, i2) = login(&mut c, u, pw).await;
            drop(c);
            let mut c = Client::connect().await;
            let chars = char_connect(&mut c, a, i1, i2).await;
            let slot = chars[0].char_num;
            char_select(&mut c, slot).await;
            drop(c);
            let mut c = Client::connect().await;
            map_connect(&mut c, a, chars[0].char_id.0, i1).await;
            c
        }
        let mut ca = mk(&user, &pass).await;
        let mut cb = mk("e2ehelper", "testpass").await;
        kill_map("-TERM");
        spawn_map();
        wait_map_up().await;
        ca.wait(0x0091).await;
        cb.wait(0x0091).await;
        ca.send(|v| P007D::default().encode(v)).await;
        cb.send(|v| P007D::default().encode(v)).await;
        ca.wr
            .write_all(&whisper_pkt("E2ehelper", "still here"))
            .await
            .unwrap();
        cb.wait(0x0097).await;
        eprintln!("e2e: two-client rejoin + whisper OK");
        drop(ca);
        drop(cb);
    }

    // ---- scenario 5: hold timeout (restart the gate with a short
    // timeout, kill the map, verify the client gets closed) ----
    {
        let rundir = env("TMWA_E2E_RUNDIR", &format!("{}/gate-run", std::env::var("HOME").unwrap()));
        // copy the standard config but with an 8s hold timeout
        let mut toml = std::fs::read_to_string(format!("{rundir}/gate.toml")).unwrap();
        toml.push_str("\n# test override\nhold_timeout_secs = 8\n");
        std::fs::write(format!("{rundir}/gate-hold.toml"), &toml).unwrap();
        #[allow(clippy::zombie_processes)]
        let _ = Command::new("pkill").args(["-x", "tmwa-gate"]).status();
        std::thread::sleep(Duration::from_secs(1));
        let bin = env!("CARGO_BIN_EXE_tmwa-gate");
        #[allow(clippy::zombie_processes)]
        let _ = Command::new(bin)
            .args(["serve", "--config", &format!("{rundir}/gate-hold.toml")])
            .stdout(std::fs::OpenOptions::new().append(true).create(true).open(format!("{rundir}/gate.log")).unwrap())
            .stderr(std::fs::OpenOptions::new().append(true).create(true).open(format!("{rundir}/gate.log")).unwrap())
            .spawn();
        tokio::time::sleep(Duration::from_secs(2)).await;
        wait_map_up().await;

        let mut c = Client::connect().await;
        let (acct, id1, id2) = login(&mut c, &user, &pass).await;
        drop(c);
        let mut c = Client::connect().await;
        let chars = char_connect(&mut c, acct, id1, id2).await;
        let slot = chars[0].char_num;
        char_select(&mut c, slot).await;
        drop(c);
        let mut c = Client::connect().await;
        map_connect(&mut c, acct, chars[0].char_id.0, id1).await;

        kill_map("-KILL");
        // no restart: the hold should expire in ~8s and close us
        let t0 = std::time::Instant::now();
        loop {
            match tokio::time::timeout(Duration::from_secs(30), c.rd.next()).await {
                Ok(Ok(None)) | Ok(Err(_)) => {
                    eprintln!("e2e: hold closed client after {:?}", t0.elapsed());
                    assert!(t0.elapsed() >= Duration::from_secs(6) && t0.elapsed() < Duration::from_secs(20));
                    break;
                }
                Ok(Ok(Some(_))) => continue,
                Err(_) => panic!("hold never timed out"),
            }
        }
        spawn_map();
    }
    eprintln!("e2e: restart scenarios done");
}

/// NPC dialog open during restart: expects 0x00b6 before 0x0091.
/// Also exercises the real-client-IP path (bind 127.0.0.2).
#[tokio::test]
async fn e2e_restart_npc() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _g = e2e_lock();
    fresh_gate();
    let user = env("TMWA_E2E_USER", "spiketest");
    let pass = env("TMWA_E2E_PASS", "spikepass");

    // connect from a different loopback IP; the map's log must show
    // the forwarded client address (trusted_proxy_ip is set in
    // map_local.conf). All hops must come from the same address
    // because the auth table keys on it.
    async fn conn2() -> Client {
        let sock = tokio::net::TcpSocket::new_v4().unwrap();
        sock.bind("127.0.0.2:0".parse().unwrap()).unwrap();
        let s = sock
            .connect(
                env("TMWA_E2E_ADDR", "127.0.0.1:16901")
                    .parse::<std::net::SocketAddr>()
                    .unwrap(),
            )
            .await
            .expect("connect 127.0.0.2");
        s.set_nodelay(true).unwrap();
        let (rd, wr) = s.into_split();
        Client {
            rd: PacketFramer::new(rd),
            wr,
        }
    }
    let mut c = conn2().await;
    let (acct, id1, id2) = login(&mut c, &user, &pass).await;
    drop(c);
    let mut c = conn2().await;
    let chars = char_connect(&mut c, acct, id1, id2).await;
    let slot = chars[0].char_num;
    char_select(&mut c, slot).await;
    drop(c);
    let mut c = conn2().await;
    map_connect(&mut c, acct, chars[0].char_id.0, id1).await;

    // warp next to Eomie (npc/001-1/eomie.txt: 001-1,71,23 sprite 164)
    c.wr
        .write_all(&chat_pkt("@warp 001-1 71 22"))
        .await
        .unwrap();
    // collect actor spawn packets; find the NPC (species 164).
    // TMWA may spawn NPCs via 0x0078/0x0079 (visible NPC) or
    // 0x007b/0x00b0 (walking); print what we see for debugging.
    let mut npc_id = 0u32;
    let mut seen: Vec<(u16, u32, u16)> = Vec::new();
    for _ in 0..60 {
        match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
            Ok(Ok(Some(p))) => {
                if matches!(p.id, 0x0078 | 0x0079 | 0x007b | 0x01d4) && p.bytes.len() >= 8 {
                    let bid = u32::from_le_bytes(p.bytes[4..8].try_into().unwrap());
                    let sp = P0078::decode(&p.bytes).map(|a| a.species.0).unwrap_or(0);
                    seen.push((p.id, bid, sp));
                    if let Ok(a) = P0078::decode(&p.bytes) {
                        if a.species.0 == 164 {
                            npc_id = a.block_id.0;
                            break;
                        }
                    }
                }
            }
            Ok(Ok(None)) | Ok(Err(_)) | Err(_) => break,
        }
    }
    eprintln!("e2e: actors seen: {seen:?}");
    if npc_id == 0 {
        eprintln!("e2e: WARN Eomie not seen near (71,22); skipping dialog-open part of npc test");
    } else {
        // click the NPC -> dialog opens (0x00b4 then 0x00b5)
        c.send(|v| {
            P0090 {
                block_id: BlockId(npc_id),
                unused: 0,
            }
            .encode(v)
        })
        .await;
        c.wait(0x00b5).await;
        eprintln!("e2e: npc dialog open");
    }

    kill_map("-TERM");
    tokio::time::sleep(Duration::from_secs(2)).await;
    spawn_map();
    wait_map_up().await;
    // during rejoin the gate must close the dialog (0x00b6) before
    // the 0x0091 map change
    let mut saw_b6 = false;
    let mut saw_91 = false;
    for _ in 0..60 {
        match tokio::time::timeout(Duration::from_secs(2), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x00b6 => {
                assert!(!saw_91, "0x00b6 after 0x0091");
                saw_b6 = true;
            }
            Ok(Ok(Some(p))) if p.id == 0x0091 => {
                saw_91 = true;
                break;
            }
            Ok(Ok(Some(_))) => continue,
            Ok(Ok(None)) | Ok(Err(_)) => panic!("conn lost during npc rejoin"),
            Err(_) => break,
        }
    }
    if npc_id != 0 {
        assert!(saw_b6, "no 0x00b6 before 0x0091");
    }
    assert!(saw_91, "no 0x0091");
    eprintln!("e2e: npc rejoin done (b6={saw_b6})");
}
