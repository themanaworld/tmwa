#include "login.hpp"
//    login.cpp - Core of the login server.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2014 MadCamel
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/wait.h>

#include <netdb.h>
#include <unistd.h>

#include <sys/resource.h>

#include <ctime>

#include <algorithm>
#include <set>

#include "../ints/udl.hpp"

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../generic/db.hpp"
#include "../generic/random.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/tty.hpp"
#include "../io/write.hpp"

#include "../net/socket.hpp"
#include "../net/timer.hpp"
#include "../net/vomit.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/core.hpp"
#include "../mmo/extract.hpp"
#include "../mmo/human_time_diff.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/md5more.hpp"
#include "../mmo/mmo.hpp"
#include "../mmo/utils.hpp"
#include "../mmo/version.hpp"

#include "../poison.hpp"

constexpr int MAX_SERVERS = 30;

constexpr AccountId START_ACCOUNT_NUM = wrap<AccountId>(2000000);
constexpr AccountId END_ACCOUNT_NUM = wrap<AccountId>(100000000);

struct mmo_account
{
    AccountName userid;
    AccountPass passwd;
    int passwdenc;

    AccountId account_id;
    int login_id1;
    int login_id2;
    AccountId char_id;
    timestamp_milliseconds_buffer lastlogin;
    SEX sex;
};

struct mmo_char_server
{
    ServerName name;
    IP4Address ip;
    uint16_t port;
    uint16_t users;
};

static
AccountId account_id_count = START_ACCOUNT_NUM;
static
int server_num;
static
int new_account = 0;
static
int login_port = 6900;
static
IP4Address lan_char_ip = IP4_LOCALHOST;
static
IP4Mask lan_subnet = IP4Mask(IP4_LOCALHOST, IP4_BROADCAST);
static
AString update_host;
static
AccountName userid;
static
AccountPass passwd;
static
ServerName main_server;

static
AString account_filename = "save/account.txt"_s;
static
AString gm_account_filename = "save/gm_account.txt"_s;
static
AString login_log_filename = "log/login.log"_s;
static
AString login_log_unknown_packets_filename = "log/login_unknown_packets.log"_s;
static
int save_unknown_packets = 0;
static
tick_t creation_time_GM_account_file;
static
std::chrono::seconds gm_account_filename_check_timer = std::chrono::seconds(15);

static
int display_parse_login = 0;   // 0: no, 1: yes
static
int display_parse_admin = 0;   // 0: no, 1: yes
static
int display_parse_fromchar = 0;    // 0: no, 1: yes (without packet 0x2714), 2: all packets

static
Array<struct mmo_char_server, MAX_SERVERS> server;
static
Array<Session *, MAX_SERVERS> server_session;
static
Array<int, MAX_SERVERS> server_freezeflag;    // Char-server anti-freeze system. Counter. 5 ok, 4...0 freezed
static
int anti_freeze_enable = 0;
static
std::chrono::seconds anti_freeze_interval = std::chrono::seconds(15);

static
Session *login_session;

enum class ACO
{
    DENY_ALLOW,
    ALLOW_DENY,
    MUTUAL_FAILURE,
};

static
ACO access_order = ACO::DENY_ALLOW;
static
std::vector<IP4Mask>
access_allow, access_deny, access_ladmin;

static
GmLevel min_level_to_connect = GmLevel::from(0_u32);  // minimum level of player/GM (0: player, 1-99: gm) to connect on the server
static
int add_to_unlimited_account = 0;  // Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account.
static
int start_limited_time = -1;   // Starting additional sec from now for the limited time at creation of accounts (-1: unlimited time, 0 or more: additional sec from now)
static
int check_ip_flag = 1;         // It's to check IP of a player between login-server and char-server (part of anti-hacking system)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
void SessionDeleter::operator()(SessionData *)
{
    assert(false && "login server does not have sessions anymore"_s);
}
#pragma GCC diagnostic pop

constexpr int AUTH_FIFO_SIZE = 256;
struct AuthFifo
{
    AccountId account_id;
    int login_id1, login_id2;
    IP4Address ip;
    SEX sex;
    int delflag;
};
static
Array<AuthFifo, AUTH_FIFO_SIZE> auth_fifo;
static
int auth_fifo_pos = 0;

struct AuthData
{
    AccountId account_id;
    SEX sex;
    AccountName userid;
    AccountCrypt pass;
    timestamp_milliseconds_buffer lastlogin;
    int logincount;
    int state;                 // packet 0x006a value + 1 (0: compte OK)
    AccountEmail email;             // e-mail (by default: a@a.com)
    timestamp_seconds_buffer error_message;     // Message of error code #6 = Your are Prohibited to log in until %s (packet 0x006a)
    TimeT ban_until_time;      // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
    TimeT connect_until_time;  // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
    IP4Address last_ip;           // save of last IP of connection
    VString<254> memo;             // a memo field
    int account_reg2_num;
    Array<struct global_reg, ACCOUNT_REG2_NUM> account_reg2;
};
static
std::vector<AuthData> auth_data;

static
int admin_state = 0;
static
AccountPass admin_pass;
static
AString gm_pass;
static
int level_new_gm = 60;

static
Map<AccountId, GM_Account> gm_account_db;

static
pid_t pid = 0; // For forked DB writes


namespace e
{
enum class VERSION_2 : uint8_t
{
    /// client supports updatehost
    UPDATEHOST = 0x01,
    /// send servers in forward order
    SERVERORDER = 0x02,
};
ENUM_BITWISE_OPERATORS(VERSION_2)
}
using e::VERSION_2;

