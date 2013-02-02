#include "skill.hpp"

#include <cstdlib>
#include <cstring>
#include <ctime>

#include "../common/cxxstdio.hpp"
#include "../common/mt_rand.hpp"
#include "../common/nullpo.hpp"
#include "../common/socket.hpp"

#include "battle.hpp"
#include "clif.hpp"
#include "magic.hpp"
#include "map.hpp"
#include "mob.hpp"
#include "pc.hpp"

#include "../poison.hpp"

#define SKILLUNITTIMER_INVERVAL 100

// This table is wrong.
// The only used skill with a status effect is NPC_SELFDESTRUCT,
// and due to the misnumber, it wasn't active anyway.
static __attribute__((deprecated))
earray<StatusChange, SkillID, MAX_SKILL_DB> SkillStatusChangeTable //=
{{
    // 0-
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // NV_BASIC
    StatusChange::NEGATIVE1,    // SM_SWORD
    StatusChange::NEGATIVE1,    // SM_TWOHAND
    StatusChange::NEGATIVE1,    // SM_RECOVERY
    StatusChange::NEGATIVE1,    // SM_BASH
    StatusChange::NEGATIVE1,    // SM_PROVOKE
    StatusChange::NEGATIVE1,    // SM_MAGNUM
    StatusChange::NEGATIVE1,    // SM_ENDURE
    StatusChange::NEGATIVE1,    // MG_SRECOVERY
    // 10-
    StatusChange::NEGATIVE1,    // MG_SIGHT
    StatusChange::NEGATIVE1,    // MG_NAPALMBEAT
    StatusChange::NEGATIVE1,    // MG_SAFETYWALL
    StatusChange::NEGATIVE1,    // MG_SOULSTRIKE
    StatusChange::NEGATIVE1,    // MG_COLDBOLT
    StatusChange::NEGATIVE1,    // MG_FROSTDIVER
    StatusChange::NEGATIVE1,    // MG_STONECURSE
    StatusChange::NEGATIVE1,    // MG_FIREBALL
    StatusChange::NEGATIVE1,    // MG_FIREWALL
    StatusChange::NEGATIVE1,    // MG_FIREBOLT
    // 20-
    StatusChange::NEGATIVE1,    // MG_LIGHTNINGBOLT
    StatusChange::NEGATIVE1,    // MG_THUNDERSTORM
    StatusChange::NEGATIVE1,    // AL_DP
    StatusChange::NEGATIVE1,    // AL_DEMONBANE
    StatusChange::NEGATIVE1,    // AL_RUWACH
    StatusChange::NEGATIVE1,    // AL_PNEUMA
    StatusChange::NEGATIVE1,    // AL_TELEPORT
    StatusChange::NEGATIVE1,    // AL_WARP
    StatusChange::NEGATIVE1,    // AL_HEAL
    StatusChange::NEGATIVE1,    // AL_INCAGI
    // 30-
    StatusChange::NEGATIVE1,    // AL_DECAGI
    StatusChange::NEGATIVE1,    // AL_HOLYWATER
    StatusChange::NEGATIVE1,    // AL_CRUCIS
    StatusChange::NEGATIVE1,    // AL_ANGELUS
    StatusChange::NEGATIVE1,    // AL_BLESSING
    StatusChange::NEGATIVE1,    // AL_CURE
    StatusChange::NEGATIVE1,    // MC_INCCARRY
    StatusChange::NEGATIVE1,    // MC_DISCOUNT
    StatusChange::NEGATIVE1,    // MC_OVERCHARGE
    StatusChange::NEGATIVE1,    // MC_PUSHCART
    // 40-
    StatusChange::NEGATIVE1,    // MC_IDENTIFY
    StatusChange::NEGATIVE1,    // MC_VENDING
    StatusChange::NEGATIVE1,    // MC_MAMMONITE
    StatusChange::NEGATIVE1,    // AC_OWL
    StatusChange::NEGATIVE1,    // AC_VULTURE
    StatusChange::NEGATIVE1,    // AC_CONCENTRATION
    StatusChange::NEGATIVE1,    // AC_DOUBLE
    StatusChange::NEGATIVE1,    // AC_SHOWER
    StatusChange::NEGATIVE1,    // TF_DOUBLE
    StatusChange::NEGATIVE1,    // TF_MISS
    // 50-
    StatusChange::NEGATIVE1,    // TF_STEAL
    StatusChange::NEGATIVE1,    // TF_HIDING
    StatusChange::NEGATIVE1,    // TF_POISON
    StatusChange::NEGATIVE1,    // TF_DETOXIFY
    StatusChange::NEGATIVE1,    // ALL_RESURRECTION
    StatusChange::NEGATIVE1,    // KN_SPEARMASTERY
    StatusChange::NEGATIVE1,    // KN_PIERCE
    StatusChange::NEGATIVE1,    // KN_BRANDISHSPEAR
    StatusChange::NEGATIVE1,    // KN_SPEARSTAB
    StatusChange::NEGATIVE1,    // KN_SPEARBOOMERANG
    // 60-
    StatusChange::NEGATIVE1,    // KN_TWOHANDQUICKEN
    StatusChange::NEGATIVE1,    // KN_AUTOCOUNTER
    StatusChange::NEGATIVE1,    // KN_BOWLINGBASH
    StatusChange::NEGATIVE1,    // KN_RIDING
    StatusChange::NEGATIVE1,    // KN_CAVALIERMASTERY
    StatusChange::NEGATIVE1,    // PR_MACEMASTERY
    StatusChange::NEGATIVE1,    // PR_IMPOSITIO
    StatusChange::NEGATIVE1,    // PR_SUFFRAGIUM
    StatusChange::NEGATIVE1,    // PR_ASPERSIO
    StatusChange::NEGATIVE1,    // PR_BENEDICTIO
    // 70-
    StatusChange::NEGATIVE1,    // PR_SANCTUARY
    StatusChange::NEGATIVE1,    // PR_SLOWPOISON
    StatusChange::NEGATIVE1,    // PR_STRECOVERY
    StatusChange::NEGATIVE1,    // PR_KYRIE
    StatusChange::NEGATIVE1,    // PR_MAGNIFICAT
    StatusChange::NEGATIVE1,    // PR_GLORIA
    StatusChange::NEGATIVE1,    // PR_LEXDIVINA
    StatusChange::NEGATIVE1,    // PR_TURNUNDEAD
    StatusChange::NEGATIVE1,    // PR_LEXAETERNA
    StatusChange::NEGATIVE1,    // PR_MAGNUS
    // 80-
    StatusChange::NEGATIVE1,    // WZ_FIREPILLAR
    StatusChange::NEGATIVE1,    // WZ_SIGHTRASHER
    StatusChange::NEGATIVE1,    // WZ_FIREIVY
    StatusChange::NEGATIVE1,    // WZ_METEOR
    StatusChange::NEGATIVE1,    // WZ_JUPITEL
    StatusChange::NEGATIVE1,    // WZ_VERMILION
    StatusChange::NEGATIVE1,    // WZ_WATERBALL
    StatusChange::NEGATIVE1,    // WZ_ICEWALL
    StatusChange::NEGATIVE1,    // WZ_FROSTNOVA
    StatusChange::NEGATIVE1,    // WZ_STORMGUST
    // 90-
    StatusChange::NEGATIVE1,    // WZ_EARTHSPIKE
    StatusChange::NEGATIVE1,    // WZ_HEAVENDRIVE
    StatusChange::NEGATIVE1,    // WZ_QUAGMIRE
    StatusChange::NEGATIVE1,    // WZ_ESTIMATION
    StatusChange::NEGATIVE1,    // BS_IRON
    StatusChange::NEGATIVE1,    // BS_STEEL
    StatusChange::NEGATIVE1,    // BS_ENCHANTEDSTONE
    StatusChange::NEGATIVE1,    // BS_ORIDEOCON
    StatusChange::NEGATIVE1,    // BS_DAGGER
    StatusChange::NEGATIVE1,    // BS_SWORD
    // 100-
    StatusChange::NEGATIVE1,    // BS_TWOHANDSWORD
    StatusChange::NEGATIVE1,    // BS_AXE
    StatusChange::NEGATIVE1,    // BS_MACE
    StatusChange::NEGATIVE1,    // BS_KNUCKLE
    StatusChange::NEGATIVE1,    // BS_SPEAR
    StatusChange::NEGATIVE1,    // BS_HILTBINDING
    StatusChange::NEGATIVE1,    // BS_FINDINGORE
    StatusChange::NEGATIVE1,    // BS_WEAPONRESEARCH
    StatusChange::NEGATIVE1,    // BS_REPAIRWEAPON
    StatusChange::NEGATIVE1,    // BS_SKINTEMPER
    // 110-
    StatusChange::NEGATIVE1,    // BS_HAMMERFALL
    StatusChange::NEGATIVE1,    // BS_ADRENALINE
    StatusChange::NEGATIVE1,    // BS_WEAPONPERFECT
    StatusChange::NEGATIVE1,    // BS_OVERTHRUST
    StatusChange::NEGATIVE1,    // BS_MAXIMIZE
    StatusChange::NEGATIVE1,    // HT_SKIDTRAP
    StatusChange::NEGATIVE1,    // HT_LANDMINE
    StatusChange::NEGATIVE1,    // HT_ANKLESNARE
    StatusChange::NEGATIVE1,    // HT_SHOCKWAVE
    StatusChange::NEGATIVE1,    // HT_SANDMAN
    // 120-
    StatusChange::NEGATIVE1,    // HT_FLASHER
    StatusChange::NEGATIVE1,    // HT_FREEZINGTRAP
    StatusChange::NEGATIVE1,    // HT_BLASTMINE
    StatusChange::NEGATIVE1,    // HT_CLAYMORETRAP
    StatusChange::NEGATIVE1,    // HT_REMOVETRAP
    StatusChange::NEGATIVE1,    // HT_TALKIEBOX
    StatusChange::NEGATIVE1,    // HT_BEASTBANE
    StatusChange::NEGATIVE1,    // HT_FALCON
    StatusChange::NEGATIVE1,    // HT_STEELCROW
    StatusChange::NEGATIVE1,    // HT_BLITZBEAT
    // 130-
    StatusChange::NEGATIVE1,    // HT_DETECTING
    StatusChange::NEGATIVE1,    // HT_SPRINGTRAP
    StatusChange::NEGATIVE1,    // AS_RIGHT
    StatusChange::NEGATIVE1,    // AS_LEFT
    StatusChange::NEGATIVE1,    // AS_KATAR
    StatusChange::NEGATIVE1,    // AS_CLOAKING
    StatusChange::NEGATIVE1,    // AS_SONICBLOW
    StatusChange::NEGATIVE1,    // AS_GRIMTOOTH
    StatusChange::NEGATIVE1,    // AS_ENCHANTPOISON
    StatusChange::NEGATIVE1,    // AS_POISONREACT
    // 140-
    StatusChange::NEGATIVE1,    // AS_VENOMDUST
    StatusChange::NEGATIVE1,    // AS_SPLASHER
    StatusChange::NEGATIVE1,    // NV_FIRSTAID
    StatusChange::NEGATIVE1,    // NV_TRICKDEAD
    StatusChange::NEGATIVE1,    // SM_MOVINGRECOVERY
    StatusChange::NEGATIVE1,    // SM_FATALBLOW
    StatusChange::NEGATIVE1,    // SM_AUTOBERSERK
    StatusChange::NEGATIVE1,    // AC_MAKINGARROW
    StatusChange::NEGATIVE1,    // AC_CHARGEARROW
    StatusChange::NEGATIVE1,    // TF_SPRINKLESAND
    // 150-
    StatusChange::NEGATIVE1,    // TF_BACKSLIDING
    StatusChange::NEGATIVE1,    // TF_PICKSTONE
    StatusChange::NEGATIVE1,    // TF_THROWSTONE
    StatusChange::NEGATIVE1,    // MC_CARTREVOLUTION
    StatusChange::NEGATIVE1,    // MC_CHANGECART
    StatusChange::NEGATIVE1,    // MC_LOUD
    StatusChange::NEGATIVE1,    // AL_HOLYLIGHT
    StatusChange::NEGATIVE1,    // MG_ENERGYCOAT
    StatusChange::NEGATIVE1,    // NPC_PIERCINGATT
    StatusChange::NEGATIVE1,    // NPC_MENTALBREAKER
    // 160-
    StatusChange::NEGATIVE1,    // NPC_RANGEATTACK
    StatusChange::NEGATIVE1,    // NPC_ATTRICHANGE
    StatusChange::NEGATIVE1,    // NPC_CHANGEWATER
    StatusChange::NEGATIVE1,    // NPC_CHANGEGROUND
    StatusChange::NEGATIVE1,    // NPC_CHANGEFIRE
    StatusChange::NEGATIVE1,    // NPC_CHANGEWIND
    StatusChange::NEGATIVE1,    // NPC_CHANGEPOISON
    StatusChange::NEGATIVE1,    // NPC_CHANGEHOLY
    StatusChange::NEGATIVE1,    // NPC_CHANGEDARKNESS
    StatusChange::NEGATIVE1,    // NPC_CHANGETELEKINESIS
    // 170-
    StatusChange::NEGATIVE1,    // NPC_CRITICALSLASH
    StatusChange::NEGATIVE1,    // NPC_COMBOATTACK
    StatusChange::NEGATIVE1,    // NPC_GUIDEDATTACK
    SC_SELFDESTRUCTION,         // NPC_SELFDESTRUCTION
    StatusChange::NEGATIVE1,    // NPC_SPLASHATTACK
    StatusChange::NEGATIVE1,    // NPC_SUICIDE
    StatusChange::NEGATIVE1,    // NPC_POISON
    StatusChange::NEGATIVE1,    // NPC_BLINDATTACK
    StatusChange::NEGATIVE1,    // NPC_SILENCEATTACK
    StatusChange::NEGATIVE1,    // NPC_STUNATTACK
    // 180-
    StatusChange::NEGATIVE1,    // NPC_PETRIFYATTACK
    StatusChange::NEGATIVE1,    // NPC_CURSEATTACK
    StatusChange::NEGATIVE1,    // NPC_SLEEPATTACK
    StatusChange::NEGATIVE1,    // NPC_RANDOMATTACK
    StatusChange::NEGATIVE1,    // NPC_WATERATTACK
    StatusChange::NEGATIVE1,    // NPC_GROUNDATTACK
    StatusChange::NEGATIVE1,    // NPC_FIREATTACK
    StatusChange::NEGATIVE1,    // NPC_WINDATTACK
    StatusChange::NEGATIVE1,    // NPC_POISONATTACK
    StatusChange::NEGATIVE1,    // NPC_HOLYATTACK
    // 190-
    StatusChange::NEGATIVE1,    // NPC_DARKNESSATTACK
    StatusChange::NEGATIVE1,    // NPC_TELEKINESISATTACK
    StatusChange::NEGATIVE1,    // NPC_MAGICALATTACK
    StatusChange::NEGATIVE1,    // NPC_METAMORPHOSIS
    StatusChange::NEGATIVE1,    // NPC_PROVOCATION
    StatusChange::NEGATIVE1,    // NPC_SMOKING
    StatusChange::NEGATIVE1,    // NPC_SUMMONSLAVE
    StatusChange::NEGATIVE1,    // NPC_EMOTION
    StatusChange::NEGATIVE1,    // NPC_TRANSFORMATION
    StatusChange::NEGATIVE1,    // NPC_BLOODDRAIN
    // 200-
    StatusChange::NEGATIVE1,    // NPC_ENERGYDRAIN
    StatusChange::NEGATIVE1,    // NPC_KEEPING
    StatusChange::NEGATIVE1,    // NPC_DARKBREATH
    StatusChange::NEGATIVE1,    // NPC_DARKBLESSING
    StatusChange::NEGATIVE1,    // NPC_BARRIER
    StatusChange::NEGATIVE1,    // NPC_DEFENDER
    StatusChange::NEGATIVE1,    // NPC_LICK
    StatusChange::NEGATIVE1,    // NPC_HALLUCINATION
    StatusChange::NEGATIVE1,    // NPC_REBIRTH
    StatusChange::NEGATIVE1,    // NPC_SUMMONMONSTER
    // 210-
    StatusChange::NEGATIVE1,    // RG_SNATCHER
    StatusChange::NEGATIVE1,    // RG_STEALCOIN
    StatusChange::NEGATIVE1,    // RG_BACKSTAP
    StatusChange::NEGATIVE1,    // RG_TUNNELDRIVE
    StatusChange::NEGATIVE1,    // RG_RAID
    StatusChange::NEGATIVE1,    // RG_STRIPWEAPON
    StatusChange::NEGATIVE1,    // RG_STRIPSHIELD
    StatusChange::NEGATIVE1,    // RG_STRIPARMOR
    StatusChange::NEGATIVE1,    // RG_STRIPHELM
    StatusChange::NEGATIVE1,    // RG_INTIMIDATE
    // 220-
    StatusChange::NEGATIVE1,    // RG_GRAFFITI
    StatusChange::NEGATIVE1,    // RG_FLAGGRAFFITI
    StatusChange::NEGATIVE1,    // RG_CLEANER
    StatusChange::NEGATIVE1,    // RG_GANGSTER
    StatusChange::NEGATIVE1,    // RG_COMPULSION
    StatusChange::NEGATIVE1,    // RG_PLAGIARISM
    StatusChange::NEGATIVE1,    // AM_AXEMASTERY
    StatusChange::NEGATIVE1,    // AM_LEARNINGPOTION
    StatusChange::NEGATIVE1,    // AM_PHARMACY
    StatusChange::NEGATIVE1,    // AM_DEMONSTRATION
    // 230-
    StatusChange::NEGATIVE1,    // AM_ACIDTERROR
    StatusChange::NEGATIVE1,    // AM_POTIONPITCHER
    StatusChange::NEGATIVE1,    // AM_CANNIBALIZE
    StatusChange::NEGATIVE1,    // AM_SPHEREMINE
    StatusChange::NEGATIVE1,    // AM_CP_WEAPON
    StatusChange::NEGATIVE1,    // AM_CP_SHIELD
    StatusChange::NEGATIVE1,    // AM_CP_ARMOR
    StatusChange::NEGATIVE1,    // AM_CP_HELM
    StatusChange::NEGATIVE1,    // AM_BIOETHICS
    StatusChange::NEGATIVE1,    // AM_BIOTECHNOLOGY
    // 240-
    StatusChange::NEGATIVE1,    // AM_CREATECREATURE
    StatusChange::NEGATIVE1,    // AM_CULTIVATION
    StatusChange::NEGATIVE1,    // AM_FLAMECONTROL
    StatusChange::NEGATIVE1,    // AM_CALLHOMUN
    StatusChange::NEGATIVE1,    // AM_REST
    StatusChange::NEGATIVE1,    // AM_DRILLMASTER
    StatusChange::NEGATIVE1,    // AM_HEALHOMUN
    StatusChange::NEGATIVE1,    // AM_RESURRECTHOMUN
    StatusChange::NEGATIVE1,    // CR_TRUST
    StatusChange::NEGATIVE1,    // CR_AUTOGUARD
    // 250-
    StatusChange::NEGATIVE1,    // CR_SHIELDCHARGE
    StatusChange::NEGATIVE1,    // CR_SHIELDBOOMERANG
    StatusChange::NEGATIVE1,    // CR_REFLECTSHIELD
    StatusChange::NEGATIVE1,    // CR_HOLYCROSS
    StatusChange::NEGATIVE1,    // CR_GRANDCROSS
    StatusChange::NEGATIVE1,    // CR_DEVOTION
    StatusChange::NEGATIVE1,    // CR_PROVIDENCE
    StatusChange::NEGATIVE1,    // CR_DEFENDER
    StatusChange::NEGATIVE1,    // CR_SPEARQUICKEN
    StatusChange::NEGATIVE1,    // MO_IRONHAND
    // 260-
    StatusChange::NEGATIVE1,    // MO_SPIRITSRECOVERY
    StatusChange::NEGATIVE1,    // MO_CALLSPIRITS
    StatusChange::NEGATIVE1,    // MO_ABSORBSPIRITS
    StatusChange::NEGATIVE1,    // MO_TRIPLEATTACK
    StatusChange::NEGATIVE1,    // MO_BODYRELOCATION
    StatusChange::NEGATIVE1,    // MO_DODGE
    StatusChange::NEGATIVE1,    // MO_INVESTIGATE
    StatusChange::NEGATIVE1,    // MO_FINGEROFFENSIVE
    StatusChange::NEGATIVE1,    // MO_STEELBODY
    StatusChange::NEGATIVE1,    // MO_BLADESTOP
    // 270-
    StatusChange::NEGATIVE1,    // MO_EXPLOSIONSPIRITS
    StatusChange::NEGATIVE1,    // MO_EXTREMITYFIST
    StatusChange::NEGATIVE1,    // MO_CHAINCOMBO
    StatusChange::NEGATIVE1,    // MO_COMBOFINISH
    StatusChange::NEGATIVE1,    // SA_ADVANCEDBOOK
    StatusChange::NEGATIVE1,    // SA_CASTCANCEL
    StatusChange::NEGATIVE1,    // SA_MAGICROD
    StatusChange::NEGATIVE1,    // SA_SPELLBREAKER
    StatusChange::NEGATIVE1,    // SA_FREECAST
    StatusChange::NEGATIVE1,    // SA_AUTOSPELL
    // 280-
    StatusChange::NEGATIVE1,    // SA_FLAMELAUNCHER
    StatusChange::NEGATIVE1,    // SA_FROSTWEAPON
    StatusChange::NEGATIVE1,    // SA_LIGHTNINGLOADER
    StatusChange::NEGATIVE1,    // SA_SEISMICWEAPON
    StatusChange::NEGATIVE1,    // SA_DRAGONOLOGY
    StatusChange::NEGATIVE1,    // SA_VOLCANO
    StatusChange::NEGATIVE1,    // SA_DELUGE
    StatusChange::NEGATIVE1,    // SA_VIOLENTGALE
    StatusChange::NEGATIVE1,    // SA_LANDPROTECTOR
    StatusChange::NEGATIVE1,    // SA_DISPELL
    // 290-
    StatusChange::NEGATIVE1,    // SA_ABRACADABRA
    StatusChange::NEGATIVE1,    // SA_MONOCELL
    StatusChange::NEGATIVE1,    // SA_CLASSCHANGE
    StatusChange::NEGATIVE1,    // SA_SUMMONMONSTER
    StatusChange::NEGATIVE1,    // SA_REVERSEORCISH
    StatusChange::NEGATIVE1,    // SA_DEATH
    StatusChange::NEGATIVE1,    // SA_FORTUNE
    StatusChange::NEGATIVE1,    // SA_TAMINGMONSTER
    StatusChange::NEGATIVE1,    // SA_QUESTION
    StatusChange::NEGATIVE1,    // SA_GRAVITY
    // 300-
    StatusChange::NEGATIVE1,    // SA_LEVELUP
    StatusChange::NEGATIVE1,    // SA_INSTANTDEATH
    StatusChange::NEGATIVE1,    // SA_FULLRECOVERY
    StatusChange::NEGATIVE1,    // SA_COMA
    StatusChange::NEGATIVE1,    // BD_ADAPTATION
    StatusChange::NEGATIVE1,    // BD_ENCORE
    StatusChange::NEGATIVE1,    // BD_LULLABY
    StatusChange::NEGATIVE1,    // BD_RICHMANKIM
    StatusChange::NEGATIVE1,    // BD_ETERNALCHAOS
    StatusChange::NEGATIVE1,    // BD_DRUMBATTLEFIELD
    // 310-
    StatusChange::NEGATIVE1,    // BD_RINGNIBELUNGEN
    StatusChange::NEGATIVE1,    // BD_ROKISWEIL
    StatusChange::NEGATIVE1,    // BD_INTOABYSS
    StatusChange::NEGATIVE1,    // BD_SIEGFRIED
    StatusChange::NEGATIVE1,    // BD_RAGNAROK
    StatusChange::NEGATIVE1,    // BA_MUSICALLESSON
    StatusChange::NEGATIVE1,    // BA_MUSICALSTRIKE
    StatusChange::NEGATIVE1,    // BA_DISSONANCE
    StatusChange::NEGATIVE1,    // BA_FROSTJOKE
    StatusChange::NEGATIVE1,    // BA_WHISTLE
    // 320-
    StatusChange::NEGATIVE1,    // BA_ASSASSINCROSS
    StatusChange::NEGATIVE1,    // BA_POEMBRAGI
    StatusChange::NEGATIVE1,    // BA_APPLEIDUN
    StatusChange::NEGATIVE1,    // DC_DANCINGLESSON
    StatusChange::NEGATIVE1,    // DC_THROWARROW
    StatusChange::NEGATIVE1,    // DC_UGLYDANCE
    StatusChange::NEGATIVE1,    // DC_SCREAM
    StatusChange::NEGATIVE1,    // DC_HUMMING
    StatusChange::NEGATIVE1,    // DC_DONTFORGETME
    StatusChange::NEGATIVE1,    // DC_FORTUNEKISS
    // 330-
    StatusChange::NEGATIVE1,    // DC_SERVICEFORYOU
    StatusChange::NEGATIVE1,    // NPC_SELFDESTRUCTION2
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // WE_MALE
    StatusChange::NEGATIVE1,    // WE_FEMALE
    StatusChange::NEGATIVE1,    // WE_CALLPARTNER
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // NPC_DARKCROSS
    StatusChange::NEGATIVE1,    // (none)
    // 340-
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    // 350-
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // (none)
    StatusChange::NEGATIVE1,    // LK_AURABLADE
    StatusChange::NEGATIVE1,    // LK_PARRYING
    StatusChange::NEGATIVE1,    // LK_CONCENTRATION
    StatusChange::NEGATIVE1,    // LK_TENSIONRELAX
    StatusChange::NEGATIVE1,    // LK_BERSERK
    // 360-
    StatusChange::NEGATIVE1,    // LK_FURY
    StatusChange::NEGATIVE1,    // HP_ASSUMPTIO
    StatusChange::NEGATIVE1,    // HP_BASILICA
    StatusChange::NEGATIVE1,    // HP_MEDITATIO
    StatusChange::NEGATIVE1,    // HW_SOULDRAIN
    StatusChange::NEGATIVE1,    // HW_MAGICCRASHER
    StatusChange::NEGATIVE1,    // HW_MAGICPOWER
    StatusChange::NEGATIVE1,    // PA_PRESSURE
    StatusChange::NEGATIVE1,    // PA_SACRIFICE
    StatusChange::NEGATIVE1,    // PA_GOSPEL
    // 370-
    StatusChange::NEGATIVE1,    // CH_PALMSTRIKE
    StatusChange::NEGATIVE1,    // CH_TIGERFIST
    StatusChange::NEGATIVE1,    // CH_CHAINCRUSH
    StatusChange::NEGATIVE1,    // PF_HPCONVERSION
    StatusChange::NEGATIVE1,    // PF_SOULCHANGE
    StatusChange::NEGATIVE1,    // PF_SOULBURN
    StatusChange::NEGATIVE1,    // ASC_KATAR
    StatusChange::NEGATIVE1,    // ASC_HALLUCINATION
    StatusChange::NEGATIVE1,    // ASC_EDP
    StatusChange::NEGATIVE1,    // ASC_BREAKER
    // 380-
    StatusChange::NEGATIVE1,    // SN_SIGHT
    StatusChange::NEGATIVE1,    // SN_FALCONASSAULT
    StatusChange::NEGATIVE1,    // SN_SHARPSHOOTING
    StatusChange::NEGATIVE1,    // SN_WINDWALK
    StatusChange::NEGATIVE1,    // WS_MELTDOWN
    StatusChange::NEGATIVE1,    // WS_CREATECOIN
    StatusChange::NEGATIVE1,    // WS_CREATENUGGET
    StatusChange::NEGATIVE1,    // WS_CARTBOOST
    StatusChange::NEGATIVE1,    // WS_SYSTEMCREATE
    StatusChange::NEGATIVE1,    // ST_CHASEWALK
    // 390-
    StatusChange::NEGATIVE1,    // ST_REJECTSWORD
    StatusChange::NEGATIVE1,    // ST_STEALBACKPACK
    StatusChange::NEGATIVE1,    // CR_ALCHEMY
    StatusChange::NEGATIVE1,    // CR_SYNTHESISPOTION
    StatusChange::NEGATIVE1,    // CG_ARROWVULCAN
    StatusChange::NEGATIVE1,    // CG_MOONLIT
    StatusChange::NEGATIVE1,    // CG_MARIONETTE
    StatusChange::NEGATIVE1,    // LK_SPIRALPIERCE
    StatusChange::NEGATIVE1,    // LK_HEADCRUSH
    StatusChange::NEGATIVE1,    // LK_JOINTBEAT
    // 400-
    StatusChange::NEGATIVE1,    // HW_NAPALMVULCAN
    StatusChange::NEGATIVE1,    // CH_SOULCOLLECT
    StatusChange::NEGATIVE1,    // PF_MINDBREAKER
    StatusChange::NEGATIVE1,    // PF_MEMORIZE
    StatusChange::NEGATIVE1,    // PF_FOGWALL
    StatusChange::NEGATIVE1,    // PF_SPIDERWEB
    StatusChange::NEGATIVE1,    // ASC_METEORASSAULT
    StatusChange::NEGATIVE1,    // ASC_CDP
    StatusChange::NEGATIVE1,    // WE_BABY
    StatusChange::NEGATIVE1,    // WE_CALLPARENT
    // 410-
    StatusChange::NEGATIVE1,    // WE_CALLBABY
    StatusChange::NEGATIVE1,    // TK_RUN
    StatusChange::NEGATIVE1,    // TK_READYSTORM
    StatusChange::NEGATIVE1,    // TK_STORMKICK
    StatusChange::NEGATIVE1,    // TK_READYDOWN
    StatusChange::NEGATIVE1,    // TK_DOWNKICK
    StatusChange::NEGATIVE1,    // TK_READYTURN
    StatusChange::NEGATIVE1,    // TK_TURNKICK
    StatusChange::NEGATIVE1,    // TK_READYCOUNTER
    StatusChange::NEGATIVE1,    // TK_COUNTER
}};

