/*
 *	@(#) diplib.wdata.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Check 3DO wdata on a CD-ROM.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.cd.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
*/
/* Defines for WData */
#define	NUM_SAMPLES	256		/* Number of samples */ 
#define	SAMPLE_SIZE	64		/* Bytes in each sample */
#define	WTHRESHOLD	10		/* Threshold = 10.0 */

typedef struct WCallbackInfo
{
	uint32 wc_BitSkip;
	uint32 wc_EnergyLo;
	uint32 wc_EnergyHi;
} WCallbackInfo;

/*****************************************************************************
*/
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

/*****************************************************************************
*/
static DIPIR_RETURN
Check3DODiscOnce(DDDFile *fd, uint32 speed)
{
	Boolean goodDisc;
	WCallbackInfo wc;
	int32 err;

	wc.wc_EnergyLo = wc.wc_EnergyHi = 0;
	wc.wc_BitSkip = 4 / speed;
	err = DeviceGetWData(fd, 0, speed,
			SAMPLE_SIZE, SAMPLE_SIZE * NUM_SAMPLES,
			WDataCallback, &wc);
	if (err == DDD_OPEN_IGNORE)
		/* Driver says to skip the WData test. */
		return DIPIR_RETURN_OK;
	if (err < 0)
		return DIPIR_RETURN_TROJAN;
	PRINTF(("WData lo %x, hi %x\n", wc.wc_EnergyLo, wc.wc_EnergyHi));

	goodDisc = ((wc.wc_EnergyHi / wc.wc_EnergyLo) > WTHRESHOLD);
	if (!goodDisc)
		return DIPIR_RETURN_TROJAN;
	return DIPIR_RETURN_OK;
}

/*****************************************************************************
*/
static DIPIR_RETURN
Check3DODiscMultiple(DDDFile *fd, uint32 speed, uint32 numTries)
{
	uint32 try;
	DIPIR_RETURN ret;

	for (try = 0;  try < numTries;  try++)
	{
		ret = Check3DODiscOnce(fd, speed);
		if (ret == DIPIR_RETURN_OK)
			return DIPIR_RETURN_OK;
	}
	return DIPIR_RETURN_TROJAN;
}

/*****************************************************************************
*/
DIPIR_RETURN
Check3DODisc(DDDFile *fd, DiscInfo *cdinfo, uint32 num4xTries, uint32 num2xTries)
{
	uint32 speed;
	DIPIR_RETURN ret;

	/* 
	 * Because of a bug in the drive's DSP, we sometimes get bad data.
	 * If it appears to fail, give it a few more chances.
	 * If it is really a bad disc, it will never pass.
	 * First try it at full speed.
	 */
	speed = cdinfo->di_DefaultSpeed;
	if (speed > 4)
		speed = 4;
	ret = Check3DODiscMultiple(fd, speed, num4xTries);
	if (ret == DIPIR_RETURN_OK)
		return DIPIR_RETURN_OK;

	/* Now try at 2x. */
	if (speed > 2)
		speed = 2;
	ret = Check3DODiscMultiple(fd, speed, num2xTries);
	if (ret == DIPIR_RETURN_OK)
		return DIPIR_RETURN_OK;
	return DIPIR_RETURN_TROJAN;
}