//------------------------------
// Writing function of logs file
//------------------------------
#define LOGIN_LOG(fmt, ...) \
    login_log(STRPRINTF(fmt, ## __VA_ARGS__))
static
void login_log(XString line)
{
    io::AppendFile logfp(login_log_filename);
    if (!logfp.is_open())
        return;
    log_with_timestamp(logfp, line);
}

static
void delete_login(Session *sess)
{
    (void)sess;
}

static
void delete_fromchar(Session *sess)
{
    auto it = std::find(server_session.begin(), server_session.end(), sess);
    assert (it != server_session.end());
    int id = it - server_session.begin();
    IP4Address ip = sess->client_ip;
    PRINTF("Char-server '%s' has disconnected.\n"_fmt, server[id].name);
    LOGIN_LOG("Char-server '%s' has disconnected (ip: %s).\n"_fmt,
               server[id].name, ip);
    server_session[id] = nullptr;
    server[id] = mmo_char_server{};
}

static
void delete_admin(Session *s)
{
    PRINTF("Remote administration has disconnected (session #%d).\n"_fmt,
            s);
}


//----------------------------------------------------------------------
// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)
//----------------------------------------------------------------------
static
GmLevel isGM(AccountId account_id)
{
    GM_Account *p = gm_account_db.search(account_id);
    if (p == NULL)
        return GmLevel();
    return p->level;
}

//-------------------------------------------------------
// Reading function of GM accounts file (and their level)
//-------------------------------------------------------
static
int read_gm_account(void)
{
    int c = 0;

    gm_account_db.clear();

    creation_time_GM_account_file = file_modified(gm_account_filename);

    io::ReadFile fp(gm_account_filename);
    if (!fp.is_open())
    {
        PRINTF("read_gm_account: GM accounts file [%s] not found.\n"_fmt,
                gm_account_filename);
        PRINTF("                 Actually, there is no GM accounts on the server.\n"_fmt);
        LOGIN_LOG("read_gm_account: GM accounts file [%s] not found.\n"_fmt,
                   gm_account_filename);
        LOGIN_LOG("                 Actually, there is no GM accounts on the server.\n"_fmt);
        return 1;
    }
    // limited to 4000, because we send information to char-servers (more than 4000 GM accounts???)
    // int (id) + int (level) = 8 bytes * 4000 = 32k (limit of packets in windows)
    AString line;
    while (fp.getline(line) && c < 4000)
    {
        if (is_comment(line))
            continue;
        GM_Account p {};
        if (!extract(line, record<' '>(&p.account_id, &p.level)))
            PRINTF("read_gm_account: file [%s], invalid 'id_acount level' format: '%s'\n"_fmt,
                 gm_account_filename, line);
        else
        {
            GmLevel GM_level = isGM(p.account_id);
            if (GM_level)
            {                   // if it's not a new account
                if (GM_level == p.level)
                    PRINTF("read_gm_account: GM account %d defined twice (same level: %d).\n"_fmt,
                         p.account_id, p.level);
                else
                    PRINTF("read_gm_account: GM account %d defined twice (levels: %d and %d).\n"_fmt,
                         p.account_id, GM_level, p.level);
            }
            if (GM_level != p.level)
            {                   // if new account or new level
                gm_account_db.insert(p.account_id, p);
                if (!GM_level)
                {               // if new account
                    c++;
                    if (c >= 4000)
                    {
                        PRINTF("***WARNING: 4000 GM accounts found. Next GM accounts are not readed.\n"_fmt);
                        LOGIN_LOG("***WARNING: 4000 GM accounts found. Next GM accounts are not readed.\n"_fmt);
                    }
                }
            }
        }
    }

    PRINTF("read_gm_account: file '%s' readed (%d GM accounts found).\n"_fmt,
            gm_account_filename, c);
    LOGIN_LOG("read_gm_account: file '%s' readed (%d GM accounts found).\n"_fmt,
               gm_account_filename, c);

    return 0;
}

//---------------------
// Access control by IP
//---------------------
static
bool check_ip(IP4Address ip)
{
    enum class ACF
    {
        DEF,
        ALLOW,
        DENY
    };
    ACF flag = ACF::DEF;

    if (access_allow.empty() && access_deny.empty())
        return 1;
    // When there is no restriction, all IP are authorised.

    if (std::find_if(access_allow.begin(), access_allow.end(),
                [&ip](IP4Mask m)
                {
                    return m.covers(ip);
                }) != access_allow.end())
    {
        {
            flag = ACF::ALLOW;
            if (access_order == ACO::ALLOW_DENY)
                // With 'allow, deny' (deny if not allow), allow has priority
                return 1;
        }
    }

    if (std::find_if(access_deny.begin(), access_deny.end(),
                [&ip](IP4Mask m)
                {
                    return m.covers(ip);
                }) != access_deny.end())
    {
        {
            flag = ACF::DENY;
            return 0;
            // At this point, if it's 'deny', we refuse connection.
        }
    }

    return flag == ACF::ALLOW || access_order == ACO::DENY_ALLOW;
    // With 'mutual-failture', only 'allow' and non 'deny' IP are authorised.
    //   A non 'allow' (even non 'deny') IP is not authorised. It's like: if allowed and not denied, it's authorised.
    //   So, it's disapproval if you have no description at the time of 'mutual-failture'.
    // With 'deny,allow' (allow if not deny), because here it's not deny, we authorise.
}

//--------------------------------
// Access control by IP for ladmin
//--------------------------------
static
bool check_ladminip(IP4Address ip)
{
    if (access_ladmin.empty())
        // When there is no restriction, all IP are authorised.
        return true;

    return std::find_if(access_ladmin.begin(), access_ladmin.end(),
            [&ip](IP4Mask m)
            {
                return m.covers(ip);
            }) != access_ladmin.end();
}

//-----------------------------------------------
// Search an account id
//   (return account pointer or nullptr (if not found))
//-----------------------------------------------
static
AuthData *search_account(AccountName account_name)
{
    for (AuthData& ad : auth_data)
    {
        {
            if (ad.userid == account_name)
                return &ad;
        }
    }

    return nullptr;
}

//--------------------------------------------------------
// Create a string to save the account in the account file
//--------------------------------------------------------
static
AString mmo_auth_tostr(const AuthData *p)
{
    MString str;
    str += STRPRINTF(
            "%d\t"
            "%s\t"
            "%s\t"
            "%s\t"
            "%c\t"
            "%d\t"
            "%d\t"
            "%s\t"
            "%s\t"
            "%lld\t"
            "%s\t"
            "%s\t"
            "%lld\t"_fmt,
            p->account_id,
            p->userid,
            p->pass,
            p->lastlogin,
            sex_to_char(p->sex),
            p->logincount,
            p->state,
            p->email,
            p->error_message,
            p->connect_until_time,
            p->last_ip,
            p->memo,
            p->ban_until_time);

    assert (p->account_reg2_num < ACCOUNT_REG2_NUM);
    for (int i = 0; i < p->account_reg2_num; i++)
        if (p->account_reg2[i].str)
            str += STRPRINTF("%s,%d "_fmt,
                    p->account_reg2[i].str, p->account_reg2[i].value);

    return AString(str);
}

static
bool extract(XString line, AuthData *ad)
{
    std::vector<struct global_reg> vars;
    VString<1> sex;
    VString<15> ip;
    if (!extract(line,
                record<'\t'>(
                    &ad->account_id,
                    &ad->userid,
                    &ad->pass,
                    &ad->lastlogin,
                    &sex,
                    &ad->logincount,
                    &ad->state,
                    &ad->email,
                    &ad->error_message,
                    &ad->connect_until_time,
                    &ip,
                    &ad->memo,
                    &ad->ban_until_time,
                    vrec<' '>(&vars))))
        return false;
    ad->last_ip = IP4Address();
    if (ip != "-"_s && !extract(ip, &ad->last_ip))
        return false;
    if (!(ad->account_id < END_ACCOUNT_NUM))
        return false;
    // TODO replace *every* lookup with a map lookup
    static std::set<AccountId> seen_ids;
    static std::set<AccountName> seen_names;
    // we don't have to worry about deleted characters,
    // this is only called during startup
    auto _seen_id = seen_ids.insert(ad->account_id);
    if (!_seen_id.second)
        return false;
    auto _seen_name = seen_names.insert(ad->userid);
    if (!_seen_name.second)
    {
        seen_ids.erase(_seen_id.first);
        return false;
    }
    // If a password is not encrypted, we encrypt it now.
    // A password beginning with ! and - in the memo field is our magic
    if (!ad->pass.startswith('!') && ad->memo.startswith('-'))
    {
        XString pass = ad->pass;
        AccountPass plain = stringish<AccountPass>(pass);
        ad->pass = MD5_saltcrypt(plain, make_salt());
        ad->memo = '!';
    }

    if (sex.size() != 1)
        return false;
    ad->sex  = sex_from_char(sex.front());
    if (ad->sex == SEX::NEUTRAL)
        return false;

    if (!e_mail_check(ad->email))
        ad->email = DEFAULT_EMAIL;

    if (!ad->error_message || ad->state != 7)
        // 7, because state is packet 0x006a value + 1
        ad->error_message = stringish<timestamp_seconds_buffer>("-"_s);

    if (vars.size() > ACCOUNT_REG2_NUM)
        return false;
    std::copy(vars.begin(), vars.end(), ad->account_reg2.begin());
    ad->account_reg2_num = vars.size();

    return true;
}

//---------------------------------
// Reading of the accounts database
//---------------------------------
static
int mmo_auth_init(void)
{
    int gm_count = 0;

    io::ReadFile in(account_filename);
    if (!in.is_open())
    {
        // no account file -> no account -> no login, including char-server (ERROR)
        // not anymore! :-)
        PRINTF(SGR_BOLD SGR_RED "mmo_auth_init: Accounts file [%s] not found." SGR_RESET "\n"_fmt,
             account_filename);
        return 0;
    }

    AString line;
    while (in.getline(line))
    {
        if (is_comment(line))
            continue;
        if (std::find_if(line.begin(), line.end(),
                    [](unsigned char c) { return c < ' ' && c != '\t'; }
                    ) != line.end())
            continue;

        AuthData ad {};
        if (!extract(line, &ad))
        {
            if (extract(line, record<'\t'>(&ad.account_id, "%newid%"_s)))
            {
                if (account_id_count < ad.account_id)
                    account_id_count = ad.account_id;
            }
            else
                LOGIN_LOG("Account skipped\n%s"_fmt, line);
            continue;
        }

        auth_data.push_back(ad);

        if (isGM(ad.account_id))
            gm_count++;

        if (account_id_count < next(ad.account_id))
            account_id_count = next(ad.account_id);
    }

    AString str = STRPRINTF("%s has %zu accounts (%d GMs)\n"_fmt,
            account_filename, auth_data.size(), gm_count);
    PRINTF("mmo_auth_init: %s\n"_fmt, str);
    LOGIN_LOG("%s\n"_fmt, line);

    return 0;
}

//------------------------------------------
// Writing of the accounts database file
//------------------------------------------
static
void mmo_auth_sync(void)
{
    io::WriteLock fp(account_filename);

    if (!fp.is_open())
    {
        PRINTF("uh-oh - unable to save accounts\n"_fmt);
        return;
    }
    FPRINTF(fp,
             "// Accounts file: here are saved all information about the accounts.\n"_fmt);
    FPRINTF(fp,
             "// Structure: ID, account name, password, last login time, sex, # of logins, state, email, error message for state 7, validity time, last (accepted) login ip, memo field, ban timestamp, repeated(register text, register value)\n"_fmt);
    FPRINTF(fp, "// Some explanations:\n"_fmt);
    FPRINTF(fp,
             "//   account name    : between 4 to 23 char for a normal account (standard client can't send less than 4 char).\n"_fmt);
    FPRINTF(fp, "//   account password: between 4 to 23 char\n"_fmt);
    FPRINTF(fp,
             "//   sex             : M or F for normal accounts, S for server accounts\n"_fmt);
    FPRINTF(fp,
             "//   state           : 0: account is ok, 1 to 256: error code of packet 0x006a + 1\n"_fmt);
    FPRINTF(fp,
             "//   email           : between 3 to 39 char (a@a.com is like no email)\n"_fmt);
    FPRINTF(fp,
             "//   error message   : text for the state 7: 'Your are Prohibited to login until <text>'. Max 19 char\n"_fmt);
    FPRINTF(fp,
             "//   valitidy time   : 0: unlimited account, <other value>: date calculated by addition of 1/1/1970 + value (number of seconds since the 1/1/1970)\n"_fmt);
    FPRINTF(fp, "//   memo field      : max 254 char\n"_fmt);
    FPRINTF(fp,
             "//   ban time        : 0: no ban, <other value>: banned until the date: date calculated by addition of 1/1/1970 + value (number of seconds since the 1/1/1970)\n"_fmt);
    for (const AuthData& ad : auth_data)
    {
        if (!ad.account_id)
            continue;

        AString line = mmo_auth_tostr(&ad);
        fp.put_line(line);
    }
    FPRINTF(fp, "%d\t%%newid%%\n"_fmt, account_id_count);
}

// We want to sync the DB to disk as little as possible as it's fairly
// resource intensive. therefore most player-triggerable events that
// update the account DB will not immideately trigger a save. Instead
// we save periodicly on a timer.
//-----------------------------------------------------
static
void check_auth_sync(TimerData *, tick_t)
{
    if (pid != 0)
    {
        int status;
        pid_t temp = waitpid(pid, &status, WNOHANG);

        // Need to check status too?
        if (temp == 0)
        {
            return;
        }
    }

    // This can take a lot of time. Fork a child to handle the work and return at once
    // If we're unable to fork just continue running the function normally
    if ((pid = fork()) > 0)
        return;

    // If we're a child, run as a lower priority process
    if (pid == 0)
        setpriority(PRIO_PROCESS, getpid(), 10);

    mmo_auth_sync();

    // If we're a child we should suicide now.
    if (pid == 0)
        _exit(0);

    return;
}

//--------------------------------------------------------------------
// Packet send to all char-servers, except one (wos: without our self)
//--------------------------------------------------------------------
static
void charif_sendallwos(Session *ss, const uint8_t *buf, size_t len)
{
    for (int i = 0; i < MAX_SERVERS; i++)
    {
        Session *s = server_session[i];
        if (s && s != ss)
        {
            WFIFO_BUF_CLONE(s, buf, len);
            WFIFOSET(s, len);
        }
    }
}

//-----------------------------------------------------
// Send GM accounts to all char-server
//-----------------------------------------------------
static
void send_GM_accounts(void)
{
    uint8_t buf[32000];
    int len;

    len = 4;
    WBUFW(buf, 0) = 0x2732;
    for (const AuthData& ad : auth_data)
    {
        // send only existing accounts. We can not create a GM account when server is online.
        if (GmLevel GM_value = isGM(ad.account_id))
        {
            WBUFL(buf, len) = unwrap<AccountId>(ad.account_id);
            WBUFB(buf, len + 4) = static_cast<uint8_t>(GM_value.get_all_bits());
            len += 5;
        }
    }
    WBUFW(buf, 2) = len;
    charif_sendallwos(nullptr, buf, len);
}

//-----------------------------------------------------
// Check if GM file account have been changed
//-----------------------------------------------------
static
void check_GM_file(TimerData *, tick_t)
{
    // if we would not check
    if (gm_account_filename_check_timer == interval_t::zero())
        return;

    // get last modify time/date
    tick_t new_time = file_modified(gm_account_filename);

    if (new_time != creation_time_GM_account_file)
    {
        read_gm_account();
        send_GM_accounts();
    }
}

//-------------------------------------
// Account creation (with e-mail check)
//-------------------------------------
static
AccountId mmo_auth_new(struct mmo_account *account, SEX sex, AccountEmail email)
{
    while (isGM(account_id_count))
        account_id_count = next(account_id_count);

    struct AuthData ad {};
    ad.account_id = account_id_count;
    account_id_count = next(account_id_count);

    ad.userid = account->userid;
    ad.pass = MD5_saltcrypt(account->passwd, make_salt());
    ad.lastlogin = stringish<timestamp_milliseconds_buffer>("-"_s);
    ad.sex = sex;
    ad.logincount = 0;
    ad.state = 0;

    if (!e_mail_check(email))
        ad.email = DEFAULT_EMAIL;
    else
        ad.email = email;

    ad.error_message = stringish<timestamp_seconds_buffer>("-"_s);
    ad.ban_until_time = TimeT();

    if (start_limited_time < 0)
        ad.connect_until_time = TimeT(); // unlimited
    else
    {
        // limited time
        TimeT timestamp = static_cast<time_t>(TimeT::now()) + start_limited_time;
        // there used to be a silly overflow check here, but it wasn't
        // correct, and we don't support time-limited accounts.
        ad.connect_until_time = timestamp;
    }

    ad.last_ip = IP4Address();
    ad.memo = "!"_s;
    ad.account_reg2_num = 0;
    auth_data.push_back(ad);

    return ad.account_id;
}

//---------------------------------------
// Check/authentification of a connection
//---------------------------------------
static
int mmo_auth(struct mmo_account *account, Session *s)
{
    char new_account_sex = '\0';

    IP4Address ip = s->client_ip;

    // Account creation with _M/_F
    if (account->passwdenc == 0
        && (account->userid.endswith("_F"_s) || account->userid.endswith("_M"_s))
        && new_account == 1 && account_id_count < END_ACCOUNT_NUM
        && (account->userid.size() - 2) >= 4 && account->passwd.size() >= 4)
    {
        new_account_sex = account->userid.back();
        account->userid = stringish<AccountName>(account->userid.xrslice_h(2));
    }

    // Strict account search
    AuthData *ad = search_account(account->userid);

    if (ad)
    {
        int encpasswdok = 0;
        if (new_account_sex)
        {
            LOGIN_LOG("Attempt of creation of an already existant account (account: %s_%c, ip: %s)\n"_fmt,
                 account->userid, new_account_sex, ip);
            return 9;           // 9 = Account already exists
        }
        if ((!pass_ok(account->passwd, ad->pass)) && !encpasswdok)
        {
            if (account->passwdenc == 0)
                LOGIN_LOG("Invalid password (account: %s, ip: %s)\n"_fmt,
                     account->userid, ip);

            return 1;           // 1 = Incorrect Password
        }

        if (ad->state)
        {
            LOGIN_LOG("Connection refused (account: %s, state: %d, ip: %s)\n"_fmt,
                 account->userid, ad->state,
                 ip);
            switch (ad->state)
            {                   // packet 0x006a value + 1
                case 1:        // 0 = Unregistered ID
                case 2:        // 1 = Incorrect Password
                case 3:        // 2 = This ID is expired
                case 4:        // 3 = Rejected from Server
                case 5:        // 4 = You have been blocked by the GM Team
                case 6:        // 5 = Your Game's EXE file is not the latest version
                case 7:        // 6 = Your are Prohibited to log in until %s
                case 8:        // 7 = Server is jammed due to over populated
                case 9:        // 8 = No MSG (actually, all states after 9 except 99 are No MSG, use only this)
                case 100:      // 99 = This ID has been totally erased
                    return ad->state - 1;
                default:
                    return 99;  // 99 = ID has been totally erased
            }
        }

        if (ad->ban_until_time)
        {
            // if account is banned
            timestamp_seconds_buffer tmpstr;
            stamp_time(tmpstr, &ad->ban_until_time);
            if (ad->ban_until_time > TimeT::now())
            {
                // always banned
                LOGIN_LOG("Connection refused (account: %s, banned until %s, ip: %s)\n"_fmt,
                     account->userid, tmpstr, ip);
                return 6;       // 6 = Your are Prohibited to log in until %s
            }
            else
            {
                // ban is finished
                LOGIN_LOG("End of ban (account: %s, previously banned until %s -> not more banned, ip: %s)\n"_fmt,
                     account->userid, tmpstr, ip);
                ad->ban_until_time = TimeT(); // reset the ban time
            }
        }

        if (ad->connect_until_time
            && ad->connect_until_time < TimeT::now())
        {
            LOGIN_LOG("Connection refused (account: %s, expired ID, ip: %s)\n"_fmt,
                 account->userid, ip);
            return 2;           // 2 = This ID is expired
        }

        LOGIN_LOG("Authentification accepted (account: %s (id: %d), ip: %s)\n"_fmt,
                   account->userid, ad->account_id, ip);
    }
    else
    {
        if (new_account_sex == '\0')
        {
            LOGIN_LOG("Unknown account (account: %s, ip: %s)\n"_fmt,
                 account->userid, ip);
            return 0;           // 0 = Unregistered ID
        }
        else
        {
            AccountId new_id = mmo_auth_new(account, sex_from_char(new_account_sex), DEFAULT_EMAIL);
            LOGIN_LOG("Account creation and authentification accepted (account %s (id: %d), sex: %c, connection with _F/_M, ip: %s)\n"_fmt,
                 account->userid, new_id,
                 new_account_sex, ip);
            ad = &auth_data.back();
        }
    }

    timestamp_milliseconds_buffer tmpstr;
    stamp_time(tmpstr);

    account->account_id = ad->account_id;
    account->login_id1 = random_::generate();
    account->login_id2 = random_::generate();
    account->lastlogin = ad->lastlogin;
    ad->lastlogin = tmpstr;
    account->sex = ad->sex;
    ad->last_ip = ip;
    ad->logincount++;

    return -1;                  // account OK
}

//-------------------------------
// Char-server anti-freeze system
//-------------------------------
static
void char_anti_freeze_system(TimerData *, tick_t)
{
    int i;

    for (i = 0; i < MAX_SERVERS; i++)
    {
        if (server_session[i])
        {                       // if char-server is online
            if (server_freezeflag[i]-- < 1)
            {                   // Char-server anti-freeze system. Counter. 5 ok, 4...0 freezed
                PRINTF("Char-server anti-freeze system: char-server #%d '%s' is freezed -> disconnection.\n"_fmt,
                     i, server[i].name);
                LOGIN_LOG("Char-server anti-freeze system: char-server #%d '%s' is freezed -> disconnection.\n"_fmt,
                     i, server[i].name);
                server_session[i]->set_eof();
            }
        }
    }
}

//--------------------------------
// Packet parsing for char-servers
//--------------------------------
static
void parse_fromchar(Session *s)
{
    IP4Address ip = s->client_ip;

    int id;
    for (id = 0; id < MAX_SERVERS; id++)
        if (server_session[id] == s)
            break;
    if (id == MAX_SERVERS)
    {
        s->set_eof();
        return;
    }

    while (RFIFOREST(s) >= 2)
    {
        if (display_parse_fromchar == 2 || (display_parse_fromchar == 1 && RFIFOW(s, 0) != 0x2714))   // 0x2714 is done very often (number of players)
            PRINTF("parse_fromchar: connection #%d, packet: 0x%x (with being read: %zu bytes).\n"_fmt,
                 s, RFIFOW(s, 0), RFIFOREST(s));

        switch (RFIFOW(s, 0))
        {
                // request from map-server via char-server to reload GM accounts (by Yor).
            case 0x2709:
                LOGIN_LOG("Char-server '%s': Request to re-load GM configuration file (ip: %s).\n"_fmt,
                     server[id].name, ip);
                read_gm_account();
                // send GM accounts to all char-servers
                send_GM_accounts();
                RFIFOSKIP(s, 2);
                break;

            case 0x2712:       // request from char-server to authentify an account
                if (RFIFOREST(s) < 19)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    int i;
                    for (i = 0; i < AUTH_FIFO_SIZE; i++)
                    {
                        if (auth_fifo[i].account_id == acc &&
                            auth_fifo[i].login_id1 == RFIFOL(s, 6) &&
                            auth_fifo[i].login_id2 == RFIFOL(s, 10) &&    // relate to the versions higher than 18
                            auth_fifo[i].sex == static_cast<SEX>(RFIFOB(s, 14)) &&
                            (!check_ip_flag
                             || auth_fifo[i].ip == RFIFOIP(s, 15))
                            && !auth_fifo[i].delflag)
                        {
                            int p;
                            auth_fifo[i].delflag = 1;
                            LOGIN_LOG("Char-server '%s': authentification of the account %d accepted (ip: %s).\n"_fmt,
                                 server[id].name, acc, ip);
                            for (const AuthData& ad : auth_data)
                            {
                                if (ad.account_id == acc)
                                {
                                    WFIFOW(s, 0) = 0x2729;    // Sending of the account_reg2
                                    WFIFOL(s, 4) = unwrap<AccountId>(acc);
                                    int j;
                                    for (p = 8, j = 0;
                                         j < ad.account_reg2_num;
                                         p += 36, j++)
                                    {
                                        WFIFO_STRING(s, p, ad.account_reg2[j].str, 32);
                                        WFIFOL(s, p + 32) = ad.account_reg2[j].value;
                                    }
                                    WFIFOW(s, 2) = p;
                                    WFIFOSET(s, p);
                                    WFIFOW(s, 0) = 0x2713;
                                    WFIFOL(s, 2) = unwrap<AccountId>(acc);
                                    WFIFOB(s, 6) = 0;
                                    WFIFO_STRING(s, 7, ad.email, 40);
                                    WFIFOL(s, 47) = static_cast<time_t>(ad.connect_until_time);
                                    WFIFOSET(s, 51);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    // authentification not found
                    if (i == AUTH_FIFO_SIZE)
                    {
                        LOGIN_LOG("Char-server '%s': authentification of the account %d REFUSED (ip: %s).\n"_fmt,
                             server[id].name, acc, ip);
                        WFIFOW(s, 0) = 0x2713;
                        WFIFOL(s, 2) = unwrap<AccountId>(acc);
                        WFIFOB(s, 6) = 1;
                        // It is unnecessary to send email
                        // It is unnecessary to send validity date of the account
                        WFIFOSET(s, 51);
                    }
                }
                RFIFOSKIP(s, 19);
                break;

            case 0x2714:
                if (RFIFOREST(s) < 6)
                    return;
                server[id].users = RFIFOL(s, 2);
                if (anti_freeze_enable)
                    server_freezeflag[id] = 5;  // Char anti-freeze system. Counter. 5 ok, 4...0 freezed
                RFIFOSKIP(s, 6);
                break;

                // we receive a e-mail creation of an account with a default e-mail (no answer)
            case 0x2715:
                if (RFIFOREST(s) < 46)
                    return;
            {
                AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                AccountEmail email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 6));
                if (!e_mail_check(email))
                    LOGIN_LOG("Char-server '%s': Attempt to create an e-mail on an account with a default e-mail REFUSED - e-mail is invalid (account: %d, ip: %s)\n"_fmt,
                         server[id].name, acc, ip);
                else
                {
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc
                            && (ad.email == DEFAULT_EMAIL || !ad.email))
                        {
                            ad.email = email;
                            LOGIN_LOG("Char-server '%s': Create an e-mail on an account with a default e-mail (account: %d, new e-mail: %s, ip: %s).\n"_fmt,
                                 server[id].name, acc, email, ip);
                            goto x2715_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': Attempt to create an e-mail on an account with a default e-mail REFUSED - account doesn't exist or e-mail of account isn't default e-mail (account: %d, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                }
            x2715_out:
                RFIFOSKIP(s, 46);
                break;

                // We receive an e-mail/limited time request, because a player comes back from a map-server to the char-server
            }
            case 0x2716:
                if (RFIFOREST(s) < 6)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                for (const AuthData& ad : auth_data)
                {
                    if (ad.account_id == account_id)
                    {
                        LOGIN_LOG("Char-server '%s': e-mail of the account %d found (ip: %s).\n"_fmt,
                                server[id].name, account_id, ip);
                        WFIFOW(s, 0) = 0x2717;
                        WFIFOL(s, 2) = unwrap<AccountId>(account_id);
                        WFIFO_STRING(s, 6, ad.email, 40);
                        WFIFOL(s, 46) = static_cast<time_t>(ad.connect_until_time);
                        WFIFOSET(s, 50);
                        goto x2716_end;
                    }
                }
                LOGIN_LOG("Char-server '%s': e-mail of the account %d NOT found (ip: %s).\n"_fmt,
                        server[id].name, account_id, ip);
            }
            x2716_end:
                RFIFOSKIP(s, 6);
                break;

            case 0x2720:       // To become GM request
                if (RFIFOREST(s) < 4 || RFIFOREST(s) < RFIFOW(s, 2))
                    return;
                {
                    unsigned char buf[10];
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 4));
                    WBUFW(buf, 0) = 0x2721;
                    WBUFL(buf, 2) = unwrap<AccountId>(acc);
                    WBUFL(buf, 6) = 0;
                    size_t len = RFIFOW(s, 2) - 8;
                    AString pass = RFIFO_STRING(s, 8, len);

                    if (pass == gm_pass)
                    {
                        // only non-GM can become GM
                        if (!isGM(acc))
                        {
                            // if we autorise creation
                            if (level_new_gm)
                            {
                                // if we can open the file to add the new GM
                                io::AppendFile fp(gm_account_filename);
                                if (fp.is_open())
                                {
                                    timestamp_seconds_buffer tmpstr;
                                    stamp_time(tmpstr);
                                    FPRINTF(fp,
                                             "\n// %s: @GM command on account %d\n%d %d\n"_fmt,
                                             tmpstr,
                                             acc, acc, level_new_gm);
                                    if (!fp.close())
                                    {
                                        PRINTF("warning: didn't actually save GM file\n"_fmt);
                                    }
                                    WBUFL(buf, 6) = level_new_gm;
                                    read_gm_account();
                                    send_GM_accounts();
                                    PRINTF("GM Change of the account %d: level 0 -> %d.\n"_fmt,
                                         acc, level_new_gm);
                                    LOGIN_LOG("Char-server '%s': GM Change of the account %d: level 0 -> %d (ip: %s).\n"_fmt,
                                         server[id].name, acc,
                                         level_new_gm, ip);
                                }
                                else
                                {
                                    PRINTF("Error of GM change (suggested account: %d, correct password, unable to add a GM account in GM accounts file)\n"_fmt,
                                         acc);
                                    LOGIN_LOG("Char-server '%s': Error of GM change (suggested account: %d, correct password, unable to add a GM account in GM accounts file, ip: %s).\n"_fmt,
                                         server[id].name, acc, ip);
                                }
                            }
                            else
                            {
                                PRINTF("Error of GM change (suggested account: %d, correct password, but GM creation is disable (level_new_gm = 0))\n"_fmt,
                                     acc);
                                LOGIN_LOG("Char-server '%s': Error of GM change (suggested account: %d, correct password, but GM creation is disable (level_new_gm = 0), ip: %s).\n"_fmt,
                                     server[id].name, acc, ip);
                            }
                        }
                        else
                        {
                            PRINTF("Error of GM change (suggested account: %d (already GM), correct password).\n"_fmt,
                                 acc);
                            LOGIN_LOG("Char-server '%s': Error of GM change (suggested account: %d (already GM), correct password, ip: %s).\n"_fmt,
                                 server[id].name, acc, ip);
                        }
                    }
                    else
                    {
                        PRINTF("Error of GM change (suggested account: %d, invalid password).\n"_fmt,
                             acc);
                        LOGIN_LOG("Char-server '%s': Error of GM change (suggested account: %d, invalid password, ip: %s).\n"_fmt,
                             server[id].name, acc, ip);
                    }
                    charif_sendallwos(nullptr, buf, 10);
                }
                RFIFOSKIP(s, RFIFOW(s, 2));
                return;

                // Map server send information to change an email of an account via char-server
            case 0x2722:       // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
                if (RFIFOREST(s) < 86)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    AccountEmail actual_email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 6).to_print());
                    AccountEmail new_email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 46));
                    if (!e_mail_check(actual_email))
                        LOGIN_LOG("Char-server '%s': Attempt to modify an e-mail on an account (@email GM command), but actual email is invalid (account: %d, ip: %s)\n"_fmt,
                             server[id].name, acc, ip);
                    else if (!e_mail_check(new_email))
                        LOGIN_LOG("Char-server '%s': Attempt to modify an e-mail on an account (@email GM command) with a invalid new e-mail (account: %d, ip: %s)\n"_fmt,
                             server[id].name, acc, ip);
                    else if (new_email == DEFAULT_EMAIL)
                        LOGIN_LOG("Char-server '%s': Attempt to modify an e-mail on an account (@email GM command) with a default e-mail (account: %d, ip: %s)\n"_fmt,
                             server[id].name, acc, ip);
                    else
                    {
                        for (AuthData& ad : auth_data)
                        {
                            if (ad.account_id == acc)
                            {
                                if (ad.email == actual_email)
                                {
                                    ad.email = new_email;
                                    LOGIN_LOG("Char-server '%s': Modify an e-mail on an account (@email GM command) (account: %d (%s), new e-mail: %s, ip: %s).\n"_fmt,
                                         server[id].name, acc,
                                         ad.userid, new_email, ip);
                                }
                                else
                                    LOGIN_LOG("Char-server '%s': Attempt to modify an e-mail on an account (@email GM command), but actual e-mail is incorrect (account: %d (%s), actual e-mail: %s, proposed e-mail: %s, ip: %s).\n"_fmt,
                                         server[id].name, acc,
                                         ad.userid,
                                         ad.email, actual_email, ip);
                                goto x2722_out;
                            }
                        }
                        LOGIN_LOG("Char-server '%s': Attempt to modify an e-mail on an account (@email GM command), but account doesn't exist (account: %d, ip: %s).\n"_fmt,
                                server[id].name, acc, ip);
                    }
                }
            x2722_out:
                RFIFOSKIP(s, 86);
                break;

                // Receiving of map-server via char-server a status change resquest (by Yor)
            case 0x2724:
                if (RFIFOREST(s) < 10)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    int statut = RFIFOL(s, 6);
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            if (ad.state != statut)
                            {
                                LOGIN_LOG("Char-server '%s': Status change (account: %d, new status %d, ip: %s).\n"_fmt,
                                     server[id].name, acc, statut,
                                     ip);
                                if (statut != 0)
                                {
                                    unsigned char buf[16];
                                    WBUFW(buf, 0) = 0x2731;
                                    WBUFL(buf, 2) = unwrap<AccountId>(acc);
                                    WBUFB(buf, 6) = 0; // 0: change of statut, 1: ban
                                    WBUFL(buf, 7) = statut;    // status or final date of a banishment
                                    charif_sendallwos(nullptr, buf, 11);
                                    for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                        if (auth_fifo[j].account_id == acc)
                                            auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                }
                                ad.state = statut;
                            }
                            else
                                LOGIN_LOG("Char-server '%s':  Error of Status change - actual status is already the good status (account: %d, status %d, ip: %s).\n"_fmt,
                                     server[id].name, acc, statut,
                                     ip);
                            goto x2724_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': Error of Status change (account: %d not found, suggested status %d, ip: %s).\n"_fmt,
                            server[id].name, acc, statut, ip);
                x2724_out:
                    RFIFOSKIP(s, 10);
                }
                return;

            case 0x2725:       // Receiving of map-server via char-server a ban resquest (by Yor)
                if (RFIFOREST(s) < 18)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            TimeT now = TimeT::now();
                            TimeT timestamp;
                            if (!ad.ban_until_time
                                || ad.ban_until_time < now)
                                timestamp = now;
                            else
                                timestamp = ad.ban_until_time;
                            struct tm tmtime = timestamp;
                            HumanTimeDiff ban_diff;
                            RFIFO_STRUCT(s, 6, ban_diff);
                            tmtime.tm_year += ban_diff.year;
                            tmtime.tm_mon += ban_diff.month;
                            tmtime.tm_mday += ban_diff.day;
                            tmtime.tm_hour += ban_diff.hour;
                            tmtime.tm_min += ban_diff.minute;
                            tmtime.tm_sec += ban_diff.second;
                            timestamp = tmtime;
                            if (timestamp.okay())
                            {
                                if (timestamp <= now)
                                    timestamp = TimeT();
                                if (ad.ban_until_time != timestamp)
                                {
                                    if (timestamp)
                                    {
                                        unsigned char buf[16];
                                        timestamp_seconds_buffer tmpstr;
                                        if (timestamp)
                                            stamp_time(tmpstr, &timestamp);
                                        LOGIN_LOG("Char-server '%s': Ban request (account: %d, new final date of banishment: %lld (%s), ip: %s).\n"_fmt,
                                                server[id].name, acc,
                                                timestamp,
                                                tmpstr,
                                                ip);
                                        WBUFW(buf, 0) = 0x2731;
                                        WBUFL(buf, 2) = unwrap<AccountId>(ad.account_id);
                                        WBUFB(buf, 6) = 1; // 0: change of statut, 1: ban
                                        WBUFL(buf, 7) = static_cast<time_t>(timestamp); // status or final date of a banishment
                                        charif_sendallwos(nullptr, buf, 11);
                                        for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                        {
                                            if (auth_fifo[j].account_id == acc)
                                                auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                        }
                                    }
                                    else
                                    {
                                        LOGIN_LOG("Char-server '%s': Error of ban request (account: %d, new date unbans the account, ip: %s).\n"_fmt,
                                             server[id].name, acc,
                                             ip);
                                    }
                                    ad.ban_until_time = timestamp;
                                }
                                else
                                {
                                    LOGIN_LOG("Char-server '%s': Error of ban request (account: %d, no change for ban date, ip: %s).\n"_fmt,
                                         server[id].name, acc, ip);
                                }
                            }
                            else
                            {
                                LOGIN_LOG("Char-server '%s': Error of ban request (account: %d, invalid date, ip: %s).\n"_fmt,
                                     server[id].name, acc, ip);
                            }
                            goto x2725_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': Error of ban request (account: %d not found, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                x2725_out:
                    RFIFOSKIP(s, 18);
                }
                return;

            case 0x2727:       // Change of sex (sex is reversed)
                if (RFIFOREST(s) < 6)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            {
                                unsigned char buf[16];
                                SEX sex;
                                if (ad.sex == SEX::FEMALE)
                                    sex = SEX::MALE;
                                else
                                    sex = SEX::FEMALE;
                                LOGIN_LOG("Char-server '%s': Sex change (account: %d, new sex %c, ip: %s).\n"_fmt,
                                     server[id].name, acc,
                                     sex_to_char(sex),
                                     ip);
                                for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                {
                                    if (auth_fifo[j].account_id == acc)
                                        auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                }
                                ad.sex = sex;
                                WBUFW(buf, 0) = 0x2723;
                                WBUFL(buf, 2) = unwrap<AccountId>(acc);
                                WBUFB(buf, 6) = static_cast<uint8_t>(sex);
                                charif_sendallwos(nullptr, buf, 7);
                            }
                            goto x2727_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': Error of sex change (account: %d not found, sex would be reversed, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                x2727_out:
                    RFIFOSKIP(s, 6);
                }
                return;

            case 0x2728:       // We receive account_reg2 from a char-server, and we send them to other char-servers.
                if (RFIFOREST(s) < 4 || RFIFOREST(s) < RFIFOW(s, 2))
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 4));
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            LOGIN_LOG("Char-server '%s': receiving (from the char-server) of account_reg2 (account: %d, ip: %s).\n"_fmt,
                                 server[id].name, acc, ip);
                            size_t len = RFIFOW(s, 2);
                            int j, p;
                            for (p = 8, j = 0;
                                 p < len && j < ACCOUNT_REG2_NUM;
                                 p += 36, j++)
                            {
                                ad.account_reg2[j].str = stringish<VarName>(RFIFO_STRING<32>(s, p).to_print());
                                ad.account_reg2[j].value = RFIFOL(s, p + 32);
                            }
                            ad.account_reg2_num = j;
                            // Sending information towards the other char-servers.
                            uint8_t buf[len];
                            RFIFO_BUF_CLONE(s, buf, len);
                            WBUFW(buf, 0) = 0x2729;
                            charif_sendallwos(s, buf, WBUFW(buf, 2));
                            goto x2728_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': receiving (from the char-server) of account_reg2 (account: %d not found, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                }
            x2728_out:
                RFIFOSKIP(s, RFIFOW(s, 2));
                break;

            case 0x272a:       // Receiving of map-server via char-server a unban resquest (by Yor)
                if (RFIFOREST(s) < 6)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            if (ad.ban_until_time)
                            {
                                ad.ban_until_time = TimeT();
                                LOGIN_LOG("Char-server '%s': UnBan request (account: %d, ip: %s).\n"_fmt,
                                     server[id].name, acc, ip);
                            }
                            else
                            {
                                LOGIN_LOG("Char-server '%s': Error of UnBan request (account: %d, no change for unban date, ip: %s).\n"_fmt,
                                     server[id].name, acc, ip);
                            }
                            goto x272a_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': Error of UnBan request (account: %d not found, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                x272a_out:
                    RFIFOSKIP(s, 6);
                }
                return;

                // request from char-server to change account password
            case 0x2740:       // 0x2740 <account_id>.L <actual_password>.24B <new_password>.24B
                if (RFIFOREST(s) < 54)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(s, 2));
                    AccountPass actual_pass = stringish<AccountPass>(RFIFO_STRING<24>(s, 6).to_print());
                    AccountPass new_pass = stringish<AccountPass>(RFIFO_STRING<24>(s, 30).to_print());

                    int status = 0;

                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            if (pass_ok(actual_pass, ad.pass))
                            {
                                if (new_pass.size() < 4)
                                    status = 3;
                                else
                                {
                                    status = 1;
                                    ad.pass = MD5_saltcrypt(new_pass, make_salt());
                                    LOGIN_LOG("Char-server '%s': Change pass success (account: %d (%s), ip: %s.\n"_fmt,
                                         server[id].name, acc,
                                         ad.userid, ip);
                                }
                            }
                            else
                            {
                                status = 2;
                                LOGIN_LOG("Char-server '%s': Attempt to modify a pass failed, wrong password. (account: %d (%s), ip: %s).\n"_fmt,
                                     server[id].name, acc,
                                     ad.userid, ip);
                            }
                            goto x2740_out;
                        }
                    }
                x2740_out:
                    WFIFOW(s, 0) = 0x2741;
                    WFIFOL(s, 2) = unwrap<AccountId>(acc);
                    WFIFOB(s, 6) = status;    // 0: acc not found, 1: success, 2: password mismatch, 3: pass too short
                    WFIFOSET(s, 7);
                }

                RFIFOSKIP(s, 54);
                break;

            default:
            {
                io::AppendFile logfp(login_log_unknown_packets_filename);
                if (logfp.is_open())
                {
                    timestamp_milliseconds_buffer timestr;
                    stamp_time(timestr);
                    FPRINTF(logfp,
                             "%s: receiving of an unknown packet -> disconnection\n"_fmt,
                             timestr);
                    FPRINTF(logfp,
                             "parse_fromchar: connection #%d (ip: %s), packet: 0x%x (with being read: %zu).\n"_fmt,
                             s, ip, RFIFOW(s, 0), RFIFOREST(s));
                    FPRINTF(logfp, "Detail (in hex):\n"_fmt);
                    FPRINTF(logfp,
                             "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F\n"_fmt);
                    char tmpstr[16 + 1] {};
                    int i;
                    for (i = 0; i < RFIFOREST(s); i++)
                    {
                        if ((i & 15) == 0)
                            FPRINTF(logfp, "%04X "_fmt, i);
                        FPRINTF(logfp, "%02x "_fmt, RFIFOB(s, i));
                        if (RFIFOB(s, i) > 0x1f)
                            tmpstr[i % 16] = RFIFOB(s, i);
                        else
                            tmpstr[i % 16] = '.';
                        if ((i - 7) % 16 == 0)  // -8 + 1
                            FPRINTF(logfp, " "_fmt);
                        else if ((i + 1) % 16 == 0)
                        {
                            FPRINTF(logfp, " %s\n"_fmt, tmpstr);
                            std::fill(tmpstr + 0, tmpstr + 17, '\0');
                        }
                    }
                    if (i % 16 != 0)
                    {
                        for (int j = i; j % 16 != 0; j++)
                        {
                            FPRINTF(logfp, "   "_fmt);
                            if ((j - 7) % 16 == 0)  // -8 + 1
                                FPRINTF(logfp, " "_fmt);
                        }
                        FPRINTF(logfp, " %s\n"_fmt, tmpstr);
                    }
                    FPRINTF(logfp, "\n"_fmt);
                }
            }
                PRINTF("parse_fromchar: Unknown packet 0x%x (from a char-server)! -> disconnection.\n"_fmt,
                     RFIFOW(s, 0));
                s->set_eof();
                PRINTF("Char-server has been disconnected (unknown packet).\n"_fmt);
                return;
        }
    }
    return;
}

