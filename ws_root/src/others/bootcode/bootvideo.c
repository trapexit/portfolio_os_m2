/* @(#) bootvideo.c 96/09/20 1.43 */

/********************************************************************************
*
*	bootvideo.c
*
*	This file contains the routines which set up and turn on the boot
*	video display.
*
********************************************************************************/

#include <hardware/PPC.h>
#include <hardware/bda.h>
#include <hardware/cde.h>
#include <hardware/bridgit.h>
#include <hardware/m2vdl.h>
#include <kernel/types.h>
#include <kernel/sysinfo.h>
#include <bootcode/bootglobals.h>
#include <bootcode/boothw.h>
#include "bootcode.h"

extern bootGlobals BootGlobals;

void DrawString(char *pString, charCoords *pCoords);

#define	SYSTYPE_IS_UPGRADE	(BootGlobals.bg_SystemType == SYSINFO_SYSTYPE_UPGRADE)


/********************************************************************************
*
*	DrawColorBars
*
*	This routine draws a pattern of color bars into the frame buffer.
*
*	Input:	None
*	Output:	None
*	Calls:	None
*
********************************************************************************/

#ifdef	BUILD_DEBUGGER

static void DrawColorBars(void) {

	ubyte		*pPixel,
			bits,
			redbits,
			greenbits,
			band,
			block,
			pixel;
	const ubyte	red[8] =   {0xFF, 0xFF, 0,    0,    0xFF, 0xFF, 0,    0x0},
			green[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0,    0,    0,    0x0},
			blue[8] =  {0xFF, 0,    0xFF, 0,    0xFF, 0,    0xFF, 0x0};
	uint32		line,
			blocksize,
			color;

	bits = 3 * ((4 - BootGlobals.bg_PixelBytes) / 2);
	redbits = 16 - (2 * bits);
	greenbits = 8 - bits;
	blocksize = BootGlobals.bg_LinePixels / COL_BLOCKS;
	pPixel = (ubyte *)BootGlobals.bg_DisplayPointer;
	for (band = BANDS; band > 0; band--) {
		for (line = 0; line < blocksize; line++) {
			for (block = 0; block < COL_BLOCKS; block++) {
				color  = ((red[block]   >> bits) * band / BANDS) << redbits;
				color += ((green[block] >> bits) * band / BANDS) << greenbits;
				color += ((blue[block]  >> bits) * band / BANDS);
				for (pixel = 0; pixel < blocksize; pixel++) {
					if (bits) {
						*(uint16 *)pPixel  = color;
						pPixel += 2;
					} else {
						*(uint32 *)pPixel  = color;
						pPixel += 4;
					}
				}
			}
		}
	}
	line = BANDS * blocksize;
	blocksize = BootGlobals.bg_LinePixels / BW_BLOCKS;
	for (; line < BootGlobals.bg_ScrnLines; line++) {
		for (block = 0; block < BW_BLOCKS; block++) {
			color  = ((red[0]   >> bits) * block / BW_BLOCKS) << redbits;
			color += ((green[0] >> bits) * block / BW_BLOCKS) << greenbits;
			color += ((blue[0]  >> bits) * block / BW_BLOCKS);
			for (pixel = 0; pixel < blocksize; pixel++) {
				if (bits) {
					*(uint16 *)pPixel  = color;
					pPixel += 2;
				} else {
					*(uint32 *)pPixel  = color;
					pPixel += 4;
				}
			}
		}
	}
}

#endif	/* ifdef BUILD_DEBUGGER */


/********************************************************************************
*
*	DrawCharacter
*
*	This routine draws a character into a frame buffer.  Note that the
*	input coordinates are modified by this routine.
*
*	Input:	character = ASCII character
*		*pCoords = Pointer to charCoords structure
*	Output:	None
*	Calls:	DrawString
*
********************************************************************************/

