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
#include "../io/extract.hpp"
#include "../io/lock.hpp"
#include "../io/read.hpp"
#include "../io/span.hpp"
#include "../io/tty.hpp"
#include "../io/write.hpp"

#include "../net/socket.hpp"
#include "../net/timer.hpp"

#include "../proto2/any-user.hpp"
#include "../proto2/login-admin.hpp"
#include "../proto2/login-char.hpp"
#include "../proto2/login-user.hpp"
#include "../proto2/char-map.hpp"
#include "../proto2/char-user.hpp"

#include "../mmo/config_parse.hpp"
#include "../mmo/cxxstdio_enums.hpp"
#include "../mmo/extract_enums.hpp"
#include "../mmo/human_time_diff.hpp"
#include "../mmo/version.hpp"

#include "../high/core.hpp"
#include "../high/extract_mmo.hpp"
#include "../high/mmo.hpp"
#include "../high/utils.hpp"

#include "../wire/packets.hpp"

#include "char_conf.hpp"
#include "char_lan_conf.hpp"
#include "globals.hpp"
#include "inter.hpp"
#include "inter_conf.hpp"
#include "int_party.hpp"
#include "int_storage.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace char_
{
struct char_session_data : SessionData
{
    AccountId account_id;
    int login_id1, login_id2;
    SEX sex;
    unsigned short packet_client_version;
    AccountEmail email;
};
} // namespace char_

void SessionDeleter::operator()(SessionData *sd)
{
    really_delete1 static_cast<char_::char_session_data *>(sd);
}

namespace char_
{

auto iter_map_sessions() -> decltype(filter_iterator<Session *>(std::declval<Array<Session *, MAX_MAP_SERVERS> *>()))
{
    return filter_iterator<Session *>(&server_session);
}



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
    io::AppendFile logfp(char_conf.char_log_filename, true);
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
        p->last_point = char_conf.start_point;
    }
    if (p->sex == SEX::UNSPECIFIED)
    {
        for (AuthFifoEntry& afi : auth_fifo)
        {
            if (afi.account_id == k->account_id)
            {
                p->sex = afi.sex;
            }
        }
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
            "%s,%d,%d,%d\t"
            "%c\t"_fmt,
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
            p->save_point.map_, p->save_point.x, p->save_point.y, p->partner_id,
            sex_to_char(p->sex));

    for (IOff0 i : IOff0::iter())
    {
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
    }
    str_p += '\t';

    // cart was here (no longer supported)
    str_p += '\t';

    for (SkillID i : erange(SkillID(), MAX_SKILL))
    {
        if (p->skill[i].lv)
        {
            str_p += STRPRINTF("%d,%d "_fmt,
                    i,
                    p->skill[i].lv | (static_cast<uint16_t>(p->skill[i].flags) << 16));
        }
    }
    str_p += '\t';

    assert (p->global_reg_num < GLOBAL_REG_NUM);
    for (int i = 0; i < p->global_reg_num; i++)
    {
        if (p->global_reg[i].str)
        {
            str_p += STRPRINTF("%s,%d "_fmt,
                    p->global_reg[i].str,
                    p->global_reg[i].value);
        }
     }
    str_p += '\t';

    return AString(str_p);
}

struct skill_loader
{
    SkillID id;
    uint16_t level;
    SkillFlags flags;
};

static
bool impl_extract(XString str, struct skill_loader *s)
{
    uint32_t flags_and_level;
    if (!extract(str,
                record<','>(&s->id, &flags_and_level)))
        return false;
    s->level = flags_and_level & 0xffff;
    s->flags = SkillFlags(flags_and_level >> 16);
    return true;
}
} // namespace char

//-------------------------------------------------------------------------
// Function to set the character from the line (at read of characters file)
//-------------------------------------------------------------------------
static
bool impl_extract(XString str, CharPair *cp)
{
    using namespace tmwa::char_;

    CharKey *k = &cp->key;
    CharData *p = cp->data.get();

    uint32_t unused_guild_id, unused_pet_id;
    VString<1> sex;
    XString unused_memos;
    std::vector<Item> inventory;
    XString unused_cart;
    std::vector<struct skill_loader> skills;
    std::vector<GlobalReg> vars;
    XString hair_style;
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
                    record<','>(&hair_style, &p->hair_color, &p->clothes_color),
                    record<','>(&p->weapon, &p->shield, &p->head_top, &p->head_mid, &p->head_bottom),
                    &p->last_point,
                    // somebody was silly and stuck partner id as a field
                    // of this, instead of adding a new \t
                    // or putting it elsewhere, like by pet/guild
                    record<','>(&p->save_point.map_, &p->save_point.x, &p->save_point.y, &p->partner_id),
                    &sex,
                    vrec<' '>(&inventory),
                    &unused_cart,
                    vrec<' '>(&skills),
                    vrec<' '>(&vars))))
        return false;

    if (sex.size() != 1)
        p->sex = SEX::UNSPECIFIED;
    else
        p->sex = sex_from_char(sex.front());
    // leftover corruption from Platinum
    if (hair_style == "-1"_s)
    {
        p->hair = 0;
    }
    else if (!extract(hair_style, &p->hair))
        return false;

    if (WISP_SERVER_NAME == k->name)
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

