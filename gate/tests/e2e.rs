//! End-to-end test against a live tmwa-map behind tmwa-gate.
//!
//! Disabled unless TMWA_E2E=1; connection details come from env:
#![allow(clippy::field_reassign_with_default)]
#![allow(clippy::collapsible_if)]
//!   TMWA_E2E_ADDR   client listen addr (default 127.0.0.1:16911)
//!   TMWA_E2E_MAPLINK map-link listen addr (default 127.0.0.1:6131)
//!   TMWA_E2E_HTTP   http/websocket listen addr (default 127.0.0.1:8081)
//!   TMWA_E2E_MAPPORT  tmwa-map client port (default 5122)
//!   TMWA_E2E_USER / TMWA_E2E_PASS  account (default spiketest/spikepass)
//!   TMWA_E2E_MAPUSER/_MAPPASS    map-link auth (default s1/p1)
//!   TMWA_E2E_SAVE_DIR save dir holding athena.txt/party.txt/...
//!                    (default ~/projects/tmw/serverdata/world/save)
//!   TMWA_E2E_ACCOUNT_TXT  account.txt path
//!                    (default ~/projects/tmw/serverdata/login/save)
//!   TMWA_E2E_RUNDIR  gate runtime dir for the fresh DB/config/logs
//!                    (default $TMPDIR/tmwa-gate-e2e-<pid>-<n>)
//!   TMWA_E2E_MAPBIN  tmwa-map binary (default ~/tmw-test/prefix/bin)
//!   TMWA_E2E_MAPDIR  tmwa-map working dir holding conf/, npc/, db/
//!                    (default ~/projects/tmw/serverdata/world/map)
//!
//! The tests run their own `tmwa-gate serve` + `tmwa-map` on ports
//! distinct from the defaults, in a fresh rundir per test holding
//! every writable file (config, DB, logs, socket, mapreg/gm log).
//! Spawned processes are tracked by Child handle and killed on drop;
//! nothing touches a stack started outside the test.

use std::path::{Path, PathBuf};
use std::process::{Child, Command};
use std::time::Duration;

use tokio::io::AsyncWriteExt;
use tokio::net::TcpStream;
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};

use tmwa_gate::db::Db;
use tmwa_gate::net::framing::{Packet, PacketFramer};
use tmwa_gate::proto::types::*;
use tmwa_gate::proto::*;

fn env(k: &str, d: &str) -> String {
    std::env::var(k).unwrap_or_else(|_| d.into())
}

/// Client listen addr the gate binds and clients connect to.
fn gate_addr() -> std::net::SocketAddr {
    env("TMWA_E2E_ADDR", "127.0.0.1:16911").parse().unwrap()
}
/// Map-link listen addr: the gate binds it, tmwa-map connects to it.
fn maplink_addr() -> std::net::SocketAddr {
    env("TMWA_E2E_MAPLINK", "127.0.0.1:6131").parse().unwrap()
}
/// HTTP/WS listen addr of the gate.
fn http_addr() -> std::net::SocketAddr {
    env("TMWA_E2E_HTTP", "127.0.0.1:8081").parse().unwrap()
}
/// Client-facing port tmwa-map listens on and advertises in 0x2af8.
fn map_port() -> u16 {
    env("TMWA_E2E_MAPPORT", "5122").parse().unwrap()
}
fn map_user() -> String {
    env("TMWA_E2E_MAPUSER", "s1").replace('\'', "")
}
fn map_pass() -> String {
    env("TMWA_E2E_MAPPASS", "p1").replace('\'', "")
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
        let s = TcpStream::connect(gate_addr())
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

/// Login, registering `<name>_M` if the account doesn't exist yet.
async fn login_or_register(c: &mut Client, name: &str, pass: &str) -> (u32, u32, u32) {
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
            Ok(Ok(Some(p))) if p.id == 0x006a => {
                let e = P006A::decode(&p.bytes).unwrap();
                if e.error_code == 0 {
                    let mut c3 = Client::connect().await;
                    return login(&mut c3, &format!("{name}_M"), pass).await;
                }
            }
            _ => {}
        }
        tokio::time::sleep(Duration::from_secs(6)).await;
        *c = Client::connect().await;
    }
    panic!("helper login failed")
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

/// Everything one e2e run owns: a rundir holding every writable
/// file plus the spawned gate/map processes, tracked by handle and
/// killed on drop (also on panic unwind).
struct E2e {
    rundir: PathBuf,
    mapdir: PathBuf,
    db: PathBuf,
    gate: Option<Child>,
    map: Option<Child>,
}

impl E2e {
    fn gate_conf(&self) -> PathBuf {
        self.rundir.join("gate.toml")
    }
    fn db_path(&self) -> PathBuf {
        // TMWA_E2E_DB is an escape hatch; normally the rundir DB.
        match std::env::var("TMWA_E2E_DB") {
            Ok(p) => PathBuf::from(p),
            Err(_) => self.db.clone(),
        }
    }
    fn gate_log(&self) -> PathBuf {
        self.rundir.join("gate.log")
    }
    fn map_stdout(&self) -> PathBuf {
        self.rundir.join("map.stdout.log")
    }
    fn map_stderr(&self) -> PathBuf {
        self.rundir.join("map.stderr.log")
    }
    fn socket(&self) -> PathBuf {
        self.rundir.join("gate.sock")
    }
    /// pid of the tracked tmwa-map child, if it is running.
    fn map_pid(&self) -> Option<u32> {
        self.map.as_ref().map(|c| c.id())
    }

    /// Spawn `tmwa-gate serve` on the given rundir config.
    fn spawn_gate(&mut self, conf: &str) {
        self.kill_gate();
        let log = std::fs::OpenOptions::new()
            .append(true)
            .create(true)
            .open(self.gate_log())
            .unwrap();
        let child = Command::new(env!("CARGO_BIN_EXE_tmwa-gate"))
            .args(["serve", "--config"])
            .arg(self.rundir.join(conf))
            .env("RUST_LOG", "debug")
            .stdout(log.try_clone().unwrap())
            .stderr(log)
            .spawn()
            .expect("spawn tmwa-gate");
        self.gate = Some(child);
    }

