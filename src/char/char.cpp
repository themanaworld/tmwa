#include "char.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netdb.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

#include <fstream>

#include "../common/core.hpp"
#include "../common/cxxstdio.hpp"
#include "../common/db.hpp"
#include "../common/extract.hpp"
#include "../common/lock.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"
#include "../common/version.hpp"

#include "inter.hpp"
#include "int_party.hpp"
#include "int_storage.hpp"

#include "../poison.hpp"

static
struct mmo_map_server server[MAX_MAP_SERVERS];
static
int server_fd[MAX_MAP_SERVERS];
static
int server_freezeflag[MAX_MAP_SERVERS];    // Map-server anti-freeze system. Counter. 5 ok, 4...0 freezed
static
int anti_freeze_enable = 0;
static
std::chrono::seconds ANTI_FREEZE_INTERVAL = std::chrono::seconds(6);

constexpr
std::chrono::milliseconds DEFAULT_AUTOSAVE_INTERVAL =
        std::chrono::minutes(5);

// TODO replace all string forms of IP addresses with class instances
static
int login_fd, char_fd;
static
char userid[24];
static
char passwd[24];
static
char server_name[20];
static
char wisp_server_name[24] = "Server";
static
char login_ip_str[16];
static
int login_ip;
static
int login_port = 6900;
static
char char_ip_str[16];
static
int char_ip;
static
int char_port = 6121;
static
int char_maintenance;
static
int char_new;
static
int email_creation = 0;        // disabled by default
static
char char_txt[1024];
static
char unknown_char_name[1024] = "Unknown";
static
char char_log_filename[1024] = "log/char.log";
//Added for lan support
static
char lan_map_ip[128];
static
int subneti[4];
static
int subnetmaski[4];
static
int name_ignoring_case = 0;    // Allow or not identical name for characters but with a different case by [Yor]
static
int char_name_option = 0;      // Option to know which letters/symbols are authorised in the name of a character (0: all, 1: only those in char_name_letters, 2: all EXCEPT those in char_name_letters) by [Yor]
static
char char_name_letters[1024] = "";  // list of letters/symbols authorised (or not) in a character name. by [Yor]

struct char_session_data
{
    int account_id, login_id1, login_id2, sex;
    unsigned short packet_tmw_version;
    int found_char[9];
    char email[40];             // e-mail (default: a@a.com) by [Yor]
    TimeT connect_until_time;  // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
};

#define AUTH_FIFO_SIZE 256
static
struct
{
    int account_id, char_id, login_id1, login_id2, ip, char_pos, delflag,
        sex;
    unsigned short packet_tmw_version;
    TimeT connect_until_time;  // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
} auth_fifo[AUTH_FIFO_SIZE];
static
int auth_fifo_pos = 0;

static
int check_ip_flag = 1;         // It's to check IP of a player between char-server and other servers (part of anti-hacking system)

static
int char_id_count = 150000;
static
struct mmo_charstatus *char_dat;
static
int char_num, char_max;
static
int max_connect_user = 0;
static
std::chrono::milliseconds autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
static
int start_zeny = 500;
static
int start_weapon = 1201;
static
int start_armor = 1202;

// Initial position (it's possible to set it in conf file)
static
struct point start_point = { "new_1-1.gat", 53, 111 };

static
struct gm_account *gm_account = NULL;
static
int GM_num = 0;

// online players by [Yor]
static
char online_txt_filename[1024] = "online.txt";
static
char online_html_filename[1024] = "online.html";
static
int online_sorting_option = 0; // sorting option to display online players in online files
static
int online_refresh_html = 20;  // refresh time (in sec) of the html file in the explorer
static
int online_gm_display_min_level = 20;  // minimum GM level to display 'GM' when we want to display it

static
int *online_chars;              // same size of char_dat, and id value of current server (or -1)
static
TimeT update_online;           // to update online files when we receiving information from a server (not less than 8 seconds)

static
pid_t pid = 0;                  // For forked DB writes

//------------------------------
// Writing function of logs file
//------------------------------
void char_log(const_string line)
{
    FILE *logfp = fopen_(char_log_filename, "a");
    if (!logfp)
        return;
    log_with_timestamp(logfp, line);
    fclose_(logfp);
}

//----------------------------------------------------------------------
// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)
//----------------------------------------------------------------------
static
int isGM(int account_id)
{
    int i;

    for (i = 0; i < GM_num; i++)
        if (gm_account[i].account_id == account_id)
            return gm_account[i].level;
    return 0;
}

//----------------------------------------------
// Search an character id
//   (return character index or -1 (if not found))
//   If exact character name is not found,
//   the function checks without case sensitive
//   and returns index if only 1 character is found
//   and similar to the searched name.
//----------------------------------------------
int search_character_index(const char *character_name)
{
    int i, quantity, index;

    quantity = 0;
    index = -1;
    for (i = 0; i < char_num; i++)
    {
        // Without case sensitive check (increase the number of similar character names found)
        if (strcasecmp(char_dat[i].name, character_name) == 0)
        {
            // Strict comparison (if found, we finish the function immediatly with correct value)
            if (strcmp(char_dat[i].name, character_name) == 0)
                return i;
            quantity++;
            index = i;
        }
    }
    // Here, the exact character name is not found
    // We return the found index of a similar account ONLY if there is 1 similar character
    if (quantity == 1)
        return index;

    // Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
    return -1;
}

//-------------------------------------
// Return character name with the index
//-------------------------------------
char *search_character_name(int index)
{

    if (index >= 0 && index < char_num)
        return char_dat[index].name;

    return unknown_char_name;
}

//-------------------------------------------------
// Function to create the character line (for save)
//-------------------------------------------------
static
std::string mmo_char_tostr(struct mmo_charstatus *p)
{
    // on multi-map server, sometimes it's posssible that last_point become void. (reason???) We check that to not lost character at restart.
    if (p->last_point.map[0] == '\0')
    {
        memcpy(p->last_point.map, "prontera.gat", 16);
        p->last_point.x = 273;
        p->last_point.y = 354;
    }

    std::string str_p;
    str_p += STRPRINTF(
            "%d\t"
            "%d,%d\t"
            "%s\t"
            "%d,%d,%d\t"
            "%d,%d,%d\t"
            "%d,%d,%d,%d\t"
            "%d,%d,%d,%d,%d,%d\t"
            "%d,%d\t"
            "%d,%d,%d\t"
            "%d,%d,%d\t"
            "%d,%d,%d\t"
            "%d,%d,%d,%d,%d\t"
            "%s,%d,%d\t"
            "%s,%d,%d,%d\t",
            p->char_id,
            p->account_id, p->char_num,
            p->name,
            p->species, p->base_level, p->job_level,
            p->base_exp, p->job_exp, p->zeny,
            p->hp, p->max_hp, p->sp, p->max_sp,
            p->attrs[ATTR::STR], p->attrs[ATTR::AGI], p->attrs[ATTR::VIT], p->attrs[ATTR::INT], p->attrs[ATTR::DEX], p->attrs[ATTR::LUK],
            p->status_point, p->skill_point,
            p->option, p->karma, p->manner,
            p->party_id, 0/*guild_id*/, 0/*pet_id*/,
            p->hair, p->hair_color, p->clothes_color,
            p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
            p->last_point.map, p->last_point.x, p->last_point.y,
            p->save_point.map, p->save_point.x, p->save_point.y, p->partner_id);
    for (int i = 0; i < 10; i++)
        if (p->memo_point[i].map[0])
        {
            str_p += STRPRINTF("%s,%d,%d ",
                    p->memo_point[i].map, p->memo_point[i].x, p->memo_point[i].y);
        }
    str_p += '\t';

    for (int i = 0; i < MAX_INVENTORY; i++)
        if (p->inventory[i].nameid)
        {
            str_p += STRPRINTF("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",
                    p->inventory[i].id,
                    p->inventory[i].nameid,
                    p->inventory[i].amount,
                    p->inventory[i].equip,
                    p->inventory[i].identify,
                    p->inventory[i].refine,
                    p->inventory[i].attribute,
                    p->inventory[i].card[0],
                    p->inventory[i].card[1],
                    p->inventory[i].card[2],
                    p->inventory[i].card[3],
                    p->inventory[i].broken);
        }
    str_p += '\t';

    for (int i = 0; i < MAX_CART; i++)
        if (p->cart[i].nameid)
        {
            str_p += STRPRINTF("%d,%d,%d,%hhu,%d,%hd,%hhu,%d,%d,%d,%d,%d ",
                    p->cart[i].id,
                    p->cart[i].nameid,
                    p->cart[i].amount,
                    p->cart[i].equip,
                    p->cart[i].identify,
                    p->cart[i].refine,
                    p->cart[i].attribute,
                    p->cart[i].card[0],
                    p->cart[i].card[1],
                    p->cart[i].card[2],
                    p->cart[i].card[3],
                    p->cart[i].broken);
        }
    str_p += '\t';

    for (SkillID i : erange(SkillID(), MAX_SKILL))
        if (p->skill[i].lv)
        {
            str_p += STRPRINTF("%d,%d ",
                    i,
                    p->skill[i].lv | (uint16_t(p->skill[i].flags) << 16));
        }
    str_p += '\t';

    for (int i = 0; i < p->global_reg_num; i++)
        if (p->global_reg[i].str[0])
            str_p += STRPRINTF("%s,%d ",
                    p->global_reg[i].str,
                    p->global_reg[i].value);
    str_p += '\t';

    return str_p;
}

static
bool extract(const_string str, struct point *p)
{
    return extract(str, record<','>(&p->map, &p->x, &p->y));
}

struct skill_loader
{
    SkillID id;
    uint16_t level;
    SkillFlags flags;
};

static
bool extract(const_string str, struct skill_loader *s)
{
    uint32_t flags_and_level;
    if (!extract(str,
                record<','>(&s->id, &flags_and_level)))
        return false;
    s->level = flags_and_level & 0xffff;
    s->flags = SkillFlags(flags_and_level >> 16);
    return true;
}

//-------------------------------------------------------------------------
// Function to set the character from the line (at read of characters file)
//-------------------------------------------------------------------------
static
bool extract(const_string str, struct mmo_charstatus *p)
{
    // initilialise character
    memset(p, '\0', sizeof(struct mmo_charstatus));

    uint32_t unused_guild_id, unused_pet_id;
    std::vector<struct point> memos;
    std::vector<struct item> inventory, cart;
    std::vector<struct skill_loader> skills;
    std::vector<struct global_reg> vars;
    if (!extract(str,
                record<'\t'>(
                    &p->char_id,
                    record<','>(&p->account_id, &p->char_num),
                    &p->name,
                    record<','>(&p->species, &p->base_level, &p->job_level),
                    record<','>(&p->base_exp, &p->job_exp, &p->zeny),
                    record<','>(&p->hp, &p->max_hp, &p->sp, &p->max_sp),
                    record<','>(&p->attrs[ATTR::STR], &p->attrs[ATTR::AGI], &p->attrs[ATTR::VIT], &p->attrs[ATTR::INT], &p->attrs[ATTR::DEX], &p->attrs[ATTR::LUK]),
                    record<','>(&p->status_point, &p->skill_point),
                    record<','>(&p->option, &p->karma, &p->manner),
                    record<','>(&p->party_id, &unused_guild_id, &unused_pet_id),
                    record<','>(&p->hair, &p->hair_color, &p->clothes_color),
                    record<','>(&p->weapon, &p->shield, &p->head_top, &p->head_mid, &p->head_bottom),
                    &p->last_point,
                    // somebody was silly and stuck partner id as a field
                    // of this, instead of adding a new \t
                    // or putting it elsewhere, like by pet/guild
                    record<','>(&p->save_point.map, &p->save_point.x, &p->save_point.y, &p->partner_id),
                    vrec<' '>(&memos),
                    vrec<' '>(&inventory),
                    vrec<' '>(&cart),
                    vrec<' '>(&skills),
                    vrec<' '>(&vars))))
        return false;

    if (strcmp(wisp_server_name, p->name) == 0)
        return false;

    for (int i = 0; i < char_num; i++)
    {
        if (char_dat[i].char_id == p->char_id)
            return false;
        if (strcmp(char_dat[i].name, p->name) == 0)
            return false;
    }

    if (memos.size() > 10)
        return false;
    std::copy(memos.begin(), memos.end(), p->memo_point);
    // number of memo points is not saved - it just detects map name '\0'

    if (inventory.size() > MAX_INVENTORY)
        return false;
    std::copy(inventory.begin(), inventory.end(), p->inventory);
    // number of inventory items is not saved - it just detects nameid 0

    if (cart.size() > MAX_CART)
       return false;
    std::copy(cart.begin(), cart.end(), p->cart);
    // number of cart items is not saved - it just detects nameid 0

    for (struct skill_loader& sk : skills)
    {
        if (sk.id > MAX_SKILL)
            return false;
        p->skill[sk.id].lv = sk.level;
        p->skill[sk.id].flags = sk.flags;
    }

    if (vars.size() > GLOBAL_REG_NUM)
        return false;
    std::copy(vars.begin(), vars.end(), p->global_reg);
    p->global_reg_num = vars.size();

    return true;
}

