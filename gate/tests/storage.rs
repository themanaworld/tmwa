//! Storage/importer tests: fixture files in the tmwa formats, an
//! import, a test-only re-export and a load/save round trip.

use std::fmt::Write as _;
use tmwa_gate::auth::password::*;
use tmwa_gate::db::{self, Db};
use tmwa_gate::import::{self, ImportFiles};
use tmwa_gate::proto::*;

/// tmwa's `pass_ok`: recompute `MD5_saltcrypt(password, salt)` from the
/// stored `!salt$hash` string and compare.
fn verify_legacy(password: &[u8], stored: &str) -> bool {
    let Some(salt) = legacy_salt(stored) else {
        return false;
    };
    if salt.is_empty() {
        return false;
    }
    md5_saltcrypt(password, salt.as_bytes()) == stored
}

fn fixtures_dir() -> tempfile::TempDir {
    let d = tempfile::tempdir().unwrap();
    // account.txt: plaintext (memo '-'), legacy hash, banned, state 7,
    // default email, ## vars, malformed line, %newid%.
    std::fs::write(
        d.path().join("account.txt"),
        "// Accounts\n\
2000000\talice\t!xF];6$bf80d2e93be8cc38906cba48\t2026-09-30 14:24:00.590\tM\t64\t0\ta@a.com\t-\t0\t127.0.0.1\t!\t0\t##foo,7 ##bar,-3\n\
2000001\tbob\thunter2\t-\tF\t1\t0\tbob@example.com\t-\t0\t-\t-\t0\t\n\
2000002\tcarol\t!salt!$01b9d165bad5ee929de4ab03\t-\tS\t0\t7\t-\t-\t0\t-\t-\t0\t\n\
2000003\tdave\t!12345$d7dcddc821ffc58264a16bce\t-\tM\t0\t0\tdave@example.com\t-\t0\t-\t-\t0\t\n\
broken line without enough fields\n\
2000006\tgarbage\tnotahash\t-\tM\t0\t0\ta@a.com\t-\t0\t-\t!\t0\t\n\
2000004\terin\t!12345$d7dcddc821ffc58264a16bce\t-\tM\t0\t0\tnofetch\t-\t0\t-\t-\t1893456000\t\n\
2000004\terin2\t!12345$d7dcddc821ffc58264a16bce\t-\tM\t0\t0\ta@a.com\t-\t0\t-\t-\t0\t\n\
2000005\t%newid%\n",
    )
    .unwrap();
    // athena.txt: items with equip bits, skills with flags, vars,
    // unspecified sex (falls back to account sex M for erin? - see note)
    std::fs::write(
        d.path().join("athena.txt"),
        "// Characters\n\
150000\t2000000,0\tAlice\t1,100,1\t57,15,35\t1064,1064,139,139\t99,34,99,27,99,99\t1409,0\t0,0,0\t0,0,0\t-1,1,0\t1,0,0,0,0\t029-1,59,91\t001-1,32,59,0\tF\t0,522,1,2,1,0,0,0,0,0,0,0 0,535,10,0,1,0,0,0,0,0,0,0 0,1201,1,512,1,0,0,0,0,0,0,0 \t\t8,393222 200,9 \tFLAGS,21020672 TUT_var,1790767931 \n\
150001\t2000001,1\tBobby\t1,5,1\t1,2,3\t100,100,50,50\t1,1,1,1,1,1\t0,0\t0,0,0\t0,0,0\t0,0,0\t0,0,0,0,0\t001-1,10,20\t001-1,10,20,0\tS\t \t\t \t \n\
150002\t%newid%\n",
    )
    .unwrap();
    std::fs::write(
        d.path().join("party.txt"),
        "// Parties\n\
234\tCoolParty\t1,1\t2000000,1\tAlice\t2000001,0\tBobby\t\n\
235\t%newid%\n",
    )
    .unwrap();
    std::fs::write(
        d.path().join("storage.txt"),
        "// Storage\n\
2000000,2\t0,535,10,0,0,0,0,0,0,0,0 0,1201,1,512,0,0,0,0,0,0,0\t\n",
    )
    .unwrap();
    std::fs::write(
        d.path().join("accreg.txt"),
        "// Account registers\n\
2000000\t#var1,11 #var2,22 \n",
    )
    .unwrap();
    d
}

