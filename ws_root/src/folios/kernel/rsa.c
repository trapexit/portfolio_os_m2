/* @(#) rsa.c 96/03/04 1.17 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <dipir/rsa.h>
#include <dipir/dipirpub.h>
#include <stdio.h>
#include <kernel/internalf.h>

#define DBUG(x)		/*printf x*/

#define	DipirRoutines \
	((PublicDipirRoutines *)(KB_FIELD(kb_DipirRoutines)))



/*****************************************************************************
  RSADigestInit
  Begin the process of checking RSA signature on a bunch of data.
  This is the first of three routines that is called to do the signature check.
*/

int32
RSADigestInit(DipirDigestContext *info)
{
	int32 ret;

	ret = (*DipirRoutines->pdr_GenRSADigestInit)(info);
	DBUG(("RSADigestInit results = %ld\n", ret));
	return ret;
}

/*****************************************************************************
  RSADigestUpdate
  Pass in the next chunk of data to be checked.
  This is the middle of the three routines needed to do this.
  Note: Return value of 0 denotes failure, value of 1 denotes success.
*/

int32
RSADigestUpdate(DipirDigestContext *info, uchar *input, uint32 inputLen)
{
	int32 ret;

	ret = (*DipirRoutines->pdr_GenRSADigestUpdate)(info, input, inputLen);
	DBUG(("RSADigestUpdate results = %ld\n", ret));
	return ret;
}

/*****************************************************************************
  RSADigestFinal
  Pass in the signature.  It is compared to the signature calculated
  from all the data passed in to RSADigestUpdate.
  This is the last of the three routines needed to do this.
  Note: Return value of 0 denotes failure, value of 1 denotes success.
*/

int32
RSADigestFinal(DipirDigestContext *info, uchar *signature, uint32 sigLen)
{
	int32 ret;

	ret = (*DipirRoutines->pdr_GenRSADigestFinal)(info, signature, sigLen);
	DBUG(("RSADigestFinal results = %ld\n", ret));
	return ret;
}

