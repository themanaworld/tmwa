//! Admin command tests: drive `serve::admin::dispatch` against a
//! fresh fixture DB and check the text output matches what
//! mirror-lake's tmwa.py pcmd parser expects:
//!   * `search <name>` — a line containing the name, or
//!     "No account found."
//!   * `create` — a line containing "[id: N]"
//!   * `getcount` — a line "<server name> : <count>"
#![allow(clippy::await_holding_lock)]

use std::path::PathBuf;
use std::process::Command;
use std::sync::Mutex;

static LOCK: Mutex<()> = Mutex::new(());

fn test_state() -> std::sync::Arc<tmwa_gate::serve::state::State> {
    let dir = std::env::temp_dir().join(format!("admintest-{}", std::process::id()));
    let _ = std::fs::remove_dir_all(&dir);
    std::fs::create_dir_all(&dir).unwrap();
    let db_path = dir.join("gate.db");
    // fresh import of the fixture save files
    let home = std::env::var("HOME").unwrap();
    let save = std::env::var("TMWA_E2E_SAVE_DIR")
        .unwrap_or_else(|_| format!("{home}/projects/tmw/serverdata/world/save"));
    let account_txt = std::env::var("TMWA_E2E_ACCOUNT_TXT")
        .unwrap_or_else(|_| format!("{home}/projects/tmw/serverdata/login/save/account.txt"));
    let out = Command::new(env!("CARGO_BIN_EXE_tmwa-gate"))
        .args([
            "import",
            "--db",
            db_path.to_str().unwrap(),
            "--save-dir",
            &save,
            "--account-txt",
            &account_txt,
        ])
        .output()
        .unwrap();
    assert!(out.status.success(), "import: {out:?}");
    let cfg =
        tmwa_gate::config::Config::load(&PathBuf::from("/home/bjorn/gate-run/gate.toml")).unwrap();
    let db = tmwa_gate::db::Db::open(&db_path).unwrap();
    std::sync::Arc::new(tmwa_gate::serve::state::State::new(
        cfg,
        std::sync::Arc::new(db),
    ))
}

async fn cmd(
    st: &std::sync::Arc<tmwa_gate::serve::state::State>,
    name: &str,
    args: &[&str],
) -> serde_json::Value {
    tmwa_gate::serve::admin::dispatch(st, name, args.iter().map(|s| s.to_string()).collect(), None)
        .await
}

fn text(v: &serde_json::Value) -> String {
    v.get("text")
        .and_then(|t| t.as_str())
        .unwrap_or_default()
        .to_string()
}

#[tokio::test]
async fn admin_mirror_lake() {
    let _g = LOCK.lock().unwrap_or_else(|e| e.into_inner());
    let st = test_state();

    // search finds the fixture account and the line contains the
    // name (mirror-lake checks `"<user>" in ln`)
    let v = cmd(&st, "search", &["spiketest"]).await;
    let t = text(&v);
    assert!(v["ok"].as_bool().unwrap(), "{v}");
    assert!(t.contains("spiketest"), "{t}");
    let v = cmd(&st, "search", &["nosuchuser"]).await;
    assert!(text(&v).contains("No account found"), "{v}");

    // create emits "[id: N]"
    let v = cmd(&st, "create", &["newacc", "n@e.st", "pw123"]).await;
    let t = text(&v);
    assert!(t.contains("[id: "), "{t}");
    let id: i64 = t
        .split("[id: ")
        .nth(1)
        .unwrap()
        .trim()
        .replace("].", "")
        .trim_end_matches(']')
        .parse()
        .unwrap();

    // getcount: a line containing the server name, ':'-separated int
    let v = cmd(&st, "getcount", &[]).await;
    let t = text(&v);
    let name = st.cfg.char_.server_name.clone();
    let line = t
        .lines()
        .find(|l| l.contains(&name))
        .unwrap_or_else(|| panic!("no server line in {t:?}"));
    line.split(':')
        .nth(1)
        .unwrap()
        .trim()
        .parse::<i64>()
        .unwrap();

    // set / ga / del round trip
    let v = cmd(&st, "set", &[&id.to_string(), "##VAULT", "7"]).await;
    assert!(v["ok"].as_bool().unwrap(), "{v}");
    let v = cmd(&st, "ga", &[&id.to_string()]).await;
    assert!(text(&v).contains("##VAULT,7"), "{v}");
    let v = cmd(&st, "del", &[&id.to_string(), "##VAULT"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "ga", &[&id.to_string()]).await;
    assert!(!text(&v).contains("##VAULT"), "{v}");

    // memo + find
    let v = cmd(&st, "memo", &["newacc", "mymemo"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "find", &["--memo", "mymemo"]).await;
    assert!(text(&v).contains("newacc"), "{v}");

    // check / password
    let v = cmd(&st, "check", &["newacc", "pw123"]).await;
    assert!(text(&v).contains("correct"), "{v}");
    let v = cmd(&st, "password", &["newacc", "otherpw"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "check", &["newacc", "otherpw"]).await;
    assert!(text(&v).contains("correct"), "{v}");
    let v = cmd(&st, "check", &["newacc", "badpw"]).await;
    assert!(text(&v).contains("INCORRECT"), "{v}");

    // ban / state / delete
    let v = cmd(&st, "banset", &["newacc", "2030/01/01"]).await;
    assert!(v["ok"].as_bool().unwrap(), "{v}");
    let v = cmd(&st, "unban", &["newacc"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "block", &["newacc"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "unblock", &["newacc"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "state", &["newacc", "9"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "id", &["newacc"]).await;
    assert!(text(&v).contains(&id.to_string()), "{v}");
    let v = cmd(&st, "name", &[&id.to_string()]).await;
    assert!(text(&v).contains("newacc"), "{v}");
    let v = cmd(&st, "email", &["newacc", "n2@e.st"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "who", &["newacc"]).await;
    assert!(text(&v).contains("n2@e.st"), "{v}");
    let v = cmd(&st, "list", &[]).await;
    assert!(text(&v).contains("newacc"), "{v}");
    let v = cmd(&st, "delete", &["newacc"]).await;
    assert!(text(&v).contains("DELETED"), "{v}");

    // online/status/find/chars/kick/version/help shapes
    let v = cmd(&st, "online", &[]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "chars", &["--account", "spiketest"]).await;
    assert!(text(&v).contains("Spiketest"), "{v}");
    let v = cmd(&st, "kick", &["Spiketest"]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "version", &[]).await;
    assert!(v["ok"].as_bool().unwrap());
    let v = cmd(&st, "help", &[]).await;
    assert!(v["ok"].as_bool().unwrap());
}
