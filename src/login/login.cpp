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
#include "../io/extract.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/span.hpp"
#include "../io/tty.hpp"
#include "../io/write.hpp"

#include "../net/socket.hpp"
#include "../net/timer.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/human_time_diff.hpp"
#include "../mmo/ids.hpp"
#include "../mmo/version.hpp"

#include "../proto2/any-user.hpp"
#include "../proto2/login-admin.hpp"
#include "../proto2/login-char.hpp"
#include "../proto2/login-user.hpp"

#include "../high/core.hpp"
#include "../high/extract_mmo.hpp"
#include "../high/md5more.hpp"
#include "../high/mmo.hpp"
#include "../high/utils.hpp"

#include "../wire/packets.hpp"

#include "globals.hpp"
#include "login_conf.hpp"
#include "login_lan_conf.hpp"

#include "../poison.hpp"


namespace tmwa
{
DIAG_PUSH();
DIAG_I(missing_noreturn);
void SessionDeleter::operator()(SessionData *)
{
    assert(false && "login server does not have sessions anymore"_s);
}
DIAG_POP();

// out of namespace because ADL is dumb
bool extract(XString str, login::ACO *aco)
{
    using login::ACO;
    if (str == "deny,allow"_s || str == "deny, allow"_s)
    {
        *aco = ACO::DENY_ALLOW;
        return true;
    }
    if (str == "allow,deny"_s || str == "allow, deny"_s)
    {
        *aco = ACO::ALLOW_DENY;
        return true;
    }
    // typo from old config files
    if (str == "mutual-failture"_s || str == "mutual-failure"_s)
    {
        *aco = ACO::MUTUAL_FAILURE;
        return true;
    }
    return false;
}
namespace login
{
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


//------------------------------
// Writing function of logs file
//------------------------------
#define LOGIN_LOG(fmt, ...) \
    tmwa::login::login_log(STRPRINTF(fmt, ## __VA_ARGS__))
static
void login_log(XString line)
{
    io::AppendFile logfp(login_conf.login_log_filename);
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
    Option<P<GM_Account>> p = gm_account_db.search(account_id);
    return TRY_UNWRAP(p, return GmLevel())->level;
}

//-------------------------------------------------------
// Reading function of GM accounts file (and their level)
//-------------------------------------------------------
static
int read_gm_account(void)
{
    int c = 0;

    gm_account_db.clear();

    creation_time_GM_account_file = file_modified(login_conf.gm_account_filename);

    io::ReadFile fp(login_conf.gm_account_filename);
    if (!fp.is_open())
    {
        PRINTF("read_gm_account: GM accounts file [%s] not found.\n"_fmt,
                login_conf.gm_account_filename);
        PRINTF("                 Actually, there is no GM accounts on the server.\n"_fmt);
        LOGIN_LOG("read_gm_account: GM accounts file [%s] not found.\n"_fmt,
                login_conf.gm_account_filename);
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
                    login_conf.gm_account_filename, line);
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
            login_conf.gm_account_filename, c);
    LOGIN_LOG("read_gm_account: file '%s' readed (%d GM accounts found).\n"_fmt,
            login_conf.gm_account_filename, c);

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

    if (login_conf.allow.empty() && login_conf.deny.empty())
        return 1;
    // When there is no restriction, all IP are authorised.

    if (std::find_if(login_conf.allow.begin(), login_conf.allow.end(),
                [&ip](IP4Mask m)
                {
                    return m.covers(ip);
                }) != login_conf.allow.end())
    {
        {
            flag = ACF::ALLOW;
            if (login_conf.order == ACO::ALLOW_DENY)
                // With 'allow, deny' (deny if not allow), allow has priority
                return 1;
        }
    }

    if (std::find_if(login_conf.deny.begin(), login_conf.deny.end(),
                [&ip](IP4Mask m)
                {
                    return m.covers(ip);
                }) != login_conf.deny.end())
    {
        {
            flag = ACF::DENY;
            return 0;
            // At this point, if it's 'deny', we refuse connection.
        }
    }

    return flag == ACF::ALLOW || login_conf.order == ACO::DENY_ALLOW;
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
    if (login_conf.ladminallowip.empty())
        // When there is no restriction, all IP are authorised.
        return true;

    return std::find_if(login_conf.ladminallowip.begin(), login_conf.ladminallowip.end(),
            [&ip](IP4Mask m)
            {
                return m.covers(ip);
            }) != login_conf.ladminallowip.end();
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
            TimeT(),
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
bool impl_extract(XString line, AuthData *ad)
{
    TimeT unused_connect_until_time;
    std::vector<GlobalReg> vars;
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
                    &unused_connect_until_time,
                    &ip,
                    &ad->memo,
                    &ad->ban_until_time,
                    vrec<' '>(&vars))))
        return false;
    if (ad->lastlogin == stringish<timestamp_milliseconds_buffer>("-"_s))
        stamp_time(ad->lastlogin);
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

    io::ReadFile in(login_conf.account_filename);
    if (!in.is_open())
    {
        // no account file -> no account -> no login, including char-server (ERROR)
        // not anymore! :-)
        PRINTF(SGR_BOLD SGR_RED "mmo_auth_init: Accounts file [%s] not found." SGR_RESET "\n"_fmt,
                login_conf.account_filename);
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
            login_conf.account_filename, auth_data.size(), gm_count);
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
    io::WriteLock fp(login_conf.account_filename);

