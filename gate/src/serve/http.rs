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
use axum::body::Body;
use axum::extract::{ConnectInfo, Request, State as AxState};
use axum::http::{HeaderMap, StatusCode};
use axum::middleware::Next;
use axum::response::{IntoResponse, Response};
use axum::routing::{any, get};

use serde_json::{Value, json};

use super::state::State;

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
}

impl HttpState {
    pub fn new(st: Arc<State>) -> Self {
        HttpState {
            st,
            rate: Mutex::new(RateState {
                limiters: HashMap::new(),
                bad: HashMap::new(),
            }),
        }
    }
}

fn client_ip(headers: &HeaderMap, peer: IpAddr, st: &State) -> IpAddr {
    // trust X-Forwarded-For only from a configured proxy
    if st
        .cfg
        .http
        .trusted_proxies
        .iter()
        .any(|p| p.parse::<IpAddr>().map(|t| t == peer).unwrap_or(false))
    {
        if let Some(xff) = headers.get("x-forwarded-for") {
            if let Ok(s) = xff.to_str() {
                if let Some(first) = s.split(',').next() {
                    if let Ok(ip) = first.trim().parse() {
                        return ip;
                    }
                }
            }
        }
    }
    peer
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
async fn create_account(
    AxState(hs): AxState<Arc<HttpState>>,
    ConnectInfo(peer): ConnectInfo<std::net::SocketAddr>,
    headers: HeaderMap,
    body: Result<Json<Value>, serde_json::Error>,
) -> Response {
    let ip = client_ip(&headers, peer.ip(), &hs.st);
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

    let Ok(Json(b)) = body else {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    };
    let get = |k: &str| b.get(k).and_then(|v| v.as_str()).unwrap_or("");
    let (user, pass, email) = (get("username"), get("password"), get("email"));
    let user_ok = regex::Regex::new(r"^[a-zA-Z0-9]{4,23}$")
        .unwrap()
        .is_match(user);
    let pass_ok = regex::Regex::new(r"^[a-zA-Z0-9]{4,23}$")
        .unwrap()
        .is_match(pass);
    let email_re = regex::Regex::new(
        r"^$|^(?:[a-zA-Z0-9.$&+=_~-]{1,34}@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,35}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,34}[a-zA-Z0-9])?){0,9})$",
    )
    .unwrap();
    let email_ok = email_re.is_match(email) && email.len() < 40;
    if !(user_ok && pass_ok && email_ok) {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    }

    let (user, pass, email) = (user.to_string(), pass.to_string(), email.to_string());
    let uname = user.clone();
    let exists = super::admin::run_db(st, move |c| {
        c.query_row(
            "SELECT COUNT(*) FROM accounts WHERE name=?1",
            [&uname],
            |r| r.get::<_, i64>(0),
        )
        .unwrap_or(0)
            > 0
    })
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
    let uname = user.clone();
    let created = super::admin::run_db(st, move |c| {
        let h = crate::auth::password::hash_argon2id(pass.as_bytes())
            .map_err(|_| ())?;
        let id: i64 = c
            .query_row(
                "SELECT COALESCE(MAX(id)+1, 2000000) FROM accounts",
                [],
                |r| r.get(0),
            )
            .map_err(|_| ())?;
        c.execute(
            "INSERT INTO accounts (id,name,password_hash,password_scheme,email,state,ban_until,memo,last_login,login_count,last_ip,created_at) \
             VALUES (?1,?2,?3,'argon2id',?4,0,0,'',0,0,0,strftime('%s','now')*1000)",
            rusqlite::params![id, uname, h, email2],
        )
        .map_err(|_| ())?;
        Ok::<i64, ()>(id)
    })
    .await;
    let Ok(_id) = created else {
        cooldown(2_000);
        return api_error(StatusCode::INTERNAL_SERVER_ERROR, "create failed");
    };
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
    body: Result<Json<Value>, serde_json::Error>,
) -> Response {
    let ip = client_ip(&headers, peer.ip(), &hs.st);
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

    let Ok(Json(b)) = body else {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    };
    let get = |k: &str| b.get(k).and_then(|v| v.as_str()).unwrap_or("").to_string();
    let (email, user, pass, code) = (get("email"), get("username"), get("password"), get("code"));

    let email_re = regex::Regex::new(
        r"^(?:[a-zA-Z0-9.$&+=_~-]{1,34}@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,35}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,34}[a-zA-Z0-9])?){0,9})$",
    )
    .unwrap();
    let user_re = regex::Regex::new(r"^[a-zA-Z0-9]{4,23}$").unwrap();

    // stage 1: email-only body → find accounts, mail a reset code
    if !email.is_empty() && user.is_empty() {
        if !email_re.is_match(&email) || email.len() < 3 || email.len() >= 40 || email == "a@a.com"
        {
            cooldown(300_000);
            return api_error(StatusCode::BAD_REQUEST, "malformed request");
        }
        let em = email.clone();
        let accounts: Vec<(i64, String, String)> = super::admin::run_db(st, move |c| {
            let mut q = c
                .prepare("SELECT id,name,email FROM accounts WHERE email=?1")
                .unwrap();
            q.query_map([&em], |r| {
                Ok((
                    r.get::<_, i64>(0)?,
                    r.get::<_, String>(1)?,
                    r.get::<_, Option<String>>(2)?.unwrap_or_default(),
                ))
            })
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
        let pending: bool = super::admin::run_db(st, move |c| {
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
        let code = uuid();
        let names: String = accounts.iter().map(|a| format!("{}\n", a.1)).collect();
        let reset_url = st.cfg.http.reset_url.clone();
        let code2 = code.clone();
        let exp = chrono::Utc::now().timestamp_millis() + 3_600_000;
        let acct_ids: Vec<i64> = accounts.iter().map(|a| a.0).collect();
        super::admin::run_db(st, move |c| {
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
        if user_re.is_match(&user) {
            return api_error(StatusCode::NOT_IMPLEMENTED, "not yet implemented");
        }
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    }

    // stage 2: username + password + code
    let code_ok = regex::Regex::new(r"^[a-zA-Z0-9-_]{6,128}$")
        .unwrap()
        .is_match(&code);
    if !(user_re.is_match(&user)
        && regex::Regex::new(r"^[a-zA-Z0-9]{4,23}$")
            .unwrap()
            .is_match(&pass)
        && code_ok)
    {
        cooldown(300_000);
        return api_error(StatusCode::BAD_REQUEST, "malformed request");
    }
    let code2 = code.clone();
    let rows: Vec<(i64, i64, String)> =
        super::admin::run_db(st, move |c| {
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
        super::admin::run_db(st, move |c| {
            let _ = c.execute("DELETE FROM password_resets WHERE code=?1", [&code3]);
        })
        .await;
        return api_error(StatusCode::REQUEST_TIMEOUT, "request expired");
    }
    // the account must be one of the code's accounts
    let u = user.clone();
    let acct_ids2: Vec<i64> = rows.iter().map(|r| r.0).collect();
    let found: Option<i64> = super::admin::run_db(st, move |c| {
        acct_ids2
            .iter()
            .find(|id| {
                c.query_row("SELECT name FROM accounts WHERE id=?1", [*id], |r| {
                    r.get::<_, String>(0)
                })
                .map(|n| n == u)
                .unwrap_or(false)
            })
            .copied()
    })
    .await;
    let Some(acct_id) = found else {
        cooldown(300_000);
        let code3 = code.clone();
        super::admin::run_db(st, move |c| {
            let _ = c.execute("DELETE FROM password_resets WHERE code=?1", [&code3]);
        })
        .await;
        return api_error(StatusCode::UNAUTHORIZED, "foreign account");
    };
    let h = crate::auth::password::hash_argon2id(pass.as_bytes()).unwrap_or_default();
    super::admin::run_db(st, move |c| {
        c.execute(
            "UPDATE accounts SET password_hash=?1, password_scheme='argon2id', legacy_salt=NULL WHERE id=?2",
            rusqlite::params![h, acct_id],
        )
        .unwrap();
        let _ = c.execute("DELETE FROM password_resets WHERE code=?1", [&code]);
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

fn uuid() -> String {
    // v4-style random token
    let b = (0..16)
        .map(|_| State::random_u32().unwrap_or(0))
        .collect::<Vec<_>>();
    format!(
        "{:08x}-{:04x}-{:04x}-{:04x}-{:04x}{:08x}",
        b[0],
        b[1] & 0xffff,
        (b[2] & 0x0fff) | 0x4000,
        (b[3] & 0x3fff) | 0x8000,
        b[4] & 0xffff,
        b[5]
    )
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
    let ip = client_ip(req.headers(), peer.ip(), &hs.st);
    let token = req
        .headers()
        .get("x-captcha-token")
        .and_then(|v| v.to_str().ok())
        .unwrap_or("")
        .to_string();
    let re = regex::Regex::new(r"^[a-zA-Z0-9-_]{20,4000}$").unwrap();
    if !re.is_match(&token) {
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
    match reqwest::get(&url).await {
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
        .route(&format!("{base}/account"), any(account_dispatch))
        .route_layer(axum::middleware::from_fn_with_state(hs.clone(), captcha));
    axum::Router::new()
        .route(&format!("{base}/server"), get(server))
        .merge(account)
        .route(&hs.st.cfg.http.ws_path, get(super::ws::handle_ws))
        .fallback(|| async { jserr(StatusCode::NOT_FOUND, "not found") })
        .with_state(hs)
}

async fn account_dispatch(
    hs: AxState<Arc<HttpState>>,
    peer: ConnectInfo<std::net::SocketAddr>,
    headers: HeaderMap,
    req: Request<Body>,
) -> Response {
    let m = req.method().clone();
    let (_parts, body) = req.into_parts();
    let body = axum::body::to_bytes(body, 1 << 20)
        .await
        .unwrap_or_default();
    let json = serde_json::from_slice::<Value>(&body).map(Json);
    match m.as_str() {
        "POST" => create_account(hs, peer, headers, json).await,
        "PUT" => reset_password(hs, peer, headers, json).await,
        _ => jserr(StatusCode::NOT_FOUND, "not found"),
    }
    .into_response()
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