struct skill_name_db skill_names[] =
{
    {AC_OWL, "OWL", "Owl's_Eye"},

    {NPC_EMOTION, "EMOTION", "NPC_EMOTION"},
    {NPC_POISON, "POISON", "NPC_POISON"},
    {NPC_SELFDESTRUCTION, "SELFDESTRUCTION", "Kabooooom!"},
    {NPC_SUMMONSLAVE, "SUMMONSLAVE", "NPC_SUMMONSLAVE"},

    {NV_EMOTE, "EMOTE", "Emote_Skill"},
    {NV_TRADE, "TRADE", "Trade_Skill"},
    {NV_PARTY, "PARTY", "Party_Skill"},

    {TMW_MAGIC, "MAGIC", "General Magic"},
    {TMW_MAGIC_LIFE, "MAGIC_LIFE", "Life Magic"},
    {TMW_MAGIC_WAR, "MAGIC_WAR", "War Magic"},
    {TMW_MAGIC_TRANSMUTE, "MAGIC_TRANSMUTE", "Transmutation Magic"},
    {TMW_MAGIC_NATURE, "MAGIC_NATURE", "Nature Magic"},
    {TMW_MAGIC_ETHER, "MAGIC_ETHER", "Astral Magic"},
    {TMW_MAGIC_DARK, "MAGIC_DARK", "Dark Magic"},
    {TMW_MAGIC_LIGHT, "MAGIC_LIGHT", "Light Magic"},

    {TMW_BRAWLING, "BRAWLING", "Brawling"},
    {TMW_LUCKY_COUNTER, "LUCKY_COUNTER", "Lucky Counter"},
    {TMW_SPEED, "SPEED", "Speed"},
    {TMW_RESIST_POISON, "RESIST_POISON", "Resist Poison"},
    {TMW_ASTRAL_SOUL, "ASTRAL_SOUL", "Astral Soul"},
    {TMW_RAGING, "RAGING", "Raging"},

    {SkillID::ZERO, nullptr, nullptr}
};

static
const int dirx[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
static
const int diry[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

static
int rdamage;

earray<struct skill_db, SkillID, MAX_SKILL_DB> skill_db;


static
int skill_attack(BF attack_type, struct block_list *src,
        struct block_list *dsrc, struct block_list *bl,
        SkillID skillid, int skilllv, unsigned int tick, BCT flag);
static
int skill_delunitgroup(struct skill_unit_group *group);
static
void skill_devotion_end(struct map_session_data *md,
        struct map_session_data *sd, int target);
static
struct skill_unit *skill_initunit(struct skill_unit_group *group, int idx,
        int x, int y);
static
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
        int count, SkillID skillid, int skilllv, int unit_id);
static
void skill_status_change_timer(timer_id tid, tick_t tick,
        custom_id_t id, custom_data_t data);
static
int skill_unitgrouptickset_delete(struct block_list *bl, int group_id);
static
struct skill_unit_group_tickset *skill_unitgrouptickset_search(
        struct block_list *bl, int group_id);


int skill_get_hit(SkillID id)
{
    return skill_db[id].hit;
}

int skill_get_inf(SkillID id)
{
    return skill_db[id].inf;
}

int skill_get_pl(SkillID id)
{
    return skill_db[id].pl;
}

int skill_get_nk(SkillID id)
{
    return skill_db[id].nk;
}

int skill_get_max(SkillID id)
{
    return skill_db[id].max;
}

int skill_get_max_raise(SkillID id)
{
    return skill_db[id].max_raise;
}

int skill_get_range(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].range[lv - 1];
}

static
int skill_get_hp(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].hp[lv - 1];
}

int skill_get_sp(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].sp[lv - 1];
}

static
int skill_get_zeny(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].zeny[lv - 1];
}

int skill_get_num(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].num[lv - 1];
}

int skill_get_cast(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].cast[lv - 1];
}

int skill_get_delay(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].delay[lv - 1];
}

int skill_get_time(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].upkeep_time[lv - 1];
}

int skill_get_time2(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].upkeep_time2[lv - 1];
}

int skill_get_castdef(SkillID id)
{
    return skill_db[id].cast_def_rate;
}

int skill_get_weapontype(SkillID id)
{
    return skill_db[id].weapon;
}

int skill_get_inf2(SkillID id)
{
    return skill_db[id].inf2;
}

int skill_get_maxcount(SkillID id)
{
    return skill_db[id].maxcount;
}

int skill_get_blewcount(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].blewcount[lv - 1];
}

static
int skill_get_mhp(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].mhp[lv - 1];
}

static
int skill_get_castnodex(SkillID id, int lv)
{
    return (lv <= 0) ? 0 : skill_db[id].castnodex[lv - 1];
}

/* プロトタイプ */
static
struct skill_unit_group *skill_unitsetting(struct block_list *src,
                                            SkillID skillid, int skilllv,
                                            int x, int y, int flag);
static
int skill_check_condition(struct map_session_data *sd, int type);
static
void skill_status_change_timer_sub(struct block_list *bl,
        struct block_list *src, StatusChange type, unsigned int tick);
static
void skill_landprotector(struct block_list *bl, SkillID skillid, int *alive);
static
void skill_trap_splash(struct block_list *bl,
        struct block_list *src, int tick, int splash_count);
static
void skill_count_target(struct block_list *bl,
        struct block_list *src, int *c);

static
int distance(int x0, int y0, int x1, int y1)
{
    int dx, dy;

    dx = abs(x0 - x1);
    dy = abs(y0 - y1);
    return dx > dy ? dx : dy;
}

/*==========================================
 * スキル追加効果
 *------------------------------------------
 */
int skill_additional_effect(struct block_list *src, struct block_list *bl,
                             SkillID skillid, int skilllv, BF attack_type,
                             unsigned int)
{
    struct map_session_data *sd = NULL;
    struct mob_data *md = NULL;

    int luk;

    int sc_def_mdef, sc_def_vit, sc_def_int, sc_def_luk;
    int sc_def_mdef2, sc_def_vit2, sc_def_int2, sc_def_luk2;
    int sc_def_phys_shield_spell;

    nullpo_ret(src);
    nullpo_ret(bl);

    if (skilllv < 0)
        return 0;

    if (src->type == BL_PC)
    {
        nullpo_ret(sd = (struct map_session_data *) src);
    }
    else if (src->type == BL_MOB)
    {
        nullpo_ret(md = (struct mob_data *) src);  //未使用？
    }

    sc_def_phys_shield_spell = 0;
    if (battle_get_sc_data(bl)[SC_PHYS_SHIELD].timer != -1)
        sc_def_phys_shield_spell =
            battle_get_sc_data(bl)[SC_PHYS_SHIELD].val1;

    //対象の耐性
    luk = battle_get_luk(bl);
    sc_def_mdef = 100 - (3 + battle_get_mdef(bl) + luk / 3);
    sc_def_vit = 100 - (3 + battle_get_vit(bl) + luk / 3);
    sc_def_int = 100 - (3 + battle_get_int(bl) + luk / 3);
    sc_def_luk = 100 - (3 + luk);
    //自分の耐性
    luk = battle_get_luk(src);
    sc_def_mdef2 = 100 - (3 + battle_get_mdef(src) + luk / 3);
    sc_def_vit2 = 100 - (3 + battle_get_vit(src) + luk / 3);
    sc_def_int2 = 100 - (3 + battle_get_int(src) + luk / 3);
    sc_def_luk2 = 100 - (3 + luk);
    if (bl->type == BL_MOB)
    {
        if (sc_def_mdef > 50)
            sc_def_mdef = 50;
        if (sc_def_vit > 50)
            sc_def_vit = 50;
        if (sc_def_int > 50)
            sc_def_int = 50;
        if (sc_def_luk > 50)
            sc_def_luk = 50;
    }
    if (sc_def_mdef < 0)
        sc_def_mdef = 0;
    if (sc_def_vit < 0)
        sc_def_vit = 0;
    if (sc_def_int < 0)
        sc_def_int = 0;

    switch (skillid)
    {
        case NPC_POISON:
            if (MRAND(100) <
                50 - (sc_def_vit >> 2) - (sc_def_phys_shield_spell) +
                (skilllv >> 2))
                skill_status_change_start(bl, SC_POISON,
                                           skilllv, 0, 0, 0, skilllv, 0);
            break;
    }

    if (not (sd && bool(attack_type & BF_WEAPON)))
        return 0;
    earray<int, BadSC, BadSC::COUNT> arr_sc_def_card1 //=
    {{
        sc_def_mdef,    // stone
        sc_def_mdef,    // freeze
        sc_def_vit,     // stan
        sc_def_int,     // sleep
        sc_def_vit,     // poison
        sc_def_luk,     // curse
        sc_def_vit,     // silence
        sc_def_int,     // confusion
        sc_def_int,     // blind
    }}, arr_sc_def_card2 //=
    {{
        sc_def_mdef2,   // stone
        sc_def_mdef2,   // freeze
        sc_def_vit2,    // stan
        sc_def_int2,    // sleep
        sc_def_vit2,    // poison
        sc_def_luk2,    // curse
        sc_def_vit2,    // silence
        sc_def_int2,    // confusion
        sc_def_int2,    // blind
    }};

    for (BadSC bi : erange(BadSC(), BadSC::COUNT))
    {
        StatusChange si = BadSC_to_SC(bi);
        int sc_def_card1 = arr_sc_def_card1[bi];
        int eff1 = sd->addeff[bi];
        if (sd->state.arrow_atk)
            eff1 += sd->arrow_addeff[bi];
        if (MRAND(10000) < eff1 * sc_def_card1 / 100)
        {
            if (battle_config.battle_log)
                PRINTF("PC %d skill_addeff: cardによる異常発動 %d %d\n",
                     sd->bl.id, si, eff1);

            skill_status_change_start(bl, si, 7, 0, 0, 0,
                                    (bi == BadSC::CONFUSION)
                                    ? 10000 + 7000
                                    : 0,
                                    0);
        }

        int sc_def_card2 = arr_sc_def_card2[bi];
        int eff2 = sd->addeff2[bi];

        if (MRAND(10000) < eff2 * sc_def_card2 / 100)
        {
            if (battle_config.battle_log)
                PRINTF("PC %d skill_addeff: cardによる異常発動 %d %d\n",
                     src->id, si, eff2);
            skill_status_change_start(src, si, 7, 0, 0, 0,
                                   (bi == BadSC::CONFUSION)
                                   ? 10000 + 7000
                                   : 0,
                                   0);
        }
    }
    return 0;
}