static void DrawCharacter(char character, charCoords *pCoords) {

	typedef struct {
		uint16		column[CHAR_WIDTH];
	} chardata;

	typedef struct {
		chardata	character[120];
		ubyte		junk;
	} charset;

	ExtVolumeLabel	*pVolumeLabel;

	RomTag		*pROMTagTable,
			*pCharacterDataTag;

	ubyte		column,
			row,
			*pPixel;

	uint32		columndata,
			white,
			black = 0,
			pixel,
			offset;

	charset		*pCharacterData;

	pVolumeLabel = BootGlobals.bg_ROMVolumeLabel;
	pROMTagTable = (RomTag *)((uint32)pVolumeLabel + sizeof(ExtVolumeLabel));

	pCharacterDataTag = FindROMTag(pROMTagTable, pVolumeLabel->dl_NumRomTags,
		RT_SUBSYS_ROM, ROM_CHARACTER_DATA);
	pCharacterData = (charset *)((uint32)pROMTagTable +
		pVolumeLabel->dl_VolumeBlockSize * (uint32)pCharacterDataTag->rt_Offset);

	if (((pCoords->hcoord + 6) * CHAR_WIDTH) >= BootGlobals.bg_LinePixels) {
		DrawString("Error, hcoord too big", pCoords);
	}
	if (((pCoords->vcoord + 6) * CHAR_HEIGHT) >= BootGlobals.bg_ScrnLines) {
		DrawString("Error, vcoord too big", pCoords);
	}

	if (character == '\n') {
		pCoords->hcoord = 0;
		pCoords->vcoord += 1;
	} else if (character == '\f') {
		pCoords->vcoord += 1;
	} else if (character == '\r') {
		pCoords->hcoord = pCoords->stringCR;
		pCoords->vcoord += 1;
	} else {
		white = (BootGlobals.bg_PixelBytes == 2) ? 0x739C : 0x00E0E0E0;
		character = character - 32;
		pPixel = (ubyte *)BootGlobals.bg_DisplayPointer +
			BootGlobals.bg_LinePixels * BootGlobals.bg_PixelBytes * CHAR_HEIGHT * (pCoords->vcoord + 2) +
			BootGlobals.bg_PixelBytes * CHAR_WIDTH * (pCoords->hcoord + 2);

		for (column = 0; column < CHAR_WIDTH; column++) {
			columndata = pCharacterData->character[character].column[column];
			for (row = 0; row < CHAR_HEIGHT; row++) {
				pixel = (columndata & ((1 << (CHAR_HEIGHT - 1)) >> row)) ? white : black;
				offset = row * BootGlobals.bg_LinePixels * BootGlobals.bg_PixelBytes;

				if (BootGlobals.bg_PixelBytes == 2) *(uint16 *)(pPixel + offset) = pixel;
				else *(uint32 *)(pPixel + offset) = pixel;
			}
			pPixel += BootGlobals.bg_PixelBytes;
		}
		pCoords->hcoord += 1;
	}

	if (((pCoords->hcoord + 5) * CHAR_WIDTH) >= BootGlobals.bg_LinePixels) {
		pCoords->hcoord = 0;
		pCoords->vcoord += 1;
	}
	if (((pCoords->vcoord + 5) * CHAR_HEIGHT) >= BootGlobals.bg_ScrnLines) pCoords->vcoord = 0;
}


/********************************************************************************
*
*	DrawString
*
*	This routine feeds a string to DrawCharacter one character at a time.
*
*	Input:	*pString = Pointer to the string
*		*pCoords = Pointer to charCoords structure
*	Output:	None
*	Calls:	DrawCharacter
*
********************************************************************************/

static void DrawString(char *pString, charCoords *pCoords) {

	pCoords->stringCR = pCoords->hcoord;
	while (*pString) {
		DrawCharacter(*pString, pCoords);
		pString++;
	}
}


/********************************************************************************
*
*	SetupFrameBuffer
*
*	This routine creates the boot image in the frame buffer.
*
*	Input:	None
*	Output:	None
*	Calls:	DrawColorBars (BUILD_DEBUGGER only)
*		DrawString
*
********************************************************************************/

