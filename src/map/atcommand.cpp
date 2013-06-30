#include "atcommand.hpp"

#include <cmath>
#include <cstring>
#include <ctime>

#include <fstream>

#include "../common/core.hpp"
#include "../common/cxxstdio.hpp"
#include "../common/extract.hpp"
#include "../common/human_time_diff.hpp"
#include "../common/io.hpp"
#include "../common/mmo.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/socket.hpp"
#include "../common/timer.hpp"
#include "../common/utils2.hpp"

#include "battle.hpp"
#include "chrif.hpp"
#include "clif.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "pc.hpp"
#include "script.hpp"
#include "skill.hpp"
#include "storage.hpp"
#include "tmw.hpp"
#include "trade.hpp"

#include "../poison.hpp"

#define ATCOMMAND_FUNC(x) static \
int atcommand_##x(const int fd, dumb_ptr<map_session_data> sd, ZString message)
ATCOMMAND_FUNC(setup);
ATCOMMAND_FUNC(broadcast);
ATCOMMAND_FUNC(localbroadcast);
ATCOMMAND_FUNC(charwarp);
ATCOMMAND_FUNC(warp);
ATCOMMAND_FUNC(where);
ATCOMMAND_FUNC(goto);
ATCOMMAND_FUNC(jump);
ATCOMMAND_FUNC(who);
ATCOMMAND_FUNC(whogroup);
ATCOMMAND_FUNC(whomap);
ATCOMMAND_FUNC(whomapgroup);
ATCOMMAND_FUNC(whogm);         // by Yor
ATCOMMAND_FUNC(save);
ATCOMMAND_FUNC(load);
ATCOMMAND_FUNC(speed);
ATCOMMAND_FUNC(storage);
ATCOMMAND_FUNC(option);
ATCOMMAND_FUNC(hide);
ATCOMMAND_FUNC(die);
ATCOMMAND_FUNC(kill);
ATCOMMAND_FUNC(alive);
ATCOMMAND_FUNC(kami);
ATCOMMAND_FUNC(heal);
ATCOMMAND_FUNC(item);
ATCOMMAND_FUNC(itemreset);
ATCOMMAND_FUNC(itemcheck);
ATCOMMAND_FUNC(baselevelup);
ATCOMMAND_FUNC(joblevelup);
ATCOMMAND_FUNC(help);
ATCOMMAND_FUNC(gm);
ATCOMMAND_FUNC(pvpoff);
ATCOMMAND_FUNC(pvpon);
ATCOMMAND_FUNC(model);
ATCOMMAND_FUNC(spawn);
ATCOMMAND_FUNC(killmonster);
ATCOMMAND_FUNC(killmonster2);
ATCOMMAND_FUNC(gat);
ATCOMMAND_FUNC(packet);
ATCOMMAND_FUNC(statuspoint);
ATCOMMAND_FUNC(skillpoint);
ATCOMMAND_FUNC(zeny);
template<ATTR attr>
ATCOMMAND_FUNC(param);
ATCOMMAND_FUNC(recall);
ATCOMMAND_FUNC(recallall);
ATCOMMAND_FUNC(revive);
ATCOMMAND_FUNC(character_stats);
ATCOMMAND_FUNC(character_stats_all);
ATCOMMAND_FUNC(character_option);
ATCOMMAND_FUNC(character_save);
ATCOMMAND_FUNC(doom);
ATCOMMAND_FUNC(doommap);
ATCOMMAND_FUNC(raise);
ATCOMMAND_FUNC(raisemap);
ATCOMMAND_FUNC(character_baselevel);
ATCOMMAND_FUNC(character_joblevel);
ATCOMMAND_FUNC(kick);
ATCOMMAND_FUNC(kickall);
ATCOMMAND_FUNC(questskill);
ATCOMMAND_FUNC(charquestskill);
ATCOMMAND_FUNC(lostskill);
ATCOMMAND_FUNC(charlostskill);
ATCOMMAND_FUNC(party);
ATCOMMAND_FUNC(charskreset);
ATCOMMAND_FUNC(charstreset);
ATCOMMAND_FUNC(charreset);
ATCOMMAND_FUNC(charstpoint);
ATCOMMAND_FUNC(charmodel);
ATCOMMAND_FUNC(charskpoint);
ATCOMMAND_FUNC(charzeny);
ATCOMMAND_FUNC(reloaditemdb);
ATCOMMAND_FUNC(reloadmobdb);
ATCOMMAND_FUNC(reloadskilldb);
ATCOMMAND_FUNC(reloadscript);
ATCOMMAND_FUNC(reloadgmdb);    // by Yor
ATCOMMAND_FUNC(mapexit);
ATCOMMAND_FUNC(idsearch);
ATCOMMAND_FUNC(mapinfo);
ATCOMMAND_FUNC(dye);           //** by fritz
ATCOMMAND_FUNC(hair_style);    //** by fritz
ATCOMMAND_FUNC(hair_color);    //** by fritz
ATCOMMAND_FUNC(all_stats);     //** by fritz
ATCOMMAND_FUNC(char_change_sex);   // by Yor
ATCOMMAND_FUNC(char_block);    // by Yor
ATCOMMAND_FUNC(char_ban);      // by Yor
ATCOMMAND_FUNC(char_unblock);  // by Yor
ATCOMMAND_FUNC(char_unban);    // by Yor
ATCOMMAND_FUNC(partyspy);      // [Syrus22]
ATCOMMAND_FUNC(partyrecall);   // by Yor
ATCOMMAND_FUNC(enablenpc);
ATCOMMAND_FUNC(disablenpc);
ATCOMMAND_FUNC(servertime);    // by Yor
ATCOMMAND_FUNC(chardelitem);   // by Yor
ATCOMMAND_FUNC(email);         // by Yor
ATCOMMAND_FUNC(effect);        //by Apple
ATCOMMAND_FUNC(character_item_list);   // by Yor
ATCOMMAND_FUNC(character_storage_list);    // by Yor
ATCOMMAND_FUNC(character_cart_list);   // by Yor
ATCOMMAND_FUNC(addwarp);       // by MouseJstr
ATCOMMAND_FUNC(killer);        // by MouseJstr
ATCOMMAND_FUNC(npcmove);       // by MouseJstr
ATCOMMAND_FUNC(killable);      // by MouseJstr
ATCOMMAND_FUNC(charkillable);  // by MouseJstr
ATCOMMAND_FUNC(chareffect);    // by MouseJstr
ATCOMMAND_FUNC(dropall);       // by MouseJstr
ATCOMMAND_FUNC(chardropall);   // by MouseJstr
ATCOMMAND_FUNC(storeall);      // by MouseJstr
ATCOMMAND_FUNC(charstoreall);  // by MouseJstr
ATCOMMAND_FUNC(summon);
ATCOMMAND_FUNC(rain);
ATCOMMAND_FUNC(snow);
ATCOMMAND_FUNC(sakura);
ATCOMMAND_FUNC(fog);
ATCOMMAND_FUNC(leaves);
ATCOMMAND_FUNC(adjgmlvl);      // by MouseJstr
ATCOMMAND_FUNC(adjcmdlvl);     // by MouseJstr
ATCOMMAND_FUNC(trade);         // by MouseJstr
ATCOMMAND_FUNC(char_wipe);     // [Fate]
ATCOMMAND_FUNC(set_magic);     // [Fate]
ATCOMMAND_FUNC(magic_info);    // [Fate]
ATCOMMAND_FUNC(log);           // [Fate]
ATCOMMAND_FUNC(tee);           // [Fate]
ATCOMMAND_FUNC(invisible);     // [Fate]
ATCOMMAND_FUNC(visible);       // [Fate]
ATCOMMAND_FUNC(list_nearby);   // [Fate]
ATCOMMAND_FUNC(iterate_forward_over_players);  // [Fate]
ATCOMMAND_FUNC(iterate_backwards_over_players);    // [Fate]
ATCOMMAND_FUNC(skillpool_info);    // [Fate]
ATCOMMAND_FUNC(skillpool_focus);   // [Fate]
ATCOMMAND_FUNC(skillpool_unfocus); // [Fate]
ATCOMMAND_FUNC(skill_learn);   // [Fate]
ATCOMMAND_FUNC(wgm);
ATCOMMAND_FUNC(ipcheck);
ATCOMMAND_FUNC(doomspot);

/*==========================================
 *AtCommandInfo atcommand_info[]構造体の定義
 *------------------------------------------
 */
struct AtCommandInfo
{
    ZString command;
    int level;
    int (*proc)(const int fd, dumb_ptr<map_session_data> sd, ZString message);


    AtCommandInfo(ZString c, int l, int (*p)(const int, dumb_ptr<map_session_data>, ZString))
    : command(c), level(l), proc(p)
    {}
};

// First char of commands is configured in atcommand_athena.conf. Leave @ in this list for default value.
// to set default level, read atcommand_athena.conf first please.
static
AtCommandInfo atcommand_info[] =
{
    {"@setup", 40, atcommand_setup},
    {"@charwarp", 60, atcommand_charwarp},
    {"@warp", 40, atcommand_warp},
    {"@where", 1, atcommand_where},
    {"@goto", 20, atcommand_goto},
    {"@jump", 40, atcommand_jump},
    {"@who", 20, atcommand_who},
    {"@whogroup", 20, atcommand_whogroup},
    {"@whomap", 20, atcommand_whomap},
    {"@whomapgroup", 20, atcommand_whomapgroup},
    {"@whogm", 20, atcommand_whogm},   // by Yor
    {"@save", 40, atcommand_save},
    {"@return", 40, atcommand_load},
    {"@load", 40, atcommand_load},
    {"@speed", 40, atcommand_speed},
    {"@storage", 1, atcommand_storage},
    {"@option", 40, atcommand_option},
    {"@hide", 40, atcommand_hide},  // + /hide
    {"@die", 1, atcommand_die},
    {"@kill", 60, atcommand_kill},
    {"@alive", 60, atcommand_alive},
    {"@kami", 40, atcommand_kami},
    {"@heal", 40, atcommand_heal},
    {"@item", 60, atcommand_item},
    {"@itemreset", 40, atcommand_itemreset},
    {"@itemcheck", 60, atcommand_itemcheck},
    {"@blvl", 60, atcommand_baselevelup},
    {"@jlvl", 60, atcommand_joblevelup},
    {"@help", 20, atcommand_help},
    {"@gm", 100, atcommand_gm},
    {"@pvpoff", 40, atcommand_pvpoff},
    {"@pvpon", 40, atcommand_pvpon},
    {"@model", 20, atcommand_model},
    {"@spawn", 50, atcommand_spawn},
    {"@killmonster", 60, atcommand_killmonster},
    {"@killmonster2", 40, atcommand_killmonster2},
    {"@gat", 99, atcommand_gat}, // debug function
    {"@packet", 99, atcommand_packet},    // debug function
    {"@stpoint", 60, atcommand_statuspoint},
    {"@skpoint", 60, atcommand_skillpoint},
    {"@zeny", 60, atcommand_zeny},
    {"@str", 60, atcommand_param<ATTR::STR>},
    {"@agi", 60, atcommand_param<ATTR::AGI>},
    {"@vit", 60, atcommand_param<ATTR::VIT>},
    {"@int", 60, atcommand_param<ATTR::INT>},
    {"@dex", 60, atcommand_param<ATTR::DEX>},
    {"@luk", 60, atcommand_param<ATTR::LUK>},
    {"@recall", 60, atcommand_recall},    // + /recall
    {"@revive", 60, atcommand_revive},
    {"@charstats", 40, atcommand_character_stats},
    {"@charstatsall", 40, atcommand_character_stats_all},
    {"@charoption", 60, atcommand_character_option},
    {"@charsave", 60, atcommand_character_save},
    {"@doom", 80, atcommand_doom},
    {"@doommap", 80, atcommand_doommap},
    {"@raise", 80, atcommand_raise},
    {"@raisemap", 80, atcommand_raisemap},
    {"@charbaselvl", 60, atcommand_character_baselevel},
    {"@charjlvl", 60, atcommand_character_joblevel},
    {"@kick", 20, atcommand_kick},  // + right click menu for GM "(name) force to quit"
    {"@kickall", 99, atcommand_kickall},
    {"@questskill", 40, atcommand_questskill},
    {"@charquestskill", 60, atcommand_charquestskill},
    {"@lostskill", 40, atcommand_lostskill},
    {"@charlostskill", 60, atcommand_charlostskill},
    {"@party", 1, atcommand_party},
    {"@mapexit", 99, atcommand_mapexit},
    {"@idsearch", 60, atcommand_idsearch},
    {"@mapmove", 40, atcommand_warp},    // /mm command
    {"@broadcast", 40, atcommand_broadcast},   // /b and /nb command
    {"@localbroadcast", 40, atcommand_localbroadcast},    // /lb and /nlb command
    {"@recallall", 80, atcommand_recallall},
    {"@charskreset", 60, atcommand_charskreset},
    {"@charstreset", 60, atcommand_charstreset},
    {"@reloaditemdb", 99, atcommand_reloaditemdb},  // admin command
    {"@reloadmobdb", 99, atcommand_reloadmobdb}, // admin command
    {"@reloadskilldb", 99, atcommand_reloadskilldb},   // admin command
    {"@reloadscript", 99, atcommand_reloadscript},  // admin command
    {"@reloadgmdb", 99, atcommand_reloadgmdb},    // admin command
    {"@charreset", 60, atcommand_charreset},
    {"@charmodel", 50, atcommand_charmodel},
    {"@charskpoint", 60, atcommand_charskpoint},
    {"@charstpoint", 60, atcommand_charstpoint},
    {"@charzeny", 60, atcommand_charzeny},
    {"@mapinfo", 99, atcommand_mapinfo},
    {"@dye", 40, atcommand_dye}, // by fritz
    {"@ccolor", 40, atcommand_dye},  // by fritz
    {"@hairstyle", 40, atcommand_hair_style},  // by fritz
    {"@haircolor", 40, atcommand_hair_color},  // by fritz
    {"@allstats", 60, atcommand_all_stats}, // by fritz
    {"@charchangesex", 60, atcommand_char_change_sex}, // by Yor
    {"@block", 60, atcommand_char_block},  // by Yor
    {"@unblock", 60, atcommand_char_unblock},    // by Yor
    {"@ban", 60, atcommand_char_ban},    // by Yor
    {"@unban", 60, atcommand_char_unban},  // by Yor
    {"@partyspy", 60, atcommand_partyspy},  // [Syrus22]
    {"@partyrecall", 60, atcommand_partyrecall}, // by Yor
    {"@enablenpc", 80, atcommand_enablenpc},   // []
    {"@disablenpc", 80, atcommand_disablenpc},    // []
    {"@servertime", 0, atcommand_servertime}, // by Yor
    {"@chardelitem", 60, atcommand_chardelitem}, // by Yor
    {"@listnearby", 40, atcommand_list_nearby},   // by Yor
    {"@email", 0, atcommand_email},    // by Yor
    {"@effect", 40, atcommand_effect},    // by Apple
    {"@charitemlist", 40, atcommand_character_item_list}, // by Yor
    {"@charstoragelist", 40, atcommand_character_storage_list},    // by Yor
    {"@charcartlist", 40, atcommand_character_cart_list}, // by Yor
    {"@addwarp", 20, atcommand_addwarp}, // by MouseJstr
    {"@killer", 60, atcommand_killer},    // by MouseJstr
    {"@npcmove", 20, atcommand_npcmove}, // by MouseJstr
    {"@killable", 40, atcommand_killable},  // by MouseJstr
    {"@charkillable", 40, atcommand_charkillable},  // by MouseJstr
    {"@chareffect", 40, atcommand_chareffect},    // MouseJstr
    {"@dropall", 40, atcommand_dropall}, // MouseJstr
    {"@chardropall", 40, atcommand_chardropall}, // MouseJstr
    {"@storeall", 40, atcommand_storeall},  // MouseJstr
    {"@charstoreall", 40, atcommand_charstoreall},  // MouseJstr
    {"@rain", 99, atcommand_rain},
    {"@snow", 99, atcommand_snow},
    {"@sakura", 99, atcommand_sakura},
    {"@fog", 99, atcommand_fog},
    {"@leaves", 99, atcommand_leaves},
    {"@summon", 60, atcommand_summon},
    {"@adjgmlvl", 99, atcommand_adjgmlvl},
    {"@adjcmdlvl", 99, atcommand_adjcmdlvl},
    {"@trade", 60, atcommand_trade},
    {"@charwipe", 60, atcommand_char_wipe},   // [Fate]
    {"@setmagic", 99, atcommand_set_magic}, // [Fate]
    {"@magicinfo", 60, atcommand_magic_info},  // [Fate]
    {"@log", 60, atcommand_log}, // [Fate]
    {"@l", 60, atcommand_log},   // [Fate]
    {"@tee", 60, atcommand_tee}, // [Fate]
    {"@t", 60, atcommand_tee},   // [Fate]
    {"@invisible", 60, atcommand_invisible},   // [Fate]
    {"@visible", 60, atcommand_visible}, // [Fate]
    {"@hugo", 60, atcommand_iterate_forward_over_players},    // [Fate]
    {"@linus", 60, atcommand_iterate_backwards_over_players},    // [Fate]
    {"@sp-info", 40, atcommand_skillpool_info},  // [Fate]
    {"@sp-focus", 80, atcommand_skillpool_focus},    // [Fate]
    {"@sp-unfocus", 80, atcommand_skillpool_unfocus},    // [Fate]
    {"@skill-learn", 80, atcommand_skill_learn}, // [Fate]
    {"@wgm", 0, atcommand_wgm},
    {"@ipcheck", 60, atcommand_ipcheck},
    {"@doomspot", 60, atcommand_doomspot},

// add new commands before this line
    {ZString(), 1, nullptr}
};