//---------------------------------------
// Packet parsing for administation login
//---------------------------------------
static
void parse_admin(Session *s)
{
    IP4Address ip = s->client_ip;

    while (RFIFOREST(s) >= 2)
    {
        if (display_parse_admin == 1)
            PRINTF("parse_admin: connection #%d, packet: 0x%x (with being read: %zu).\n"_fmt,
                 s, RFIFOW(s, 0), RFIFOREST(s));

        switch (RFIFOW(s, 0))
        {
            case 0x7530:       // Request of the server version
                LOGIN_LOG("'ladmin': Sending of the server version (ip: %s)\n"_fmt,
                           ip);
                WFIFOW(s, 0) = 0x7531;
                WFIFO_STRUCT(s, 2, CURRENT_LOGIN_SERVER_VERSION);
                WFIFOSET(s, 10);
                RFIFOSKIP(s, 2);
                break;

            case 0x7532:       // Request of end of connection
                LOGIN_LOG("'ladmin': End of connection (ip: %s)\n"_fmt,
                           ip);
                RFIFOSKIP(s, 2);
                s->set_eof();
                break;

            case 0x7920:       // Request of an accounts list
                if (RFIFOREST(s) < 10)
                    return;
                {
                    AccountId st = wrap<AccountId>(RFIFOL(s, 2));
                    AccountId ed = wrap<AccountId>(RFIFOL(s, 6));
                    RFIFOSKIP(s, 10);
                    WFIFOW(s, 0) = 0x7921;
                    if (!(ed < END_ACCOUNT_NUM) || ed < st)
                        ed = END_ACCOUNT_NUM;
                    LOGIN_LOG("'ladmin': Sending an accounts list (ask: from %d to %d, ip: %s)\n"_fmt,
                         st, ed, ip);
                    // Sending accounts information
                    int len = 4;
                    for (const AuthData& ad : auth_data)
                    {
                        if (len >= 30000)
                            break;
                        AccountId account_id = ad.account_id;
                        if (!(account_id < st) && !(ed < account_id))
                        {
                            WFIFOL(s, len) = unwrap<AccountId>(account_id);
                            WFIFOB(s, len + 4) = static_cast<uint8_t>(isGM(account_id).get_all_bits());
                            WFIFO_STRING(s, len + 5, ad.userid, 24);
                            WFIFOB(s, len + 29) = static_cast<uint8_t>(ad.sex);
                            WFIFOL(s, len + 30) = ad.logincount;
                            if (ad.state == 0 && ad.ban_until_time)  // if no state and banished
                                WFIFOL(s, len + 34) = 7;  // 6 = Your are Prohibited to log in until %s
                            else
                                WFIFOL(s, len + 34) = ad.state;
                            len += 38;
                        }
                    }
                    WFIFOW(s, 2) = len;
                    WFIFOSET(s, len);
                }
                break;

            case 0x7924:
            {                   // [Fate] Itemfrob package: change item IDs
                if (RFIFOREST(s) < 10)
                    return;
                uint8_t buf[10];
                RFIFO_BUF_CLONE(s, buf, 10);
                // forward package to char servers
                charif_sendallwos(nullptr, buf, 10);
                RFIFOSKIP(s, 10);
                WFIFOW(s, 0) = 0x7925;
                WFIFOSET(s, 2);
                break;
            }

            case 0x7930:       // Request for an account creation
                if (RFIFOREST(s) < 91)
                    return;
                {
                    struct mmo_account ma;
                    ma.userid = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    ma.passwd = stringish<AccountPass>(RFIFO_STRING<24>(s, 26).to_print());
                    ma.lastlogin = stringish<timestamp_milliseconds_buffer>("-"_s);
                    ma.sex = sex_from_char(RFIFOB(s, 50));
                    WFIFOW(s, 0) = 0x7931;
                    WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                    WFIFO_STRING(s, 6, ma.userid, 24);
                    if (ma.userid.size() < 4 || ma.passwd.size() < 4)
                    {
                        LOGIN_LOG("'ladmin': Attempt to create an invalid account (account or pass is too short, ip: %s)\n"_fmt,
                             ip);
                    }
                    else if (ma.sex != SEX::FEMALE && ma.sex != SEX::MALE)
                    {
                        LOGIN_LOG("'ladmin': Attempt to create an invalid account (account: %s, invalid sex, ip: %s)\n"_fmt,
                             ma.userid, ip);
                    }
                    else if (!(account_id_count < END_ACCOUNT_NUM))
                    {
                        LOGIN_LOG("'ladmin': Attempt to create an account, but there is no more available id number (account: %s, sex: %c, ip: %s)\n"_fmt,
                             ma.userid, ma.sex, ip);
                    }
                    else
                    {
                        for (const AuthData& ad : auth_data)
                        {
                            if (ad.userid == ma.userid)
                            {
                                LOGIN_LOG("'ladmin': Attempt to create an already existing account (account: %s ip: %s)\n"_fmt,
                                     ad.userid, ip);
                                goto x7930_out;
                            }
                        }
                        {
                            AccountEmail email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 51));
                            AccountId new_id = mmo_auth_new(&ma, ma.sex, email);
                            LOGIN_LOG("'ladmin': Account creation (account: %s (id: %d), sex: %c, email: %s, ip: %s)\n"_fmt,
                                 ma.userid, new_id,
                                 ma.sex, auth_data.back().email, ip);
                            WFIFOL(s, 2) = unwrap<AccountId>(new_id);
                        }
                    }
                x7930_out:
                    WFIFOSET(s, 30);
                    RFIFOSKIP(s, 91);
                }
                break;

            case 0x7932:       // Request for an account deletion
                if (RFIFOREST(s) < 26)
                    return;
            {
                WFIFOW(s, 0) = 0x7933;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                AuthData *ad = search_account(account_name);
                if (ad)
                {
                    // Char-server is notified of deletion (for characters deletion).
                    uint8_t buf[6];
                    WBUFW(buf, 0) = 0x2730;
                    WBUFL(buf, 2) = unwrap<AccountId>(ad->account_id);
                    charif_sendallwos(nullptr, buf, 6);
                    // send answer
                    WFIFO_STRING(s, 6, ad->userid, 24);
                    WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                    // save deleted account in log file
                    LOGIN_LOG("'ladmin': Account deletion (account: %s, id: %d, ip: %s) - saved in next line:\n"_fmt,
                         ad->userid, ad->account_id,
                         ip);
                    {
                        AString buf2 = mmo_auth_tostr(ad);
                        LOGIN_LOG("%s\n"_fmt, buf2);
                    }
                    // delete account
                    ad->userid = AccountName();
                    ad->account_id = AccountId();
                }
                else
                {
                    WFIFO_STRING(s, 6, account_name, 24);
                    LOGIN_LOG("'ladmin': Attempt to delete an unknown account (account: %s, ip: %s)\n"_fmt,
                         account_name, ip);
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 26);
                break;

            case 0x7934:       // Request to change a password
                if (RFIFOREST(s) < 50)
                    return;
            {
                WFIFOW(s, 0) = 0x7935;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                AuthData *ad = search_account(account_name);
                if (ad)
                {
                    WFIFO_STRING(s, 6, ad->userid, 24);
                    AccountPass plain = stringish<AccountPass>(RFIFO_STRING<24>(s, 26));
                    ad->pass = MD5_saltcrypt(plain, make_salt());
                    WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                    LOGIN_LOG("'ladmin': Modification of a password (account: %s, new password: %s, ip: %s)\n"_fmt,
                         ad->userid, ad->pass, ip);
                }
                else
                {
                    WFIFO_STRING(s, 6, account_name, 24);
                    LOGIN_LOG("'ladmin': Attempt to modify the password of an unknown account (account: %s, ip: %s)\n"_fmt,
                         account_name, ip);
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 50);
                break;

            case 0x7936:       // Request to modify a state
                if (RFIFOREST(s) < 50)
                    return;
                {
                    WFIFOW(s, 0) = 0x7937;
                    WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                    AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    int statut = RFIFOL(s, 26);
                    timestamp_seconds_buffer error_message = stringish<timestamp_seconds_buffer>(RFIFO_STRING<20>(s, 30).to_print());
                    if (statut != 7 || !error_message)
                    {
                        // 7: // 6 = Your are Prohibited to log in until %s
                        error_message = stringish<timestamp_seconds_buffer>("-"_s);
                    }
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        WFIFO_STRING(s, 6, ad->userid, 24);
                        WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                        if (ad->state == statut
                            && ad->error_message == error_message)
                            LOGIN_LOG("'ladmin': Modification of a state, but the state of the account is already the good state (account: %s, received state: %d, ip: %s)\n"_fmt,
                                 account_name, statut, ip);
                        else
                        {
                            if (statut == 7)
                                LOGIN_LOG("'ladmin': Modification of a state (account: %s, new state: %d - prohibited to login until '%s', ip: %s)\n"_fmt,
                                     ad->userid, statut,
                                     error_message, ip);
                            else
                                LOGIN_LOG("'ladmin': Modification of a state (account: %s, new state: %d, ip: %s)\n"_fmt,
                                     ad->userid, statut, ip);
                            if (ad->state == 0)
                            {
                                unsigned char buf[16];
                                WBUFW(buf, 0) = 0x2731;
                                WBUFL(buf, 2) = unwrap<AccountId>(ad->account_id);
                                WBUFB(buf, 6) = 0; // 0: change of statut, 1: ban
                                WBUFL(buf, 7) = statut;    // status or final date of a banishment
                                charif_sendallwos(nullptr, buf, 11);
                                for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                    if (auth_fifo[j].account_id ==
                                        ad->account_id)
                                        auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                            }
                            ad->state = statut;
                            ad->error_message = error_message;
                        }
                    }
                    else
                    {
                        WFIFO_STRING(s, 6, account_name, 24);
                        LOGIN_LOG("'ladmin': Attempt to modify the state of an unknown account (account: %s, received state: %d, ip: %s)\n"_fmt,
                             account_name, statut, ip);
                    }
                    WFIFOL(s, 30) = statut;
                }
                WFIFOSET(s, 34);
                RFIFOSKIP(s, 50);
                break;

            case 0x7938:       // Request for servers list and # of online players
                LOGIN_LOG("'ladmin': Sending of servers list (ip: %s)\n"_fmt, ip);
                server_num = 0;
                for (int i = 0; i < MAX_SERVERS; i++)
                {
                    if (server_session[i])
                    {
                        WFIFOIP(s, 4 + server_num * 32) = server[i].ip;
                        WFIFOW(s, 4 + server_num * 32 + 4) = server[i].port;
                        WFIFO_STRING(s, 4 + server_num * 32 + 6, server[i].name, 20);
                        WFIFOW(s, 4 + server_num * 32 + 26) = server[i].users;
                        WFIFOW(s, 4 + server_num * 32 + 28) = 0; //maintenance;
                        WFIFOW(s, 4 + server_num * 32 + 30) = 0; //is_new;
                        server_num++;
                    }
                }
                WFIFOW(s, 0) = 0x7939;
                WFIFOW(s, 2) = 4 + 32 * server_num;
                WFIFOSET(s, 4 + 32 * server_num);
                RFIFOSKIP(s, 2);
                break;

            case 0x793a:       // Request to password check
                if (RFIFOREST(s) < 50)
                    return;
            {
                WFIFOW(s, 0) = 0x793b;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                const AuthData *ad = search_account(account_name);
                if (ad)
                {
                    WFIFO_STRING(s, 6, ad->userid, 24);
                    AccountPass pass = stringish<AccountPass>(RFIFO_STRING<24>(s, 26));
                    if (pass_ok(pass, ad->pass))
                    {
                        WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                        LOGIN_LOG("'ladmin': Check of password OK (account: %s, password: %s, ip: %s)\n"_fmt,
                             ad->userid, ad->pass,
                             ip);
                    }
                    else
                    {
                        LOGIN_LOG("'ladmin': Failure of password check (account: %s, proposed pass: %s, ip: %s)\n"_fmt,
                             ad->userid, pass.to_print(), ip);
                    }
                }
                else
                {
                    WFIFO_STRING(s, 6, account_name, 24);
                    LOGIN_LOG("'ladmin': Attempt to check the password of an unknown account (account: %s, ip: %s)\n"_fmt,
                         account_name, ip);
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 50);
                break;

            case 0x793c:       // Request to modify sex
                if (RFIFOREST(s) < 27)
                    return;
            {
                WFIFOW(s, 0) = 0x793d;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                WFIFO_STRING(s, 6, account_name, 24);
                {
                    SEX sex = sex_from_char(RFIFOB(s, 26));
                    if (sex != SEX::FEMALE && sex != SEX::MALE)
                    {
                        LOGIN_LOG("'ladmin': Attempt to give an invalid sex (account: %s, received sex: %c, ip: %s)\n"_fmt,
                                account_name, sex_to_char(sex), ip);
                    }
                    else
                    {
                        AuthData *ad = search_account(account_name);
                        if (ad)
                        {
                            WFIFO_STRING(s, 6, ad->userid, 24);
                            if (ad->sex != sex)
                            {
                                unsigned char buf[16];
                                WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                                for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                    if (auth_fifo[j].account_id ==
                                        ad->account_id)
                                        auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                ad->sex = sex;
                                LOGIN_LOG("'ladmin': Modification of a sex (account: %s, new sex: %c, ip: %s)\n"_fmt,
                                     ad->userid, sex_to_char(sex), ip);
                                // send to all char-server the change
                                WBUFW(buf, 0) = 0x2723;
                                WBUFL(buf, 2) = unwrap<AccountId>(ad->account_id);
                                WBUFB(buf, 6) = static_cast<uint8_t>(ad->sex);
                                charif_sendallwos(nullptr, buf, 7);
                            }
                            else
                            {
                                LOGIN_LOG("'ladmin': Modification of a sex, but the sex is already the good sex (account: %s, sex: %c, ip: %s)\n"_fmt,
                                     ad->userid, sex_to_char(sex), ip);
                            }
                        }
                        else
                        {
                            LOGIN_LOG("'ladmin': Attempt to modify the sex of an unknown account (account: %s, received sex: %c, ip: %s)\n"_fmt,
                                 account_name, sex_to_char(sex), ip);
                        }
                    }
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 27);
                break;

            case 0x793e:       // Request to modify GM level
                if (RFIFOREST(s) < 27)
                    return;
            {
                WFIFOW(s, 0) = 0x793f;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                WFIFO_STRING(s, 6, account_name, 24);
                bool reread = false;
                {
                    GmLevel new_gm_level = GmLevel::from(static_cast<uint32_t>(RFIFOB(s, 26)));
                    {
                        const AuthData *ad = search_account(account_name);
                        if (ad)
                        {
                            AccountId acc = ad->account_id;
                            WFIFO_STRING(s, 6, ad->userid, 24);
                            if (isGM(acc) != new_gm_level)
                            {
                                // modification of the file
                                AccountId GM_account;
                                GmLevel GM_level;
                                int modify_flag;
                                io::WriteLock fp2(gm_account_filename);
                                if (fp2.is_open())
                                {
                                    io::ReadFile fp(gm_account_filename);
                                    if (fp.is_open())
                                    {
                                        timestamp_seconds_buffer tmpstr;
                                        stamp_time(tmpstr);
                                        modify_flag = 0;
                                        // read/write GM file
                                        AString line;
                                        while (fp.getline(line))
                                        {
                                            if (is_comment(line))
                                                fp2.put_line(line);
                                            else
                                            {
                                                if (!extract(line, record<' '>(&GM_account, &GM_level)))
                                                    fp2.put_line(line);
                                                else if (GM_account != acc)
                                                    fp2.put_line(line);
                                                else if (!new_gm_level)
                                                {
                                                    FPRINTF(fp2,
                                                             "// %s: 'ladmin' GM level removed on account %d '%s' (previous level: %d)\n//%d %d\n"_fmt,
                                                             tmpstr,
                                                             acc,
                                                             ad->userid,
                                                             GM_level, acc,
                                                             new_gm_level);
                                                    modify_flag = 1;
                                                }
                                                else
                                                {
                                                    FPRINTF(fp2,
                                                             "// %s: 'ladmin' GM level on account %d '%s' (previous level: %d)\n%d %d\n"_fmt,
                                                             tmpstr,
                                                             acc,
                                                             ad->userid,
                                                             GM_level, acc,
                                                             new_gm_level);
                                                    modify_flag = 1;
                                                }
                                            }
                                        }
                                        if (modify_flag == 0)
                                            FPRINTF(fp2,
                                                     "// %s: 'ladmin' GM level on account %d '%s' (previous level: 0)\n%d %d\n"_fmt,
                                                     tmpstr, acc,
                                                     ad->userid, acc,
                                                     new_gm_level);
                                    }
                                    else
                                    {
                                        LOGIN_LOG("'ladmin': Attempt to modify of a GM level - impossible to read GM accounts file (account: %s (%d), received GM level: %d, ip: %s)\n"_fmt,
                                             ad->userid, acc,
                                             new_gm_level, ip);
                                    }
                                    WFIFOL(s, 2) = unwrap<AccountId>(acc);
                                    LOGIN_LOG("'ladmin': Modification of a GM level (account: %s (%d), new GM level: %d, ip: %s)\n"_fmt,
                                         ad->userid, acc,
                                         new_gm_level, ip);
                                    reread = true;
                                }
                                else
                                {
                                    LOGIN_LOG("'ladmin': Attempt to modify of a GM level - impossible to write GM accounts file (account: %s (%d), received GM level: %d, ip: %s)\n"_fmt,
                                         ad->userid, acc,
                                         new_gm_level, ip);
                                }
                            }
                            else
                            {
                                LOGIN_LOG("'ladmin': Attempt to modify of a GM level, but the GM level is already the good GM level (account: %s (%d), GM level: %d, ip: %s)\n"_fmt,
                                     ad->userid, acc,
                                     new_gm_level, ip);
                            }
                        }
                        else
                        {
                            LOGIN_LOG("'ladmin': Attempt to modify the GM level of an unknown account (account: %s, received GM level: %d, ip: %s)\n"_fmt,
                                 account_name, new_gm_level,
                                 ip);
                        }
                    }
                }
                if (reread)
                {
                    // read and send new GM informations
                    read_gm_account();
                    send_GM_accounts();
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 27);
                break;

            case 0x7940:       // Request to modify e-mail
                if (RFIFOREST(s) < 66)
                    return;
            {
                WFIFOW(s, 0) = 0x7941;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                WFIFO_STRING(s, 6, account_name, 24);
                {
                    AccountEmail email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 26));
                    if (!e_mail_check(email))
                    {
                        LOGIN_LOG("'ladmin': Attempt to give an invalid e-mail (account: %s, ip: %s)\n"_fmt,
                             account_name, ip);
                    }
                    else
                    {
                        AuthData *ad = search_account(account_name);
                        if (ad)
                        {
                            WFIFO_STRING(s, 6, ad->userid, 24);
                            ad->email = email;
                            WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                            LOGIN_LOG("'ladmin': Modification of an email (account: %s, new e-mail: %s, ip: %s)\n"_fmt,
                                 ad->userid, email, ip);
                        }
                        else
                        {
                            LOGIN_LOG("'ladmin': Attempt to modify the e-mail of an unknown account (account: %s, received e-mail: %s, ip: %s)\n"_fmt,
                                 account_name, email, ip);
                        }
                    }
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 66);
                break;

            case 0x7942:       // Request to modify memo field
                if (RFIFOREST(s) < 28
                    || RFIFOREST(s) < (28 + RFIFOW(s, 26)))
                    return;
            {
                WFIFOW(s, 0) = 0x7943;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                AuthData *ad = search_account(account_name);
                if (ad)
                {
                    WFIFO_STRING(s, 6, ad->userid, 24);
                    ad->memo = ""_s;
                    if (RFIFOW(s, 26) == 0)
                    {
                        ad->memo = "!"_s;
                    }
                    else
                    {
                        size_t len = RFIFOW(s, 26);
                        // may truncate
                        ad->memo = RFIFO_STRING(s, 28, len);
                    }
                    ad->memo = ad->memo.to_print();
                    WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                    LOGIN_LOG("'ladmin': Modification of a memo field (account: %s, new memo: %s, ip: %s)\n"_fmt,
                         ad->userid, ad->memo, ip);
                }
                else
                {
                    WFIFO_STRING(s, 6, account_name, 24);
                    LOGIN_LOG("'ladmin': Attempt to modify the memo field of an unknown account (account: %s, ip: %s)\n"_fmt,
                         account_name, ip);
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 28 + RFIFOW(s, 26));
                break;

            case 0x7944:       // Request to found an account id
                if (RFIFOREST(s) < 26)
                    return;
            {
                WFIFOW(s, 0) = 0x7945;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                const AuthData *ad = search_account(account_name);
                if (ad)
                {
                    WFIFO_STRING(s, 6, ad->userid, 24);
                    WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                    LOGIN_LOG("'ladmin': Request (by the name) of an account id (account: %s, id: %d, ip: %s)\n"_fmt,
                            ad->userid, ad->account_id,
                            ip);
                }
                else
                {
                    WFIFO_STRING(s, 6, account_name, 24);
                    LOGIN_LOG("'ladmin': ID request (by the name) of an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                }
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 26);
                break;

            case 0x7946:       // Request to found an account name
                if (RFIFOREST(s) < 6)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                WFIFOW(s, 0) = 0x7947;
                WFIFOL(s, 2) = unwrap<AccountId>(account_id);
                WFIFO_ZERO(s, 6, 24);
                for (const AuthData& ad : auth_data)
                {
                    if (ad.account_id == account_id)
                    {
                        WFIFO_STRING(s, 6, ad.userid, 24);
                        LOGIN_LOG("'ladmin': Request (by id) of an account name (account: %s, id: %d, ip: %s)\n"_fmt,
                                ad.userid, account_id, ip);
                        goto x7946_out;
                    }
                }
                LOGIN_LOG("'ladmin': Name request (by id) of an unknown account (id: %d, ip: %s)\n"_fmt,
                        account_id, ip);
                WFIFO_STRING(s, 6, ""_s, 24);
            x7946_out:
                WFIFOSET(s, 30);
            }
                RFIFOSKIP(s, 6);
                break;

            case 0x7948:       // Request to change the validity limit (timestamp) (absolute value)
                if (RFIFOREST(s) < 30)
                    return;
                {
                    WFIFOW(s, 0) = 0x7949;
                    WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                    AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 26));
                    timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("unlimited"_s);
                    if (timestamp)
                        stamp_time(tmpstr, &timestamp);
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        WFIFO_STRING(s, 6, ad->userid, 24);
                        LOGIN_LOG("'ladmin': Change of a validity limit (account: %s, new validity: %lld (%s), ip: %s)\n"_fmt,
                                ad->userid,
                                timestamp,
                                tmpstr,
                                ip);
                        ad->connect_until_time = timestamp;
                        WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                    }
                    else
                    {
                        WFIFO_STRING(s, 6, account_name, 24);
                        LOGIN_LOG("'ladmin': Attempt to change the validity limit of an unknown account (account: %s, received validity: %lld (%s), ip: %s)\n"_fmt,
                                account_name,
                                timestamp,
                                tmpstr,
                                ip);
                    }
                    WFIFOL(s, 30) = static_cast<time_t>(timestamp);
                }
                WFIFOSET(s, 34);
                RFIFOSKIP(s, 30);
                break;

            case 0x794a:       // Request to change the final date of a banishment (timestamp) (absolute value)
                if (RFIFOREST(s) < 30)
                    return;
                {
                    WFIFOW(s, 0) = 0x794b;
                    WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                    AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 26));
                    if (timestamp <= TimeT::now())
                        timestamp = TimeT();
                    timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("no banishment"_s);
                    if (timestamp)
                        stamp_time(tmpstr, &timestamp);
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        WFIFO_STRING(s, 6, ad->userid, 24);
                        WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                        LOGIN_LOG("'ladmin': Change of the final date of a banishment (account: %s, new final date of banishment: %lld (%s), ip: %s)\n"_fmt,
                                ad->userid, timestamp,
                                tmpstr,
                                ip);
                        if (ad->ban_until_time != timestamp)
                        {
                            if (timestamp)
                            {
                                unsigned char buf[16];
                                WBUFW(buf, 0) = 0x2731;
                                WBUFL(buf, 2) = unwrap<AccountId>(ad->account_id);
                                WBUFB(buf, 6) = 1; // 0: change of statut, 1: ban
                                WBUFL(buf, 7) = static_cast<time_t>(timestamp); // status or final date of a banishment
                                charif_sendallwos(nullptr, buf, 11);
                                for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                    if (auth_fifo[j].account_id ==
                                        ad->account_id)
                                        auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                            }
                            ad->ban_until_time = timestamp;
                        }
                    }
                    else
                    {
                        WFIFO_STRING(s, 6, account_name, 24);
                        LOGIN_LOG("'ladmin': Attempt to change the final date of a banishment of an unknown account (account: %s, received final date of banishment: %lld (%s), ip: %s)\n"_fmt,
                                account_name, timestamp,
                                tmpstr,
                                ip);
                    }
                    WFIFOL(s, 30) = static_cast<time_t>(timestamp);
                }
                WFIFOSET(s, 34);
                RFIFOSKIP(s, 30);
                break;

            case 0x794c:       // Request to change the final date of a banishment (timestamp) (relative change)
                if (RFIFOREST(s) < 38)
                    return;
                {
                    WFIFOW(s, 0) = 0x794d;
                    WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                    AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                        WFIFO_STRING(s, 6, ad->userid, 24);
                        TimeT timestamp;
                        TimeT now = TimeT::now();
                        if (!ad->ban_until_time
                            || ad->ban_until_time < now)
                            timestamp = now;
                        else
                            timestamp = ad->ban_until_time;
                        struct tm tmtime = timestamp;
                        HumanTimeDiff ban_diff;
                        RFIFO_STRUCT(s, 26, ban_diff);
                        tmtime.tm_year += ban_diff.year;
                        tmtime.tm_mon += ban_diff.month;
                        tmtime.tm_mday += ban_diff.day;
                        tmtime.tm_hour += ban_diff.hour;
                        tmtime.tm_min += ban_diff.minute;
                        tmtime.tm_sec += ban_diff.second;
                        timestamp = tmtime;
                        if (timestamp.okay())
                        {
                            if (timestamp <= now)
                                timestamp = TimeT();
                            timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("no banishment"_s);
                            if (timestamp)
                                stamp_time(tmpstr, &timestamp);
                            LOGIN_LOG("'ladmin': Adjustment of a final date of a banishment (account: %s, (%+d y %+d m %+d d %+d h %+d mn %+d s) -> new validity: %lld (%s), ip: %s)\n"_fmt,
                                    ad->userid,
                                    ban_diff.year, ban_diff.month,
                                    ban_diff.day, ban_diff.hour,
                                    ban_diff.minute, ban_diff.second,
                                    timestamp,
                                    tmpstr,
                                    ip);
                            if (ad->ban_until_time != timestamp)
                            {
                                if (timestamp)
                                {
                                    unsigned char buf[16];
                                    WBUFW(buf, 0) = 0x2731;
                                    WBUFL(buf, 2) = unwrap<AccountId>(ad->account_id);
                                    WBUFB(buf, 6) = 1; // 0: change of statut, 1: ban
                                    WBUFL(buf, 7) = static_cast<time_t>(timestamp); // status or final date of a banishment
                                    charif_sendallwos(nullptr, buf, 11);
                                    for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                        if (auth_fifo[j].account_id ==
                                            ad->account_id)
                                            auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                }
                                ad->ban_until_time = timestamp;
                            }
                        }
                        else
                        {
                            timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("no banishment"_s);
                            if (ad->ban_until_time)
                                stamp_time(tmpstr, &ad->ban_until_time);
                            LOGIN_LOG("'ladmin': Impossible to adjust the final date of a banishment (account: %s, %lld (%s) + (%+d y %+d m %+d d %+d h %+d mn %+d s) -> ???, ip: %s)\n"_fmt,
                                    ad->userid,
                                    ad->ban_until_time,
                                    tmpstr,
                                    ban_diff.year, ban_diff.month,
                                    ban_diff.day, ban_diff.hour,
                                    ban_diff.minute, ban_diff.second,
                                    ip);
                        }
                        WFIFOL(s, 30) = static_cast<time_t>(ad->ban_until_time);
                    }
                    else
                    {
                        WFIFO_STRING(s, 6, account_name, 24);
                        LOGIN_LOG("'ladmin': Attempt to adjust the final date of a banishment of an unknown account (account: %s, ip: %s)\n"_fmt,
                                account_name, ip);
                        WFIFOL(s, 30) = 0;
                    }
                }
                WFIFOSET(s, 34);
                RFIFOSKIP(s, 38);
                break;

            case 0x794e:       // Request to send a broadcast message
                if (RFIFOREST(s) < 8
                    || RFIFOREST(s) < (8 + RFIFOL(s, 4)))
                    return;
                WFIFOW(s, 0) = 0x794f;
                WFIFOW(s, 2) = -1;
                if (RFIFOL(s, 4) < 1)
                {
                    LOGIN_LOG("'ladmin': Receiving a message for broadcast, but message is void (ip: %s)\n"_fmt,
                         ip);
                }
                else
                {
                    // at least 1 char-server
                    for (int i = 0; i < MAX_SERVERS; i++)
                        if (server_session[i])
                            goto x794e_have_server;
                    LOGIN_LOG("'ladmin': Receiving a message for broadcast, but no char-server is online (ip: %s)\n"_fmt,
                            ip);
                    goto x794e_have_no_server;
                    {
                    x794e_have_server:
                        // overwrite the -1
                        WFIFOW(s, 2) = 0;

                        size_t len = RFIFOL(s, 4);
                        AString message = RFIFO_STRING(s, 8, len).to_print();
                        LOGIN_LOG("'ladmin': Receiving a message for broadcast (message: %s, ip: %s)\n"_fmt,
                                message, ip);
                        // send same message to all char-servers (no answer)
                        uint8_t buf[len + 8];
                        RFIFO_BUF_CLONE(s, buf, 8 + len);
                        WBUFW(buf, 0) = 0x2726;
                        charif_sendallwos(nullptr, buf, 8 + len);
                    }
                }
            x794e_have_no_server:
                WFIFOSET(s, 4);
                RFIFOSKIP(s, 8 + RFIFOL(s, 4));
                break;

            case 0x7950:       // Request to change the validity limite (timestamp) (relative change)
                if (RFIFOREST(s) < 38)
                    return;
                {
                    WFIFOW(s, 0) = 0x7951;
                    WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                    AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                        WFIFO_STRING(s, 6, ad->userid, 24);
                        if (add_to_unlimited_account == 0 && !ad->connect_until_time)
                        {
                            LOGIN_LOG("'ladmin': Attempt to adjust the validity limit of an unlimited account (account: %s, ip: %s)\n"_fmt,
                                 ad->userid, ip);
                            WFIFOL(s, 30) = 0;
                        }
                        else
                        {
                            TimeT now = TimeT::now();
                            TimeT timestamp = ad->connect_until_time;
                            if (!timestamp || timestamp < now)
                                timestamp = now;
                            struct tm tmtime = timestamp;
                            HumanTimeDiff v_diff;
                            RFIFO_STRUCT(s, 26, v_diff);
                            tmtime.tm_year += v_diff.year;
                            tmtime.tm_mon += v_diff.month;
                            tmtime.tm_mday += v_diff.day;
                            tmtime.tm_hour += v_diff.hour;
                            tmtime.tm_min += v_diff.minute;
                            tmtime.tm_sec += v_diff.second;
                            timestamp = tmtime;
                            if (timestamp.okay())
                            {
                                timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("unlimited"_s);
                                timestamp_seconds_buffer tmpstr2 = stringish<timestamp_seconds_buffer>("unlimited"_s);
                                if (ad->connect_until_time)
                                    stamp_time(tmpstr, &ad->connect_until_time);
                                if (timestamp)
                                    stamp_time(tmpstr2, &timestamp);
                                LOGIN_LOG("'ladmin': Adjustment of a validity limit (account: %s, %lld (%s) + (%+d y %+d m %+d d %+d h %+d mn %+d s) -> new validity: %lld (%s), ip: %s)\n"_fmt,
                                        ad->userid,
                                        ad->connect_until_time,
                                        tmpstr,
                                        v_diff.year,
                                        v_diff.month,
                                        v_diff.day,
                                        v_diff.hour,
                                        v_diff.minute,
                                        v_diff.second,
                                        timestamp,
                                        tmpstr2,
                                        ip);
                                ad->connect_until_time = timestamp;
                                WFIFOL(s, 30) = static_cast<time_t>(timestamp);
                            }
                            else
                            {
                                timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("unlimited"_s);
                                if (ad->connect_until_time)
                                    stamp_time(tmpstr, &ad->connect_until_time);
                                LOGIN_LOG("'ladmin': Impossible to adjust a validity limit (account: %s, %lld (%s) + (%+d y %+d m %+d d %+d h %+d mn %+d s) -> ???, ip: %s)\n"_fmt,
                                        ad->userid,
                                        ad->connect_until_time,
                                        tmpstr,
                                        v_diff.year,
                                        v_diff.month,
                                        v_diff.day,
                                        v_diff.hour,
                                        v_diff.minute,
                                        v_diff.second,
                                        ip);
                                WFIFOL(s, 30) = 0;
                            }
                        }
                    }
                    else
                    {
                        WFIFO_STRING(s, 6, account_name, 24);
                        LOGIN_LOG("'ladmin': Attempt to adjust the validity limit of an unknown account (account: %s, ip: %s)\n"_fmt,
                             account_name, ip);
                        WFIFOL(s, 30) = 0;
                    }
                }
                WFIFOSET(s, 34);
                RFIFOSKIP(s, 38);
                break;

            case 0x7952:       // Request about informations of an account (by account name)
                if (RFIFOREST(s) < 26)
                    return;
            {
                WFIFOW(s, 0) = 0x7953;
                WFIFOL(s, 2) = unwrap<AccountId>(AccountId());
                AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                const AuthData *ad = search_account(account_name);
                if (ad)
                {
                    WFIFOL(s, 2) = unwrap<AccountId>(ad->account_id);
                    // TODO fix size (there's a lot of other stuff wrong with this packet too)
                    WFIFOB(s, 6) = static_cast<uint8_t>(isGM(ad->account_id).get_all_bits());
                    WFIFO_STRING(s, 7, ad->userid, 24);
                    WFIFOB(s, 31) = static_cast<uint8_t>(ad->sex);
                    WFIFOL(s, 32) = ad->logincount;
                    WFIFOL(s, 36) = ad->state;
                    WFIFO_STRING(s, 40, ad->error_message, 20);
                    WFIFO_STRING(s, 60, ad->lastlogin, 24);
                    WFIFO_STRING(s, 84, convert_for_printf(ad->last_ip), 16);
                    WFIFO_STRING(s, 100, ad->email, 40);
                    WFIFOL(s, 140) = static_cast<time_t>(ad->connect_until_time);
                    WFIFOL(s, 144) = static_cast<time_t>(ad->ban_until_time);
                    size_t len = ad->memo.size() + 1;
                    WFIFOW(s, 148) = len;
                    WFIFO_STRING(s, 150, ad->memo, len);
                    LOGIN_LOG("'ladmin': Sending information of an account (request by the name; account: %s, id: %d, ip: %s)\n"_fmt,
                         ad->userid, ad->account_id,
                         ip);
                    WFIFOSET(s, 150 + len);
                }
                else
                {
                    WFIFO_STRING(s, 7, account_name, 24);
                    WFIFOW(s, 148) = 0;
                    LOGIN_LOG("'ladmin': Attempt to obtain information (by the name) of an unknown account (account: %s, ip: %s)\n"_fmt,
                         account_name, ip);
                    WFIFOSET(s, 150);
                }
            }
                RFIFOSKIP(s, 26);
                break;

            case 0x7954:       // Request about information of an account (by account id)
                if (RFIFOREST(s) < 6)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                WFIFOW(s, 0) = 0x7953;
                WFIFOL(s, 2) = unwrap<AccountId>(account_id);
                WFIFO_ZERO(s, 7, 24);
                for (const AuthData& ad : auth_data)
                {
                    if (ad.account_id == account_id)
                    {
                        LOGIN_LOG("'ladmin': Sending information of an account (request by the id; account: %s, id: %d, ip: %s)\n"_fmt,
                             ad.userid, RFIFOL(s, 2), ip);
                        WFIFOB(s, 6) = static_cast<uint8_t>(isGM(ad.account_id).get_all_bits());
                        WFIFO_STRING(s, 7, ad.userid, 24);
                        WFIFOB(s, 31) = static_cast<uint8_t>(ad.sex);
                        WFIFOL(s, 32) = ad.logincount;
                        WFIFOL(s, 36) = ad.state;
                        WFIFO_STRING(s, 40, ad.error_message, 20);
                        WFIFO_STRING(s, 60, ad.lastlogin, 24);
                        WFIFO_STRING(s, 84, convert_for_printf(ad.last_ip), 16);
                        WFIFO_STRING(s, 100, ad.email, 40);
                        WFIFOL(s, 140) = static_cast<time_t>(ad.connect_until_time);
                        WFIFOL(s, 144) = static_cast<time_t>(ad.ban_until_time);
                        size_t len = ad.memo.size() + 1;
                        WFIFOW(s, 148) = len;
                        WFIFO_STRING(s, 150, ad.memo, len);
                        WFIFOSET(s, 150 + len);
                        goto x7954_out;
                    }
                }
                {
                    LOGIN_LOG("'ladmin': Attempt to obtain information (by the id) of an unknown account (id: %d, ip: %s)\n"_fmt,
                         RFIFOL(s, 2), ip);
                    WFIFO_STRING(s, 7, ""_s, 24);
                    WFIFOW(s, 148) = 0;
                    WFIFOSET(s, 150);
                }
            }
            x7954_out:
                RFIFOSKIP(s, 6);
                break;

            case 0x7955:       // Request to reload GM file (no answer)
                LOGIN_LOG("'ladmin': Request to re-load GM configuration file (ip: %s).\n"_fmt,
                     ip);
                read_gm_account();
                // send GM accounts to all char-servers
                send_GM_accounts();
                RFIFOSKIP(s, 2);
                break;

            default:
            {
                io::AppendFile logfp(login_log_unknown_packets_filename);
                if (logfp.is_open())
                {
                    timestamp_milliseconds_buffer timestr;
                    stamp_time(timestr);
                    FPRINTF(logfp,
                             "%s: receiving of an unknown packet -> disconnection\n"_fmt,
                             timestr);
                    FPRINTF(logfp,
                             "parse_admin: connection #%d (ip: %s), packet: 0x%x (with being read: %zu).\n"_fmt,
                             s, ip, RFIFOW(s, 0), RFIFOREST(s));
                    FPRINTF(logfp, "Detail (in hex):\n"_fmt);
                    FPRINTF(logfp,
                             "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F\n"_fmt);
                    char tmpstr[16 + 1] {};
                    int i;
                    for (i = 0; i < RFIFOREST(s); i++)
                    {
                        if ((i & 15) == 0)
                            FPRINTF(logfp, "%04X "_fmt, i);
                        FPRINTF(logfp, "%02x "_fmt, RFIFOB (s, i));
                        if (RFIFOB(s, i) > 0x1f)
                            tmpstr[i % 16] = RFIFOB(s, i);
                        else
                            tmpstr[i % 16] = '.';
                        if ((i - 7) % 16 == 0)  // -8 + 1
                            FPRINTF(logfp, " "_fmt);
                        else if ((i + 1) % 16 == 0)
                        {
                            FPRINTF(logfp, " %s\n"_fmt, tmpstr);
                            std::fill(tmpstr + 0, tmpstr + 17, '\0');
                        }
                    }
                    if (i % 16 != 0)
                    {
                        for (int j = i; j % 16 != 0; j++)
                        {
                            FPRINTF(logfp, "   "_fmt);
                            if ((j - 7) % 16 == 0)  // -8 + 1
                                FPRINTF(logfp, " "_fmt);
                        }
                        FPRINTF(logfp, " %s\n"_fmt, tmpstr);
                    }
                    FPRINTF(logfp, "\n"_fmt);
                }
            }
                LOGIN_LOG("'ladmin': End of connection, unknown packet (ip: %s)\n"_fmt,
                     ip);
                s->set_eof();
                PRINTF("Remote administration has been disconnected (unknown packet).\n"_fmt);
                return;
        }
        //WFIFOW(fd,0) = 0x791f;
        //WFIFOSET(fd,2);
    }
    return;
}