static void SetupFrameBuffer(void) {

#ifdef	BUILD_DEBUGGER

	charCoords	coords;

	DrawColorBars();

	if (BootGlobals.bg_VideoMode == BG_VIDEO_MODE_PAL) {
		coords.hcoord = 15;
		coords.vcoord = 6;
	} else {
		coords.hcoord = 10;
		coords.vcoord = 4;
	}
	DrawString(LOGO_STRING, &coords);

	coords.hcoord += LOGO_WIDTH;
	coords.vcoord -= LOGO_HEIGHT;
	DrawString("                        \r", &coords);

	if (BootGlobals.bg_NumCPUs == 1)
            DrawString(" Welcome to M2.         \r", &coords);
        else
            DrawString(" Welcome to M2/MP.      \r", &coords);

	DrawString("                        \r", &coords);
	DrawString(" Just put those other   \r", &coords);
	DrawString(" *toy* machines in the  \r", &coords);
	DrawString(" garage or something.   \r", &coords);
	DrawString("                        \r", &coords);

#else	/* ifdef BUILD_DEBUGGER */

	ExtVolumeLabel	*pVolumeLabel;

	RomTag		*pROMTagTable,
			*pStaticScreenTag;

	uint8		*pPixel;

	uint32		pStaticScreen,
			line,
			target,
			length,
			xcoord,
			ycoord,
			keycolor,
			background,
			pixbytes;

	pVolumeLabel = BootGlobals.bg_ROMVolumeLabel;
	pROMTagTable = (RomTag *)((uint32)pVolumeLabel + sizeof(ExtVolumeLabel));

	pStaticScreenTag = FindROMTag(pROMTagTable, pVolumeLabel->dl_NumRomTags,
		RT_SUBSYS_ROM, ROM_STATIC_SCREEN);
	pStaticScreen = (uint32)((uint32)pROMTagTable +
		pVolumeLabel->dl_VolumeBlockSize * (uint32)pStaticScreenTag->rt_Offset);

	pixbytes = BootGlobals.bg_PixelBytes;
	length = (pStaticScreenTag->rt_Reserved3[0] >> 16) * pixbytes;
	xcoord = pStaticScreenTag->rt_Reserved3[1] >> 16;
	ycoord = pStaticScreenTag->rt_Reserved3[1] & 0xFFFF;
	keycolor = pStaticScreenTag->rt_Reserved3[2];
	background = pStaticScreenTag->rt_Reserved3[3];

	/* Paint frame buffer with bacground color */
	for (target = 0; target < BootGlobals.bg_DisplaySize/sizeof(uint32); target++)
		((uint32*)BootGlobals.bg_DisplayPointer)[target] = background;

	/* Transfer static screen art to frame buffer and map key color to background */
	target = (uint32)BootGlobals.bg_DisplayPointer + ycoord * BootGlobals.bg_LineBytes + xcoord * pixbytes;
	for (line = 0; line < (pStaticScreenTag->rt_Reserved3[0] & 0xFFFF); line++) {
		MemoryMove(pStaticScreen, target, length);
		if (keycolor) for (pPixel = (uint8 *)target; pPixel < (uint8 *)(target + length); pPixel += pixbytes) {
			if ((pixbytes == 2) && (*(uint16 *)(pPixel) == keycolor)) *(uint16 *)(pPixel) = (uint16)background;
			if ((pixbytes == 4) && (*(uint32 *)(pPixel) == keycolor)) *(uint32 *)(pPixel) = background;
		}
		pStaticScreen += length;
		target += BootGlobals.bg_LineBytes;
	}

#endif	/* ifdef BUILD_DEBUGGER */

	/* If this is an upgrade system then make sure an Opera is attached */
	if (SYSTYPE_IS_UPGRADE && !GetBDAGPIO(BDAMREF_GPIO_OPERAFLAG, 0)) {

		charCoords	coords;

		coords.hcoord = 12;
		coords.vcoord = 16;
		DrawString("                          \r", &coords);
		DrawString(" Please connect and turn  \r", &coords);
		DrawString(" on your REAL multiplayer \r", &coords);
		DrawString("                          \r", &coords);
	}
}