fn import_to(files: &ImportFiles) -> Db {
    let db = Db::open_memory().unwrap();
    let sum = import::run(files, &db, |_| {}).unwrap();
    assert_eq!(sum.accounts, 6);
    assert_eq!(sum.characters, 2);
    assert_eq!(sum.parties, 1);
    assert_eq!(sum.storage_entries, 2);
    assert_eq!(sum.vars, 2);
    assert_eq!(sum.skipped.len(), 2);
    db
}

#[test]
fn import_fixture() {
    let d = fixtures_dir();
    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = import_to(&files);

    // accounts field by field
    let (hash, scheme, salt, email, state, ban, memo): (
        String,
        String,
        Option<String>,
        Option<String>,
        i64,
        i64,
        String,
    ) = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT password_hash,password_scheme,legacy_salt,email,state,ban_until,memo
             FROM accounts WHERE name='alice'",
                [],
                |r| {
                    Ok((
                        r.get(0)?,
                        r.get(1)?,
                        r.get(2)?,
                        r.get(3)?,
                        r.get(4)?,
                        r.get(5)?,
                        r.get(6)?,
                    ))
                },
            )
        })
        .unwrap();
    assert_eq!(scheme, "argon2id-md5");
    assert_eq!(salt.as_deref(), Some("xF];6"));
    assert_eq!(
        verify("argon2id-md5", &hash, salt.as_deref(), b"spikepass").unwrap(),
        Verify::OkNeedsRehash
    );
    assert_eq!(email, None); // a@a.com maps to NULL
    assert_eq!(state, 0);
    assert_eq!(ban, 0);
    assert_eq!(memo, "!");

    // plaintext import (bob) -> argon2id
    let (hash2, scheme2): (String, String) = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT password_hash,password_scheme FROM accounts WHERE name='bob'",
                [],
                |r| Ok((r.get(0)?, r.get(1)?)),
            )
        })
        .unwrap();
    assert_eq!(scheme2, "argon2id");
    assert_eq!(
        verify("argon2id", &hash2, None, b"hunter2").unwrap(),
        Verify::Ok
    );

    // state 7 error message + ban
    let (st, err, ban7): (i64, Option<String>, i64) = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT state,error_message,ban_until FROM accounts WHERE name='carol'",
                [],
                |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)),
            )
        })
        .unwrap();
    assert_eq!(st, 7);
    assert_eq!(err.as_deref(), Some("-"));
    assert_eq!(ban7, 0);
    // erin: ban timestamp preserved; erin's duplicate id line skipped,
    // erin's invalid email normalized to a@a.com -> NULL
    let (ban_e, em_e): (i64, Option<String>) = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT ban_until,email FROM accounts WHERE name='erin'",
                [],
                |r| Ok((r.get(0)?, r.get(1)?)),
            )
        })
        .unwrap();
    assert_eq!(ban_e, 1893456000);
    assert_eq!(em_e, None);
    // ## vars on alice in scope 2
    let vars: Vec<(String, i64)> = db.with_conn(|conn| {
        conn.prepare(
            "SELECT name,value FROM account_vars WHERE account_id=2000000 AND scope=2 ORDER BY name",
        )?
        .query_map([], |r| Ok((r.get(0)?, r.get(1)?)))?
        .collect::<Result<_, _>>()
    }).unwrap();
    assert_eq!(vars, vec![("##bar".into(), -3), ("##foo".into(), 7)]);

    // accreg # vars in scope 1
    let vars1: Vec<(String, i64)> = db.with_conn(|conn| {
        conn.prepare(
            "SELECT name,value FROM account_vars WHERE account_id=2000000 AND scope=1 ORDER BY name",
        )?
        .query_map([], |r| Ok((r.get(0)?, r.get(1)?)))?
        .collect::<Result<_, _>>()
    }).unwrap();
    assert_eq!(vars1, vec![("#var1".into(), 11), ("#var2".into(), 22)]);

    // meta ids continue above tmwa's
    assert_eq!(db.meta("next_account_id").unwrap(), Some(2000007));
    assert_eq!(db.meta("next_char_id").unwrap(), Some(150002));
    assert_eq!(db.meta("next_party_id").unwrap(), Some(235));

    // party
    let pm: Vec<(i64, String, i64)> = db.with_conn(|conn| {
        conn.prepare(
            "SELECT account_id,char_name,leader FROM party_members WHERE party_id=234 ORDER BY account_id",
        )?
        .query_map([], |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)))?
        .collect::<Result<_, _>>()
    }).unwrap();
    assert_eq!(
        pm,
        vec![(2000000, "Alice".into(), 1), (2000001, "Bobby".into(), 0)]
    );

    // storage
    let si: Vec<(i64, i64, i64, i64)> = db.with_conn(|conn| {
        conn.prepare(
            "SELECT idx,item_id,amount,equip FROM storage_items WHERE account_id=2000000 ORDER BY idx",
        )?
        .query_map([], |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?, r.get(3)?)))?
        .collect::<Result<_, _>>()
    }).unwrap();
    assert_eq!(si, vec![(0, 535, 10, 0), (1, 1201, 1, 512)]);
}

