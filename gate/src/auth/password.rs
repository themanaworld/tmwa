//! Password hashing.
//!
//! New passwords are stored as argon2id PHC strings (`argon2id`
//! scheme). Legacy tmwa passwords are `MD5_saltcrypt` strings of the
//! form `!salt$hash` (truncated to 31 characters); imported accounts
//! keep them wrapped in argon2id (`argon2id-md5` scheme) until the
//! next successful login rehashes the plaintext directly.

use argon2::Argon2;
use argon2::password_hash::phc::PasswordHash;
use argon2::password_hash::{PasswordHasher, PasswordVerifier};
use md5::{Digest, Md5};

/// Password storage scheme as recorded in `accounts.password_scheme`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Scheme {
    /// argon2id PHC string of the plaintext password.
    Argon2id,
    /// argon2id PHC string of the legacy `!salt$hash` string.
    Argon2idMd5,
}

impl Scheme {
    pub fn as_str(self) -> &'static str {
        match self {
            Scheme::Argon2id => "argon2id",
            Scheme::Argon2idMd5 => "argon2id-md5",
        }
    }

    pub fn parse(s: &str) -> Option<Scheme> {
        match s {
            "argon2id" => Some(Scheme::Argon2id),
            "argon2id-md5" => Some(Scheme::Argon2idMd5),
            _ => None,
        }
    }
}

#[derive(Debug, thiserror::Error)]
pub enum PasswordError {
    #[error("argon2: {0}")]
    Argon2(String),
    #[error("unknown password scheme {0:?}")]
    UnknownScheme(String),
    #[error("argon2id-md5 entry without legacy_salt")]
    MissingSalt,
}

fn md5_hex(data: &[u8]) -> String {
    Md5::digest(data)
        .iter()
        .map(|b| format!("{b:02x}"))
        .collect()
}

/// Port of tmwa's `MD5_saltcrypt` (src/high/md5more.cpp):
/// `!salt$md5hex(md5hex(pass) + md5hex(salt))`, truncated to 31 chars
/// by the `SNPRINTF(obuf, 32)` in the original.
pub fn md5_saltcrypt(password: &[u8], salt: &[u8]) -> String {
    let h1 = md5_hex(password);
    let h2 = md5_hex(salt);
    let h3 = md5_hex(format!("{h1}{h2}").as_bytes());
    let mut s = format!("!{}${h3}", String::from_utf8_lossy(salt));
    s.truncate(31);
    s
}

/// The salt of a stored `!salt$hash` legacy string, matching tmwa's
/// `pass_ok` exactly: the first character is skipped whatever it is,
/// and the salt is everything after it up to the first '$' or the end
/// of the string.
pub fn legacy_salt(stored: &str) -> Option<&str> {
    let rest = stored.get(1..)?;
    if rest.is_empty() {
        return None;
    }
    Some(match rest.find('$') {
        Some(i) => &rest[..i],
        None => rest,
    })
}

pub fn hash_argon2id(password: &[u8]) -> Result<String, PasswordError> {
    let h = Argon2::default()
        .hash_password(password)
        .map_err(|e| PasswordError::Argon2(e.to_string()))?;
    Ok(h.to_string())
}

/// Wrap a complete legacy `!salt$hash` string as `argon2id-md5`.
pub fn wrap_legacy(legacy: &str) -> Result<String, PasswordError> {
    hash_argon2id(legacy.as_bytes())
}

/// Result of verifying a password against a stored entry.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Verify {
    /// Matches; stored hash is already in the preferred form.
    Ok,
    /// Matches, but the stored entry should be rehashed to
    /// `argon2id(plaintext)` (legacy or wrapped entry).
    OkNeedsRehash,
    /// Does not match.
    Fail,
}

/// Verify `plain` against a stored password. `scheme` is the value of
/// `accounts.password_scheme`; `salt` is `accounts.legacy_salt`.
pub fn verify(
    scheme: &str,
    hash: &str,
    salt: Option<&str>,
    plain: &[u8],
) -> Result<Verify, PasswordError> {
    match Scheme::parse(scheme) {
        Some(Scheme::Argon2id) => {
            let parsed =
                PasswordHash::new(hash).map_err(|e| PasswordError::Argon2(e.to_string()))?;
            Ok(
                if Argon2::default().verify_password(plain, &parsed).is_ok() {
                    Verify::Ok
                } else {
                    Verify::Fail
                },
            )
        }
        Some(Scheme::Argon2idMd5) => {
            let salt = salt.ok_or(PasswordError::MissingSalt)?;
            let legacy = md5_saltcrypt(plain, salt.as_bytes());
            let parsed =
                PasswordHash::new(hash).map_err(|e| PasswordError::Argon2(e.to_string()))?;
            Ok(
                if Argon2::default()
                    .verify_password(legacy.as_bytes(), &parsed)
                    .is_ok()
                {
                    Verify::OkNeedsRehash
                } else {
                    Verify::Fail
                },
            )
        }
        None => Err(PasswordError::UnknownScheme(scheme.to_string())),
    }
}