// If your last arg is not a ZString, you probably wanted extract()
// but not always ...
static
bool asplit(ZString raw, ZString *last)
{
    *last = raw;
    return true;
}

// but this case is just so common and useful. In fact, is the previous ever used otherwise?
static
bool asplit(ZString raw, CharName *last)
{
    if (raw.size() < 4 || raw.size() > 23)
        return false;
    *last = stringish<CharName>(raw);
    return true;
}

// huh.
static
bool asplit(ZString raw, NpcName *last)
{
    if (!raw || raw.size() > 23)
        return false;
    *last = stringish<NpcName>(raw);
    return true;
}

// This differs from extract() in that it does not consume extra spaces.
template<class F, class... R, typename=typename std::enable_if<sizeof...(R) != 0>::type>
bool asplit(ZString raw, F *first_arg, R *... rest_args)
{
    ZString::iterator it = std::find(raw.begin(), raw.end(), ' ');
    XString frist = raw.xislice_h(it);
    while (*it == ' ')
        ++it;
    ZString rest = raw.xislice_t(it);
    return extract(frist, first_arg) && asplit(rest, rest_args...);
}

/*==========================================
 * get_atcommand_level @コマンドの必要レベルを取得
 *------------------------------------------
 */
static
int get_atcommand_level(const AtCommandInfo *type)
{
    if (type)
        return type->level;

    return 100;                 // 100: command can not be used
}

static
FILE *get_gm_log();

/*========================================
 * At-command logging
 */
void log_atcommand(dumb_ptr<map_session_data> sd, XString cmd)
{
    FILE *fp = get_gm_log();
    if (!fp)
        return;
    timestamp_seconds_buffer tmpstr;
    stamp_time(tmpstr);
    FPRINTF(fp, "[%s] %s(%d,%d) %s(%d) : ",
            tmpstr,
            sd->bl_m->name_, sd->bl_x, sd->bl_y,
            sd->status.name, sd->status.account_id);
    fwrite(cmd.data(), 1, cmd.size(), fp);
}

FString gm_logfile_name;
/*==========================================
 * Log a timestamped line to GM log file
 *------------------------------------------
 */
FILE *get_gm_log()
{
    if (!gm_logfile_name)
        return NULL;

    struct tm ctime = TimeT::now();

    int year = ctime.tm_year + 1900;
    int month = ctime.tm_mon + 1;
    int logfile_nr = (year * 12) + month;

    static FILE *gm_logfile = NULL;
    static int last_logfile_nr = 0;
    if (logfile_nr == last_logfile_nr)
        return gm_logfile;
    last_logfile_nr = logfile_nr;

    FString fullname = STRPRINTF("%s.%04d-%02d",
            gm_logfile_name, year, month);

    if (gm_logfile)
        fclose(gm_logfile);

    gm_logfile = fopen(fullname.c_str(), "a");

    if (!gm_logfile)
    {
        perror("GM log file");
        gm_logfile_name = FString();
    }
    return gm_logfile;
}

static
AtCommandInfo *atcommand(const int level, ZString message);
/*==========================================
 *is_atcommand @コマンドに存在するかどうか確認する
 *------------------------------------------
 */
bool is_atcommand(const int fd, dumb_ptr<map_session_data> sd,
        ZString message, int gmlvl)
{
    nullpo_retr(false, sd);

    if (!message.startswith('@'))
        return false;

    AtCommandInfo *info = atcommand(gmlvl > 0 ? gmlvl : pc_isGM(sd), message);

    if (!info)
    {
        FString output = STRPRINTF("GM command not found: %s",
                message);
        clif_displaymessage(fd, output);
        return true; // don't show in chat
    }

    {
        XString command;
        ZString arg;
        asplit(message, &command, &arg);

        {
            if (info->proc(fd, sd, arg) != 0)
            {
                // Command can not be executed
                FString output = STRPRINTF("%s failed.", FString(command));
                clif_displaymessage(fd, output);
            }
            else
            {
                if (get_atcommand_level(info) != 0)    // Don't log level 0 commands
                    log_atcommand(sd, message);
            }
        }

        return true;
    }
}

/*==========================================
 *
 *------------------------------------------
 */