#[test]
fn character_load_save() {
    let d = fixtures_dir();
    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = import_to(&files);
    let (key, cd) = db.load_character(150000).unwrap();
    assert_eq!(key.name.to_string_lossy(), "Alice");
    assert_eq!(key.account_id, AccountId(2000000));
    assert_eq!(cd.base_level, 100);
    assert_eq!(cd.hp, 1064);
    assert_eq!(cd.species, Species(1));
    assert_eq!(cd.hair, 0); // "-1" normalized to 0
    assert_eq!(cd.sex, Sex(0)); // F
    assert_eq!(cd.inventory[0].nameid, ItemNameId(522));
    assert_eq!(cd.inventory[0].equip, Epos(2));
    assert_eq!(cd.inventory[1].nameid, ItemNameId(535));
    assert_eq!(cd.inventory[2].equip, Epos(512));
    assert_eq!(cd.skill[8].lv, 6); // 393222 & 0xffff
    assert_eq!(cd.skill[8].flags, SkillFlags(6));
    assert_eq!(cd.skill[200].lv, 9);
    assert_eq!(cd.global_reg_num, 2);
    assert_eq!(cd.global_reg[0].str.as_bytes(), b"FLAGS");
    assert_eq!(cd.global_reg[0].value, 21020672);
    assert_eq!(cd.account_reg[0].str.as_bytes(), b"#var1");
    assert_eq!(cd.account_reg2[0].str.as_bytes(), b"##bar");
    assert_eq!(cd.last_point.map_.as_bytes(), b"029-1");

    // Bobby: unspecified sex resolved from account (F)
    let (k2, cd2) = db.load_character(150001).unwrap();
    assert_eq!(cd2.sex, Sex(0));
    assert_eq!(k2.char_num, 1);

    // save a change and reload
    let mut cd3 = cd;
    cd3.hp = 999;
    cd3.inventory[7].nameid = ItemNameId(7049);
    cd3.inventory[7].amount = 3;
    db.with_conn(|c| {
        let tx = c.transaction()?;
        db::save_character_conn(&tx, &key, &cd3)?;
        tx.commit()
    })
    .unwrap();
    let (_, cd4) = db.load_character(150000).unwrap();
    assert_eq!(cd4.hp, 999);
    assert_eq!(cd4.inventory[7].nameid, ItemNameId(7049));
    assert_eq!(cd4.inventory[0].nameid, ItemNameId(522));

    // list
    let list = db
        .with_conn(|c| db::char_ids_of_account_conn(c, 2000000))
        .unwrap();
    assert_eq!(list, [150000]);
}