/********************************************************************************
*
*	SetupVDL
*
*	This routine creates a VDL in memory to control the boot display.
*
*	Input:	pVDL = Pointer to empty boot VDL template
*	Output:	None
*	Calls:	None
*
********************************************************************************/

static void SetupVDL(bootVDL *pVDL) {

	uint32		*pDisplay;
	VDLHeader	*pHeaderEntry;
	ShortVDL	*pShortEntry;

	pDisplay = BootGlobals.bg_DisplayPointer;
	if (pVDL == &BootGlobals.bg_BootVDL0) {
		pDisplay = (uint32 *)((uint32)pDisplay + (BootGlobals.bg_LineBytes));
	}


	/* Create 1st VDL entry to span unused blank lines at top of screen */

	pHeaderEntry = &pVDL->bv_TopBlank;
	pHeaderEntry->DMACtl =
		VDL_DMA_NWORDSFIELD(sizeof(VDLHeader) / sizeof(uint32)) |
		VDL_DMA_NLINESFIELD(BootGlobals.bg_BlankLines);
	pHeaderEntry->UpperPtr = pDisplay;
	pHeaderEntry->LowerPtr = pDisplay;
	pHeaderEntry->NextVDL = (VDLHeader *)&pVDL->bv_ActiveRegion;


	/* Create 2nd VDL entry to span active display region */

	pShortEntry = &pVDL->bv_ActiveRegion;
	pHeaderEntry = &pShortEntry->sv;
	pHeaderEntry->DMACtl =
		VDL_DMA_MOD_FIELD(BootGlobals.bg_LineBytes * 2) |
		VDL_DMA_ENABLE |
		VDL_DMA_LDLOWER |
		VDL_DMA_LDUPPER |
		VDL_DMA_NWORDSFIELD(sizeof(ShortVDL) / sizeof(uint32)) |
		VDL_DMA_NLINESFIELD(BootGlobals.bg_ScrnLines / 2);
	pHeaderEntry->UpperPtr = pDisplay;
	pHeaderEntry->LowerPtr = pDisplay;
	pHeaderEntry->NextVDL = &pVDL->bv_BottomBlank;
	pShortEntry->sv_DispCtl0 =
		VDL_DC | VDL_DC_0 |
		VDL_CTL_FIELD(VDL_DC_HINTCTL, VDL_CTL_DISABLE) |
		VDL_CTL_FIELD(VDL_DC_VINTCTL, VDL_CTL_DISABLE) |
		VDL_CTL_FIELD(VDL_DC_DITHERCTL, VDL_CTL_DISABLE) |
		VDL_CTL_FIELD(VDL_DC_MTXBYPCTL, VDL_CTL_DISABLE);
	pShortEntry->sv_DispCtl1 =
		VDL_DC | VDL_DC_1 |
		VDL_CTL_FIELD(VDL_DC_HINTCTL, VDL_CTL_DISABLE) |
		VDL_CTL_FIELD(VDL_DC_VINTCTL, VDL_CTL_DISABLE) |
		VDL_CTL_FIELD(VDL_DC_DITHERCTL, VDL_CTL_DISABLE) |
		VDL_CTL_FIELD(VDL_DC_MTXBYPCTL, VDL_CTL_DISABLE);
	pShortEntry->sv_AVCtl =
		VDL_AV |
		VDL_AV_LD_HSTART |
		VDL_AV_LD_HWIDTH |
		VDL_AV_FIELD(VDL_AV_HSTART, BootGlobals.bg_HStart) |
		VDL_AV_FIELD(VDL_AV_HWIDTH, BootGlobals.bg_LinePixels);
	pShortEntry->sv_ListCtl =
		VDL_LC |
		VDL_LC_LD_BYPASSTYPE | VDL_LC_BYPASSTYPE_MSB |
		VDL_LC_LD_FBFORMAT | VDL_LC_FBFORMAT_16;


	/* Create 3rd VDL entry to span unused blank lines at bottom of screen */

	pHeaderEntry = &pVDL->bv_BottomBlank;
	pHeaderEntry->DMACtl =
		VDL_DMA_NWORDSFIELD(sizeof (VDLHeader) / sizeof (uint32)) |
		VDL_DMA_NLINESFIELD(0);
	pHeaderEntry->UpperPtr = pDisplay;
	pHeaderEntry->LowerPtr = pDisplay;
	pHeaderEntry->NextVDL = &pVDL->bv_TopBlank;
}


