#include "ladmin.hpp"
//    ladmin.cpp - Local administration tool.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 MadCamel
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

#include <netdb.h>
#include <unistd.h>

#include <cassert>

#include <algorithm>

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"
#include "../io/tty.hpp"
#include "../io/write.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/core.hpp"
#include "../mmo/human_time_diff.hpp"
#include "../mmo/ip.hpp"
#include "../mmo/mmo.hpp"
#include "../mmo/socket.hpp"
#include "../mmo/utils.hpp"
#include "../mmo/version.hpp"

#include "../poison.hpp"


static
int eathena_interactive_session;
#define Iprintf if (eathena_interactive_session) PRINTF

//-------------------------------INSTRUCTIONS------------------------------
// Set the variables below:
//   IP of the login server.
//   Port where the login-server listens incoming packets.
//   Password of administration (same of config_athena.conf).
// IMPORTANT:
//   Be sure that you authorize remote administration in login-server
//   (see login_athena.conf, 'admin_state' parameter)
//-------------------------------------------------------------------------
static
IP4Address login_ip = IP4_LOCALHOST;   // IP of login-server
static
int login_port = 6900;    // Port of login-server
static
AccountPass admin_pass = stringish<AccountPass>("admin"_s);    // Administration password
static
AString ladmin_log_filename = "log/ladmin.log"_s;
//-------------------------------------------------------------------------
//  LIST of COMMANDs that you can type at the prompt:
//    To use these commands you can only type only the first letters.
//    You must type a minimum of letters (you can not type 'a',
//      because ladmin doesn't know if it's for 'aide' or for 'add')
//    <Example> q <= quit, li <= list, pass <= password, etc.
//
//  Note: every time you must give a account_name, you can use "" or '' (spaces can be included)
//
//  help/?
//    Display the description of the commands
//  help/? [command]
//    Display the description of the specified command
//
//  add <account_name> <sex> <password>
//    Create an account with the default email (a@a.com).
//    Concerning the sex, only the first letter is used (F or M).
//    The e-mail is set to a@a.com (default e-mail). It's like to have no e-mail.
//    When the password is omitted, the input is done without displaying of the pressed keys.
//    <example> add testname Male testpass
//
//  ban yyyy/mm/dd hh:mm:ss <account name>
//    Changes the final date of a banishment of an account.
//    Like banset, but <account name> is at end.
//
//  banadd <account_name> <modifier>
//    Adds or substracts time from the final date of a banishment of an account.
//    Modifier is done as follows:
//      Adjustment value (-1, 1, +1, etc...)
//      Modified element:
//        a or y: year
//        m:  month
//        j or d: day
//        h:  hour
//        mn: minute
//        s:  second
//    <example> banadd testname +1m-2mn1s-6y
//              this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
//  NOTE: If you modify the final date of a non-banished account,
//        you fix the final date to (actual time +- adjustments)
//
//  banset <account_name> yyyy/mm/dd [hh:mm:ss]
//    Changes the final date of a banishment of an account.
//    Default time [hh:mm:ss]: 23:59:59.
//  banset <account_name> 0
//    Set a non-banished account (0 = unbanished).
//
//  block <account name>
//    Set state 5 (You have been blocked by the GM Team) to an account.
//    Like state <account name> 5.
//
//  check <account_name> <password>
//    Check the validity of a password for an account
//    NOTE: Server will never sends back a password.
//          It's the only method you have to know if a password is correct.
//          The other method is to have a ('physical') access to the accounts file.
//
//  create <account_name> <sex> <email> <password>
//    Like the 'add' command, but with e-mail moreover.
//    <example> create testname Male my@mail.com testpass
//
//  delete <account name>
//    Remove an account.
//    This order requires confirmation. After confirmation, the account is deleted.
//
//  email <account_name> <email>
//    Modify the e-mail of an account.
//
//  getcount
//    Give the number of players online on all char-servers.
//
//  gm <account_name> [GM_level]
//    Modify the GM level of an account.
//    Default value remove GM level (GM level = 0).
//    <example> gm testname 80
//
//  id <account name>
//    Give the id of an account.
//
//  info <account_id>
//    Display complete information of an account.
//
//  kami <message>
//    Sends a broadcast message on all map-server (in yellow).
//  kamib <message>
//    Sends a broadcast message on all map-server (in blue).
//
//  list [start_id [end_id]]
//    Display a list of accounts.
//    'start_id', 'end_id': indicate end and start identifiers.
//    Research by name is not possible with this command.
//    <example> list 10 9999999
//
//  listban [start_id [end_id]]
//    Like list/ls, but only for accounts with state or banished
//
//  listgm [start_id [end_id]]
//    Like list/ls, but only for GM accounts
//
//  listok [start_id [end_id]]
//    Like list/ls, but only for accounts without state and not banished
//
//  memo <account_name> <memo>
//    Modify the memo of an account.
//    'memo': it can have until 253 characters (with spaces or not).
//
//  name <account_id>
//    Give the name of an account.
//
//  password <account_name> <new_password>
//    Change the password of an account.
//    When new password is omitted, the input is done without displaying of the pressed keys.
//
//  quit/end/exit
//    End of the program of administration
//
//  reloadgm
//    Reload GM configuration file
//
//  search <expression>
//    Seek accounts.
//    Displays the accounts whose names correspond.
//  search -r/-e/--expr/--regex <expression>
//    Seek accounts by regular expression.
//    Displays the accounts whose names correspond.
//
//  sex <account_name> <sex>
//    Modify the sex of an account.
//    <example> sex testname Male
//
//  state <account_name> <new_state> <error_message_#7>
//    Change the state of an account.
//    'new_state': state is the state of the packet 0x006a + 1. The possibilities are:
//                 0 = Account ok            6 = Your Game's EXE file is not the latest version
//                 1 = Unregistered ID       7 = You are Prohibited to log in until %s
//                 2 = Incorrect Password    8 = Server is jammed due to over populated
//                 3 = This ID is expired    9 = No MSG
//                 4 = Rejected from Server  100 = This ID has been totally erased
//                 5 = You have been blocked by the GM Team
//                 all other values are 'No MSG', then use state 9 please.
//    'error_message_#7': message of the code error 6 = Your are Prohibited to log in until %s (packet 0x006a)
//
//  timeadd <account_name> <modifier>
//    Adds or substracts time from the validity limit of an account.
//    Modifier is done as follows:
//      Adjustment value (-1, 1, +1, etc...)
//      Modified element:
//        a or y: year
//        m:  month
//        j or d: day
//        h:  hour
//        mn: minute
//        s:  second
//    <example> timeadd testname +1m-2mn1s-6y
//              this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
//  NOTE: You can not modify a unlimited validity limit.
//        If you want modify it, you want probably create a limited validity limit.
//        So, at first, you must set the validity limit to a date/time.
//
//  timeset <account_name> yyyy/mm/dd [hh:mm:ss]
//    Changes the validity limit of an account.
//    Default time [hh:mm:ss]: 23:59:59.
//  timeset <account_name> 0
//    Gives an unlimited validity limit (0 = unlimited).
//
//  unban/unbanish <account name>
//    Unban an account.
//    Like banset <account name> 0.
//
//  unblock <account name>
//    Set state 0 (Account ok) to an account.
//    Like state <account name> 0.
//
//  version
//    Display the version of the login-server.
//
//  who <account name>
//    Displays complete information of an account.
//
//-------------------------------------------------------------------------
static
Session *login_session;
static
int bytes_to_read = 0;         // flag to know if we waiting bytes from login-server
static
TString parameters; // needs to be global since it's passed to the parse function
// really should be added to session data
static
AccountId list_first, list_last;
static
int list_type, list_count;  // parameter to display a list of accounts
static
int already_exit_function = 0; // sometimes, the exit function is called twice... so, don't log twice the message

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
void SessionDeleter::operator()(SessionData *)
{
    assert(false && "ladmin does not have sessions"_s);
}
#pragma GCC diagnostic pop

