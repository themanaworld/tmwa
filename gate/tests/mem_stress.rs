//! Memory-retention probe for the serialized maplink DB writer.
//!
//! The round-2 load test saw the gate hold ~20 GB of anonymous RSS
//! after a congestion burst. Likely suspect: glibc malloc arenas
//! retain the freed burst (the `db_jobs` queue plus per-job
//! allocations on spawn_blocking worker threads) instead of
//! returning it to the kernel.
//!
//! Not run by default — floods ~1 GB. Run explicitly:
//!
//!     TMWA_MEMSTRESS=1 cargo test --test mem_stress -- --nocapture

use std::sync::Arc;
use tmwa_gate::proto::*;

fn rss_kb() -> u64 {
    for line in std::fs::read_to_string("/proc/self/status")
        .unwrap()
        .lines()
    {
        if let Some(rest) = line.strip_prefix("VmRSS:") {
            return rest.trim().trim_end_matches("kB").trim().parse().unwrap();
        }
    }
    0
}

fn mb(kb: u64) -> f64 {
    kb as f64 / 1024.0
}

#[tokio::test]
async fn db_queue_retention() {
    if std::env::var("TMWA_MEMSTRESS").is_err() {
        eprintln!("mem_stress: skipped (set TMWA_MEMSTRESS=1)");
        return;
    }
    let mut cfg = tmwa_gate::config::Config::default();
    let dir = tempfile::tempdir().unwrap();
    cfg.gate.db = dir.path().join("gate.db");
    cfg.gate.gm_account_file = dir.path().join("gm_account.txt");
    cfg.gate.online_txt = dir.path().join("online.txt");
    cfg.gate.online_html = dir.path().join("online.html");
    cfg.gate.admin_socket = dir.path().join("gate.sock");
    let st = Arc::new(tmwa_gate::serve::state::State::new(
        cfg,
        Arc::new(tmwa_gate::db::Db::open_memory().unwrap()),
    ));
    let st2 = st.clone();
    tokio::spawn(async move {
        tmwa_gate::serve::state::db_writer(st2).await;
    });

    const N: u32 = 120_000;
    let base = rss_kb();
    // enqueue faster than the writer can drain: the queue itself is
    // the transient the load test produced
    for i in 0..N {
        let key = CharKey {
            char_id: CharId(150000 + (i % 2000)),
            ..Default::default()
        };
        st.queue_save(i, key, Arc::new(CharData::default()));
    }
    let queued = rss_kb();
    // wait for the queue to commit completely
    let _ = st.db_barrier().await;
    let drained = rss_kb();
    tokio::time::sleep(std::time::Duration::from_secs(5)).await;
    let settled = rss_kb();
    eprintln!(
        "VmRSS: base {:.0}M -> queued {:.0}M -> drained {:.0}M -> settled {:.0}M",
        mb(base),
        mb(queued),
        mb(drained),
        mb(settled)
    );
    // a second burst of the same size: retained arenas should be
    // reused, a true leak would grow again
    for i in 0..N {
        let key = CharKey {
            char_id: CharId(150000 + (i % 2000)),
            ..Default::default()
        };
        st.queue_save(i, key, Arc::new(CharData::default()));
    }
    let _ = st.db_barrier().await;
    tokio::time::sleep(std::time::Duration::from_secs(5)).await;
    let settled2 = rss_kb();
    eprintln!("VmRSS after second burst: {:.0}M", mb(settled2));
}