    fn kill_gate(&mut self) {
        if let Some(mut c) = self.gate.take() {
            let _ = c.kill();
            let _ = c.wait();
        }
    }

    /// Spawn tmwa-map with the rundir master conf as argv (cwd =
    /// mapdir so the cwd-relative conf/, npc/, db/ paths resolve).
    fn spawn_map(&mut self) {
        self.spawn_map_env(&[]);
    }
    fn spawn_map_env(&mut self, extra_env: &[(&str, &str)]) {
        self.reap_map();
        let mut cmd = Command::new(env(
            "TMWA_E2E_MAPBIN",
            &format!(
                "{}/tmw-test/prefix/bin/tmwa-map",
                std::env::var("HOME").unwrap()
            ),
        ));
        for (k, v) in extra_env {
            cmd.env(k, v);
        }
        cmd.current_dir(&self.mapdir)
            .arg(self.rundir.join("e2e-tmwa-map.conf"))
            .stdout(append_log(&self.map_stdout()))
            .stderr(append_log(&self.map_stderr()));
        self.map = Some(cmd.spawn().expect("spawn tmwa-map"));
    }

    /// Signal the tracked map child and reap it.
    fn kill_map(&mut self, sig: &str) {
        if let Some(mut c) = self.map.take() {
            let _ = Command::new("kill")
                .arg(sig)
                .arg(c.id().to_string())
                .status();
            let _ = c.wait();
        }
    }

    /// Reap a map child that died on its own (no signal sent).
    fn reap_map(&mut self) {
        if let Some(mut c) = self.map.take() {
            let _ = c.kill();
            let _ = c.wait();
        }
    }

    /// Restart the gate with `hold_timeout_secs = <secs>` injected
    /// into a copy of gate.toml; the map reconnects on its own.
    fn restart_gate_hold_timeout(&mut self, secs: u64) {
        let toml = std::fs::read_to_string(self.gate_conf()).unwrap();
        let toml = toml.replace("\n[map]", &format!("\nhold_timeout_secs = {secs}\n\n[map]"));
        std::fs::write(self.rundir.join("gate-hold.toml"), &toml).unwrap();
        self.spawn_gate("gate-hold.toml");
    }

    fn admin_cmd(&self, args: &[&str]) -> String {
        let out = Command::new(env!("CARGO_BIN_EXE_tmwa-gate"))
            .arg("admin")
            .arg("--socket")
            .arg(self.socket())
            .args(args)
            .output()
            .unwrap();
        String::from_utf8_lossy(&out.stdout).to_string()
    }

    /// Block until the map link reports a live map server.
    fn wait_map_up_sync(&self) {
        for _ in 0..150 {
            if self.admin_cmd(&["status"]).contains("\"id\"") {
                return;
            }
            std::thread::sleep(Duration::from_secs(1));
        }
        panic!("map server never registered");
    }

    /// Wait until the map-link admin reports a live map server.
    async fn wait_map_up(&self) {
        for _ in 0..80 {
            let st = self.admin_cmd(&["status"]);
            if st.contains("\"id\"") {
                return;
            }
            tokio::time::sleep(Duration::from_secs(1)).await;
        }
        panic!("map server never re-registered");
    }
}

impl Drop for E2e {
    fn drop(&mut self) {
        self.reap_map();
        self.kill_gate();
        // the rundir stays in $TMPDIR for post-mortem debugging
        eprintln!("e2e: rundir was {}", self.rundir.display());
    }
}

fn append_log(p: &Path) -> std::fs::File {
    std::fs::OpenOptions::new()
        .append(true)
        .create(true)
        .open(p)
        .unwrap()
}

/// Write the tmwa-map argv confs: a master conf mirroring
/// conf/tmwa-map.conf but with `map_conf:` pointed at our rundir
/// file, which imports conf/map_athena.conf and then overrides the
/// link/identity keys and every writable path. Config assignment
/// order applies (later lines win, including over the map_local.conf
/// import inside map_athena.conf), and all relative paths resolve
/// against the spawn cwd (mapdir).
fn write_map_confs(rundir: &Path, mapdir: &Path) {
    let stock = std::fs::read_to_string(mapdir.join("conf/tmwa-map.conf"))
        .expect("read conf/tmwa-map.conf");
    let mut master = String::new();
    let mut map_conf_done = false;
    for line in stock.lines() {
        let t = line.trim_start();
        if t.starts_with("map_conf:") && !t.starts_with("//") {
            // first map_conf becomes ours; a second one would
            // re-import map_athena.conf and double-load NPCs
            if !map_conf_done {
                master.push_str(&format!(
                    "map_conf: {}\n",
                    rundir.join("e2e-map.conf").display()
                ));
                map_conf_done = true;
            }
            continue;
        }
        master.push_str(line);
        master.push('\n');
    }
    assert!(map_conf_done, "conf/tmwa-map.conf has no map_conf line");
    std::fs::write(rundir.join("e2e-tmwa-map.conf"), master).unwrap();

    let maplink = maplink_addr();
    let char_ip = if maplink.ip().is_unspecified() {
        std::net::Ipv4Addr::LOCALHOST.to_string()
    } else {
        maplink.ip().to_string()
    };
    let conf = format!(
        "// e2e map conf: stock world config first (pulls in\n\
         // conf/map_local.conf), then the e2e overrides win.\n\
         import: conf/map_athena.conf\n\
         userid: {user}\n\
         passwd: {pass}\n\
         char_ip: {char_ip}\n\
         char_port: {char_port}\n\
         map_ip: 127.0.0.1\n\
         map_port: {map_port}\n\
         trusted_proxy_ip: 127.0.0.1\n\
         mapreg_txt: {rundir}/mapreg.txt\n\
         gm_log: {rundir}/gm.log\n\
         log_file: {rundir}/map.log\n",
        user = map_user(),
        pass = map_pass(),
        char_port = maplink.port(),
        map_port = map_port(),
        rundir = rundir.display(),
    );
    std::fs::write(rundir.join("e2e-map.conf"), conf).unwrap();
}

