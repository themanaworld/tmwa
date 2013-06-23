#include <arpa/inet.h>

#include <netdb.h>
#include <unistd.h>

#include <cassert>

#include <fstream>

#include "../common/cxxstdio.hpp"
#include "../common/core.hpp"
#include "../common/md5calc.hpp"
#include "../common/socket.hpp"
#include "../common/version.hpp"
#include "../common/utils.hpp"

#include "../poison.hpp"

#define LADMIN_CONF_NAME        "conf/ladmin_athena.conf"

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
char loginserverip[16] = "127.0.0.1";   // IP of login-server
static
int loginserverport = 6900;    // Port of login-server
static
char loginserveradminpassword[24] = "admin";    // Administration password
static
int passenc = 2;               // Encoding type of the password
static
char ladmin_log_filename[1024] = "log/ladmin.log";
//-------------------------------------------------------------------------
//  LIST of COMMANDs that you can type at the prompt:
//    To use these commands you can only type only the first letters.
//    You must type a minimum of letters (you can not type 'a',
//      because ladmin doesn't know if it's for 'aide' or for 'add')
//    <Example> q <= quit, li <= list, pass <= passwd, etc.
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
//  ban/banish yyyy/mm/dd hh:mm:ss <account name>
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
//  del <account name>
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
//  list/ls [start_id [end_id]]
//    Display a list of accounts.
//    'start_id', 'end_id': indicate end and start identifiers.
//    Research by name is not possible with this command.
//    <example> list 10 9999999
//
//  listBan/lsBan [start_id [end_id]]
//    Like list/ls, but only for accounts with state or banished
//
//  listGM/lsGM [start_id [end_id]]
//    Like list/ls, but only for GM accounts
//
//  listOK/lsOK [start_id [end_id]]
//    Like list/ls, but only for accounts without state and not banished
//
//  memo <account_name> <memo>
//    Modify the memo of an account.
//    'memo': it can have until 253 characters (with spaces or not).
//
//  name <account_id>
//    Give the name of an account.
//
//  passwd <account_name> <new_password>
//    Change the password of an account.
//    When new password is omitted, the input is done without displaying of the pressed keys.
//
//  quit/end/exit
//    End of the program of administration
//
//  reloadGM
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
int login_fd;
static
int login_ip;
static
int bytes_to_read = 0;         // flag to know if we waiting bytes from login-server
static
char parameters[1024]; // needs to be global since it's passed to the parse function
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
    ladmin_log(static_cast<const std::string&>(STRPRINTF(fmt, ## __VA_ARGS__)))
static
void ladmin_log(const_string line)
{
    FILE *logfp = fopen_(ladmin_log_filename, "a");
    if (!logfp)
        return;
    log_with_timestamp(logfp, line);
    fclose_(logfp);
}

//---------------------------------------------
// Function to return ordonal text of a number.
//---------------------------------------------
static
const char *makeordinal(int number)
{
    if ((number % 10) < 4 && (number % 10) != 0
        && (number < 10 || number > 20))
    {
        if ((number % 10) == 1)
            return "st";
        else if ((number % 10) == 2)
            return "nd";
        else
            return "rd";
    }
    else
    {
        return "th";
    }
}

//-----------------------------------------------------------------------------------------
// Function to test of the validity of an account name (return 0 if incorrect, and 1 if ok)
//-----------------------------------------------------------------------------------------
static
int verify_accountname(const char *account_name)
{
    int i;

    for (i = 0; account_name[i]; i++)
    {
        if (account_name[i] < 32)
        {
            PRINTF("Illegal character found in the account name (%d%s character).\n",
                 i + 1, makeordinal(i + 1));
            LADMIN_LOG("Illegal character found in the account name (%d%s character).\n",
                  i + 1, makeordinal(i + 1));
            return 0;
        }
    }

    if (strlen(account_name) < 4)
    {
        PRINTF("Account name is too short. Please input an account name of 4-23 bytes.\n");
        LADMIN_LOG("Account name is too short. Please input an account name of 4-23 bytes.\n");
        return 0;
    }

    if (strlen(account_name) > 23)
    {
        PRINTF("Account name is too long. Please input an account name of 4-23 bytes.\n");
        LADMIN_LOG("Account name is too long. Please input an account name of 4-23 bytes.\n");
        return 0;
    }

    return 1;
}

//----------------------------------
// Sub-function: Input of a password
//----------------------------------
static
int typepasswd(char *password)
{
    char password1[1023] {};
    char password2[1023] {};
    int letter;
    int i;

    LADMIN_LOG("No password was given. Request to obtain a password.\n");

    PRINTF("\033[1;36m Type the password > \033[0;32;42m");
    i = 0;
    while ((letter = getchar()) != '\n')
        password1[i++] = letter;
    PRINTF("\033[0m\033[1;36m Verify the password > \033[0;32;42m");
    i = 0;
    while ((letter = getchar()) != '\n')
        password2[i++] = letter;

    PRINTF("\033[0m");
    fflush(stdout);
    fflush(stdin);

    if (strcmp(password1, password2) != 0)
    {
        PRINTF("Password verification failed. Please input same password.\n");
        LADMIN_LOG("Password verification failed. Please input same password.\n");
        LADMIN_LOG("  First password: %s, second password: %s.\n",
                    password1, password2);
        return 0;
    }
    LADMIN_LOG("Typed password: %s.\n", password1);
    strcpy(password, password1);
    return 1;
}

//------------------------------------------------------------------------------------
// Sub-function: Test of the validity of password (return 0 if incorrect, and 1 if ok)
//------------------------------------------------------------------------------------
static
int verify_password(const char *password)
{
    int i;

    for (i = 0; password[i]; i++)
    {
        if (password[i] < 32)
        {
            PRINTF("Illegal character found in the password (%d%s character).\n",
                 i + 1, makeordinal(i + 1));
            LADMIN_LOG("Illegal character found in the password (%d%s character).\n",
                  i + 1, makeordinal(i + 1));
            return 0;
        }
    }

    if (strlen(password) < 4)
    {
        PRINTF("Account name is too short. Please input an account name of 4-23 bytes.\n");
        LADMIN_LOG("Account name is too short. Please input an account name of 4-23 bytes.\n");
        return 0;
    }

    if (strlen(password) > 23)
    {
        PRINTF("Password is too long. Please input a password of 4-23 bytes.\n");
        LADMIN_LOG("Password is too long. Please input a password of 4-23 bytes.\n");
        return 0;
    }

    return 1;
}

//------------------------------------------------------------------
// Sub-function: Check the name of a command (return complete name)
//-----------------------------------------------------------------
static
int check_command(char *command)
{
// help
    if (strncmp(command, "help", 1) == 0
             && strncmp(command, "help", strlen(command)) == 0)
        strcpy(command, "help");
// general commands
    else if (strncmp(command, "add", 2) == 0 && strncmp(command, "add", strlen(command)) == 0)
        strcpy(command, "add");
    else if ((strncmp(command, "ban", 3) == 0
              && strncmp(command, "ban", strlen(command)) == 0)
             ||(strncmp(command, "banish", 4) == 0
                 && strncmp(command, "banish", strlen(command)) == 0))
        strcpy(command, "ban");
    else if ((strncmp(command, "banadd", 4) == 0 && strncmp(command, "banadd", strlen(command)) == 0) || // not 1 letter command: 'ba' or 'bs'? 'banadd' or 'banset' ?
             strcmp(command, "ba") == 0)
        strcpy(command, "banadd");
    else if ((strncmp(command, "banset", 4) == 0 && strncmp(command, "banset", strlen(command)) == 0) || // not 1 letter command: 'ba' or 'bs'? 'banadd' or 'banset' ?
             strcmp(command, "bs") == 0)
        strcpy(command, "banset");
    else if (strncmp(command, "block", 2) == 0
             && strncmp(command, "block", strlen(command)) == 0)
        strcpy(command, "block");
    else if (strncmp(command, "check", 2) == 0 && strncmp(command, "check", strlen(command)) == 0)   // not 1 letter command: 'check' or 'create'?
        strcpy(command, "check");
    else if (strncmp(command, "create", 2) == 0 && strncmp(command, "create", strlen(command)) == 0) // not 1 letter command: 'check' or 'create'?
        strcpy(command, "create");
    else if (strncmp(command, "delete", 1) == 0
             && strncmp(command, "delete", strlen(command)) == 0)
        strcpy(command, "delete");
    else if ((strncmp(command, "email", 2) == 0 && strncmp(command, "email", strlen(command)) == 0) ||   // not 1 letter command: 'email', 'end' or 'exit'?
             (strncmp(command, "e-mail", 2) == 0
              && strncmp(command, "e-mail", strlen(command)) == 0))
        strcpy(command, "email");
    else if (strncmp(command, "getcount", 2) == 0 && strncmp(command, "getcount", strlen(command)) == 0) // not 1 letter command: 'getcount' or 'gm'?
        strcpy(command, "getcount");
//  else if (strncmp(command, "gm", 2) == 0 && strncmp(command, "gm", strlen(command)) == 0) // not 1 letter command: 'getcount' or 'gm'?
//      strcpy(command, "gm");
//  else if (strncmp(command, "id", 2) == 0 && strncmp(command, "id", strlen(command)) == 0) // not 1 letter command: 'id' or 'info'?
//      strcpy(command, "id");
    else if (strncmp(command, "info", 2) == 0 && strncmp(command, "info", strlen(command)) == 0) // not 1 letter command: 'id' or 'info'?
        strcpy(command, "info");
//  else if (strncmp(command, "kami", 4) == 0 && strncmp(command, "kami", strlen(command)) == 0) // only all letters command: 'kami' or 'kamib'?
//      strcpy(command, "kami");
//  else if (strncmp(command, "kamib", 5) == 0 && strncmp(command, "kamib", strlen(command)) == 0) // only all letters command: 'kami' or 'kamib'?
//      strcpy(command, "kamib");
    else if ((strncmp(command, "list", 2) == 0 && strncmp(command, "list", strlen(command)) == 0) || // 'list' is default list command
             strcmp(command, "ls") == 0)
        strcpy(command, "list");
    else if (strncmp(command, "itemfrob", 6) == 0)
        strcpy(command, "itemfrob");
    else if ((strncmp(command, "listban", 5) == 0
              && strncmp(command, "listban", strlen(command)) == 0)
             ||(strncmp(command, "lsban", 3) == 0
                 && strncmp(command, "lsban", strlen(command)) == 0)
             || strcmp(command, "lb") == 0)
        strcpy(command, "listban");
    else if ((strncmp(command, "listgm", 5) == 0
              && strncmp(command, "listgm", strlen(command)) == 0)
             ||(strncmp(command, "lsgm", 3) == 0
                 && strncmp(command, "lsgm", strlen(command)) == 0)
             || strcmp(command, "lg") == 0)
        strcpy(command, "listgm");
    else if ((strncmp(command, "listok", 5) == 0
              && strncmp(command, "listok", strlen(command)) == 0)
             ||(strncmp(command, "lsok", 3) == 0
                 && strncmp(command, "lsok", strlen(command)) == 0)
             || strcmp(command, "lo") == 0)
        strcpy(command, "listok");
    else if (strncmp(command, "memo", 1) == 0
             && strncmp(command, "memo", strlen(command)) == 0)
        strcpy(command, "memo");
    else if (strncmp(command, "name", 1) == 0
             && strncmp(command, "name", strlen(command)) == 0)
        strcpy(command, "name");
    else if ((strncmp(command, "password", 1) == 0
              && strncmp(command, "password", strlen(command)) == 0)
             || strcmp(command, "passwd") == 0)
        strcpy(command, "password");
    else if (strncmp(command, "reloadgm", 1) == 0
             && strncmp(command, "reloadgm", strlen(command)) == 0)
        strcpy(command, "reloadgm");
    else if (strncmp(command, "search", 3) == 0 && strncmp(command, "search", strlen(command)) == 0) // not 1 letter command: 'search', 'state' or 'sex'?
        strcpy(command, "search"); // not 2 letters command: 'search' or 'sex'?
//  else if (strncmp(command, "sex", 3) == 0 && strncmp(command, "sex", strlen(command)) == 0) // not 1 letter command: 'search', 'state' or 'sex'?
//      strcpy(command, "sex"); // not 2 letters command: 'search' or 'sex'?
    else if (strncmp(command, "state", 2) == 0 && strncmp(command, "state", strlen(command)) == 0)   // not 1 letter command: 'search', 'state' or 'sex'?
        strcpy(command, "state");
    else if ((strncmp(command, "timeadd", 5) == 0 && strncmp(command, "timeadd", strlen(command)) == 0) ||   // not 1 letter command: 'ta' or 'ts'? 'timeadd' or 'timeset'?
             strcmp(command, "ta") == 0)
        strcpy(command, "timeadd");
    else if ((strncmp(command, "timeset", 5) == 0 && strncmp(command, "timeset", strlen(command)) == 0) ||   // not 1 letter command: 'ta' or 'ts'? 'timeadd' or 'timeset'?
             strcmp(command, "ts") == 0)
        strcpy(command, "timeset");
    else if ((strncmp(command, "unban", 5) == 0
              && strncmp(command, "unban", strlen(command)) == 0)
             ||(strncmp(command, "unbanish", 4) == 0
                 && strncmp(command, "unbanish", strlen(command)) == 0))
        strcpy(command, "unban");
    else if (strncmp(command, "unblock", 4) == 0
             && strncmp(command, "unblock", strlen(command)) == 0)
        strcpy(command, "unblock");
    else if (strncmp(command, "version", 1) == 0
             && strncmp(command, "version", strlen(command)) == 0)
        strcpy(command, "version");
    else if (strncmp(command, "who", 1) == 0
             && strncmp(command, "who", strlen(command)) == 0)
        strcpy(command, "who");
// quit
    else if (strncmp(command, "quit", 1) == 0
             && strncmp(command, "quit", strlen(command)) == 0)
        strcpy(command, "quit");
    else if (strncmp(command, "exit", 2) == 0 && strncmp(command, "exit", strlen(command)) == 0) // not 1 letter command: 'email', 'end' or 'exit'?
        strcpy(command, "exit");
    else if (strncmp(command, "end", 2) == 0 && strncmp(command, "end", strlen(command)) == 0)   // not 1 letter command: 'email', 'end' or 'exit'?
        strcpy(command, "end");

    return 0;
}

//-----------------------------------------
// Sub-function: Display commands of ladmin
//-----------------------------------------
static
void display_help(const char *param)
{
    char command[1023] {};
    int i;

    if (sscanf(param, "%s ", command) < 1 || strlen(command) == 0)
        strcpy(command, "");   // any value that is not a command

    if (command[0] == '?')
    {
        strcpy(command, "help");
    }

    // lowercase for command
    for (i = 0; command[i]; i++)
        command[i] = tolower(command[i]);

    // Analyse of the command
    check_command(command);    // give complete name to the command

    LADMIN_LOG("Displaying of the commands or a command.\n");

    if (strcmp(command, "help") == 0)
    {
        PRINTF("help/?\n");
        PRINTF("  Display the description of the commands\n");
        PRINTF("help/? [command]\n");
        PRINTF("  Display the description of the specified command\n");
// general commands
    }
    else if (strcmp(command, "add") == 0)
    {
        PRINTF("add <account_name> <sex> <password>\n");
        PRINTF("  Create an account with the default email (a@a.com).\n");
        PRINTF("  Concerning the sex, only the first letter is used (F or M).\n");
        PRINTF("  The e-mail is set to a@a.com (default e-mail). It's like to have no e-mail.\n");
        PRINTF("  When the password is omitted,\n");
        PRINTF("  the input is done without displaying of the pressed keys.\n");
        PRINTF("  <example> add testname Male testpass\n");
    }
    else if (strcmp(command, "ban") == 0)
    {
        PRINTF("ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("  Changes the final date of a banishment of an account.\n");
        PRINTF("  Like banset, but <account name> is at end.\n");
    }
    else if (strcmp(command, "banadd") == 0)
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
    else if (strcmp(command, "banset") == 0)
    {
        PRINTF("banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("  Changes the final date of a banishment of an account.\n");
        PRINTF("  Default time [hh:mm:ss]: 23:59:59.\n");
        PRINTF("banset <account_name> 0\n");
        PRINTF("  Set a non-banished account (0 = unbanished).\n");
    }
    else if (strcmp(command, "block") == 0)
    {
        PRINTF("block <account name>\n");
        PRINTF("  Set state 5 (You have been blocked by the GM Team) to an account.\n");
        PRINTF("  This command works like state <account_name> 5.\n");
    }
    else if (strcmp(command, "check") == 0)
    {
        PRINTF("check <account_name> <password>\n");
        PRINTF("  Check the validity of a password for an account.\n");
        PRINTF("  NOTE: Server will never sends back a password.\n");
        PRINTF("        It's the only method you have to know if a password is correct.\n");
        PRINTF("        The other method is to have a ('physical') access to the accounts file.\n");
    }
    else if (strcmp(command, "create") == 0)
    {
        PRINTF("create <account_name> <sex> <email> <password>\n");
        PRINTF("  Like the 'add' command, but with e-mail moreover.\n");
        PRINTF("  <example> create testname Male my@mail.com testpass\n");
    }
    else if (strcmp(command, "delete") == 0)
    {
        PRINTF("del <account name>\n");
        PRINTF("  Remove an account.\n");
        PRINTF("  This order requires confirmation. After confirmation, the account is deleted.\n");
    }
    else if (strcmp(command, "email") == 0)
    {
        PRINTF("email <account_name> <email>\n");
        PRINTF("  Modify the e-mail of an account.\n");
    }
    else if (strcmp(command, "getcount") == 0)
    {
        PRINTF("getcount\n");
        PRINTF("  Give the number of players online on all char-servers.\n");
    }
    else if (strcmp(command, "gm") == 0)
    {
        PRINTF("gm <account_name> [GM_level]\n");
        PRINTF("  Modify the GM level of an account.\n");
        PRINTF("  Default value remove GM level (GM level = 0).\n");
        PRINTF("  <example> gm testname 80\n");
    }
    else if (strcmp(command, "id") == 0)
    {
        PRINTF("id <account name>\n");
        PRINTF("  Give the id of an account.\n");
    }
    else if (strcmp(command, "info") == 0)
    {
        PRINTF("info <account_id>\n");
        PRINTF("  Display complete information of an account.\n");
    }
    else if (strcmp(command, "kami") == 0)
    {
        PRINTF("kami <message>\n");
        PRINTF("  Sends a broadcast message on all map-server (in yellow).\n");
    }
    else if (strcmp(command, "kamib") == 0)
    {
        PRINTF("kamib <message>\n");
        PRINTF("  Sends a broadcast message on all map-server (in blue).\n");
    }
    else if (strcmp(command, "list") == 0)
    {
        PRINTF("list/ls [start_id [end_id]]\n");
        PRINTF("  Display a list of accounts.\n");
        PRINTF("  'start_id', 'end_id': indicate end and start identifiers.\n");
        PRINTF("  Research by name is not possible with this command.\n");
        PRINTF("  <example> list 10 9999999\n");
    }
    else if (strcmp(command, "itemfrob") == 0)
    {
        PRINTF("itemfrob <source-id> <dest-id>\n");
        PRINTF("  Translates item IDs for all accounts.\n");
        PRINTF("  Any items matching the source item ID will be mapped to the dest-id.\n");
        PRINTF("  <example> itemfrob 500 700\n");
    }
    else if (strcmp(command, "listban") == 0)
    {
        PRINTF("listBan/lsBan [start_id [end_id]]\n");
        PRINTF("  Like list/ls, but only for accounts with state or banished.\n");
    }
    else if (strcmp(command, "listgm") == 0)
    {
        PRINTF("listGM/lsGM [start_id [end_id]]\n");
        PRINTF("  Like list/ls, but only for GM accounts.\n");
    }
    else if (strcmp(command, "listok") == 0)
    {
        PRINTF("listOK/lsOK [start_id [end_id]]\n");
        PRINTF("  Like list/ls, but only for accounts without state and not banished.\n");
    }
    else if (strcmp(command, "memo") == 0)
    {
        PRINTF("memo <account_name> <memo>\n");
        PRINTF("  Modify the memo of an account.\n");
        PRINTF("  'memo': it can have until 253 characters (with spaces or not).\n");
    }
    else if (strcmp(command, "name") == 0)
    {
        PRINTF("name <account_id>\n");
        PRINTF("  Give the name of an account.\n");
    }
    else if (strcmp(command, "password") == 0)
    {
        PRINTF("passwd <account_name> <new_password>\n");
        PRINTF("  Change the password of an account.\n");
        PRINTF("  When new password is omitted,\n");
        PRINTF("  the input is done without displaying of the pressed keys.\n");
    }
    else if (strcmp(command, "reloadgm") == 0)
    {
        PRINTF("reloadGM\n");
        PRINTF("  Reload GM configuration file\n");
    }
    else if (strcmp(command, "search") == 0)
    {
        PRINTF("search <expression>\n");
        PRINTF("  Seek accounts.\n");
        PRINTF("  Displays the accounts whose names correspond.\n");
//          PRINTF("search -r/-e/--expr/--regex <expression>\n");
//          PRINTF("  Seek accounts by regular expression.\n");
//          PRINTF("  Displays the accounts whose names correspond.\n");
    }
    else if (strcmp(command, "sex") == 0)
    {
        PRINTF("sex <account_name> <sex>\n");
        PRINTF("  Modify the sex of an account.\n");
        PRINTF("  <example> sex testname Male\n");
    }
    else if (strcmp(command, "state") == 0)
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
    else if (strcmp(command, "timeadd") == 0)
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
    else if (strcmp(command, "timeadd") == 0)
    {
        PRINTF("timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("  Changes the validity limit of an account.\n");
        PRINTF("  Default time [hh:mm:ss]: 23:59:59.\n");
        PRINTF("timeset <account_name> 0\n");
        PRINTF("  Gives an unlimited validity limit (0 = unlimited).\n");
    }
    else if (strcmp(command, "unban") == 0)
    {
        PRINTF("unban/unbanish <account name>\n");
        PRINTF("  Remove the banishment of an account.\n");
        PRINTF("  This command works like banset <account_name> 0.\n");
    }
    else if (strcmp(command, "unblock") == 0)
    {
        PRINTF("unblock <account name>\n");
        PRINTF("  Set state 0 (Account ok) to an account.\n");
        PRINTF("  This command works like state <account_name> 0.\n");
    }
    else if (strcmp(command, "version") == 0)
    {
        PRINTF("version\n");
        PRINTF("  Display the version of the login-server.\n");
    }
    else if (strcmp(command, "who") == 0)
    {
        PRINTF("who <account name>\n");
        PRINTF("  Displays complete information of an account.\n");
// quit
    }
    else if (strcmp(command, "quit") == 0 ||
             strcmp(command, "exit") == 0 ||
             strcmp(command, "end") == 0)
    {
        PRINTF("quit/end/exit\n");
        PRINTF("  End of the program of administration.\n");
// unknown command
    }
    else
    {
        if (strlen(command) > 0)
            PRINTF("Unknown command [%s] for help. Displaying of all commands.\n",
                 command);
        PRINTF(" help/?                          -- Display this help\n");
        PRINTF(" help/? [command]                -- Display the help of the command\n");
        PRINTF(" add <account_name> <sex> <password>  -- Create an account with default email\n");
        PRINTF(" ban/banish yyyy/mm/dd hh:mm:ss <account name> -- Change final date of a ban\n");
        PRINTF(" banadd/ba <account_name> <modifier>  -- Add or substract time from the final\n");
        PRINTF("   example: ba apple +1m-2mn1s-2y        date of a banishment of an account\n");
        PRINTF(" banset/bs <account_name> yyyy/mm/dd [hh:mm:ss] -- Change final date of a ban\n");
        PRINTF(" banset/bs <account_name> 0           -- Un-banish an account\n");
        PRINTF(" block <account name>     -- Set state 5 (blocked by the GM Team) to an account\n");
        PRINTF(" check <account_name> <password>      -- Check the validity of a password\n");
        PRINTF(" create <account_name> <sex> <email> <passwrd> -- Create an account with email\n");
        PRINTF(" del <account name>                   -- Remove an account\n");
        PRINTF(" email <account_name> <email>         -- Modify an email of an account\n");
        PRINTF(" getcount                             -- Give the number of players online\n");
        PRINTF(" gm <account_name> [GM_level]         -- Modify the GM level of an account\n");
        PRINTF(" id <account name>                    -- Give the id of an account\n");
        PRINTF(" info <account_id>                    -- Display all information of an account\n");
        PRINTF(" itemfrob <source-id> <dest-id>       -- Map all items from one item ID to another\n");
        PRINTF(" kami <message>                       -- Sends a broadcast message (in yellow)\n");
        PRINTF(" kamib <message>                      -- Sends a broadcast message (in blue)\n");
        PRINTF(" list/ls [First_id [Last_id]]         -- Display a list of accounts\n");
        PRINTF(" listBan/lsBan [First_id [Last_id] ]  -- Display a list of accounts\n");
        PRINTF("                                         with state or banished\n");
        PRINTF(" listGM/lsGM [First_id [Last_id]]     -- Display a list of GM accounts\n");
        PRINTF(" listOK/lsOK [First_id [Last_id] ]    -- Display a list of accounts\n");
        PRINTF("                                         without state and not banished\n");
        PRINTF(" memo <account_name> <memo>           -- Modify the memo of an account\n");
        PRINTF(" name <account_id>                    -- Give the name of an account\n");
        PRINTF(" passwd <account_name> <new_password> -- Change the password of an account\n");
        PRINTF(" quit/end/exit                        -- End of the program of administation\n");
        PRINTF(" reloadGM                             -- Reload GM configuration file\n");
        PRINTF(" search <expression>                  -- Seek accounts\n");
//          PRINTF(" search -e/-r/--expr/--regex <expressn> -- Seek accounts by regular-expression\n");
        PRINTF(" sex <nomcompte> <sexe>               -- Modify the sex of an account\n");
        PRINTF(" state <account_name> <new_state> <error_message_#7> -- Change the state\n");
        PRINTF(" timeadd/ta <account_name> <modifier> -- Add or substract time from the\n");
        PRINTF("   example: ta apple +1m-2mn1s-2y        validity limit of an account\n");
        PRINTF(" timeset/ts <account_name> yyyy/mm/dd [hh:mm:ss] -- Change the validify limit\n");
        PRINTF(" timeset/ts <account_name> 0          -- Give a unlimited validity limit\n");
        PRINTF(" unban/unbanish <account name>        -- Remove the banishment of an account\n");
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
int addaccount(const char *param, int emailflag)
{
    char name[1023] {};
    char sex[1023] {};
    char email[1023] {};
    char password[1023] {};

    if (emailflag == 0)
    {                           // add command
        if (sscanf(param, "\"%[^\"]\" %s %[^\r\n]", name, sex, password) < 2 &&    // password can be void
            sscanf(param, "'%[^']' %s %[^\r\n]", name, sex, password) < 2 &&   // password can be void
            sscanf(param, "%s %s %[^\r\n]", name, sex, password) < 2)
        {                       // password can be void
            PRINTF("Please input an account name, a sex and a password.\n");
            PRINTF("<example> add testname Male testpass\n");
            LADMIN_LOG("Incomplete parameters to create an account ('add' command).\n");
            return 136;
        }
        strcpy(email, "a@a.com");  // default email
    }
    else
    {                           // 1: create command
        if (sscanf(param, "\"%[^\"]\" %s %s %[^\r\n]", name, sex, email, password) < 3 &&  // password can be void
            sscanf(param, "'%[^']' %s %s %[^\r\n]", name, sex, email, password) < 3 && // password can be void
            sscanf(param, "%s %s %s %[^\r\n]", name, sex, email,
                    password) < 3)
        {                       // password can be void
            PRINTF("Please input an account name, a sex and a password.\n");
            PRINTF("<example> create testname Male my@mail.com testpass\n");
            LADMIN_LOG("Incomplete parameters to create an account ('create' command).\n");
            return 136;
        }
    }
    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    sex[0] = toupper(sex[0]);
    if (strchr("MF", sex[0]) == NULL)
    {
        PRINTF("Illegal gender [%s]. Please input M or F.\n", sex);
        LADMIN_LOG("Illegal gender [%s]. Please input M or F.\n",
                    sex);
        return 103;
    }

    if (strlen(email) < 3)
    {
        PRINTF("Email is too short [%s]. Please input a valid e-mail.\n",
                email);
        LADMIN_LOG("Email is too short [%s]. Please input a valid e-mail.\n",
              email);
        return 109;
    }
    if (strlen(email) > 39)
    {
        PRINTF("Email is too long [%s]. Please input an e-mail with 39 bytes at the most.\n",
             email);
        LADMIN_LOG("Email is too long [%s]. Please input an e-mail with 39 bytes at the most.\n",
              email);
        return 109;
    }
    if (e_mail_check(email) == 0)
    {
        PRINTF("Invalid email [%s]. Please input a valid e-mail.\n",
                email);
        LADMIN_LOG("Invalid email [%s]. Please input a valid e-mail.\n",
                     email);
        return 109;
    }

    if (strlen(password) == 0)
    {
        if (typepasswd(password) == 0)
            return 108;
    }
    if (verify_password(password) == 0)
        return 104;

    LADMIN_LOG("Request to login-server to create an account.\n");

    WFIFOW(login_fd, 0) = 0x7930;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFO_STRING(login_fd, 26, password, 24);
    WFIFOB(login_fd, 50) = sex[0];
    WFIFO_STRING(login_fd, 51, email, 40);
    WFIFOSET(login_fd, 91);
    bytes_to_read = 1;

    return 0;
}

//---------------------------------------------------------------------------------
// Sub-function: Add/substract time to the final date of a banishment of an account
//---------------------------------------------------------------------------------
static
int banaddaccount(const char *param)
{
    char name[1023] {};
    char modif[1023] {};
    int year, month, day, hour, minute, second;
    const char *p_modif;
    int value, i;

    year = month = day = hour = minute = second = 0;

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, modif) < 2 &&
        sscanf(param, "'%[^']' %[^\r\n]", name, modif) < 2 &&
        sscanf(param, "%s %[^\r\n]", name, modif) < 2)
    {
        PRINTF("Please input an account name and a modifier.\n");
        PRINTF("  <example>: banadd testname +1m-2mn1s-6y\n");
        PRINTF("             this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("             and 6 years at the same time.\n");
        LADMIN_LOG("Incomplete parameters to modify the ban date/time of an account ('banadd' command).\n");
        return 136;
    }
    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    // lowercase for modif
    for (i = 0; modif[i]; i++)
        modif[i] = tolower(modif[i]);
    p_modif = modif;
    while (strlen(p_modif) > 0)
    {
        value = atoi(p_modif);
        if (value == 0)
        {
            p_modif++;
        }
        else
        {
            if (p_modif[0] == '-' || p_modif[0] == '+')
                p_modif++;
            while (strlen(p_modif) > 0 && p_modif[0] >= '0'
                   && p_modif[0] <= '9')
            {
                p_modif++;
            }
            if (p_modif[0] == 's')
            {
                second = value;
                p_modif++;
            }
            else if (p_modif[0] == 'm' && p_modif[1] == 'n')
            {
                minute = value;
                p_modif += 2;
            }
            else if (p_modif[0] == 'h')
            {
                hour = value;
                p_modif++;
            }
            else if (p_modif[0] == 'd' || p_modif[0] == 'j')
            {
                day = value;
                p_modif += 2;
            }
            else if (p_modif[0] == 'm')
            {
                month = value;
                p_modif++;
            }
            else if (p_modif[0] == 'y' || p_modif[0] == 'a')
            {
                year = value;
                p_modif++;
            }
            else
            {
                p_modif++;
            }
        }
    }

    PRINTF(" year:   %d\n", year);
    PRINTF(" month:  %d\n", month);
    PRINTF(" day:    %d\n", day);
    PRINTF(" hour:   %d\n", hour);
    PRINTF(" minute: %d\n", minute);
    PRINTF(" second: %d\n", second);

    if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0
        && second == 0)
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
        return 137;
    }
    if (year > 127 || year < -127)
    {
        PRINTF("Please give a correct adjustment for the years (from -127 to 127).\n");
        LADMIN_LOG("Abnormal adjustement for the year ('banadd' command).\n");
        return 137;
    }
    if (month > 255 || month < -255)
    {
        PRINTF("Please give a correct adjustment for the months (from -255 to 255).\n");
        LADMIN_LOG("Abnormal adjustement for the month ('banadd' command).\n");
        return 137;
    }
    if (day > 32767 || day < -32767)
    {
        PRINTF("Please give a correct adjustment for the days (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the days ('banadd' command).\n");
        return 137;
    }
    if (hour > 32767 || hour < -32767)
    {
        PRINTF("Please give a correct adjustment for the hours (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the hours ('banadd' command).\n");
        return 137;
    }
    if (minute > 32767 || minute < -32767)
    {
        PRINTF("Please give a correct adjustment for the minutes (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the minutes ('banadd' command).\n");
        return 137;
    }
    if (second > 32767 || second < -32767)
    {
        PRINTF("Please give a correct adjustment for the seconds (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the seconds ('banadd' command).\n");
        return 137;
    }

    LADMIN_LOG("Request to login-server to modify a ban date/time.\n");

    WFIFOW(login_fd, 0) = 0x794c;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOW(login_fd, 26) = year;
    WFIFOW(login_fd, 28) = month;
    WFIFOW(login_fd, 30) = day;
    WFIFOW(login_fd, 32) = hour;
    WFIFOW(login_fd, 34) = minute;
    WFIFOW(login_fd, 36) = second;
    WFIFOSET(login_fd, 38);
    bytes_to_read = 1;

    return 0;
}

//-----------------------------------------------------------------------
// Sub-function of sub-function banaccount, unbanaccount or bansetaccount
// Set the final date of a banishment of an account
//-----------------------------------------------------------------------
static
int bansetaccountsub(const char *name, const char *date, const char *time_)
{
    int year, month, day, hour, minute, second;
    year = month = day = hour = minute = second = 0;

    // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
    TimeT ban_until_time = TimeT();
    struct tm tmtime = ban_until_time;   // initialize

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (atoi(date) != 0 &&
        ((sscanf(date, "%d/%d/%d", &year, &month, &day) < 3 &&
          sscanf(date, "%d-%d-%d", &year, &month, &day) < 3 &&
          sscanf(date, "%d.%d.%d", &year, &month, &day) < 3) ||
         sscanf(time_, "%d:%d:%d", &hour, &minute, &second) < 3))
    {
        PRINTF("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n");
        PRINTF("You can imput 0 instead of if you use 'banset' command.\n");
        LADMIN_LOG("Invalid format for the date/time ('banset' or 'ban' command).\n");
        return 102;
    }

    if (atoi(date) == 0)
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
            return 102;
        }
        month = month - 1;
        if (day < 1 || day > 31)
        {
            PRINTF("Please give a correct value for the day (from 1 to 31).\n");
            LADMIN_LOG("Invalid day for the date ('banset' or 'ban' command).\n");
            return 102;
        }
        if (((month == 3 || month == 5 || month == 8 || month == 10)
             && day > 30) ||(month == 1 && day > 29))
        {
            PRINTF("Please give a correct value for a day of this month (%d).\n",
                 month);
            LADMIN_LOG("Invalid day for this month ('banset' or 'ban' command).\n");
            return 102;
        }
        if (hour < 0 || hour > 23)
        {
            PRINTF("Please give a correct value for the hour (from 0 to 23).\n");
            LADMIN_LOG("Invalid hour for the time ('banset' or 'ban' command).\n");
            return 102;
        }
        if (minute < 0 || minute > 59)
        {
            PRINTF("Please give a correct value for the minutes (from 0 to 59).\n");
            LADMIN_LOG("Invalid minute for the time ('banset' or 'ban' command).\n");
            return 102;
        }
        if (second < 0 || second > 59)
        {
            PRINTF("Please give a correct value for the seconds (from 0 to 59).\n");
            LADMIN_LOG("Invalid second for the time ('banset' or 'ban' command).\n");
            return 102;
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
            return 102;
        }
    }

    LADMIN_LOG("Request to login-server to set a ban.\n");

    WFIFOW(login_fd, 0) = 0x794a;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOL(login_fd, 26) = static_cast<time_t>(ban_until_time);
    WFIFOSET(login_fd, 30);
    bytes_to_read = 1;

    return 0;
}

//---------------------------------------------------------------------
// Sub-function: Set the final date of a banishment of an account (ban)
//---------------------------------------------------------------------
static
int banaccount(const char *param)
{
    char name[1023] {};
    char date[1023] {};
    char time_[1023] {};

    if (sscanf(param, "%s %s \"%[^\"]\"", date, time_, name) < 3 &&
        sscanf(param, "%s %s '%[^']'", date, time_, name) < 3 &&
        sscanf(param, "%s %s %[^\r\n]", date, time_, name) < 3)
    {
        PRINTF("Please input an account name, a date and a hour.\n");
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n");
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("           unban/unbanish <account name>\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a ban ('banset' or 'ban' command).\n");
        return 136;
    }

    return bansetaccountsub(name, date, time_);
}

//------------------------------------------------------------------------
// Sub-function: Set the final date of a banishment of an account (banset)
//------------------------------------------------------------------------
static
int bansetaccount(const char *param)
{
    char name[1023] {};
    char date[1023] {};
    char time_[1023] {};

    if (sscanf(param, "\"%[^\"]\" %s %[^\r\n]", name, date, time_) < 2 &&   // if date = 0, time_ can be void
        sscanf(param, "'%[^']' %s %[^\r\n]", name, date, time_) < 2 &&  // if date = 0, time_ can be void
        sscanf(param, "%s %s %[^\r\n]", name, date, time_) < 2)
    {                           // if date = 0, time_ can be void
        PRINTF("Please input an account name, a date and a hour.\n");
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n");
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("           unban/unbanish <account name>\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a ban ('banset' or 'ban' command).\n");
        return 136;
    }

    if (time_[0] == '\0')
        strcpy(time_, "23:59:59");

    return bansetaccountsub(name, date, time_);
}

//-------------------------------------------------
// Sub-function: unbanishment of an account (unban)
//-------------------------------------------------
static
int unbanaccount(const char *param)
{
    char name[1023] {};

    if (strlen(param) == 0 ||
        (sscanf(param, "\"%[^\"]\"", name) < 1 &&
         sscanf(param, "'%[^']'", name) < 1 &&
         sscanf(param, "%[^\r\n]", name) < 1) || strlen(name) == 0)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           banset <account_name> 0   (0 = un-banished)\n");
        PRINTF("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
        PRINTF("           unban/unbanish <account name>\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a ban ('unban' command).\n");
        return 136;
    }

    return bansetaccountsub(name, "0", "");
}

//---------------------------------------------------------
// Sub-function: Asking to check the validity of a password
// (Note: never send back a password with login-server!! security of passwords)
//---------------------------------------------------------
static
int checkaccount(const char *param)
{
    char name[1023] {};
    char password[1023] {};

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, password) < 1 &&    // password can be void
        sscanf(param, "'%[^']' %[^\r\n]", name, password) < 1 &&   // password can be void
        sscanf(param, "%s %[^\r\n]", name, password) < 1)
    {                           // password can be void
        PRINTF("Please input an account name.\n");
        PRINTF("<example> check testname password\n");
        LADMIN_LOG("Incomplete parameters to check the password of an account ('check' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (strlen(password) == 0)
    {
        if (typepasswd(password) == 0)
            return 134;
    }
    if (verify_password(password) == 0)
        return 131;

    LADMIN_LOG("Request to login-server to check a password.\n");

    WFIFOW(login_fd, 0) = 0x793a;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFO_STRING(login_fd, 26, password, 24);
    WFIFOSET(login_fd, 50);
    bytes_to_read = 1;

    return 0;
}

//------------------------------------------------
// Sub-function: Asking for deletion of an account
//------------------------------------------------
static
int delaccount(const char *param)
{
    char name[1023] {};
    char letter;
    int i;

    if (strlen(param) == 0 ||
        (sscanf(param, "\"%[^\"]\"", name) < 1 &&
         sscanf(param, "'%[^']'", name) < 1 &&
         sscanf(param, "%[^\r\n]", name) < 1) || strlen(name) == 0)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<example> del testnametodelete\n");
        LADMIN_LOG("No name given to delete an account ('delete' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    char confirm[1023] {};
    while (confirm[0] != 'n'
           && confirm[0] != 'y')
    {
        PRINTF("\033[1;36m ** Are you really sure to DELETE account [%s]? (y/n) > \033[0m", name);
        fflush(stdout);
        strzcpy(confirm, "", sizeof(confirm));
        i = 0;
        while ((letter = getchar()) != '\n')
            confirm[i++] = letter;
    }

    if (confirm[0] == 'n')
    {
        PRINTF("Deletion canceled.\n");
        LADMIN_LOG("Deletion canceled by user ('delete' command).\n");
        return 121;
    }

    LADMIN_LOG("Request to login-server to delete an acount.\n");

    WFIFOW(login_fd, 0) = 0x7932;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOSET(login_fd, 26);
    bytes_to_read = 1;

    return 0;
}

//----------------------------------------------------------
// Sub-function: Asking to modification of an account e-mail
//----------------------------------------------------------
static
int changeemail(const char *param)
{
    char name[1023] {};
    char email[1023] {};

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, email) < 2 &&
        sscanf(param, "'%[^']' %[^\r\n]", name, email) < 2 &&
        sscanf(param, "%s %[^\r\n]", name, email) < 2)
    {
        PRINTF("Please input an account name and an email.\n");
        PRINTF("<example> email testname newemail\n");
        LADMIN_LOG("Incomplete parameters to change the email of an account ('email' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (strlen(email) < 3)
    {
        PRINTF("Email is too short [%s]. Please input a valid e-mail.\n",
                email);
        LADMIN_LOG("Email is too short [%s]. Please input a valid e-mail.\n",
              email);
        return 109;
    }
    if (strlen(email) > 39)
    {
        PRINTF("Email is too long [%s]. Please input an e-mail with 39 bytes at the most.\n",
             email);
        LADMIN_LOG("Email is too long [%s]. Please input an e-mail with 39 bytes at the most.\n",
              email);
        return 109;
    }
    if (e_mail_check(email) == 0)
    {
        PRINTF("Invalid email [%s]. Please input a valid e-mail.\n",
                email);
        LADMIN_LOG("Invalid email [%s]. Please input a valid e-mail.\n",
                     email);
        return 109;
    }

    LADMIN_LOG("Request to login-server to change an email.\n");

    WFIFOW(login_fd, 0) = 0x7940;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFO_STRING(login_fd, 26, email, 40);
    WFIFOSET(login_fd, 66);
    bytes_to_read = 1;

    return 0;
}

//-----------------------------------------------------
// Sub-function: Asking of the number of online players
//-----------------------------------------------------
static
int getlogincount(void)
{
    LADMIN_LOG("Request to login-server to obtain the # of online players.\n");

    WFIFOW(login_fd, 0) = 0x7938;
    WFIFOSET(login_fd, 2);
    bytes_to_read = 1;

    return 0;
}

//----------------------------------------------------------
// Sub-function: Asking to modify the GM level of an account
//----------------------------------------------------------
static
int changegmlevel(const char *param)
{
    char name[1023] {};
    int GM_level = 0;

    if (sscanf(param, "\"%[^\"]\" %d", name, &GM_level) < 1 &&
        sscanf(param, "'%[^']' %d", name, &GM_level) < 1 &&
        sscanf(param, "%s %d", name, &GM_level) < 1)
    {
        PRINTF("Please input an account name and a GM level.\n");
        PRINTF("<example> gm testname 80\n");
        LADMIN_LOG("Incomplete parameters to change the GM level of an account ('gm' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (GM_level < 0 || GM_level > 99)
    {
        PRINTF("Illegal GM level [%d]. Please input a value from 0 to 99.\n",
             GM_level);
        LADMIN_LOG("Illegal GM level [%d]. The value can be from 0 to 99.\n",
              GM_level);
        return 103;
    }

    LADMIN_LOG("Request to login-server to change a GM level.\n");

    WFIFOW(login_fd, 0) = 0x793e;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOB(login_fd, 26) = GM_level;
    WFIFOSET(login_fd, 27);
    bytes_to_read = 1;

    return 0;
}

//---------------------------------------------
// Sub-function: Asking to obtain an account id
//---------------------------------------------
static
int idaccount(const char *param)
{
    char name[1023] {};

    if (strlen(param) == 0 ||
        (sscanf(param, "\"%[^\"]\"", name) < 1 &&
         sscanf(param, "'%[^']'", name) < 1 &&
         sscanf(param, "%[^\r\n]", name) < 1) || strlen(name) == 0)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<example> id testname\n");
        LADMIN_LOG("No name given to search an account id ('id' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    LADMIN_LOG("Request to login-server to know an account id.\n");

    WFIFOW(login_fd, 0) = 0x7944;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOSET(login_fd, 26);
    bytes_to_read = 1;

    return 0;
}

//----------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its id)
//----------------------------------------------------------------------------
static
int infoaccount(int account_id)
{
    if (account_id < 0)
    {
        PRINTF("Please input a positive value for the id.\n");
        LADMIN_LOG("Negative value was given to found the account.\n");
        return 136;
    }

    LADMIN_LOG("Request to login-server to obtain information about an account (by its id).\n");

    WFIFOW(login_fd, 0) = 0x7954;
    WFIFOL(login_fd, 2) = account_id;
    WFIFOSET(login_fd, 6);
    bytes_to_read = 1;

    return 0;
}

//---------------------------------------
// Sub-function: Send a broadcast message
//---------------------------------------
static
int sendbroadcast(short type, const char *message)
{
    if (strlen(message) == 0)
    {
        PRINTF("Please input a message.\n");
        if (type == 0)
        {
            PRINTF("<example> kami a message\n");
        }
        else
        {
            PRINTF("<example> kamib a message\n");
        }
        LADMIN_LOG("The message is void ('kami(b)' command).\n");
        return 136;
    }

    WFIFOW(login_fd, 0) = 0x794e;
    WFIFOW(login_fd, 2) = type;
    size_t len = strlen(message) + 1;
    WFIFOL(login_fd, 4) = len;
    WFIFO_STRING(login_fd, 8, message, len);
    WFIFOSET(login_fd, 8 + len);
    bytes_to_read = 1;

    return 0;
}


//--------------------------------------------------------
// Sub-function: Asking to Displaying of the accounts list
//--------------------------------------------------------
static
int listaccount(char *param, int type)
{
//int list_first, list_last, list_type; // parameter to display a list of accounts
    int i;

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
        for (i = 0; param[i]; i++)
            param[i] = tolower(param[i]);
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
    {                           // if list (list_type == 0)
        switch (sscanf(param, "%d %d", &list_first, &list_last))
        {
            case 0:
                // get all accounts = use default
                break;
            case 1:
                list_last = 0;
                // use tests of the following value
                FALLTHROUGH;
            default:
                if (list_first < 0)
                    list_first = 0;
                if (list_last < list_first || list_last < 0)
                    list_last = 0;
                break;
        }
    }

    LADMIN_LOG("Request to login-server to obtain the list of accounts from %d to %d.\n",
          list_first, list_last);

    WFIFOW(login_fd, 0) = 0x7920;
    WFIFOL(login_fd, 2) = list_first;
    WFIFOL(login_fd, 6) = list_last;
    WFIFOSET(login_fd, 10);
    bytes_to_read = 1;

    //          0123456789 01 01234567890123456789012301234 012345 0123456789012345678901234567
    Iprintf("account_id GM user_name               sex    count state\n");
    Iprintf("-------------------------------------------------------------------------------\n");
    list_count = 0;

    return 0;
}

//--------------------------------------------------------
// Sub-function: Frobnicate items
//--------------------------------------------------------
static
int itemfrob(const char *param)
{
    int source_id, dest_id;

    if (sscanf(param, "%d %d", &source_id, &dest_id) < 2)
    {
        PRINTF("You must provide the source and destination item IDs.\n");
        return 1;
    }

    WFIFOW(login_fd, 0) = 0x7924;
    WFIFOL(login_fd, 2) = source_id;
    WFIFOL(login_fd, 6) = dest_id;
    WFIFOSET(login_fd, 10);
    bytes_to_read = 1;          // all logging is done to the three main servers

    return 0;
}

//--------------------------------------------
// Sub-function: Asking to modify a memo field
//--------------------------------------------
static
int changememo(const char *param)
{
    char name[1023] {};
    char memo[1023] {};

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, memo) < 1 &&    // memo can be void
        sscanf(param, "'%[^']' %[^\r\n]", name, memo) < 1 &&   // memo can be void
        sscanf(param, "%s %[^\r\n]", name, memo) < 1)
    {                           // memo can be void
        PRINTF("Please input an account name and a memo.\n");
        PRINTF("<example> memo testname new memo\n");
        LADMIN_LOG("Incomplete parameters to change the memo of an account ('email' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    size_t len = strlen(memo);
    size_t len1 = len + 1;
    if (len > 254)
    {
        PRINTF("Memo is too long (%zu characters).\n", len);
        PRINTF("Please input a memo of 254 bytes at the maximum.\n");
        LADMIN_LOG("Email is too long (%zu characters). Please input a memo of 254 bytes at the maximum.\n",
              len);
        return 102;
    }

    LADMIN_LOG("Request to login-server to change a memo.\n");

    WFIFOW(login_fd, 0) = 0x7942;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOW(login_fd, 26) = len1;
    WFIFO_STRING(login_fd, 28, memo, len);
    WFIFOSET(login_fd, 28 + len1);
    bytes_to_read = 1;

    return 0;
}

//-----------------------------------------------
// Sub-function: Asking to obtain an account name
//-----------------------------------------------
static
int nameaccount(int id)
{
    if (id < 0)
    {
        PRINTF("Please input a positive value for the id.\n");
        LADMIN_LOG("Negativ id given to search an account name ('name' command).\n");
        return 136;
    }

    LADMIN_LOG("Request to login-server to know an account name.\n");

    WFIFOW(login_fd, 0) = 0x7946;
    WFIFOL(login_fd, 2) = id;
    WFIFOSET(login_fd, 6);
    bytes_to_read = 1;

    return 0;
}

//------------------------------------------
// Sub-function: Asking to modify a password
// (Note: never send back a password with login-server!! security of passwords)
//------------------------------------------
static
int changepasswd(const char *param)
{
    char name[1023] {};
    char password[1023] {};

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, password) < 1 &&
        sscanf(param, "'%[^']' %[^\r\n]", name, password) < 1 &&
        sscanf(param, "%s %[^\r\n]", name, password) < 1)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<example> passwd testname newpassword\n");
        LADMIN_LOG("Incomplete parameters to change the password of an account ('password' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (strlen(password) == 0)
    {
        if (typepasswd(password) == 0)
            return 134;
    }
    if (verify_password(password) == 0)
        return 131;

    LADMIN_LOG("Request to login-server to change a password.\n");

    WFIFOW(login_fd, 0) = 0x7934;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFO_STRING(login_fd, 26, password, 24);
    WFIFOSET(login_fd, 50);
    bytes_to_read = 1;

    return 0;
}

//----------------------------------------------------------------------
// Sub-function: Request to login-server to reload GM configuration file
// this function have no answer
//----------------------------------------------------------------------
static
int reloadGM(char *params)
{
    WFIFOW(login_fd, 0) = 0x7955;
    WFIFOSET(login_fd, 2);
    bytes_to_read = 0;

    LADMIN_LOG("Request to reload the GM configuration file sended.\n");
    PRINTF("Request to reload the GM configuration file sended.\n");
    PRINTF("Check the actual GM accounts (after reloading):\n");
    listaccount(params, 1);    // 1: to list only GM

    return 180;
}

//-----------------------------------------------------
// Sub-function: Asking to modify the sex of an account
//-----------------------------------------------------
static
int changesex(const char *param)
{
    char name[1023] {};
    char sex[1023] {};

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, sex) < 2 &&
        sscanf(param, "'%[^']' %[^\r\n]", name, sex) < 2 &&
        sscanf(param, "%s %[^\r\n]", name, sex) < 2)
    {
        PRINTF("Please input an account name and a sex.\n");
        PRINTF("<example> sex testname Male\n");
        LADMIN_LOG("Incomplete parameters to change the sex of an account ('sex' command).\n");
        return 136;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    sex[0] = toupper(sex[0]);
    if (strchr("MF", sex[0]) == NULL)
    {
        PRINTF("Illegal gender [%s]. Please input M or F.\n", sex);
        LADMIN_LOG("Illegal gender [%s]. Please input M or F.\n",
                    sex);
        return 103;
    }

    LADMIN_LOG("Request to login-server to change a sex.\n");

    WFIFOW(login_fd, 0) = 0x793c;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOB(login_fd, 26) = sex[0];
    WFIFOSET(login_fd, 27);
    bytes_to_read = 1;

    return 0;
}

//-------------------------------------------------------------------------
// Sub-function of sub-function changestate, blockaccount or unblockaccount
// Asking to modify the state of an account
//-------------------------------------------------------------------------
static
int changestatesub(const char *name, int state, const char *error_message7)
{
    const char *error_message = error_message7;

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
        return 151;
    }

    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (state != 7)
    {
        error_message = "-";
    }
    else
    {
        if (strlen(error_message) < 1)
        {
            PRINTF("Error message is too short. Please input a message of 1-19 bytes.\n");
            LADMIN_LOG("Error message is too short. Please input a message of 1-19 bytes.\n");
            return 102;
        }
        if (strlen(error_message) > 19)
        {
            PRINTF("Error message is too long. Please input a message of 1-19 bytes.\n");
            LADMIN_LOG("Error message is too long. Please input a message of 1-19 bytes.\n");
            return 102;
        }
    }

    LADMIN_LOG("Request to login-server to change a state.\n");

    WFIFOW(login_fd, 0) = 0x7936;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOL(login_fd, 26) = state;
    WFIFO_STRING(login_fd, 30, error_message, 20);
    WFIFOSET(login_fd, 50);
    bytes_to_read = 1;

    return 0;
}

//-------------------------------------------------------
// Sub-function: Asking to modify the state of an account
//-------------------------------------------------------
static
int changestate(const char *param)
{
    char name[1023] {};
    char error_message[1023] {};
    int state;

    if (sscanf(param, "\"%[^\"]\" %d %[^\r\n]", name, &state, error_message) < 2
        && sscanf(param, "'%[^']' %d %[^\r\n]", name, &state, error_message) < 2
        && sscanf(param, "%s %d %[^\r\n]", name, &state, error_message) < 2)
    {
        PRINTF("Please input an account name and a state.\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Incomplete parameters to change the state of an account ('state' command).\n");
        return 136;
    }

    return changestatesub(name, state, error_message);
}

//-------------------------------------------
// Sub-function: Asking to unblock an account
//-------------------------------------------
static
int unblockaccount(const char *param)
{
    char name[1023] {};

    if (strlen(param) == 0 ||
        (sscanf(param, "\"%[^\"]\"", name) < 1 &&
         sscanf(param, "'%[^']'", name) < 1 &&
         sscanf(param, "%[^\r\n]", name) < 1) || strlen(name) == 0)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Incomplete parameters to change the state of an account ('unblock' command).\n");
        return 136;
    }

    return changestatesub(name, 0, "-");   // state 0, no error message
}

//-------------------------------------------
// Sub-function: Asking to unblock an account
//-------------------------------------------
static
int blockaccount(const char *param)
{
    char name[1023] {};

    if (strlen(param) == 0 ||
        (sscanf(param, "\"%[^\"]\"", name) < 1 &&
         sscanf(param, "'%[^']'", name) < 1 &&
         sscanf(param, "%[^\r\n]", name) < 1) || strlen(name) == 0)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<examples> state testname 5\n");
        PRINTF("           state testname 7 end of your ban\n");
        PRINTF("           block <account name>\n");
        PRINTF("           unblock <account name>\n");
        LADMIN_LOG("Incomplete parameters to change the state of an account ('block' command).\n");
        return 136;
    }

    return changestatesub(name, 5, "-");   // state 5, no error message
}

//---------------------------------------------------------------------
// Sub-function: Add/substract time to the validity limit of an account
//---------------------------------------------------------------------
static
int timeaddaccount(const char *param)
{
    char name[1023] {};
    char modif[1023] {};
    int year, month, day, hour, minute, second;
    const char *p_modif;
    int value, i;

    year = month = day = hour = minute = second = 0;

    if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, modif) < 2 &&
        sscanf(param, "'%[^']' %[^\r\n]", name, modif) < 2 &&
        sscanf(param, "%s %[^\r\n]", name, modif) < 2)
    {
        PRINTF("Please input an account name and a modifier.\n");
        PRINTF("  <example>: timeadd testname +1m-2mn1s-6y\n");
        PRINTF("             this example adds 1 month and 1 second, and substracts 2 minutes\n");
        PRINTF("             and 6 years at the same time.\n");
        LADMIN_LOG("Incomplete parameters to modify a limit time ('timeadd' command).\n");
        return 136;
    }
    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    // lowercase for modif
    for (i = 0; modif[i]; i++)
        modif[i] = tolower(modif[i]);
    p_modif = modif;
    while (strlen(p_modif) > 0)
    {
        value = atoi(p_modif);
        if (value == 0)
        {
            p_modif++;
        }
        else
        {
            if (p_modif[0] == '-' || p_modif[0] == '+')
                p_modif++;
            while (strlen(p_modif) > 0 && p_modif[0] >= '0'
                   && p_modif[0] <= '9')
            {
                p_modif++;
            }
            if (p_modif[0] == 's')
            {
                second = value;
                p_modif++;
            }
            else if (p_modif[0] == 'm' && p_modif[1] == 'n')
            {
                minute = value;
                p_modif += 2;
            }
            else if (p_modif[0] == 'h')
            {
                hour = value;
                p_modif++;
            }
            else if (p_modif[0] == 'd' || p_modif[0] == 'j')
            {
                day = value;
                p_modif += 2;
            }
            else if (p_modif[0] == 'm')
            {
                month = value;
                p_modif++;
            }
            else if (p_modif[0] == 'y' || p_modif[0] == 'a')
            {
                year = value;
                p_modif++;
            }
            else
            {
                p_modif++;
            }
        }
    }

    PRINTF(" year:   %d\n", year);
    PRINTF(" month:  %d\n", month);
    PRINTF(" day:    %d\n", day);
    PRINTF(" hour:   %d\n", hour);
    PRINTF(" minute: %d\n", minute);
    PRINTF(" second: %d\n", second);

    if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0
        && second == 0)
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
        return 137;
    }
    if (year > 127 || year < -127)
    {
        PRINTF("Please give a correct adjustment for the years (from -127 to 127).\n");
        LADMIN_LOG("Abnormal adjustement for the year ('timeadd' command).\n");
        return 137;
    }
    if (month > 255 || month < -255)
    {
        PRINTF("Please give a correct adjustment for the months (from -255 to 255).\n");
        LADMIN_LOG("Abnormal adjustement for the month ('timeadd' command).\n");
        return 137;
    }
    if (day > 32767 || day < -32767)
    {
        PRINTF("Please give a correct adjustment for the days (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the days ('timeadd' command).\n");
        return 137;
    }
    if (hour > 32767 || hour < -32767)
    {
        PRINTF("Please give a correct adjustment for the hours (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the hours ('timeadd' command).\n");
        return 137;
    }
    if (minute > 32767 || minute < -32767)
    {
        PRINTF("Please give a correct adjustment for the minutes (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the minutes ('timeadd' command).\n");
        return 137;
    }
    if (second > 32767 || second < -32767)
    {
        PRINTF("Please give a correct adjustment for the seconds (from -32767 to 32767).\n");
        LADMIN_LOG("Abnormal adjustement for the seconds ('timeadd' command).\n");
        return 137;
    }

    LADMIN_LOG("Request to login-server to modify a time limit.\n");

    WFIFOW(login_fd, 0) = 0x7950;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOW(login_fd, 26) = year;
    WFIFOW(login_fd, 28) = month;
    WFIFOW(login_fd, 30) = day;
    WFIFOW(login_fd, 32) = hour;
    WFIFOW(login_fd, 34) = minute;
    WFIFOW(login_fd, 36) = second;
    WFIFOSET(login_fd, 38);
    bytes_to_read = 1;

    return 0;
}

