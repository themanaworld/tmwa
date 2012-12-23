#ifndef PC_T_HPP
#define PC_T_HPP

#include <cstdint>

enum class PC_GAINEXP_REASON
{
    KILLING = 0,
#define PC_GAINEXP_REASON_KILLING PC_GAINEXP_REASON::KILLING
    HEALING = 1,
#define PC_GAINEXP_REASON_HEALING PC_GAINEXP_REASON::HEALING
    // temporary rename to avoid collision with npc subtypes
    SCRIPT_  = 2,
#define PC_GAINEXP_REASON_SCRIPT PC_GAINEXP_REASON::SCRIPT_

    COUNT,
};

enum class ADDITEM
{
    EXIST,
#define ADDITEM_EXIST ADDITEM::EXIST
    NEW,
#define ADDITEM_NEW ADDITEM::NEW
    OVERAMOUNT,
#define ADDITEM_OVERAMOUNT ADDITEM::OVERAMOUNT

    // when used as error in nullpo_retr
    ZERO = 0,
};

enum class CalcStatus
{
    NOW,
    LATER ,
};

enum class PickupFail : uint8_t
{
    OKAY        = 0,
    BAD_ITEM    = 1,
    TOO_HEAVY   = 2,
    TOO_FAR     = 3,
    INV_FULL    = 4,
    STACK_FULL  = 5,
    DROP_STEAL  = 6,
};

#endif // PC_T_HPP
