use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "tmwa-gate", about = "TMWA client gateway")]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Subcommand)]
enum Command {
    /// Run the gateway (login, char, map relay, HTTP).
    Serve,
    /// Talk to a running gateway over its admin socket.
    Admin {
        /// Emit JSON where applicable.
        #[arg(long)]
        json: bool,
        /// Admin command and arguments, e.g. `drain`.
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },
}

fn main() {
    let cli = Cli::parse();
    match cli.command {
        Command::Serve => println!("serve: not implemented yet"),
        Command::Admin { json, args } => {
            println!("admin (json={json:?}, args={args:?}): not implemented yet")
        }
    }
}
