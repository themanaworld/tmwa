// mimalloc instead of glibc malloc: the db_jobs backlog during a
// map-link congestion burst is a large transient (~1 GB per ~120k
// queued saves), and glibc's per-thread arenas never return it —
// RSS stayed elevated permanently in load testing. mimalloc hands
// freed memory back to the OS.
#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

pub mod auth;
pub mod config;
pub mod db;
pub mod import;
pub mod net;
pub mod proto;
pub mod serve;