#[test]
fn password_legacy() {
    // vectors validated against tmwa's MD5_saltcrypt (real account
    // hash for spikepass/xF];6 reproduces the production line)
    assert_eq!(
        md5_saltcrypt(b"spikepass", b"xF];6"),
        "!xF];6$bf80d2e93be8cc38906cba48"
    );
    assert_eq!(
        md5_saltcrypt(b"hunter2", b"salt!"),
        "!salt!$01b9d165bad5ee929de4ab03"
    );
    assert_eq!(
        md5_saltcrypt(b"a", b"12345"),
        "!12345$d7dcddc821ffc58264a16bce"
    );
    assert!(verify_legacy(
        b"spikepass",
        "!xF];6$bf80d2e93be8cc38906cba48"
    ));
    assert!(!verify_legacy(b"wrong", "!xF];6$bf80d2e93be8cc38906cba48"));

    // wrapped verify + needs rehash
    let wrapped = wrap_legacy("!xF];6$bf80d2e93be8cc38906cba48").unwrap();
    assert_eq!(
        verify("argon2id-md5", &wrapped, Some("xF];6"), b"spikepass").unwrap(),
        Verify::OkNeedsRehash
    );
    assert_eq!(
        verify("argon2id-md5", &wrapped, Some("xF];6"), b"nope").unwrap(),
        Verify::Fail
    );
    // wrong scheme fails closed
    assert!(verify("rot13", &wrapped, None, b"x").is_err());
}

/// Test-only exporters mirroring tmwa's tostr writers (char.cpp
/// mmo_char_tostr, int_party.cpp, int_storage.cpp, inter.cpp).
mod export {
    use super::*;

    pub fn char_tostr(key: &CharKey, p: &CharData) -> String {
        let mut s = format!(
            "{id}\t{acct},{slot}\t{name}\t{sp},{bl},{jl}\t{be},{je},{zen}\t{hp},{mhp},{sp_},{msp}\t{a0},{a1},{a2},{a3},{a4},{a5}\t{stp},{skp}\t{opt},{kar},{man}\t{pid},0,0\t{hair},{hc},{cc}\t{weap},{shi},{t},{m},{b}\t{lm},{lx},{ly}\t{sm},{sx},{sy},{partner}\t{sex}\t",
            id = key.char_id.0,
            acct = key.account_id.0,
            slot = key.char_num,
            name = key.name.to_string_lossy(),
            sp = p.species.0,
            bl = p.base_level,
            jl = p.job_level,
            be = p.base_exp,
            je = p.job_exp,
            zen = p.zeny,
            hp = p.hp,
            mhp = p.max_hp,
            sp_ = p.sp,
            msp = p.max_sp,
            a0 = p.attrs[0],
            a1 = p.attrs[1],
            a2 = p.attrs[2],
            a3 = p.attrs[3],
            a4 = p.attrs[4],
            a5 = p.attrs[5],
            stp = p.status_point,
            skp = p.skill_point,
            opt = p.option.0,
            kar = p.karma,
            man = p.manner,
            pid = p.party_id.0,
            hair = p.hair,
            hc = p.hair_color,
            cc = p.clothes_color,
            weap = p.weapon.0,
            shi = p.shield.0,
            t = p.head_top.0,
            m = p.head_mid.0,
            b = p.head_bottom.0,
            lm = p.last_point.map_.to_string_lossy(),
            lx = p.last_point.x,
            ly = p.last_point.y,
            sm = p.save_point.map_.to_string_lossy(),
            sx = p.save_point.x,
            sy = p.save_point.y,
            partner = p.partner_id.0,
            sex = sex_char(p.sex),
        );
        for it in p.inventory.iter() {
            if it.nameid.0 != 0 {
                s += &format!(
                    "0,{},{},{},1,0,0,0,0,0,0,0 ",
                    it.nameid.0, it.amount, it.equip.0
                );
            }
        }
        s.push('\t');
        s.push('\t');
        for (i, sk) in p.skill.iter().enumerate() {
            if sk.lv != 0 {
                s += &format!("{},{} ", i, sk.lv as u32 | ((sk.flags.0 as u32) << 16));
            }
        }
        s.push('\t');
        for reg in p.global_reg.iter().take(p.global_reg_num as usize) {
            if !reg.str.as_bytes().is_empty() {
                s += &format!("{},{} ", reg.str.to_string_lossy(), reg.value);
            }
        }
        s.push('\t');
        s
    }

    pub fn sex_char(s: Sex) -> char {
        match s.0 {
            0 => 'F',
            1 => 'M',
            3 => 'N',
            _ => 'S',
        }
    }

    pub fn accreg_tostr(account_id: i64, vars: &[(String, i64)]) -> String {
        let mut s = format!("{}\t", account_id);
        for (n, v) in vars {
            s += &format!("{},{} ", n, v);
        }
        s
    }
}