/*=========================================================================
 スキル攻撃吹き飛ばし処理
-------------------------------------------------------------------------*/
static
int skill_blown(struct block_list *src, struct block_list *target, int count)
{
    int dx = 0, dy = 0, nx, ny;
    int x = target->x, y = target->y;
    int ret;
    MS prev_state = MS_IDLE;
    int moveblock;
    struct map_session_data *sd = NULL;
    struct mob_data *md = NULL;
    struct skill_unit *su = NULL;

    nullpo_ret(src);
    nullpo_ret(target);

    if (target->type == BL_PC)
    {
        nullpo_ret(sd = (struct map_session_data *) target);
    }
    else if (target->type == BL_MOB)
    {
        nullpo_ret(md = (struct mob_data *) target);
    }
    else if (target->type == BL_SKILL)
    {
        nullpo_ret(su = (struct skill_unit *) target);
    }
    else
        return 0;

    if (!(count & 0x10000 && (sd || md || su)))
    {                           /* 指定なしなら位置関係から方向を求める */
        dx = target->x - src->x;
        dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
        dy = target->y - src->y;
        dy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
    }
    if (dx == 0 && dy == 0)
    {
        int dir = battle_get_dir(target);
        if (dir >= 0 && dir < 8)
        {
            dx = -dirx[dir];
            dy = -diry[dir];
        }
    }

    ret = path_blownpos(target->m, x, y, dx, dy, count & 0xffff);
    nx = ret >> 16;
    ny = ret & 0xffff;
    moveblock = (x / BLOCK_SIZE != nx / BLOCK_SIZE
                 || y / BLOCK_SIZE != ny / BLOCK_SIZE);

    if (count & 0x20000)
    {
        battle_stopwalking(target, 1);
        if (sd)
        {
            sd->to_x = nx;
            sd->to_y = ny;
            sd->walktimer = 1;
            clif_walkok(sd);
            clif_movechar(sd);
        }
        else if (md)
        {
            md->to_x = nx;
            md->to_y = ny;
            prev_state = md->state.state;
            md->state.state = MS_WALK;
            clif_fixmobpos(md);
        }
    }
    else
        battle_stopwalking(target, 2);

    dx = nx - x;
    dy = ny - y;

    if (sd)                     /* 画面外に出たので消去 */
        map_foreachinmovearea(std::bind(clif_pcoutsight, ph::_1, sd),
                target->m, x - AREA_SIZE, y - AREA_SIZE,
                x + AREA_SIZE, y + AREA_SIZE,
                dx, dy, BL_NUL);
    else if (md)
        map_foreachinmovearea(std::bind(clif_moboutsight, ph::_1, md),
                target->m, x - AREA_SIZE, y - AREA_SIZE,
                x + AREA_SIZE, y + AREA_SIZE,
                dx, dy, BL_PC);

    if (su)
    {
        skill_unit_move_unit_group(su->group, target->m, dx, dy);
    }
    else
    {
//      eptr<struct status_change, StatusChange> sc_data=battle_get_sc_data(target);
        if (moveblock)
            map_delblock(target);
        target->x = nx;
        target->y = ny;
        if (moveblock)
            map_addblock(target);
/*ダンス中にエフェクトは移動しないらしい
                if (sc_data && sc_data[SC_DANCING].timer!=-1){ //対象がダンス中なのでエフェクトも移動
                        struct skill_unit_group *sg= (struct skill_unit_group *)sc_data[SC_DANCING].val2;
                        if (sg)
                                skill_unit_move_unit_group(sg,target->m,dx,dy);
                }
*/
    }

    if (sd)
    {                           /* 画面内に入ってきたので表示 */
        map_foreachinmovearea(std::bind(clif_pcinsight, ph::_1, sd),
                target->m, nx - AREA_SIZE, ny - AREA_SIZE,
                nx + AREA_SIZE, ny + AREA_SIZE,
                -dx, -dy, BL_NUL);
        if (count & 0x20000)
            sd->walktimer = -1;
    }
    else if (md)
    {
        map_foreachinmovearea(std::bind(clif_mobinsight, ph::_1, md),
                target->m, nx - AREA_SIZE, ny - AREA_SIZE,
                nx + AREA_SIZE, ny + AREA_SIZE,
                -dx, -dy, BL_PC);
        if (count & 0x20000)
            md->state.state = prev_state;
    }

    skill_unit_move(target, gettick(), (count & 0xffff) + 7); /* スキルユニットの判定 */

    return 0;
}

/*
 * =========================================================================
 * スキル攻撃効果処理まとめ
 * flagの説明。16進図
 *      00XRTTff
 *  ff  = magicで計算に渡される）
 *      TT      = パケットのtype部分(0でデフォルト）
 *  X   = パケットのスキルLv
 *  R   = 予約（skill_area_subで使用する）
 *-------------------------------------------------------------------------
 */

int skill_attack(BF attack_type, struct block_list *src,
        struct block_list *dsrc, struct block_list *bl,
        SkillID skillid, int skilllv, unsigned int tick, BCT flag)
{
    struct Damage dmg;
    eptr<struct status_change, StatusChange> sc_data;
    int type, lv, damage;

    rdamage = 0;
    nullpo_ret(src);
    nullpo_ret(dsrc);
    nullpo_ret(bl);

    sc_data = battle_get_sc_data(bl);

//何もしない判定ここから
    if (dsrc->m != bl->m)       //対象が同じマップにいなければ何もしない
        return 0;
    if (src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL)    //prevよくわからない※
        return 0;
    if (src->type == BL_PC && pc_isdead((struct map_session_data *) src))  //術者？がPCですでに死んでいたら何もしない
        return 0;
    if (dsrc->type == BL_PC && pc_isdead((struct map_session_data *) dsrc))    //術者？がPCですでに死んでいたら何もしない
        return 0;
    if (bl->type == BL_PC && pc_isdead((struct map_session_data *) bl))    //対象がPCですでに死んでいたら何もしない
        return 0;
    if (sc_data && sc_data[SC_HIDING].timer != -1)
    {                           //ハイディング状態で
        if (skill_get_pl(skillid) != 2)    //スキルの属性が地属性でなければ何もしない
            return 0;
    }
    if (src->type == BL_PC && ((struct map_session_data *) src)->chatID)    //術者がPCでチャット中なら何もしない
        return 0;
    if (dsrc->type == BL_PC && ((struct map_session_data *) dsrc)->chatID)  //術者がPCでチャット中なら何もしない
        return 0;

//何もしない判定ここまで

    type = -1;
    lv = (flag >> 20) & 0xf;
    dmg = battle_calc_attack(attack_type, src, bl, skillid, skilllv, flag & 0xff); //ダメージ計算

    damage = dmg.damage + dmg.damage2;

    if (lv == 15)
        lv = -1;

    if (flag & 0xff00)
        type = (flag & 0xff00) >> 8;

    if (damage <= 0 || damage < dmg.div_)   //吹き飛ばし判定？※
        dmg.blewcount = 0;

//武器スキル？ここから
    //AppleGirl Was Here
    if (bool(attack_type & BF_MAGIC)
        && damage > 0
        && src != bl
        && src == dsrc)
    {                           //Blah Blah
        if (bl->type == BL_PC)
        {                       //Blah Blah
            struct map_session_data *tsd = (struct map_session_data *) bl;
            if (tsd->magic_damage_return > 0)
            {                   //More Blah
                rdamage += damage * tsd->magic_damage_return / 100;
                if (rdamage < 1)
                    rdamage = 1;
            }
        }
    }
    //Stop Here
    if (bool(attack_type & BF_WEAPON)
        && damage > 0
        && src != bl
        && src == dsrc)
    {                           //武器スキル＆ダメージあり＆使用者と対象者が違う＆src=dsrc
        if (bool(dmg.flag & BF_SHORT))
        {                       //近距離攻撃時？※
            if (bl->type == BL_PC)
            {                   //対象がPCの時
                struct map_session_data *tsd = (struct map_session_data *) bl;
                nullpo_ret(tsd);
                if (tsd->short_weapon_damage_return > 0)
                {               //近距離攻撃跳ね返し？※
                    rdamage += damage * tsd->short_weapon_damage_return / 100;
                    if (rdamage < 1)
                        rdamage = 1;
                }
            }
        }
        else if (bool(dmg.flag & BF_LONG))
        {                       //遠距離攻撃時？※
            if (bl->type == BL_PC)
            {                   //対象がPCの時
                struct map_session_data *tsd = (struct map_session_data *) bl;
                nullpo_ret(tsd);
                if (tsd->long_weapon_damage_return > 0)
                {               //遠距離攻撃跳ね返し？※
                    rdamage += damage * tsd->long_weapon_damage_return / 100;
                    if (rdamage < 1)
                        rdamage = 1;
                }
            }
        }
        if (rdamage > 0)
            clif_damage(src, src, tick, dmg.amotion, 0, rdamage, 1, 4, 0);
    }
//武器スキル？ここまで

    switch (skillid)
    {
        case NPC_SELFDESTRUCTION:
            break;
        default:
            clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion,
                               damage, dmg.div_, skillid,
                               (lv != 0) ? lv : skilllv,
                               (skillid == SkillID::ZERO) ? 5 : type);
    }
    if (dmg.blewcount > 0)
    {                           /* 吹き飛ばし処理とそのパケット */
        skill_blown(dsrc, bl, dmg.blewcount);
        if (bl->type == BL_MOB)
            clif_fixmobpos((struct mob_data *) bl);
        else
            clif_fixpos(bl);
    }

    map_freeblock_lock();
    /* 実際にダメージ処理を行う */
    battle_damage(src, bl, damage, 0);

    /* ダメージがあるなら追加効果判定 */
    if (bl->prev != NULL)
    {
        struct map_session_data *sd = (struct map_session_data *) bl;
        nullpo_ret(sd);
        if (bl->type != BL_PC || (sd && !pc_isdead(sd)))
        {
            if (damage > 0)
                skill_additional_effect(src, bl, skillid, skilllv,
                                         attack_type, tick);
            if (bl->type == BL_MOB && src != bl)    /* スキル使用条件のMOBスキル */
            {
                struct mob_data *md = (struct mob_data *) bl;
                nullpo_ret(md);
                if (battle_config.mob_changetarget_byskill == 1)
                {
                    int target;
                    target = md->target_id;
                    if (src->type == BL_PC)
                        md->target_id = src->id;
                    mobskill_use(md, tick, MSC_SKILLUSED, skillid);
                    md->target_id = target;
                }
                else
                    mobskill_use(md, tick, MSC_SKILLUSED, skillid);
            }
        }
    }

    if (src->type == BL_PC
        && bool(dmg.flag & BF_WEAPON)
        && src != bl
        && src == dsrc
        && damage > 0)
    {
        struct map_session_data *sd = (struct map_session_data *) src;
        int hp = 0, sp = 0;
        nullpo_ret(sd);
        if (sd->hp_drain_rate && dmg.damage > 0
            && MRAND(100) < sd->hp_drain_rate)
        {
            hp += (dmg.damage * sd->hp_drain_per) / 100;
        }
        if (sd->hp_drain_rate_ && dmg.damage2 > 0
            && MRAND(100) < sd->hp_drain_rate_)
        {
            hp += (dmg.damage2 * sd->hp_drain_per_) / 100;
        }
        if (sd->sp_drain_rate > 0 && dmg.damage > 0
            && MRAND(100) < sd->sp_drain_rate)
        {
            sp += (dmg.damage * sd->sp_drain_per) / 100;
        }
        if (sd->sp_drain_rate_ > 0 && dmg.damage2 > 0
            && MRAND(100) < sd->sp_drain_rate_)
        {
            sp += (dmg.damage2 * sd->sp_drain_per_) / 100;
        }
        if (hp || sp)
            pc_heal(sd, hp, sp);
    }

    if (rdamage > 0)
        battle_damage(bl, src, rdamage, 0);

    map_freeblock_unlock();

    return (dmg.damage + dmg.damage2);  /* 与ダメを返す */
}

typedef int(*SkillFunc)(struct block_list *, struct block_list *,
        SkillID, int,
        unsigned int, BCT);

static
void skill_area_sub(struct block_list *bl,
        struct block_list *src, SkillID skill_id, int skill_lv,
        unsigned int tick, BCT flag, SkillFunc func)
{
    nullpo_retv(bl);

    if (bl->type != BL_PC && bl->type != BL_MOB && bl->type != BL_SKILL)
        return;

    if (battle_check_target(src, bl, flag) > 0)
        func(src, bl, skill_id, skill_lv, tick, flag);
}

int skill_check_unit_range(int, int, int, int, SkillID)
{
    return 0;
}

static
void skill_check_unit_range2_sub(struct block_list *bl, int *c)
{
    nullpo_retv(bl);
    nullpo_retv(c);

    if (bl->prev == NULL || (bl->type != BL_PC && bl->type != BL_MOB))
        return;

    if (bl->type == BL_PC && pc_isdead((struct map_session_data *) bl))
        return;

    (*c)++;
}

int skill_check_unit_range2(int m, int x, int y, int range)
{
    int c = 0;

    map_foreachinarea(std::bind(skill_check_unit_range2_sub, ph::_1, &c),
            m, x - range, y - range,
            x + range, y + range, BL_NUL);

    return c;
}

/*==========================================
 *
 *------------------------------------------
 */
static
void skill_timer(timer_id, tick_t tick, custom_id_t id, custom_data_t data)
{
    struct map_session_data *sd = NULL;
    struct mob_data *md = NULL;
    struct block_list *src = map_id2bl(id), *target;
    struct skill_timerskill *skl = NULL;

    nullpo_retv(src);

    if (src->prev == NULL)
        return;

    if (src->type == BL_PC)
    {
        nullpo_retv(sd = (struct map_session_data *) src);
        skl = &sd->skilltimerskill[data];
    }
    else if (src->type == BL_MOB)
    {
        nullpo_retv(md = (struct mob_data *) src);
        skl = &md->skilltimerskill[data];
    }

    else
        return;

    nullpo_retv(skl);

    skl->timer = -1;
    if (skl->target_id)
    {
        target = map_id2bl(skl->target_id);
        if (target == NULL)
            return;
        if (target->prev == NULL)
            return;
        if (src->m != target->m)
            return;
        if (sd && pc_isdead(sd))
            return;
        if (target->type == BL_PC
            && pc_isdead((struct map_session_data *) target))
            return;

        switch (skl->skill_id)
        {
            default:
                skill_attack(skl->type.bf, src, src, target, skl->skill_id,
                              skl->skill_lv, tick, skl->flag);
                break;
        }
    }
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_cleartimerskill(struct block_list *src)
{
    int i;

    nullpo_ret(src);

    if (src->type == BL_PC)
    {
        struct map_session_data *sd = (struct map_session_data *) src;
        nullpo_ret(sd);
        for (i = 0; i < MAX_SKILLTIMERSKILL; i++)
        {
            if (sd->skilltimerskill[i].timer != -1)
            {
                delete_timer(sd->skilltimerskill[i].timer, skill_timer);
                sd->skilltimerskill[i].timer = -1;
            }
        }
    }
    else if (src->type == BL_MOB)
    {
        struct mob_data *md = (struct mob_data *) src;
        nullpo_ret(md);
        for (i = 0; i < MAX_MOBSKILLTIMERSKILL; i++)
        {
            if (md->skilltimerskill[i].timer != -1)
            {
                delete_timer(md->skilltimerskill[i].timer, skill_timer);
                md->skilltimerskill[i].timer = -1;
            }
        }
    }

    return 0;
}

/* 範囲スキル使用処理小分けここまで
 * -------------------------------------------------------------------------
 */

// these variables are set in the 'else' branches,
// and used in the (recursive) 'if' branch
static int skill_area_temp_id, skill_area_temp_hp;


/*==========================================
 * スキル使用（詠唱完了、ID指定攻撃系）
 * （スパゲッティに向けて１歩前進！(ダメポ)）
 *------------------------------------------
 */
int skill_castend_damage_id(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv,
        unsigned int tick, BCT flag)
{
    struct map_session_data *sd = NULL;

    nullpo_retr(1, src);
    nullpo_retr(1, bl);

    if (src->type == BL_PC)
        sd = (struct map_session_data *) src;
    if (sd && pc_isdead(sd))
        return 1;

    if (bl->prev == NULL)
        return 1;
    if (bl->type == BL_PC && pc_isdead((struct map_session_data *) bl))
        return 1;
    map_freeblock_lock();
    switch (skillid)
    {
        case NPC_POISON:
            skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick,
                          flag);
            break;

        case NPC_SELFDESTRUCTION:  /* 自爆 */
            if (flag & 1)
            {
                /* 個別にダメージを与える */
                if (src->type == BL_MOB)
                {
                    struct mob_data *mb = (struct mob_data *) src;
                    nullpo_retr(1, mb);
                    mb->hp = skill_area_temp_hp;
                    if (bl->id != skill_area_temp_id)
                        skill_attack(BF_MISC, src, src, bl,
                                      NPC_SELFDESTRUCTION, skilllv, tick,
                                      flag);
                    mb->hp = 1;
                }
            }
            else
            {
                struct mob_data *md;
                if ((md = (struct mob_data *) src))
                {
                    skill_area_temp_id = bl->id;
                    skill_area_temp_hp = battle_get_hp(src);
                    map_foreachinarea(std::bind(skill_area_sub, ph::_1, src, skillid, skilllv, tick, flag | BCT_ENEMY | BCT_lo_x01, skill_castend_damage_id),
                            bl->m, bl->x - 5, bl->y - 5,
                            bl->x + 5, bl->y + 5, BL_NUL);
                    battle_damage(src, src, md->hp, 0);
                }
            }
            break;

            /* HP吸収/HP吸収魔法 */
        case SkillID::ZERO:
            if (sd)
            {
                if (flag & 3)
                {
                    if (bl->id != skill_area_temp_id)
                        skill_attack(BF_WEAPON, src, src, bl, skillid,
                                      skilllv, tick, BCT_mid_x05);
                }
                else
                {
                    int ar = sd->splash_range;
                    skill_area_temp_id = bl->id;
                    map_foreachinarea(std::bind(skill_area_sub, ph::_1, src, skillid, skilllv, tick, flag | BCT_ENEMY | BCT_lo_x01, skill_castend_damage_id),
                            bl->m, bl->x - ar, bl->y - ar,
                            bl->x + ar, bl->y + ar, BL_NUL);
                }
            }
            break;

        default:
            map_freeblock_unlock();
            return 1;
    }
    map_freeblock_unlock();

    return 0;
}

/*==========================================
 * スキル使用（詠唱完了、ID指定支援系）
 *------------------------------------------
 */
int skill_castend_nodamage_id(struct block_list *src, struct block_list *bl,
        SkillID skillid, int skilllv,
        unsigned int, BCT)
{
    struct map_session_data *sd = NULL;
    struct map_session_data *dstsd = NULL;
    struct mob_data *md = NULL;
    struct mob_data *dstmd = NULL;
    int sc_def_vit, sc_def_mdef, strip_fix;

    nullpo_retr(1, src);
    nullpo_retr(1, bl);

    if (src->type == BL_PC)
        sd = (struct map_session_data *) src;
    else if (src->type == BL_MOB)
        md = (struct mob_data *) src;

    sc_def_vit = 100 - (3 + battle_get_vit(bl) + battle_get_luk(bl) / 3);
    sc_def_vit = 100 - (3 + battle_get_vit(bl) + battle_get_luk(bl) / 3);
    sc_def_mdef = 100 - (3 + battle_get_mdef(bl) + battle_get_luk(bl) / 3);
    strip_fix = battle_get_dex(src) - battle_get_dex(bl);

    if (bl->type == BL_PC)
    {
        nullpo_retr(1, dstsd = (struct map_session_data *) bl);
    }
    else if (bl->type == BL_MOB)
    {
        nullpo_retr(1, dstmd = (struct mob_data *) bl);
        if (sc_def_vit > 50)
            sc_def_vit = 50;
        if (sc_def_mdef > 50)
            sc_def_mdef = 50;
    }
    if (sc_def_vit < 0)
        sc_def_vit = 0;
    if (sc_def_mdef < 0)
        sc_def_mdef = 0;
    if (strip_fix < 0)
        strip_fix = 0;

    if (bl == NULL || bl->prev == NULL)
        return 1;
    if (sd && pc_isdead(sd))
        return 1;
    if (dstsd && pc_isdead(dstsd))
        return 1;

    map_freeblock_lock();
    switch (skillid)
    {
        case NPC_SELFDESTRUCTION:  /* 自爆 */
            skill_status_change_start(bl, SkillStatusChangeTable[skillid],
                                       skilllv, uint16_t(skillid), 0, 0,
                                       skill_get_time(skillid, skilllv), 0);
            break;

        case NPC_SUMMONSLAVE:  /* 手下召喚 */
            if (md && !md->master_id)
            {
                mob_summonslave(md,
                                 mob_db[md->mob_class].skill[md->skillidx].val,
                                 skilllv,
                                 (true) ? 1 : 0);
            }
            break;

        case NPC_EMOTION:      /* エモーション */
            if (md)
                clif_emotion(&md->bl,
                              mob_db[md->mob_class].skill[md->skillidx].val[0]);
            break;

        default:
            PRINTF("Unknown skill used:%d\n", skillid);
            map_freeblock_unlock();
            return 1;
    }

    map_freeblock_unlock();
    return 0;
}

/*==========================================
 * スキル使用（詠唱完了、ID指定）
 *------------------------------------------
 */
