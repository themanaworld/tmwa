pub mod framing;

use std::net::{IpAddr, Ipv4Addr};

/// Map any client address to the IPv4 the gate and tmwa-map use.
///
/// IPv4 passes through unchanged. An IPv6 address is hashed (FNV-1a
/// over the /64 prefix) into 240.0.0.0/4, which tmwa-map happily
/// carries through logs, `last_ip`, bans and `@ip`; operators can
/// correlate back because the gate logs the real address at login.
/// This keeps IPv6 web players distinct rather than lumping them all
/// onto 127.0.0.1 or each other.
pub fn map_ip(ip: IpAddr) -> Ipv4Addr {
    match ip {
        IpAddr::V4(v) => v,
        IpAddr::V6(v6) => {
            if let Some(v4) = v6.to_ipv4_mapped().or_else(|| v6.to_ipv4()) {
                return v4;
            }
            let seg = v6.segments();
            // hash the /64 prefix only: privacy extensions change the
            // host bits constantly but the prefix is the player
            let mut h: u64 = 0xcbf29ce484222325;
            for w in &seg[..4] {
                h ^= *w as u64;
                h = h.wrapping_mul(0x100000001b3);
            }
            let bits = (h as u32) & 0x0fff_ffff;
            Ipv4Addr::from(0xF000_0000u32 | bits)
        }
    }
}

/// The IPv4 used for HTTP rate limiting and XFF checks. IPv6 peers
/// are keyed by their /64 (privacy-extension-safe), represented in
/// the same 240/4 space.
pub fn rate_ip(ip: IpAddr) -> Ipv4Addr {
    match ip {
        IpAddr::V4(v) => v,
        IpAddr::V6(v6) => match v6.to_ipv4_mapped().or_else(|| v6.to_ipv4()) {
            Some(v4) => v4,
            None => map_ip(ip),
        },
    }
}

/// IPv4 network mask, mirroring tmwa's `IP4Mask`
/// (src/net/ip.cpp `impl_extract`). Accepted forms:
///
/// - `a.b.c.d` — /32, covers only that host
/// - `a.` / `a.b.` / `a.b.c.` — trailing-dot shorthand for /8, /16, /24
/// - `a.b.c.d/e.f.g.h` — dotted netmask
/// - `a.b.c.d/n` — CIDR prefix length (0..=32)
///
/// The address is masked on construction, so `10.9.9.9/8` covers
/// all of 10.0.0.0/8 like tmwa does.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Ip4Mask {
    addr: Ipv4Addr,
    mask: Ipv4Addr,
}

impl Ip4Mask {
    pub fn new(addr: Ipv4Addr, mask: Ipv4Addr) -> Ip4Mask {
        Ip4Mask {
            addr: Ipv4Addr::from(addr.to_bits() & mask.to_bits()),
            mask,
        }
    }

    /// True when `ip` falls inside this mask.
    pub fn covers(&self, ip: Ipv4Addr) -> bool {
        ip.to_bits() & self.mask.to_bits() == self.addr.to_bits()
    }

    pub fn addr(&self) -> Ipv4Addr {
        self.addr
    }

    pub fn mask(&self) -> Ipv4Addr {
        self.mask
    }
}

fn mask_from_bits(bits: u32) -> Ipv4Addr {
    debug_assert!(bits <= 32);
    Ipv4Addr::from(u32::MAX.checked_shl(32 - bits).unwrap_or(0))
}

/// Why an `Ip4Mask` string failed to parse.
#[derive(Debug)]
pub struct Ip4MaskError;

impl std::fmt::Display for Ip4MaskError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.write_str("invalid IPv4 subnet mask")
    }
}
impl std::error::Error for Ip4MaskError {}

impl std::str::FromStr for Ip4Mask {
    type Err = Ip4MaskError;

    fn from_str(s: &str) -> Result<Ip4Mask, Ip4MaskError> {
        if let Some((l, r)) = s.split_once('/') {
            if r.is_empty() {
                return Err(Ip4MaskError);
            }
            let a: Ipv4Addr = l.parse().map_err(|_| Ip4MaskError)?;
            // dotted netmask, else CIDR prefix length
            if let Ok(m) = r.parse::<Ipv4Addr>() {
                return Ok(Ip4Mask::new(a, m));
            }
            let bits: u32 = r.parse().map_err(|_| Ip4MaskError)?;
            if bits > 32 {
                return Err(Ip4MaskError);
            }
            return Ok(Ip4Mask::new(a, mask_from_bits(bits)));
        }
        if let Ok(a) = s.parse::<Ipv4Addr>() {
            // bare host: /32
            return Ok(Ip4Mask::new(a, Ipv4Addr::from(u32::MAX)));
        }
        // trailing-dot shorthand: "a." / "a.b." / "a.b.c." / "a.b.c.d."
        if let Some(prefix) = s.strip_suffix('.') {
            let parts: Vec<&str> = prefix.split('.').collect();
            let bits = match parts.len() {
                1 => 8,
                2 => 16,
                3 => 24,
                4 => 32,
                _ => return Err(Ip4MaskError),
            };
            let mut octets = [0u8; 4];
            for (o, p) in octets.iter_mut().zip(parts) {
                *o = p.parse().map_err(|_| Ip4MaskError)?;
            }
            return Ok(Ip4Mask::new(Ipv4Addr::from(octets), mask_from_bits(bits)));
        }
        Err(Ip4MaskError)
    }
}