AtCommandInfo *atcommand(const int level, ZString message)
{
    ZString p = message;

    if (battle_config.atc_gmonly != 0 && !level)    // level = pc_isGM(sd)
        return nullptr;
    if (!p)
    {
        FPRINTF(stderr, "at command message is empty\n");
        return nullptr;
    }

    if (p.startswith('@'))
    {
        ZString::iterator space = std::find(p.begin(), p.end(), ' ');
        XString command = p.xislice_h(space);
        int i = 0;

        while (atcommand_info[i].command)
        {
            if (command == atcommand_info[i].command
                && level >= atcommand_info[i].level)
            {
                return &atcommand_info[i];
            }
            i++;
        }
    }
    return nullptr;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void atkillmonster_sub(dumb_ptr<block_list> bl, int flag)
{
    nullpo_retv(bl);

    dumb_ptr<mob_data> md = bl->as_mob();
    if (flag)
        mob_damage(NULL, md, md->hp, 2);
    else
        mob_delete(md);
}

/*==========================================
 *
 *------------------------------------------
 */
static
AtCommandInfo *get_atcommandinfo_byname(XString name)
{
    for (int i = 0; atcommand_info[i].command; i++)
        if (atcommand_info[i].command.xslice_t(1) == name)
            return &atcommand_info[i];

    return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_config_read(ZString cfgName)
{
    std::ifstream in(cfgName.c_str());
    if (!in.is_open())
    {
        PRINTF("At commands configuration file not found: %s\n", cfgName);
        return 1;
    }

    FString line;
    while (io::getline(in, line))
    {
        SString w1;
        TString w2;
        if (!split_key_value(line, &w1, &w2))
            continue;
        AtCommandInfo *p = get_atcommandinfo_byname(w1);
        if (p != NULL)
        {
            p->level = atoi(w2.c_str());
            if (p->level > 100)
                p->level = 100;
            else if (p->level < 0)
                p->level = 0;
        }
        else if (w1 == "import")
            atcommand_config_read(w2);
        else
            PRINTF("%s: bad line: %s\n", cfgName, line);
    }

    return 0;
}

/*==========================================
// @ command processing functions
 *------------------------------------------
 */

/*==========================================
 * @setup - Safely set a chars levels and warp them to a special place
 * TAW Specific
 *------------------------------------------
 */
int atcommand_setup(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int level = 1;
    CharName character;

    if (!asplit(message, &level, &character))
    {
        clif_displaymessage(fd, "Usage: @setup <level> <char name>");
        return -1;
    }
    level--;

    FString buf;
    buf = STRPRINTF("-255 %s", character);
    atcommand_character_baselevel(fd, sd, buf);

    buf = STRPRINTF("%d %s", level, character);
    atcommand_character_baselevel(fd, sd, buf);

    // Emote skill
    buf = STRPRINTF("1 1 %s", character);
    atcommand_skill_learn(fd, sd, buf);

    // Trade skill
    buf = STRPRINTF("2 1 %s", character);
    atcommand_skill_learn(fd, sd, buf);

    // Party skill
    STRPRINTF("2 2 %s", character);
    atcommand_skill_learn(fd, sd, buf);

    STRPRINTF("018-1.gat 24 98 %s", character);
    atcommand_charwarp(fd, sd, buf);

    return 0;

}

/*==========================================
 * @rura+
 *------------------------------------------
 */
int atcommand_charwarp(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MapName map_name;
    CharName character;
    int x = 0, y = 0;

    if (!asplit(message, &map_name, &x, &y, &character))
    {
        clif_displaymessage(fd,
                "Usage: @charwarp/@rura+ <mapname> <x> <y> <char name>");
        return -1;
    }

    if (x <= 0)
        x = random_::in(1, 399);
    if (y <= 0)
        y = random_::in(1, 399);

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {
            // you can rura+ only lower or same GM level
            if (x > 0 && x < 800 && y > 0 && y < 800)
            {
                map_local *m = map_mapname2mapid(map_name);
                if (m != nullptr && m->flag.nowarpto
                    && battle_config.any_warp_GM_min_level > pc_isGM(sd))
                {
                    clif_displaymessage(fd,
                            "You are not authorised to warp someone to this map.");
                    return -1;
                }
                if (pl_sd->bl_m && pl_sd->bl_m->flag.nowarp
                    && battle_config.any_warp_GM_min_level > pc_isGM(sd))
                {
                    clif_displaymessage(fd,
                            "You are not authorised to warp this player from its actual map.");
                    return -1;
                }
                if (pc_setpos(pl_sd, map_name, x, y, BeingRemoveWhy::WARPED) == 0)
                {
                    clif_displaymessage(pl_sd->fd, "Warped.");
                    clif_displaymessage(fd, "Player warped (message sends to player too).");
                }
                else
                {
                    clif_displaymessage(fd, "Map not found.");
                    return -1;
                }
            }
            else
            {
                clif_displaymessage(fd, "Coordinates out of range.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_warp(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MapName map_name;
    int x = 0, y = 0;

    if (!message
        || !extract(message, record<' ', 1>(&map_name, &x, &y)))
    {
        clif_displaymessage(fd,
                "Please, enter a map (usage: @warp <mapname> <x> <y>).");
        return -1;
    }

    if (x <= 0)
        x = random_::in(1, 399);
    if (y <= 0)
        y = random_::in(1, 399);

    if (x > 0 && x < 800 && y > 0 && y < 800)
    {
        map_local *m = map_mapname2mapid(map_name);
        if (m != nullptr && m->flag.nowarpto
            && battle_config.any_warp_GM_min_level > pc_isGM(sd))
        {
            clif_displaymessage(fd,
                    "You are not authorised to warp you to this map.");
            return -1;
        }
        if (sd->bl_m && sd->bl_m->flag.nowarp
            && battle_config.any_warp_GM_min_level > pc_isGM(sd))
        {
            clif_displaymessage(fd,
                    "You are not authorised to warp you from your actual map.");
            return -1;
        }
        if (pc_setpos(sd, map_name, x, y, BeingRemoveWhy::WARPED) == 0)
            clif_displaymessage(fd, "Warped.");
        else
        {
            clif_displaymessage(fd, "Map not found.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Coordinates out of range.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_where(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;
    extract(message, &character);

    dumb_ptr<map_session_data> pl_sd = character.to__actual() ? map_nick2sd(character) : sd;
    if (pl_sd != NULL &&
        !((battle_config.hide_GM_session
           || bool(pl_sd->status.option & Option::HIDE))
          && (pc_isGM(pl_sd) > pc_isGM(sd))))
    {                           // you can look only lower or same level
        FString output = STRPRINTF("%s: %s (%d,%d)",
                pl_sd->status.name,
                pl_sd->mapname_, pl_sd->bl_x, pl_sd->bl_y);
        clif_displaymessage(fd, output);
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_goto(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @jumpto/@warpto/@goto <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pl_sd->bl_m && pl_sd->bl_m->flag.nowarpto
            && battle_config.any_warp_GM_min_level > pc_isGM(sd))
        {
            clif_displaymessage(fd,
                    "You are not authorised to warp you to the map of this player.");
            return -1;
        }
        if (sd->bl_m && sd->bl_m->flag.nowarp
            && battle_config.any_warp_GM_min_level > pc_isGM(sd))
        {
            clif_displaymessage(fd,
                    "You are not authorised to warp you from your actual map.");
            return -1;
        }
        pc_setpos(sd, pl_sd->mapname_, pl_sd->bl_x, pl_sd->bl_y, BeingRemoveWhy::WARPED);
        FString output = STRPRINTF("Jump to %s", character);
        clif_displaymessage(fd, output);
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_jump(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int x = 0, y = 0;
    // may fail
    extract(message, record<' '>(&x, &y));

    if (x <= 0)
        x = random_::in(1, 399);
    if (y <= 0)
        y = random_::in(1, 399);
    if (x > 0 && x < 800 && y > 0 && y < 800)
    {
        if (sd->bl_m && sd->bl_m->flag.nowarpto
            && battle_config.any_warp_GM_min_level > pc_isGM(sd))
        {
            clif_displaymessage(fd,
                    "You are not authorised to warp you to your actual map.");
            return -1;
        }
        if (sd->bl_m && sd->bl_m->flag.nowarp
            && battle_config.any_warp_GM_min_level > pc_isGM(sd))
        {
            clif_displaymessage(fd,
                    "You are not authorised to warp you from your actual map.");
            return -1;
        }
        pc_setpos(sd, sd->mapname_, x, y, BeingRemoveWhy::WARPED);
        FString output = STRPRINTF("Jump to %d %d", x, y);
        clif_displaymessage(fd, output);
    }
    else
    {
        clif_displaymessage(fd, "Coordinates out of range.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_who(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int count;
    int pl_GM_level, GM_level;
    VString<23> match_text = message;
    match_text = match_text.to_lower();

    count = 0;
    GM_level = pc_isGM(sd);
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            pl_GM_level = pc_isGM(pl_sd);
            if (!
                ((battle_config.hide_GM_session
                  || bool(pl_sd->status.option & Option::HIDE))
                 && (pl_GM_level > GM_level)))
            {
                // you can look only lower or same level
                VString<23> player_name = pl_sd->status.name.to__lower();
                if (player_name.contains_seq(match_text))
                {
                    // search with no case sensitive
                    FString output;
                    if (pl_GM_level > 0)
                        output = STRPRINTF(
                                "Name: %s (GM:%d) | Location: %s %d %d",
                                pl_sd->status.name, pl_GM_level,
                                pl_sd->mapname_, pl_sd->bl_x, pl_sd->bl_y);
                    else
                        output = STRPRINTF(
                                "Name: %s | Location: %s %d %d",
                                pl_sd->status.name, pl_sd->mapname_,
                                pl_sd->bl_x, pl_sd->bl_y);
                    clif_displaymessage(fd, output);
                    count++;
                }
            }
        }
    }

    if (count == 0)
        clif_displaymessage(fd, "No player found.");
    else if (count == 1)
        clif_displaymessage(fd, "1 player found.");
    else
    {
        FString output = STRPRINTF("%d players found.", count);
        clif_displaymessage(fd, output);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_whogroup(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int count;
    int pl_GM_level, GM_level;
    struct party *p;

    VString<23> match_text = message;
    match_text = match_text.to_lower();

    count = 0;
    GM_level = pc_isGM(sd);
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            pl_GM_level = pc_isGM(pl_sd);
            if (!
                ((battle_config.hide_GM_session
                  || bool(pl_sd->status.option & Option::HIDE))
                 && (pl_GM_level > GM_level)))
            {
                // you can look only lower or same level
                VString<23> player_name = pl_sd->status.name.to__lower();
                if (player_name.contains_seq(match_text))
                {
                    // search with no case sensitive
                    p = party_search(pl_sd->status.party_id);
                    PartyName temp0 = p ? p->name : stringish<PartyName>("None");
                    FString output;
                    if (pl_GM_level > 0)
                        output = STRPRINTF(
                                "Name: %s (GM:%d) | Party: '%s'",
                                pl_sd->status.name, pl_GM_level, temp0);
                    clif_displaymessage(fd, output);
                    count++;
                }
            }
        }
    }

    if (count == 0)
        clif_displaymessage(fd, "No player found.");
    else if (count == 1)
        clif_displaymessage(fd, "1 player found.");
    else
    {
        FString output = STRPRINTF("%d players found.", count);
        clif_displaymessage(fd, output);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_whomap(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int count;
    int pl_GM_level, GM_level;
    map_local *map_id;

    {
        MapName map_name;
        extract(message, &map_name);
        map_id = map_mapname2mapid(map_name);
        if (map_id == nullptr)
            map_id = sd->bl_m;
    }

    count = 0;
    GM_level = pc_isGM(sd);
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            pl_GM_level = pc_isGM(pl_sd);
            if (!
                ((battle_config.hide_GM_session
                  || bool(pl_sd->status.option & Option::HIDE))
                 && (pl_GM_level > GM_level)))
            {                   // you can look only lower or same level
                if (pl_sd->bl_m == map_id)
                {
                    FString output;
                    if (pl_GM_level > 0)
                        output = STRPRINTF(
                                "Name: %s (GM:%d) | Location: %s %d %d",
                                pl_sd->status.name, pl_GM_level,
                                pl_sd->mapname_, pl_sd->bl_x, pl_sd->bl_y);
                    else
                        output = STRPRINTF(
                                "Name: %s | Location: %s %d %d",
                                pl_sd->status.name, pl_sd->mapname_,
                                pl_sd->bl_x, pl_sd->bl_y);
                    clif_displaymessage(fd, output);
                    count++;
                }
            }
        }
    }

    FString output = STRPRINTF("%d players found in map '%s'.",
            count, map_id->name_);
    clif_displaymessage(fd, output);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_whomapgroup(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int count;
    int pl_GM_level, GM_level;
    struct party *p;

    map_local *map_id;
    {
        MapName map_name;
        extract(message, &map_name);
        map_id = map_mapname2mapid(map_name);
        if (map_id == nullptr)
            map_id = sd->bl_m;
    }

    count = 0;
    GM_level = pc_isGM(sd);
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            pl_GM_level = pc_isGM(pl_sd);
            if (!
                ((battle_config.hide_GM_session
                  || bool(pl_sd->status.option & Option::HIDE))
                 && (pl_GM_level > GM_level)))
            {
                // you can look only lower or same level
                if (pl_sd->bl_m == map_id)
                {
                    p = party_search(pl_sd->status.party_id);
                    PartyName temp0 = p ? p->name : stringish<PartyName>("None");
                    FString output;
                    if (pl_GM_level > 0)
                        output = STRPRINTF("Name: %s (GM:%d) | Party: '%s'",
                                pl_sd->status.name, pl_GM_level, temp0);
                    else
                        output = STRPRINTF("Name: %s | Party: '%s'",
                                pl_sd->status.name, temp0);
                    clif_displaymessage(fd, output);
                    count++;
                }
            }
        }
    }

    FString output;
    if (count == 0)
        output = STRPRINTF("No player found in map '%s'.", map_id->name_);
    else if (count == 1)
        output = STRPRINTF("1 player found in map '%s'.", map_id->name_);
    else
    {
        output = STRPRINTF("%d players found in map '%s'.", count, map_id->name_);
    }
    clif_displaymessage(fd, output);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_whogm(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int count;
    int pl_GM_level, GM_level;
    struct party *p;

    VString<23> match_text = message;
    match_text = match_text.to_lower();

    count = 0;
    GM_level = pc_isGM(sd);
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            pl_GM_level = pc_isGM(pl_sd);
            if (pl_GM_level > 0)
            {
                if (!
                    ((battle_config.hide_GM_session
                      || bool(pl_sd->status.option & Option::HIDE))
                     && (pl_GM_level > GM_level)))
                {
                    // you can look only lower or same level
                    VString<23> player_name = pl_sd->status.name.to__lower();
                    if (player_name.contains_seq(match_text))
                    {
                        // search with no case sensitive
                        FString output;
                        output = STRPRINTF(
                                "Name: %s (GM:%d) | Location: %s %d %d",
                                pl_sd->status.name, pl_GM_level,
                                pl_sd->mapname_, pl_sd->bl_x, pl_sd->bl_y);
                        clif_displaymessage(fd, output);
                        output = STRPRINTF(
                                "       BLvl: %d | Job: %s (Lvl: %d)",
                                pl_sd->status.base_level,
                                "Novice/Human",
                                pl_sd->status.job_level);
                        clif_displaymessage(fd, output);
                        p = party_search(pl_sd->status.party_id);
                        PartyName temp0 = p ? p->name : stringish<PartyName>("None");
                        output = STRPRINTF(
                                "       Party: '%s'",
                                temp0);
                        clif_displaymessage(fd, output);
                        count++;
                    }
                }
            }
        }
    }

    if (count == 0)
        clif_displaymessage(fd, "No GM found.");
    else if (count == 1)
        clif_displaymessage(fd, "1 GM found.");
    else
    {
        FString output = STRPRINTF("%d GMs found.", count);
        clif_displaymessage(fd, output);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_save(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    nullpo_retr(-1, sd);

    pc_setsavepoint(sd, sd->mapname_, sd->bl_x, sd->bl_y);
    pc_makesavestatus(sd);
    chrif_save(sd);
    clif_displaymessage(fd, "Character data respawn point saved.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_load(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    map_local *m = map_mapname2mapid(sd->status.save_point.map_);
    if (m != nullptr && m->flag.nowarpto
        && battle_config.any_warp_GM_min_level > pc_isGM(sd))
    {
        clif_displaymessage(fd,
                             "You are not authorised to warp you to your save map.");
        return -1;
    }
    if (sd->bl_m && sd->bl_m->flag.nowarp
        && battle_config.any_warp_GM_min_level > pc_isGM(sd))
    {
        clif_displaymessage(fd,
                             "You are not authorised to warp you from your actual map.");
        return -1;
    }

    pc_setpos(sd, sd->status.save_point.map_, sd->status.save_point.x,
               sd->status.save_point.y, BeingRemoveWhy::GONE);
    clif_displaymessage(fd, "Warping to respawn point.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_speed(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    if (!message)
    {
        FString output = STRPRINTF(
                "Please, enter a speed value (usage: @speed <%d-%d>).",
                static_cast<uint32_t>(MIN_WALK_SPEED.count()),
                static_cast<uint32_t>(MAX_WALK_SPEED.count()));
        clif_displaymessage(fd, output);
        return -1;
    }

    interval_t speed = static_cast<interval_t>(atoi(message.c_str()));
    if (speed >= MIN_WALK_SPEED && speed <= MAX_WALK_SPEED)
    {
        sd->speed = speed;
        //sd->walktimer = x;
        //この文を追加 by れ
        clif_updatestatus(sd, SP::SPEED);
        clif_displaymessage(fd, "Speed changed.");
    }
    else
    {
        FString output = STRPRINTF(
                "Please, enter a valid speed value (usage: @speed <%d-%d>).",
                static_cast<uint32_t>(MIN_WALK_SPEED.count()),
                static_cast<uint32_t>(MAX_WALK_SPEED.count()));
        clif_displaymessage(fd, output);
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_storage(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    struct storage *stor;       //changes from Freya/Yor
    nullpo_retr(-1, sd);

    if (sd->state.storage_open)
    {
        clif_displaymessage(fd, "msg_table[250]");
        return -1;
    }

    if ((stor = account2storage2(sd->status.account_id)) != NULL
        && stor->storage_status == 1)
    {
        clif_displaymessage(fd, "msg_table[250]");
        return -1;
    }

    storage_storageopen(sd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_option(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    nullpo_retr(-1, sd);

    Opt1 param1 = Opt1::ZERO;
    Opt2 param2 = Opt2::ZERO;
    Option param3 = Option::ZERO;

    if (!extract(message, record<',', 1>(&param1, &param2, &param3)))
    {
        clif_displaymessage(fd,
                "Please, enter at least a option (usage: @option <param1:0+> <param2:0+> <param3:0+>).");
        return -1;
    }

    sd->opt1 = param1;
    sd->opt2 = param2;
    sd->status.option = param3;

    clif_changeoption(sd);
    pc_calcstatus(sd, 0);
    clif_displaymessage(fd, "Options changed.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_hide(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    if (bool(sd->status.option & Option::HIDE))
    {
        sd->status.option &= ~Option::HIDE;
        clif_displaymessage(fd, "Invisible: Off.");    // Invisible: Off
    }
    else
    {
        sd->status.option |= Option::HIDE;
        clif_displaymessage(fd, "Invisible: On.");    // Invisible: On
    }
    clif_changeoption(sd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_die(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    pc_damage(NULL, sd, sd->status.hp + 1);
    clif_displaymessage(fd, "A pity! You've died.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_kill(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @kill <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can kill only lower or same level
            pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
            clif_displaymessage(fd, "Character killed.");
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_alive(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    sd->status.hp = sd->status.max_hp;
    sd->status.sp = sd->status.max_sp;
    pc_setstand(sd);
    if (static_cast<interval_t>(battle_config.pc_invincible_time) > interval_t::zero())
        pc_setinvincibletimer(sd, static_cast<interval_t>(battle_config.pc_invincible_time));
    clif_updatestatus(sd, SP::HP);
    clif_updatestatus(sd, SP::SP);
    clif_resurrection(sd, 1);
    clif_displaymessage(fd, "You've been revived! It's a miracle!");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_kami(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    if (!message)
    {
        clif_displaymessage(fd,
                "Please, enter a message (usage: @kami <message>).");
        return -1;
    }

    intif_GMmessage(message);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_heal(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int hp = 0, sp = 0;        // [Valaris] thanks to fov

    extract(message, record<' '>(&hp, &sp));

    if (hp == 0 && sp == 0)
    {
        hp = sd->status.max_hp - sd->status.hp;
        sp = sd->status.max_sp - sd->status.sp;
    }
    else
    {
        if (hp > 0 && (hp > sd->status.max_hp || hp > (sd->status.max_hp - sd->status.hp))) // fix positiv overflow
            hp = sd->status.max_hp - sd->status.hp;
        else if (hp < 0 && (hp < -sd->status.max_hp || hp < (1 - sd->status.hp)))   // fix negativ overflow
            hp = 1 - sd->status.hp;
        if (sp > 0 && (sp > sd->status.max_sp || sp > (sd->status.max_sp - sd->status.sp))) // fix positiv overflow
            sp = sd->status.max_sp - sd->status.sp;
        else if (sp < 0 && (sp < -sd->status.max_sp || sp < (1 - sd->status.sp)))   // fix negativ overflow
            sp = 1 - sd->status.sp;
    }

    if (hp < 0)            // display like damage
        clif_damage(sd, sd, gettick(), interval_t::zero(), interval_t::zero(), -hp, 0, DamageType::RETURNED, 0);

    if (hp != 0 || sp != 0)
    {
        pc_heal(sd, hp, sp);
        if (hp >= 0 && sp >= 0)
            clif_displaymessage(fd, "HP, SP recovered.");
        else
            clif_displaymessage(fd, "HP or/and SP modified.");
    }
    else
    {
        clif_displaymessage(fd, "HP and SP are already with the good value.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @item command (usage: @item <name/id_of_item> <quantity>)
 *------------------------------------------
 */
int atcommand_item(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    ItemName item_name;
    int number = 0, item_id;
    struct item_data *item_data;
    int get_count, i;

    if (!extract(message, record<' ', 1>(&item_name, &number)))
    {
        clif_displaymessage(fd,
                "Please, enter an item name/id (usage: @item <item name or ID> [quantity]).");
        return -1;
    }

    if (number <= 0)
        number = 1;

    item_id = 0;
    if ((item_data = itemdb_searchname(item_name)) != NULL ||
        (item_data = itemdb_exists(atoi(item_name.c_str()))) != NULL)
        item_id = item_data->nameid;

    if (item_id >= 500)
    {
        get_count = number;
        if (item_data->type == ItemType::WEAPON
            || item_data->type == ItemType::ARMOR
            || item_data->type == ItemType::_7
            || item_data->type == ItemType::_8)
        {
            get_count = 1;
        }
        for (i = 0; i < number; i += get_count)
        {
            struct item item_tmp {};
            item_tmp.nameid = item_id;
            item_tmp.identify = 1;
            PickupFail flag;
            if ((flag = pc_additem(sd, &item_tmp, get_count))
                != PickupFail::OKAY)
                clif_additem(sd, 0, 0, flag);
        }
        clif_displaymessage(fd, "Item created.");
    }
    else
    {
        clif_displaymessage(fd, "Invalid item ID or name.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_itemreset(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    int i;

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].amount
            && sd->status.inventory[i].equip == EPOS::ZERO)
            pc_delitem(sd, i, sd->status.inventory[i].amount, 0);
    }
    clif_displaymessage(fd, "All of your items have been removed.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_itemcheck(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    pc_checkitem(sd);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_baselevelup(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int level, i;

    if (!extract(message, &level) || !level)
    {
        clif_displaymessage(fd,
                "Please, enter a level adjustement (usage: @blvl <number of levels>).");
        return -1;
    }

    if (level > 0)
    {
        if (sd->status.base_level == battle_config.maximum_level)
        {                       // check for max level by Valaris
            clif_displaymessage(fd, "Base level can't go any higher.");
            return -1;
        }                       // End Addition
        if (level > battle_config.maximum_level || level > (battle_config.maximum_level - sd->status.base_level))   // fix positiv overflow
            level = battle_config.maximum_level - sd->status.base_level;
        for (i = 1; i <= level; i++)
            sd->status.status_point += (sd->status.base_level + i + 14) / 4;
        sd->status.base_level += level;
        clif_updatestatus(sd, SP::BASELEVEL);
        clif_updatestatus(sd, SP::NEXTBASEEXP);
        clif_updatestatus(sd, SP::STATUSPOINT);
        pc_calcstatus(sd, 0);
        pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
        clif_misceffect(sd, 0);
        clif_displaymessage(fd, "Base level raised.");
    }
    else
    {
        if (sd->status.base_level == 1)
        {
            clif_displaymessage(fd, "Base level can't go any lower.");
            return -1;
        }
        if (level < -battle_config.maximum_level || level < (1 - sd->status.base_level))    // fix negativ overflow
            level = 1 - sd->status.base_level;
        if (sd->status.status_point > 0)
        {
            for (i = 0; i > level; i--)
                sd->status.status_point -=
                    (sd->status.base_level + i + 14) / 4;
            if (sd->status.status_point < 0)
                sd->status.status_point = 0;
            clif_updatestatus(sd, SP::STATUSPOINT);
        }                       // to add: remove status points from stats
        sd->status.base_level += level;
        clif_updatestatus(sd, SP::BASELEVEL);
        clif_updatestatus(sd, SP::NEXTBASEEXP);
        pc_calcstatus(sd, 0);
        clif_displaymessage(fd, "Base level lowered.");
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// TODO: merge this with pc_setparam(SP::JOBLEVEL)
// then fix the funny 50 and/or 10 limitation.
int atcommand_joblevelup(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int up_level = 50, level;

    if (!extract(message, &level) || !level)
    {
        clif_displaymessage(fd,
                "Please, enter a level adjustement (usage: @jlvl <number of levels>).");
        return -1;
    }

    up_level -= 40;

    if (level > 0)
    {
        if (sd->status.job_level == up_level)
        {
            clif_displaymessage(fd, "Job level can't go any higher.");
            return -1;
        }
        if (level > up_level || level > (up_level - sd->status.job_level))  // fix positiv overflow
            level = up_level - sd->status.job_level;
        sd->status.job_level += level;
        clif_updatestatus(sd, SP::JOBLEVEL);
        clif_updatestatus(sd, SP::NEXTJOBEXP);
        sd->status.skill_point += level;
        clif_updatestatus(sd, SP::SKILLPOINT);
        pc_calcstatus(sd, 0);
        clif_misceffect(sd, 1);
        clif_displaymessage(fd, "Job level raised.");
    }
    else
    {
        if (sd->status.job_level == 1)
        {
            clif_displaymessage(fd, "Job level can't go any lower.");
            return -1;
        }
        if (level < -up_level || level < (1 - sd->status.job_level))    // fix negativ overflow
            level = 1 - sd->status.job_level;
        sd->status.job_level += level;
        clif_updatestatus(sd, SP::JOBLEVEL);
        clif_updatestatus(sd, SP::NEXTJOBEXP);
        if (sd->status.skill_point > 0)
        {
            sd->status.skill_point += level;
            if (sd->status.skill_point < 0)
                sd->status.skill_point = 0;
            clif_updatestatus(sd, SP::SKILLPOINT);
        }                       // to add: remove status points from skills
        pc_calcstatus(sd, 0);
        clif_displaymessage(fd, "Job level lowered.");
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_help(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    std::ifstream in(help_txt.c_str());
    if (in.is_open())
    {
        clif_displaymessage(fd, "Help commands:");
        int gm_level = pc_isGM(sd);
        FString line;
        while (io::getline(in, line))
        {
            SString w1;
            TString w2;
            if (!split_key_value(line, &w1, &w2))
                continue;
            int level;
            extract(w1, &level);
            if (gm_level >= level)
                clif_displaymessage(fd, w2);
        }
    }
    else
    {
        clif_displaymessage(fd, "File help.txt not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_gm(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    if (!message)
    {
        clif_displaymessage(fd,
                "Please, enter a password (usage: @gm <password>).");
        return -1;
    }

    if (pc_isGM(sd))
    {                           // a GM can not use this function. only a normal player (become gm is not for gm!)
        clif_displaymessage(fd, "You already have some GM powers.");
        return -1;
    }
    else
        chrif_changegm(sd->status.account_id, message);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_pvpoff(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    if (battle_config.pk_mode)
    {                           //disable command if server is in PK mode [Valaris]
        clif_displaymessage(fd, "This option cannot be used in PK Mode.");
        return -1;
    }

    if (sd->bl_m->flag.pvp)
    {
        sd->bl_m->flag.pvp = 0;
        for (int i = 0; i < fd_max; i++)
        {
            if (!session[i])
                continue;
            dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
            if (pl_sd && pl_sd->state.auth)
            {
                if (sd->bl_m == pl_sd->bl_m)
                {
                    pl_sd->pvp_timer.cancel();
                }
            }
        }
        clif_displaymessage(fd, "PvP: Off.");
    }
    else
    {
        clif_displaymessage(fd, "PvP is already Off.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_pvpon(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    if (battle_config.pk_mode)
    {                           //disable command if server is in PK mode [Valaris]
        clif_displaymessage(fd, "This option cannot be used in PK Mode.");
        return -1;
    }

    if (!sd->bl_m->flag.pvp && !sd->bl_m->flag.nopvp)
    {
        sd->bl_m->flag.pvp = 1;
        for (int i = 0; i < fd_max; i++)
        {
            if (!session[i])
                continue;
            dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
            if (pl_sd && pl_sd->state.auth)
            {
                if (sd->bl_m == pl_sd->bl_m && !pl_sd->pvp_timer)
                {
                    pl_sd->pvp_timer = Timer(gettick() + std::chrono::milliseconds(200),
                            std::bind(pc_calc_pvprank_timer, ph::_1, ph::_2, pl_sd->bl_id));
                    pl_sd->pvp_rank = 0;
                    pl_sd->pvp_lastusers = 0;
                    pl_sd->pvp_point = 5;
                }
            }
        }
        clif_displaymessage(fd, "PvP: On.");
    }
    else
    {
        clif_displaymessage(fd, "PvP is already On.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_model(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int hair_style = 0, hair_color = 0, cloth_color = 0;

    if (!extract(message, record<' ', 1>(&hair_style, &hair_color, &cloth_color)))
    {
        FString output = STRPRINTF(
                "Please, enter at least a value (usage: @model <hair ID: %d-%d> <hair color: %d-%d> <clothes color: %d-%d>).",
                MIN_HAIR_STYLE, MAX_HAIR_STYLE,
                MIN_HAIR_COLOR, MAX_HAIR_COLOR,
                MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);
        clif_displaymessage(fd, output);
        return -1;
    }

    if (hair_style >= MIN_HAIR_STYLE && hair_style <= MAX_HAIR_STYLE &&
        hair_color >= MIN_HAIR_COLOR && hair_color <= MAX_HAIR_COLOR &&
        cloth_color >= MIN_CLOTH_COLOR && cloth_color <= MAX_CLOTH_COLOR)
    {
        {
            pc_changelook(sd, LOOK::HAIR, hair_style);
            pc_changelook(sd, LOOK::HAIR_COLOR, hair_color);
            pc_changelook(sd, LOOK::CLOTHES_COLOR, cloth_color);
            clif_displaymessage(fd, "Appearence changed.");
        }
    }
    else
    {
        clif_displaymessage(fd, "An invalid number was specified.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @dye && @ccolor
 *------------------------------------------
 */
int atcommand_dye(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int cloth_color = 0;

    if (!extract(message, &cloth_color))
    {
        FString output = STRPRINTF(
                "Please, enter a clothes color (usage: @dye/@ccolor <clothes color: %d-%d>).",
                MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);
        clif_displaymessage(fd, output);
        return -1;
    }

    if (cloth_color >= MIN_CLOTH_COLOR && cloth_color <= MAX_CLOTH_COLOR)
    {
        {
            pc_changelook(sd, LOOK::CLOTHES_COLOR, cloth_color);
            clif_displaymessage(fd, "Appearence changed.");
        }
    }
    else
    {
        clif_displaymessage(fd, "An invalid number was specified.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @hairstyle && @hstyle
 *------------------------------------------
 */
int atcommand_hair_style(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int hair_style = 0;

    if (!extract(message, &hair_style))
    {
        FString output = STRPRINTF(
                "Please, enter a hair style (usage: @hairstyle/@hstyle <hair ID: %d-%d>).",
                MIN_HAIR_STYLE, MAX_HAIR_STYLE);
        clif_displaymessage(fd, output);
        return -1;
    }

    if (hair_style >= MIN_HAIR_STYLE && hair_style <= MAX_HAIR_STYLE)
    {
        {
            pc_changelook(sd, LOOK::HAIR, hair_style);
            clif_displaymessage(fd, "Appearence changed.");
        }
    }
    else
    {
        clif_displaymessage(fd, "An invalid number was specified.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @haircolor && @hcolor
 *------------------------------------------
 */
int atcommand_hair_color(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int hair_color = 0;

    if (!extract(message, &hair_color))
    {
        FString output = STRPRINTF(
                "Please, enter a hair color (usage: @haircolor/@hcolor <hair color: %d-%d>).",
                MIN_HAIR_COLOR, MAX_HAIR_COLOR);
        clif_displaymessage(fd, output);
        return -1;
    }

    if (hair_color >= MIN_HAIR_COLOR && hair_color <= MAX_HAIR_COLOR)
    {
        {
            pc_changelook(sd, LOOK::HAIR_COLOR, hair_color);
            clif_displaymessage(fd, "Appearence changed.");
        }
    }
    else
    {
        clif_displaymessage(fd, "An invalid number was specified.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_spawn(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MobName monster;
    int mob_id;
    int number = 0;
    int x = 0, y = 0;
    int count;
    int i, j, k;
    int mx, my, range;

    if (!extract(message, record<' ', 1>(&monster, &number, &x, &y)))
    {
        clif_displaymessage(fd, "Give a monster name/id please.");
        return -1;
    }

    // If monster identifier/name argument is a name
    if ((mob_id = mobdb_searchname(monster)) == 0) // check name first (to avoid possible name begining by a number)
        mob_id = mobdb_checkid(atoi(monster.c_str()));

    if (mob_id == 0)
    {
        clif_displaymessage(fd, "Invalid monster ID or name.");
        return -1;
    }

    if (number <= 0)
        number = 1;

    // If value of atcommand_spawn_quantity_limit directive is greater than or equal to 1 and quantity of monsters is greater than value of the directive
    if (battle_config.atc_spawn_quantity_limit >= 1
        && number > battle_config.atc_spawn_quantity_limit)
        number = battle_config.atc_spawn_quantity_limit;

    if (battle_config.etc_log)
        PRINTF("@spawn monster='%s' id=%d count=%d (%d,%d)\n",
                monster, mob_id, number, x, y);

    count = 0;
    range = sqrt(number) / 2;
    range = range * 2 + 5;      // calculation of an odd number (+ 4 area around)
    for (i = 0; i < number; i++)
    {
        j = 0;
        k = 0;
        while (j++ < 8 && k == 0)
        {
            // try 8 times to spawn the monster (needed for close area)
            if (x <= 0)
                mx = sd->bl_x + random_::in(-range / 2, range / 2 );
            else
                mx = x;
            if (y <= 0)
                my = sd->bl_y + random_::in(-range / 2, range / 2);
            else
                my = y;
            k = mob_once_spawn(sd, MOB_THIS_MAP, mx, my, MobName(), mob_id, 1, NpcEvent());
        }
        count += (k != 0) ? 1 : 0;
    }

    if (count != 0)
        if (number == count)
            clif_displaymessage(fd, "All monster summoned!");
        else
        {
            FString output = STRPRINTF("%d monster(s) summoned!",
                    count);
            clif_displaymessage(fd, output);
        }
    else
    {
        clif_displaymessage(fd, "Invalid monster ID or name.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void atcommand_killmonster_sub(const int fd, dumb_ptr<map_session_data> sd,
        ZString message, const int drop)
{
    map_local *map_id;
    {
        MapName map_name;
        extract(message, &map_name);
        map_id = map_mapname2mapid(map_name);
        if (map_id == nullptr)
            map_id = sd->bl_m;
    }

    map_foreachinarea(std::bind(atkillmonster_sub, ph::_1, drop),
            map_id,
            0, 0,
            map_id->xs, map_id->ys,
            BL::MOB);

    clif_displaymessage(fd, "All monsters killed!");
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_killmonster(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    atcommand_killmonster_sub(fd, sd, message, 1);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void atlist_nearby_sub(dumb_ptr<block_list> bl, int fd)
{
    nullpo_retv(bl);

    FString buf = STRPRINTF(" - \"%s\"",
            bl->as_player()->status.name);
    clif_displaymessage(fd, buf);
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_list_nearby(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    clif_displaymessage(fd, "Nearby players:");
    map_foreachinarea(std::bind(atlist_nearby_sub, ph::_1, fd),
            sd->bl_m,
            sd->bl_x - 1, sd->bl_y - 1,
            sd->bl_x + 1, sd->bl_x + 1,
            BL::PC);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_killmonster2(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    atcommand_killmonster_sub(fd, sd, message, 0);

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_gat(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    int y;

    for (y = 2; y >= -2; y--)
    {
        FString output = STRPRINTF(
                "%s (x= %d, y= %d) %02X %02X %02X %02X %02X",
                sd->bl_m->name_, sd->bl_x - 2, sd->bl_y + y,
                map_getcell(sd->bl_m, sd->bl_x - 2, sd->bl_y + y),
                map_getcell(sd->bl_m, sd->bl_x - 1, sd->bl_y + y),
                map_getcell(sd->bl_m, sd->bl_x, sd->bl_y + y),
                map_getcell(sd->bl_m, sd->bl_x + 1, sd->bl_y + y),
                map_getcell(sd->bl_m, sd->bl_x + 2, sd->bl_y + y));
        clif_displaymessage(fd, output);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_packet(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    StatusChange type {};
    int flag = 0;

    if (!extract(message, record<' '>(&type, &flag)))
    {
        clif_displaymessage(fd,
                "Please, enter a status type/flag (usage: @packet <status type> <flag>).");
        return -1;
    }

    clif_status_change(sd, type, flag);

    return 0;
}

/*==========================================
 * @stpoint (Rewritten by [Yor])
 *------------------------------------------
 */
int atcommand_statuspoint(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int point, new_status_point;

    if (!extract(message, &point) || point == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a number (usage: @stpoint <number of points>).");
        return -1;
    }

    new_status_point = sd->status.status_point + point;
    if (point > 0 && (point > 0x7FFF || new_status_point > 0x7FFF)) // fix positiv overflow
        new_status_point = 0x7FFF;
    else if (point < 0 && (point < -0x7FFF || new_status_point < 0))    // fix negativ overflow
        new_status_point = 0;

    if (new_status_point != sd->status.status_point)
    {
        sd->status.status_point = new_status_point;
        clif_updatestatus(sd, SP::STATUSPOINT);
        clif_displaymessage(fd, "Number of status points changed!");
    }
    else
    {
        if (point < 0)
            clif_displaymessage(fd, "Impossible to decrease the number/value.");
        else
            clif_displaymessage(fd, "Impossible to increase the number/value.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @skpoint (Rewritten by [Yor])
 *------------------------------------------
 */
int atcommand_skillpoint(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int point, new_skill_point;

    if (!extract(message, &point) || point == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a number (usage: @skpoint <number of points>).");
        return -1;
    }

    new_skill_point = sd->status.skill_point + point;
    if (point > 0 && (point > 0x7FFF || new_skill_point > 0x7FFF))  // fix positiv overflow
        new_skill_point = 0x7FFF;
    else if (point < 0 && (point < -0x7FFF || new_skill_point < 0)) // fix negativ overflow
        new_skill_point = 0;

    if (new_skill_point != sd->status.skill_point)
    {
        sd->status.skill_point = new_skill_point;
        clif_updatestatus(sd, SP::SKILLPOINT);
        clif_displaymessage(fd, "Number of skill points changed!");
    }
    else
    {
        if (point < 0)
            clif_displaymessage(fd, "Impossible to decrease the number/value.");
        else
            clif_displaymessage(fd, "Impossible to increase the number/value.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @zeny (Rewritten by [Yor])
 *------------------------------------------
 */
int atcommand_zeny(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int zeny, new_zeny;

    if (!extract(message, &zeny) || zeny == 0)
    {
        clif_displaymessage(fd,
                "Please, enter an amount (usage: @zeny <amount>).");
        return -1;
    }

    new_zeny = sd->status.zeny + zeny;
    if (zeny > 0 && (zeny > MAX_ZENY || new_zeny > MAX_ZENY))   // fix positiv overflow
        new_zeny = MAX_ZENY;
    else if (zeny < 0 && (zeny < -MAX_ZENY || new_zeny < 0))    // fix negativ overflow
        new_zeny = 0;

    if (new_zeny != sd->status.zeny)
    {
        sd->status.zeny = new_zeny;
        clif_updatestatus(sd, SP::ZENY);
        clif_displaymessage(fd, "Number of zenys changed!");
    }
    else
    {
        if (zeny < 0)
            clif_displaymessage(fd, "Impossible to decrease the number/value.");
        else
            clif_displaymessage(fd, "Impossible to increase the number/value.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
template<ATTR attr>
int atcommand_param(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int value = 0, new_value;

    if (!extract(message, &value)
        || value == 0)
    {
        // there was a clang bug here
        // fortunately, STRPRINTF was not actually needed
        clif_displaymessage(fd,
                "Please, enter a valid value (usage: @str,@agi,@vit,@int,@dex,@luk <+/-adjustement>).");
        return -1;
    }

    new_value = sd->status.attrs[attr] + value;
    if (value > 0 && (value > battle_config.max_parameter || new_value > battle_config.max_parameter))  // fix positiv overflow
        new_value = battle_config.max_parameter;
    else if (value < 0 && (value < -battle_config.max_parameter || new_value < 1))  // fix negativ overflow
        new_value = 1;

    if (new_value != sd->status.attrs[attr])
    {
        sd->status.attrs[attr] = new_value;
        clif_updatestatus(sd, attr_to_sp(attr));
        clif_updatestatus(sd, attr_to_usp(attr));
        pc_calcstatus(sd, 0);
        clif_displaymessage(fd, "Stat changed.");
    }
    else
    {
        if (value < 0)
            clif_displaymessage(fd, "Impossible to decrease the number/value.");
        else
            clif_displaymessage(fd, "Impossible to increase the number/value.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
//** Stat all by fritz (rewritten by [Yor])
int atcommand_all_stats(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int count, value = 0, new_value;

    if (!extract(message, &value)
        || value == 0)
        value = battle_config.max_parameter;

    count = 0;
    for (ATTR attr : ATTRs)
    {
        new_value = sd->status.attrs[attr] + value;
        if (value > 0 && (value > battle_config.max_parameter || new_value > battle_config.max_parameter))  // fix positiv overflow
            new_value = battle_config.max_parameter;
        else if (value < 0 && (value < -battle_config.max_parameter || new_value < 1))  // fix negativ overflow
            new_value = 1;

        if (new_value != sd->status.attrs[attr])
        {
            sd->status.attrs[attr] = new_value;
            clif_updatestatus(sd, attr_to_sp(attr));
            clif_updatestatus(sd, attr_to_usp(attr));
            pc_calcstatus(sd, 0);
            count++;
        }
    }

    if (count > 0)              // if at least 1 stat modified
        clif_displaymessage(fd, "All stats changed!");
    else
    {
        if (value < 0)
            clif_displaymessage(fd, "Impossible to decrease a stat.");
        else
            clif_displaymessage(fd, "Impossible to increase a stat.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_recall(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @recall <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can recall only lower or same level
            if (sd->bl_m && sd->bl_m->flag.nowarpto
                && battle_config.any_warp_GM_min_level > pc_isGM(sd))
            {
                clif_displaymessage(fd,
                        "You are not authorised to warp somenone to your actual map.");
                return -1;
            }
            if (pl_sd->bl_m && pl_sd->bl_m->flag.nowarp
                && battle_config.any_warp_GM_min_level > pc_isGM(sd))
            {
                clif_displaymessage(fd,
                        "You are not authorised to warp this player from its actual map.");
                return -1;
            }
            pc_setpos(pl_sd, sd->mapname_, sd->bl_x, sd->bl_y, BeingRemoveWhy::QUIT);
            FString output = STRPRINTF("%s recalled!", character);
            clif_displaymessage(fd, output);
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_revive(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @revive <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        pl_sd->status.hp = pl_sd->status.max_hp;
        pc_setstand(pl_sd);
        if (static_cast<interval_t>(battle_config.pc_invincible_time) > interval_t::zero())
            pc_setinvincibletimer(sd, static_cast<interval_t>(battle_config.pc_invincible_time));
        clif_updatestatus(pl_sd, SP::HP);
        clif_updatestatus(pl_sd, SP::SP);
        clif_resurrection(pl_sd, 1);
        clif_displaymessage(fd, "Character revived.");
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_character_stats(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charstats <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        FString output;
        output = STRPRINTF("'%s' stats:", pl_sd->status.name);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Base Level - %d", pl_sd->status.base_level),
        clif_displaymessage(fd, output);
        output = STRPRINTF("Job - Novice/Human (level %d)", pl_sd->status.job_level);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Hp - %d", pl_sd->status.hp);
        clif_displaymessage(fd, output);
        output = STRPRINTF("MaxHp - %d", pl_sd->status.max_hp);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Sp - %d", pl_sd->status.sp);
        clif_displaymessage(fd, output);
        output = STRPRINTF("MaxSp - %d", pl_sd->status.max_sp);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Str - %3d", pl_sd->status.attrs[ATTR::STR]);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Agi - %3d", pl_sd->status.attrs[ATTR::AGI]);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Vit - %3d", pl_sd->status.attrs[ATTR::VIT]);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Int - %3d", pl_sd->status.attrs[ATTR::INT]);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Dex - %3d", pl_sd->status.attrs[ATTR::DEX]);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Luk - %3d", pl_sd->status.attrs[ATTR::LUK]);
        clif_displaymessage(fd, output);
        output = STRPRINTF("Zeny - %d", pl_sd->status.zeny);
        clif_displaymessage(fd, output);
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
//** Character Stats All by fritz
int atcommand_character_stats_all(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    int count;

    count = 0;
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            FString gmlevel;
            if (pc_isGM(pl_sd) > 0)
                gmlevel = STRPRINTF("| GM Lvl: %d", pc_isGM(pl_sd));
            else
                gmlevel = " ";

            FString output;
            output = STRPRINTF(
                    "Name: %s | BLvl: %d | Job: Novice/Human (Lvl: %d) | HP: %d/%d | SP: %d/%d",
                    pl_sd->status.name, pl_sd->status.base_level,
                    pl_sd->status.job_level,
                    pl_sd->status.hp, pl_sd->status.max_hp,
                    pl_sd->status.sp, pl_sd->status.max_sp);
            clif_displaymessage(fd, output);
            output = STRPRINTF("STR: %d | AGI: %d | VIT: %d | INT: %d | DEX: %d | LUK: %d | Zeny: %d %s",
                     pl_sd->status.attrs[ATTR::STR],
                     pl_sd->status.attrs[ATTR::AGI],
                     pl_sd->status.attrs[ATTR::VIT],
                     pl_sd->status.attrs[ATTR::INT],
                     pl_sd->status.attrs[ATTR::DEX],
                     pl_sd->status.attrs[ATTR::LUK],
                     pl_sd->status.zeny,
                     gmlevel);
            clif_displaymessage(fd, output);
            clif_displaymessage(fd, "--------");
            count++;
        }
    }

    if (count == 0)
        clif_displaymessage(fd, "No player found.");
    else if (count == 1)
        clif_displaymessage(fd, "1 player found.");
    else
    {
        FString output = STRPRINTF("%d players found.", count);
        clif_displaymessage(fd, output);
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_character_option(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    Opt1 opt1;
    Opt2 opt2;
    Option opt3;
    CharName character;
    if (!asplit(message, &opt1, &opt2, &opt3, &character))
    {
        clif_displaymessage(fd,
                "Please, enter valid options and a player name (usage: @charoption <param1> <param2> <param3> <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {
            // you can change option only to lower or same level
            pl_sd->opt1 = opt1;
            pl_sd->opt2 = opt2;
            pl_sd->status.option = opt3;

            clif_changeoption(pl_sd);
            pc_calcstatus(pl_sd, 0);
            clif_displaymessage(fd, "Character's options changed.");
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * charchangesex command (usage: charchangesex <player_name>)
 *------------------------------------------
 */
int atcommand_char_change_sex(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charchangesex <name>).");
        return -1;
    }

    {
        chrif_char_ask_name(sd->status.account_id, character, 5, HumanTimeDiff());    // type: 5 - changesex
        clif_displaymessage(fd, "Character name sends to char-server to ask it.");
    }

    return 0;
}

/*==========================================
 * charblock command (usage: charblock <player_name>)
 * This command do a definitiv ban on a player
 *------------------------------------------
 */
int atcommand_char_block(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @block <name>).");
        return -1;
    }

    {
        chrif_char_ask_name(sd->status.account_id, character, 1, HumanTimeDiff());    // type: 1 - block
        clif_displaymessage(fd, "Character name sends to char-server to ask it.");
    }

    return 0;
}

/*==========================================
 * charban command (usage: charban <time> <player_name>)
 * This command do a limited ban on a player
 * Time is done as follows:
 *   Adjustment value (-1, 1, +1, etc...)
 *   Modified element:
 *     a or y: year
 *     m:  month
 *     j or d: day
 *     h:  hour
 *     mn: minute
 *     s:  second
 * <example> @ban +1m-2mn1s-6y test_player
 *           this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
 *------------------------------------------
 */
int atcommand_char_ban(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    HumanTimeDiff modif;
    CharName character;

    if (!asplit(message, &modif, &character)
        || !modif)
    {
        clif_displaymessage(fd,
                "Please, enter ban time and a player name (usage: @charban/@ban/@banish/@charbanish <time> <name>).");
        return -1;
    }

    {
        chrif_char_ask_name(sd->status.account_id, character, 2, modif);  // type: 2 - ban
        clif_displaymessage(fd, "Character name sends to char-server to ask it.");
    }

    return 0;
}

/*==========================================
 * charunblock command (usage: charunblock <player_name>)
 *------------------------------------------
 */
int atcommand_char_unblock(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charunblock <player_name>).");
        return -1;
    }

    {
        // send answer to login server via char-server
        chrif_char_ask_name(sd->status.account_id, character, 3, HumanTimeDiff());    // type: 3 - unblock
        clif_displaymessage(fd, "Character name sends to char-server to ask it.");
    }

    return 0;
}

/*==========================================
 * charunban command (usage: charunban <player_name>)
 *------------------------------------------
 */
int atcommand_char_unban(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charunban <player_name>).");
        return -1;
    }

    {
        // send answer to login server via char-server
        chrif_char_ask_name(sd->status.account_id, character, 4, HumanTimeDiff());    // type: 4 - unban
        clif_displaymessage(fd, "Character name sends to char-server to ask it.");
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_character_save(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MapName map_name;
    CharName character;
    int x = 0, y = 0;

    if (!asplit(message, &map_name, &x, &y, &character)
        || x < 0 || y < 0)
    {
        clif_displaymessage(fd,
                "Please, enter a valid save point and a player name (usage: @charsave <map> <x> <y> <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {
            // you can change save point only to lower or same gm level
            map_local *m = map_mapname2mapid(map_name);
            if (m == nullptr)
            {
                clif_displaymessage(fd, "Map not found.");
                return -1;
            }
            else
            {
                if (m != nullptr && m->flag.nowarpto
                    && battle_config.any_warp_GM_min_level > pc_isGM(sd))
                {
                    clif_displaymessage(fd,
                            "You are not authorised to set this map as a save map.");
                    return -1;
                }
                pc_setsavepoint(pl_sd, map_name, x, y);
                clif_displaymessage(fd, "Character's respawn point changed.");
            }
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_doom(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd
            && pl_sd->state.auth && i != fd
            && pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can doom only lower or same gm level
            pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
            clif_displaymessage(pl_sd->fd, "The holy messenger has given judgement.");
        }
    }
    clif_displaymessage(fd, "Judgement was made.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_doommap(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd
            && pl_sd->state.auth && i != fd && sd->bl_m == pl_sd->bl_m
            && pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can doom only lower or same gm level
            pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
            clif_displaymessage(pl_sd->fd, "The holy messenger has given judgement.");
        }
    }
    clif_displaymessage(fd, "Judgement was made.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void atcommand_raise_sub(dumb_ptr<map_session_data> sd)
{
    if (sd && sd->state.auth && pc_isdead(sd))
    {
        sd->status.hp = sd->status.max_hp;
        sd->status.sp = sd->status.max_sp;
        pc_setstand(sd);
        clif_updatestatus(sd, SP::HP);
        clif_updatestatus(sd, SP::SP);
        clif_resurrection(sd, 1);
        clif_displaymessage(sd->fd, "Mercy has been shown.");
    }
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_raise(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        atcommand_raise_sub(pl_sd);
    }
    clif_displaymessage(fd, "Mercy has been granted.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_raisemap(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd
            && pl_sd->state.auth && sd->bl_m == pl_sd->bl_m)
            atcommand_raise_sub(pl_sd);
    }
    clif_displaymessage(fd, "Mercy has been granted.");

    return 0;
}

/*==========================================
 * atcommand_character_baselevel @charbaselvlで対象キャラのレベルを上げる
 *------------------------------------------
*/
int atcommand_character_baselevel(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;
    int level = 0, i;

    if (!asplit(message, &level, &character)
        || level == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a level adjustement and a player name (usage: @charblvl <#> <nickname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can change base level only lower or same gm level
            if (level > 0)
            {
                if (pl_sd->status.base_level == battle_config.maximum_level)
                {               // check for max level by Valaris
                    clif_displaymessage(fd, "Character's base level can't go any higher.");
                    return 0;
                }               // End Addition
                if (level > battle_config.maximum_level || level > (battle_config.maximum_level - pl_sd->status.base_level))    // fix positiv overflow
                    level =
                        battle_config.maximum_level -
                        pl_sd->status.base_level;
                for (i = 1; i <= level; i++)
                    pl_sd->status.status_point +=
                        (pl_sd->status.base_level + i + 14) / 4;
                pl_sd->status.base_level += level;
                clif_updatestatus(pl_sd, SP::BASELEVEL);
                clif_updatestatus(pl_sd, SP::NEXTBASEEXP);
                clif_updatestatus(pl_sd, SP::STATUSPOINT);
                pc_calcstatus(pl_sd, 0);
                pc_heal(pl_sd, pl_sd->status.max_hp, pl_sd->status.max_sp);
                clif_misceffect(pl_sd, 0);
                clif_displaymessage(fd, "Character's base level raised.");
            }
            else
            {
                if (pl_sd->status.base_level == 1)
                {
                    clif_displaymessage(fd, "Character's base level can't go any lower.");
                    return -1;
                }
                if (level < -battle_config.maximum_level || level < (1 - pl_sd->status.base_level)) // fix negativ overflow
                    level = 1 - pl_sd->status.base_level;
                if (pl_sd->status.status_point > 0)
                {
                    for (i = 0; i > level; i--)
                        pl_sd->status.status_point -=
                            (pl_sd->status.base_level + i + 14) / 4;
                    if (pl_sd->status.status_point < 0)
                        pl_sd->status.status_point = 0;
                    clif_updatestatus(pl_sd, SP::STATUSPOINT);
                }               // to add: remove status points from stats
                pl_sd->status.base_level += level;
                pl_sd->status.base_exp = 0;
                clif_updatestatus(pl_sd, SP::BASELEVEL);
                clif_updatestatus(pl_sd, SP::NEXTBASEEXP);
                clif_updatestatus(pl_sd, SP::BASEEXP);
                pc_calcstatus(pl_sd, 0);
                clif_displaymessage(fd, "Character's base level lowered.");
            }
            // Reset their stat points to prevent extra points from stacking
            atcommand_charstreset(fd, sd, character.to__actual());
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;                   //正常終了
}

/*==========================================
 * atcommand_character_joblevel @charjoblvlで対象キャラのJobレベルを上げる
 *------------------------------------------
 */
int atcommand_character_joblevel(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;
    int max_level = 50, level = 0;

    if (!asplit(message, &level, &character)
        || level == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a level adjustement and a player name (usage: @charjlvl <#> <nickname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {
            // you can change job level only lower or same gm level
            max_level -= 40;

            if (level > 0)
            {
                if (pl_sd->status.job_level == max_level)
                {
                    clif_displaymessage(fd, "Character's job level can't go any higher.");
                    return -1;
                }
                if (pl_sd->status.job_level + level > max_level)
                    level = max_level - pl_sd->status.job_level;
                pl_sd->status.job_level += level;
                clif_updatestatus(pl_sd, SP::JOBLEVEL);
                clif_updatestatus(pl_sd, SP::NEXTJOBEXP);
                pl_sd->status.skill_point += level;
                clif_updatestatus(pl_sd, SP::SKILLPOINT);
                pc_calcstatus(pl_sd, 0);
                clif_misceffect(pl_sd, 1);
                clif_displaymessage(fd, "character's job level raised.");
            }
            else
            {
                if (pl_sd->status.job_level == 1)
                {
                    clif_displaymessage(fd, "Character's job level can't go any lower.");
                    return -1;
                }
                if (pl_sd->status.job_level + level < 1)
                    level = 1 - pl_sd->status.job_level;
                pl_sd->status.job_level += level;
                clif_updatestatus(pl_sd, SP::JOBLEVEL);
                clif_updatestatus(pl_sd, SP::NEXTJOBEXP);
                if (pl_sd->status.skill_point > 0)
                {
                    pl_sd->status.skill_point += level;
                    if (pl_sd->status.skill_point < 0)
                        pl_sd->status.skill_point = 0;
                    clif_updatestatus(pl_sd, SP::SKILLPOINT);
                }               // to add: remove status points from skills
                pc_calcstatus(pl_sd, 0);
                clif_displaymessage(fd, "Character's job level lowered.");
            }
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_kick(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @kick <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))    // you can kick only lower or same gm level
            clif_GM_kick(sd, pl_sd, 1);
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_kickall(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd
            && pl_sd->state.auth && pc_isGM(sd) >= pc_isGM(pl_sd))
        {
            // you can kick only lower or same gm level
            if (sd->status.account_id != pl_sd->status.account_id)
                clif_GM_kick(sd, pl_sd, 0);
        }
    }

    clif_displaymessage(fd, "All players have been kicked!");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_questskill(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    SkillID skill_id;

    if (!extract(message, &skill_id))
    {
        clif_displaymessage(fd,
                "Please, enter a quest skill number (usage: @questskill <#:0+>).");
        return -1;
    }

    if (/*skill_id >= SkillID() &&*/ skill_id < SkillID::MAX_SKILL_DB)
    {
        if (skill_get_inf2(skill_id) & 0x01)
        {
            if (pc_checkskill(sd, skill_id) == 0)
            {
                pc_skill(sd, skill_id, 1, 0);
                clif_displaymessage(fd, "You have learned the skill.");
            }
            else
            {
                clif_displaymessage(fd, "You already have this quest skill.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "This skill number doesn't exist or isn't a quest skill.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "This skill number doesn't exist.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_charquestskill(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    SkillID skill_id;

    if (!asplit(message, &skill_id, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a quest skill number and a player name (usage: @charquestskill <#:0+> <char_name>).");
        return -1;
    }

    if (/*skill_id >= SkillID() &&*/ skill_id < SkillID::MAX_SKILL_DB)
    {
        if (skill_get_inf2(skill_id) & 0x01)
        {
            dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
            if (pl_sd != NULL)
            {
                if (pc_checkskill(pl_sd, skill_id) == 0)
                {
                    pc_skill(pl_sd, skill_id, 1, 0);
                    clif_displaymessage(fd, "This player has learned the skill.");
                }
                else
                {
                    clif_displaymessage(fd, "This player already has this quest skill.");
                    return -1;
                }
            }
            else
            {
                clif_displaymessage(fd, "Character not found.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "This skill number doesn't exist or isn't a quest skill.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "This skill number doesn't exist.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_lostskill(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    SkillID skill_id;

    if (!extract(message, &skill_id))
    {
        clif_displaymessage(fd,
                "Please, enter a quest skill number (usage: @lostskill <#:0+>).");
        return -1;
    }

    if (/*skill_id >= SkillID() &&*/ skill_id < MAX_SKILL)
    {
        if (skill_get_inf2(skill_id) & 0x01)
        {
            if (pc_checkskill(sd, skill_id) > 0)
            {
                sd->status.skill[skill_id].lv = 0;
                sd->status.skill[skill_id].flags = SkillFlags::ZERO;
                clif_skillinfoblock(sd);
                clif_displaymessage(fd, "You have forgotten the skill.");
            }
            else
            {
                clif_displaymessage(fd, "You don't have this quest skill.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "This skill number doesn't exist or isn't a quest skill.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "This skill number doesn't exist.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_charlostskill(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    SkillID skill_id;

    if (!asplit(message, &skill_id, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a quest skill number and a player name (usage: @charlostskill <#:0+> <char_name>).");
        return -1;
    }

    if (/*skill_id >= SkillID() &&*/ skill_id < MAX_SKILL)
    {
        if (skill_get_inf2(skill_id) & 0x01)
        {
            dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
            if (pl_sd != NULL)
            {
                if (pc_checkskill(pl_sd, skill_id) > 0)
                {
                    pl_sd->status.skill[skill_id].lv = 0;
                    pl_sd->status.skill[skill_id].flags = SkillFlags::ZERO;
                    clif_skillinfoblock(pl_sd);
                    clif_displaymessage(fd, "This player has forgotten the skill.");
                }
                else
                {
                    clif_displaymessage(fd, "This player doesn't have this quest skill.");
                    return -1;
                }
            }
            else
            {
                clif_displaymessage(fd, "Character not found.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "This skill number doesn't exist or isn't a quest skill.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "This skill number doesn't exist.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_party(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    PartyName party;

    if (!extract(message, &party) || !party)
    {
        clif_displaymessage(fd,
                "Please, enter a party name (usage: @party <party_name>).");
        return -1;
    }

    party_create(sd, party);

    return 0;
}

/*==========================================
 * @mapexitでマップサーバーを終了させる
 *------------------------------------------
 */
int atcommand_mapexit(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            if (sd->status.account_id != pl_sd->status.account_id)
                clif_GM_kick(sd, pl_sd, 0);
        }
    }
    clif_GM_kick(sd, sd, 0);

    runflag = 0;

    return 0;
}

/*==========================================
 * idsearch <part_of_name>: revrited by [Yor]
 *------------------------------------------
 */
int atcommand_idsearch(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    ItemName item_name;
    int i, match;
    struct item_data *item;

    if (!extract(message, &item_name) || !item_name)
    {
        clif_displaymessage(fd,
                "Please, enter a part of item name (usage: @idsearch <part_of_item_name>).");
        return -1;
    }

    FString output = STRPRINTF("The reference result of '%s' (name: id):", item_name);
    clif_displaymessage(fd, output);
    match = 0;
    for (i = 0; i < 20000; i++)
    {
        if ((item = itemdb_exists(i)) != NULL
            && item->jname.contains_seq(item_name))
        {
            match++;
            output = STRPRINTF("%s: %d", item->jname, item->nameid);
            clif_displaymessage(fd, output);
        }
    }
    output = STRPRINTF("It is %d affair above.", match);
    clif_displaymessage(fd, output);

    return 0;
}

/*==========================================
 * Character Skill Reset
 *------------------------------------------
 */
int atcommand_charskreset(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charskreset <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can reset skill points only lower or same gm level
            pc_resetskill(pl_sd);
            FString output = STRPRINTF(
                    "'%s' skill points reseted!", character);
            clif_displaymessage(fd, output);
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Stat Reset
 *------------------------------------------
 */
int atcommand_charstreset(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charstreset <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can reset stats points only lower or same gm level
            pc_resetstate(pl_sd);
            FString output = STRPRINTF(
                    "'%s' stats points reseted!",
                    character);
            clif_displaymessage(fd, output);
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Reset
 *------------------------------------------
 */
int atcommand_charreset(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charreset <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can reset a character only for lower or same GM level
            pc_resetstate(pl_sd);
            pc_resetskill(pl_sd);
            pc_setglobalreg(pl_sd, stringish<VarName>("MAGIC_FLAGS"), 0);  // [Fate] Reset magic quest variables
            pc_setglobalreg(pl_sd, stringish<VarName>("MAGIC_EXP"), 0);    // [Fate] Reset magic experience
            FString output = STRPRINTF(
                    "'%s' skill and stats points reseted!", character);
            clif_displaymessage(fd, output);
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Wipe
 *------------------------------------------
 */
int atcommand_char_wipe(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charwipe <charname>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can reset a character only for lower or same GM level
            int i;

            // Reset base level
            pl_sd->status.base_level = 1;
            pl_sd->status.base_exp = 0;
            clif_updatestatus(pl_sd, SP::BASELEVEL);
            clif_updatestatus(pl_sd, SP::NEXTBASEEXP);
            clif_updatestatus(pl_sd, SP::BASEEXP);

            // Reset job level
            pl_sd->status.job_level = 1;
            pl_sd->status.job_exp = 0;
            clif_updatestatus(pl_sd, SP::JOBLEVEL);
            clif_updatestatus(pl_sd, SP::NEXTJOBEXP);
            clif_updatestatus(pl_sd, SP::JOBEXP);

            // Zeny to 50
            pl_sd->status.zeny = 50;
            clif_updatestatus(pl_sd, SP::ZENY);

            // Clear inventory
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                if (sd->status.inventory[i].amount)
                {
                    if (bool(sd->status.inventory[i].equip))
                        pc_unequipitem(pl_sd, i, CalcStatus::NOW);
                    pc_delitem(pl_sd, i, sd->status.inventory[i].amount, 0);
                }
            }

            // Give knife and shirt
            struct item item;
            item.nameid = 1201; // knife
            item.identify = 1;
            item.broken = 0;
            pc_additem(pl_sd, &item, 1);
            item.nameid = 1202; // shirt
            pc_additem(pl_sd, &item, 1);

            // Reset stats and skills
            pc_calcstatus(pl_sd, 0);
            pc_resetstate(pl_sd);
            pc_resetskill(pl_sd);
            pc_setglobalreg(pl_sd, stringish<VarName>("MAGIC_FLAGS"), 0);  // [Fate] Reset magic quest variables
            pc_setglobalreg(pl_sd, stringish<VarName>("MAGIC_EXP"), 0);    // [Fate] Reset magic experience

            FString output = STRPRINTF("%s:  wiped.", character);
            clif_displaymessage(fd, output);
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Model by chbrules
 *------------------------------------------
 */
int atcommand_charmodel(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    unsigned hair_style = 0, hair_color = 0, cloth_color = 0;
    CharName character;

    if (!asplit(message, &hair_style, &hair_color, &cloth_color, &character))
    {
        FString output = STRPRINTF(
                "Please, enter a valid model and a player name (usage: @charmodel <hair ID: %d-%d> <hair color: %d-%d> <clothes color: %d-%d> <name>).",
                MIN_HAIR_STYLE, MAX_HAIR_STYLE,
                MIN_HAIR_COLOR, MAX_HAIR_COLOR,
                MIN_CLOTH_COLOR, MAX_CLOTH_COLOR);
        clif_displaymessage(fd, output);
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (hair_style >= MIN_HAIR_STYLE && hair_style <= MAX_HAIR_STYLE &&
            hair_color >= MIN_HAIR_COLOR && hair_color <= MAX_HAIR_COLOR &&
            cloth_color >= MIN_CLOTH_COLOR && cloth_color <= MAX_CLOTH_COLOR)
        {
            {
                pc_changelook(pl_sd, LOOK::HAIR, hair_style);
                pc_changelook(pl_sd, LOOK::HAIR_COLOR, hair_color);
                pc_changelook(pl_sd, LOOK::CLOTHES_COLOR, cloth_color);
                clif_displaymessage(fd, "Appearence changed.");
            }
        }
        else
        {
            clif_displaymessage(fd, "An invalid number was specified.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Skill Point (Rewritten by [Yor])
 *------------------------------------------
 */
int atcommand_charskpoint(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    int new_skill_point;
    int point = 0;

    if (!asplit(message, &point, &character)
        || point == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a number and a player name (usage: @charskpoint <amount> <name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        new_skill_point = pl_sd->status.skill_point + point;
        if (point > 0 && (point > 0x7FFF || new_skill_point > 0x7FFF))  // fix positiv overflow
            new_skill_point = 0x7FFF;
        else if (point < 0 && (point < -0x7FFF || new_skill_point < 0)) // fix negativ overflow
            new_skill_point = 0;
        if (new_skill_point != pl_sd->status.skill_point)
        {
            pl_sd->status.skill_point = new_skill_point;
            clif_updatestatus(pl_sd, SP::SKILLPOINT);
            clif_displaymessage(fd, "Character's number of skill points changed!");
        }
        else
        {
            if (point < 0)
                clif_displaymessage(fd, "Impossible to decrease the number/value.");
            else
                clif_displaymessage(fd, "Impossible to increase the number/value.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Status Point (rewritten by [Yor])
 *------------------------------------------
 */
int atcommand_charstpoint(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    int new_status_point;
    int point = 0;

    if (!asplit(message, &point, &character)
        || point == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a number and a player name (usage: @charstpoint <amount> <name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        new_status_point = pl_sd->status.status_point + point;
        if (point > 0 && (point > 0x7FFF || new_status_point > 0x7FFF)) // fix positiv overflow
            new_status_point = 0x7FFF;
        else if (point < 0 && (point < -0x7FFF || new_status_point < 0))    // fix negativ overflow
            new_status_point = 0;
        if (new_status_point != pl_sd->status.status_point)
        {
            pl_sd->status.status_point = new_status_point;
            clif_updatestatus(pl_sd, SP::STATUSPOINT);
            clif_displaymessage(fd, "Character's number of status points changed!");
        }
        else
        {
            if (point < 0)
                clif_displaymessage(fd, "Impossible to decrease the number/value.");
            else
                clif_displaymessage(fd, "Impossible to increase the number/value.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Character Zeny Point (Rewritten by [Yor])
 *------------------------------------------
 */
int atcommand_charzeny(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    int zeny = 0, new_zeny;

    if (!asplit(message, &zeny, &character) || zeny == 0)
    {
        clif_displaymessage(fd,
                "Please, enter a number and a player name (usage: @charzeny <zeny> <name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        new_zeny = pl_sd->status.zeny + zeny;
        if (zeny > 0 && (zeny > MAX_ZENY || new_zeny > MAX_ZENY))   // fix positiv overflow
            new_zeny = MAX_ZENY;
        else if (zeny < 0 && (zeny < -MAX_ZENY || new_zeny < 0))    // fix negativ overflow
            new_zeny = 0;
        if (new_zeny != pl_sd->status.zeny)
        {
            pl_sd->status.zeny = new_zeny;
            clif_updatestatus(pl_sd, SP::ZENY);
            clif_displaymessage(fd, "Character's number of zenys changed!");
        }
        else
        {
            if (zeny < 0)
                clif_displaymessage(fd, "Impossible to decrease the number/value.");
            else
                clif_displaymessage(fd, "Impossible to increase the number/value.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * Recall All Characters Online To Your Location
 *------------------------------------------
 */
int atcommand_recallall(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    int count;

    if (sd->bl_m && sd->bl_m->flag.nowarpto
        && battle_config.any_warp_GM_min_level > pc_isGM(sd))
    {
        clif_displaymessage(fd,
                "You are not authorised to warp somenone to your actual map.");
        return -1;
    }

    count = 0;
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd
            && pl_sd->state.auth
            && sd->status.account_id != pl_sd->status.account_id
            && pc_isGM(sd) >= pc_isGM(pl_sd))
        {
            // you can recall only lower or same level
            if (pl_sd->bl_m && pl_sd->bl_m->flag.nowarp
                && battle_config.any_warp_GM_min_level > pc_isGM(sd))
                count++;
            else
                pc_setpos(pl_sd, sd->mapname_, sd->bl_x, sd->bl_y, BeingRemoveWhy::QUIT);
        }
    }

    clif_displaymessage(fd, "All characters recalled!");
    if (count)
    {
        FString output = STRPRINTF(
                "Because you are not authorised to warp from some maps, %d player(s) have not been recalled.",
                count);
        clif_displaymessage(fd, output);
    }

    return 0;
}

/*==========================================
 * Recall online characters of a party to your location
 *------------------------------------------
 */
int atcommand_partyrecall(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    PartyName party_name;
    struct party *p;
    int count;

    if (!extract(message, &party_name) || !party_name)
    {
        clif_displaymessage(fd,
                "Please, enter a party name/id (usage: @partyrecall <party_name/id>).");
        return -1;
    }

    if (sd->bl_m && sd->bl_m->flag.nowarpto
        && battle_config.any_warp_GM_min_level > pc_isGM(sd))
    {
        clif_displaymessage(fd,
                "You are not authorised to warp somenone to your actual map.");
        return -1;
    }

    if ((p = party_searchname(party_name)) != NULL ||  // name first to avoid error when name begin with a number
        (p = party_search(atoi(message.c_str()))) != NULL)
    {
        count = 0;
        for (int i = 0; i < fd_max; i++)
        {
            if (!session[i])
                continue;
            dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
            if (pl_sd && pl_sd->state.auth
                && sd->status.account_id != pl_sd->status.account_id
                && pl_sd->status.party_id == p->party_id)
            {
                if (pl_sd->bl_m && pl_sd->bl_m->flag.nowarp
                    && battle_config.any_warp_GM_min_level > pc_isGM(sd))
                    count++;
                else
                    pc_setpos(pl_sd, sd->mapname_, sd->bl_x, sd->bl_y, BeingRemoveWhy::QUIT);
            }
        }
        FString output = STRPRINTF("All online characters of the %s party are near you.", p->name);
        clif_displaymessage(fd, output);
        if (count)
        {
            output = STRPRINTF(
                    "Because you are not authorised to warp from some maps, %d player(s) have not been recalled.",
                    count);
            clif_displaymessage(fd, output);
        }
    }
    else
    {
        clif_displaymessage(fd, "Incorrect name or ID, or no one from the party is online.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_reloaditemdb(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    itemdb_reload();
    clif_displaymessage(fd, "Item database reloaded.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_reloadmobdb(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    mob_reload();
    clif_displaymessage(fd, "Monster database reloaded.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_reloadskilldb(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    skill_reload();
    clif_displaymessage(fd, "Skill database reloaded.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_reloadscript(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    do_init_npc();
    do_init_script();

    npc_event_do_oninit();

    clif_displaymessage(fd, "Scripts reloaded.");

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_reloadgmdb(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    chrif_reloadGMdb();

    clif_displaymessage(fd, "Login-server asked to reload GM accounts and their level.");

    return 0;
}

/*==========================================
 * @mapinfo <map name> [0-3] by MC_Cameri
 * => Shows information about the map [map name]
 * 0 = no additional information
 * 1 = Show users in that map and their location
 * 2 = Shows NPCs in that map
 *------------------------------------------
 */
int atcommand_mapinfo(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    dumb_ptr<npc_data> nd = NULL;
    MapName map_name;
    const char *direction = NULL;
    int list = 0;

    extract(message, record<' '>(&list, &map_name));

    if (list < 0 || list > 3)
    {
        clif_displaymessage(fd,
                "Please, enter at least a valid list number (usage: @mapinfo <0-3> [map]).");
        return -1;
    }

    if (!map_name)
        map_name = sd->mapname_;

    map_local *m_id = map_mapname2mapid(map_name);
    if (m_id != nullptr)
    {
        clif_displaymessage(fd, "Map not found.");
        return -1;
    }

    clif_displaymessage(fd, "------ Map Info ------");
    FString output = STRPRINTF("Map Name: %s", map_name);
    clif_displaymessage(fd, output);
    output = STRPRINTF("Players In Map: %d", m_id->users);
    clif_displaymessage(fd, output);
    output = STRPRINTF("NPCs In Map: %d", m_id->npc_num);
    clif_displaymessage(fd, output);
    clif_displaymessage(fd, "------ Map Flags ------");
    output = STRPRINTF("Player vs Player: %s | No Party: %s",
             (m_id->flag.pvp) ? "True" : "False",
             (m_id->flag.pvp_noparty) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Dead Branch: %s",
             (m_id->flag.nobranch) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Memo: %s",
             (m_id->flag.nomemo) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Penalty: %s",
             (m_id->flag.nopenalty) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Return: %s",
             (m_id->flag.noreturn) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Save: %s",
             (m_id->flag.nosave) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Teleport: %s",
             (m_id->flag.noteleport) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Monster Teleport: %s",
             (m_id->flag.monster_noteleport) ? "True" : "False");
    clif_displaymessage(fd, output);
    output = STRPRINTF("No Zeny Penalty: %s",
             (m_id->flag.nozenypenalty) ? "True" : "False");
    clif_displaymessage(fd, output);

    switch (list)
    {
        case 0:
            // Do nothing. It's list 0, no additional display.
            break;
        case 1:
            clif_displaymessage(fd, "----- Players in Map -----");
            for (int i = 0; i < fd_max; i++)
            {
                if (!session[i])
                    continue;
                dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
                if (pl_sd && pl_sd->state.auth
                    && pl_sd->mapname_ == map_name)
                {
                    output = STRPRINTF(
                            "Player '%s' (session #%d) | Location: %d,%d",
                            pl_sd->status.name, i, pl_sd->bl_x, pl_sd->bl_y);
                    clif_displaymessage(fd, output);
                }
            }
            break;
        case 2:
            clif_displaymessage(fd, "----- NPCs in Map -----");
            for (int i = 0; i < m_id->npc_num;)
            {
                nd = m_id->npc[i];
                switch (nd->dir)
                {
                    case DIR::S:
                        direction = "North";
                        break;
                    case DIR::SW:
                        direction = "North West";
                        break;
                    case DIR::W:
                        direction = "West";
                        break;
                    case DIR::NW:
                        direction = "South West";
                        break;
                    case DIR::N:
                        direction = "South";
                        break;
                    case DIR::NE:
                        direction = "South East";
                        break;
                    case DIR::E:
                        direction = "East";
                        break;
                    case DIR::SE:
                        direction = "North East";
                        break;
#if 0
                    case 9:
                        direction = "North";
                        break;
#endif
                    default:
                        direction = "Unknown";
                        break;
                }
                output = STRPRINTF(
                         "NPC %d: %s | Direction: %s | Sprite: %d | Location: %d %d",
                         ++i, nd->name, direction, nd->npc_class, nd->bl_x,
                         nd->bl_y);
                clif_displaymessage(fd, output);
            }
            break;
        default:               // normally impossible to arrive here
            clif_displaymessage(fd,
                    "Please, enter at least a valid list number (usage: @mapinfo <0-2> [map]).");
            return -1;
    }

    return 0;
}

/*==========================================
 *Spy Commands by Syrus22
 *------------------------------------------
 */
int atcommand_partyspy(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    PartyName party_name;

    if (!extract(message, &party_name))
    {
        clif_displaymessage(fd,
                "Please, enter a party name/id (usage: @partyspy <party_name/id>).");
        return -1;
    }

    struct party *p;
    if ((p = party_searchname(party_name)) != NULL ||  // name first to avoid error when name begin with a number
        (p = party_search(atoi(message.c_str()))) != NULL)
    {
        if (sd->partyspy == p->party_id)
        {
            sd->partyspy = 0;
            FString output = STRPRINTF("No longer spying on the %s party.", p->name);
            clif_displaymessage(fd, output);
        }
        else
        {
            sd->partyspy = p->party_id;
            FString output = STRPRINTF("Spying on the %s party.", p->name);
            clif_displaymessage(fd, output);
        }
    }
    else
    {
        clif_displaymessage(fd, "Incorrect name or ID, or no one from the party is online.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_enablenpc(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    NpcName NPCname;

    if (!extract(message, &NPCname) || !NPCname)
    {
        clif_displaymessage(fd,
                "Please, enter a NPC name (usage: @npcon <NPC_name>).");
        return -1;
    }

    if (npc_name2id(NPCname) != NULL)
    {
        npc_enable(NPCname, 1);
        clif_displaymessage(fd, "Npc Enabled.");
    }
    else
    {
        clif_displaymessage(fd, "This NPC doesn't exist.");
        return -1;
    }

    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_disablenpc(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    NpcName NPCname;

    if (!extract(message, &NPCname) || !NPCname)
    {
        clif_displaymessage(fd,
                "Please, enter a NPC name (usage: @npcoff <NPC_name>).");
        return -1;
    }

    if (npc_name2id(NPCname) != NULL)
    {
        npc_enable(NPCname, 0);
        clif_displaymessage(fd, "Npc Disabled.");
    }
    else
    {
        clif_displaymessage(fd, "This NPC doesn't exist.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @time/@date/@server_date/@serverdate/@server_time/@servertime: Display the date/time of the server (by [Yor]
 * Calculation management of GM modification (@day/@night GM commands) is done
 *------------------------------------------
 */
int atcommand_servertime(const int fd, dumb_ptr<map_session_data>,
        ZString)
{
    timestamp_seconds_buffer tsbuf;
    stamp_time(tsbuf);
    FString temp = STRPRINTF("Server time: %s", tsbuf);
    clif_displaymessage(fd, temp);

    {
        if (0 == 0)
            clif_displaymessage(fd, "Game time: The game is in permanent daylight.");
    }

    return 0;
}

/*==========================================
 * @chardelitem <item_name_or_ID> <quantity> <player> (by [Yor]
 * removes <quantity> item from a character
 * item can be equiped or not.
 * Inspired from a old command created by RoVeRT
 *------------------------------------------
 */
int atcommand_chardelitem(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;
    ItemName item_name;
    int i, number = 0, item_id, item_position, count;
    struct item_data *item_data;

    if (!asplit(message, &item_name, &number, &character) || number < 1)
    {
        clif_displaymessage(fd,
                "Please, enter an item name/id, a quantity and a player name (usage: @chardelitem <item_name_or_ID> <quantity> <player>).");
        return -1;
    }

    item_id = 0;
    if ((item_data = itemdb_searchname(item_name)) != NULL ||
        (item_data = itemdb_exists(atoi(item_name.c_str()))) != NULL)
        item_id = item_data->nameid;

    if (item_id > 500)
    {
        dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
        if (pl_sd != NULL)
        {
            if (pc_isGM(sd) >= pc_isGM(pl_sd))
            {                   // you can kill only lower or same level
                item_position = pc_search_inventory(pl_sd, item_id);
                if (item_position >= 0)
                {
                    count = 0;
                    for (i = 0; i < number && item_position >= 0; i++)
                    {
                        pc_delitem(pl_sd, item_position, 1, 0);
                        count++;
                        item_position = pc_search_inventory(pl_sd, item_id);   // for next loop
                    }
                    FString output = STRPRINTF(
                            "%d item(s) removed by a GM.",
                            count);
                    clif_displaymessage(pl_sd->fd, output);

                    if (number == count)
                        output = STRPRINTF("%d item(s) removed from the player.", count);
                    else
                        output = STRPRINTF("%d item(s) removed. Player had only %d on %d items.", count, count, number);
                    clif_displaymessage(fd, output);
                }
                else
                {
                    clif_displaymessage(fd, "Character does not have the item.");
                    return -1;
                }
            }
            else
            {
                clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "Character not found.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Invalid item ID or name.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @broadcast by [Valaris]
 *------------------------------------------
 */
int atcommand_broadcast(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    if (!message)
    {
        clif_displaymessage(fd,
                "Please, enter a message (usage: @broadcast <message>).");
        return -1;
    }

    FString output = STRPRINTF("%s : %s", sd->status.name, message);
    intif_GMmessage(output);

    return 0;
}

/*==========================================
 * @localbroadcast by [Valaris]
 *------------------------------------------
 */
int atcommand_localbroadcast(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    if (!message)
    {
        clif_displaymessage(fd,
                "Please, enter a message (usage: @localbroadcast <message>).");
        return -1;
    }

    FString output = STRPRINTF("%s : %s", sd->status.name, message);

    clif_GMmessage(sd, output, 1);   // 1: ALL_SAMEMAP

    return 0;
}

/*==========================================
 * @email <actual@email> <new@email> by [Yor]
 *------------------------------------------
 */
int atcommand_email(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    AccountEmail actual_email;
    AccountEmail new_email;

    if (!extract(message, record<' '>(&actual_email, &new_email)))
    {
        clif_displaymessage(fd,
                "Please enter 2 emails (usage: @email <actual@email> <new@email>).");
        return -1;
    }

    if (!e_mail_check(actual_email))
    {
        clif_displaymessage(fd, "Invalid actual email. If you have default e-mail, type a@a.com.");   // Invalid actual email. If you have default e-mail, give a@a.com.
        return -1;
    }
    else if (!e_mail_check(new_email))
    {
        clif_displaymessage(fd, "Invalid new email. Please enter a real e-mail.");
        return -1;
    }
    else if (new_email == DEFAULT_EMAIL)
    {
        clif_displaymessage(fd, "New email must be a real e-mail.");
        return -1;
    }
    else if (actual_email == new_email)
    {
        clif_displaymessage(fd, "New email must be different of the actual e-mail.");
        return -1;
    }
    else
    {
        chrif_changeemail(sd->status.account_id, actual_email, new_email);
        clif_displaymessage(fd, "Information sended to login-server via char-server.");
    }

    return 0;
}

/*==========================================
 *@effect
 *------------------------------------------
 */
int atcommand_effect(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    int type = 0, flag = 0;

    if (!extract(message, record<' '>(&type, &flag)))
    {
        clif_displaymessage(fd,
                "Please, enter at least a option (usage: @effect <type+>).");
        return -1;
    }
    if (flag <= 0)
    {
        clif_specialeffect(sd, type, flag);
        clif_displaymessage(fd, "Your Effect Has Changed.");   // Your effect has changed.
    }
    else
    {
        for (int i = 0; i < fd_max; i++)
        {
            if (!session[i])
                continue;
            dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
            if (pl_sd && pl_sd->state.auth)
            {
                clif_specialeffect(pl_sd, type, flag);
                clif_displaymessage(pl_sd->fd, "Your Effect Has Changed.");    // Your effect has changed.
            }
        }
    }

    return 0;
}

/*==========================================
 * @charitemlist <character>: Displays the list of a player's items.
 *------------------------------------------
 */
int atcommand_character_item_list(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    struct item_data *item_data, *item_temp;
    int i, j, count, counter, counter2;
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charitemlist <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can look items only lower or same level
            counter = 0;
            count = 0;
            for (i = 0; i < MAX_INVENTORY; i++)
            {
                if (pl_sd->status.inventory[i].nameid > 0
                    && (item_data =
                        itemdb_search(pl_sd->status.inventory[i].nameid)) !=
                    NULL)
                {
                    counter = counter + pl_sd->status.inventory[i].amount;
                    count++;
                    if (count == 1)
                    {
                        FString output = STRPRINTF(
                                "------ Items list of '%s' ------",
                                pl_sd->status.name);
                        clif_displaymessage(fd, output);
                    }
                    EPOS equip = pl_sd->status.inventory[i].equip;
                    MString equipstr;
                    if (bool(equip))
                    {
                        equipstr += "| equiped: ";
                        if (bool(equip & EPOS::GLOVES))
                            equipstr += "robe/gargment, ";
                        if (bool(equip & EPOS::CAPE))
                            equipstr += "left accessory, ";
                        if (bool(equip & EPOS::MISC1))
                            equipstr += "body/armor, ";
                        if ((equip & (EPOS::WEAPON | EPOS::SHIELD)) == EPOS::WEAPON)
                            equipstr += "right hand, ";
                        if ((equip & (EPOS::WEAPON | EPOS::SHIELD)) == EPOS::SHIELD)
                            equipstr += "left hand, ";
                        if ((equip & (EPOS::WEAPON | EPOS::SHIELD)) == (EPOS::WEAPON | EPOS::SHIELD))
                            equipstr += "both hands, ";
                        if (bool(equip & EPOS::SHOES))
                            equipstr += "feet, ";
                        if (bool(equip & EPOS::MISC2))
                            equipstr += "right accessory, ";
                        if ((equip & (EPOS::TORSO | EPOS::HAT | EPOS::LEGS)) == EPOS::LEGS)
                            equipstr += "lower head, ";
                        if ((equip & (EPOS::TORSO | EPOS::HAT | EPOS::LEGS)) == EPOS::HAT)
                            equipstr += "top head, ";
                        if ((equip & (EPOS::TORSO | EPOS::HAT | EPOS::LEGS)) == (EPOS::HAT | EPOS::LEGS))
                            equipstr += "lower/top head, ";
                        if ((equip & (EPOS::TORSO | EPOS::HAT | EPOS::LEGS)) == EPOS::TORSO)
                            equipstr += "mid head, ";
                        if ((equip & (EPOS::TORSO | EPOS::HAT | EPOS::LEGS)) == (EPOS::TORSO | EPOS::LEGS))
                            equipstr += "lower/mid head, ";
                        if ((equip & (EPOS::TORSO | EPOS::HAT | EPOS::LEGS)) == (EPOS::TORSO | EPOS::HAT | EPOS::LEGS))
                            equipstr += "lower/mid/top head, ";
                        // remove final ', '
                        equipstr.pop_back(2);
                    }
                    else
                        equipstr = MString();

                    FString output;
                    if (sd->status.inventory[i].refine)
                        output = STRPRINTF("%d %s %+d (%s %+d, id: %d) %s",
                                 pl_sd->status.inventory[i].amount,
                                 item_data->name,
                                 pl_sd->status.inventory[i].refine,
                                 item_data->jname,
                                 pl_sd->status.inventory[i].refine,
                                 pl_sd->status.inventory[i].nameid,
                                 FString(equipstr));
                    else
                        output = STRPRINTF("%d %s (%s, id: %d) %s",
                                 pl_sd->status.inventory[i].amount,
                                 item_data->name, item_data->jname,
                                 pl_sd->status.inventory[i].nameid,
                                 FString(equipstr));
                    clif_displaymessage(fd, output);

                    MString voutput;
                    counter2 = 0;
                    for (j = 0; j < item_data->slot; j++)
                    {
                        if (pl_sd->status.inventory[i].card[j])
                        {
                            if ((item_temp =
                                 itemdb_search(pl_sd->status.
                                                inventory[i].card[j])) !=
                                NULL)
                            {
                                if (!voutput)
                                    voutput += STRPRINTF(
                                            " -> (card(s): "
                                            "#%d %s (%s), ",
                                            ++counter2,
                                            item_temp->name,
                                            item_temp->jname);
                                else
                                    voutput += STRPRINTF(
                                            "#%d %s (%s), ",
                                            ++counter2,
                                            item_temp->name,
                                            item_temp->jname);
                            }
                        }
                    }
                    if (voutput)
                    {
                        // replace trailing ", "
                        voutput.pop_back();
                        voutput.back() = ')';
                        clif_displaymessage(fd, FString(voutput));
                    }
                }
            }
            if (count == 0)
                clif_displaymessage(fd, "No item found on this player.");
            else
            {
                FString output = STRPRINTF(
                        "%d item(s) found in %d kind(s) of items.",
                        counter, count);
                clif_displaymessage(fd, output);
            }
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @charstoragelist <character>: Displays the items list of a player's storage.
 *------------------------------------------
 */
int atcommand_character_storage_list(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    struct storage *stor;
    struct item_data *item_data, *item_temp;
    int i, j, count, counter, counter2;
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charitemlist <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can look items only lower or same level
            if ((stor = account2storage2(pl_sd->status.account_id)) != NULL)
            {
                counter = 0;
                count = 0;
                for (i = 0; i < MAX_STORAGE; i++)
                {
                    if (stor->storage_[i].nameid > 0
                        && (item_data =
                            itemdb_search(stor->storage_[i].nameid)) != NULL)
                    {
                        counter = counter + stor->storage_[i].amount;
                        count++;
                        if (count == 1)
                        {
                            FString output = STRPRINTF(
                                    "------ Storage items list of '%s' ------",
                                    pl_sd->status.name);
                            clif_displaymessage(fd, output);
                        }
                        FString output;
                        if (stor->storage_[i].refine)
                            output = STRPRINTF("%d %s %+d (%s %+d, id: %d)",
                                     stor->storage_[i].amount,
                                     item_data->name,
                                     stor->storage_[i].refine,
                                     item_data->jname,
                                     stor->storage_[i].refine,
                                     stor->storage_[i].nameid);
                        else
                            output = STRPRINTF("%d %s (%s, id: %d)",
                                     stor->storage_[i].amount,
                                     item_data->name, item_data->jname,
                                     stor->storage_[i].nameid);
                        clif_displaymessage(fd, output);

                        MString voutput;
                        counter2 = 0;
                        for (j = 0; j < item_data->slot; j++)
                        {
                            if (stor->storage_[i].card[j])
                            {
                                if ((item_temp =
                                     itemdb_search(stor->
                                                    storage_[i].card[j])) !=
                                    NULL)
                                {
                                    if (!voutput)
                                        voutput += STRPRINTF(
                                                " -> (card(s): "
                                                "#%d %s (%s), ",
                                                ++counter2,
                                                item_temp->name,
                                                item_temp->jname);
                                    else
                                        voutput += STRPRINTF(
                                                "#%d %s (%s), ",
                                                ++counter2,
                                                item_temp->name,
                                                item_temp->jname);
                                }
                            }
                        }
                        if (voutput)
                        {
                            // replace last ", "
                            voutput.pop_back();
                            voutput.back() = ')';
                            clif_displaymessage(fd, FString(voutput));
                        }
                    }
                }
                if (count == 0)
                    clif_displaymessage(fd,
                            "No item found in the storage of this player.");
                else
                {
                    FString output = STRPRINTF(
                            "%d item(s) found in %d kind(s) of items.",
                            counter, count);
                    clif_displaymessage(fd, output);
                }
            }
            else
            {
                clif_displaymessage(fd, "This player has no storage.");
                return -1;
            }
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @charcartlist <character>: Displays the items list of a player's cart.
 *------------------------------------------
 */
int atcommand_character_cart_list(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    struct item_data *item_data, *item_temp;
    int i, j, count, counter, counter2;
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd,
                "Please, enter a player name (usage: @charitemlist <char name>).");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can look items only lower or same level
            counter = 0;
            count = 0;
            for (i = 0; i < MAX_CART; i++)
            {
                if (pl_sd->status.cart[i].nameid > 0
                    && (item_data =
                        itemdb_search(pl_sd->status.cart[i].nameid)) != NULL)
                {
                    counter = counter + pl_sd->status.cart[i].amount;
                    count++;
                    if (count == 1)
                    {
                        FString output = STRPRINTF(
                                "------ Cart items list of '%s' ------",
                                pl_sd->status.name);
                        clif_displaymessage(fd, output);
                    }

                    FString output;
                    if (pl_sd->status.cart[i].refine)
                        output = STRPRINTF("%d %s %+d (%s %+d, id: %d)",
                                pl_sd->status.cart[i].amount,
                                item_data->name,
                                pl_sd->status.cart[i].refine,
                                item_data->jname,
                                pl_sd->status.cart[i].refine,
                                pl_sd->status.cart[i].nameid);
                    else

                        output = STRPRINTF("%d %s (%s, id: %d)",
                                pl_sd->status.cart[i].amount,
                                item_data->name, item_data->jname,
                                pl_sd->status.cart[i].nameid);
                    clif_displaymessage(fd, output);

                    MString voutput;
                    counter2 = 0;
                    for (j = 0; j < item_data->slot; j++)
                    {
                        if (pl_sd->status.cart[i].card[j])
                        {
                            if ((item_temp =
                                 itemdb_search(pl_sd->status.
                                                cart[i].card[j])) != NULL)
                            {
                                if (!voutput)
                                    voutput += STRPRINTF(
                                            " -> (card(s): "
                                            "#%d %s (%s), ",
                                            ++counter2,
                                            item_temp->name,
                                            item_temp->jname);
                                else
                                    voutput += STRPRINTF(
                                            "#%d %s (%s), ",
                                            ++counter2,
                                            item_temp->name,
                                            item_temp->jname);
                            }
                        }
                    }
                    if (voutput)
                    {
                        voutput.pop_back();
                        voutput.back() = '0';
                        clif_displaymessage(fd, FString(voutput));
                    }
                }
            }
            if (count == 0)
                clif_displaymessage(fd,
                        "No item found in the cart of this player.");
            else
            {
                FString output = STRPRINTF("%d item(s) found in %d kind(s) of items.",
                         counter, count);
                clif_displaymessage(fd, output);
            }
        }
        else
        {
            clif_displaymessage(fd, "Your GM level don't authorise you to do this action on this player.");
            return -1;
        }
    }
    else
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    return 0;
}

/*==========================================
 * @killer by MouseJstr
 * enable killing players even when not in pvp
 *------------------------------------------
 */
int atcommand_killer(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    sd->special_state.killer = !sd->special_state.killer;

    if (sd->special_state.killer)
        clif_displaymessage(fd, "You be a killa...");
    else
        clif_displaymessage(fd, "You gonna be own3d...");

    return 0;
}

/*==========================================
 * @killable by MouseJstr
 * enable other people killing you
 *------------------------------------------
 */
int atcommand_killable(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    sd->special_state.killable = !sd->special_state.killable;

    if (sd->special_state.killable)
        clif_displaymessage(fd, "You gonna be own3d...");
    else
        clif_displaymessage(fd, "You be a killa...");

    return 0;
}

/*==========================================
 * @charkillable by MouseJstr
 * enable another player to be killed
 *------------------------------------------
 */
int atcommand_charkillable(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
        return -1;

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd == NULL)
        return -1;

    pl_sd->special_state.killable = !pl_sd->special_state.killable;

    if (pl_sd->special_state.killable)
        clif_displaymessage(fd, "The player is now killable");
    else
        clif_displaymessage(fd, "The player is no longer killable");

    return 0;
}

/*==========================================
 * @npcmove by MouseJstr
 *
 * move a npc
 *------------------------------------------
 */
int atcommand_npcmove(const int, dumb_ptr<map_session_data> sd,
        ZString message)
{
    NpcName character;
    int x = 0, y = 0;
    dumb_ptr<npc_data> nd = 0;

    if (sd == NULL)
        return -1;

    if (!asplit(message, &x, &y, &character))
        return -1;

    nd = npc_name2id(character);
    if (nd == NULL)
        return -1;

    npc_enable(character, 0);
    nd->bl_x = x;
    nd->bl_y = y;
    npc_enable(character, 1);

    return 0;
}

/*==========================================
 * @addwarp by MouseJstr
 *
 * Create a new static warp point.
 *------------------------------------------
 */
int atcommand_addwarp(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MapName mapname;
    int x, y;

    if (!extract(message, record<' '>(&mapname, &x, &y)))
        return -1;

    FString w1 = STRPRINTF("%s,%d,%d", sd->mapname_, sd->bl_x, sd->bl_y);
    FString w3 = STRPRINTF("%s%d%d%d%d", mapname, sd->bl_x, sd->bl_y, x, y);
    FString w4 = STRPRINTF("1,1,%s.gat,%d,%d", mapname, x, y);

    NpcName w3name = stringish<NpcName>(w3);
    int ret = npc_parse_warp(w1, ZString("warp"), w3name, w4);

    FString output = STRPRINTF("New warp NPC => %s", w3);
    clif_displaymessage(fd, output);

    return ret;
}

/*==========================================
 * @chareffect by [MouseJstr]
 *
 * Create a effect localized on another character
 *------------------------------------------
 */
int atcommand_chareffect(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName target;
    int type = 0;

    if (!asplit(message, &type, &target))
    {
        clif_displaymessage(fd, "usage: @chareffect <type+> <target>.");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(target);
    if (pl_sd == NULL)
        return -1;

    clif_specialeffect(pl_sd, type, 0);
    clif_displaymessage(fd, "Your Effect Has Changed.");   // Your effect has changed.

    return 0;
}

/*==========================================
 * @dropall by [MouseJstr]
 *
 * Drop all your possession on the ground
 *------------------------------------------
 */
int atcommand_dropall(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    int i;
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].amount)
        {
            if (bool(sd->status.inventory[i].equip))
                pc_unequipitem(sd, i, CalcStatus::NOW);
            pc_dropitem(sd, i, sd->status.inventory[i].amount);
        }
    }
    return 0;
}

/*==========================================
 * @chardropall by [MouseJstr]
 *
 * Throw all the characters possessions on the ground.  Normally
 * done in response to them being disrespectful of a GM
 *------------------------------------------
 */
int atcommand_chardropall(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
        return -1;
    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd == NULL)
        return -1;
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        if (pl_sd->status.inventory[i].amount)
        {
            if (bool(pl_sd->status.inventory[i].equip))
                pc_unequipitem(pl_sd, i, CalcStatus::NOW);
            pc_dropitem(pl_sd, i, pl_sd->status.inventory[i].amount);
        }
    }

    clif_displaymessage(pl_sd->fd, "Ever play 52 card pickup?");
    clif_displaymessage(fd, "It is done");
    //clif_displaymessage(fd, "It is offical.. your a jerk");

    return 0;
}

/*==========================================
 * @storeall by [MouseJstr]
 *
 * Put everything into storage to simplify your inventory to make
 * debugging easie
 *------------------------------------------
 */
int atcommand_storeall(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    int i;
    nullpo_retr(-1, sd);

    if (!sd->state.storage_open)
    {                           //Open storage.
        switch (storage_storageopen(sd))
        {
            case 2:            //Try again
                clif_displaymessage(fd, "run this command again..");
                return 0;
            case 1:            //Failure
                clif_displaymessage(fd,
                        "You can't open the storage currently.");
                return 1;
        }
    }
    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (sd->status.inventory[i].amount)
        {
            if (bool(sd->status.inventory[i].equip))
                pc_unequipitem(sd, i, CalcStatus::NOW);
            storage_storageadd(sd, i, sd->status.inventory[i].amount);
        }
    }
    storage_storageclose(sd);

    clif_displaymessage(fd, "It is done");
    return 0;
}

/*==========================================
 * @charstoreall by [MouseJstr]
 *
 * A way to screw with players who piss you off
 *------------------------------------------
 */
int atcommand_charstoreall(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
        return -1;
    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd == NULL)
        return -1;

    if (storage_storageopen(pl_sd) == 1)
    {
        clif_displaymessage(fd,
                "Had to open the characters storage window...");
        clif_displaymessage(fd, "run this command again..");
        return 0;
    }
    for (int i = 0; i < MAX_INVENTORY; i++)
    {
        if (pl_sd->status.inventory[i].amount)
        {
            if (bool(pl_sd->status.inventory[i].equip))
                pc_unequipitem(pl_sd, i, CalcStatus::NOW);
            storage_storageadd(pl_sd, i, sd->status.inventory[i].amount);
        }
    }
    storage_storageclose(pl_sd);

    clif_displaymessage(pl_sd->fd,
            "Everything you own has been put away for safe keeping.");
    clif_displaymessage(pl_sd->fd,
            "go to the nearest kafka to retrieve it..");
    clif_displaymessage(pl_sd->fd, "   -- the management");

    clif_displaymessage(fd, "It is done");

    return 0;
}

/*==========================================
 * It is made to rain.
 *------------------------------------------
 */
int atcommand_rain(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    int effno = 0;
    effno = 161;
    nullpo_retr(-1, sd);
    if (effno < 0 || sd->bl_m->flag.rain)
        return -1;

    sd->bl_m->flag.rain = 1;
    clif_specialeffect(sd, effno, 2);
    return 0;
}

/*==========================================
 * It is made to snow.
 *------------------------------------------
 */
int atcommand_snow(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    int effno = 0;
    effno = 162;
    nullpo_retr(-1, sd);
    if (effno < 0 || sd->bl_m->flag.snow)
        return -1;

    sd->bl_m->flag.snow = 1;
    clif_specialeffect(sd, effno, 2);
    return 0;
}

/*==========================================
 * Cherry tree snowstorm is made to fall. (Sakura)
 *------------------------------------------
 */
int atcommand_sakura(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    int effno = 0;
    effno = 163;
    nullpo_retr(-1, sd);
    if (effno < 0 || sd->bl_m->flag.sakura)
        return -1;

    sd->bl_m->flag.sakura = 1;
    clif_specialeffect(sd, effno, 2);
    return 0;
}

/*==========================================
 * Fog hangs over.
 *------------------------------------------
 */
int atcommand_fog(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    int effno = 0;
    effno = 233;
    nullpo_retr(-1, sd);
    if (effno < 0 || sd->bl_m->flag.fog)
        return -1;

    sd->bl_m->flag.fog = 1;
    clif_specialeffect(sd, effno, 2);

    return 0;
}

/*==========================================
 * Fallen leaves fall.
 *------------------------------------------
 */
int atcommand_leaves(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    int effno = 0;
    effno = 333;
    nullpo_retr(-1, sd);
    if (effno < 0 || sd->bl_m->flag.leaves)
        return -1;

    sd->bl_m->flag.leaves = 1;
    clif_specialeffect(sd, effno, 2);
    return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_summon(const int, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MobName name;
    int mob_id = 0;
    int x = 0;
    int y = 0;
    int id = 0;
    tick_t tick = gettick();

    nullpo_retr(-1, sd);

    if (!extract(message, &name) || !name)
        return -1;

    if ((mob_id = atoi(name.c_str())) == 0)
        mob_id = mobdb_searchname(name);
    if (mob_id == 0)
        return -1;

    x = sd->bl_x + random_::in(-5, 4);
    y = sd->bl_y + random_::in(-5, 4);

    id = mob_once_spawn(sd, MOB_THIS_MAP, x, y, JAPANESE_NAME, mob_id, 1, NpcEvent());
    dumb_ptr<mob_data> md = map_id_as_mob(id);
    if (md)
    {
        md->master_id = sd->bl_id;
        md->state.special_mob_ai = 1;
        md->mode = mob_db[md->mob_class].mode | MobMode::AGGRESSIVE;
        md->deletetimer = Timer(tick + std::chrono::minutes(1),
                std::bind(mob_timer_delete, ph::_1, ph::_2,
                    id));
        clif_misceffect(md, 344);
    }

    return 0;
}

/*==========================================
 * @adjcmdlvl by [MouseJstr]
 *
 * Temp adjust the GM level required to use a GM command
 *
 * Used during beta testing to allow players to use GM commands
 * for short periods of time
 *------------------------------------------
 */
int atcommand_adjcmdlvl(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    int newlev;
    XString cmd;

    if (!extract(message, record<' '>(&newlev, &cmd)))
    {
        clif_displaymessage(fd, "usage: @adjcmdlvl <lvl> <command>.");
        return -1;
    }

    for (int i = 0; atcommand_info[i].command; i++)
        if (cmd == atcommand_info[i].command.xslice_t(1))
        {
            atcommand_info[i].level = newlev;
            clif_displaymessage(fd, "@command level changed.");
            return 0;
        }

    clif_displaymessage(fd, "@command not found.");
    return -1;
}

/*==========================================
 * @adjgmlvl by [MouseJstr]
 *
 * Create a temp GM
 *
 * Used during beta testing to allow players to use GM commands
 * for short periods of time
 *------------------------------------------
 */
int atcommand_adjgmlvl(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    int newlev;
    CharName user;

    if (!asplit(message, &newlev, &user)
        || newlev < 0 || newlev > 99)
    {
        clif_displaymessage(fd, "usage: @adjgmlvl <lvl> <user>.");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(user);
    if (pl_sd == NULL)
        return -1;

    pc_set_gm_level(pl_sd->status.account_id, newlev);

    return 0;
}

/*==========================================
 * @trade by [MouseJstr]
 *
 * Open a trade window with a remote player
 *
 * If I have to jump to a remote player one more time, I am
 * gonna scream!
 *------------------------------------------
 */
int atcommand_trade(const int, dumb_ptr<map_session_data> sd,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
        return -1;

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd)
    {
        trade_traderequest(sd, pl_sd->bl_id);
        return 0;
    }
    return -1;
}

/* Magic atcommands by Fate */

static
SkillID magic_skills[] =
{
    SkillID::TMW_MAGIC,
    SkillID::TMW_MAGIC_LIFE,
    SkillID::TMW_MAGIC_WAR,
    SkillID::TMW_MAGIC_TRANSMUTE,
    SkillID::TMW_MAGIC_NATURE,
    SkillID::TMW_MAGIC_ETHER,
};

constexpr
size_t magic_skills_nr = sizeof(magic_skills) / sizeof(magic_skills[0]);

static
ZString magic_skill_names[magic_skills_nr] =
{
    "magic",
    "life",
    "war",
    "transmute",
    "nature",
    "astral"
};

int atcommand_magic_info(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd, "Usage: @magicinfo <char_name>");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd)
    {
        FString buf = STRPRINTF(
                "`%s' has the following magic skills:",
                character);
        clif_displaymessage(fd, buf);

        for (size_t i = 0; i < magic_skills_nr; i++)
        {
            SkillID sk = magic_skills[i];
            buf = STRPRINTF(
                    "%d in %s",
                    pl_sd->status.skill[sk].lv,
                    magic_skill_names[i]);
            if (pl_sd->status.skill[sk].lv)
                clif_displaymessage(fd, buf);
        }

        return 0;
    }
    else
        clif_displaymessage(fd, "Character not found.");

    return -1;
}

static
void set_skill(dumb_ptr<map_session_data> sd, SkillID i, int level)
{
    sd->status.skill[i].lv = level;
}

int atcommand_set_magic(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    XString magic_type;
    int value;

    if (!asplit(message, &magic_type, &value, &character))
    {
        clif_displaymessage(fd,
                "Usage: @setmagic <school> <value> <char-name>, where <school> is either `magic', one of the school names, or `all'.");
        return -1;
    }

    SkillID skill_index = SkillID::NEGATIVE;
    if ("all" == magic_type)
        skill_index = SkillID::ZERO;
    else
    {
        for (size_t i = 0; i < magic_skills_nr; i++)
        {
            if (magic_skill_names[i] == magic_type)
            {
                skill_index = magic_skills[i];
                break;
            }
        }
    }

    if (skill_index == SkillID::NEGATIVE)
    {
        clif_displaymessage(fd,
                "Incorrect school of magic.  Use `magic', `nature', `life', `war', `transmute', `ether', or `all'.");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (skill_index == SkillID::ZERO)
            for (SkillID sk : magic_skills)
                set_skill(pl_sd, sk, value);
        else
            set_skill(pl_sd, skill_index, value);

        clif_skillinfoblock(pl_sd);
        return 0;
    }
    else
        clif_displaymessage(fd, "Character not found.");

    return -1;
}

int atcommand_log(const int, dumb_ptr<map_session_data>,
        ZString)
{
    return 0;
    // only used for (implicit) logging
}

int atcommand_tee(const int, dumb_ptr<map_session_data> sd,
        ZString message)
{
    MString data;
    data += sd->status.name.to__actual();
    data += " : ";
    data += message;
    clif_message(sd, FString(data));
    return 0;
}

int atcommand_invisible(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    pc_invisibility(sd, 1);
    return 0;
}

int atcommand_visible(const int, dumb_ptr<map_session_data> sd,
        ZString)
{
    pc_invisibility(sd, 0);
    return 0;
}

static
int atcommand_jump_iterate(const int fd, dumb_ptr<map_session_data> sd,
        dumb_ptr<map_session_data> (*get_start)(void),
        dumb_ptr<map_session_data> (*get_next)(dumb_ptr<map_session_data>))
{
    dumb_ptr<map_session_data> pl_sd;

    pl_sd = map_id_as_player(sd->followtarget);

    if (pl_sd)
        pl_sd = get_next(pl_sd);

    if (!pl_sd)
        pl_sd = get_start();

    if (pl_sd == sd)
    {
        pl_sd = get_next(pl_sd);
        if (!pl_sd)
            pl_sd = get_start();
    }

    if (pl_sd->bl_m && pl_sd->bl_m->flag.nowarpto
        && battle_config.any_warp_GM_min_level > pc_isGM(sd))
    {
        clif_displaymessage(fd,
                "You are not authorised to warp you to the map of this player.");
        return -1;
    }
    if (sd->bl_m && sd->bl_m->flag.nowarp
        && battle_config.any_warp_GM_min_level > pc_isGM(sd))
    {
        clif_displaymessage(fd,
                "You are not authorised to warp you from your actual map.");
        return -1;
    }
    pc_setpos(sd, pl_sd->bl_m->name_, pl_sd->bl_x, pl_sd->bl_y, BeingRemoveWhy::WARPED);
    FString output = STRPRINTF("Jump to %s", pl_sd->status.name);
    clif_displaymessage(fd, output);

    sd->followtarget = pl_sd->bl_id;

    return 0;
}

int atcommand_iterate_forward_over_players(const int fd, dumb_ptr<map_session_data> sd, ZString)
{
    return atcommand_jump_iterate(fd, sd, map_get_first_session, map_get_next_session);
}

int atcommand_iterate_backwards_over_players(const int fd, dumb_ptr<map_session_data> sd, ZString)
{
    return atcommand_jump_iterate(fd, sd, map_get_last_session, map_get_prev_session);
}

int atcommand_wgm(const int fd, dumb_ptr<map_session_data> sd,
        ZString message)
{
    if (tmw_CheckChatSpam(sd, message))
        return 0;

    tmw_GmHackMsg(STRPRINTF("[GM] %s: %s", sd->status.name, message));
    if (!pc_isGM(sd))
        clif_displaymessage(fd, "Message sent.");

    return 0;
}


int atcommand_skillpool_info(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd, "Usage: @sp-info <char_name>");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        SkillID pool_skills[MAX_SKILL_POOL];
        int pool_skills_nr = skill_pool(pl_sd, pool_skills);
        int i;

        FString buf = STRPRINTF(
                "Active skills %d out of %d for %s:",
                pool_skills_nr, skill_pool_max(pl_sd), character);
        clif_displaymessage(fd, buf);
        for (i = 0; i < pool_skills_nr; ++i)
        {
            buf = STRPRINTF(" - %s [%d]: power %d",
                    skill_name(pool_skills[i]),
                    pool_skills[i],
                    skill_power(pl_sd, pool_skills[i]));
            clif_displaymessage(fd, buf);
        }

        buf = STRPRINTF("Learned skills out of %d for %s:",
                skill_pool_skills_size, character);
        clif_displaymessage(fd, buf);

        for (i = 0; i < skill_pool_skills_size; ++i)
        {
            const FString& name = skill_name(skill_pool_skills[i]);
            int lvl = pl_sd->status.skill[skill_pool_skills[i]].lv;

            if (lvl)
            {
                buf = STRPRINTF(" - %s [%d]: lvl %d",
                        name, skill_pool_skills[i], lvl);
                clif_displaymessage(fd, buf);
            }
        }

    }
    else
        clif_displaymessage(fd, "Character not found.");

    return 0;
}

int atcommand_skillpool_focus(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    SkillID skill;

    if (!asplit(message, &skill, &character))
    {
        clif_displaymessage(fd, "Usage: @sp-focus <skill-nr> <char_name>");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (skill_pool_activate(pl_sd, skill))
            clif_displaymessage(fd, "Activation failed.");
        else
            clif_displaymessage(fd, "Activation successful.");
    }
    else
        clif_displaymessage(fd, "Character not found.");

    return 0;
}

int atcommand_skillpool_unfocus(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    SkillID skill;

    if (!asplit(message, &skill, &character))
    {
        clif_displaymessage(fd, "Usage: @sp-unfocus <skill-nr> <char_name>");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        if (skill_pool_deactivate(pl_sd, skill))
            clif_displaymessage(fd, "Deactivation failed.");
        else
            clif_displaymessage(fd, "Deactivation successful.");
    }
    else
        clif_displaymessage(fd, "Character not found.");

    return 0;
}

int atcommand_skill_learn(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    CharName character;
    SkillID skill;
    int level;

    if (!asplit(message, &skill, &level, &character))
    {
        clif_displaymessage(fd,
                "Usage: @skill-learn <skill-nr> <level> <char_name>");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd != NULL)
    {
        set_skill(pl_sd, skill, level);
        clif_skillinfoblock(pl_sd);
    }
    else
        clif_displaymessage(fd, "Character not found.");

    return 0;
}

int atcommand_ipcheck(const int fd, dumb_ptr<map_session_data>,
        ZString message)
{
    struct sockaddr_in sai;
    CharName character;
    socklen_t sa_len = sizeof(struct sockaddr);
    unsigned long ip;

    if (!asplit(message, &character))
    {
        clif_displaymessage(fd, "Usage: @ipcheck <char name>");
        return -1;
    }

    dumb_ptr<map_session_data> pl_sd = map_nick2sd(character);
    if (pl_sd == NULL)
    {
        clif_displaymessage(fd, "Character not found.");
        return -1;
    }

    if (getpeername(pl_sd->fd, reinterpret_cast<struct sockaddr *>(&sai), &sa_len))
    {
        clif_displaymessage(fd,
                "Guru Meditation Error: getpeername() failed");
        return -1;
    }

    ip = sai.sin_addr.s_addr;

    // We now have the IP address of a character.
    // Loop over all logged in sessions looking for matches.

    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd && pl_sd->state.auth)
        {
            if (getpeername(pl_sd->fd, reinterpret_cast<struct sockaddr *>(&sai), &sa_len))
                continue;

            // Is checking GM levels really needed here?
            if (ip == sai.sin_addr.s_addr)
            {
                FString output = STRPRINTF(
                        "Name: %s | Location: %s %d %d",
                        pl_sd->status.name, pl_sd->mapname_,
                        pl_sd->bl_x, pl_sd->bl_y);
                clif_displaymessage(fd, output);
            }
        }
    }

    clif_displaymessage(fd, "End of list");
    return 0;
}

int atcommand_doomspot(const int fd, dumb_ptr<map_session_data> sd,
        ZString)
{
    for (int i = 0; i < fd_max; i++)
    {
        if (!session[i])
            continue;
        dumb_ptr<map_session_data> pl_sd = dumb_ptr<map_session_data>(static_cast<map_session_data *>(session[i]->session_data.get()));
        if (pl_sd
            && pl_sd->state.auth && i != fd && sd->bl_m == pl_sd->bl_m
            && sd->bl_x == pl_sd->bl_x && sd->bl_y == pl_sd->bl_y
            && pc_isGM(sd) >= pc_isGM(pl_sd))
        {                       // you can doom only lower or same gm level
            pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
            clif_displaymessage(pl_sd->fd, "The holy messenger has given judgement.");
        }
    }
    clif_displaymessage(fd, "Judgement was made.");

    return 0;
}
