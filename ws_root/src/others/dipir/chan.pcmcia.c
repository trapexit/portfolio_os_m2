#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/cde.h"
#include "hardware/visa.h"
#include "dipir/hw.pcmcia.h"
#ifdef BUILD_PC16550
#include "hardware/ns16550.h"
#include "dipir/hw.pc16550.h"
#endif /* BUILD_PC16550 */
#include "dipir.h"
#include "insysrom.h"

#define	HWR_EVENBYTES	HWR_CHANSPEC1	/* Read only even bytes */
#define	HWR_COMMON	HWR_CHANSPEC2	/* Read from common space */

/* System ROM */
#define	ROM_START	(SYSROMIMAGE)
#define	ROM_SIZE	((uint32)theBootGlobals->bg_SystemROM.mr_Size)
#define ROM_FSOFFSET	((uint32)theBootGlobals->bg_ROMVolumeLabel - ROM_START)

#define	FIRST_PCMCIA_SLOT	5
#define	CARDROMSIZE		(64*1024*1024)
#ifdef BUILD_PC16550
#define	PC16550_SLOT		((Slot)7)
#endif /* BUILD_PC16550 */

/*
 * Hardware registers and bits, per slot.
 */
const uint32 _CycleTimeReg[] = {
	CDE_DEV5_CYCLE_TIME, CDE_DEV6_CYCLE_TIME, CDE_DEV7_CYCLE_TIME
};
const uint32 _DevConfReg[] = {
	CDE_DEV5_CONF,      CDE_DEV6_CONF,      CDE_DEV7_CONF
};
const uint32 _VisaConfReg[] = {
	CDE_DEV5_VISA_CONF, CDE_DEV6_VISA_CONF, CDE_DEV7_VISA_CONF
};
const uint32 _DevSetupReg[] = {
	CDE_DEV5_SETUP,     CDE_DEV6_SETUP,     CDE_DEV7_SETUP
};

const uint32 _DipirBit[] = {
	CDE_DEV5_DIPIR,     CDE_DEV6_DIPIR,     CDE_DEV7_DIPIR
};
const uint32 _BlockBit[] = {
	CDE_DEV5_BLOCK,     CDE_DEV6_BLOCK,     CDE_DEV7_BLOCK
};
const uint32 _DetectBit[] = {
	CDE_DEV5_PRESENT,   CDE_DEV6_PRESENT,   CDE_DEV7_PRESENT
};
const uint32 _WriteProtBit[] = {
	CDE_DEV5_WRTPROT,   CDE_DEV6_WRTPROT,   CDE_DEV7_WRTPROT
};
const uint32 _VisaDIPBit[] = {
	CDE_DEV5_VISA_DIP,  CDE_DEV6_VISA_DIP,  CDE_DEV7_VISA_DIP
};
const uint32 _VisaDisBit[] = {
	CDE_DEV5_VISA_DIS,  CDE_DEV6_VISA_DIS,  CDE_DEV7_VISA_DIS
};
const uint32 _SlotAddr[] = {
	BIO_DEV_5,          BIO_DEV_6,          BIO_DEV_7
};


#define	CycleTimeReg(slot) _CycleTimeReg[(slot) - FIRST_PCMCIA_SLOT]
#define	DevConfReg(slot)  _DevConfReg [(slot) - FIRST_PCMCIA_SLOT]
#define	VisaConfReg(slot) _VisaConfReg[(slot) - FIRST_PCMCIA_SLOT]
#define	DevSetupReg(slot) _DevSetupReg[(slot) - FIRST_PCMCIA_SLOT]
#define	DipirBit(slot)	  _DipirBit   [(slot) - FIRST_PCMCIA_SLOT]
#define	BlockBit(slot)	  _BlockBit   [(slot) - FIRST_PCMCIA_SLOT]
#define	DetectBit(slot)	  _DetectBit  [(slot) - FIRST_PCMCIA_SLOT]
#define	WriteProtBit(slot) _WriteProtBit[(slot) - FIRST_PCMCIA_SLOT]
#define VisaDIPBit(slot)  _VisaDIPBit [(slot) - FIRST_PCMCIA_SLOT]
#define	VisaDisBit(slot)  _VisaDisBit [(slot) - FIRST_PCMCIA_SLOT]
#define	SlotAddr(slot)    _SlotAddr   [(slot) - FIRST_PCMCIA_SLOT]

