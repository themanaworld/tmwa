#include "md5calc.hpp"
#include <string.h>
#include "mt_rand.hpp"

// auxilary data
/*
sin() constant table
# Reformatted output of:
echo 'scale=40; obase=16; for (i=1;i<=64;i++) print 2^32 * sin(i), "\n"' |
bc | sed 's/^-//;s/^/0x/;s/\..*$/,/'
*/
static const uint32_t T[64] =
{
    // used by round 1
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, //0
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, //4
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, //8
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, //12
    // used by round 2
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, //16
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, //20
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, //24
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, //28
    // used by round 3
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, //32
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, //36
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, //40
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, //44
    // used by round 4
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, //48
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1, //52
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, //56
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391, //60
};

// auxilary functions
// note - the RFC defines these by non-CS conventions: or=v, and=(empty)
static inline uint32_t rotate_left(uint32_t val, unsigned shift)
{
    return val << shift | val >> (32-shift);
}

static inline uint32_t F(uint32_t X, uint32_t Y, uint32_t Z)
{
    return (X & Y) | (~X & Z);
}
static inline uint32_t G(uint32_t X, uint32_t Y, uint32_t Z)
{
    return (X & Z) | (Y & ~Z);
}
static inline uint32_t H(uint32_t X, uint32_t Y, uint32_t Z)
{
    return X ^ Y ^ Z;
}
static inline uint32_t I(uint32_t X, uint32_t Y, uint32_t Z)
{
    return Y ^ (X | ~Z);
}

static const struct
{
    uint8_t k : 4;
    uint8_t : 0;
    uint8_t s : 5;
//    uint8_t i : 6; just increments constantly, from 1 .. 64 over all rounds
}
MD5_round1[16] =
{
    { 0,  7}, { 1, 12}, { 2, 17}, { 3, 22},
    { 4,  7}, { 5, 12}, { 6, 17}, { 7, 22},
    { 8,  7}, { 9, 12}, {10, 17}, {11, 22},
    {12,  7}, {13, 12}, {14, 17}, {15, 22},
},
MD5_round2[16] =
{
    { 1,  5}, { 6,  9}, {11, 14}, { 0, 20},
    { 5,  5}, {10,  9}, {15, 14}, { 4, 20},
    { 9,  5}, {14,  9}, { 3, 14}, { 8, 20},
    {13,  5}, { 2,  9}, { 7, 14}, {12, 20},
},
MD5_round3[16] =
{
    { 5,  4}, { 8, 11}, {11, 16}, {14, 23},
    { 1,  4}, { 4, 11}, { 7, 16}, {10, 23},
    {13,  4}, { 0, 11}, { 3, 16}, { 6, 23},
    { 9,  4}, {12, 11}, {15, 16}, { 2, 23},
},
MD5_round4[16] =
{
    { 0,  6}, { 7, 10}, {14, 15}, { 5, 21},
    {12,  6}, { 3, 10}, {10, 15}, { 1, 21},
    { 8,  6}, {15, 10}, { 6, 15}, {13, 21},
    { 4,  6}, {11, 10}, { 2, 15}, { 9, 21},
};


void MD5_init(MD5_state* state)
{
    // in the RFC, these are specified as bytes, interpreted as little-endian
    state->val[0] = 0x67452301;
    state->val[1] = 0xEFCDAB89;
    state->val[2] = 0x98BADCFE;
    state->val[3] = 0x10325476;
}

void MD5_do_block(MD5_state* state, MD5_block block)
{
#define X block.data
#define a state->val[(16-i)%4]
#define b state->val[(17-i)%4]
#define c state->val[(18-i)%4]
#define d state->val[(19-i)%4]
    // save the values
    const MD5_state saved = *state;
    // round 1
    for (int i=0; i<16; i++)
    {
#define k MD5_round1[i].k
#define s MD5_round1[i].s
        a = b + rotate_left(a + F(b,c,d) + X[k] + T[i+0x0], s);
#undef k
#undef s
    }
    // round 2
    for (int i=0; i<16; i++)
    {
#define k MD5_round2[i].k
#define s MD5_round2[i].s
        a = b + rotate_left(a + G(b,c,d) + X[k] + T[i+0x10], s);
#undef k
#undef s
    }
    // round 3
    for (int i=0; i<16; i++)
    {
#define k MD5_round3[i].k
#define s MD5_round3[i].s
        a = b + rotate_left(a + H(b,c,d) + X[k] + T[i+0x20], s);
#undef k
#undef s
    }
    // round 4
    for (int i=0; i<16; i++)
    {
#define k MD5_round4[i].k
#define s MD5_round4[i].s
        a = b + rotate_left(a + I(b,c,d) + X[k] + T[i+0x30], s);
#undef k
#undef s
    }
    // adjust state based on original
    state->val[0] += saved.val[0];
    state->val[1] += saved.val[1];
    state->val[2] += saved.val[2];
    state->val[3] += saved.val[3];
#undef a
#undef b
#undef c
#undef d
}

void MD5_to_bin(MD5_state state, uint8_t out[0x10])
{
    for (int i=0; i<0x10; i++)
        out[i] = state.val[i/4] >> 8*(i%4);
}

static const char hex[] = "0123456789abcdef";

void MD5_to_str(MD5_state state, char out[0x21])
{
    uint8_t bin[16];
    MD5_to_bin(state, bin);
    for (int i=0; i<0x10; i++)
        out[2*i] = hex[bin[i] >> 4],
        out[2*i+1] = hex[bin[i] & 0xf];
    out[0x20] = '\0';
}