/// Import a fresh DB from the save fixtures, write all config into
/// a fresh rundir and spawn this test's own gate + map there.
fn fresh_gate() -> E2e {
    let home = std::env::var("HOME").unwrap();
    static RUN: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);
    let rundir = match std::env::var("TMWA_E2E_RUNDIR") {
        Ok(d) => PathBuf::from(d),
        Err(_) => std::env::temp_dir().join(format!(
            "tmwa-gate-e2e-{}-{}",
            std::process::id(),
            RUN.fetch_add(1, std::sync::atomic::Ordering::SeqCst)
        )),
    };
    let mapdir = PathBuf::from(env(
        "TMWA_E2E_MAPDIR",
        &format!("{home}/projects/tmw/serverdata/world/map"),
    ));
    let account_txt = env(
        "TMWA_E2E_ACCOUNT_TXT",
        &format!("{home}/projects/tmw/serverdata/login/save/account.txt"),
    );
    let save_dir = env(
        "TMWA_E2E_SAVE_DIR",
        &format!("{home}/projects/tmw/serverdata/world/save"),
    );
    // Only wipe a rundir the tests created before (or a fresh one):
    // TMWA_E2E_RUNDIR=~/gate-run must not delete the live runtime dir.
    let marker = rundir.join(".e2e-rundir");
    if rundir.exists() && !marker.exists() {
        let empty = std::fs::read_dir(&rundir)
            .map(|mut d| d.next().is_none())
            .unwrap_or(false);
        assert!(
            empty,
            "TMWA_E2E_RUNDIR {} is not empty and has no .e2e-rundir marker; refusing to reuse it",
            rundir.display()
        );
    }
    let _ = std::fs::remove_dir_all(&rundir);
    std::fs::create_dir_all(&rundir).unwrap();
    std::fs::write(&marker, b"").unwrap();
    let db = rundir.join("gate.db");
    let bin = env!("CARGO_BIN_EXE_tmwa-gate");

    // fresh DB (including WAL sidecars, or the new file replays
    // the old journal)
    let _ = std::fs::remove_file(&db);
    let _ = std::fs::remove_file(format!("{}-wal", db.display()));
    let _ = std::fs::remove_file(format!("{}-shm", db.display()));
    let out = Command::new(bin)
        .arg("import")
        .arg("--db")
        .arg(&db)
        .arg("--save-dir")
        .arg(&save_dir)
        .arg("--account-txt")
        .arg(&account_txt)
        .output()
        .expect("tmwa-gate import");
    eprintln!("import: {}", String::from_utf8_lossy(&out.stdout));
    assert!(out.status.success(), "import failed: {:?}", out);

    // GM levels for the fixture accounts (same dir as account.txt);
    // fall back to granting the first imported account full GM.
    let gm = Path::new(&account_txt)
        .parent()
        .unwrap()
        .join("gm_account.txt");
    let gm_txt = std::fs::read_to_string(&gm).unwrap_or_else(|_| "2000000 99\n".into());
    std::fs::write(rundir.join("gm_account.txt"), gm_txt).unwrap();

    // gate config: everything writable lives in the rundir, and the
    // three listen ports differ from a live stack's defaults.
    let addr = gate_addr();
    let maplink = maplink_addr();
    let toml = format!(
        "[gate]\nlisten = '{addr}'\npublic_ip = '{ip}'\npublic_port = {port}\ndb = '{db}'\ngm_account_file = '{rundir}/gm_account.txt'\nonline_txt = '{rundir}/online.txt'\nonline_html = '{rundir}/online.html'\nadmin_socket = '{rundir}/gate.sock'\n\n[map]\nlisten = '{maplink}'\nuserid = '{muser}'\npassword = '{mpass}'\n\n[login]\nnew_account = true\n\n[char]\nserver_name = 'The Mana World'\nstart_point = '001-1.gat,32,23'\nchar_name_letters = [\"$ &\'()*+,-.\", \"0123456789\", \";<=>?\", \"ABCDEFGHIJKLMNOPRSTQUVWXYZ\", \"\\\\^_`\", \"abcdefghijklmnoprstquvwxyz\"]\n\n[http]\nlisten = '{http}'\nws_path = '/tmwa'\ncaptcha = false\n",
        ip = addr.ip(),
        port = addr.port(),
        db = db.display(),
        rundir = rundir.display(),
        muser = map_user(),
        mpass = map_pass(),
        http = http_addr(),
    );
    std::fs::write(rundir.join("gate.toml"), &toml).unwrap();
    write_map_confs(&rundir, &mapdir);

    let mut fx = E2e {
        rundir,
        mapdir,
        db,
        gate: None,
        map: None,
    };
    // the gate first so the map's connect succeeds on its first try;
    // the map takes tens of seconds to load its world before it
    // even dials the link, so startup order is not load-bearing.
    fx.spawn_gate("gate.toml");
    fx.spawn_map();
    eprintln!("e2e: rundir {}", fx.rundir.display());

    // wait for the client port, then for the map to register:
    // returning earlier races tests whose char stage has no
    // no-map retry (e.g. the ws flow)
    for _ in 0..100 {
        if std::net::TcpStream::connect(addr).is_ok() {
            break;
        }
        std::thread::sleep(Duration::from_millis(200));
    }
    fx.wait_map_up_sync();
    fx
}