extern const ChannelDriver PCMCIAChannelDriver;

#define	ClearBlockBits(slot) \
	ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_BBLOCK, \
		DipirBit(slot) | BlockBit(slot))

/*****************************************************************************
 Initialize PCMCIA channel driver.
*/
	static void
InitPCMCIA(void)
{
	return;
}

/*****************************************************************************
 Set a PCMCIA slot to access PCMCIA Attribute space.
*/
	static void
SetAttributeSpace(Slot slot)
{
	SetPowerBusBits(M2_DEVICE_ID_CDE, DevConfReg(slot), CDE_ATTR_SPACE);
}

/*****************************************************************************
 Set a PCMCIA slot to access PCMCIA Common space.
*/
	static void
SetCommonSpace(Slot slot)
{
	ClearPowerBusBits(M2_DEVICE_ID_CDE, DevConfReg(slot), CDE_ATTR_SPACE);
}

/*****************************************************************************
 Reset a VISA card.
*/
	static void
ResetVisa(Slot slot)
{
	/* To reset VISA, set to Common space and do a Config Download. */
	SetCommonSpace(slot);
	AsmVisaConfigDownload(PBDevsBaseAddr(PowerSlot(M2_DEVICE_ID_CDE)),
		DevConfReg(slot), CDE_VISA_CONF_DOWNLOAD,
		CDE_DEV_STATE, VisaDIPBit(slot));
}

/*****************************************************************************
 Read the next word from VISA internal ROM.
*/
	static uint32
NextVisaWord(Slot slot)
{
	/*
	 * Visa ROM is accessed serially.  To get the next word, 
	 * set to Attribute space and do a Config Download.
	 * The data (32 bits) appears in the VisaConf register.
	 */
	SetAttributeSpace(slot);
	AsmVisaConfigDownload(PBDevsBaseAddr(PowerSlot(M2_DEVICE_ID_CDE)),
		DevConfReg(slot), CDE_VISA_CONF_DOWNLOAD,
		CDE_DEV_STATE, VisaDIPBit(slot));
	return ReadPowerBusRegister(M2_DEVICE_ID_CDE, VisaConfReg(slot));
}

/*****************************************************************************
 Read data from the VISA internal ROM.
*/
	static int32
ReadVisa(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	uint32 voffset;
	uint32 word;
	uint32 wbyte;
	uint8 *dest = (uint8 *) buf;

	PRINTF(("ReadVisa(%x,%x,%x)\n", offset, len, buf));
	ResetVisa(dev->dev.hwr_Slot);

	/*
	 * We can't randomly-access the VISA ROM.
	 * Step thru it bytewise until we reach the desired offset,
	 * then start copying bytes to the destination buffer.
	 */
	wbyte = sizeof(uint32);
	word = 0; /* Not needed, but keeps the compiler happy. */
	for (voffset = 0;  voffset < offset + len;  voffset++)
	{
		if (wbyte >= sizeof(uint32))
		{
			/* No more bytes in this word; get the next word. */
			word = NextVisaWord(dev->dev.hwr_Slot);
			wbyte = 0;
			if (voffset == 0)
			{
				/*
				 * This is the first word in the ROM
				 * (the VISA config word).  Extract the size 
				 * of VISA ROM from it and truncate the read
				 * if it extends beyond the ROM size.
				 */
				uint32 visaSize = 4 * ((word >> 24) & 0xFF);
				if (offset + len > visaSize)
				{
					PRINTF(("VisaRead(%x,%x) > %x!\n",
						offset, len, visaSize));
					len = visaSize - offset;
				}
			}
		}
		if (voffset >= offset)
		{
			/* NOTE: VISA ROM is little-endian byte order. */
			*dest++ = (word >> (wbyte * 8)) & 0xFF;
		}
		wbyte++;
	}

	/* Restore to standard space (Attribute space). */
	SetAttributeSpace(dev->dev.hwr_Slot);
	return len;
}

/*****************************************************************************
 How many bits do we have to right-shift the mask to get the
 lowest order 1-bit into bit position 0?
*/
	static uint32