//------------------------------
// Writing function of logs file
//------------------------------
#define LADMIN_LOG(fmt, ...)    \
    ladmin_log(STRPRINTF(fmt, ## __VA_ARGS__))
static
void ladmin_log(XString line)
{
    io::AppendFile logfp(ladmin_log_filename);
    if (!logfp.is_open())
        return;
    log_with_timestamp(logfp, line);
}

static __attribute__((noreturn))
void delete_fromlogin(Session *)
{
    {
        PRINTF("Impossible to have a connection with the login-server [%s:%d] !\n"_fmt,
                login_ip, login_port);
        LADMIN_LOG("Impossible to have a connection with the login-server [%s:%d] !\n"_fmt,
                login_ip, login_port);
        exit(0);
    }
}

static
bool qsplit(ZString src)
{
    return !src;
}

template<class F, class... R>
static
bool qsplit(ZString src, F first, R... rest)
{
    if (!src)
        return false;
    XString one;
    if (src.startswith('\''))
    {
        src = src.xslice_t(1);
        auto it = std::find(src.begin(), src.end(), '\'');
        if (it == src.end())
            return false;
        one = src.xislice_h(it);
        src = src.xislice_t(it + 1);
        while (src.startswith(' '))
            src = src.xslice_t(1);
    }
    else if (src.startswith('"'))
    {
        src = src.xslice_t(1);
        auto it = std::find(src.begin(), src.end(), '"');
        if (it == src.end())
            return false;
        one = src.xislice_h(it);
        src = src.xislice_t(it + 1);
        while (src.startswith(' '))
            src = src.xslice_t(1);
    }
    else
    {
        auto it = std::find(src.begin(), src.end(), ' ');
        one = src.xislice_h(it);
        src = src.xislice_t(it);
        while (src.startswith(' '))
            src = src.xslice_t(1);
    }
    return extract(one, first) && qsplit(src, rest...);
}

//-----------------------------------------
// Sub-function: Display commands of ladmin
//-----------------------------------------
static
void display_help(ZString param)
{
    XString command = param.xislice_h(std::find(param.begin(), param.end(), ' '));

    if (command.startswith('?'))
        command = "help"_s;

    LADMIN_LOG("Displaying of the commands or a command.\n"_fmt);

    if (command == "help"_s)
    {
        PRINTF("help/?\n"_fmt);
        PRINTF("  Display the description of the commands\n"_fmt);
        PRINTF("help/? [command]\n"_fmt);
        PRINTF("  Display the description of the specified command\n"_fmt);
    }
    else if (command == "add"_s)
    {
        PRINTF("add <account_name> <sex> <password>\n"_fmt);
        PRINTF("  Create an account with the default email (a@a.com).\n"_fmt);
        PRINTF("  Concerning the sex, only the first letter is used (F or M).\n"_fmt);
        PRINTF("  The e-mail is set to a@a.com (default e-mail). It's like to have no e-mail.\n"_fmt);
        PRINTF("  When the password is omitted,\n"_fmt);
        PRINTF("  the input is done without displaying of the pressed keys.\n"_fmt);
        PRINTF("  <example> add testname Male testpass\n"_fmt);
    }
    else if (command == "ban"_s)
    {
        PRINTF("ban yyyy/mm/dd hh:mm:ss <account name>\n"_fmt);
        PRINTF("  Changes the final date of a banishment of an account.\n"_fmt);
        PRINTF("  Like banset, but <account name> is at end.\n"_fmt);
    }
    else if (command == "banadd"_s)
    {
        PRINTF("banadd <account_name> <modifier>\n"_fmt);
        PRINTF("  Adds or substracts time from the final date of a banishment of an account.\n"_fmt);
        PRINTF("  Modifier is done as follows:\n"_fmt);
        PRINTF("    Adjustment value (-1, 1, +1, etc...)\n"_fmt);
        PRINTF("    Modified element:\n"_fmt);
        PRINTF("      a or y: year\n"_fmt);
        PRINTF("      m:  month\n"_fmt);
        PRINTF("      j or d: day\n"_fmt);
        PRINTF("      h:  hour\n"_fmt);
        PRINTF("      mn: minute\n"_fmt);
        PRINTF("      s:  second\n"_fmt);
        PRINTF("  <example> banadd testname +1m-2mn1s-6y\n"_fmt);
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n"_fmt);
        PRINTF("            and 6 years at the same time.\n"_fmt);
        PRINTF("NOTE: If you modify the final date of a non-banished account,\n"_fmt);
        PRINTF("      you fix the final date to (actual time +- adjustments)\n"_fmt);
    }
    else if (command == "banset"_s)
    {
        PRINTF("banset <account_name> yyyy/mm/dd [hh:mm:ss]\n"_fmt);
        PRINTF("  Changes the final date of a banishment of an account.\n"_fmt);
        PRINTF("  Default time [hh:mm:ss]: 23:59:59.\n"_fmt);
        PRINTF("banset <account_name> 0\n"_fmt);
        PRINTF("  Set a non-banished account (0 = unbanished).\n"_fmt);
    }
    else if (command == "block"_s)
    {
        PRINTF("block <account name>\n"_fmt);
        PRINTF("  Set state 5 (You have been blocked by the GM Team) to an account.\n"_fmt);
        PRINTF("  This command works like state <account_name> 5.\n"_fmt);
    }
    else if (command == "check"_s)
    {
        PRINTF("check <account_name> <password>\n"_fmt);
        PRINTF("  Check the validity of a password for an account.\n"_fmt);
        PRINTF("  NOTE: Server will never sends back a password.\n"_fmt);
        PRINTF("        It's the only method you have to know if a password is correct.\n"_fmt);
        PRINTF("        The other method is to have a ('physical') access to the accounts file.\n"_fmt);
    }
    else if (command == "create"_s)
    {
        PRINTF("create <account_name> <sex> <email> <password>\n"_fmt);
        PRINTF("  Like the 'add' command, but with e-mail moreover.\n"_fmt);
        PRINTF("  <example> create testname Male my@mail.com testpass\n"_fmt);
    }
    else if (command == "delete"_s)
    {
        PRINTF("delete <account name>\n"_fmt);
        PRINTF("  Remove an account.\n"_fmt);
        PRINTF("  This order requires confirmation. After confirmation, the account is deleted.\n"_fmt);
    }
    else if (command == "email"_s)
    {
        PRINTF("email <account_name> <email>\n"_fmt);
        PRINTF("  Modify the e-mail of an account.\n"_fmt);
    }
    else if (command == "getcount"_s)
    {
        PRINTF("getcount\n"_fmt);
        PRINTF("  Give the number of players online on all char-servers.\n"_fmt);
    }
    else if (command == "gm"_s)
    {
        PRINTF("gm <account_name> [GM_level]\n"_fmt);
        PRINTF("  Modify the GM level of an account.\n"_fmt);
        PRINTF("  Default value remove GM level (GM level = 0).\n"_fmt);
        PRINTF("  <example> gm testname 80\n"_fmt);
    }
    else if (command == "id"_s)
    {
        PRINTF("id <account name>\n"_fmt);
        PRINTF("  Give the id of an account.\n"_fmt);
    }
    else if (command == "info"_s)
    {
        PRINTF("info <account_id>\n"_fmt);
        PRINTF("  Display complete information of an account.\n"_fmt);
    }
    else if (command == "kami"_s)
    {
        PRINTF("kami <message>\n"_fmt);
        PRINTF("  Sends a broadcast message on all map-server (in yellow).\n"_fmt);
    }
    else if (command == "kamib"_s)
    {
        PRINTF("kamib <message>\n"_fmt);
        PRINTF("  Sends a broadcast message on all map-server (in blue).\n"_fmt);
    }
    else if (command == "list"_s)
    {
        PRINTF("list/ls [start_id [end_id]]\n"_fmt);
        PRINTF("  Display a list of accounts.\n"_fmt);
        PRINTF("  'start_id', 'end_id': indicate end and start identifiers.\n"_fmt);
        PRINTF("  Research by name is not possible with this command.\n"_fmt);
        PRINTF("  <example> list 10 9999999\n"_fmt);
    }
    else if (command == "itemfrob"_s)
    {
        PRINTF("itemfrob <source-id> <dest-id>\n"_fmt);
        PRINTF("  Translates item IDs for all accounts.\n"_fmt);
        PRINTF("  Any items matching the source item ID will be mapped to the dest-id.\n"_fmt);
        PRINTF("  <example> itemfrob 500 700\n"_fmt);
    }
    else if (command == "listban"_s)
    {
        PRINTF("listban [start_id [end_id]]\n"_fmt);
        PRINTF("  Like list/ls, but only for accounts with state or banished.\n"_fmt);
    }
    else if (command == "listgm"_s)
    {
        PRINTF("listgm [start_id [end_id]]\n"_fmt);
        PRINTF("  Like list/ls, but only for GM accounts.\n"_fmt);
    }
    else if (command == "listok"_s)
    {
        PRINTF("listok [start_id [end_id]]\n"_fmt);
        PRINTF("  Like list/ls, but only for accounts without state and not banished.\n"_fmt);
    }
    else if (command == "memo"_s)
    {
        PRINTF("memo <account_name> <memo>\n"_fmt);
        PRINTF("  Modify the memo of an account.\n"_fmt);
        PRINTF("  'memo': it can have until 253 characters (with spaces or not).\n"_fmt);
    }
    else if (command == "name"_s)
    {
        PRINTF("name <account_id>\n"_fmt);
        PRINTF("  Give the name of an account.\n"_fmt);
    }
    else if (command == "password"_s)
    {
        PRINTF("password <account_name> <new_password>\n"_fmt);
        PRINTF("  Change the password of an account.\n"_fmt);
        PRINTF("  When new password is omitted,\n"_fmt);
        PRINTF("  the input is done without displaying of the pressed keys.\n"_fmt);
    }
    else if (command == "reloadgm"_s)
    {
        PRINTF("reloadGM\n"_fmt);
        PRINTF("  Reload GM configuration file\n"_fmt);
    }
    else if (command == "search"_s)
    {
        PRINTF("search <expression>\n"_fmt);
        PRINTF("  Seek accounts.\n"_fmt);
        PRINTF("  Displays the accounts whose names correspond.\n"_fmt);
    }
    else if (command == "sex"_s)
    {
        PRINTF("sex <account_name> <sex>\n"_fmt);
        PRINTF("  Modify the sex of an account.\n"_fmt);
        PRINTF("  <example> sex testname Male\n"_fmt);
    }
    else if (command == "state"_s)
    {
        PRINTF("state <account_name> <new_state> <error_message_#7>\n"_fmt);
        PRINTF("  Change the state of an account.\n"_fmt);
        PRINTF("  'new_state': state is the state of the packet 0x006a + 1.\n"_fmt);
        PRINTF("               The possibilities are:\n"_fmt);
        PRINTF("               0 = Account ok\n"_fmt);
        PRINTF("               1 = Unregistered ID\n"_fmt);
        PRINTF("               2 = Incorrect Password\n"_fmt);
        PRINTF("               3 = This ID is expired\n"_fmt);
        PRINTF("               4 = Rejected from Server\n"_fmt);
        PRINTF("               5 = You have been blocked by the GM Team\n"_fmt);
        PRINTF("               6 = Your Game's EXE file is not the latest version\n"_fmt);
        PRINTF("               7 = You are Prohibited to log in until...\n"_fmt);
        PRINTF("               8 = Server is jammed due to over populated\n"_fmt);
        PRINTF("               9 = No MSG\n"_fmt);
        PRINTF("               100 = This ID has been totally erased\n"_fmt);
        PRINTF("               all other values are 'No MSG', then use state 9 please.\n"_fmt);
        PRINTF("  'error_message_#7': message of the code error 6\n"_fmt);
        PRINTF("                      = Your are Prohibited to log in until... (packet 0x006a)\n"_fmt);
    }
    else if (command == "timeadd"_s)
    {
        PRINTF("timeadd <account_name> <modifier>\n"_fmt);
        PRINTF("  Adds or substracts time from the validity limit of an account.\n"_fmt);
        PRINTF("  Modifier is done as follows:\n"_fmt);
        PRINTF("    Adjustment value (-1, 1, +1, etc...)\n"_fmt);
        PRINTF("    Modified element:\n"_fmt);
        PRINTF("      a or y: year\n"_fmt);
        PRINTF("      m:  month\n"_fmt);
        PRINTF("      j or d: day\n"_fmt);
        PRINTF("      h:  hour\n"_fmt);
        PRINTF("      mn: minute\n"_fmt);
        PRINTF("      s:  second\n"_fmt);
        PRINTF("  <example> timeadd testname +1m-2mn1s-6y\n"_fmt);
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n"_fmt);
        PRINTF("            and 6 years at the same time.\n"_fmt);
        PRINTF("NOTE: You can not modify a unlimited validity limit.\n"_fmt);
        PRINTF("      If you want modify it, you want probably create a limited validity limit.\n"_fmt);
        PRINTF("      So, at first, you must set the validity limit to a date/time.\n"_fmt);
    }
    else if (command == "timeadd"_s)
    {
        PRINTF("timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n"_fmt);
        PRINTF("  Changes the validity limit of an account.\n"_fmt);
        PRINTF("  Default time [hh:mm:ss]: 23:59:59.\n"_fmt);
        PRINTF("timeset <account_name> 0\n"_fmt);
        PRINTF("  Gives an unlimited validity limit (0 = unlimited).\n"_fmt);
    }
    else if (command == "unban"_s)
    {
        PRINTF("unban/unbanish <account name>\n"_fmt);
        PRINTF("  Remove the banishment of an account.\n"_fmt);
        PRINTF("  This command works like banset <account_name> 0.\n"_fmt);
    }
    else if (command == "unblock"_s)
    {
        PRINTF("unblock <account name>\n"_fmt);
        PRINTF("  Set state 0 (Account ok) to an account.\n"_fmt);
        PRINTF("  This command works like state <account_name> 0.\n"_fmt);
    }
    else if (command == "version"_s)
    {
        PRINTF("version\n"_fmt);
        PRINTF("  Display the version of the login-server.\n"_fmt);
    }
    else if (command == "who"_s)
    {
        PRINTF("who <account name>\n"_fmt);
        PRINTF("  Displays complete information of an account.\n"_fmt);
    }
    else if (command == "quit"_s
        || command == "exit"_s
        || command == "end"_s)
    {
        PRINTF("quit/end/exit\n"_fmt);
        PRINTF("  End of the program of administration.\n"_fmt);
    }
    else
    {
        if (command)
            PRINTF("Unknown command [%s] for help. Displaying of all commands.\n"_fmt,
                 AString(command));
        PRINTF(" help/?                          -- Display this help\n"_fmt);
        PRINTF(" help/? [command]                -- Display the help of the command\n"_fmt);
        PRINTF(" add <account_name> <sex> <password>  -- Create an account with default email\n"_fmt);
        PRINTF(" ban yyyy/mm/dd hh:mm:ss <account name> -- Change final date of a ban\n"_fmt);
        PRINTF(" banadd <account_name> <modifier>     -- Add or substract time from the final\n"_fmt);
        PRINTF("   example: ba apple +1m-2mn1s-2y        date of a banishment of an account\n"_fmt);
        PRINTF(" banset <account_name> yyyy/mm/dd [hh:mm:ss] -- Change final date of a ban\n"_fmt);
        PRINTF(" banset <account_name> 0              -- Un-banish an account\n"_fmt);
        PRINTF(" block <account name>     -- Set state 5 (blocked by the GM Team) to an account\n"_fmt);
        PRINTF(" check <account_name> <password>      -- Check the validity of a password\n"_fmt);
        PRINTF(" create <account_name> <sex> <email> <passwrd> -- Create an account with email\n"_fmt);
        PRINTF(" delete <account name>                -- Remove an account\n"_fmt);
        PRINTF(" email <account_name> <email>         -- Modify an email of an account\n"_fmt);
        PRINTF(" getcount                             -- Give the number of players online\n"_fmt);
        PRINTF(" gm <account_name> [GM_level]         -- Modify the GM level of an account\n"_fmt);
        PRINTF(" id <account name>                    -- Give the id of an account\n"_fmt);
        PRINTF(" info <account_id>                    -- Display all information of an account\n"_fmt);
        PRINTF(" itemfrob <source-id> <dest-id>       -- Map all items from one item ID to another\n"_fmt);
        PRINTF(" kami <message>                       -- Sends a broadcast message (in yellow)\n"_fmt);
        PRINTF(" kamib <message>                      -- Sends a broadcast message (in blue)\n"_fmt);
        PRINTF(" list [First_id [Last_id]]            -- Display a list of accounts\n"_fmt);
        PRINTF(" listban [First_id [Last_id] ]        -- Display a list of accounts\n"_fmt);
        PRINTF("                                         with state or banished\n"_fmt);
        PRINTF(" listgm [First_id [Last_id]]          -- Display a list of GM accounts\n"_fmt);
        PRINTF(" listok [First_id [Last_id] ]         -- Display a list of accounts\n"_fmt);
        PRINTF("                                         without state and not banished\n"_fmt);
        PRINTF(" memo <account_name> <memo>           -- Modify the memo of an account\n"_fmt);
        PRINTF(" name <account_id>                    -- Give the name of an account\n"_fmt);
        PRINTF(" password <account_name> <new_password> -- Change the password of an account\n"_fmt);
        PRINTF(" quit/end/exit                        -- End of the program of administation\n"_fmt);
        PRINTF(" reloadGM                             -- Reload GM configuration file\n"_fmt);
        PRINTF(" search <expression>                  -- Seek accounts\n"_fmt);
        PRINTF(" sex <nomcompte> <sexe>               -- Modify the sex of an account\n"_fmt);
        PRINTF(" state <account_name> <new_state> <error_message_#7> -- Change the state\n"_fmt);
        PRINTF(" timeadd <account_name> <modifier>    -- Add or substract time from the\n"_fmt);
        PRINTF("   example: ta apple +1m-2mn1s-2y        validity limit of an account\n"_fmt);
        PRINTF(" timeset <account_name> yyyy/mm/dd [hh:mm:ss] -- Change the validify limit\n"_fmt);
        PRINTF(" timeset <account_name> 0             -- Give a unlimited validity limit\n"_fmt);
        PRINTF(" unban <account name>                 -- Remove the banishment of an account\n"_fmt);
        PRINTF(" unblock <account name>               -- Set state 0 (Account ok) to an account\n"_fmt);
        PRINTF(" version                              -- Gives the version of the login-server\n"_fmt);
        PRINTF(" who <account name>                   -- Display all information of an account\n"_fmt);
        PRINTF(" who <account name>                   -- Display all information of an account\n"_fmt);
        PRINTF(" Note: To use spaces in an account name, type \"<account name>\" (or ').\n"_fmt);
    }
}

//-----------------------------
// Sub-function: add an account
//-----------------------------
static
void addaccount(ZString param, int emailflag)
{
    AccountName name;
    VString<1> sex_;
    XString email_;
    AccountPass password;

    if (emailflag == 0)
    {
        // add command
        if (!qsplit(param, &name, &sex_, &password))
        {
            PRINTF("Please input an account name, a sex and a password.\n"_fmt);
            PRINTF("<example> add testname Male testpass\n"_fmt);
            LADMIN_LOG("Incomplete parameters to create an account ('add' command).\n"_fmt);
            return;
        }
        email_ = DEFAULT_EMAIL;
    }
    else
    {
        // 1: create command
        if (!qsplit(param, &name, &sex_, &email_, &password))
        {
            PRINTF("Please input an account name, a sex and a password.\n"_fmt);
            PRINTF("<example> create testname Male my@mail.com testpass\n"_fmt);
            LADMIN_LOG("Incomplete parameters to create an account ('create' command).\n"_fmt);
            return;
        }
    }
    char sex = sex_.front();
    if (!name.is_print())
        return;

    if (!"MF"_s.contains(sex))
    {
        PRINTF("Illegal gender [%c]. Please input M or F.\n"_fmt, sex);
        LADMIN_LOG("Illegal gender [%c]. Please input M or F.\n"_fmt, sex);
        return;
    }

    if (!e_mail_check(email_))
    {
        PRINTF("Invalid email [%s]. Please input a valid e-mail.\n"_fmt,
                AString(email_));
        LADMIN_LOG("Invalid email [%s]. Please input a valid e-mail.\n"_fmt,
                AString(email_));
        return;
    }
    AccountEmail email = stringish<AccountEmail>(email_);

    if (!password.is_print())
        return;

    LADMIN_LOG("Request to login-server to create an account.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7930;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFO_STRING(login_session, 26, password, 24);
    WFIFOB(login_session, 50) = sex;
    WFIFO_STRING(login_session, 51, email, 40);
    WFIFOSET(login_session, 91);
    bytes_to_read = 1;
}

//---------------------------------------------------------------------------------
// Sub-function: Add/substract time to the final date of a banishment of an account
//---------------------------------------------------------------------------------
static
void banaddaccount(ZString param)
{
    AccountName name;
    HumanTimeDiff modif {};

    if (!qsplit(param, &name, &modif))
    {
        PRINTF("Please input an account name and a modifier.\n"_fmt);
        PRINTF("  <example>: banadd testname +1m-2mn1s-6y\n"_fmt);
        PRINTF("             this example adds 1 month and 1 second, and substracts 2 minutes\n"_fmt);
        PRINTF("             and 6 years at the same time.\n"_fmt);
        LADMIN_LOG("Incomplete parameters to modify the ban date/time of an account ('banadd' command).\n"_fmt);
        return;
    }
    if (!name.is_print())
        return;

    if (!modif)
    {
        PRINTF("Please give an adjustment with this command:\n"_fmt);
        PRINTF("  Adjustment value (-1, 1, +1, etc...)\n"_fmt);
        PRINTF("  Modified element:\n"_fmt);
        PRINTF("    a or y: year\n"_fmt);
        PRINTF("    m: month\n"_fmt);
        PRINTF("    j or d: day\n"_fmt);
        PRINTF("    h: hour\n"_fmt);
        PRINTF("    mn: minute\n"_fmt);
        PRINTF("    s: second\n"_fmt);
        PRINTF("  <example> banadd testname +1m-2mn1s-6y\n"_fmt);
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n"_fmt);
        PRINTF("            and 6 years at the same time.\n"_fmt);
        LADMIN_LOG("No adjustment isn't an adjustment ('banadd' command).\n"_fmt);
        return;
    }

    LADMIN_LOG("Request to login-server to modify a ban date/time.\n"_fmt);

    WFIFOW(login_session, 0) = 0x794c;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFO_STRUCT(login_session, 26, modif);
    WFIFOSET(login_session, 38);
    bytes_to_read = 1;
}

//-----------------------------------------------------------------------
// Sub-function of sub-function banaccount, unbanaccount or bansetaccount
// Set the final date of a banishment of an account
//-----------------------------------------------------------------------
static
void bansetaccountsub(AccountName name, XString date, XString time_)
{
    int year, month, day, hour, minute, second;
    year = month = day = hour = minute = second = 0;

    // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
    TimeT ban_until_time = TimeT();
    struct tm tmtime = ban_until_time;   // initialize

    if (!name.is_print())
        return;

    if (date != "0"_s
        && ((!extract(date, record<'/'>(&year, &month, &day))
                && !extract(date, record<'-'>(&year, &month, &day))
                && !extract(date, record<'.'>(&year, &month, &day)))
            || !extract(time_, record<':'>(&hour, &minute, &second))))
    {
        PRINTF("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n"_fmt);
        PRINTF("You can imput 0 instead of if you use 'banset' command.\n"_fmt);
        LADMIN_LOG("Invalid format for the date/time ('banset' or 'ban' command).\n"_fmt);
        return;
    }

    if (date == "0"_s)
    {
        ban_until_time = TimeT();
    }
    else
    {
        if (year < 70)
        {
            year = year + 100;
        }
        if (year >= 1900)
        {
            year = year - 1900;
        }
        if (month < 1 || month > 12)
        {
            PRINTF("Please give a correct value for the month (from 1 to 12).\n"_fmt);
            LADMIN_LOG("Invalid month for the date ('banset' or 'ban' command).\n"_fmt);
            return;
        }
        month = month - 1;
        if (day < 1 || day > 31)
        {
            PRINTF("Please give a correct value for the day (from 1 to 31).\n"_fmt);
            LADMIN_LOG("Invalid day for the date ('banset' or 'ban' command).\n"_fmt);
            return;
        }
        if (((month == 3 || month == 5 || month == 8 || month == 10)
             && day > 30) || (month == 1 && day > 29))
        {
            PRINTF("Please give a correct value for a day of this month (%d).\n"_fmt,
                 month);
            LADMIN_LOG("Invalid day for this month ('banset' or 'ban' command).\n"_fmt);
            return;
        }
        if (hour < 0 || hour > 23)
        {
            PRINTF("Please give a correct value for the hour (from 0 to 23).\n"_fmt);
            LADMIN_LOG("Invalid hour for the time ('banset' or 'ban' command).\n"_fmt);
            return;
        }
        if (minute < 0 || minute > 59)
        {
            PRINTF("Please give a correct value for the minutes (from 0 to 59).\n"_fmt);
            LADMIN_LOG("Invalid minute for the time ('banset' or 'ban' command).\n"_fmt);
            return;
        }
        if (second < 0 || second > 59)
        {
            PRINTF("Please give a correct value for the seconds (from 0 to 59).\n"_fmt);
            LADMIN_LOG("Invalid second for the time ('banset' or 'ban' command).\n"_fmt);
            return;
        }
        tmtime.tm_year = year;
        tmtime.tm_mon = month;
        tmtime.tm_mday = day;
        tmtime.tm_hour = hour;
        tmtime.tm_min = minute;
        tmtime.tm_sec = second;
        tmtime.tm_isdst = -1;  // -1: no winter/summer time modification
        ban_until_time = tmtime;
        if (ban_until_time.error())
        {
            PRINTF("Invalid date.\n"_fmt);
            PRINTF("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n"_fmt);
            PRINTF("You can imput 0 instead of if you use 'banset' command.\n"_fmt);
            LADMIN_LOG("Invalid date. ('banset' or 'ban' command).\n"_fmt);
            return;
        }
    }

    LADMIN_LOG("Request to login-server to set a ban.\n"_fmt);

    WFIFOW(login_session, 0) = 0x794a;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOL(login_session, 26) = static_cast<time_t>(ban_until_time);
    WFIFOSET(login_session, 30);
    bytes_to_read = 1;
}

//---------------------------------------------------------------------
// Sub-function: Set the final date of a banishment of an account (ban)
//---------------------------------------------------------------------
static
void banaccount(ZString param)
{
    AccountName name;
    XString date;
    XString time_;

    if (!qsplit(param, &date, &time_, &name))
    {
        PRINTF("Please input an account name, a date and a hour.\n"_fmt);
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n"_fmt);
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n"_fmt);
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n"_fmt);
        PRINTF("           unban/unbanish <account name>\n"_fmt);
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n"_fmt);
        LADMIN_LOG("Incomplete parameters to set a ban ('banset' or 'ban' command).\n"_fmt);
        return;
    }

    if (!time_)
        time_ = "23:59:59"_s;

    bansetaccountsub(name, date, time_);
}

//------------------------------------------------------------------------
// Sub-function: Set the final date of a banishment of an account (banset)
//------------------------------------------------------------------------
static
void bansetaccount(ZString param)
{
    AccountName name;
    XString date;
    XString time_;

    if (!qsplit(param, &name, &date, &time_)
            && !qsplit(param, &name, &date))
    {
        PRINTF("Please input an account name, a date and a hour.\n"_fmt);
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n"_fmt);
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n"_fmt);
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n"_fmt);
        PRINTF("           unban/unbanish <account name>\n"_fmt);
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n"_fmt);
        LADMIN_LOG("Incomplete parameters to set a ban ('banset' or 'ban' command).\n"_fmt);
        return;
    }

    if (!time_)
        time_ = "23:59:59"_s;

    bansetaccountsub(name, date, time_);
}

//-------------------------------------------------
// Sub-function: unbanishment of an account (unban)
//-------------------------------------------------
static
void unbanaccount(ZString param)
{
    AccountName name;

    if (!qsplit(param, &name))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n"_fmt);
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n"_fmt);
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n"_fmt);
        PRINTF("           unban/unbanish <account name>\n"_fmt);
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n"_fmt);
        LADMIN_LOG("Incomplete parameters to set a ban ('unban' command).\n"_fmt);
        return;
    }

    bansetaccountsub(name, "0"_s, ""_s);
}

//---------------------------------------------------------
// Sub-function: Asking to check the validity of a password
// (Note: never send back a password with login-server!! security of passwords)
//---------------------------------------------------------
static
void checkaccount(ZString param)
{
    AccountName name;
    AccountPass password;

    if (!qsplit(param, &name, &password))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<example> check testname password\n"_fmt);
        LADMIN_LOG("Incomplete parameters to check the password of an account ('check' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
        return;

    if (!password.is_print())
        return;

    LADMIN_LOG("Request to login-server to check a password.\n"_fmt);

    WFIFOW(login_session, 0) = 0x793a;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFO_STRING(login_session, 26, password, 24);
    WFIFOSET(login_session, 50);
    bytes_to_read = 1;
}

//------------------------------------------------
// Sub-function: Asking for deletion of an account
//------------------------------------------------
static
void delaccount(ZString param)
{
    AccountName name;

    if (!qsplit(param, &name))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<example> delete testnametodelete\n"_fmt);
        LADMIN_LOG("No name given to delete an account ('delete' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
        return;

    char confirm;
    do
    {
        PRINTF(SGR_BOLD SGR_CYAN " ** Are you really sure to DELETE account [%s]? (y/n) > " SGR_RESET ""_fmt, name);
        fflush(stdout);
        int seek = getchar();
        confirm = seek;
        if (seek == EOF)
            confirm = 'n';
        else
            while (seek != '\n' && seek != EOF)
                seek = getchar();
    }
    while (!"yn"_s.contains(confirm));

    if (confirm == 'n')
    {
        PRINTF("Deletion canceled.\n"_fmt);
        LADMIN_LOG("Deletion canceled by user ('delete' command).\n"_fmt);
        return;
    }

    LADMIN_LOG("Request to login-server to delete an acount.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7932;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOSET(login_session, 26);
    bytes_to_read = 1;
}

//----------------------------------------------------------
// Sub-function: Asking to modification of an account e-mail
//----------------------------------------------------------
static
void changeemail(ZString param)
{
    AccountName name;
    XString email_;
    if (!qsplit(param, &name, &email_))
    {
        PRINTF("Please input an account name and an email.\n"_fmt);
        PRINTF("<example> email testname newemail\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the email of an account ('email' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
        return;

    if (!e_mail_check(email_))
    {
        PRINTF("Invalid email [%s]. Please input a valid e-mail.\n"_fmt,
                AString(email_));
        LADMIN_LOG("Invalid email [%s]. Please input a valid e-mail.\n"_fmt,
                AString(email_));
        return;
    }
    AccountEmail email = stringish<AccountEmail>(email_);

    LADMIN_LOG("Request to login-server to change an email.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7940;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFO_STRING(login_session, 26, email, 40);
    WFIFOSET(login_session, 66);
    bytes_to_read = 1;
}

//-----------------------------------------------------
// Sub-function: Asking of the number of online players
//-----------------------------------------------------
static
void getlogincount(void)
{
    LADMIN_LOG("Request to login-server to obtain the # of online players.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7938;
    WFIFOSET(login_session, 2);
    bytes_to_read = 1;
}

//----------------------------------------------------------
// Sub-function: Asking to modify the GM level of an account
//----------------------------------------------------------
static
void changegmlevel(ZString param)
{
    AccountName name;
    int GM_level = 0;

    if (!qsplit(param, &name, &GM_level))
    {
        PRINTF("Please input an account name and a GM level.\n"_fmt);
        PRINTF("<example> gm testname 80\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the GM level of an account ('gm' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
        return;

    if (GM_level < 0 || GM_level > 99)
    {
        PRINTF("Illegal GM level [%d]. Please input a value from 0 to 99.\n"_fmt,
             GM_level);
        LADMIN_LOG("Illegal GM level [%d]. The value can be from 0 to 99.\n"_fmt,
              GM_level);
        return;
    }

    LADMIN_LOG("Request to login-server to change a GM level.\n"_fmt);

    WFIFOW(login_session, 0) = 0x793e;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOB(login_session, 26) = GM_level;
    WFIFOSET(login_session, 27);
    bytes_to_read = 1;
}

//---------------------------------------------
// Sub-function: Asking to obtain an account id
//---------------------------------------------
static
void idaccount(ZString param)
{
    AccountName name;

    if (!qsplit(param, &name))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<example> id testname\n"_fmt);
        LADMIN_LOG("No name given to search an account id ('id' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
    {
        return;
    }

    LADMIN_LOG("Request to login-server to know an account id.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7944;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOSET(login_session, 26);
    bytes_to_read = 1;
}

//----------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its id)
//----------------------------------------------------------------------------
static
void infoaccount(AccountId account_id)
{
    LADMIN_LOG("Request to login-server to obtain information about an account (by its id).\n"_fmt);

    WFIFOW(login_session, 0) = 0x7954;
    WFIFOL(login_session, 2) = unwrap<AccountId>(account_id);
    WFIFOSET(login_session, 6);
    bytes_to_read = 1;
}

//---------------------------------------
// Sub-function: Send a broadcast message
//---------------------------------------
static
void sendbroadcast(ZString message)
{
    if (!message)
    {
        PRINTF("Please input a message.\n"_fmt);
        {
            PRINTF("<example> kami a message\n"_fmt);
        }
        LADMIN_LOG("The message is void ('kami' command).\n"_fmt);
        return;
    }

    WFIFOW(login_session, 0) = 0x794e;
    WFIFOW(login_session, 2) = 0;
    size_t len = message.size() + 1;
    WFIFOL(login_session, 4) = len;
    WFIFO_STRING(login_session, 8, message, len);
    WFIFOSET(login_session, 8 + len);
    bytes_to_read = 1;
}


//--------------------------------------------------------
// Sub-function: Asking to Displaying of the accounts list
//--------------------------------------------------------
static
void listaccount(ZString param, int type)
{
//int list_first, list_last, list_type; // parameter to display a list of accounts
    list_type = type;

    // set default values
    list_first = AccountId();
    list_last = AccountId();

    if (list_type == 1)
    {                           // if listgm
        // get all accounts = use default
    }
    else if (list_type == 2)
    {                           // if search
        // get all accounts = use default
    }
    else if (list_type == 3)
    {                           // if listban
        // get all accounts = use default
    }
    else if (list_type == 4)
    {                           // if listok
        // get all accounts = use default
    }
    else
    {
        // if list (list_type == 0)
        extract(param, record<' '>(&list_first, &list_last));
        if (list_last < list_first)
            list_last = AccountId();
    }

    LADMIN_LOG("Request to login-server to obtain the list of accounts from %d to %d.\n"_fmt,
          list_first, list_last);

    WFIFOW(login_session, 0) = 0x7920;
    WFIFOL(login_session, 2) = unwrap<AccountId>(list_first);
    WFIFOL(login_session, 6) = unwrap<AccountId>(list_last);
    WFIFOSET(login_session, 10);
    bytes_to_read = 1;

    //          0123456789 01 01234567890123456789012301234 012345 0123456789012345678901234567
    Iprintf("account_id GM user_name               sex    count state\n"_fmt);
    Iprintf("-------------------------------------------------------------------------------\n"_fmt);
    list_count = 0;
}

//--------------------------------------------------------
// Sub-function: Frobnicate items
//--------------------------------------------------------
static
int itemfrob(ZString param)
{
    ItemNameId source_id, dest_id;

    if (!extract(param, record<' '>(&source_id, &dest_id)))
    {
        PRINTF("You must provide the source and destination item IDs.\n"_fmt);
        return 1;
    }

    WFIFOW(login_session, 0) = 0x7924;
    WFIFOL(login_session, 2) = unwrap<ItemNameId>(source_id);
    WFIFOL(login_session, 6) = unwrap<ItemNameId>(dest_id);
    WFIFOSET(login_session, 10);
    bytes_to_read = 1;          // all logging is done to the three main servers

    return 0;
}

//--------------------------------------------
// Sub-function: Asking to modify a memo field
//--------------------------------------------
static
void changememo(ZString param)
{
    AccountName name;
    XString memo;

    if (!qsplit(param, &name, &memo) && !qsplit(param, &name))
    {
        PRINTF("Please input an account name and a memo.\n"_fmt);
        PRINTF("<example> memo testname new memo\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the memo of an account ('email' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
        return;

    size_t len = memo.size();
    size_t len1 = len + 1;
    if (len > 254)
    {
        PRINTF("Memo is too long (%zu characters).\n"_fmt, len);
        PRINTF("Please input a memo of 254 bytes at the maximum.\n"_fmt);
        LADMIN_LOG("Email is too long (%zu characters). Please input a memo of 254 bytes at the maximum.\n"_fmt,
              len);
        return;
    }

    LADMIN_LOG("Request to login-server to change a memo.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7942;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOW(login_session, 26) = len1;
    WFIFO_STRING(login_session, 28, memo, len);
    WFIFOSET(login_session, 28 + len1);
    bytes_to_read = 1;
}

//-----------------------------------------------
// Sub-function: Asking to obtain an account name
//-----------------------------------------------
static
void nameaccount(AccountId id)
{
    LADMIN_LOG("Request to login-server to know an account name.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7946;
    WFIFOL(login_session, 2) = unwrap<AccountId>(id);
    WFIFOSET(login_session, 6);
    bytes_to_read = 1;
}

//------------------------------------------
// Sub-function: Asking to modify a password
// (Note: never send back a password with login-server!! security of passwords)
//------------------------------------------
static
void changepasswd(ZString param)
{
    AccountName name;
    AccountPass password;

    if (!qsplit(param, &name, &password))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<example> password testname newpassword\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the password of an account ('password' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
    {
        return;
    }

    if (!password.is_print())
        return;

    LADMIN_LOG("Request to login-server to change a password.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7934;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFO_STRING(login_session, 26, password, 24);
    WFIFOSET(login_session, 50);
    bytes_to_read = 1;
}

//----------------------------------------------------------------------
// Sub-function: Request to login-server to reload GM configuration file
// this function have no answer
//----------------------------------------------------------------------
static
void reloadGM(ZString params)
{
    WFIFOW(login_session, 0) = 0x7955;
    WFIFOSET(login_session, 2);
    bytes_to_read = 0;

    LADMIN_LOG("Request to reload the GM configuration file sended.\n"_fmt);
    PRINTF("Request to reload the GM configuration file sended.\n"_fmt);
    PRINTF("Check the actual GM accounts (after reloading):\n"_fmt);
    listaccount(params, 1);    // 1: to list only GM
}

//-----------------------------------------------------
// Sub-function: Asking to modify the sex of an account
//-----------------------------------------------------
static
void changesex(ZString param)
{
    AccountName name;
    VString<1> sex_;

    if (!qsplit(param, &name, &sex_))
    {
        PRINTF("Please input an account name and a sex.\n"_fmt);
        PRINTF("<example> sex testname Male\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the sex of an account ('sex' command).\n"_fmt);
        return;
    }
    char sex = sex_.front();

    if (!name.is_print())
    {
        PRINTF("bad name\n"_fmt);
        return;
    }

    if (!"MF"_s.contains(sex))
    {
        PRINTF("Illegal gender [%c]. Please input M or F.\n"_fmt, sex);
        LADMIN_LOG("Illegal gender [%c]. Please input M or F.\n"_fmt, sex);
        return;
    }

    LADMIN_LOG("Request to login-server to change a sex.\n"_fmt);

    WFIFOW(login_session, 0) = 0x793c;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOB(login_session, 26) = sex;
    WFIFOSET(login_session, 27);
    bytes_to_read = 1;
}

//-------------------------------------------------------------------------
// Sub-function of sub-function changestate, blockaccount or unblockaccount
// Asking to modify the state of an account
//-------------------------------------------------------------------------
static
void changestatesub(AccountName name, int state, XString error_message)
{
    if ((state < 0 || state > 9) && state != 100)
    {                           // Valid values: 0: ok, or value of the 0x006a packet + 1
        PRINTF("Please input one of these states:\n"_fmt);
        PRINTF("  0 = Account ok            6 = Your Game's EXE file is not the latest version\n"_fmt);
        PRINTF("  1 = Unregistered ID       7 = You are Prohibited to log in until + message\n"_fmt);
        PRINTF("  2 = Incorrect Password    8 = Server is jammed due to over populated\n"_fmt);
        PRINTF("  3 = This ID is expired    9 = No MSG\n"_fmt);
        PRINTF("  4 = Rejected from Server  100 = This ID has been totally erased\n"_fmt);
        PRINTF("  5 = You have been blocked by the GM Team\n"_fmt);
        PRINTF("<examples> state testname 5\n"_fmt);
        PRINTF("           state testname 7 end of your ban\n"_fmt);
        PRINTF("           block <account name>\n"_fmt);
        PRINTF("           unblock <account name>\n"_fmt);
        LADMIN_LOG("Invalid value for the state of an account ('state', 'block' or 'unblock' command).\n"_fmt);
        return;
    }

    if (!name.is_print())
        return;

    if (state != 7)
    {
        error_message = "-"_s;
    }
    else
    {
        if (error_message.size() < 1)
        {
            PRINTF("Error message is too short. Please input a message of 1-19 bytes.\n"_fmt);
            LADMIN_LOG("Error message is too short. Please input a message of 1-19 bytes.\n"_fmt);
            return;
        }
        if (error_message.size() > 19)
        {
            PRINTF("Error message is too long. Please input a message of 1-19 bytes.\n"_fmt);
            LADMIN_LOG("Error message is too long. Please input a message of 1-19 bytes.\n"_fmt);
            return;
        }
    }

    LADMIN_LOG("Request to login-server to change a state.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7936;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOL(login_session, 26) = state;
    WFIFO_STRING(login_session, 30, error_message, 20);
    WFIFOSET(login_session, 50);
    bytes_to_read = 1;
}

//-------------------------------------------------------
// Sub-function: Asking to modify the state of an account
//-------------------------------------------------------
static
void changestate(ZString param)
{
    AccountName name;
    int state;
    XString error_message;

    if (!qsplit(param, &name, &state, &error_message) && !qsplit(param, &name, &state))
    {
        PRINTF("Please input an account name and a state.\n"_fmt);
        PRINTF("<examples> state testname 5\n"_fmt);
        PRINTF("           state testname 7 end of your ban\n"_fmt);
        PRINTF("           block <account name>\n"_fmt);
        PRINTF("           unblock <account name>\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the state of an account ('state' command).\n"_fmt);
        return;
    }

    changestatesub(name, state, error_message);
}

//-------------------------------------------
// Sub-function: Asking to unblock an account
//-------------------------------------------
static
void unblockaccount(ZString param)
{
    AccountName name;

    if (!qsplit(param, &name))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<examples> state testname 5\n"_fmt);
        PRINTF("           state testname 7 end of your ban\n"_fmt);
        PRINTF("           block <account name>\n"_fmt);
        PRINTF("           unblock <account name>\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the state of an account ('unblock' command).\n"_fmt);
        return;
    }

    changestatesub(name, 0, "-"_s);   // state 0, no error message
}

//-------------------------------------------
// Sub-function: Asking to unblock an account
//-------------------------------------------
static
void blockaccount(ZString param)
{
    AccountName name;

    if (!qsplit(param, &name))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<examples> state testname 5\n"_fmt);
        PRINTF("           state testname 7 end of your ban\n"_fmt);
        PRINTF("           block <account name>\n"_fmt);
        PRINTF("           unblock <account name>\n"_fmt);
        LADMIN_LOG("Incomplete parameters to change the state of an account ('block' command).\n"_fmt);
        return;
    }

    changestatesub(name, 5, "-"_s);   // state 5, no error message
}

//---------------------------------------------------------------------
// Sub-function: Add/substract time to the validity limit of an account
//---------------------------------------------------------------------
static
void timeaddaccount(ZString param)
{
    AccountName name;
    HumanTimeDiff modif {};

    if (!qsplit(param, &name, &modif))
    {
        PRINTF("Please input an account name and a modifier.\n"_fmt);
        PRINTF("  <example>: timeadd testname +1m-2mn1s-6y\n"_fmt);
        PRINTF("             this example adds 1 month and 1 second, and substracts 2 minutes\n"_fmt);
        PRINTF("             and 6 years at the same time.\n"_fmt);
        LADMIN_LOG("Incomplete parameters to modify a limit time ('timeadd' command).\n"_fmt);
        return;
    }
    if (name.is_print())
    {
        return;
    }

    if (!modif)
    {
        PRINTF("Please give an adjustment with this command:\n"_fmt);
        PRINTF("  Adjustment value (-1, 1, +1, etc...)\n"_fmt);
        PRINTF("  Modified element:\n"_fmt);
        PRINTF("    a or y: year\n"_fmt);
        PRINTF("    m:      month\n"_fmt);
        PRINTF("    j or d: day\n"_fmt);
        PRINTF("    h:      hour\n"_fmt);
        PRINTF("    mn:     minute\n"_fmt);
        PRINTF("    s:      second\n"_fmt);
        PRINTF("  <example> timeadd testname +1m-2mn1s-6y\n"_fmt);
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n"_fmt);
        PRINTF("            and 6 years at the same time.\n"_fmt);
        LADMIN_LOG("No adjustment isn't an adjustment ('timeadd' command).\n"_fmt);
        return;
    }

    LADMIN_LOG("Request to login-server to modify a time limit.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7950;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFO_STRUCT(login_session, 26, modif);
    WFIFOSET(login_session, 38);
    bytes_to_read = 1;
}

//-------------------------------------------------
// Sub-function: Set a validity limit of an account
//-------------------------------------------------
static
void timesetaccount(ZString param)
{
    AccountName name;
    XString date;
    XString time_;
    int year, month, day, hour, minute, second;

    year = month = day = hour = minute = second = 0;

    // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
    TimeT connect_until_time = TimeT();
    struct tm tmtime = connect_until_time;   // initialize

    if (!qsplit(param, &name, &date, &time_)
            && !qsplit(param, &name, &date))
    {
        PRINTF("Please input an account name, a date and a hour.\n"_fmt);
        PRINTF("<example>: timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n"_fmt);
        PRINTF("           timeset <account_name> 0   (0 = unlimited)\n"_fmt);
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n"_fmt);
        LADMIN_LOG("Incomplete parameters to set a limit time ('timeset' command).\n"_fmt);
        return;
    }
    if (!name.is_print())
        return;

    if (!time_)
        time_ = "23:59:59"_s;

    if (date != "0"_s
        && ((!extract(date, record<'/'>(&year, &month, &day))
                && !extract(date, record<'-'>(&year, &month, &day))
                && !extract(date, record<'.'>(&year, &month, &day)))
            || !extract(time_, record<':'>(&hour, &minute, &second))))
    {
        PRINTF("Please input 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n"_fmt);
        LADMIN_LOG("Invalid format for the date/time ('timeset' command).\n"_fmt);
        return;
    }

    if (date == "0"_s)
    {
        connect_until_time = TimeT();
    }
    else
    {
        if (year < 70)
        {
            year = year + 100;
        }
        if (year >= 1900)
        {
            year = year - 1900;
        }
        if (month < 1 || month > 12)
        {
            PRINTF("Please give a correct value for the month (from 1 to 12).\n"_fmt);
            LADMIN_LOG("Invalid month for the date ('timeset' command).\n"_fmt);
            return;
        }
        month = month - 1;
        if (day < 1 || day > 31)
        {
            PRINTF("Please give a correct value for the day (from 1 to 31).\n"_fmt);
            LADMIN_LOG("Invalid day for the date ('timeset' command).\n"_fmt);
            return;
        }
        if (((month == 3 || month == 5 || month == 8 || month == 10)
             && day > 30) ||(month == 1 && day > 29))
        {
            PRINTF("Please give a correct value for a day of this month (%d).\n"_fmt,
                 month);
            LADMIN_LOG("Invalid day for this month ('timeset' command).\n"_fmt);
            return;
        }
        if (hour < 0 || hour > 23)
        {
            PRINTF("Please give a correct value for the hour (from 0 to 23).\n"_fmt);
            LADMIN_LOG("Invalid hour for the time ('timeset' command).\n"_fmt);
            return;
        }
        if (minute < 0 || minute > 59)
        {
            PRINTF("Please give a correct value for the minutes (from 0 to 59).\n"_fmt);
            LADMIN_LOG("Invalid minute for the time ('timeset' command).\n"_fmt);
            return;
        }
        if (second < 0 || second > 59)
        {
            PRINTF("Please give a correct value for the seconds (from 0 to 59).\n"_fmt);
            LADMIN_LOG("Invalid second for the time ('timeset' command).\n"_fmt);
            return;
        }
        tmtime.tm_year = year;
        tmtime.tm_mon = month;
        tmtime.tm_mday = day;
        tmtime.tm_hour = hour;
        tmtime.tm_min = minute;
        tmtime.tm_sec = second;
        tmtime.tm_isdst = -1;  // -1: no winter/summer time modification
        connect_until_time = tmtime;
        if (connect_until_time.error())
        {
            PRINTF("Invalid date.\n"_fmt);
            PRINTF("Please add 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n"_fmt);
            LADMIN_LOG("Invalid date. ('timeset' command).\n"_fmt);
            return;
        }
    }

    LADMIN_LOG("Request to login-server to set a time limit.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7948;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOL(login_session, 26) = static_cast<time_t>(connect_until_time);
    WFIFOSET(login_session, 30);
    bytes_to_read = 1;
}

//------------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its name)
//------------------------------------------------------------------------------
static
void whoaccount(ZString param)
{
    AccountName name;

    if (!qsplit(param, &name))
    {
        PRINTF("Please input an account name.\n"_fmt);
        PRINTF("<example> who testname\n"_fmt);
        LADMIN_LOG("No name was given to found the account.\n"_fmt);
        return;
    }
    if (!name.is_print())
    {
        return;
    }

    LADMIN_LOG("Request to login-server to obtain information about an account (by its name).\n"_fmt);

    WFIFOW(login_session, 0) = 0x7952;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOSET(login_session, 26);
    bytes_to_read = 1;
}

//--------------------------------------------------------
// Sub-function: Asking of the version of the login-server
//--------------------------------------------------------
static
void checkloginversion(void)
{
    LADMIN_LOG("Request to login-server to obtain its version.\n"_fmt);

    WFIFOW(login_session, 0) = 0x7530;
    WFIFOSET(login_session, 2);
    bytes_to_read = 1;
}

//---------------------------------------------
// Prompt function
// this function wait until user type a command
// and analyse the command.
//---------------------------------------------
static
void prompt(void)
{
    // while we don't wait new packets
    while (bytes_to_read == 0)
    {
        Iprintf("\n"_fmt);
        Iprintf(SGR_GREEN "To list the commands, type 'enter'." SGR_RESET "\n"_fmt);
        Iprintf(SGR_CYAN "Ladmin-> " SGR_RESET ""_fmt);
        Iprintf(SGR_BOLD ""_fmt);
        fflush(stdout);

        // get command and parameter
        // TODO figure out a better way to do stdio
        static auto cin = make_unique<io::ReadFile>(io::FD::stdin().dup());
        AString buf;
        cin->getline(buf);

        Iprintf(SGR_RESET ""_fmt);
        fflush(stdout);

        if (!cin->is_open())
            exit(0);

        if (!buf.is_print())
        {
            PRINTF("Cowardly refusing to execute a command that includes control or non-ascii characters\n"_fmt);
            LADMIN_LOG("Cowardly refusing to execute a command that includes control or non-ascii characters\n"_fmt);
            continue;
        }

        // extract command name and parameters
        auto space = std::find(buf.begin(), buf.end(), ' ');
        AString command = buf.xislice_h(space);
        while (*space == ' ')
            ++space;

        parameters = buf.xislice_t(space);

        if (!command || command.startswith('?'))
            command = "help"_s;

        if (!parameters)
        {
            LADMIN_LOG("Command: '%s' (without parameters)\n"_fmt,
                    command);
        }
        else
        {
            // We don't want passwords in the log - Camel
            if (command == "create"_s || command == "add"_s || command == "password"_s) {
                AString name, email_, password;
                VString<1> sex_;

                if (qsplit(parameters, &name, &sex_, &email_, &password))
                    LADMIN_LOG("Command: '%s', parameters: '%s %s %s ***'\n"_fmt,
                            command, name, sex_, email_);
                else if (qsplit(parameters, &name, &sex_, &password))
                    LADMIN_LOG("Command: '%s', parameters: '%s %s ***'\n"_fmt,
                            command, name, sex_);
                else if (qsplit(parameters, &name, &password))
                    LADMIN_LOG("Command: '%s', parameters: '%s ***'\n"_fmt,
                            command, name);
                else
                    LADMIN_LOG("Command: '%s' (invalid parameters)\n"_fmt, command);
            }
            else {
                LADMIN_LOG("Command: '%s', parameters: '%s'\n"_fmt,
                        command, parameters);
            }
        }

        // Analyse of the command
        if (command == "help"_s)
            display_help(parameters);
        else if (command == "add"_s)
            addaccount(parameters, 0); // 0: no email
        else if (command == "ban"_s)
            banaccount(parameters);
        else if (command == "banadd"_s)
            banaddaccount(parameters);
        else if (command == "banset"_s)
            bansetaccount(parameters);
        else if (command == "block"_s)
            blockaccount(parameters);
        else if (command == "check"_s)
            checkaccount(parameters);
        else if (command == "create"_s)
            addaccount(parameters, 1); // 1: with email
        else if (command == "delete"_s)
            delaccount(parameters);
        else if (command == "email"_s)
            changeemail(parameters);
        else if (command == "getcount"_s)
            getlogincount();
        else if (command == "gm"_s)
            changegmlevel(parameters);
        else if (command == "id"_s)
            idaccount(parameters);
        else if (command == "info"_s)
            infoaccount(wrap<AccountId>(static_cast<uint32_t>(atoi(parameters.c_str()))));
        else if (command == "kami"_s)
            sendbroadcast(parameters);  // flag for normal
        else if (command == "itemfrob"_s)
            itemfrob(parameters);  // 0: to list all
        else if (command == "list"_s)
            listaccount(parameters, 0);    // 0: to list all
        else if (command == "listban"_s)
            listaccount(parameters, 3);    // 3: to list only accounts with state or bannished
        else if (command == "listgm"_s)
            listaccount(parameters, 1);    // 1: to list only GM
        else if (command == "listok"_s)
            listaccount(parameters, 4);    // 4: to list only accounts without state and not bannished
        else if (command == "memo"_s)
            changememo(parameters);
        else if (command == "name"_s)
            nameaccount(wrap<AccountId>(static_cast<uint32_t>(atoi(parameters.c_str()))));
        else if (command == "password"_s)
            changepasswd(parameters);
        else if (command == "reloadgm"_s)
            reloadGM(parameters);
        else if (command == "search"_s)
            listaccount(parameters, 2);    // 2: to list with pattern
        else if (command == "sex"_s)
            changesex(parameters);
        else if (command == "state"_s)
            changestate(parameters);
        else if (command == "timeadd"_s)
            timeaddaccount(parameters);
        else if (command == "timeset"_s)
            timesetaccount(parameters);
        else if (command == "unban"_s)
            unbanaccount(parameters);
        else if (command == "unblock"_s)
            unblockaccount(parameters);
        else if (command == "version"_s)
            checkloginversion();
        else if (command == "who"_s)
            whoaccount(parameters);
        else if (command == "quit"_s
            || command == "exit"_s
            || command == "end"_s)
        {
            PRINTF("Bye.\n"_fmt);
            exit(0);
        }
        else
        {
            PRINTF("Unknown command [%s].\n"_fmt, buf);
            LADMIN_LOG("Unknown command [%s].\n"_fmt, buf);
        }
    }
}

//-------------------------------------------------------------
// Function: Parse receiving informations from the login-server
//-------------------------------------------------------------
static
void parse_fromlogin(Session *s)
{
    while (RFIFOREST(s) >= 2)
    {
        switch (RFIFOW(s, 0))
        {
            case 0x7919:       // answer of a connection request
                if (RFIFOREST(s) < 3)
                    return;
                if (RFIFOB(s, 2) != 0)
                {
                    PRINTF("Error at login:\n"_fmt);
                    PRINTF(" - incorrect password,\n"_fmt);
                    PRINTF(" - administration system not activated, or\n"_fmt);
                    PRINTF(" - unauthorised IP.\n"_fmt);
                    LADMIN_LOG("Error at login: incorrect password, administration system not activated, or unauthorised IP.\n"_fmt);
                    s->set_eof();
                    //bytes_to_read = 1; // not stop at prompt
                }
                else
                {
                    Iprintf("Established connection.\n"_fmt);
                    LADMIN_LOG("Established connection.\n"_fmt);
                    Iprintf("Reading of the version of the login-server...\n"_fmt);
                    LADMIN_LOG("Reading of the version of the login-server...\n"_fmt);
                    //bytes_to_read = 1; // unchanged
                    checkloginversion();
                }
                RFIFOSKIP(s, 3);
                break;

            case 0x7531:       // Displaying of the version of the login-server
                if (RFIFOREST(s) < 10)
                    return;
            {
                Iprintf("  Login-Server [%s:%d]\n"_fmt,
                        login_ip, login_port);
                Version version;
                RFIFO_STRUCT(login_session, 2, version);
                Iprintf("  tmwA version %hhu.%hhu.%hhu (dev? %hhu) (flags %hhx) (which %hhx) (vend %hu)\n"_fmt,
                        version.major, version.minor, version.patch,
                        version.devel,

                        version.flags, version.which,
                        version.vend);
            }
                bytes_to_read = 0;
                RFIFOSKIP(s, 10);
                break;

            case 0x7925:       // Itemfrob-OK
                RFIFOSKIP(s, 2);
                bytes_to_read = 0;
                break;

            case 0x7921:       // Displaying of the list of accounts
                if (RFIFOREST(s) < 4 || RFIFOREST(s) < RFIFOW(s, 2))
                    return;
                if (RFIFOW(s, 2) < 5)
                {
                    LADMIN_LOG("  Receiving of a void accounts list.\n"_fmt);
                    if (list_count == 0)
                    {
                        Iprintf("No account found.\n"_fmt);
                    }
                    else if (list_count == 1)
                    {
                        Iprintf("1 account found.\n"_fmt);
                    }
                    else
                        Iprintf("%d accounts found.\n"_fmt, list_count);
                    bytes_to_read = 0;
                }
                else
                {
                    int i;
                    LADMIN_LOG("  Receiving of a accounts list.\n"_fmt);
                    for (i = 4; i < RFIFOW(s, 2); i += 38)
                    {
                        AccountName userid = stringish<AccountName>(RFIFO_STRING<24>(s, i + 5));
                        VString<23> lower_userid = userid.to_lower();
                        // what?
                        list_first = next(wrap<AccountId>(RFIFOL(s, i)));
                        // here are checks...
                        if (list_type == 0
                            || (list_type == 1 && RFIFOB(s, i + 4) > 0)
                            || (list_type == 2 && lower_userid.contains_seq(parameters))
                            || (list_type == 3 && RFIFOL(s, i + 34) != 0)
                            || (list_type == 4 && RFIFOL(s, i + 34) == 0))
                        {
                            PRINTF("%10d "_fmt, RFIFOL(s, i));
                            if (RFIFOB(s, i + 4) == 0)
                                PRINTF("   "_fmt);
                            else
                                PRINTF("%2d "_fmt, RFIFOB(s, i + 4));
                            PRINTF("%-24s"_fmt, userid);
                            if (RFIFOB(s, i + 29) == 0)
                                PRINTF("%-5s "_fmt, "Femal"_s);
                            else if (RFIFOB(s, i + 29) == 1)
                                PRINTF("%-5s "_fmt, "Male"_s);
                            else
                                PRINTF("%-5s "_fmt, "Servr"_s);
                            PRINTF("%6d "_fmt, RFIFOL(s, i + 30));
                            switch (RFIFOL(s, i + 34))
                            {
                                case 0:
                                    PRINTF("%-27s\n"_fmt, "Account OK"_s);
                                    break;
                                case 1:
                                    PRINTF("%-27s\n"_fmt, "Unregistered ID"_s);
                                    break;
                                case 2:
                                    PRINTF("%-27s\n"_fmt, "Incorrect Password"_s);
                                    break;
                                case 3:
                                    PRINTF("%-27s\n"_fmt, "This ID is expired"_s);
                                    break;
                                case 4:
                                    PRINTF("%-27s\n"_fmt,
                                            "Rejected from Server"_s);
                                    break;
                                case 5:
                                    PRINTF("%-27s\n"_fmt, "Blocked by the GM Team"_s);   // You have been blocked by the GM Team
                                    break;
                                case 6:
                                    PRINTF("%-27s\n"_fmt, "Your EXE file is too old"_s); // Your Game's EXE file is not the latest version
                                    break;
                                case 7:
                                    PRINTF("%-27s\n"_fmt, "Banishement or"_s);
                                    PRINTF("                                                   Prohibited to login until...\n"_fmt);   // You are Prohibited to log in until %s
                                    break;
                                case 8:
                                    PRINTF("%-27s\n"_fmt,
                                            "Server is over populated"_s);
                                    break;
                                case 9:
                                    PRINTF("%-27s\n"_fmt, "No MSG"_s);
                                    break;
                                default:   // 100
                                    PRINTF("%-27s\n"_fmt, "This ID is totally erased"_s);    // This ID has been totally erased
                                    break;
                            }
                            list_count++;
                        }
                    }
                    // asking of the following acounts
                    LADMIN_LOG("Request to login-server to obtain the list of accounts from %d to %d (complement).\n"_fmt,
                          list_first, list_last);
                    WFIFOW(login_session, 0) = 0x7920;
                    WFIFOL(login_session, 2) = unwrap<AccountId>(list_first);
                    WFIFOL(login_session, 6) = unwrap<AccountId>(list_last);
                    WFIFOSET(login_session, 10);
                    bytes_to_read = 1;
                }
                RFIFOSKIP(s, RFIFOW(s, 2));
                break;

            case 0x7931:       // Answer of login-server about an account creation
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId accid = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!accid)
                {
                    PRINTF("Account [%s] creation failed. Same account already exists.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] creation failed. Same account already exists.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s] is successfully created [id: %d].\n"_fmt,
                            name, accid);
                    LADMIN_LOG("Account [%s] is successfully created [id: %d].\n"_fmt,
                            name, accid);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7933:       // Answer of login-server about an account deletion
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId accid = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!accid)
                {
                    PRINTF("Account [%s] deletion failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] deletion failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] is successfully DELETED.\n"_fmt,
                            name, accid);
                    LADMIN_LOG("Account [%s][id: %d] is successfully DELETED.\n"_fmt,
                            name, accid);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7935:       // answer of the change of an account password
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId accid = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!accid)
                {
                    PRINTF("Account [%s] password changing failed.\n"_fmt,
                            name);
                    PRINTF("Account [%s] doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account password changing failed. The compte [%s] doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] password successfully changed.\n"_fmt,
                            name, accid);
                    LADMIN_LOG("Account [%s][id: %d] password successfully changed.\n"_fmt,
                            name, accid);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7937:       // answer of the change of an account state
                if (RFIFOREST(s) < 34)
                    return;
            {
                AccountId accid = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                int state = RFIFOL(s, 30);
                if (!accid)
                {
                    PRINTF("Account [%s] state changing failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] state changing failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    MString tmpstr;
                    tmpstr += STRPRINTF(
                             "Account [%s] state successfully changed in ["_fmt,
                             name);
                    switch (state)
                    {
                        case 0:
                            tmpstr += "0: Account OK"_s;
                            break;
                        case 1:
                            tmpstr += "1: Unregistered ID"_s;
                            break;
                        case 2:
                            tmpstr += "2: Incorrect Password"_s;
                            break;
                        case 3:
                            tmpstr += "3: This ID is expired"_s;
                            break;
                        case 4:
                            tmpstr += "4: Rejected from Server"_s;
                            break;
                        case 5:
                            tmpstr += "5: You have been blocked by the GM Team"_s;
                            break;
                        case 6:
                            tmpstr += "6: [Your Game's EXE file is not the latest version"_s;
                            break;
                        case 7:
                            tmpstr += "7: You are Prohibited to log in until..."_s;
                            break;
                        case 8:
                            tmpstr += "8: Server is jammed due to over populated"_s;
                            break;
                        case 9:
                            tmpstr += "9: No MSG"_s;
                            break;
                        default:   // 100
                            tmpstr += "100: This ID is totally erased"_s;
                            break;
                    }
                    tmpstr += ']';
                    AString tmpstr_ = AString(tmpstr);
                    PRINTF("%s\n"_fmt, tmpstr_);
                    LADMIN_LOG("%s\n"_fmt, tmpstr_);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 34);
                break;

            case 0x7939:       // answer of the number of online players
                if (RFIFOREST(s) < 4 || RFIFOREST(s) < RFIFOW(s, 2))
                    return;
                {
                    // Get length of the received packet
                    LADMIN_LOG("  Receiving of the number of online players.\n"_fmt);
                    // Read information of the servers
                    if (RFIFOW(s, 2) < 5)
                    {
                        PRINTF("  No server is connected to the login-server.\n"_fmt);
                    }
                    else
                    {
                        PRINTF("  Number of online players (server: number).\n"_fmt);
                        // Displaying of result
                        for (int i = 4; i < RFIFOW(s, 2); i += 32)
                        {
                            ServerName name = stringish<ServerName>(RFIFO_STRING<20>(s, i + 6));
                            PRINTF("    %-20s : %5d\n"_fmt, name,
                                    RFIFOW(s, i + 26));
                        }
                    }
                }
                bytes_to_read = 0;
                RFIFOSKIP(s, RFIFOW(s, 2));
                break;

            case 0x793b:       // answer of the check of a password
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("The account [%s] doesn't exist or the password is incorrect.\n"_fmt,
                            name);
                    LADMIN_LOG("The account [%s] doesn't exist or the password is incorrect.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("The proposed password is correct for the account [%s][id: %d].\n"_fmt,
                            name, account_id);
                    LADMIN_LOG("The proposed password is correct for the account [%s][id: %d].\n"_fmt,
                            name, account_id);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x793d:       // answer of the change of an account sex
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] sex changing failed.\n"_fmt,
                            name);
                    PRINTF("Account [%s] doesn't exist or the sex is already the good sex.\n"_fmt,
                            name);
                    LADMIN_LOG("Account sex changing failed. The compte [%s] doesn't exist or the sex is already the good sex.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] sex successfully changed.\n"_fmt,
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] sex successfully changed.\n"_fmt,
                            name, account_id);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x793f:       // answer of the change of an account GM level
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] GM level changing failed.\n"_fmt,
                            name);
                    PRINTF("Account [%s] doesn't exist, the GM level is already the good GM level\n"_fmt,
                            name);
                    PRINTF("or it's impossible to modify the GM accounts file.\n"_fmt);
                    LADMIN_LOG("Account GM level changing failed. The compte [%s] doesn't exist, the GM level is already the good sex or it's impossible to modify the GM accounts file.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] GM level successfully changed.\n"_fmt,
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] GM level successfully changed.\n"_fmt,
                            name, account_id);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7941:       // answer of the change of an account email
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] e-mail changing failed.\n"_fmt,
                            name);
                    PRINTF("Account [%s] doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account e-mail changing failed. The compte [%s] doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] e-mail successfully changed.\n"_fmt,
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] e-mail successfully changed.\n"_fmt,
                            name, account_id);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7943:       // answer of the change of an account memo
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] memo changing failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] memo changing failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] memo successfully changed.\n"_fmt,
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] memo successfully changed.\n"_fmt,
                            name, account_id);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7945:       // answer of an account id search
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Unable to find the account [%s] id. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Unable to find the account [%s] id. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    PRINTF("The account [%s] have the id: %d.\n"_fmt,
                            name, account_id);
                    LADMIN_LOG("The account [%s] have the id: %d.\n"_fmt,
                            name, account_id);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7947:       // answer of an account name search
                if (RFIFOREST(s) < 30)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!name)
                {
                    PRINTF("Unable to find the account [%d] name. Account doesn't exist.\n"_fmt,
                            account_id);
                    LADMIN_LOG("Unable to find the account [%d] name. Account doesn't exist.\n"_fmt,
                            account_id);
                }
                else
                {
                    PRINTF("The account [id: %d] have the name: %s.\n"_fmt,
                            account_id, name);
                    LADMIN_LOG("The account [id: %d] have the name: %s.\n"_fmt,
                            account_id, name);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 30);
                break;

            case 0x7949:       // answer of an account validity limit set
                if (RFIFOREST(s) < 34)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (RFIFOL(s, 2) == -1)
                {
                    PRINTF("Account [%s] validity limit changing failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] validity limit changing failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Validity Limit of the account [%s][id: %d] successfully changed to [unlimited].\n"_fmt,
                                name, account_id);
                        LADMIN_LOG("Validity Limit of the account [%s][id: %d] successfully changed to [unlimited].\n"_fmt,
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Validity Limit of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, account_id, tmpstr);
                        LADMIN_LOG("Validity Limit of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, account_id,
                                tmpstr);
                    }
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 34);
                break;

            case 0x794b:       // answer of an account ban set
                if (RFIFOREST(s) < 34)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] final date of banishment changing failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] final date of banishment changing failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n"_fmt,
                                name, account_id);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n"_fmt,
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, RFIFOL(s, 2), tmpstr);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, RFIFOL(s, 2),
                                tmpstr);
                    }
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 34);
                break;

            case 0x794d:       // answer of an account ban date/time changing
                if (RFIFOREST(s) < 34)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] final date of banishment changing failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] final date of banishment changing failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n"_fmt,
                                name, account_id);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n"_fmt,
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, account_id,
                                tmpstr);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, account_id,
                                tmpstr);
                    }
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 34);
                break;

            case 0x794f:       // answer of a broadcast
                if (RFIFOREST(s) < 4)
                    return;
                if (RFIFOW(s, 2) == static_cast<uint16_t>(-1))
                {
                    PRINTF("Message sending failed. No online char-server.\n"_fmt);
                    LADMIN_LOG("Message sending failed. No online char-server.\n"_fmt);
                }
                else
                {
                    PRINTF("Message successfully sended to login-server.\n"_fmt);
                    LADMIN_LOG("Message successfully sended to login-server.\n"_fmt);
                }
                bytes_to_read = 0;
                RFIFOSKIP(s, 4);
                break;

            case 0x7951:       // answer of an account validity limit changing
                if (RFIFOREST(s) < 34)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!account_id)
                {
                    PRINTF("Account [%s] validity limit changing failed. Account doesn't exist.\n"_fmt,
                            name);
                    LADMIN_LOG("Account [%s] validity limit changing failed. Account doesn't exist.\n"_fmt,
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Validity limit of the account [%s][id: %d] unchanged.\n"_fmt,
                                name, account_id);
                        PRINTF("The account have an unlimited validity limit or\n"_fmt);
                        PRINTF("the changing is impossible with the proposed adjustments.\n"_fmt);
                        LADMIN_LOG("Validity limit of the account [%s][id: %d] unchanged. The account have an unlimited validity limit or the changing is impossible with the proposed adjustments.\n"_fmt,
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, account_id,
                                tmpstr);
                        LADMIN_LOG("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n"_fmt,
                                name, account_id,
                                tmpstr);
                    }
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(s, 34);
                break;

            case 0x7953:       // answer of a request about informations of an account (by account name/id)
                if (RFIFOREST(s) < 150
                    || RFIFOREST(s) < (150 + RFIFOW(s, 148)))
                    return;
                {
                    AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                    // TODO fix size (there's a lot of other stuff wrong with this packet too
                    GmLevel gm = GmLevel::from(static_cast<uint32_t>(RFIFOB(s, 6)));
                    AccountName userid = stringish<AccountName>(RFIFO_STRING<24>(s, 7));
                    SEX sex = static_cast<SEX>(RFIFOB(s, 31));
                    int connections = RFIFOL(s, 32);
                    int state = RFIFOL(s, 36);
                    timestamp_seconds_buffer error_message = stringish<timestamp_seconds_buffer>(RFIFO_STRING<20>(s, 40));
                    timestamp_milliseconds_buffer lastlogin = stringish<timestamp_milliseconds_buffer>(RFIFO_STRING<24>(s, 60));
                    VString<15> last_ip_ = RFIFO_STRING<16>(s, 84);
                    AccountEmail email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 100));
                    TimeT connect_until_time = static_cast<time_t>(RFIFOL(s, 140));
                    TimeT ban_until_time = static_cast<time_t>(RFIFOL(s, 144));
                    AString memo = RFIFO_STRING(s, 150, RFIFOW(s, 148));
                    if (!account_id)
                    {
                        PRINTF("Unabled to find the account [%s]. Account doesn't exist.\n"_fmt,
                                userid);
                        LADMIN_LOG("Unabled to find the account [%s]. Account doesn't exist.\n"_fmt,
                                userid);
                    }
                    else if (!userid)
                    {
                        PRINTF("Unabled to find the account [id: %d]. Account doesn't exist.\n"_fmt,
                                account_id);
                        LADMIN_LOG("Unabled to find the account [id: %d]. Account doesn't exist.\n"_fmt,
                                account_id);
                    }
                    else
                    {
                        LADMIN_LOG("Receiving information about an account.\n"_fmt);
                        PRINTF("The account is set with:\n"_fmt);
                        if (!gm)
                        {
                            PRINTF(" Id:     %d (non-GM)\n"_fmt, account_id);
                        }
                        else
                        {
                            PRINTF(" Id:     %d (GM level %d)\n"_fmt,
                                    account_id, gm);
                        }
                        PRINTF(" Name:   '%s'\n"_fmt, userid);
                        if (sex == SEX::FEMALE)
                            PRINTF(" Sex:    Female\n"_fmt);
                        else if (sex == SEX::MALE)
                            PRINTF(" Sex:    Male\n"_fmt);
                        else // doesn't happen anymore
                            PRINTF(" Sex:    Server\n"_fmt);
                        PRINTF(" E-mail: %s\n"_fmt, email);
                        switch (state)
                        {
                            case 0:
                                PRINTF(" Statut: 0 [Account OK]\n"_fmt);
                                break;
                            case 1:
                                PRINTF(" Statut: 1 [Unregistered ID]\n"_fmt);
                                break;
                            case 2:
                                PRINTF(" Statut: 2 [Incorrect Password]\n"_fmt);
                                break;
                            case 3:
                                PRINTF(" Statut: 3 [This ID is expired]\n"_fmt);
                                break;
                            case 4:
                                PRINTF(" Statut: 4 [Rejected from Server]\n"_fmt);
                                break;
                            case 5:
                                PRINTF(" Statut: 5 [You have been blocked by the GM Team]\n"_fmt);
                                break;
                            case 6:
                                PRINTF(" Statut: 6 [Your Game's EXE file is not the latest version]\n"_fmt);
                                break;
                            case 7:
                                PRINTF(" Statut: 7 [You are Prohibited to log in until %s]\n"_fmt,
                                     error_message);
                                break;
                            case 8:
                                PRINTF(" Statut: 8 [Server is jammed due to over populated]\n"_fmt);
                                break;
                            case 9:
                                PRINTF(" Statut: 9 [No MSG]\n"_fmt);
                                break;
                            default:   // 100
                                PRINTF(" Statut: %d [This ID is totally erased]\n"_fmt,
                                        state);
                                break;
                        }
                        if (!ban_until_time)
                        {
                            PRINTF(" Banishment: not banished.\n"_fmt);
                        }
                        else
                        {
                            timestamp_seconds_buffer tmpstr;
                            stamp_time(tmpstr, &ban_until_time);
                            PRINTF(" Banishment: until %s.\n"_fmt, tmpstr);
                        }
                        if (connections > 1)
                            PRINTF(" Count:  %d connections.\n"_fmt,
                                    connections);
                        else
                            PRINTF(" Count:  %d connection.\n"_fmt,
                                    connections);
                        PRINTF(" Last connection at: %s (ip: %s)\n"_fmt,
                                lastlogin, last_ip_);
                        if (!connect_until_time)
                        {
                            PRINTF(" Validity limit: unlimited.\n"_fmt);
                        }
                        else
                        {
                            timestamp_seconds_buffer tmpstr;
                            stamp_time(tmpstr, &connect_until_time);
                            PRINTF(" Validity limit: until %s.\n"_fmt,
                                    tmpstr);
                        }
                        PRINTF(" Memo:   '%s'\n"_fmt, memo);
                    }
                }
                bytes_to_read = 0;
                RFIFOSKIP(s, 150 + RFIFOW(s, 148));
                break;

            default:
                PRINTF("Remote administration has been disconnected (unknown packet).\n"_fmt);
                LADMIN_LOG("'End of connection, unknown packet.\n"_fmt);
                s->set_eof();
                return;
        }
    }

    // if we don't wait new packets, do the prompt
    prompt();
}