namespace char_
{
//---------------------------------
// Function to read characters file
//---------------------------------
static
int mmo_char_init(void)
{
    char_keys.clear();
    online_chars.clear();

    io::ReadFile in(char_conf.char_txt);
    if (!in.is_open())
    {
        PRINTF("Characters file not found: %s.\n"_fmt, char_conf.char_txt);
        CHAR_LOG("Characters file not found: %s.\n"_fmt, char_conf.char_txt);
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
            char_keys.size(), char_conf.char_txt);
    CHAR_LOG("mmo_char_init: %zu characters read in %s.\n"_fmt,
            char_keys.size(), char_conf.char_txt);

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
    io::WriteLock fp(char_conf.char_txt);
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
CharPair *make_new_char(Session *s, CharName name, const Stats6& stats, uint8_t slot, uint16_t hair_color, uint16_t hair_style, short& error_code)
{
    // ugh
    char_session_data *sd = static_cast<char_session_data *>(s->session_data.get());

    // remove control characters from the name
    if (!name.to__actual().is_print())
    {
        CHAR_LOG("Make new char error (control char received in the name): (connection #%d, account: %d).\n"_fmt,
                s, sd->account_id);
        error_code = 0x02;
        return nullptr;
    }

    // Eliminate whitespace
    if (name.to__actual() != name.to__actual().strip())
    {
        CHAR_LOG("Make new char error (leading/trailing whitespace): (connection #%d, account: %d, name: '%s'.\n"_fmt,
                s, sd->account_id, name);
        error_code = 0x02;
        return nullptr;
    }

    // check lenght of character name
    if (name.to__actual().size() < char_conf.min_name_length)
    {
        CHAR_LOG("Make new char error (character name too small): (connection #%d, account: %d, name: '%s').\n"_fmt,
                s, sd->account_id, name);
        error_code = 0x02;
        return nullptr;
    }

    // Check Authorised letters/symbols in the name of the character
    {
        // only letters/symbols in char_name_letters are authorised
        for (uint8_t c : name.to__actual())
        {
            if (!char_conf.char_name_letters[c])
            {
                CHAR_LOG("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c.\n"_fmt,
                        s, sd->account_id, name, c);
                error_code = 0x02;
                return nullptr;
            }
        }
    }

    // TODO this comment is obsolete
    // this is why it needs to be unsigned
    if (stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk != char_conf.total_stat_sum)
    {
        CHAR_LOG("Make new char error (invalid stats): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d\n"_fmt,
                s, sd->account_id, slot, name,
                stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
                stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk);
        error_code = 0x03;
        return nullptr;
    }

    if (slot >= char_conf.char_slots)
    {
        CHAR_LOG("Make new char error (invalid slot): (connection #%d, account: %d) slot %d, name: %s\n"_fmt,
                s, sd->account_id, slot, name);
        error_code = 0x05;
        return nullptr;
    }

    if (hair_style > char_conf.max_hair_style || hair_color > char_conf.max_hair_color)
    {
        CHAR_LOG("Make new char error (invalid hair): (connection #%d, account: %d) slot %d, name: %s, hair: %d, hair color: %d\n"_fmt,
                s, sd->account_id, slot, name, hair_style, hair_color);
        error_code = 0x04;
        return nullptr;
    }

    // check individual stat value
    for (int i = 0; i < 6; i++)
    {
        uint8_t statsi = reinterpret_cast<const uint8_t *>(&stats)[i];
        if (statsi < char_conf.min_stat_value || statsi > char_conf.max_stat_value)
        {
            CHAR_LOG("Make new char error (invalid stat value: not between 1 to 9): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d\n"_fmt,
                    s, sd->account_id, slot, name,
                    stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
                    stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk,
                    hair_style, hair_color);
            error_code = 0x03;
            return nullptr;
        }
    }

    for (const CharPair& cd : char_keys)
    {
        if (cd.key.name == name)
        {
            CHAR_LOG("Make new char error (name already exists): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n"_fmt,
                    s, sd->account_id, slot, name, cd.key.name,
                    stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
                    stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk,
                    hair_style, hair_color);
            error_code = 0x01;
            return nullptr;
        }
        if (cd.key.account_id == sd->account_id
            && cd.key.char_num == slot)
        {
            CHAR_LOG("Make new char error (slot already used): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n"_fmt,
                    s, sd->account_id, slot, name, cd.key.name,
                    stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
                    stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk,
                    hair_style, hair_color);
            error_code = 0x05;
            return nullptr;
        }
    }

    if (WISP_SERVER_NAME == name)
    {
        CHAR_LOG("Make new char error (name used is wisp name for server): (connection #%d, account: %d) slot %d, name: %s (actual name whisper server: %s), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d.\n"_fmt,
                s, sd->account_id, slot, name, WISP_SERVER_NAME,
                stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
                stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk,
                hair_style, hair_color);
        error_code = 0x01;
        return nullptr;
    }

    IP4Address ip = s->client_ip;

    CHAR_LOG("Creation of New Character: (connection #%d, account: %d) slot %d, character Name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d. [%s]\n"_fmt,
            s, sd->account_id, slot, name,
            stats.str, stats.agi, stats.vit, stats.int_, stats.dex, stats.luk,
            stats.str + stats.agi + stats.vit + stats.int_ + stats.dex + stats.luk,
            hair_style, hair_color, ip);

    CharPair cp;
    CharKey& ck = cp.key;
    CharData& cd = *cp.data;

    ck.char_id = char_id_count; char_id_count = next(char_id_count);
    ck.account_id = sd->account_id;
    ck.char_num = slot;
    ck.name = name;
    cd.species = Species();
    cd.sex = SEX::NEUTRAL;
    cd.base_level = 1;
    cd.job_level = 1;
    cd.base_exp = 0;
    cd.job_exp = 0;
    cd.zeny = 0;
    cd.attrs[ATTR::STR] = stats.str;
    cd.attrs[ATTR::AGI] = stats.agi;
    cd.attrs[ATTR::VIT] = stats.vit;
    cd.attrs[ATTR::INT] = stats.int_;
    cd.attrs[ATTR::DEX] = stats.dex;
    cd.attrs[ATTR::LUK] = stats.luk;
    cd.max_hp = 40 * (100 + cd.attrs[ATTR::VIT]) / 100;
    cd.max_sp = 11 * (100 + cd.attrs[ATTR::INT]) / 100;
    cd.hp = cd.max_hp;
    cd.sp = cd.max_sp;
    cd.status_point = 0;
    cd.skill_point = 0;
    cd.option = static_cast<Opt0>(0x0000); // Opt0 is only declared
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
    cd.last_point = char_conf.start_point;
    cd.save_point = char_conf.start_point;
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
    io::WriteFile fp(char_conf.online_txt_filename);
    if (fp.is_open())
    {
        io::WriteFile fp2(char_conf.online_html_filename);
        if (fp2.is_open())
        {
            // get time
            timestamp_seconds_buffer timetemp;
            stamp_time(timetemp);
            // write heading
            FPRINTF(fp2, "<HTML>\n"_fmt);
            FPRINTF(fp2, "  <META http-equiv=\"Refresh\" content=\"%d\">\n"_fmt, char_conf.online_refresh_html); // update on client explorer every x seconds
            FPRINTF(fp2, "  <HEAD>\n"_fmt);
            FPRINTF(fp2, "    <TITLE>Online Players on %s</TITLE>\n"_fmt,
                    char_conf.server_name);
            FPRINTF(fp2, "  </HEAD>\n"_fmt);
            FPRINTF(fp2, "  <BODY>\n"_fmt);
            FPRINTF(fp2, "    <H3>Online Players on %s (%s):</H3>\n"_fmt,
                    char_conf.server_name, timetemp);
            FPRINTF(fp, "Online Players on %s (%s):\n"_fmt, char_conf.server_name, timetemp);
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
                            if (gml.satisfies(char_conf.online_gm_display_min_level))
                                FPRINTF(fp, "%-24s (GM) "_fmt, cd.key.name);
                            else
                                FPRINTF(fp, "%-24s      "_fmt, cd.key.name);
                        }
                        // name of the character in the html (no < >, because that create problem in html code)
                        FPRINTF(fp2, "        <td>"_fmt);
                        if (gml.satisfies(char_conf.online_gm_display_min_level))
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
                        if (gml.satisfies(char_conf.online_gm_display_min_level))
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
    for (IOff0 i : IOff0::iter())
    {
        if (p->inventory[i].nameid && p->inventory[i].amount
            && bool(p->inventory[i].equip & equipmask))
        {
            return p->inventory[i].nameid;
        }
    }
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

    Packet_Head<0x006b> head_6b;
    std::vector<Packet_Repeat<0x006b>> repeat_6b;

    head_6b.unused = {};

    for (int i = 0; i < found_num; i++)
    {
        const CharPair *cp = found_char[i];

        const CharKey *k = &cp->key;
        const CharData *p = cp->data.get();

        Packet_Repeat<0x006b> info;
        CharSelect& sel = info.char_select;

        sel.char_id = k->char_id;
        sel.base_exp = p->base_exp;
        sel.zeny = p->zeny;
        sel.job_exp = p->job_exp;
        sel.job_level = p->job_level;

        sel.shoes = find_equip_view(cp, EPOS::SHOES);
        sel.gloves = find_equip_view(cp, EPOS::GLOVES);
        sel.cape = find_equip_view(cp, EPOS::CAPE);
        sel.misc1 = find_equip_view(cp, EPOS::MISC1);
        sel.option = p->option;

        sel.karma = p->karma;
        sel.manner = p->manner;

        sel.status_point = p->status_point;
        sel.hp = std::min(p->hp, 0x7fff);
        sel.max_hp = std::min(p->max_hp, 0x7fff);
        sel.sp = std::min(p->sp, 0x7fff);
        sel.max_sp = std::min(p->max_sp, 0x7fff);
        sel.speed = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // p->speed;
        sel.species = p->species;
        sel.hair_style = p->hair;
        sel.weapon = 0; // p->weapon; // dont send weapon since TMW does not support it
        sel.base_level = p->base_level;
        sel.skill_point = p->skill_point;
        sel.head_bottom = p->head_bottom;
        sel.shield = p->shield;
        sel.head_top = p->head_top;
        sel.head_mid = p->head_mid;
        sel.hair_color = p->hair_color;
        sel.misc2 = find_equip_view(cp, EPOS::MISC2); // = p->clothes_color;

        sel.char_name = k->name;

        sel.stats.str = saturate<uint8_t>(p->attrs[ATTR::STR]);
        sel.stats.agi = saturate<uint8_t>(p->attrs[ATTR::AGI]);
        sel.stats.vit = saturate<uint8_t>(p->attrs[ATTR::VIT]);
        sel.stats.int_ = saturate<uint8_t>(p->attrs[ATTR::INT]);
        sel.stats.dex = saturate<uint8_t>(p->attrs[ATTR::DEX]);
        sel.stats.luk = saturate<uint8_t>(p->attrs[ATTR::LUK]);
        sel.char_num = k->char_num;
        if (p->sex == SEX::UNSPECIFIED)
            sel.sex = sd->sex;
        else
            sel.sex = p->sex;

        repeat_6b.push_back(info);
    }

    send_vpacket<0x006b, 24, 106>(s, head_6b, repeat_6b);

    return 0;
}

static
int set_account_reg2(AccountId acc, Slice<GlobalReg> reg)
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
                cd.data->account_reg2[i] = GlobalReg{};
            c++;
        }
    }
    return c;
}

