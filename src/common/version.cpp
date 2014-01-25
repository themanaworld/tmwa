#include "version.hpp"

#include "../conf/version.hpp"

#include "../strings/xstring.hpp"

#include "extract.hpp"

Version CURRENT_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, 0,
    VENDOR_VERSION,
};
Version CURRENT_LOGIN_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_LOGIN,
    VENDOR_VERSION,
};
Version CURRENT_CHAR_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_CHAR | TMWA_SERVER_INTER,
    VENDOR_VERSION,
};
Version CURRENT_MAP_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_MAP,
    VENDOR_VERSION,
};

#define S2(a) #a
#define S(a) S2(a)

const char CURRENT_VERSION_STRING[] = "TMWA "
        S(VERSION_MAJOR) "." S(VERSION_MINOR) "." S(VERSION_PATCH)
        " dev" S(VERSION_DEVEL) " (" VENDOR " " S(VENDOR_VERSION) ")";

bool extract(XString str, Version *vers)
{
    *vers = {};
    return extract(str, record<'.'>(&vers->major, &vers->minor, &vers->patch));
}
