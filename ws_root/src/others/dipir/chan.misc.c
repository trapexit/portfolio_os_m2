/*
 *	@(#) chan.misc.c 96/07/22 1.20
 *	Copyright 1995, The 3DO Company
 *
 * Channel driver for miscellaneous hardware 
 * (with no ROM nor device-dipir):
 *	Graphics
 *	Audio
 *	MPEG
 *	Control port
 */

#include "kernel/types.h"
#include "dipir/hw.audio.h"
#include "dipir/hw.graphics.h"
#include "dipir/hw.cport.h"
#include "dipir.h"
#include "insysrom.h"

extern const ChannelDriver MiscChannelDriver;

/*****************************************************************************
*/
	static void
InitMiscChan(void)
{
	return;
}

/*****************************************************************************
*/
	static void
ProbeAudio(void)
{
	HWResource_Audio audio;

	/* Determine audio configuration */
	memset(&audio, 0, sizeof(audio));
	switch (theBootGlobals->bg_AudioConfig)
	{
	case BG_AUDIO_CONFIG_ASAHI:
		audio.audio_SharedConfig = 
			MakeAudioConfig(1, B20MP_AUDWS_LENGTH, 
				B20_BITS_PER_FRAME-1, 
				B20MP_BIT_CLOCK_DIVIDER-1);
		audio.audio_InputConfig = 
			MakeAudinConfig(B20MP_DATA_INPUT_EDGE, 1, 1, 
				B20MP_SYNC_TO_DATA_OFFSET, 
				B20_SUB_FRAME_LENGTH);
		audio.audio_OutputConfig = 
			MakeAudoutConfig(B20MP_DATA_OUTPUT_EDGE, 1, 0, 
				B20MP_SYNC_TO_DATA_OFFSET, 
				B20_SUB_FRAME_LENGTH);
		break;
	case BG_AUDIO_CONFIG_CS4216:
		audio.audio_SharedConfig = 
			MakeAudioConfig(1, B20DC_AUDWS_LENGTH, 
				B20_BITS_PER_FRAME-1, 
				B20DC_BIT_CLOCK_DIVIDER-1);
		audio.audio_InputConfig =
			MakeAudinConfig(B20DC_DATA_INPUT_EDGE, 1, 1, 
				B20DC_SYNC_TO_DATA_OFFSET, 
				B20_SUB_FRAME_LENGTH);
		audio.audio_OutputConfig = 
			MakeAudoutConfig(B20DC_DATA_OUTPUT_EDGE, 1, 0, 
				B20DC_SYNC_TO_DATA_OFFSET, 
				B20_SUB_FRAME_LENGTH );
		break;
	default:
		EPRINTF(("Invalid audio config %x\n", 
			theBootGlobals->bg_AudioConfig));
		HardBoot();
	}

	audio.audio_Addr = 0;
	(void) UpdateHWResource("AUDIO", CHANNEL_ONBOARD, (Slot)1, 
		HWR_NODIPIR, &MiscChannelDriver, 0,
		&audio, sizeof(audio));
}

/*****************************************************************************
*/
	static void
ProbeGraf(void)
{
	HWResource_Graf graf;

	graf.graf_Dummy = 42;
	(void) UpdateHWResource("BDA_VDU\\2.1", CHANNEL_ONBOARD, (Slot)2, 
		HWR_NODIPIR, &MiscChannelDriver, 0,
		&graf, sizeof(graf));
}

/*****************************************************************************
*/
	static void
ProbeMPEG(void)
{
	(void) UpdateHWResource("MPEG1", CHANNEL_ONBOARD, (Slot)3, 
		HWR_NODIPIR, &MiscChannelDriver, 0,
		NULL, 0);
}

/*****************************************************************************
*/
	static void
ProbeControlPort(void)
{
	HWResource_ControlPort cport;

	memset(&cport, 0, sizeof(cport));
	if (theBootGlobals->bg_VideoMode == BG_VIDEO_MODE_PAL)
	{
		/* Long clock 7: 8 scan lines high, 8 scan lines low */
		/* Short clock: 3 ms / 33.9 ns - 1 = 87.49, make it 87 */
		cport.cport_Config1     = 0x00070057;
		cport.cport_Config1Fast = 0x00070016;
	} else /* NTSC */
	{
		/* Long clock 7: 8 scan lines high, 8 scan lines low */
		/* Short clock: 3 ms / 40.7 ns - 1 = 72.71, make it 73 */
		cport.cport_Config1     = 0x00070049;
		cport.cport_Config1Fast = 0x00070012;
	}
	(void) UpdateHWResource("CONTROLPORT", CHANNEL_ONBOARD, (Slot)4, 
		HWR_NODIPIR, &MiscChannelDriver, 0,
		&cport, sizeof(cport));
}

/*****************************************************************************
*/
	static void
ProbeMiscChan(void)
{
	ProbeAudio();
	ProbeGraf();
	ProbeMPEG();
	ProbeControlPort();
}

/*****************************************************************************
*/
	static int32
ReadMiscChan(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	TOUCH(buf);
	return -1;
}

/*****************************************************************************
*/
	static int32
MapMiscChan(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	TOUCH(paddr);
	return -1;
}

/*****************************************************************************
*/
	static int32
UnmapMiscChan(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return -1;
}

/*****************************************************************************
*/
	static int32
DeviceControlMiscChan(DipirHWResource *dev, uint32 cmd)
{
	TOUCH(dev);
	switch (cmd)
	{
	case CHAN_BLOCK:
	case CHAN_UNBLOCK:
		return 0;
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
ChannelControlMiscChan(uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_DISALLOW_UNBLOCK:
		return 0;
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
RetryLabelMiscChan(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

/*****************************************************************************
*/
const ChannelDriver MiscChannelDriver =
{
	InitMiscChan,
	ProbeMiscChan,
	ReadMiscChan,
	MapMiscChan,
	UnmapMiscChan,
	DeviceControlMiscChan,
	ChannelControlMiscChan,
	RetryLabelMiscChan
};

