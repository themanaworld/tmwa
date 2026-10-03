// mimalloc instead of glibc malloc: the db_jobs backlog during a
// map-link congestion burst is a large transient (~1 GB per ~120k
// queued saves), and glibc's per-thread arenas never return it —
// RSS stayed elevated permanently in load testing. mimalloc hands
// freed memory back to the OS.
#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

#[allow(dead_code)]
pub mod auth;
#[allow(dead_code)]
pub mod config;
#[allow(dead_code)]
pub mod db;
#[allow(dead_code)]
pub mod import;
#[allow(dead_code)]
pub mod net;
#[allow(dead_code)]
pub mod proto;
#[allow(dead_code)]
pub mod serve;
