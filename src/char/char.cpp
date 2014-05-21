#include "char.hpp"
//    char.cpp - Character server.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013-2014 MadCamel
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

#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <set>

#include "../ints/cmp.hpp"
#include "../ints/udl.hpp"

#include "../strings/mstring.hpp"
#include "../strings/astring.hpp"
#include "../strings/zstring.hpp"
#include "../strings/xstring.hpp"

#include "../generic/array.hpp"

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
#include "../mmo/mmo.hpp"
#include "../mmo/utils.hpp"
#include "../mmo/version.hpp"

#include "inter.hpp"
#include "int_party.hpp"
#include "int_storage.hpp"

#include "../poison.hpp"

static
Array<struct mmo_map_server, MAX_MAP_SERVERS> server;
static
Array<Session *, MAX_MAP_SERVERS> server_session;
static
Array<int, MAX_MAP_SERVERS> server_freezeflag;    // Map-server anti-freeze system. Counter. 5 ok, 4...0 freezed
static
int anti_freeze_enable = 0;
static
std::chrono::seconds anti_freeze_interval = std::chrono::seconds(6);

constexpr
std::chrono::milliseconds DEFAULT_AUTOSAVE_INTERVAL =
        std::chrono::minutes(5);

static
Session *login_session, *char_session;
static
AccountName userid;
static
AccountPass passwd;
static
ServerName server_name;
static
CharName wisp_server_name = stringish<CharName>("Server"_s);
static
IP4Address login_ip;
static
int login_port = 6900;
static
IP4Address char_ip;
static
int char_port = 6121;
static
AString char_txt;
static
CharName unknown_char_name = stringish<CharName>("Unknown"_s);
static
AString char_log_filename = "log/char.log"_s;
//Added for lan support
static
IP4Address lan_map_ip = IP4_LOCALHOST;
static
IP4Mask lan_subnet = IP4Mask(IP4_LOCALHOST, IP4_BROADCAST);
static
int char_name_option = 0;      // Option to know which letters/symbols are authorised in the name of a character (0: all, 1: only those in char_name_letters, 2: all EXCEPT those in char_name_letters) by [Yor]
static
std::bitset<256> char_name_letters;  // list of letters/symbols authorised (or not) in a character name. by [Yor]
static constexpr
GmLevel default_gm_level = GmLevel::from(0_u32);


struct char_session_data : SessionData
{
    AccountId account_id;
    int login_id1, login_id2;
    SEX sex;
    unsigned short packet_tmw_version;
    AccountEmail email;
    TimeT connect_until_time;
};

void SessionDeleter::operator()(SessionData *sd)
{
    really_delete1 static_cast<char_session_data *>(sd);
}

struct AuthFifoEntry
{
    AccountId account_id;
    CharId char_id;
    int login_id1, login_id2;
    IP4Address ip;
    int delflag;
    SEX sex;
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
CharId char_id_count = wrap<CharId>(150000);
static
std::vector<CharPair> char_keys;
static
int max_connect_user = 0;
static
std::chrono::milliseconds autosave_time = DEFAULT_AUTOSAVE_INTERVAL;

// Initial position (it's possible to set it in conf file)
static
struct point start_point = { {"001-1.gat"_s}, 273, 354 };

static
std::vector<GM_Account> gm_accounts;

// online players by [Yor]
static
AString online_txt_filename = "online.txt"_s;
static
AString online_html_filename = "online.html"_s;
static
int online_sorting_option = 0; // sorting option to display online players in online files
static
int online_refresh_html = 20;  // refresh time (in sec) of the html file in the explorer
static
GmLevel online_gm_display_min_level = GmLevel::from(20_u32);  // minimum GM level to display 'GM' when we want to display it

static
std::vector<Session *> online_chars;              // same size of char_keys, and id value of current server (or -1)
static
TimeT update_online;           // to update online files when we receiving information from a server (not less than 8 seconds)

static
pid_t pid = 0;                  // For forked DB writes

static
void create_online_files(void);

static
void delete_tologin(Session *sess)
{
    assert (sess == login_session);
    PRINTF("Char-server can't connect to login-server (connection #%d).\n"_fmt,
            sess);
    login_session = nullptr;
}
static
void delete_char(Session *sess)
{
    (void)sess;
}
static
void delete_frommap(Session *sess)
{
    auto it = std::find(server_session.begin(), server_session.end(), sess);
    assert (it != server_session.end());
    int id = it - server_session.begin();
    PRINTF("Map-server %d (session #%d) has disconnected.\n"_fmt, id,
            sess);
    server[id] = mmo_map_server{};
    server_session[id] = nullptr;
    for (Session *& oci : online_chars)
        if (oci == sess)
            oci = nullptr;
    create_online_files(); // update online players files (to remove all online players of this server)
}

//------------------------------
// Writing function of logs file
//------------------------------
void char_log(XString line)
{
    io::AppendFile logfp(char_log_filename, true);
    if (!logfp.is_open())
        return;
    log_with_timestamp(logfp, line);
}

//----------------------------------------------------------------------
// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)
//----------------------------------------------------------------------
static
GmLevel isGM(AccountId account_id)
{
    for (GM_Account& gma : gm_accounts)
        if (gma.account_id == account_id)
            return gma.level;
    return default_gm_level;
}

//----------------------------------------------
// Search an character id
//   (return character pointer or nullptr (if not found))
//   If exact character name is not found,
//   the function checks without case sensitive
//   and returns index if only 1 character is found
//   and similar to the searched name.
//----------------------------------------------
const CharPair *search_character(CharName character_name)
{
    for (const CharPair& cd : char_keys)
    {
        {
            // Strict comparison (if found, we finish the function immediatly with correct value)
            if (cd.key.name == character_name)
                return &cd;
        }
    }

    // Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
    return nullptr;
}

const CharPair *search_character_id(CharId char_id)
{
    for (const CharPair& cd : char_keys)
    {
        if (cd.key.char_id == char_id)
            return &cd;
    }

    return nullptr;
}

// TODO make these DIE already
Session *server_for(const CharPair *mcs)
{
    if (!mcs)
        return nullptr;
    return online_chars[mcs - &char_keys.front()];
}
static
Session *& server_for_m(const CharPair *mcs)
{
    return online_chars[mcs - &char_keys.front()];
}

//-------------------------------------------------
// Function to create the character line (for save)
//-------------------------------------------------
static
AString mmo_char_tostr(struct CharPair *cp)
{
    CharKey *k = &cp->key;
    CharData *p = cp->data.get();
    // on multi-map server, sometimes it's posssible that last_point become void. (reason???) We check that to not lost character at restart.
    if (!p->last_point.map_)
    {
        p->last_point = start_point;
    }

    MString str_p;
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
            "%s,%d,%d,%d\t"_fmt,
            k->char_id,
            k->account_id, k->char_num,
            k->name,
            p->species, p->base_level, p->job_level,
            p->base_exp, p->job_exp, p->zeny,
            p->hp, p->max_hp, p->sp, p->max_sp,
            p->attrs[ATTR::STR], p->attrs[ATTR::AGI], p->attrs[ATTR::VIT], p->attrs[ATTR::INT], p->attrs[ATTR::DEX], p->attrs[ATTR::LUK],
            p->status_point, p->skill_point,
            p->option, p->karma, p->manner,
            p->party_id, 0/*guild_id*/, 0/*pet_id*/,
            p->hair, p->hair_color, p->clothes_color,
            p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
            p->last_point.map_, p->last_point.x, p->last_point.y,
            p->save_point.map_, p->save_point.x, p->save_point.y, p->partner_id);

    // memos were here (no longer supported)
    str_p += '\t';

    for (int i = 0; i < MAX_INVENTORY; i++)
        if (p->inventory[i].nameid)
        {
            str_p += STRPRINTF("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d "_fmt,
                    0 /*id*/,
                    p->inventory[i].nameid,
                    p->inventory[i].amount,
                    p->inventory[i].equip,
                    1 /*identify*/,
                    0 /*refine*/,
                    0 /*attribute*/,
                    0 /*card[0]*/,
                    0 /*card[1]*/,
                    0 /*card[2]*/,
                    0 /*card[3]*/,
                    0 /*broken*/);
        }
    str_p += '\t';

    // cart was here (no longer supported)
    str_p += '\t';

    for (SkillID i : erange(SkillID(), MAX_SKILL))
        if (p->skill[i].lv)
        {
            str_p += STRPRINTF("%d,%d "_fmt,
                    i,
                    p->skill[i].lv | (static_cast<uint16_t>(p->skill[i].flags) << 16));
        }
    str_p += '\t';

    assert (p->global_reg_num < GLOBAL_REG_NUM);
    for (int i = 0; i < p->global_reg_num; i++)
        if (p->global_reg[i].str)
            str_p += STRPRINTF("%s,%d "_fmt,
                    p->global_reg[i].str,
                    p->global_reg[i].value);
    str_p += '\t';

    return AString(str_p);
}

static
bool extract(XString str, struct point *p)
{
    return extract(str, record<','>(&p->map_, &p->x, &p->y));
}

struct skill_loader
{
    SkillID id;
    uint16_t level;
    SkillFlags flags;
};

static
bool extract(XString str, struct skill_loader *s)
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
bool extract(XString str, CharPair *cp)
{
    CharKey *k = &cp->key;
    CharData *p = cp->data.get();

    uint32_t unused_guild_id, unused_pet_id;
    XString unused_memos;
    std::vector<struct item> inventory;
    XString unused_cart;
    std::vector<struct skill_loader> skills;
    std::vector<struct global_reg> vars;
    if (!extract(str,
                record<'\t'>(
                    &k->char_id,
                    record<','>(&k->account_id, &k->char_num),
                    &k->name,
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
                    record<','>(&p->save_point.map_, &p->save_point.x, &p->save_point.y, &p->partner_id),
                    &unused_memos,
                    vrec<' '>(&inventory),
                    &unused_cart,
                    vrec<' '>(&skills),
                    vrec<' '>(&vars))))
        return false;

    if (wisp_server_name == k->name)
        return false;

    // TODO replace *every* lookup with a map lookup
    static std::set<CharId> seen_ids;
    static std::set<CharName> seen_names;
    // we don't have to worry about deleted characters,
    // this is only called during startup
    auto _seen_id = seen_ids.insert(k->char_id);
    if (!_seen_id.second)
        return false;
    auto _seen_name = seen_names.insert(k->name);
    if (!_seen_name.second)
    {
        seen_ids.erase(_seen_id.first);
        return false;
    }

    // memos were here - no longer supported

    if (inventory.size() > MAX_INVENTORY)
        return false;
    std::copy(inventory.begin(), inventory.end(), p->inventory.begin());
    // number of inventory items is not saved - it just detects nameid 0

    // cart was here - no longer supported

    for (struct skill_loader& sk : skills)
    {
        if (sk.id >= MAX_SKILL)
            return false;
        p->skill[sk.id].lv = sk.level;
        p->skill[sk.id].flags = sk.flags;
    }

    if (vars.size() > GLOBAL_REG_NUM)
        return false;
    std::copy(vars.begin(), vars.end(), p->global_reg.begin());
    p->global_reg_num = vars.size();

    return true;
}

