#include <arpa/inet.h>
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

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"
#include "../strings/vstring.hpp"

#include "../generic/md5.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/read.hpp"
#include "../io/tty.hpp"
#include "../io/write.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/core.hpp"
#include "../mmo/human_time_diff.hpp"
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
AccountPass admin_pass = stringish<AccountPass>("admin");    // Administration password
static
AString ladmin_log_filename = "log/ladmin.log";
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
int list_first, list_last, list_type, list_count;  // parameter to display a list of accounts
static
int already_exit_function = 0; // sometimes, the exit function is called twice... so, don't log twice the message

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
void SessionDeleter::operator()(SessionData *)
{
    assert(false && "ladmin does not have sessions");
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

static
void delete_fromlogin(Session *)
{
    {
        PRINTF("Impossible to have a connection with the login-server [%s:%d] !\n",
                login_ip, login_port);
        LADMIN_LOG("Impossible to have a connection with the login-server [%s:%d] !\n",
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
        command = "help";

    LADMIN_LOG("Displaying of the commands or a command.\n");

    if (command == "help")
    {
        PRINTF("help/?\n");
        PRINTF("  Display the description of the commands\n");
        PRINTF("help/? [command]\n");
        PRINTF("  Display the description of the specified command\n");
    }
    else if (command == "add")
    {
        PRINTF("add <account_name> <sex> <password>\n");
        PRINTF("  Create an account with the default email (a@a.com).\n");
        PRINTF("  Concerning the sex, only the first letter is used (F or M).\n");
        PRINTF("  The e-mail is set to a@a.com (default e-mail). It's like to have no e-mail.\n");
        PRINTF("  When the password is omitted,\n");
        PRINTF("  the input is done without displaying of the pressed keys.\n");
        PRINTF("  <example> add testname Male testpass\n");
    }
    else if (command == "ban")
    {
        PRINTF("ban yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("  Changes the final date of a banishment of an account.\n");
        PRINTF("  Like banset, but <account name> is at end.\n");
    }
    else if (command == "banadd")
    {
        PRINTF("banadd <account_name> <modifier>\n");
        PRINTF("  Adds or substracts time from the final date of a banishment of an account.\n");
        PRINTF("  Modifier is done as follows:\n");
        PRINTF("    Adjustment value (-1, 1, +1, etc...)\n");
        PRINTF("    Modified element:\n");
        PRINTF("      a or y: year\n");
        PRINTF("      m:  month\n");
        PRINTF("      j or d: day\n");
        PRINTF("      h:  hour\n");
        PRINTF("      mn: minute\n");
        PRINTF("      s:  second\n");
        PRINTF("  <example> banadd testname +1m-2mn1s-6y\n");
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("            and 6 years at the same time.\n");
        PRINTF("NOTE: If you modify the final date of a non-banished account,\n");
        PRINTF("      you fix the final date to (actual time +- adjustments)\n");
    }
    else if (command == "banset")
    {
        PRINTF("banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("  Changes the final date of a banishment of an account.\n");
        PRINTF("  Default time [hh:mm:ss]: 23:59:59.\n");
        PRINTF("banset <account_name> 0\n");
        PRINTF("  Set a non-banished account (0 = unbanished).\n");
    }
    else if (command == "block")
    {
        PRINTF("block <account name>\n");
        PRINTF("  Set state 5 (You have been blocked by the GM Team) to an account.\n");
        PRINTF("  This command works like state <account_name> 5.\n");
    }
    else if (command == "check")
    {
        PRINTF("check <account_name> <password>\n");
        PRINTF("  Check the validity of a password for an account.\n");
        PRINTF("  NOTE: Server will never sends back a password.\n");
        PRINTF("        It's the only method you have to know if a password is correct.\n");
        PRINTF("        The other method is to have a ('physical') access to the accounts file.\n");
    }
    else if (command == "create")
    {
        PRINTF("create <account_name> <sex> <email> <password>\n");
        PRINTF("  Like the 'add' command, but with e-mail moreover.\n");
        PRINTF("  <example> create testname Male my@mail.com testpass\n");
    }
    else if (command == "delete")
    {
        PRINTF("delete <account name>\n");
        PRINTF("  Remove an account.\n");
        PRINTF("  This order requires confirmation. After confirmation, the account is deleted.\n");
    }
    else if (command == "email")
    {
        PRINTF("email <account_name> <email>\n");
        PRINTF("  Modify the e-mail of an account.\n");
    }
    else if (command == "getcount")
    {
        PRINTF("getcount\n");
        PRINTF("  Give the number of players online on all char-servers.\n");
    }
    else if (command == "gm")
    {
        PRINTF("gm <account_name> [GM_level]\n");
        PRINTF("  Modify the GM level of an account.\n");
        PRINTF("  Default value remove GM level (GM level = 0).\n");
        PRINTF("  <example> gm testname 80\n");
    }
    else if (command == "id")
    {
        PRINTF("id <account name>\n");
        PRINTF("  Give the id of an account.\n");
    }
    else if (command == "info")
    {
        PRINTF("info <account_id>\n");
        PRINTF("  Display complete information of an account.\n");
    }
    else if (command == "kami")
    {
        PRINTF("kami <message>\n");
        PRINTF("  Sends a broadcast message on all map-server (in yellow).\n");
    }
    else if (command == "kamib")
    {
        PRINTF("kamib <message>\n");
        PRINTF("  Sends a broadcast message on all map-server (in blue).\n");
    }
    else if (command == "list")
    {
        PRINTF("list/ls [start_id [end_id]]\n");
        PRINTF("  Display a list of accounts.\n");
        PRINTF("  'start_id', 'end_id': indicate end and start identifiers.\n");
        PRINTF("  Research by name is not possible with this command.\n");
        PRINTF("  <example> list 10 9999999\n");
    }
    else if (command == "itemfrob")
    {
        PRINTF("itemfrob <source-id> <dest-id>\n");
        PRINTF("  Translates item IDs for all accounts.\n");
        PRINTF("  Any items matching the source item ID will be mapped to the dest-id.\n");
        PRINTF("  <example> itemfrob 500 700\n");
    }
    else if (command == "listban")
    {
        PRINTF("listban [start_id [end_id]]\n");
        PRINTF("  Like list/ls, but only for accounts with state or banished.\n");
    }
    else if (command == "listgm")
    {
        PRINTF("listgm [start_id [end_id]]\n");
        PRINTF("  Like list/ls, but only for GM accounts.\n");
    }
    else if (command == "listok")
    {
        PRINTF("listok [start_id [end_id]]\n");
        PRINTF("  Like list/ls, but only for accounts without state and not banished.\n");
    }
    else if (command == "memo")
    {
        PRINTF("memo <account_name> <memo>\n");
        PRINTF("  Modify the memo of an account.\n");
        PRINTF("  'memo': it can have until 253 characters (with spaces or not).\n");
    }
    else if (command == "name")
    {
        PRINTF("name <account_id>\n");
        PRINTF("  Give the name of an account.\n");
    }
    else if (command == "password")
    {
        PRINTF("password <account_name> <new_password>\n");
        PRINTF("  Change the password of an account.\n");
        PRINTF("  When new password is omitted,\n");
        PRINTF("  the input is done without displaying of the pressed keys.\n");
    }
    else if (command == "reloadgm")
    {
        PRINTF("reloadGM\n");
        PRINTF("  Reload GM configuration file\n");
    }
    else if (command == "search")
    {
        PRINTF("search <expression>\n");
        PRINTF("  Seek accounts.\n");
        PRINTF("  Displays the accounts whose names correspond.\n");
    }
    else if (command == "sex")
    {
        PRINTF("sex <account_name> <sex>\n");
        PRINTF("  Modify the sex of an account.\n");
        PRINTF("  <example> sex testname Male\n");
    }
    else if (command == "state")
    {
        PRINTF("state <account_name> <new_state> <error_message_#7>\n");
        PRINTF("  Change the state of an account.\n");
        PRINTF("  'new_state': state is the state of the packet 0x006a + 1.\n");
        PRINTF("               The possibilities are:\n");
        PRINTF("               0 = Account ok\n");
        PRINTF("               1 = Unregistered ID\n");
        PRINTF("               2 = Incorrect Password\n");
        PRINTF("               3 = This ID is expired\n");
        PRINTF("               4 = Rejected from Server\n");
        PRINTF("               5 = You have been blocked by the GM Team\n");
        PRINTF("               6 = Your Game's EXE file is not the latest version\n");
        PRINTF("               7 = You are Prohibited to log in until...\n");
        PRINTF("               8 = Server is jammed due to over populated\n");
        PRINTF("               9 = No MSG\n");
        PRINTF("               100 = This ID has been totally erased\n");
        PRINTF("               all other values are 'No MSG', then use state 9 please.\n");
        PRINTF("  'error_message_#7': message of the code error 6\n");
        PRINTF("                      = Your are Prohibited to log in until... (packet 0x006a)\n");
    }
    else if (command == "timeadd")
    {
        PRINTF("timeadd <account_name> <modifier>\n");
        PRINTF("  Adds or substracts time from the validity limit of an account.\n");
        PRINTF("  Modifier is done as follows:\n");
        PRINTF("    Adjustment value (-1, 1, +1, etc...)\n");
        PRINTF("    Modified element:\n");
        PRINTF("      a or y: year\n");
        PRINTF("      m:  month\n");
        PRINTF("      j or d: day\n");
        PRINTF("      h:  hour\n");
        PRINTF("      mn: minute\n");
        PRINTF("      s:  second\n");
        PRINTF("  <example> timeadd testname +1m-2mn1s-6y\n");
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("            and 6 years at the same time.\n");
        PRINTF("NOTE: You can not modify a unlimited validity limit.\n");
        PRINTF("      If you want modify it, you want probably create a limited validity limit.\n");
        PRINTF("      So, at first, you must set the validity limit to a date/time.\n");
    }
    else if (command == "timeadd")
    {
        PRINTF("timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("  Changes the validity limit of an account.\n");
        PRINTF("  Default time [hh:mm:ss]: 23:59:59.\n");
        PRINTF("timeset <account_name> 0\n");
        PRINTF("  Gives an unlimited validity limit (0 = unlimited).\n");
    }
    else if (command == "unban")
    {
        PRINTF("unban/unbanish <account name>\n");
        PRINTF("  Remove the banishment of an account.\n");
        PRINTF("  This command works like banset <account_name> 0.\n");
    }
    else if (command == "unblock")
    {
        PRINTF("unblock <account name>\n");
        PRINTF("  Set state 0 (Account ok) to an account.\n");
        PRINTF("  This command works like state <account_name> 0.\n");
    }
    else if (command == "version")
    {
        PRINTF("version\n");
        PRINTF("  Display the version of the login-server.\n");
    }
    else if (command == "who")
    {
        PRINTF("who <account name>\n");
        PRINTF("  Displays complete information of an account.\n");
    }
    else if (command == "quit"
        || command == "exit"
        || command == "end")
    {
        PRINTF("quit/end/exit\n");
        PRINTF("  End of the program of administration.\n");
    }
    else
    {
        if (command)
            PRINTF("Unknown command [%s] for help. Displaying of all commands.\n",
                 AString(command));
        PRINTF(" help/?                          -- Display this help\n");
        PRINTF(" help/? [command]                -- Display the help of the command\n");
        PRINTF(" add <account_name> <sex> <password>  -- Create an account with default email\n");
        PRINTF(" ban yyyy/mm/dd hh:mm:ss <account name> -- Change final date of a ban\n");
        PRINTF(" banadd <account_name> <modifier>     -- Add or substract time from the final\n");
        PRINTF("   example: ba apple +1m-2mn1s-2y        date of a banishment of an account\n");
        PRINTF(" banset <account_name> yyyy/mm/dd [hh:mm:ss] -- Change final date of a ban\n");
        PRINTF(" banset <account_name> 0              -- Un-banish an account\n");
        PRINTF(" block <account name>     -- Set state 5 (blocked by the GM Team) to an account\n");
        PRINTF(" check <account_name> <password>      -- Check the validity of a password\n");
        PRINTF(" create <account_name> <sex> <email> <passwrd> -- Create an account with email\n");
        PRINTF(" delete <account name>                -- Remove an account\n");
        PRINTF(" email <account_name> <email>         -- Modify an email of an account\n");
        PRINTF(" getcount                             -- Give the number of players online\n");
        PRINTF(" gm <account_name> [GM_level]         -- Modify the GM level of an account\n");
        PRINTF(" id <account name>                    -- Give the id of an account\n");
        PRINTF(" info <account_id>                    -- Display all information of an account\n");
        PRINTF(" itemfrob <source-id> <dest-id>       -- Map all items from one item ID to another\n");
        PRINTF(" kami <message>                       -- Sends a broadcast message (in yellow)\n");
        PRINTF(" kamib <message>                      -- Sends a broadcast message (in blue)\n");
        PRINTF(" list [First_id [Last_id]]            -- Display a list of accounts\n");
        PRINTF(" listban [First_id [Last_id] ]        -- Display a list of accounts\n");
        PRINTF("                                         with state or banished\n");
        PRINTF(" listgm [First_id [Last_id]]          -- Display a list of GM accounts\n");
        PRINTF(" listok [First_id [Last_id] ]         -- Display a list of accounts\n");
        PRINTF("                                         without state and not banished\n");
        PRINTF(" memo <account_name> <memo>           -- Modify the memo of an account\n");
        PRINTF(" name <account_id>                    -- Give the name of an account\n");
        PRINTF(" password <account_name> <new_password> -- Change the password of an account\n");
        PRINTF(" quit/end/exit                        -- End of the program of administation\n");
        PRINTF(" reloadGM                             -- Reload GM configuration file\n");
        PRINTF(" search <expression>                  -- Seek accounts\n");
        PRINTF(" sex <nomcompte> <sexe>               -- Modify the sex of an account\n");
        PRINTF(" state <account_name> <new_state> <error_message_#7> -- Change the state\n");
        PRINTF(" timeadd <account_name> <modifier>    -- Add or substract time from the\n");
        PRINTF("   example: ta apple +1m-2mn1s-2y        validity limit of an account\n");
        PRINTF(" timeset <account_name> yyyy/mm/dd [hh:mm:ss] -- Change the validify limit\n");
        PRINTF(" timeset <account_name> 0             -- Give a unlimited validity limit\n");
        PRINTF(" unban <account name>                 -- Remove the banishment of an account\n");
        PRINTF(" unblock <account name>               -- Set state 0 (Account ok) to an account\n");
        PRINTF(" version                              -- Gives the version of the login-server\n");
        PRINTF(" who <account name>                   -- Display all information of an account\n");
        PRINTF(" who <account name>                   -- Display all information of an account\n");
        PRINTF(" Note: To use spaces in an account name, type \"<account name>\" (or ').\n");
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
            PRINTF("Please input an account name, a sex and a password.\n");
            PRINTF("<example> add testname Male testpass\n");
            LADMIN_LOG("Incomplete parameters to create an account ('add' command).\n");
            return;
        }
        email_ = DEFAULT_EMAIL;
    }
    else
    {
        // 1: create command
        if (!qsplit(param, &name, &sex_, &email_, &password))
        {
            PRINTF("Please input an account name, a sex and a password.\n");
            PRINTF("<example> create testname Male my@mail.com testpass\n");
            LADMIN_LOG("Incomplete parameters to create an account ('create' command).\n");
            return;
        }
    }
    char sex = sex_.front();
    if (!name.is_print())
        return;

    if (!XString("MF").contains(sex))
    {
        PRINTF("Illegal gender [%c]. Please input M or F.\n", sex);
        LADMIN_LOG("Illegal gender [%c]. Please input M or F.\n", sex);
        return;
    }

    if (!e_mail_check(email_))
    {
        PRINTF("Invalid email [%s]. Please input a valid e-mail.\n",
                AString(email_));
        LADMIN_LOG("Invalid email [%s]. Please input a valid e-mail.\n",
                AString(email_));
        return;
    }
    AccountEmail email = stringish<AccountEmail>(email_);

    if (!password.is_print())
        return;

    LADMIN_LOG("Request to login-server to create an account.\n");

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
        PRINTF("Please input an account name and a modifier.\n");
        PRINTF("  <example>: banadd testname +1m-2mn1s-6y\n");
        PRINTF("             this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("             and 6 years at the same time.\n");
        LADMIN_LOG("Incomplete parameters to modify the ban date/time of an account ('banadd' command).\n");
        return;
    }
    if (!name.is_print())
        return;

    if (!modif)
    {
        PRINTF("Please give an adjustment with this command:\n");
        PRINTF("  Adjustment value (-1, 1, +1, etc...)\n");
        PRINTF("  Modified element:\n");
        PRINTF("    a or y: year\n");
        PRINTF("    m: month\n");
        PRINTF("    j or d: day\n");
        PRINTF("    h: hour\n");
        PRINTF("    mn: minute\n");
        PRINTF("    s: second\n");
        PRINTF("  <example> banadd testname +1m-2mn1s-6y\n");
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("            and 6 years at the same time.\n");
        LADMIN_LOG("No adjustment isn't an adjustment ('banadd' command).\n");
        return;
    }

    LADMIN_LOG("Request to login-server to modify a ban date/time.\n");

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

    if (date != "0"
        && ((!extract(date, record<'/'>(&year, &month, &day))
                && !extract(date, record<'-'>(&year, &month, &day))
                && !extract(date, record<'.'>(&year, &month, &day)))
            || !extract(time_, record<':'>(&hour, &minute, &second))))
    {
        PRINTF("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n");
        PRINTF("You can imput 0 instead of if you use 'banset' command.\n");
        LADMIN_LOG("Invalid format for the date/time ('banset' or 'ban' command).\n");
        return;
    }

    if (date == "0")
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
            PRINTF("Please give a correct value for the month (from 1 to 12).\n");
            LADMIN_LOG("Invalid month for the date ('banset' or 'ban' command).\n");
            return;
        }
        month = month - 1;
        if (day < 1 || day > 31)
        {
            PRINTF("Please give a correct value for the day (from 1 to 31).\n");
            LADMIN_LOG("Invalid day for the date ('banset' or 'ban' command).\n");
            return;
        }
        if (((month == 3 || month == 5 || month == 8 || month == 10)
             && day > 30) || (month == 1 && day > 29))
        {
            PRINTF("Please give a correct value for a day of this month (%d).\n",
                 month);
            LADMIN_LOG("Invalid day for this month ('banset' or 'ban' command).\n");
            return;
        }
        if (hour < 0 || hour > 23)
        {
            PRINTF("Please give a correct value for the hour (from 0 to 23).\n");
            LADMIN_LOG("Invalid hour for the time ('banset' or 'ban' command).\n");
            return;
        }
        if (minute < 0 || minute > 59)
        {
            PRINTF("Please give a correct value for the minutes (from 0 to 59).\n");
            LADMIN_LOG("Invalid minute for the time ('banset' or 'ban' command).\n");
            return;
        }
        if (second < 0 || second > 59)
        {
            PRINTF("Please give a correct value for the seconds (from 0 to 59).\n");
            LADMIN_LOG("Invalid second for the time ('banset' or 'ban' command).\n");
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
            PRINTF("Invalid date.\n");
            PRINTF("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n");
            PRINTF("You can imput 0 instead of if you use 'banset' command.\n");
            LADMIN_LOG("Invalid date. ('banset' or 'ban' command).\n");
            return;
        }
    }

    LADMIN_LOG("Request to login-server to set a ban.\n");

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
        PRINTF("Please input an account name, a date and a hour.\n");
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n");
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("           unban/unbanish <account name>\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a ban ('banset' or 'ban' command).\n");
        return;
    }

    if (!time_)
        time_ = "23:59:59";

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
        PRINTF("Please input an account name, a date and a hour.\n");
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n");
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("           unban/unbanish <account name>\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a ban ('banset' or 'ban' command).\n");
        return;
    }

    if (!time_)
        time_ = "23:59:59";

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
        PRINTF("Please input an account name.\n");
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n");
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("           unban/unbanish <account name>\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a ban ('unban' command).\n");
        return;
    }

    bansetaccountsub(name, "0", "");
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
        PRINTF("Please input an account name.\n");
        PRINTF("<example> check testname password\n");
        LADMIN_LOG("Incomplete parameters to check the password of an account ('check' command).\n");
        return;
    }

    if (!name.is_print())
        return;

    if (!password.is_print())
        return;

    LADMIN_LOG("Request to login-server to check a password.\n");

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
        PRINTF("Please input an account name.\n");
        PRINTF("<example> delete testnametodelete\n");
        LADMIN_LOG("No name given to delete an account ('delete' command).\n");
        return;
    }

    if (!name.is_print())
        return;

    char confirm;
    do
    {
        PRINTF(SGR_BOLD SGR_CYAN " ** Are you really sure to DELETE account [%s]? (y/n) > " SGR_RESET, name);
        fflush(stdout);
        int seek = getchar();
        confirm = seek;
        if (seek == EOF)
            confirm = 'n';
        else
            while (seek != '\n' && seek != EOF)
                seek = getchar();
    }
    while (!XString("yn").contains(confirm));

    if (confirm == 'n')
    {
        PRINTF("Deletion canceled.\n");
        LADMIN_LOG("Deletion canceled by user ('delete' command).\n");
        return;
    }

    LADMIN_LOG("Request to login-server to delete an acount.\n");

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
        PRINTF("Please input an account name and an email.\n");
        PRINTF("<example> email testname newemail\n");
        LADMIN_LOG("Incomplete parameters to change the email of an account ('email' command).\n");
        return;
    }

    if (!name.is_print())
        return;

    if (!e_mail_check(email_))
    {
        PRINTF("Invalid email [%s]. Please input a valid e-mail.\n",
                AString(email_));
        LADMIN_LOG("Invalid email [%s]. Please input a valid e-mail.\n",
                AString(email_));
        return;
    }
    AccountEmail email = stringish<AccountEmail>(email_);

    LADMIN_LOG("Request to login-server to change an email.\n");

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
    LADMIN_LOG("Request to login-server to obtain the # of online players.\n");

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
        PRINTF("Please input an account name and a GM level.\n");
        PRINTF("<example> gm testname 80\n");
        LADMIN_LOG("Incomplete parameters to change the GM level of an account ('gm' command).\n");
        return;
    }

    if (!name.is_print())
        return;

    if (GM_level < 0 || GM_level > 99)
    {
        PRINTF("Illegal GM level [%d]. Please input a value from 0 to 99.\n",
             GM_level);
        LADMIN_LOG("Illegal GM level [%d]. The value can be from 0 to 99.\n",
              GM_level);
        return;
    }

    LADMIN_LOG("Request to login-server to change a GM level.\n");

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
        PRINTF("Please input an account name.\n");
        PRINTF("<example> id testname\n");
        LADMIN_LOG("No name given to search an account id ('id' command).\n");
        return;
    }

    if (!name.is_print())
    {
        return;
    }

    LADMIN_LOG("Request to login-server to know an account id.\n");

    WFIFOW(login_session, 0) = 0x7944;
    WFIFO_STRING(login_session, 2, name, 24);
    WFIFOSET(login_session, 26);
    bytes_to_read = 1;
}