//-------------------------------------------------
// Sub-function: Set a validity limit of an account
//-------------------------------------------------
static
int timesetaccount(const char *param)
{
    char name[1023] {};
    char date[1023] {};
    char time_[1023] {};
    int year, month, day, hour, minute, second;

    year = month = day = hour = minute = second = 0;

    // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
    TimeT connect_until_time = TimeT();
    struct tm tmtime = connect_until_time;   // initialize

    if (sscanf(param, "\"%[^\"]\" %s %[^\r\n]", name, date, time_) < 2 &&   // if date = 0, time_ can be void
        sscanf(param, "'%[^']' %s %[^\r\n]", name, date, time_) < 2 &&  // if date = 0, time_ can be void
        sscanf(param, "%s %s %[^\r\n]", name, date, time_) < 2)
    {                           // if date = 0, time_ can be void
        PRINTF("Please input an account name, a date and a hour.\n");
        PRINTF("<example>: timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
        PRINTF("           timeset <account_name> 0   (0 = unlimited)\n");
        PRINTF("           Default time [hh:mm:ss]: 23:59:59.\n");
        LADMIN_LOG("Incomplete parameters to set a limit time ('timeset' command).\n");
        return 136;
    }
    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    if (time_[0] == '\0')
        strcpy(time_, "23:59:59");

    if (atoi(date) != 0 &&
        ((sscanf(date, "%d/%d/%d", &year, &month, &day) < 3 &&
          sscanf(date, "%d-%d-%d", &year, &month, &day) < 3 &&
          sscanf(date, "%d.%d.%d", &year, &month, &day) < 3 &&
          sscanf(date, "%d'%d'%d", &year, &month, &day) < 3) ||
         sscanf(time_, "%d:%d:%d", &hour, &minute, &second) < 3))
    {
        PRINTF("Please input 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n");
        LADMIN_LOG("Invalid format for the date/time ('timeset' command).\n");
        return 102;
    }

    if (atoi(date) == 0)
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
            return 102;
        }
        month = month - 1;
        if (day < 1 || day > 31)
        {
            PRINTF("Please give a correct value for the day (from 1 to 31).\n");
            LADMIN_LOG("Invalid day for the date ('timeset' command).\n");
            return 102;
        }
        if (((month == 3 || month == 5 || month == 8 || month == 10)
             && day > 30) ||(month == 1 && day > 29))
        {
            PRINTF("Please give a correct value for a day of this month (%d).\n",
                 month);
            LADMIN_LOG("Invalid day for this month ('timeset' command).\n");
            return 102;
        }
        if (hour < 0 || hour > 23)
        {
            PRINTF("Please give a correct value for the hour (from 0 to 23).\n");
            LADMIN_LOG("Invalid hour for the time ('timeset' command).\n");
            return 102;
        }
        if (minute < 0 || minute > 59)
        {
            PRINTF("Please give a correct value for the minutes (from 0 to 59).\n");
            LADMIN_LOG("Invalid minute for the time ('timeset' command).\n");
            return 102;
        }
        if (second < 0 || second > 59)
        {
            PRINTF("Please give a correct value for the seconds (from 0 to 59).\n");
            LADMIN_LOG("Invalid second for the time ('timeset' command).\n");
            return 102;
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
            return 102;
        }
    }

    LADMIN_LOG("Request to login-server to set a time limit.\n");

    WFIFOW(login_fd, 0) = 0x7948;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOL(login_fd, 26) = static_cast<time_t>(connect_until_time);
    WFIFOSET(login_fd, 30);
    bytes_to_read = 1;

    return 0;
}

