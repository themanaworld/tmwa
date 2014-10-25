#include "md5more.hpp"
//    md5more.cpp - Non-basic MD5 functions.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>

#include "../compat/rawmem.hpp"

#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"

#include "../net/ip.hpp"

#include "mmo.hpp"

#include "../poison.hpp"


namespace tmwa
{
#define X block.data

// TODO - refactor MD5 into a stream, and merge the implementations
// I once implemented an ostream that does it ...
MD5_state MD5_from_FILE(io::ReadFile& in)
{
    uint64_t total_len = 0;

    uint8_t buf[0x40];
    uint8_t block_len = 0;

    MD5_state state;
    MD5_init(&state);

    MD5_block block;

    while (true)
    {
        size_t rv = in.get(sign_cast<char *>(buf + block_len), 0x40 - block_len);
        if (!rv)
            break;
        total_len += 8 * rv; // in bits
        block_len += rv;
        if (block_len != 0x40)
            continue;
        for (int i = 0; i < 0x10; i++)
            X[i] = buf[4 * i + 0] | buf[4 * i + 1] << 8 | buf[4 * i + 2] << 16 | buf[4 * i + 3] << 24;
        MD5_do_block(&state, block);
        block_len = 0;
    }
    // no more input, just pad and append the length
    buf[block_len] = 0x80;
    really_memset0(buf + block_len + 1, 0x40 - block_len - 1);
    if (block_len < 0x38)
    {
        for (int i = 0; i < 8; i++)
            buf[0x38 + i] = total_len >> i * 8;
    }
    for (int i = 0; i < 0x10; i++)
        X[i] = buf[4 * i + 0] | buf[4 * i + 1] << 8 | buf[4 * i + 2] << 16 | buf[4 * i + 3] << 24;
    MD5_do_block(&state, block);
    if (0x38 <= block_len)
    {
        really_memset0(buf, 0x38);
        for (int i = 0; i < 8; i++)
            buf[0x38 + i] = total_len >> i * 8;
        for (int i = 0; i < 0x10; i++)
            X[i] = buf[4 * i + 0] | buf[4 * i + 1] << 8 | buf[4 * i + 2] << 16 | buf[4 * i + 3] << 24;
        MD5_do_block(&state, block);
    }
    return state;
}


// Hash a password with a salt.
// Whoever wrote this FAILS programming
AccountCrypt MD5_saltcrypt(AccountPass key, SaltString salt)
{
    char cbuf[64] {};

    // hash the key then the salt
    // buf ends up as a 64-char NUL-terminated string
    md5_string tbuf, tbuf2;
    MD5_to_str(MD5_from_string(key), tbuf);
    MD5_to_str(MD5_from_string(salt), tbuf2);
    const auto it = std::copy(tbuf.begin(), tbuf.end(), std::begin(cbuf));
    auto it2 = std::copy(tbuf2.begin(), tbuf2.end(), it);
    assert(it2 == std::end(cbuf));

    md5_string tbuf3;
    MD5_to_str(MD5_from_string(XString(std::begin(cbuf), it2, nullptr)), tbuf3);

    VString<31> obuf;

    // This truncates the string, but we have to keep it like that for compatibility
    SNPRINTF(obuf, 32, "!%s$%s"_fmt, salt, tbuf3);
    return stringish<AccountCrypt>(obuf);
}

SaltString make_salt(void)
{
    char salt[5];
    for (int i = 0; i < 5; i++)
        // 126 would probably actually be okay
        salt[i] = random_::in(48, 125);
    return stringish<SaltString>(XString(salt + 0, salt + 5, nullptr));
}

bool pass_ok(AccountPass password, AccountCrypt crypted)
{
    // crypted is like !salt$hash
    auto begin = crypted.begin() + 1;
    auto end = std::find(begin, crypted.end(), '$');
    SaltString salt = stringish<SaltString>(crypted.xislice(begin, end));

    return crypted == MD5_saltcrypt(password, salt);
}

// [M|h]ashes up an IP address and a secret key
// to return a hopefully unique masked IP.
IP4Address MD5_ip(IP4Address ip)
{
    static SaltString secret = make_salt();

    // MD5sum a secret + the IP address
    VString<31> ipbuf;
    SNPRINTF(ipbuf, 32, "%s %s"_fmt, ip, secret);
    md5_binary obuf;
    MD5_to_bin(MD5_from_string(ipbuf), obuf);

    // Fold the md5sum to 32 bits, pack the bytes to an in_addr
    return IP4Address({
            static_cast<uint8_t>(obuf[0] ^ obuf[1] ^ obuf[8] ^ obuf[9]),
            static_cast<uint8_t>(obuf[2] ^ obuf[3] ^ obuf[10] ^ obuf[11]),
            static_cast<uint8_t>(obuf[4] ^ obuf[5] ^ obuf[12] ^ obuf[13]),
            static_cast<uint8_t>(obuf[6] ^ obuf[7] ^ obuf[14] ^ obuf[15]),
    });
}
} // namespace tmwa