//----------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its id)
//----------------------------------------------------------------------------
static
void infoaccount(int account_id)
{
    if (account_id < 0)
    {
        PRINTF("Please input a positive value for the id.\n");
        LADMIN_LOG("Negative value was given to found the account.\n");
        return;
    }

    LADMIN_LOG("Request to login-server to obtain information about an account (by its id).\n");

    WFIFOW(login_session, 0) = 0x7954;
    WFIFOL(login_session, 2) = account_id;
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
        PRINTF("Please input a message.\n");
        {
            PRINTF("<example> kami a message\n");
        }
        LADMIN_LOG("The message is void ('kami' command).\n");
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
    list_first = 0;
    list_last = 0;

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
        if (list_first < 0)
            list_first = 0;
        if (list_last < list_first || list_last < 0)
            list_last = 0;
    }

    LADMIN_LOG("Request to login-server to obtain the list of accounts from %d to %d.\n",
          list_first, list_last);

    WFIFOW(login_session, 0) = 0x7920;
    WFIFOL(login_session, 2) = list_first;
    WFIFOL(login_session, 6) = list_last;
    WFIFOSET(login_session, 10);
    bytes_to_read = 1;

    //          0123456789 01 01234567890123456789012301234 012345 0123456789012345678901234567
    Iprintf("account_id GM user_name               sex    count state\n");
    Iprintf("-------------------------------------------------------------------------------\n");
    list_count = 0;
}

