#include "utils.hpp"

#include <netinet/in.h>
#include <sys/time.h>

#include <algorithm>

#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/write.hpp"

#include "extract.hpp"

#include "../poison.hpp"

//---------------------------------------------------
// E-mail check: return 0 (not correct) or 1 (valid).
//---------------------------------------------------
bool e_mail_check(XString email)
{
    // athena limits
    if (email.size() < 3 || email.size() > 39)
        return 0;

    // part of RFC limits (official reference of e-mail description)
    XString::iterator at = std::find(email.begin(), email.end(), '@');
    if (at == email.end())
        return 0;
    XString username = email.xislice_h(at);
    XString hostname = email.xislice_t(at + 1);
    if (!username || !hostname)
        return 0;
    if (hostname.contains('@'))
        return 0;
    if (hostname.front() == '.' || hostname.back() == '.')
        return 0;
    if (hostname.contains_seq(".."))
        return 0;
    if (email.contains_any(" ;"))
        return 0;
    return email.is_print();
}

//-------------------------------------------------
// Return numerical value of a switch configuration
// on/off, english, français, deutsch, español
//-------------------------------------------------
int config_switch(ZString str)
{
    if (str == "true" || str == "on" || str == "yes"
        || str == "oui" || str == "ja"
        || str == "si")
        return 1;
    if (str == "false" || str == "off" || str == "no"
        || str == "non" || str == "nein")
        return 0;

    int rv;
    if (extract(str, &rv))
        return rv;
    FPRINTF(stderr, "Fatal: bad option value %s", str);
    abort();
}

static_assert(sizeof(timestamp_seconds_buffer) == 20, "seconds buffer");
static_assert(sizeof(timestamp_milliseconds_buffer) == 24, "millis buffer");

void stamp_time(timestamp_seconds_buffer& out, const TimeT *t)
{
    struct tm when = t ? *t : TimeT::now();
    char buf[20];
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &when);
    out = stringish<timestamp_seconds_buffer>(const_(buf));
}
void stamp_time(timestamp_milliseconds_buffer& out)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm when = TimeT(tv.tv_sec);
    char buf[24];
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &when);
    sprintf(buf + 19, ".%03d", static_cast<int>(tv.tv_usec / 1000));
    out = stringish<timestamp_milliseconds_buffer>(const_(buf));
}

void log_with_timestamp(io::WriteFile& out, XString line)
{
    if (!line)
    {
        out.put('\n');
        return;
    }
    timestamp_milliseconds_buffer tmpstr;
    stamp_time(tmpstr);
    out.really_put(tmpstr.data(), tmpstr.size());
    out.really_put(": ", 2);
    out.put_line(line);
}