//------------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its name)
//------------------------------------------------------------------------------
static
int whoaccount(const char *param)
{
    char name[1023] {};

    if (strlen(param) == 0 ||
        (sscanf(param, "\"%[^\"]\"", name) < 1 &&
         sscanf(param, "'%[^']'", name) < 1 &&
         sscanf(param, "%[^\r\n]", name) < 1) || strlen(name) == 0)
    {
        PRINTF("Please input an account name.\n");
        PRINTF("<example> who testname\n");
        LADMIN_LOG("No name was given to found the account.\n");
        return 136;
    }
    if (verify_accountname(name) == 0)
    {
        return 102;
    }

    LADMIN_LOG("Request to login-server to obtain information about an account (by its name).\n");

    WFIFOW(login_fd, 0) = 0x7952;
    WFIFO_STRING(login_fd, 2, name, 24);
    WFIFOSET(login_fd, 26);
    bytes_to_read = 1;

    return 0;
}

//--------------------------------------------------------
// Sub-function: Asking of the version of the login-server
//--------------------------------------------------------
static
int checkloginversion(void)
{
    LADMIN_LOG("Request to login-server to obtain its version.\n");

    WFIFOW(login_fd, 0) = 0x7530;
    WFIFOSET(login_fd, 2);
    bytes_to_read = 1;

    return 0;
}