//--------------------------------------------
// Test to know if an IP come from LAN or WAN.
//--------------------------------------------
static
bool lan_ip_check(IP4Address p)
{
    bool lancheck = lan_subnet.covers(p);

    PRINTF("LAN test (result): %s.\n"_fmt,
            (lancheck) ? SGR_BOLD SGR_CYAN "LAN source" SGR_RESET ""_s : SGR_BOLD SGR_GREEN "WAN source" SGR_RESET ""_s);
    return lancheck;
}

//----------------------------------------------------------------------------------------
// Default packet parsing (normal players or administation/char-server connexion requests)
//----------------------------------------------------------------------------------------
static
void parse_login(Session *s)
{
    struct mmo_account account;
    int result, j;

    IP4Address ip = s->client_ip;

    while (RFIFOREST(s) >= 2)
    {
        if (display_parse_login == 1)
        {
            if (RFIFOW(s, 0) == 0x64 || RFIFOW(s, 0) == 0x01dd)
            {
                if (RFIFOREST(s) >= ((RFIFOW(s, 0) == 0x64) ? 55 : 47))
                {
                    AccountName account_name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                    PRINTF("parse_login: connection #%d, packet: 0x%x (with being read: %zu), account: %s.\n"_fmt,
                         s, RFIFOW(s, 0), RFIFOREST(s),
                         account_name);
                }
            }
            else if (RFIFOW(s, 0) == 0x2710)
            {
                if (RFIFOREST(s) >= 86)
                {
                    ServerName server_name = stringish<ServerName>(RFIFO_STRING<20>(s, 60));
                    PRINTF("parse_login: connection #%d, packet: 0x%x (with being read: %zu), server: %s.\n"_fmt,
                         s, RFIFOW(s, 0), RFIFOREST(s),
                         server_name);
                }
            }
            else
                PRINTF("parse_login: connection #%d, packet: 0x%x (with being read: %zu).\n"_fmt,
                     s, RFIFOW(s, 0), RFIFOREST(s));
        }

        switch (RFIFOW(s, 0))
        {
            case 0x200:        // New alive packet: structure: 0x200 <account.userid>.24B. used to verify if client is always alive.
                if (RFIFOREST(s) < 26)
                    return;
                RFIFOSKIP(s, 26);
                break;

            case 0x204:        // New alive packet: structure: 0x204 <encrypted.account.userid>.16B. (new ragexe from 22 june 2004)
                if (RFIFOREST(s) < 18)
                    return;
                RFIFOSKIP(s, 18);
                break;

            case 0x64:         // Ask connection of a client
                if (RFIFOREST(s) < 55)
                    return;

                account.userid = stringish<AccountName>(RFIFO_STRING<24>(s, 6).to_print());
                account.passwd = stringish<AccountPass>(RFIFO_STRING<24>(s, 30).to_print());
                account.passwdenc = 0;

                LOGIN_LOG("Request for connection (non encryption mode) of %s (ip: %s).\n"_fmt,
                    account.userid, ip);

                if (!check_ip(ip))
                {
                    LOGIN_LOG("Connection refused: IP isn't authorised (deny/allow, ip: %s).\n"_fmt,
                         ip);
                    WFIFOW(s, 0) = 0x6a;
                    WFIFOB(s, 2) = 0x03;
                    WFIFO_ZERO(s, 3, 20);
                    WFIFOSET(s, 23);
                    RFIFOSKIP(s, 55);
                    break;
                }

                result = mmo_auth(&account, s);
                if (result == -1)
                {
                    VERSION_2 version_2 = static_cast<VERSION_2>(RFIFOB(s, 54));
                    if (!bool(version_2 & VERSION_2::UPDATEHOST)
                        || !bool(version_2 & VERSION_2::SERVERORDER))
                        result = 5; // client too old
                }
                if (result == -1)
                {
                    GmLevel gm_level = isGM(account.account_id);
                    if (!(gm_level.satisfies(min_level_to_connect)))
                    {
                        LOGIN_LOG("Connection refused: the minimum GM level for connection is %d (account: %s, GM level: %d, ip: %s).\n"_fmt,
                             min_level_to_connect, account.userid,
                             gm_level, ip);
                        WFIFOW(s, 0) = 0x81;
                        WFIFOB(s, 2) = 1; // 01 = Server closed
                        WFIFOSET(s, 3);
                    }
                    else
                    {
                        // int version_2 = RFIFOB(fd, 54);   // version 2

                        if (gm_level)
                            PRINTF("Connection of the GM (level:%d) account '%s' accepted.\n"_fmt,
                                 gm_level, account.userid);
                        else
                            PRINTF("Connection of the account '%s' accepted.\n"_fmt,
                                 account.userid);

                        /*
                         * Add a 0x0063 packet, which contains the name of the update host.  The packet will only
                         * be sent if login_athena.conf contains a non-null entry for "update_host:"
                         *
                         * Because older clients cannot handle the 0x63 packet, we check the "version 2" value
                         * from the incoming 0x64 packet (the byte at offset 54).  If bit 0 of this is set,
                         * then the client can safely accept the 0x63 packet.  The "version 2" value is not
                         * otherwise used by eAthena.
                         *
                         * All supported clients now send both, so the check is removed.
                         */
                        // if (version_2 & VERSION_2_UPDATEHOST)
                        {
                            if (update_host)
                            {
                                size_t host_len = update_host.size() + 1;
                                WFIFOW(s, 0) = 0x63;
                                WFIFOW(s, 2) = 4 + host_len;
                                WFIFO_STRING(s, 4, update_host, host_len);
                                WFIFOSET(s, 4 + host_len);
                            }
                        }

                        // Load list of char servers into outbound packet
                        server_num = 0;
                        // if (version_2 & VERSION_2_SERVERORDER)
                        for (int i = 0; i < MAX_SERVERS; i++)
                        {
                            if (server_session[i])
                            {
                                if (lan_ip_check(ip))
                                    WFIFOIP(s, 47 + server_num * 32) = lan_char_ip;
                                else
                                    WFIFOIP(s, 47 + server_num * 32) = server[i].ip;
                                WFIFOW(s, 47 + server_num * 32 + 4) = server[i].port;
                                WFIFO_STRING(s, 47 + server_num * 32 + 6, server[i].name, 20);
                                WFIFOW(s, 47 + server_num * 32 + 26) = server[i].users;
                                WFIFOW(s, 47 + server_num * 32 + 28) = 0; //maintenance;
                                WFIFOW(s, 47 + server_num * 32 + 30) = 0; //is_new;
                                server_num++;
                            }
                        }
                        // if at least 1 char-server
                        if (server_num > 0)
                        {
                            WFIFOW(s, 0) = 0x69;
                            WFIFOW(s, 2) = 47 + 32 * server_num;
                            WFIFOL(s, 4) = account.login_id1;
                            WFIFOL(s, 8) = unwrap<AccountId>(account.account_id);
                            WFIFOL(s, 12) = account.login_id2;
                            WFIFOL(s, 16) = 0;    // in old version, that was for ip (not more used)
                            WFIFO_STRING(s, 20, account.lastlogin, 24);    // in old version, that was for name (not more used)
                            WFIFOB(s, 46) = static_cast<uint8_t>(account.sex);
                            WFIFOSET(s, 47 + 32 * server_num);
                            if (auth_fifo_pos >= AUTH_FIFO_SIZE)
                                auth_fifo_pos = 0;
                            auth_fifo[auth_fifo_pos].account_id =
                                account.account_id;
                            auth_fifo[auth_fifo_pos].login_id1 =
                                account.login_id1;
                            auth_fifo[auth_fifo_pos].login_id2 =
                                account.login_id2;
                            auth_fifo[auth_fifo_pos].sex = account.sex;
                            auth_fifo[auth_fifo_pos].delflag = 0;
                            auth_fifo[auth_fifo_pos].ip =
                                s->client_ip;
                            auth_fifo_pos++;
                            // if no char-server, don't send void list of servers, just disconnect the player with proper message
                        }
                        else
                        {
                            LOGIN_LOG("Connection refused: there is no char-server online (account: %s, ip: %s).\n"_fmt,
                                 account.userid, ip);
                            WFIFOW(s, 0) = 0x81;
                            WFIFOB(s, 2) = 1; // 01 = Server closed
                            WFIFOSET(s, 3);
                        }
                    }
                }
                else
                {
                    WFIFO_ZERO(s, 0, 23);
                    WFIFOW(s, 0) = 0x6a;
                    WFIFOB(s, 2) = result;
                    if (result == 6)
                    {
                        // 6 = Your are Prohibited to log in until %s
                        const AuthData *ad = search_account(account.userid);
                        if (ad)
                        {
                            if (ad->ban_until_time)
                            {
                                // if account is banned, we send ban timestamp
                                timestamp_seconds_buffer tmpstr;
                                stamp_time(tmpstr, &ad->ban_until_time);
                                WFIFO_STRING(s, 3, tmpstr, 20);
                            }
                            else
                            {   // we send error message
                                WFIFO_STRING(s, 3, ad->error_message, 20);
                            }
                        }
                    }
                    WFIFOSET(s, 23);
                }
                RFIFOSKIP(s, (RFIFOW(s, 0) == 0x64) ? 55 : 47);
                break;

            case 0x2710:       // Connection request of a char-server
                if (RFIFOREST(s) < 86)
                    return;
                {
                    // TODO: this is exceptionally silly. Fix it.
                    int len;
                    account.userid = stringish<AccountName>(RFIFO_STRING<24>(s, 2).to_print());
                    account.passwd = stringish<AccountPass>(RFIFO_STRING<24>(s, 26).to_print());
                    account.passwdenc = 0;
                    ServerName server_name = stringish<ServerName>(RFIFO_STRING<20>(s, 60).to_print());
                    LOGIN_LOG("Connection request of the char-server '%s' @ %s:%d (ip: %s)\n"_fmt,
                            server_name, RFIFOIP(s, 54), RFIFOW(s, 58), ip);
                    if (account.userid == userid && account.passwd == passwd)
                    {
                        // If this is the main server, and we don't already have a main server
                        if (!server_session[0]
                            && server_name == main_server)
                        {
                            account.account_id = wrap<AccountId>(0_u32);
                            goto x2710_okay;
                        }
                        else
                        {
                            int i;
                            for (i = 1; i < MAX_SERVERS; i++)
                            {
                                if (!server_session[i])
                                {
                                    account.account_id = wrap<AccountId>(i);
                                    goto x2710_okay;
                                }
                            }
                        }
                    }
                    goto x2710_refused;

                    {
                    x2710_okay:
                        LOGIN_LOG("Connection of the char-server '%s' accepted (account: %s, pass: %s, ip: %s)\n"_fmt,
                             server_name, account.userid,
                             account.passwd, ip);
                        PRINTF("Connection of the char-server '%s' accepted.\n"_fmt,
                             server_name);
                        server[unwrap<AccountId>(account.account_id)] = mmo_char_server{};
                        server[unwrap<AccountId>(account.account_id)].ip = RFIFOIP(s, 54);
                        server[unwrap<AccountId>(account.account_id)].port = RFIFOW(s, 58);
                        server[unwrap<AccountId>(account.account_id)].name = server_name;
                        server[unwrap<AccountId>(account.account_id)].users = 0;
                        //maintenance = RFIFOW(fd, 82);
                        //is_new = RFIFOW(fd, 84);
                        server_session[unwrap<AccountId>(account.account_id)] = s;
                        if (anti_freeze_enable)
                            server_freezeflag[unwrap<AccountId>(account.account_id)] = 5;  // Char-server anti-freeze system. Counter. 5 ok, 4...0 freezed
                        WFIFOW(s, 0) = 0x2711;
                        WFIFOB(s, 2) = 0;
                        WFIFOSET(s, 3);
                        s->set_parsers(SessionParsers{.func_parse= parse_fromchar, .func_delete= delete_fromchar});
                        realloc_fifo(s, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);
                        // send GM account to char-server
                        len = 4;
                        WFIFOW(s, 0) = 0x2732;
                        for (const AuthData& ad : auth_data)
                            // send only existing accounts. We can not create a GM account when server is online.
                            if (GmLevel GM_value = isGM(ad.account_id))
                            {
                                WFIFOL(s, len) = unwrap<AccountId>(ad.account_id);
                                WFIFOB(s, len + 4) = static_cast<uint8_t>(GM_value.get_all_bits());
                                len += 5;
                            }
                        WFIFOW(s, 2) = len;
                        WFIFOSET(s, len);
                        goto x2710_done;
                    }
                    {
                    x2710_refused:
                        LOGIN_LOG("Connexion of the char-server '%s' REFUSED (account: %s, pass: %s, ip: %s)\n"_fmt,
                             server_name, account.userid,
                             account.passwd, ip);
                        WFIFOW(s, 0) = 0x2711;
                        WFIFOB(s, 2) = 3;
                        WFIFOSET(s, 3);
                    }
                }
            x2710_done:
                RFIFOSKIP(s, 86);
                return;

            case 0x7530:       // Request of the server version
                LOGIN_LOG("Sending of the server version (ip: %s)\n"_fmt,
                           ip);
                WFIFOW(s, 0) = 0x7531;
            {
                Version version = CURRENT_LOGIN_SERVER_VERSION;
                version.flags = new_account ? 1 : 0;
                WFIFO_STRUCT(s, 2, version);
                WFIFOSET(s, 10);
            }
                RFIFOSKIP(s, 2);
                break;

            case 0x7532:       // Request to end connection
                LOGIN_LOG("End of connection (ip: %s)\n"_fmt, ip);
                s->set_eof();
                return;

            case 0x7918:       // Request for administation login
                if (RFIFOREST(s) < 4
                    || RFIFOREST(s) < ((RFIFOW(s, 2) == 0) ? 28 : 20))
                    return;
                WFIFOW(s, 0) = 0x7919;
                WFIFOB(s, 2) = 1;
                if (!check_ladminip(s->client_ip))
                {
                    LOGIN_LOG("'ladmin'-login: Connection in administration mode refused: IP isn't authorised (ladmin_allow, ip: %s).\n"_fmt,
                         ip);
                }
                else
                {
                    if (RFIFOW(s, 2) == 0)
                    {
                        // non encrypted password
                        AccountPass password = stringish<AccountPass>(RFIFO_STRING<24>(s, 4).to_print());
                        // If remote administration is enabled and password sent by client matches password read from login server configuration file
                        if ((admin_state == 1)
                            && (password == admin_pass))
                        {
                            LOGIN_LOG("'ladmin'-login: Connection in administration mode accepted (non encrypted password: %s, ip: %s)\n"_fmt,
                                 password, ip);
                            PRINTF("Connection of a remote administration accepted (non encrypted password).\n"_fmt);
                            WFIFOB(s, 2) = 0;
                            s->set_parsers(SessionParsers{.func_parse= parse_admin, .func_delete= delete_admin});
                        }
                        else if (admin_state != 1)
                            LOGIN_LOG("'ladmin'-login: Connection in administration mode REFUSED - remote administration is disabled (non encrypted password: %s, ip: %s)\n"_fmt,
                                 password, ip);
                        else
                            LOGIN_LOG("'ladmin'-login: Connection in administration mode REFUSED - invalid password (non encrypted password: %s, ip: %s)\n"_fmt,
                                 password, ip);
                    }
                    else
                    {
                        // encrypted password
                        {
                            LOGIN_LOG("'ladmin'-login: Connection in administration mode REFUSED - encrypted login is disabled (ip: %s)\n"_fmt,
                                     ip);
                        }
                    }
                }
                WFIFOSET(s, 3);
                RFIFOSKIP(s, (RFIFOW(s, 2) == 0) ? 28 : 20);
                break;

            default:
                if (save_unknown_packets)
                {
                    io::AppendFile logfp(login_log_unknown_packets_filename);
                    if (logfp.is_open())
                    {
                        timestamp_milliseconds_buffer timestr;
                        stamp_time(timestr);
                        FPRINTF(logfp,
                                 "%s: receiving of an unknown packet -> disconnection\n"_fmt,
                                 timestr);
                        FPRINTF(logfp,
                                 "parse_login: connection #%d (ip: %s), packet: 0x%x (with being read: %zu).\n"_fmt,
                                 s, ip, RFIFOW(s, 0),
                                 RFIFOREST(s));
                        FPRINTF(logfp, "Detail (in hex):\n"_fmt);
                        FPRINTF(logfp,
                                 "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F\n"_fmt);

                        char tmpstr[16 + 1] {};

                        int i;
                        for (i = 0; i < RFIFOREST(s); i++)
                        {
                            if ((i & 15) == 0)
                                FPRINTF(logfp, "%04X "_fmt, i);
                            FPRINTF(logfp, "%02x "_fmt, RFIFOB(s, i));
                            if (RFIFOB(s, i) > 0x1f)
                                tmpstr[i % 16] = RFIFOB(s, i);
                            else
                                tmpstr[i % 16] = '.';
                            if ((i - 7) % 16 == 0)  // -8 + 1
                                FPRINTF(logfp, " "_fmt);
                            else if ((i + 1) % 16 == 0)
                            {
                                FPRINTF(logfp, " %s\n"_fmt, tmpstr);
                                std::fill(tmpstr + 0, tmpstr + 17, '\0');
                            }
                        }
                        if (i % 16 != 0)
                        {
                            for (j = i; j % 16 != 0; j++)
                            {
                                FPRINTF(logfp, "   "_fmt);
                                if ((j - 7) % 16 == 0)  // -8 + 1
                                    FPRINTF(logfp, " "_fmt);
                            }
                            FPRINTF(logfp, " %s\n"_fmt, tmpstr);
                        }
                        FPRINTF(logfp, "\n"_fmt);
                    }
                }
                LOGIN_LOG("End of connection, unknown packet (ip: %s)\n"_fmt, ip);
                s->set_eof();
                return;
        }
    }
    return;
}

