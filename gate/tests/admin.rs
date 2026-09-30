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
use tmwa_gate::db::Db;

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

    // sequential ids via the shared create path: admin create and
    // the in-game path (Db::create_account) must both consume
    // next_account_id
    let meta0: i64 = st
        .db
        .blocking(move |db| {
            db.with_conn(|c| {
                Ok(c.query_row(
                    "SELECT value FROM meta WHERE key='next_account_id'",
                    [],
                    |r| r.get(0),
                )
                .unwrap())
            })
        })
        .await
        .unwrap();
    let v = cmd(&st, "create", &["seqone", "s@e.st", "pw123"]).await;
    let id1: i64 = text(&v)
        .split("[id: ")
        .nth(1)
        .unwrap()
        .split(']')
        .next()
        .unwrap()
        .parse()
        .unwrap();
    let id2 = st
        .db
        .blocking(move |db| {
            db.with_conn(|c| {
                Db::create_account(c, "seqtwo", "x", "t@e.st")
                    .map_err(|e| rusqlite::Error::ToSqlConversionFailure(e.to_string().into()))
            })
        })
        .await
        .unwrap();
    assert_eq!(id2, id1 + 1);
    assert_eq!(id1, meta0, "id {id1} != meta {meta0}");
    cmd(&st, "delete", &["seqone"]).await;
    cmd(&st, "delete", &["seqtwo"]).await;

    // account delete removes its characters' party membership too
    let v = cmd(&st, "create", &["partytest", "p@e.st", "pw123"]).await;
    let pid_acct: i64 = text(&v)
        .split("[id: ")
        .nth(1)
        .unwrap()
        .split(']')
        .next()
        .unwrap()
        .parse()
        .unwrap();
    // clone the fixture character under this account, then mark it
    // a party member — exercises the delete path through its party
    let cid: i64 = st
        .db
        .blocking(move |db| {
            db.with_conn(move |c| {
                c.execute(
                    "INSERT INTO characters SELECT 90000001, ?1, slot, 'partyc', sex, species, base_level, job_level, base_exp, job_exp, zeny, hp, max_hp, sp, max_sp, attr_str, attr_agi, attr_vit, attr_int, attr_dex, attr_luk, status_point, skill_point, option_, karma, manner, party_id, hair, hair_color, clothes_color, weapon, shield, head_top, head_mid, head_bottom, last_map, last_x, last_y, save_map, save_x, save_y, partner_id FROM characters LIMIT 1",
                    [pid_acct],
                )
                .unwrap();
                Ok(90000001i64)
            })
        })
        .await
        .unwrap();
    // put it in a party the way the map would: party row + member,
    // then load it into the gate's party cache
    st.db.blocking(move |db| {
        db.with_conn(move |c| {
            c.execute(
                "UPDATE characters SET party_id=1 WHERE id=?1",
                [cid],
            )
            .unwrap();
            c.execute(
                "INSERT OR REPLACE INTO parties (id,name,exp_share,item_share) VALUES (1,'ptest',0,0)",
                [],
            )
            .unwrap();
            c.execute(
                "INSERT OR REPLACE INTO party_members (party_id,account_id,char_name,leader) VALUES (1,?1,'partyc',1)",
                [pid_acct],
            )
            .unwrap();
            Ok::<(), rusqlite::Error>(())
        })
    })
    .await
    .unwrap();
    tmwa_gate::serve::maplink::load_parties(&st);
    let v = cmd(&st, "delete", &["partytest"]).await;
    assert!(text(&v).contains("DELETED"), "{v}");
    let left: i64 = st
        .db
        .blocking(move |db| {
            db.with_conn(move |c| {
                Ok(c.query_row(
                    "SELECT COUNT(*) FROM party_members WHERE account_id=?1",
                    [pid_acct],
                    |r| r.get::<_, i64>(0),
                )
                .unwrap())
            })
        })
        .await
        .unwrap();
    assert_eq!(left, 0, "party member row left after delete");
    let v = cmd(&st, "find", &["--id", &pid_acct.to_string()]).await;
    assert!(!text(&v).contains("partytest"), "{v}");

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