// Divorce a character from it's partner and let the map server know
static
int char_divorce(CharPair *cp)
{
    if (cp == nullptr)
        return 0;

    CharKey *ck = &cp->key;
    CharData *cs = cp->data.get();

    if (!cs->partner_id)
    {
        Packet_Fixed<0x2b12> fixed_12;
        fixed_12.char_id = ck->char_id;
        // partner id 0 means failure
        fixed_12.partner_id = cs->partner_id;
        for (Session *ss : iter_map_sessions())
        {
            send_fpacket<0x2b12, 10>(ss, fixed_12);
        }
        return 0;
    }

    Packet_Fixed<0x2b12> fixed_12;
    fixed_12.char_id = ck->char_id;

    for (CharPair& cd : char_keys)
    {
        if (cd.key.char_id == cs->partner_id
            && cd.data->partner_id == ck->char_id)
        {
            fixed_12.partner_id = cs->partner_id;
            for (Session *ss : iter_map_sessions())
            {
                send_fpacket<0x2b12, 10>(ss, fixed_12);
            }

            cs->partner_id = CharId();
            cd.data->partner_id = CharId();
            return 0;
        }
        // The other char doesn't have us as their partner, so just clear our partner
        // Don't worry about this, as the map server should verify itself that the other doesn't have us as a partner, and so won't mess with their marriage
        else if (cd.key.char_id == cs->partner_id)
        {
            fixed_12.partner_id = cs->partner_id;
            for (Session *ss : iter_map_sessions())
            {
                send_fpacket<0x2b12, 10>(ss, fixed_12);
            }

            cs->partner_id = CharId();
            return 0;
        }
    }

    // Our partner wasn't found, so just clear our marriage
    fixed_12.partner_id = cs->partner_id;
    cs->partner_id = CharId();
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x2b12, 10>(ss, fixed_12);
    }

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
        Packet_Fixed<0x2afe> fixed_fe;
        fixed_fe.account_id = ck->account_id;
        for (Session *ss : iter_map_sessions())
        {
            send_fpacket<0x2afe, 6>(ss, fixed_fe);
        }
    }

    return 0;
}