//----------------------------------
// Reading Lan Support configuration
//----------------------------------
static
bool login_lan_config(XString w1, ZString w2)
{
    struct hostent *h = NULL;

    {
        if (w1 == "lan_char_ip"_s)
        {
            // Read Char-Server Lan IP Address
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                lan_char_ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
            }
            else
            {
                PRINTF("Bad IP value: %s\n"_fmt, w2);
                return false;
            }
            PRINTF("LAN IP of char-server: %s.\n"_fmt, lan_char_ip);
        }
        else if (w1 == "subnet"_s /*backward compatibility*/
                || w1 == "lan_subnet"_s)
        {
            if (!extract(w2, &lan_subnet))
            {
                PRINTF("Bad IP mask: %s\n"_fmt, w2);
                return false;
            }
            PRINTF("Sub-network of the char-server: %s.\n"_fmt,
                    lan_subnet);
        }
        else
        {
            return false;
        }
    }
    return true;
}

static
bool lan_check()
{
    // log the LAN configuration
    LOGIN_LOG("The LAN configuration of the server is set:\n"_fmt);
    LOGIN_LOG("- with LAN IP of char-server: %s.\n"_fmt, lan_char_ip);
    LOGIN_LOG("- with the sub-network of the char-server: %s.\n"_fmt,
            lan_subnet);

    // sub-network check of the char-server
    {
        PRINTF("LAN test of LAN IP of the char-server: "_fmt);
        if (!lan_ip_check(lan_char_ip))
        {
            PRINTF(SGR_BOLD SGR_RED "***ERROR: LAN IP of the char-server doesn't belong to the specified Sub-network"_fmt SGR_RESET "\n");
            LOGIN_LOG("***ERROR: LAN IP of the char-server doesn't belong to the specified Sub-network.\n"_fmt);
            return false;
        }
    }

    return true;
}

