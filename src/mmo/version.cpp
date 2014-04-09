#include "version.hpp"

#include "../conf/version.hpp"

#include "../strings/xstring.hpp"

#include "extract.hpp"

#include "../poison.hpp"

Version CURRENT_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, 0,
    VENDOR_POINT,
};
Version CURRENT_LOGIN_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_LOGIN,
    VENDOR_POINT,
};
Version CURRENT_CHAR_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_CHAR | TMWA_SERVER_INTER,
    VENDOR_POINT,
};
Version CURRENT_MAP_SERVER_VERSION =
{
    VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
    VERSION_DEVEL,

    0, TMWA_SERVER_MAP,
    VENDOR_POINT,
};

const char CURRENT_VERSION_STRING[] = VERSION_STRING;

bool extract(XString str, Version *vers)
{
    *vers = {};
    // TODO should I try to extract dev and vend also?
    // It would've been useful during the magic migration.
    return extract(str, record<'.'>(&vers->major, &vers->minor, &vers->patch));
}
