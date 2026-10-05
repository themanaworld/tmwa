//! HTTP API — a Rust port of tmw-api's /api/tmwa router plus the
//! gate's WebSocket client path (see ws.rs for the upgrade handler).
//!
//! Behaviour mirrors ~/projects/tmw/api/src/routers/tmwa/*:
//!   GET  {base}/server            — JSON GameServer status
//!   POST {base}/account           — create account (captcha)
//!   PUT  {base}/account           — password reset, two stages
//!
//! {base} defaults to "/api/tmwa" so existing consumers work.

use std::collections::HashMap;
use std::net::IpAddr;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use axum::Json;
use axum::body::{Body, Bytes};
use axum::extract::{ConnectInfo, DefaultBodyLimit, Request, State as AxState};
use axum::http::{HeaderMap, StatusCode};
use axum::middleware::Next;
use axum::response::{IntoResponse, Response};
use axum::routing::{get, post};

use serde_json::{Value, json};

use super::state::State;

static RE_USER: std::sync::LazyLock<regex::Regex> =
    std::sync::LazyLock::new(|| regex::Regex::new(r"^[a-zA-Z0-9]{4,23}$").unwrap());
static RE_EMAIL: std::sync::LazyLock<regex::Regex> = std::sync::LazyLock::new(|| {
    regex::Regex::new(
            r"^(?:[a-zA-Z0-9.$&+=_~-]{1,34}@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,35}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,34}[a-zA-Z0-9])?){0,9})$",
        )
        .unwrap()
});
static RE_CODE: std::sync::LazyLock<regex::Regex> =
    std::sync::LazyLock::new(|| regex::Regex::new(r"^[a-zA-Z0-9-_]{6,128}$").unwrap());
static RE_EMAIL_OPT: std::sync::LazyLock<regex::Regex> = std::sync::LazyLock::new(|| {
    regex::Regex::new(
        r"^(|(?:[a-zA-Z0-9.$&+=_~-]{1,34}@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,35}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,34}[a-zA-Z0-9])?){0,9}))$",
    )
    .unwrap()
});
static RE_TOKEN: std::sync::LazyLock<regex::Regex> =
    std::sync::LazyLock::new(|| regex::Regex::new(r"^[a-zA-Z0-9-_]{20,4000}$").unwrap());

const MAX_DANGER: u32 = 5;
const BAN: Duration = Duration::from_secs(6 * 3600);

pub(crate) struct RateState {
    /// per-route, per-ip cooldown expiry
    limiters: HashMap<String, HashMap<IpAddr, Instant>>,
    /// per-ip offence score
    bad: HashMap<IpAddr, (u32, Instant)>,
}

impl RateState {
    /// True while an active cooldown or ban applies; writes a 429/418
    /// reply. Mirrors limiter.js's checkRateLimiter.
    fn check(&mut self, route: &str, ip: IpAddr) -> Option<(StatusCode, Value, u64)> {
        let now = Instant::now();
        if let Some(t) = self
            .limiters
            .get(route)
            .and_then(|m| m.get(&ip))
            .filter(|t| **t > now)
        {
            let left = t.duration_since(now).as_secs().max(1);
            return Some((
                StatusCode::TOO_MANY_REQUESTS,
                json!({"status":"error","error":"too many requests","retry":left}),
                left,
            ));
        }
        if let Some((lvl, _)) = self.bad.get(&ip) {
            if *lvl >= MAX_DANGER {
                return Some((
                    StatusCode::IM_A_TEAPOT,
                    json!({"status":"error","error":"banned"}),
                    0,
                ));
            }
        }
        None
    }

    /// Apply a cooldown; >=5 min counts as a bad-actor offence
    /// (ban after MAX_DANGER, decays after BAN_HOURS), like limiter.js.
    fn cooldown(&mut self, route: &str, ip: IpAddr, ms: u64) {
        let now = Instant::now();
        // lazily bound map growth: drop expired entries while here
        for m in self.limiters.values_mut() {
            m.retain(|_, t| *t > now);
        }
        self.limiters.retain(|_, m| !m.is_empty());
        self.bad.retain(|_, (_, t)| *t > now);
        self.limiters
            .entry(route.to_string())
            .or_default()
            .insert(ip, now + Duration::from_millis(ms));
        if ms >= 300_000 {
            let (lvl, _) = *self.bad.get(&ip).unwrap_or(&(0, now));
            self.bad.insert(ip, (lvl + 1, now + BAN));
        }
    }
}