/********************************************************************************
*
*	GetClockSpeeds
*
*	This routine determines the bus and CPU clock speeds to the nearest
*	MHz and stores them in the appropriate boot globals fields.  It also
*	determines the number of CPU counter ticks per second based on the
*	rounded off clock speeds.
*
*	It is assumed that this routine will only be called after enough
*	time has been allowed for the video clocks to stabilize; the minimum
*	recommended delay is 5 fields after the encoder is brought out of
*	reset.  The number of fields specified for the bus clock measurement
*	should be even to average out any time differences between even and
*	odd fields, and should be large enough to average over any field length
*	fluctuations; 10 is the recommended number.
*
*	The bus clock is determined by using the CPU's time base counter to
*	measure the length of the specified number of video field and
*	calculating as follows:
*
*	(bus cycles/count) = 4
*	(fields/second) = 59.94 for NTSC or 50 for PAL
*
*	BusClock = (counts/field) * (bus cycles/count) * (fields/second)
*
*	This number is then rounded to the nearest MHz.  Nominal ticks per
*	second is then calculated as 1/4 the rounded bus frequency.
*
*	The CPU clock is determined by assuming the CPU performs one NOP per
*	cycle when running from it's cache, and using the CPU's time base
*	counter to measure the time to execute 1000 NOPs.  Then we assume the
*	CPU clock is an integer multiple of the bus clock and calculate as
*	follows:
*
*	CPUClockCycles = 1000
*	BusClockCycles = 4 * (counts for 1000 NOPs)
*	IntegerMultiplier = (10 * (CPUClockCycles/BusClockCycles) + 5) / 10
*
*	CPUClock = BusClock * IntegerMultiplier
*
*	Note that the instruction cache MUST be enabled for this to yield
*	the correct result.  In addition it is assumed that no interrupts
*	will occur during the measurement.  The final number is
*
*	Now take all that you've learned in the previous paragraphs, and
*	forget it all. Well, not quite...
*
*	The hardware guy chose to use a crystal which is not an integral number
*	of MHz. Due to our timing innaccuracies, we can't figure out the
*	actual ultra precise value that we need. So what we do instead is
*	take the rough calculated bus clock value and look up in a table to
*	find the real full clock frequency. If we don't find a close match,
*	we stick with the value we calculated. This will let us handle test
*	cases where the clock speed is set to vastly different values.
*
*	Input:	fields = Number of fields for bus clock measurement
*	Output:	None
*	Calls:	FieldCount
*		CPUCount
*
********************************************************************************/

typedef struct
{
    uint32 clk_BusClock;
    uint32 clk_CPUClock;
} Clocks;

static const Clocks presetClocks[] =
{
    {33333333, 66666667},
    {0,0}
};


