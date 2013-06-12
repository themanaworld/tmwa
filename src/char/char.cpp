#include "char.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <netdb.h>
#include <unistd.h>

#include <cassert>
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
char char_txt[1024];
static
char unknown_char_name[24] = "Unknown";
static
char char_log_filename[1024] = "log/char.log";
//Added for lan support
static
char lan_map_ip[128];
static
uint8_t subneti[4];
static
uint8_t subnetmaski[4];
static
int name_ignoring_case = 0;    // Allow or not identical name for characters but with a different case by [Yor]
static
int char_name_option = 0;      // Option to know which letters/symbols are authorised in the name of a character (0: all, 1: only those in char_name_letters, 2: all EXCEPT those in char_name_letters) by [Yor]
static
char char_name_letters[1024] = "";  // list of letters/symbols authorised (or not) in a character name. by [Yor]

struct char_session_data : SessionData
{
    int account_id, login_id1, login_id2, sex;
    unsigned short packet_tmw_version;
    char email[40];             // e-mail (default: a@a.com) by [Yor]
    TimeT connect_until_time;  // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
};

void SessionDeleter::operator()(SessionData *sd)
{
    really_delete1 static_cast<char_session_data *>(sd);
}

struct AuthFifoEntry
{
    int account_id;
    int char_id;
    int login_id1, login_id2;
    int ip;
    int delflag;
    int sex;
    unsigned short packet_tmw_version;
    TimeT connect_until_time;  // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
};
static
std::array<AuthFifoEntry, 256> auth_fifo;
static
auto auth_fifo_iter = auth_fifo.begin();

static
int check_ip_flag = 1;         // It's to check IP of a player between char-server and other servers (part of anti-hacking system)

static
int char_id_count = 150000;
static
std::vector<mmo_charstatus> char_data;
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
std::vector<GM_Account> gm_accounts;

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
std::vector<int> online_chars;              // same size of char_data, and id value of current server (or -1)
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
    for (GM_Account& gma : gm_accounts)
        if (gma.account_id == account_id)
            return gma.level;
    return 0;
}

//----------------------------------------------
// Search an character id
//   (return character pointer or nullptr (if not found))
//   If exact character name is not found,
//   the function checks without case sensitive
//   and returns index if only 1 character is found
//   and similar to the searched name.
//----------------------------------------------
const mmo_charstatus *search_character(const char *character_name)
{
    int quantity = 0;
    const mmo_charstatus *index = nullptr;
    for (const mmo_charstatus& cd : char_data)
    {
        // Without case sensitive check (increase the number of similar character names found)
        if (strcasecmp(cd.name, character_name) == 0)
        {
            // Strict comparison (if found, we finish the function immediatly with correct value)
            if (strcmp(cd.name, character_name) == 0)
                return &cd;
            quantity++;
            index = &cd;
        }
    }
    // Here, the exact character name is not found
    // We return the found index of a similar account ONLY if there is 1 similar character
    if (quantity == 1)
        return index;

    // Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
    return nullptr;
}

