#ifndef MD5CALC_HPP
#define MD5CALC_HPP

#include "sanity.hpp"

#include <netinet/in.h>

#include <cstdint>
#include <cstddef>
#include <cstdio>

/// The digest state - becomes the output
typedef struct
{
    // classically named {A,B,C,D}
    // but use an so we can index
    uint32_t val[4];
} MD5_state;
typedef struct
{
    uint32_t data[16];
} MD5_block;

// Implementation
void MD5_init(MD5_state* state);
void MD5_do_block(MD5_state* state, MD5_block block);

// Output formatting
void MD5_to_bin(MD5_state state, uint8_t out[0x10]);
void MD5_to_str(MD5_state state, char out[0x21]);

// Convenience
MD5_state MD5_from_string(const char* msg, const size_t msglen);
MD5_state MD5_from_cstring(const char* msg);
MD5_state MD5_from_FILE(FILE* in);


// statically-allocated output
// whoever wrote this fails basic understanding of
const char *MD5_saltcrypt(const char *key, const char *salt);

/// return some random characters (statically allocated)
// Currently, returns a 5-char string
const char *make_salt(void);

/// check plaintext password against saved saltcrypt
bool pass_ok(const char *password, const char *crypted);

/// This returns an in_addr because it is configurable whether it gets called at all
struct in_addr MD5_ip(char *secret, struct in_addr ip);

#endif // MD5CALC_HPP
