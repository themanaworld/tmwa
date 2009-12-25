/*
// This is the ``Mersenne Twister'' random number generator MT19937, which
// generates pseudorandom integers uniformly distributed in 0..(2^32 - 1)
// starting from any odd seed in 0..(2^32 - 1).  This version is a recode
// by Shawn Cokus (Cokus@math.washington.edu) on March 8, 1998 of a version by
// Takuji Nishimura (who had suggestions from Topher Cooper and Marc Rieffel in
// July-August 1997).
//
// Effectiveness of the recoding (on Goedel2.math.washington.edu, a DEC Alpha
// running OSF/1) using GCC -O3 as a compiler: before recoding: 51.6 sec. to
// generate 300 million random numbers; after recoding: 24.0 sec. for the same
// (i.e., 46.5% of original time), so speed is now about 12.5 million random
// number generations per second on this machine.
//
// According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
// (and paraphrasing a bit in places), the Mersenne Twister is ``designed
// with consideration of the flaws of various existing generators,'' has
// a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
// equidistributed, and ``has passed many stringent tests, including the
// die-hard test of G. Marsaglia and the load test of P. Hellekalek and
// S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
// to 5012 bytes of static data, depending on data type sizes, and the code
// is quite short as well).  It generates random numbers in batches of 624
// at a time, so the caching and pipelining of modern systems is exploited.
// It is also divide- and mod-free.
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation (either version 2 of the License or, at your
// option, any later version).  This library is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY, without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
// the GNU Library General Public License for more details.  You should have
// received a copy of the GNU Library General Public License along with this
// library; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307, USA.
//
// The code as Shawn received it included the following notice:
//
//   Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.  When
//   you use this, send an e-mail to <matumoto@math.keio.ac.jp> with
//   an appropriate reference to your work.
//
// It would be nice to CC: <Cokus@math.washington.edu> when you write.
//
*/

#include <time.h>
#include "mt_rand.h"

#define N              (624)    /* length of state vector */
#define M              (397)    /* a period parameter */
#define K              (0x9908B0DFU)    /* a magic constant */
#define hiBit(u)       ((u) & 0x80000000U)  /* mask all but highest bit of u */
#define loBit(u)       ((u) & 0x00000001U)  /* mask all but lowest bit of u */
#define loBits(u)      ((u) & 0x7FFFFFFFU)  /* mask the highest bit of u */
#define mixBits(u, v)  (hiBit(u)|loBits(v)) /* move hi bit of u to hi bit of v */

static unsigned long state[N + 1];  /* state vector + 1 extra to not violate ANSI C */
static unsigned long *next;     /* next random value is computed from here */
static int left = -1;           /* can *next++ this many times before reloading */

void mt_seed (unsigned long seed)
{
    register unsigned long x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
    register int j;

    for (left = 0, *s++ = x, j = N; --j; *s++ = (x *= 69069U) & 0xFFFFFFFFU);
}

unsigned long mt_reload (void)
{
    register unsigned long *p0 = state, *p2 = state + 2, *pM =
        state + M, s0, s1;
    register int j;

    if (left < -1)
        mt_seed (time (NULL));

    left = N - 1, next = state + 1;

    for (s0 = state[0], s1 = state[1], j = N - M + 1; --j;
         s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits (s0, s1) >> 1) ^ (loBit (s1) ? K : 0U);

    for (pM = state, j = M; --j; s0 = s1, s1 = *p2++)
        *p0++ = *pM++ ^ (mixBits (s0, s1) >> 1) ^ (loBit (s1) ? K : 0U);

    s1 = state[0], *p0 =
        *pM ^ (mixBits (s0, s1) >> 1) ^ (loBit (s1) ? K : 0U);
    s1 ^= (s1 >> 11);
    s1 ^= (s1 << 7) & 0x9D2C5680U;
    s1 ^= (s1 << 15) & 0xEFC60000U;
    return (s1 ^ (s1 >> 18));
}

unsigned long mt_random (void)
{
    unsigned long y;

    if (--left < 0)
        return (mt_reload ());

    y = *next++;
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    return (y ^ (y >> 18));
}

int mt_rand (void)
{
    unsigned long r = mt_random ();
    while (r >> 16)
        r = (r & 0xFFFF) + (r >> 16);

    return (r);
}