//---------------------------------
// Function to read characters file
//---------------------------------
static
int mmo_char_init(void)
{
    char_keys.clear();
    online_chars.clear();

    io::ReadFile in(char_txt);
    if (!in.is_open())
    {
        PRINTF("Characters file not found: %s.\n"_fmt, char_txt);
        CHAR_LOG("Characters file not found: %s.\n"_fmt, char_txt);
        CHAR_LOG("Id for the next created character: %d.\n"_fmt,
                char_id_count);
        return 0;
    }

    int line_count = 0;
    AString line;
    while (in.getline(line))
    {
        line_count++;

        if (is_comment(line))
            continue;

        {
            CharId i;
            if (extract(line, record<'\t'>(&i, "%newid%"_s)))
            {
                if (char_id_count < i)
                    char_id_count = i;
                continue;
            }
        }

        CharPair cd;
        if (!extract(line, &cd))
        {
            CHAR_LOG("Char skipped\n%s"_fmt, line);
            continue;
        }
        if (char_id_count < next(cd.key.char_id))
            char_id_count = next(cd.key.char_id);
        char_keys.push_back(std::move(cd));
        online_chars.push_back(nullptr);
    }

    PRINTF("mmo_char_init: %zu characters read in %s.\n"_fmt,
            char_keys.size(), char_txt);
    CHAR_LOG("mmo_char_init: %zu characters read in %s.\n"_fmt,
            char_keys.size(), char_txt);

    CHAR_LOG("Id for the next created character: %d.\n"_fmt,
            char_id_count);

    return 0;
}

//---------------------------------------------------------
// Function to save characters in files (speed up by [Yor])
//---------------------------------------------------------
static
void mmo_char_sync(void)
{
    io::WriteLock fp(char_txt);
    if (!fp.is_open())
    {
        PRINTF("WARNING: Server can't not save characters.\n"_fmt);
        CHAR_LOG("WARNING: Server can't not save characters.\n"_fmt);
        return;
    }
    {
        // yes, we need a mutable reference to do the saves ...
        for (CharPair& cd : char_keys)
        {
            AString line = mmo_char_tostr(&cd);
            fp.put_line(line);
        }
        FPRINTF(fp, "%d\t%%newid%%\n"_fmt, char_id_count);
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
    if ((pid = fork()) > 0) {
        return;
    }

    // If we're a child, run as a lower priority process
    if (pid == 0)
        setpriority(PRIO_PROCESS, getpid(), 10);

    mmo_char_sync();
    inter_save();

    // If we're a child we should suicide now.
    if (pid == 0)
        _exit(0);
}

//-----------------------------------
// Function to create a new character
//-----------------------------------
static
CharPair *make_new_char(Session *s, CharName name, const uint8_t (&stats)[6], uint8_t slot, uint16_t hair_color, uint16_t hair_style)
{
    // ugh
    char_session_data *sd = static_cast<char_session_data *>(s->session_data.get());

    // remove control characters from the name
    if (!name.to__actual().is_print())
    {
        CHAR_LOG("Make new char error (control char received in the name): (connection #%d, account: %d).\n"_fmt,
                s, sd->account_id);
        return nullptr;
    }

    // Eliminate whitespace
    if (name.to__actual() != name.to__actual().strip())
    {
        CHAR_LOG("Make new char error (leading/trailing whitespace): (connection #%d, account: %d, name: '%s'.\n"_fmt,
                s, sd->account_id, name);
        return nullptr;
    }

    // check lenght of character name
    if (name.to__actual().size() < 4)
    {
        CHAR_LOG("Make new char error (character name too small): (connection #%d, account: %d, name: '%s').\n"_fmt,
                s, sd->account_id, name);
        return nullptr;
    }

    // Check Authorised letters/symbols in the name of the character
    if (char_name_option == 1)
    {
        // only letters/symbols in char_name_letters are authorised
        for (uint8_t c : name.to__actual())
            if (!char_name_letters[c])
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n"_fmt,
                        s, sd->account_id, name, c);
                return nullptr;
            }
    }
    else if (char_name_option == 2)
    {
        // letters/symbols in char_name_letters are forbidden
        for (uint8_t c : name.to__actual())
            if (char_name_letters[c])
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n"_fmt,
                        s, sd->account_id, name, c);
                return nullptr;
            }
    }                           // else, all letters/symbols are authorised (except control char removed before)

    // this is why it needs to be unsigned
    if (stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5] != 5 * 6 ||   // stats
        slot >= 9 ||
        hair_style >= 20 ||
        hair_color >= 12)
    {
        CHAR_LOG("Make new char error (invalid values): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d\n"_fmt,
                s, sd->account_id, slot, name,
                stats[0], stats[1], stats[2], stats[3], stats[4], stats[5],
                stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5],
                hair_style, hair_color);
        return nullptr;
    }

    // check individual stat value
    for (int i = 0; i < 6; i++)
    {
        if (stats[i] < 1 || stats[i] > 9)
        {
            CHAR_LOG("Make new char error (invalid stat value: not between 1 to 9): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d\n"_fmt,
                    s, sd->account_id, slot, name,
                    stats[0], stats[1], stats[2], stats[3], stats[4], stats[5],
                    stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5],
                    hair_style, hair_color);
            return nullptr;
        }
    }

    for (const CharPair& cd : char_keys)
    {
        if (cd.key.name == name)
        {
            CHAR_LOG("Make new char error (name already exists): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n"_fmt,
                    s, sd->account_id, slot, name, cd.key.name,
                    stats[0], stats[1], stats[2], stats[3], stats[4], stats[5],
                    stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5],
                    hair_style, hair_color);
            return nullptr;
        }
        if (cd.key.account_id == sd->account_id
            && cd.key.char_num == slot)
        {
            CHAR_LOG("Make new char error (slot already used): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n"_fmt,
                    s, sd->account_id, slot, name, cd.key.name,
                    stats[0], stats[1], stats[2], stats[3], stats[4], stats[5],
                    stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5],
                    hair_style, hair_color);
            return nullptr;
        }
    }

    if (wisp_server_name == name)
    {
        CHAR_LOG("Make new char error (name used is wisp name for server): (connection #%d, account: %d) slot %d, name: %s (actual name whisper server: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n"_fmt,
                s, sd->account_id, slot, name, wisp_server_name,
                stats[0], stats[1], stats[2], stats[3], stats[4], stats[5],
                stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5],
                hair_style, hair_color);
        return nullptr;
    }

    IP4Address ip = s->client_ip;

    CHAR_LOG("Creation of New Character: (connection #%d, account: %d) slot %d, character Name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d. [%s]\n"_fmt,
            s, sd->account_id, slot, name,
            stats[0], stats[1], stats[2], stats[3], stats[4], stats[5],
            stats[0] + stats[1] + stats[2] + stats[3] + stats[4] + stats[5],
            hair_style, hair_color, ip);

    CharPair cp;
    CharKey& ck = cp.key;
    CharData& cd = *cp.data;

    ck.char_id = char_id_count; char_id_count = next(char_id_count);
    ck.account_id = sd->account_id;
    ck.char_num = slot;
    ck.name = name;
    cd.species = Species();
    cd.base_level = 1;
    cd.job_level = 1;
    cd.base_exp = 0;
    cd.job_exp = 0;
    cd.zeny = 0;
    cd.attrs[ATTR::STR] = stats[0];
    cd.attrs[ATTR::AGI] = stats[1];
    cd.attrs[ATTR::VIT] = stats[2];
    cd.attrs[ATTR::INT] = stats[3];
    cd.attrs[ATTR::DEX] = stats[4];
    cd.attrs[ATTR::LUK] = stats[5];
    cd.max_hp = 40 * (100 + cd.attrs[ATTR::VIT]) / 100;
    cd.max_sp = 11 * (100 + cd.attrs[ATTR::INT]) / 100;
    cd.hp = cd.max_hp;
    cd.sp = cd.max_sp;
    cd.status_point = 0;
    cd.skill_point = 0;
    cd.option = static_cast<Option>(0x0000); // Option is only declared
    cd.karma = 0;
    cd.manner = 0;
    cd.party_id = PartyId();
    //cd.guild_id = 0;
    cd.hair = hair_style;
    cd.hair_color = hair_color;
    cd.clothes_color = 0;
    // removed initial armor/weapon - unused and problematic
    cd.weapon = ItemLook::NONE;
    cd.shield = ItemNameId();
    cd.head_top = ItemNameId();
    cd.head_mid = ItemNameId();
    cd.head_bottom = ItemNameId();
    cd.last_point = start_point;
    cd.save_point = start_point;
    char_keys.push_back(std::move(cp));
    online_chars.push_back(nullptr);

    return &char_keys.back();
}

