#include "utils.hpp"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <netinet/in.h>

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
            break;
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