GetShift(uint32 mask)
{
	uint32 shift;

	for (shift = 0;  shift < 32;  shift++)
		if (mask & (1 << shift))
			break;
	return shift;
}

/*****************************************************************************
 Insert the value into the field specified by the mask.
*/
	static void
InsertField(uint32 *pReg, uint32 mask, uint32 value)
{
	uint32 shift = GetShift(mask);
	*pReg = (*pReg & ~mask) | ((value << shift) & mask);
}

/*****************************************************************************
 Extract the field specified by the mask.
*/
	static uint32
ExtractField(uint32 reg, uint32 mask)
{
	return (reg & mask) >> GetShift(mask);
}

/*****************************************************************************
 Set low-level slot hardware characteristics. 
*/
	static void
InitSlotTiming(Slot slot)
{
	uint32 cycleTime;
	uint32 setup;

	uint32 nsecPerTick = 1000000000 / theBootGlobals->bg_BusClock;
#define	NsecToTick(ns) (((ns) + nsecPerTick-1) / nsecPerTick)

	/*
	 * Per the PCMCIA spec (release 2.01, pg 4-21):
	 * Read Cycle Time    = 300 ns
	 * Address Setup Time =  30 ns
	 * Address Hold Time  =  20 ns
	 */
	cycleTime = 0;
	InsertField(&cycleTime, CDE_CYCLE_TIME, NsecToTick(300));

	setup = CDE_DATAWIDTH_8;
	InsertField(&setup, CDE_READ_SETUP,    NsecToTick(30));
	InsertField(&setup, CDE_READ_HOLD,     NsecToTick(20));
	InsertField(&setup, CDE_WRITEN_SETUP,  NsecToTick(30));
	InsertField(&setup, CDE_WRITEN_HOLD,   NsecToTick(20));
	InsertField(&setup, CDE_READ_SETUP_IO, NsecToTick(30));

	WritePowerBusRegister(M2_DEVICE_ID_CDE, CycleTimeReg(slot), cycleTime);
	WritePowerBusRegister(M2_DEVICE_ID_CDE, DevSetupReg(slot), setup);
	SetPowerBusBits(M2_DEVICE_ID_CDE, DevConfReg(slot), CDE_IOCONF);
}

/*****************************************************************************
 Initialize a PCMCIA slot.
*/
	static void
InitSlot(DipirHWResource *dev)
{
	VisaHeader visa;
	uint32 reg;

	/* Clear the block bits. */
	ClearBlockBits(dev->dev.hwr_Slot);

	/* Check for write-protect. */
	reg = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_DEV_STATE);
	if (reg & WriteProtBit(dev->dev.hwr_Slot))
		dev->dev.hwr_Perms |= HWR_WRITE_PROTECT;

	/* Do the VISA initialization. */
	ResetVisa(dev->dev.hwr_Slot);
	if (ReadVisa(dev, 0, sizeof(visa), &visa) != sizeof(visa))
		visa.visa_Magic = 0;
	if (dev->dev_Flags & HWR_COMMON)
		SetCommonSpace(dev->dev.hwr_Slot);

	if (visa.visa_Magic == VISA_MAGIC)
	{
		/* VISA is present on the card. */
		PRINTF(("Found VISA, config %x\n", visa.visa_Config));
		dev->dev.hwr_Perms |= HWR_XIP_OK | HWR_UNSIGNED_OK;
		/* Change name of HWResource. */
		strcpy(dev->dev.hwr_Name, "VISA00\\00");
		SPutHex(ExtractField(visa.visa_Config, VISA_JUMPERS),
			&dev->dev.hwr_Name[4], 2);
		SPutHex(ExtractField(visa.visa_Config, VISA_VERSION),
			&dev->dev.hwr_Name[7], 2);
	} else
	{
		/* No VISA on the card. */
		PRINTF(("No VISA: sig %x\n", visa.visa_Magic));
		/* Disable VISA checking. */
		SetPowerBusBits(M2_DEVICE_ID_CDE, CDE_VISA_DIS, 
				VisaDisBit(dev->dev.hwr_Slot));
	}

	InitSlotTiming(dev->dev.hwr_Slot);
}

