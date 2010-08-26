// $Id: atcommand.h 148 2004-09-30 14:05:37Z MouseJstr $
#ifndef _ATCOMMAND_H_
#define _ATCOMMAND_H_

#include "map.h"

enum AtCommandType
{
    AtCommand_None = -1,
    AtCommand_Broadcast = 0,
    AtCommand_Setup,
    AtCommand_LocalBroadcast,
    AtCommand_MapMove,
    AtCommand_ResetState,
    AtCommand_CharWarp,
    AtCommand_Warp,
    AtCommand_Where,
    AtCommand_Goto,
    AtCommand_Jump,
    AtCommand_Who,
    AtCommand_WhoGroup,
    AtCommand_WhoMap,
    AtCommand_WhoMapGroup,
    AtCommand_WhoGM,
    AtCommand_Save,
    AtCommand_Load,
    AtCommand_Speed,
    AtCommand_Storage,
    AtCommand_GuildStorage,
    AtCommand_Option,
    AtCommand_Hide,
    AtCommand_Die,
    AtCommand_Kill,
    AtCommand_Alive,
    AtCommand_Kami,
    AtCommand_KamiB,
    AtCommand_Heal,
    AtCommand_Item,
    AtCommand_ItemReset,
    AtCommand_ItemCheck,
    AtCommand_BaseLevelUp,
    AtCommand_JobLevelUp,
    AtCommand_Help,
    AtCommand_GM,
    AtCommand_PvPOff,
    AtCommand_PvPOn,
    AtCommand_GvGOff,
    AtCommand_GvGOn,
    AtCommand_Model,
    AtCommand_Go,
    AtCommand_Spawn,
    AtCommand_Monster,
    AtCommand_KillMonster,
    AtCommand_KillMonster2,
    AtCommand_Produce,
    AtCommand_Memo,
    AtCommand_GAT,
    AtCommand_Packet,
    AtCommand_StatusPoint,
    AtCommand_SkillPoint,
    AtCommand_Zeny,
    AtCommand_Param,
    AtCommand_Strength,
    AtCommand_Agility,
    AtCommand_Vitality,
    AtCommand_Intelligence,
    AtCommand_Dexterity,
    AtCommand_Luck,
    AtCommand_GuildLevelUp,
    AtCommand_Recall,
    AtCommand_Revive,
    AtCommand_CharacterStats,
    AtCommand_CharacterStatsAll,
    AtCommand_CharacterOption,
    AtCommand_CharacterSave,
    AtCommand_CharacterLoad,
    AtCommand_Night,
    AtCommand_Day,
    AtCommand_Doom,
    AtCommand_DoomMap,
    AtCommand_Raise,
    AtCommand_RaiseMap,
    AtCommand_CharacterBaseLevel,
    AtCommand_CharacterJobLevel,
    AtCommand_Kick,
    AtCommand_KickAll,
    AtCommand_AllSkills,
    AtCommand_QuestSkill,
    AtCommand_CharQuestSkill,
    AtCommand_LostSkill,
    AtCommand_CharLostSkill,
    AtCommand_Party,
    AtCommand_Guild,
    AtCommand_AgitStart,
    AtCommand_AgitEnd,
    AtCommand_MapExit,
    AtCommand_IDSearch,
    AtCommand_CharSkReset,
    AtCommand_CharStReset,
    AtCommand_CharReset,
    //by chbrules
    AtCommand_CharModel,
    AtCommand_CharSKPoint,
    AtCommand_CharSTPoint,
    AtCommand_CharZeny,
    AtCommand_RecallAll,
    AtCommand_ReloadItemDB,
    AtCommand_ReloadMobDB,
    AtCommand_ReloadSkillDB,
    AtCommand_ReloadScript,
    AtCommand_ReloadGMDB,
    AtCommand_MapInfo,
    AtCommand_Dye,
    AtCommand_HairStyle,
    AtCommand_HairColor,
    AtCommand_AllStats,
    AtCommand_CharChangeSex,    // by Yor
    AtCommand_CharBlock,        // by Yor
    AtCommand_CharBan,          // by Yor
    AtCommand_CharUnBlock,      // by Yor
    AtCommand_CharUnBan,        // by Yor
    AtCommand_MountPeco,        // by Valaris
    AtCommand_CharMountPeco,    // by Yor
    AtCommand_GuildSpy,         // [Syrus22]
    AtCommand_PartySpy,         // [Syrus22]
    AtCommand_GuildRecall,      // by Yor
    AtCommand_PartyRecall,      // by Yor
    AtCommand_Enablenpc,
    AtCommand_Disablenpc,
    AtCommand_ServerTime,       // by Yor
    AtCommand_CharDelItem,      // by Yor
    AtCommand_Jail,             // by Yor
    AtCommand_UnJail,           // by Yor
    AtCommand_Disguise,         // [Valaris]
    AtCommand_UnDisguise,       // by Yor
    AtCommand_IgnoreList,       // by Yor
    AtCommand_CharIgnoreList,   // by Yor
    AtCommand_InAll,            // by Yor
    AtCommand_ExAll,            // by Yor
    AtCommand_CharDisguise,     // Kalaspuff
    AtCommand_CharUnDisguise,   // Kalaspuff
    AtCommand_EMail,            // by Yor
    AtCommand_Hatch,
    AtCommand_Effect,           // by Apple
    AtCommand_Char_Item_List,   // by Yor
    AtCommand_Char_Storage_List,    // by Yor
    AtCommand_Char_Cart_List,   // by Yor
    AtCommand_AddWarp,          // by MouseJstr
    AtCommand_Follow,           // by MouseJstr
    AtCommand_SkillOn,          // by MouseJstr
    AtCommand_SkillOff,         // by MouseJstr
    AtCommand_Killer,           // by MouseJstr
    AtCommand_NpcMove,          // by MouseJstr
    AtCommand_Killable,         // by MouseJstr
    AtCommand_CharKillable,     // by MouseJstr
    AtCommand_Chareffect,       // by MouseJstr
    AtCommand_Chardye,          // by MouseJstr
    AtCommand_Charhairstyle,    // by MouseJstr
    AtCommand_Charhaircolor,    // by MouseJstr
    AtCommand_Dropall,          // by MouseJstr
    AtCommand_Chardropall,      // by MouseJstr
    AtCommand_Storeall,         // by MouseJstr
    AtCommand_Charstoreall,     // by MouseJstr
    AtCommand_Skillid,          // by MouseJstr
    AtCommand_Useskill,         // by MouseJstr
    AtCommand_Summon,
    AtCommand_Rain,
    AtCommand_Snow,
    AtCommand_Sakura,
    AtCommand_Fog,
    AtCommand_Leaves,
    AtCommand_AdjGmLvl,
    AtCommand_AdjCmdLvl,
    AtCommand_Trade,
    AtCommand_UnMute,
    AtCommand_CharWipe,
    AtCommand_SetMagic,
    AtCommand_MagicInfo,
    AtCommand_Log,
    AtCommand_Tee,
    AtCommand_Invisible,
    AtCommand_Visible,
    AtCommand_IterateForward,
    AtCommand_IterateBackward,
    AtCommand_Wgm,
    AtCommand_IpCheck,
    AtCommand_ListNearby,       // [fate]
    // end
    AtCommand_Unknown,
    AtCommand_MAX
};