static void GetClockSpeeds(uint32 fields) {

	uint32 busCycles;
	uint32 clockMultiplier;
	uint64 ticks;
	uint32 i;

	ticks = (uint64)FieldCount(fields);
	if (BootGlobals.bg_VideoMode == BG_VIDEO_MODE_PAL) {
	    ticks = (ticks * 50) / fields;
	} else {
	    ticks = (ticks * 5994) / (100 * fields);
	}

	BootGlobals.bg_BusClock    = ticks * 4; /* 1 tick per 4 bus cycles   */
	BootGlobals.bg_BusClock	  += 500000;	/* round to nearest multiple */
	BootGlobals.bg_BusClock	  /= 1000000;	/* of megahertz              */
	BootGlobals.bg_BusClock	  *= 1000000;	/* units in hertz            */

	i = 0;
	while (presetClocks[i].clk_BusClock)
	{
	    if ((presetClocks[i].clk_BusClock / 1000000) == (BootGlobals.bg_BusClock / 1000000))
            {
                BootGlobals.bg_BusClock    = presetClocks[i].clk_BusClock;
                BootGlobals.bg_CPUClock    = presetClocks[i].clk_CPUClock;
                BootGlobals.bg_TicksPerSec = BootGlobals.bg_BusClock / 4;
                return;
            }
            i++;
	}

	BootGlobals.bg_TicksPerSec = BootGlobals.bg_BusClock / 4;

#ifdef BUILD_CACHESOFF
 	EnableICache();		/* Must enable ICache in order to call CPUCount() */
#endif

	busCycles               = 4 * CPUCount(1000);
	clockMultiplier         = ((10000 / busCycles) + 5) / 10;
	BootGlobals.bg_CPUClock = BootGlobals.bg_BusClock * clockMultiplier;

#ifdef BUILD_CACHESOFF
 	DisableICache();	/* Disable the ICache only if it was alread disabled */
#endif
}


/********************************************************************************
*
*	EnableNullVideo
*
*	This routine enables the video circuitry to display a framebufferless,
*	blank screen.  This allows us to do some other stuff in memory without
*	corrupting the video output by accidently stomping on any preexisting
*	framebuffer or VDL.
*
*	If the encoder or the VDU is in reset then we:
*
*	- Reset them both just to be sure
*	- Start the VDU and point it at a NULL VDL (displays a blank image)
*	- Start the encoder
*	- Wait 20 fields to allow the video signals to stabilize
*	- Take another 10 fields to measure the system clocks
*
*	Note that the video signals are actually pretty stable after just a
*	couple fields, but some monitors may take as many as 30 fields to
*	sync to the video signal so we make sure we wait at least that long
*	to avoid video rolls and or glitches when the real boot screen is
*	displayed.
*
*	If the encoder and VDU are both already running then we bonk the VDU
*	over the head to make sure it's in a known state:
*
*	- Point the VDU at a NULL VDL
*	- Wait a couple fields to sync to a field top
*	- Reset the VDU
*	- Start the VDU back up
*	- Take another 10 fields to measure the system clocks
*
*	Input:	None
*	Output:	None
*	Calls:	FieldCount
*		GetClockSpeeds
*
********************************************************************************/

static void EnableNullVideo(void) {

	if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
		BDA_WRITE(BDAVDU_VCFG, 0x40000000);
	} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
		BDA_WRITE(BDAVDU_VCFG, 0x40000000);
	} else {
		BDA_WRITE(BDAVDU_VCFG, 0);
	}

	BootGlobals.bg_NullVDL.DMACtl = VDL_DMA_NWORDSFIELD(sizeof(VDLHeader) / sizeof(uint32));
	FlushDCache((uint32)&BootGlobals.bg_NullVDL.DMACtl, sizeof(BootGlobals.bg_NullVDL.DMACtl));
	if (BDA_READ(BDAVDU_VRST)) {
		BDA_WRITE(BDAVDU_VRST, 3);
		BDA_WRITE(BDAVDU_VRST, 2);
		BDA_WRITE(BDAVDU_FV0A, (uint32)&BootGlobals.bg_NullVDL);
		BDA_WRITE(BDAVDU_FV1A, (uint32)&BootGlobals.bg_NullVDL);
		BDA_WRITE(BDAVDU_VRST, 0);
		while (SetROMSysInfo(SYSINFO_TAG_INTERLACED, 0, 0) != SYSINFO_SUCCESS);
		FieldCount(20);
	} else {
		BDA_WRITE(BDAVDU_FV0A, (uint32)&BootGlobals.bg_NullVDL);
		BDA_WRITE(BDAVDU_FV1A, (uint32)&BootGlobals.bg_NullVDL);
		while (SetROMSysInfo(SYSINFO_TAG_INTERLACED, 0, 0) != SYSINFO_SUCCESS);
		FieldCount(2);
		BDA_WRITE(BDAVDU_VRST, 1);
		BDA_WRITE(BDAVDU_VRST, 0);
	}
	GetClockSpeeds(10);
}