static
void skill_castend_id(timer_id tid, tick_t tick, custom_id_t id, custom_data_t)
{
    struct map_session_data *sd = map_id2sd(id) /*,*target_sd=NULL */ ;
    struct block_list *bl;
    int range, inf2;

    nullpo_retv( sd);

    if (sd->bl.prev == NULL)    //prevが無いのはありなの？
        return;

    if (sd->skilltimer != tid)  /* タイマIDの確認 */
        return;
    sd->skilltimer = -1;

    if ((bl = map_id2bl(sd->skilltarget)) == NULL || bl->prev == NULL)
    {
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return;
    }
    if (sd->bl.m != bl->m || pc_isdead(sd))
    {                           //マップが違うか自分が死んでいる
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return;
    }

    inf2 = skill_get_inf2(sd->skillid);
    if (((skill_get_inf(sd->skillid) & 1) || inf2 & 4) &&  // 彼我敵対関係チェック
        battle_check_target(&sd->bl, bl, BCT_ENEMY) <= 0)
    {
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return;
    }
    if (inf2 & 0xC00 && sd->bl.id != bl->id)
    {
        int fail_flag = 1;
        if (inf2 & 0x400 && battle_check_target(&sd->bl, bl, BCT_PARTY) > 0)
            fail_flag = 0;
        if (fail_flag)
        {
            clif_skill_fail(sd, sd->skillid, 0, 0);
            sd->canact_tick = tick;
            sd->canmove_tick = tick;
            sd->skillitem = SkillID::NEGATIVE;
            sd->skillitemlv = -1;
            return;
        }
    }

    range = skill_get_range(sd->skillid, sd->skilllv);
    if (range < 0)
        range = battle_get_range(&sd->bl) - (range + 1);
    range += battle_config.pc_skill_add_range;
    if (battle_config.skill_out_range_consume)
    {
        // changed to allow casting when target walks out of range [Valaris]
        if (range < distance(sd->bl.x, sd->bl.y, bl->x, bl->y))
        {
            clif_skill_fail(sd, sd->skillid, 0, 0);
            sd->canact_tick = tick;
            sd->canmove_tick = tick;
            sd->skillitem = SkillID::NEGATIVE;
            sd->skillitemlv = -1;
            return;
        }
    }
    if (!skill_check_condition(sd, 1))
    {                           /* 使用条件チェック */
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return;
    }
    sd->skillitem = SkillID::NEGATIVE;
    sd->skillitemlv = -1;
    if (battle_config.skill_out_range_consume)
    {
        if (range < distance(sd->bl.x, sd->bl.y, bl->x, bl->y))
        {
            clif_skill_fail(sd, sd->skillid, 0, 0);
            sd->canact_tick = tick;
            sd->canmove_tick = tick;
            return;
        }
    }

    if (battle_config.pc_skill_log)
        PRINTF("PC %d skill castend skill=%d\n",
                sd->bl.id, sd->skillid);
    pc_stop_walking(sd, 0);

    switch (skill_get_nk(sd->skillid))
    {
            /* 攻撃系/吹き飛ばし系 */
        case 0:
        case 2:
            skill_castend_damage_id(&sd->bl, bl,
                    sd->skillid, sd->skilllv,
                    tick, BCT_ZERO);
            break;
        case 1:                /* 支援系 */
            skill_castend_nodamage_id(&sd->bl, bl,
                    sd->skillid, sd->skilllv,
                    tick, BCT_ZERO);
            break;
    }
}

/*==========================================
 * スキル使用（詠唱完了、map指定）
 *------------------------------------------
 */
int skill_castend_map(struct map_session_data *sd, SkillID skill_num,
                       const char *mapname)
{
    nullpo_ret(sd);
    if (sd->bl.prev == NULL || pc_isdead(sd))
        return 0;

    if (bool(sd->opt1)
        || bool(sd->status.option & Option::HIDE2))
        return 0;

    if (skill_num != sd->skillid)   /* 不正パケットらしい */
        return 0;

    pc_stopattack(sd);

    if (battle_config.pc_skill_log)
        PRINTF("PC %d skill castend skill =%d map=%s\n",
                sd->bl.id, skill_num, mapname);
    pc_stop_walking(sd, 0);

    if (strcmp(mapname, "cancel") == 0)
        return 0;

    return 0;
}

/*==========================================
 * スキルユニット設定処理
 *------------------------------------------
 */
struct skill_unit_group *skill_unitsetting(struct block_list *src,
        SkillID skillid, int skilllv,
        int x, int y, int)
{
    struct skill_unit_group *group;
    int i, count = 1, limit_ = 10000, val1_ = 0, val2_ = 0;
    BCT target = BCT_ENEMY;
    int interval = 1000, range_ = 0;

    nullpo_ret(src);

    nullpo_retr(NULL, group = skill_initunitgroup(src, count, skillid, skilllv, 0));
    group->limit = limit_;
    group->val1 = val1_;
    group->val2 = val2_;
    group->target_flag = target;
    group->interval = interval;
    group->range = range_;
    for (i = 0; i < count; i++)
    {
        struct skill_unit *unit;
        int ux = x, uy = y, val1 = skilllv, val2 = 0, limit =
            group->limit, alive = 1;
        int range = group->range;
        //直上スキルの場合設置座標上にランドプロテクターがないかチェック
        if (range <= 0)
            map_foreachinarea(std::bind(skill_landprotector, ph::_1, skillid, &alive),
                    src->m, ux, uy,
                    ux, uy, BL_SKILL);

        if (alive)
        {
            nullpo_retr(NULL, unit = skill_initunit(group, i, ux, uy));
            unit->val1 = val1;
            unit->val2 = val2;
            unit->limit = limit;
            unit->range = range;
        }
    }
    return group;
}

/*==========================================
 * スキルユニットの発動イベント
 *------------------------------------------
 */
static
int skill_unit_onplace(struct skill_unit *src, struct block_list *bl,
                        unsigned int tick)
{
    struct skill_unit_group *sg;
    struct block_list *ss;
    struct skill_unit_group_tickset *ts;
    struct map_session_data *srcsd = NULL;
    int diff, goflag, splash_count = 0;

    nullpo_ret(src);
    nullpo_ret(bl);

    if (bl->prev == NULL || !src->alive
        || (bl->type == BL_PC && pc_isdead((struct map_session_data *) bl)))
        return 0;

    nullpo_ret(sg = src->group);
    nullpo_ret(ss = map_id2bl(sg->src_id));

    if (ss->type == BL_PC)
        nullpo_ret(srcsd = (struct map_session_data *) ss);
    if (srcsd && srcsd->chatID)
        return 0;

    if (bl->type != BL_PC && bl->type != BL_MOB)
        return 0;
    nullpo_ret(ts = skill_unitgrouptickset_search(bl, sg->group_id));
    diff = DIFF_TICK(tick, ts->tick);
    goflag = (diff > sg->interval || diff < 0);

    //対象がLP上に居る場合は無効
    map_foreachinarea(std::bind(skill_landprotector, ph::_1, SkillID::ZERO, &goflag),
            bl->m, bl->x, bl->y,
            bl->x, bl->y, BL_SKILL);

    if (!goflag)
        return 0;
    ts->tick = tick;
    ts->group_id = sg->group_id;

    switch (sg->unit_id)
    {
        case 0x83:             /* サンクチュアリ */
        {
            int race = battle_get_race(bl);
            int damage_flag =
                (battle_check_undead(race, battle_get_elem_type(bl))
                 || race == 6) ? 1 : 0;

            if (battle_get_hp(bl) >= battle_get_max_hp(bl) && !damage_flag)
                break;

            if ((sg->val1--) <= 0)
            {
                skill_delunitgroup(sg);
                return 0;
            }
            if (!damage_flag)
            {
                int heal = sg->val2;
                if (bl->type == BL_PC
                    && ((struct map_session_data *) bl)->
                    special_state.no_magic_damage)
                    heal = 0;   /* 黄金蟲カード（ヒール量０） */
                battle_heal(NULL, bl, heal, 0, 0);
            }
            else
                skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id,
                              sg->skill_lv, tick, BCT_ZERO);
        }
            break;

        case 0x84:             /* マグヌスエクソシズム */
        {
            int race = battle_get_race(bl);
            int damage_flag =
                (battle_check_undead(race, battle_get_elem_type(bl))
                 || race == 6) ? 1 : 0;

            if (!damage_flag)
                return 0;
            skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id,
                          sg->skill_lv, tick, BCT_ZERO);
        }
            break;

        case 0x85:             /* ニューマ */
        {
            struct skill_unit *unit2;
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            if (sc_data && sc_data[SC_PNEUMA].timer == -1)
                skill_status_change_start(bl, SC_PNEUMA, sg->skill_lv,
                        (int) src, 0, 0, 0, 0);
            else if ((unit2 = (struct skill_unit *) sc_data[SC_PNEUMA].val2)
                     && unit2 != src)
            {
                if (DIFF_TICK(sg->tick, unit2->group->tick) > 0)
                    skill_status_change_start(bl, SC_PNEUMA, sg->skill_lv,
                                               (int) src, 0, 0, 0, 0);
                ts->tick -= sg->interval;
            }
        }
            break;
        case 0x7e:             /* セイフティウォール */
        {
            struct skill_unit *unit2;
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            if (sc_data && sc_data[SC_SAFETYWALL].timer == -1)
                skill_status_change_start(bl, SC_SAFETYWALL, sg->skill_lv,
                        (int) src, 0, 0, 0, 0);
            else if ((unit2 = (struct skill_unit *) sc_data[SC_SAFETYWALL].val2)
                     && unit2 != src)
            {
                if (sg->val1 < unit2->group->val1)
                    skill_status_change_start(bl, SC_SAFETYWALL, sg->skill_lv,
                                               (int) src, 0, 0, 0, 0);
                ts->tick -= sg->interval;
            }
        }
            break;

        case 0x86:             /* ロードオブヴァーミリオン(＆ストームガスト ＆グランドクロス) */
            skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id,
                          sg->skill_lv, tick, BCT_ZERO);
            break;

        case 0x7f:             /* ファイヤーウォール */
            if ((src->val2--) > 0)
                skill_attack(BF_MAGIC, ss, &src->bl, bl,
                              sg->skill_id, sg->skill_lv, tick, BCT_ZERO);
            if (src->val2 <= 0)
                skill_delunit(src);
            break;

        case 0x87:             /* ファイアーピラー(発動前) */
            skill_delunit(src);
            skill_unitsetting(ss, sg->skill_id, sg->skill_lv, src->bl.x,
                               src->bl.y, 1);
            break;

        case 0x88:             /* ファイアーピラー(発動後) */
            if (DIFF_TICK(tick, sg->tick) < 150)
                skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id,
                              sg->skill_lv, tick, BCT_ZERO);
            break;

        case 0x90:             /* スキッドトラップ */
        {
            int i, c = skill_get_blewcount(sg->skill_id, sg->skill_lv);
            for (i = 0; i < c; i++)
                skill_blown(&src->bl, bl, 1 | 0x30000);
            sg->unit_id = 0x8c;
            clif_changelook(&src->bl, LOOK_BASE, sg->unit_id);
            sg->limit = DIFF_TICK(tick, sg->tick) + 1500;
        }
            break;

        case 0x93:             /* ランドマイン */
            skill_attack(BF_MISC, ss, &src->bl, bl, sg->skill_id,
                          sg->skill_lv, tick, BCT_ZERO);
            sg->unit_id = 0x8c;
            clif_changelook(&src->bl, LOOK_BASE, 0x88);
            sg->limit = DIFF_TICK(tick, sg->tick) + 1500;
            break;

        case 0x8f:             /* ブラストマイン */
        case 0x94:             /* ショックウェーブトラップ */
        case 0x95:             /* サンドマン */
        case 0x96:             /* フラッシャー */
        case 0x97:             /* フリージングトラップ */
        case 0x98:             /* クレイモアートラップ */
            map_foreachinarea(std::bind(skill_count_target, ph::_1,  &src->bl, &splash_count),
                    src->bl.m, src->bl.x - src->range, src->bl.y - src->range,
                    src->bl.x + src->range, src->bl.y + src->range, BL_NUL);
            map_foreachinarea(std::bind(skill_trap_splash, ph::_1, &src->bl, tick, splash_count),
                    src->bl.m, src->bl.x - src->range, src->bl.y - src->range,
                    src->bl.x + src->range, src->bl.y + src->range, BL_NUL);
            sg->unit_id = 0x8c;
            clif_changelook(&src->bl, LOOK_BASE, sg->unit_id);
            sg->limit = DIFF_TICK(tick, sg->tick) + 1500;
            break;

        case 0x91:             /* アンクルスネア */
        {
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            if (sg->val2 == 0 && sc_data && sc_data[SC_ANKLE].timer == -1)
            {
                int moveblock = (bl->x / BLOCK_SIZE != src->bl.x / BLOCK_SIZE
                                  || bl->y / BLOCK_SIZE !=
                                  src->bl.y / BLOCK_SIZE);
                int sec = skill_get_time2(sg->skill_id,
                                            sg->skill_lv) -
                    (double) battle_get_agi(bl) * 0.1;
                if (bool(battle_get_mode(bl) & MobMode::BOSS))
                    sec = sec / 5;
                battle_stopwalking(bl, 1);
                skill_status_change_start(bl, SC_ANKLE,
                        sg->skill_lv, 0, 0, 0,
                        sec, 0);

                if (moveblock)
                    map_delblock(bl);
                bl->x = src->bl.x;
                bl->y = src->bl.y;
                if (moveblock)
                    map_addblock(bl);
                if (bl->type == BL_MOB)
                    clif_fixmobpos((struct mob_data *) bl);
                else
                    clif_fixpos(bl);
                sg->limit = DIFF_TICK(tick, sg->tick) + sec;
                sg->val2 = bl->id;
            }
        }
            break;

        case 0x80:             /* ワープポータル(発動後) */
            if (bl->type == BL_PC)
            {
                struct map_session_data *sd = (struct map_session_data *) bl;
                if (sd && src->bl.m == bl->m && src->bl.x == bl->x
                    && src->bl.y == bl->y && src->bl.x == sd->to_x
                    && src->bl.y == sd->to_y)
                {
                    if (battle_config.chat_warpportal || !sd->chatID)
                    {
                        if ((sg->val1--) > 0)
                        {
                            pc_setpos(sd, sg->valstr, sg->val2 >> 16,
                                       sg->val2 & 0xffff, 3);
                            if (sg->src_id == bl->id
                                || (strcmp(map[src->bl.m].name, sg->valstr)
                                    == 0 && src->bl.x == (sg->val2 >> 16)
                                    && src->bl.y == (sg->val2 & 0xffff)))
                                skill_delunitgroup(sg);
                        }
                        else
                            skill_delunitgroup(sg);
                    }
                }
            }
            else if (bl->type == BL_MOB && battle_config.mob_warpportal)
            {
                int m = map_mapname2mapid(sg->valstr);
                mob_warp((struct mob_data *) bl, m, sg->val2 >> 16,
                          sg->val2 & 0xffff, 3);
            }
            break;

        case 0x8e:             /* クァグマイア */
        {
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (bl->type == BL_PC
                && ((struct map_session_data *) bl)->
                special_state.no_magic_damage)
                break;
            if (battle_get_sc_data(bl)[type].timer == -1)
                skill_status_change_start(bl, type, sg->skill_lv, (int) src,
                                           0, 0,
                                           skill_get_time2(sg->skill_id,
                                                            sg->skill_lv), 0);
        }
            break;
        case 0x92:             /* ベノムダスト */
        {
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (sc_data && sc_data[type].timer == -1)
                skill_status_change_start(bl, type, sg->skill_lv, (int) src,
                                           0, 0,
                                           skill_get_time2(sg->skill_id,
                                                            sg->skill_lv), 0);
        }
            break;
        case 0x9a:             /* ボルケーノ */
        case 0x9b:             /* デリュージ */
        case 0x9c:             /* バイオレントゲイル */
        {
            struct skill_unit *unit2;
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (sc_data && sc_data[type].timer == -1)
                skill_status_change_start(bl, type, sg->skill_lv, (int) src,
                                           0, 0,
                                           skill_get_time2(sg->skill_id,
                                                            sg->skill_lv), 0);
            else if ((unit2 = (struct skill_unit *) sc_data[type].val2)
                     && unit2 != src)
            {
                if (DIFF_TICK(sg->tick, unit2->group->tick) > 0)
                    skill_status_change_start(bl, type, sg->skill_lv,
                                               (int) src, 0, 0,
                                               skill_get_time2(sg->skill_id,
                                                                sg->skill_lv),
                                               0);
                ts->tick -= sg->interval;
            }
        } break;

        case 0x9e:             /* 子守唄 */
        case 0x9f:             /* ニヨルドの宴 */
        case 0xa0:             /* 永遠の混沌 */
        case 0xa1:             /* 戦太鼓の響き */
        case 0xa2:             /* ニーベルングの指輪 */
        case 0xa3:             /* ロキの叫び */
        case 0xa4:             /* 深淵の中に */
        case 0xa5:             /* 不死身のジークフリード */
        case 0xa6:             /* 不協和音 */
        case 0xa7:             /* 口笛 */
        case 0xa8:             /* 夕陽のアサシンクロス */
        case 0xa9:             /* ブラギの詩 */
        case 0xab:             /* 自分勝手なダンス */
        case 0xac:             /* ハミング */
        case 0xad:             /* 私を忘れないで… */
        case 0xae:             /* 幸運のキス */
        case 0xaf:             /* サービスフォーユー */
        case 0xb4:
        {
            struct skill_unit *unit2;
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (sg->src_id == bl->id)
                break;
            if (sc_data && sc_data[type].timer == -1)
                skill_status_change_start(bl, type, sg->skill_lv, sg->val1,
                                           sg->val2, (int) src,
                                           skill_get_time2(sg->skill_id,
                                                            sg->skill_lv), 0);
            else if ((unit2 = (struct skill_unit *) sc_data[type].val4)
                     && unit2 != src)
            {
                if (unit2->group
                    && DIFF_TICK(sg->tick, unit2->group->tick) > 0)
                    skill_status_change_start(bl, type, sg->skill_lv,
                                               sg->val1, sg->val2, (int) src,
                                               skill_get_time2(sg->skill_id,
                                                                sg->skill_lv),
                                               0);
                ts->tick -= sg->interval;
            }
        } break;

        case 0xaa:             /* イドゥンの林檎 */
        {
            struct skill_unit *unit2;
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (sg->src_id == bl->id)
                break;
            if (sc_data && sc_data[type].timer == -1)
                skill_status_change_start(bl, type, sg->skill_lv,
                                           (sg->val1) >> 16,
                                           (sg->val1) & 0xffff, (int) src,
                                           skill_get_time2(sg->skill_id,
                                                            sg->skill_lv), 0);
            else if ((unit2 = (struct skill_unit *) sc_data[type].val4)
                     && unit2 != src)
            {
                if (DIFF_TICK(sg->tick, unit2->group->tick) > 0)
                    skill_status_change_start(bl, type, sg->skill_lv,
                                               (sg->val1) >> 16,
                                               (sg->val1) & 0xffff, (int) src,
                                               skill_get_time2(sg->skill_id,
                                                                sg->skill_lv),
                                               0);
                ts->tick -= sg->interval;
            }
        } break;

        case 0xb1:             /* デモンストレーション */
            skill_attack(BF_WEAPON, ss, &src->bl, bl, sg->skill_id,
                          sg->skill_lv, tick, BCT_ZERO);
            if (bl->type == BL_PC && MRAND(100) < sg->skill_lv
                && battle_config.equipment_breaking)
                pc_breakweapon((struct map_session_data *) bl);
            break;
        case 0x99:             /* トーキーボックス */
            if (sg->src_id == bl->id)   //自分が踏んでも発動しない
                break;
            if (sg->val2 == 0)
            {
                sg->unit_id = 0x8c;
                clif_changelook(&src->bl, LOOK_BASE, sg->unit_id);
                sg->limit = DIFF_TICK(tick, sg->tick) + 5000;
                sg->val2 = -1;  //踏んだ
            }
            break;
        case 0xb2:             /* あなたを_会いたいです */
        case 0xb3:             /* ゴスペル */
        case 0xb6:             /* フォグウォール */
            //とりあえず何もしない
            break;

        case 0xb7:             /* スパイダーウェッブ */
            if (sg->val2 == 0)
            {
                int moveblock = (bl->x / BLOCK_SIZE != src->bl.x / BLOCK_SIZE
                                  || bl->y / BLOCK_SIZE !=
                                  src->bl.y / BLOCK_SIZE);
                skill_additional_effect(ss, bl, sg->skill_id, sg->skill_lv,
                                         BF_MISC, tick);
                if (moveblock)
                    map_delblock(bl);
                bl->x = (&src->bl)->x;
                bl->y = (&src->bl)->y;
                if (moveblock)
                    map_addblock(bl);
                if (bl->type == BL_MOB)
                    clif_fixmobpos((struct mob_data *) bl);
                else
                    clif_fixpos(bl);
                sg->limit =
                    DIFF_TICK(tick,
                               sg->tick) + skill_get_time2(sg->skill_id,
                                                            sg->skill_lv);
                sg->val2 = bl->id;
            }
            break;

