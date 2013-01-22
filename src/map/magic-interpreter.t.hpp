#ifndef MAGIC_INTERPRETER_T_HPP
#define MAGIC_INTERPRETER_T_HPP

#include "../common/utils2.hpp"

enum class SPELLARG : uint8_t
{
    NONE,
#define SPELLARG_NONE SPELLARG::NONE
    PC,
#define SPELLARG_PC SPELLARG::PC
    STRING,
#define SPELLARG_STRING SPELLARG::STRING
};

enum class TY : uint8_t
{
    UNDEF,
#define TY_UNDEF TY::UNDEF
    INT,
#define TY_INT TY::INT
    DIR,
#define TY_DIR TY::DIR
    STRING,
#define TY_STRING TY::STRING
    ENTITY,
#define TY_ENTITY TY::ENTITY
    LOCATION,
#define TY_LOCATION TY::LOCATION
    AREA,
#define TY_AREA TY::AREA
    SPELL,
#define TY_SPELL TY::SPELL
    INVOCATION,
#define TY_INVOCATION TY::INVOCATION
    FAIL = 127,
#define TY_FAIL TY::FAIL

    NEGATIVE_1 = 255,
};

// Note: there is also a typedef by this name in <dirent.h>
// but we should be fine since we never include it.
// (in the long term we should still rename this though)
enum class DIR : uint8_t
{
    S   = 0,
#define DIR_S DIR::S
    SW  = 1,
#define DIR_SW DIR::SW
    W   = 2,
#define DIR_W DIR::W
    NW  = 3,
#define DIR_NW DIR::NW
    N   = 4,
#define DIR_N DIR::N
    NE  = 5,
#define DIR_NE DIR::NE
    E   = 6,
#define DIR_E DIR::E
    SE  = 7,
#define DIR_SE DIR::SE

    COUNT,
};

enum class AREA : uint8_t
{
    LOCATION,
#define AREA_LOCATION AREA::LOCATION
    UNION,
#define AREA_UNION AREA::UNION
    RECT,
#define AREA_RECT AREA::RECT
    BAR,
#define AREA_BAR AREA::BAR
};

enum class EXPR : uint8_t
{
    VAL,
#define EXPR_VAL EXPR::VAL
    LOCATION,
#define EXPR_LOCATION EXPR::LOCATION
    AREA,
#define EXPR_AREA EXPR::AREA
    FUNAPP,
#define EXPR_FUNAPP EXPR::FUNAPP
    ID,
#define EXPR_ID EXPR::ID
    SPELLFIELD,
#define EXPR_SPELLFIELD EXPR::SPELLFIELD
};

// temporary rename to avoid collision with enum value
// in magic-interpreter-parser
enum class EFFECT_ : uint8_t
{
    SKIP,
#define EFFECT_SKIP EFFECT_::SKIP
    ABORT,
#define EFFECT_ABORT EFFECT_::ABORT
    ASSIGN,
#define EFFECT_ASSIGN EFFECT_::ASSIGN
    FOREACH,
#define EFFECT_FOREACH EFFECT_::FOREACH
    FOR,
#define EFFECT_FOR EFFECT_::FOR
    IF,
#define EFFECT_IF EFFECT_::IF
    SLEEP,
#define EFFECT_SLEEP EFFECT_::SLEEP
    // temporary rename to avoid collision with NpcSubtype
    SCRIPT_,
#define EFFECT_SCRIPT EFFECT_::SCRIPT_
    BREAK,
#define EFFECT_BREAK EFFECT_::BREAK
    OP,
#define EFFECT_OP EFFECT_::OP
    END,
#define EFFECT_END EFFECT_::END
    CALL,
#define EFFECT_CALL EFFECT_::CALL
};

enum class FOREACH_FILTER : uint8_t
{
    MOB,
#define FOREACH_FILTER_MOB FOREACH_FILTER::MOB
    PC,
#define FOREACH_FILTER_PC FOREACH_FILTER::PC
    ENTITY,
#define FOREACH_FILTER_ENTITY FOREACH_FILTER::ENTITY
    TARGET,
#define FOREACH_FILTER_TARGET FOREACH_FILTER::TARGET
    SPELL,
#define FOREACH_FILTER_SPELL FOREACH_FILTER::SPELL
    NPC,
#define FOREACH_FILTER_NPC FOREACH_FILTER::NPC
};

enum class SPELLGUARD : uint8_t
{
    CONDITION,
#define SPELLGUARD_CONDITION SPELLGUARD::CONDITION
    COMPONENTS,
#define SPELLGUARD_COMPONENTS SPELLGUARD::COMPONENTS
    CATALYSTS,
#define SPELLGUARD_CATALYSTS SPELLGUARD::CATALYSTS
    CHOICE,
#define SPELLGUARD_CHOICE SPELLGUARD::CHOICE
    MANA,
#define SPELLGUARD_MANA SPELLGUARD::MANA
    CASTTIME,
#define SPELLGUARD_CASTTIME SPELLGUARD::CASTTIME
    EFFECT,
#define SPELLGUARD_EFFECT SPELLGUARD::EFFECT
};

namespace e
{
enum class SPELL_FLAG : uint8_t
{
    ZERO        = 0,

    // spell associated not with caster but with place
    LOCAL       = 1 << 0,
#define SPELL_FLAG_LOCAL SPELL_FLAG::LOCAL
    // spell invocation never uttered
    SILENT      = 1 << 1,
#define SPELL_FLAG_SILENT SPELL_FLAG::SILENT
    // `magic word' only:  don't require spellcasting ability
    NONMAGIC    = 1 << 2,
#define SPELL_FLAG_NONMAGIC SPELL_FLAG::NONMAGIC
};
ENUM_BITWISE_OPERATORS(SPELL_FLAG)
}
using e::SPELL_FLAG;

enum class CONT_STACK : uint8_t
{
    FOREACH,
#define CONT_STACK_FOREACH CONT_STACK::FOREACH
    FOR,
#define CONT_STACK_FOR CONT_STACK::FOR
    PROC,
#define CONT_STACK_PROC CONT_STACK::PROC
};

namespace e
{
enum class INVOCATION_FLAG : uint8_t
{
    ZERO        = 0,

    // Bound directly to the caster (i.e., ignore its location)
    BOUND       = 1 << 0,
#define INVOCATION_FLAG_BOUND INVOCATION_FLAG::BOUND
    // Used `abort' to terminate
    ABORTED     = 1 << 1,
#define INVOCATION_FLAG_ABORTED INVOCATION_FLAG::ABORTED
    // On magical attacks: if we run out of steam, stop attacking altogether
    STOPATTACK  = 1 << 2,
#define INVOCATION_FLAG_STOPATTACK INVOCATION_FLAG::STOPATTACK
};
ENUM_BITWISE_OPERATORS(INVOCATION_FLAG)
}
using e::INVOCATION_FLAG;

#endif // MAGIC_INTERPRETER_T_HPP