#[test]
fn reexport_round_trip() {
    let d = fixtures_dir();
    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = import_to(&files);
    let (key, cd) = db.load_character(150000).unwrap();
    let line = export::char_tostr(&key, &cd);
    // compare against the input line with tmwa's own normalization:
    // hair "-1" -> 0 (fixture), rest identical.
    let expect = std::fs::read_to_string(d.path().join("athena.txt"))
        .unwrap()
        .lines()
        .nth(1)
        .unwrap()
        .replace("\t-1,", "\t0,");
    // the tmwa writer appends a trailing tab after the vars list
    assert_eq!(
        line.trim_end_matches(['\t', ' ']),
        expect.trim_end_matches(['\t', ' '])
    );

    // accreg line
    let vars: Vec<(String, i64)> = db.with_conn(|conn| {
        conn.prepare(
            "SELECT name,value FROM account_vars WHERE account_id=2000000 AND scope=1 ORDER BY name",
        )?
        .query_map([], |r| Ok((r.get(0)?, r.get(1)?)))?
        .collect::<Result<_, _>>()
    }).unwrap();
    let acc = export::accreg_tostr(2000000, &vars);
    let expect = std::fs::read_to_string(d.path().join("accreg.txt"))
        .unwrap()
        .lines()
        .nth(1)
        .unwrap()
        .to_string();
    assert_eq!(acc, expect);
}

/// 10k synthetic accounts through the importer: parse + parallel
/// argon2 hashing + single-transaction insert. Run manually with
/// `cargo test --test storage timing_10k -- --ignored --nocapture`.
#[test]
#[ignore]
fn timing_10k() {
    let d = tempfile::tempdir().unwrap();
    let mut acct = String::from("// Accounts\n");
    for i in 0..10000 {
        let id = 2000000 + i;
        writeln!(
            acct,
            "{id}\tuser{i}\tpass{i}\t-\tM\t0\t0\ta@a.com\t-\t0\t-\t!\t0\t"
        )
        .unwrap();
    }
    writeln!(acct, "{}\t%newid%", 2000000 + 10000).unwrap();
    std::fs::write(d.path().join("account.txt"), acct).unwrap();
    for f in ["athena.txt", "party.txt", "storage.txt", "accreg.txt"] {
        std::fs::write(d.path().join(f), "").unwrap();
    }
    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = Db::open_memory().unwrap();
    let t0 = std::time::Instant::now();
    let sum = import::run(&files, &db, |m| println!("{m}")).unwrap();
    println!(
        "10k accounts total {:.2}s (hashing {:.2}s)",
        t0.elapsed().as_secs_f64(),
        sum.password_seconds
    );
}

/// 0x2b01-style saves must not touch account_vars: tmwa persists
/// `#` vars only via 0x3004 and `##` vars only via 0x2b10.
#[test]
fn save_character_keeps_account_vars() {
    let d = fixtures_dir();
    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = import_to(&files);
    let (key, mut cd) = db.load_character(150000).unwrap();
    // an admin / 0x2b10 path sets a newer ## var
    db.with_conn(|c| db::set_account_vars(c, 2000000, 2, &[("##foo".to_string(), 99)]))
        .unwrap();
    // stale CharData save (still has the imported ##foo=7 snapshot)
    cd.hp = 500;
    db.with_conn(|c| {
        let tx = c.transaction()?;
        db::save_character_conn(&tx, &key, &cd)?;
        tx.commit()
    })
    .unwrap();
    assert_eq!(
        db.with_conn(|c| db::get_account_vars_conn(c, 2000000, 2))
            .unwrap(),
        vec![("##bar".to_string(), -3), ("##foo".to_string(), 99)]
    );
}