/*      default:
                if (battle_config.error_log)
                        PRINTF("skill_unit_onplace: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
                break;*/
    }
    if (bl->type == BL_MOB && ss != bl) /* スキル使用条件のMOBスキル */
    {
        if (battle_config.mob_changetarget_byskill == 1)
        {
            int target = ((struct mob_data *) bl)->target_id;
            if (ss->type == BL_PC)
                ((struct mob_data *) bl)->target_id = ss->id;
            mobskill_use((struct mob_data *) bl, tick,
                          MSC_SKILLUSED, sg->skill_id);
            ((struct mob_data *) bl)->target_id = target;
        }
        else
            mobskill_use((struct mob_data *) bl, tick,
                          MSC_SKILLUSED, sg->skill_id);
    }

    return 0;
}

/*==========================================
 * スキルユニットから離脱する(もしくはしている)場合
 *------------------------------------------
 */
static
int skill_unit_onout(struct skill_unit *src, struct block_list *bl,
                      unsigned int tick)
{
    struct skill_unit_group *sg;

    nullpo_ret(src);
    nullpo_ret(bl);
    nullpo_ret(sg = src->group);

    if (bl->prev == NULL || !src->alive)
        return 0;

    if (bl->type != BL_PC && bl->type != BL_MOB)
        return 0;

    switch (sg->unit_id)
    {
        case 0x7e:             /* セイフティウォール */
        case 0x85:             /* ニューマ */
        case 0x8e:             /* クァグマイア */
        {
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            StatusChange type =
                (sg->unit_id == 0x85) ? SC_PNEUMA : SC_SAFETYWALL;
            if (sc_data
                && sc_data[type].timer != -1
                && ((struct skill_unit *) sc_data[type].val2) == src)
            {
                skill_status_change_end(bl, type, -1);
            }
        } break;

        case 0x91:             /* アンクルスネア */
        {
            struct block_list *target = map_id2bl(sg->val2);
            if (target && target == bl)
            {
                skill_status_change_end(bl, SC_ANKLE, -1);
                sg->limit = DIFF_TICK(tick, sg->tick) + 1000;
            }
        }
            break;
        case 0xb5:
        case 0xb8:
        {
            sg->limit = DIFF_TICK(tick, sg->tick) + 1000;
        }
            break;
        case 0xb6:
        {
            sg->limit = DIFF_TICK(tick, sg->tick) + 1000;
        }
            break;
        case 0x9a:             /* ボルケーノ */
        case 0x9b:             /* デリュージ */
        case 0x9c:             /* バイオレントゲイル */
        {
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            struct skill_unit *su;
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (sc_data && sc_data[type].timer != -1
                && (su = ((struct skill_unit *) sc_data[type].val2))
                && su == src)
            {
                skill_status_change_end(bl, type, -1);
            }
        }
            break;

        case 0x9e:             /* 子守唄 */
        case 0x9f:             /* ニヨルドの宴 */
        case 0xa0:             /* 永遠の混沌 */
        case 0xa1:             /* 戦太鼓の響き */
        case 0xa2:             /* ニーベルングの指輪 */
        case 0xa3:             /* ロキの叫び */
        case 0xa4:             /* 深淵の中に */
        case 0xa5:             /* 不死身のジークフリード */
        case 0xa6:             /* 不協和音 */
        case 0xa7:             /* 口笛 */
        case 0xa8:             /* 夕陽のアサシンクロス */
        case 0xa9:             /* ブラギの詩 */
        case 0xaa:             /* イドゥンの林檎 */
        case 0xab:             /* 自分勝手なダンス */
        case 0xac:             /* ハミング */
        case 0xad:             /* 私を忘れないで… */
        case 0xae:             /* 幸運のキス */
        case 0xaf:             /* サービスフォーユー */
        case 0xb4:
        {
            eptr<struct status_change, StatusChange> sc_data = battle_get_sc_data(bl);
            struct skill_unit *su;
            StatusChange type = SkillStatusChangeTable[sg->skill_id];
            if (sc_data && sc_data[type].timer != -1
                && (su = ((struct skill_unit *) sc_data[type].val4))
                && su == src)
            {
                skill_status_change_end(bl, type, -1);
            }
        }
            break;
        case 0xb7:             /* スパイダーウェッブ */
        {
            sg->limit = DIFF_TICK(tick, sg->tick) + 1000;
        }
            break;

/*      default:
                if (battle_config.error_log)
                        PRINTF("skill_unit_onout: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
                break;*/
    }
    skill_unitgrouptickset_delete(bl, sg->group_id);
    return 0;
}

/*==========================================
 * スキルユニットの削除イベント
 *------------------------------------------
 */
static
int skill_unit_ondelete(struct skill_unit *src, struct block_list *bl,
                         unsigned int tick)
{
    struct skill_unit_group *sg;

    nullpo_ret(src);
    nullpo_ret(bl);
    nullpo_ret(sg = src->group);

    if (bl->prev == NULL || !src->alive)
        return 0;

    if (bl->type != BL_PC && bl->type != BL_MOB)
        return 0;

    switch (sg->unit_id)
    {
        case 0x85:             /* ニューマ */
        case 0x7e:             /* セイフティウォール */
        case 0x8e:             /* クァグマイヤ */
        case 0x9a:             /* ボルケーノ */
        case 0x9b:             /* デリュージ */
        case 0x9c:             /* バイオレントゲイル */
        case 0x9e:             /* 子守唄 */
        case 0x9f:             /* ニヨルドの宴 */
        case 0xa0:             /* 永遠の混沌 */
        case 0xa1:             /* 戦太鼓の響き */
        case 0xa2:             /* ニーベルングの指輪 */
        case 0xa3:             /* ロキの叫び */
        case 0xa4:             /* 深淵の中に */
        case 0xa5:             /* 不死身のジークフリード */
        case 0xa6:             /* 不協和音 */
        case 0xa7:             /* 口笛 */
        case 0xa8:             /* 夕陽のアサシンクロス */
        case 0xa9:             /* ブラギの詩 */
        case 0xaa:             /* イドゥンの林檎 */
        case 0xab:             /* 自分勝手なダンス */
        case 0xac:             /* ハミング */
        case 0xad:             /* 私を忘れないで… */
        case 0xae:             /* 幸運のキス */
        case 0xaf:             /* サービスフォーユー */
        case 0xb4:
            return skill_unit_onout(src, bl, tick);

/*      default:
                if (battle_config.error_log)
                        PRINTF("skill_unit_ondelete: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
                break;*/
    }
    skill_unitgrouptickset_delete(bl, sg->group_id);
    return 0;
}

/*==========================================
 * スキルユニットの限界イベント
 *------------------------------------------
 */
static
int skill_unit_onlimit(struct skill_unit *src, unsigned int)
{
    struct skill_unit_group *sg;

    nullpo_ret(src);
    nullpo_ret(sg = src->group);

    switch (sg->unit_id)
    {
        case 0x81:             /* ワープポータル(発動前) */
        {
            struct skill_unit_group *group =
                skill_unitsetting(map_id2bl(sg->src_id), sg->skill_id,
                                   sg->skill_lv,
                                   src->bl.x, src->bl.y, 1);
            if (group == NULL)
                return 0;
            CREATE(group->valstr, char, 24);
            memcpy(group->valstr, sg->valstr, 24);
            group->val2 = sg->val2;
        }
            break;

        case 0x8d:             /* アイスウォール */
            map_setcell(src->bl.m, src->bl.x, src->bl.y, src->val2);
            break;
        case 0xb2:             /* あなたに会いたい */
        {
            struct map_session_data *sd = NULL;
            struct map_session_data *p_sd = NULL;
            if ((sd =
                 (struct map_session_data *)(map_id2bl(sg->src_id))) ==
                NULL)
                return 0;
            if ((p_sd = pc_get_partner(sd)) == NULL)
                return 0;

            pc_setpos(p_sd, map[src->bl.m].name, src->bl.x, src->bl.y, 3);
        }
            break;
    }
    return 0;
}

/*==========================================
 * スキルユニットのダメージイベント
 *------------------------------------------
 */
int skill_unit_ondamaged(struct skill_unit *src, struct block_list *bl,
                          int damage, unsigned int)
{
    struct skill_unit_group *sg;

    nullpo_ret(src);
    nullpo_ret(sg = src->group);

    switch (sg->unit_id)
    {
        case 0x8d:             /* アイスウォール */
            src->val1 -= damage;
            break;
        case 0x8f:             /* ブラストマイン */
        case 0x98:             /* クレイモアートラップ */
            skill_blown(bl, &src->bl, 2);  //吹き飛ばしてみる
            break;
        default:
            damage = 0;
            break;
    }
    return damage;
}

/*---------------------------------------------------------------------------- */

/*==========================================
 * スキル使用（詠唱完了、場所指定）
 *------------------------------------------
 */
static
void skill_castend_pos(timer_id tid, tick_t tick, custom_id_t id, custom_data_t)
{
    struct map_session_data *sd = map_id2sd(id) /*,*target_sd=NULL */ ;
    int maxcount;

    nullpo_retv(sd);

    if (sd->bl.prev == NULL)
        return;
    if (sd->skilltimer != tid)  /* タイマIDの確認 */
        return;
    sd->skilltimer = -1;
    if (pc_isdead(sd))
    {
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return;
    }

    if (battle_config.pc_land_skill_limit)
    {
        maxcount = skill_get_maxcount(sd->skillid);
        if (maxcount > 0)
        {
            int i, c;
            for (i = c = 0; i < MAX_SKILLUNITGROUP; i++)
            {
                if (sd->skillunit[i].alive_count > 0
                    && sd->skillunit[i].skill_id == sd->skillid)
                    c++;
            }
            if (c >= maxcount)
            {
                clif_skill_fail(sd, sd->skillid, 0, 0);
                sd->canact_tick = tick;
                sd->canmove_tick = tick;
                sd->skillitem = SkillID::NEGATIVE;
                sd->skillitemlv = -1;
                return;
            }
        }
    }

    int range = skill_get_range(sd->skillid, sd->skilllv);
    if (range < 0)
        range = battle_get_range(&sd->bl) - (range + 1);
    range += battle_config.pc_skill_add_range;
    if (battle_config.skill_out_range_consume)
    {                           // changed to allow casting when target walks out of range [Valaris]
        if (range < distance(sd->bl.x, sd->bl.y, sd->skillx, sd->skilly))
        {
            clif_skill_fail(sd, sd->skillid, 0, 0);
            sd->canact_tick = tick;
            sd->canmove_tick = tick;
            sd->skillitem = SkillID::NEGATIVE;
            sd->skillitemlv = -1;
            return;
        }
    }
    if (!skill_check_condition(sd, 1))
    {                           /* 使用条件チェック */
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return;
    }
    sd->skillitem = SkillID::NEGATIVE;
    sd->skillitemlv = -1;
    if (battle_config.skill_out_range_consume)
    {
        if (range < distance(sd->bl.x, sd->bl.y, sd->skillx, sd->skilly))
        {
            clif_skill_fail(sd, sd->skillid, 0, 0);
            sd->canact_tick = tick;
            sd->canmove_tick = tick;
            return;
        }
    }

    if (battle_config.pc_skill_log)
        PRINTF("PC %d skill castend skill=%d\n",
                sd->bl.id, sd->skillid);
    pc_stop_walking(sd, 0);
}


/*==========================================
 * スキル使用条件（偽で使用失敗）
 *------------------------------------------
 */
int skill_check_condition(struct map_session_data *sd, int type)
{
    int hp, sp, hp_rate, sp_rate, zeny, weapon, spiritball,
        lv, mhp;
    int index[10], itemid[10], amount[10];

    nullpo_ret(sd);

    if (battle_config.gm_skilluncond > 0
        && pc_isGM(sd) >= battle_config.gm_skilluncond)
    {
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return 1;
    }

    if (bool(sd->opt1))
    {
        clif_skill_fail(sd, sd->skillid, 0, 0);
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return 0;
    }
    if (pc_is90overweight(sd))
    {
        clif_skill_fail(sd, sd->skillid, 9, 0);
        sd->skillitem = SkillID::NEGATIVE;
        sd->skillitemlv = -1;
        return 0;
    }

    if (sd->skillitem == sd->skillid)
    {                           /* アイテムの場合無条件成功 */
        if (type & 1)
        {
            sd->skillitem = SkillID::NEGATIVE;
            sd->skillitemlv = -1;
        }
        return 1;
    }
    if (bool(sd->opt1))
    {
        clif_skill_fail(sd, sd->skillid, 0, 0);
        return 0;
    }

    SkillID skill = sd->skillid;
    lv = sd->skilllv;
    hp = skill_get_hp(skill, lv);  /* 消費HP */
    sp = skill_get_sp(skill, lv);  /* 消費SP */
    hp_rate = (lv <= 0) ? 0 : skill_db[skill].hp_rate[lv - 1];
    sp_rate = (lv <= 0) ? 0 : skill_db[skill].sp_rate[lv - 1];
    zeny = skill_get_zeny(skill, lv);
    weapon = skill_db[skill].weapon;
    spiritball = (lv <= 0) ? 0 : skill_db[skill].spiritball[lv - 1];
    mhp = skill_get_mhp(skill, lv);    /* 消費HP */
    for (int i = 0; i < 10; i++)
    {
        itemid[i] = skill_db[skill].itemid[i];
        amount[i] = skill_db[skill].amount[i];
    }
    if (mhp > 0)
        hp += (sd->status.max_hp * mhp) / 100;
    if (hp_rate > 0)
        hp += (sd->status.hp * hp_rate) / 100;
    else
        hp += (sd->status.max_hp * abs(hp_rate)) / 100;
    if (sp_rate > 0)
        sp += (sd->status.sp * sp_rate) / 100;
    else
        sp += (sd->status.max_sp * abs(sp_rate)) / 100;
    if (sd->dsprate != 100)
        sp = sp * sd->dsprate / 100;    /* 消費SP修正 */

    if (!(type & 2))
    {
        if (hp > 0 && sd->status.hp < hp)
        {                       /* HPチェック */
            clif_skill_fail(sd, skill, 2, 0);  /* HP不足：失敗通知 */
            return 0;
        }
        if (sp > 0 && sd->status.sp < sp)
        {                       /* SPチェック */
            clif_skill_fail(sd, skill, 1, 0);  /* SP不足：失敗通知 */
            return 0;
        }
        if (zeny > 0 && sd->status.zeny < zeny)
        {
            clif_skill_fail(sd, skill, 5, 0);
            return 0;
        }
        if (!(weapon & (1 << sd->status.weapon)))
        {
            clif_skill_fail(sd, skill, 6, 0);
            return 0;
        }
        if (spiritball > 0 && sd->spiritball < spiritball)
        {
            clif_skill_fail(sd, skill, 0, 0);  // 氣球不足
            return 0;
        }
    }

    for (int i = 0; i < 10; i++)
    {
        index[i] = -1;
        if (itemid[i] <= 0)
            continue;
        if (itemid[i] >= 715 && itemid[i] <= 717
            && sd->special_state.no_gemstone)
            continue;

        index[i] = pc_search_inventory(sd, itemid[i]);
        if (index[i] < 0 || sd->status.inventory[index[i]].amount < amount[i])
        {
            if (itemid[i] == 716 || itemid[i] == 717)
                clif_skill_fail(sd, skill, (7 + (itemid[i] - 716)), 0);
            else
                clif_skill_fail(sd, skill, 0, 0);
            return 0;
        }
    }

    if (!(type & 1))
        return 1;

    {
        for (int i = 0; i < 10; i++)
        {
            if (index[i] >= 0)
                pc_delitem(sd, index[i], amount[i], 0);    // アイテム消費
        }
    }

    if (type & 2)
        return 1;

    pc_heal(sd, -sp, -hp);     // [Fate] This might suppress some dupe messages

    if (zeny > 0)               // Zeny消費
        pc_payzeny(sd, zeny);
    if (spiritball > 0)         // 氣球消費
        pc_delspiritball(sd, spiritball, 0);

    return 1;
}

/*==========================================
 * 詠唱時間計算
 *------------------------------------------
 */
int skill_castfix(struct block_list *bl, int time)
{
    struct map_session_data *sd;
    struct mob_data *md;        // [Valaris]
    eptr<struct status_change, StatusChange> sc_data;
    int dex;
    int castrate = 100;
    SkillID skill;
    int lv, castnodex;

    nullpo_ret(bl);

    if (bl->type == BL_MOB)
    {                           // Crash fix [Valaris]
        md = (struct mob_data *) bl;
        skill = md->skillid;
        lv = md->skilllv;
    }

    else
    {
        sd = (struct map_session_data *) bl;
        skill = sd->skillid;
        lv = sd->skilllv;
    }

    sc_data = battle_get_sc_data(bl);
    dex = battle_get_dex(bl);

    if (skill > MAX_SKILL_DB /*|| skill < SkillID()*/)
        return 0;

    castnodex = skill_get_castnodex(skill, lv);

    if (time == 0)
        return 0;
    if (castnodex > 0 && bl->type == BL_PC)
        castrate = ((struct map_session_data *) bl)->castrate;
    else if (castnodex <= 0 && bl->type == BL_PC)
    {
        castrate = ((struct map_session_data *) bl)->castrate;
        time =
            time * castrate * (battle_config.castrate_dex_scale -
                               dex) / (battle_config.castrate_dex_scale *
                                       100);
        time = time * battle_config.cast_rate / 100;
    }

    return (time > 0) ? time : 0;
}

/*==========================================
 * ディレイ計算
 *------------------------------------------
 */
int skill_delayfix(struct block_list *bl, int time)
{
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);

    sc_data = battle_get_sc_data(bl);
    if (time <= 0)
        return 0;

    if (bl->type == BL_PC)
    {
        if (battle_config.delay_dependon_dex)   /* dexの影響を計算する */
            time =
                time * (battle_config.castrate_dex_scale -
                        battle_get_dex(bl)) /
                battle_config.castrate_dex_scale;
        time = time * battle_config.delay_rate / 100;
    }

    return (time > 0) ? time : 0;
}

/*==========================================
 * スキル使用（ID指定）
 *------------------------------------------
 */
int skill_use_id(struct map_session_data *sd, int target_id,
        SkillID skill_num, int skill_lv)
{
    unsigned int tick;
    int casttime = 0, delay = 0, range_;
    int forcecast = 0;
    struct block_list *bl;
    eptr<struct status_change, StatusChange> sc_data;
    tick = gettick();

    nullpo_ret(sd);

    if ((bl = map_id2bl(target_id)) == NULL)
    {
/*              if (battle_config.error_log)
                        PRINTF("skill target not found %d\n",target_id); */
        return 0;
    }
    if (sd->bl.m != bl->m || pc_isdead(sd))
        return 0;

    sc_data = sd->sc_data;

    /* 沈黙や異常（ただし、グリムなどの判定をする） */
    if (bool(sd->opt1))
        return 0;

    if (bool(sd->status.option & Option::HIDE2))
        return 0;

    if (skill_get_inf2(skill_num) & 0x200 && sd->bl.id == target_id)
        return 0;

    sd->skillid = skill_num;
    sd->skilllv = skill_lv;

    if (!skill_check_condition(sd, 0))
        return 0;

    /* 射程と障害物チェック */
    range_ = skill_get_range(skill_num, skill_lv);
    if (range_ < 0)
        range_ = battle_get_range(&sd->bl) - (range_ + 1);
    if (!battle_check_range(&sd->bl, bl, range_))
        return 0;

    pc_stopattack(sd);

    casttime = skill_castfix(&sd->bl, skill_get_cast(skill_num, skill_lv));
    delay = skill_delayfix(&sd->bl, skill_get_delay(skill_num, skill_lv));
    sd->state.skillcastcancel = skill_db[skill_num].castcancel;

    if (battle_config.pc_skill_log)
        PRINTF("PC %d skill use target_id=%d skill=%d lv=%d cast=%d\n",
                sd->bl.id, target_id, skill_num, skill_lv, casttime);

    if (casttime > 0 || forcecast)
    {                           /* 詠唱が必要 */
        struct mob_data *md;

        /* 詠唱反応モンスター */
        if (bl->type == BL_MOB && (md = (struct mob_data *) bl)
            && bool(mob_db[md->mob_class].mode & MobMode::CAST_SENSOR)
            && md->state.state != MS_ATTACK
            && sd->invincible_timer == -1)
        {
            md->target_id = sd->bl.id;
            md->state.attackable = true;
            md->min_chase = 13;
        }
    }

    if (casttime <= 0)          /* 詠唱の無いものはキャンセルされない */
        sd->state.skillcastcancel = 0;

    sd->skilltarget = target_id;
    sd->skillx = 0;
    sd->skilly = 0;
    sd->canact_tick = tick + casttime + delay;
    sd->canmove_tick = tick;

    if (casttime > 0)
    {
        sd->skilltimer = add_timer(tick + casttime, skill_castend_id, sd->bl.id, 0);
        pc_stop_walking(sd, 0);
    }
    else
    {
        sd->skilltimer = -1;
        skill_castend_id(sd->skilltimer, tick, sd->bl.id, 0);
    }

    return 0;
}

