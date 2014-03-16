#ifndef TMWA_MAP_CLIF_T_HPP
#define TMWA_MAP_CLIF_T_HPP

# include <cstdint>

enum class BeingRemoveWhy : uint8_t
{
    // general disappearance
    GONE = 0,
    // only case handled specially in client
    DEAD = 1,
    QUIT = 2,
    WARPED = 3,
    // handled specially in clif_clearchar - sent as 0 over network
    DISGUISE = 9,

    // handled speciall in mob_warp - not actually sent over network
    NEGATIVE1 = 0xff,
};

#endif // TMWA_MAP_CLIF_T_HPP