#[tokio::test]
#[allow(clippy::await_holding_lock)]
async fn e2e_all() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _e2e_guard = e2e_lock();
    let fx = fresh_gate();
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
    assert_eq!(sel.port as u16, gate_addr().port());
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
        let db = Db::open(&fx.db_path()).unwrap();
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
    // the map binds the new member's session via its own ordering of
    // 0x3822/0x3821/0x3825 — retry the chat until it lands
    let mut pmsg = Vec::new();
    let mut pm = P0108::default();
    pm.repeat = b"hi party\0"
        .iter()
        .map(|&x| P0108Repeat { c: x })
        .collect();
    pm.encode(&mut pmsg);
    let mut got_chat = false;
    for _ in 0..6 {
        c.wr.write_all(&pmsg).await.unwrap();
        match tokio::time::timeout(Duration::from_secs(10), c2.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x0109 => {
                got_chat = true;
                break;
            }
            Ok(Ok(Some(_))) => continue,
            _ => continue,
        }
    }
    assert!(got_chat, "party chat never delivered");
    // leave
    c.send(|v| P0100::default().encode(v)).await;
    c.wait(0x0105).await; // party left
    eprintln!("e2e: party OK");

    // ## and # vars via the map link (acting as a second map server)
    {
        let link = TcpStream::connect(maplink_addr()).await.unwrap();
        let (lrd, mut lwr) = link.into_split();
        let mut lfr = PacketFramer::new(lrd);
        let mut p = P2AF8::default();
        p.account_name = f24(&map_user());
        p.account_pass = f24(&map_pass());
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
        let db = Db::open(&fx.db_path()).unwrap();
        let v2 = db.get_account_vars(acct as i64, 2).unwrap();
        assert!(v2.iter().any(|(n, v)| n == "##e2e_var" && *v == 4242));
        let v1 = db.get_account_vars(acct as i64, 1).unwrap();
        assert!(v1.iter().any(|(n, v)| n == "#e2e" && *v == 77));
        eprintln!("e2e: ##/# vars + storage OK");
    }

    // 7. online.txt should list two players (poll: the file is
    // refreshed from the map's periodic 0x2aff)
    {
        let txt = fx.rundir.join("online.txt");
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
#[allow(clippy::await_holding_lock)]
async fn e2e_restart() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _e2e_guard = e2e_lock();
    let mut fx = fresh_gate();
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
        let t_kill = std::time::Instant::now();

        fx.kill_map(sig);
        // SIGTERM: the map's shutdown notice reaches the gate before
        // the client sockets close, so the hold announcement must
        // land almost at once. SIGKILL goes through the 3 s grace.
        if sig == "-TERM" {
            let t0 = std::time::Instant::now();
            let mut announced = false;
            while t0.elapsed() < Duration::from_secs(3) {
                match tokio::time::timeout(Duration::from_millis(300), c.rd.next()).await {
                    Ok(Ok(Some(p))) if p.id == 0x009a => {
                        announced = true;
                        break;
                    }
                    Ok(Ok(Some(_))) => continue,
                    _ => break,
                }
            }
            assert!(announced, "no hold announcement after map SIGTERM");
            eprintln!("e2e: term hold announced in {}ms", t0.elapsed().as_millis());
        }
        // the map must come back before hold_timeout
        tokio::time::sleep(Duration::from_secs(2)).await;
        fx.spawn_map();
        fx.wait_map_up().await;
        // rejoin: client sees 0x0091 (map-change) from the gate
        let p = c.wait(0x0091).await;
        let p91 = P0091::decode(&p.bytes).unwrap();
        assert!(!p91.map_name.to_string_lossy().is_empty(), "empty map name");
        eprintln!("e2e: rejoined at {}", p91.map_name.to_string_lossy());
        eprintln!(
            "e2e: {name} restart rejoined ({}ms from kill)",
            t_kill.elapsed().as_millis()
        );
        // client acks the map change, then the login burst lands;
        // the 0x01ee inventory must still carry the candy for the
        // SIGTERM case (the shutdown saved it)
        c.send(|v| P007D::default().encode(v)).await;
        if sig == "-TERM" {
            let mut saved = false;
            for _ in 0..40 {
                match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
                    Ok(Ok(Some(p))) if p.id == 0x01ee => {
                        if let Ok(inv) = P01EE::decode(&p.bytes) {
                            saved = inv.repeat.iter().any(|r| r.name_id.0 == 535);
                            if saved {
                                break;
                            }
                        }
                    }
                    Ok(Ok(Some(_))) => continue,
                    _ => break,
                }
            }
            assert!(saved, "item lost across SIGTERM restart");
        } else {
            tokio::time::sleep(Duration::from_secs(3)).await;
        }

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

    // ---- scenario 1b: warp then SIGTERM — the rejoin must use the
    // newly saved map, not the stale login-time one ----
    {
        let mut c = Client::connect().await;
        let (acct, id1, id2) = login(&mut c, &user, &pass).await;
        drop(c);
        let mut c = Client::connect().await;
        let chars = char_connect(&mut c, acct, id1, id2).await;
        let slot = chars[0].char_num;
        let sel = char_select(&mut c, slot).await;
        let _ = sel; // 0x0071 contents don't matter; compare 0x0091s
        drop(c);
        let mut c = Client::connect().await;
        map_connect(&mut c, acct, chars[0].char_id.0, id1).await;

        // warp to a different map; the shutdown save must persist it
        c.wr.write_all(&chat_pkt("@warp 029-2 22 24"))
            .await
            .unwrap();
        tokio::time::sleep(Duration::from_secs(3)).await;
        fx.kill_map("-TERM");
        fx.spawn_map();
        fx.wait_map_up().await;
        let p = c.wait(0x0091).await;
        let p91 = P0091::decode(&p.bytes).unwrap();
        let nm = p91.map_name.to_string_lossy();
        eprintln!("e2e: warp+term rejoin map = {nm}");
        assert!(nm.contains("029-2"), "rejoin used stale map: {nm}");
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

        let out = fx.admin_cmd(&["drain", "--wait"]);
        assert!(out.contains(r#""unsaved":[]"#), "drain not clean: {out}");
        eprintln!("e2e: drain saved all players");

        fx.kill_map("-TERM");
        tokio::time::sleep(Duration::from_secs(1)).await;
        fx.spawn_map();
        fx.wait_map_up().await;
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
        fx.kill_map("-TERM");
        fx.spawn_map();
        fx.wait_map_up().await;
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
        fx.restart_gate_hold_timeout(8);
        tokio::time::sleep(Duration::from_secs(1)).await;
        fx.wait_map_up().await;

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

        fx.kill_map("-KILL");
        // no restart: the hold should expire in ~8s and close us
        let t0 = std::time::Instant::now();
        loop {
            match tokio::time::timeout(Duration::from_secs(30), c.rd.next()).await {
                Ok(Ok(None)) | Ok(Err(_)) => {
                    eprintln!("e2e: hold closed client after {:?}", t0.elapsed());
                    assert!(
                        t0.elapsed() >= Duration::from_secs(6)
                            && t0.elapsed() < Duration::from_secs(20)
                    );
                    break;
                }
                Ok(Ok(Some(_))) => continue,
                Err(_) => panic!("hold never timed out"),
            }
        }
        fx.spawn_map();
    }
    eprintln!("e2e: restart scenarios done");
}

/// NPC dialog open during restart: expects 0x00b6 before 0x0091.
/// Also exercises the real-client-IP path (bind 127.0.0.2).
#[tokio::test]
#[allow(clippy::await_holding_lock)]
async fn e2e_restart_npc() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _e2e_guard = e2e_lock();
    let mut fx = fresh_gate();
    let user = env("TMWA_E2E_USER", "spiketest");
    let pass = env("TMWA_E2E_PASS", "spikepass");

    // connect from a different loopback IP; the map's log must show
    // the forwarded client address (trusted_proxy_ip is set in
    // map_local.conf). All hops must come from the same address
    // because the auth table keys on it.
    async fn conn2() -> Client {
        let sock = tokio::net::TcpSocket::new_v4().unwrap();
        sock.bind("127.0.0.2:0".parse().unwrap()).unwrap();
        let s = sock.connect(gate_addr()).await.expect("connect 127.0.0.2");
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

    // sanity: a normal chat echoes back (proves the map session is
    // live), then @npc warps us next to Sorfina
    // the map rate-limits client packets (~300ms min interval) —
    // pace our sends
    tokio::time::sleep(Duration::from_secs(3)).await;
    c.wr.write_all(&chat_pkt("e2e probe")).await.unwrap();
    let mut echoed = false;
    for _ in 0..40 {
        match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x008d => {
                echoed = true;
                break;
            }
            Ok(Ok(Some(_))) => continue,
            Err(_) => continue,
            _ => break,
        }
    }
    eprintln!("e2e: chat echo = {echoed}");
    tokio::time::sleep(Duration::from_millis(600)).await;
    c.wr.write_all(&chat_pkt("@npc Sorfina")).await.unwrap();
    // dump everything the map answers (0x008e display messages tell
    // us why a command was rejected)
    for _ in 0..20 {
        match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
            Ok(Ok(Some(p))) => {
                if p.id == 0x008e {
                    if let Ok(m) = P008E::decode(&p.bytes) {
                        eprintln!(
                            "  server says: {:?}",
                            String::from_utf8_lossy(
                                &m.repeat.iter().map(|c| c.c).collect::<Vec<_>>()
                            )
                        );
                    }
                }
            }
            _ => break,
        }
    }
    tokio::time::sleep(Duration::from_secs(2)).await;
    // collect actor spawn packets; find the NPC near us
    let mut npc_candidates: Vec<u32> = Vec::new();
    let mut seen: Vec<(u32, u16, u16, u16)> = Vec::new();
    for _ in 0..60 {
        match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
            Ok(Ok(Some(p))) => {
                if matches!(p.id, 0x0078 | 0x0079 | 0x007b | 0x01d4) && p.bytes.len() >= 8 {
                    if let Ok(a) = P0078::decode(&p.bytes) {
                        seen.push((a.block_id.0, a.species.0, a.pos.x, a.pos.y));
                        // NPC sprites (low ids); monsters are >=1000
                        if a.species.0 < 1000 {
                            npc_candidates.push(a.block_id.0);
                        }
                    }
                }
            }
            Ok(Ok(None)) | Ok(Err(_)) | Err(_) => break,
        }
    }
    eprintln!("e2e: actors seen: {seen:?}");
    // click each NPC-ish being until one opens a dialog
    let mut npc_id = 0u32;
    for bid in npc_candidates.clone() {
        c.send(|v| {
            P0090 {
                block_id: BlockId(bid),
                unused: 0,
            }
            .encode(v)
        })
        .await;
        match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x00b4 || p.id == 0x00b5 => {
                npc_id = bid;
                break;
            }
            Ok(Ok(Some(_))) | Err(_) => continue,
            _ => break,
        }
    }
    if npc_id == 0 {
        eprintln!("e2e: WARN Sorfina not seen near (27,26); skipping dialog-open part of npc test");
    } else {
        eprintln!("e2e: npc dialog open (block {npc_id})");
    }

    fx.kill_map("-TERM");
    tokio::time::sleep(Duration::from_secs(2)).await;
    fx.spawn_map();
    fx.wait_map_up().await;
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

    // the map must have logged the real client IP (127.0.0.2) in the
    // pre-auth line: trusted_proxy_ip lets the gate pass it through
    let mlog = std::fs::read_to_string(fx.map_stdout()).unwrap_or_default();
    assert!(
        mlog.contains("[127.0.0.2]"),
        "map never saw the real client IP"
    );
    eprintln!("e2e: real client IP forwarded (map saw 127.0.0.2)");

    // ---- hold timeout ----
    // restart the gate with an 8s hold_timeout, kill the map, and
    // don't restart it: the client must be closed
    {
        fx.restart_gate_hold_timeout(8);
        tokio::time::sleep(Duration::from_secs(1)).await;
        fx.wait_map_up().await;

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

        fx.kill_map("-KILL");
        let t0 = std::time::Instant::now();
        loop {
            match tokio::time::timeout(Duration::from_secs(30), c.rd.next()).await {
                Ok(Ok(None)) | Ok(Err(_)) => {
                    let el = t0.elapsed();
                    eprintln!("e2e: hold closed client after {el:?}");
                    assert!(el >= Duration::from_secs(6) && el < Duration::from_secs(25));
                    break;
                }
                Ok(Ok(Some(_))) => continue,
                Err(_) => panic!("hold never timed out"),
            }
        }
        fx.spawn_map();
    }
    eprintln!("e2e: npc/hold-timeout done");
}