impl<'de> serde::Deserialize<'de> for Ip4Mask {
    fn deserialize<D: serde::Deserializer<'de>>(d: D) -> Result<Ip4Mask, D::Error> {
        let s = <String as serde::Deserialize>::deserialize(d)?;
        s.parse().map_err(serde::de::Error::custom)
    }
}

/// Resolve the client IP for a request: when the peer is a trusted
/// proxy, walk X-Forwarded-For from the right past trusted proxies
/// and take the first untrusted entry. Never the leftmost — clients
/// can prepend anything. Returns `peer` when untrusted or when no
/// usable XFF is present.
pub fn forwarded_for(peer: IpAddr, xff: Option<&str>, trusted: &[String]) -> IpAddr {
    let is_trusted = |ip: IpAddr| {
        trusted
            .iter()
            .any(|t| t.parse::<IpAddr>().map(|t| t == ip).unwrap_or(false))
    };
    if !is_trusted(peer) {
        return peer;
    }
    if let Some(xff) = xff {
        let mut entries: Vec<IpAddr> = xff
            .split(',')
            .filter_map(|s| s.trim().parse::<IpAddr>().ok())
            .collect();
        // keep walking left while the entry is itself a proxy
        while let Some(&last) = entries.last() {
            if is_trusted(last) {
                entries.pop();
            } else {
                return last;
            }
        }
    }
    peer
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::str::FromStr;

    fn v4(s: &str) -> IpAddr {
        IpAddr::V4(Ipv4Addr::from_str(s).unwrap())
    }
    fn v6(s: &str) -> IpAddr {
        s.parse().unwrap()
    }

    #[test]
    fn map_ip_v4_passthrough() {
        assert_eq!(map_ip(v4("1.2.3.4")), Ipv4Addr::new(1, 2, 3, 4));
    }

    #[test]
    fn map_ip_v4_mapped() {
        assert_eq!(map_ip(v6("::ffff:1.2.3.4")), Ipv4Addr::new(1, 2, 3, 4));
    }

    #[test]
    fn map_ip_v6_pseudo() {
        let a = map_ip(v6("2001:db8:1::1"));
        let b = map_ip(v6("2001:db8:1::2")); // same /64
        let c = map_ip(v6("2001:db8:2::1")); // different /64
        assert_eq!(a, b);
        assert_ne!(a, c);
        assert!(a.octets()[0] >= 240);
        // stable across calls
        assert_eq!(a, map_ip(v6("2001:db8:1::9")));
    }

    fn mask(s: &str) -> Ip4Mask {
        s.parse().unwrap()
    }

    #[test]
    fn ip4mask_forms() {
        // bare host -> /32
        let m = mask("127.0.0.1");
        assert!(m.covers(Ipv4Addr::new(127, 0, 0, 1)));
        assert!(!m.covers(Ipv4Addr::new(127, 0, 0, 2)));

        // CIDR prefix
        let m = mask("10.0.0.0/8");
        assert_eq!(m.mask(), Ipv4Addr::new(255, 0, 0, 0));
        assert!(m.covers(Ipv4Addr::new(10, 1, 2, 3)));
        assert!(!m.covers(Ipv4Addr::new(1, 2, 3, 4)));

        // dotted netmask
        let m = mask("192.168.1.0/255.255.255.0");
        assert!(m.covers(Ipv4Addr::new(192, 168, 1, 10)));
        assert!(!m.covers(Ipv4Addr::new(192, 168, 2, 10)));

        // trailing-dot shorthand
        assert_eq!(mask("10.").mask(), Ipv4Addr::new(255, 0, 0, 0));
        assert_eq!(mask("192.168.").mask(), Ipv4Addr::new(255, 255, 0, 0));
        assert_eq!(mask("192.168.1.").mask(), Ipv4Addr::new(255, 255, 255, 0));
        assert_eq!(mask("127.0.0.1.").mask(), Ipv4Addr::new(255, 255, 255, 255));

        // the address is masked on construction, like tmwa
        assert_eq!(mask("10.9.9.9/8").addr(), Ipv4Addr::new(10, 0, 0, 0));
        assert!(mask("10.9.9.9/8").covers(Ipv4Addr::new(10, 1, 2, 3)));

        // /0 covers everything
        assert!(mask("0.0.0.0/0").covers(Ipv4Addr::new(1, 2, 3, 4)));

        // invalid forms
        for s in ["", "bogus", "1.2.3.4/", "1.2.3.4/33", "1.2.3.4/bogus", "."] {
            assert!(s.parse::<Ip4Mask>().is_err(), "{s}");
        }
    }

    #[test]
    fn forwarded_for_untrusted_peer() {
        // socket address wins when the peer isn't a proxy
        assert_eq!(
            forwarded_for(v4("9.9.9.9"), Some("1.1.1.1"), &["127.0.0.1".into()]),
            v4("9.9.9.9")
        );
    }

    #[test]
    fn forwarded_for_right_to_left() {
        let trusted = vec!["10.0.0.1".to_string(), "10.0.0.2".to_string()];
        // client spoofed leftmost; real client 1.2.3.4, then proxies
        assert_eq!(
            forwarded_for(v4("10.0.0.1"), Some("6.6.6.6, 1.2.3.4, 10.0.0.2"), &trusted),
            v4("1.2.3.4")
        );
        // all entries trusted → keep the peer
        assert_eq!(
            forwarded_for(v4("10.0.0.1"), Some("10.0.0.2"), &trusted),
            v4("10.0.0.1")
        );
        // no header → peer
        assert_eq!(
            forwarded_for(v4("10.0.0.1"), None, &trusted),
            v4("10.0.0.1")
        );
    }
}