    if (!fp.is_open())
    {
        PRINTF("uh-oh - unable to save accounts\n"_fmt);
        return;
    }
    FPRINTF(fp,
            "// Accounts file: here are saved all information about the accounts.\n"_fmt);
    FPRINTF(fp,
            "// Structure: ID, account name, password, last login time, sex, # of logins, state, email, error message for state 7, validity time (unused), last (accepted) login ip, memo field, ban timestamp, repeated(register text, register value)\n"_fmt);
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


static
auto iter_char_sessions() -> decltype(filter_iterator<Session *>(&server_session))
{
    return filter_iterator<Session *>(&server_session);
}

//-----------------------------------------------------
// Send GM accounts to all char-server
//-----------------------------------------------------
static
void send_GM_accounts(Session *only=nullptr)
{
    std::vector<Packet_Repeat<0x2732>> tail;

    for (const AuthData& ad : auth_data)
    {
        // send only existing accounts. We can not create a GM account when server is online.
        if (GmLevel GM_value = isGM(ad.account_id))
        {
            Packet_Repeat<0x2732> item;
            item.account_id = ad.account_id;
            item.gm_level = GM_value;
            tail.push_back(item);
        }
    }
    if (only)
    {
        send_packet_repeatonly<0x2732, 4, 5>(only, tail);
        return;
    }
    for (Session *ss : iter_char_sessions())
    {
        send_packet_repeatonly<0x2732, 4, 5>(ss, tail);
    }
}

//-----------------------------------------------------
// Check if GM file account have been changed
//-----------------------------------------------------
static
void check_GM_file(TimerData *, tick_t)
{
    // if we would not check
    if (login_conf.gm_account_filename_check_timer == interval_t::zero())
        return;

    // get last modify time/date
    tick_t new_time = file_modified(login_conf.gm_account_filename);

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
    stamp_time(ad.lastlogin);
    ad.sex = sex;
    ad.logincount = 0;
    ad.state = 0;

    if (!e_mail_check(email))
        ad.email = DEFAULT_EMAIL;
    else
        ad.email = email;

    ad.error_message = stringish<timestamp_seconds_buffer>("-"_s);
    ad.ban_until_time = TimeT();

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
        && login_conf.new_account && account_id_count < END_ACCOUNT_NUM
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

    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(s, &packet_id))
    {
        if (login_conf.display_parse_fromchar == 2 || (login_conf.display_parse_fromchar == 1 && packet_id != 0x2714))   // 0x2714 is done very often (number of players)
            PRINTF("parse_fromchar: connection #%d, packet: 0x%x (with being read: %zu bytes).\n"_fmt,
                    s, packet_id, packet_avail(s));

        switch (packet_id)
        {
            case 0x2712:       // request from char-server to authentify an account
            {
                Packet_Fixed<0x2712> fixed;
                rv = recv_fpacket<0x2712, 19>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    int i;
                    for (i = 0; i < AUTH_FIFO_SIZE; i++)
                    {
                        if (auth_fifo[i].account_id == acc &&
                            auth_fifo[i].login_id1 == fixed.login_id1 &&
                            auth_fifo[i].login_id2 == fixed.login_id2 &&    // relate to the versions higher than 18
                            auth_fifo[i].sex == fixed.sex &&
                            auth_fifo[i].ip == fixed.ip
                            && !auth_fifo[i].delflag)
                        {
                            auth_fifo[i].delflag = 1;
                            LOGIN_LOG("Char-server '%s': authentification of the account %d accepted (ip: %s).\n"_fmt,
                                    server[id].name, acc, ip);
                            for (const AuthData& ad : auth_data)
                            {
                                if (ad.account_id == acc)
                                {
                                    Packet_Head<0x2729> head_29;
                                    head_29.account_id = acc;
                                    std::vector<Packet_Repeat<0x2729>> repeat_29;
                                    int j;
                                    for (j = 0;
                                         j < ad.account_reg2_num;
                                         j++)
                                    {
                                        Packet_Repeat<0x2729> item;
                                        item.name = ad.account_reg2[j].str;
                                        item.value = ad.account_reg2[j].value;
                                        repeat_29.push_back(item);
                                    }
                                    send_vpacket<0x2729, 8, 36>(s, head_29, repeat_29);

                                    Packet_Fixed<0x2713> fixed_13;
                                    fixed_13.account_id = acc;
                                    fixed_13.invalid = 0;
                                    fixed_13.email = ad.email;

                                    send_fpacket<0x2713, 51>(s, fixed_13);
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

                        Packet_Fixed<0x2713> fixed_13;
                        fixed_13.account_id = acc;
                        fixed_13.invalid = 1;
                        // fixed_13.email
                        // fixed_13.connect_until

                        send_fpacket<0x2713, 51>(s, fixed_13);
                    }
                }
                break;
            }

            case 0x2714:
            {
                Packet_Fixed<0x2714> fixed;
                rv = recv_fpacket<0x2714, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                server[id].users = fixed.users;
                if (login_conf.anti_freeze_enable)
                    server_freezeflag[id] = 5;  // Char anti-freeze system. Counter. 5 ok, 4...0 freezed
                break;
            }

                // We receive an e-mail/limited time request, because a player comes back from a map-server to the char-server
            case 0x2716:
            {
                Packet_Fixed<0x2716> fixed;
                rv = recv_fpacket<0x2716, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId account_id = fixed.account_id;
                for (const AuthData& ad : auth_data)
                {
                    if (ad.account_id == account_id)
                    {
                        LOGIN_LOG("Char-server '%s': e-mail of the account %d found (ip: %s).\n"_fmt,
                                server[id].name, account_id, ip);

                        Packet_Fixed<0x2717> fixed_17;
                        fixed_17.account_id = account_id;
                        fixed_17.email = ad.email;

                        send_fpacket<0x2717, 50>(s, fixed_17);
                        if (rv != RecvResult::Complete)
                            break;
                        goto x2716_end;
                    }
                }
                LOGIN_LOG("Char-server '%s': e-mail of the account %d NOT found (ip: %s).\n"_fmt,
                        server[id].name, account_id, ip);
            x2716_end:
                break;
            }

                // Map server send information to change an email of an account via char-server
            case 0x2722:       // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
            {
                Packet_Fixed<0x2722> fixed;
                rv = recv_fpacket<0x2722, 86>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    AccountEmail actual_email = stringish<AccountEmail>(fixed.old_email.to_print());
                    AccountEmail new_email = fixed.new_email;
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
                break;
            }

                // Receiving of map-server via char-server a status change resquest (by Yor)
            case 0x2724:
            {
                Packet_Fixed<0x2724> fixed;
                rv = recv_fpacket<0x2724, 10>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    int statut = fixed.status;
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
                                    Packet_Fixed<0x2731> fixed_31;
                                    fixed_31.account_id = acc;
                                    fixed_31.ban_not_status = 0;
                                    fixed_31.status_or_ban_until = static_cast<time_t>(statut);

                                    for (Session *ss : iter_char_sessions())
                                    {
                                        send_fpacket<0x2731, 11>(ss, fixed_31);
                                    }

                                    for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                    {
                                        if (auth_fifo[j].account_id == acc)
                                            auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                    }
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
                    ;
                }
                break;
            }

            case 0x2725:       // Receiving of map-server via char-server a ban resquest (by Yor)
            {
                Packet_Fixed<0x2725> fixed;
                rv = recv_fpacket<0x2725, 18>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
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
                            HumanTimeDiff ban_diff = fixed.ban_add;
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
                                        timestamp_seconds_buffer tmpstr;
                                        if (timestamp)
                                            stamp_time(tmpstr, &timestamp);
                                        LOGIN_LOG("Char-server '%s': Ban request (account: %d, new final date of banishment: %lld (%s), ip: %s).\n"_fmt,
                                                server[id].name, acc,
                                                timestamp,
                                                tmpstr,
                                                ip);
                                        Packet_Fixed<0x2731> fixed_31;
                                        fixed_31.account_id = ad.account_id;
                                        fixed_31.ban_not_status = 1;
                                        fixed_31.status_or_ban_until = timestamp;

                                        for (Session *ss : iter_char_sessions())
                                        {
                                            send_fpacket<0x2731, 11>(ss, fixed_31);
                                        }

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
                    ;
                }
                break;
            }

            case 0x2727:       // Change of sex (sex is reversed)
            {
                Packet_Fixed<0x2727> fixed;
                rv = recv_fpacket<0x2727, 7>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            {
                                SEX sex = fixed.sex;
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

                                Packet_Fixed<0x2723> fixed_23;
                                fixed_23.account_id = acc;
                                fixed_23.sex = sex;

                                for (Session *ss : iter_char_sessions())
                                {
                                    send_fpacket<0x2723, 7>(ss, fixed_23);
                                }
                            }
                            goto x2727_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': Error of sex change (account: %d not found, sex would be reversed, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                x2727_out:
                    ;
                }
                break;
            }

            case 0x2728:       // We receive account_reg2 from a char-server, and we send them to other char-servers.
            {
                Packet_Head<0x2728> head;
                std::vector<Packet_Repeat<0x2728>> repeat;
                rv = recv_vpacket<0x2728, 8, 36>(s, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = head.account_id;
                    for (AuthData& ad : auth_data)
                    {
                        if (ad.account_id == acc)
                        {
                            LOGIN_LOG("Char-server '%s': receiving (from the char-server) of account_reg2 (account: %d, ip: %s).\n"_fmt,
                                    server[id].name, acc, ip);

                            const size_t count = std::min(ACCOUNT_REG2_NUM, repeat.size());
                            for (size_t j = 0; j < count; ++j)
                            {
                                ad.account_reg2[j].str = repeat[j].name;
                                ad.account_reg2[j].value = repeat[j].value;
                            }
                            ad.account_reg2_num = count;

                            // Sending information towards the other char-servers.
                            Packet_Head<0x2729> head_29;
                            std::vector<Packet_Repeat<0x2729>> repeat_29(repeat.size());
                            head_29.account_id = head.account_id;
                            for (size_t j = 0; j < count; ++j)
                            {
                                repeat_29[j].name = repeat[j].name;
                                repeat_29[j].value = repeat[j].value;
                            }

                            for (Session *ss : iter_char_sessions())
                            {
                                if (ss == s)
                                    continue;
                                send_vpacket<0x2729, 8, 36>(ss, head_29, repeat_29);
                            }
                            goto x2728_out;
                        }
                    }
                    LOGIN_LOG("Char-server '%s': receiving (from the char-server) of account_reg2 (account: %d not found, ip: %s).\n"_fmt,
                            server[id].name, acc, ip);
                }
            x2728_out:
                break;
            }

            case 0x272a:       // Receiving of map-server via char-server a unban resquest (by Yor)
            {
                Packet_Fixed<0x272a> fixed;
                rv = recv_fpacket<0x272a, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
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
                    ;
                }
                break;
            }