//---------------------------------------------
// Prompt function
// this function wait until user type a command
// and analyse the command.
//---------------------------------------------
static
int prompt(void)
{
    int i, j;
    char buf[1024];
    char *p;

    // while we don't wait new packets
    while (bytes_to_read == 0)
    {
        // for help with the console colors look here:
        // http://www.edoceo.com/liberum/?doc=PRINTF-with-color
        // some code explanation (used here):
        // \033[2J : clear screen and go up/left (0, 0 position)
        // \033[K  : clear line from actual position to end of the line
        // \033[0m : reset color parameter
        // \033[1m : use bold for font
        Iprintf("\n");
        Iprintf("\033[32mTo list the commands, type 'enter'.\033[0m\n");
        Iprintf("\033[0;36mLadmin-> \033[0m");
        Iprintf("\033[1m");
        fflush(stdout);

        // get command and parameter
        strzcpy(buf, "", sizeof(buf));
        fflush(stdin);
        if (!fgets(buf, 1023, stdin))
            exit(0);
        buf[1023] = '\0';

        Iprintf("\033[0m");
        fflush(stdout);

        if (!eathena_interactive_session && !strlen(buf))
            exit(0);

        // remove final \n
        if ((p = strrchr(buf, '\n')) != NULL)
            p[0] = '\0';
        // remove all control char
        for (i = 0; buf[i]; i++)
            if (buf[i] < 32)
            {
                // remove cursor control.
                if (buf[i] == 27
                    && buf[i + 1] == '['
                    && (buf[i + 2] == 'H' // home position (cursor)
                        || buf[i + 2] == 'J' // clear screen
                        || buf[i + 2] == 'A' // up 1 line
                        || buf[i + 2] == 'B' // down 1 line
                        || buf[i + 2] == 'C' // right 1 position
                        || buf[i + 2] == 'D' // left 1 position
                        || buf[i + 2] == 'G')) // center cursor (windows)
                {
                    for (j = i; buf[j]; j++)
                        buf[j] = buf[j + 3];
                }
                else if (buf[i] == 27 && buf[i + 1] == '['
                         && buf[i + 2] == '2' && buf[i + 3] == 'J')
                {
                    // clear screen
                    for (j = i; buf[j]; j++)
                        buf[j] = buf[j + 4];
                }
                else if (buf[i] == 27
                        && buf[i + 1] == '['
                        && buf[i + 3] == '~'
                        && (buf[i + 2] == '1' // home (windows)
                            || buf[i + 2] == '2' // insert (windows)
                            || buf[i + 2] == '3' // del (windows)
                            || buf[i + 2] == '4' // end (windows)
                            || buf[i + 2] == '5' // pgup (windows)
                            || buf[i + 2] == '6')) // pgdown (windows)
                {
                    for (j = i; buf[j]; j++)
                        buf[j] = buf[j + 4];
                }
                else
                {
                    // remove other control char.
                    for (j = i; buf[j]; j++)
                        buf[j] = buf[j + 1];
                }
                i--;
            }

        char command[1024] {};
        // extract command name and parameters
        strzcpy(parameters, "", sizeof(parameters));
        sscanf(buf, "%1023s %[^\n]", command, parameters);
        command[1023] = '\0';
        parameters[1023] = '\0';

        // lowercase for command line
        for (i = 0; command[i]; i++)
            command[i] = tolower(command[i]);

        if (command[0] == '?' || strlen(command) == 0)
        {
            strcpy(buf, "help");
            strcpy(command, "help");
        }

        // Analyse of the command
        check_command(command);    // give complete name to the command

        if (strlen(parameters) == 0)
        {
            LADMIN_LOG("Command: '%s' (without parameters)\n",
                        command);
        }
        else
        {
            LADMIN_LOG("Command: '%s', parameters: '%s'\n",
                        command, parameters);
        }

        // Analyse of the command
// help
        if (strcmp(command, "help") == 0)
        {
            display_help(parameters);
// general commands
        }
        else if (strcmp(command, "add") == 0)
        {
            addaccount(parameters, 0); // 0: no email
        }
        else if (strcmp(command, "ban") == 0)
        {
            banaccount(parameters);
        }
        else if (strcmp(command, "banadd") == 0)
        {
            banaddaccount(parameters);
        }
        else if (strcmp(command, "banset") == 0)
        {
            bansetaccount(parameters);
        }
        else if (strcmp(command, "block") == 0)
        {
            blockaccount(parameters);
        }
        else if (strcmp(command, "check") == 0)
        {
            checkaccount(parameters);
        }
        else if (strcmp(command, "create") == 0)
        {
            addaccount(parameters, 1); // 1: with email
        }
        else if (strcmp(command, "delete") == 0)
        {
            delaccount(parameters);
        }
        else if (strcmp(command, "email") == 0)
        {
            changeemail(parameters);
        }
        else if (strcmp(command, "getcount") == 0)
        {
            getlogincount();
        }
        else if (strcmp(command, "gm") == 0)
        {
            changegmlevel(parameters);
        }
        else if (strcmp(command, "id") == 0)
        {
            idaccount(parameters);
        }
        else if (strcmp(command, "info") == 0)
        {
            infoaccount(atoi(parameters));
        }
        else if (strcmp(command, "kami") == 0)
        {
            sendbroadcast(0, parameters);  // flag for normal
        }
        else if (strcmp(command, "kamib") == 0)
        {
            sendbroadcast(0x10, parameters);   // flag for blue
        }
        else if (strcmp(command, "itemfrob") == 0)
        {
            itemfrob(parameters);  // 0: to list all
        }
        else if (strcmp(command, "list") == 0)
        {
            listaccount(parameters, 0);    // 0: to list all
        }
        else if (strcmp(command, "listban") == 0)
        {
            listaccount(parameters, 3);    // 3: to list only accounts with state or bannished
        }
        else if (strcmp(command, "listgm") == 0)
        {
            listaccount(parameters, 1);    // 1: to list only GM
        }
        else if (strcmp(command, "listok") == 0)
        {
            listaccount(parameters, 4);    // 4: to list only accounts without state and not bannished
        }
        else if (strcmp(command, "memo") == 0)
        {
            changememo(parameters);
        }
        else if (strcmp(command, "name") == 0)
        {
            nameaccount(atoi(parameters));
        }
        else if (strcmp(command, "password") == 0)
        {
            changepasswd(parameters);
        }
        else if (strcmp(command, "reloadgm") == 0)
        {
            reloadGM(parameters);
        }
        else if (strcmp(command, "search") == 0)
        {                       // no regex in C version
            listaccount(parameters, 2);    // 2: to list with pattern
        }
        else if (strcmp(command, "sex") == 0)
        {
            changesex(parameters);
        }
        else if (strcmp(command, "state") == 0)
        {
            changestate(parameters);
        }
        else if (strcmp(command, "timeadd") == 0)
        {
            timeaddaccount(parameters);
        }
        else if (strcmp(command, "timeset") == 0)
        {
            timesetaccount(parameters);
        }
        else if (strcmp(command, "unban") == 0)
        {
            unbanaccount(parameters);
        }
        else if (strcmp(command, "unblock") == 0)
        {
            unblockaccount(parameters);
        }
        else if (strcmp(command, "version") == 0)
        {
            checkloginversion();
        }
        else if (strcmp(command, "who") == 0)
        {
            whoaccount(parameters);
// quit
        }
        else if (strcmp(command, "quit") == 0 ||
                 strcmp(command, "exit") == 0 ||
                 strcmp(command, "end") == 0)
        {
            PRINTF("Bye.\n");
            exit(0);
// unknown command
        }
        else
        {
            PRINTF("Unknown command [%s].\n", buf);
            LADMIN_LOG("Unknown command [%s].\n", buf);
        }
    }

    return 0;
}