//---------------------------------
// Function to read characters file
//---------------------------------
static
int mmo_char_init(void)
{
    char_max = 256;
    CREATE(char_dat, struct mmo_charstatus, 256);
    CREATE(online_chars, int, 256);
    for (int i = 0; i < char_max; i++)
        online_chars[i] = -1;

    char_num = 0;

    std::ifstream in(char_txt);
    if (!in.is_open())
    {
        PRINTF("Characters file not found: %s.\n", char_txt);
        CHAR_LOG("Characters file not found: %s.\n", char_txt);
        CHAR_LOG("Id for the next created character: %d.\n",
                  char_id_count);
        return 0;
    }

    int line_count = 0;
    std::string line;
    while (std::getline(in, line))
    {
        line_count++;

        if (line[0] == '/' && line[1] == '/')
            continue;
        if (line.back() == '\r')
        {
            line.back() = 0;
        }

        {
            int i, j = 0;
            if (SSCANF(line, "%d\t%%newid%%%n", &i, &j) == 1 && j > 0)
            {
                if (char_id_count < i)
                    char_id_count = i;
                continue;
            }
        }

        if (char_num >= char_max)
        {
            char_max += 256;
            RECREATE(char_dat, struct mmo_charstatus, char_max);
            RECREATE(online_chars, int, char_max);
            for (int i = char_max - 256; i < char_max; i++)
                online_chars[i] = -1;
        }

        if (!extract(line, &char_dat[char_num]))
        {
            CHAR_LOG("Char skipped\n%s", line);
            continue;
        }
        if (char_dat[char_num].char_id >= char_id_count)
            char_id_count = char_dat[char_num].char_id + 1;
        char_num++;
    }

    PRINTF("mmo_char_init: %d characters read in %s.\n",
            char_num, char_txt);
    CHAR_LOG("mmo_char_init: %d characters read in %s.\n",
            char_num, char_txt);

    CHAR_LOG("Id for the next created character: %d.\n",
            char_id_count);

    return 0;
}

//---------------------------------------------------------
// Function to save characters in files (speed up by [Yor])
//---------------------------------------------------------
static
void mmo_char_sync(void)
{
    int i, j, k;
    int lock;
    FILE *fp;
    int id[char_num];

    // Sorting before save (by [Yor])
    for (i = 0; i < char_num; i++)
    {
        id[i] = i;
        for (j = 0; j < i; j++)
        {
            if ((char_dat[i].account_id < char_dat[id[j]].account_id) ||
                // if same account id, we sort by slot.
                (char_dat[i].account_id == char_dat[id[j]].account_id &&
                 char_dat[i].char_num < char_dat[id[j]].char_num))
            {
                for (k = i; k > j; k--)
                    id[k] = id[k - 1];
                id[j] = i;      // id[i]
                break;
            }
        }
    }

    // Data save
    fp = lock_fopen(char_txt, &lock);
    if (fp == NULL)
    {
        PRINTF("WARNING: Server can't not save characters.\n");
        CHAR_LOG("WARNING: Server can't not save characters.\n");
    }
    else
    {
        for (i = 0; i < char_num; i++)
        {
            // use of sorted index
            std::string line = mmo_char_tostr(&char_dat[id[i]]);
            fwrite(line.data(), 1, line.size(), fp);
            fputc('\n', fp);
        }
        FPRINTF(fp, "%d\t%%newid%%\n", char_id_count);
        lock_fclose(fp, char_txt, &lock);
    }
}

//----------------------------------------------------
// Function to save (in a periodic way) datas in files
//----------------------------------------------------
static
void mmo_char_sync_timer(TimerData *, tick_t)
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

    mmo_char_sync();
    inter_save();

    // If we're a child we should suicide now.
    if (pid == 0)
        _exit(0);
}

//----------------------------------------------------
// Remove trailing whitespace from a name
//----------------------------------------------------
static
void remove_trailing_blanks(char *name)
{
    char *tail = name + strlen(name) - 1;

    while (tail > name && *tail == ' ')
        *tail-- = 0;
}

//----------------------------------------------------
// Remove prefix whitespace from a name
//----------------------------------------------------
static
void remove_prefix_blanks(char *name)
{
    char *dst = name;
    char *src = name;

    while (*src == ' ')         // find first nonblank
        ++src;
    while ((*dst++ = *src++));  // `strmove'
}

//-----------------------------------
// Function to create a new character
//-----------------------------------
static
int make_new_char(int fd, const uint8_t *dat)
{
    // ugh
    char *cdat = reinterpret_cast<char *>(const_cast<uint8_t *>(dat));
    int i, j;
    struct char_session_data *sd = (struct char_session_data *)session[fd]->session_data;

    // remove control characters from the name
    cdat[23] = '\0';
    if (remove_control_chars(cdat))
    {
        CHAR_LOG("Make new char error (control char received in the name): (connection #%d, account: %d).\n",
             fd, sd->account_id);
        return -1;
    }

    // Eliminate whitespace
    remove_trailing_blanks(cdat);
    remove_prefix_blanks(cdat);

    // check lenght of character name
    if (strlen(cdat) < 4)
    {
        CHAR_LOG("Make new char error (character name too small): (connection #%d, account: %d, name: '%s').\n",
             fd, sd->account_id, cdat);
        return -1;
    }

    // Check Authorised letters/symbols in the name of the character
    if (char_name_option == 1)
    {                           // only letters/symbols in char_name_letters are authorised
        for (i = 0; cdat[i]; i++)
            if (strchr(char_name_letters, cdat[i]) == NULL)
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n",
                     fd, sd->account_id, cdat, cdat[i]);
                return -1;
            }
    }
    else if (char_name_option == 2)
    {                           // letters/symbols in char_name_letters are forbidden
        for (i = 0; cdat[i]; i++)
            if (strchr(char_name_letters, cdat[i]) != NULL)
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n",
                     fd, sd->account_id, cdat, cdat[i]);
                return -1;
            }
    }                           // else, all letters/symbols are authorised (except control char removed before)

    // this is why it needs to be unsigned
    if (dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29] != 5 * 6 ||   // stats
        dat[30] >= 9 ||         // slots (dat[30] can not be negativ)
        dat[33] >= 20 || // hair style
        dat[31] >= 12)
    {                           // hair color (dat[31] can not be negativ)
        CHAR_LOG("Make new char error (invalid values): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d\n",
             fd, sd->account_id, dat[30], dat, dat[24], dat[25],
             dat[26], dat[27], dat[28], dat[29],
             dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
             dat[33], dat[31]);
        return -1;
    }

    // check individual stat value
    for (i = 24; i <= 29; i++)
    {
        if (dat[i] < 1 || dat[i] > 9)
        {
            CHAR_LOG("Make new char error (invalid stat value: not between 1 to 9): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d\n",
                 fd, sd->account_id, dat[30], dat, dat[24], dat[25],
                 dat[26], dat[27], dat[28], dat[29],
                 dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
                 dat[33], dat[31]);
            return -1;
        }
    }

    for (i = 0; i < char_num; i++)
    {
        if ((name_ignoring_case != 0 && strcmp(char_dat[i].name, cdat) == 0)
            || (name_ignoring_case == 0
                && strcasecmp(char_dat[i].name, cdat) == 0))
        {
            CHAR_LOG("Make new char error (name already exists): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n",
                 fd, sd->account_id, dat[30], cdat, char_dat[i].name,
                 dat[24], dat[25], dat[26], dat[27], dat[28], dat[29],
                 dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
                 dat[33], dat[31]);
            return -1;
        }
        if (char_dat[i].account_id == sd->account_id
            && char_dat[i].char_num == dat[30])
        {
            CHAR_LOG("Make new char error (slot already used): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n",
                 fd, sd->account_id, dat[30], cdat, char_dat[i].name,
                 dat[24], dat[25], dat[26], dat[27], dat[28], dat[29],
                 dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
                 dat[33], dat[31]);
            return -1;
        }
    }

    if (strcmp(wisp_server_name, cdat) == 0)
    {
        CHAR_LOG("Make new char error (name used is wisp name for server): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n",
             fd, sd->account_id, dat[30], cdat, char_dat[i].name,
             dat[24], dat[25], dat[26], dat[27], dat[28], dat[29],
             dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
             dat[33], dat[31]);
        return -1;
    }

    if (char_num >= char_max)
    {
        char_max += 256;
        RECREATE(char_dat, struct mmo_charstatus, char_max);
        RECREATE(online_chars, int, char_max);
        for (j = char_max - 256; j < char_max; j++)
            online_chars[j] = -1;
    }

    char ip[16];
    unsigned char *sin_addr =
        (unsigned char *) &session[fd]->client_addr.sin_addr;
    sprintf(ip, "%d.%d.%d.%d", sin_addr[0], sin_addr[1], sin_addr[2],
             sin_addr[3]);

    CHAR_LOG("Creation of New Character: (connection #%d, account: %d) slot %d, character Name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d. [%s]\n",
         fd, sd->account_id, dat[30], cdat, dat[24], dat[25], dat[26],
         dat[27], dat[28], dat[29],
         dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33],
         dat[31], ip);

    memset(&char_dat[i], 0, sizeof(struct mmo_charstatus));

    char_dat[i].char_id = char_id_count++;
    char_dat[i].account_id = sd->account_id;
    char_dat[i].char_num = dat[30];
    strcpy(char_dat[i].name, cdat);
    char_dat[i].species = 0;
    char_dat[i].base_level = 1;
    char_dat[i].job_level = 1;
    char_dat[i].base_exp = 0;
    char_dat[i].job_exp = 0;
    char_dat[i].zeny = start_zeny;
    char_dat[i].attrs[ATTR::STR] = dat[24];
    char_dat[i].attrs[ATTR::AGI] = dat[25];
    char_dat[i].attrs[ATTR::VIT] = dat[26];
    char_dat[i].attrs[ATTR::INT] = dat[27];
    char_dat[i].attrs[ATTR::DEX] = dat[28];
    char_dat[i].attrs[ATTR::LUK] = dat[29];
    char_dat[i].max_hp = 40 * (100 + char_dat[i].attrs[ATTR::VIT]) / 100;
    char_dat[i].max_sp = 11 * (100 + char_dat[i].attrs[ATTR::INT]) / 100;
    char_dat[i].hp = char_dat[i].max_hp;
    char_dat[i].sp = char_dat[i].max_sp;
    char_dat[i].status_point = 0;
    char_dat[i].skill_point = 0;
    char_dat[i].option = static_cast<Option>(0x0000); // Option is only declared
    char_dat[i].karma = 0;
    char_dat[i].manner = 0;
    char_dat[i].party_id = 0;
    //char_dat[i].guild_id = 0;
    char_dat[i].hair = dat[33];
    char_dat[i].hair_color = dat[31];
    char_dat[i].clothes_color = 0;
    char_dat[i].inventory[0].nameid = start_weapon; // Knife
    char_dat[i].inventory[0].amount = 1;
    char_dat[i].inventory[0].equip = EPOS::WEAPON;
    char_dat[i].inventory[0].identify = 1;
    char_dat[i].inventory[0].broken = 0;
    char_dat[i].inventory[1].nameid = start_armor;  // Cotton Shirt
    char_dat[i].inventory[1].amount = 1;
    char_dat[i].inventory[1].equip = EPOS::TORSO;
    char_dat[i].inventory[1].identify = 1;
    char_dat[i].inventory[1].broken = 0;
    char_dat[i].weapon = 1;
    char_dat[i].shield = 0;
    char_dat[i].head_top = 0;
    char_dat[i].head_mid = 0;
    char_dat[i].head_bottom = 0;
    memcpy(&char_dat[i].last_point, &start_point, sizeof(start_point));
    memcpy(&char_dat[i].save_point, &start_point, sizeof(start_point));
    char_num++;

    return i;
}