/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
int skill_use_pos(struct map_session_data *sd,
        int skill_x, int skill_y,
        SkillID skill_num, int skill_lv)
{
    struct block_list bl;
    eptr<struct status_change, StatusChange> sc_data;
    unsigned int tick;
    int casttime = 0, delay = 0, range;

    nullpo_ret(sd);

    if (pc_isdead(sd))
        return 0;

    sc_data = sd->sc_data;

    if (bool(sd->opt1))
        return 0;

    if (bool(sd->status.option & Option::HIDE2))
        return 0;

    sd->skillid = skill_num;
    sd->skilllv = skill_lv;
    sd->skillx = skill_x;
    sd->skilly = skill_y;
    if (!skill_check_condition(sd, 0))
        return 0;

    /* 射程と障害物チェック */
    bl.type = BL_NUL;
    bl.m = sd->bl.m;
    bl.x = skill_x;
    bl.y = skill_y;
    range = skill_get_range(skill_num, skill_lv);
    if (range < 0)
        range = battle_get_range(&sd->bl) - (range + 1);
    if (!battle_check_range(&sd->bl, &bl, range))
        return 0;

    pc_stopattack(sd);

    casttime = skill_castfix(&sd->bl, skill_get_cast(skill_num, skill_lv));
    delay = skill_delayfix(&sd->bl, skill_get_delay(skill_num, skill_lv));
    sd->state.skillcastcancel = skill_db[skill_num].castcancel;

    if (battle_config.pc_skill_log)
        PRINTF("PC %d skill use target_pos= (%d,%d) skill=%d lv=%d cast=%d\n",
                sd->bl.id, skill_x, skill_y,
                skill_num, skill_lv, casttime);

    if (casttime <= 0)          /* 詠唱の無いものはキャンセルされない */
        sd->state.skillcastcancel = 0;

    sd->skilltarget = 0;
    tick = gettick();
    sd->canact_tick = tick + casttime + delay;
    sd->canmove_tick = tick;

    if (casttime > 0)
    {
        sd->skilltimer = add_timer(tick + casttime, skill_castend_pos, sd->bl.id, 0);
        pc_stop_walking(sd, 0);
    }
    else
    {
        sd->skilltimer = -1;
        skill_castend_pos(sd->skilltimer, tick, sd->bl.id, 0);
    }

    return 0;
}

/*==========================================
 * スキル詠唱キャンセル
 *------------------------------------------
 */
int skill_castcancel(struct block_list *bl, int type)
{
    int inf;

    nullpo_ret(bl);

    if (bl->type == BL_PC)
    {
        struct map_session_data *sd = (struct map_session_data *) bl;
        unsigned long tick = gettick();
        nullpo_ret(sd);
        sd->canact_tick = tick;
        sd->canmove_tick = tick;
        if (sd->skilltimer != -1)
        {
            if (!type)
            {
                if ((inf = skill_get_inf(sd->skillid)) == 2 || inf == 32)
                    delete_timer(sd->skilltimer, skill_castend_pos);
                else
                    delete_timer(sd->skilltimer, skill_castend_id);
            }
            else
            {
                if ((inf = skill_get_inf(sd->skillid_old)) == 2 || inf == 32)
                    delete_timer(sd->skilltimer, skill_castend_pos);
                else
                    delete_timer(sd->skilltimer, skill_castend_id);
            }
            sd->skilltimer = -1;
            clif_skillcastcancel(bl);
        }

        return 0;
    }
    else if (bl->type == BL_MOB)
    {
        struct mob_data *md = (struct mob_data *) bl;
        nullpo_ret(md);
        if (md->skilltimer != -1)
        {
            if ((inf = skill_get_inf(md->skillid)) == 2 || inf == 32)
                delete_timer(md->skilltimer, mobskill_castend_pos);
            else
                delete_timer(md->skilltimer, mobskill_castend_id);
            md->skilltimer = -1;
            clif_skillcastcancel(bl);
        }
        return 0;
    }
    return 1;
}

/*==========================================
 * ディボーション 有効確認
 *------------------------------------------
 */
void skill_devotion(struct map_session_data *md, int)
{
    // 総確認
    int n;

    nullpo_retv(md);

    for (n = 0; n < 5; n++)
    {
        if (md->dev.val1[n])
        {
            struct map_session_data *sd = map_id2sd(md->dev.val1[n]);
            // 相手が見つからない // 相手をディボしてるのが自分じゃない // 距離が離れてる
            if (sd == NULL
                || (md->bl.id != 0/* was something else - TODO remove this */)
                || skill_devotion3(&md->bl, md->dev.val1[n]))
            {
                skill_devotion_end(md, sd, n);
            }
        }
    }
}

void skill_devotion2(struct block_list *bl, int crusader)
{
    // 被ディボーションが歩いた時の距離チェック
    struct map_session_data *sd = map_id2sd(crusader);

    nullpo_retv(bl);

    if (sd)
        skill_devotion3(&sd->bl, bl->id);
}

int skill_devotion3(struct block_list *bl, int target)
{
    // クルセが歩いた時の距離チェック
    struct map_session_data *md;
    struct map_session_data *sd;
    int n, r = 0;

    nullpo_retr(1, bl);

    if ((md = (struct map_session_data *) bl) == NULL
        || (sd = map_id2sd(target)) == NULL)
        return 1;
    else
        r = distance(bl->x, bl->y, sd->bl.x, sd->bl.y);

    if ( + 6 < r)
    {                           // 許容範囲を超えてた
        for (n = 0; n < 5; n++)
            if (md->dev.val1[n] == target)
                md->dev.val2[n] = 0;    // 離れた時は、糸を切るだけ
        return 1;
    }
    return 0;
}

void skill_devotion_end(struct map_session_data *md,
                         struct map_session_data *sd, int target)
{
    // クルセと被ディボキャラのリセット
    nullpo_retv(md);
    nullpo_retv(sd);

    md->dev.val1[target] = md->dev.val2[target] = 0;
}

int skill_gangsterparadise(struct map_session_data *, int)
{
    return 0;
}

/*==========================================
 * ランドプロテクターチェック(foreachinarea)
 *------------------------------------------
 */
void skill_landprotector(struct block_list *, SkillID, int *)
{
}

/*==========================================
 * イドゥンの林檎の回復処理(foreachinarea)
 *------------------------------------------
 */
static
void skill_idun_heal(struct block_list *bl, struct skill_unit *unit)
{
    struct skill_unit_group *sg;
    int heal;

    nullpo_retv(bl);
    nullpo_retv(unit);
    nullpo_retv(sg = unit->group);

    heal =
        30 + sg->skill_lv * 5 + ((sg->val1) >> 16) * 5 +
        ((sg->val1) & 0xfff) / 2;

    if (bl->type == BL_SKILL || bl->id == sg->src_id)
        return;

    if (bl->type == BL_PC || bl->type == BL_MOB)
    {
        battle_heal(NULL, bl, heal, 0, 0);
    }
}

/*==========================================
 * 指定範囲内でsrcに対して有効なターゲットのblの数を数える(foreachinarea)
 *------------------------------------------
 */
void skill_count_target(struct block_list *bl,
        struct block_list *src, int *c)
{
    nullpo_retv(bl);

    if (src == NULL)
        return;
    if (c == NULL)
        return;
    if (battle_check_target(src, bl, BCT_ENEMY) > 0)
        (*c)++;
}

/*==========================================
 * トラップ範囲処理(foreachinarea)
 *------------------------------------------
 */
void skill_trap_splash(struct block_list *bl,
        struct block_list *src, int tick, int splash_count)
{
    struct skill_unit *unit;
    struct skill_unit_group *sg;
    struct block_list *ss;
    int i;

    nullpo_retv(bl);
    nullpo_retv(src);
    unit = (struct skill_unit *) src;
    nullpo_retv(sg = unit->group);
    nullpo_retv(ss = map_id2bl(sg->src_id));

    if (battle_check_target(src, bl, BCT_ENEMY) > 0)
    {
        switch (sg->unit_id)
        {
            case 0x95:         /* サンドマン */
            case 0x96:         /* フラッシャー */
            case 0x94:         /* ショックウェーブトラップ */
                skill_additional_effect(ss, bl, sg->skill_id, sg->skill_lv,
                                         BF_MISC, tick);
                break;
            case 0x8f:         /* ブラストマイン */
            case 0x98:         /* クレイモアートラップ */
                for (i = 0; i < splash_count; i++)
                {
                    skill_attack(BF_MISC, ss, src, bl, sg->skill_id,
                                  sg->skill_lv, tick,
                                  (sg->val2) ? BCT_mid_x05 : BCT_ZERO);
                }
                // TODO: determine if this was supposed to break
                FALLTHROUGH;
            case 0x97:         /* フリージングトラップ */
                skill_attack(BF_WEAPON, ss, src, bl, sg->skill_id,
                              sg->skill_lv, tick, (sg->val2) ? BCT_mid_x05 : BCT_ZERO);
                break;
            default:
                break;
        }
    }
}

/*----------------------------------------------------------------------------
 * ステータス異常
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ステータス異常タイマー範囲処理
 *------------------------------------------
 */
void skill_status_change_timer_sub(struct block_list *bl,
        struct block_list *src, StatusChange type, unsigned int)
{
    nullpo_retv(bl);
    nullpo_retv(src);

    if (bl->type != BL_PC && bl->type != BL_MOB)
        return;

    switch (type)
    {
        case SC_SIGHT:         /* サイト */
            if (bool((*battle_get_option(bl)) & (Option::HIDE2 | Option::CLOAK)))
            {
                skill_status_change_end(bl, SC_HIDING, -1);
            }
            break;
    }
}

/*==========================================
 * ステータス異常終了
 *------------------------------------------
 */
int skill_status_change_active(struct block_list *bl, StatusChange type)
{
    eptr<struct status_change, StatusChange> sc_data;

    nullpo_ret(bl);
    if (bl->type != BL_PC && bl->type != BL_MOB)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_active: neither MOB nor PC !\n");
        return 0;
    }

    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;

    return sc_data[type].timer != -1;
}

int skill_status_change_end(struct block_list *bl, StatusChange type, int tid)
{
    eptr<struct status_change, StatusChange> sc_data;
    int opt_flag = 0, calc_flag = 0;
    short *sc_count;
    Option *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;

    nullpo_ret(bl);
    if (bl->type != BL_PC && bl->type != BL_MOB)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_end: neither MOB nor PC !\n");
        return 0;
    }
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    nullpo_ret(sc_count = battle_get_sc_count(bl));
    nullpo_ret(option = battle_get_option(bl));
    nullpo_ret(opt1 = battle_get_opt1(bl));
    nullpo_ret(opt2 = battle_get_opt2(bl));
    nullpo_ret(opt3 = battle_get_opt3(bl));

    if ((*sc_count) > 0 && sc_data[type].timer != -1
        && (sc_data[type].timer == tid || tid == -1))
    {

        if (tid == -1)          // タイマから呼ばれていないならタイマ削除をする
            delete_timer(sc_data[type].timer, skill_status_change_timer);

        /* 該当の異常を正常に戻す */
        sc_data[type].timer = -1;
        (*sc_count)--;

        switch (type)
        {                       /* 異常の種類ごとの処理 */
            case SC_HIDING:
            case SC_SPEEDPOTION0:  /* 増速ポーション */
            case SC_ATKPOT:    /* attack potion [Valaris] */
            case SC_MATKPOT:   /* magic attack potion [Valaris] */
            case SC_PHYS_SHIELD:
            case SC_HASTE:
                calc_flag = 1;
                break;
            {
                struct map_session_data *md = map_id2sd(sc_data[type].val1);
                sc_data[type].val1 = sc_data[type].val2 = 0;
                skill_devotion(md, bl->id);
                calc_flag = 1;
            }
                break;
            case SC_NOCHAT:    //チャット禁止状態
                break;
            case SC_SELFDESTRUCTION:   /* 自爆 */
            {
                //自分のダメージは0にして
                struct mob_data *md = NULL;
                if (bl->type == BL_MOB && (md = (struct mob_data *) bl))
                    skill_castend_damage_id(bl, bl,
                           SkillID( sc_data[type].val2), sc_data[type].val1,
                           gettick(), BCT_ZERO);
            }
                break;
                /* option1 */
            case SC_FREEZE:
                sc_data[type].val3 = 0;
                break;

                /* option2 */
            case SC_POISON:    /* 毒 */
            case SC_BLIND:     /* 暗黒 */
            case SC_CURSE:
                calc_flag = 1;
                break;
        }

        if (bl->type == BL_PC && type < SC_SENDMAX)
            clif_status_change(bl, type, 0);   /* アイコン消去 */

        switch (type)
        {                       /* 正常に戻るときなにか処理が必要 */
            case SC_STONE:
            case SC_FREEZE:
            case SC_STAN:
            case SC_SLEEP:
                *opt1 = Opt1::ZERO;
                opt_flag = 1;
                break;

            case SC_POISON:
                *opt2 &= ~Opt2::_poison;
                opt_flag = 1;
                break;

            case SC_CURSE:
                *opt2 &= ~Opt2::_curse;
                opt_flag = 1;
                break;

            case SC_SILENCE:
                *opt2 &= ~Opt2::_silence;
                opt_flag = 1;
                break;

            case SC_BLIND:
                *opt2 &= ~Opt2::BLIND;
                opt_flag = 1;
                break;

            case SC_SLOWPOISON:
                if (sc_data[SC_POISON].timer != -1)
                    *opt2 |= Opt2::_poison;
                *opt2 &= ~Opt2::_slowpoison;
                opt_flag = 1;
                break;

            case SC_SPEEDPOTION0:
                *opt2 &= ~Opt2::_speedpotion0;
                opt_flag = 1;
                break;

            case SC_ATKPOT:
                *opt2 &= ~Opt2::_atkpot;
                opt_flag = 1;
                break;

            case SC_HIDING:
                *option &= ~Option::HIDE2;
                opt_flag = 1;
                break;

            case SC_SIGHT:
                *option &= ~Option::SIGHT;
                opt_flag = 1;
                break;
        }

        if (night_flag == 1
            && !bool(*opt2 & Opt2::BLIND)
            && bl->type == BL_PC)
        {                       // by [Yor]
            *opt2 |= Opt2::BLIND;
            opt_flag = 1;
        }

        if (opt_flag)           /* optionの変更を伝える */
            clif_changeoption(bl);

        if (bl->type == BL_PC && calc_flag)
            pc_calcstatus((struct map_session_data *) bl, 0);  /* ステータス再計算 */
    }

    return 0;
}

int skill_update_heal_animation(struct map_session_data *sd)
{
    const Opt2 mask = Opt2::_heal;

    nullpo_ret(sd);
    bool was_active = bool(sd->opt2 & mask);
    bool is_active = sd->quick_regeneration_hp.amount > 0;

    if (was_active == is_active)
        return 0;               // no update

    if (is_active)
        sd->opt2 |= mask;
    else
        sd->opt2 &= ~mask;

    return clif_changeoption(&sd->bl);
}

/*==========================================
 * ステータス異常終了タイマー
 *------------------------------------------
 */
void skill_status_change_timer(timer_id tid, tick_t tick, custom_id_t id, custom_data_t data)
{
    StatusChange type = StatusChange(data);
    struct block_list *bl;
    struct map_session_data *sd = NULL;
    eptr<struct status_change, StatusChange> sc_data;
    //short *sc_count; //使ってない？

    if ((bl = map_id2bl(id)) == NULL)
        return;
    //該当IDがすでに消滅しているというのはいかにもありそうなのでスルーしてみる
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return;

    if (bl->type == BL_PC)
        sd = (struct map_session_data *) bl;

    //sc_count=battle_get_sc_count(bl); //使ってない？

    if (sc_data[type].timer != tid)
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_timer %d != %d\n", tid,
                    sc_data[type].timer);
    }

    if (sc_data[type].spell_invocation)
    {                           // Must report termination
        spell_effect_report_termination(sc_data[type].spell_invocation,
                                         bl->id, type, 0);
        sc_data[type].spell_invocation = 0;
    }

    switch (type)
    {                           /* 特殊な処理になる場合 */
        case SC_HIDING:        /* ハイディング */
            if (sd)
            {                   /* SPがあって、時間制限の間は持続 */
                if (sd->status.sp > 0 && (--sc_data[type].val2) > 0)
                {
                    if (sc_data[type].val2 % (sc_data[type].val1 + 3) == 0)
                    {
                        sd->status.sp--;
                        clif_updatestatus(sd, SP_SP);
                    }
                    sc_data[type].timer = add_timer(   /* タイマー再設定 */
                                                        1000 + tick,
                                                        skill_status_change_timer,
                                                        bl->id, data);
                    return;
                }
            }
            break;

        case SC_SIGHT:         /* サイト */
        {
            const int range = 7;
            map_foreachinarea(std::bind(skill_status_change_timer_sub, ph::_1, bl, type, tick),
                    bl->m, bl->x - range, bl->y - range,
                    bl->x + range, bl->y + range, BL_NUL);

            if ((--sc_data[type].val2) > 0)
            {
                sc_data[type].timer = add_timer(   /* タイマー再設定 */
                                                    250 + tick,
                                                    skill_status_change_timer,
                                                    bl->id, data);
                return;
            }
        }
            break;

        case SC_STONE:
            if (sc_data[type].val2 != 0)
            {
                Opt1 *opt1 = battle_get_opt1(bl);
                sc_data[type].val2 = 0;
                sc_data[type].val4 = 0;
                battle_stopwalking(bl, 1);
                if (opt1)
                {
                    *opt1 = Opt1::_stone1;
                    clif_changeoption(bl);
                }
                sc_data[type].timer =
                    add_timer(1000 + tick, skill_status_change_timer, bl->id,
                               data);
                return;
            }
            else if ((--sc_data[type].val3) > 0)
            {
                int hp = battle_get_max_hp(bl);
                if ((++sc_data[type].val4) % 5 == 0
                    && battle_get_hp(bl) > hp >> 2)
                {
                    hp = hp / 100;
                    if (hp < 1)
                        hp = 1;
                    if (bl->type == BL_PC)
                        pc_heal((struct map_session_data *) bl, -hp, 0);
                    else if (bl->type == BL_MOB)
                    {
                        struct mob_data *md;
                        if ((md = ((struct mob_data *) bl)) == NULL)
                            break;
                        md->hp -= hp;
                    }
                }
                sc_data[type].timer =
                    add_timer(1000 + tick, skill_status_change_timer, bl->id,
                               data);
                return;
            }
            break;
        case SC_POISON:
            if (sc_data[SC_SLOWPOISON].timer == -1)
            {
                const int resist_poison =
                    skill_power_bl(bl, TMW_RESIST_POISON) >> 3;
                if (resist_poison)
                    sc_data[type].val1 -= MRAND(resist_poison + 1);

                if ((--sc_data[type].val1) > 0)
                {

                    int hp = battle_get_max_hp(bl);
                    if (battle_get_hp(bl) > hp >> 4)
                    {
                        if (bl->type == BL_PC)
                        {
                            hp = 3 + hp * 3 / 200;
                            pc_heal((struct map_session_data *) bl, -hp, 0);
                        }
                        else if (bl->type == BL_MOB)
                        {
                            struct mob_data *md;
                            if ((md = ((struct mob_data *) bl)) == NULL)
                                break;
                            hp = 3 + hp / 200;
                            md->hp -= hp;
                        }
                    }
                    sc_data[type].timer =
                        add_timer(1000 + tick, skill_status_change_timer,
                                   bl->id, data);
                }
            }
            else
                sc_data[type].timer =
                    add_timer(2000 + tick, skill_status_change_timer, bl->id,
                               data);
            break;

            /* 時間切れ無し？？ */
        case SC_WEIGHT50:
        case SC_WEIGHT90:
        case SC_BROKNWEAPON:
        case SC_BROKNARMOR:
            if (sc_data[type].timer == tid)
                sc_data[type].timer =
                    add_timer(1000 * 600 + tick, skill_status_change_timer,
                               bl->id, data);
            return;

        case SC_NOCHAT:        //チャット禁止状態
            if (sd && battle_config.muting_players)
            {
                time_t timer;
                if ((++sd->status.manner)
                    && time(&timer) <
                    ((sc_data[type].val2) + 60 * (0 - sd->status.manner)))
                {               //開始からstatus.manner分経ってないので継続
                    sc_data[type].timer = add_timer(   /* タイマー再設定(60秒) */
                                                        60000 + tick,
                                                        skill_status_change_timer,
                                                        bl->id, data);
                    return;
                }
            }
            break;
        case SC_SELFDESTRUCTION:   /* 自爆 */
            if (--sc_data[type].val3 > 0)
            {
                struct mob_data *md;
                if (bl->type == BL_MOB && (md = (struct mob_data *) bl)
                    && md->stats[MOB_SPEED] > 250)
                {
                    md->stats[MOB_SPEED] -= 250;
                    md->next_walktime = tick;
                }
                sc_data[type].timer = add_timer(   /* タイマー再設定 */
                                                    1000 + tick,
                                                    skill_status_change_timer,
                                                    bl->id, data);
                return;
            }
            break;

        case SC_FLYING_BACKPACK:
            clif_updatestatus(sd, SP_WEIGHT);
            break;

    }

    skill_status_change_end(bl, type, tid);
}