static
void parse_tologin(Session *ls)
{
    assert (ls == login_session);

    char_session_data *sd = static_cast<char_session_data *>(ls->session_data.get());

    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(ls, &packet_id))
    {
        switch (packet_id)
        {
            case 0x2711:
            {
                Packet_Fixed<0x2711> fixed;
                rv = recv_fpacket<0x2711, 3>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                if (fixed.code)
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
                    {
                        if (server_session[i] && server[i].maps[0])   // if map-server online and at least 1 map
                            break;
                    }
                    if (i == MAX_MAP_SERVERS)
                        PRINTF("Awaiting maps from map-server.\n"_fmt);
                }
                break;
            }

            case 0x2713:
            {
                Packet_Fixed<0x2713> fixed;
                rv = recv_fpacket<0x2713, 51>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                for (io::FD i : iter_fds())
                {
                    AccountId acc = fixed.account_id;
                    Session *s2 = get_session(i);
                    if (!s2)
                        continue;
                    sd = static_cast<char_session_data *>(s2->session_data.get());
                    if (sd && sd->account_id == acc)
                    {
                        if (fixed.invalid != 0)
                        {
                            Packet_Fixed<0x006c> fixed_6c;
                            fixed_6c.code = 0x42;
                            send_fpacket<0x006c, 3>(s2, fixed_6c);
                        }
                        else if (char_conf.max_connect_user == 0
                                 || count_users() < char_conf.max_connect_user)
                        {
                            sd->email = stringish<AccountEmail>(fixed.email);
                            if (!e_mail_check(sd->email))
                                sd->email = DEFAULT_EMAIL;
                            // send characters to player
                            mmo_char_send006b(s2, sd);
                        }
                        else
                        {
                            // refuse connection: too much online players
                            Packet_Fixed<0x006c> fixed_6c;
                            fixed_6c.code = 0;
                            send_fpacket<0x006c, 3>(s2, fixed_6c);
                        }
                        break;
                    }
                }
                break;
            }

                // Receiving of an e-mail/time limit from the login-server (answer of a request because a player comes back from map-server to char-server) by [Yor]
            case 0x2717:
            {
                Packet_Fixed<0x2717> fixed;
                rv = recv_fpacket<0x2717, 50>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                for (io::FD i : iter_fds())
                {
                    AccountId acc = fixed.account_id;
                    Session *s2 = get_session(i);
                    if (!s2)
                        continue;
                    sd = static_cast<char_session_data *>(s2->session_data.get());
                    if (sd)
                    {
                        if (sd->account_id == acc)
                        {
                            sd->email = fixed.email;
                            if (!e_mail_check(sd->email))
                                sd->email = DEFAULT_EMAIL;
                            break;
                        }
                    }
                }
                break;
            }

            case 0x2723:       // changesex reply (modified by [Yor])
            {
                Packet_Fixed<0x2723> fixed;
                rv = recv_fpacket<0x2723, 7>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    SEX sex = fixed.sex;
                    if (acc)
                    {
                        disconnect_player(acc);
                    }
                    Packet_Fixed<0x2b0d> fixed_0d;
                    fixed_0d.account_id = acc;
                    fixed_0d.sex = sex;
                    for (Session *ss : iter_map_sessions())
                    {
                        send_fpacket<0x2b0d, 7>(ss, fixed_0d);
                    }
                }
                break;
            }

            case 0x2726:       // Request to send a broadcast message (no answer)
            {
                Packet_Head<0x2726> head;
                AString repeat;
                rv = recv_vpacket<0x2726, 8, 1>(ls, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                if (!repeat)
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
                        AString message = repeat.to_print().lstrip();
                        // if message is only composed of spaces
                        if (!message)
                            CHAR_LOG("Receiving a message for broadcast, but message is only a lot of spaces.\n"_fmt);
                        // else send message to all map-servers
                        else
                        {
                            CHAR_LOG("'ladmin': Receiving a message for broadcast (message (in yellow): %s)\n"_fmt,
                                    message);
                            // send broadcast to all map-servers
                            for (Session *ss : iter_map_sessions())
                            {
                                send_packet_repeatonly<0x3800, 4, 1>(ss, message);
                            }
                        }
                    }
                }
                break;
            }

                // account_reg2変更通知
            case 0x2729:
            {
                Packet_Head<0x2729> head;
                std::vector<Packet_Repeat<0x2729>> repeat;
                rv = recv_vpacket<0x2729, 8, 36>(ls, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                {
                    Array<GlobalReg, ACCOUNT_REG2_NUM> reg;
                    int j = 0;
                    AccountId acc = head.account_id;
                    for (const auto& info : repeat)
                    {
                        reg[j].str = info.name;
                        reg[j].value = info.value;
                        ++j;
                        if (j == ACCOUNT_REG2_NUM)
                            break;
                    }
                    set_account_reg2(acc, Slice<GlobalReg>(reg.begin(), j));

                    Packet_Head<0x2b11> head_11;
                    head_11.account_id = head.account_id;
                    std::vector<Packet_Repeat<0x2b11>> repeat_11(repeat.size());
                    for (size_t k = 0; k < repeat.size(); ++k)
                    {
                        repeat_11[k].name = repeat[k].name;
                        repeat_11[k].value = repeat[k].value;
                    }
                    for (Session *ss : iter_map_sessions())
                    {
                        send_vpacket<0x2b11, 8, 36>(ss, head_11, repeat_11);
                    }
                }
                break;
            }

                // Account deletion notification (from login-server)
            case 0x2730:
            {
                Packet_Fixed<0x2730> fixed;
                rv = recv_fpacket<0x2730, 6>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId aid = fixed.account_id;

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
                    Packet_Fixed<0x2b13> fixed_13;
                    fixed_13.account_id = aid;
                    for (Session *ss : iter_map_sessions())
                    {
                        send_fpacket<0x2b13, 6>(ss, fixed_13);
                    }
                }
                // disconnect player if online on char-server
                disconnect_player(aid);
                break;
            }

                // State change of account/ban notification (from login-server) by [Yor]
            case 0x2731:
            {
                Packet_Fixed<0x2731> fixed;
                rv = recv_fpacket<0x2731, 11>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId aid = fixed.account_id;
                // send to all map-servers to disconnect the player
                {
                    Packet_Fixed<0x2b14> fixed_14;
                    fixed_14.account_id = aid;
                    fixed_14.ban_not_status = fixed.ban_not_status; // 0: change of statut, 1: ban
                    fixed_14.status_or_ban_until = fixed.status_or_ban_until; // status or final date of a banishment
                    for (Session *ss : iter_map_sessions())
                    {
                        send_fpacket<0x2b14, 11>(ss, fixed_14);
                    }
                }
                // disconnect player if online on char-server
                disconnect_player(aid);
                break;
            }

                // Receiving GM acounts info from login-server (by [Yor])
            case 0x2732:
            {
                std::vector<Packet_Repeat<0x2732>> repeat;
                rv = recv_packet_repeatonly<0x2732, 4, 5>(ls, repeat);
                if (rv != RecvResult::Complete)
                    break;

                {
                    gm_accounts.resize(repeat.size());
                    for (size_t k = 0; k < repeat.size(); ++k)
                    {
                        gm_accounts[k].account_id = repeat[k].account_id;
                        gm_accounts[k].level = repeat[k].gm_level;
                    }
                    PRINTF("From login-server: receiving of %zu GM accounts information.\n"_fmt,
                            gm_accounts.size());
                    CHAR_LOG("From login-server: receiving of %zu GM accounts information.\n"_fmt,
                            gm_accounts.size());
                    create_online_files(); // update online players files (perhaps some online players change of GM level)
                    // send new gm acccounts level to map-servers
                    std::vector<Packet_Repeat<0x2b15>> repeat_15(repeat.size());
                    for (size_t k = 0; k < repeat.size(); ++k)
                    {
                        repeat_15[k].account_id = repeat[k].account_id;
                        repeat_15[k].gm_level = repeat[k].gm_level;
                    }
                    for (Session *ss : iter_map_sessions())
                    {
                        send_packet_repeatonly<0x2b15, 4, 5>(ss, repeat_15);
                    }
                }
                break;
            }

            case 0x2741:       // change password reply
            {
                Packet_Fixed<0x2741> fixed;
                rv = recv_fpacket<0x2741, 7>(ls, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    int status = fixed.status;

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
                                Packet_Fixed<0x0062> fixed_62;
                                fixed_62.status = status;
                                send_fpacket<0x0062, 3>(s2, fixed_62);
                                break;
                            }
                        }
                    }
                }
                break;
            }

            default:
            {
                ls->set_eof();
                return;
            }
        }
    }
    if (rv == RecvResult::Error)
        ls->set_eof();
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

    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(ms, &packet_id))
    {
        switch (packet_id)
        {
                // Receiving map names list from the map-server
            case 0x2afa:
            {
                std::vector<Packet_Repeat<0x2afa>> repeat;
                rv = recv_packet_repeatonly<0x2afa, 4, 16>(ms, repeat);
                if (rv != RecvResult::Complete)
                    break;

                for (MapName &foo : server[id].maps)
                    foo = MapName();

                for (size_t j = 0; j < repeat.size(); ++j)
                {
                    server[id].maps[j] = repeat[j].map_name;
                }
                const size_t j = repeat.size();

                {
                    PRINTF("Map-Server %d connected: %zu maps, from IP %s port %d.\n"_fmt,
                            id, j, server[id].ip, server[id].port);
                    PRINTF("Map-server %d loading complete.\n"_fmt, id);
                    CHAR_LOG("Map-Server %d connected: %zu maps, from IP %s port %d. Map-server %d loading complete.\n"_fmt,
                            id, j, server[id].ip,
                            server[id].port, id);
                }

                Packet_Fixed<0x2afb> fixed_fb;
                fixed_fb.unknown = 0;
                send_fpacket<0x2afb, 27>(ms, fixed_fb);

                {
                    if (j == 0)
                    {
                        PRINTF("WARNING: Map-Server %d have NO map.\n"_fmt, id);
                        CHAR_LOG("WARNING: Map-Server %d have NO map.\n"_fmt,
                                id);
                        // Transmitting maps information to the other map-servers
                    }
                    else
                    {
                        Packet_Head<0x2b04> head_04;
                        head_04.ip = server[id].ip;
                        head_04.port = server[id].port;
                        std::vector<Packet_Repeat<0x2b04>> repeat_04(j);
                        for (int i = 0; i < j; ++i)
                        {
                            repeat_04[i].map_name = server[id].maps[i];
                        }
                        for (Session *ss : iter_map_sessions())
                        {
                            if (ss == ms)
                                continue;
                            send_vpacket<0x2b04, 10, 16>(ss, head_04, repeat_04);
                        }
                    }
                    // Transmitting the maps of the other map-servers to the new map-server
                    for (int x = 0; x < MAX_MAP_SERVERS; x++)
                    {
                        if (server_session[x] && x != id)
                        {
                            Packet_Head<0x2b04> head_04;
                            head_04.ip = server[x].ip;
                            head_04.port = server[x].port;
                            std::vector<Packet_Repeat<0x2b04>> repeat_04;
                            for (int i = 0; i < MAX_MAP_PER_SERVER; i++)
                            {
                                if (server[x].maps[i])
                                {
                                    Packet_Repeat<0x2b04> info;
                                    info.map_name = server[x].maps[i];
                                    repeat_04.push_back(info);
                                }
                            }
                            if (repeat.size())
                            {
                                send_vpacket<0x2b04, 10, 16>(ms, head_04, repeat_04);
                            }
                        }
                    }
                }
                break;
            }

                // 認証要求
            case 0x2afc:
            {
                Packet_Fixed<0x2afc> fixed;
                rv = recv_fpacket<0x2afc, 22>(ms, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId account_id = fixed.account_id;
                CharId char_id = fixed.char_id;
                int login_id1 = fixed.login_id1;
                int login_id2 = fixed.login_id2;
                IP4Address ip = fixed.ip;
                for (AuthFifoEntry& afi : auth_fifo)
                {
                    if (afi.account_id == account_id &&
                        afi.char_id == char_id &&
                        afi.login_id1 == login_id1 &&
                        // here, it's the only area where it's possible that we doesn't know login_id2 (map-server asks just after 0x72 packet, that doesn't given the value)
                        (afi.login_id2 == login_id2 || login_id2 == 0) &&  // relate to the versions higher than 18
                        afi.ip == ip
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
                        Packet_Payload<0x2afd> payload_fd; // not file descriptor
                        payload_fd.account_id = account_id;
                        payload_fd.login_id2 = afi.login_id2;
                        cd->sex = afi.sex;
                        payload_fd.packet_client_version = afi.packet_client_version;
                        FPRINTF(stderr,
                                "From queue index %zd: recalling packet version %d\n"_fmt,
                                (&afi - &auth_fifo.front()), afi.packet_client_version);
                        payload_fd.char_key = *ck;
                        payload_fd.char_data = *cd;
                        send_ppacket<0x2afd>(ms, payload_fd);
                        goto x2afc_out;
                    }
                }
                {
                    Packet_Fixed<0x2afe> fixed_fe;
                    fixed_fe.account_id = account_id;
                    send_fpacket<0x2afe, 6>(ms, fixed_fe);
                    PRINTF("auth_fifo search error! account %d not authentified.\n"_fmt,
                            account_id);
                }
            x2afc_out:
                break;
            }

                // MAPサーバー上のユーザー数受信
            case 0x2aff:
            {
                Packet_Head<0x2aff> head;
                std::vector<Packet_Repeat<0x2aff>> repeat;
                rv = recv_vpacket<0x2aff, 6, 4>(ms, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                server[id].users = head.users;
                assert (head.users == repeat.size());
                if (char_conf.anti_freeze_enable)
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
                    CharId char_id = repeat[i].char_id;
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
                break;
            }

                // キャラデータ保存
            case 0x2b01:
            {
                Packet_Payload<0x2b01> payload;
                rv = recv_ppacket<0x2b01>(ms, payload);
                if (rv != RecvResult::Complete)
                    break;

                AccountId aid = payload.account_id;
                CharId cid = payload.char_id;
                for (CharPair& cd : char_keys)
                {
                    if (cd.key.account_id == aid &&
                        cd.key.char_id == cid)
                    {
                        cd.key = payload.char_key;
                        *cd.data = payload.char_data;
                        break;
                    }
                }
                break;
            }

                // キャラセレ要求
            case 0x2b02:
            {
                Packet_Fixed<0x2b02> fixed;
                rv = recv_fpacket<0x2b02, 18>(ms, fixed);
                if (rv != RecvResult::Complete)
                    break;

                AccountId account_id = fixed.account_id;
                if (auth_fifo_iter == auth_fifo.end())
                    auth_fifo_iter = auth_fifo.begin();
                auth_fifo_iter->account_id = account_id;
                auth_fifo_iter->char_id = CharId();
                auth_fifo_iter->login_id1 = fixed.login_id1;
                auth_fifo_iter->login_id2 = fixed.login_id2;
                auth_fifo_iter->delflag = 2;
                auth_fifo_iter->ip = fixed.ip;
                auth_fifo_iter++;

                Packet_Fixed<0x2b03> fixed_03;
                fixed_03.account_id = account_id;
                fixed_03.unknown = 0;
                send_fpacket<0x2b03, 7>(ms, fixed_03);
                break;
            }

                // マップサーバー間移動要求
            case 0x2b05:
            {
                Packet_Fixed<0x2b05> fixed;
                rv = recv_fpacket<0x2b05, 49>(ms, fixed);
                if (rv != RecvResult::Complete)
                    break;

                if (auth_fifo_iter == auth_fifo.end())
                    auth_fifo_iter = auth_fifo.begin();

                Packet_Fixed<0x2b06> fixed_06;
                fixed_06.account_id = fixed.account_id;
                //fixed_06.error = fixed.login_id1;
                //fixed_06.unknown = fixed.login_id2;
                fixed_06.char_id = fixed.char_id;
                fixed_06.map_name = fixed.map_name;
                fixed_06.x = fixed.x;
                fixed_06.y = fixed.y;
                fixed_06.map_ip = fixed.map_ip;
                fixed_06.map_port = fixed.map_port;

                auth_fifo_iter->account_id = fixed.account_id;
                auth_fifo_iter->login_id1 = fixed.login_id1;
                auth_fifo_iter->login_id2 = fixed.login_id2;
                auth_fifo_iter->char_id = fixed.char_id;
                auth_fifo_iter->delflag = 0;
                auth_fifo_iter->sex = fixed.sex;
                auth_fifo_iter->ip = fixed.client_ip;

                // default, if not found in the loop
                fixed_06.error = 1;
                for (const CharPair& cd : char_keys)
                {
                    AccountId aid = fixed.account_id;
                    CharId cid = fixed.char_id;
                    if (cd.key.account_id == aid &&
                        cd.key.char_id == cid)
                    {
                        auth_fifo_iter++;
                        fixed_06.error = 0;
                        break;
                    }
                }
                send_fpacket<0x2b06, 44>(ms, fixed_06);
                break;
            }

                // Map server send information to change an email of an account -> login-server
            case 0x2b0c:
            {
                Packet_Fixed<0x2b0c> fixed;
                rv = recv_fpacket<0x2b0c, 86>(ms, fixed);
                if (rv != RecvResult::Complete)
                    break;

                if (login_session)
                {               // don't send request if no login-server
                    Packet_Fixed<0x2722> fixed_22;
                    fixed_22.account_id = fixed.account_id;
                    fixed_22.old_email = fixed.old_email;
                    fixed_22.new_email = fixed.new_email;
                    send_fpacket<0x2722, 86>(login_session, fixed_22);
                }
                break;
            }

                // Map server ask char-server about a character name to do some operations (all operations are transmitted to login-server)
            case 0x2b0e:
            {
                Packet_Fixed<0x2b0e> fixed;
                rv = recv_fpacket<0x2b0e, 44>(ms, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId acc = fixed.account_id;
                    CharName character_name = fixed.char_name;
                    int operation = fixed.operation;
                    // prepare answer
                    Packet_Fixed<0x2b0f> fixed_0f;
                    fixed_0f.account_id = acc;
                    fixed_0f.operation = operation;
                    // search character
                    const CharPair *cd = search_character(character_name);
                    // TODO invert the below logic to shrink this code
                    // Current logic:
                    // 1. if no character, return error 1
                    // 2. else if gm level too low, return error 2
                    // 3. else if login server offline, return error 3
                    // 4. else return error 0 and maybe do other stuff
                    if (cd)
                    {
                        const CharKey *ck = &cd->key;
                        fixed_0f.char_name = ck->name;
                        fixed_0f.error = 0;
                        switch (operation)
                        {
                            case 1:    // block
                            {
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        Packet_Fixed<0x2724> fixed_24;
                                        fixed_24.account_id = ck->account_id;
                                        fixed_24.status = 5;
                                        send_fpacket<0x2724, 10>(login_session, fixed_24);
                                    }
                                    else
                                        fixed_0f.error = 3;
                                }
                                else
                                    fixed_0f.error = 2;
                                break;
                            }
                            case 2:    // ban
                            {
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        Packet_Fixed<0x2725> fixed_25;
                                        fixed_25.account_id = ck->account_id;
                                        HumanTimeDiff ban_change = fixed.ban_add;
                                        fixed_25.ban_add = ban_change;
                                        send_fpacket<0x2725, 18>(login_session, fixed_25);
                                    }
                                    else
                                        fixed_0f.error = 3;
                                }
                                else
                                    fixed_0f.error = 2;
                                break;
                            }
                            case 3:    // unblock
                            {
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        Packet_Fixed<0x2724> fixed_24;
                                        fixed_24.account_id = ck->account_id;
                                        fixed_24.status = 0;
                                        send_fpacket<0x2724, 10>(login_session, fixed_24);
                                    }
                                    else
                                        fixed_0f.error = 3;
                                }
                                else
                                    fixed_0f.error = 2;
                                break;
                            }
                            case 4:    // unban
                            {
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        Packet_Fixed<0x272a> fixed_2a;
                                        fixed_2a.account_id = ck->account_id;
                                        send_fpacket<0x272a, 6>(login_session, fixed_2a);
                                    }
                                    else
                                        fixed_0f.error = 3;
                                }
                                else
                                    fixed_0f.error = 2;
                                break;
                            }
                            case 5:    // changesex
                            case 6:    // changesex
                            case 7:    // changesex
                            {
                                if (!acc
                                    || isGM(acc).overwhelms(isGM(ck->account_id)))
                                {
                                    if (login_session)
                                    {   // don't send request if no login-server
                                        Packet_Fixed<0x2727> fixed_27;
                                        fixed_27.account_id = ck->account_id;
                                        switch (operation)
                                        {
                                            case 5:
                                                fixed_27.sex = SEX::FEMALE;
                                                break;
                                            case 6:
                                                fixed_27.sex = SEX::MALE;
                                                break;
                                            case 7:
                                                fixed_27.sex = SEX::NEUTRAL;
                                                break;
                                        }
                                        send_fpacket<0x2727, 7>(login_session, fixed_27);
                                    }
                                    else
                                        fixed_0f.error = 3;
                                }
                                else
                                    fixed_0f.error = 2;
                                break;
                            }
                        }
                    }
                    else
                    {
                        // character name not found
                        fixed_0f.char_name = character_name;
                        fixed_0f.error = 1;
                    }
                    // send answer if a player ask, not if the server ask
                    if (acc)
                    {
                        send_fpacket<0x2b0f, 34>(ms, fixed_0f);
                    }
                    break;
                }
            }

                // account_reg保存要求
            case 0x2b10:
            {
                Packet_Head<0x2b10> head;
                std::vector<Packet_Repeat<0x2b10>> repeat;
                rv = recv_vpacket<0x2b10, 8, 36>(ms, head, repeat);
                if (rv != RecvResult::Complete)
                    break;

                {
                    Array<GlobalReg, ACCOUNT_REG2_NUM> reg;
                    AccountId acc = head.account_id;
                    auto jlim = std::min(repeat.size(), ACCOUNT_REG2_NUM);
                    for (size_t j = 0; j < jlim; ++j)
                    {
                        reg[j].str = repeat[j].name;
                        reg[j].value = repeat[j].value;
                    }
                    set_account_reg2(acc, Slice<GlobalReg>(reg.begin(), jlim));
                    // loginサーバーへ送る
                    if (login_session)
                    {
                        // don't send request if no login-server
                        Packet_Head<0x2728> head_28;
                        std::vector<Packet_Repeat<0x2728>> repeat_28(repeat.size());
                        for (size_t j = 0; j < repeat.size(); ++j)
                        {
                            repeat_28[j].name = repeat[j].name;
                            repeat_28[j].value = repeat[j].value;
                        }
                        send_vpacket<0x2728, 8, 36>(login_session, head_28, repeat_28);
                    }
                    break;
                }
            }

                // Map server is requesting a divorce
            case 0x2b16:
            {
                Packet_Fixed<0x2b16> fixed;
                rv = recv_fpacket<0x2b16, 6>(ms, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    CharId cid = fixed.char_id;
                    for (CharPair& cd : char_keys)
                    {
                        if (cd.key.char_id == cid)
                        {
                            char_divorce(&cd);
                            break;
                        }
                    }

                    break;

                }
            }

            default:
                // inter server処理に渡す
            {
                RecvResult r = inter_parse_frommap(ms, packet_id);
                if (r == RecvResult::Complete)
                    break;
                if (r == RecvResult::Incomplete)
                    return;
                // inter server処理でもない場合は切断
                PRINTF("char: unknown packet 0x%04x (%zu bytes to read in buffer)! (from map).\n"_fmt,
                        packet_id, packet_avail(ms));
                ms->set_eof();
                return;
            }
        }
    }
    if (rv == RecvResult::Error)
        ms->set_eof();
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
    bool lancheck = char_lan_conf.lan_subnet.covers(addr);

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
                    Packet_Fixed<0x0081> fixed_81;
                    fixed_81.error_code = 1;
                    send_fpacket<0x0081, 3>(s, fixed_81);
                    return;
                }
            }

            Packet_Fixed<0x0071> fixed_71;
            fixed_71.char_id = ck->char_id;
            fixed_71.map_name = cd->last_point.map_;
            PRINTF("Character selection '%s' (account: %d, slot: %d) [%s]\n"_fmt,
                    ck->name,
                    sd->account_id, ck->char_num, ip);
            PRINTF("--Send IP of map-server. "_fmt);
            if (lan_ip_check(ip))
                fixed_71.ip = char_lan_conf.lan_map_ip;
            else
                fixed_71.ip = server[i].ip;
            fixed_71.port = server[i].port;
            send_fpacket<0x0071, 28>(s, fixed_71);

            if (auth_fifo_iter == auth_fifo.end())
                auth_fifo_iter = auth_fifo.begin();
            auth_fifo_iter->account_id = sd->account_id;
            auth_fifo_iter->char_id = ck->char_id;
            auth_fifo_iter->login_id1 = sd->login_id1;
            auth_fifo_iter->login_id2 = sd->login_id2;
            auth_fifo_iter->delflag = 0;
            auth_fifo_iter->sex = sd->sex;
            auth_fifo_iter->ip = s->client_ip;
            auth_fifo_iter->packet_client_version = sd->packet_client_version;
            auth_fifo_iter++;
        }
    }
}

