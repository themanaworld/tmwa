//! Protocol codec tests: sizes, round trips and hand-built vectors
//! checked byte-for-byte against the offsets in tools/protocol.py.

use tmwa_gate::proto::types::*;
use tmwa_gate::proto::*;

/// Encoded size of a default packet equals the size declared in
/// protocol.py for every fixed-size packet, and decode(encode(x))
/// round-trips byte-identically for every packet in the table.
#[test]
fn all_packets_round_trip() {
    for &id in ALL_PACKET_IDS {
        let expect = packet_len(id).unwrap_or_else(|| panic!("packet_len missing 0x{id:04x}"));
        let enc = encode_default(id).unwrap_or_else(|| panic!("encode_default missing 0x{id:04x}"));
        if let PacketLen::Fixed(n) = expect {
            assert_eq!(enc.len(), n, "fixed size for 0x{id:04x}");
        } else {
            // variable packets still have a sane minimum length
            assert!(enc.len() >= 2);
        }
        let again =
            decode_encode(id, &enc).unwrap_or_else(|e| panic!("decode failed for 0x{id:04x}: {e}"));
        assert_eq!(again, enc, "re-encode mismatch for 0x{id:04x}");
    }
}

/// packet_len covers every declared packet id.
#[test]
fn packet_len_covers_all() {
    for &id in ALL_PACKET_IDS {
        assert!(packet_len(id).is_some(), "0x{id:04x} missing");
    }
}

#[test]
fn p0064_login() {
    let p = P0064 {
        client_protocol_version: ClientVersion(0x01020304),
        account_name: FixedStr::<24>::try_from_str("testuser").unwrap(),
        account_pass: FixedStr::<24>::try_from_str("secret").unwrap(),
        flags: 3,
    };
    let mut v = Vec::new();
    p.encode(&mut v);
    assert_eq!(v.len(), 55);
    assert_eq!(&v[0..2], &[0x64, 0x00]);
    assert_eq!(&v[2..6], &[0x04, 0x03, 0x02, 0x01]);
    assert_eq!(&v[6..14], b"testuser");
    assert_eq!(v[14], 0);
    assert_eq!(&v[30..36], b"secret");
    assert_eq!(v[54], 3);
    let d = P0064::decode(&v).unwrap();
    assert_eq!(d.client_protocol_version, ClientVersion(0x01020304));
    assert_eq!(d.account_name.as_bytes(), b"testuser");
    assert_eq!(d.flags, 3);
}

#[test]
fn p0072_map_connect() {
    let p = P0072 {
        account_id: AccountId(2000000),
        char_id: CharId(150000),
        login_id1: 0xA5A5A5A5,
        client_tick: 0x01020304,
        sex: Sex(1),
    };
    let mut v = Vec::new();
    p.encode(&mut v);
    assert_eq!(v.len(), 19);
    assert_eq!(&v[0..2], &[0x72, 0x00]);
    assert_eq!(&v[2..6], &2000000u32.to_le_bytes());
    assert_eq!(&v[6..10], &150000u32.to_le_bytes());
    assert_eq!(&v[10..14], &0xA5A5A5A5u32.to_le_bytes());
    assert_eq!(&v[14..18], &0x01020304u32.to_le_bytes());
    assert_eq!(v[18], 1);
    let d = P0072::decode(&v).unwrap();
    assert_eq!(d.char_id, CharId(150000));
}

#[test]
fn p0091_change_map() {
    let p = P0091 {
        map_name: FixedStr::<16>::try_from_str("001-1").unwrap(),
        x: 32,
        y: 59,
    };
    let mut v = Vec::new();
    p.encode(&mut v);
    assert_eq!(v.len(), 22);
    assert_eq!(&v[0..2], &[0x91, 0x00]);
    assert_eq!(&v[2..7], b"001-1");
    assert_eq!(&v[18..20], &32u16.to_le_bytes());
    assert_eq!(&v[20..22], &59u16.to_le_bytes());
    let d = P0091::decode(&v).unwrap();
    assert_eq!(d.x, 32);
    assert_eq!(d.y, 59);
}