pub struct HttpState {
    pub st: Arc<State>,
    pub(crate) rate: Mutex<RateState>,
    http_client: reqwest::Client,
}

impl HttpState {
    pub fn new(st: Arc<State>) -> Self {
        HttpState {
            st,
            rate: Mutex::new(RateState {
                limiters: HashMap::new(),
                bad: HashMap::new(),
            }),
            http_client: reqwest::Client::builder()
                .timeout(Duration::from_secs(10))
                .build()
                .unwrap_or_default(),
        }
    }
}

fn jserr(code: StatusCode, error: &str) -> Response {
    (code, Json(json!({"status":"error","error":error}))).into_response()
}

fn route_key(req: &Request) -> String {
    format!("{}{}", req.method(), req.uri().path())
}

/// The tmw-api shape: `{"status":"error","error":"..."}` with a real
/// status code.
fn api_error(status: StatusCode, error: &str) -> Response {
    jserr(status, error)
}

/// POST create: validate, reject duplicates, store argon2id, mail.
///
/// The body is buffered raw and parsed by hand (not via the Json
/// extractor): tmw-api accepts any content type, and a malformed
/// body is a chargeable offence (5 min cooldown), not a rejection.
async fn create_account(
    AxState(hs): AxState<Arc<HttpState>>,
    ConnectInfo(peer): ConnectInfo<std::net::SocketAddr>,
    headers: HeaderMap,
    body: Bytes,
) -> Response {
    let ip = crate::net::forwarded_for(
        peer.ip(),
        headers.get("x-forwarded-for").and_then(|v| v.to_str().ok()),
        &hs.st.cfg.http.trusted_proxies,
    );
    let route = "POST/account";
    if let Some((c, j, retry)) = hs.rate.lock().unwrap().check(route, ip) {
        let mut r = (c, Json(j)).into_response();
        if retry > 0 {
            r.headers_mut()
                .insert("Retry-After", retry.to_string().parse().unwrap());
        }
        return r;
    }
    let st = &hs.st;
    let cooldown = |ms: u64| hs.rate.lock().unwrap().cooldown(route, ip, ms);

    let Ok(b) = serde_json::from_slice::<Value>(&body) else {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    };
    let get = |k: &str| b.get(k).and_then(|v| v.as_str()).unwrap_or("");
    let (user, pass, email) = (get("username"), get("password"), get("email"));
    let ok = RE_USER.is_match(user)
        && RE_USER.is_match(pass)
        && RE_EMAIL_OPT.is_match(email)
        && email.len() < 40;
    if !ok {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    }

    let (user, pass, email) = (user.to_string(), pass.to_string(), email.to_string());
    let uname = user.clone();
    let exists = st
        .db
        .blocking(move |db| db.account_id_by_name(&uname).unwrap_or(None).is_some())
        .await;
    if exists {
        cooldown(2_000);
        return api_error(StatusCode::CONFLICT, "already exists");
    }

    let email2 = if email.len() >= 3 {
        email.clone()
    } else {
        "a@a.com".to_string()
    };
    let Ok(h) = crate::auth::password::hash_argon2id(pass.as_bytes()) else {
        return api_error(StatusCode::INTERNAL_SERVER_ERROR, "hashing failed");
    };
    // Same allocator as in-game _M/_F and admin create: the
    // next_account_id meta counter (login.cpp's account_id_count),
    // bumped inside the insert transaction. MAX(id)+1 would diverge
    // from the counter and wedge every create on the PK.
    let uname = user.clone();
    let created = st
        .db
        .blocking_conn(move |c| crate::db::Db::create_account(c, &uname, &h, &email2))
        .await;
    match created {
        Ok(_id) => {}
        // raced another create of the same name
        Err(crate::db::DbError::NameTaken) => {
            cooldown(2_000);
            return api_error(StatusCode::CONFLICT, "already exists");
        }
        Err(e) => {
            tracing::warn!("http create_account: {e}");
            cooldown(2_000);
            return api_error(StatusCode::INTERNAL_SERVER_ERROR, "create failed");
        }
    }
    cooldown(299_000);
    if email != "a@a.com" {
        send_mail(
            st,
            &email,
            "The Mana World account registration",
            &format!(
                "Your account (\"{user}\") was created successfully.\nHave fun playing The Mana World!"
            ),
        );
    }
    (StatusCode::CREATED, Json(json!({"status":"success"}))).into_response()
}