                // request from char-server to change account password
            case 0x2740:       // 0x2740 <account_id>.L <actual_password>.24B <new_password>.24B
            {
                Packet_Fixed<0x2740> fixed;
                rv = recv_fpacket<0x2740, 54>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    AccountPass actual_pass = stringish<AccountPass>(fixed.old_pass.to_print());
                    AccountPass new_pass = stringish<AccountPass>(fixed.new_pass.to_print());

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
                    Packet_Fixed<0x2741> fixed_41;
                    fixed_41.account_id = acc;
                    fixed_41.status = status;
                    send_fpacket<0x2741, 7>(s, fixed_41);
                }

                break;
            }

            default:
            {
                {
                    timestamp_milliseconds_buffer timestr;
                    stamp_time(timestr);
                    FPRINTF(stderr,
                            "%s: receiving of an unknown packet -> disconnection\n"_fmt,
                            timestr);
                    FPRINTF(stderr,
                            "parse_fromchar: connection #%d (ip: %s), packet: 0x%x (with being read: %zu).\n"_fmt,
                            s, ip, packet_id, packet_avail(s));
                    FPRINTF(stderr, "Detail (in hex):\n"_fmt);
                    packet_dump(s);
                }
                PRINTF("parse_fromchar: Unknown packet 0x%x (from a char-server)! -> disconnection.\n"_fmt,
                        packet_id);
                s->set_eof();
                PRINTF("Char-server has been disconnected (unknown packet).\n"_fmt);
                return;
            }
        }
    }
    if (rv == RecvResult::Error)
        s->set_eof();
    return;
}

