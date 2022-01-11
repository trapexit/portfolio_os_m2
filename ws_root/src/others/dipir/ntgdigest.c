/*
 *	@(#) ntgdigest.c 96/02/19 1.7
 *	Copyright 1994,1995, The 3DO Company
 *
 *	code for RSA and MD5 checking
 *	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
 */

#include "kernel/types.h"
#include "dipir/rsa.h"
#include "global.h"
#include "dipir.h"
#include "insysrom.h"

#undef DUMP_DIGEST /* 1 = dump checksum, 2 = dump bytes */

#ifdef DUMP_DIGEST
uint32 digestCount;
uint32 digestChecksum;
#endif

/*
 * We have two contexts to take care of. Dipir can happen in the
 * middle of the kernels rsa check, so they must be kept separate.
 */
#define DIPIR_CONTEXT	0


/* Routines called by either dipir or OS */
void
InitDigest(MD5_CTX *d, Boolean noprint)
{
	TOUCH(noprint);
#ifdef DUMP_DIGEST
	if (!noprint)
		PRINTF(("=== Start digest(%x)\n", d));
	digestCount = 0;
	digestChecksum = 0;
#endif
	if (d == DIPIR_CONTEXT)
		d = &dtmp->dt_DipirDigestContext;
	MD5Init(d);
}

void
UpdateDigest(MD5_CTX *d, void *buffer, uint32 bufLen, Boolean noprint)
{
	TOUCH(noprint);
#ifdef DUMP_DIGEST
    {	uint32 i;
	digestCount += bufLen;
	for (i = 0;  i < bufLen;  i++)  digestChecksum ^= ((uint8*)buffer)[i];
#if DUMP_DIGEST == 2
	if (!noprint)
		DumpBytes("Digest", buffer, bufLen);
#endif
    }
#endif
	if (d == DIPIR_CONTEXT)
		d = &dtmp->dt_DipirDigestContext;
	MD5Update(d, buffer, bufLen);
}

void
FinalDigest(MD5_CTX *d, void *result, Boolean noprint)
{
	TOUCH(noprint);
#ifdef DUMP_DIGEST
	if (!noprint)
	{
		PRINTF(("=== End digest(%x) count %x, chksum %x\n",
			d, digestCount, digestChecksum));
	}
#endif
	if (d == DIPIR_CONTEXT)
	{
		d = &dtmp->dt_DipirDigestContext;
		result = dtmp->dt_DipirDigest.digest;
	}
	MD5Final(result,d);
}

/* Routines called only by dipir */
void DipirInitDigest(void)
{
	InitDigest(DIPIR_CONTEXT, FALSE);
}

void DipirUpdateDigest(void *buffer, uint32 bufLen)
{
	UpdateDigest(DIPIR_CONTEXT, buffer, bufLen, FALSE);
}

void DipirFinalDigest(void)
{
	FinalDigest(DIPIR_CONTEXT, 0, FALSE);
}