// ---- WebSocket transport -------------------------------------------------

type WsInner = tokio_tungstenite::WebSocketStream<tokio_tungstenite::MaybeTlsStream<TcpStream>>;

struct WsClient {
    rd: PacketFramer<WsRead<futures_util::stream::SplitStream<WsInner>>>,
    wr: futures_util::stream::SplitSink<WsInner, tokio_tungstenite::tungstenite::Message>,
}

// The PacketFramer expects AsyncRead — WS is message-oriented, so the
// read side is a tiny adapter that concatenates binary frames.
struct WsRead<S> {
    s: S,
    buf: Vec<u8>,
    pos: usize,
    close_code: Option<u16>,
}

impl<S> tokio::io::AsyncRead for WsRead<S>
where
    S: futures_util::Stream<
            Item = Result<
                tokio_tungstenite::tungstenite::Message,
                tokio_tungstenite::tungstenite::Error,
            >,
        > + Unpin,
{
    fn poll_read(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
        buf: &mut tokio::io::ReadBuf<'_>,
    ) -> std::task::Poll<std::io::Result<()>> {
        loop {
            if self.pos < self.buf.len() {
                let n = buf.remaining().min(self.buf.len() - self.pos);
                buf.put_slice(&self.buf[self.pos..self.pos + n]);
                self.pos += n;
                if self.pos >= self.buf.len() {
                    self.buf.clear();
                    self.pos = 0;
                }
                return std::task::Poll::Ready(Ok(()));
            }
            match futures_util::ready!(futures_util::Stream::poll_next(
                std::pin::Pin::new(&mut self.s),
                cx
            )) {
                Some(Ok(tokio_tungstenite::tungstenite::Message::Binary(d))) => {
                    self.buf = d.to_vec();
                    self.pos = 0;
                }
                Some(Ok(_)) => continue,
                Some(Err(e)) => {
                    return std::task::Poll::Ready(Err(std::io::Error::other(e.to_string())));
                }
                None => return std::task::Poll::Ready(Ok(())),
            }
        }
    }
}