/// PUT: two-stage password reset (request by email → code → set new
/// password), backed by the password_resets table (1 h expiry, one
/// pending per email).
async fn reset_password(
    AxState(hs): AxState<Arc<HttpState>>,
    ConnectInfo(peer): ConnectInfo<std::net::SocketAddr>,
    headers: HeaderMap,
    body: Bytes,
) -> Response {
    let ip = crate::net::forwarded_for(
        peer.ip(),
        headers.get("x-forwarded-for").and_then(|v| v.to_str().ok()),
        &hs.st.cfg.http.trusted_proxies,
    );
    let route = "PUT/account";
    if let Some((c, j, retry)) = hs.rate.lock().unwrap().check(route, ip) {
        let mut r = (c, Json(j)).into_response();
        if retry > 0 {
            r.headers_mut()
                .insert("Retry-After", retry.to_string().parse().unwrap());
        }
        return r;
    }
    let st = &hs.st;
    let cooldown = |ms: u64| hs.rate.lock().unwrap().cooldown(route, ip, ms);

    let Ok(b) = serde_json::from_slice::<Value>(&body) else {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    };
    let get = |k: &str| b.get(k).and_then(|v| v.as_str()).unwrap_or("").to_string();
    let (email, user, pass, code) = (get("email"), get("username"), get("password"), get("code"));

    // stage 1: email-only body → find accounts, mail a reset code
    if !email.is_empty() && user.is_empty() {
        if !RE_EMAIL.is_match(&email) || email.len() < 3 || email.len() >= 40 || email == "a@a.com"
        {
            cooldown(300_000);
            return api_error(StatusCode::BAD_REQUEST, "malformed request");
        }
        let em = email.clone();
        let accounts: Vec<(i64, String)> = st.db.blocking_conn(move |c| {
            let mut q = c
                .prepare("SELECT id,name FROM accounts WHERE email=?1")
                .unwrap();
            q.query_map([&em], |r| Ok((r.get::<_, i64>(0)?, r.get::<_, String>(1)?)))
                .unwrap()
                .flatten()
                .collect()
        })
        .await;
        if accounts.is_empty() {
            cooldown(8_000);
            return api_error(StatusCode::NOT_FOUND, "no accounts found");
        }
        // one pending reset per email: any account row for it
        let em2 = email.clone();
        let pending: bool = st.db.blocking_conn(move |c| {
            c.query_row(
                "SELECT COUNT(*) FROM password_resets r JOIN accounts a ON a.id=r.account_id                  WHERE a.email=?1 AND r.expires_at > strftime('%s','now')*1000",
                [&em2],
                |r| r.get::<_, i64>(0),
            )
            .unwrap_or(0)
                > 0
        })
        .await;
        if pending {
            cooldown(5_000);
            return api_error(
                StatusCode::from_u16(425).unwrap(),
                "operation already pending",
            );
        }
        let Some(code) = uuid() else {
            return api_error(StatusCode::INTERNAL_SERVER_ERROR, "token mint failed");
        };
        let names: String = accounts.iter().map(|a| format!("{}\n", a.1)).collect();
        let reset_url = st.cfg.http.reset_url.clone();
        let code2 = code.clone();
        let exp = chrono::Utc::now().timestamp_millis() + 3_600_000;
        let acct_ids: Vec<i64> = accounts.iter().map(|a| a.0).collect();
        st.db.blocking_conn(move |c| {
            for aid in &acct_ids {
                c.execute(
                    "INSERT INTO password_resets (code,account_id,expires_at) VALUES (?1,?2,?3)",
                    rusqlite::params![code2, aid, exp],
                )
                .unwrap();
            }
        })
        .await;
        send_mail(
            st,
            &email,
            "The Mana World password reset",
            &format!(
                "You are receiving this email because someone (you?) has requested a password reset on The Mana World \
                 with your email address.\nIf you did not request a password reset please ignore this email.\n\n\
                 The following Legacy accounts are associated with this email address:\n{names}\n\
                 To proceed with the password reset:\n{reset_url}{code}"
            ),
        );
        cooldown(8_000);
        return (StatusCode::OK, Json(json!({"status":"success"}))).into_response();
    }

    // username-only → not implemented, like tmw-api
    if !user.is_empty() && pass.is_empty() && code.is_empty() {
        if RE_USER.is_match(&user) {
            return api_error(StatusCode::NOT_IMPLEMENTED, "not yet implemented");
        }
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    }

    // stage 2: username + password + code
    if !(RE_USER.is_match(&user) && RE_USER.is_match(&pass) && RE_CODE.is_match(&code)) {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    }
    let code2 = code.clone();
    let rows: Vec<(i64, i64, String)> = st.db.blocking_conn(move |c| {
        let mut q = c
            .prepare(
                "SELECT r.account_id, r.expires_at, COALESCE(a.email,'')                      FROM password_resets r JOIN accounts a ON a.id=r.account_id                      WHERE r.code=?1",
            )
            .unwrap();
        q.query_map([&code2], |r| {
            Ok((
                r.get::<_, i64>(0)?,
                r.get::<_, i64>(1)?,
                r.get::<_, String>(2)?,
            ))
        })
        .unwrap()
        .flatten()
        .collect()
    })
    .await;
    let Some((_aid0, exp, r_email)) = rows.first().cloned() else {
        cooldown(300_000);
        return api_error(StatusCode::REQUEST_TIMEOUT, "request expired");
    };
    if exp < chrono::Utc::now().timestamp_millis() {
        cooldown(300_000);
        let code3 = code.clone();
        st.db.blocking_conn(move |c| {
            let _ = c.execute("DELETE FROM password_resets WHERE code=?1", [&code3]);
        })
        .await;
        return api_error(StatusCode::REQUEST_TIMEOUT, "request expired");
    }
    // the account must be one of the code's accounts
    let u = user.clone();
    let named_id: Option<i64> = st
        .db
        .blocking(move |db| db.account_id_by_name(&u).unwrap_or(None))
        .await;
    let Some(acct_id) = named_id.filter(|id| rows.iter().any(|r| r.0 == *id)) else {
        cooldown(300_000);
        let code3 = code.clone();
        st.db.blocking_conn(move |c| {
            let _ = c.execute("DELETE FROM password_resets WHERE code=?1", [&code3]);
        })
        .await;
        return api_error(StatusCode::UNAUTHORIZED, "foreign account");
    };
    let Ok(h) = crate::auth::password::hash_argon2id(pass.as_bytes()) else {
        return api_error(StatusCode::INTERNAL_SERVER_ERROR, "hashing failed");
    };
    st.db.blocking(move |db| {
        let _ = db.set_password(acct_id, &h, crate::auth::password::Scheme::Argon2id, None);
        let _ = db.with_conn(|c| c.execute("DELETE FROM password_resets WHERE code=?1", [&code]));
    })
    .await;
    cooldown(299_000);
    send_mail(
        st,
        &r_email,
        "The Mana World password reset",
        &format!(
            "You have successfully reset the password for Legacy account \"{user}\".\nHave fun playing The Mana World!\n\n\u{26a0} If you did not perform this password reset, please contact us ASAP to secure your account."
        ),
    );
    (StatusCode::OK, Json(json!({"status":"success"}))).into_response()
}