/*==========================================
 * ステータス異常開始
 *------------------------------------------
 */
int skill_status_change_start(struct block_list *bl, StatusChange type,
        int val1, int val2, int val3, int val4,
        int tick, int flag)
{
    return skill_status_effect(bl, type, val1, val2, val3, val4, tick, flag,
                                0);
}

int skill_status_effect(struct block_list *bl, StatusChange type,
        int val1, int val2, int val3, int val4,
        int tick, int flag, int spell_invocation)
{
    struct map_session_data *sd = NULL;
    eptr<struct status_change, StatusChange> sc_data;
    short *sc_count;
    Option *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;
    int opt_flag = 0, calc_flag = 0;
    int race, elem, undead_flag;
    SP updateflag = SP::ZERO;
    int scdef = 0;

    nullpo_ret(bl);
    if (bl->type == BL_SKILL)
        return 0;
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    nullpo_ret(sc_count = battle_get_sc_count(bl));
    nullpo_ret(option = battle_get_option(bl));
    nullpo_ret(opt1 = battle_get_opt1(bl));
    nullpo_ret(opt2 = battle_get_opt2(bl));
    nullpo_ret(opt3 = battle_get_opt3(bl));

    race = battle_get_race(bl);
    MobMode mode = battle_get_mode(bl);
    elem = battle_get_elem_type(bl);
    undead_flag = battle_check_undead(race, elem);

    switch (type)
    {
        case SC_STONE:
        case SC_FREEZE:
            scdef = 3 + battle_get_mdef(bl) + battle_get_luk(bl) / 3;
            break;
        case SC_STAN:
        case SC_SILENCE:
        case SC_POISON:
            scdef = 3 + battle_get_vit(bl) + battle_get_luk(bl) / 3;
            break;
        case SC_SLEEP:
        case SC_BLIND:
            scdef = 3 + battle_get_int(bl) + battle_get_luk(bl) / 3;
            break;
        case SC_CURSE:
            scdef = 3 + battle_get_luk(bl);
            break;

//      case SC_CONFUSION:
        default:
            scdef = 0;
    }
    if (scdef >= 100)
        return 0;
    if (bl->type == BL_PC)
    {
        sd = (struct map_session_data *) bl;

        if (SC_STONE <= type && type <= SC_BLIND)
        {
            BadSC bsc = BadSC_from_SC(type);
            /* カードによる耐性 */
            if (sd && sd->reseff[bsc] > 0
                && MRAND(10000) < sd->reseff[bsc])
            {
                if (battle_config.battle_log)
                    PRINTF("PC %d skill_sc_start: cardによる異常耐性発動\n",
                            sd->bl.id);
                return 0;
            }
        }
    }
    else if (bl->type == BL_MOB)
    {
    }
    else
    {
        if (battle_config.error_log)
            PRINTF("skill_status_change_start: neither MOB nor PC !\n");
        return 0;
    }

    if (type == SC_FREEZE && undead_flag && !(flag & 1))
        return 0;

    if (bool(mode & MobMode::BOSS)
        && (type == SC_STONE
            || type == SC_FREEZE
            || type == SC_STAN
            || type == SC_SLEEP
            || type == SC_SILENCE
        )
        && !(flag & 1))
    {
        /* ボスには効かない(ただしカードによる効果は適用される) */
        return 0;
    }
    if (type == SC_FREEZE || type == SC_STAN || type == SC_SLEEP)
        battle_stopwalking(bl, 1);

    if (sc_data[type].timer != -1)
    {                           /* すでに同じ異常になっている場合タイマ解除 */
        if (sc_data[type].val1 > val1
            && type != SC_SPEEDPOTION0
            && type != SC_ATKPOT
            && type != SC_MATKPOT) // added atk and matk potions [Valaris]
            return 0;
        if (type >= SC_STAN && type <= SC_BLIND)
            return 0;           /* 継ぎ足しができない状態異常である時は状態異常を行わない */
        {
            (*sc_count)--;
            delete_timer(sc_data[type].timer, skill_status_change_timer);
            sc_data[type].timer = -1;
        }
    }

    switch (type)
    {                           /* 異常の種類ごとの処理 */
        case SC_SLOWPOISON:
            if (sc_data[SC_POISON].timer == -1)
                return 0;
            break;

        case SC_SPEEDPOTION0:  /* 増速ポーション */
            *opt2 |= Opt2::_speedpotion0;
            calc_flag = 1;
            tick = 1000 * tick;
//          val2 = 5*(2+type-SC_SPEEDPOTION0);
            break;

            /* atk & matk potions [Valaris] */
        case SC_ATKPOT:
            *opt2 |= Opt2::_atkpot;
            FALLTHROUGH;
        case SC_MATKPOT:
            calc_flag = 1;
            tick = 1000 * tick;
            break;

        case SC_NOCHAT:        //チャット禁止状態
        {
            time_t timer;

            if (!battle_config.muting_players)
                break;

            tick = 60000;
            if (!val2)
                val2 = time(&timer);
            // updateflag = SP_MANNER;
        }
            break;
        case SC_SELFDESTRUCTION:   //自爆
            val3 = tick / 1000;
            tick = 1000;
            break;

            /* option1 */
        case SC_STONE:         /* 石化 */
            if (!(flag & 2))
            {
                int sc_def = battle_get_mdef(bl) * 200;
                tick = tick - sc_def;
            }
            val3 = tick / 1000;
            if (val3 < 1)
                val3 = 1;
            tick = 5000;
            val2 = 1;
            break;
        case SC_SLEEP:         /* 睡眠 */
            if (!(flag & 2))
            {
//              int sc_def = 100 - (battle_get_int(bl) + battle_get_luk(bl)/3);
//              tick = tick * sc_def / 100;
//              if(tick < 1000) tick = 1000;
                tick = 30000;   //睡眠はステータス耐性に関わらず30秒
            }
            break;
        case SC_FREEZE:        /* 凍結 */
            if (!(flag & 2))
            {
                int sc_def = 100 - battle_get_mdef(bl);
                tick = tick * sc_def / 100;
            }
            break;
        case SC_STAN:          /* スタン（val2にミリ秒セット） */
            if (!(flag & 2))
            {
                int sc_def =
                    100 - (battle_get_vit(bl) + battle_get_luk(bl) / 3);
                tick = tick * sc_def / 100;
            }
            break;

            /* option2 */
        case SC_POISON:        /* 毒 */
            calc_flag = 1;
            if (!(flag & 2))
            {
                int sc_def =
                    100 - (battle_get_vit(bl) + battle_get_luk(bl) / 5);
                tick = tick * sc_def / 100;
            }
            val3 = tick / 1000;
            if (val3 < 1)
                val3 = 1;
            tick = 1000;
            break;
        case SC_SILENCE:       /* 沈黙（レックスデビーナ） */
            if (!(flag & 2))
            {
                int sc_def = 100 - battle_get_vit(bl);
                tick = tick * sc_def / 100;
            }
            break;
        case SC_BLIND:         /* 暗黒 */
            calc_flag = 1;
            if (!(flag & 2))
            {
                int sc_def =
                    battle_get_lv(bl) / 10 + battle_get_int(bl) / 15;
                tick = 30000 - sc_def;
            }
            break;
        case SC_CURSE:
            calc_flag = 1;
            if (!(flag & 2))
            {
                int sc_def = 100 - battle_get_vit(bl);
                tick = tick * sc_def / 100;
            }
            break;

            /* option */
        case SC_HIDING:        /* ハイディング */
            calc_flag = 1;
            if (bl->type == BL_PC)
            {
                val2 = tick / 1000; /* 持続時間 */
                tick = 1000;
            }
            break;

        case SC_SIGHT:         /* サイト/ルアフ */
            val2 = tick / 250;
            tick = 10;
            break;

            /* セーフティウォール、ニューマ */
        case SC_SAFETYWALL:
        case SC_PNEUMA:
            tick = ((struct skill_unit *) val2)->group->limit;
            break;

            /* アンクル */
        case SC_ANKLE:
            break;

        case SC_WEIGHT50:
        case SC_WEIGHT90:
        case SC_BROKNWEAPON:
        case SC_BROKNARMOR:
            tick = 600 * 1000;
            break;

        case SC_HASTE:
            calc_flag = 1;
            break;
        case SC_PHYS_SHIELD:
        case SC_MBARRIER:
        case SC_HALT_REGENERATE:
        case SC_HIDE:
            break;
        case SC_FLYING_BACKPACK:
            updateflag = SP_WEIGHT;
            break;
        default:
            if (battle_config.error_log)
                PRINTF("UnknownStatusChange [%d]\n", type);
            return 0;
    }

    if (bl->type == BL_PC && type < SC_SENDMAX)
        clif_status_change(bl, type, 1);   /* アイコン表示 */

    /* optionの変更 */
    switch (type)
    {
        case SC_STONE:
        case SC_FREEZE:
        case SC_STAN:
        case SC_SLEEP:
            battle_stopattack(bl); /* 攻撃停止 */
            skill_stop_dancing(bl, 0); /* 演奏/ダンスの中断 */
            /* 同時に掛からないステータス異常を解除 */
            for (StatusChange i : MAJOR_STATUS_EFFECTS_1)
            {
                if (sc_data[i].timer != -1)
                {
                    (*sc_count)--;
                    delete_timer(sc_data[i].timer,
                                  skill_status_change_timer);
                    sc_data[i].timer = -1;
                }
            }
            switch (type)
            {
            case SC_STONE:  *opt1 = Opt1::_stone6; break;
            case SC_FREEZE: *opt1 = Opt1::_freeze; break;
            case SC_STAN:   *opt1 = Opt1::_stan; break;
            case SC_SLEEP:  *opt1 = Opt1::_sleep; break;
            }
            opt_flag = 1;
            break;
        case SC_POISON:
            if (sc_data[SC_SLOWPOISON].timer == -1)
            {
                *opt2 |= Opt2::_poison;
                opt_flag = 1;
            }
            break;

        case SC_CURSE:
            *opt2 |= Opt2::_curse;
            opt_flag = 1;
            break;
        case SC_SILENCE:
            *opt2 |= Opt2::_silence;
            opt_flag = 1;
            break;
        case SC_BLIND:
            *opt2 |= Opt2::BLIND;
            opt_flag = 1;
            break;

        case SC_SLOWPOISON:
            *opt2 &= ~Opt2::_poison;
            *opt2 |= Opt2::_slowpoison;
            opt_flag = 1;
            break;
        case SC_HIDING:
            battle_stopattack(bl); /* 攻撃停止 */
            *option |= Option::HIDE2;
            opt_flag = 1;
            break;
        case SC_SIGHT:
            *option |= Option::SIGHT;
            opt_flag = 1;
            break;
    }

    if (opt_flag)               /* optionの変更 */
        clif_changeoption(bl);

    (*sc_count)++;              /* ステータス異常の数 */

    sc_data[type].val1 = val1;
    sc_data[type].val2 = val2;
    sc_data[type].val3 = val3;
    sc_data[type].val4 = val4;
    if (sc_data[type].spell_invocation) // Supplant by newer spell
        spell_effect_report_termination(sc_data[type].spell_invocation,
                                         bl->id, type, 1);

    sc_data[type].spell_invocation = spell_invocation;

    /* タイマー設定 */
    sc_data[type].timer =
        add_timer(gettick() + tick, skill_status_change_timer, bl->id,
                   custom_data_t(type));

    if (bl->type == BL_PC && calc_flag)
        pc_calcstatus(sd, 0);  /* ステータス再計算 */

    if (bl->type == BL_PC && updateflag != SP::ZERO)
        clif_updatestatus(sd, updateflag); /* ステータスをクライアントに送る */

    return 0;
}

/*==========================================
 * ステータス異常全解除
 *------------------------------------------
 */
int skill_status_change_clear(struct block_list *bl, int type)
{
    eptr<struct status_change, StatusChange> sc_data;
    short *sc_count;
    Option *option;
    Opt1 *opt1;
    Opt2 *opt2;
    Opt3 *opt3;

    nullpo_ret(bl);
    sc_data = battle_get_sc_data(bl);
    if (not sc_data)
        return 0;
    nullpo_ret(sc_count = battle_get_sc_count(bl));
    nullpo_ret(option = battle_get_option(bl));
    nullpo_ret(opt1 = battle_get_opt1(bl));
    nullpo_ret(opt2 = battle_get_opt2(bl));
    nullpo_ret(opt3 = battle_get_opt3(bl));

    if (*sc_count == 0)
        return 0;
    for (StatusChange i : erange(StatusChange(), MAX_STATUSCHANGE))
    {
        if (sc_data[i].timer != -1)
        {
            skill_status_change_end(bl, i, -1);
        }
    }
    *sc_count = 0;
    *opt1 = Opt1::ZERO;
    *opt2 = Opt2::ZERO;
    *opt3 = Opt3::ZERO;
    *option &= Option::MASK;

    if (night_flag == 1 && type == 1)   // by [Yor]
        *opt2 |= Opt2::BLIND;

    if (type == 0 || type & 2)
        clif_changeoption(bl);

    return 0;
}

