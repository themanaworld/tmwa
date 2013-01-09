#ifndef LOGIN_HPP
#define LOGIN_HPP

#define MAX_SERVERS 30

#define LOGIN_CONF_NAME "conf/login_athena.conf"
#define LAN_CONF_NAME "conf/lan_support.conf"

#define START_ACCOUNT_NUM 2000000
#define END_ACCOUNT_NUM 100000000

struct mmo_account
{
    char userid[24];
    char passwd[24];
    int passwdenc;

    long account_id;
    long login_id1;
    long login_id2;
    long char_id;
    char lastlogin[24];
    int sex;
};

struct mmo_char_server
{
    char name[20];
    long ip;
    short port;
    int users;
    int maintenance;
    int is_new;
};

#endif // LOGIN_HPP
