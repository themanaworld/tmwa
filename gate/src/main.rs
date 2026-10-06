use std::path::PathBuf;

use clap::{Parser, Subcommand};
use tmwa_gate::db::Db;
use tmwa_gate::import::{self, ImportFiles};

/// Parse `tmwa-gate admin` args into a JSON request. Passwords can
/// come from stdin (--password-stdin) instead of argv.
fn parse_admin_args(args: &[String]) -> (String, Vec<String>, Option<String>) {
    let mut rest: Vec<String> = Vec::new();
    let mut pw = None;
    for a in args.iter() {
        if a == "--password-stdin" {
            use std::io::BufRead;
            pw = std::io::stdin().lock().lines().next().and_then(|l| l.ok());
            continue;
        }
        rest.push(a.clone());
    }
    let cmd = rest.first().cloned().unwrap_or_default();
    (cmd, rest.into_iter().skip(1).collect(), pw)
}

/// print!-equivalent that fails instead of panicking on a broken
/// pipe, so stdin mode can't die mid-way through output.
fn writeln_line(s: &str) -> std::io::Result<()> {
    use std::io::Write;
    let mut o = std::io::stdout().lock();
    o.write_all(s.as_bytes())?;
    if !s.ends_with('\n') {
        o.write_all(b"\n")?;
    }
    o.flush()
}

/// Send one JSON line to the admin socket and print the reply.
async fn admin_cli(sock: &std::path::Path, args: &[String], json: bool) -> std::io::Result<()> {
    // stdin mode: one command per line, tmwa-admin style
    if args.is_empty() {
        use std::io::BufRead;
        let mut stdin = std::io::stdin().lock();
        loop {
            let mut line = String::new();
            if stdin.read_line(&mut line)? == 0 {
                break;
            }
            let line = line.trim();
            let parts: Vec<String> = line.split_whitespace().map(|s| s.to_string()).collect();
            if parts.is_empty() {
                continue;
            }
            let (cmd, cargs, pw) = parse_admin_args(&parts);
            if matches!(cmd.as_str(), "quit" | "exit" | "end" | "q") {
                let _ = writeln_line("Bye.");
                break;
            }
            // stdin mode never dies on a socket error: print an
            // error line (worded like tmwa-admin's login-server
            // connect failure) and continue with the next command
            let reply = match admin_request(sock, &cmd, &cargs, pw).await {
                Ok(r) => r,
                Err(e) => {
                    let _ = writeln_line(&format!(
                        "Impossible to have a connection with the gateway [{e}]"
                    ));
                    continue;
                }
            };
            let out = if json {
                format!("{reply}")
            } else if let Some(t) = reply.get("text").and_then(|t| t.as_str()) {
                t.to_string()
            } else if let Some(e) = reply.get("error").and_then(|t| t.as_str()) {
                format!("{e}\n")
            } else {
                format!("{reply}")
            };
            if writeln_line(&out).is_err() {
                return Ok(()); // stdout pipe closed: exit quietly
            }
        }
        return Ok(());
    }

    let (cmd, cargs, pw) = parse_admin_args(args);
    let reply = admin_request(sock, &cmd, &cargs, pw).await?;
    let ok = reply.get("ok").and_then(|v| v.as_bool()).unwrap_or(false);
    if json {
        println!("{reply}");
    } else if let Some(t) = reply.get("text").and_then(|t| t.as_str()) {
        print!("{t}");
    } else if let Some(e) = reply.get("error").and_then(|t| t.as_str()) {
        eprintln!("{e}");
    } else {
        println!("{reply}");
    }
    if !ok {
        std::process::exit(1);
    }
    Ok(())
}

async fn admin_request(
    sock: &std::path::Path,
    cmd: &str,
    args: &[String],
    password: Option<String>,
) -> std::io::Result<serde_json::Value> {
    use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
    let conn = tokio::net::UnixStream::connect(sock).await?;
    let (rd, mut wr) = conn.into_split();
    let mut req = serde_json::Map::new();
    req.insert("cmd".into(), cmd.into());
    req.insert("args".into(), serde_json::json!(args));
    if let Some(p) = password {
        req.insert("password".into(), p.into());
    }
    wr.write_all(serde_json::Value::from(req).to_string().as_bytes())
        .await?;
    wr.write_all(b"\n").await?;
    let mut lines = BufReader::new(rd).lines();
    let line = lines.next_line().await?.unwrap_or_else(|| "{}".into());
    Ok(serde_json::from_str(&line).unwrap_or(serde_json::json!({})))
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
        #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
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
