/*
 *	@(#) chan.microcard.c 96/06/27 1.34
 *	Copyright 1995, The 3DO Company
 *
 * Channel driver for Microcards.
 */

#include "kernel/types.h"
#include "hardware/cde.h"
#include "hardware/microcard.h"
#include "dipir/hw.microcard.h"
#include "setjmp.h"
#include "dipir.h"
#include "insysrom.h"

#define	DEFAULT_SPEED		CDE_MICRO_STAT_1MHz
#define	DEFAULT_ROM_SIZE	256
#define	MILLISEC		1

static int32 ReadMicrocard(DipirHWResource *dev, uint32 offset, uint32 len, void *buf);

extern const ChannelDriver MicrocardChannelDriver;

/*****************************************************************************
*/
	static void
InitMicrocard(void)
{
}

/*****************************************************************************
  Reset via Microcard status register.
*/
	static void
Reset(void)
{
	WritePowerBusRegister(M2_DEVICE_ID_CDE, CDE_MICRO_STATUS, 
		CDE_MICRO_STAT_RESET | DEFAULT_SPEED);
	WritePowerBusRegister(M2_DEVICE_ID_CDE, CDE_MICRO_STATUS, 
		DEFAULT_SPEED);
}

/*****************************************************************************
  Wait for a bit in the Microcard status register to become 1 or 0.
*/
	static Boolean
WaitStatus(uint32 statbit, uint32 val, uint32 timeout)
{
	uint32 status;
	TimerState tm;

	if (val) val = statbit;
	ResetTimer(&tm);
	while (ReadMilliTimer(&tm) <= timeout)
	{
		status = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_MICRO_STATUS);
		if ((status & statbit) == val)
			return TRUE;
	}
	return FALSE;
}

/*****************************************************************************
  Send a byte to a Microcard.
*/
	static Boolean
SendByte(uint32 b)
{
	uint32 reg;

	/* High order 2 bits of the byte specify the address to send it to. */
	switch (b & 0x300)
	{
	default:     /* Cannot happen, but keeps the compiler happy. */
	case 0x000:  reg = CDE_MICRO_RWS;  break;
	case 0x100:  reg = CDE_MICRO_WI;   break;
	case 0x200:  reg = CDE_MICRO_WOB;  break;
	case 0x300:  reg = CDE_MICRO_WO;   break;
	}
	WritePowerBusRegister(M2_DEVICE_ID_CDE, reg, b & 0xFF);
	return WaitStatus(CDE_MICRO_STAT_OUTFULL, 0, 100*MILLISEC);
}

/*****************************************************************************
  Read a byte from a Microcard.
*/
	static uint32
ReadByte(void)
{
	if (!WaitStatus(CDE_MICRO_STAT_INFULL, 1, 1000*MILLISEC))
		longjmp(dtmp->dt_JmpBuf, JMP_OFFLINE);
	return ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_MICRO_RWS);
}

/*****************************************************************************
  See if there is a card in a particular Microslot.
*/
	static void
ProbeSlot(Slot slot, Boolean hwOnly)
{
	uint32 stat;
	uint32 id;
	uint32 cardType;
	uint32 attn;
	uint32 flags;
	MicrocardHeader hdr;
	DipirHWResource *dev;
	char name[32];

	/* Select the slot. */
	if (!SendByte(MC_OUTSYNCH))
		return;
	if (!SendByte(MC_OUTSELECT | (slot << 2)))
		return;
	if (theBootGlobals->bg_DipirFlags & DF_HARD_RESET)
	{
		/* Boot time: do a full reset. */
		if (!SendByte(MC_OUTRESET))
			return;
		/* Reset the CDE Microslot register. */
		Reset();
		/* Select the slot again. */
		if (!SendByte(MC_OUTSYNCH))
			return;
		if (!SendByte(MC_OUTSELECT | (slot << 2)))
			return;
	}

	/* Try to read the ID byte. */
	if (!SendByte(MC_OUTIDENTIFY))
		return;

	/* Check the CARDLESS bit. */
	stat = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_MICRO_STATUS);
	if (stat & CDE_MICRO_STAT_CARDLESS)
	{
		PRINTF(("Microslot %d empty, stat %x\n", slot, stat));
		return;
	}

	/* Read the ID byte (response to OUTIDENTIFY). */
	id = ReadByte();
	cardType = MC_CARDTYPE(id);

	if (stat & CDE_MICRO_STAT_ATTN)
	{
		/* Read and clear the ATTN register. */
		if (!SendByte(MC_OUTATTENREG))
			return;
		attn = ReadByte();
	} else
	{
		attn = 0;
	}

	if (hwOnly)
		return;

	/* Create device name with proper card type & revision. */
	strcpy(name, "Microcard00\\00");
	SPutHex(cardType, name+9, 2);
	SPutHex(MC_CARDVERSION(id), name+12, 2);

	/* We have a card; create the HWResource. */
	flags = 0;
	if (attn & MC_NEWCARD)
		flags |= HWR_MEDIA_ACCESS;
	dev = UpdateHWResource(name, CHANNEL_MICROCARD, slot, flags, 
		&MicrocardChannelDriver, DEFAULT_ROM_SIZE,
		NULL, 0);

	if (cardType == MC_STORAGECARD && (id & MC_DOWNLOADABLE) == 0)
	{
		/* Special case for vanilla Storage Card: use config ROM. */
		dev->dev.hwr_ROMUserStart = SC_ROMOFFSET;
		dev->dev.hwr_ROMSize += SC_ROMOFFSET;
	}

	/* See if we have an optional MicrocardHeader. */
	if (ReadMicrocard(dev, 0, sizeof(MicrocardHeader), &hdr) == sizeof(hdr))
	{
		if (hdr.mh_Magic == MICROCARD_MAGIC)
		{
			/* ROM size is stored in the ROM header itself. */
			dev->dev.hwr_ROMSize = hdr.mh_RomSize;
			dev->dev.hwr_ROMUserStart = hdr.mh_RomUserOffset;
		}
	}
}