//-------------------------------------------------------------
// Function to create the online files (txt and html). by [Yor]
//-------------------------------------------------------------
static
void create_online_files(void)
{
    // write files
    io::WriteFile fp(online_txt_filename);
    if (fp.is_open())
    {
        io::WriteFile fp2(online_html_filename);
        if (fp2.is_open())
        {
            // get time
            timestamp_seconds_buffer timetemp;
            stamp_time(timetemp);
            // write heading
            FPRINTF(fp2, "<HTML>\n"_fmt);
            FPRINTF(fp2, "  <META http-equiv=\"Refresh\" content=\"%d\">\n"_fmt, online_refresh_html); // update on client explorer every x seconds
            FPRINTF(fp2, "  <HEAD>\n"_fmt);
            FPRINTF(fp2, "    <TITLE>Online Players on %s</TITLE>\n"_fmt,
                    server_name);
            FPRINTF(fp2, "  </HEAD>\n"_fmt);
            FPRINTF(fp2, "  <BODY>\n"_fmt);
            FPRINTF(fp2, "    <H3>Online Players on %s (%s):</H3>\n"_fmt,
                    server_name, timetemp);
            FPRINTF(fp, "Online Players on %s (%s):\n"_fmt, server_name, timetemp);
            FPRINTF(fp, "\n"_fmt);

            int players = 0;
            // This used to be conditional on having any players,
            // but this simplifies the logic.
            //if (players > 0)
            {
                int j = 0;          // count the number of characters for the txt version and to set the separate line
                FPRINTF(fp2, "    <table border=\"1\" cellspacing=\"1\">\n"_fmt);
                FPRINTF(fp2, "      <tr>\n"_fmt);
                {
                    FPRINTF(fp2, "        <th>Name</th>\n"_fmt);
                    {
                        FPRINTF(fp, "Name                          "_fmt); // 30
                        j += 30;
                    }
                }
                FPRINTF(fp2, "      </tr>\n"_fmt);
                FPRINTF(fp, "\n"_fmt);
                for (int k = 0; k < j; k++)
                    FPRINTF(fp, "-"_fmt);
                FPRINTF(fp, "\n"_fmt);

                // display each player.
                for (CharPair& cd : char_keys)
                {
                    if (!server_for(&cd))
                        continue;
                    players++;
                    FPRINTF(fp2, "      <tr>\n"_fmt);
                    // displaying the character name
                    {
                        // without/with 'GM' display
                        GmLevel gml = isGM(cd.key.account_id);
                        {
                            if (gml.satisfies(online_gm_display_min_level))
                                FPRINTF(fp, "%-24s (GM) "_fmt, cd.key.name);
                            else
                                FPRINTF(fp, "%-24s      "_fmt, cd.key.name);
                        }
                        // name of the character in the html (no < >, because that create problem in html code)
                        FPRINTF(fp2, "        <td>"_fmt);
                        if (gml.satisfies(online_gm_display_min_level))
                            FPRINTF(fp2, "<b>"_fmt);
                        for (char c : cd.key.name.to__actual())
                        {
                            switch (c)
                            {
                            case '&':
                                FPRINTF(fp2, "&amp;"_fmt);
                                break;
                            case '<':
                                FPRINTF(fp2, "&lt;"_fmt);
                                break;
                            case '>':
                                FPRINTF(fp2, "&gt;"_fmt);
                                break;
                            default:
                                FPRINTF(fp2, "%c"_fmt, c);
                                break;
                            };
                        }
                        if (gml.satisfies(online_gm_display_min_level))
                            FPRINTF(fp2, "</b> (GM)"_fmt);
                        FPRINTF(fp2, "</td>\n"_fmt);
                    }
                    FPRINTF(fp, "\n"_fmt);
                    FPRINTF(fp2, "      </tr>\n"_fmt);
                }
                FPRINTF(fp2, "    </table>\n"_fmt);
                FPRINTF(fp, "\n"_fmt);
            }

            // Displaying number of online players
            if (players == 0)
            {
                FPRINTF(fp2, "    <p>No user is online.</p>\n"_fmt);
                FPRINTF(fp, "No user is online.\n"_fmt);
            }
            else if (players == 1)
            {
                // no display if only 1 player
            }
            else
            {
                FPRINTF(fp2, "    <p>%d users are online.</p>\n"_fmt, players);
                FPRINTF(fp, "%d users are online.\n"_fmt, players);
            }
            FPRINTF(fp2, "  </BODY>\n"_fmt);
            FPRINTF(fp2, "</HTML>\n"_fmt);
        }
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
        if (server_session[i])
            users += server[i].users;

    return users;
}

//----------------------------------------
// [Fate] Find inventory item based on equipment mask, return view.  ID must match view ID (!).
//----------------------------------------
static
ItemNameId find_equip_view(const CharPair *cp, EPOS equipmask)
{
    CharData *p = cp->data.get();
    for (int i = 0; i < MAX_INVENTORY; i++)
        if (p->inventory[i].nameid && p->inventory[i].amount
            && bool(p->inventory[i].equip & equipmask))
            return p->inventory[i].nameid;
    return ItemNameId();
}

//----------------------------------------
// Function to send characters to a player
//----------------------------------------
static
int mmo_char_send006b(Session *s, struct char_session_data *sd)
{
    int found_num = 0;
    std::array<const CharPair *, 9> found_char;
    for (const CharPair& cd : char_keys)
    {
        if (cd.key.account_id == sd->account_id)
        {
            found_char[found_num] = &cd;
            found_num++;
            if (found_num == 9)
                break;
        }
    }

    const int offset = 24;
    WFIFO_ZERO(s, 0, offset + found_num * 106);
    WFIFOW(s, 0) = 0x6b;
    WFIFOW(s, 2) = offset + found_num * 106;

    for (int i = 0; i < found_num; i++)
    {
        const CharPair *cp = found_char[i];
        int j = offset + (i * 106);

        const CharKey *k = &cp->key;
        const CharData *p = cp->data.get();

        WFIFOL(s, j) = unwrap<CharId>(k->char_id);
        WFIFOL(s, j + 4) = p->base_exp;
        WFIFOL(s, j + 8) = p->zeny;
        WFIFOL(s, j + 12) = p->job_exp;
        WFIFOL(s, j + 16) = p->job_level;

        WFIFOW(s, j + 20) = unwrap<ItemNameId>(find_equip_view(cp, EPOS::SHOES));
        WFIFOW(s, j + 22) = unwrap<ItemNameId>(find_equip_view(cp, EPOS::GLOVES));
        WFIFOW(s, j + 24) = unwrap<ItemNameId>(find_equip_view(cp, EPOS::CAPE));
        WFIFOW(s, j + 26) = unwrap<ItemNameId>(find_equip_view(cp, EPOS::MISC1));
        WFIFOL(s, j + 28) = static_cast<uint16_t>(p->option);

        WFIFOL(s, j + 32) = p->karma;
        WFIFOL(s, j + 36) = p->manner;

        WFIFOW(s, j + 40) = p->status_point;
        WFIFOW(s, j + 42) = std::min(p->hp, 0x7fff);
        WFIFOW(s, j + 44) = std::min(p->max_hp, 0x7fff);
        WFIFOW(s, j + 46) = std::min(p->sp, 0x7fff);
        WFIFOW(s, j + 48) = std::min(p->max_sp, 0x7fff);
        WFIFOW(s, j + 50) = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // p->speed;
        WFIFOW(s, j + 52) = unwrap<Species>(p->species);
        WFIFOW(s, j + 54) = p->hair;
//      WFIFOW(s,j+56) = p->weapon; // dont send weapon since TMW does not support it
        WFIFOW(s, j + 56) = 0;
        WFIFOW(s, j + 58) = p->base_level;
        WFIFOW(s, j + 60) = p->skill_point;
        WFIFOW(s, j + 62) = unwrap<ItemNameId>(p->head_bottom);
        WFIFOW(s, j + 64) = unwrap<ItemNameId>(p->shield);
        WFIFOW(s, j + 66) = unwrap<ItemNameId>(p->head_top);
        WFIFOW(s, j + 68) = unwrap<ItemNameId>(p->head_mid);
        WFIFOW(s, j + 70) = p->hair_color;
        WFIFOW(s, j + 72) = unwrap<ItemNameId>(find_equip_view(cp, EPOS::MISC2));
//      WFIFOW(s,j+72) = p->clothes_color;

        WFIFO_STRING(s, j + 74, k->name.to__actual(), 24);

        WFIFOB(s, j + 98) = saturate<uint8_t>(p->attrs[ATTR::STR]);
        WFIFOB(s, j + 99) = saturate<uint8_t>(p->attrs[ATTR::AGI]);
        WFIFOB(s, j + 100) = saturate<uint8_t>(p->attrs[ATTR::VIT]);
        WFIFOB(s, j + 101) = saturate<uint8_t>(p->attrs[ATTR::INT]);
        WFIFOB(s, j + 102) = saturate<uint8_t>(p->attrs[ATTR::DEX]);
        WFIFOB(s, j + 103) = saturate<uint8_t>(p->attrs[ATTR::LUK]);
        WFIFOB(s, j + 104) = k->char_num;
    }

    WFIFOSET(s, WFIFOW(s, 2));

    return 0;
}

static
int set_account_reg2(AccountId acc, Slice<global_reg> reg)
{
    size_t num = reg.size();
    assert (num < ACCOUNT_REG2_NUM);
    int c = 0;
    for (CharPair& cd : char_keys)
    {
        if (cd.key.account_id == acc)
        {
            for (int i = 0; i < num; ++i)
                cd.data->account_reg2[i] = reg[i];
            cd.data->account_reg2_num = num;
            for (int i = num; i < ACCOUNT_REG2_NUM; ++i)
                cd.data->account_reg2[i] = global_reg{};
            c++;
        }
    }
    return c;
}

// Divorce a character from it's partner and let the map server know
static
int char_divorce(CharPair *cp)
{
    uint8_t buf[10];

    if (cp == NULL)
        return 0;

    CharKey *ck = &cp->key;
    CharData *cs = cp->data.get();

    if (!cs->partner_id)
    {
        WBUFW(buf, 0) = 0x2b12;
        WBUFL(buf, 2) = unwrap<CharId>(ck->char_id);
        // partner id 0 means failure
        WBUFL(buf, 6) = unwrap<CharId>(cs->partner_id);
        mapif_sendall(buf, 10);
        return 0;
    }

    WBUFW(buf, 0) = 0x2b12;
    WBUFL(buf, 2) = unwrap<CharId>(ck->char_id);

    for (CharPair& cd : char_keys)
    {
        if (cd.key.char_id == cs->partner_id
            && cd.data->partner_id == ck->char_id)
        {
            WBUFL(buf, 6) = unwrap<CharId>(cs->partner_id);
            mapif_sendall(buf, 10);
            cs->partner_id = CharId();
            cd.data->partner_id = CharId();
            return 0;
        }
        // The other char doesn't have us as their partner, so just clear our partner
        // Don't worry about this, as the map server should verify itself that the other doesn't have us as a partner, and so won't mess with their marriage
        else if (cd.key.char_id == cs->partner_id)
        {
            WBUFL(buf, 6) = unwrap<CharId>(cs->partner_id);
            mapif_sendall(buf, 10);
            cs->partner_id = CharId();
            return 0;
        }
    }

    // Our partner wasn't found, so just clear our marriage
    WBUFL(buf, 6) = unwrap<CharId>(cs->partner_id);
    cs->partner_id = CharId();
    mapif_sendall(buf, 10);

    return 0;
}

//----------------------------------------------------------------------
// Force disconnection of an online player (with account value) by [Yor]
//----------------------------------------------------------------------
static
void disconnect_player(AccountId accound_id)
{
    // disconnect player if online on char-server
    for (io::FD i : iter_fds())
    {
        Session *s = get_session(i);
        if (!s)
            continue;
        struct char_session_data *sd = static_cast<char_session_data *>(s->session_data.get());
        if (sd)
        {
            if (sd->account_id == accound_id)
            {
                s->set_eof();
                return;
            }
        }
    }
}

// キャラ削除に伴うデータ削除
static
int char_delete(CharPair *cp)
{
    CharKey *ck = &cp->key;
    CharData *cs = cp->data.get();

    // パーティー脱退
    if (cs->party_id)
        inter_party_leave(cs->party_id, ck->account_id);
    // 離婚
    if (cs->partner_id)
        char_divorce(cp);

    // Force the character (and all on the same account) to leave all map servers
    {
        unsigned char buf[6];
        WBUFW(buf, 0) = 0x2afe;
        WBUFL(buf, 2) = unwrap<AccountId>(ck->account_id);
        mapif_sendall(buf, 6);
    }

    return 0;
}

static
void parse_tologin(Session *ls)
{
    // only login-server can have an access to here.
    // so, if it isn't the login-server, we disconnect the session (fd != login_fd).
    if (ls != login_session)
    {
        ls->set_eof();
        return;
    }

    char_session_data *sd = static_cast<char_session_data *>(ls->session_data.get());

    while (RFIFOREST(ls) >= 2)
    {
        switch (RFIFOW(ls, 0))
        {
            case 0x2711:
                if (RFIFOREST(ls) < 3)
                    return;
                if (RFIFOB(ls, 2))
                {
                    PRINTF("Can not connect to login-server.\n"_fmt);
                    PRINTF("The server communication passwords (default s1/p1) is probably invalid.\n"_fmt);
                    PRINTF("Also, please make sure your accounts file (default: accounts.txt) has those values present.\n"_fmt);
                    PRINTF("If you changed the communication passwords, change them back at map_athena.conf and char_athena.conf\n"_fmt);
                    exit(1);
                }
                else
                {
                    PRINTF("Connected to login-server (connection #%d).\n"_fmt,
                            ls);
                    // if no map-server already connected, display a message...
                    int i;
                    for (i = 0; i < MAX_MAP_SERVERS; i++)
                        if (server_session[i] && server[i].maps[0])   // if map-server online and at least 1 map
                            break;
                    if (i == MAX_MAP_SERVERS)
                        PRINTF("Awaiting maps from map-server.\n"_fmt);
                }
                RFIFOSKIP(ls, 3);
                break;

            case 0x2713:
                if (RFIFOREST(ls) < 51)
                    return;
                for (io::FD i : iter_fds())
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(ls, 2));
                    Session *s2 = get_session(i);
                    if (!s2)
                        continue;
                    sd = static_cast<char_session_data *>(s2->session_data.get());
                    if (sd && sd->account_id == acc)
                    {
                        if (RFIFOB(ls, 6) != 0)
                        {
                            WFIFOW(s2, 0) = 0x6c;
                            WFIFOB(s2, 2) = 0x42;
                            WFIFOSET(s2, 3);
                        }
                        else if (max_connect_user == 0
                                 || count_users() < max_connect_user)
                        {
                            sd->email = stringish<AccountEmail>(RFIFO_STRING<40>(ls, 7));
                            if (!e_mail_check(sd->email))
                                sd->email = DEFAULT_EMAIL;
                            sd->connect_until_time = static_cast<time_t>(RFIFOL(ls, 47));
                            // send characters to player
                            mmo_char_send006b(s2, sd);
                        }
                        else
                        {
                            // refuse connection: too much online players
                            WFIFOW(s2, 0) = 0x6c;
                            WFIFOB(s2, 2) = 0;
                            WFIFOSET(s2, 3);
                        }
                        break;
                    }
                }
                RFIFOSKIP(ls, 51);
                break;

                // Receiving of an e-mail/time limit from the login-server (answer of a request because a player comes back from map-server to char-server) by [Yor]
            case 0x2717:
                if (RFIFOREST(ls) < 50)
                    return;
                for (io::FD i : iter_fds())
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(ls, 2));
                    Session *s2 = get_session(i);
                    if (!s2)
                        continue;
                    sd = static_cast<char_session_data *>(s2->session_data.get());
                    if (sd)
                    {
                        if (sd->account_id == acc)
                        {
                            sd->email = stringish<AccountEmail>(RFIFO_STRING<40>(ls, 6));
                            if (!e_mail_check(sd->email))
                                sd->email = DEFAULT_EMAIL;
                            sd->connect_until_time = static_cast<time_t>(RFIFOL(ls, 46));
                            break;
                        }
                    }
                }
                RFIFOSKIP(ls, 50);
                break;

            case 0x2721:       // gm reply
                if (RFIFOREST(ls) < 10)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(ls, 2));
                    GmLevel gml = GmLevel::from(RFIFOL(ls, 2));
                    unsigned char buf[10];
                    WBUFW(buf, 0) = 0x2b0b;
                    WBUFL(buf, 2) = unwrap<AccountId>(acc);
                    WBUFL(buf, 6) = gml.get_all_bits();
                    mapif_sendall(buf, 10);
                }
                RFIFOSKIP(ls, 10);
                break;

            case 0x2723:       // changesex reply (modified by [Yor])
                if (RFIFOREST(ls) < 7)
                    return;
                {
                    unsigned char buf[7];
                    AccountId acc = wrap<AccountId>(RFIFOL(ls, 2));
                    SEX sex = static_cast<SEX>(RFIFOB(ls, 6));
                    RFIFOSKIP(ls, 7);
                    if (acc)
                    {
                        for (CharPair& cp : char_keys)
                        {
                            CharKey& ck = cp.key;
                            CharData& cd = *cp.data;
                            if (ck.account_id == acc)
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
                                cd.shield = ItemNameId();
                                cd.head_top = ItemNameId();
                                cd.head_mid = ItemNameId();
                                cd.head_bottom = ItemNameId();
                            }
                        }
                        // disconnect player if online on char-server
                        disconnect_player(acc);
                    }
                    WBUFW(buf, 0) = 0x2b0d;
                    WBUFL(buf, 2) = unwrap<AccountId>(acc);
                    WBUFB(buf, 6) = static_cast<uint8_t>(sex);
                    mapif_sendall(buf, 7);
                }
                break;

            case 0x2726:       // Request to send a broadcast message (no answer)
                if (RFIFOREST(ls) < 8
                    || RFIFOREST(ls) < (8 + RFIFOL(ls, 4)))
                    return;
                if (RFIFOL(ls, 4) < 1)
                    CHAR_LOG("Receiving a message for broadcast, but message is void.\n"_fmt);
                else
                {
                    int i;
                    // at least 1 map-server
                    for (i = 0; i < MAX_MAP_SERVERS; i++)
                        if (server_session[i])
                            break;
                    if (i == MAX_MAP_SERVERS)
                        CHAR_LOG("'ladmin': Receiving a message for broadcast, but no map-server is online.\n"_fmt);
                    else
                    {
                        size_t len = RFIFOL(ls, 4);
                        AString message = RFIFO_STRING(ls, 8, len).to_print().lstrip();
                        // if message is only composed of spaces
                        if (!message)
                            CHAR_LOG("Receiving a message for broadcast, but message is only a lot of spaces.\n"_fmt);
                        // else send message to all map-servers
                        else
                        {
                            CHAR_LOG("'ladmin': Receiving a message for broadcast (message (in yellow): %s)\n"_fmt,
                                    message);
                            // send broadcast to all map-servers
                            uint8_t buf[4 + len];
                            WBUFW(buf, 0) = 0x3800;
                            WBUFW(buf, 2) = 4 + len;
                            WBUF_STRING(buf, 4, message, len);
                            mapif_sendall(buf, WBUFW(buf, 2));
                        }
                    }
                }
                RFIFOSKIP(ls, 8 + RFIFOL(ls, 4));
                break;

                // account_reg2変更通知
            case 0x2729:
                if (RFIFOREST(ls) < 4 || RFIFOREST(ls) < RFIFOW(ls, 2))
                    return;
                {
                    Array<struct global_reg, ACCOUNT_REG2_NUM> reg;
                    int j, p;
                    AccountId acc = wrap<AccountId>(RFIFOL(ls, 4));
                    for (p = 8, j = 0;
                         p < RFIFOW(ls, 2) && j < ACCOUNT_REG2_NUM;
                         p += 36, j++)
                    {
                        reg[j].str = stringish<VarName>(RFIFO_STRING<32>(ls, p));
                        reg[j].value = RFIFOL(ls, p + 32);
                    }
                    set_account_reg2(acc, Slice<struct global_reg>(reg.begin(), j));

                    size_t len = RFIFOW(ls, 2);
                    uint8_t buf[len];
                    RFIFO_BUF_CLONE(ls, buf, len);
                    WBUFW(buf, 0) = 0x2b11;
                    mapif_sendall(buf, len);
                }
                RFIFOSKIP(ls, RFIFOW(ls, 2));
                break;

            case 0x7924:
            {                   // [Fate] Itemfrob package: forwarded from login-server
                if (RFIFOREST(ls) < 10)
                    return;
                ItemNameId source_id = wrap<ItemNameId>(RFIFOL(ls, 2));
                ItemNameId dest_id = wrap<ItemNameId>(RFIFOL(ls, 6));
                unsigned char buf[10];

                WBUFW(buf, 0) = 0x2afa;
                WBUFL(buf, 2) = unwrap<ItemNameId>(source_id);
                WBUFL(buf, 6) = unwrap<ItemNameId>(dest_id);

                mapif_sendall(buf, 10);    // forward package to map servers
                for (CharPair& cp : char_keys)
                {
                    CharKey *k = &cp.key;
                    CharData& cd = *cp.data.get();
                    CharData *c = &cd;
                    struct storage *s = account2storage(k->account_id);
                    int changes = 0;
                    int j;
#define FIX(v) if (v == source_id) {v = dest_id; ++changes; }
                    for (j = 0; j < MAX_INVENTORY; j++)
                        FIX(c->inventory[j].nameid);
                    // used to FIX cart, but it's no longer supported
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
                        CHAR_LOG("itemfrob(%d -> %d):  `%s'(%d, account %d): changed %d times\n"_fmt,
                                source_id, dest_id, k->name, k->char_id,
                                k->account_id, changes);

                }

                mmo_char_sync();
                inter_storage_save();
                RFIFOSKIP(ls, 10);
                break;
            }

                // Account deletion notification (from login-server)
            case 0x2730:
                if (RFIFOREST(ls) < 6)
                    return;
            {
                AccountId aid = wrap<AccountId>(RFIFOL(ls, 2));

                // Deletion of all characters of the account
//#warning "This comment is a lie, but it's still true."
                // needs to use index because they may move during resize
                for (int idx = 0; idx < char_keys.size(); idx++)
                {
                    CharPair& cp = char_keys[idx];
                    CharKey& ck = cp.key;
                    if (ck.account_id == aid)
                    {
                        char_delete(&cp);
                        if (&cp != &char_keys.back())
                        {
                            std::swap(cp, char_keys.back());
                            // if moved character owns to deleted account, check again it's character
                            // YES this is the newly swapped one
                            // we could avoid this by working backwards
                            if (ck.account_id == aid)
                            {
                                idx--;
                                // Correct moved character reference in the character's owner by [Yor]
                            }
                        }
                        char_keys.pop_back();
                    }
                }
                // Deletion of the storage
                inter_storage_delete(aid);
                // send to all map-servers to disconnect the player
                {
                    unsigned char buf[6];
                    WBUFW(buf, 0) = 0x2b13;
                    WBUFL(buf, 2) = unwrap<AccountId>(aid);
                    mapif_sendall(buf, 6);
                }
                // disconnect player if online on char-server
                disconnect_player(aid);
            }
                RFIFOSKIP(ls, 6);
                break;

                // State change of account/ban notification (from login-server) by [Yor]
            case 0x2731:
                if (RFIFOREST(ls) < 11)
                    return;
            {
                AccountId aid = wrap<AccountId>(RFIFOL(ls, 2));
                // send to all map-servers to disconnect the player
                {
                    unsigned char buf[11];
                    WBUFW(buf, 0) = 0x2b14;
                    WBUFL(buf, 2) = unwrap<AccountId>(aid);
                    WBUFB(buf, 6) = RFIFOB(ls, 6);    // 0: change of statut, 1: ban
                    WBUFL(buf, 7) = RFIFOL(ls, 7);    // status or final date of a banishment
                    mapif_sendall(buf, 11);
                }
                // disconnect player if online on char-server
                disconnect_player(aid);
            }
                RFIFOSKIP(ls, 11);
                break;

                // Receiving GM acounts info from login-server (by [Yor])
            case 0x2732:
                if (RFIFOREST(ls) < 4 || RFIFOREST(ls) < RFIFOW(ls, 2))
                    return;
                {
                    size_t len = RFIFOW(ls, 2);
                    uint8_t buf[len];
                    gm_accounts.clear();
                    gm_accounts.reserve((len - 4) / 5);
                    for (int i = 4; i < len; i += 5)
                    {
                        gm_accounts.push_back({wrap<AccountId>(RFIFOL(ls, i)), GmLevel::from(static_cast<uint32_t>(RFIFOB(ls, i + 4)))});
                    }
                    PRINTF("From login-server: receiving of %zu GM accounts information.\n"_fmt,
                            gm_accounts.size());
                    CHAR_LOG("From login-server: receiving of %zu GM accounts information.\n"_fmt,
                            gm_accounts.size());
                    create_online_files(); // update online players files (perhaps some online players change of GM level)
                    // send new gm acccounts level to map-servers
                    RFIFO_BUF_CLONE(ls, buf, len);
                    WBUFW(buf, 0) = 0x2b15;
                    mapif_sendall(buf, len);
                }
                RFIFOSKIP(ls, RFIFOW(ls, 2));
                break;

            case 0x2741:       // change password reply
                if (RFIFOREST(ls) < 7)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(ls, 2));
                    int status = RFIFOB(ls, 6);

                    for (io::FD i : iter_fds())
                    {
                        Session *s2 = get_session(i);
                        if (!s2)
                            continue;
                        sd = static_cast<char_session_data *>(s2->session_data.get());
                        if (sd)
                        {
                            if (sd->account_id == acc)
                            {
                                WFIFOW(s2, 0) = 0x62;
                                WFIFOB(s2, 2) = status;
                                WFIFOSET(s2, 3);
                                break;
                            }
                        }
                    }
                }
                RFIFOSKIP(ls, 7);
                break;

            default:
                ls->set_eof();
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

    for (i = 0; i < MAX_MAP_SERVERS; i++)
    {
        if (server_session[i])
        {                       // if map-server is online
            if (server_freezeflag[i]-- < 1)
            {                   // Map-server anti-freeze system. Counter. 5 ok, 4...0 freezed
                PRINTF("Map-server anti-freeze system: char-server #%d is freezed -> disconnection.\n"_fmt,
                        i);
                CHAR_LOG("Map-server anti-freeze system: char-server #%d is freezed -> disconnection.\n"_fmt,
                        i);
                server_session[i]->set_eof();
            }
        }
    }
}

