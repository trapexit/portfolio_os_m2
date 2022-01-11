/* @(#) rand.c 95/09/04 1.6 */

#include <kernel/types.h>
#include <stdlib.h>

/* The random-number generator that the world is expected to use */

static unsigned _random_number_seed[55] =
/* The values here are just those that would be put in this horrid
   array by a call to __srand(1). DO NOT CHANGE __srand() without
   making a corresponding change to these initial values.
*/
{   0x00000001, 0x66d78e85, 0xd5d38c09, 0x0a09d8f5, 0xbf1f87fb,
    0xcb8df767, 0xbdf70769, 0x503d1234, 0x7f4f84c8, 0x61de02a3,
    0xa7408dae, 0x7a24bde8, 0x5115a2ea, 0xbbe62e57, 0xf6d57fff,
    0x632a837a, 0x13861d77, 0xe19f2e7c, 0x695f5705, 0x87936b2e,
    0x50a19a6e, 0x728b0e94, 0xc5cc55ae, 0xb10a8ab1, 0x856f72d7,
    0xd0225c17, 0x51c4fda3, 0x89ed9861, 0xf1db829f, 0xbcfbc59d,
    0x83eec189, 0x6359b159, 0xcc505c30, 0x9cbc5ac9, 0x2fe230f9,
    0x39f65e42, 0x75157bd2, 0x40c158fb, 0x27eb9a3e, 0xc582a2d9,
    0x0569d6c2, 0xed8e30b3, 0x1083ddd2, 0x1f1da441, 0x5660e215,
    0x04f32fc5, 0xe18eef99, 0x4a593208, 0x5b7bed4c, 0x8102fc40,
    0x515341d9, 0xacff3dfa, 0x6d096cb5, 0x2bb3cc1d, 0x253d15ff
};

static int _random_j = 23, _random_k = 54;

int rand()
{
/* See Knuth vol 2 section 3.2.2 for a discussion of this random
   number generator.
*/
    unsigned int temp;
    temp = (_random_number_seed[_random_k] += _random_number_seed[_random_j]);
    if (--_random_j < 0) _random_j = 54, --_random_k;
    else if (--_random_k < 0) _random_k = 54;
    return (temp & 0x7fffffff);         /* result is a 31-bit value */
    /* It seems that it would not be possible, under ANSI rules, to */
    /* implement this as a 32-bit value. What a shame!              */
}

void srand(unsigned int seed)
{
/* This only allows you to put 32 bits of seed into the random sequence,
   but it is very improbable that you have any good source of randomness
   that good to start with anyway! A linear congruential generator
   started from the seed is used to expand from 32 to 32*55 bits.
*/
    int i;
    _random_j = 23;
    _random_k = 54;
    for (i = 0; i<55; i++)
    {   _random_number_seed[i] = seed + (seed >> 16);
/* This is not even a good way of setting the initial values.  For instance */
/* a better scheme would have r<n+1> = 7^4*r<n> mod (3^31-1).  Still I will */
/* leave this for now.                                                      */
        seed = 69069*seed + 1725307361;  /* computed modulo 2^32 */
    }
}
