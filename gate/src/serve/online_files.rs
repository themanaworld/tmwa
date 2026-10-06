//! online.txt / online.html rendering (port of char.cpp
//! create_online_files) plus the shared UTC timestamp formatter.

use std::fmt::Write as _;

use super::{State, is_gm};
use crate::proto::Opt0;

/// Write online.txt / online.html (port of char.cpp
/// create_online_files).
pub(super) async fn write_online_files(st: &State) {
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
    let mut names: Vec<(u32, String)> = Vec::new();
    {
        let online = st.online.lock().unwrap();
        let chars = st.chars.lock().unwrap();
        for &cid in online.keys() {
            if let Some(c) = chars.get(&cid) {
                if c.data.option.0 & Opt0::HIDE != 0 {
                    continue;
                }
                let gm = is_gm(st, c.key.account_id.0);
                names.push((gm, c.key.name.to_string_lossy()));
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
    for (gm, name) in &names {
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
pub(crate) fn format_time(secs: u64) -> String {
    chrono::DateTime::from_timestamp(secs as i64, 0)
        .map(|t| t.format("%Y-%m-%d %H:%M:%S").to_string())
        .unwrap_or_default()
}