//-------------------------------------------------------------
// Function: Parse receiving informations from the login-server
//-------------------------------------------------------------
static
void parse_fromlogin(int fd)
{
    if (session[fd]->eof)
    {
        PRINTF("Impossible to have a connection with the login-server [%s:%d] !\n",
             loginserverip, loginserverport);
        LADMIN_LOG("Impossible to have a connection with the login-server [%s:%d] !\n",
              loginserverip, loginserverport);
        delete_session(fd);
        exit(0);
    }

//  PRINTF("parse_fromlogin : %d %d %d\n", fd, RFIFOREST(fd), RFIFOW(fd,0));

    while (RFIFOREST(fd) >= 2)
    {
        switch (RFIFOW(fd, 0))
        {
            case 0x7919:       // answer of a connection request
                if (RFIFOREST(fd) < 3)
                    return;
                if (RFIFOB(fd, 2) != 0)
                {
                    PRINTF("Error at login:\n");
                    PRINTF(" - incorrect password,\n");
                    PRINTF(" - administration system not activated, or\n");
                    PRINTF(" - unauthorised IP.\n");
                    LADMIN_LOG("Error at login: incorrect password, administration system not activated, or unauthorised IP.\n");
                    session[fd]->eof = 1;
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
                RFIFOSKIP(fd, 3);
                break;

            case 0x01dc:       // answer of a coding key request
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                {
                    char md5str[64] = "";
                    size_t key_len = RFIFOW(fd, 2) - 4;
                    uint8_t md5bin[32];
                    char md5key[key_len];
                    RFIFO_STRING(fd, 4, md5key, key_len);
                    if (passenc == 1)
                    {
                        strcpy(md5str, md5key);
                        strcat(md5str, loginserveradminpassword);
                    }
                    else if (passenc == 2)
                    {
                        strcpy(md5str, loginserveradminpassword);
                        strcat(md5str, md5key);
                    }
                    MD5_to_bin(MD5_from_cstring(md5str), md5bin);
                    WFIFOW(login_fd, 0) = 0x7918;  // Request for administation login (encrypted password)
                    WFIFOW(login_fd, 2) = passenc; // Encrypted type
                    really_memcpy(static_cast<uint8_t *>(WFIFOP(login_fd, 4)), md5bin, 16);
                    WFIFOSET(login_fd, 20);
                    Iprintf("Receiving of the MD5 key.\n");
                    LADMIN_LOG("Receiving of the MD5 key.\n");
                    Iprintf("Sending of the encrypted password...\n");
                    LADMIN_LOG("Sending of the encrypted password...\n");
                }
                bytes_to_read = 1;
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

            case 0x7531:       // Displaying of the version of the login-server
                if (RFIFOREST(fd) < 10)
                    return;
                Iprintf("  Login-Server [%s:%d]\n", loginserverip,
                         loginserverport);
                if (RFIFOB(login_fd, 5) == 0)
                {
                    Iprintf("  eAthena version stable-%d.%d",
                            RFIFOB(login_fd, 2),
                            RFIFOB(login_fd, 3));
                }
                else
                {
                    Iprintf("  eAthena version dev-%d.%d",
                            RFIFOB(login_fd, 2),
                            RFIFOB(login_fd, 3));
                }
                if (RFIFOB(login_fd, 4) == 0)
                    Iprintf(" revision %d", RFIFOB(login_fd, 4));
                if (RFIFOB(login_fd, 6) == 0)
                {
                    Iprintf("%d.\n", RFIFOW(login_fd, 8));
                }
                else
                    Iprintf("-mod%d.\n", RFIFOW(login_fd, 8));
                bytes_to_read = 0;
                RFIFOSKIP(fd, 10);
                break;

            case 0x7925:       // Itemfrob-OK
                RFIFOSKIP(fd, 2);
                bytes_to_read = 0;
                break;

            case 0x7921:       // Displaying of the list of accounts
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                if (RFIFOW(fd, 2) < 5)
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
                    for (i = 4; i < RFIFOW(fd, 2); i += 38)
                    {
                        int j;
                        char userid[24];
                        char lower_userid[24] {};
                        RFIFO_STRING(fd, i + 5, userid, 24);
                        for (j = 0; userid[j]; j++)
                            lower_userid[j] = tolower(userid[j]);
                        list_first = RFIFOL(fd, i) + 1;
                        // here are checks...
                        if (list_type == 0 ||
                            (list_type == 1 && RFIFOB(fd, i + 4) > 0) ||
                            (list_type == 2
                             && strstr(lower_userid, parameters) != NULL)
                            ||(list_type == 3 && RFIFOL(fd, i + 34) != 0)
                            ||(list_type == 4 && RFIFOL(fd, i + 34) == 0))
                        {
                            PRINTF("%10d ", RFIFOL(fd, i));
                            if (RFIFOB(fd, i + 4) == 0)
                                PRINTF("   ");
                            else
                                PRINTF("%2d ", RFIFOB(fd, i + 4));
                            PRINTF("%-24s", userid);
                            if (RFIFOB(fd, i + 29) == 0)
                                PRINTF("%-5s ", "Femal");
                            else if (RFIFOB(fd, i + 29) == 1)
                                PRINTF("%-5s ", "Male");
                            else
                                PRINTF("%-5s ", "Servr");
                            PRINTF("%6d ", RFIFOL(fd, i + 30));
                            switch (RFIFOL(fd, i + 34))
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
                    WFIFOW(login_fd, 0) = 0x7920;
                    WFIFOL(login_fd, 2) = list_first;
                    WFIFOL(login_fd, 6) = list_last;
                    WFIFOSET(login_fd, 10);
                    bytes_to_read = 1;
                }
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

            case 0x7931:       // Answer of login-server about an account creation
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int accid = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7933:       // Answer of login-server about an account deletion
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int accid = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7935:       // answer of the change of an account password
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int accid = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7937:       // answer of the change of an account state
                if (RFIFOREST(fd) < 34)
                    return;
            {
                int accid = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
                int state = RFIFOL(fd, 30);
                if (accid == -1)
                {
                    PRINTF("Account [%s] state changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] state changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    std::string tmpstr = STRPRINTF(
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
                    PRINTF("%s\n", tmpstr);
                    LADMIN_LOG("%s\n", tmpstr);
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(fd, 34);
                break;

            case 0x7939:       // answer of the number of online players
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                {
                    // Get length of the received packet
                    LADMIN_LOG("  Receiving of the number of online players.\n");
                    // Read information of the servers
                    if (RFIFOW(fd, 2) < 5)
                    {
                        PRINTF("  No server is connected to the login-server.\n");
                    }
                    else
                    {
                        PRINTF("  Number of online players (server: number).\n");
                        // Displaying of result
                        for (int i = 4; i < RFIFOW(fd, 2); i += 32)
                        {
                            char name[20];
                            RFIFO_STRING(fd, i + 6, name, 20);
                            PRINTF("    %-20s : %5d\n", name,
                                    RFIFOW(fd, i + 26));
                        }
                    }
                }
                bytes_to_read = 0;
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

            case 0x793b:       // answer of the check of a password
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x793d:       // answer of the change of an account sex
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x793f:       // answer of the change of an account GM level
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7941:       // answer of the change of an account email
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7943:       // answer of the change of an account memo
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7945:       // answer of an account id search
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7947:       // answer of an account name search
                if (RFIFOREST(fd) < 30)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
                if (strcmp(name, "") == 0)
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
                RFIFOSKIP(fd, 30);
                break;

            case 0x7949:       // answer of an account validity limit set
                if (RFIFOREST(fd) < 34)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
                if (RFIFOL(fd, 2) == -1)
                {
                    PRINTF("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(fd, 30));
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
                RFIFOSKIP(fd, 34);
                break;

            case 0x794b:       // answer of an account ban set
                if (RFIFOREST(fd) < 34)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
                if (account_id == -1)
                {
                    PRINTF("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(fd, 30));
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
                                name, RFIFOL(fd, 2), tmpstr);
                        LADMIN_LOG("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n",
                                name, RFIFOL(fd, 2),
                                tmpstr);
                    }
                }
                bytes_to_read = 0;
            }
                RFIFOSKIP(fd, 34);
                break;

            case 0x794d:       // answer of an account ban date/time changing
                if (RFIFOREST(fd) < 34)
                    return;
            {
                int account_id = RFIFOL(fd, 2);
                char name[24];
                RFIFO_STRING(fd, 6, name, 24);
                if (account_id == -1)
                {
                    PRINTF("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                    LADMIN_LOG("Account [%s] final date of banishment changing failed. Account doesn't exist.\n",
                            name);
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(fd, 30));
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
                RFIFOSKIP(fd, 34);
                break;

            case 0x794f:       // answer of a broadcast
                if (RFIFOREST(fd) < 4)
                    return;
                if (RFIFOW(fd, 2) == static_cast<uint16_t>(-1))
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
                RFIFOSKIP(fd, 4);
                break;

            case 0x7951:       // answer of an account validity limit changing
                if (RFIFOREST(fd) < 34)
                    return;
                if (RFIFOL(fd, 2) == -1)
                {
                    PRINTF("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            static_cast<const char *>(RFIFOP(fd, 6)));
                    LADMIN_LOG("Account [%s] validity limit changing failed. Account doesn't exist.\n",
                            static_cast<const char *>(RFIFOP(fd, 6)));
                }
                else
                {
                    TimeT timestamp = static_cast<time_t>(RFIFOL(fd, 30));
                    if (!timestamp)
                    {
                        PRINTF("Validity limit of the account [%s][id: %d] unchanged.\n",
                                static_cast<const char *>(RFIFOP(fd, 6)), RFIFOL(fd, 2));
                        PRINTF("The account have an unlimited validity limit or\n");
                        PRINTF("the changing is impossible with the proposed adjustments.\n");
                        LADMIN_LOG("Validity limit of the account [%s][id: %d] unchanged. The account have an unlimited validity limit or the changing is impossible with the proposed adjustments.\n",
                                static_cast<const char *>(RFIFOP(fd, 6)), RFIFOL(fd, 2));
                    }
                    else
                    {
                        timestamp_seconds_buffer tmpstr;
                        stamp_time(tmpstr, &timestamp);
                        PRINTF("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n",
                                static_cast<const char *>(RFIFOP(fd, 6)), RFIFOL(fd, 2),
                                tmpstr);
                        LADMIN_LOG("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n",
                                static_cast<const char *>( RFIFOP(fd, 6)), RFIFOL(fd, 2),
                                tmpstr);
                    }
                }
                bytes_to_read = 0;
                RFIFOSKIP(fd, 34);
                break;

            case 0x7953:       // answer of a request about informations of an account (by account name/id)
                if (RFIFOREST(fd) < 150
                    || RFIFOREST(fd) < (150 + RFIFOW(fd, 148)))
                    return;
                {
                    char userid[24], error_message[20], lastlogin[24],
                        last_ip[16], email[40], memo[255];
                    TimeT ban_until_time;  // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
                    TimeT connect_until_time;  // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
                    RFIFO_STRING(fd, 7, userid, 24);
                    RFIFO_STRING(fd, 40, error_message, 20);
                    RFIFO_STRING(fd, 60, lastlogin, 24);
                    RFIFO_STRING(fd, 84, last_ip, 16);
                    RFIFO_STRING(fd, 100, email, 40);
                    connect_until_time = static_cast<time_t>(RFIFOL(fd, 140));
                    ban_until_time = static_cast<time_t>(RFIFOL(fd, 144));
                    RFIFO_STRING(fd, 150, memo, RFIFOW(fd, 148));
                    if (RFIFOL(fd, 2) == -1)
                    {
                        PRINTF("Unabled to find the account [%s]. Account doesn't exist.\n",
                             parameters);
                        LADMIN_LOG("Unabled to find the account [%s]. Account doesn't exist.\n",
                              parameters);
                    }
                    else if (strlen(userid) == 0)
                    {
                        PRINTF("Unabled to find the account [id: %s]. Account doesn't exist.\n",
                             parameters);
                        LADMIN_LOG("Unabled to find the account [id: %s]. Account doesn't exist.\n",
                              parameters);
                    }
                    else
                    {
                        LADMIN_LOG("Receiving information about an account.\n");
                        PRINTF("The account is set with:\n");
                        if (RFIFOB(fd, 6) == 0)
                        {
                            PRINTF(" Id:     %d (non-GM)\n", RFIFOL(fd, 2));
                        }
                        else
                        {
                            PRINTF(" Id:     %d (GM level %d)\n",
                                    RFIFOL(fd, 2), RFIFOB(fd, 6));
                        }
                        PRINTF(" Name:   '%s'\n", userid);
                        if (RFIFOB(fd, 31) == 0)
                            PRINTF(" Sex:    Female\n");
                        else if (RFIFOB(fd, 31) == 1)
                            PRINTF(" Sex:    Male\n");
                        else
                            PRINTF(" Sex:    Server\n");
                        PRINTF(" E-mail: %s\n", email);
                        switch (RFIFOL(fd, 36))
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
                                     RFIFOL(fd, 36));
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
                        if (RFIFOL(fd, 32) > 1)
                            PRINTF(" Count:  %d connections.\n",
                                    RFIFOL(fd, 32));
                        else
                            PRINTF(" Count:  %d connection.\n",
                                    RFIFOL(fd, 32));
                        PRINTF(" Last connection at: %s (ip: %s)\n",
                                lastlogin, last_ip);
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
                RFIFOSKIP(fd, 150 + RFIFOW(fd, 148));
                break;

            default:
                PRINTF("Remote administration has been disconnected (unknown packet).\n");
                LADMIN_LOG("'End of connection, unknown packet.\n");
                session[fd]->eof = 1;
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

    if ((login_fd = make_connection(login_ip, loginserverport)) < 0)
        return 0;

    if (passenc == 0)
    {
        WFIFOW(login_fd, 0) = 0x7918;  // Request for administation login
        WFIFOW(login_fd, 2) = 0;   // no encrypted
        WFIFO_STRING(login_fd, 4, loginserveradminpassword, 24);
        WFIFOSET(login_fd, 28);
        bytes_to_read = 1;

        Iprintf("Sending of the password...\n");
        LADMIN_LOG("Sending of the password...\n");
    }
    else
    {
        WFIFOW(login_fd, 0) = 0x791a;  // Sending request about the coding key
        WFIFOSET(login_fd, 2);
        bytes_to_read = 1;
        Iprintf("Request about the MD5 key...\n");
        LADMIN_LOG("Request about the MD5 key...\n");
    }

    return 0;
}

//-----------------------------------
// Reading general configuration file
//-----------------------------------
static
int ladmin_config_read(const char *cfgName)
{
    std::ifstream in(cfgName);
    if (!in.is_open())
    {
        PRINTF("\033[0mConfiguration file (%s) not found.\n", cfgName);
        return 1;
    }

    Iprintf("\033[0m---Start reading of Ladmin configuration file (%s)\n",
         cfgName);
    std::string line;
    while (std::getline(in, line))
    {
        std::string w1, w2;
        if (!split_key_value(line, &w1, &w2))
            continue;

        if (w1 == "login_ip")
        {
            struct hostent *h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                Iprintf("Login server IP address: %s -> %d.%d.%d.%d\n",
                        w2,
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]));
                sprintf(loginserverip, "%d.%d.%d.%d",
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]));
            }
            else
                strzcpy(loginserverip, w2.c_str(), 16);
        }
        else if (w1 == "login_port")
        {
            loginserverport = atoi(w2.c_str());
        }
        else if (w1 == "admin_pass")
        {
            strzcpy(loginserveradminpassword, w2.c_str(), sizeof(loginserveradminpassword));
        }
        else if (w1 == "passenc")
        {
            passenc = atoi(w2.c_str());
            if (passenc < 0 || passenc > 2)
                passenc = 0;
        }
        else if (w1 == "ladmin_log_filename")
        {
            strzcpy(ladmin_log_filename, w2.c_str(), sizeof(ladmin_log_filename));
        }
        else if (w1 == "import")
        {
            ladmin_config_read(w2.c_str());
        }
        else
        {
            PRINTF("WARNING: unknown ladmin config key: %s\n", w1);
        }
    }

    login_ip = inet_addr(loginserverip);

    Iprintf("---End reading of Ladmin configuration file.\n");

    return 0;
}

//--------------------------------------
// Function called at exit of the server
//--------------------------------------
void term_func(void)
{

    if (already_exit_function == 0)
    {
        delete_session(login_fd);

        Iprintf("\033[0m----End of Ladmin (normal end with closing of all files).\n");
        LADMIN_LOG("----End of Ladmin (normal end with closing of all files).\n");

        already_exit_function = 1;
    }
}

//------------------------
// Main function of ladmin
//------------------------
int do_init(int argc, char **argv)
{
    eathena_interactive_session = isatty(0);
    // read ladmin configuration
    ladmin_config_read((argc > 1) ? argv[1] : LADMIN_CONF_NAME);

    LADMIN_LOG("");
    LADMIN_LOG("Configuration file readed.\n");

    set_defaultparse(parse_fromlogin);

    Iprintf("EAthena login-server administration tool.\n");
    Iprintf("(for eAthena version %d.%d.%d.)\n", ATHENA_MAJOR_VERSION,
             ATHENA_MINOR_VERSION, ATHENA_REVISION);

    LADMIN_LOG("Ladmin is ready.\n");
    Iprintf("Ladmin is \033[1;32mready\033[0m.\n\n");

    Connect_login_server();

    return 0;
}