impl WsClient {
    async fn connect() -> WsClient {
        let url = format!("ws://{}/tmwa", http_addr());
        let (ws, _) =
            tokio_tungstenite::connect_async_with_config(tungstenite_url(&url), None, false)
                .await
                .expect("ws connect");
        let (wr, rd) = futures_util::StreamExt::split(ws);
        WsClient {
            rd: PacketFramer::new(WsRead {
                s: rd,
                buf: Vec::new(),
                pos: 0,
                close_code: None,
            }),
            wr,
        }
    }
}

impl<S> WsRead<S> {
    fn close_code(&self) -> Option<u16> {
        self.close_code
    }
}

fn tungstenite_url(u: &str) -> tokio_tungstenite::tungstenite::http::Uri {
    u.parse().unwrap()
}

// The ws Client wrapper shares the same send/wait helpers; to keep
// this small, do the login/char/map handshake by hand here.
#[tokio::test]
#[allow(clippy::await_holding_lock)]
async fn e2e_ws() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _g = e2e_lock();
    let mut fx = fresh_gate();

    use futures_util::SinkExt;
    let mut c = WsClient::connect().await;

    // login over WS
    let mut v = Vec::new();
    P0064 {
        client_protocol_version: ClientVersion(999),
        account_name: f24(&env("TMWA_E2E_USER", "spiketest")),
        account_pass: f24(&env("TMWA_E2E_PASS", "spikepass")),
        flags: 3,
    }
    .encode(&mut v);
    c.wr.send(tokio_tungstenite::tungstenite::Message::Binary(v.into()))
        .await
        .unwrap();
    let p = c.rd.next().await.unwrap().unwrap();
    assert_eq!(p.id, 0x0069);
    let p69 = P0069::decode(&p.bytes).unwrap();
    // the login role is one-shot: the gate must close with a normal
    // 1000 close frame, not a bare TCP-style drop (1005)
    let code = {
        // drain until the close frame; read the raw stream since
        // the framer treats the close as plain EOF
        for _ in 0..10 {
            match tokio::time::timeout(
                Duration::from_secs(5),
                futures_util::StreamExt::next(&mut c.rd.reader_mut().s),
            )
            .await
            {
                Ok(Some(Ok(tokio_tungstenite::tungstenite::Message::Close(f)))) => {
                    c.rd.reader_mut().close_code = f.map(|f| f.code.into());
                    break;
                }
                Ok(Some(Err(_))) | Ok(None) | Err(_) => break,
                Ok(Some(Ok(_))) => continue,
            }
        }
        c.rd.reader().close_code()
    };
    assert_eq!(code, Some(1000), "login ws close code {code:?}");
    drop(c);

    // char stage on a fresh WS connection
    let mut c = WsClient::connect().await;
    let mut v = Vec::new();
    P0065 {
        account_id: p69.account_id,
        login_id1: p69.login_id1,
        login_id2: p69.login_id2,
        unused_client_protocol_version: 0,
        sex: Sex(1),
    }
    .encode(&mut v);
    c.wr.send(tokio_tungstenite::tungstenite::Message::Binary(v.into()))
        .await
        .unwrap();
    let mut chars = Vec::new();
    let _ = chars;
    loop {
        let p = c.rd.next().await.unwrap().unwrap();
        if p.id == 0x006b {
            let l = P006B::decode(&p.bytes).unwrap();
            chars = l.repeat;
            break;
        }
    }
    let mut v = Vec::new();
    P0066 { code: 0 }.encode(&mut v);
    c.wr.send(tokio_tungstenite::tungstenite::Message::Binary(v.into()))
        .await
        .unwrap();
    loop {
        let p = c.rd.next().await.unwrap().unwrap();
        if p.id == 0x0071 {
            break;
        }
    }
    // the char role ends by the client disconnecting; closing the
    // ws client-side should still complete a normal 1000 handshake
    let _ =
        c.wr.send(tokio_tungstenite::tungstenite::Message::Close(Some(
            tokio_tungstenite::tungstenite::protocol::CloseFrame {
                code: tokio_tungstenite::tungstenite::protocol::frame::coding::CloseCode::Normal,
                reason: "".into(),
            },
        )))
        .await;
    for _ in 0..10 {
        match tokio::time::timeout(
            Duration::from_secs(5),
            futures_util::StreamExt::next(&mut c.rd.reader_mut().s),
        )
        .await
        {
            Ok(Some(Ok(tokio_tungstenite::tungstenite::Message::Close(f)))) => {
                c.rd.reader_mut().close_code = f.map(|f| f.code.into());
                break;
            }
            Ok(Some(Err(_))) | Ok(None) | Err(_) => break,
            Ok(Some(Ok(_))) => continue,
        }
    }
    assert_eq!(c.rd.reader().close_code(), Some(1000), "char ws close code");
    drop(c);

    // map stage on a third WS connection
    let mut c = WsClient::connect().await;
    let mut v = Vec::new();
    P0072 {
        account_id: p69.account_id,
        char_id: chars[0].char_select.char_id,
        login_id1: p69.login_id1,
        client_tick: 999999,
        sex: Sex(1),
    }
    .encode(&mut v);
    c.wr.send(tokio_tungstenite::tungstenite::Message::Binary(v.into()))
        .await
        .unwrap();
    let mut got_map = false;
    for _ in 0..40 {
        match tokio::time::timeout(Duration::from_secs(5), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x0073 || p.id == 0x0091 => {
                got_map = true;
                break;
            }
            Ok(Ok(Some(_))) => continue,
            _ => break,
        }
    }
    assert!(got_map, "ws client never reached the map");
    eprintln!("e2e: ws login+char+map OK");

    // hold/rejoin over WS: kill the map, wait for the 0x0091
    fx.kill_map("-TERM");
    fx.spawn_map();
    fx.wait_map_up().await;
    let mut rejoined = false;
    let mut idle = 0;
    for _ in 0..60 {
        match tokio::time::timeout(Duration::from_secs(3), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x0091 => {
                rejoined = true;
                break;
            }
            Ok(Ok(Some(_))) => continue,
            Err(_) => {
                idle += 1;
                if idle > 20 {
                    break;
                }
            }
            _ => break,
        }
    }
    assert!(rejoined, "ws client never rejoined");
    eprintln!("e2e: ws hold+rejoin OK");
}