//-------------------------------------------------
// Function to create the character line (for save)
//-------------------------------------------------
__inline__ static
std::string mmo_char_tostr(struct mmo_charstatus *p)
{
    // on multi-map server, sometimes it's posssible that last_point become void. (reason???) We check that to not lost character at restart.
    if (p->last_point.map[0] == '\0')
    {
        memcpy(p->last_point.map, "001-1.gat", 10);
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

    for (const mmo_charstatus& cd : char_data)
    {
        if (cd.char_id == p->char_id)
            return false;
        if (strcmp(cd.name, p->name) == 0)
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
    char_data.clear();
    online_chars.clear();

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

        mmo_charstatus cd {};
        if (!extract(line, &cd))
        {
            CHAR_LOG("Char skipped\n%s", line);
            continue;
        }
        if (cd.char_id >= char_id_count)
            char_id_count = cd.char_id + 1;
        char_data.push_back(std::move(cd));
        online_chars.push_back(-1);
    }

    PRINTF("mmo_char_init: %zu characters read in %s.\n",
            char_data.size(), char_txt);
    CHAR_LOG("mmo_char_init: %zu characters read in %s.\n",
            char_data.size(), char_txt);

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
    int lock;
    FILE *fp;

    // Data save
    fp = lock_fopen(char_txt, &lock);
    if (fp == NULL)
    {
        PRINTF("WARNING: Server can't not save characters.\n");
        CHAR_LOG("WARNING: Server can't not save characters.\n");
        return;
    }
    {
        // yes, we need a mutable reference to do the saves ...
        for (mmo_charstatus& cd : char_data)
        {
            std::string line = mmo_char_tostr(&cd);
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
mmo_charstatus *make_new_char(int fd, const uint8_t *dat)
{
    // ugh
    char *cdat = reinterpret_cast<char *>(const_cast<uint8_t *>(dat));
    char_session_data *sd = static_cast<char_session_data *>(session[fd]->session_data.get());

    // remove control characters from the name
    cdat[23] = '\0';
    if (remove_control_chars(cdat))
    {
        CHAR_LOG("Make new char error (control char received in the name): (connection #%d, account: %d).\n",
             fd, sd->account_id);
        return nullptr;
    }

    // Eliminate whitespace
    remove_trailing_blanks(cdat);
    remove_prefix_blanks(cdat);

    // check lenght of character name
    if (strlen(cdat) < 4)
    {
        CHAR_LOG("Make new char error (character name too small): (connection #%d, account: %d, name: '%s').\n",
             fd, sd->account_id, cdat);
        return nullptr;
    }

    // Check Authorised letters/symbols in the name of the character
    if (char_name_option == 1)
    {
        // only letters/symbols in char_name_letters are authorised
        for (int i = 0; cdat[i]; i++)
            if (strchr(char_name_letters, cdat[i]) == NULL)
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n",
                     fd, sd->account_id, cdat, cdat[i]);
                return nullptr;
            }
    }
    else if (char_name_option == 2)
    {
        // letters/symbols in char_name_letters are forbidden
        for (int i = 0; cdat[i]; i++)
            if (strchr(char_name_letters, cdat[i]) != NULL)
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n",
                     fd, sd->account_id, cdat, cdat[i]);
                return nullptr;
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
        return nullptr;
    }

    // check individual stat value
    for (int i = 24; i <= 29; i++)
    {
        if (dat[i] < 1 || dat[i] > 9)
        {
            CHAR_LOG("Make new char error (invalid stat value: not between 1 to 9): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d\n",
                 fd, sd->account_id, dat[30], dat, dat[24], dat[25],
                 dat[26], dat[27], dat[28], dat[29],
                 dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
                 dat[33], dat[31]);
            return nullptr;
        }
    }

    for (const mmo_charstatus& cd : char_data)
    {
        if ((name_ignoring_case != 0 && strcmp(cd.name, cdat) == 0)
            || (name_ignoring_case == 0
                && strcasecmp(cd.name, cdat) == 0))
        {
            CHAR_LOG("Make new char error (name already exists): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n",
                 fd, sd->account_id, dat[30], cdat, cd.name,
                 dat[24], dat[25], dat[26], dat[27], dat[28], dat[29],
                 dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
                 dat[33], dat[31]);
            return nullptr;
        }
        if (cd.account_id == sd->account_id
            && cd.char_num == dat[30])
        {
            CHAR_LOG("Make new char error (slot already used): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n",
                 fd, sd->account_id, dat[30], cdat, cd.name,
                 dat[24], dat[25], dat[26], dat[27], dat[28], dat[29],
                 dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
                 dat[33], dat[31]);
            return nullptr;
        }
    }

    if (strcmp(wisp_server_name, cdat) == 0)
    {
        CHAR_LOG("Make new char error (name used is wisp name for server): (connection #%d, account: %d) slot %d, name: %s (actual name whisper server: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n",
             fd, sd->account_id, dat[30], cdat, wisp_server_name,
             dat[24], dat[25], dat[26], dat[27], dat[28], dat[29],
             dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29],
             dat[33], dat[31]);
        return nullptr;
    }

    char ip[16];
    uint8_t *sin_addr = reinterpret_cast<uint8_t *>(&session[fd]->client_addr.sin_addr);
    sprintf(ip, "%d.%d.%d.%d", sin_addr[0], sin_addr[1], sin_addr[2],
             sin_addr[3]);

    CHAR_LOG("Creation of New Character: (connection #%d, account: %d) slot %d, character Name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d. [%s]\n",
         fd, sd->account_id, dat[30], cdat, dat[24], dat[25], dat[26],
         dat[27], dat[28], dat[29],
         dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33],
         dat[31], ip);

    mmo_charstatus cd {};

    cd.char_id = char_id_count++;
    cd.account_id = sd->account_id;
    cd.char_num = dat[30];
    strcpy(cd.name, cdat);
    cd.species = 0;
    cd.base_level = 1;
    cd.job_level = 1;
    cd.base_exp = 0;
    cd.job_exp = 0;
    cd.zeny = start_zeny;
    cd.attrs[ATTR::STR] = dat[24];
    cd.attrs[ATTR::AGI] = dat[25];
    cd.attrs[ATTR::VIT] = dat[26];
    cd.attrs[ATTR::INT] = dat[27];
    cd.attrs[ATTR::DEX] = dat[28];
    cd.attrs[ATTR::LUK] = dat[29];
    cd.max_hp = 40 * (100 + cd.attrs[ATTR::VIT]) / 100;
    cd.max_sp = 11 * (100 + cd.attrs[ATTR::INT]) / 100;
    cd.hp = cd.max_hp;
    cd.sp = cd.max_sp;
    cd.status_point = 0;
    cd.skill_point = 0;
    cd.option = static_cast<Option>(0x0000); // Option is only declared
    cd.karma = 0;
    cd.manner = 0;
    cd.party_id = 0;
    //cd.guild_id = 0;
    cd.hair = dat[33];
    cd.hair_color = dat[31];
    cd.clothes_color = 0;
    // TODO: remove this - it's not used
    cd.inventory[0].nameid = start_weapon; // Knife
    cd.inventory[0].amount = 1;
    cd.inventory[0].equip = EPOS::WEAPON;
    cd.inventory[0].identify = 1;
    cd.inventory[0].broken = 0;
    cd.inventory[1].nameid = start_armor;  // Cotton Shirt
    cd.inventory[1].amount = 1;
    cd.inventory[1].equip = EPOS::MISC1;
    cd.inventory[1].identify = 1;
    cd.inventory[1].broken = 0;
    cd.weapon = ItemLook::BLADE;
    cd.shield = 0;
    cd.head_top = 0;
    cd.head_mid = 0;
    cd.head_bottom = 0;
    memcpy(&cd.last_point, &start_point, sizeof(start_point));
    memcpy(&cd.save_point, &start_point, sizeof(start_point));
    char_data.push_back(std::move(cd));
    online_chars.push_back(-1);

    return &char_data.back();
}