//-----------------------------------
// Reading general configuration file
//-----------------------------------
static
bool login_config(XString w1, ZString w2)
{
    {
        if (w1 == "admin_state"_s)
        {
            admin_state = config_switch(w2);
        }
        else if (w1 == "admin_pass"_s)
        {
            admin_pass = stringish<AccountPass>(w2);
        }
        else if (w1 == "ladminallowip"_s)
        {
            if (w2 == "clear"_s)
            {
                access_ladmin.clear();
            }
            else
            {
                // a.b.c.d/0.0.0.0 (canonically, 0.0.0.0/0) covers all
                if (w2 == "all"_s)
                {
                    // reset all previous values
                    access_ladmin.clear();
                    // set to all
                    access_ladmin.push_back(IP4Mask());
                }
                else if (w2
                        && !(access_ladmin.size() == 1
                            && access_ladmin.front().mask() == IP4Address()))
                {
                    // don't add IP if already 'all'
                    IP4Mask n;
                    if (!extract(w2, &n))
                    {
                        PRINTF("Bad IP mask: %s\n"_fmt, w2);
                        return false;
                    }
                    access_ladmin.push_back(n);
                }
            }
        }
        else if (w1 == "gm_pass"_s)
        {
            gm_pass = w2;
        }
        else if (w1 == "level_new_gm"_s)
        {
            level_new_gm = atoi(w2.c_str());
        }
        else if (w1 == "new_account"_s)
        {
            new_account = config_switch(w2);
        }
        else if (w1 == "login_port"_s)
        {
            login_port = atoi(w2.c_str());
        }
        else if (w1 == "account_filename"_s)
        {
            account_filename = w2;
        }
        else if (w1 == "gm_account_filename"_s)
        {
            gm_account_filename = w2;
        }
        else if (w1 == "gm_account_filename_check_timer"_s)
        {
            gm_account_filename_check_timer = std::chrono::seconds(atoi(w2.c_str()));
        }
        else if (w1 == "login_log_filename"_s)
        {
            login_log_filename = w2;
        }
        else if (w1 == "login_log_unknown_packets_filename"_s)
        {
            login_log_unknown_packets_filename = w2;
        }
        else if (w1 == "save_unknown_packets"_s)
        {
            save_unknown_packets = config_switch(w2);
        }
        else if (w1 == "display_parse_login"_s)
        {
            display_parse_login = config_switch(w2);   // 0: no, 1: yes
        }
        else if (w1 == "display_parse_admin"_s)
        {
            display_parse_admin = config_switch(w2);   // 0: no, 1: yes
        }
        else if (w1 == "display_parse_fromchar"_s)
        {
            display_parse_fromchar = config_switch(w2);    // 0: no, 1: yes (without packet 0x2714), 2: all packets
        }
        else if (w1 == "min_level_to_connect"_s)
        {
            min_level_to_connect = GmLevel::from(static_cast<uint32_t>(atoi(w2.c_str())));
        }
        else if (w1 == "add_to_unlimited_account"_s)
        {
            add_to_unlimited_account = config_switch(w2);
        }
        else if (w1 == "start_limited_time"_s)
        {
            start_limited_time = atoi(w2.c_str());
        }
        else if (w1 == "check_ip_flag"_s)
        {
            check_ip_flag = config_switch(w2);
        }
        else if (w1 == "order"_s)
        {
            if (w2 == "deny,allow"_s || w2 == "deny, allow"_s)
                access_order = ACO::DENY_ALLOW;
            else if (w2 == "allow,deny"_s || w2 == "allow, deny"_s)
                access_order = ACO::ALLOW_DENY;
            else if (w2 == "mutual-failture"_s || w2 == "mutual-failure"_s)
                access_order = ACO::MUTUAL_FAILURE;
            else
            {
                PRINTF("Bad order: %s\n"_fmt, w2);
                return false;
            }
        }
        else if (w1 == "allow"_s)
        {
            if (w2 == "clear"_s)
            {
                access_allow.clear();
            }
            else
            {
                if (w2 == "all"_s)
                {
                    // reset all previous values
                    access_allow.clear();
                    // set to all
                    access_allow.push_back(IP4Mask());
                }
                else if (w2
                        && !(access_allow.size() == 1
                            && access_allow.front().mask() == IP4Address()))
                {
                    // don't add IP if already 'all'
                    IP4Mask n;
                    if (!extract(w2, &n))
                    {
                        PRINTF("Bad IP mask: %s\n"_fmt, w2);
                        return false;
                    }
                    access_allow.push_back(n);
                }
            }
        }
        else if (w1 == "deny"_s)
        {
            if (w2 == "clear"_s)
            {
                access_deny.clear();
            }
            else
            {
                if (w2 == "all"_s)
                {
                    // reset all previous values
                    access_deny.clear();
                    // set to all
                    access_deny.push_back(IP4Mask());
                }
                else if (w2
                        && !(access_deny.size() == 1
                            && access_deny.front().mask() == IP4Address()))
                {
                    // don't add IP if already 'all'
                    IP4Mask n;
                    if (!extract(w2, &n))
                    {
                        PRINTF("Bad IP mask: %s\n"_fmt, w2);
                        return false;
                    }
                    access_deny.push_back(n);
                }
            }
        }
        else if (w1 == "anti_freeze_enable"_s)
        {
            anti_freeze_enable = config_switch(w2);
        }
        else if (w1 == "anti_freeze_interval"_s)
        {
            anti_freeze_interval = std::max(
                    std::chrono::seconds(atoi(w2.c_str())),
                    std::chrono::seconds(5));
        }
        else if (w1 == "update_host"_s)
        {
            update_host = w2;
        }
        else if (w1 == "main_server"_s)
        {
            main_server = stringish<ServerName>(w2);
        }
        else if (w1 == "userid"_s)
        {
            userid = stringish<AccountName>(w2);
        }
        else if (w1 == "passwd"_s)
        {
            passwd = stringish<AccountPass>(w2);
        }
        else
        {
            return false;
        }
    }

    return true;
}

