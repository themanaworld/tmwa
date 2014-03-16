#ifndef TMWA_MMO_VERSION_HPP
#define TMWA_MMO_VERSION_HPP

# include <cstdint>

# include "../strings/fwd.hpp"

// TODO make these bitwise enums
# define TMWA_FLAG_REGISTRATION 0x01

# define TMWA_SERVER_LOGIN      0x01
# define TMWA_SERVER_CHAR       0x02
# define TMWA_SERVER_INTER      0x04
# define TMWA_SERVER_MAP        0x08

struct Version
{
    uint8_t major;
    uint8_t minor; // flavor1
    uint8_t patch; // flavor2
    uint8_t devel; // flavor3

    uint8_t flags;
    uint8_t which;
    uint16_t vend;
    // can't add vendor name yet
};
static_assert(sizeof(Version) == 8, "this is sent over the network, can't change");

extern Version CURRENT_VERSION;

extern Version CURRENT_LOGIN_SERVER_VERSION;
extern Version CURRENT_CHAR_SERVER_VERSION;
extern Version CURRENT_MAP_SERVER_VERSION;

extern const char CURRENT_VERSION_STRING[];

bool extract(XString str, Version *vers);

constexpr
bool operator < (Version l, Version r)
{
    return (l.major < r.major
            || (l.major == r.major
                && (l.minor < r.minor
                    || (l.minor == r.minor
                        && (l.patch < r.patch
                            || (l.patch == r.patch
                                && (l.devel < r.devel
                                    || (l.devel == r.devel
                                        && l.vend < r.vend))))))));
}
constexpr
bool operator > (Version l, Version r)
{
    return r < l;
}
constexpr
bool operator <= (Version l, Version r)
{
    return !(r < l);
}
constexpr
bool operator >= (Version l, Version r)
{
    return !(l < r);
}

#endif // TMWA_MMO_VERSION_HPP
