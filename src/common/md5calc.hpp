#ifndef MD5CALC_HPP
#define MD5CALC_HPP

#include "sanity.hpp"

#include <netinet/in.h>

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <array>

#include "mmo.hpp"
#include "strings.hpp"

/// The digest state - becomes the output
struct MD5_state
{
    // classically named {A,B,C,D}
    // but use an so we can index
    uint32_t val[4];
};
struct MD5_block
{
    uint32_t data[16];
};

struct md5_binary : std::array<uint8_t, 0x10> {};
struct md5_string : VString<0x20> {};
struct SaltString : VString<5> {};

// Implementation
void MD5_init(MD5_state *state);
void MD5_do_block(MD5_state *state, MD5_block block);

// Output formatting
void MD5_to_bin(MD5_state state, md5_binary& out);
void MD5_to_str(MD5_state state, md5_string& out);

// Convenience
MD5_state MD5_from_string(XString msg);
MD5_state MD5_from_FILE(FILE* in);


// whoever wrote this fails basic understanding of
AccountCrypt MD5_saltcrypt(AccountPass key, SaltString salt);

/// return some random characters (statically allocated)
// Currently, returns a 5-char string
SaltString make_salt(void);

/// check plaintext password against saved saltcrypt
bool pass_ok(AccountPass password, AccountCrypt crypted);

/// This returns an in_addr because it is configurable whether it gets called at all
struct in_addr MD5_ip(struct in_addr ip);

#endif // MD5CALC_HPP
