#ifndef MD5CALC_H
#define MD5CALC_H

#include "sanity.h"

#include <netinet/in.h>

#include <stdint.h> // uint32_t, uint8_t
#include <stddef.h> // size_t
#include <stdio.h> // FILE*

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


/// Output in ASCII - with lowercase hex digits, null-terminated
// these may overlap safely
static void MD5_String (const char *string, char output[33]) __attribute__((deprecated));
static inline void MD5_String (const char *string, char output[33]) {
    MD5_to_str(MD5_from_cstring(string), output);
}
/// Output in binary
static void MD5_String2binary (const char *string, uint8_t output[16]) __attribute__((deprecated));
static inline void MD5_String2binary (const char *string, uint8_t output[16]) {
    MD5_to_bin(MD5_from_cstring(string), output);
}

// statically-allocated output
// whoever wrote this fails basic understanding of
const char *MD5_saltcrypt(const char *key, const char *salt);

/// return some random characters (statically allocated)
// Currently, returns a 5-char string
const char *make_salt(void);

/// check plaintext password against saved saltcrypt
bool pass_ok(const char *password, const char *crypted);

/// This returns an in_addr_t because it is configurable whether it gets called at all
in_addr_t MD5_ip(char *secret, in_addr_t ip);

#endif
