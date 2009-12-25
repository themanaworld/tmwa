#ifndef __mt_rand_h
#define __mt_rand_h

void mt_seed (unsigned long seed);
unsigned long mt_reload (void);
unsigned long mt_random (void);
int  mt_rand (void);

#endif /* __mt_rand_h */