//-------------------------------------
// Displaying of configuration warnings
//-------------------------------------
static
bool display_conf_warnings(void)
{
    bool rv = true;
    if (admin_state != 0 && admin_state != 1)
    {
        PRINTF("***WARNING: Invalid value for admin_state parameter -> set to 0 (no remote admin).\n"_fmt);
        admin_state = 0;
        rv = false;
    }

    if (admin_state == 1)
    {
        if (!admin_pass)
        {
            PRINTF("***WARNING: Administrator password is void (admin_pass).\n"_fmt);
            rv = false;
        }
        else if (admin_pass == stringish<AccountPass>("admin"_s))
        {
            PRINTF("***WARNING: You are using the default administrator password (admin_pass).\n"_fmt);
            PRINTF("            We highly recommend that you change it.\n"_fmt);
        }
    }

    if (!gm_pass)
    {
        PRINTF("***WARNING: 'To GM become' password is void (gm_pass).\n"_fmt);
        PRINTF("            We highly recommend that you set one password.\n"_fmt);
        rv = false;
    }
    else if (gm_pass == "gm"_s)
    {
        PRINTF("***WARNING: You are using the default GM password (gm_pass).\n"_fmt);
        PRINTF("            We highly recommend that you change it.\n"_fmt);
    }

    if (level_new_gm < 0 || level_new_gm > 99)
    {
        PRINTF("***WARNING: Invalid value for level_new_gm parameter -> set to 60 (default).\n"_fmt);
        level_new_gm = 60;
        rv = false;
    }

    if (new_account != 0 && new_account != 1)
    {
        PRINTF("***WARNING: Invalid value for new_account parameter -> set to 0 (no new account).\n"_fmt);
        new_account = 0;
        rv = false;
    }

    if (login_port < 1024 || login_port > 65535)
    {
        PRINTF("***WARNING: Invalid value for login_port parameter -> set to 6900 (default).\n"_fmt);
        login_port = 6900;
        rv = false;
    }

    if (gm_account_filename_check_timer.count() < 0)
    {
        PRINTF("***WARNING: Invalid value for gm_account_filename_check_timer parameter.\n"_fmt);
        PRINTF("            -> set to 15 sec (default).\n"_fmt);
        gm_account_filename_check_timer = std::chrono::seconds(15);
        rv = false;
    }
    else if (gm_account_filename_check_timer == std::chrono::seconds(1))
    {
        PRINTF("***WARNING: Invalid value for gm_account_filename_check_timer parameter.\n"_fmt);
        PRINTF("            -> set to 2 sec (minimum value).\n"_fmt);
        gm_account_filename_check_timer = std::chrono::seconds(2);
        rv = false;
    }

    if (save_unknown_packets != 0 && save_unknown_packets != 1)
    {
        PRINTF("WARNING: Invalid value for save_unknown_packets parameter -> set to 0-no save.\n"_fmt);
        save_unknown_packets = 0;
        rv = false;
    }

    if (display_parse_login != 0 && display_parse_login != 1)
    {                           // 0: no, 1: yes
        PRINTF("***WARNING: Invalid value for display_parse_login parameter\n"_fmt);
        PRINTF("            -> set to 0 (no display).\n"_fmt);
        display_parse_login = 0;
        rv = false;
    }

    if (display_parse_admin != 0 && display_parse_admin != 1)
    {                           // 0: no, 1: yes
        PRINTF("***WARNING: Invalid value for display_parse_admin parameter\n"_fmt);
        PRINTF("            -> set to 0 (no display).\n"_fmt);
        display_parse_admin = 0;
        rv = false;
    }

    if (display_parse_fromchar < 0 || display_parse_fromchar > 2)
    {                           // 0: no, 1: yes (without packet 0x2714), 2: all packets
        PRINTF("***WARNING: Invalid value for display_parse_fromchar parameter\n"_fmt);
        PRINTF("            -> set to 0 (no display).\n"_fmt);
        display_parse_fromchar = 0;
        rv = false;
    }

    if (add_to_unlimited_account != 0 && add_to_unlimited_account != 1)
    {                           // 0: no, 1: yes
        PRINTF("***WARNING: Invalid value for add_to_unlimited_account parameter\n"_fmt);
        PRINTF("            -> set to 0 (impossible to add a time to an unlimited account).\n"_fmt);
        add_to_unlimited_account = 0;
        rv = false;
    }

    if (start_limited_time < -1)
    {                           // -1: create unlimited account, 0 or more: additionnal sec from now to create limited time
        PRINTF("***WARNING: Invalid value for start_limited_time parameter\n"_fmt);
        PRINTF("            -> set to -1 (new accounts are created with unlimited time).\n"_fmt);
        start_limited_time = -1;
        rv = false;
    }

    if (check_ip_flag != 0 && check_ip_flag != 1)
    {                           // 0: no, 1: yes
        PRINTF("***WARNING: Invalid value for check_ip_flag parameter\n"_fmt);
        PRINTF("            -> set to 1 (check players ip between login-server & char-server).\n"_fmt);
        check_ip_flag = 1;
        rv = false;
    }

    if (access_order == ACO::DENY_ALLOW)
    {
        if (access_deny.size() == 1 && access_deny.front().mask() == IP4Address())
        {
            PRINTF("***WARNING: The IP security order is 'deny,allow' (allow if not deny).\n"_fmt);
            PRINTF("            And you refuse ALL IP.\n"_fmt);
            rv = false;
        }
    }
    else if (access_order == ACO::ALLOW_DENY)
    {
        if (access_allow.empty())
        {
            PRINTF("***WARNING: The IP security order is 'allow,deny' (deny if not allow).\n"_fmt);
            PRINTF("            But, NO IP IS AUTHORISED!\n"_fmt);
            rv = false;
        }
    }
    else
    {
        // ACO::MUTUAL_FAILURE
        if (access_allow.empty())
        {
            PRINTF("***WARNING: The IP security order is 'mutual-failture'\n"_fmt);
            PRINTF("            (allow if in the allow list and not in the deny list).\n"_fmt);
            PRINTF("            But, NO IP IS AUTHORISED!\n"_fmt);
            rv = false;
        }
        else if (access_deny.size() == 1 && access_deny.front().mask() == IP4Address())
        {
            PRINTF("***WARNING: The IP security order is mutual-failture\n"_fmt);
            PRINTF("            (allow if in the allow list and not in the deny list).\n"_fmt);
            PRINTF("            But, you refuse ALL IP!\n"_fmt);
            rv = false;
        }
    }
    return rv;
}