#[test]
fn p2afc_map_auth() {
    let p = P2AFC {
        account_id: AccountId(2000000),
        char_id: CharId(150000),
        login_id1: 1,
        login_id2: 2,
        ip: Ip4Address([127, 0, 0, 1]),
    };
    let mut v = Vec::new();
    p.encode(&mut v);
    assert_eq!(v.len(), 22);
    assert_eq!(&v[0..2], &[0xFC, 0x2A]);
    assert_eq!(&v[2..6], &2000000u32.to_le_bytes());
    assert_eq!(&v[6..10], &150000u32.to_le_bytes());
    assert_eq!(&v[18..22], &[127, 0, 0, 1]);
    let d = P2AFC::decode(&v).unwrap();
    assert_eq!(d.ip, Ip4Address([127, 0, 0, 1]));
}

/// 0x2afd carries a full CharData; set non-default values in the
/// inventory, skills and account registers to catch layout mistakes.
#[test]
#[allow(clippy::field_reassign_with_default)]
fn p2afd_char_data() {
    let mut p = P2AFD::default();
    p.account_id = AccountId(2000000);
    p.login_id2 = 0xABCD;
    p.client_protocol_version = ClientVersion(2);
    p.char_key.name = FixedStr::<24>::try_from_str("Spiketest").unwrap();
    p.char_key.account_id = AccountId(2000000);
    p.char_key.char_id = CharId(150000);
    p.char_key.char_num = 0;
    let cd = &mut p.char_data;
    cd.hp = 1064;
    cd.max_hp = 1064;
    cd.inventory[3].nameid = ItemNameId(535);
    cd.inventory[3].amount = 10;
    cd.inventory[4].nameid = ItemNameId(1201);
    cd.inventory[4].equip = Epos(2);
    cd.skill[8].lv = 5;
    cd.global_reg_num = 1;
    cd.global_reg[0].str = FixedStr::<32>::try_from_str("#testvar").unwrap();
    cd.global_reg[0].value = 42;
    cd.account_reg[0].str = FixedStr::<32>::try_from_str("accvar").unwrap();
    cd.account_reg[0].value = -7;

    let mut v = Vec::new();
    p.encode(&mut v);
    assert_eq!(v.len(), P2AFD::WIRE_LEN);
    assert_eq!(&v[0..2], &[0xFD, 0x2A]);
    // packet length field covers the whole packet
    assert_eq!(&v[2..4], &(v.len() as u16).to_le_bytes());

    let d = P2AFD::decode(&v).unwrap();
    let cd = &d.char_data;
    assert_eq!(cd.hp, 1064);
    assert_eq!(cd.inventory[3].nameid, ItemNameId(535));
    assert_eq!(cd.inventory[3].amount, 10);
    assert_eq!(cd.inventory[4].equip, Epos(2));
    assert_eq!(cd.skill[8].lv, 5);
    assert_eq!(cd.global_reg[0].str.as_bytes(), b"#testvar");
    assert_eq!(cd.global_reg[0].value, 42);
    assert_eq!(cd.account_reg[0].value, -7);
    assert_eq!(d.char_key.name.as_bytes(), b"Spiketest");

    // byte-level spot checks inside the big struct: char_data starts at
    // offset 53; hp is field 6 of CharData -> 53 + 22.
    let cd_off = 53;
    assert_eq!(&v[cd_off + 22..cd_off + 26], &1064i32.to_le_bytes());
    // inventory[3].nameid: inventory begins after 4+4+4+4+2+2+2+4+4+4+4
    // +2*6+4+2*5+1+1+12+1+4+2+20+20 = 125 bytes of scalars.
    let inv_off = cd_off + 125 + 3 * 6;
    assert_eq!(&v[inv_off..inv_off + 2], &535u16.to_le_bytes());
}

/// Variable-length packet: repeat entries are counted correctly and the
/// length field is written.
#[test]
fn p0069_variable() {
    let mut p = P0069 {
        login_id1: 7,
        ..Default::default()
    };
    p.repeat.push(P0069Repeat {
        ip: Ip4Address([127, 0, 0, 1]),
        port: 6121,
        server_name: FixedStr::<20>::try_from_str("test").unwrap(),
        users: 5,
        maintenance: 0,
        is_new: 0,
    });
    p.repeat.push(P0069Repeat::default());
    let mut v = Vec::new();
    p.encode(&mut v);
    assert_eq!(v.len(), 47 + 2 * 32);
    assert_eq!(&v[2..4], &(v.len() as u16).to_le_bytes());
    let d = P0069::decode(&v).unwrap();
    assert_eq!(d.repeat.len(), 2);
    assert_eq!(d.repeat[0].port, 6121);
}