/********************************************************************************
*
*	EnableBootVideo
*
*	This routine replaces an existing display with the real bootcode static
*	screen display.  This is done simply by pointing the VDU at the real
*	boot VDL.
*
*	It is assumed that EnableNullVideo has been called prior to this
*	routine in order to put the VDU into a known and happy state.
*
*	Input:	None
*	Output:	None
*	Calls:	FlushDCacheAll
*
********************************************************************************/

static void EnableBootVideo(void) {

	FlushDCacheAll();
	BDA_WRITE(BDAVDU_FV0A, (uint32)&BootGlobals.bg_BootVDL0);
	BDA_WRITE(BDAVDU_FV1A, (uint32)&BootGlobals.bg_BootVDL1);
}


/********************************************************************************
*
*	ReInitMemory
*
*	This routine reinitializes memory, being careful to avoid any ranges
*	which need to be preserved.  It also takes care of figuring out where
*	to place the boot display framebuffer to avoid these preserved ranges.
*
*	This routine is not called during the very first dipir event after a
*	hard reset since in that case it is assumed that the hard reset handler
*	has already cleared memory and determined the framebuffer placement.
*
*	Input:	None
*	Output:	None
*	Calls:	MemorySet
*
********************************************************************************/

static void ReInitMemory(void) {

	ubyte		pattern;

	uint32		beginRAM,
			endRAM,
			target,
			endp,
			length;
	BootAlloc	*ba;

	beginRAM = BootGlobals.bg_SystemRAM.mr_Start;
	endRAM = beginRAM + BootGlobals.bg_SystemRAM.mr_Size;
#ifdef	BUILD_DEBUGGER
	pattern = 0x55;
#else	/* ifdef BUILD_DEBUGGER */
	pattern = 0xAA;
#endif	/* ifdef BUILD_DEBUGGER */

	/* Check for illegal values and range overlaps */
	SanityCheckBootAllocs();

	/* Initialize gaps between ranges */
	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc) {
		target = (uint32)ba->ba_Start + ba->ba_Size;
		endp = ISNODE(&BootGlobals.bg_BootAllocs, ba->ba_Next) ?
			(uint32)ba->ba_Next->ba_Start : endRAM;
		length = endp - target;
		DBUGPV("Target = ", target);
		DBUGPV("Length = ", length);
		if (length > 0) {
			DBUG("Initing range\n");
			MemorySet(pattern, target, length);
		}
	}
}


/********************************************************************************
*
*	SetupBootVideo
*
*	This routine does everything necessary to setup and enable the boot
*	video display.  Note that interrupts must be off while in this routine.
*
*	Input:	None
*	Output:	None
*	Calls:	EnableNullVideo
*		ReInitMemory
*		SetupFrameBuffer
*		SetupVDL
*		FlushDCacheAll
*		EnableBootVideo
*
********************************************************************************/

void SetupBootVideo(void) {

	EnableNullVideo();
	if (CDE_READ(CDE_BASE, CDE_VISA_DIS) & CDE_NOT_1ST_DIPIR) {
		ReInitMemory();
	}
	SetupFrameBuffer();
	SetupVDL(&BootGlobals.bg_BootVDL0);
	SetupVDL(&BootGlobals.bg_BootVDL1);
	EnableBootVideo();
	if (SYSTYPE_IS_UPGRADE) while (!GetBDAGPIO(BDAMREF_GPIO_OPERAFLAG, 0));
}

