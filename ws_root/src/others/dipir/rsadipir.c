/*
 *	@(#) rsadipir.c 96/11/26 1.38
 *	Copyright 1994, The 3DO Company
 *
 *	code for RSA and MD5 checking
 *
 *	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
 */
#include "dipir.h"
#include "insysrom.h"

/* This struct is used by the kernel when it is calling GenRSADigestXXX.   */
/* Caller should always pass in a 128 byte buffer, broken down as follows: */
typedef struct GenRSAInfo {
	uint32	spares1[3];		/*  3 uint32s */
	uint32	lengthDigested;		/*  1 uint32s */
	uint32	spares2[6];		/*  6 uint32s */
	MD5_CTX	md5;			/* 22 uint32s */
} GenRSAInfo;


/* #define DUMP_RSA_DETAILS 1 */

#ifdef DUMP_RSA_DETAILS
#define	RSA_PRINTF(args) PRINTF (args)
#else
#define	RSA_PRINTF(args)
#endif


static const unsigned char PUBLIC_KEY_EXP[] = { 
	1, 0, 1
};

#define	DECLARE_KEY(name,size) \
	static const unsigned char name##_MOD[size+1]; \
	static const A_RSA_KEY name = { \
		{ (POINTER) name##_MOD, size+1 }, \
		{ (POINTER) PUBLIC_KEY_EXP, sizeof(PUBLIC_KEY_EXP) } }; \
	static const unsigned char name##_MOD[size+1] =  /* data follows */

DECLARE_KEY(a_thdo_key_64, 64)
{
  0x00,
  0xB1, 0x94, 0x62, 0xB0, 0x0D, 0x8D, 0x6E, 0x1E, 0xC9, 0x09, 0xAB, 0x38,
  0x5E, 0x06, 0xFE, 0x03, 0x4B, 0xFD, 0x28, 0x2E, 0x9F, 0xFD, 0xC5, 0x84,
  0x83, 0x8C, 0x15, 0xF1, 0x25, 0x93, 0xDD, 0x1E, 0x3A, 0x8B, 0x56, 0x26,
  0xF1, 0xB9, 0xD0, 0xED, 0x0C, 0x38, 0x4E, 0xF6, 0xC5, 0xD1, 0x45, 0x12,
  0xBD, 0x72, 0xDD, 0xB8, 0x5B, 0x44, 0x08, 0x0E, 0x04, 0x72, 0xC0, 0x3D,
  0x0A, 0xFC, 0x4C, 0x97
};

DECLARE_KEY(a_app_key_64, 64)
{
  0x00,
  0xBC, 0x0B, 0x19, 0x90, 0x86, 0xC7, 0xF2, 0x6C, 0xBC, 0x9D, 0x50, 0xF4,
  0x04, 0x94, 0x4D, 0xB4, 0x78, 0x9F, 0xCB, 0xFC, 0xF7, 0xAD, 0x8D, 0xBC,
  0x21, 0x20, 0x89, 0x8A, 0xBE, 0xAA, 0xF3, 0x11, 0xEE, 0xA2, 0x02, 0x29,
  0x03, 0x56, 0x08, 0x84, 0x1F, 0xA4, 0x10, 0x73, 0xAB, 0xBD, 0x5D, 0x37,
  0x50, 0x0C, 0x60, 0xB5, 0x3B, 0xFB, 0x46, 0x60, 0x57, 0x40, 0x38, 0x1B,
  0x72, 0xC9, 0xDB, 0x71
};

/* FIXME: Get real key for this... */
DECLARE_KEY(a_alt_key_64, 64)
{
  0x00,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff
};

DECLARE_KEY(a_key_128, 128)
{
  0x00,
  0xD9, 0x21, 0x10, 0x53, 0x0B, 0x3E, 0x3B, 0xCA, 0x02, 0x27, 0x86, 0x99, 
  0x0C, 0x7B, 0x68, 0xD7, 0x26, 0x14, 0xA4, 0x4F, 0xF1, 0x0A, 0x15, 0x29,
  0x22, 0x67, 0xA1, 0x49, 0x13, 0x56, 0x8D, 0x99, 0x81, 0x1A, 0xF1, 0xD6, 
  0xE2, 0xCF, 0x53, 0x64, 0x61, 0x42, 0x6A, 0x0F, 0x02, 0x2E, 0x68, 0x8E,
  0xD6, 0x1F, 0xD2, 0x47, 0x92, 0x74, 0xF8, 0xF6, 0xFF, 0x3F, 0xA9, 0xBE, 
  0x1B, 0xB7, 0xA1, 0x22, 0x1D, 0x7E, 0x7F, 0x45, 0x99, 0xF7, 0x11, 0x37,
  0x9F, 0x40, 0x2D, 0x91, 0x40, 0x43, 0xB3, 0xD9, 0x06, 0xA0, 0xEC, 0x72, 
  0x59, 0x2D, 0xED, 0x84, 0xB5, 0x96, 0xE0, 0x17, 0x9B, 0x56, 0xBA, 0x14,
  0x54, 0x64, 0x79, 0xD2, 0x73, 0xDD, 0x0E, 0xAE, 0x60, 0x81, 0x89, 0x03, 
  0xBF, 0x1A, 0xE2, 0x5F, 0x3A, 0x62, 0x8A, 0xDB, 0xF1, 0x23, 0xE1, 0x05,
  0x8C, 0xF3, 0x3F, 0xD5, 0xC8, 0xAA, 0xF1, 0xB7
};

DECLARE_KEY(a_demo_key_64, 64)
{
  0x00,
  0xc0, 0x76, 0x47, 0x97, 0xb8, 0xbe, 0xc8, 0x97, 0x2a, 0x0e, 0xd8, 0xc9,
  0x0a, 0x8c, 0x33, 0x4d, 0xd0, 0x49, 0xad, 0xd0, 0x22, 0x2c, 0x09, 0xd2,
  0x0b, 0xe0, 0xa7, 0x9e, 0x33, 0x89, 0x10, 0xbc, 0xae, 0x42, 0x20, 0x60,
  0x90, 0x6a, 0xe0, 0x22, 0x1d, 0xe3, 0xf3, 0xfc, 0x74, 0x7c, 0xcf, 0x98,
  0xae, 0xcc, 0x85, 0xd6, 0xed, 0xc5, 0x2d, 0x93, 0xd5, 0xb7, 0x39, 0x67,
  0x76, 0x16, 0x05, 0x25
};

DECLARE_KEY(a_demo_key_128, 128)
{
  0x00,
  0xD5, 0x68, 0x6E, 0xEB, 0x50, 0x26, 0xD5, 0xE5, 0xB7, 0x0E, 0x8C, 0xFB, 
  0x82, 0xE6, 0xD4, 0x2E, 0x94, 0x12, 0xEF, 0xA5, 0x32, 0xBB, 0xC3, 0x65, 
  0xF7, 0x1B, 0x9E, 0xB4, 0x6C, 0xAF, 0xCD, 0xEA, 0x47, 0x0D, 0x4F, 0xC2, 
  0x16, 0x7F, 0xD9, 0x97, 0x65, 0x92, 0x0D, 0x65, 0xCA, 0x26, 0xE1, 0xDC, 
  0x61, 0x7C, 0x74, 0xC5, 0x4B, 0x60, 0x10, 0x85, 0x6A, 0x13, 0x86, 0xFB, 
  0xB2, 0x98, 0x8B, 0xD9, 0x19, 0x61, 0xB4, 0x7A, 0x5B, 0x87, 0x97, 0xAF, 
  0x60, 0x9E, 0xC9, 0x94, 0x28, 0x4D, 0x50, 0x2A, 0xD8, 0x05, 0xD2, 0x25,
  0xD3, 0x93, 0x74, 0x1D, 0xBA, 0x57, 0xFD, 0xB3, 0x54, 0xE2, 0x4F, 0x75, 
  0xAB, 0x8C, 0xE9, 0x6F, 0xF5, 0x68, 0xA9, 0xAA, 0x4C, 0xB7, 0x5A, 0x79, 
  0xD6, 0xD4, 0xE4, 0x16, 0xA2, 0xFE, 0x6F, 0x93, 0x4D, 0x59, 0x1A, 0x06, 
  0xF6, 0x08, 0xED, 0x33, 0x97, 0xA0, 0x8B, 0x73
};


const unsigned int PKCS1_64[] = 
{
	0x0001ffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xff003020,
	0x300c0608, 0x2a864886, 0xf70d0205, 0x05000410
};

const unsigned int PKCS1_128[] = 
{
	0x0001ffff, 0xffffffff, 0xffffffff, 0xffffffff,
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xff003020,
	0x300c0608, 0x2a864886, 0xf70d0205, 0x05000410
};


extern int32 aRSAx(uchar *pOutput, uchar *pInput, uchar *pMod,
		    uint32 exp, uint32 sigSize);
extern void InitDigest(MD5_CTX *d, Boolean usermode);
extern void UpdateDigest(MD5_CTX *d, void *buffer, uint32 bufLen, Boolean usermode);
extern void FinalDigest(MD5_CTX *d, void *result, Boolean usermode);

/*****************************************************************************
 NewRSAFinal
 Check a signature, with a caller-supplied key and digest context.
*/
static int 
NewRSAFinal(A_RSA_KEY *key, uchar *signature, int signatureLen, uchar *d)
{
	int ret;
	unsigned int partOut[(SIG_128_LEN+sizeof(int)-1)/sizeof(int)];
	int32 *mod_point;
	uint8 *exp_point;
	int exp;
	int i;

	if (d == 0) 
		d = dtmp->dt_DipirDigest.digest;

	if (signatureLen != 64 && signatureLen != 128)
	{
		return 0;
	}

	mod_point = (int32 *)((POINTER)key->modulus.data);
	exp_point = (uint8 *)((POINTER)key->exponent.data);
	exp = 0;

	for (i = 0; i < key->exponent.len ; i++ )
	{
		exp <<= 8;
		exp += *exp_point++;
	}

	ret = aRSAx((uchar *)partOut, (uchar *)signature, (uchar *)mod_point,
		    exp, signatureLen);
	if (ret != 0)
	{
		return 0;
	}

	/* first compare the constant part */
	if (signatureLen == 64)
		ret = memcmp(partOut, PKCS1_64, signatureLen-16);
	else
		ret = memcmp(partOut, PKCS1_128, signatureLen-16);
	if (ret == 0)
	{	/* passed first check */
		/* now compare the 16 byte digest */
		ret = memcmp(&partOut[(signatureLen-sizeof(Digest))/sizeof(int)],
				d, sizeof(Digest));
		if (ret == 0)
		{
			return 1;
		}
	}

#if 0
	{
		int *my_d = (int *)d; 

		RSA_PRINTF(("BOGUS!\n"));
		printf("partOut:\n");
		for (i = 0;  i < signatureLen/4;  i++)
			printf("%8x ", partOut[i]);
		printf("\n");

		printf("d: %8x %8x %8x %8x\n",
			my_d[0], my_d[1], my_d[2], my_d[3]);
	}
#endif
	return 0;
}

/*****************************************************************************
 ContextRSAFinal
 Check a signature, with a caller-specified key and digest context.
 If this is the unencrypted dipir, also check the demo key.
*/
static int
ContextRSAFinal(uchar *signature, KeyID key, uchar *d, Boolean usermode)
{
	int signatureLen;
	int ok = FALSE;
	int ok2 = FALSE;
	int dok = FALSE;

	TOUCH(usermode);
	signatureLen = KeyLen(key);
	switch (key)
	{
	case KEY_THDO_64:
		/* Try the demo 64 byte key (if in development mode) */
		if (theBootGlobals->bg_DipirControl & DC_DEMOKEY)
		{
		dok = NewRSAFinal(&a_demo_key_64, signature, signatureLen, d);
		if (dok) break;
		}
		/* Now try the real OS 64 byte key. */
		ok = NewRSAFinal(&a_thdo_key_64, signature, signatureLen, d);
		if (ok) break;
		/* Last chance: try the alternate 64 byte key. */
		ok2 = NewRSAFinal(&a_alt_key_64, signature, signatureLen, d);
		break;

	case KEY_APP_64:
		/* Try the demo 64 byte key (if in development mode) */
		if (theBootGlobals->bg_DipirControl & DC_DEMOKEY)
		{
		dok = NewRSAFinal(&a_demo_key_64, signature, signatureLen, d);
		if (dok) break;
		}
		/* Now try the real App 64 byte key. */
		ok = NewRSAFinal(&a_app_key_64, signature, signatureLen, d);
		if (ok) break;
		/* Last chance: try the alternate 64 byte key. */
		ok2 = NewRSAFinal(&a_alt_key_64, signature, signatureLen, d);
		break;
	case KEY_128:
		/* Try the demo 128 byte key (if in development mode) */
		if (theBootGlobals->bg_DipirControl & DC_DEMOKEY)
		{
		dok = NewRSAFinal(&a_demo_key_128, signature, signatureLen, d);
		if (dok) break;
		}
		/* Now try the real 128 byte key. */
		ok = NewRSAFinal(&a_key_128, signature, signatureLen, d);
		break;
#ifdef DEBUG
	default:
		PRINTF(("DIPIR: Unknown key %x!\n", key));
#endif
	}
#ifdef DEBUG
	if ((theBootGlobals->bg_DipirControl & DC_DEMOKEY) && !usermode)
	{
		char *msg;
		if (dok)	msg = "ok (demo key)";
		else if (ok2)	msg = "ok (real alt key)";
		else if (ok)	msg = "ok (real key)";
		else		msg = "** FAILED **";
		PRINTF(("DIPIR: RSA %s\n", msg));
	}
#endif

	if (dok || ok2 || ok) return 1;
	if (theBootGlobals->bg_DipirControl & DC_NOKEY) return 1;
	return 0;
}

/*****************************************************************************
 RSAFinal
 Check a signature, with a caller-specified key, using the dipir context.
*/
int
RSAFinal(uchar *signature, KeyID key)
{
	return ContextRSAFinal(signature, key, 0, FALSE);
}

/*****************************************************************************
 RSAFinalWithKey
 Check a signature, with a caller-supplied key, using the dipir context.
*/
int
RSAFinalWithKey(A_RSA_KEY *key, unsigned char *signature, int signatureLen)
{
	return NewRSAFinal(key, signature, signatureLen, 0);
}

/*****************************************************************************
  RSAInit
  Initialize keys.
*/
int 
RSAInit(Boolean indipir)
{
	static int initdone = 0;

	TOUCH(indipir);
	if (initdone)
		return 0;	/* already initialized */

	/* Would be nice if PUBLIC_APPKEY_64_MOD and PUBLIC_KEY_128_MOD
	 * could come from a RomTag in the system ROM. */

	initdone = 1; /* Keys have been initialized */
	return 0;
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name GenRSADigestInit
|||	Initialize a DipirDigestContext structure
|||
|||	  Synopsis
|||
|||	    int32 GenRSADigestInit(DipirDigestContext *info);
|||
|||	  Description
|||
|||	    Initializes the DipirDigestContext structure pointed to
|||	    by the info argument.  This prepares the structure for
|||	    subsequent calls to GenRSADigestUpdate() and 
|||	    GenRSADigestFinal().  Returns 1 on success, or 0 if an 
|||	    error occurs.
|||
|||	  Arguments
|||
|||	    info
|||	        Pointer to a DipirDigestContext structure.
|||
|||	  Return Value
|||
|||	    Returns 1 on success, or 0 if an error occurs.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
|||	  See Also
|||
|||	    GenRSADigestUpdate(), GenRSADigestFinal()
|||
**/

/*****************************************************************************
  GenRSADigestInit
  General purpose MD5/RSA routine(s) that allows MD5 data to be passed in
  in chunks rather than as a single buffer.  Then final result is signed.
  This is the first of the three routines needed to do this.
*/
int32 
GenRSADigestInit(DipirDigestContext *ainfo)
{
	int32 status;
	GenRSAInfo *info = (GenRSAInfo *) ainfo;

	status = RSAInit(FALSE);
	if (status != 0)
	     return 0;

	/* Clear out the GenRSA info struct */
	memset(info, 0, sizeof(GenRSAInfo));

	InitDigest(&info->md5, TRUE);
	return 1;
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name GenRSADigestUpdate
|||	Add more data to an RSA digest.
|||
|||	  Synopsis
|||
|||	    int32 GenRSADigestUpdate(DipirDigestContext *info, uchar *buffer, uint32 len);
|||
|||	  Description
|||
|||	    Adds the data in the specified buffer to the digest being
|||	    computed in the info argument.  Info is a pointer to a 
|||	    structure previously initialized by GenRSADigestInit().
|||	    Any number of calls to GenRSADigestUpdate() may be made
|||	    after a call to GenRSADigestInit() and before a call to
|||	    GenRSADigestFinal().  Returns 1 on success, or 0 if an 
|||	    error occurs.
|||
|||	  Arguments
|||
|||	    info
|||	        Pointer to a DipirDigestContext structure previously
|||	        initialized with GenRSADigestInit().
|||
|||	    buffer
|||	        Pointer to a buffer of data to be digested.
|||
|||	    len
|||	        Length of the buffer, in bytes.
|||
|||	  Return Value
|||
|||	    Returns 1 on success, or 0 if an error occurs.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
|||	  See Also
|||
|||	    GenRSADigestInit(), GenRSADigestFinal()
|||
**/

/*****************************************************************************
  GenRSADigestUpdate
  General purpose MD5/RSA routine(s) that allows MD5 data to be passed in
  in chunks rather than as a single buffer.  Then final result is signed.
  This is the middle of the three routines needed to do this.
*/
int32 
GenRSADigestUpdate(DipirDigestContext *ainfo, uchar *input, uint32 inputLen)
{
	GenRSAInfo *info = (GenRSAInfo *) ainfo;

#ifdef DEBUG
	info->lengthDigested += inputLen;
#endif
	UpdateDigest(&info->md5, input, inputLen, TRUE);
	return 1;
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name GenRSADigestFinal
|||	Check the signature on an RSA digest.
|||
|||	  Synopsis
|||
|||	    int32 GenRSADigestFinal(DipirDigestContext *info, uchar *signature, uint32 signatureLen);
|||
|||	  Description
|||
|||	    Checks the final calculated RSA digest against a supplied
|||	    signature.  Returns 1 if the signature is correct, or 0
|||	    if it is not.
|||
|||	  Arguments
|||
|||	    info
|||	        Pointer to a DipirDigestContext structure previously
|||	        initialied with GenRSADigestInit().
|||
|||	    signature
|||	        Pointer to a signature buffer.
|||
|||	    signatureLen
|||	        Length of the signature buffer, in bytes.
|||	        Currently, this must be 128.
|||
|||	  Return Value
|||
|||	    Returns 1 on success, or 0 if an error occurs.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
|||	  See Also
|||
|||	    GenRSADigestInit(), GenRSADigestUpdate()
|||
**/

/*****************************************************************************
  GenRSADigestFinal
  General purpose MD5/RSA routine(s) that allows MD5 data to be passed in
  in chunks rather than as a single buffer.  Then final result is signed.
  This is the last of the three routines needed to do this.
*/
int32 
GenRSADigestFinal(DipirDigestContext *ainfo, uchar *signature, uint32 signatureLen)
{
	GenRSAInfo *info = (GenRSAInfo *) ainfo;
	uchar digest[16];

	FinalDigest(&info->md5, digest, TRUE);
	if (signatureLen != 128)
		return 0;
	return ContextRSAFinal(signature, KEY_128, digest, TRUE);
}