/*****************************************************************************
*/
	static void
ResetMicrocard(Boolean hwOnly)
{
	Slot slot;

	Reset();
	for (slot = 0;  slot < NUM_MICRO_SLOTS;  slot++)
		ProbeSlot(slot, hwOnly);
}

/*****************************************************************************
*/
	static void
ProbeMicrocard(void)
{
	ResetMicrocard(FALSE);
}

/*****************************************************************************
*/
	void
InitMicroslotForBoot(void)
{
	ResetMicrocard(TRUE);
}

/*****************************************************************************
*/
	static int32
ReadMicrocard(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	uint32 cmd;
	uint32 i;
	uint32 stat;
	uint32 id;
	uint8 *bbuf = buf;

	if (offset > dev->dev.hwr_ROMSize)
		return 0;
	i = dev->dev.hwr_ROMSize - offset;
	if (i < len)
		len = i;

	/* Send OUTSYNCH, OUTSELECT, OUTIDENTIFY */
	if (!SendByte(MC_OUTSYNCH))
		return -1;
	if (!SendByte(MC_OUTSELECT | (dev->dev.hwr_Slot << 2)))
		return -1;
	if (!SendByte(MC_OUTIDENTIFY))
		return -1;

	/* Check the CARDLESS bit. */
	stat = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_MICRO_STATUS);
	if (stat & CDE_MICRO_STAT_CARDLESS)
	{
		/* The card went away! */
		EPRINTF(("Microcard removed during dipir!\n"));
		longjmp(dtmp->dt_JmpBuf, JMP_OFFLINE);
	}

	/* Read the response to the OUTIDENTIFY packet. */
	id = ReadByte();
	if (id & MC_DOWNLOADABLE)
	{
		/* Card understands OUTDOWNLOAD command. */
		cmd = MC_OUTDOWNLOAD;
	} else if (MC_CARDTYPE(id) == MC_STORAGECARD)
	{
		/* Special case for MicroStorage card. */
		cmd = SC_READCONFIG;
	} else
	{
		/* We don't know how to read the ROM from this card. */
		return -1;
	}

	if (len == 0)
		return 0;

	if (!SendByte(cmd))
		return -1;
	if (!SendByte(MC_OUTBYTE | ((offset >> 16) & 0xFF)))
		return -1;
	if (!SendByte(MC_OUTBYTE | ((offset >> 8) & 0xFF)))
		return -1;
	if (!SendByte(MC_OUTBYTE | ((offset) & 0xFF)))
		return -1;
	if (!SendByte(MC_OUTBYTE | (((len-1) >> 8) & 0xFF)))
		return -1;
	if (!SendByte(MC_OUTBYTE | ((len-1) & 0xFF)))
		return -1;

	for (i = 0;  i < len;  i++)
	{
		*bbuf++ = ReadByte();
	}
	return len;
}

/*****************************************************************************
*/
	static int32
MapMicrocard(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	/* Can't map a Microcard. */
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	TOUCH(paddr);
	return -1;
}

/*****************************************************************************
*/
	static int32
UnmapMicrocard(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return -1;
}

/*****************************************************************************
*/
	static int32
DeviceControlMicrocard(DipirHWResource *dev, uint32 cmd)
{
	TOUCH(dev);
	switch (cmd)
	{
	case CHAN_BLOCK:
		/* Can't really block a Microslot */
		return 0;
	case CHAN_UNBLOCK:
		return 0;
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
ChannelControlMicrocard(uint32 cmd)
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
RetryLabelMicrocard(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

/*****************************************************************************
*/
const ChannelDriver MicrocardChannelDriver = 
{
	InitMicrocard,
	ProbeMicrocard,
	ReadMicrocard,
	MapMicrocard,
	UnmapMicrocard,
	DeviceControlMicrocard,
	ChannelControlMicrocard,
	RetryLabelMicrocard
};

