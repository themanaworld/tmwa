#ifndef VERSION_HPP
#define VERSION_HPP

// TODO generate this from ./configure
// Actually, it should be done from the Makefile
// it should be possible to use a ./config.status for a long time

# define TMWA_VERSION_MAJOR     13
# define TMWA_VERSION_MINOR     9
# define TMWA_VERSION_PATCH     12
# define TMWA_DEVELOP_FLAG      1

// TODO make these bitwise enums
# define TMWA_FLAG_REGISTRATION 0x01

# define TMWA_SERVER_LOGIN      0x01
# define TMWA_SERVER_CHAR       0x02
# define TMWA_SERVER_INTER      0x04
# define TMWA_SERVER_MAP        0x08

# define TMWA_VENDOR            "Vanilla"
# define TMWA_VENDOR_VERSION    0

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
static_assert(sizeof(Version) == 8, "this is send over the network, can't change");

constexpr Version CURRENT_LOGIN_SERVER_VERSION =
{
    TMWA_VERSION_MAJOR, TMWA_VERSION_MINOR, TMWA_VERSION_PATCH,
    TMWA_DEVELOP_FLAG,

    0, TMWA_SERVER_LOGIN,
    TMWA_VENDOR_VERSION,
};
constexpr Version CURRENT_CHAR_SERVER_VERSION =
{
    TMWA_VERSION_MAJOR, TMWA_VERSION_MINOR, TMWA_VERSION_PATCH,
    TMWA_DEVELOP_FLAG,

    0, TMWA_SERVER_CHAR | TMWA_SERVER_INTER,
    TMWA_VENDOR_VERSION,
};
constexpr Version CURRENT_MAP_SERVER_VERSION =
{
    TMWA_VERSION_MAJOR, TMWA_VERSION_MINOR, TMWA_VERSION_PATCH,
    TMWA_DEVELOP_FLAG,

    0, TMWA_SERVER_MAP,
    TMWA_VENDOR_VERSION,
};

#endif // VERSION_HPP
