use std::path::PathBuf;

use clap::{Parser, Subcommand};
use tmwa_gate::db::Db;
use tmwa_gate::import::{self, ImportFiles};

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
        Command::Admin { json, args } => {
            println!("admin (json={json:?}, args={args:?}): not implemented yet")
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