static
void parse_frommap(Session *ms)
{
    int id;
    for (id = 0; id < MAX_MAP_SERVERS; id++)
        if (server_session[id] == ms)
            break;
    if (id == MAX_MAP_SERVERS)
    {
        ms->set_eof();
        return;
    }

    while (RFIFOREST(ms) >= 2)
    {
        switch (RFIFOW(ms, 0))
        {
                // request from map-server to reload GM accounts. Transmission to login-server (by Yor)
            case 0x2af7:
                if (login_session)
                {               // don't send request if no login-server
                    WFIFOW(login_session, 0) = 0x2709;
                    WFIFOSET(login_session, 2);
                }
                RFIFOSKIP(ms, 2);
                break;

                // Receiving map names list from the map-server
            case 0x2afa:
                if (RFIFOREST(ms) < 4 || RFIFOREST(ms) < RFIFOW(ms, 2))
                    return;
            {
                for (MapName &foo : server[id].maps)
                    foo = MapName();
                int j = 0;
                for (int i = 4; i < RFIFOW(ms, 2); i += 16)
                {
                    server[id].maps[j] = RFIFO_STRING<16>(ms, i);
                    j++;
                }
                {
                    PRINTF("Map-Server %d connected: %d maps, from IP %s port %d.\n"_fmt,
                            id, j, server[id].ip, server[id].port);
                    PRINTF("Map-server %d loading complete.\n"_fmt, id);
                    CHAR_LOG("Map-Server %d connected: %d maps, from IP %s port %d. Map-server %d loading complete.\n"_fmt,
                            id, j, server[id].ip,
                            server[id].port, id);
                }
                WFIFOW(ms, 0) = 0x2afb;
                WFIFOB(ms, 2) = 0;
                WFIFO_STRING(ms, 3, wisp_server_name.to__actual(), 24);
                WFIFOSET(ms, 27);
                {
                    unsigned char buf[16384];
                    if (j == 0)
                    {
                        PRINTF("WARNING: Map-Server %d have NO map.\n"_fmt, id);
                        CHAR_LOG("WARNING: Map-Server %d have NO map.\n"_fmt,
                                id);
                        // Transmitting maps information to the other map-servers
                    }
                    else
                    {
                        WBUFW(buf, 0) = 0x2b04;
                        WBUFW(buf, 2) = j * 16 + 10;
                        WBUFIP(buf, 4) = server[id].ip;
                        WBUFW(buf, 8) = server[id].port;
                        // server[id].maps[i] = RFIFO_STRING(fd, 4 + i * 16)
                        for (int i = 0; i < j; ++i)
                            WBUF_STRING(buf, 10, server[id].maps[i], 16);
                        mapif_sendallwos(ms, buf, WBUFW(buf, 2));
                    }
                    // Transmitting the maps of the other map-servers to the new map-server
                    for (int x = 0; x < MAX_MAP_SERVERS; x++)
                    {
                        if (server_session[x] && x != id)
                        {
                            WFIFOW(ms, 0) = 0x2b04;
                            WFIFOIP(ms, 4) = server[x].ip;
                            WFIFOW(ms, 8) = server[x].port;
                            j = 0;
                            for (int i = 0; i < MAX_MAP_PER_SERVER; i++)
                                if (server[x].maps[i])
                                    WFIFO_STRING(ms, 10 + (j++) * 16, server[x].maps[i], 16);
                            if (j > 0)
                            {
                                WFIFOW(ms, 2) = j * 16 + 10;
                                WFIFOSET(ms, WFIFOW(ms, 2));
                            }
                        }
                    }
                }
            }
                RFIFOSKIP(ms, RFIFOW(ms, 2));
                break;

                // 認証要求
            case 0x2afc:
                if (RFIFOREST(ms) < 22)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(ms, 2));
                CharId char_id = wrap<CharId>(RFIFOL(ms, 6));
                int login_id1 = RFIFOL(ms, 10);
                int login_id2 = RFIFOL(ms, 14);
                IP4Address ip = RFIFOIP(ms, 18);
                for (AuthFifoEntry& afi : auth_fifo)
                {
                    if (afi.account_id == account_id &&
                        afi.char_id == char_id &&
                        afi.login_id1 == login_id1 &&
                        // here, it's the only area where it's possible that we doesn't know login_id2 (map-server asks just after 0x72 packet, that doesn't given the value)
                        (afi.login_id2 == login_id2 || login_id2 == 0) &&  // relate to the versions higher than 18
                        (!check_ip_flag || afi.ip == ip)
                        && !afi.delflag)
                    {
                        CharPair *cp = nullptr;
                        for (CharPair& cdi : char_keys)
                        {
                            if (cdi.key.char_id == afi.char_id)
                            {
                                cp = &cdi;
                                break;
                            }
                        }
                        assert (cp && "uh-oh - deleted while in queue?"_s);

                        CharKey *ck = &cp->key;
                        CharData *cd = cp->data.get();

                        afi.delflag = 1;
                        WFIFOW(ms, 0) = 0x2afd;
                        WFIFOW(ms, 2) = 18 + sizeof(*ck) + sizeof(*cd);
                        WFIFOL(ms, 4) = unwrap<AccountId>(account_id);
                        WFIFOL(ms, 8) = afi.login_id2;
                        WFIFOL(ms, 12) = static_cast<time_t>(afi.connect_until_time);
                        cd->sex = afi.sex;
                        WFIFOW(ms, 16) = afi.packet_tmw_version;
                        FPRINTF(stderr,
                                "From queue index %zd: recalling packet version %d\n"_fmt,
                                (&afi - &auth_fifo.front()), afi.packet_tmw_version);
                        WFIFO_STRUCT(ms, 18, *ck);
                        WFIFO_STRUCT(ms, 18 + sizeof(*ck), *cd);
                        WFIFOSET(ms, WFIFOW(ms, 2));
                        goto x2afc_out;
                    }
                }
                {
                    WFIFOW(ms, 0) = 0x2afe;
                    WFIFOL(ms, 2) = unwrap<AccountId>(account_id);
                    WFIFOSET(ms, 6);
                    PRINTF("auth_fifo search error! account %d not authentified.\n"_fmt,
                            account_id);
                }
            }
            x2afc_out:
                RFIFOSKIP(ms, 22);
                break;

                // MAPサーバー上のユーザー数受信
            case 0x2aff:
                if (RFIFOREST(ms) < 6 || RFIFOREST(ms) < RFIFOW(ms, 2))
                    return;
                server[id].users = RFIFOW(ms, 4);
                if (anti_freeze_enable)
                    server_freezeflag[id] = 5;  // Map anti-freeze system. Counter. 5 ok, 4...0 freezed
                // remove all previously online players of the server
                for (Session *& oci : online_chars)
                {
                    // there was a bug here
                    if (oci == ms)
                        oci = nullptr;
                }
                // add online players in the list by [Yor]
                for (int i = 0; i < server[id].users; i++)
                {
                    CharId char_id = wrap<CharId>(RFIFOL(ms, 6 + i * 4));
                    for (const CharPair& cd : char_keys)
                    {
                        if (cd.key.char_id == char_id)
                        {
                            server_for_m(&cd) = ms;
                            break;
                        }
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
                RFIFOSKIP(ms, RFIFOW(ms, 2));
                break;

                // キャラデータ保存
            case 0x2b01:
                if (RFIFOREST(ms) < 4 || RFIFOREST(ms) < RFIFOW(ms, 2))
                    return;
            {
                AccountId aid = wrap<AccountId>(RFIFOL(ms, 4));
                CharId cid = wrap<CharId>(RFIFOL(ms, 8));
                for (CharPair& cd : char_keys)
                {
                    if (cd.key.account_id == aid &&
                        cd.key.char_id == cid)
                    {
                        RFIFO_STRUCT(ms, 12, cd.key);
                        RFIFO_STRUCT(ms, 12 + sizeof(cd.key), *cd.data);
                        break;
                    }
                }
            }
                RFIFOSKIP(ms, RFIFOW(ms, 2));
                break;

                // キャラセレ要求
            case 0x2b02:
                if (RFIFOREST(ms) < 18)
                    return;
            {
                AccountId account_id = wrap<AccountId>(RFIFOL(ms, 2));
                if (auth_fifo_iter == auth_fifo.end())
                    auth_fifo_iter = auth_fifo.begin();
                auth_fifo_iter->account_id = account_id;
                auth_fifo_iter->char_id = CharId();
                auth_fifo_iter->login_id1 = RFIFOL(ms, 6);
                auth_fifo_iter->login_id2 = RFIFOL(ms, 10);
                auth_fifo_iter->delflag = 2;
                auth_fifo_iter->connect_until_time = TimeT();    // unlimited/unknown time by default (not display in map-server)
                auth_fifo_iter->ip = RFIFOIP(ms, 14);
                auth_fifo_iter++;
                WFIFOW(ms, 0) = 0x2b03;
                WFIFOL(ms, 2) = unwrap<AccountId>(account_id);
                WFIFOB(ms, 6) = 0;
                WFIFOSET(ms, 7);
            }
                RFIFOSKIP(ms, 18);
                break;

                // マップサーバー間移動要求
            case 0x2b05:
                if (RFIFOREST(ms) < 49)
                    return;
                if (auth_fifo_iter == auth_fifo.end())
                    auth_fifo_iter = auth_fifo.begin();

                RFIFO_WFIFO_CLONE(ms, ms, 44);
                // overwrite
                WFIFOW(ms, 0) = 0x2b06;
                auth_fifo_iter->account_id = wrap<AccountId>(RFIFOL(ms, 2));
                auth_fifo_iter->char_id = wrap<CharId>(RFIFOL(ms, 14));
                auth_fifo_iter->login_id1 = RFIFOL(ms, 6);
                auth_fifo_iter->login_id2 = RFIFOL(ms, 10);
                auth_fifo_iter->delflag = 0;
                auth_fifo_iter->sex = static_cast<SEX>(RFIFOB(ms, 44));
                auth_fifo_iter->connect_until_time = TimeT();    // unlimited/unknown time by default (not display in map-server)
                auth_fifo_iter->ip = RFIFOIP(ms, 45);

                // default, if not found in the loop
                WFIFOW(ms, 6) = 1;
                for (const CharPair& cd : char_keys)
                {
                    AccountId aid = wrap<AccountId>(RFIFOL(ms, 2));
                    CharId cid = wrap<CharId>(RFIFOL(ms, 14));
                    if (cd.key.account_id == aid &&
                        cd.key.char_id == cid)
                    {
                        auth_fifo_iter++;
                        WFIFOL(ms, 6) = 0;
                        break;
                    }
                }
                WFIFOSET(ms, 44);
                RFIFOSKIP(ms, 49);
                break;

                // it is a request to become GM
            case 0x2b0a:
                if (RFIFOREST(ms) < 4 || RFIFOREST(ms) < RFIFOW(ms, 2))
                    return;
            {
                int account_id = RFIFOL(ms, 4);
                if (login_session)
                {               // don't send request if no login-server
                    size_t len = RFIFOW(ms, 2);
                    RFIFO_WFIFO_CLONE(ms, login_session, len);
                    WFIFOW(login_session, 0) = 0x2720;
                    WFIFOSET(login_session, len);
                }
                else
                {
                    WFIFOW(ms, 0) = 0x2b0b;
                    WFIFOL(ms, 2) = account_id;
                    WFIFOL(ms, 6) = 0;
                    WFIFOSET(ms, 10);
                }
            }
                RFIFOSKIP(ms, RFIFOW(ms, 2));
                break;

                // Map server send information to change an email of an account -> login-server
            case 0x2b0c:
                if (RFIFOREST(ms) < 86)
                    return;
                if (login_session)
                {               // don't send request if no login-server
                    RFIFO_WFIFO_CLONE(ms, login_session, 86); // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
                    WFIFOW(login_session, 0) = 0x2722;
                    WFIFOSET(login_session, 86);
                }
                RFIFOSKIP(ms, 86);
                break;

                // Map server ask char-server about a character name to do some operations (all operations are transmitted to login-server)
            case 0x2b0e:
                if (RFIFOREST(ms) < 44)
                    return;
                {
                    AccountId acc = wrap<AccountId>(RFIFOL(ms, 2));  // account_id of who ask (-1 if nobody)
                    CharName character_name = stringish<CharName>(RFIFO_STRING<24>(ms, 6));
                    int operation = RFIFOW(ms, 30);
                    // prepare answer
                    WFIFOW(ms, 0) = 0x2b0f;    // answer
                    WFIFOL(ms, 2) = unwrap<AccountId>(acc);   // who want do operation
                    WFIFOW(ms, 30) = operation;  // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex
                    // search character
                    const CharPair *cd = search_character(character_name);
                    if (cd)
                    {
                        const CharKey *ck = &cd->key;
                        WFIFO_STRING(ms, 6, ck->name.to__actual(), 24); // put correct name if found
                        WFIFOW(ms, 32) = 0;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                        switch (RFIFOW(ms, 30))
                        {
                            case 1:    // block
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_session, 0) = 0x2724;
                                        WFIFOL(login_session, 2) = unwrap<AccountId>(ck->account_id);  // account value
                                        WFIFOL(login_session, 6) = 5;   // status of the account
                                        WFIFOSET(login_session, 10);
                                    }
                                    else
                                        WFIFOW(ms, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(ms, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 2:    // ban
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_session, 0) = 0x2725;
                                        WFIFOL(login_session, 2) = unwrap<AccountId>(ck->account_id);  // account value
                                        HumanTimeDiff ban_change;
                                        RFIFO_STRUCT(ms, 32, ban_change);
                                        WFIFO_STRUCT(login_session, 6, ban_change);
                                        WFIFOSET(login_session, 18);
                                    }
                                    else
                                        WFIFOW(ms, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(ms, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 3:    // unblock
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_session, 0) = 0x2724;
                                        WFIFOL(login_session, 2) = unwrap<AccountId>(ck->account_id);  // account value
                                        WFIFOL(login_session, 6) = 0;   // status of the account
                                        WFIFOSET(login_session, 10);
                                    }
                                    else
                                        WFIFOW(ms, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(ms, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 4:    // unban
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_session, 0) = 0x272a;
                                        WFIFOL(login_session, 2) = unwrap<AccountId>(ck->account_id);  // account value
                                        WFIFOSET(login_session, 6);
                                    }
                                    else
                                        WFIFOW(ms, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(ms, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                            case 5:    // changesex
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        WFIFOW(login_session, 0) = 0x2727;
                                        WFIFOL(login_session, 2) = unwrap<AccountId>(ck->account_id);  // account value
                                        WFIFOSET(login_session, 6);
                                    }
                                    else
                                        WFIFOW(ms, 32) = 3;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                }
                                else
                                    WFIFOW(ms, 32) = 2;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                                break;
                        }
                    }
                    else
                    {
                        // character name not found
                        WFIFO_STRING(ms, 6, character_name.to__actual(), 24);
                        WFIFOW(ms, 32) = 1;    // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
                    }
                    // send answer if a player ask, not if the server ask
                    if (acc)
                    {
                        WFIFOSET(ms, 34);
                    }
                    RFIFOSKIP(ms, 44);
                    break;
                }

//      case 0x2b0f: not more used (available for futur usage)

                // account_reg保存要求
            case 0x2b10:
                if (RFIFOREST(ms) < 4 || RFIFOREST(ms) < RFIFOW(ms, 2))
                    return;
                {
                    Array<struct global_reg, ACCOUNT_REG2_NUM> reg;
                    int p, j;
                    AccountId acc = wrap<AccountId>(RFIFOL(ms, 4));
                    for (p = 8, j = 0;
                         p < RFIFOW(ms, 2) && j < ACCOUNT_REG2_NUM;
                         p += 36, j++)
                    {
                        reg[j].str = stringish<VarName>(RFIFO_STRING<32>(ms, p));
                        reg[j].value = RFIFOL(ms, p + 32);
                    }
                    set_account_reg2(acc, Slice<struct global_reg>(reg.begin(), j));
                    // loginサーバーへ送る
                    if (login_session)
                    {
                        // don't send request if no login-server
                        RFIFO_WFIFO_CLONE(ms, login_session, RFIFOW(ms, 2));
                        WFIFOW(login_session, 0) = 0x2728;
                        WFIFOSET(login_session, WFIFOW(login_session, 2));
                    }
                    RFIFOSKIP(ms, RFIFOW(ms, 2));
                    break;
                }

                // Map server is requesting a divorce
            case 0x2b16:
                if (RFIFOREST(ms) < 4)
                    return;
                {
                    CharId cid = wrap<CharId>(RFIFOL(ms, 2));
                    for (CharPair& cd : char_keys)
                        if (cd.key.char_id == cid)
                        {
                            char_divorce(&cd);
                            break;
                        }

                    RFIFOSKIP(ms, 6);
                    break;

                }

            default:
                // inter server処理に渡す
            {
                int r = inter_parse_frommap(ms);
                if (r == 1)     // 処理できた
                    break;
                if (r == 2)     // パケット長が足りない
                    return;
            }
                // inter server処理でもない場合は切断
                PRINTF("char: unknown packet 0x%04x (%zu bytes to read in buffer)! (from map).\n"_fmt,
                        RFIFOW(ms, 0), RFIFOREST(ms));
                ms->set_eof();
                return;
        }
    }
}