MD5_state MD5_from_string(const char* msg, const size_t msglen)
{
    MD5_state state;
    MD5_init(&state);
    MD5_block block;
    size_t rem = msglen;
    while (rem >= 64)
    {
        for (int i=0; i<0x10; i++)
            X[i] = msg[4*i+0] | msg[4*i+1]<<8 | msg[4*i+2]<<16 | msg[4*i+3]<<24;
        MD5_do_block(&state, block);
        msg += 64;
        rem -= 64;
    }
    // now pad 1-512 bits + the 64-bit length - may be two blocks
    uint8_t buf[0x40] = {};
    memcpy (buf, msg, rem);
    buf[rem] = 0x80; // a single one bit
    if (64 - rem > 8)
    {
        for (int i=0; i<8; i++)
            buf[0x38+i] = ((uint64_t)msglen*8) >> (i*8);
    }
    for (int i=0; i<0x10; i++)
        X[i] = buf[4*i+0] | buf[4*i+1]<<8 | buf[4*i+2]<<16 | buf[4*i+3]<<24;
    MD5_do_block(&state, block);
    if (64 - rem <= 8)
    {
        memset(buf,'\0', 0x38);
        for (int i=0; i<8; i++)
            buf[0x38+i] = ((uint64_t)msglen*8) >> (i*8);
        for (int i=0; i<0x10; i++)
            X[i] = buf[4*i+0] | buf[4*i+1]<<8 | buf[4*i+2]<<16 | buf[4*i+3]<<24;
        MD5_do_block(&state, block);
    }
    return state;
}

// This could be reimplemented without the strlen()
MD5_state MD5_from_cstring(const char* msg)
{
    return MD5_from_string(msg, strlen(msg));
}

MD5_state MD5_from_FILE(FILE* in) {
    uint64_t total_len = 0;

    uint8_t buf[0x40];
    uint8_t block_len = 0;

    MD5_state state;
    MD5_init(&state);

    MD5_block block;

    while (true)
    {
        size_t rv = fread(buf + block_len, 1, 0x40 - block_len, in);
        if (!rv)
            break;
        total_len += 8*rv; // in bits
        block_len += rv;
        if (block_len != 0x40)
            continue;
        for (int i=0; i<0x10; i++)
            X[i] = buf[4*i+0] | buf[4*i+1]<<8 | buf[4*i+2]<<16 | buf[4*i+3]<<24;
        MD5_do_block(&state, block);
        block_len = 0;
    }
    // no more input, just pad and append the length
    buf[block_len] = 0x80;
    memset(buf + block_len + 1, '\0', 0x40 - block_len - 1);
    if (block_len < 0x38)
    {
        for (int i=0; i<8; i++)
            buf[0x38+i] = total_len >> i*8;
    }
    for (int i=0; i<0x10; i++)
        X[i] = buf[4*i+0] | buf[4*i+1]<<8 | buf[4*i+2]<<16 | buf[4*i+3]<<24;
    MD5_do_block(&state, block);
    if (0x38 <= block_len)
    {
        memset(buf, '\0', 0x38);
        for (int i=0; i<8; i++)
            buf[0x38+i] = total_len >> i*8;
        for (int i=0; i<0x10; i++)
            X[i] = buf[4*i+0] | buf[4*i+1]<<8 | buf[4*i+2]<<16 | buf[4*i+3]<<24;
        MD5_do_block(&state, block);
    }
    return state;
}


// Hash a password with a salt.
// Whoever wrote this FAILS programming
const char *MD5_saltcrypt(const char *key, const char *salt)
{
    char buf[65];

    // hash the key then the salt
    // buf ends up as a 64-char NUL-terminated string
    MD5_to_str(MD5_from_cstring(key), buf);
    MD5_to_str(MD5_from_cstring(salt), buf+32);

    // Hash the buffer back into sbuf - this is stupid
    // (luckily, putting the result into itself is safe)
    MD5_to_str(MD5_from_cstring(buf), buf+32);

    static char obuf[33];
    // This truncates the string, but we have to keep it like that for compatibility
    snprintf(obuf, 32, "!%s$%s", salt, buf+32);
    return obuf;
}

const char *make_salt(void) {
    static char salt[6];
    for (int i=0; i<5; i++)
        salt[i] = MPRAND(48, 78);
    return salt;
}

bool pass_ok(const char *password, const char *crypted) {
    char buf[40];
    strncpy(buf, crypted, 40);
    char *salt = buf + 1;
    *strchr(salt, '$') = '\0';

    return !strcmp(crypted, MD5_saltcrypt(password, salt));
}

// [M|h]ashes up an IP address and a secret key
// to return a hopefully unique masked IP.
in_addr_t MD5_ip(char *secret, in_addr_t ip)
{
    char ipbuf[32];
    uint8_t obuf[16];
    union {
        struct bytes {
            uint8_t b1;
            uint8_t b2;
            uint8_t b3;
            uint8_t b4;
        } bytes;
        in_addr_t ip;
    } conv;

    // MD5sum a secret + the IP address
    memset(&ipbuf, 0, sizeof(ipbuf));
    snprintf(ipbuf, sizeof(ipbuf), "%lu%s", (unsigned long)ip, secret);
    /// TODO stop it from being a cstring
    MD5_to_bin(MD5_from_cstring(ipbuf), obuf);

    // Fold the md5sum to 32 bits, pack the bytes to an in_addr_t
    conv.bytes.b1 = obuf[0] ^ obuf[1] ^ obuf[8] ^ obuf[9];
    conv.bytes.b2 = obuf[2] ^ obuf[3] ^ obuf[10] ^ obuf[11];
    conv.bytes.b3 = obuf[4] ^ obuf[5] ^ obuf[12] ^ obuf[13];
    conv.bytes.b4 = obuf[6] ^ obuf[7] ^ obuf[14] ^ obuf[15];

    return conv.ip;
}