/* クローキング検査（周りに移動不可能地帯があるか） */
int skill_check_cloaking(struct block_list *bl)
{
    static int dx[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    static int dy[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    int end = 1, i;

    nullpo_ret(bl);

    if (bl->type == BL_PC && battle_config.pc_cloak_check_type & 1)
        return 0;
    if (bl->type == BL_MOB && battle_config.monster_cloak_check_type & 1)
        return 0;
    for (i = 0; i < sizeof(dx) / sizeof(dx[0]); i++)
    {
        int c = map_getcell(bl->m, bl->x + dx[i], bl->y + dy[i]);
        if (c == 1 || c == 5)
            end = 0;
    }
    if (end)
    {
        *battle_get_option(bl) &= ~Option::CLOAK;  /* 念のための処理 */
    }
    return end;
}

/*
 *----------------------------------------------------------------------------
 * スキルユニット
 *----------------------------------------------------------------------------
 */

/*==========================================
 * 演奏/ダンスをやめる
 * flag 1で合奏中なら相方にユニットを任せる
 *
 *------------------------------------------
 */
void skill_stop_dancing(struct block_list *, int)
{
    // TODO remove this
}

/*==========================================
 * スキルユニット初期化
 *------------------------------------------
 */
struct skill_unit *skill_initunit(struct skill_unit_group *group, int idx,
                                   int x, int y)
{
    struct skill_unit *unit;

    nullpo_retr(NULL, group);
    nullpo_retr(NULL, unit = &group->unit[idx]);

    if (!unit->alive)
        group->alive_count++;

    unit->bl.id = map_addobject(&unit->bl);
    unit->bl.type = BL_SKILL;
    unit->bl.m = group->map;
    unit->bl.x = x;
    unit->bl.y = y;
    unit->group = group;
    unit->val1 = unit->val2 = 0;
    unit->alive = 1;

    map_addblock(&unit->bl);
    return unit;
}

void skill_unit_timer_sub_ondelete(struct block_list *bl,
        struct block_list *src, unsigned int tick);
/*==========================================
 * スキルユニット削除
 *------------------------------------------
 */
int skill_delunit(struct skill_unit *unit)
{
    struct skill_unit_group *group;
    int range;

    nullpo_ret(unit);
    if (!unit->alive)
        return 0;
    nullpo_ret(group = unit->group);

    /* onlimitイベント呼び出し */
    skill_unit_onlimit(unit, gettick());

    /* ondeleteイベント呼び出し */
    range = group->range;
    map_foreachinarea(std::bind(skill_unit_timer_sub_ondelete, ph::_1,  &unit->bl, gettick()),
            unit->bl.m, unit->bl.x - range, unit->bl.y - range,
            unit->bl.x + range, unit->bl.y + range, BL_NUL);

    unit->group = NULL;
    unit->alive = 0;
    map_delobjectnofree(unit->bl.id, BL_SKILL);
    if (group->alive_count > 0 && (--group->alive_count) <= 0)
        skill_delunitgroup(group);

    return 0;
}

/*==========================================
 * スキルユニットグループ初期化
 *------------------------------------------
 */
static
int skill_unit_group_newid = 10;
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
        int count, SkillID skillid, int skilllv, int unit_id)
{
    int i;
    struct skill_unit_group *group = NULL, *list = NULL;
    int maxsug = 0;

    nullpo_retr(NULL, src);

    if (src->type == BL_PC)
    {
        list = ((struct map_session_data *) src)->skillunit;
        maxsug = MAX_SKILLUNITGROUP;
    }
    else if (src->type == BL_MOB)
    {
        list = ((struct mob_data *) src)->skillunit;
        maxsug = MAX_MOBSKILLUNITGROUP;
    }
    if (list)
    {
        for (i = 0; i < maxsug; i++)    /* 空いているもの検索 */
            if (list[i].group_id == 0)
            {
                group = &list[i];
                break;
            }

        if (group == NULL)
        {                       /* 空いてないので古いもの検索 */
            int j = 0;
            unsigned maxdiff = 0, x, tick = gettick();
            for (i = 0; i < maxsug; i++)
                if ((x = DIFF_TICK(tick, list[i].tick)) > maxdiff)
                {
                    maxdiff = x;
                    j = i;
                }
            skill_delunitgroup(&list[j]);
            group = &list[j];
        }
    }

    if (group == NULL)
    {
        PRINTF("skill_initunitgroup: error unit group !\n");
        exit(1);
    }

    group->src_id = src->id;
    group->party_id = battle_get_party_id(src);
    group->group_id = skill_unit_group_newid++;
    if (skill_unit_group_newid <= 0)
        skill_unit_group_newid = 10;
    CREATE(group->unit, struct skill_unit, count);
    group->unit_count = count;
    group->val1 = group->val2 = 0;
    group->skill_id = skillid;
    group->skill_lv = skilllv;
    group->unit_id = unit_id;
    group->map = src->m;
    group->range = 0;
    group->limit = 10000;
    group->interval = 1000;
    group->tick = gettick();
    group->valstr = NULL;

    return group;
}

/*==========================================
 * スキルユニットグループ削除
 *------------------------------------------
 */
int skill_delunitgroup(struct skill_unit_group *group)
{
    int i;

    nullpo_ret(group);
    if (group->unit_count <= 0)
        return 0;

    group->alive_count = 0;
    if (group->unit != NULL)
    {
        for (i = 0; i < group->unit_count; i++)
            if (group->unit[i].alive)
                skill_delunit(&group->unit[i]);
    }
    if (group->valstr != NULL)
    {
        map_freeblock(group->valstr);
        group->valstr = NULL;
    }

    map_freeblock(group->unit);    /* free()の替わり */
    group->unit = NULL;
    group->src_id = 0;
    group->group_id = 0;
    group->unit_count = 0;
    return 0;
}

/*==========================================
 * スキルユニットグループ全削除
 *------------------------------------------
 */
int skill_clear_unitgroup(struct block_list *src)
{
    struct skill_unit_group *group = NULL;
    int maxsug = 0;

    nullpo_ret(src);

    if (src->type == BL_PC)
    {
        group = ((struct map_session_data *) src)->skillunit;
        maxsug = MAX_SKILLUNITGROUP;
    }
    else if (src->type == BL_MOB)
    {
        group = ((struct mob_data *) src)->skillunit;
        maxsug = MAX_MOBSKILLUNITGROUP;
    }
    if (group)
    {
        int i;
        for (i = 0; i < maxsug; i++)
            if (group[i].group_id > 0 && group[i].src_id == src->id)
                skill_delunitgroup(&group[i]);
    }
    return 0;
}

/*==========================================
 * スキルユニットグループの被影響tick検索
 *------------------------------------------
 */
struct skill_unit_group_tickset *skill_unitgrouptickset_search(
        struct block_list *bl, int group_id)
{
    int i, j = 0, k, s = group_id % MAX_SKILLUNITGROUPTICKSET;
    struct skill_unit_group_tickset *set = NULL;

    nullpo_ret(bl);

    if (bl->type == BL_PC)
    {
        set = ((struct map_session_data *) bl)->skillunittick;
    }
    else
    {
        set = ((struct mob_data *) bl)->skillunittick;
    }
    if (set == NULL)
        return 0;
    for (i = 0; i < MAX_SKILLUNITGROUPTICKSET; i++)
        if (set[(k = (i + s) % MAX_SKILLUNITGROUPTICKSET)].group_id ==
            group_id)
            return &set[k];
        else if (set[k].group_id == 0)
            j = k;

    return &set[j];
}

/*==========================================
 * スキルユニットグループの被影響tick削除
 *------------------------------------------
 */
int skill_unitgrouptickset_delete(struct block_list *bl, int group_id)
{
    int i, s = group_id % MAX_SKILLUNITGROUPTICKSET;
    struct skill_unit_group_tickset *set = NULL, *ts;

    nullpo_ret(bl);

    if (bl->type == BL_PC)
    {
        set = ((struct map_session_data *) bl)->skillunittick;
    }
    else
    {
        set = ((struct mob_data *) bl)->skillunittick;
    }

    if (set != NULL)
    {

        for (i = 0; i < MAX_SKILLUNITGROUPTICKSET; i++)
            if ((ts =
                 &set[(i + s) % MAX_SKILLUNITGROUPTICKSET])->group_id ==
                group_id)
                ts->group_id = 0;

    }
    return 0;
}

/*==========================================
 * スキルユニットタイマー発動処理用(foreachinarea)
 *------------------------------------------
 */
static
void skill_unit_timer_sub_onplace(struct block_list *bl,
        struct block_list *src, unsigned int tick)
{
    struct skill_unit *su;

    nullpo_retv(bl);

    su = (struct skill_unit *) src;

    if (su && su->alive)
    {
        struct skill_unit_group *sg;
        sg = su->group;
        if (sg && battle_check_target(src, bl, sg->target_flag) > 0)
            skill_unit_onplace(su, bl, tick);
    }
}

/*==========================================
 * スキルユニットタイマー削除処理用(foreachinarea)
 *------------------------------------------
 */
void skill_unit_timer_sub_ondelete(struct block_list *bl,
        struct block_list *src, unsigned int tick)
{
    struct skill_unit *su;

    nullpo_retv(bl);

    su = (struct skill_unit *) src;

    if (su && su->alive)
    {
        struct skill_unit_group *sg;
        sg = su->group;
        if (sg && battle_check_target(src, bl, sg->target_flag) > 0)
            skill_unit_ondelete(su, bl, tick);
    }
}

/*==========================================
 * スキルユニットタイマー処理用(foreachobject)
 *------------------------------------------
 */
static
void skill_unit_timer_sub(struct block_list *bl, unsigned int tick)
{
    struct skill_unit *unit;
    struct skill_unit_group *group;
    int range;

    nullpo_retv(bl);
    unit = (struct skill_unit *) bl;
    nullpo_retv(group = unit->group);

    if (!unit->alive)
        return;

    range = (unit->range != 0) ? unit->range : group->range;

    /* onplaceイベント呼び出し */
    if (unit->alive && unit->range >= 0)
    {
        map_foreachinarea(std::bind(skill_unit_timer_sub_onplace, ph::_1, bl, tick),
                bl->m, bl->x - range, bl->y - range,
                bl->x + range, bl->y + range, BL_NUL);
        if (group->unit_id == 0xaa
            && DIFF_TICK(tick, group->tick) >= 6000 * group->val2)
        {
            map_foreachinarea(std::bind(skill_idun_heal, ph::_1, unit),
                    bl->m, bl->x - range, bl->y - range,
                    bl->x + range, bl->y + range, BL_NUL);
            group->val2++;
        }
    }
    /* 時間切れ削除 */
    if (unit->alive &&
        (DIFF_TICK(tick, group->tick) >= group->limit
         || DIFF_TICK(tick, group->tick) >= unit->limit))
    {
        switch (group->unit_id)
        {

            case 0x8f:         /* ブラストマイン */
                group->unit_id = 0x8c;
                clif_changelook(bl, LOOK_BASE, group->unit_id);
                group->limit = DIFF_TICK(tick + 1500, group->tick);
                unit->limit = DIFF_TICK(tick + 1500, group->tick);
                break;
            case 0x90:         /* スキッドトラップ */
            case 0x91:         /* アンクルスネア */
            case 0x93:         /* ランドマイン */
            case 0x94:         /* ショックウェーブトラップ */
            case 0x95:         /* サンドマン */
            case 0x96:         /* フラッシャー */
            case 0x97:         /* フリージングトラップ */
            case 0x98:         /* クレイモアートラップ */
            case 0x99:         /* トーキーボックス */
            {
                struct block_list *src = map_id2bl(group->src_id);
                if (group->unit_id == 0x91 && group->val2)
                    ;
                else
                {
                    if (src && src->type == BL_PC)
                    {
                        struct item item_tmp;
                        memset(&item_tmp, 0, sizeof(item_tmp));
                        item_tmp.nameid = 1065;
                        item_tmp.identify = 1;
                        map_addflooritem(&item_tmp, 1, bl->m, bl->x, bl->y, NULL, NULL, NULL, 0);  // 罠返還
                    }
                }
            }
                // TODO: determine if this was supposed to be break
                FALLTHROUGH;
            default:
                skill_delunit(unit);
        }
    }

    if (group->unit_id == 0x8d)
    {
        unit->val1 -= 5;
        if (unit->val1 <= 0 && unit->limit + group->tick > tick + 700)
            unit->limit = DIFF_TICK(tick + 700, group->tick);
    }
}

/*==========================================
 * スキルユニットタイマー処理
 *------------------------------------------
 */
static
void skill_unit_timer(timer_id, tick_t tick, custom_id_t, custom_data_t)
{
    map_freeblock_lock();

    map_foreachobject(std::bind(skill_unit_timer_sub, ph::_1, tick), BL_SKILL);

    map_freeblock_unlock();
}

/*==========================================
 * スキルユニット移動時処理用(foreachinarea)
 *------------------------------------------
 */
static
void skill_unit_out_all_sub(struct block_list *bl,
        struct block_list *src, unsigned int tick)
{
    struct skill_unit *unit;
    struct skill_unit_group *group;
    int range;

    nullpo_retv(bl);
    nullpo_retv(src);
    unit = (struct skill_unit *) bl;
    nullpo_retv(group = unit->group);

    if (!unit->alive || src->prev == NULL)
        return;

    range = (unit->range != 0) ? unit->range : group->range;

    if (range < 0 || battle_check_target(bl, src, group->target_flag) <= 0)
        return;

    if (src->x >= bl->x - range && src->x <= bl->x + range &&
        src->y >= bl->y - range && src->y <= bl->y + range)
        skill_unit_onout(unit, src, tick);
}

/*==========================================
 * スキルユニット移動時処理
 *------------------------------------------
 */
int skill_unit_out_all(struct block_list *bl, unsigned int tick, int range)
{
    nullpo_ret(bl);

    if (bl->prev == NULL)
        return 0;

    if (range < 7)
        range = 7;
    map_foreachinarea(std::bind(skill_unit_out_all_sub, ph::_1, bl, tick),
            bl->m, bl->x - range, bl->y - range,
            bl->x + range, bl->y + range, BL_SKILL);

    return 0;
}

/*==========================================
 * スキルユニット移動時処理用(foreachinarea)
 *------------------------------------------
 */
static
void skill_unit_move_sub(struct block_list *bl,
        struct block_list *src, unsigned int tick)
{
    struct skill_unit *unit;
    struct skill_unit_group *group;
    int range;

    nullpo_retv(bl);
    nullpo_retv(unit = (struct skill_unit *) bl);
    nullpo_retv(src);

    if (!unit->alive || src->prev == NULL)
        return;

    if ((group = unit->group) == NULL)
        return;
    range = (unit->range != 0) ? unit->range : group->range;

    if (range < 0 || battle_check_target(bl, src, group->target_flag) <= 0)
        return;

    if (src->x >= bl->x - range && src->x <= bl->x + range &&
        src->y >= bl->y - range && src->y <= bl->y + range)
        skill_unit_onplace(unit, src, tick);
    else
        skill_unit_onout(unit, src, tick);
}

/*==========================================
 * スキルユニット移動時処理
 *------------------------------------------
 */
int skill_unit_move(struct block_list *bl, unsigned int tick, int range)
{
    nullpo_ret(bl);

    if (bl->prev == NULL)
        return 0;

    if (range < 7)
        range = 7;
    map_foreachinarea(std::bind(skill_unit_move_sub, ph::_1, bl, tick),
            bl->m, bl->x - range, bl->y - range,
            bl->x + range, bl->y + range, BL_SKILL);

    return 0;
}

/*==========================================
 * スキルユニット自体の移動時処理(foreachinarea)
 *------------------------------------------
 */
static
void skill_unit_move_unit_group_sub(struct block_list *bl,
        struct block_list *src, unsigned int tick)
{
    struct skill_unit *unit;
    struct skill_unit_group *group;
    int range;

    nullpo_retv(bl);
    nullpo_retv(src);
    unit = (struct skill_unit *) src;
    nullpo_retv(group = unit->group);

    if (!unit->alive || bl->prev == NULL)
        return;

    range = (unit->range != 0) ? unit->range : group->range;

    if (range < 0 || battle_check_target(src, bl, group->target_flag) <= 0)
        return;
    if (bl->x >= src->x - range && bl->x <= src->x + range &&
        bl->y >= src->y - range && bl->y <= src->y + range)
        skill_unit_onplace(unit, bl, tick);
    else
        skill_unit_onout(unit, bl, tick);
}

/*==========================================
 * スキルユニット自体の移動時処理
 * 引数はグループと移動量
 *------------------------------------------
 */
int skill_unit_move_unit_group(struct skill_unit_group *group, int m, int dx,
                                int dy)
{
    nullpo_ret(group);

    if (group->unit_count <= 0)
        return 0;

    if (group->unit != NULL)
    {
        if (!battle_config.unit_movement_type)
        {
            int i;
            for (i = 0; i < group->unit_count; i++)
            {
                struct skill_unit *unit = &group->unit[i];
                if (unit->alive && !(m == unit->bl.m && dx == 0 && dy == 0))
                {
                    int range = unit->range;
                    map_delblock(&unit->bl);
                    unit->bl.m = m;
                    unit->bl.x += dx;
                    unit->bl.y += dy;
                    map_addblock(&unit->bl);
                    if (range > 0)
                    {
                        if (range < 7)
                            range = 7;
                        map_foreachinarea(std::bind(skill_unit_move_unit_group_sub, ph::_1, &unit->bl, gettick()),
                                unit->bl.m, unit->bl.x - range, unit->bl.y - range,
                                unit->bl.x + range, unit->bl.y + range, BL_NUL);
                    }
                }
            }
        }
        else
        {
            int i, j, *r_flag, *s_flag, *m_flag;
            struct skill_unit *unit1;
            struct skill_unit *unit2;
            r_flag = (int *) malloc(sizeof(int) * group->unit_count);
            s_flag = (int *) malloc(sizeof(int) * group->unit_count);
            m_flag = (int *) malloc(sizeof(int) * group->unit_count);
            memset(r_flag, 0, sizeof(int) * group->unit_count);   // 継承フラグ
            memset(s_flag, 0, sizeof(int) * group->unit_count);   // 継承フラグ
            memset(m_flag, 0, sizeof(int) * group->unit_count);   // 継承フラグ

            //先にフラグを全部決める
            for (i = 0; i < group->unit_count; i++)
            {
                int move_check = 0;    // かぶりフラグ
                unit1 = &group->unit[i];
                for (j = 0; j < group->unit_count; j++)
                {
                    unit2 = &group->unit[j];
                    if (unit1->bl.m == m && unit1->bl.x + dx == unit2->bl.x
                        && unit1->bl.y + dy == unit2->bl.y)
                    {
                        //移動先にユニットがかぶってたら
                        s_flag[i] = 1;  // 移動前のユニットナンバーの継承フラグon
                        r_flag[j] = 1;  // かぶるユニットナンバーの残留フラグon
                        move_check = 1; //ユニットがかぶった。
                        break;
                    }
                }
                if (!move_check)    // ユニットがかぶってなかったら
                    m_flag[i] = 1;  // 移動前ユニットナンバーの移動フラグon
            }

            //フラグに基づいてユニット移動
            for (i = 0; i < group->unit_count; i++)
            {
                unit1 = &group->unit[i];
                if (m_flag[i])
                {               // 移動フラグがonで
                    if (!r_flag[i])
                    {           // 残留フラグがoffなら
                        //単純移動(rangeも継承の必要無し)
                        int range = unit1->range;
                        map_delblock(&unit1->bl);
                        unit1->bl.m = m;
                        unit1->bl.x += dx;
                        unit1->bl.y += dy;
                        map_addblock(&unit1->bl);
                        if (range > 0)
                        {
                            if (range < 7)
                                range = 7;
                            map_foreachinarea(std::bind(skill_unit_move_unit_group_sub, ph::_1, &unit1->bl, gettick()),

                                    unit1->bl.m, unit1->bl.x - range, unit1->bl.y - range,
                                    unit1->bl.x + range, unit1->bl.y + range, BL_NUL);
                        }
                    }
                    else
                    {           // 残留フラグがonなら
                        //空ユニットになるので、継承可能なユニットを探す
                        for (j = 0; j < group->unit_count; j++)
                        {
                            unit2 = &group->unit[j];
                            if (s_flag[j] && !r_flag[j])
                            {
                                // 継承移動(range継承付き)
                                int range = unit1->range;
                                map_delblock(&unit2->bl);
                                unit2->bl.m = m;
                                unit2->bl.x = unit1->bl.x + dx;
                                unit2->bl.y = unit1->bl.y + dy;
                                unit2->range = unit1->range;
                                map_addblock(&unit2->bl);
                                if (range > 0)
                                {
                                    if (range < 7)
                                        range = 7;
                                    map_foreachinarea(std::bind(skill_unit_move_unit_group_sub, ph::_1, &unit2->bl, gettick()),

                                            unit2->bl.m, unit2->bl.x - range, unit2->bl.y - range,
                                            unit2->bl.x + range, unit2->bl.y + range, BL_NUL);
                                }
                                s_flag[j] = 0;  // 継承完了したのでoff
                                break;
                            }
                        }
                    }
                }
            }
            free(r_flag);
            free(s_flag);
            free(m_flag);
        }
    }
    return 0;
}

/*----------------------------------------------------------------------------
 * アイテム合成
 *----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 * 初期化系
 */

static
SP scan_stat(char *statname)
{
    if (!strcasecmp(statname, "str"))
        return SP_STR;
    if (!strcasecmp(statname, "dex"))
        return SP_DEX;
    if (!strcasecmp(statname, "agi"))
        return SP_AGI;
    if (!strcasecmp(statname, "vit"))
        return SP_VIT;
    if (!strcasecmp(statname, "int"))
        return SP_INT;
    if (!strcasecmp(statname, "luk"))
        return SP_LUK;
    if (!strcasecmp(statname, "none"))
        return SP::ZERO;

    FPRINTF(stderr, "Unknown stat `%s'\n", statname);
    return SP::ZERO;
}

/*==========================================
 * スキル関係ファイル読み込み
 * skill_db.txt スキルデータ
 * skill_cast_db.txt スキルの詠唱時間とディレイデータ
 *------------------------------------------
 */
static
int skill_readdb(void)
{
    int j, k;
    FILE *fp;
    char line[1024], *p;

    /* The main skill database */
    memset(&skill_db, 0, sizeof(skill_db));
    fp = fopen_("db/skill_db.txt", "r");
    if (fp == NULL)
    {
        PRINTF("can't read db/skill_db.txt\n");
        return 1;
    }
    while (fgets(line, 1020, fp))
    {
        char *split[50], *split2[MAX_SKILL_LEVEL];
        if (line[0] == '/' && line[1] == '/')
            continue;
        for (j = 0, p = line; j < 18 && p; j++)
        {
            while (*p == '\t' || *p == ' ')
                p++;
            split[j] = p;
            p = strchr(p, ',');
            if (p)
                *p++ = 0;
        }
        if (split[17] == NULL || j < 18)
        {
            FPRINTF(stderr, "Incomplete skill db data online (%d entries)\n",
                     j);
            continue;
        }

        SkillID i = SkillID(atoi(split[0]));
        if (/*i < SkillID() ||*/ i > MAX_SKILL_DB)
            continue;

        memset(split2, 0, sizeof(split2));
        for (j = 0, p = split[1]; j < MAX_SKILL_LEVEL && p; j++)
        {
            split2[j] = p;
            p = strchr(p, ':');
            if (p)
                *p++ = 0;
        }
        for (k = 0; k < MAX_SKILL_LEVEL; k++)
            skill_db[i].range[k] =
                (split2[k]) ? atoi(split2[k]) : atoi(split2[0]);
        skill_db[i].hit = atoi(split[2]);
        skill_db[i].inf = atoi(split[3]);
        skill_db[i].pl = atoi(split[4]);
        skill_db[i].nk = atoi(split[5]);
        skill_db[i].max_raise = atoi(split[6]);
        skill_db[i].max = atoi(split[7]);

        memset(split2, 0, sizeof(split2));
        for (j = 0, p = split[8]; j < MAX_SKILL_LEVEL && p; j++)
        {
            split2[j] = p;
            p = strchr(p, ':');
            if (p)
                *p++ = 0;
        }
        for (k = 0; k < MAX_SKILL_LEVEL; k++)
            skill_db[i].num[k] =
                (split2[k]) ? atoi(split2[k]) : atoi(split2[0]);

        if (strcasecmp(split[9], "yes") == 0)
            skill_db[i].castcancel = 1;
        else
            skill_db[i].castcancel = 0;
        skill_db[i].cast_def_rate = atoi(split[9]);
        skill_db[i].inf2 = atoi(split[10]);
        skill_db[i].maxcount = atoi(split[11]);
        if (strcasecmp(split[13], "weapon") == 0)
            skill_db[i].skill_type = BF_WEAPON;
        else if (strcasecmp(split[12], "magic") == 0)
            skill_db[i].skill_type = BF_MAGIC;
        else if (strcasecmp(split[12], "misc") == 0)
            skill_db[i].skill_type = BF_MISC;
        else
            skill_db[i].skill_type = BF::ZERO;
        memset(split2, 0, sizeof(split2));
        for (j = 0, p = split[14]; j < MAX_SKILL_LEVEL && p; j++)
        {
            split2[j] = p;
            p = strchr(p, ':');
            if (p)
                *p++ = 0;
        }
        for (k = 0; k < MAX_SKILL_LEVEL; k++)
            skill_db[i].blewcount[k] =
                (split2[k]) ? atoi(split2[k]) : atoi(split2[0]);

        if (!strcasecmp(split[15], "passive"))
        {
            skill_pool_register(i);
            skill_db[i].poolflags = SKILL_POOL_FLAG;
        }
        else if (!strcasecmp(split[15], "active"))
        {
            skill_pool_register(i);
            skill_db[i].poolflags = SKILL_POOL_FLAG | SKILL_POOL_ACTIVE;
        }
        else
            skill_db[i].poolflags = SkillFlags::ZERO;

        skill_db[i].stat = scan_stat(split[16]);

        char *tmp = strdup(split[17]);
        {
            // replace "_" by " "
            char *s = tmp;
            while ((s = strchr(s, '_')))
                *s = ' ';
            if ((s = strchr(tmp, '\t'))
                || (s = strchr(tmp, ' '))
                || (s = strchr(tmp, '\n')))
                *s = '\000';
        }
        skill_lookup_by_id(i).desc = tmp;
    }
    fclose_(fp);
    PRINTF("read db/skill_db.txt done\n");

    return 0;
}

void skill_reload(void)
{
    /*
     *
     * <empty skill database>
     * <?>
     *
     */

    do_init_skill();
}

/*==========================================
 * スキル関係初期化処理
 *------------------------------------------
 */
int do_init_skill(void)
{
    skill_readdb();

    add_timer_interval(gettick() + SKILLUNITTIMER_INVERVAL,
                        skill_unit_timer, 0, 0, SKILLUNITTIMER_INVERVAL);

    return 0;
}

constexpr size_t num_names = sizeof(skill_names) / sizeof(skill_names[0]);

skill_name_db& skill_lookup_by_id(SkillID id)
{
    for (skill_name_db& ner : skill_names)
        if (ner.id == id)
            return ner;
    return skill_names[num_names - 1];
}

skill_name_db& skill_lookup_by_name(const char *name)
{
    for (skill_name_db& ner : skill_names)
        if (!strcasecmp(name, ner.name) || !strcasecmp(name, ner.desc))
            return ner;
    return skill_names[num_names - 1];
}