typedef enum AtCommandType AtCommandType;

typedef struct AtCommandInfo
{
    AtCommandType type;
    const char *command;
    int  level;
    int  (*proc) (const int, struct map_session_data *,
                  const char *command, const char *message);
} AtCommandInfo;

AtCommandType is_atcommand (const int fd, struct map_session_data *sd,
                            const char *message, int gmlvl);

AtCommandType atcommand (const int level, const char *message,
                         AtCommandInfo * info);
int  get_atcommand_level (const AtCommandType type);

char *msg_txt (int msg_number); // [Yor]

int  atcommand_item (const int fd, struct map_session_data *sd, const char *command, const char *message);  // [Valaris]
int  atcommand_warp (const int fd, struct map_session_data *sd, const char *command, const char *message);  // [Yor]
int  atcommand_spawn (const int fd, struct map_session_data *sd, const char *command, const char *message); // [Valaris]
int  atcommand_goto (const int fd, struct map_session_data *sd, const char *command, const char *message);  // [Yor]
int  atcommand_recall (const int fd, struct map_session_data *sd, const char *command, const char *message);    // [Yor]

int  atcommand_config_read (const char *cfgName);
int  msg_config_read (const char *cfgName);

void log_atcommand (struct map_session_data *sd, const char *fmt, ...);
void gm_log (const char *fmt, ...);

#endif