/// Drop expired cooldowns, bad actors and password_resets rows.
/// Runs on a 10 min timer; also keeps the in-memory maps bounded.
pub(crate) async fn prune(st: &Arc<State>) {
    let now = Instant::now();
    let now_ms = chrono::Utc::now().timestamp_millis();
    // (RateState lives in HttpState which isn't reachable from here;
    // prune the DB side here and bound the maps inside the handlers.)
    st.db.blocking_conn(move |c| {
        let n = c
            .execute(
                "DELETE FROM password_resets WHERE expires_at < ?1",
                [now_ms],
            )
            .unwrap_or(0);
        if n > 0 {
            tracing::info!("http: pruned {n} expired password reset(s)");
        }
    })
    .await;
    let _ = now;
}

fn uuid() -> Option<String> {
    // v4-style random token. A getrandom failure aborts the mint:
    // random_u32's contract forbids falling back to predictable bits,
    // so there is no unwrap_or path here at all.
    let mut b = [0u8; 16];
    getrandom::fill(&mut b).ok()?;
    b[6] = (b[6] & 0x0f) | 0x40; // version 4
    b[8] = (b[8] & 0x3f) | 0x80; // variant 10
    let h: String = b.iter().map(|x| format!("{x:02x}")).collect();
    Some(format!(
        "{}-{}-{}-{}-{}",
        &h[0..8],
        &h[8..12],
        &h[12..16],
        &h[16..20],
        &h[20..32]
    ))
}