//------------------------------------
// Function to connect to login-server
//------------------------------------
static
int Connect_login_server(void)
{
    Iprintf("Attempt to connect to login-server...\n"_fmt);
    LADMIN_LOG("Attempt to connect to login-server...\n"_fmt);

    login_session = make_connection(login_ip, login_port, SessionParsers{.func_parse= parse_fromlogin, .func_delete= delete_fromlogin});

    if (!login_session)
        return 0;

    {
        WFIFOW(login_session, 0) = 0x7918;  // Request for administation login
        WFIFOW(login_session, 2) = 0;   // no encrypted
        WFIFO_STRING(login_session, 4, admin_pass, 24);
        WFIFOSET(login_session, 28);
        bytes_to_read = 1;

        Iprintf("Sending of the password...\n"_fmt);
        LADMIN_LOG("Sending of the password...\n"_fmt);
    }

    return 0;
}

static
bool admin_confs(XString w1, ZString w2)
{
    {
        if (w1 == "login_ip"_s)
        {
            struct hostent *h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                Iprintf("Login server IP address: %s -> %s\n"_fmt,
                        w2, login_ip);
                login_ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
            }
        }
        else if (w1 == "login_port"_s)
        {
            login_port = atoi(w2.c_str());
        }
        else if (w1 == "admin_pass"_s)
        {
            admin_pass = stringish<AccountPass>(w2);
        }
        else if (w1 == "ladmin_log_filename"_s)
        {
            ladmin_log_filename = w2;
        }
        else
        {
            PRINTF("WARNING: unknown ladmin config key: %s\n"_fmt, AString(w1));
            return false;
        }
    }
    return true;
}