static
int search_mapserver(XString map)
{
    for (int i = 0; i < MAX_MAP_SERVERS; i++)
    {
        if (server_session[i])
        {
            for (int j = 0; server[i].maps[j]; j++)
            {
                if (server[i].maps[j] == map)
                    return i;
            }
        }
    }

    return -1;
}

//-----------------------------------------------------
// Test to know if an IP come from LAN or WAN. by [Yor]
//-----------------------------------------------------
static
int lan_ip_check(IP4Address addr)
{
    bool lancheck = lan_subnet.covers(addr);

    PRINTF("LAN test (result): %s.\n"_fmt,
            (lancheck) ? SGR_BOLD SGR_CYAN "LAN source" SGR_RESET ""_s : SGR_BOLD SGR_GREEN "WAN source" SGR_RESET ""_s);
    return lancheck;
}

static
void handle_x0066(Session *s, struct char_session_data *sd, uint8_t rfifob_2, IP4Address ip)
{
    {
        CharPair *cp = nullptr;
        for (CharPair& cdi : char_keys)
        {
            if (cdi.key.account_id == sd->account_id && cdi.key.char_num == rfifob_2)
            {
                cp = &cdi;
                break;
            }
        }
        if (cp)
        {
            CharKey *ck = &cp->key;
            CharData *cd = cp->data.get();

            CHAR_LOG("Character Selected, Account ID: %d, Character Slot: %d, Character Name: %s [%s]\n"_fmt,
                    sd->account_id, rfifob_2,
                    ck->name, ip);
            // searching map server
            int i = search_mapserver(cd->last_point.map_);
            // if map is not found, we check major cities
            if (i < 0)
            {
                int j;
                // get first online server (with a map)
                i = 0;
                for (j = 0; j < MAX_MAP_SERVERS; j++)
                    if (server_session[j]
                        && server[j].maps[0])
                    {   // change save point to one of map found on the server (the first)
                        i = j;
                        cd->last_point.map_ = server[j].maps[0];
                        PRINTF("Map-server #%d found with a map: '%s'.\n"_fmt,
                                j, server[j].maps[0]);
                        // coordonates are unknown
                        break;
                    }
                // if no map-server is connected, we send: server closed
                if (j == MAX_MAP_SERVERS)
                {
                    WFIFOW(s, 0) = 0x81;
                    WFIFOB(s, 2) = 1; // 01 = Server closed
                    WFIFOSET(s, 3);
                    return;
                }
            }
            WFIFOW(s, 0) = 0x71;
            WFIFOL(s, 2) = unwrap<CharId>(ck->char_id);
            WFIFO_STRING(s, 6, cd->last_point.map_, 16);
            PRINTF("Character selection '%s' (account: %d, slot: %d) [%s]\n"_fmt,
                    ck->name,
                    sd->account_id, ck->char_num, ip);
            PRINTF("--Send IP of map-server. "_fmt);
            if (lan_ip_check(ip))
                WFIFOIP(s, 22) = lan_map_ip;
            else
                WFIFOIP(s, 22) = server[i].ip;
            WFIFOW(s, 26) = server[i].port;
            WFIFOSET(s, 28);
            if (auth_fifo_iter == auth_fifo.end())
                auth_fifo_iter = auth_fifo.begin();
            auth_fifo_iter->account_id = sd->account_id;
            auth_fifo_iter->char_id = ck->char_id;
            auth_fifo_iter->login_id1 = sd->login_id1;
            auth_fifo_iter->login_id2 = sd->login_id2;
            auth_fifo_iter->delflag = 0;
            auth_fifo_iter->sex = sd->sex;
            auth_fifo_iter->connect_until_time = sd->connect_until_time;
            auth_fifo_iter->ip = s->client_ip;
            auth_fifo_iter->packet_tmw_version = sd->packet_tmw_version;
            auth_fifo_iter++;
        }
    }
}