fn send_mail(st: &Arc<State>, to: &str, subject: &str, text: &str) {
    let from = st.cfg.http.mailer_from.clone();
    let path = st.cfg.http.sendmail.clone();
    let to = to.to_string();
    let subject = subject.to_string();
    let text = text.to_string();
    tokio::task::spawn_blocking(move || {
        use std::io::Write;
        let mail = format!("From: {from}\nTo: {to}\nSubject: {subject}\n\n{text}\n");
        let p = std::process::Command::new(&path)
            .arg("-i")
            .arg("--")
            .arg(&to)
            .stdin(std::process::Stdio::piped())
            .spawn();
        match p {
            Ok(mut p) => {
                if let Some(mut i) = p.stdin.take() {
                    let _ = i.write_all(mail.as_bytes());
                }
                let _ = p.wait();
            }
            Err(e) => tracing::warn!("sendmail: {e}"),
        }
    });
}

async fn server(AxState(hs): AxState<Arc<HttpState>>) -> impl IntoResponse {
    let st = &hs.st;
    let online = st.count_users() as u64;
    let status = if st.map_count() > 0 {
        "Online"
    } else {
        "OfflineTemporarily"
    };
    (
        [("Cache-Control", "public, max-age=5")],
        Json(json!({
            "@context": "http://schema.org",
            "@type": "GameServer",
            "name": st.cfg.http.name,
            "url": st.cfg.http.url,
            "playersOnline": online,
            "serverStatus": status,
        })),
    )
}

/// Captcha middleware — tmw-api's checkCaptcha on /account.
async fn captcha(
    AxState(hs): AxState<Arc<HttpState>>,
    ConnectInfo(peer): ConnectInfo<std::net::SocketAddr>,
    req: Request<Body>,
    next: Next,
) -> Response {
    let ip = crate::net::forwarded_for(
        peer.ip(),
        req.headers()
            .get("x-forwarded-for")
            .and_then(|v| v.to_str().ok()),
        &hs.st.cfg.http.trusted_proxies,
    );
    let token = req
        .headers()
        .get("x-captcha-token")
        .and_then(|v| v.to_str().ok())
        .unwrap_or("")
        .to_string();
    if !RE_TOKEN.is_match(&token) {
        hs.rate
            .lock()
            .unwrap()
            .cooldown(&route_key(&req), ip, 300_000);
        return api_error(StatusCode::FORBIDDEN, "no token sent");
    }
    if !hs.st.cfg.http.captcha {
        return next.run(req).await;
    }
    // verify with Google's siteverify
    let secret = hs.st.cfg.http.recaptcha_secret.clone();
    let url =
        format!("https://www.google.com/recaptcha/api/siteverify?secret={secret}&response={token}");
    match hs.http_client.get(&url).send().await {
        Ok(r) => {
            let ok = r
                .json::<Value>()
                .await
                .ok()
                .and_then(|v| v.get("success").and_then(|s| s.as_bool()))
                .unwrap_or(false);
            if ok {
                next.run(req).await
            } else {
                api_error(StatusCode::FORBIDDEN, "invalid token")
            }
        }
        Err(_) => api_error(StatusCode::FORBIDDEN, "captcha check failed"),
    }
}

