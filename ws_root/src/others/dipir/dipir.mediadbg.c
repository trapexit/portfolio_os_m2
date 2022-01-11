/*
 *	@(#) dipir.mediadbg.c 96/07/02 1.8
 *	Copyright 1996 The 3DO Company
 *
 * Device-dipir for testing RomApp (non-3DO) media.
 * The purpose of this dipir is merely to debug WData stuff.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.cd.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

#undef	DUMP_RAW_DATA 

const DipirRoutines *dipr;


/*****************************************************************************
*/
typedef struct WCallbackInfo
{
	uint32 wc_BitSkip;
	uint32 wc_EnergyLo;
	uint32 wc_EnergyHi;
#ifdef DUMP_RAW_DATA
	uint8 *wc_RawPtr;
	uint8 *wc_RawEnd;
#endif
} WCallbackInfo;

static int32 
WDataCallback(DDDFile *fd, void *callbackArg, void *wBuf, uint32 bufSize)
{
	WCallbackInfo *wc = callbackArg;
	uint8 *buf = wBuf;
	uint32 cosHi, cosLo;
	uint32 sinHi, sinLo;
	uint32 phase;
	uint32 bitx;
	uint32 bytex;
	uint8 tbyte;
	uint8 tbit;

	TOUCH(fd);
#ifdef DUMP_RAW_DATA
	{
		uint32 n;
		n = wc->wc_RawEnd - wc->wc_RawPtr;
		if (n > bufSize) n = bufSize;
		memcpy(wc->wc_RawPtr, buf, n);
		wc->wc_RawPtr += n;
	}
#endif
	cosHi = sinHi = cosLo = sinLo = 0;
	phase = 0;
	bytex = 0;
	bitx = 8;
	tbyte = 0; /* Not needed, but keeps the compiler happy. */
	/* Loop thru all bits in the sample. */
	for (;;)
	{
		/* Get the next bit. */
		if (bitx >= 8)
		{
			/* No more bits in this byte; get next byte. */
			if (bytex >= bufSize)
				break;
			tbyte = buf[bytex++];
			bitx = 0;
		}
		tbit = (tbyte >> (7-bitx)) & 1;
		bitx += wc->wc_BitSkip;

		if (tbit)
		{
			switch (phase)
			{
			case 0:  cosHi++;  cosLo++;  break;
			case 1:  sinHi++;            break;
			case 2:  cosHi--;  sinLo++;  break;
			case 3:  sinHi--;            break;
			case 4:  cosHi++;  cosLo--;  break;
			case 5:  sinHi++;            break;
			case 6:  cosHi--;  sinLo--;  break;
			case 7:  sinHi--;            break;
			}
		}
		if (++phase >= 8) phase = 0;
	}
	/* Add this sample's contribution to the total. */
	wc->wc_EnergyHi += (sinHi * sinHi) + (cosHi * cosHi);
	wc->wc_EnergyLo += (sinLo * sinLo) + (cosLo * cosLo);
	return 0;
}

#define	SAMPLE_SIZE	512
#define	NUM_SAMPLES	32

#ifdef DUMP_RAW_DATA
uint8 RawData[4096];
#endif

static void
CheckWData(DDDFile *fd, uint32 block, uint32 speed, uint32 bitSkip)
{
	WCallbackInfo wc;
	int32 err;

	wc.wc_EnergyLo = wc.wc_EnergyHi = 0;
	wc.wc_BitSkip = bitSkip;
#ifdef DUMP_RAW_DATA
	wc.wc_RawPtr = RawData;
	wc.wc_RawEnd = RawData + sizeof(RawData);
#endif
	err = DeviceGetWData(fd, block, speed,
			SAMPLE_SIZE, SAMPLE_SIZE * NUM_SAMPLES,
			WDataCallback, &wc);
	if (err < 0)
		PRINTF(("WData error %x\n", err));
	else
		PRINTF(("%d\t%d\t%d\t%d\t%d\n",
			block, speed, bitSkip, wc.wc_EnergyLo, wc.wc_EnergyHi));
}

/*****************************************************************************
*/
DIPIR_RETURN 
Validate(DDDFile *fd, uint32 dipirID, uint32 dddID)
{
	uint32 i;

	TOUCH(dipirID);
	TOUCH(dddID);

#define	CHECK(blk,speed,skip) \
	for (i = 0;  i < 15;  i++) CheckWData(fd, blk,       speed,skip); \

#ifdef DUMP_RAW_DATA
	PRINTF(("RawData at %x\n", RawData));
#endif
	CHECK( 118125, 2, 2);	/* 1000 */
	CHECK(  28125, 2, 2);	/* 500 */
	CHECK(  73875, 2, 2);	/* 250 */
	CHECK( 253875, 2, 2);	/* 126 */

	if (fd) for (;;) ; /* Hang here, so we can see the messages. */
	return DIPIR_RETURN_OK;
}

/*****************************************************************************
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	TOUCH(arg);
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	PRINTF(("MEDIA_DEBUG newdipir entered!\n"));
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd, dipirID, dddID);
	}
	return DIPIR_RETURN_TROJAN;
}
