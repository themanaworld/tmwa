#include "utils.hpp"

#include <netinet/in.h>
#include <sys/time.h>

#include <algorithm>

#include "../poison.hpp"

//-----------------------------------------------------
// Function to suppress control characters in a string.
//-----------------------------------------------------
int remove_control_chars(char *str)
{
    int i;
    int change = 0;

    for (i = 0; str[i]; i++)
    {
        if (0 <= str[i] && str[i] < 32)
        {
            str[i] = '_';
            change = 1;
        }
    }

    return change;
}

//---------------------------------------------------
// E-mail check: return 0 (not correct) or 1 (valid).
//---------------------------------------------------
int e_mail_check(const char *email)
{
    char ch;
    const char *last_arobas;

    // athena limits
    if (strlen(email) < 3 || strlen(email) > 39)
        return 0;

    // part of RFC limits (official reference of e-mail description)
    if (strchr(email, '@') == NULL || email[strlen(email) - 1] == '@')
        return 0;

    if (email[strlen(email) - 1] == '.')
        return 0;

    last_arobas = strrchr(email, '@');

    if (strstr(last_arobas, "@.") != NULL ||
        strstr(last_arobas, "..") != NULL)
        return 0;

    for (ch = 1; ch < 32; ch++)
    {
        if (strchr(last_arobas, ch) != NULL)
        {
            return 0;
        }
    }

    if (strchr(last_arobas, ' ') != NULL ||
        strchr(last_arobas, ';') != NULL)
        return 0;

    // all correct
    return 1;
}

//-------------------------------------------------
// Return numerical value of a switch configuration
// on/off, english, français, deutsch, español
//-------------------------------------------------
int config_switch (const char *str)
{
    if (strcasecmp(str, "on") == 0 || strcasecmp(str, "yes") == 0
        || strcasecmp(str, "oui") == 0 || strcasecmp(str, "ja") == 0
        || strcasecmp(str, "si") == 0)
        return 1;
    if (strcasecmp(str, "off") == 0 || strcasecmp(str, "no") == 0
        || strcasecmp(str, "non") == 0 || strcasecmp(str, "nein") == 0)
        return 0;

    return atoi(str);
}

const char *ip2str(struct in_addr ip, bool extra_dot)
{
    const uint8_t *p = (const uint8_t *)(&ip);
    static char buf[17];
    if (extra_dot)
        sprintf(buf, "%d.%d.%d.%d.", p[0], p[1], p[2], p[3]);
    else
        sprintf(buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return buf;
}

bool split_key_value(const std::string& line, std::string *w1, std::string *w2)
{
    std::string::const_iterator begin = line.begin(), end = line.end();

    if (line[0] == '/' && line[1] == '/')
        return false;
    if (line.back() == '\r')
        --end;
    if (line.empty())
        return false;

    if (std::find_if(begin, end,
                [](unsigned char c) { return c < ' '; }
                ) != line.end())
        return false;
    std::string::const_iterator colon = std::find(begin, end, ':');
    if (colon == end)
        return false;
    w1->assign(begin, colon);
    ++colon;
    while (std::isspace(*colon))
        ++colon;
    w2->assign(colon, end);
    return true;
}

static_assert(sizeof(timestamp_seconds_buffer) == 20, "seconds buffer");
static_assert(sizeof(timestamp_milliseconds_buffer) == 24, "millis buffer");

void stamp_time(timestamp_seconds_buffer& out, TimeT *t)
{
    struct tm when = t ? *t : TimeT::now();
    strftime(out, 20, "%Y-%m-%d %H:%M:%S", &when);
}
void stamp_time(timestamp_milliseconds_buffer& out)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm when = TimeT(tv.tv_sec);
    strftime(out, 20, "%Y-%m-%d %H:%M:%S", &when);
    sprintf(out + 19, ".%03d", int(tv.tv_usec / 1000));
}

void log_with_timestamp(FILE *out, const_string line)
{
    if (!line)
    {
        fputc('\n', out);
        return;
    }
    timestamp_milliseconds_buffer tmpstr;
    stamp_time(tmpstr);
    fwrite(tmpstr, 1, sizeof(tmpstr), out);
    fputs(": ", out);
    fwrite(line.data(), 1, line.size(), out);
    if (line.back() != '\n')
        fputc('\n', out);
}