/// Password import rules: plaintext only when pass lacks '!' AND memo
/// starts with '-'; other non-'!' entries are wrapped as argon2id-md5
/// and (like tmwa) never verify.
#[test]
fn password_edge_cases() {
    // pass_ok's salt: skip first char whatever it is, up to '$' or end
    assert_eq!(
        legacy_salt("!xF];6$bf80d2e93be8cc38906cba48"),
        Some("xF];6")
    );
    assert_eq!(legacy_salt("garbage$rest"), Some("arbage"));
    assert_eq!(legacy_salt("nodollar"), Some("odollar"));
    assert_eq!(legacy_salt("!"), None);
    assert_eq!(legacy_salt(""), None);
    // a non-legacy string never verifies
    assert!(!verify_legacy(b"notahash", "notahash"));
    assert!(!verify_legacy(b"x", "!xF];6$bf80d2e93be8cc38906cba49"));
}

#[test]
fn garbage_password_import() {
    let d = fixtures_dir();
    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = import_to(&files);
    // 'garbage' account: pass 'notahash' does not start with '!'
    // and memo is '!' (not '-') -> treated as legacy, never verifies
    let (hash, scheme, salt) = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT password_hash,password_scheme,legacy_salt
             FROM accounts WHERE name='garbage'",
                [],
                |r| {
                    Ok((
                        r.get::<usize, String>(0)?,
                        r.get::<usize, String>(1)?,
                        r.get::<usize, Option<String>>(2)?,
                    ))
                },
            )
        })
        .unwrap();
    assert_eq!(scheme, "argon2id-md5");
    assert_eq!(salt.as_deref(), Some("otahash"));
    assert_eq!(
        verify("argon2id-md5", &hash, salt.as_deref(), b"notahash").unwrap(),
        Verify::Fail
    );
    // bob's memo was '-' and pass plaintext -> memo stored as '!'
    let memo: String = db
        .with_conn(|conn| {
            conn.query_row("SELECT memo FROM accounts WHERE name='bob'", [], |r| {
                r.get(0)
            })
        })
        .unwrap();
    assert_eq!(memo, "!");
}

/// Real save files pick up non-UTF-8 bytes (a name truncated inside
/// a multi-byte char) and rows for long-gone accounts: both are
/// skipped with a warning instead of failing the import.
#[test]
fn import_skips_bad_utf8_and_orphan_vars() {
    let d = fixtures_dir();
    // a line that is not valid UTF-8 (truncated multibyte char)
    use std::io::Write;
    std::fs::OpenOptions::new()
        .append(true)
        .open(d.path().join("party.txt"))
        .unwrap()
        .write_all(b"300\tBroken\xff\xfeParty\t1,1\t2000000,1\tAlice\t\n")
        .unwrap();
    // accreg + char rows for an account that doesn't exist
    std::fs::write(d.path().join("accreg.txt"), "9999999\t#var3,33\n").unwrap();
    std::fs::OpenOptions::new()
        .append(true)
        .open(d.path().join("athena.txt"))
        .unwrap()
        .write_all(
            b"150009\t9999999,2\tOrphan\t1,5,1\t1,2,3\t100,100,50,50\t1,1,1,1,1,1\t0,0\t0,0,0\t0,0,0\t0,0,0\t0,0,0\t0,0,0,0,0\t001-1,10,20\t001-1,10,20,0\tS\t \t\t \t \n",
        )
        .unwrap();

    let files = ImportFiles {
        save_dir: d.path().to_path_buf(),
        account_txt: None,
        athena_txt: None,
        party_txt: None,
        storage_txt: None,
        accreg_txt: None,
    };
    let db = Db::open_memory().unwrap();
    let sum = import::run(&files, &db, |_| {}).unwrap();
    // the two fixture skips plus: the utf-8 party line, the orphan
    // accreg row and the orphan character
    assert_eq!(sum.skipped.len(), 5, "skipped: {:?}", sum.skipped);
    assert!(sum.skipped.iter().any(|s| s.contains("invalid utf-8")));
    assert!(sum.skipped.iter().any(|s| s.contains("9999999")));
    // the orphan's char and vars must not have landed
    let n: i64 = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM characters WHERE name='Orphan'",
                [],
                |r| r.get(0),
            )
        })
        .unwrap();
    assert_eq!(n, 0);
    let v: i64 = db
        .with_conn(|conn| {
            conn.query_row(
                "SELECT COUNT(*) FROM account_vars WHERE name='#var3'",
                [],
                |r| r.get(0),
            )
        })
        .unwrap();
    assert_eq!(v, 0);
}
