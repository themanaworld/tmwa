#ifndef TMWA_MAP_PC_T_HPP
#define TMWA_MAP_PC_T_HPP

# include <cstdint>

enum class PC_GAINEXP_REASON
{
    KILLING = 0,
    HEALING = 1,
    SCRIPT  = 2,
    SHARING = 3,

    UNKNOWN,
    COUNT,
};

enum class ADDITEM
{
    EXIST,
    NEW,
    OVERAMOUNT,

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

#endif // TMWA_MAP_PC_T_HPP