//-------------------------------------------------------------
// Function to create the online files (txt and html). by [Yor]
//-------------------------------------------------------------
static
void create_online_files(void)
{
    int i, j, k, l;            // for loops
    int players;               // count the number of players
    FILE *fp;                   // for the txt file
    FILE *fp2;                  // for the html file
    char temp[256];             // to prepare what we must display
    int id[char_num];

    // Get number of online players, id of each online players
    players = 0;
    // sort online characters.
    for (i = 0; i < char_num; i++)
    {
        if (online_chars[i] != -1)
        {
            id[players] = i;
            // use sorting option
            switch (online_sorting_option)
            {
                case 1:        // by name (without case sensitive)
                {
                    char *p_name = char_dat[i].name;    //speed up sorting when there are a lot of players. But very rarely players have same name.
                    for (j = 0; j < players; j++)
                        if (strcasecmp(p_name, char_dat[id[j]].name) < 0 ||
                            // if same name, we sort with case sensitive.
                            (strcasecmp(p_name, char_dat[id[j]].name) == 0 &&
                             strcmp(p_name, char_dat[id[j]].name) < 0))
                        {
                            for (k = players; k > j; k--)
                                id[k] = id[k - 1];
                            id[j] = i;  // id[players]
                            break;
                        }
                }
                    break;
                case 2:        // by zeny
                    for (j = 0; j < players; j++)
                        if (char_dat[i].zeny < char_dat[id[j]].zeny ||
                            // if same number of zenys, we sort by name.
                            (char_dat[i].zeny == char_dat[id[j]].zeny &&
                             strcasecmp(char_dat[i].name,
                                      char_dat[id[j]].name) < 0))
                        {
                            for (k = players; k > j; k--)
                                id[k] = id[k - 1];
                            id[j] = i;  // id[players]
                            break;
                        }
                    break;
                case 3:        // by base level
                    for (j = 0; j < players; j++)
                        if (char_dat[i].base_level <
                            char_dat[id[j]].base_level ||
                            // if same base level, we sort by base exp.
                            (char_dat[i].base_level ==
                             char_dat[id[j]].base_level
                             && char_dat[i].base_exp <
                             char_dat[id[j]].base_exp))
                        {
                            for (k = players; k > j; k--)
                                id[k] = id[k - 1];
                            id[j] = i;  // id[players]
                            break;
                        }
                    break;
                case 5:        // by location map name
                {
                    int cpm_result;    // A lot of player maps are identical. So, test if done often twice.
                    for (j = 0; j < players; j++)
                        if ((cpm_result = strcmp(char_dat[i].last_point.map, char_dat[id[j]].last_point.map)) < 0 ||   // no map are identical and with upper cases (not use strcasecmp)
                            // if same map name, we sort by name.
                            (cpm_result == 0 &&
                             strcasecmp(char_dat[i].name,
                                      char_dat[id[j]].name) < 0))
                        {
                            for (k = players; k > j; k--)
                                id[k] = id[k - 1];
                            id[j] = i;  // id[players]
                            break;
                        }
                }
                    break;
                default:       // 0 or invalid value: no sorting
                    break;
            }
            players++;
        }
    }

    // write files
    fp = fopen_(online_txt_filename, "w");
    if (fp != NULL)
    {
        fp2 = fopen_(online_html_filename, "w");
        if (fp2 != NULL)
        {
            // get time
#warning "Need to convert/check the PHP code"
            timestamp_seconds_buffer timetemp;
            stamp_time(timetemp);
            // write heading
            FPRINTF(fp2, "<HTML>\n");
            FPRINTF(fp2, "  <META http-equiv=\"Refresh\" content=\"%d\">\n", online_refresh_html); // update on client explorer every x seconds
            FPRINTF(fp2, "  <HEAD>\n");
            FPRINTF(fp2, "    <TITLE>Online Players on %s</TITLE>\n",
                     server_name);
            FPRINTF(fp2, "  </HEAD>\n");
            FPRINTF(fp2, "  <BODY>\n");
            FPRINTF(fp2, "    <H3>Online Players on %s (%s):</H3>\n",
                     server_name, timetemp);
            FPRINTF(fp, "Online Players on %s (%s):\n", server_name, timetemp);
            FPRINTF(fp, "\n");

            // If we display at least 1 player
            if (players > 0)
            {
                j = 0;          // count the number of characters for the txt version and to set the separate line
                FPRINTF(fp2, "    <table border=\"1\" cellspacing=\"1\">\n");
                FPRINTF(fp2, "      <tr>\n");
                {
                    FPRINTF(fp2, "        <td><b>Name</b></td>\n");
                    {
                        FPRINTF(fp, "Name                          "); // 30
                        j += 30;
                    }
                }
                FPRINTF(fp2, "      </tr>\n");
                FPRINTF(fp, "\n");
                for (k = 0; k < j; k++)
                    FPRINTF(fp, "-");
                FPRINTF(fp, "\n");

                // display each player.
                for (i = 0; i < players; i++)
                {
                    // get id of the character (more speed)
                    j = id[i];
                    FPRINTF(fp2, "      <tr>\n");
                    // displaying the character name
                    {           // without/with 'GM' display
                        strcpy(temp, char_dat[j].name);
                        l = isGM(char_dat[j].account_id);
                        {
                            if (l >= online_gm_display_min_level)
                                FPRINTF(fp, "%-24s (GM) ", temp);
                            else
                                FPRINTF(fp, "%-24s      ", temp);
                        }
                        // name of the character in the html (no < >, because that create problem in html code)
                        FPRINTF(fp2, "        <td>");
                        if (l >= online_gm_display_min_level)
                            FPRINTF(fp2, "<b>");
                        for (k = 0; temp[k]; k++)
                        {
                            switch (temp[k])
                            {
                                case '<':  // <
                                    FPRINTF(fp2, "&lt;");
                                    break;
                                case '>':  // >
                                    FPRINTF(fp2, "&gt;");
                                    break;
                                default:
                                    FPRINTF(fp2, "%c", temp[k]);
                                    break;
                            };
                        }
                        if (l >= online_gm_display_min_level)
                            FPRINTF(fp2, "</b> (GM)");
                        FPRINTF(fp2, "</td>\n");
                    }
                    FPRINTF(fp, "\n");
                    FPRINTF(fp2, "      </tr>\n");
                }
                FPRINTF(fp2, "    </table>\n");
                FPRINTF(fp, "\n");
            }

            // Displaying number of online players
            if (players == 0)
            {
                FPRINTF(fp2, "    <p>No user is online.</p>\n");
                FPRINTF(fp, "No user is online.\n");
                // no display if only 1 player
            }
            else if (players == 1)
            {
            }
            else
            {
                FPRINTF(fp2, "    <p>%d users are online.</p>\n", players);
                FPRINTF(fp, "%d users are online.\n", players);
            }
            FPRINTF(fp2, "  </BODY>\n");
            FPRINTF(fp2, "</HTML>\n");
            fclose_(fp2);
        }
        fclose_(fp);
    }

    return;
}

//---------------------------------------------------------------------
// This function return the number of online players in all map-servers
//---------------------------------------------------------------------
static
int count_users(void)
{
    int i, users;

    users = 0;
    for (i = 0; i < MAX_MAP_SERVERS; i++)
        if (server_fd[i] >= 0)
            users += server[i].users;

    return users;
}

//----------------------------------------
// [Fate] Find inventory item based on equipment mask, return view.  ID must match view ID (!).
//----------------------------------------
static
int find_equip_view(struct mmo_charstatus *p, EPOS equipmask)
{
    int i;
    for (i = 0; i < MAX_INVENTORY; i++)
        if (p->inventory[i].nameid && p->inventory[i].amount
            && bool(p->inventory[i].equip & equipmask))
            return p->inventory[i].nameid;
    return 0;
}

//----------------------------------------
// Function to send characters to a player
//----------------------------------------
static
int mmo_char_send006b(int fd, struct char_session_data *sd)
{
    int i, j, found_num;
    struct mmo_charstatus *p;
    const int offset = 24;

    found_num = 0;
    for (i = 0; i < char_num; i++)
    {
        if (char_dat[i].account_id == sd->account_id)
        {
            sd->found_char[found_num] = i;
            found_num++;
            if (found_num == 9)
                break;
        }
    }
    for (i = found_num; i < 9; i++)
        sd->found_char[i] = -1;

    memset(WFIFOP(fd, 0), 0, offset + found_num * 106);
    WFIFOW(fd, 0) = 0x6b;
    WFIFOW(fd, 2) = offset + found_num * 106;

    for (i = 0; i < found_num; i++)
    {
        p = &char_dat[sd->found_char[i]];
        j = offset + (i * 106); // increase speed of code

        WFIFOL(fd, j) = p->char_id;
        WFIFOL(fd, j + 4) = p->base_exp;
        WFIFOL(fd, j + 8) = p->zeny;
        WFIFOL(fd, j + 12) = p->job_exp;
        WFIFOL(fd, j + 16) = 0;    //p->job_level; // [Fate] We no longer reveal this to the player, as its meaning is weird.

        WFIFOW(fd, j + 20) = find_equip_view(p, EPOS::SHOES);
        WFIFOW(fd, j + 22) = find_equip_view(p, EPOS::GLOVES);
        WFIFOW(fd, j + 24) = find_equip_view(p, EPOS::CAPE);
        WFIFOW(fd, j + 26) = find_equip_view(p, EPOS::TORSO);
        WFIFOL(fd, j + 28) = static_cast<uint16_t>(p->option);

        WFIFOL(fd, j + 32) = p->karma;
        WFIFOL(fd, j + 36) = p->manner;

        WFIFOW(fd, j + 40) = p->status_point;
        WFIFOW(fd, j + 42) = (p->hp > 0x7fff) ? 0x7fff : p->hp;
        WFIFOW(fd, j + 44) = (p->max_hp > 0x7fff) ? 0x7fff : p->max_hp;
        WFIFOW(fd, j + 46) = (p->sp > 0x7fff) ? 0x7fff : p->sp;
        WFIFOW(fd, j + 48) = (p->max_sp > 0x7fff) ? 0x7fff : p->max_sp;
        WFIFOW(fd, j + 50) = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // p->speed;
        WFIFOW(fd, j + 52) = p->species;
        WFIFOW(fd, j + 54) = p->hair;
//      WFIFOW(fd,j+56) = p->weapon; // dont send weapon since TMW does not support it
        WFIFOW(fd, j + 56) = 0;
        WFIFOW(fd, j + 58) = p->base_level;
        WFIFOW(fd, j + 60) = p->skill_point;
        WFIFOW(fd, j + 62) = p->head_bottom;
        WFIFOW(fd, j + 64) = p->shield;
        WFIFOW(fd, j + 66) = p->head_top;
        WFIFOW(fd, j + 68) = p->head_mid;
        WFIFOW(fd, j + 70) = p->hair_color;
        WFIFOW(fd, j + 72) = find_equip_view(p, EPOS::MISC2);
//      WFIFOW(fd,j+72) = p->clothes_color;

        memcpy(WFIFOP(fd, j + 74), p->name, 24);

        WFIFOB(fd, j + 98) = min(p->attrs[ATTR::STR], 255);
        WFIFOB(fd, j + 99) = min(p->attrs[ATTR::AGI], 255);
        WFIFOB(fd, j + 100) = min(p->attrs[ATTR::VIT], 255);
        WFIFOB(fd, j + 101) = min(p->attrs[ATTR::INT], 255);
        WFIFOB(fd, j + 102) = min(p->attrs[ATTR::DEX], 255);
        WFIFOB(fd, j + 103) = min(p->attrs[ATTR::LUK], 255);
        WFIFOB(fd, j + 104) = p->char_num;
    }

    WFIFOSET(fd, WFIFOW(fd, 2));

    return 0;
}

static
int set_account_reg2(int acc, int num, struct global_reg *reg)
{
    int i, c;

    c = 0;
    for (i = 0; i < char_num; i++)
    {
        if (char_dat[i].account_id == acc)
        {
            memcpy(char_dat[i].account_reg2, reg,
                    sizeof(char_dat[i].account_reg2));
            char_dat[i].account_reg2_num = num;
            c++;
        }
    }
    return c;
}

// Divorce a character from it's partner and let the map server know
static
int char_divorce(struct mmo_charstatus *cs)
{
    int i;
    uint8_t buf[10];

    if (cs == NULL)
        return 0;

    if (cs->partner_id <= 0)
    {
        WBUFW(buf, 0) = 0x2b12;
        WBUFL(buf, 2) = cs->char_id;
        WBUFL(buf, 6) = 0;     // partner id 0 means failure
        mapif_sendall(buf, 10);
        return 0;
    }

    WBUFW(buf, 0) = 0x2b12;
    WBUFL(buf, 2) = cs->char_id;

    for (i = 0; i < char_num; i++)
    {
        if (char_dat[i].char_id == cs->partner_id
            && char_dat[i].partner_id == cs->char_id)
        {
            WBUFL(buf, 6) = cs->partner_id;
            mapif_sendall(buf, 10);
            cs->partner_id = 0;
            char_dat[i].partner_id = 0;
            return 0;
        }
        // The other char doesn't have us as their partner, so just clear our partner
        // Don't worry about this, as the map server should verify itself that the other doesn't have us as a partner, and so won't mess with their marriage
        else if (char_dat[i].char_id == cs->partner_id)
        {
            WBUFL(buf, 6) = cs->partner_id;
            mapif_sendall(buf, 10);
            cs->partner_id = 0;
            return 0;
        }
    }

    // Our partner wasn't found, so just clear our marriage
    WBUFL(buf, 6) = cs->partner_id;
    cs->partner_id = 0;
    mapif_sendall(buf, 10);

    return 0;
}

//----------------------------------------------------------------------
// Force disconnection of an online player (with account value) by [Yor]
//----------------------------------------------------------------------
static
int disconnect_player(int accound_id)
{
    int i;
    struct char_session_data *sd;

    // disconnect player if online on char-server
    for (i = 0; i < fd_max; i++)
    {
        if (session[i] && (sd = (struct char_session_data*)session[i]->session_data))
        {
            if (sd->account_id == accound_id)
            {
                session[i]->eof = 1;
                return 1;
            }
        }
    }

    return 0;
}

// キャラ削除に伴うデータ削除
static
int char_delete(struct mmo_charstatus *cs)
{
    // パーティー脱退
    if (cs->party_id)
        inter_party_leave(cs->party_id, cs->account_id);
    // 離婚
    if (cs->partner_id)
        char_divorce(cs);

    // Force the character (and all on the same account) to leave all map servers
    {
        unsigned char buf[6];
        WBUFW(buf, 0) = 0x2afe;
        WBUFL(buf, 2) = cs->account_id;
        mapif_sendall(buf, 6);
    }

    return 0;
}