pub fn router(hs: Arc<HttpState>) -> axum::Router {
    let base = hs.st.cfg.http.base.clone();
    let account = axum::Router::new()
        .route(
            &format!("{base}/account"),
            post(create_account).put(reset_password),
        )
        // same 1 MB cap the old manual to_bytes used; other methods
        // get tmw-api's 404, not axum's 405
        .route_layer(DefaultBodyLimit::max(1 << 20))
        .route_layer(axum::middleware::from_fn_with_state(hs.clone(), captcha))
        .method_not_allowed_fallback(|| async { jserr(StatusCode::NOT_FOUND, "not found") });
    axum::Router::new()
        .route(&format!("{base}/server"), get(server))
        .merge(account)
        .route(&hs.st.cfg.http.ws_path, get(super::ws::handle_ws))
        .fallback(|| async { jserr(StatusCode::NOT_FOUND, "not found") })
        .with_state(hs)
}

/// Run the HTTP listener; installs the WS handler route too.
pub async fn run(st: Arc<State>) -> std::io::Result<()> {
    let listen = st.cfg.http.listen.clone();
    let r = router(Arc::new(HttpState::new(st)));
    let lis = tokio::net::TcpListener::bind(&listen).await?;
    tracing::info!("http on {listen}");
    axum::serve(
        lis,
        r.into_make_service_with_connect_info::<std::net::SocketAddr>(),
    )
    .await
    .map_err(|e| std::io::Error::other(e.to_string()))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::Config;

    fn test_state() -> (Arc<HttpState>, Arc<State>) {
        let db = Arc::new(crate::db::Db::open_memory().unwrap());
        db.with_conn(|c| crate::db::set_meta_conn(c, "next_account_id", 2000000))
            .unwrap();
        let st = Arc::new(State::new(Config::default(), db));
        (Arc::new(HttpState::new(st.clone())), st)
    }

    fn acct_body(user: &str, pass: &str, email: &str) -> Bytes {
        serde_json::to_vec(&json!({"username": user, "password": pass, "email": email}))
            .unwrap()
            .into()
    }

    /// Account creation must draw from the next_account_id meta
    /// counter (like in-game _M/_F and admin create), not MAX(id)+1.
    #[tokio::test]
    async fn create_uses_meta_allocator() {
        let (hs, st) = test_state();
        let r = create_account(
            AxState(hs.clone()),
            ConnectInfo("127.0.0.1:1".parse().unwrap()),
            HeaderMap::new(),
            acct_body("testuser", "testpass", "a@a.com"),
        )
        .await;
        assert_eq!(r.status(), StatusCode::CREATED);
        assert_eq!(st.db.meta("next_account_id").unwrap(), Some(2000001));

        // A same-name create from another ip is a conflict and does
        // not consume an id.
        let r = create_account(
            AxState(hs.clone()),
            ConnectInfo("10.9.8.7:1".parse().unwrap()),
            HeaderMap::new(),
            acct_body("testuser", "testpass", "a@a.com"),
        )
        .await;
        assert_eq!(r.status(), StatusCode::CONFLICT);
        assert_eq!(st.db.meta("next_account_id").unwrap(), Some(2000001));
    }

    /// The reset token is a v4-shaped uuid; a fresh mint never
    /// repeats.
    #[test]
    fn uuid_shape() {
        let a = uuid().unwrap();
        let b = uuid().unwrap();
        assert_ne!(a, b);
        assert_eq!(a.len(), 36);
        assert_eq!(&a[14..15], "4");
        assert!("89ab".contains(&a[19..20]));
        assert!(RE_CODE.is_match(&a));
    }
}