static
void parse_char(Session *s)
{
    IP4Address ip = s->client_ip;

    assert (s != login_session);

    if (!login_session)
    {
        s->set_eof();
        return;
    }

    char_session_data *sd = static_cast<char_session_data *>(s->session_data.get());

    RecvResult rv = RecvResult::Complete;
    uint16_t packet_id;
    while (rv == RecvResult::Complete && packet_peek_id(s, &packet_id))
    {
        switch (packet_id)
        {
            case 0x61:         // change password request
            {
                Packet_Fixed<0x0061> fixed;
                rv = recv_fpacket<0x0061, 50>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    Packet_Fixed<0x2740> fixed_40;
                    fixed_40.account_id = sd->account_id;
                    AccountPass old_pass = fixed.old_pass;
                    fixed_40.old_pass = old_pass;
                    AccountPass new_pass = fixed.new_pass;
                    fixed_40.new_pass = new_pass;
                    send_fpacket<0x2740, 54>(login_session, fixed_40);
                }
                break;
            }

            case 0x65:         // 接続要求
            {
                Packet_Fixed<0x0065> fixed;
                rv = recv_fpacket<0x0065, 17>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                {
                    AccountId account_id = fixed.account_id;
                    GmLevel GM_value = isGM(account_id);
                    if (GM_value)
                        PRINTF("Account Logged On; Account ID: %d (GM level %d).\n"_fmt,
                                account_id, GM_value);
                    else
                        PRINTF("Account Logged On; Account ID: %d.\n"_fmt,
                                account_id);
                    if (sd == nullptr)
                    {
                        s->session_data = make_unique<char_session_data, SessionDeleter>();
                        sd = static_cast<char_session_data *>(s->session_data.get());
                        sd->email = stringish<AccountEmail>("no mail"_s);  // put here a mail without '@' to refuse deletion if we don't receive the e-mail
                    }
                    sd->account_id = account_id;
                    sd->login_id1 = fixed.login_id1;
                    sd->login_id2 = fixed.login_id2;
                    sd->packet_client_version = fixed.packet_client_version;
                    sd->sex = fixed.sex;

                    // formerly: send back account_id
                    Packet_Payload<0x8000> special;
                    special.magic_packet_length = 4;
                    send_ppacket<0x8000>(s, special);

                    if(sd->packet_client_version < MIN_CLIENT_VERSION)
                    {
                        Packet_Fixed<0x006a> fixed_6a;
                        fixed_6a.error_code = 5;
                        send_fpacket<0x006a, 23>(s, fixed_6a);
                        goto x65_out;
                    }

                    // search authentification
                    for (AuthFifoEntry& afi : auth_fifo)
                    {
                        if (afi.account_id == sd->account_id
                            && afi.login_id1 == sd->login_id1
                            && afi.login_id2 == sd->login_id2
                            && afi.ip == s->client_ip
                            && afi.delflag == 2)
                        {
                            afi.delflag = 1;
                            if (char_conf.max_connect_user == 0
                                || count_users() < char_conf.max_connect_user)
                            {
                                {
                                    // there is always a login server
                                    // request to login-server to obtain e-mail/time limit
                                    Packet_Fixed<0x2716> fixed_16;
                                    fixed_16.account_id = sd->account_id;
                                    send_fpacket<0x2716, 6>(login_session, fixed_16);
                                }
                                // Record client version
                                afi.packet_client_version =
                                    sd->packet_client_version;
                                // send characters to player
                                mmo_char_send006b(s, sd);
                            }
                            else
                            {
                                // refuse connection (over populated)
                                Packet_Fixed<0x006c> fixed_6c;
                                fixed_6c.code = 0;
                                send_fpacket<0x006c, 3>(s, fixed_6c);
                            }
                            goto x65_out;
                        }
                    }
                    // authentification not found
                    {
                        {
                            // there is always a login-server
                            Packet_Fixed<0x2712> fixed_12;
                            fixed_12.account_id = sd->account_id;
                            fixed_12.login_id1 = sd->login_id1;
                            fixed_12.login_id2 = sd->login_id2;  // relate to the versions higher than 18
                            fixed_12.sex = sd->sex;
                            fixed_12.ip = s->client_ip;
                            send_fpacket<0x2712, 19>(login_session, fixed_12);
                        }
                    }
                }
            x65_out:
                break;
            }

            case 0x66:         // キャラ選択
            {
                Packet_Fixed<0x0066> fixed;
                rv = recv_fpacket<0x0066, 3>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                if (!sd)
                {
                    s->set_eof();
                    return;
                }
                handle_x0066(s, sd, fixed.code, ip);
                break;
            }

            case 0x67:         // 作成
            {
                Packet_Fixed<0x0067> fixed;
                rv = recv_fpacket<0x0067, 37>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                if (!sd)
                {
                    s->set_eof();
                    return;
                }
                CharName name = fixed.char_name;
                Stats6 stats = fixed.stats;
                uint8_t slot = fixed.slot;
                uint16_t hair_color = fixed.hair_color;
                uint16_t hair_style = fixed.hair_style;
                short error_code = 0;
                const CharPair *cp = make_new_char(s, name, stats, slot, hair_color, hair_style, error_code);
                if (!cp)
                {
                    Packet_Fixed<0x006e> fixed_6e;
                    fixed_6e.code = error_code;
                    send_fpacket<0x006e, 3>(s, fixed_6e);
                    break;
                }
                const CharKey *ck = &cp->key;
                const CharData *cd = cp->data.get();

                Packet_Fixed<0x006d> fixed_6d;

                fixed_6d.char_select.char_id = ck->char_id;
                fixed_6d.char_select.base_exp = cd->base_exp;
                fixed_6d.char_select.zeny = cd->zeny;
                fixed_6d.char_select.job_exp = cd->job_exp;
                fixed_6d.char_select.job_level = cd->job_level;

                fixed_6d.char_select.shoes = ItemNameId();
                fixed_6d.char_select.gloves = ItemNameId();
                fixed_6d.char_select.cape = ItemNameId();
                fixed_6d.char_select.misc1 = ItemNameId();
                fixed_6d.char_select.option = Opt0();
                fixed_6d.char_select.unused = 0;

                // this was buggy until the protocol became generated
                // but the client ignores it anyway
                fixed_6d.char_select.karma = cd->karma;
                fixed_6d.char_select.manner = cd->manner;

                fixed_6d.char_select.status_point = 0x30;
                fixed_6d.char_select.hp = saturate<int16_t>(cd->hp);
                fixed_6d.char_select.max_hp = saturate<int16_t>(cd->max_hp);
                fixed_6d.char_select.sp = saturate<int16_t>(cd->sp);
                fixed_6d.char_select.max_sp = saturate<int16_t>(cd->max_sp);
                fixed_6d.char_select.speed = static_cast<uint16_t>(DEFAULT_WALK_SPEED.count());   // cd->speed;
                fixed_6d.char_select.species = cd->species;
                fixed_6d.char_select.hair_style = cd->hair;
                fixed_6d.char_select.weapon = 0;

                fixed_6d.char_select.base_level = cd->base_level;
                fixed_6d.char_select.skill_point = cd->skill_point;

                fixed_6d.char_select.head_bottom = ItemNameId();
                fixed_6d.char_select.shield = cd->shield;
                fixed_6d.char_select.head_top = cd->head_top;
                fixed_6d.char_select.head_mid = cd->head_mid;
                fixed_6d.char_select.hair_color = cd->hair_color;
                fixed_6d.char_select.misc2 = ItemNameId();

                fixed_6d.char_select.char_name = ck->name;

                fixed_6d.char_select.stats.str = saturate<uint8_t>(cd->attrs[ATTR::STR]);
                fixed_6d.char_select.stats.agi = saturate<uint8_t>(cd->attrs[ATTR::AGI]);
                fixed_6d.char_select.stats.vit = saturate<uint8_t>(cd->attrs[ATTR::VIT]);
                fixed_6d.char_select.stats.int_ = saturate<uint8_t>(cd->attrs[ATTR::INT]);
                fixed_6d.char_select.stats.dex = saturate<uint8_t>(cd->attrs[ATTR::DEX]);
                fixed_6d.char_select.stats.luk = saturate<uint8_t>(cd->attrs[ATTR::LUK]);
                fixed_6d.char_select.char_num = ck->char_num;
                fixed_6d.char_select.sex = cd->sex;

                send_fpacket<0x006d, 108>(s, fixed_6d);
                break;
            }

            case 0x68:         // delete char //Yor's Fix
            {
                Packet_Fixed<0x0068> fixed;
                rv = recv_fpacket<0x0068, 46>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                if (!sd)
                {
                    s->set_eof();
                    return;
                }

                {
                    {
                        CharId cid = fixed.char_id;
                        CharPair *cs = nullptr;
                        for (CharPair& cd : char_keys)
                        {
                            if (cd.key.char_id == cid)
                            {
                                if (cd.key.account_id == sd->account_id)
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
                            Packet_Fixed<0x006f> fixed_6f;
                            send_fpacket<0x006f, 2>(s, fixed_6f);
                            goto x68_out;
                        }
                    }

                    {
                        Packet_Fixed<0x0070> fixed_70;
                        fixed_70.code = 0;
                        send_fpacket<0x0070, 3>(s, fixed_70);
                    }
                }
            x68_out:
                break;
            }

            case 0x2af8:       // マップサーバーログイン
            {
                Packet_Fixed<0x2af8> fixed;
                rv = recv_fpacket<0x2af8, 60>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                int i;
                Packet_Fixed<0x2af9> fixed_f9;
                for (i = 0; i < MAX_MAP_SERVERS; i++)
                {
                    if (!server_session[i])
                        break;
                }
                AccountName userid_ = fixed.account_name;
                AccountPass passwd_ = fixed.account_pass;
                if (i == MAX_MAP_SERVERS || userid_ != char_conf.userid
                    || passwd_ != char_conf.passwd)
                {
                    fixed_f9.code = 3;
                    send_fpacket<0x2af9, 3>(s, fixed_f9);
                }
                else
                {
                    fixed_f9.code = 0;
                    s->set_parsers(SessionParsers{.func_parse= parse_frommap, .func_delete= delete_frommap});
                    server_session[i] = s;
                    if (char_conf.anti_freeze_enable)
                        server_freezeflag[i] = 5;   // Map anti-freeze system. Counter. 5 ok, 4...0 freezed
                    // ignore fixed.unknown
                    server[i].ip = fixed.ip;
                    server[i].port = fixed.port;
                    server[i].users = 0;
                    for (MapName& mapi : server[i].maps)
                        mapi = MapName();
                    send_fpacket<0x2af9, 3>(s, fixed_f9);
                    realloc_fifo(s, FIFOSIZE_SERVERLINK,
                                  FIFOSIZE_SERVERLINK);
                    // send gm acccounts level to map-servers
                    std::vector<Packet_Repeat<0x2b15>> repeat_15(gm_accounts.size());
                    auto it = repeat_15.begin();
                    for (const GM_Account& gma : gm_accounts)
                    {
                        it->account_id = gma.account_id;
                        it->gm_level = gma.level;
                        ++it;
                    }
                    send_packet_repeatonly<0x2b15, 4, 5>(s, repeat_15);
                    // justification: we switched the session parsers
                    parse_frommap(s);
                    return;
                }
                break;
            }

            case 0x7530:       // Athena情報所得
            {
                Packet_Fixed<0x7530> fixed;
                rv = recv_fpacket<0x7530, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                Packet_Fixed<0x7531> fixed_31;
                fixed_31.version = CURRENT_CHAR_SERVER_VERSION;
                send_fpacket<0x7531, 10>(s, fixed_31);
                break;
            }

            case 0x7532:       // 接続の切断(defaultと処理は一緒だが明示的にするため)
            {
                Packet_Fixed<0x7532> fixed;
                rv = recv_fpacket<0x7532, 2>(s, fixed);
                if (rv != RecvResult::Complete)
                    break;

                s->set_eof();
                return;
            }

            default:
                s->set_eof();
                return;
        }
    }
    if (rv == RecvResult::Error)
        s->set_eof();
}