//-------------------------------
// Save configuration in log file
//-------------------------------
static
void save_config_in_log(void)
{
    // a newline in the log...
    LOGIN_LOG(""_fmt);
    LOGIN_LOG("The login-server starting...\n"_fmt);

    // save configuration in log file
    LOGIN_LOG("The configuration of the server is set:\n"_fmt);

    if (admin_state != 1)
        LOGIN_LOG("- with no remote administration.\n"_fmt);
    else if (!admin_pass)
        LOGIN_LOG("- with a remote administration with a VOID password.\n"_fmt);
    else if (admin_pass == stringish<AccountPass>("admin"_s))
        LOGIN_LOG("- with a remote administration with the DEFAULT password.\n"_fmt);
    else
        LOGIN_LOG("- with a remote administration with the password of %zu character(s).\n"_fmt,
                admin_pass.size());
    if (access_ladmin.empty()
        || (access_ladmin.size() == 1 && access_ladmin.front().mask() == IP4Address()))
    {
        LOGIN_LOG("- to accept any IP for remote administration\n"_fmt);
    }
    else
    {
        LOGIN_LOG("- to accept following IP for remote administration:\n"_fmt);
        for (const IP4Mask& ae : access_ladmin)
            LOGIN_LOG("  %s\n"_fmt, ae);
    }

    if (!gm_pass)
        LOGIN_LOG("- with a VOID 'To GM become' password (gm_pass).\n"_fmt);
    else if (gm_pass == "gm"_s)
        LOGIN_LOG("- with the DEFAULT 'To GM become' password (gm_pass).\n"_fmt);
    else
        LOGIN_LOG("- with a 'To GM become' password (gm_pass) of %zu character(s).\n"_fmt,
                gm_pass.size());
    if (level_new_gm == 0)
        LOGIN_LOG("- to refuse any creation of GM with @gm.\n"_fmt);
    else
        LOGIN_LOG("- to create GM with level '%d' when @gm is used.\n"_fmt,
                   level_new_gm);

    if (new_account == 1)
        LOGIN_LOG("- to ALLOW new users (with _F/_M).\n"_fmt);
    else
        LOGIN_LOG("- to NOT ALLOW new users (with _F/_M).\n"_fmt);
    LOGIN_LOG("- with port: %d.\n"_fmt, login_port);
    LOGIN_LOG("- with the accounts file name: '%s'.\n"_fmt,
               account_filename);
    LOGIN_LOG("- with the GM accounts file name: '%s'.\n"_fmt,
               gm_account_filename);
    if (gm_account_filename_check_timer == interval_t::zero())
        LOGIN_LOG("- to NOT check GM accounts file modifications.\n"_fmt);
    else
        LOGIN_LOG("- to check GM accounts file modifications every %lld seconds.\n"_fmt,
             maybe_cast<long long>(gm_account_filename_check_timer.count()));

    // not necessary to log the 'login_log_filename', we are inside :)

    LOGIN_LOG("- with the unknown packets file name: '%s'.\n"_fmt,
               login_log_unknown_packets_filename);
    if (save_unknown_packets)
        LOGIN_LOG("- to SAVE all unkown packets.\n"_fmt);
    else
        LOGIN_LOG("- to SAVE only unkown packets sending by a char-server or a remote administration.\n"_fmt);
    if (display_parse_login)
        LOGIN_LOG("- to display normal parse packets on console.\n"_fmt);
    else
        LOGIN_LOG("- to NOT display normal parse packets on console.\n"_fmt);
    if (display_parse_admin)
        LOGIN_LOG("- to display administration parse packets on console.\n"_fmt);
    else
        LOGIN_LOG("- to NOT display administration parse packets on console.\n"_fmt);
    if (display_parse_fromchar)
        LOGIN_LOG("- to display char-server parse packets on console.\n"_fmt);
    else
        LOGIN_LOG("- to NOT display char-server parse packets on console.\n"_fmt);

    if (!min_level_to_connect)  // 0: all players, 1-99 at least gm level x
        LOGIN_LOG("- with no minimum level for connection.\n"_fmt);
    else
        LOGIN_LOG("- to accept only GM with level %d or more.\n"_fmt,
                   min_level_to_connect);

    if (add_to_unlimited_account)
        LOGIN_LOG("- to authorize adjustment (with timeadd ladmin) on an unlimited account.\n"_fmt);
    else
        LOGIN_LOG("- to refuse adjustment (with timeadd ladmin) on an unlimited account. You must use timeset (ladmin command) before.\n"_fmt);

    if (start_limited_time < 0)
        LOGIN_LOG("- to create new accounts with an unlimited time.\n"_fmt);
    else if (start_limited_time == 0)
        LOGIN_LOG("- to create new accounts with a limited time: time of creation.\n"_fmt);
    else
        LOGIN_LOG("- to create new accounts with a limited time: time of creation + %d second(s).\n"_fmt,
             start_limited_time);

    if (check_ip_flag)
        LOGIN_LOG("- with control of players IP between login-server and char-server.\n"_fmt);
    else
        LOGIN_LOG("- to not check players IP between login-server and char-server.\n"_fmt);

    if (access_order == ACO::DENY_ALLOW)
    {
        if (access_deny.empty())
        {
            LOGIN_LOG("- with the IP security order: 'deny,allow' (allow if not deny). You refuse no IP.\n"_fmt);
        }
        else if (access_deny.size() == 1 && access_deny.front().mask() == IP4Address())
        {
            LOGIN_LOG("- with the IP security order: 'deny,allow' (allow if not deny). You refuse ALL IP.\n"_fmt);
        }
        else
        {
            LOGIN_LOG("- with the IP security order: 'deny,allow' (allow if not deny). Refused IP are:\n"_fmt);
            for (IP4Mask ae : access_deny)
                LOGIN_LOG("  %s\n"_fmt, ae);
        }
    }
    else if (access_order == ACO::ALLOW_DENY)
    {
        if (access_allow.empty())
        {
            LOGIN_LOG("- with the IP security order: 'allow,deny' (deny if not allow). But, NO IP IS AUTHORISED!\n"_fmt);
        }
        else if (access_allow.size() == 1 && access_allow.front().mask() == IP4Address())
        {
            LOGIN_LOG("- with the IP security order: 'allow,deny' (deny if not allow). You authorise ALL IP.\n"_fmt);
        }
        else
        {
            LOGIN_LOG("- with the IP security order: 'allow,deny' (deny if not allow). Authorised IP are:\n"_fmt);
            for (IP4Mask ae : access_allow)
                LOGIN_LOG("  %s\n"_fmt, ae);
        }
    }
    else
    {                           // ACO_MUTUAL_FAILTURE
        LOGIN_LOG("- with the IP security order: 'mutual-failture' (allow if in the allow list and not in the deny list).\n"_fmt);
        if (access_allow.empty())
        {
            LOGIN_LOG("  But, NO IP IS AUTHORISED!\n"_fmt);
        }
        else if (access_deny.size() == 1 && access_deny.front().mask() == IP4Address())
        {
            LOGIN_LOG("  But, you refuse ALL IP!\n"_fmt);
        }
        else
        {
            if (access_allow.size() == 1 && access_allow.front().mask() == IP4Address())
            {
                LOGIN_LOG("  You authorise ALL IP.\n"_fmt);
            }
            else
            {
                LOGIN_LOG("  Authorised IP are:\n"_fmt);
                for (IP4Mask ae : access_allow)
                    LOGIN_LOG("    %s\n"_fmt, ae);
            }
            LOGIN_LOG("  Refused IP are:\n"_fmt);
            for (IP4Mask ae : access_deny)
                LOGIN_LOG("    %s\n"_fmt, ae);
        }
    }
}

//--------------------------------------
// Function called at exit of the server
//--------------------------------------
void term_func(void)
{
    mmo_auth_sync();

    auth_data.clear();
    gm_account_db.clear();
    for (int i = 0; i < MAX_SERVERS; i++)
    {
        Session *s = server_session[i];
        if (s)
            delete_session(s);
    }
    delete_session(login_session);

    LOGIN_LOG("----End of login-server (normal end with closing of all files).\n"_fmt);
}

static
bool login_confs(XString key, ZString value)
{
    unsigned sum = 0;
    sum += login_config(key, value);
    sum += login_lan_config(key, value);
    if (sum >= 2)
        abort();
    return sum;
}

//------------------------------
// Main function of login-server
//------------------------------
int do_init(Slice<ZString> argv)
{
    ZString argv0 = argv.pop_front();
    bool loaded_config_yet = false;
    while (argv)
    {
        ZString argvi = argv.pop_front();
        if (argvi.startswith('-'))
        {
            if (argvi == "--help"_s)
            {
                PRINTF("Usage: %s [--help] [--version] [files...]\n"_fmt,
                        argv0);
                exit(0);
            }
            else if (argvi == "--version"_s)
            {
                PRINTF("%s\n"_fmt, CURRENT_VERSION_STRING);
                exit(0);
            }
            else
            {
                FPRINTF(stderr, "Unknown argument: %s\n"_fmt, argvi);
                runflag = false;
            }
        }
        else
        {
            loaded_config_yet = true;
            runflag &= load_config_file(argvi, login_confs);
        }
    }

    if (!loaded_config_yet)
        runflag &= load_config_file("conf/tmwa-login.conf"_s, login_confs);

    // not in login_config_read, because we can use 'import' option, and display same message twice or more
    // (why is that bad?)
    runflag &= display_conf_warnings();
    // not before, because log file name can be changed
    // (that doesn't stop the char-server though)
    save_config_in_log();
    runflag &= lan_check();

    for (int i = 0; i < AUTH_FIFO_SIZE; i++)
        auth_fifo[i].delflag = 1;
    for (int i = 0; i < MAX_SERVERS; i++)
        server_session[i] = nullptr;

    read_gm_account();
    mmo_auth_init();
//     set_termfunc (mmo_auth_sync);
    login_session = make_listen_port(login_port, SessionParsers{.func_parse= parse_login, .func_delete= delete_login});


    Timer(gettick() + std::chrono::minutes(5),
            check_auth_sync,
            std::chrono::minutes(5)
    ).detach();

    if (anti_freeze_enable > 0)
    {
        Timer(gettick() + std::chrono::seconds(1),
                char_anti_freeze_system,
                anti_freeze_interval
        ).detach();
    }

    // add timer to check GM accounts file modification
    std::chrono::seconds j = gm_account_filename_check_timer;
    if (j == interval_t::zero())
        j = std::chrono::minutes(1);
    Timer(gettick() + j,
            check_GM_file,
            j).detach();

    LOGIN_LOG("The login-server is ready (Server is listening on the port %d).\n"_fmt,
         login_port);

    PRINTF("The login-server is " SGR_BOLD SGR_GREEN "ready" SGR_RESET " (Server is listening on the port %d).\n\n"_fmt,
         login_port);

    return 0;
}
