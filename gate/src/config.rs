//! `tmwa-gate serve --config gate.toml`
//!
//! Option names and defaults mirror tmwa's login_conf / char_conf /
//! inter_conf (tools/config.py); options that no longer apply
//! (login<->char link, LAN splitting, ladmin, file paths replaced by
//! the DB, account sex) are dropped.

use std::net::SocketAddr;
use std::path::PathBuf;

use serde::Deserialize;

#[derive(Debug, Deserialize, Default)]
#[serde(default)]
pub struct Config {
    pub gate: GateConf,
    pub login: LoginConf,
    #[serde(rename = "char")]
    pub char_: CharConf,
    pub inter: InterConf,
    pub map: MapConf,
}

fn default_listen() -> String {
    "0.0.0.0:6901".into()
}
fn default_map_listen() -> String {
    "127.0.0.1:6121".into()
}

#[derive(Debug, Deserialize)]
#[serde(default)]
pub struct GateConf {
    /// The single client-facing listen address.
    pub listen: String,
    /// Advertised to clients in 0x0069/0x0071.
    pub public_ip: String,
    pub public_port: u16,
    /// SQLite database path.
    pub db: PathBuf,
    /// tmwa gm_account.txt file, reloaded every
    /// gm_account_filename_check_timer seconds.
    pub gm_account_file: PathBuf,
    pub online_txt: PathBuf,
    pub online_html: PathBuf,
    /// Unix socket for the admin channel.
    pub admin_socket: PathBuf,
    /// How long a client may be held while its map server restarts.
    pub hold_timeout_secs: u64,
    /// Announcement sent once when a client's map server goes down.
    pub hold_message: String,
}

impl Default for GateConf {
    fn default() -> Self {
        GateConf {
            listen: default_listen(),
            public_ip: "127.0.0.1".into(),
            public_port: 6901,
            db: "gate.db".into(),
            gm_account_file: "save/gm_account.txt".into(),
            online_txt: "online.txt".into(),
            online_html: "online.html".into(),
            admin_socket: "tmwa-gate.sock".into(),
            hold_timeout_secs: 180,
            hold_message: "Server restarting, please wait.".into(),
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(default)]
pub struct MapConf {
    /// Listener for tmwa-map connections (localhost by default).
    pub listen: String,
    /// What tmwa-map sends in 0x2af8.
    pub userid: String,
    pub password: String,
}

impl Default for MapConf {
    fn default() -> Self {
        MapConf {
            listen: default_map_listen(),
            userid: String::new(),
            password: String::new(),
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(default)]
pub struct LoginConf {
    /// Allow `*_M` / `*_F` account creation on the login packet.
    pub new_account: bool,
    /// Sent as packet 0x0063 when the client flags bit0.
    pub update_host: String,
    /// Minimum GM level allowed to log in.
    pub min_level_to_connect: u32,
    /// IP ACL: order + allow/deny CIDR lists (tmwa semantics:
    /// `deny_allow` = deny list wins ties... mirrors login.cpp
    /// check_ip).
    pub order: String,
    pub allow: Vec<String>,
    pub deny: Vec<String>,
    /// Login flood protection.
    pub conn_limit_enable: bool,
    /// Seconds one IP may wait between login attempts.
    pub conn_limit_interval: u64,
    /// Seconds between gm_account_file mtime checks.
    pub gm_account_filename_check_timer: u64,
}

impl Default for LoginConf {
    fn default() -> Self {
        LoginConf {
            new_account: false,
            update_host: String::new(),
            min_level_to_connect: 0,
            order: "deny_allow".into(),
            allow: vec![],
            deny: vec![],
            conn_limit_enable: true,
            conn_limit_interval: 5,
            gm_account_filename_check_timer: 15,
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(default)]
pub struct CharConf {
    pub server_name: String,
    /// Refuse logins above this online count; 0 = unlimited.
    pub max_connect_user: i32,
    /// New characters start here: "map,x,y".
    pub start_point: String,
    /// Allowed characters in new names (all strings concatenated;
    /// tmwa takes multiple `char_name_letters:` lines).
    pub char_name_letters: Vec<String>,
    pub online_gm_display_min_level: u32,
    pub online_refresh_html: u32,
    pub max_hair_style: u16,
    pub max_hair_color: u16,
    pub min_stat_value: u16,
    pub max_stat_value: u16,
    pub total_stat_sum: u16,
    pub min_name_length: u16,
    pub char_slots: u16,
}

impl Default for CharConf {
    fn default() -> Self {
        CharConf {
            server_name: "The Mana World".into(),
            max_connect_user: 0,
            start_point: "001-1.gat,273,354".into(),
            char_name_letters: vec![],
            online_gm_display_min_level: 20,
            online_refresh_html: 20,
            max_hair_style: 20,
            max_hair_color: 11,
            min_stat_value: 1,
            max_stat_value: 9,
            total_stat_sum: 30,
            min_name_length: 4,
            char_slots: 9,
        }
    }
}

#[derive(Debug, Deserialize)]
#[serde(default)]
pub struct InterConf {
    /// Max level spread for party exp share.
    pub party_share_level: u32,
}

impl Default for InterConf {
    fn default() -> Self {
        InterConf {
            party_share_level: 10,
        }
    }
}

impl Config {
    pub fn load(path: &std::path::Path) -> Result<Config, Box<dyn std::error::Error>> {
        let text = std::fs::read_to_string(path)?;
        Ok(toml::from_str(&text)?)
    }

    pub fn listen_addr(&self) -> Result<SocketAddr, std::net::AddrParseError> {
        self.gate.listen.parse()
    }

    pub fn map_listen_addr(&self) -> Result<SocketAddr, std::net::AddrParseError> {
        self.map.listen.parse()
    }

    /// "map,x,y" -> (map, x, y); falls back to the tmwa default.
    pub fn start_point(&self) -> (String, i16, i16) {
        let mut it = self.char_.start_point.split(',');
        let m = it.next().unwrap_or("001-1.gat").to_string();
        let x = it.next().and_then(|s| s.parse().ok()).unwrap_or(273);
        let y = it.next().and_then(|s| s.parse().ok()).unwrap_or(354);
        (m, x, y)
    }

    pub fn name_letters(&self) -> std::collections::BTreeSet<u8> {
        self.char_
            .char_name_letters
            .iter()
            .flat_map(|s| s.bytes())
            .collect()
    }
}