static
void parse_tologin(int fd)
{
    struct char_session_data *sd;

    // only login-server can have an access to here.
    // so, if it isn't the login-server, we disconnect the session (fd != login_fd).
    if (fd != login_fd || session[fd]->eof)
    {
        if (fd == login_fd)
        {
            PRINTF("Char-server can't connect to login-server (connection #%d).\n",
                 fd);
            login_fd = -1;
        }
        delete_session(fd);
        return;
    }

    sd = (struct char_session_data*)session[fd]->session_data;

    while (RFIFOREST(fd) >= 2)
    {
//      PRINTF("parse_tologin: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

        switch (RFIFOW(fd, 0))
        {
            case 0x2711:
                if (RFIFOREST(fd) < 3)
                    return;
                if (RFIFOB(fd, 2))
                {
//              PRINTF("connect login server error : %d\n", RFIFOB(fd,2));
                    PRINTF("Can not connect to login-server.\n");
                    PRINTF("The server communication passwords (default s1/p1) is probably invalid.\n");
                    PRINTF("Also, please make sure your accounts file (default: accounts.txt) has those values present.\n");
                    PRINTF("If you changed the communication passwords, change them back at map_athena.conf and char_athena.conf\n");
                    exit(1);
                }
                else
                {
                    PRINTF("Connected to login-server (connection #%d).\n",
                            fd);
                    // if no map-server already connected, display a message...
                    int i;
                    for (i = 0; i < MAX_MAP_SERVERS; i++)
                        if (server_fd[i] >= 0 && server[i].map[0][0])   // if map-server online and at least 1 map
                            break;
                    if (i == MAX_MAP_SERVERS)
                        PRINTF("Awaiting maps from map-server.\n");
                }
                RFIFOSKIP(fd, 3);
                break;

            case 0x2713:
                if (RFIFOREST(fd) < 51)
                    return;
//          PRINTF("parse_tologin 2713 : %d\n", RFIFOB(fd,6));
                for (int i = 0; i < fd_max; i++)
                {
                    if (session[i] && (sd = (struct char_session_data*)session[i]->session_data)
                        && sd->account_id == RFIFOL(fd, 2))
                    {
                        if (RFIFOB(fd, 6) != 0)
                        {
                            WFIFOW(i, 0) = 0x6c;
                            WFIFOB(i, 2) = 0x42;
                            WFIFOSET(i, 3);
                        }
                        else if (max_connect_user == 0
                                 || count_users() < max_connect_user)
                        {
//                      if (max_connect_user == 0)
//                          PRINTF("max_connect_user (unlimited) -> accepted.\n");
//                      else
//                          PRINTF("count_users(): %d < max_connect_user (%d) -> accepted.\n", count_users(), max_connect_user);
                            memcpy(sd->email, RFIFOP(fd, 7), 40);
                            if (e_mail_check(sd->email) == 0)
                                strzcpy(sd->email, "a@a.com", 40); // default e-mail
                            sd->connect_until_time = static_cast<time_t>(RFIFOL(fd, 47));
                            // send characters to player
                            mmo_char_send006b(i, sd);
                        }
                        else
                        {
                            // refuse connection: too much online players
//                      PRINTF("count_users(): %d < max_connect_use (%d) -> fail...\n", count_users(), max_connect_user);
                            WFIFOW(i, 0) = 0x6c;
                            WFIFOW(i, 2) = 0;
                            WFIFOSET(i, 3);
                        }
                        break;
                    }
                }
                RFIFOSKIP(fd, 51);
                break;

                // Receiving of an e-mail/time limit from the login-server (answer of a request because a player comes back from map-server to char-server) by [Yor]
            case 0x2717:
                if (RFIFOREST(fd) < 50)
                    return;
                for (int i = 0; i < fd_max; i++)
                {
                    if (session[i] && (sd = (struct char_session_data*)session[i]->session_data))
                    {
                        if (sd->account_id == RFIFOL(fd, 2))
                        {
                            memcpy(sd->email, RFIFOP(fd, 6), 40);
                            if (e_mail_check(sd->email) == 0)
                                strzcpy(sd->email, "a@a.com", 40); // default e-mail
                            sd->connect_until_time = static_cast<time_t>(RFIFOL(fd, 46));
                            break;
                        }
                    }
                }
                RFIFOSKIP(fd, 50);
                break;

            case 0x2721:       // gm reply
                if (RFIFOREST(fd) < 10)
                    return;
                {
                    unsigned char buf[10];
                    WBUFW(buf, 0) = 0x2b0b;
                    WBUFL(buf, 2) = RFIFOL(fd, 2);    // account
                    WBUFL(buf, 6) = RFIFOL(fd, 6);    // GM level
                    mapif_sendall(buf, 10);
//          PRINTF("parse_tologin: To become GM answer: char -> map.\n");
                }
                RFIFOSKIP(fd, 10);
                break;

            case 0x2723:       // changesex reply (modified by [Yor])
                if (RFIFOREST(fd) < 7)
                    return;
                {
                    int acc, sex, i, j;
                    unsigned char buf[7];
                    acc = RFIFOL(fd, 2);
                    sex = RFIFOB(fd, 6);
                    RFIFOSKIP(fd, 7);
                    if (acc > 0)
                    {
                        for (i = 0; i < char_num; i++)
                        {
                            if (char_dat[i].account_id == acc)
                            {
                                char_dat[i].sex = sex;
//                      auth_fifo[i].sex = sex;
                                // to avoid any problem with equipment and invalid sex, equipment is unequiped.
                                for (j = 0; j < MAX_INVENTORY; j++)
                                {
                                    if (char_dat[i].inventory[j].nameid
                                        && bool(char_dat[i].inventory[j].equip))
                                        char_dat[i].inventory[j].equip = EPOS::ZERO;
                                }
                                char_dat[i].weapon = 0;
                                char_dat[i].shield = 0;
                                char_dat[i].head_top = 0;
                                char_dat[i].head_mid = 0;
                                char_dat[i].head_bottom = 0;
                            }
                        }
                        // disconnect player if online on char-server
                        disconnect_player(acc);
                    }
                    WBUFW(buf, 0) = 0x2b0d;
                    WBUFL(buf, 2) = acc;
                    WBUFB(buf, 6) = sex;
                    mapif_sendall(buf, 7);
                }
                break;

            case 0x2726:       // Request to send a broadcast message (no answer)
                if (RFIFOREST(fd) < 8
                    || RFIFOREST(fd) < (8 + RFIFOL(fd, 4)))
                    return;
                if (RFIFOL(fd, 4) < 1)
                    CHAR_LOG("Receiving a message for broadcast, but message is void.\n");
                else
                {
                    int i;
                    // at least 1 map-server
                    for (i = 0; i < MAX_MAP_SERVERS; i++)
                        if (server_fd[i] >= 0)
                            break;
                    if (i == MAX_MAP_SERVERS)
                        CHAR_LOG("'ladmin': Receiving a message for broadcast, but no map-server is online.\n");
                    else
                    {
                        uint8_t buf[128];
                        char message[RFIFOL(fd, 4) + 1];   // +1 to add a null terminated if not exist in the packet
                        int lp;
                        char *p;
                        memset(message, '\0', sizeof(message));
                        memcpy(message, RFIFOP(fd, 8), RFIFOL(fd, 4));
                        message[sizeof(message) - 1] = '\0';
                        remove_control_chars(message);
                        // remove all first spaces
                        p = message;
                        while (p[0] == ' ')
                            p++;
                        // if message is only composed of spaces
                        if (p[0] == '\0')
                            CHAR_LOG("Receiving a message for broadcast, but message is only a lot of spaces.\n");
                        // else send message to all map-servers
                        else
                        {
                            if (RFIFOW(fd, 2) == 0)
                            {
                                const char *message_ptr = message;
                                CHAR_LOG("'ladmin': Receiving a message for broadcast (message (in yellow): %s)\n",
                                     message_ptr);
                                lp = 4;
                            }
                            else
                            {
                                const char *message_ptr = message;
                                CHAR_LOG("'ladmin': Receiving a message for broadcast (message (in blue): %s)\n",
                                     message_ptr);
                                lp = 8;
                            }
                            // send broadcast to all map-servers
                            WBUFW(buf, 0) = 0x3800;
                            WBUFW(buf, 2) = lp + sizeof(message);
                            WBUFL(buf, 4) = 0x65756c62;    // only write if in blue (lp = 8)
                            memcpy(WBUFP(buf, lp), message, sizeof(message));
                            mapif_sendall(buf, WBUFW(buf, 2));
                        }
                    }
                }
                RFIFOSKIP(fd, 8 + RFIFOL(fd, 4));
                break;

                // account_reg2変更通知
            case 0x2729:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                {
                    struct global_reg reg[ACCOUNT_REG2_NUM];
                    unsigned char buf[4096];
                    int j, p, acc;
                    acc = RFIFOL(fd, 4);
                    for (p = 8, j = 0;
                         p < RFIFOW(fd, 2) && j < ACCOUNT_REG2_NUM;
                         p += 36, j++)
                    {
                        memcpy(reg[j].str, RFIFOP(fd, p), 32);
                        reg[j].value = RFIFOL(fd, p + 32);
                    }
                    set_account_reg2(acc, j, reg);
                    // 同垢ログインを禁止していれば送る必要は無い
                    memcpy(buf, RFIFOP(fd, 0), RFIFOW(fd, 2));
                    WBUFW(buf, 0) = 0x2b11;
                    mapif_sendall(buf, WBUFW(buf, 2));
                    RFIFOSKIP(fd, RFIFOW(fd, 2));
//          PRINTF("char: save_account_reg_reply\n");
                }
                break;

            case 0x7924:
            {                   // [Fate] Itemfrob package: forwarded from login-server
                if (RFIFOREST(fd) < 10)
                    return;
                int source_id = RFIFOL(fd, 2);
                int dest_id = RFIFOL(fd, 6);
                unsigned char buf[10];

                WBUFW(buf, 0) = 0x2afa;
                WBUFL(buf, 2) = source_id;
                WBUFL(buf, 6) = dest_id;

                mapif_sendall(buf, 10);    // forward package to map servers
                for (int i = 0; i < char_num; i++)
                {
                    struct mmo_charstatus *c = char_dat + i;
                    struct storage *s = account2storage(c->account_id);
                    int changes = 0;
                    int j;
#define FIX(v) if (v == source_id) {v = dest_id; ++changes; }
                    for (j = 0; j < MAX_INVENTORY; j++)
                        FIX(c->inventory[j].nameid);
                    for (j = 0; j < MAX_CART; j++)
                        FIX(c->cart[j].nameid);
                    FIX(c->weapon);
                    FIX(c->shield);
                    FIX(c->head_top);
                    FIX(c->head_mid);
                    FIX(c->head_bottom);

                    if (s)
                        for (j = 0; j < s->storage_amount; j++)
                            FIX(s->storage_[j].nameid);
#undef FIX
                    if (changes)
                        CHAR_LOG("itemfrob(%d -> %d):  `%s'(%d, account %d): changed %d times\n",
                             source_id, dest_id, c->name, c->char_id,
                             c->account_id, changes);

                }

                mmo_char_sync();
                inter_storage_save();
                RFIFOSKIP(fd, 10);
                break;
            }

                // Account deletion notification (from login-server)
            case 0x2730:
                if (RFIFOREST(fd) < 6)
                    return;
                // Deletion of all characters of the account
                for (int i = 0; i < char_num; i++)
                {
                    if (char_dat[i].account_id == RFIFOL(fd, 2))
                    {
                        char_delete(&char_dat[i]);
                        if (i < char_num - 1)
                        {
                            memcpy(&char_dat[i], &char_dat[char_num - 1],
                                    sizeof(struct mmo_charstatus));
                            // if moved character owns to deleted account, check again it's character
                            if (char_dat[i].account_id == RFIFOL(fd, 2))
                            {
                                i--;
                                // Correct moved character reference in the character's owner by [Yor]
                            }
                            else
                            {
                                int j, k;
                                struct char_session_data *sd2;
                                for (j = 0; j < fd_max; j++)
                                {
                                    if (session[j]
                                        && (sd2 = (struct char_session_data*)session[j]->session_data)
                                        && sd2->account_id ==
                                        char_dat[char_num - 1].account_id)
                                    {
                                        for (k = 0; k < 9; k++)
                                        {
                                            if (sd2->found_char[k] ==
                                                char_num - 1)
                                            {
                                                sd2->found_char[k] = i;
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        char_num--;
                    }
                }
                // Deletion of the storage
                inter_storage_delete(RFIFOL(fd, 2));
                // send to all map-servers to disconnect the player
                {
                    unsigned char buf[6];
                    WBUFW(buf, 0) = 0x2b13;
                    WBUFL(buf, 2) = RFIFOL(fd, 2);
                    mapif_sendall(buf, 6);
                }
                // disconnect player if online on char-server
                disconnect_player(RFIFOL(fd, 2));
                RFIFOSKIP(fd, 6);
                break;

                // State change of account/ban notification (from login-server) by [Yor]
            case 0x2731:
                if (RFIFOREST(fd) < 11)
                    return;
                // send to all map-servers to disconnect the player
                {
                    unsigned char buf[11];
                    WBUFW(buf, 0) = 0x2b14;
                    WBUFL(buf, 2) = RFIFOL(fd, 2);
                    WBUFB(buf, 6) = RFIFOB(fd, 6);    // 0: change of statut, 1: ban
                    WBUFL(buf, 7) = RFIFOL(fd, 7);    // status or final date of a banishment
                    mapif_sendall(buf, 11);
                }
                // disconnect player if online on char-server
                disconnect_player(RFIFOL(fd, 2));
                RFIFOSKIP(fd, 11);
                break;

                // Receiving GM acounts info from login-server (by [Yor])
            case 0x2732:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                {
                    uint8_t buf[32000];
                    if (gm_account != NULL)
                        free(gm_account);
                    CREATE(gm_account, struct gm_account, (RFIFOW(fd, 2) - 4) / 5);
                    GM_num = 0;
                    for (int i = 4; i < RFIFOW(fd, 2); i = i + 5)
                    {
                        gm_account[GM_num].account_id = RFIFOL(fd, i);
                        gm_account[GM_num].level = (int) RFIFOB(fd, i + 4);
                        //PRINTF("GM account: %d -> level %d\n", gm_account[GM_num].account_id, gm_account[GM_num].level);
                        GM_num++;
                    }
                    PRINTF("From login-server: receiving of %d GM accounts information.\n",
                         GM_num);
                    CHAR_LOG("From login-server: receiving of %d GM accounts information.\n",
                         GM_num);
                    create_online_files(); // update online players files (perhaps some online players change of GM level)
                    // send new gm acccounts level to map-servers
                    memcpy(buf, RFIFOP(fd, 0), RFIFOW(fd, 2));
                    WBUFW(buf, 0) = 0x2b15;
                    mapif_sendall(buf, RFIFOW(fd, 2));
                }
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

            case 0x2741:       // change password reply
                if (RFIFOREST(fd) < 7)
                    return;
                {
                    int acc, status, i;
                    acc = RFIFOL(fd, 2);
                    status = RFIFOB(fd, 6);

                    for (i = 0; i < fd_max; i++)
                    {
                        if (session[i] && (sd = (struct char_session_data*)session[i]->session_data))
                        {
                            if (sd->account_id == acc)
                            {
                                WFIFOW(i, 0) = 0x62;
                                WFIFOB(i, 2) = status;
                                WFIFOSET(i, 3);
                                break;
                            }
                        }
                    }
                }
                RFIFOSKIP(fd, 7);
                break;

            default:
                session[fd]->eof = 1;
                return;
        }
    }
}

//--------------------------------
// Map-server anti-freeze system
//--------------------------------
static
void map_anti_freeze_system(TimerData *, tick_t)
{
    int i;

    //PRINTF("Entering in map_anti_freeze_system function to check freeze of servers.\n");
    for (i = 0; i < MAX_MAP_SERVERS; i++)
    {
        if (server_fd[i] >= 0)
        {                       // if map-server is online
            //PRINTF("map_anti_freeze_system: server #%d, flag: %d.\n", i, server_freezeflag[i]);
            if (server_freezeflag[i]-- < 1)
            {                   // Map-server anti-freeze system. Counter. 5 ok, 4...0 freezed
                PRINTF("Map-server anti-freeze system: char-server #%d is freezed -> disconnection.\n",
                     i);
                CHAR_LOG("Map-server anti-freeze system: char-server #%d is freezed -> disconnection.\n",
                     i);
                session[server_fd[i]]->eof = 1;
            }
        }
    }
}

static
void parse_frommap(int fd)
{
    int i, j;
    int id;

    for (id = 0; id < MAX_MAP_SERVERS; id++)
        if (server_fd[id] == fd)
            break;
    if (id == MAX_MAP_SERVERS || session[fd]->eof)
    {
        if (id < MAX_MAP_SERVERS)
        {
            PRINTF("Map-server %d (session #%d) has disconnected.\n", id,
                    fd);
            memset(&server[id], 0, sizeof(struct mmo_map_server));
            server_fd[id] = -1;
            for (j = 0; j < char_num; j++)
                if (online_chars[j] == fd)
                    online_chars[j] = -1;
            create_online_files(); // update online players files (to remove all online players of this server)
        }
        delete_session(fd);
        return;
    }

    while (RFIFOREST(fd) >= 2)
    {
//      PRINTF("parse_frommap: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

        switch (RFIFOW(fd, 0))
        {
                // request from map-server to reload GM accounts. Transmission to login-server (by Yor)
            case 0x2af7:
                if (login_fd > 0)
                {               // don't send request if no login-server
                    WFIFOW(login_fd, 0) = 0x2709;
                    WFIFOSET(login_fd, 2);
//              PRINTF("char : request from map-server to reload GM accounts -> login-server.\n");
                }
                RFIFOSKIP(fd, 2);
                break;

                // Receiving map names list from the map-server
            case 0x2afa:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                memset(server[id].map, 0, sizeof(server[id].map));
                j = 0;
                for (i = 4; i < RFIFOW(fd, 2); i += 16)
                {
                    memcpy(server[id].map[j], RFIFOP(fd, i), 16);
//              PRINTF("set map %d.%d : %s\n", id, j, server[id].map[j]);
                    j++;
                }
                {
                    unsigned char *p = (unsigned char *) &server[id].ip;
                    PRINTF("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %d.\n",
                         id, j, p[0], p[1], p[2], p[3], server[id].port);
                    PRINTF("Map-server %d loading complete.\n", id);
                    CHAR_LOG("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %d. Map-server %d loading complete.\n",
                         id, j, p[0], p[1], p[2], p[3],
                         server[id].port, id);
                }
                WFIFOW(fd, 0) = 0x2afb;
                WFIFOB(fd, 2) = 0;
                memcpy(WFIFOP(fd, 3), wisp_server_name, 24);  // name for wisp to player
                WFIFOSET(fd, 27);
                {
                    unsigned char buf[16384];
                    int x;
                    if (j == 0)
                    {
                        PRINTF("WARNING: Map-Server %d have NO map.\n", id);
                        CHAR_LOG("WARNING: Map-Server %d have NO map.\n",
                                  id);
                        // Transmitting maps information to the other map-servers
                    }
                    else
                    {
                        WBUFW(buf, 0) = 0x2b04;
                        WBUFW(buf, 2) = j * 16 + 10;
                        WBUFL(buf, 4) = server[id].ip;
                        WBUFW(buf, 8) = server[id].port;
                        memcpy(WBUFP(buf, 10), RFIFOP(fd, 4), j * 16);
                        mapif_sendallwos(fd, buf, WBUFW(buf, 2));
                    }
                    // Transmitting the maps of the other map-servers to the new map-server
                    for (x = 0; x < MAX_MAP_SERVERS; x++)
                    {
                        if (server_fd[x] >= 0 && x != id)
                        {
                            WFIFOW(fd, 0) = 0x2b04;
                            WFIFOL(fd, 4) = server[x].ip;
                            WFIFOW(fd, 8) = server[x].port;
                            j = 0;
                            for (i = 0; i < MAX_MAP_PER_SERVER; i++)
                                if (server[x].map[i][0])
                                    memcpy(WFIFOP(fd, 10 + (j++) * 16),
                                            server[x].map[i], 16);
                            if (j > 0)
                            {
                                WFIFOW(fd, 2) = j * 16 + 10;
                                WFIFOSET(fd, WFIFOW(fd, 2));
                            }
                        }
                    }
                }
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

                // 認証要求
            case 0x2afc:
                if (RFIFOREST(fd) < 22)
                    return;
                //PRINTF("auth_fifo search: account: %d, char: %d, secure: %08x-%08x\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14));
                for (i = 0; i < AUTH_FIFO_SIZE; i++)
                {
                    if (auth_fifo[i].account_id == RFIFOL(fd, 2) &&
                        auth_fifo[i].char_id == RFIFOL(fd, 6) &&
                        auth_fifo[i].login_id1 == RFIFOL(fd, 10) &&
                        // here, it's the only area where it's possible that we doesn't know login_id2 (map-server asks just after 0x72 packet, that doesn't given the value)
                        (auth_fifo[i].login_id2 == RFIFOL(fd, 14) || RFIFOL(fd, 14) == 0) &&  // relate to the versions higher than 18
                        (!check_ip_flag || auth_fifo[i].ip == RFIFOL(fd, 18))
                        && !auth_fifo[i].delflag)
                    {
                        auth_fifo[i].delflag = 1;
                        WFIFOW(fd, 0) = 0x2afd;
                        WFIFOW(fd, 2) = 18 + sizeof(struct mmo_charstatus);
                        WFIFOL(fd, 4) = RFIFOL(fd, 2);
                        WFIFOL(fd, 8) = auth_fifo[i].login_id2;
                        WFIFOL(fd, 12) = static_cast<time_t>(auth_fifo[i].connect_until_time);
                        char_dat[auth_fifo[i].char_pos].sex =
                            auth_fifo[i].sex;
                        WFIFOW(fd, 16) = auth_fifo[i].packet_tmw_version;
                        FPRINTF(stderr,
                                 "From queue index %d: recalling packet version %d\n",
                                 i, auth_fifo[i].packet_tmw_version);
                        memcpy(WFIFOP(fd, 18),
                                &char_dat[auth_fifo[i].char_pos],
                                sizeof(struct mmo_charstatus));
                        WFIFOSET(fd, WFIFOW(fd, 2));
                        //PRINTF("auth_fifo search success (auth #%d, account %d, character: %d).\n", i, RFIFOL(fd,2), RFIFOL(fd,6));
                        break;
                    }
                }
                if (i == AUTH_FIFO_SIZE)
                {
                    WFIFOW(fd, 0) = 0x2afe;
                    WFIFOL(fd, 2) = RFIFOL(fd, 2);
                    WFIFOSET(fd, 6);
                    PRINTF("auth_fifo search error! account %d not authentified.\n",
                         RFIFOL(fd, 2));
                }
                RFIFOSKIP(fd, 22);
                break;

                // MAPサーバー上のユーザー数受信
            case 0x2aff:
                if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                server[id].users = RFIFOW(fd, 4);
                if (anti_freeze_enable)
                    server_freezeflag[id] = 5;  // Map anti-freeze system. Counter. 5 ok, 4...0 freezed
                // remove all previously online players of the server
                for (i = 0; i < char_num; i++)
                    if (online_chars[i] == id)
                        online_chars[i] = -1;
                // add online players in the list by [Yor]
                for (i = 0; i < server[id].users; i++)
                {
                    int char_id = RFIFOL(fd, 6 + i * 4);
                    for (j = 0; j < char_num; j++)
                        if (char_dat[j].char_id == char_id)
                        {
                            online_chars[j] = id;
                            //PRINTF("%d\n", char_id);
                            break;
                        }
                }
                if (update_online < TimeT::now())
                {
                    // Time is done
                    update_online = static_cast<time_t>(TimeT::now()) + 8;
                    create_online_files();
                    // only every 8 sec. (normally, 1 server send users every 5 sec.) Don't update every time, because that takes time, but only every 2 connection.
                    // it set to 8 sec because is more than 5 (sec) and if we have more than 1 map-server, informations can be received in shifted.
                }
                RFIFOSKIP(fd, 6 + i * 4);
                break;

                // キャラデータ保存
            case 0x2b01:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                for (i = 0; i < char_num; i++)
                {
                    if (char_dat[i].account_id == RFIFOL(fd, 4) &&
                        char_dat[i].char_id == RFIFOL(fd, 8))
                        break;
                }
                if (i != char_num)
                    memcpy(&char_dat[i], RFIFOP(fd, 12),
                            sizeof(struct mmo_charstatus));
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

                // キャラセレ要求
            case 0x2b02:
                if (RFIFOREST(fd) < 18)
                    return;
                if (auth_fifo_pos >= AUTH_FIFO_SIZE)
                    auth_fifo_pos = 0;
                //PRINTF("auth_fifo set (auth #%d) - account: %d, secure: %08x-%08x\n", auth_fifo_pos, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));
                auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd, 2);
                auth_fifo[auth_fifo_pos].char_id = 0;
                auth_fifo[auth_fifo_pos].login_id1 = RFIFOL(fd, 6);
                auth_fifo[auth_fifo_pos].login_id2 = RFIFOL(fd, 10);
                auth_fifo[auth_fifo_pos].delflag = 2;
                auth_fifo[auth_fifo_pos].char_pos = 0;
                auth_fifo[auth_fifo_pos].connect_until_time = TimeT();    // unlimited/unknown time by default (not display in map-server)
                auth_fifo[auth_fifo_pos].ip = RFIFOL(fd, 14);
                auth_fifo_pos++;
                WFIFOW(fd, 0) = 0x2b03;
                WFIFOL(fd, 2) = RFIFOL(fd, 2);
                WFIFOB(fd, 6) = 0;
                WFIFOSET(fd, 7);
                RFIFOSKIP(fd, 18);
                break;

                // マップサーバー間移動要求
            case 0x2b05:
                if (RFIFOREST(fd) < 49)
                    return;
                if (auth_fifo_pos >= AUTH_FIFO_SIZE)
                    auth_fifo_pos = 0;
                WFIFOW(fd, 0) = 0x2b06;
                memcpy(WFIFOP(fd, 2), RFIFOP(fd, 2), 42);
                //PRINTF("auth_fifo set (auth#%d) - account: %d, secure: 0x%08x-0x%08x\n", auth_fifo_pos, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));
                auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd, 2);
                auth_fifo[auth_fifo_pos].char_id = RFIFOL(fd, 14);
                auth_fifo[auth_fifo_pos].login_id1 = RFIFOL(fd, 6);
                auth_fifo[auth_fifo_pos].login_id2 = RFIFOL(fd, 10);
                auth_fifo[auth_fifo_pos].delflag = 0;
                auth_fifo[auth_fifo_pos].sex = RFIFOB(fd, 44);
                auth_fifo[auth_fifo_pos].connect_until_time = TimeT();    // unlimited/unknown time by default (not display in map-server)
                auth_fifo[auth_fifo_pos].ip = RFIFOL(fd, 45);
                for (i = 0; i < char_num; i++)
                    if (char_dat[i].account_id == RFIFOL(fd, 2) &&
                        char_dat[i].char_id == RFIFOL(fd, 14))
                    {
                        auth_fifo[auth_fifo_pos].char_pos = i;
                        auth_fifo_pos++;
                        WFIFOL(fd, 6) = 0;
                        break;
                    }
                if (i == char_num)
                    WFIFOW(fd, 6) = 1;
                WFIFOSET(fd, 44);
                RFIFOSKIP(fd, 49);
                break;

                // キャラ名検索
            case 0x2b08:
                if (RFIFOREST(fd) < 6)
                    return;
                for (i = 0; i < char_num; i++)
                {
                    if (char_dat[i].char_id == RFIFOL(fd, 2))
                        break;
                }
                WFIFOW(fd, 0) = 0x2b09;
                WFIFOL(fd, 2) = RFIFOL(fd, 2);
                if (i != char_num)
                    memcpy(WFIFOP(fd, 6), char_dat[i].name, 24);
                else
                    memcpy(WFIFOP(fd, 6), unknown_char_name, 24);
                WFIFOSET(fd, 30);
                RFIFOSKIP(fd, 6);
                break;

                // it is a request to become GM
            case 0x2b0a:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
//          PRINTF("parse_frommap: change gm -> login, account: %d, pass: '%s'.\n", RFIFOL(fd,4), RFIFOP(fd,8));
                if (login_fd > 0)
                {               // don't send request if no login-server
                    WFIFOW(login_fd, 0) = 0x2720;
                    memcpy(WFIFOP(login_fd, 2), RFIFOP(fd, 2),
                            RFIFOW(fd, 2) - 2);
                    WFIFOSET(login_fd, RFIFOW(fd, 2));
                }
                else
                {
                    WFIFOW(fd, 0) = 0x2b0b;
                    WFIFOL(fd, 2) = RFIFOL(fd, 4);
                    WFIFOL(fd, 6) = 0;
                    WFIFOSET(fd, 10);
                }
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

                // Map server send information to change an email of an account -> login-server
            case 0x2b0c:
                if (RFIFOREST(fd) < 86)
                    return;
                if (login_fd > 0)
                {               // don't send request if no login-server
                    memcpy(WFIFOP(login_fd, 0), RFIFOP(fd, 0), 86);  // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
                    WFIFOW(login_fd, 0) = 0x2722;
                    WFIFOSET(login_fd, 86);
                }
                RFIFOSKIP(fd, 86);
                break;

                // Map server ask char-server about a character name to do some operations (all operations are transmitted to login-server)
            case 0x2b0e:
                if (RFIFOREST(fd) < 44)
                    return;
                {
                    char character_name[24];
                    int acc = RFIFOL(fd, 2);  // account_id of who ask (-1 if nobody)
                    memcpy(character_name, RFIFOP(fd, 6), 24);
                    character_name[sizeof(character_name) - 1] = '\0';
                    // prepare answer
                    WFIFOW(fd, 0) = 0x2b0f;    // answer
                    WFIFOL(fd, 2) = acc;   // who want do operation
                    WFIFOW(fd, 30) = RFIFOW(fd, 30);  // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex
                    // search character
                    i = search_character_index(character_name);
                    if (i >= 0)
                    {
                        memcpy(WFIFOP(fd, 6), search_character_name(i), 24); // put correct name if found
                        WFIFOW(fd, 32) = 0;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                        switch (RFIFOW(fd, 30))
                        {
                            case 1:    // block
                                if (acc == -1
                                    || isGM(acc) >=
                                    isGM(char_dat[i].account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2724;
                                        WFIFOL(login_fd, 2) = char_dat[i].account_id;  // account value
                                        WFIFOL(login_fd, 6) = 5;   // status of the account
                                        WFIFOSET(login_fd, 10);
//                          PRINTF("char : status -> login: account %d, status: %d \n", char_dat[i].account_id, 5);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 2:    // ban
                                if (acc == -1
                                    || isGM(acc) >=
                                    isGM(char_dat[i].account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2725;
                                        WFIFOL(login_fd, 2) = char_dat[i].account_id;  // account value
                                        WFIFOW(login_fd, 6) = RFIFOW(fd, 32); // year
                                        WFIFOW(login_fd, 8) = RFIFOW(fd, 34); // month
                                        WFIFOW(login_fd, 10) = RFIFOW(fd, 36);    // day
                                        WFIFOW(login_fd, 12) = RFIFOW(fd, 38);    // hour
                                        WFIFOW(login_fd, 14) = RFIFOW(fd, 40);    // minute
                                        WFIFOW(login_fd, 16) = RFIFOW(fd, 42);    // second
                                        WFIFOSET(login_fd, 18);
//                          PRINTF("char : status -> login: account %d, ban: %dy %dm %dd %dh %dmn %ds\n",
//                                 char_dat[i].account_id, (short)RFIFOW(fd,32), (short)RFIFOW(fd,34), (short)RFIFOW(fd,36), (short)RFIFOW(fd,38), (short)RFIFOW(fd,40), (short)RFIFOW(fd,42));
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 3:    // unblock
                                if (acc == -1
                                    || isGM(acc) >=
                                    isGM(char_dat[i].account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2724;
                                        WFIFOL(login_fd, 2) = char_dat[i].account_id;  // account value
                                        WFIFOL(login_fd, 6) = 0;   // status of the account
                                        WFIFOSET(login_fd, 10);
//                          PRINTF("char : status -> login: account %d, status: %d \n", char_dat[i].account_id, 0);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 4:    // unban
                                if (acc == -1
                                    || isGM(acc) >=
                                    isGM(char_dat[i].account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x272a;
                                        WFIFOL(login_fd, 2) = char_dat[i].account_id;  // account value
                                        WFIFOSET(login_fd, 6);
//                          PRINTF("char : status -> login: account %d, unban request\n", char_dat[i].account_id);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 5:    // changesex
                                if (acc == -1
                                    || isGM(acc) >=
                                    isGM(char_dat[i].account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2727;
                                        WFIFOL(login_fd, 2) = char_dat[i].account_id;  // account value
                                        WFIFOSET(login_fd, 6);
//                          PRINTF("char : status -> login: account %d, change sex request\n", char_dat[i].account_id);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                        }
                    }
                    else
                    {
                        // character name not found
                        memcpy(WFIFOP(fd, 6), character_name, 24);
                        WFIFOW(fd, 32) = 1;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                    }
                    // send answer if a player ask, not if the server ask
                    if (acc != -1)
                    {
                        WFIFOSET(fd, 34);
                    }
                    RFIFOSKIP(fd, 44);
                    break;
                }

//      case 0x2b0f: not more used (available for futur usage)

                // account_reg保存要求
            case 0x2b10:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                {
                    struct global_reg reg[ACCOUNT_REG2_NUM];
                    int p, acc;
                    acc = RFIFOL(fd, 4);
                    for (p = 8, j = 0;
                         p < RFIFOW(fd, 2) && j < ACCOUNT_REG2_NUM;
                         p += 36, j++)
                    {
                        memcpy(reg[j].str, RFIFOP(fd, p), 32);
                        reg[j].value = RFIFOL(fd, p + 32);
                    }
                    set_account_reg2(acc, j, reg);
                    // loginサーバーへ送る
                    if (login_fd > 0)
                    {           // don't send request if no login-server
                        memcpy(WFIFOP(login_fd, 0), RFIFOP(fd, 0),
                                RFIFOW(fd, 2));
                        WFIFOW(login_fd, 0) = 0x2728;
                        WFIFOSET(login_fd, WFIFOW(login_fd, 2));
                    }
                    // ワールドへの同垢ログインがなければmapサーバーに送る必要はない
                    //memcpy(buf, RFIFOP(fd,0), RFIFOW(fd,2));
                    //WBUFW(buf,0) = 0x2b11;
                    //mapif_sendall(buf, WBUFW(buf,2));
                    RFIFOSKIP(fd, RFIFOW(fd, 2));
//          PRINTF("char: save_account_reg (from map)\n");
                    break;
                }

                // Map server is requesting a divorce
            case 0x2b16:
                if (RFIFOREST(fd) < 4)
                    return;
                {
                    for (i = 0; i < char_num; i++)
                        if (char_dat[i].char_id == RFIFOL(fd, 2))
                            break;

                    if (i != char_num)
                        char_divorce(&char_dat[i]);

                    RFIFOSKIP(fd, 6);
                    break;

                }

            default:
                // inter server処理に渡す
            {
                int r = inter_parse_frommap(fd);
                if (r == 1)     // 処理できた
                    break;
                if (r == 2)     // パケット長が足りない
                    return;
            }
                // inter server処理でもない場合は切断
                PRINTF("char: unknown packet 0x%04x (%zu bytes to read in buffer)! (from map).\n",
                     RFIFOW(fd, 0), RFIFOREST(fd));
                session[fd]->eof = 1;
                return;
        }
    }
}

static
int search_mapserver(const char *map)
{
    int i, j;
    char temp_map[16];
    int temp_map_len;

//  PRINTF("Searching the map-server for map '%s'... ", map);
    strzcpy(temp_map, map, sizeof(temp_map));
    if (strchr(temp_map, '.') != NULL)
        temp_map[strchr(temp_map, '.') - temp_map + 1] = '\0'; // suppress the '.gat', but conserve the '.' to be sure of the name of the map

    temp_map_len = strlen(temp_map);
    for (i = 0; i < MAX_MAP_SERVERS; i++)
        if (server_fd[i] >= 0)
            for (j = 0; server[i].map[j][0]; j++)
                //PRINTF("%s : %s = %d\n", server[i].map[j], map, strncmp(server[i].map[j], temp_map, temp_map_len));
                if (strncmp(server[i].map[j], temp_map, temp_map_len) == 0)
                {
//                  PRINTF("found -> server #%d.\n", i);
                    return i;
                }

//  PRINTF("not found.\n");
    return -1;
}

//-----------------------------------------------------
// Test to know if an IP come from LAN or WAN. by [Yor]
//-----------------------------------------------------
static
int lan_ip_check(unsigned char *p)
{
    int i;
    int lancheck = 1;

//  PRINTF("lan_ip_check: to compare: %d.%d.%d.%d, network: %d.%d.%d.%d/%d.%d.%d.%d\n",
//         p[0], p[1], p[2], p[3],
//         subneti[0], subneti[1], subneti[2], subneti[3],
//         subnetmaski[0], subnetmaski[1], subnetmaski[2], subnetmaski[3]);
    for (i = 0; i < 4; i++)
    {
        if ((subneti[i] & subnetmaski[i]) != (p[i] & subnetmaski[i]))
        {
            lancheck = 0;
            break;
        }
    }
    PRINTF("LAN test (result): %s source\033[0m.\n",
            (lancheck) ? "\033[1;36mLAN" : "\033[1;32mWAN");
    return lancheck;
}

static
void handle_x0066(int fd, struct char_session_data *sd, uint8_t rfifob_2, uint8_t *p)
{
    const char *ip = ip2str(session[fd]->client_addr.sin_addr);

    // if we activated email creation and email is default email
    if (email_creation != 0 && strcmp(sd->email, "a@a.com") == 0
        && login_fd > 0)
    {               // to modify an e-mail, login-server must be online
        WFIFOW(fd, 0) = 0x70;
        WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
        WFIFOSET(fd, 3);

        // otherwise, load the character
    }
    else
    {
        int ch;
        for (ch = 0; ch < 9; ch++)
            if (sd->found_char[ch] >= 0
                && char_dat[sd->found_char[ch]].char_num == rfifob_2)
                break;
        if (ch != 9)
        {
            CHAR_LOG("Character Selected, Account ID: %d, Character Slot: %d, Character Name: %s [%s]\n",
                 sd->account_id, rfifob_2,
                 char_dat[sd->found_char[ch]].name, ip);
            // searching map server
            int i = search_mapserver(char_dat[sd->found_char[ch]].last_point.map);
            // if map is not found, we check major cities
            if (i < 0)
            {
                int j;
                // get first online server (with a map)
                i = 0;
                for (j = 0; j < MAX_MAP_SERVERS; j++)
                    if (server_fd[j] >= 0
                        && server[j].map[0][0])
                    {   // change save point to one of map found on the server (the first)
                        i = j;
                        memcpy(char_dat[sd->found_char[ch]].last_point.map,
                                server[j].map[0], 16);
                        PRINTF("Map-server #%d found with a map: '%s'.\n",
                             j, server[j].map[0]);
                        // coordonates are unknown
                        break;
                    }
                // if no map-server is connected, we send: server closed
                if (j == MAX_MAP_SERVERS)
                {
                    WFIFOW(fd, 0) = 0x81;
                    WFIFOL(fd, 2) = 1; // 01 = Server closed
                    WFIFOSET(fd, 3);
                    return;
                }
            }
            WFIFOW(fd, 0) = 0x71;
            WFIFOL(fd, 2) = char_dat[sd->found_char[ch]].char_id;
            memcpy(WFIFOP(fd, 6),
                    char_dat[sd->found_char[ch]].last_point.map,
                    16);
            PRINTF("Character selection '%s' (account: %d, slot: %d) [%s]\n",
                 char_dat[sd->found_char[ch]].name,
                 sd->account_id, ch, ip);
            PRINTF("--Send IP of map-server. ");
            if (lan_ip_check(p))
                WFIFOL(fd, 22) = inet_addr(lan_map_ip);
            else
                WFIFOL(fd, 22) = server[i].ip;
            WFIFOW(fd, 26) = server[i].port;
            WFIFOSET(fd, 28);
            if (auth_fifo_pos >= AUTH_FIFO_SIZE)
                auth_fifo_pos = 0;
            //PRINTF("auth_fifo set #%d - account %d, char: %d, secure: %08x-%08x\n", auth_fifo_pos, sd->account_id, char_dat[sd->found_char[ch]].char_id, sd->login_id1, sd->login_id2);
            auth_fifo[auth_fifo_pos].account_id = sd->account_id;
            auth_fifo[auth_fifo_pos].char_id =
                char_dat[sd->found_char[ch]].char_id;
            auth_fifo[auth_fifo_pos].login_id1 = sd->login_id1;
            auth_fifo[auth_fifo_pos].login_id2 = sd->login_id2;
            auth_fifo[auth_fifo_pos].delflag = 0;
            auth_fifo[auth_fifo_pos].char_pos =
                sd->found_char[ch];
            auth_fifo[auth_fifo_pos].sex = sd->sex;
            auth_fifo[auth_fifo_pos].connect_until_time =
                sd->connect_until_time;
            auth_fifo[auth_fifo_pos].ip =
                session[fd]->client_addr.sin_addr.s_addr;
            auth_fifo[auth_fifo_pos].packet_tmw_version =
                sd->packet_tmw_version;
            auth_fifo_pos++;
        }
    }
}

static
void parse_char(int fd)
{
    int i, ch;
    char email[40];
    struct char_session_data *sd;
    unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;

    if (login_fd < 0 || session[fd]->eof)
    {                           // disconnect any player (already connected to char-server or coming back from map-server) if login-server is diconnected.
        if (fd == login_fd)
            login_fd = -1;
        delete_session(fd);
        return;
    }

    sd = (struct char_session_data*)session[fd]->session_data;

    while (RFIFOREST(fd) >= 2)
    {
//      if (RFIFOW(fd,0) < 30000)
//          PRINTF("parse_char: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

        switch (RFIFOW(fd, 0))
        {
            case 0x20b:        //20040622暗号化ragexe対応
                if (RFIFOREST(fd) < 19)
                    return;
                RFIFOSKIP(fd, 19);
                break;

            case 0x61:         // change password request
                if (RFIFOREST(fd) < 50)
                    return;
                {
                    WFIFOW(login_fd, 0) = 0x2740;
                    WFIFOL(login_fd, 2) = sd->account_id;
                    memcpy(WFIFOP(login_fd, 6), RFIFOP(fd, 2), 24);
                    memcpy(WFIFOP(login_fd, 30), RFIFOP(fd, 26), 24);
                    WFIFOSET(login_fd, 54);
                }
                RFIFOSKIP(fd, 50);
                break;

            case 0x65:         // 接続要求
                if (RFIFOREST(fd) < 17)
                    return;
                {
                    int GM_value;
                    if ((GM_value = isGM(RFIFOL(fd, 2))))
                        PRINTF("Account Logged On; Account ID: %d (GM level %d).\n",
                             RFIFOL(fd, 2), GM_value);
                    else
                        PRINTF("Account Logged On; Account ID: %d.\n",
                                RFIFOL(fd, 2));
                    if (sd == NULL)
                    {
                        CREATE(sd, struct char_session_data, 1);
                        session[fd]->session_data = sd;
                        memcpy(sd->email, "no mail", 40);  // put here a mail without '@' to refuse deletion if we don't receive the e-mail
                        sd->connect_until_time = TimeT(); // unknow or illimited (not displaying on map-server)
                    }
                    sd->account_id = RFIFOL(fd, 2);
                    sd->login_id1 = RFIFOL(fd, 6);
                    sd->login_id2 = RFIFOL(fd, 10);
                    sd->packet_tmw_version = RFIFOW(fd, 14);
                    sd->sex = RFIFOB(fd, 16);
                    // send back account_id
                    WFIFOL(fd, 0) = RFIFOL(fd, 2);
                    WFIFOSET(fd, 4);
                    // search authentification
                    for (i = 0; i < AUTH_FIFO_SIZE; i++)
                    {
                        if (auth_fifo[i].account_id == sd->account_id &&
                            auth_fifo[i].login_id1 == sd->login_id1 &&
                            auth_fifo[i].login_id2 == sd->login_id2 &&  // relate to the versions higher than 18
                            (!check_ip_flag
                             || auth_fifo[i].ip ==
                             session[fd]->client_addr.sin_addr.s_addr)
                            && auth_fifo[i].delflag == 2)
                        {
                            auth_fifo[i].delflag = 1;
                            if (max_connect_user == 0
                                || count_users() < max_connect_user)
                            {
                                if (login_fd > 0)
                                {   // don't send request if no login-server
                                    // request to login-server to obtain e-mail/time limit
                                    WFIFOW(login_fd, 0) = 0x2716;
                                    WFIFOL(login_fd, 2) = sd->account_id;
                                    WFIFOSET(login_fd, 6);
                                }
                                // Record client version
                                auth_fifo[i].packet_tmw_version =
                                    sd->packet_tmw_version;
                                // send characters to player
                                mmo_char_send006b(fd, sd);
                            }
                            else
                            {
                                // refuse connection (over populated)
                                WFIFOW(fd, 0) = 0x6c;
                                WFIFOW(fd, 2) = 0;
                                WFIFOSET(fd, 3);
                            }
                            break;
                        }
                    }
                    // authentification not found
                    if (i == AUTH_FIFO_SIZE)
                    {
                        if (login_fd > 0)
                        {       // don't send request if no login-server
                            WFIFOW(login_fd, 0) = 0x2712;  // ask login-server to authentify an account
                            WFIFOL(login_fd, 2) = sd->account_id;
                            WFIFOL(login_fd, 6) = sd->login_id1;
                            WFIFOL(login_fd, 10) = sd->login_id2;  // relate to the versions higher than 18
                            WFIFOB(login_fd, 14) = sd->sex;
                            WFIFOL(login_fd, 15) =
                                session[fd]->client_addr.sin_addr.s_addr;
                            WFIFOSET(login_fd, 19);
                        }
                        else
                        {       // if no login-server, we must refuse connection
                            WFIFOW(fd, 0) = 0x6c;
                            WFIFOW(fd, 2) = 0;
                            WFIFOSET(fd, 3);
                        }
                    }
                }
                RFIFOSKIP(fd, 17);
                break;

            case 0x66:         // キャラ選択
                if (!sd || RFIFOREST(fd) < 3)
                    return;
                handle_x0066(fd, sd, RFIFOB(fd, 2), p);
                RFIFOSKIP(fd, 3);
                break;

            case 0x67:         // 作成
                if (!sd || RFIFOREST(fd) < 37)
                    return;
                i = make_new_char(fd, static_cast<const uint8_t *>(RFIFOP(fd, 2)));
                if (i < 0)
                {
                    WFIFOW(fd, 0) = 0x6e;
                    WFIFOB(fd, 2) = 0x00;
                    WFIFOSET(fd, 3);
                    RFIFOSKIP(fd, 37);
                    break;
                }

                WFIFOW(fd, 0) = 0x6d;
                memset(WFIFOP(fd, 2), 0, 106);

                WFIFOL(fd, 2) = char_dat[i].char_id;
                WFIFOL(fd, 2 + 4) = char_dat[i].base_exp;
                WFIFOL(fd, 2 + 8) = char_dat[i].zeny;
                WFIFOL(fd, 2 + 12) = char_dat[i].job_exp;
                WFIFOL(fd, 2 + 16) = char_dat[i].job_level;

                WFIFOL(fd, 2 + 28) = char_dat[i].karma;
                WFIFOL(fd, 2 + 32) = char_dat[i].manner;

                WFIFOW(fd, 2 + 40) = 0x30;
                WFIFOW(fd, 2 + 42) =
                    (char_dat[i].hp > 0x7fff) ? 0x7fff : char_dat[i].hp;
                WFIFOW(fd, 2 + 44) =
                    (char_dat[i].max_hp >
                     0x7fff) ? 0x7fff : char_dat[i].max_hp;
                WFIFOW(fd, 2 + 46) =
                    (char_dat[i].sp > 0x7fff) ? 0x7fff : char_dat[i].sp;
                WFIFOW(fd, 2 + 48) =
                    (char_dat[i].max_sp >
                     0x7fff) ? 0x7fff : char_dat[i].max_sp;
                WFIFOW(fd, 2 + 50) = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // char_dat[i].speed;
                WFIFOW(fd, 2 + 52) = char_dat[i].species;
                WFIFOW(fd, 2 + 54) = char_dat[i].hair;

                WFIFOW(fd, 2 + 58) = char_dat[i].base_level;
                WFIFOW(fd, 2 + 60) = char_dat[i].skill_point;

                WFIFOW(fd, 2 + 64) = char_dat[i].shield;
                WFIFOW(fd, 2 + 66) = char_dat[i].head_top;
                WFIFOW(fd, 2 + 68) = char_dat[i].head_mid;
                WFIFOW(fd, 2 + 70) = char_dat[i].hair_color;

                memcpy(WFIFOP(fd, 2 + 74), char_dat[i].name, 24);

                WFIFOB(fd, 2 + 98) = min(char_dat[i].attrs[ATTR::STR], 255);
                WFIFOB(fd, 2 + 99) = min(char_dat[i].attrs[ATTR::AGI], 255);
                WFIFOB(fd, 2 + 100) = min(char_dat[i].attrs[ATTR::VIT], 255);
                WFIFOB(fd, 2 + 101) = min(char_dat[i].attrs[ATTR::INT], 255);
                WFIFOB(fd, 2 + 102) = min(char_dat[i].attrs[ATTR::DEX], 255);
                WFIFOB(fd, 2 + 103) = min(char_dat[i].attrs[ATTR::LUK], 255);
                WFIFOB(fd, 2 + 104) = char_dat[i].char_num;

                WFIFOSET(fd, 108);
                RFIFOSKIP(fd, 37);
                for (ch = 0; ch < 9; ch++)
                {
                    if (sd->found_char[ch] == -1)
                    {
                        sd->found_char[ch] = i;
                        break;
                    }
                }
                break;

            case 0x68:         // delete char //Yor's Fix
                if (!sd || RFIFOREST(fd) < 46)
                    return;
                memcpy(email, RFIFOP(fd, 6), 40);
                if (e_mail_check(email) == 0)
                    strzcpy(email, "a@a.com", 40); // default e-mail

                // if we activated email creation and email is default email
                if (email_creation != 0 && strcmp(sd->email, "a@a.com") == 0
                    && login_fd > 0)
                {               // to modify an e-mail, login-server must be online
                    // if sended email is incorrect e-mail
                    if (strcmp(email, "a@a.com") == 0)
                    {
                        WFIFOW(fd, 0) = 0x70;
                        WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
                        WFIFOSET(fd, 3);
                        RFIFOSKIP(fd, 46);
                        // we act like we have selected a character
                    }
                    else
                    {
                        // we change the packet to set it like selection.
                        for (i = 0; i < 9; i++)
                            if (char_dat[sd->found_char[i]].char_id ==
                                RFIFOL(fd, 2))
                            {
                                // we save new e-mail
                                memcpy(sd->email, email, 40);
                                // we send new e-mail to login-server ('online' login-server is checked before)
                                WFIFOW(login_fd, 0) = 0x2715;
                                WFIFOL(login_fd, 2) = sd->account_id;
                                memcpy(WFIFOP(login_fd, 6), email, 40);
                                WFIFOSET(login_fd, 46);
                                RFIFOSKIP(fd, 46);
                                handle_x0066(fd, sd, char_dat[sd->found_char[i]].char_num, p);
                                break;
                            }
                        if (i == 9)
                        {
                            WFIFOW(fd, 0) = 0x70;
                            WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
                            WFIFOSET(fd, 3);
                            RFIFOSKIP(fd, 46);
                        }
                    }

                    // otherwise, we delete the character
                }
                else
                {
                    /*if (strcasecmp(email, sd->email) != 0) { // if it's an invalid email
                     * WFIFOW(fd, 0) = 0x70;
                     * WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
                     * WFIFOSET(fd, 3);
                     * // if mail is correct
                     * } else { */
                    for (i = 0; i < 9; i++)
                    {
                        struct mmo_charstatus *cs = NULL;
                        if (sd->found_char[i] >= 0
                            && (cs =
                                &char_dat[sd->found_char[i]])->char_id ==
                            RFIFOL(fd, 2))
                        {
                            char_delete(cs);   // deletion process

                            if (sd->found_char[i] != char_num - 1)
                            {
                                memcpy(&char_dat[sd->found_char[i]],
                                        &char_dat[char_num - 1],
                                        sizeof(struct mmo_charstatus));
                                // Correct moved character reference in the character's owner
                                {
                                    int j, k;
                                    struct char_session_data *sd2;
                                    for (j = 0; j < fd_max; j++)
                                    {
                                        if (session[j]
                                            && (sd2 = (struct char_session_data*)
                                                session[j]->session_data)
                                            && sd2->account_id ==
                                            char_dat[char_num - 1].account_id)
                                        {
                                            for (k = 0; k < 9; k++)
                                            {
                                                if (sd2->found_char[k] ==
                                                    char_num - 1)
                                                {
                                                    sd2->found_char[k] =
                                                        sd->found_char[i];
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }
                            }

                            char_num--;
                            for (ch = i; ch < 9 - 1; ch++)
                                sd->found_char[ch] = sd->found_char[ch + 1];
                            sd->found_char[8] = -1;
                            WFIFOW(fd, 0) = 0x6f;
                            WFIFOSET(fd, 2);
                            break;
                        }
                    }

                    if (i == 9)
                    {
                        WFIFOW(fd, 0) = 0x70;
                        WFIFOB(fd, 2) = 0;
                        WFIFOSET(fd, 3);
                    }
                    //}
                    RFIFOSKIP(fd, 46);
                }
                break;

            case 0x2af8:       // マップサーバーログイン
                if (RFIFOREST(fd) < 60)
                    return;
                WFIFOW(fd, 0) = 0x2af9;
                for (i = 0; i < MAX_MAP_SERVERS; i++)
                {
                    if (server_fd[i] < 0)
                        break;
                }
                if (i == MAX_MAP_SERVERS || strcmp((const char *)RFIFOP(fd, 2), userid)
                    || strcmp((const char *)RFIFOP(fd, 26), passwd))
                {
                    WFIFOB(fd, 2) = 3;
                    WFIFOSET(fd, 3);
                    RFIFOSKIP(fd, 60);
                }
                else
                {
                    int len;
                    WFIFOB(fd, 2) = 0;
                    session[fd]->func_parse = parse_frommap;
                    server_fd[i] = fd;
                    if (anti_freeze_enable)
                        server_freezeflag[i] = 5;   // Map anti-freeze system. Counter. 5 ok, 4...0 freezed
                    server[i].ip = RFIFOL(fd, 54);
                    server[i].port = RFIFOW(fd, 58);
                    server[i].users = 0;
                    memset(server[i].map, 0, sizeof(server[i].map));
                    WFIFOSET(fd, 3);
                    RFIFOSKIP(fd, 60);
                    realloc_fifo(fd, FIFOSIZE_SERVERLINK,
                                  FIFOSIZE_SERVERLINK);
                    // send gm acccounts level to map-servers
                    len = 4;
                    WFIFOW(fd, 0) = 0x2b15;
                    for (i = 0; i < GM_num; i++)
                    {
                        WFIFOL(fd, len) = gm_account[i].account_id;
                        WFIFOB(fd, len + 4) =
                            (unsigned char) gm_account[i].level;
                        len += 5;
                    }
                    WFIFOW(fd, 2) = len;
                    WFIFOSET(fd, len);
                    return;
                }
                break;

            case 0x187:        // Alive信号？
                if (RFIFOREST(fd) < 6)
                    return;
                RFIFOSKIP(fd, 6);
                break;

            case 0x7530:       // Athena情報所得
                WFIFOW(fd, 0) = 0x7531;
                WFIFOB(fd, 2) = ATHENA_MAJOR_VERSION;
                WFIFOB(fd, 3) = ATHENA_MINOR_VERSION;
                WFIFOB(fd, 4) = ATHENA_REVISION;
                WFIFOB(fd, 5) = ATHENA_RELEASE_FLAG;
                WFIFOB(fd, 6) = ATHENA_OFFICIAL_FLAG;
                WFIFOB(fd, 7) = ATHENA_SERVER_INTER | ATHENA_SERVER_CHAR;
                WFIFOW(fd, 8) = ATHENA_MOD_VERSION;
                WFIFOSET(fd, 10);
                RFIFOSKIP(fd, 2);
                return;

            case 0x7532:       // 接続の切断(defaultと処理は一緒だが明示的にするため)
                session[fd]->eof = 1;
                return;

            default:
                session[fd]->eof = 1;
                return;
        }
    }
}

// 全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendall(const uint8_t *buf, unsigned int len)
{
    int i, c;

    c = 0;
    for (i = 0; i < MAX_MAP_SERVERS; i++)
    {
        int fd;
        if ((fd = server_fd[i]) >= 0)
        {
            memcpy(WFIFOP(fd, 0), buf, len);
            WFIFOSET(fd, len);
            c++;
        }
    }
    return c;
}

// 自分以外の全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendallwos(int sfd, const uint8_t *buf, unsigned int len)
{
    int i, c;

    c = 0;
    for (i = 0; i < MAX_MAP_SERVERS; i++)
    {
        int fd;
        if ((fd = server_fd[i]) >= 0 && fd != sfd)
        {
            memcpy(WFIFOP(fd, 0), buf, len);
            WFIFOSET(fd, len);
            c++;
        }
    }
    return c;
}

// MAPサーバーにデータ送信（map鯖生存確認有り）
int mapif_send(int fd, const uint8_t *buf, unsigned int len)
{
    int i;

    if (fd >= 0)
    {
        for (i = 0; i < MAX_MAP_SERVERS; i++)
        {
            if (fd == server_fd[i])
            {
                memcpy(WFIFOP(fd, 0), buf, len);
                WFIFOSET(fd, len);
                return 1;
            }
        }
    }
    return 0;
}

static
void send_users_tologin(TimerData *, tick_t)
{
    int users = count_users();
    uint8_t buf[16];

    if (login_fd > 0 && session[login_fd])
    {
        // send number of user to login server
        WFIFOW(login_fd, 0) = 0x2714;
        WFIFOL(login_fd, 2) = users;
        WFIFOSET(login_fd, 6);
    }
    // send number of players to all map-servers
    WBUFW(buf, 0) = 0x2b00;
    WBUFL(buf, 2) = users;
    mapif_sendall(buf, 6);
}

static
void check_connect_login_server(TimerData *, tick_t)
{
    if (login_fd <= 0 || session[login_fd] == NULL)
    {
        PRINTF("Attempt to connect to login-server...\n");
        if ((login_fd = make_connection(login_ip, login_port)) < 0)
            return;
        session[login_fd]->func_parse = parse_tologin;
        realloc_fifo(login_fd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);
        WFIFOW(login_fd, 0) = 0x2710;
        memset(WFIFOP(login_fd, 2), 0, 24);
        memcpy(WFIFOP(login_fd, 2), userid,
                strlen(userid) < 24 ? strlen(userid) : 24);
        memset(WFIFOP(login_fd, 26), 0, 24);
        memcpy(WFIFOP(login_fd, 26), passwd,
                strlen(passwd) < 24 ? strlen(passwd) : 24);
        WFIFOL(login_fd, 50) = 0;
        WFIFOL(login_fd, 54) = char_ip;
        WFIFOL(login_fd, 58) = char_port;
        memset(WFIFOP(login_fd, 60), 0, 20);
        memcpy(WFIFOP(login_fd, 60), server_name,
                strlen(server_name) < 20 ? strlen(server_name) : 20);
        WFIFOW(login_fd, 80) = 0;
        WFIFOW(login_fd, 82) = char_maintenance;
        WFIFOW(login_fd, 84) = char_new;
        WFIFOSET(login_fd, 86);
    }
}

//-------------------------------------------
// Reading Lan Support configuration by [Yor]
//-------------------------------------------
static
int lan_config_read(const char *lancfgName)
{
    struct hostent *h = NULL;

    // set default configuration
    strzcpy(lan_map_ip, "127.0.0.1", sizeof(lan_map_ip));
    subneti[0] = 127;
    subneti[1] = 0;
    subneti[2] = 0;
    subneti[3] = 1;
    for (int j = 0; j < 4; j++)
        subnetmaski[j] = 255;

    std::ifstream in(lancfgName);

    if (!in.is_open())
    {
        PRINTF("LAN support configuration file not found: %s\n", lancfgName);
        return 1;
    }

    PRINTF("---start reading of Lan Support configuration...\n");

    std::string line;
    while (std::getline(in, line))
    {
        std::string w1, w2;
        if (!split_key_value(line, &w1, &w2))
            continue;

        if (w1 == "lan_map_ip")
        {
            // Read map-server Lan IP Address
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                sprintf(lan_map_ip, "%d.%d.%d.%d",
                         (unsigned char) h->h_addr[0],
                         (unsigned char) h->h_addr[1],
                         (unsigned char) h->h_addr[2],
                         (unsigned char) h->h_addr[3]);
            }
            else
            {
                strzcpy(lan_map_ip, w2.c_str(), sizeof(lan_map_ip));
            }
            PRINTF("LAN IP of map-server: %s.\n", lan_map_ip);
        }
        else if (w1 == "subnet")
        {
            // Read Subnetwork
            for (int j = 0; j < 4; j++)
                subneti[j] = 0;
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                for (int j = 0; j < 4; j++)
                    subneti[j] = (unsigned char) h->h_addr[j];
            }
            else
            {
                SSCANF(w2, "%d.%d.%d.%d", &subneti[0], &subneti[1],
                        &subneti[2], &subneti[3]);
            }
            PRINTF("Sub-network of the map-server: %d.%d.%d.%d.\n",
                    subneti[0], subneti[1], subneti[2], subneti[3]);
        }
        else if (w1 == "subnetmask")
        {
            // Read Subnetwork Mask
            for (int j = 0; j < 4; j++)
                subnetmaski[j] = 255;
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                for (int j = 0; j < 4; j++)
                    subnetmaski[j] = (unsigned char) h->h_addr[j];
            }
            else
            {
                SSCANF(w2, "%d.%d.%d.%d", &subnetmaski[0], &subnetmaski[1],
                        &subnetmaski[2], &subnetmaski[3]);
            }
            PRINTF("Sub-network mask of the map-server: %d.%d.%d.%d.\n",
                    subnetmaski[0], subnetmaski[1], subnetmaski[2],
                    subnetmaski[3]);
        }
        else
        {
            PRINTF("WARNING: unknown lan config key: %s\n", w1);
        }
    }

    // sub-network check of the map-server
    {
        unsigned char p[4];
        sscanf(lan_map_ip, "%hhu.%hhu.%hhu.%hhu", &p[0], &p[1], &p[2], &p[3]);
        PRINTF("LAN test of LAN IP of the map-server: ");
        if (lan_ip_check(p) == 0)
        {
            PRINTF("\033[1;31m***ERROR: LAN IP of the map-server doesn't belong to the specified Sub-network.\033[0m\n");
        }
    }

    PRINTF("---End reading of Lan Support configuration...\n");

    return 0;
}

static
int char_config_read(const char *cfgName)
{
    struct hostent *h = NULL;

    std::ifstream in(cfgName);

    if (!in.is_open())
    {
        PRINTF("Configuration file not found: %s.\n", cfgName);
        exit(1);
    }

    std::string line;
    while (std::getline(in, line))
    {
        std::string w1, w2;
        if (!split_key_value(line, &w1, &w2))
            continue;

        if (w1 == "userid")
            strzcpy(userid, w2.c_str(), 24);
        else if (w1 == "passwd")
            strzcpy(passwd, w2.c_str(), 24);
        else if (w1 == "server_name")
        {
            strzcpy(server_name, w2.c_str(), sizeof(server_name));
            PRINTF("%s server has been intialized\n", w2);
        }
        else if (w1 == "wisp_server_name")
        {
            if (w2.size() >= 4)
                strzcpy(wisp_server_name, w2.c_str(), sizeof(wisp_server_name));
        }
        else if (w1 == "login_ip")
        {
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                PRINTF("Login server IP address : %s -> %d.%d.%d.%d\n", w2,
                        (unsigned char) h->h_addr[0],
                        (unsigned char) h->h_addr[1],
                        (unsigned char) h->h_addr[2],
                        (unsigned char) h->h_addr[3]);
                sprintf(login_ip_str, "%d.%d.%d.%d",
                         (unsigned char) h->h_addr[0],
                         (unsigned char) h->h_addr[1],
                         (unsigned char) h->h_addr[2],
                         (unsigned char) h->h_addr[3]);
            }
            else
                strzcpy(login_ip_str, w2.c_str(), 16);
        }
        else if (w1 == "login_port")
        {
            login_port = atoi(w2.c_str());
        }
        else if (w1 == "char_ip")
        {
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                PRINTF("Character server IP address : %s -> %d.%d.%d.%d\n",
                        w2, (unsigned char) h->h_addr[0],
                        (unsigned char) h->h_addr[1],
                        (unsigned char) h->h_addr[2],
                        (unsigned char) h->h_addr[3]);
                sprintf(char_ip_str, "%d.%d.%d.%d",
                         (unsigned char) h->h_addr[0],
                         (unsigned char) h->h_addr[1],
                         (unsigned char) h->h_addr[2],
                         (unsigned char) h->h_addr[3]);
            }
            else
                strzcpy(char_ip_str, w2.c_str(), 16);
        }
        else if (w1 == "char_port")
        {
            char_port = atoi(w2.c_str());
        }
        else if (w1 == "char_maintenance")
        {
            char_maintenance = atoi(w2.c_str());
        }
        else if (w1 == "char_new")
        {
            char_new = atoi(w2.c_str());
        }
        else if (w1 == "email_creation")
        {
            email_creation = config_switch(w2.c_str());
        }
        else if (w1 == "char_txt")
        {
            strzcpy(char_txt, w2.c_str(), sizeof(char_txt));
        }
        else if (w1 == "max_connect_user")
        {
            max_connect_user = atoi(w2.c_str());
            if (max_connect_user < 0)
                max_connect_user = 0;   // unlimited online players
        }
        else if (w1 == "check_ip_flag")
        {
            check_ip_flag = config_switch(w2.c_str());
        }
        else if (w1 == "autosave_time")
        {
            autosave_interval = std::chrono::seconds(atoi(w2.c_str()));
            if (autosave_interval <= std::chrono::seconds::zero())
                autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
        }
        else if (w1 == "start_point")
        {
            char map[32];
            int x, y;
            if (SSCANF(w2, "%[^,],%d,%d", map, &x, &y) < 3)
                continue;
            if (strstr(map, ".gat") != NULL)
            {                   // Verify at least if '.gat' is in the map name
                memcpy(start_point.map, map, 16);
                start_point.x = x;
                start_point.y = y;
            }
        }
        else if (w1 == "start_zeny")
        {
            start_zeny = atoi(w2.c_str());
            if (start_zeny < 0)
                start_zeny = 0;
        }
        else if (w1 == "start_weapon")
        {
            start_weapon = atoi(w2.c_str());
            if (start_weapon < 0)
                start_weapon = 0;
        }
        else if (w1 == "start_armor")
        {
            start_armor = atoi(w2.c_str());
            if (start_armor < 0)
                start_armor = 0;
        }
        else if (w1 == "unknown_char_name")
        {
            strzcpy(unknown_char_name, w2.c_str(), 24);
        }
        else if (w1 == "char_log_filename")
        {
            strzcpy(char_log_filename, w2.c_str(), sizeof(char_log_filename));
        }
        else if (w1 == "name_ignoring_case")
        {
            name_ignoring_case = config_switch(w2.c_str());
        }
        else if (w1 == "char_name_option")
        {
            char_name_option = atoi(w2.c_str());
        }
        else if (w1 == "char_name_letters")
        {
            strzcpy(char_name_letters, w2.c_str(), sizeof(char_name_letters));
        }
        else if (w1 == "online_txt_filename")
        {
            strzcpy(online_txt_filename, w2.c_str(), sizeof(online_txt_filename));
        }
        else if (w1 == "online_html_filename")
        {
            strzcpy(online_html_filename, w2.c_str(), sizeof(online_html_filename));
        }
        else if (w1 == "online_sorting_option")
        {
            online_sorting_option = atoi(w2.c_str());
        }
        else if (w1 == "online_gm_display_min_level")
        {                       // minimum GM level to display 'GM' when we want to display it
            online_gm_display_min_level = atoi(w2.c_str());
            if (online_gm_display_min_level < 5)    // send online file every 5 seconds to player is enough
                online_gm_display_min_level = 5;
        }
        else if (w1 == "online_refresh_html")
        {
            online_refresh_html = atoi(w2.c_str());
            if (online_refresh_html < 1)
                online_refresh_html = 1;
        }
        else if (w1 == "anti_freeze_enable")
        {
            anti_freeze_enable = config_switch(w2.c_str());
        }
        else if (w1 == "anti_freeze_interval")
        {
            ANTI_FREEZE_INTERVAL = std::max(
                    std::chrono::seconds(atoi(w2.c_str())),
                    std::chrono::seconds(5));
        }
        else if (w1 == "import")
        {
            char_config_read(w2.c_str());
        }
        else
        {
            PRINTF("WARNING: unknown char config key: %s\n", w1);
        }
    }

    return 0;
}

void term_func(void)
{
    int i;

    // write online players files with no player
    for (i = 0; i < char_num; i++)
        online_chars[i] = -1;
    create_online_files();
    free(online_chars);

    mmo_char_sync();
    inter_save();

    if (gm_account != NULL)
        free(gm_account);

    free(char_dat);
    delete_session(login_fd);
    delete_session(char_fd);

    CHAR_LOG("----End of char-server (normal end with closing of all files).\n");
}

int do_init(int argc, char **argv)
{
    int i;

    // a newline in the log...
    CHAR_LOG("");
    CHAR_LOG("The char-server starting...\n");

    char_config_read((argc < 2) ? CHAR_CONF_NAME : argv[1]);
    lan_config_read((argc > 1) ? argv[1] : LOGIN_LAN_CONF_NAME);

    login_ip = inet_addr(login_ip_str);
    char_ip = inet_addr(char_ip_str);

    for (i = 0; i < MAX_MAP_SERVERS; i++)
    {
        memset(&server[i], 0, sizeof(struct mmo_map_server));
        server_fd[i] = -1;
    }

    mmo_char_init();

    update_online = TimeT::now();
    create_online_files();     // update online players files at start of the server

    inter_init((argc > 2) ? argv[2] : inter_cfgName);  // inter server 初期化

//    set_termfunc (do_final);
    set_defaultparse(parse_char);

    char_fd = make_listen_port(char_port);

    add_timer_interval(gettick() + std::chrono::seconds(1),
            check_connect_login_server, std::chrono::seconds(10));
    add_timer_interval(gettick() + std::chrono::seconds(1),
            send_users_tologin, std::chrono::seconds(5));
    add_timer_interval(gettick() + autosave_interval,
            mmo_char_sync_timer, autosave_interval);

    if (anti_freeze_enable > 0)
    {
        add_timer_interval(gettick() + std::chrono::seconds(1),
                map_anti_freeze_system, ANTI_FREEZE_INTERVAL);
    }

    CHAR_LOG("The char-server is ready (Server is listening on the port %d).\n",
              char_port);

    PRINTF("The char-server is \033[1;32mready\033[0m (Server is listening on the port %d).\n\n",
         char_port);

    return 0;
}