//--------------------------------------------------------
// Sub-function: Frobnicate items
//--------------------------------------------------------
static
int itemfrob(ZString param)
{
    int source_id, dest_id;

    if (!extract(param, record<' '>(&source_id, &dest_id)))
    {
        PRINTF("You must provide the source and destination item IDs.\n");
        return 1;
    }

    WFIFOW(login_session, 0) = 0x7924;
    WFIFOL(login_session, 2) = source_id;
    WFIFOL(login_session, 6) = dest_id;
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
        PRINTF("Please input an account name and a memo.\n");
        PRINTF("<example> memo testname new memo\n");
        LADMIN_LOG("Incomplete parameters to change the memo of an account ('email' command).\n");
        return;
    }

    if (!name.is_print())
        return;

    size_t len = memo.size();
    size_t len1 = len + 1;
    if (len > 254)
    {
        PRINTF("Memo is too long (%zu characters).\n", len);
        PRINTF("Please input a memo of 254 bytes at the maximum.\n");
        LADMIN_LOG("Email is too long (%zu characters). Please input a memo of 254 bytes at the maximum.\n",
              len);
        return;
    }

    LADMIN_LOG("Request to login-server to change a memo.\n");

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
void nameaccount(int id)
{
    if (id < 0)
    {
        PRINTF("Please input a positive value for the id.\n");
        LADMIN_LOG("Negativ id given to search an account name ('name' command).\n");
        return;
    }

    LADMIN_LOG("Request to login-server to know an account name.\n");

    WFIFOW(login_session, 0) = 0x7946;
    WFIFOL(login_session, 2) = id;
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
        PRINTF("Please input an account name.\n");
        PRINTF("<example> password testname newpassword\n");
        LADMIN_LOG("Incomplete parameters to change the password of an account ('password' command).\n");
        return;
    }

    if (!name.is_print())
    {
        return;
    }

    if (!password.is_print())
        return;

    LADMIN_LOG("Request to login-server to change a password.\n");

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

    LADMIN_LOG("Request to reload the GM configuration file sended.\n");
    PRINTF("Request to reload the GM configuration file sended.\n");
    PRINTF("Check the actual GM accounts (after reloading):\n");
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
        PRINTF("Please input an account name and a sex.\n");
        PRINTF("<example> sex testname Male\n");
        LADMIN_LOG("Incomplete parameters to change the sex of an account ('sex' command).\n");
        return;
    }
    char sex = sex_.front();

    if (!name.is_print())
    {
        PRINTF("bad name\n");
        return;
    }

    if (!XString("MF").contains(sex))
    {
        PRINTF("Illegal gender [%c]. Please input M or F.\n", sex);
        LADMIN_LOG("Illegal gender [%c]. Please input M or F.\n", sex);
        return;
    }

    LADMIN_LOG("Request to login-server to change a sex.\n");

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
        PRINTF("Please input one of these states:\n");
        PRINTF("  0 = Account ok            6 = Your Game's EXE file is not the latest version\n");
        PRINTF("  1 = Unregistered ID       7 = You are Prohibited to log in until + message\n");
        PRINTF("  2 = Incorrect Password    8 = Server is jammed due to over populated\n");
        PRINTF("  3 = This ID is expired    9 = No MSG\n");
        PRINTF("  4 = Rejected from Server  100 = This ID has been totally erased\n");
        PRINTF("  5 = You have been blocked by the GM Team\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Invalid value for the state of an account ('state', 'block' or 'unblock' command).\n");
        return;
    }

    if (!name.is_print())
        return;

    if (state != 7)
    {
        error_message = "-";
    }
    else
    {
        if (error_message.size() < 1)
        {
            PRINTF("Error message is too short. Please input a message of 1-19 bytes.\n");
            LADMIN_LOG("Error message is too short. Please input a message of 1-19 bytes.\n");
            return;
        }
        if (error_message.size() > 19)
        {
            PRINTF("Error message is too long. Please input a message of 1-19 bytes.\n");
            LADMIN_LOG("Error message is too long. Please input a message of 1-19 bytes.\n");
            return;
        }
    }

    LADMIN_LOG("Request to login-server to change a state.\n");

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
        PRINTF("Please input an account name and a state.\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Incomplete parameters to change the state of an account ('state' command).\n");
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
        PRINTF("Please input an account name.\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Incomplete parameters to change the state of an account ('unblock' command).\n");
        return;
    }

    changestatesub(name, 0, "-");   // state 0, no error message
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
        PRINTF("Please input an account name.\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Incomplete parameters to change the state of an account ('block' command).\n");
        return;
    }

    changestatesub(name, 5, "-");   // state 5, no error message
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
        PRINTF("Please input an account name and a modifier.\n");
        PRINTF("  <example>: timeadd testname +1m-2mn1s-6y\n");
        PRINTF("             this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("             and 6 years at the same time.\n");
        LADMIN_LOG("Incomplete parameters to modify a limit time ('timeadd' command).\n");
        return;
    }
    if (name.is_print())
    {
        return;
    }

    if (!modif)
    {
        PRINTF("Please give an adjustment with this command:\n");
        PRINTF("  Adjustment value (-1, 1, +1, etc...)\n");
        PRINTF("  Modified element:\n");
        PRINTF("    a or y: year\n");
        PRINTF("    m:      month\n");
        PRINTF("    j or d: day\n");
        PRINTF("    h:      hour\n");
        PRINTF("    mn:     minute\n");
        PRINTF("    s:      second\n");
        PRINTF("  <example> timeadd testname +1m-2mn1s-6y\n");
        PRINTF("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("            and 6 years at the same time.\n");
        LADMIN_LOG("No adjustment isn't an adjustment ('timeadd' command).\n");
        return;
    }

    LADMIN_LOG("Request to login-server to modify a time limit.\n");

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
        PRINTF("Please input an account name, a date and a hour.\n");
        PRINTF("<example>: timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           timeset <account_name> 0   (0 = unlimited)\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a limit time ('timeset' command).\n");
        return;
    }
    if (!name.is_print())
        return;

    if (!time_)
        time_ = "23:59:59";

    if (date != "0"
        && ((!extract(date, record<'/'>(&year, &month, &day))
                && !extract(date, record<'-'>(&year, &month, &day))
                && !extract(date, record<'.'>(&year, &month, &day)))
            || !extract(time_, record<':'>(&hour, &minute, &second))))
    {
        PRINTF("Please input 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n");
        LADMIN_LOG("Invalid format for the date/time ('timeset' command).\n");
        return;
    }

    if (date == "0")
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
            PRINTF("Please give a correct value for the month (from 1 to 12).\n");
            LADMIN_LOG("Invalid month for the date ('timeset' command).\n");
            return;
        }
        month = month - 1;
        if (day < 1 || day > 31)
        {
            PRINTF("Please give a correct value for the day (from 1 to 31).\n");
            LADMIN_LOG("Invalid day for the date ('timeset' command).\n");
            return;
        }
        if (((month == 3 || month == 5 || month == 8 || month == 10)
             && day > 30) ||(month == 1 && day > 29))
        {
            PRINTF("Please give a correct value for a day of this month (%d).\n",
                 month);
            LADMIN_LOG("Invalid day for this month ('timeset' command).\n");
            return;
        }
        if (hour < 0 || hour > 23)
        {
            PRINTF("Please give a correct value for the hour (from 0 to 23).\n");
            LADMIN_LOG("Invalid hour for the time ('timeset' command).\n");
            return;
        }
        if (minute < 0 || minute > 59)
        {
            PRINTF("Please give a correct value for the minutes (from 0 to 59).\n");
            LADMIN_LOG("Invalid minute for the time ('timeset' command).\n");
            return;
        }
        if (second < 0 || second > 59)
        {
            PRINTF("Please give a correct value for the seconds (from 0 to 59).\n");
            LADMIN_LOG("Invalid second for the time ('timeset' command).\n");
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
            PRINTF("Invalid date.\n");
            PRINTF("Please add 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n");
            LADMIN_LOG("Invalid date. ('timeset' command).\n");
            return;
        }
    }

    LADMIN_LOG("Request to login-server to set a time limit.\n");

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
        PRINTF("Please input an account name.\n");
        PRINTF("<example> who testname\n");
        LADMIN_LOG("No name was given to found the account.\n");
        return;
    }
    if (!name.is_print())
    {
        return;
    }

    LADMIN_LOG("Request to login-server to obtain information about an account (by its name).\n");

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
    LADMIN_LOG("Request to login-server to obtain its version.\n");

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
        Iprintf("\n");
        Iprintf(SGR_GREEN "To list the commands, type 'enter'." SGR_RESET "\n");
        Iprintf(SGR_CYAN "Ladmin-> " SGR_RESET);
        Iprintf(SGR_BOLD);
        fflush(stdout);

        // get command and parameter
        // TODO figure out a better way to do stdio
        static auto cin = make_unique<io::ReadFile>(io::FD::stdin().dup());
        AString buf;
        cin->getline(buf);

        Iprintf(SGR_RESET);
        fflush(stdout);

        if (!cin->is_open())
            exit(0);

        if (!buf.is_print())
        {
            printf("Cowardly refusing to execute a command that includes control or non-ascii characters\n");
            LADMIN_LOG("Cowardly refusing to execute a command that includes control or non-ascii characters\n");
            continue;
        }

        // extract command name and parameters
        auto space = std::find(buf.begin(), buf.end(), ' ');
        AString command = buf.xislice_h(space);
        while (*space == ' ')
            ++space;

        parameters = buf.xislice_t(space);

        if (!command || command.startswith('?'))
            command = "help";

        if (!parameters)
        {
            LADMIN_LOG("Command: '%s' (without parameters)\n",
                    command);
        }
        else
        {
            // We don't want passwords in the log - Camel
            if (command == "create" || command == "add" || command == "password") {
                AString name, email_, password;
                VString<1> sex_;

                if (qsplit(parameters, &name, &sex_, &email_, &password))
                    LADMIN_LOG("Command: '%s', parameters: '%s %s %s ***'\n",
                            command, name, sex_, email_);
                else if (qsplit(parameters, &name, &sex_, &password))
                    LADMIN_LOG("Command: '%s', parameters: '%s %s ***'\n",
                            command, name, sex_);
                else if (qsplit(parameters, &name, &password))
                    LADMIN_LOG("Command: '%s', parameters: '%s ***'\n",
                            command, name);
                else
                    LADMIN_LOG("Command: '%s' (invalid parameters)\n", command);
            }
            else {
                LADMIN_LOG("Command: '%s', parameters: '%s'\n",
                        command, parameters);
            }
        }

        // Analyse of the command
        if (command == "help")
            display_help(parameters);
        else if (command == "add")
            addaccount(parameters, 0); // 0: no email
        else if (command == "ban")
            banaccount(parameters);
        else if (command == "banadd")
            banaddaccount(parameters);
        else if (command == "banset")
            bansetaccount(parameters);
        else if (command == "block")
            blockaccount(parameters);
        else if (command == "check")
            checkaccount(parameters);
        else if (command == "create")
            addaccount(parameters, 1); // 1: with email
        else if (command == "delete")
            delaccount(parameters);
        else if (command == "email")
            changeemail(parameters);
        else if (command == "getcount")
            getlogincount();
        else if (command == "gm")
            changegmlevel(parameters);
        else if (command == "id")
            idaccount(parameters);
        else if (command == "info")
            infoaccount(atoi(parameters.c_str()));
        else if (command == "kami")
            sendbroadcast(parameters);  // flag for normal
        else if (command == "itemfrob")
            itemfrob(parameters);  // 0: to list all
        else if (command == "list")
            listaccount(parameters, 0);    // 0: to list all
        else if (command == "listban")
            listaccount(parameters, 3);    // 3: to list only accounts with state or bannished
        else if (command == "listgm")
            listaccount(parameters, 1);    // 1: to list only GM
        else if (command == "listok")
            listaccount(parameters, 4);    // 4: to list only accounts without state and not bannished
        else if (command == "memo")
            changememo(parameters);
        else if (command == "name")
            nameaccount(atoi(parameters.c_str()));
        else if (command == "password")
            changepasswd(parameters);
        else if (command == "reloadgm")
            reloadGM(parameters);
        else if (command == "search")
            listaccount(parameters, 2);    // 2: to list with pattern
        else if (command == "sex")
            changesex(parameters);
        else if (command == "state")
            changestate(parameters);
        else if (command == "timeadd")
            timeaddaccount(parameters);
        else if (command == "timeset")
            timesetaccount(parameters);
        else if (command == "unban")
            unbanaccount(parameters);
        else if (command == "unblock")
            unblockaccount(parameters);
        else if (command == "version")
            checkloginversion();
        else if (command == "who")
            whoaccount(parameters);
        else if (command == "quit"
            || command == "exit"
            || command == "end")
        {
            PRINTF("Bye.\n");
            exit(0);
        }
        else
        {
            PRINTF("Unknown command [%s].\n", buf);
            LADMIN_LOG("Unknown command [%s].\n", buf);
        }
    }
}