static
void parse_char(Session *s)
{
    IP4Address ip = s->client_ip;

    if (!login_session)
    {
        s->set_eof();

        // I sure *hope* this doesn't happen ...
        if (s == login_session)
            login_session = nullptr;
        return;
    }

    char_session_data *sd = static_cast<char_session_data *>(s->session_data.get());

    while (RFIFOREST(s) >= 2)
    {
        switch (RFIFOW(s, 0))
        {
            case 0x20b:        //20040622暗号化ragexe対応
                if (RFIFOREST(s) < 19)
                    return;
                RFIFOSKIP(s, 19);
                break;

            case 0x61:         // change password request
                if (RFIFOREST(s) < 50)
                    return;
                {
                    WFIFOW(login_session, 0) = 0x2740;
                    WFIFOL(login_session, 2) = unwrap<AccountId>(sd->account_id);
                    AccountPass old_pass = stringish<AccountPass>(RFIFO_STRING<24>(s, 2));
                    WFIFO_STRING(login_session, 6, old_pass, 24);
                    AccountPass new_pass = stringish<AccountPass>(RFIFO_STRING<24>(s, 26));
                    WFIFO_STRING(login_session, 30, new_pass, 24);
                    WFIFOSET(login_session, 54);
                }
                RFIFOSKIP(s, 50);
                break;

            case 0x65:         // 接続要求
                if (RFIFOREST(s) < 17)
                    return;
                {
                    AccountId account_id = wrap<AccountId>(RFIFOL(s, 2));
                    GmLevel GM_value = isGM(account_id);
                    if (GM_value)
                        PRINTF("Account Logged On; Account ID: %d (GM level %d).\n"_fmt,
                                account_id, GM_value);
                    else
                        PRINTF("Account Logged On; Account ID: %d.\n"_fmt,
                                account_id);
                    if (sd == NULL)
                    {
                        s->session_data = make_unique<char_session_data, SessionDeleter>();
                        sd = static_cast<char_session_data *>(s->session_data.get());
                        sd->email = stringish<AccountEmail>("no mail"_s);  // put here a mail without '@' to refuse deletion if we don't receive the e-mail
                        sd->connect_until_time = TimeT(); // unknow or illimited (not displaying on map-server)
                    }
                    sd->account_id = account_id;
                    sd->login_id1 = RFIFOL(s, 6);
                    sd->login_id2 = RFIFOL(s, 10);
                    sd->packet_tmw_version = RFIFOW(s, 14);
                    sd->sex = static_cast<SEX>(RFIFOB(s, 16));
                    // send back account_id
                    WFIFOL(s, 0) = unwrap<AccountId>(account_id);
                    WFIFOSET(s, 4);
                    // search authentification
                    for (AuthFifoEntry& afi : auth_fifo)
                    {
                        if (afi.account_id == sd->account_id
                            && afi.login_id1 == sd->login_id1
                            && afi.login_id2 == sd->login_id2
                            && (!check_ip_flag
                                || afi.ip == s->client_ip)
                            && afi.delflag == 2)
                        {
                            afi.delflag = 1;
                            if (max_connect_user == 0
                                || count_users() < max_connect_user)
                            {
                                if (login_session)
                                {   // don't send request if no login-server
                                    // request to login-server to obtain e-mail/time limit
                                    WFIFOW(login_session, 0) = 0x2716;
                                    WFIFOL(login_session, 2) = unwrap<AccountId>(sd->account_id);
                                    WFIFOSET(login_session, 6);
                                }
                                // Record client version
                                afi.packet_tmw_version =
                                    sd->packet_tmw_version;
                                // send characters to player
                                mmo_char_send006b(s, sd);
                            }
                            else
                            {
                                // refuse connection (over populated)
                                WFIFOW(s, 0) = 0x6c;
                                WFIFOB(s, 2) = 0;
                                WFIFOSET(s, 3);
                            }
                            goto x65_out;
                        }
                    }
                    // authentification not found
                    {
                        if (login_session)
                        {
                            // don't send request if no login-server
                            WFIFOW(login_session, 0) = 0x2712;  // ask login-server to authentify an account
                            WFIFOL(login_session, 2) = unwrap<AccountId>(sd->account_id);
                            WFIFOL(login_session, 6) = sd->login_id1;
                            WFIFOL(login_session, 10) = sd->login_id2;  // relate to the versions higher than 18
                            WFIFOB(login_session, 14) = static_cast<uint8_t>(sd->sex);
                            WFIFOIP(login_session, 15) = s->client_ip;
                            WFIFOSET(login_session, 19);
                        }
                        else
                        {       // if no login-server, we must refuse connection
                            WFIFOW(s, 0) = 0x6c;
                            WFIFOB(s, 2) = 0;
                            WFIFOSET(s, 3);
                        }
                    }
                }
            x65_out:
                RFIFOSKIP(s, 17);
                break;

            case 0x66:         // キャラ選択
                if (!sd || RFIFOREST(s) < 3)
                    return;
                handle_x0066(s, sd, RFIFOB(s, 2), ip);
                RFIFOSKIP(s, 3);
                break;

            case 0x67:         // 作成
                if (!sd || RFIFOREST(s) < 37)
                    return;
            {
                CharName name = stringish<CharName>(RFIFO_STRING<24>(s, 2));
                uint8_t stats[6];
                for (int i = 0; i < 6; ++i)
                    stats[i] = RFIFOB(s, 26 + i);
                uint8_t slot = RFIFOB(s, 32);
                uint16_t hair_color = RFIFOW(s, 33);
                uint16_t hair_style = RFIFOW(s, 35);
                const CharPair *cp = make_new_char(s, name, stats, slot, hair_color, hair_style);
                if (!cp)
                {
                    WFIFOW(s, 0) = 0x6e;
                    WFIFOB(s, 2) = 0x00;
                    WFIFOSET(s, 3);
                    RFIFOSKIP(s, 37);
                    break;
                }
                const CharKey *ck = &cp->key;
                const CharData *cd = cp->data.get();

                WFIFOW(s, 0) = 0x6d;
                WFIFO_ZERO(s, 2, 106);

                WFIFOL(s, 2) = unwrap<CharId>(ck->char_id);
                WFIFOL(s, 2 + 4) = cd->base_exp;
                WFIFOL(s, 2 + 8) = cd->zeny;
                WFIFOL(s, 2 + 12) = cd->job_exp;
                WFIFOL(s, 2 + 16) = cd->job_level;

                WFIFOL(s, 2 + 28) = cd->karma;
                WFIFOL(s, 2 + 32) = cd->manner;

                WFIFOW(s, 2 + 40) = 0x30;
                WFIFOW(s, 2 + 42) = saturate<int16_t>(cd->hp);
                WFIFOW(s, 2 + 44) = saturate<int16_t>(cd->max_hp);
                WFIFOW(s, 2 + 46) = saturate<int16_t>(cd->sp);
                WFIFOW(s, 2 + 48) = saturate<int16_t>(cd->max_sp);
                WFIFOW(s, 2 + 50) = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // cd->speed;
                WFIFOW(s, 2 + 52) = unwrap<Species>(cd->species);
                WFIFOW(s, 2 + 54) = cd->hair;

                WFIFOW(s, 2 + 58) = cd->base_level;
                WFIFOW(s, 2 + 60) = cd->skill_point;

                WFIFOW(s, 2 + 64) = unwrap<ItemNameId>(cd->shield);
                WFIFOW(s, 2 + 66) = unwrap<ItemNameId>(cd->head_top);
                WFIFOW(s, 2 + 68) = unwrap<ItemNameId>(cd->head_mid);
                WFIFOW(s, 2 + 70) = cd->hair_color;

                WFIFO_STRING(s, 2 + 74, ck->name.to__actual(), 24);

                WFIFOB(s, 2 + 98) = saturate<uint8_t>(cd->attrs[ATTR::STR]);
                WFIFOB(s, 2 + 99) = saturate<uint8_t>(cd->attrs[ATTR::AGI]);
                WFIFOB(s, 2 + 100) = saturate<uint8_t>(cd->attrs[ATTR::VIT]);
                WFIFOB(s, 2 + 101) = saturate<uint8_t>(cd->attrs[ATTR::INT]);
                WFIFOB(s, 2 + 102) = saturate<uint8_t>(cd->attrs[ATTR::DEX]);
                WFIFOB(s, 2 + 103) = saturate<uint8_t>(cd->attrs[ATTR::LUK]);
                WFIFOB(s, 2 + 104) = ck->char_num;

                WFIFOSET(s, 108);
            }
                RFIFOSKIP(s, 37);
                break;

            case 0x68:         // delete char //Yor's Fix
                if (!sd || RFIFOREST(s) < 46)
                    return;
            {
                AccountEmail email = stringish<AccountEmail>(RFIFO_STRING<40>(s, 6));
                if (!e_mail_check(email))
                    email = DEFAULT_EMAIL;

                {
                    {
                        CharId cid = wrap<CharId>(RFIFOL(s, 2));
                        CharPair *cs = nullptr;
                        for (CharPair& cd : char_keys)
                        {
                            if (cd.key.char_id == cid)
                            {
                                cs = &cd;
                                break;
                            }
                        }

                        if (cs)
                        {
                            char_delete(cs);   // deletion process

                            if (cs != &char_keys.back())
                            {
                                std::swap(*cs, char_keys.back());
                            }

                            char_keys.pop_back();
                            WFIFOW(s, 0) = 0x6f;
                            WFIFOSET(s, 2);
                            goto x68_out;
                        }
                    }

                    {
                        WFIFOW(s, 0) = 0x70;
                        WFIFOB(s, 2) = 0;
                        WFIFOSET(s, 3);
                    }
                }
            }
            x68_out:
                RFIFOSKIP(s, 46);
                break;

            case 0x2af8:       // マップサーバーログイン
                if (RFIFOREST(s) < 60)
                    return;
            {
                int i;
                WFIFOW(s, 0) = 0x2af9;
                for (i = 0; i < MAX_MAP_SERVERS; i++)
                {
                    if (!server_session[i])
                        break;
                }
                AccountName userid_ = stringish<AccountName>(RFIFO_STRING<24>(s, 2));
                AccountPass passwd_ = stringish<AccountPass>(RFIFO_STRING<24>(s, 26));
                if (i == MAX_MAP_SERVERS || userid_ != userid
                    || passwd_ != passwd)
                {
                    WFIFOB(s, 2) = 3;
                    WFIFOSET(s, 3);
                    RFIFOSKIP(s, 60);
                }
                else
                {
                    int len;
                    WFIFOB(s, 2) = 0;
                    s->set_parsers(SessionParsers{.func_parse= parse_frommap, .func_delete= delete_frommap});
                    server_session[i] = s;
                    if (anti_freeze_enable)
                        server_freezeflag[i] = 5;   // Map anti-freeze system. Counter. 5 ok, 4...0 freezed
                    // ignore RFIFOL(fd, 50)
                    server[i].ip = RFIFOIP(s, 54);
                    server[i].port = RFIFOW(s, 58);
                    server[i].users = 0;
                    for (MapName& mapi : server[i].maps)
                        mapi = MapName();
                    WFIFOSET(s, 3);
                    RFIFOSKIP(s, 60);
                    realloc_fifo(s, FIFOSIZE_SERVERLINK,
                                  FIFOSIZE_SERVERLINK);
                    // send gm acccounts level to map-servers
                    len = 4;
                    WFIFOW(s, 0) = 0x2b15;
                    for (const GM_Account& gma : gm_accounts)
                    {
                        WFIFOL(s, len) = unwrap<AccountId>(gma.account_id);
                        WFIFOB(s, len + 4) = gma.level.get_all_bits();
                        len += 5;
                    }
                    WFIFOW(s, 2) = len;
                    WFIFOSET(s, len);
                    return;
                }
            }
                break;

            case 0x187:        // Alive信号？
                if (RFIFOREST(s) < 6)
                    return;
                RFIFOSKIP(s, 6);
                break;

            case 0x7530:       // Athena情報所得
                WFIFOW(s, 0) = 0x7531;
                WFIFO_STRUCT(s, 2, CURRENT_CHAR_SERVER_VERSION);
                WFIFOSET(s, 10);
                RFIFOSKIP(s, 2);
                return;

            case 0x7532:       // 接続の切断(defaultと処理は一緒だが明示的にするため)
                s->set_eof();
                return;

            default:
                s->set_eof();
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
        Session *s = server_session[i];
        if (s)
        {
            WFIFO_BUF_CLONE(s, buf, len);
            WFIFOSET(s, len);
            c++;
        }
    }
    return c;
}

// 自分以外の全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendallwos(Session *ss, const uint8_t *buf, unsigned int len)
{
    int i, c;

    c = 0;
    for (i = 0; i < MAX_MAP_SERVERS; i++)
    {
        Session *s = server_session[i];
        if (s && s != ss)
        {
            WFIFO_BUF_CLONE(s, buf, len);
            WFIFOSET(s, len);
            c++;
        }
    }
    return c;
}

// MAPサーバーにデータ送信（map鯖生存確認有り）
int mapif_send(Session *s, const uint8_t *buf, unsigned int len)
{
    int i;

    if (s)
    {
        for (i = 0; i < MAX_MAP_SERVERS; i++)
        {
            if (s == server_session[i])
            {
                WFIFO_BUF_CLONE(s, buf, len);
                WFIFOSET(s, len);
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

    if (login_session)
    {
        // send number of user to login server
        WFIFOW(login_session, 0) = 0x2714;
        WFIFOL(login_session, 2) = users;
        WFIFOSET(login_session, 6);
    }
    // send number of players to all map-servers
    WBUFW(buf, 0) = 0x2b00;
    WBUFL(buf, 2) = users;
    mapif_sendall(buf, 6);
}

static
void check_connect_login_server(TimerData *, tick_t)
{
    if (!login_session)
    {
        PRINTF("Attempt to connect to login-server...\n"_fmt);
        login_session = make_connection(login_ip, login_port,
                SessionParsers{.func_parse= parse_tologin, .func_delete= delete_tologin});
        if (!login_session)
            return;
        realloc_fifo(login_session, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);
        WFIFOW(login_session, 0) = 0x2710;
        WFIFO_ZERO(login_session, 2, 24);
        WFIFO_STRING(login_session, 2, userid, 24);
        WFIFO_STRING(login_session, 26, passwd, 24);
        WFIFOL(login_session, 50) = 0;
        WFIFOIP(login_session, 54) = char_ip;
        WFIFOL(login_session, 58) = char_port;
        WFIFO_STRING(login_session, 60, server_name, 20);
        WFIFOW(login_session, 80) = 0;
        WFIFOW(login_session, 82) = 0; //char_maintenance;
        WFIFOW(login_session, 84) = 0; //char_new;
        WFIFOSET(login_session, 86);
    }
}

//-------------------------------------------
// Reading Lan Support configuration by [Yor]
//-------------------------------------------
static
bool char_lan_config(XString w1, ZString w2)
{
    struct hostent *h = NULL;

    {
        if (w1 == "lan_map_ip"_s)
        {
            // Read map-server Lan IP Address
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                lan_map_ip = IP4Address({
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
            PRINTF("LAN IP of map-server: %s.\n"_fmt, lan_map_ip);
        }
        else if (w1 == "subnet"_s /*backward compatibility*/
                || w1 == "lan_subnet"_s)
        {
            if (!extract(w2, &lan_subnet))
            {
                PRINTF("Bad IP mask: %s\n"_fmt, w2);
                return false;
            }
            PRINTF("Sub-network of the map-server: %s.\n"_fmt,
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
    // sub-network check of the map-server
    {
        PRINTF("LAN test of LAN IP of the map-server: "_fmt);
        if (!lan_ip_check(lan_map_ip))
        {
            PRINTF(SGR_BOLD SGR_RED "***ERROR: LAN IP of the map-server doesn't belong to the specified Sub-network." SGR_RESET "\n"_fmt);
            return false;
        }
    }

    return true;
}

static
bool char_config(XString w1, ZString w2)
{
    struct hostent *h = NULL;

    {
        if (w1 == "userid"_s)
            userid = stringish<AccountName>(w2);
        else if (w1 == "passwd"_s)
            passwd = stringish<AccountPass>(w2);
        else if (w1 == "server_name"_s)
        {
            server_name = stringish<ServerName>(w2);
            PRINTF("%s server has been intialized\n"_fmt, w2);
        }
        else if (w1 == "wisp_server_name"_s)
        {
            if (w2.size() >= 4)
                wisp_server_name = stringish<CharName>(w2);
        }
        else if (w1 == "login_ip"_s)
        {
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                login_ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
                PRINTF("Login server IP address : %s -> %s\n"_fmt,
                        w2, login_ip);
            }
            else
            {
                PRINTF("Bad IP value: %s\n"_fmt, w2);
                return false;
            }
        }
        else if (w1 == "login_port"_s)
        {
            login_port = atoi(w2.c_str());
        }
        else if (w1 == "char_ip"_s)
        {
            h = gethostbyname(w2.c_str());
            if (h != NULL)
            {
                char_ip = IP4Address({
                        static_cast<uint8_t>(h->h_addr[0]),
                        static_cast<uint8_t>(h->h_addr[1]),
                        static_cast<uint8_t>(h->h_addr[2]),
                        static_cast<uint8_t>(h->h_addr[3]),
                });
                PRINTF("Character server IP address : %s -> %s\n"_fmt,
                        w2, char_ip);
            }
            else
            {
                PRINTF("Bad IP value: %s\n"_fmt, w2);
                return false;
            }
        }
        else if (w1 == "char_port"_s)
        {
            char_port = atoi(w2.c_str());
        }
        else if (w1 == "char_txt"_s)
        {
            char_txt = w2;
        }
        else if (w1 == "max_connect_user"_s)
        {
            max_connect_user = atoi(w2.c_str());
            if (max_connect_user < 0)
                max_connect_user = 0;   // unlimited online players
        }
        else if (w1 == "check_ip_flag"_s)
        {
            check_ip_flag = config_switch(w2);
        }
        else if (w1 == "autosave_time"_s)
        {
            autosave_time = std::chrono::seconds(atoi(w2.c_str()));
            if (autosave_time <= std::chrono::seconds::zero())
                autosave_time = DEFAULT_AUTOSAVE_INTERVAL;
        }
        else if (w1 == "start_point"_s)
        {
            extract(w2, &start_point);
        }
        else if (w1 == "unknown_char_name"_s)
        {
            unknown_char_name = stringish<CharName>(w2);
        }
        else if (w1 == "char_log_filename"_s)
        {
            char_log_filename = w2;
        }
        else if (w1 == "char_name_option"_s)
        {
            char_name_option = atoi(w2.c_str());
        }
        else if (w1 == "char_name_letters"_s)
        {
            if (!w2)
                char_name_letters.reset();
            else
                for (uint8_t c : w2)
                    char_name_letters[c] = true;
        }
        else if (w1 == "online_txt_filename"_s)
        {
            online_txt_filename = w2;
        }
        else if (w1 == "online_html_filename"_s)
        {
            online_html_filename = w2;
        }
        else if (w1 == "online_sorting_option"_s)
        {
            online_sorting_option = atoi(w2.c_str());
        }
        else if (w1 == "online_gm_display_min_level"_s)
        {
            // minimum GM level to display 'GM' when we want to display it
            return extract(w2, &online_gm_display_min_level);
        }
        else if (w1 == "online_refresh_html"_s)
        {
            online_refresh_html = atoi(w2.c_str());
            if (online_refresh_html < 1)
                online_refresh_html = 1;
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
        else
        {
            return false;
        }
    }

    return true;
}

void term_func(void)
{
    // write online players files with no player
    std::fill(online_chars.begin(), online_chars.end(), nullptr);
    create_online_files();
    online_chars.clear();

    mmo_char_sync();
    inter_save();

    gm_accounts.clear();

    char_keys.clear();
    delete_session(login_session);
    delete_session(char_session);

    CHAR_LOG("----End of char-server (normal end with closing of all files).\n"_fmt);
}

static
bool char_confs(XString key, ZString value)
{
    unsigned sum = 0;
    sum += char_config(key, value);
    sum += char_lan_config(key, value);
    sum += inter_config(key, value);
    if (sum >= 2)
        abort();
    return sum;
}

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
            runflag &= load_config_file(argvi, char_confs);
        }
    }

    if (!loaded_config_yet)
        runflag &= load_config_file("conf/tmwa-char.conf"_s, char_confs);

    // a newline in the log...
    CHAR_LOG(""_fmt);
    CHAR_LOG("The char-server starting...\n"_fmt);

    runflag &= lan_check();

    mmo_char_init();
    inter_init2();

    update_online = TimeT::now();
    create_online_files();     // update online players files at start of the server

    char_session = make_listen_port(char_port, SessionParsers{parse_char, delete_char});

    Timer(gettick() + std::chrono::seconds(1),
            check_connect_login_server,
            std::chrono::seconds(10)
    ).detach();
    Timer(gettick() + std::chrono::seconds(1),
            send_users_tologin,
            std::chrono::seconds(5)
    ).detach();
    Timer(gettick() + autosave_time,
            mmo_char_sync_timer,
            autosave_time
    ).detach();

    if (anti_freeze_enable > 0)
    {
        Timer(gettick() + std::chrono::seconds(1),
                map_anti_freeze_system,
                anti_freeze_interval
        ).detach();
    }

    CHAR_LOG("The char-server is ready (Server is listening on the port %d).\n"_fmt,
            char_port);

    PRINTF("The char-server is " SGR_BOLD SGR_GREEN "ready" SGR_RESET " (Server is listening on the port %d).\n\n"_fmt,
            char_port);

    return 0;
}
