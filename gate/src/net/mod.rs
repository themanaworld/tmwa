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