static
void send_users_tologin(TimerData *, tick_t)
{
    int users = count_users();

    if (login_session)
    {
        // send number of user to login server
        Packet_Fixed<0x2714> fixed_14;
        fixed_14.users = users;
        send_fpacket<0x2714, 6>(login_session, fixed_14);
    }
    // send number of players to all map-servers
    Packet_Fixed<0x2b00> fixed_00;
    fixed_00.users = users;
    for (Session *ss : iter_map_sessions())
    {
        send_fpacket<0x2b00, 6>(ss, fixed_00);
    }
}

static
void check_connect_login_server(TimerData *, tick_t)
{
    if (!login_session)
    {
        PRINTF("Attempt to connect to login-server...\n"_fmt);
        login_session = make_connection(char_conf.login_ip, char_conf.login_port,
                SessionParsers{.func_parse= parse_tologin, .func_delete= delete_tologin});
        if (!login_session)
            return;
        realloc_fifo(login_session, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);

        Packet_Fixed<0x2710> fixed_10;
        fixed_10.account_name = char_conf.userid;
        fixed_10.account_pass = char_conf.passwd;
        fixed_10.unknown = 0;
        fixed_10.ip = char_conf.char_ip;
        fixed_10.port = char_conf.char_port;
        fixed_10.server_name = char_conf.server_name;
        fixed_10.unknown2 = 0;
        fixed_10.maintenance = 0;
        fixed_10.is_new = 0;
        send_fpacket<0x2710, 86>(login_session, fixed_10);
    }
}