/// tmwa-map over its fd softlimit must not put clients on hold:
/// the player gets 0x0081 code 1 and a close, fast. A GM @kick on
/// a live map behaves the same (minus the 0x0081: the map already
/// accepted the player and sent its own goodbye).
#[tokio::test]
#[allow(clippy::await_holding_lock)]
async fn e2e_map_full() {
    if std::env::var("TMWA_E2E").is_err() {
        return;
    }
    let _e2e_guard = e2e_lock();
    let mut fx = fresh_gate();
    let user = env("TMWA_E2E_USER", "spiketest");
    let pass = env("TMWA_E2E_PASS", "spikepass");

    // ---- softlimit: fill the map's fd table ----
    // The real SOFT_LIMIT (fd >= FD_SETSIZE-50 = 974) is unreachable
    // in a test: the map's ~15 s auth timeout reaps raw connections
    // faster than a single filler can open them. tmwa-map honours
    // TMWA_FD_SOFT_LIMIT instead; 150 is just past its own base fds,
    // and ~60 raw conns put every later accept over it.
    fx.kill_map("-TERM");
    tokio::time::sleep(Duration::from_secs(1)).await;
    fx.spawn_map_env(&[("TMWA_FD_SOFT_LIMIT", "150")]);
    fx.wait_map_up().await;
    let script = std::env::temp_dir().join(format!("fdfill-{}.py", std::process::id()));
    std::fs::write(
        &script,
        format!("import socket, time, select\nss = []\ndone = False\nattempts = 0\nend = time.time() + 90\nwhile time.time() < end:\n    try:\n        s = socket.create_connection((\"127.0.0.1\", {port}))\n        ss.append(s)\n    except OSError:\n        pass\n    for dead in [x for x in ss[:] if x.fileno() >= 0 and select.select([x], [], [], 0)[0] and not x.recv(1, socket.MSG_PEEK)]:\n        ss.remove(dead)\n    attempts += 1\n    if not done and (len(ss) >= 140 or attempts >= 400):\n        print(len(ss), flush=True); done = True\n    time.sleep(0.02)\n", port = map_port()),
    )
    .unwrap();
    let mut filler = Command::new("sh")
        .arg("-c")
        .arg(format!("exec python3 {}", script.to_string_lossy()))
        .stdout(std::process::Stdio::piped())
        .spawn()
        .expect("fd filler");

    let mut line = String::new();
    use std::io::BufRead;
    std::io::BufReader::new(filler.stdout.take().unwrap())
        .read_line(&mut line)
        .unwrap();
    let n: usize = line.trim().parse().unwrap_or(0);
    eprintln!("e2e: fd filler opened {n} conns");
    assert!(n >= 100, "filler only opened {n}");
    // Wait until the map's fd count stops climbing: its accept queue
    // is only a handful deep, so the 170 conns drain through it in
    // bursts. If we log in while it is still draining, our upstream
    // conn sits in the backlog for tens of seconds. Once the count
    // is stable and past the soft limit, a new conn is accepted and
    // closed immediately.
    let map_fds = |pid: u32| -> usize {
        std::fs::read_dir(format!("/proc/{pid}/fd"))
            .map(|d| d.count())
            .unwrap_or(0)
    };
    let mut hit = false;
    if let Some(pid) = fx.map_pid() {
        let mut last = 0usize;
        for _ in 0..120 {
            tokio::time::sleep(Duration::from_millis(500)).await;
            let cur = map_fds(pid);
            if cur == last && cur >= 130 {
                hit = true;
                break;
            }
            last = cur;
        }
        eprintln!("e2e: map fd count stabilised at {last}");
    }
    if !hit {
        hit = std::fs::read_to_string(fx.map_stderr())
            .map(|l| l.contains("softlimit reached"))
            .unwrap_or(false);
    }
    eprintln!("e2e: map softlimit reached = {hit}");

    // a client logging in through the gate is accepted-then-closed
    // by the map: it must get 0x0081 code 1 and a close, no hold.
    let mut c = Client::connect().await;
    let (acct, id1, id2) = login(&mut c, &user, &pass).await;
    drop(c);
    let mut c = Client::connect().await;
    let chars = char_connect(&mut c, acct, id1, id2).await;
    let slot = chars[0].char_num;
    char_select(&mut c, slot).await;
    drop(c);
    let mut c = Client::connect().await;
    c.send(|v| {
        P0072 {
            account_id: AccountId(acct),
            char_id: CharId(chars[0].char_id.0),
            login_id1: id1,
            client_tick: 999999,
            sex: Sex(1),
        }
        .encode(v)
    })
    .await;
    let t0 = std::time::Instant::now();
    let mut got_0081 = false;
    let mut got_hold = false;
    let mut closed = false;
    // the map accepts new connections at its own tick rate; with the
    // fd table full of filler conns ours can sit in the listen
    // backlog for many seconds before being softlimit-closed
    while t0.elapsed() < Duration::from_secs(40) && !closed {
        match tokio::time::timeout(Duration::from_secs(40), c.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x0081 => {
                got_0081 = P0081::decode(&p.bytes).unwrap().error_code == 1;
            }
            Ok(Ok(Some(p))) if p.id == 0x009a => got_hold = true,
            Ok(Ok(Some(_))) => continue,
            Ok(Ok(None)) => closed = true,
            Ok(Err(_)) => closed = true,
            Err(_) => break,
        }
    }
    eprintln!(
        "e2e: full-map login: 0081={got_0081} hold={got_hold} closed={closed} in {:?}",
        t0.elapsed()
    );
    assert!(got_0081, "no 0x0081 for the over-capacity login");
    assert!(closed, "over-capacity client not closed");
    assert!(!got_hold, "over-capacity client was announced a hold");

    // release the fd pressure; a fresh login must work again
    let _ = filler.kill();
    let _ = filler.wait();
    fx.kill_map("-TERM");
    tokio::time::sleep(Duration::from_secs(1)).await;
    fx.spawn_map();
    fx.wait_map_up().await;
    tokio::time::sleep(Duration::from_secs(2)).await;
    let mut c = Client::connect().await;
    let (acct, id1, _id2) = login(&mut c, &user, &pass).await;
    drop(c);
    let mut c = Client::connect().await;
    let chars = char_connect(&mut c, acct, id1, _id2).await;
    char_select(&mut c, slot).await;
    drop(c);
    let mut c = Client::connect().await;
    map_connect(&mut c, acct, chars[0].char_id.0, id1).await;
    eprintln!("e2e: login works again after releasing the fds");

    // ---- GM @kick: per-player disconnect on a live map ----
    // log a second player in and kick it from the GM client.
    let mut c2 = Client::connect().await;
    let (acct2, a1, _a2) = login_or_register(&mut c2, "e2ehelper", "testpass").await;
    drop(c2);
    let mut c2 = Client::connect().await;
    let chars2 = char_connect(&mut c2, acct2, a1, _a2).await;
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
    eprintln!("e2e: helper in game, kicking it");

    c.wr.write_all(&chat_pkt("@kick E2ehelper")).await.unwrap();
    // the map's own kick path closes the upstream conn only via
    // clif_setwaitclose (~5 s), then the gate's grace (~3 s)
    let t0 = std::time::Instant::now();
    let mut closed = false;
    let mut got_hold = false;
    while t0.elapsed() < Duration::from_secs(12) && !closed {
        match tokio::time::timeout(Duration::from_secs(12), c2.rd.next()).await {
            Ok(Ok(Some(p))) if p.id == 0x009a => got_hold = true,
            Ok(Ok(Some(_))) => continue,
            Ok(Ok(None)) | Ok(Err(_)) => closed = true,
            Err(_) => break,
        }
    }
    eprintln!(
        "e2e: @kick: closed={closed} hold={got_hold} in {:?}",
        t0.elapsed()
    );
    assert!(closed, "kicked client was not closed");
    assert!(!got_hold, "kicked client was announced a hold");

    // the kick still went through map_quit -> 0x2b01, so a relog
    // lands in-game immediately
    tokio::time::sleep(Duration::from_secs(1)).await;
    let mut c2 = Client::connect().await;
    let (acct2, a1, _a2) = login_or_register(&mut c2, "e2ehelper", "testpass").await;
    drop(c2);
    let mut c2 = Client::connect().await;
    let chars2 = char_connect(&mut c2, acct2, a1, _a2).await;
    let _ = char_select(&mut c2, chars2[0].char_num).await;
    drop(c2);
    let mut c2 = Client::connect().await;
    map_connect(&mut c2, acct2, cid2, a1).await;
    eprintln!("e2e: kicked player relogged fine");
}
