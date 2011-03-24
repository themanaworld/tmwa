#ifndef MT_RAND_H
#define MT_RAND_H

# include "sanity.h"

/// Initialize the generator (called automatically with time() if you don't)
void mt_seed (uint32_t seed);
/// Get a random number
uint32_t mt_random (void);

/**
 * ModuloRand and ModuloPlusRand
 * These macros are used to replace the vast number of calls to rand()%mod
 * TODO eliminate the rest of the calls to rand()
 * MRAND(10)    returns 0..9
 * MPRAND(5,10) returns 5..14
 */
# define MRAND(mod) (mt_random() % (mod))
# define MPRAND(add, mod) ((add) + MRAND(mod))

#endif // MT_RAND_H