//---------------------------------------
// Packet parsing for administation login
//---------------------------------------
static
void parse_admin(Session *s)
{
    IP4Address ip = s->client_ip;
    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(s, &packet_id))
    {
        if (login_conf.display_parse_admin)
            PRINTF("parse_admin: connection #%d, packet: 0x%x (with being read: %zu bytes).\n"_fmt,
                    s, packet_id, packet_avail(s));

        switch (packet_id)
        {
            case 0x7530:       // Request of the server version
            {
                Packet_Fixed<0x7530> fixed;
                rv = recv_fpacket<0x7530, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                LOGIN_LOG("'ladmin': Sending of the server version (ip: %s)\n"_fmt,
                        ip);

                Packet_Fixed<0x7531> fixed_31;
                fixed_31.version = CURRENT_LOGIN_SERVER_VERSION;
                send_fpacket<0x7531, 10>(s, fixed_31);
                break;
            }

            case 0x7532:       // Request of end of connection
            {
                Packet_Fixed<0x7532> fixed;
                rv = recv_fpacket<0x7532, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                LOGIN_LOG("'ladmin': End of connection (ip: %s)\n"_fmt,
                        ip);
                s->set_eof();
                return;
            }

            case 0x7920:       // Request of an accounts list
            {
                Packet_Fixed<0x7920> fixed;
                rv = recv_fpacket<0x7920, 10>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId st = fixed.start_account_id;
                    AccountId ed = fixed.end_account_id;
                    if (!(ed < END_ACCOUNT_NUM) || ed < st || !ed)
                        ed = END_ACCOUNT_NUM;

                    LOGIN_LOG("'ladmin': Sending an accounts list (ask: from %d to %d, ip: %s)\n"_fmt,
                            st, ed, ip);

                    // Sending accounts information
                    std::vector<Packet_Repeat<0x7921>> repeat_21;

                    for (const AuthData& ad : auth_data)
                    {
                        AccountId account_id = ad.account_id;
                        if (!(account_id < st) && !(ed < account_id))
                        {
                            Packet_Repeat<0x7921> info;
                            info.account_id = account_id;
                            info.gm_level = isGM(account_id);
                            info.account_name = ad.userid;
                            info.sex = ad.sex;
                            info.login_count = ad.logincount;
                            if (ad.state == 0 && ad.ban_until_time)  // if no state and banished
                                info.status = 7;  // 6 = Your are Prohibited to log in until %s
                            else
                                info.status = ad.state;
                            repeat_21.push_back(info);
                        }
                    }
                    send_packet_repeatonly<0x7921, 4, 38>(s, repeat_21);
                }
                break;
            }

            case 0x7930:       // Request for an account creation
            {
                Packet_Fixed<0x7930> fixed;
                rv = recv_fpacket<0x7930, 91>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    struct mmo_account ma;
                    // TODO make this a 'return false' bit of the network_to_native
                    ma.userid = stringish<AccountName>(fixed.account_name.to_print());
                    ma.passwd = stringish<AccountPass>(fixed.password.to_print());
                    stamp_time(ma.lastlogin);
                    ma.sex = fixed.sex;

                    Packet_Fixed<0x7931> fixed_31;
                    fixed_31.account_id = AccountId();
                    fixed_31.account_name = ma.userid;
                    if (ma.userid.size() < 4 || ma.passwd.size() < 4)
                    {
                        LOGIN_LOG("'ladmin': Attempt to create an invalid account (account or pass is too short, ip: %s)\n"_fmt,
                                ip);
                    }
                    else if (ma.sex != SEX::FEMALE && ma.sex != SEX::MALE && ma.sex != SEX::NEUTRAL)
                    {
                        LOGIN_LOG("'ladmin': Attempt to create an invalid account (account: %s, invalid sex, ip: %s)\n"_fmt,
                                ma.userid, ip);
                    }
                    else if (!(account_id_count < END_ACCOUNT_NUM))
                    {
                        LOGIN_LOG("'ladmin': Attempt to create an account, but there is no more available id number (account: %s, sex: %c, ip: %s)\n"_fmt,
                                ma.userid, sex_to_char(ma.sex), ip);
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
                            AccountEmail email = fixed.email;
                            AccountId new_id = mmo_auth_new(&ma, ma.sex, email);
                            LOGIN_LOG("'ladmin': Account creation (account: %s (id: %d), sex: %c, email: %s, ip: %s)\n"_fmt,
                                    ma.userid, new_id,
                                    sex_to_char(ma.sex), auth_data.back().email, ip);
                            fixed_31.account_id = new_id;
                        }
                    }
                x7930_out:
                    send_fpacket<0x7931, 30>(s, fixed_31);
                }
                break;
            }

            case 0x7932:       // Request for an account deletion
            {
                Packet_Fixed<0x7932> fixed;
                rv = recv_fpacket<0x7932, 26>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7933> fixed_33;
                fixed_33.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                AuthData *ad = search_account(account_name);
                if (ad)
                {
                    // Char-server is notified of deletion (for characters deletion).
                    Packet_Fixed<0x2730> fixed_30;
                    fixed_30.account_id = ad->account_id;

                    for (Session *ss : iter_char_sessions())
                    {
                        send_fpacket<0x2730, 6>(ss, fixed_30);
                    }

                    // send answer
                    fixed_33.account_name = ad->userid;
                    fixed_33.account_id = ad->account_id;
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
                    fixed_33.account_name = account_name;
                    LOGIN_LOG("'ladmin': Attempt to delete an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                }
                send_fpacket<0x7933, 30>(s, fixed_33);
                break;
            }

            case 0x7934:       // Request to change a password
            {
                Packet_Fixed<0x7934> fixed;
                rv = recv_fpacket<0x7934, 50>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7935> fixed_35;
                fixed_35.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                AuthData *ad = search_account(account_name);
                if (ad)
                {
                    fixed_35.account_name = ad->userid;
                    AccountPass plain = stringish<AccountPass>(fixed.password);
                    ad->pass = MD5_saltcrypt(plain, make_salt());
                    fixed_35.account_id = ad->account_id;
                    LOGIN_LOG("'ladmin': Modification of a password (account: %s, new password: %s, ip: %s)\n"_fmt,
                            ad->userid, ad->pass, ip);
                }
                else
                {
                    fixed_35.account_name = account_name;
                    LOGIN_LOG("'ladmin': Attempt to modify the password of an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                }
                send_fpacket<0x7935, 30>(s, fixed_35);
                break;
            }

            case 0x7936:       // Request to modify a state
            {
                Packet_Fixed<0x7936> fixed;
                rv = recv_fpacket<0x7936, 50>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    Packet_Fixed<0x7937> fixed_37;
                    fixed_37.account_id = AccountId();
                    AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                    int statut = fixed.status;
                    timestamp_seconds_buffer error_message = stringish<timestamp_seconds_buffer>(fixed.error_message.to_print());
                    if (statut != 7 || !error_message)
                    {
                        // 7: // 6 = Your are Prohibited to log in until %s
                        error_message = stringish<timestamp_seconds_buffer>("-"_s);
                    }
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        fixed_37.account_name = ad->userid;
                        fixed_37.account_id = ad->account_id;
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
                                Packet_Fixed<0x2731> fixed_31;
                                fixed_31.account_id = ad->account_id;
                                fixed_31.ban_not_status = 0;
                                fixed_31.status_or_ban_until = static_cast<time_t>(statut);

                                for (Session *ss : iter_char_sessions())
                                {
                                    send_fpacket<0x2731, 11>(ss, fixed_31);
                                }

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
                        fixed_37.account_name = account_name;
                        LOGIN_LOG("'ladmin': Attempt to modify the state of an unknown account (account: %s, received state: %d, ip: %s)\n"_fmt,
                                account_name, statut, ip);
                    }
                    fixed_37.status = statut;
                    send_fpacket<0x7937, 34>(s, fixed_37);
                }
                break;
            }

            case 0x7938:       // Request for servers list and # of online players
            {
                Packet_Fixed<0x7938> fixed;
                rv = recv_fpacket<0x7938, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                LOGIN_LOG("'ladmin': Sending of servers list (ip: %s)\n"_fmt, ip);
                std::vector<Packet_Repeat<0x7939>> repeat_39;
                for (int i = 0; i < MAX_SERVERS; i++)
                {
                    if (server_session[i])
                    {
                        Packet_Repeat<0x7939> info;
                        info.ip = server[i].ip;
                        info.port = server[i].port;
                        info.name = server[i].name;
                        info.users = server[i].users;
                        info.maintenance = 0;
                        info.is_new = 0;
                        repeat_39.push_back(info);
                    }
                }
                send_packet_repeatonly<0x7939, 4, 32>(s, repeat_39);
                break;
            }

            case 0x793a:       // Request to password check
            {
                Packet_Fixed<0x793a> fixed;
                rv = recv_fpacket<0x793a, 50>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x793b> fixed_3b;
                fixed_3b.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                const AuthData *ad = search_account(account_name);
                if (ad)
                {
                    fixed_3b.account_name = ad->userid;
                    AccountPass pass = stringish<AccountPass>(fixed.password);
                    if (pass_ok(pass, ad->pass))
                    {
                        fixed_3b.account_id = ad->account_id;
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
                    fixed_3b.account_name = account_name;
                    LOGIN_LOG("'ladmin': Attempt to check the password of an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                }
                send_fpacket<0x793b, 30>(s, fixed_3b);
                break;
            }

            case 0x793c:       // Request to modify sex
            {
                Packet_Fixed<0x793c> fixed;
                rv = recv_fpacket<0x793c, 27>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x793d> fixed_3d;
                fixed_3d.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                fixed_3d.account_name = account_name;

                {
                    SEX sex = fixed.sex;
                    if (sex != SEX::FEMALE && sex != SEX::MALE && sex != SEX::NEUTRAL)
                    {
                        LOGIN_LOG("'ladmin': Attempt to give an invalid sex (account: %s, received sex: %c, ip: %s)\n"_fmt,
                                account_name, sex_to_char(sex), ip);
                    }
                    else
                    {
                        AuthData *ad = search_account(account_name);
                        if (ad)
                        {
                            fixed_3d.account_name = ad->userid;
                            if (ad->sex != sex)
                            {
                                fixed_3d.account_id = ad->account_id;
                                for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                {
                                    if (auth_fifo[j].account_id ==
                                        ad->account_id)
                                        auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                }
                                ad->sex = sex;
                                LOGIN_LOG("'ladmin': Modification of a sex (account: %s, new sex: %c, ip: %s)\n"_fmt,
                                        ad->userid, sex_to_char(sex), ip);

                                // send to all char-server the change
                                Packet_Fixed<0x2723> fixed_23;
                                fixed_23.account_id = ad->account_id;
                                fixed_23.sex = ad->sex;

                                for (Session *ss : iter_char_sessions())
                                {
                                    send_fpacket<0x2723, 7>(ss, fixed_23);
                                }
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
                send_fpacket<0x793d, 30>(s, fixed_3d);
                break;
            }

            case 0x793e:       // Request to modify GM level
            {
                Packet_Fixed<0x793e> fixed;
                rv = recv_fpacket<0x793e, 27>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x793f> fixed_3f;
                fixed_3f.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                fixed_3f.account_name = account_name;
                bool reread = false;
                {
                    GmLevel new_gm_level = fixed.gm_level;
                    {
                        const AuthData *ad = search_account(account_name);
                        if (ad)
                        {
                            AccountId acc = ad->account_id;
                            fixed_3f.account_name = ad->userid;
                            if (isGM(acc) != new_gm_level)
                            {
                                // modification of the file
                                AccountId GM_account;
                                GmLevel GM_level;
                                int modify_flag;
                                io::WriteLock fp2(login_conf.gm_account_filename);
                                if (fp2.is_open())
                                {
                                    io::ReadFile fp(login_conf.gm_account_filename);
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
                                    fixed_3f.account_id = acc;
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
                send_fpacket<0x793f, 30>(s, fixed_3f);
                break;
            }

            case 0x7940:       // Request to modify e-mail
            {
                Packet_Fixed<0x7940> fixed;
                rv = recv_fpacket<0x7940, 66>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7941> fixed_41;
                fixed_41.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                fixed_41.account_name = account_name;
                {
                    AccountEmail email = stringish<AccountEmail>(fixed.email);
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
                            fixed_41.account_name = ad->userid;
                            ad->email = email;
                            fixed_41.account_id = ad->account_id;
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
                send_fpacket<0x7941, 30>(s, fixed_41);
                break;
            }

            case 0x7942:       // Request to modify memo field
            {
                Packet_Head<0x7942> head;
                AString repeat;
                rv = recv_vpacket<0x7942, 28, 1>(s, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7943> fixed_43;
                fixed_43.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(head.account_name.to_print());
                AuthData *ad = search_account(account_name);
                if (ad)
                {
                    fixed_43.account_name = ad->userid;
                    ad->memo = ""_s;
                    if (!repeat/*.startswith('!')*/)
                    {
                        ad->memo = "!"_s;
                    }
                    else
                    {
                        // may truncate
                        ad->memo = repeat;
                    }
                    ad->memo = ad->memo.to_print();
                    fixed_43.account_id = ad->account_id;
                    LOGIN_LOG("'ladmin': Modification of a memo field (account: %s, new memo: %s, ip: %s)\n"_fmt,
                            ad->userid, ad->memo, ip);
                }
                else
                {
                    fixed_43.account_name = account_name;
                    LOGIN_LOG("'ladmin': Attempt to modify the memo field of an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                }
                send_fpacket<0x7943, 30>(s, fixed_43);
                break;
            }

            case 0x7944:       // Request to found an account id
            {
                Packet_Fixed<0x7944> fixed;
                rv = recv_fpacket<0x7944, 26>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7945> fixed_45;
                fixed_45.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                const AuthData *ad = search_account(account_name);
                if (ad)
                {
                    fixed_45.account_name = ad->userid;
                    fixed_45.account_id = ad->account_id;
                    LOGIN_LOG("'ladmin': Request (by the name) of an account id (account: %s, id: %d, ip: %s)\n"_fmt,
                            ad->userid, ad->account_id,
                            ip);
                }
                else
                {
                    fixed_45.account_name = account_name;
                    LOGIN_LOG("'ladmin': ID request (by the name) of an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                }
                send_fpacket<0x7945, 30>(s, fixed_45);
                break;
            }

            case 0x7946:       // Request to found an account name
            {
                Packet_Fixed<0x7946> fixed;
                rv = recv_fpacket<0x7946, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId account_id = fixed.account_id;
                Packet_Fixed<0x7947> fixed_47;
                fixed_47.account_id = account_id;
                fixed_47.account_name = {};
                for (const AuthData& ad : auth_data)
                {
                    if (ad.account_id == account_id)
                    {
                        fixed_47.account_name = ad.userid;
                        LOGIN_LOG("'ladmin': Request (by id) of an account name (account: %s, id: %d, ip: %s)\n"_fmt,
                                ad.userid, account_id, ip);
                        goto x7946_out;
                    }
                }
                LOGIN_LOG("'ladmin': Name request (by id) of an unknown account (id: %d, ip: %s)\n"_fmt,
                        account_id, ip);
                fixed_47.account_name = stringish<AccountName>(""_s);
            x7946_out:
                send_fpacket<0x7947, 30>(s, fixed_47);
                break;
            }

            case 0x794a:       // Request to change the final date of a banishment (timestamp) (absolute value)
            {
                Packet_Fixed<0x794a> fixed;
                rv = recv_fpacket<0x794a, 30>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x794b> fixed_4b;
                {
                    fixed_4b.account_id = AccountId();
                    AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                    TimeT timestamp = fixed.ban_until;
                    if (timestamp <= TimeT::now())
                        timestamp = TimeT();
                    timestamp_seconds_buffer tmpstr = stringish<timestamp_seconds_buffer>("no banishment"_s);
                    if (timestamp)
                        stamp_time(tmpstr, &timestamp);
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        fixed_4b.account_name = ad->userid;
                        fixed_4b.account_id = ad->account_id;
                        LOGIN_LOG("'ladmin': Change of the final date of a banishment (account: %s, new final date of banishment: %lld (%s), ip: %s)\n"_fmt,
                                ad->userid, timestamp,
                                tmpstr,
                                ip);
                        if (ad->ban_until_time != timestamp)
                        {
                            if (timestamp)
                            {
                                Packet_Fixed<0x2731> fixed_31;
                                fixed_31.account_id = ad->account_id;
                                fixed_31.ban_not_status = 1;
                                fixed_31.status_or_ban_until = timestamp;

                                for (Session *ss : iter_char_sessions())
                                {
                                    send_fpacket<0x2731, 11>(ss, fixed_31);
                                }

                                for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                {
                                    if (auth_fifo[j].account_id ==
                                        ad->account_id)
                                        auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                }
                            }
                            ad->ban_until_time = timestamp;
                        }
                    }
                    else
                    {
                        fixed_4b.account_name = account_name;
                        LOGIN_LOG("'ladmin': Attempt to change the final date of a banishment of an unknown account (account: %s, received final date of banishment: %lld (%s), ip: %s)\n"_fmt,
                                account_name, timestamp,
                                tmpstr,
                                ip);
                    }
                    fixed_4b.ban_until = timestamp;
                }
                send_fpacket<0x794b, 34>(s, fixed_4b);
                break;
            }

            case 0x794c:       // Request to change the final date of a banishment (timestamp) (relative change)
            {
                Packet_Fixed<0x794c> fixed;
                rv = recv_fpacket<0x794c, 38>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x794d> fixed_4d;
                {
                    fixed_4d.account_id = AccountId();
                    AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                    AuthData *ad = search_account(account_name);
                    if (ad)
                    {
                        fixed_4d.account_id = ad->account_id;
                        fixed_4d.account_name = ad->userid;
                        TimeT timestamp;
                        TimeT now = TimeT::now();
                        if (!ad->ban_until_time
                            || ad->ban_until_time < now)
                            timestamp = now;
                        else
                            timestamp = ad->ban_until_time;
                        struct tm tmtime = timestamp;
                        HumanTimeDiff ban_diff = fixed.ban_add;
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
                            LOGIN_LOG("'ladmin': Adjustment of a final date of a banishment (account: %s, (%+d y %+d m %+d d %+d h %+d mn %+d s) -> new ban: %lld (%s), ip: %s)\n"_fmt,
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
                                    Packet_Fixed<0x2731> fixed_31;
                                    fixed_31.account_id = ad->account_id;
                                    fixed_31.ban_not_status = 1;
                                    fixed_31.status_or_ban_until = timestamp;

                                    for (Session *ss : iter_char_sessions())
                                    {
                                        send_fpacket<0x2731, 11>(ss, fixed_31);
                                    }

                                    for (int j = 0; j < AUTH_FIFO_SIZE; j++)
                                    {
                                        if (auth_fifo[j].account_id ==
                                            ad->account_id)
                                            auth_fifo[j].login_id1++;   // to avoid reconnection error when come back from map-server (char-server will ask again the authentification)
                                    }
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
                        fixed_4d.ban_until = ad->ban_until_time;
                    }
                    else
                    {
                        fixed_4d.account_name = account_name;
                        LOGIN_LOG("'ladmin': Attempt to adjust the final date of a banishment of an unknown account (account: %s, ip: %s)\n"_fmt,
                                account_name, ip);
                        fixed_4d.ban_until = TimeT();
                    }
                }
                send_fpacket<0x794d, 34>(s, fixed_4d);
                break;
            }

            case 0x794e:       // Request to send a broadcast message
            {
                Packet_Head<0x794e> head;
                AString repeat;
                rv = recv_vpacket<0x794e, 8, 1>(s, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x794f> fixed_4f;
                fixed_4f.error = -1;
                if (!repeat)
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
                        fixed_4f.error = 0;

                        AString& message = repeat;
                        LOGIN_LOG("'ladmin': Receiving a message for broadcast (message: %s, ip: %s)\n"_fmt,
                                message, ip);

                        // send same message to all char-servers (no answer)
                        Packet_Head<0x2726> head_26;
                        head_26.unused = head.unused;

                        for (Session *ss : iter_char_sessions())
                        {
                            send_vpacket<0x2726, 8, 1>(ss, head_26, message);
                        }
                    }
                }
            x794e_have_no_server:
                send_fpacket<0x794f, 4>(s, fixed_4f);
                break;
            }

            case 0x7952:       // Request about informations of an account (by account name)
            {
                Packet_Fixed<0x7952> fixed;
                rv = recv_fpacket<0x7952, 26>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Head<0x7953> head_53;
                head_53.account_id = AccountId();
                AccountName account_name = stringish<AccountName>(fixed.account_name.to_print());
                const AuthData *ad = search_account(account_name);
                if (ad)
                {
                    head_53.account_id = ad->account_id;
                    head_53.gm_level = isGM(ad->account_id);
                    head_53.account_name = ad->userid;
                    head_53.sex = ad->sex;
                    head_53.login_count = ad->logincount;
                    head_53.state = ad->state;
                    head_53.error_message = ad->error_message;
                    head_53.last_login_string = ad->lastlogin;
                    head_53.ip_string = convert_for_printf(ad->last_ip);
                    head_53.email = ad->email;
                    head_53.ban_until = ad->ban_until_time;

                    XString repeat_53 = ad->memo;
                    LOGIN_LOG("'ladmin': Sending information of an account (request by the name; account: %s, id: %d, ip: %s)\n"_fmt,
                            ad->userid, ad->account_id,
                            ip);

                    send_vpacket<0x7953, 150, 1>(s, head_53, repeat_53);
                }
                else
                {
                    head_53.account_name = account_name;
                    LOGIN_LOG("'ladmin': Attempt to obtain information (by the name) of an unknown account (account: %s, ip: %s)\n"_fmt,
                            account_name, ip);
                    send_vpacket<0x7953, 150, 1>(s, head_53, ""_s);
                }
                break;
            }

            case 0x7954:       // Request about information of an account (by account id)
            {
                Packet_Fixed<0x7954> fixed;
                rv = recv_fpacket<0x7954, 6>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId account_id = fixed.account_id;
                Packet_Head<0x7953> head_53;
                head_53.account_id = account_id;
                head_53.account_name = AccountName();
                for (const AuthData& ad : auth_data)
                {
                    if (ad.account_id == account_id)
                    {
                        LOGIN_LOG("'ladmin': Sending information of an account (request by the id; account: %s, id: %d, ip: %s)\n"_fmt,
                                ad.userid, account_id, ip);
                        head_53.gm_level = isGM(ad.account_id);
                        head_53.account_name = ad.userid;
                        head_53.sex = ad.sex;
                        head_53.login_count = ad.logincount;
                        head_53.state = ad.state;
                        head_53.error_message = ad.error_message;
                        head_53.last_login_string = ad.lastlogin;
                        head_53.ip_string = convert_for_printf(ad.last_ip);
                        head_53.email = ad.email;
                        head_53.ban_until = ad.ban_until_time;
                        XString repeat_53 = ad.memo;
                        send_vpacket<0x7953, 150, 1>(s, head_53, repeat_53);
                        goto x7954_out;
                    }
                }
                {
                    LOGIN_LOG("'ladmin': Attempt to obtain information (by the id) of an unknown account (id: %d, ip: %s)\n"_fmt,
                            account_id, ip);
                    head_53.account_name = stringish<AccountName>(""_s);
                    send_vpacket<0x7953, 150, 1>(s, head_53, ""_s);
                }
            x7954_out:
                break;
            }

            case 0x7955:       // Request to reload GM file (no answer)
            {
                Packet_Fixed<0x7955> fixed;
                rv = recv_fpacket<0x7955, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                LOGIN_LOG("'ladmin': Request to re-load GM configuration file (ip: %s).\n"_fmt,
                        ip);
                read_gm_account();
                // send GM accounts to all char-servers
                send_GM_accounts();
                break;
            }

            default:
            {
                {
                    timestamp_milliseconds_buffer timestr;
                    stamp_time(timestr);
                    FPRINTF(stderr,
                            "%s: receiving of an unknown packet -> disconnection\n"_fmt,
                            timestr);
                    FPRINTF(stderr,
                            "parse_admin: connection #%d (ip: %s), packet: 0x%x (with being read: %zu).\n"_fmt,
                            s, ip, packet_id, packet_avail(s));
                    FPRINTF(stderr, "Detail (in hex):\n"_fmt);
                    packet_dump(s);
                }
                LOGIN_LOG("'ladmin': End of connection, unknown packet (ip: %s)\n"_fmt,
                        ip);
                s->set_eof();
                PRINTF("Remote administration has been disconnected (unknown packet).\n"_fmt);
                return;
            }
        }
    }
    if (rv == RecvResult::Error)
        s->set_eof();
    return;
}

//--------------------------------------------
// Test to know if an IP come from LAN or WAN.
//--------------------------------------------
static
bool lan_ip_check(IP4Address p)
{
    bool lancheck = login_lan_conf.lan_subnet.covers(p);

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
    int result;

    IP4Address ip = s->client_ip;
    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(s, &packet_id))
    {
        if (login_conf.display_parse_login)
        {
            if (packet_id == 0x64)
            {
                // handled below to handle account name
            }
            else if (packet_id == 0x2710)
            {
                // handled below to handle server name
            }
            else
                PRINTF("parse_login: connection #%d, packet: 0x%x (with being read: %zu).\n"_fmt,
                        s, packet_id, packet_avail(s));
        }

        switch (packet_id)
        {
            case 0x64:         // Ask connection of a client
            {
                Packet_Fixed<0x0064> fixed;
                rv = recv_fpacket<0x0064, 55>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                // formerly at top of while
                {
                    AccountName account_name = fixed.account_name;
                    PRINTF("parse_login: connection #%d, packet: 0x%x (with being read: %zu), account: %s.\n"_fmt,
                            s, packet_id, packet_avail(s),
                            account_name);
                }

                account.userid = fixed.account_name;
                account.passwd = fixed.account_pass;
                account.passwdenc = 0;

                LOGIN_LOG("Request for connection (non encryption mode) of %s (ip: %s).\n"_fmt,
                        account.userid, ip);

                if (!check_ip(ip))
                {
                    LOGIN_LOG("Connection refused: IP isn't authorised (deny/allow, ip: %s).\n"_fmt,
                            ip);

                    Packet_Fixed<0x006a> fixed_6a;
                    fixed_6a.error_code = 0x03;
                    fixed_6a.error_message = {};
                    send_fpacket<0x006a, 23>(s, fixed_6a);
                    break;
                }

                result = mmo_auth(&account, s);
                if (result == -1)
                {
                    if (fixed.version < MIN_CLIENT_VERSION)
                        result = 5; // client too old
                }
                if (result == -1)
                {
                    GmLevel gm_level = isGM(account.account_id);
                    if (!(gm_level.satisfies(login_conf.min_level_to_connect)))
                    {
                        LOGIN_LOG("Connection refused: the minimum GM level for connection is %d (account: %s, GM level: %d, ip: %s).\n"_fmt,
                                login_conf.min_level_to_connect, account.userid,
                                gm_level, ip);
                        Packet_Fixed<0x0081> fixed_81;
                        fixed_81.error_code = 1; // 01 = Server closed
                        send_fpacket<0x0081, 3>(s, fixed_81);
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
                            if (login_conf.update_host)
                            {
                                send_packet_repeatonly<0x0063, 4, 1>(s, login_conf.update_host);
                            }
                        }

                        // Load list of char servers into outbound packet
                        std::vector<Packet_Repeat<0x0069>> repeat_69;
                        // if (version_2 & VERSION_2_SERVERORDER)
                        for (int i = 0; i < MAX_SERVERS; i++)
                        {
                            if (server_session[i])
                            {
                                Packet_Repeat<0x0069> info;
                                if (lan_ip_check(ip))
                                    info.ip = login_lan_conf.lan_char_ip;
                                else
                                    info.ip = server[i].ip;
                                info.port = server[i].port;
                                info.server_name = server[i].name;
                                info.users = server[i].users;
                                info.maintenance = 0; //maintenance;
                                info.is_new = 0; //is_new;
                                repeat_69.push_back(info);
                            }
                        }
                        // if at least 1 char-server
                        if (repeat_69.size())
                        {
                            Packet_Head<0x0069> head_69;
                            head_69.login_id1 = account.login_id1;
                            head_69.account_id = account.account_id;
                            head_69.login_id2 = account.login_id2;
                            head_69.unused = 0;    // in old version, that was for ip (not more used)
                            head_69.last_login_string = account.lastlogin;    // in old version, that was for name (not more used)
                            head_69.unused2 = 0;
                            head_69.sex = account.sex;
                            send_vpacket<0x0069, 47, 32>(s, head_69, repeat_69);

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
                            Packet_Fixed<0x0081> fixed_81;
                            fixed_81.error_code = 1; // 01 = Server closed
                            send_fpacket<0x0081, 3>(s, fixed_81);
                        }
                    }
                }
                else
                {
                    Packet_Fixed<0x006a> fixed_6a;
                    fixed_6a.error_code = result;
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
                                fixed_6a.error_message = tmpstr;
                            }
                            else
                            {   // we send error message
                                fixed_6a.error_message = ad->error_message;
                            }
                        }
                    }
                    send_fpacket<0x006a, 23>(s, fixed_6a);
                }
                break;
            }

            case 0x2710:       // Connection request of a char-server
            {
                Packet_Fixed<0x2710> fixed;
                rv = recv_fpacket<0x2710, 86>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                // formerly at top of while
                {
                    ServerName server_name = stringish<ServerName>(fixed.server_name);
                    PRINTF("parse_login: connection #%d, packet: 0x%x (with being read: %zu), server: %s.\n"_fmt,
                            s, packet_id, packet_avail(s),
                            server_name);
                }

                {
                    // TODO: this is exceptionally silly. Fix it.
                    account.userid = stringish<AccountName>(fixed.account_name.to_print());
                    account.passwd = stringish<AccountPass>(fixed.account_pass.to_print());
                    account.passwdenc = 0;
                    ServerName server_name = stringish<ServerName>(fixed.server_name.to_print());
                    LOGIN_LOG("Connection request of the char-server '%s' @ %s:%d (ip: %s)\n"_fmt,
                            server_name, fixed.ip, fixed.port, ip);
                    if (account.userid == login_conf.userid && account.passwd == login_conf.passwd)
                    {
                        // If this is the main server, and we don't already have a main server
                        if (!server_session[0]
                            && server_name == login_conf.main_server)
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
                        server[unwrap<AccountId>(account.account_id)].ip = fixed.ip;
                        server[unwrap<AccountId>(account.account_id)].port = fixed.port;
                        server[unwrap<AccountId>(account.account_id)].name = server_name;
                        server[unwrap<AccountId>(account.account_id)].users = 0;
                        //maintenance = RFIFOW(fd, 82);
                        //is_new = RFIFOW(fd, 84);
                        server_session[unwrap<AccountId>(account.account_id)] = s;
                        if (login_conf.anti_freeze_enable)
                            server_freezeflag[unwrap<AccountId>(account.account_id)] = 5;  // Char-server anti-freeze system. Counter. 5 ok, 4...0 freezed

                        Packet_Fixed<0x2711> fixed_11;
                        fixed_11.code = 0;
                        send_fpacket<0x2711, 3>(s, fixed_11);

                        s->set_parsers(SessionParsers{.func_parse= parse_fromchar, .func_delete= delete_fromchar});
                        realloc_fifo(s, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

                        // send GM account to char-server
                        send_GM_accounts(s);
                        goto x2710_done;
                    }
                    {
                    x2710_refused:
                        LOGIN_LOG("Connexion of the char-server '%s' REFUSED (account: %s, pass: %s, ip: %s)\n"_fmt,
                                server_name, account.userid,
                                account.passwd, ip);
                        Packet_Fixed<0x2711> fixed_11;
                        fixed_11.code = 3;
                        send_fpacket<0x2711, 3>(s, fixed_11);
                    }
                }
            x2710_done:
                // justification: we switching the packet parser
                parse_fromchar(s);
                return;
            }

            case 0x7530:       // Request of the server version
            {
                Packet_Fixed<0x7530> fixed;
                rv = recv_fpacket<0x7530, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                LOGIN_LOG("Sending of the server version (ip: %s)\n"_fmt,
                        ip);

                Packet_Fixed<0x7531> fixed_31;
                Version version = CURRENT_LOGIN_SERVER_VERSION;
                version.flags = login_conf.new_account ? 1 : 0;
                fixed_31.version = version;
                send_fpacket<0x7531, 10>(s, fixed_31);
                break;
            }

            case 0x7532:       // Request to end connection
            {
                Packet_Fixed<0x7532> fixed;
                rv = recv_fpacket<0x7532, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                LOGIN_LOG("End of connection (ip: %s)\n"_fmt, ip);
                s->set_eof();
                return;
            }

            case 0x7918:       // Request for administation login
            {
                Packet_Fixed<0x7918> fixed;
                rv = recv_fpacket<0x7918, 28>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7919> fixed_19;
                fixed_19.error = 1;
                if (!check_ladminip(s->client_ip))
                {
                    LOGIN_LOG("'ladmin'-login: Connection in administration mode refused: IP isn't authorised (ladmin_allow, ip: %s).\n"_fmt,
                            ip);
                }
                else
                {
                    if (fixed.encryption_zero == 0)
                    {
                        // non encrypted password
                        AccountPass password = stringish<AccountPass>(fixed.account_pass.to_print());
                        // If remote administration is enabled and password sent by client matches password read from login server configuration file
                        if (login_conf.admin_state
                            && password == login_conf.admin_pass)
                        {
                            LOGIN_LOG("'ladmin'-login: Connection in administration mode accepted (non encrypted password: %s, ip: %s)\n"_fmt,
                                    password, ip);
                            PRINTF("Connection of a remote administration accepted (non encrypted password).\n"_fmt);
                            fixed_19.error = 0;
                            s->set_parsers(SessionParsers{.func_parse= parse_admin, .func_delete= delete_admin});
                        }
                        else if (!login_conf.admin_state)
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
                send_fpacket<0x7919, 3>(s, fixed_19);
                break;
            }

            default:
            {
                {
                    {
                        timestamp_milliseconds_buffer timestr;
                        stamp_time(timestr);
                        FPRINTF(stderr,
                                "%s: receiving of an unknown packet -> disconnection\n"_fmt,
                                timestr);
                        FPRINTF(stderr,
                                "parse_login: connection #%d (ip: %s), packet: 0x%x (with being read: %zu).\n"_fmt,
                                s, ip, packet_id,
                                packet_avail(s));
                        FPRINTF(stderr, "Detail (in hex):\n"_fmt);
                        packet_dump(s);
                    }
                }
                LOGIN_LOG("End of connection, unknown packet (ip: %s)\n"_fmt, ip);
                s->set_eof();
                return;
            }
        }
    }
    if (rv == RecvResult::Error)
        s->set_eof();
}

static
bool lan_check()
{
    // log the LAN configuration
    LOGIN_LOG("The LAN configuration of the server is set:\n"_fmt);
    LOGIN_LOG("- with LAN IP of char-server: %s.\n"_fmt, login_lan_conf.lan_char_ip);
    LOGIN_LOG("- with the sub-network of the char-server: %s.\n"_fmt,
            login_lan_conf.lan_subnet);

    // sub-network check of the char-server
    {
        PRINTF("LAN test of LAN IP of the char-server: "_fmt);
        if (!lan_ip_check(login_lan_conf.lan_char_ip))
        {
            PRINTF(SGR_BOLD SGR_RED "***ERROR: LAN IP of the char-server doesn't belong to the specified Sub-network"_fmt SGR_RESET "\n");
            LOGIN_LOG("***ERROR: LAN IP of the char-server doesn't belong to the specified Sub-network.\n"_fmt);
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

    if (login_conf.admin_state)
    {
        if (!login_conf.admin_pass)
        {
            PRINTF("***WARNING: Administrator password is void (admin_pass).\n"_fmt);
            rv = false;
        }
        else if (login_conf.admin_pass == stringish<AccountPass>("admin"_s))
        {
            PRINTF("***WARNING: You are using the default administrator password (admin_pass).\n"_fmt);
            PRINTF("            We highly recommend that you change it.\n"_fmt);
        }
    }

    if (login_conf.gm_account_filename_check_timer.count() < 0)
    {
        PRINTF("***WARNING: Invalid value for gm_account_filename_check_timer parameter.\n"_fmt);
        PRINTF("            -> set to 15 sec (default).\n"_fmt);
        login_conf.gm_account_filename_check_timer = 15_s;
        rv = false;
    }
    else if (login_conf.gm_account_filename_check_timer == 1_s)
    {
        PRINTF("***WARNING: Invalid value for gm_account_filename_check_timer parameter.\n"_fmt);
        PRINTF("            -> set to 2 sec (minimum value).\n"_fmt);
        login_conf.gm_account_filename_check_timer = 2_s;
        rv = false;
    }

    if (login_conf.order == ACO::DENY_ALLOW)
    {
        if (login_conf.deny.size() == 1 && login_conf.deny.front().mask() == IP4Address())
        {
            PRINTF("***WARNING: The IP security order is 'deny,allow' (allow if not deny).\n"_fmt);
            PRINTF("            And you refuse ALL IP.\n"_fmt);
            rv = false;
        }
    }
    else if (login_conf.order == ACO::ALLOW_DENY)
    {
        if (login_conf.allow.empty())
        {
            PRINTF("***WARNING: The IP security order is 'allow,deny' (deny if not allow).\n"_fmt);
            PRINTF("            But, NO IP IS AUTHORISED!\n"_fmt);
            rv = false;
        }
    }
    else
    {
        // ACO::MUTUAL_FAILURE
        if (login_conf.allow.empty())
        {
            PRINTF("***WARNING: The IP security order is 'mutual-failture'\n"_fmt);
            PRINTF("            (allow if in the allow list and not in the deny list).\n"_fmt);
            PRINTF("            But, NO IP IS AUTHORISED!\n"_fmt);
            rv = false;
        }
        else if (login_conf.deny.size() == 1 && login_conf.deny.front().mask() == IP4Address())
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

    if (!login_conf.admin_state)
        LOGIN_LOG("- with no remote administration.\n"_fmt);
    else if (!login_conf.admin_pass)
        LOGIN_LOG("- with a remote administration with a VOID password.\n"_fmt);
    else if (login_conf.admin_pass == stringish<AccountPass>("admin"_s))
        LOGIN_LOG("- with a remote administration with the DEFAULT password.\n"_fmt);
    else
        LOGIN_LOG("- with a remote administration with the password of %zu character(s).\n"_fmt,
                login_conf.admin_pass.size());
    if (login_conf.ladminallowip.empty()
        || (login_conf.ladminallowip.size() == 1 && login_conf.ladminallowip.front().mask() == IP4Address()))
    {
        LOGIN_LOG("- to accept any IP for remote administration\n"_fmt);
    }
    else
    {
        LOGIN_LOG("- to accept following IP for remote administration:\n"_fmt);
        for (const IP4Mask& ae : login_conf.ladminallowip)
            LOGIN_LOG("  %s\n"_fmt, ae);
    }

    if (login_conf.new_account)
        LOGIN_LOG("- to ALLOW new users (with _F/_M).\n"_fmt);
    else
        LOGIN_LOG("- to NOT ALLOW new users (with _F/_M).\n"_fmt);
    LOGIN_LOG("- with port: %d.\n"_fmt, login_conf.login_port);
    LOGIN_LOG("- with the accounts file name: '%s'.\n"_fmt,
            login_conf.account_filename);
    LOGIN_LOG("- with the GM accounts file name: '%s'.\n"_fmt,
            login_conf.gm_account_filename);
    if (login_conf.gm_account_filename_check_timer == interval_t::zero())
        LOGIN_LOG("- to NOT check GM accounts file modifications.\n"_fmt);
    else
        LOGIN_LOG("- to check GM accounts file modifications every %lld seconds.\n"_fmt,
                maybe_cast<long long>(login_conf.gm_account_filename_check_timer.count()));

    // not necessary to log the 'login_log_filename', we are inside :)

    if (login_conf.display_parse_login)
        LOGIN_LOG("- to display normal parse packets on console.\n"_fmt);
    else
        LOGIN_LOG("- to NOT display normal parse packets on console.\n"_fmt);
    if (login_conf.display_parse_admin)
        LOGIN_LOG("- to display administration parse packets on console.\n"_fmt);
    else
        LOGIN_LOG("- to NOT display administration parse packets on console.\n"_fmt);
    if (login_conf.display_parse_fromchar)
        LOGIN_LOG("- to display char-server parse packets on console.\n"_fmt);
    else
        LOGIN_LOG("- to NOT display char-server parse packets on console.\n"_fmt);

    if (!login_conf.min_level_to_connect)  // 0: all players, 1-99 at least gm level x
        LOGIN_LOG("- with no minimum level for connection.\n"_fmt);
    else
        LOGIN_LOG("- to accept only GM with level %d or more.\n"_fmt,
                login_conf.min_level_to_connect);

    if (login_conf.order == ACO::DENY_ALLOW)
    {
        if (login_conf.deny.empty())
        {
            LOGIN_LOG("- with the IP security order: 'deny,allow' (allow if not deny). You refuse no IP.\n"_fmt);
        }
        else if (login_conf.deny.size() == 1 && login_conf.deny.front().mask() == IP4Address())
        {
            LOGIN_LOG("- with the IP security order: 'deny,allow' (allow if not deny). You refuse ALL IP.\n"_fmt);
        }
        else
        {
            LOGIN_LOG("- with the IP security order: 'deny,allow' (allow if not deny). Refused IP are:\n"_fmt);
            for (IP4Mask ae : login_conf.deny)
                LOGIN_LOG("  %s\n"_fmt, ae);
        }
    }
    else if (login_conf.order == ACO::ALLOW_DENY)
    {
        if (login_conf.allow.empty())
        {
            LOGIN_LOG("- with the IP security order: 'allow,deny' (deny if not allow). But, NO IP IS AUTHORISED!\n"_fmt);
        }
        else if (login_conf.allow.size() == 1 && login_conf.allow.front().mask() == IP4Address())
        {
            LOGIN_LOG("- with the IP security order: 'allow,deny' (deny if not allow). You authorise ALL IP.\n"_fmt);
        }
        else
        {
            LOGIN_LOG("- with the IP security order: 'allow,deny' (deny if not allow). Authorised IP are:\n"_fmt);
            for (IP4Mask ae : login_conf.allow)
                LOGIN_LOG("  %s\n"_fmt, ae);
        }
    }
    else
    {                           // ACO_MUTUAL_FAILTURE
        LOGIN_LOG("- with the IP security order: 'mutual-failture' (allow if in the allow list and not in the deny list).\n"_fmt);
        if (login_conf.allow.empty())
        {
            LOGIN_LOG("  But, NO IP IS AUTHORISED!\n"_fmt);
        }
        else if (login_conf.deny.size() == 1 && login_conf.deny.front().mask() == IP4Address())
        {
            LOGIN_LOG("  But, you refuse ALL IP!\n"_fmt);
        }
        else
        {
            if (login_conf.allow.size() == 1 && login_conf.allow.front().mask() == IP4Address())
            {
                LOGIN_LOG("  You authorise ALL IP.\n"_fmt);
            }
            else
            {
                LOGIN_LOG("  Authorised IP are:\n"_fmt);
                for (IP4Mask ae : login_conf.allow)
                    LOGIN_LOG("    %s\n"_fmt, ae);
            }
            LOGIN_LOG("  Refused IP are:\n"_fmt);
            for (IP4Mask ae : login_conf.deny)
                LOGIN_LOG("    %s\n"_fmt, ae);
        }
    }
}

static
bool login_config(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    return parse_login_conf(login_conf, key, value);
}

static
bool login_lan_config(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    return parse_login_lan_conf(login_lan_conf, key, value);
}

static
bool login_confs(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    if (key.data == "login_conf"_s)
    {
        return load_config_file(value.data, login_config);
    }
    if (key.data == "login_lan_conf"_s)
    {
        return load_config_file(value.data, login_lan_config);
    }
    key.span.error("Unknown meta-key for login server"_s);
    return false;
}
} // namespace login

//--------------------------------------
// Function called at exit of the server
//--------------------------------------
void term_func(void)
{
    login::mmo_auth_sync();

    login::auth_data.clear();
    login::gm_account_db.clear();
    for (int i = 0; i < login::MAX_SERVERS; i++)
    {
        Session *s = login::server_session[i];
        if (s)
            delete_session(s);
    }
    delete_session(login::login_session);

    LOGIN_LOG("----End of login-server (normal end with closing of all files).\n"_fmt);
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
            runflag &= load_config_file(argvi, login::login_confs);
        }
    }

    if (!loaded_config_yet)
        runflag &= load_config_file("conf/tmwa-login.conf"_s, login::login_confs);

    // not in login_config_read, because we can use 'import' option, and display same message twice or more
    // (why is that bad?)
    runflag &= login::display_conf_warnings();
    // not before, because log file name can be changed
    // (that doesn't stop the char-server though)
    login::save_config_in_log();
    runflag &= login::lan_check();

    for (int i = 0; i < login::AUTH_FIFO_SIZE; i++)
        login::auth_fifo[i].delflag = 1;
    for (int i = 0; i < login::MAX_SERVERS; i++)
        login::server_session[i] = nullptr;

    login::read_gm_account();
    login::mmo_auth_init();
//     set_termfunc (mmo_auth_sync);
    login::login_session = make_listen_port(login::login_conf.login_port, SessionParsers{.func_parse= login::parse_login, .func_delete= login::delete_login});


    Timer(gettick() + 5_min,
            login::check_auth_sync,
            5_min
    ).detach();

    if (login::login_conf.anti_freeze_enable > 0)
    {
        Timer(gettick() + 1_s,
                login::char_anti_freeze_system,
                login::login_conf.anti_freeze_interval
        ).detach();
    }

    // add timer to check GM accounts file modification
    std::chrono::seconds j = login::login_conf.gm_account_filename_check_timer;
    if (j == interval_t::zero())
        j = 1_min;
    Timer(gettick() + j,
            login::check_GM_file,
            j).detach();

    LOGIN_LOG("The login-server is ready (Server is listening on the port %d).\n"_fmt,
            login::login_conf.login_port);

    PRINTF("The login-server is " SGR_BOLD SGR_GREEN "ready" SGR_RESET " (Server is listening on the port %d).\n\n"_fmt,
            login::login_conf.login_port);

    return 0;
}
// namespace login ends before term_func and do_init
} // namespace tmwa