/*****************************************************************************
 Probe for a card in a PCMCIA slot.
*/
	static void
ProbeSlot(Slot slot)
{
	uint32 reg;
	uint32 flags;
	DipirHWResource *dev;

	reg = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_DEV_DETECT);
	if ((reg & DetectBit(slot)) == 0)
	{
		/*
		 * No card in the slot.  Clear the DIPIR bit so the OS
		 * doesn't think we need to dipir it again.
		 */
		ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_BBLOCK, DipirBit(slot));
		return;
	}

	/* Create a HWResource and initialize the slot. */
	flags = 0;
	reg = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_BBLOCK);
	if (reg & (BlockBit(slot) | DipirBit(slot)))
		flags |= HWR_MEDIA_ACCESS;
	dev = UpdateHWResource("?pcard", CHANNEL_PCMCIA, slot, flags,
		&PCMCIAChannelDriver, CARDROMSIZE, 
		NULL, 0);
	dev->dev.hwr_DeviceSpecific[0] = SlotAddr(slot);
	InitSlot(dev);
}

#ifdef BUILD_PC16550
/*****************************************************************************
 Probe for the 16550 hardware in slot 7.
*/
	static void
ProbePC16550(void)
{
	DipirHWResource *dev;
	volatile NS16550 *ns = (NS16550 *) SlotAddr(PC16550_SLOT);
	HWResource_PC16550 *spec;

#define	CHECKREG(v) \
	ns->ns_SCR = (v);  _sync();  if (ns->ns_SCR != (v)) return;

	ClearBlockBits(PC16550_SLOT);
	InitSlotTiming(PC16550_SLOT);

	/*
	 * Try writing and reading from the 16550 scratch register.
	 * If it holds the written value, the 16550 must be present.
	 */
	CHECKREG(0x00);
	CHECKREG(0x5A);
	CHECKREG(0x00);
	CHECKREG(0xA5);

	/* Create a HWResource. */
	dev = UpdateHWResource("PC16550", CHANNEL_PCMCIA, PC16550_SLOT,
			HWR_NODIPIR,
			&PCMCIAChannelDriver, 0, NULL, 0);
	spec = (HWResource_PC16550 *) dev->dev.hwr_DeviceSpecific;
	spec->pcs_Addr = SlotAddr(PC16550_SLOT);
	spec->pcs_Clock = theBootGlobals->bg_BusClock / 2;
	spec->pcs_CycleTime = 100;
}
#endif /* BUILD_PC16550 */

/*****************************************************************************
 Probe all PCMCIA hardware.
*/
	static void
ProbePCMCIA(void)
{
	DipirHWResource *dev;
	Slot slot;

	/* Special case for system ROM in pseudo-slot 0. */
	dev = UpdateHWResource("SYSROM", CHANNEL_PCMCIA, (Slot)0, 
			HWR_SECURE_ROM | HWR_NODIPIR,
			&PCMCIAChannelDriver, ROM_SIZE, NULL, 0);
	dev->dev.hwr_Perms |= HWR_XIP_OK | HWR_UNSIGNED_OK;
	dev->dev.hwr_ROMUserStart = ROM_FSOFFSET;
	dev->dev.hwr_DeviceSpecific[0] = ROM_START;

#ifdef BUILD_PC16550
	/* Special case for PC16550 hardware in slot 7. */
	ProbePC16550();
#endif /* BUILD_PC16550 */

	/* Check all the real slots. */
	for (slot = FIRST_PCMCIA_SLOT;
	     slot < FIRST_PCMCIA_SLOT + NUM_PCMCIA_SLOTS;
	     slot++)
		ProbeSlot(slot);
}

/*****************************************************************************
 Read data from a PCMCIA device.
*/
	static int32
ReadPCMCIA(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	uint32 dlen;

	if (offset >= VISA_PCMCIA_ADDR)
	{
		/* Special case for reading VISA ROM. */
		return ReadVisa(dev, offset - VISA_PCMCIA_ADDR, len, buf);
	}
	if (offset > dev->dev.hwr_ROMSize)
		return 0;
	dlen = dev->dev.hwr_ROMSize - offset;
	if (dlen < len)
		len = dlen;
	if (dev->dev_Flags & HWR_EVENBYTES)
	{
		/* Read only even bytes from the device. */
		uint8 *romp;
		uint8 *bufp;

		romp = (uint8 *) (dev->dev.hwr_DeviceSpecific[0] + 2*offset);
		for (bufp = (uint8*)buf;  bufp < (uint8*)buf + len; )
		{
			*bufp = *romp;
			bufp += 1;
			romp += 2;
		}
	} else
	{
		memcpy(buf, (void*)(dev->dev.hwr_DeviceSpecific[0] + offset), len);
	}
	return len;
}

