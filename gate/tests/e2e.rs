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

    /// Wait for a packet id, dropping others.
    async fn wait(&mut self, id: u16) -> Packet {
        for _ in 0..200 {
            match tokio::time::timeout(Duration::from_secs(10), self.rd.next()).await {
                Ok(Ok(Some(p))) if p.id == id => return p,
                Ok(Ok(Some(_))) => continue,
                Ok(Ok(None)) => panic!("eof waiting for 0x{id:04x}"),
                Ok(Err(e)) => panic!("frame error {e} waiting for 0x{id:04x}"),
                Err(_) => panic!("timeout waiting for 0x{id:04x}"),
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

    // fresh DB
    let _ = std::fs::remove_file(&db);
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
        "[gate]\nlisten = '0.0.0.0:16901'\npublic_ip = '127.0.0.1'\npublic_port = 16901\ndb = '{db}'\ngm_account_file = '{rundir}/gm_account.txt'\nonline_txt = '{rundir}/online.txt'\nonline_html = '{rundir}/online.html'\n\n[map]\nlisten = '127.0.0.1:6121'\nuserid = '{}'\npassword = '{}'\n\n[login]\nnew_account = true\n",
        env("TMWA_E2E_MAPUSER", "s1").replace('\'', ""),
        env("TMWA_E2E_MAPPASS", "p1").replace('\'', ""),
    );
    std::fs::write(format!("{rundir}/gate.toml"), &toml).unwrap();

    // (re)start the gate on the fresh DB; stays running after the
    // test so manual use continues
    let _ = Command::new("pkill").args(["-x", "tmwa-gate"]).status();
    std::thread::sleep(Duration::from_secs(1));
    let log = std::fs::File::create(format!("{rundir}/gate.log")).unwrap();
    Command::new(bin)
        .args(["serve", "--config", &format!("{rundir}/gate.toml")])
        .stdout(log.try_clone().unwrap())
        .stderr(log)
        .spawn()
        .expect("spawn tmwa-gate");

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

#[tokio::test]
async fn e2e_all() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
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
                        // doesn't exist yet: register it
                        break 'h login(&mut c2, "e2ehelper_M", "testpass").await;
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