//--------------------------------------
// Function called at exit of the server
//--------------------------------------
void term_func(void)
{

    if (already_exit_function == 0)
    {
        delete_session(login_session);

        Iprintf(SGR_RESET "----End of Ladmin (normal end with closing of all files).\n"_fmt);
        LADMIN_LOG("----End of Ladmin (normal end with closing of all files).\n"_fmt);

        already_exit_function = 1;
    }
}

//------------------------
// Main function of ladmin
//------------------------
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
            runflag &= load_config_file(argvi, admin_confs);
        }
    }

    if (!loaded_config_yet)
        runflag &= load_config_file("conf/tmwa-admin.conf"_s, admin_confs);

    eathena_interactive_session = isatty(0);

    LADMIN_LOG(""_fmt);
    LADMIN_LOG("Configuration file readed.\n"_fmt);

    Iprintf("EAthena login-server administration tool.\n"_fmt);
    Version version = CURRENT_LOGIN_SERVER_VERSION;
    Iprintf("for tmwA version %hhu.%hhu.%hhu (dev? %hhu) (flags %hhx) (which %hhx) (vend %hu)\n"_fmt,
            version.major, version.minor, version.patch,
            version.devel,

            version.flags, version.which,
            version.vend);

    LADMIN_LOG("Ladmin is ready.\n"_fmt);
    Iprintf("Ladmin is " SGR_BOLD SGR_GREEN "ready" SGR_RESET ".\n\n"_fmt);

    Connect_login_server();

    return 0;
}