//-------------------------------------------------------------
// Function to create the online files (txt and html). by [Yor]
//-------------------------------------------------------------
static
void create_online_files(void)
{
    // write files
    FILE *fp = fopen_(online_txt_filename, "w");
    if (fp != NULL)
    {
        FILE *fp2 = fopen_(online_html_filename, "w");
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

            int players = 0;
            // This used to be conditional on having any players,
            // but this simplifies the logic.
            //if (players > 0)
            {
                int j = 0;          // count the number of characters for the txt version and to set the separate line
                FPRINTF(fp2, "    <table border=\"1\" cellspacing=\"1\">\n");
                FPRINTF(fp2, "      <tr>\n");
                {
                    FPRINTF(fp2, "        <th>Name</th>\n");
                    {
                        FPRINTF(fp, "Name                          "); // 30
                        j += 30;
                    }
                }
                FPRINTF(fp2, "      </tr>\n");
                FPRINTF(fp, "\n");
                for (int k = 0; k < j; k++)
                    FPRINTF(fp, "-");
                FPRINTF(fp, "\n");

                // display each player.
                for (struct mmo_charstatus& cd : char_data)
                {
                    if (online_chars[&cd - &char_data.front()] == -1)
                        continue;
                    FPRINTF(fp2, "      <tr>\n");
                    // displaying the character name
                    {
                        // without/with 'GM' display
                        int gml = isGM(cd.account_id);
                        {
                            if (gml >= online_gm_display_min_level)
                                FPRINTF(fp, "%-24s (GM) ", cd.name);
                            else
                                FPRINTF(fp, "%-24s      ", cd.name);
                        }
                        // name of the character in the html (no < >, because that create problem in html code)
                        FPRINTF(fp2, "        <td>");
                        if (gml >= online_gm_display_min_level)
                            FPRINTF(fp2, "<b>");
                        for (int k = 0; cd.name[k]; k++)
                        {
                            switch (cd.name[k])
                            {
                            case '&':
                                FPRINTF(fp2, "&amp;");
                                break;
                            case '<':
                                FPRINTF(fp2, "&lt;");
                                break;
                            case '>':
                                FPRINTF(fp2, "&gt;");
                                break;
                            default:
                                FPRINTF(fp2, "%c", cd.name[k]);
                                break;
                            };
                        }
                        if (gml >= online_gm_display_min_level)
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
            }
            else if (players == 1)
            {
                // no display if only 1 player
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
int find_equip_view(const mmo_charstatus *p, EPOS equipmask)
{
    for (int i = 0; i < MAX_INVENTORY; i++)
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
    int found_num = 0;
    std::array<const mmo_charstatus *, 9> found_char;
    for (const mmo_charstatus& cd : char_data)
    {
        if (cd.account_id == sd->account_id)
        {
            found_char[found_num] = &cd;
            found_num++;
            if (found_num == 9)
                break;
        }
    }

    const int offset = 24;
    memset(WFIFOP(fd, 0), 0, offset + found_num * 106);
    WFIFOW(fd, 0) = 0x6b;
    WFIFOW(fd, 2) = offset + found_num * 106;

    for (int i = 0; i < found_num; i++)
    {
        const mmo_charstatus *p = found_char[i];
        int j = offset + (i * 106);

        WFIFOL(fd, j) = p->char_id;
        WFIFOL(fd, j + 4) = p->base_exp;
        WFIFOL(fd, j + 8) = p->zeny;
        WFIFOL(fd, j + 12) = p->job_exp;
        WFIFOL(fd, j + 16) = p->job_level;

        WFIFOW(fd, j + 20) = find_equip_view(p, EPOS::SHOES);
        WFIFOW(fd, j + 22) = find_equip_view(p, EPOS::GLOVES);
        WFIFOW(fd, j + 24) = find_equip_view(p, EPOS::CAPE);
        WFIFOW(fd, j + 26) = find_equip_view(p, EPOS::MISC1);
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
    int c = 0;
    for (mmo_charstatus& cd : char_data)
    {
        if (cd.account_id == acc)
        {
            memcpy(cd.account_reg2, reg, sizeof(cd.account_reg2));
            cd.account_reg2_num = num;
            c++;
        }
    }
    return c;
}

// Divorce a character from it's partner and let the map server know
static
int char_divorce(struct mmo_charstatus *cs)
{
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

    for (mmo_charstatus& cd : char_data)
    {
        if (cd.char_id == cs->partner_id
            && cd.partner_id == cs->char_id)
        {
            WBUFL(buf, 6) = cs->partner_id;
            mapif_sendall(buf, 10);
            cs->partner_id = 0;
            cd.partner_id = 0;
            return 0;
        }
        // The other char doesn't have us as their partner, so just clear our partner
        // Don't worry about this, as the map server should verify itself that the other doesn't have us as a partner, and so won't mess with their marriage
        else if (cd.char_id == cs->partner_id)
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
    // disconnect player if online on char-server
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        struct char_session_data *sd = static_cast<char_session_data *>(session[i]->session_data.get());
        if (sd)
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

    char_session_data *sd = static_cast<char_session_data *>(session[fd]->session_data.get());

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
                    if (!session[i])
                        continue;
                    sd = static_cast<char_session_data *>(session[i]->session_data.get());
                    if (sd && sd->account_id == RFIFOL(fd, 2))
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
                            WFIFOB(i, 2) = 0;
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
                    if (!session[i])
                        continue;
                    sd = static_cast<char_session_data *>(session[i]->session_data.get());
                    if (sd)
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
                    unsigned char buf[7];
                    int acc = RFIFOL(fd, 2);
                    int sex = RFIFOB(fd, 6);
                    RFIFOSKIP(fd, 7);
                    if (acc > 0)
                    {
                        for (struct mmo_charstatus& cd : char_data)
                        {
                            if (cd.account_id == acc)
                            {
                                cd.sex = sex;
//                      auth_fifo[i].sex = sex;
                                // to avoid any problem with equipment and invalid sex, equipment is unequiped.
                                for (int j = 0; j < MAX_INVENTORY; j++)
                                {
                                    if (cd.inventory[j].nameid
                                        && bool(cd.inventory[j].equip))
                                        cd.inventory[j].equip = EPOS::ZERO;
                                }
                                cd.weapon = ItemLook::NONE;
                                cd.shield = 0;
                                cd.head_top = 0;
                                cd.head_mid = 0;
                                cd.head_bottom = 0;
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
                for (struct mmo_charstatus& cd : char_data)
                {
                    struct mmo_charstatus *c = &cd;
                    struct storage *s = account2storage(c->account_id);
                    int changes = 0;
                    int j;
#define FIX(v) if (v == source_id) {v = dest_id; ++changes; }
                    for (j = 0; j < MAX_INVENTORY; j++)
                        FIX(c->inventory[j].nameid);
                    for (j = 0; j < MAX_CART; j++)
                        FIX(c->cart[j].nameid);
                    // FIX(c->weapon);
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
#warning "This comment is a lie, but it's still true."
                // needs to use index because they may move during resize
                for (int idx = 0; idx < char_data.size(); idx++)
                {
                    mmo_charstatus& cd = char_data[idx];
                    if (cd.account_id == RFIFOL(fd, 2))
                    {
                        char_delete(&cd);
                        if (&cd != &char_data.back())
                        {
                            std::swap(cd, char_data.back());
                            // if moved character owns to deleted account, check again it's character
                            if (cd.account_id == RFIFOL(fd, 2))
                            {
                                idx--;
                                // Correct moved character reference in the character's owner by [Yor]
                            }
                        }
                        char_data.pop_back();
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
                    gm_accounts.clear();
                    gm_accounts.resize((RFIFOW(fd, 2) - 4) / 5);
                    for (int i = 4; i < RFIFOW(fd, 2); i = i + 5)
                    {
                        gm_accounts.push_back({static_cast<int>(RFIFOL(fd, i)), RFIFOB(fd, i + 4)});
                    }
                    PRINTF("From login-server: receiving of %zu GM accounts information.\n",
                         gm_accounts.size());
                    CHAR_LOG("From login-server: receiving of %zu GM accounts information.\n",
                         gm_accounts.size());
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
                        if (!session[i])
                            continue;
                        sd = static_cast<char_session_data *>(session[i]->session_data.get());
                        if (sd)
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
            for (int& oci : online_chars)
                if (oci == fd)
                    oci = -1;
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
            {
                memset(server[id].map, 0, sizeof(server[id].map));
                int j = 0;
                for (int i = 4; i < RFIFOW(fd, 2); i += 16)
                {
                    memcpy(server[id].map[j], RFIFOP(fd, i), 16);
//              PRINTF("set map %d.%d : %s\n", id, j, server[id].map[j]);
                    j++;
                }
                {
                    uint8_t *p = reinterpret_cast<uint8_t *>(&server[id].ip);
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
                    for (int x = 0; x < MAX_MAP_SERVERS; x++)
                    {
                        if (server_fd[x] >= 0 && x != id)
                        {
                            WFIFOW(fd, 0) = 0x2b04;
                            WFIFOL(fd, 4) = server[x].ip;
                            WFIFOW(fd, 8) = server[x].port;
                            j = 0;
                            for (int i = 0; i < MAX_MAP_PER_SERVER; i++)
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
            }
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

                // 認証要求
            case 0x2afc:
                if (RFIFOREST(fd) < 22)
                    return;
                //PRINTF("auth_fifo search: account: %d, char: %d, secure: %08x-%08x\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14));
                for (AuthFifoEntry& afi : auth_fifo)
                {
                    if (afi.account_id == RFIFOL(fd, 2) &&
                        afi.char_id == RFIFOL(fd, 6) &&
                        afi.login_id1 == RFIFOL(fd, 10) &&
                        // here, it's the only area where it's possible that we doesn't know login_id2 (map-server asks just after 0x72 packet, that doesn't given the value)
                        (afi.login_id2 == RFIFOL(fd, 14) || RFIFOL(fd, 14) == 0) &&  // relate to the versions higher than 18
                        (!check_ip_flag || afi.ip == RFIFOL(fd, 18))
                        && !afi.delflag)
                    {
                        mmo_charstatus *cd = nullptr;
                        for (mmo_charstatus& cdi : char_data)
                        {
                            if (cdi.char_id == afi.char_id)
                            {
                                cd = &cdi;
                                break;
                            }
                        }
                        assert (cd && "uh-oh - deleted while in queue?");
                        afi.delflag = 1;
                        WFIFOW(fd, 0) = 0x2afd;
                        WFIFOW(fd, 2) = 18 + sizeof(struct mmo_charstatus);
                        WFIFOL(fd, 4) = RFIFOL(fd, 2);
                        WFIFOL(fd, 8) = afi.login_id2;
                        WFIFOL(fd, 12) = static_cast<time_t>(afi.connect_until_time);
                        cd->sex = afi.sex;
                        WFIFOW(fd, 16) = afi.packet_tmw_version;
                        FPRINTF(stderr,
                                 "From queue index %zd: recalling packet version %d\n",
                                 (&afi - &auth_fifo.front()), afi.packet_tmw_version);
                        memcpy(WFIFOP(fd, 18),
                                cd,
                                sizeof(struct mmo_charstatus));
                        WFIFOSET(fd, WFIFOW(fd, 2));
                        //PRINTF("auth_fifo search success (auth #%d, account %d, character: %d).\n", i, RFIFOL(fd,2), RFIFOL(fd,6));
                        goto x2afc_out;
                    }
                }
                {
                    WFIFOW(fd, 0) = 0x2afe;
                    WFIFOL(fd, 2) = RFIFOL(fd, 2);
                    WFIFOSET(fd, 6);
                    PRINTF("auth_fifo search error! account %d not authentified.\n",
                         RFIFOL(fd, 2));
                }
            x2afc_out:
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
                for (int& oci : online_chars)
                    if (oci == id)
                        oci = -1;
                // add online players in the list by [Yor]
                for (int i = 0; i < server[id].users; i++)
                {
                    int char_id = RFIFOL(fd, 6 + i * 4);
                    for (const mmo_charstatus& cd : char_data)
                        if (cd.char_id == char_id)
                        {
                            online_chars[&cd - &char_data.front()] = id;
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
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

                // キャラデータ保存
            case 0x2b01:
                if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd, 2))
                    return;
                for (mmo_charstatus& cd : char_data)
                {
                    if (cd.account_id == RFIFOL(fd, 4) &&
                        cd.char_id == RFIFOL(fd, 8))
                    {
                        memcpy(&cd, RFIFOP(fd, 12), sizeof(struct mmo_charstatus));
                        break;
                    }
                }
                RFIFOSKIP(fd, RFIFOW(fd, 2));
                break;

                // キャラセレ要求
            case 0x2b02:
                if (RFIFOREST(fd) < 18)
                    return;
                if (auth_fifo_iter == auth_fifo.end())
                    auth_fifo_iter = auth_fifo.begin();
                auth_fifo_iter->account_id = RFIFOL(fd, 2);
                auth_fifo_iter->char_id = 0;
                auth_fifo_iter->login_id1 = RFIFOL(fd, 6);
                auth_fifo_iter->login_id2 = RFIFOL(fd, 10);
                auth_fifo_iter->delflag = 2;
                auth_fifo_iter->connect_until_time = TimeT();    // unlimited/unknown time by default (not display in map-server)
                auth_fifo_iter->ip = RFIFOL(fd, 14);
                auth_fifo_iter++;
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
                if (auth_fifo_iter == auth_fifo.end())
                    auth_fifo_iter = auth_fifo.begin();
                WFIFOW(fd, 0) = 0x2b06;
                memcpy(WFIFOP(fd, 2), RFIFOP(fd, 2), 42);
                auth_fifo_iter->account_id = RFIFOL(fd, 2);
                auth_fifo_iter->char_id = RFIFOL(fd, 14);
                auth_fifo_iter->login_id1 = RFIFOL(fd, 6);
                auth_fifo_iter->login_id2 = RFIFOL(fd, 10);
                auth_fifo_iter->delflag = 0;
                auth_fifo_iter->sex = RFIFOB(fd, 44);
                auth_fifo_iter->connect_until_time = TimeT();    // unlimited/unknown time by default (not display in map-server)
                auth_fifo_iter->ip = RFIFOL(fd, 45);

                // default, if not found in the loop
                WFIFOW(fd, 6) = 1;
                for (const mmo_charstatus& cd : char_data)
                    if (cd.account_id == RFIFOL(fd, 2) &&
                        cd.char_id == RFIFOL(fd, 14))
                    {
                        auth_fifo_iter++;
                        WFIFOL(fd, 6) = 0;
                        break;
                    }
                WFIFOSET(fd, 44);
                RFIFOSKIP(fd, 49);
                break;

                // キャラ名検索
            case 0x2b08:
                if (RFIFOREST(fd) < 6)
                    return;
            {
                const char (*name)[24] = &unknown_char_name;
                for (const mmo_charstatus& cd : char_data)
                {
                    if (cd.char_id == RFIFOL(fd, 2))
                    {
                        name = &cd.name;
                        break;
                    }
                }
                WFIFOW(fd, 0) = 0x2b09;
                WFIFOL(fd, 2) = RFIFOL(fd, 2);
                memcpy(WFIFOP(fd, 6), *name, 24);
                WFIFOSET(fd, 30);
            }
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
                    strzcpy(character_name, static_cast<const char *>(RFIFOP(fd, 6)), 24);
                    // prepare answer
                    WFIFOW(fd, 0) = 0x2b0f;    // answer
                    WFIFOL(fd, 2) = acc;   // who want do operation
                    WFIFOW(fd, 30) = RFIFOW(fd, 30);  // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex
                    // search character
                    const mmo_charstatus *cd = search_character(character_name);
                    if (cd)
                    {
                        memcpy(WFIFOP(fd, 6), cd->name, 24); // put correct name if found
                        WFIFOW(fd, 32) = 0;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                        switch (RFIFOW(fd, 30))
                        {
                            case 1:    // block
                                if (acc == -1
                                    || isGM(acc) >= isGM(cd->account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2724;
                                        WFIFOL(login_fd, 2) = cd->account_id;  // account value
                                        WFIFOL(login_fd, 6) = 5;   // status of the account
                                        WFIFOSET(login_fd, 10);
//                          PRINTF("char : status -> login: account %d, status: %d \n", char_data[i].account_id, 5);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 2:    // ban
                                if (acc == -1
                                    || isGM(acc) >= isGM(cd->account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2725;
                                        WFIFOL(login_fd, 2) = cd->account_id;  // account value
                                        WFIFOW(login_fd, 6) = RFIFOW(fd, 32); // year
                                        WFIFOW(login_fd, 8) = RFIFOW(fd, 34); // month
                                        WFIFOW(login_fd, 10) = RFIFOW(fd, 36);    // day
                                        WFIFOW(login_fd, 12) = RFIFOW(fd, 38);    // hour
                                        WFIFOW(login_fd, 14) = RFIFOW(fd, 40);    // minute
                                        WFIFOW(login_fd, 16) = RFIFOW(fd, 42);    // second
                                        WFIFOSET(login_fd, 18);
//                          PRINTF("char : status -> login: account %d, ban: %dy %dm %dd %dh %dmn %ds\n",
//                                 char_data[i].account_id, (short)RFIFOW(fd,32), (short)RFIFOW(fd,34), (short)RFIFOW(fd,36), (short)RFIFOW(fd,38), (short)RFIFOW(fd,40), (short)RFIFOW(fd,42));
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 3:    // unblock
                                if (acc == -1
                                    || isGM(acc) >= isGM(cd->account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2724;
                                        WFIFOL(login_fd, 2) = cd->account_id;  // account value
                                        WFIFOL(login_fd, 6) = 0;   // status of the account
                                        WFIFOSET(login_fd, 10);
//                          PRINTF("char : status -> login: account %d, status: %d \n", char_data[i].account_id, 0);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 4:    // unban
                                if (acc == -1
                                    || isGM(acc) >= isGM(cd->account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x272a;
                                        WFIFOL(login_fd, 2) = cd->account_id;  // account value
                                        WFIFOSET(login_fd, 6);
//                          PRINTF("char : status -> login: account %d, unban request\n", char_data[i].account_id);
                                    }
                                    else
                                        WFIFOW(fd, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(fd, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 5:    // changesex
                                if (acc == -1
                                    || isGM(acc) >= isGM(cd->account_id))
                                {
                                    if (login_fd > 0)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_fd, 0) = 0x2727;
                                        WFIFOL(login_fd, 2) = cd->account_id;  // account value
                                        WFIFOSET(login_fd, 6);
//                          PRINTF("char : status -> login: account %d, change sex request\n", char_data[i].account_id);
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
                    int p, j;
                    int acc = RFIFOL(fd, 4);
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
                    for (mmo_charstatus& cd : char_data)
                        if (cd.char_id == RFIFOL(fd, 2))
                        {
                            char_divorce(&cd);
                            break;
                        }

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

    {
        mmo_charstatus *cd = nullptr;
        for (mmo_charstatus& cdi : char_data)
        {
            if (cdi.account_id == sd->account_id && cdi.char_num == rfifob_2)
            {
                cd = &cdi;
                break;
            }
        }
        if (cd)
        {
            CHAR_LOG("Character Selected, Account ID: %d, Character Slot: %d, Character Name: %s [%s]\n",
                 sd->account_id, rfifob_2,
                 cd->name, ip);
            // searching map server
            int i = search_mapserver(cd->last_point.map);
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
                        memcpy(cd->last_point.map,
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
                    WFIFOB(fd, 2) = 1; // 01 = Server closed
                    WFIFOSET(fd, 3);
                    return;
                }
            }
            WFIFOW(fd, 0) = 0x71;
            WFIFOL(fd, 2) = cd->char_id;
            memcpy(WFIFOP(fd, 6),
                    cd->last_point.map,
                    16);
            PRINTF("Character selection '%s' (account: %d, slot: %d) [%s]\n",
                 cd->name,
                 sd->account_id, cd->char_num, ip);
            PRINTF("--Send IP of map-server. ");
            if (lan_ip_check(p))
                WFIFOL(fd, 22) = inet_addr(lan_map_ip);
            else
                WFIFOL(fd, 22) = server[i].ip;
            WFIFOW(fd, 26) = server[i].port;
            WFIFOSET(fd, 28);
            if (auth_fifo_iter == auth_fifo.end())
                auth_fifo_iter = auth_fifo.begin();
            auth_fifo_iter->account_id = sd->account_id;
            auth_fifo_iter->char_id = cd->char_id;
            auth_fifo_iter->login_id1 = sd->login_id1;
            auth_fifo_iter->login_id2 = sd->login_id2;
            auth_fifo_iter->delflag = 0;
            auth_fifo_iter->sex = sd->sex;
            auth_fifo_iter->connect_until_time = sd->connect_until_time;
            auth_fifo_iter->ip = session[fd]->client_addr.sin_addr.s_addr;
            auth_fifo_iter->packet_tmw_version = sd->packet_tmw_version;
            auth_fifo_iter++;
        }
    }
}

static
void parse_char(int fd)
{
    char email[40];
    uint8_t *p = reinterpret_cast<uint8_t *>(&session[fd]->client_addr.sin_addr);

    if (login_fd < 0 || session[fd]->eof)
    {                           // disconnect any player (already connected to char-server or coming back from map-server) if login-server is diconnected.
        if (fd == login_fd)
            login_fd = -1;
        delete_session(fd);
        return;
    }

    char_session_data *sd = static_cast<char_session_data *>(session[fd]->session_data.get());

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
                    int GM_value = isGM(RFIFOL(fd, 2));
                    if (GM_value)
                        PRINTF("Account Logged On; Account ID: %d (GM level %d).\n",
                                RFIFOL(fd, 2), GM_value);
                    else
                        PRINTF("Account Logged On; Account ID: %d.\n",
                                RFIFOL(fd, 2));
                    if (sd == NULL)
                    {
                        session[fd]->session_data = make_unique<char_session_data, SessionDeleter>();
                        sd = static_cast<char_session_data *>(session[fd]->session_data.get());
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
                    for (AuthFifoEntry& afi : auth_fifo)
                    {
                        if (afi.account_id == sd->account_id
                            && afi.login_id1 == sd->login_id1
                            && afi.login_id2 == sd->login_id2
                            && (!check_ip_flag
                                || afi.ip == session[fd]->client_addr.sin_addr.s_addr)
                            && afi.delflag == 2)
                        {
                            afi.delflag = 1;
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
                                afi.packet_tmw_version =
                                    sd->packet_tmw_version;
                                // send characters to player
                                mmo_char_send006b(fd, sd);
                            }
                            else
                            {
                                // refuse connection (over populated)
                                WFIFOW(fd, 0) = 0x6c;
                                WFIFOB(fd, 2) = 0;
                                WFIFOSET(fd, 3);
                            }
                            goto x65_out;
                        }
                    }
                    // authentification not found
                    {
                        if (login_fd > 0)
                        {
                            // don't send request if no login-server
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
                            WFIFOB(fd, 2) = 0;
                            WFIFOSET(fd, 3);
                        }
                    }
                }
            x65_out:
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
            {
                const struct mmo_charstatus *cd = make_new_char(fd, static_cast<const uint8_t *>(RFIFOP(fd, 2)));
                if (!cd)
                {
                    WFIFOW(fd, 0) = 0x6e;
                    WFIFOB(fd, 2) = 0x00;
                    WFIFOSET(fd, 3);
                    RFIFOSKIP(fd, 37);
                    break;
                }

                WFIFOW(fd, 0) = 0x6d;
                memset(WFIFOP(fd, 2), 0, 106);

                WFIFOL(fd, 2) = cd->char_id;
                WFIFOL(fd, 2 + 4) = cd->base_exp;
                WFIFOL(fd, 2 + 8) = cd->zeny;
                WFIFOL(fd, 2 + 12) = cd->job_exp;
                WFIFOL(fd, 2 + 16) = cd->job_level;

                WFIFOL(fd, 2 + 28) = cd->karma;
                WFIFOL(fd, 2 + 32) = cd->manner;

                WFIFOW(fd, 2 + 40) = 0x30;
                WFIFOW(fd, 2 + 42) = min(cd->hp, 0x7fff);
                WFIFOW(fd, 2 + 44) = min(cd->max_hp, 0x7fff);
                WFIFOW(fd, 2 + 46) = min(cd->sp, 0x7fff);
                WFIFOW(fd, 2 + 48) = min(cd->max_sp, 0x7fff);
                WFIFOW(fd, 2 + 50) = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // char_data[i].speed;
                WFIFOW(fd, 2 + 52) = cd->species;
                WFIFOW(fd, 2 + 54) = cd->hair;

                WFIFOW(fd, 2 + 58) = cd->base_level;
                WFIFOW(fd, 2 + 60) = cd->skill_point;

                WFIFOW(fd, 2 + 64) = cd->shield;
                WFIFOW(fd, 2 + 66) = cd->head_top;
                WFIFOW(fd, 2 + 68) = cd->head_mid;
                WFIFOW(fd, 2 + 70) = cd->hair_color;

                memcpy(WFIFOP(fd, 2 + 74), cd->name, 24);

                WFIFOB(fd, 2 + 98) = min(cd->attrs[ATTR::STR], 255);
                WFIFOB(fd, 2 + 99) = min(cd->attrs[ATTR::AGI], 255);
                WFIFOB(fd, 2 + 100) = min(cd->attrs[ATTR::VIT], 255);
                WFIFOB(fd, 2 + 101) = min(cd->attrs[ATTR::INT], 255);
                WFIFOB(fd, 2 + 102) = min(cd->attrs[ATTR::DEX], 255);
                WFIFOB(fd, 2 + 103) = min(cd->attrs[ATTR::LUK], 255);
                WFIFOB(fd, 2 + 104) = cd->char_num;

                WFIFOSET(fd, 108);
            }
                RFIFOSKIP(fd, 37);
                break;

            case 0x68:         // delete char //Yor's Fix
                if (!sd || RFIFOREST(fd) < 46)
                    return;
                memcpy(email, RFIFOP(fd, 6), 40);
                if (e_mail_check(email) == 0)
                    strzcpy(email, "a@a.com", 40); // default e-mail

                {
                    {
                        struct mmo_charstatus *cs = nullptr;
                        for (mmo_charstatus& cd : char_data)
                        {
                            if (cd.char_id == RFIFOL(fd, 2))
                            {
                                cs = &cd;
                                break;
                            }
                        }

                        if (cs)
                        {
                            char_delete(cs);   // deletion process

                            if (cs != &char_data.back())
                            {
                                std::swap(*cs, char_data.back());
                            }

                            char_data.pop_back();
                            WFIFOW(fd, 0) = 0x6f;
                            WFIFOSET(fd, 2);
                            goto x68_out;
                        }
                    }

                    {
                        WFIFOW(fd, 0) = 0x70;
                        WFIFOB(fd, 2) = 0;
                        WFIFOSET(fd, 3);
                    }
                }
            x68_out:
                RFIFOSKIP(fd, 46);
                break;

            case 0x2af8:       // マップサーバーログイン
                if (RFIFOREST(fd) < 60)
                    return;
            {
                int i;
                WFIFOW(fd, 0) = 0x2af9;
                for (i = 0; i < MAX_MAP_SERVERS; i++)
                {
                    if (server_fd[i] < 0)
                        break;
                }
                if (i == MAX_MAP_SERVERS || strcmp(static_cast<const char *>(RFIFOP(fd, 2)), userid)
                    || strcmp(static_cast<const char *>(RFIFOP(fd, 26)), passwd))
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
                    for (const GM_Account& gma : gm_accounts)
                    {
                        WFIFOL(fd, len) = gma.account_id;
                        WFIFOB(fd, len + 4) = gma.level;
                        len += 5;
                    }
                    WFIFOW(fd, 2) = len;
                    WFIFOSET(fd, len);
                    return;
                }
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
                         static_cast<uint8_t>(h->h_addr[0]),
                         static_cast<uint8_t>(h->h_addr[1]),
                         static_cast<uint8_t>(h->h_addr[2]),
                         static_cast<uint8_t>(h->h_addr[3]));
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
                    subneti[j] = h->h_addr[j];
            }
            else
            {
                SSCANF(w2, "%hhu.%hhu.%hhu.%hhu", &subneti[0], &subneti[1],
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
                    subnetmaski[j] = h->h_addr[j];
            }
            else
            {
                SSCANF(w2, "%hhu.%hhu.%hhu.%hhu", &subnetmaski[0], &subnetmaski[1],
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
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]));
                sprintf(login_ip_str, "%d.%d.%d.%d",
                         static_cast<uint8_t>(h->h_addr[0]),
                         static_cast<uint8_t>(h->h_addr[1]),
                         static_cast<uint8_t>(h->h_addr[2]),
                         static_cast<uint8_t>(h->h_addr[3]));
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
                        w2,
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]));
                sprintf(char_ip_str, "%d.%d.%d.%d",
                         static_cast<uint8_t>(h->h_addr[0]),
                         static_cast<uint8_t>(h->h_addr[1]),
                         static_cast<uint8_t>(h->h_addr[2]),
                         static_cast<uint8_t>(h->h_addr[3]));
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
    // write online players files with no player
    std::fill(online_chars.begin(), online_chars.end(), -1);
    create_online_files();
    online_chars.clear();

    mmo_char_sync();
    inter_save();

    gm_accounts.clear();

    char_data.clear();
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

    Timer(gettick() + std::chrono::seconds(1),
            check_connect_login_server,
            std::chrono::seconds(10)
    ).detach();
    Timer(gettick() + std::chrono::seconds(1),
            send_users_tologin,
            std::chrono::seconds(5)
    ).detach();
    Timer(gettick() + autosave_interval,
            mmo_char_sync_timer,
            autosave_interval
    ).detach();

    if (anti_freeze_enable > 0)
    {
        Timer(gettick() + std::chrono::seconds(1),
                map_anti_freeze_system,
                ANTI_FREEZE_INTERVAL
        ).detach();
    }

    CHAR_LOG("The char-server is ready (Server is listening on the port %d).\n",
              char_port);

    PRINTF("The char-server is \033[1;32mready\033[0m (Server is listening on the port %d).\n\n",
         char_port);

    return 0;
}
