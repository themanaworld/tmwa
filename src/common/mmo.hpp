/// Global structures and defines
#ifndef MMO_HPP
#define MMO_HPP

# include <ctime>

# include "utils.hpp"

# define FIFOSIZE_SERVERLINK    256*1024

// set to 0 to not check IP of player between each server.
// set to another value if you want to check (1)
# define CMP_AUTHFIFO_IP 1

# define CMP_AUTHFIFO_LOGIN2 1

# define MAX_MAP_PER_SERVER 512
# define MAX_INVENTORY 100
# define MAX_AMOUNT 30000
# define MAX_ZENY 1000000000     // 1G zeny
# define MAX_CART 100
enum class SkillID : uint16_t;
constexpr SkillID MAX_SKILL = SkillID(474); // not 450
# define GLOBAL_REG_NUM 96
# define ACCOUNT_REG_NUM 16
# define ACCOUNT_REG2_NUM 16
# define DEFAULT_WALK_SPEED 150
# define MIN_WALK_SPEED 0
# define MAX_WALK_SPEED 1000
# define MAX_STORAGE 300
# define MAX_PARTY 12

# define MIN_HAIR_STYLE battle_config.min_hair_style
# define MAX_HAIR_STYLE battle_config.max_hair_style
# define MIN_HAIR_COLOR battle_config.min_hair_color
# define MAX_HAIR_COLOR battle_config.max_hair_color
# define MIN_CLOTH_COLOR battle_config.min_cloth_color
# define MAX_CLOTH_COLOR battle_config.max_cloth_color

// for produce
# define MIN_ATTRIBUTE 0
# define MAX_ATTRIBUTE 4
# define ATTRIBUTE_NORMAL 0
# define MIN_STAR 0
# define MAX_STAR 3

# define MIN_PORTAL_MEMO 0
# define MAX_PORTAL_MEMO 2

# define MAX_STATUS_TYPE 5

# define CHAR_CONF_NAME  "conf/char_athena.conf"

struct item
{
    int id;
    short nameid;
    short amount;
    unsigned short equip;
    char identify;
    char refine;
    char attribute;
    short card[4];
    short broken;
};

struct point
{
    char map[24];
    short x, y;
};

struct skill
{
    SkillID id;
    unsigned short lv, flags;
};

struct global_reg
{
    char str[32];
    int value;
};

struct mmo_charstatus
{
    int char_id;
    int account_id;
    int partner_id;

    int base_exp, job_exp, zeny;

    short pc_class;
    short status_point, skill_point;
    int hp, max_hp, sp, max_sp;
    short option, karma, manner;
    short hair, hair_color, clothes_color;
    int party_id;

    short weapon, shield;
    short head_top, head_mid, head_bottom;

    char name[24];
    unsigned char base_level, job_level;
    short str, agi, vit, int_, dex, luk;
    unsigned char char_num, sex;

    unsigned long mapip;
    unsigned int mapport;

    struct point last_point, save_point, memo_point[10];
    struct item inventory[MAX_INVENTORY], cart[MAX_CART];
    earray<struct skill, SkillID, MAX_SKILL> skill;
    int global_reg_num;
    struct global_reg global_reg[GLOBAL_REG_NUM];
    int account_reg_num;
    struct global_reg account_reg[ACCOUNT_REG_NUM];
    int account_reg2_num;
    struct global_reg account_reg2[ACCOUNT_REG2_NUM];
};

struct storage
{
    int dirty;
    int account_id;
    short storage_status;
    short storage_amount;
    struct item storage_[MAX_STORAGE];
};

struct map_session_data;

struct gm_account
{
    int account_id;
    int level;
};

struct party_member
{
    int account_id;
    char name[24], map[24];
    int leader, online, lv;
    struct map_session_data *sd;
};

struct party
{
    int party_id;
    char name[24];
    int exp;
    int item;
    struct party_member member[MAX_PARTY];
};

struct square
{
    int val1[5];
    int val2[5];
};

#endif // MMO_HPP
