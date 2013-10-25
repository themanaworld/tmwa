#include "utils.hpp"

#include <netinet/in.h>
#include <sys/time.h>

#include <algorithm>

#include "../strings/fstring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "cxxstdio.hpp"
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

bool split_key_value(ZString line, XString *w1, ZString *w2)
{
    if (line.startswith("//"))
        return false;
    if (!line)
        return false;

    if (std::find_if(line.begin(), line.end(),
                [](unsigned char c) { return c < ' '; }
                ) != line.end())
        return false;
    ZString::iterator colon = std::find(line.begin(), line.end(), ':');
    if (colon == line.end())
        return false;
    *w1 = line.xislice_h(colon);
    ++colon;
    while (std::isspace(*colon))
        ++colon;
    *w2 = line.xislice_t(colon);
    return true;
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

void log_with_timestamp(FILE *out, XString line)
{
    if (!line)
    {
        fputc('\n', out);
        return;
    }
    timestamp_milliseconds_buffer tmpstr;
    stamp_time(tmpstr);
    fputs(tmpstr.c_str(), out);
    fputs(": ", out);
    fwrite(line.data(), 1, line.size(), out);
    if (line.back() != '\n')
        fputc('\n', out);
}
