#ifndef BATTLE_T_HPP
#define BATTLE_T_HPP

#include "../common/utils.hpp"

namespace e
{
enum class BF : uint16_t
{
    ZERO        = 0x0000,
    NEGATIVE_1  = 0xffff,

    WEAPON      = 0x0001,
#define BF_WEAPON BF::WEAPON
    MAGIC       = 0x0002,
#define BF_MAGIC BF::MAGIC
    MISC        = 0x0004,
#define BF_MISC BF::MISC
    SHORT       = 0x0010,
#define BF_SHORT BF::SHORT
    LONG        = 0x0040,
#define BF_LONG BF::LONG
    SKILL       = 0x0100,
#define BF_SKILL BF::SKILL
    NORMAL      = 0x0200,
#define BF_NORMAL BF::NORMAL
    WEAPONMASK  = 0x000f,
#define BF_WEAPONMASK BF::WEAPONMASK
    RANGEMASK   = 0x00f0,
#define BF_RANGEMASK BF::RANGEMASK
    SKILLMASK   = 0x0f00,
#define BF_SKILLMASK BF::SKILLMASK
};
ENUM_BITWISE_OPERATORS(BF)
}
using e::BF;

struct BCT
{
                        // former representation:
    uint8_t lo;         // 0x 00 00 00 ff
    uint8_t mid;        // 0x 00 00 ff 00
    uint8_t classic:4;  // 0x 00 0f 00 00
    uint8_t level:4;    // 0x 00 f0 00 00
    uint8_t unused;     // 0x ff 00 00 00

    operator bool() { return lo || mid || classic || level || unused; }
};

constexpr
BCT operator & (BCT l, BCT r) { return {uint8_t(l.lo & r.lo), uint8_t(l.mid & r.mid), uint8_t(l.classic & r.classic), uint8_t(l.level & r.level), uint8_t(l.unused & r.unused) }; }
constexpr
BCT operator | (BCT l, BCT r) { return {uint8_t(l.lo | r.lo), uint8_t(l.mid | r.mid), uint8_t(l.classic | r.classic), uint8_t(l.level | r.level), uint8_t(l.unused | r.unused) }; }
constexpr
BCT operator ^ (BCT l, BCT r) { return {uint8_t(l.lo ^ r.lo), uint8_t(l.mid ^ r.mid), uint8_t(l.classic ^ r.classic), uint8_t(l.level ^ r.level), uint8_t(l.unused ^ r.unused) }; }
inline
BCT& operator &= (BCT& l, BCT r) { return l = l & r; }
inline
BCT& operator |= (BCT& l, BCT r) { return l = l & r; }
inline
BCT& operator ^= (BCT& l, BCT r) { return l = l & r; }
// BCT operator ~(BCT r);

constexpr
bool operator == (BCT l, BCT r) { return l.lo == r.lo && l.mid == r.mid && l.classic == r.classic && l.level == r.level && l.unused == r.unused; };
constexpr
bool operator != (BCT l, BCT r) { return !(l == r); }

constexpr
BCT BCT_NOENEMY = {0x00, 0x00, 0x0, 0x0, 0x00};
constexpr
BCT BCT_ZERO = BCT_NOENEMY;
constexpr
BCT BCT_PARTY   = {0x00, 0x00, 0x1, 0x0, 0x00};
constexpr
BCT BCT_ENEMY   = {0x00, 0x00, 0x4, 0x0, 0x00};
constexpr
BCT BCT_NOPARTY = {0x00, 0x00, 0x5, 0x0, 0x00};
constexpr
BCT BCT_ALL     = {0x00, 0x00, 0x2, 0x0, 0x00};
constexpr
BCT BCT_NOONE   = {0x00, 0x00, 0x6, 0x0, 0x00};

constexpr
BCT BCT_lo_x01  = {0x01, 0x00, 0x0, 0x0, 0x00};
constexpr
BCT BCT_lo_x02  = {0x02, 0x00, 0x0, 0x0, 0x00};
constexpr
BCT BCT_mid_x05 = {0x00, 0x05, 0x0, 0x0, 0x00};
constexpr
BCT BCT_mid_x80 = {0x00, 0x80, 0x0, 0x0, 0x00};

constexpr
BCT BCT_highnib = {0x00, 0x00, 0x0, 0xf, 0x00};

#endif // BATTLE_T_HPP
