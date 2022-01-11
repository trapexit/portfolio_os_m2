/*
 *	@(#) rand.c 96/03/06 1.9
 *	Copyright 1994,1995, The 3DO Company
 *
 * Random number generator.
 */

#include "kernel/types.h"
#include "dipir.h"

extern void Discard(uint32);

/*****************************************************************************
  HardwareRandomNumber

  Using same method used in kernel/random.c
*/
uint32 
HardwareRandomNumber(void)
{
	vuint32 *noise;
	uint32 lrand;
	uint32 rrand;

	noise = (vuint32 *) 0x61ffc;

	/*
	 * Back-to-back reads of DSPPNoise may not be completely random,
	 * so we read it a couple extra times and discard the middle ones.
	 */
	lrand = *noise;
	Discard(*noise);
	Discard(*noise);
	Discard(*noise);
	rrand = *noise;
	return (lrand << 16) | rrand;
}

/*
 * Use simple and small version of random number generator for dipir.
 * This is the ANSI-recommended version.
 */

static uint32 next = 1;

/*****************************************************************************
*/
uint32
urand(void)
{
	next = next * 1103515245 + 12345;
	return next;
}

/*****************************************************************************
*/
void 
srand(uint32 seed)
{
	if (seed == 0)
		seed = HardwareRandomNumber();
	next = seed;
}

/*****************************************************************************
  ScaledRand

  Return a random number between 0 and scale-1.
*/
uint32 
ScaledRand(uint32 scale) 
{
	uint32 r;
		
	r = urand();
	if (scale > 0)
		r %= scale;
	return r;
}