//-------------------------------------------------------------
// Function: Parse receiving informations from the login-server
//-------------------------------------------------------------
static
void parse_fromlogin(Session *s)
{
//  PRINTF("parse_fromlogin : %d %d %d\n", fd, RFIFOREST(fd), RFIFOW(fd,0));

    while (RFIFOREST(s) >= 2)
    {
        switch (RFIFOW(s, 0))
        {
            case 0x7919:       // answer of a connection request
                if (RFIFOREST(s) < 3)
                    return;
                if (RFIFOB(s, 2) != 0)
                {
                    PRINTF("Error at login:\n");
                    PRINTF(" - incorrect password,\n");
                    PRINTF(" - administration system not activated, or\n");
                    PRINTF(" - unauthorised IP.\n");
                    LADMIN_LOG("Error at login: incorrect password, administration system not activated, or unauthorised IP.\n");
                    s->set_eof();
                    //bytes_to_read = 1; // not stop at prompt
                }
                else
                {
                    Iprintf("Established connection.\n");
                    LADMIN_LOG("Established connection.\n");
                    Iprintf("Reading of the version of the login-server...\n");
                    LADMIN_LOG("Reading of the version of the login-server...\n");
                    //bytes_to_read = 1; // unchanged
                    checkloginversion();
                }
                RFIFOSKIP(s, 3);
                break;

            case 0x7531:       // Displaying of the version of the login-server
                if (RFIFOREST(s) < 10)
                    return;
            {
                Iprintf("  Login-Server [%s:%d]\n",
                        login_ip, login_port);
                Version version;
                RFIFO_STRUCT(login_session, 2, version);
                Iprintf("  tmwA version %hhu.%hhu.%hhu (dev? %hhu) (flags %hhx) (which %hhx) (vend %hu)\n",
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
                    LADMIN_LOG("  Receiving of a void accounts list.\n");
                    if (list_count == 0)
                    {
                        Iprintf("No account found.\n");
                    }
                    else if (list_count == 1)
                    {
                        Iprintf("1 account found.\n");
                    }
                    else
                        Iprintf("%d accounts found.\n", list_count);
                    bytes_to_read = 0;
                }
                else
                {
                    int i;
                    LADMIN_LOG("  Receiving of a accounts list.\n");
                    for (i = 4; i < RFIFOW(s, 2); i += 38)
                    {
                        AccountName userid = stringish<AccountName>(RFIFO_STRING<24>(s, i + 5));
                        VString<23> lower_userid = userid.to_lower();
                        list_first = RFIFOL(s, i) + 1;
                        // here are checks...
                        if (list_type == 0
                            || (list_type == 1 && RFIFOB(s, i + 4) > 0)
                            || (list_type == 2 && lower_userid.contains_seq(parameters))
                            || (list_type == 3 && RFIFOL(s, i + 34) != 0)
                            || (list_type == 4 && RFIFOL(s, i + 34) == 0))
                        {
                            PRINTF("%10d ", RFIFOL(s, i));
                            if (RFIFOB(s, i + 4) == 0)
                                PRINTF("   ");
                            else
                                PRINTF("%2d ", RFIFOB(s, i + 4));
                            PRINTF("%-24s", userid);
                            if (RFIFOB(s, i + 29) == 0)
                                PRINTF("%-5s ", "Femal");
                            else if (RFIFOB(s, i + 29) == 1)
                                PRINTF("%-5s ", "Male");
                            else
                                PRINTF("%-5s ", "Servr");
                            PRINTF("%6d ", RFIFOL(s, i + 30));
                            switch (RFIFOL(s, i + 34))
                            {
                                case 0:
                                    PRINTF("%-27s\n", "Account OK");
                                    break;
                                case 1:
                                    PRINTF("%-27s\n", "Unregistered ID");
                                    break;
                                case 2:
                                    PRINTF("%-27s\n", "Incorrect Password");
                                    break;
                                case 3:
                                    PRINTF("%-27s\n", "This ID is expired");
                                    break;
                                case 4:
                                    PRINTF("%-27s\n",
                                            "Rejected from Server");
                                    break;
                                case 5:
                                    PRINTF("%-27s\n", "Blocked by the GM Team");   // You have been blocked by the GM Team
                                    break;
                                case 6:
                                    PRINTF("%-27s\n", "Your EXE file is too old"); // Your Game's EXE file is not the latest version
                                    break;
                                case 7:
                                    PRINTF("%-27s\n", "Banishement or");
                                    PRINTF("                                                   Prohibited to login until...\n");   // You are Prohibited to log in until %s
                                    break;
                                case 8:
                                    PRINTF("%-27s\n",
                                            "Server is over populated");
                                    break;
                                case 9:
                                    PRINTF("%-27s\n", "No MSG");
                                    break;
                                default:   // 100
                                    PRINTF("%-27s\n", "This ID is totally erased");    // This ID has been totally erased
                                    break;
                            }
                            list_count++;
                        }
                    }
                    // asking of the following acounts
                    LADMIN_LOG("Request to login-server to obtain the list of accounts from %d to %d (complement).\n",
                          list_first, list_last);
                    WFIFOW(login_session, 0) = 0x7920;
                    WFIFOL(login_session, 2) = list_first;
                    WFIFOL(login_session, 6) = list_last;
                    WFIFOSET(login_session, 10);
                    bytes_to_read = 1;
                }
                RFIFOSKIP(s, RFIFOW(s, 2));
                break;

            case 0x7931:       // Answer of login-server about an account creation
                if (RFIFOREST(s) < 30)
                    return;
            {
                int accid = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (accid == -1)
                {
                    PRINTF("Account [%s] creation failed. Same account already exists.\n",
                            name);
                    LADMIN_LOG("Account [%s] creation failed. Same account already exists.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s] is successfully created [id: %d].\n",
                            name, accid);
                    LADMIN_LOG("Account [%s] is successfully created [id: %d].\n",
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
                int accid = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (accid == -1)
                {
                    PRINTF("Account [%s] deletion failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] deletion failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] is successfully DELETED.\n",
                            name, accid);
                    LADMIN_LOG("Account [%s][id: %d] is successfully DELETED.\n",
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
                int accid = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (accid == -1)
                {
                    PRINTF("Account [%s] password changing failed.\n",
                            name);
                    PRINTF("Account [%s] doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account password changing failed. The compte [%s] doesn't exist.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] password successfully changed.\n",
                            name, accid);
                    LADMIN_LOG("Account [%s][id: %d] password successfully changed.\n",
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
                int accid = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                int state = RFIFOL(s, 30);
                if (accid == -1)
                {
                    PRINTF("Account [%s] state changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] state changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    MString tmpstr;
                    tmpstr += STRPRINTF(
                             "Account [%s] state successfully changed in [",
                             name);
                    switch (state)
                    {
                        case 0:
                            tmpstr += "0: Account OK";
                            break;
                        case 1:
                            tmpstr += "1: Unregistered ID";
                            break;
                        case 2:
                            tmpstr += "2: Incorrect Password";
                            break;
                        case 3:
                            tmpstr += "3: This ID is expired";
                            break;
                        case 4:
                            tmpstr += "4: Rejected from Server";
                            break;
                        case 5:
                            tmpstr += "5: You have been blocked by the GM Team";
                            break;
                        case 6:
                            tmpstr += "6: [Your Game's EXE file is not the latest version";
                            break;
                        case 7:
                            tmpstr += "7: You are Prohibited to log in until...";
                            break;
                        case 8:
                            tmpstr += "8: Server is jammed due to over populated";
                            break;
                        case 9:
                            tmpstr += "9: No MSG";
                            break;
                        default:   // 100
                            tmpstr += "100: This ID is totally erased";
                            break;
                    }
                    tmpstr += ']';
                    AString tmpstr_ = AString(tmpstr);
                    PRINTF("%s\n", tmpstr_);
                    LADMIN_LOG("%s\n", tmpstr_);
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
                    LADMIN_LOG("  Receiving of the number of online players.\n");
                    // Read information of the servers
                    if (RFIFOW(s, 2) < 5)
                    {
                        PRINTF("  No server is connected to the login-server.\n");
                    }
                    else
                    {
                        PRINTF("  Number of online players (server: number).\n");
                        // Displaying of result
                        for (int i = 4; i < RFIFOW(s, 2); i += 32)
                        {
                            ServerName name = stringish<ServerName>(RFIFO_STRING<20>(s, i + 6));
                            PRINTF("    %-20s : %5d\n", name,
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("The account [%s] doesn't exist or the password is incorrect.\n",
                            name);
                    LADMIN_LOG("The account [%s] doesn't exist or the password is incorrect.\n",
                            name);
                }
                else
                {
                    PRINTF("The proposed password is correct for the account [%s][id: %d].\n",
                            name, account_id);
                    LADMIN_LOG("The proposed password is correct for the account [%s][id: %d].\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] sex changing failed.\n",
                            name);
                    PRINTF("Account [%s] doesn't exist or the sex is already the good sex.\n",
                            name);
                    LADMIN_LOG("Account sex changing failed. The compte [%s] doesn't exist or the sex is already the good sex.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] sex successfully changed.\n",
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] sex successfully changed.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] GM level changing failed.\n",
                            name);
                    PRINTF("Account [%s] doesn't exist, the GM level is already the good GM level\n",
                            name);
                    PRINTF("or it's impossible to modify the GM accounts file.\n");
                    LADMIN_LOG("Account GM level changing failed. The compte [%s] doesn't exist, the GM level is already the good sex or it's impossible to modify the GM accounts file.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] GM level successfully changed.\n",
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] GM level successfully changed.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] e-mail changing failed.\n",
                            name);
                    PRINTF("Account [%s] doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account e-mail changing failed. The compte [%s] doesn't exist.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] e-mail successfully changed.\n",
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] e-mail successfully changed.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] memo changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] memo changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    PRINTF("Account [%s][id: %d] memo successfully changed.\n",
                            name, account_id);
                    LADMIN_LOG("Account [%s][id: %d] memo successfully changed.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Unable to find the account [%s] id. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Unable to find the account [%s] id. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    PRINTF("The account [%s] have the id: %d.\n",
                            name, account_id);
                    LADMIN_LOG("The account [%s] have the id: %d.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (!name)
                {
                    PRINTF("Unable to find the account [%d] name. Account doesn't exist.\n",
                            account_id);
                    LADMIN_LOG("Unable to find the account [%d] name. Account doesn't exist.\n",
                            account_id);
                }
                else
                {
                    PRINTF("The account [id: %d] have the name: %s.\n",
                            account_id, name);
                    LADMIN_LOG("The account [id: %d] have the name: %s.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (RFIFOL(s, 2) == -1)
                {
                    PRINTF("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Validity Limit of the account [%s][id: %d] successfully changed to [unlimited].\n",
                                name, account_id);
                        LADMIN_LOG("Validity Limit of the account [%s][id: %d] successfully changed to [unlimited].\n",
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Validity Limit of the account [%s][id: %d] successfully changed to be until %s.\n",
                                name, account_id, tmpstr);
                        LADMIN_LOG("Validity Limit of the account [%s][id: %d] successfully changed to be until %s.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n",
                                name, account_id);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n",
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n",
                                name, RFIFOL(s, 2), tmpstr);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n",
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
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n",
                                name, account_id);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n",
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n",
                                name, account_id,
                                tmpstr);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n",
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
                    PRINTF("Message sending failed. No online char-server.\n");
                    LADMIN_LOG("Message sending failed. No online char-server.\n");
                }
                else
                {
                    PRINTF("Message successfully sended to login-server.\n");
                    LADMIN_LOG("Message successfully sended to login-server.\n");
                }
                bytes_to_read = 0;
                RFIFOSKIP(s, 4);
                break;

            case 0x7951:       // answer of an account validity limit changing
                if (RFIFOREST(s) < 34)
                    return;
            {
                int account_id = RFIFOL(s, 2);
                AccountName name = stringish<AccountName>(RFIFO_STRING<24>(s, 6));
                if (account_id == -1)
                {
                    PRINTF("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(s, 30));
                    if (!timestamp)
                    {
                        PRINTF("Validity limit of the account [%s][id: %d] unchanged.\n",
                                name, account_id);
                        PRINTF("The account have an unlimited validity limit or\n");
                        PRINTF("the changing is impossible with the proposed adjustments.\n");
                        LADMIN_LOG("Validity limit of the account [%s][id: %d] unchanged. The account have an unlimited validity limit or the changing is impossible with the proposed adjustments.\n",
                                name, account_id);
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n",
                                name, account_id,
                                tmpstr);
                        LADMIN_LOG("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n",
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
                    int account_id = RFIFOL(s, 2);
                    uint8_t gm = RFIFOB(s, 6);
                    AccountName userid = stringish<AccountName>(RFIFO_STRING<24>(s, 7));
                    uint8_t sex = RFIFOB(s, 31);
                    int connections = RFIFOL(s, 32);
                    int state = RFIFOL(s, 36);
                    timestamp_seconds_buffer error_message = stringish<timestamp_seconds_buffer>(RFIFO_STRING<20>(s, 40));
                    timestamp_milliseconds_buffer lastlogin = stringish<timestamp_milliseconds_buffer>(RFIFO_STRING<24>(s, 60));
                    VString<15> last_ip_ = RFIFO_STRING<16>(s, 84);
                    AccountEmail email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 100));
                    TimeT connect_until_time = static_cast<time_t>(RFIFOL(s, 140));
                    TimeT ban_until_time = static_cast<time_t>(RFIFOL(s, 144));
                    AString memo = RFIFO_STRING(s, 150, RFIFOW(s, 148));
                    if (account_id == -1)
                    {
                        PRINTF("Unabled to find the account [%s]. Account doesn't exist.\n",
                                userid);
                        LADMIN_LOG("Unabled to find the account [%s]. Account doesn't exist.\n",
                                userid);
                    }
                    else if (!userid)
                    {
                        PRINTF("Unabled to find the account [id: %d]. Account doesn't exist.\n",
                                account_id);
                        LADMIN_LOG("Unabled to find the account [id: %d]. Account doesn't exist.\n",
                                account_id);
                    }
                    else
                    {
                        LADMIN_LOG("Receiving information about an account.\n");
                        PRINTF("The account is set with:\n");
                        if (!gm)
                        {
                            PRINTF(" Id:     %d (non-GM)\n", account_id);
                        }
                        else
                        {
                            PRINTF(" Id:     %d (GM level %d)\n",
                                    account_id, gm);
                        }
                        PRINTF(" Name:   '%s'\n", userid);
                        if (sex == 0)
                            PRINTF(" Sex:    Female\n");
                        else if (sex == 1)
                            PRINTF(" Sex:    Male\n");
                        else
                            PRINTF(" Sex:    Server\n");
                        PRINTF(" E-mail: %s\n", email);
                        switch (state)
                        {
                            case 0:
                                PRINTF(" Statut: 0 [Account OK]\n");
                                break;
                            case 1:
                                PRINTF(" Statut: 1 [Unregistered ID]\n");
                                break;
                            case 2:
                                PRINTF(" Statut: 2 [Incorrect Password]\n");
                                break;
                            case 3:
                                PRINTF(" Statut: 3 [This ID is expired]\n");
                                break;
                            case 4:
                                PRINTF(" Statut: 4 [Rejected from Server]\n");
                                break;
                            case 5:
                                PRINTF(" Statut: 5 [You have been blocked by the GM Team]\n");
                                break;
                            case 6:
                                PRINTF(" Statut: 6 [Your Game's EXE file is not the latest version]\n");
                                break;
                            case 7:
                                PRINTF(" Statut: 7 [You are Prohibited to log in until %s]\n",
                                     error_message);
                                break;
                            case 8:
                                PRINTF(" Statut: 8 [Server is jammed due to over populated]\n");
                                break;
                            case 9:
                                PRINTF(" Statut: 9 [No MSG]\n");
                                break;
                            default:   // 100
                                PRINTF(" Statut: %d [This ID is totally erased]\n",
                                        state);
                                break;
                        }
                        if (!ban_until_time)
                        {
                            PRINTF(" Banishment: not banished.\n");
                        }
                        else
                        {
                            timestamp_seconds_buffer tmpstr;
                            stamp_time(tmpstr, &ban_until_time);
                            PRINTF(" Banishment: until %s.\n", tmpstr);
                        }
                        if (connections > 1)
                            PRINTF(" Count:  %d connections.\n",
                                    connections);
                        else
                            PRINTF(" Count:  %d connection.\n",
                                    connections);
                        PRINTF(" Last connection at: %s (ip: %s)\n",
                                lastlogin, last_ip_);
                        if (!connect_until_time)
                        {
                            PRINTF(" Validity limit: unlimited.\n");
                        }
                        else
                        {
                            timestamp_seconds_buffer tmpstr;
                            stamp_time(tmpstr, &connect_until_time);
                            PRINTF(" Validity limit: until %s.\n",
                                    tmpstr);
                        }
                        PRINTF(" Memo:   '%s'\n", memo);
                    }
                }
                bytes_to_read = 0;
                RFIFOSKIP(s, 150 + RFIFOW(s, 148));
                break;

            default:
                PRINTF("Remote administration has been disconnected (unknown packet).\n");
                LADMIN_LOG("'End of connection, unknown packet.\n");
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
    Iprintf("Attempt to connect to login-server...\n");
    LADMIN_LOG("Attempt to connect to login-server...\n");

    login_session = make_connection(login_ip, login_port, SessionParsers{func_parse: parse_fromlogin, func_delete: delete_fromlogin});

    if (!login_session)
        return 0;

    {
        WFIFOW(login_session, 0) = 0x7918;  // Request for administation login
        WFIFOW(login_session, 2) = 0;   // no encrypted
        WFIFO_STRING(login_session, 4, admin_pass, 24);
        WFIFOSET(login_session, 28);
        bytes_to_read = 1;

        Iprintf("Sending of the password...\n");
        LADMIN_LOG("Sending of the password...\n");
    }

    return 0;
}

static
bool admin_confs(XString w1, ZString w2)
{
    {
        if (w1 == "login_ip")
        {
            struct hostent *h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                Iprintf("Login server IP address: %s -> %s\n",
                        w2, login_ip);
                login_ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
            }
        }
        else if (w1 == "login_port")
        {
            login_port = atoi(w2.c_str());
        }
        else if (w1 == "admin_pass")
        {
            admin_pass = stringish<AccountPass>(w2);
        }
        else if (w1 == "ladmin_log_filename")
        {
            ladmin_log_filename = w2;
        }
        else
        {
            PRINTF("WARNING: unknown ladmin config key: %s\n", AString(w1));
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

        Iprintf(SGR_RESET "----End of Ladmin (normal end with closing of all files).\n");
        LADMIN_LOG("----End of Ladmin (normal end with closing of all files).\n");

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
            if (argvi == "--help")
            {
                PRINTF("Usage: %s [--help] [--version] [files...]\n",
                        argv0);
                exit(0);
            }
            else if (argvi == "--version")
            {
                PRINTF("%s\n", CURRENT_VERSION_STRING);
                exit(0);
            }
            else
            {
                FPRINTF(stderr, "Unknown argument: %s\n", argvi);
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
        runflag &= load_config_file("conf/tmwa-admin.conf", admin_confs);

    eathena_interactive_session = isatty(0);

    LADMIN_LOG("");
    LADMIN_LOG("Configuration file readed.\n");

    Iprintf("EAthena login-server administration tool.\n");
    Version version = CURRENT_LOGIN_SERVER_VERSION;
    Iprintf("for tmwA version %hhu.%hhu.%hhu (dev? %hhu) (flags %hhx) (which %hhx) (vend %hu)\n",
            version.major, version.minor, version.patch,
            version.devel,

            version.flags, version.which,
            version.vend);

    LADMIN_LOG("Ladmin is ready.\n");
    Iprintf("Ladmin is " SGR_BOLD SGR_GREEN "ready" SGR_RESET ".\n\n");

    Connect_login_server();

    return 0;
}
