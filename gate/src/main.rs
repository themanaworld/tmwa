use std::path::PathBuf;

use clap::{Parser, Subcommand};
use tmwa_gate::db::Db;
use tmwa_gate::import::{self, ImportFiles};

/// Send one JSON line to the admin socket and print the reply.
async fn admin_cli(sock: &std::path::Path, args: &[String], _json: bool) -> std::io::Result<()> {
    use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
    let mut req = serde_json::Map::new();
    match args {
        [cmd] if cmd == "status" => {
            req.insert("cmd".into(), "status".into());
        }
        [cmd, flag] if cmd == "drain" && flag == "--wait" => {
            req.insert("cmd".into(), "drain".into());
            req.insert("wait".into(), true.into());
        }
        [cmd] if cmd == "drain" => {
            req.insert("cmd".into(), "drain".into());
        }
        _ => {
            eprintln!("admin: unknown command {args:?} (status|drain [--wait])");
            std::process::exit(1);
        }
    }
    let conn = tokio::net::UnixStream::connect(sock).await?;
    let (rd, mut wr) = conn.into_split();
    wr.write_all(serde_json::Value::from(req).to_string().as_bytes())
        .await?;
    wr.write_all(b"\n").await?;
    wr.shutdown().await.ok();
    let mut lines = BufReader::new(rd).lines();
    while let Some(line) = lines.next_line().await? {
        println!("{line}");
    }
    Ok(())
}

#[derive(Parser)]
#[command(name = "tmwa-gate", about = "TMWA client gateway")]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Subcommand)]
enum Command {
    /// Run the gateway (login, char, map relay, HTTP).
    /// Run the gateway (login, char, map relay).
    Serve {
        /// Path to gate.toml.
        #[arg(long)]
        config: PathBuf,
    },
    /// Talk to a running gateway over its admin socket.
    Admin {
        /// Emit JSON where applicable.
        #[arg(long)]
        json: bool,
        /// Admin unix socket path (or `--config` to read it from
        /// gate.toml's [gate] admin_socket).
        #[arg(long)]
        socket: Option<PathBuf>,
        /// Path to gate.toml (to find the admin socket).
        #[arg(long)]
        config: Option<PathBuf>,
        /// Admin command and arguments, e.g. `drain`.
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },
    /// Import tmwa's flat save files into the SQLite database.
    Import {
        /// Database file to create.
        #[arg(long)]
        db: PathBuf,
        /// Directory containing the tmwa save files (one dir holding
        /// all of them).
        #[arg(long)]
        save_dir: PathBuf,
        #[arg(long)]
        account_txt: Option<PathBuf>,
        #[arg(long)]
        athena_txt: Option<PathBuf>,
        #[arg(long)]
        party_txt: Option<PathBuf>,
        #[arg(long)]
        storage_txt: Option<PathBuf>,
        #[arg(long)]
        accreg_txt: Option<PathBuf>,
    },
}

fn main() {
    tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
        )
        .init();
    let cli = Cli::parse();
    match cli.command {
        Command::Serve { config } => {
            let cfg = match tmwa_gate::config::Config::load(&config) {
                Ok(c) => c,
                Err(e) => {
                    eprintln!("cannot load {}: {e}", config.display());
                    std::process::exit(1);
                }
            };
            let rt = tokio::runtime::Runtime::new().unwrap();
            if let Err(e) = rt.block_on(tmwa_gate::serve::run(cfg)) {
                eprintln!("serve: {e}");
                std::process::exit(1);
            }
        }
        Command::Admin {
            json,
            socket,
            config,
            args,
        } => {
            let sock = socket.or_else(|| {
                config.and_then(|c| {
                    tmwa_gate::config::Config::load(&c)
                        .ok()
                        .map(|cfg| cfg.gate.admin_socket)
                })
            });
            let Some(sock) = sock else {
                eprintln!("admin: need --socket or --config");
                std::process::exit(1);
            };
            let rt = tokio::runtime::Runtime::new().unwrap();
            if let Err(e) = rt.block_on(admin_cli(&sock, &args, json)) {
                eprintln!("admin: {e}");
                std::process::exit(1);
            }
        }
        Command::Import {
            db,
            save_dir,
            account_txt,
            athena_txt,
            party_txt,
            storage_txt,
            accreg_txt,
        } => {
            let files = ImportFiles {
                save_dir,
                account_txt,
                athena_txt,
                party_txt,
                storage_txt,
                accreg_txt,
            };
            let db = match Db::open(&db) {
                Ok(d) => d,
                Err(e) => {
                    eprintln!("cannot open {}: {e}", db.display());
                    std::process::exit(1);
                }
            };
            match import::run(&files, &db, |m| println!("{m}")) {
                Ok(s) => {
                    println!(
                        "imported: {} accounts, {} characters, {} parties, \
                         {} storage items, {} accreg vars",
                        s.accounts, s.characters, s.parties, s.storage_entries, s.vars
                    );
                    println!("password hashing took {:.2}s", s.password_seconds);
                    for l in &s.skipped {
                        println!("skipped: {l}");
                    }
                }
                Err(e) => {
                    eprintln!("import failed: {e}");
                    std::process::exit(1);
                }
            }
        }
    }
}
