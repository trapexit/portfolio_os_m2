/*
 *	@(#) rsa.c 95/09/13 1.10
 *	Copyright 1995, The 3DO Company
 *
 * RSA decryption algorithm.
 */

/*
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
*/

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"


#define maxSigSize   128
#define maxKeySize   (maxSigSize+1)
#define maxSigSizeW  ((maxSigSize+4)/4)
#define maxKeySizeW  ((maxKeySize+4)/4)


extern void ShiftAddMod(uint32 *pResult, uint32 *pMod, uint32 lenInWords,
			uint32 *pAddend, uint32 addNow);
extern void ModBigNum(uint32 *pResult, uint32 *pMod, uint32 lenInWords);


#ifdef NO_ASM_RTNS
/****************************************************************************
  ShiftLeft

  Shift a large value left one bit.  If high bit shifts out, return carry.
*/
uint32 ShiftLeft (uint32 *pNum, uint32 keySzW)
{
	uint32 carry = 0;
	uint32 lastCarry = 0;
	uint32 word;

	for (word = keySzW - 1; word < keySzW; word--) {
		carry = (pNum[word] & 0x80000000) >> 31;
		pNum[word] = (pNum[word] << 1) + lastCarry;
	lastCarry = carry;
	}
	return carry;
}
#endif
	
#ifdef NO_ASM_RTNS
/****************************************************************************
  AddTo

  Add two large values together.
*/
void AddTo (uint32 *pResult, uint32 *pAddend, uint32 keySzW)
{
	uint32 carry = 0;
	uint32 cNext;
	uint32 word;

	for (word = keySzW - 1; word < keySzW; word--) {
		cNext = pResult[word] + pAddend[word] + carry;
		if (cNext < pResult[word]) {
			carry = 1;
		} else {
			carry = 0;
		}
		pResult[word] = cNext;
	}
}
#endif
	
#ifdef NO_ASM_RTNS
/****************************************************************************
  SubFrom

  Subtract one large value from another
*/
void SubFrom (uint32 *pResult, uint32 *pSubt, uint32 keySzW)
{
	uint32 carry = 0;
	uint32 cNext;
	uint32 word;

	for (word = keySzW - 1; word < keySzW; word--) {
		cNext = pResult[word] - pSubt[word] - carry;
		if (cNext > pResult[word]) {
			carry = 1;
		} else {
			carry = 0;
		}
		pResult[word] = cNext;
	}
}
#endif
	
#ifdef NO_ASM_RTNS
/****************************************************************************
  Bigger

  Check to see if pX is larger than pY.
  This treats equal as being bigger.
*/
bool Bigger (uint32 *pX, uint32 *pY, uint32 keySzW)
{
	uint32 word;

	for (word = 0; word < keySzW; word++) {
		if (pX[word] > pY[word]) return TRUE;
		if (pX[word] < pY[word]) return FALSE;
	}
	return TRUE;
}
#endif

#ifdef NO_ASM_RTNS
/****************************************************************************
  ModNum

  Make sure number pResult is smaller than mod pMod
*/
void ModNum (uint32 *pResult, uint32 *pMod, uint32 keySzW)
{
	while (1) {
		if (Bigger (pResult, pMod)) {
			SubFrom (pResult, pMod);
		} else {
			break;
		}
	}
}
#endif

/****************************************************************************
  MulMod

  Multiply two values together and MOD the result.  
*/
void MulMod (uint32 *pResult, uint32 *pX, uint32 *pY,
	     uint32 *pMod, uint32 keySzW)
{
	uint32 bit;
	uint32 word;
	uint32 startBit = 0;
	uint32 startWord = keySzW;


	/* Initialize result to 0 and find start word and bit */
	for (word = 0; word < keySzW; word++) {
		pResult [word] = 0;
		/* Find the start word and bit if not known yet */
		if (!startBit) {
			if (pX[word]) {
				for (startBit = 0x80000000; ; startBit >>= 1) {
					if (pX[word] & startBit) {
						break;
					}
				}
				startWord = word;
			}
		}
	}

	/* Shift and add to multiply pX by pY */
	for (word = startWord; word < keySzW; word++) {
		for (bit = startBit; bit; bit >>= 1) {
#ifdef NO_ASM_RTNS
			ShiftLeft (pResult);
			if (pX[word] & bit) {
				AddTo (pResult, pY);
			}

			/* Mod the result with pMod */
			ModNum (pResult, pMod);
#else
			ShiftAddMod(pResult, pMod, keySzW, pY, pX[word] & bit);
#endif
		}
		startBit = 0x80000000;
	}
}

/****************************************************************************
  aRSAx

  Take input pInput (an x byte array), raise it to a power of pExp (a y byte
  array) and mod it with pMod (an x byte array).  The result is returned in
  pOutput (an x byte array).
*/
int32 aRSAx (uchar *pOutput, uchar *pInput, uchar *pMod,
	     uint32 exp, uint32 sigSize)
{
	uint32 keySzW = ((sigSize+1)+4)/4;
	uint32 expBit;
        uint32 mod [maxKeySizeW];
        uint32 input [maxKeySizeW];
        uint32 result1 [maxKeySizeW];
        uint32 result2 [maxKeySizeW];
	uint32 *pResultL;
	uint32 *pResultR;
	uint32 *pTemp;


#ifdef BUILD_STRINGS
	/* Find the start bit in the exponent */
	if (exp == 0) {
		EPRINTF(("Invalid exponent\n"));
		return -1;
	}
#endif
	for (expBit = 0x80000000; ; expBit >>= 1) {
		if (exp & expBit) {
			/* Skip the first bit */
			expBit >>= 1;
			break;
		}
	}

	mod[0] = (uint32)pMod[0];
	memcpy((uchar *)&mod[1], &pMod[1], sigSize);
	input[0] = 0;
	memcpy((uchar *)&input[1], pInput, sigSize);
	memcpy((uchar *)result2, (uchar *)input, keySzW * sizeof(uint32));

#ifdef NO_ASM_RTNS
	/* Mod the number to begin with in case its too big already */
	ModNum (result2, mod, keySzW);
#else
	/* Mod the number to begin with in case its too big already */
	ModBigNum (result2, mod, keySzW);
#endif

	pResultL = result1;
	pResultR = result2;

	for (; expBit; expBit >>= 1) {
		MulMod (pResultL, pResultR, pResultR, mod, keySzW);
		if (exp & expBit) {
			MulMod (pResultR, pResultL, input, mod, keySzW);
		} else {
			pTemp = pResultR;
			pResultR = pResultL;
			pResultL = pTemp;
		}
	}

	/* Replace memcpy... */
	memcpy (pOutput, (uchar *)&pResultR[1], sigSize);

	return 0;
}

/* Old way.  Shouldn't be needed anymore */
#if 0
/* Signatures are 64 byte quantities */
#define sigSize64    64

/****************************************************************************
  aRSA

  Take input pInput (an x byte array), raise it to a power of pExp (a y byte
  array) and mod it with pMod (an x byte array).  The result is returned in
  pOutput (an x byte array).

  Stub so old API callers can call new API.
*/
int32 aRSA (uchar *pOutput, uchar *pInput, uchar *pMod, uint32 exp)
{
	return aRSAx(pOutput, pInput, pMod, exp, sigSize64);
}
#endif