/*****************************************************************************
*/
	static int32
MapPCMCIA(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	/* Don't do mapping, because we can't map VISA. */
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	TOUCH(paddr);
	return -1;
}

/*****************************************************************************
*/
	static int32
UnmapPCMCIA(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return -1;
}

/*****************************************************************************
*/
	static int32
DeviceControlPCMCIA(DipirHWResource *dev, uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_BLOCK:
		return SetPowerBusBits(M2_DEVICE_ID_CDE, CDE_BBLOCK, 
			BlockBit(dev->dev.hwr_Slot));
	case CHAN_UNBLOCK:
		return ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_BBLOCK, 
			DipirBit(dev->dev.hwr_Slot) | 
			BlockBit(dev->dev.hwr_Slot));
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
ChannelControlPCMCIA(uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_DISALLOW_UNBLOCK:
		return 0;
	}
	return -1;
}

/*****************************************************************************
 Where should we look for the 3DO label on a PCMCIA device ROM?
*/
	static int32
RetryLabelPCMCIA(DipirHWResource *dev, uint32 *pState)
{
	switch (*pState)
	{
	case 0:
		/* Try Attribute space. */
		PRINTF(("RetryLabel: First try Attr space\n"));
		/* Should already be in Attribute space. */
		break;
	case 1:
		/* Now try Common space. */
		if (!(theBootGlobals->bg_DipirControl & DC_ALT_PCMCIA_SPACE))
			goto CheckVisa;
		PRINTF(("RetryLabel: Now try Common space\n"));
		SetCommonSpace(dev->dev.hwr_Slot);
		dev->dev_Flags |= HWR_COMMON;
		break;
	case 2:
		/* Now try Attribute space, even bytes. */
		if (!(theBootGlobals->bg_DipirControl & DC_ALT_PCMCIA_SPACE))
			goto CheckVisa;
		PRINTF(("RetryLabel: Now try Attr space, even bytes\n"));
		SetAttributeSpace(dev->dev.hwr_Slot);
		dev->dev_Flags &= ~HWR_COMMON;
		dev->dev_Flags |= HWR_EVENBYTES;
		break;
	case 3:
		/* Now try Common space, even bytes. */
		if (!(theBootGlobals->bg_DipirControl & DC_ALT_PCMCIA_SPACE))
			goto CheckVisa;
		PRINTF(("RetryLabel: Now try Common space, even bytes\n"));
		SetCommonSpace(dev->dev.hwr_Slot);
		dev->dev_Flags |= HWR_COMMON;
		break;
	case 4:
	CheckVisa:
		dev->dev_Flags &= ~(HWR_COMMON | HWR_EVENBYTES);
		/* Now try internal VISA ROM. */
		if (dev->dev.hwr_Perms & HWR_XIP_OK)
		{
			PRINTF(("RetryLabel: Now try VISA\n"));
			dev->dev.hwr_ROMUserStart = 
				VISA_PCMCIA_ADDR + sizeof(VisaHeader);
			dev->dev.hwr_ROMSize = CARDROMSIZE + VISA_PCMCIA_ADDR;
			break;
		}
		PRINTF(("RetryLabel: no VISA\n"));
		/* No VISA: fall thru */
	default:
		/* Ran out of places to try. */
		return -1;
	}

	(*pState)++;
	return dev->dev.hwr_ROMUserStart;
}

/*****************************************************************************
*/
const ChannelDriver PCMCIAChannelDriver = 
{
	InitPCMCIA,
	ProbePCMCIA,
	ReadPCMCIA,
	MapPCMCIA,
	UnmapPCMCIA,
	DeviceControlPCMCIA,
	ChannelControlPCMCIA,
	RetryLabelPCMCIA
};