static
bool lan_check()
{
    // sub-network check of the map-server
    {
        PRINTF("LAN test of LAN IP of the map-server: "_fmt);
        if (!lan_ip_check(char_lan_conf.lan_map_ip))
        {
            PRINTF(SGR_BOLD SGR_RED "***ERROR: LAN IP of the map-server doesn't belong to the specified Sub-network." SGR_RESET "\n"_fmt);
            return false;
        }
    }

    return true;
}

static
bool char_config(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    return parse_char_conf(char_conf, key, value);
}

static
bool char_lan_config(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    return parse_char_lan_conf(char_lan_conf, key, value);
}

static
bool inter_config(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    return parse_inter_conf(inter_conf, key, value);
}

static
bool char_confs(io::Spanned<XString> key, io::Spanned<ZString> value)
{
    if (key.data == "char_conf"_s)
    {
        return load_config_file(value.data, char_config);
    }
    if (key.data == "char_lan_conf"_s)
    {
        return load_config_file(value.data, char_lan_config);
    }
    if (key.data == "inter_conf"_s)
    {
        return load_config_file(value.data, inter_config);
    }
    key.span.error("Unknown meta-key for char server"_s);
    return false;
}
} // namespace char_

void term_func(void)
{
    using namespace tmwa::char_;
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

int do_init(Slice<ZString> argv)
{
    using namespace tmwa::char_;
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
            runflag &= load_config_file(argvi, char_::char_confs);
        }
    }

    if (!loaded_config_yet)
        runflag &= load_config_file("conf/tmwa-char.conf"_s, char_::char_confs);

    // a newline in the log...
    CHAR_LOG(""_fmt);
    CHAR_LOG("The char-server starting...\n"_fmt);

    runflag &= lan_check();

    mmo_char_init();
    inter_init2();

    update_online = TimeT::now();
    create_online_files();     // update online players files at start of the server

    char_session = make_listen_port(char_conf.char_port, SessionParsers{parse_char, delete_char});

    Timer(gettick() + 1_s,
            check_connect_login_server,
            10_s
    ).detach();
    Timer(gettick() + 1_s,
            send_users_tologin,
            5_s
    ).detach();
    Timer(gettick() + char_conf.autosave_time,
            mmo_char_sync_timer,
            char_conf.autosave_time
    ).detach();

    if (char_conf.anti_freeze_enable > 0)
    {
        Timer(gettick() + 1_s,
                map_anti_freeze_system,
                char_conf.anti_freeze_interval
        ).detach();
    }

    CHAR_LOG("The char-server is ready (Server is listening on the port %d).\n"_fmt,
            char_conf.char_port);

    PRINTF("The char-server is " SGR_BOLD SGR_GREEN "ready" SGR_RESET " (Server is listening on the port %d).\n\n"_fmt,
            char_conf.char_port);

    return 0;
}
// namespace char_ ends before term_func and do_init
} // namespace tmwa
