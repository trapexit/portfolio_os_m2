/*
 *		[M]ega[P]lay  - play-s3m.c - Module player for the M2 (S3M Component)
 *
 *			VERSION BY:		Mark Rearick / 3DO Zoo Crew
 *
 *			PROGRAM HISTORY:
 *
 *		001		03/22/96	Began work based off of my Gus routines on the PC...
 *		002		03/25/96	It's playing back, but heavily distorted...
 *		005		03/25/96	Distortion indentified as an unsigned sample problem...
 *		006		03/27/96	Tempo fixed to use AudioClock, also corrected timing problem (250-tempo)...
 *		050		03/28/96	It's running, and I mean well!  Nothing sounds like a mod, eh...
 *		051		03/29/96	Fixed a faily major bug with sample loop processing...
 *		052		03/29/96	Fixed another bug with > 99 patterns in orderlist...
 *		053		03/29/96	Added two routines to print information out about the soundfile...
 *		054		03/29/96	Fixed a nasty bug, NULL's within patterns occur sometimes...
 *		060		03/30/96	Found a problem using attachments (thanks Bill!), had to use instrument workaround...
 *		061		03/31/96	Changed volume divisor a little higher to help avoid clipping...
 *		062		03/31/96	Had to add the volatile identifier to Playdone to solve optimization issues...
 *		064		04/01/96	Fixed problem with negative values in globalvolume...
 *		065		04/01/96	Fixed wierd audio DC offset problem (extending beyond region)...
 *		066		04/02/96	Fixed a problem with not progressing far enought in numchannels...
 *		070		04/02/96	Changed all samples to now be linked modified at playtime (faster/better)...
 *		071		04/02/96	Expanded the Play_PrintSamples() to include messages (blank instruments)...
 *		072		04/03/96	Improved error handling and recover...
 *
 *
 *			THINGS TO DO:
 *
 *		.o.		The tempo settings for higher BPM's is not right (160+)... I still haven't figured out a good way to fix it yet...
 *
 */

/*********************************************************************/

#define	MEGA_PLAY_VERSION		"0.71"						/* This is the current version number */

/*********************************************************************/

#include <stdio.h>											/* Every good 'C' program needs this... */
#include <stdlib.h>											/* for exit() */
#include <string.h>
#include <assert.h>
#include <ctype.h>


#ifdef MACINTOSH
#include <misc:event.h>										/* This is used for handling the controller */
#include <file:fileio.h>									/* It's always nice to be able to actually LOAD the file */
#include <file:filefunctions.h>
#include <kernel:mem.h>										/* for FreeMemTrack */
#include <kernel:types.h>
#include <kernel:debug.h>									/* for print macro: CHECK_NEG */
#include <kernel:msgport.h>									/* GetMsg */
#include <kernel:task.h>
#include <audio:audio.h>									/* Sound is what this is all about */
#else
#include <misc/event.h>										/* This is used for handling the controller */
#include <file/fileio.h>									/* It's always nice to be able to actually LOAD the file */
#include <file/filefunctions.h>
#include <kernel/mem.h>		 								/* for FreeMemTrack */
#include <kernel/types.h>
#include <kernel/debug.h>									/* for print macro: CHECK_NEG */
#include <kernel/msgport.h>									/* GetMsg */
#include <kernel/task.h>
#include <audio/audio.h>									/* Sound is what this is all about */
#endif


#include "s3m-info.h"										/* Include support for the S3M format */
#include "play-s3m.h"										/* Take care of some of the internal functions */
#include "sound-m2.h"										/* This is all the M2-specific-alities */




/*********************************************************************/

void Play_InnerInit(void);									/* Initialize replay and load up song */
void Play_InnerLoop(void);									/* This is where we sit the other part of the time... how exciting! */

/*********************************************************************/

uint8 vibsine[32]={	2,
					24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,
					255,
					253,250,244,235,224,212,197,180,161,141,120,97,74,49,24};		/* Now this is a SMALL sine... */

/*********************************************************************/

								/* These are initialize upon INIT of MP */
float32 globaltempo;				/* Global tempo adjustment */
float32 globalpitch;				/* Global pitch adjustment */
float32 globalvolume;				/* Global volume adjustment */

/*********************************************************************/

Item ThisMusicTask;
								/* File related information */
RawFile *s3mfile;					/* s3m to load */
s3mhead head;						/* s3m header */
char s3mfilename[80];				/* Pretty obvious what this is */

int16 numchannels;
int16 fxchannels;
								/* Player-related information */
char dirtyperiod;					/* flag to reset period on new note */
uint8 tick;							/* timing variables */
uint8 speed;
uint8 tempo;
volatile uint16 PlayDone;					/* Are we done playing, YET?!?! */

volatile uint8 PatNum;						/* Current Pattern number */
volatile uint8 PatOrd;						/* Current order in pattern */
volatile uint8 PatRow;						/* Current row in pattern */

Pattern *patterns[MAX_PATTERNS];	/* list of pointers to patterns */
uint8 ordlist[255];					/* Order list */
channel channels[MAX_CHANNELS];		/* channel replaydata */
uint8 channelpan[MAX_CHANNELS];		/* Tracks channel panning information */

/*********************************************************************/
/**	Ahh the joys of lamer Intel for choosing the weak Endian	******/
/*********************************************************************/

#define Endian16(x) (RealEndian16((char *) &x))
void RealEndian16(char *swapme)
{
	char temp16 = swapme[0];
	swapme[0] = swapme[1];
	swapme[1] = temp16;
}

#define Endian32(x) (RealEndian32((char *) &x))
void RealEndian32(char *swapme)
{
	char temp16 = swapme[0];
	swapme[0] = swapme[3];
	swapme[3] = temp16;
	temp16 = swapme[1];
	swapme[1] = swapme[2];
	swapme[2] = temp16;
}

/*********************************************************************/
/*
 *		These routines are just my convienience calls...
 */
/*********************************************************************/

void Abort(char *errmsg)
{
	printf("ERROR: %s\n", errmsg);
	exit(1);
}


void MemCheck(char *printmsg)											/* Scavenges memory, then displays amount used */
{
	MemInfo tempinfo;

	ScavengeMem();														/* Make sure everything is clean */
	GetMemInfo(&tempinfo, sizeof(MemInfo), MEMTYPE_NORMAL);

	return;
/*	printf("%-30s: %.8d\n", printmsg, tempinfo.minfo_TaskAllocatedBytes); */
}


/*********************************************************************/
/*
 *		Here's where it all begins...
 */
/*********************************************************************/

void SetupS3MInstruments(void)
{
	uint16 InstPtrs[MAX_INSTRUMENTS];									/* Pointers to Instruments */
	uint32 InstData;													/* Pointer in file to sampledata */
	s3minstr *CurInst;													/* SCRS header */
	uint32 x;


	SeekRawFile(s3mfile, (0x60 + head.OrdNum), FILESEEK_START);
	memset(InstPtrs, 0, (sizeof(uint16) * MAX_INSTRUMENTS));
	ReadRawFile(s3mfile, &InstPtrs, (sizeof(uint16) * head.InsNum));				/* Copy to Pointers */

	for(x=0; x < head.InsNum; x++)
		Endian16(InstPtrs[x]);

	memset(&instrs, 0, (sizeof(instrument) * MAX_INSTRUMENTS));						/* Reset all the instruments to 0 */

	if ((CurInst = AllocMem(sizeof(s3minstr), MEMTYPE_NORMAL)) == NULL)				/* get temporary header */
		Abort("Memory Allocation fail in SetupS3Minstruments 01");

	for(x=1; x < (head.InsNum+1); x++)
		{
		SeekRawFile(s3mfile, ((uint32)InstPtrs[x-1]  << 4), FILESEEK_START);
		memset(CurInst, 0, sizeof(s3minstr));
		ReadRawFile(s3mfile, CurInst, sizeof(s3minstr));							/* Copy to Pointer */

		Endian16(CurInst->MemSegLo);
		Endian16(CurInst->Length);
		Endian16(CurInst->LengthHi);
		Endian16(CurInst->LoopBg);
		Endian16(CurInst->LoopBgHi);
		Endian16(CurInst->LoopNd);
		Endian16(CurInst->LoopNdHi);
		Endian16(CurInst->C2spd);
		Endian16(CurInst->C2sHi);
		Endian16(CurInst->GravPos);
		Endian16(CurInst->SbLoop);
		Endian16(CurInst->LastUs);
		Endian16(CurInst->LastUsHi);

		if (CurInst->Pack != 0)
			printf("Warning, (%d) %s is packed!\n", x, CurInst->Sname);

		if (CurInst->Type != 0)											/* if zero then nothing to do */
			{
			if (CurInst->Type == 1)
				{
				InstData = ((uint32)CurInst->MemSegLo | ((uint32)CurInst->MemSegHi << 16)) << 4;
				SeekRawFile(s3mfile, InstData, FILESEEK_START);

				M2_SetupInst(x, s3mfile, CurInst);							/* Get this bad-boy ready */
				}
			else
				printf("Warning, (%d) %s is of type !\n", x, CurInst->Type);
			}
		else
			strcpy(instrs[x].name, CurInst->Sname);						/* Let's track the name anyways */
		}

	FreeMem(CurInst, sizeof(s3minstr));
}

/*********************************************************************/

void SetupS3MPatterns(void)
{
	uint16 PatPtrs[MAX_PATTERNS];									/* Pointers to Patterns */
	uint16 CurPkLen;												/* Length of pattern */
	uint8 FirstByte;												/* First Control uint8 */
	uint8 CurChn;													/* Current work channel */
	uint8 CurRow;													/* Current work row */
	uint8 *Buf;														/* Unpack data here */
	uint8 *StartBuf;
	uint8 i, j, k;


	SeekRawFile(s3mfile, (0x60+head.OrdNum+head.InsNum*2), FILESEEK_START);
	memset(PatPtrs, 0, (sizeof(uint16) * MAX_PATTERNS));
	ReadRawFile(s3mfile, &PatPtrs, (sizeof(uint16) * head.PatNum));									/* Copy to parapointers */

	for(i=0; i < head.PatNum; i++)
		Endian16(PatPtrs[i]);

	if ((Buf = AllocMem((sizeof(char) * 32000), MEMTYPE_NORMAL)) == NULL)							/* get temporary buffer */
		Abort("Memory Allocation fail in SetupS3MPatterns 02");


	StartBuf = Buf;

	for(i=0; i < head.PatNum; i++)												/* start unpacking patterns */
		{
		Buf = StartBuf;

		SeekRawFile(s3mfile, (PatPtrs[i] << 4), FILESEEK_START);				/* move to start of pack */
		ReadRawFile(s3mfile, &CurPkLen, sizeof(uint16));						/* get length of pack */
		Endian16(CurPkLen);

		memset(Buf, 0, (sizeof(char) * 32000));									/* Zero out the buffer */
		ReadRawFile(s3mfile, Buf, (CurPkLen * sizeof(uint8)));									/* load pack */

		if ((patterns[i] = AllocMem((sizeof(uint8) * 12288), MEMTYPE_NORMAL)) == NULL)					/* get temporary buffer */
			Abort("Memory Allocation fail in SetupS3MPatterns 03");
		memset(patterns[i], 0, (sizeof(uint8) * 12288));

		for(j=0; j < 64; j++)
			for(k=0; k < 32; k++)
				{
				patterns[i]->row[j].note[k].pitchoct = 0xFF;			/* pre-clear notes */
				patterns[i]->row[j].note[k].volume   = 0xFF;			/* and volumes*/
				}

		CurRow = 0;

		while (CurRow < 64)
			{
			FirstByte = *Buf++;											/* get control uint8 */
			if ((Buf-StartBuf) > 12288)
				Abort("S3M Pattern Length Overflow!");

			if (FirstByte == 0x0)
				CurRow++;
			else														/* If it is't zero, entry is here */
				{
				CurChn = FirstByte & 31;
				if (numchannels < CurChn )
					{
					if (CurChn >= MAX_CHANNELS)
						numchannels = (MAX_CHANNELS-1);
					else
						numchannels = CurChn;
					}


				if ((FirstByte & 32) == 32)								/* get note and instrument */
					{
					patterns[i]->row[CurRow].note[CurChn].pitchoct = *(Buf++);
					patterns[i]->row[CurRow].note[CurChn].instr = *(Buf++);
					}

				if ((FirstByte & 64) == 64)								/* get volume */
					patterns[i]->row[CurRow].note[CurChn].volume = *(Buf++);

				if ((FirstByte & 128) == 128)							/* get efx and info */
					{
					patterns[i]->row[CurRow].note[CurChn].command = *(Buf++);
					patterns[i]->row[CurRow].note[CurChn].info	= *(Buf++);
					}
				}
			}
		}
	Buf = StartBuf;

	FreeMem(Buf, (sizeof(char) * 32000));

}

/*********************************************************************/

void SetupS3MPanning(void)
{
	uint32 x;


	SeekRawFile(s3mfile, (0x60+head.OrdNum+head.InsNum*2+head.PatNum*2), FILESEEK_START);
	memset(channelpan, 0, (sizeof(uint8) * MAX_CHANNELS));
	ReadRawFile(s3mfile, &channelpan, (sizeof(uint8) * MAX_CHANNELS));		/* Copy to Settings */


	for (x=0; x < MAX_CHANNELS; x+=2)					/* Initialize the channel panning information */
		{
		channelpan[x] = 0x03;
		channelpan[(x+1)] = 0x0C;
		}
}

/*********************************************************************/

Item Play_Init(char *fname)
{
	MemCheck("Play Initiated");									/* Scavenges memory, then displays amount used */

	PlayDone = 0;
	sprintf(s3mfilename, "%s", fname);
	if (strlen(fname) < 2)
		Abort("Filename must contain more than 2 characters\n");

	ThisMusicTask = CreateThread(Play_InnerInit, "Music", (CURRENTTASK->t.n_Priority+10), 2048, NULL);	/* Backgnd Music */

	while(PlayDone != 99);												/* Kinda cheezy, but it get's the job done */
	MemCheck("Play Ready");										/* Scavenges memory, then displays amount used */

	return ThisMusicTask;
}

void Play_InnerInit(void)										/* Initialize replay and load up song */
{
	FileInfo	rawInfo;

	numchannels = 0;
	fxchannels = 0;
	globaltempo = 1.0;
	globalpitch = 1.0;
	globalvolume = 1.0;

	memset(&channels, 0, (sizeof(channel) * MAX_CHANNELS));		/* Zero out the channels data */

	if (OpenRawFile(&s3mfile, s3mfilename, FILEOPEN_READ) < 0)
		Abort("opening file\n");

	if (GetRawFileInfo(s3mfile, &rawInfo, sizeof(FileInfo)) < 0)
		Abort("Error getting file information\n");

	SeekRawFile(s3mfile, 0, FILESEEK_START);							/* move to start of pack */
	memset(&head, 0, sizeof(s3mhead));
	if (ReadRawFile(s3mfile, &head, sizeof(s3mhead)) < 0)				/* Read in the header */
		Abort("Reading file");

	Endian16(head.hole1);								/* This sucks, but I see not other easy alternative, do you? */
	Endian16(head.OrdNum);								/* Intel in a fit of lameness chose the weak endian format */
	Endian16(head.InsNum);
	Endian16(head.PatNum);
	Endian16(head.Flags);
	Endian16(head.Version);
	Endian16(head.FileForm);
	Endian16(head.hole2[0]);
	Endian16(head.hole2[1]);
	Endian16(head.hole2[2]);
	Endian16(head.hole2[3]);
	Endian16(head.Special);

	if (head.Byte1A != 0x1A)											/* Let's make sure it's the right thing */
		Abort("Bad File Type!");

	SeekRawFile(s3mfile, 0x60, FILESEEK_START);									/* move to start of packed data */
	memset(&ordlist, 0, (sizeof(uint8) * head.OrdNum));

	if (ReadRawFile(s3mfile, &ordlist, (sizeof(uint8) * head.OrdNum)) < 0)		/* Read in the order list */
		Abort("Reading file");

	if (OpenAudioFolio() < 0)											/* Open the audio for use */
		Abort("Couldn't open audio folio");


	SetupS3MPatterns();
	SetupS3MInstruments();
	SetupS3MPanning();

	CloseRawFile(s3mfile);												/* We're done with the rawfile */

	M2_Init();


	Play_Start();														/* Initialize the replay routine */
	PlayDone = 99;														/* Set it up to pause */
	Play_InnerLoop();													/* Go into the wait loop */
}

/*********************************************************************/

void S3M_VolSlide(int16 whichchannel)					/* Do a volume slide */
{
	uint8 lo = (uint8)channels[whichchannel].vsl & 0x0F;
	uint8 hi = (uint8)channels[whichchannel].vsl >> 4;

	if (lo == 0)										/* normal up */
		channels[whichchannel].vol += hi;

	if (hi == 0)										/* normal down */
		channels[whichchannel].vol -= lo;

	if (lo == 0x0F)										/* fine up */
		{
		if (hi != 0 && tick == 2)
			channels[whichchannel].vol += hi;
		}

	if (hi == 0x0F)										/* fine down */
		{
		if(lo != 0 && tick == 2)
			channels[whichchannel].vol -= lo;
		}

	if (channels[whichchannel].vol > 127)				/* Let's make sure we didn't break anything... */
		channels[whichchannel].vol = 0;

	if (channels[whichchannel].vol > 64)
		channels[whichchannel].vol = 64;
}

/*********************************************************************/

void S3M_ToneDown(int16 whichchannel)					/* Pitch-bender down */
{
	uint8 lo = (uint8)channels[whichchannel].tdn & 0x0F;
	uint8 hi = (uint8)channels[whichchannel].tdn >> 4;

	if (hi == 0x0F)
		{
		if (tick == 2)
			channels[whichchannel].pitch += (lo << 2);
		}
	else
		{
		if (hi == 0x0E)
			{
			if (tick == 2)
				channels[whichchannel].pitch += lo;
			}
		else
			{
			channels[whichchannel].pitch += (uint16)(channels[whichchannel].tdn << 2);
			}
		}
}

/*********************************************************************/

void S3M_ToneUp(int16 whichchannel)						/* Pitch-bender up */
{
	uint8 lo = (uint8)channels[whichchannel].tup & 0x0F;
	uint8 hi = (uint8)channels[whichchannel].tup >> 4;

	if (hi == 0x0F)
		{
		if (tick == 2)
			channels[whichchannel].pitch -= (lo << 2);
		}
	else
		{
		if (hi==0x0E)
			{
			if (tick == 2)
				channels[whichchannel].pitch -= lo;
			}
		else
			channels[whichchannel].pitch -= (uint16)(channels[whichchannel].tup << 2);
		}
}

/*********************************************************************/

void S3M_NotePort(int16 whichchannel)					/* Portamento */
{
	if (channels[whichchannel].pitch != channels[whichchannel].prtend)
		{
		if(channels[whichchannel].prtend > channels[whichchannel].pitch)
			{
			channels[whichchannel].pitch += (uint16)channels[whichchannel].prt << 2;
			if(channels[whichchannel].pitch > channels[whichchannel].prtend)
				channels[whichchannel].pitch = channels[whichchannel].prtend;
			}

		if(channels[whichchannel].prtend < channels[whichchannel].pitch)
			{
			channels[whichchannel].pitch -= (uint16)channels[whichchannel].prt << 2;
			if(channels[whichchannel].pitch < channels[whichchannel].prtend)
				channels[whichchannel].pitch = channels[whichchannel].prtend;
			}
		}
}

/*********************************************************************/

void S3M_Vibrato(int16 whichchannel)					/* Gotta have a little vibrato! */
{
	uint16 vt;
	dirtyperiod = 1;
	vt = vibsine[channels[whichchannel].sinepos % 31] * channels[whichchannel].vib & 0x0F;
	vt >>= 1;

	channels[whichchannel].sinepos += channels[whichchannel].vib >> 4;

	if(channels[whichchannel].sinepos < 32)
		channels[whichchannel].pitch = channels[whichchannel].tpitch + vt;
	else
		channels[whichchannel].pitch = channels[whichchannel].tpitch - vt;

	if(channels[whichchannel].sinepos > 63)
		channels[whichchannel].sinepos = 0;
}

/*********************************************************************/

void S3M_Retrig(int16 whichchannel)						/* Replay the same sound */
{
	channels[whichchannel].trgcnt++;
	if (channels[whichchannel].trgcnt == channels[whichchannel].trg)
		{
		M2_StopVoice(whichchannel);
		M2_PlayVoice(whichchannel, channels[whichchannel].instr, 0);
		channels[whichchannel].trgcnt = 0;
		}
}

/*********************************************************************/

void S3M_NoteCut(int16 whichchannel)					/* Immediately cut off the current sound */
{
	channels[whichchannel].cut--;
	if (channels[whichchannel].cut == 0)
		M2_StopVoice(whichchannel);
}

/*********************************************************************/

void S3M_NDelay(int16 whichchannel)						/* Delay (into) before play */
{
	/* Doesn't look like it's implimented, does it? */

	/* I haven't come up with an easy way to support this particular function, since it needs an delay INTO the sample
		before starting (difficult) */
}

/*********************************************************************/

void DoNextTick(void)									/* This is where all the real work happens */
{
	int16 NewRow=-1, NewOrd=-1;
	uint16 noteoffs;									/* Note offset */
	int16 chancnt;

	if (tick == speed)
		{
		tick = 1;

/*		if (PatRow == 0)
			printf("%2d %2d %2d\n", PatOrd, PatNum, PatRow); */

		for(chancnt=0; chancnt <= numchannels; chancnt++)
			{
			if (patterns[PatNum] == NULL)									/* Make sure we're dealing with valid data */
				break;

			channels[chancnt].efx = patterns[PatNum]->row[PatRow].note[chancnt].command;
			channels[chancnt].info = patterns[PatNum]->row[PatRow].note[chancnt].info;

			if (patterns[PatNum]->row[PatRow].note[chancnt].instr != 0)
				{
				channels[chancnt].instr = patterns[PatNum]->row[PatRow].note[chancnt].instr;
				channels[chancnt].ninst = 1;
				}
			else
				{
				channels[chancnt].ninst = 0;
				}

			if (patterns[PatNum]->row[PatRow].note[chancnt].pitchoct < 254)
				{
				channels[chancnt].octave = patterns[PatNum]->row[PatRow].note[chancnt].pitchoct >> 4;
				channels[chancnt].note =   patterns[PatNum]->row[PatRow].note[chancnt].pitchoct & 0x0F;

				if (channels[chancnt].efx != 0x07 )
					{
					channels[chancnt].pitch = instrs[channels[chancnt].instr].tune[channels[chancnt].note] >> channels[chancnt].octave;
					channels[chancnt].new = 1;
					}
				else
					{
					channels[chancnt].prtend = instrs[channels[chancnt].instr].tune[channels[chancnt].note] >> channels[chancnt].octave;
					channels[chancnt].vol = instrs[channels[chancnt].instr].volume;
					channels[chancnt].new = 0;
					channels[chancnt].ninst = 0;
					}
				}
			else
				{
				channels[chancnt].new = 0;

				if (patterns[PatNum]->row[PatRow].note[chancnt].pitchoct == 254)
					channels[chancnt].off = 1;
				}


			if ((channels[chancnt].new != 0) || (channels[chancnt].ninst != 0))
				{
				if (patterns[PatNum]->row[PatRow].note[chancnt].volume != 255)
					channels[chancnt].vol = patterns[PatNum]->row[PatRow].note[chancnt].volume;
				else
					channels[chancnt].vol = instrs[channels[chancnt].instr].volume;
				}
			else
				{
				if (patterns[PatNum]->row[PatRow].note[chancnt].volume != 255)
					channels[chancnt].vol = patterns[PatNum]->row[PatRow].note[chancnt].volume;
				}


			if (channels[chancnt].vol > 0)						/* As much as I like divide by 0 errors, let's skip a few */
				M2_SetVol(chancnt, channels[chancnt].vol);

			if ((channels[chancnt].new != 0) || (channels[chancnt].ninst != 0))
				{
				M2_StopVoice(chancnt);
				M2_SetFrq(chancnt, channels[chancnt].pitch);

				if (channels[chancnt].efx == 0x0F)								/* check for note offset */
					noteoffs = channels[chancnt].info << 8;
				else
					noteoffs = 0;

				M2_PlayVoice(chancnt, channels[chancnt].instr, noteoffs);
				channels[chancnt].tpitch = channels[chancnt].pitch;
				}
			else
				{
				if (channels[chancnt].off == 1)
					M2_StopVoice(chancnt);

				channels[chancnt].off = 0;
				}
																				/* now process "notetick" effects */

			switch(channels[chancnt].efx)
				{
				case (0x01):													/* Set Speed */
							speed = channels[chancnt].info;
							break;
				case (0x02):													/* Jump to order */
							NewRow = 0;
							NewOrd = channels[chancnt].info;
							while(ordlist[NewOrd] > MAX_PATTERNS)
								{
								NewOrd++;
								if (NewOrd >= head.OrdNum)
									NewOrd = 0;
								}
							break;
				case (0x03):													/* Break Pattern */
							NewRow = 63;
							break;
				case (0x0B):													/* Check VSL parameter */
				case (0x0C):
				case (0x04):
							if (channels[chancnt].info != 0)
								channels[chancnt].vsl = channels[chancnt].info;
							break;
				case (0x05):													/* TDN Parameter */
							if (channels[chancnt].info != 0)
								channels[chancnt].tdn = channels[chancnt].info;
							break;
				case (0x06):													/* TUP Parameter */
							if (channels[chancnt].info != 0)
								channels[chancnt].tup = channels[chancnt].info;
							break;
				case (0x07):
							if(channels[chancnt].info != 0)
								channels[chancnt].prt = channels[chancnt].info;
							break;
				case (0x08):
							if(channels[chancnt].info != 0)
								channels[chancnt].vib = channels[chancnt].info;
							channels[chancnt].tpitch = channels[chancnt].pitch;
							break;
				case (0x11):
							if(channels[chancnt].info != 0)
								channels[chancnt].trg = channels[chancnt].info;
							channels[chancnt].trgcnt = 0;
							break;
				case (0x13):
							switch(channels[chancnt].info >> 4)
								{
								case (0x08):
											channelpan[chancnt] = channels[chancnt].info & 0x0F;
											M2_SetPan(chancnt, channelpan[chancnt]);
											break;
								case (0x0C):
											channels[chancnt].cut = channels[chancnt].info & 0x0F;
											break;
								case (0x0D):
											channels[chancnt].dly = channels[chancnt].info & 0x0F;
											break;
								}
							break;
				case (0x14):													/* Set Tempo (reset timer) */
							tempo = channels[chancnt].info;
							break;
				case (0x16):													/* Set Global Volume */
							globalvolume = (channels[chancnt].info/64);
							break;
				}
			}

		if (NewRow >= 0)														/* If we had a command to jump, we jump! */
			PatRow = NewRow;

		if (PatRow < 63 && NewOrd < 0)											/* Watch for the end of pattern */
			{
			PatRow++;
			}
		else
			{
			PatRow = 0;

			if (NewOrd >= 0)
				PatOrd = NewOrd;
			else
				PatOrd++;

			while(ordlist[PatOrd] > MAX_PATTERNS)
				{
				if (PatOrd > head.OrdNum || (ordlist[PatOrd] == 0xFF))
					{
					PatOrd = 0;
					PlayDone = TRUE;												/* Let the app know it's all over */
					break;
					}
				PatOrd++;
				}

			PatNum = ordlist[PatOrd];
			}
		}
	else																		/* mean if (tick != speed) */
		{
		for (chancnt=0; chancnt <= numchannels; chancnt++)						/* process effects */
			{
			switch (channels[chancnt].efx)
				{
				case (0x04):													/* Volume Slide */
							S3M_VolSlide(chancnt);
							break;
				case (0x05):													/* Tone slide down */
							S3M_ToneDown(chancnt);
							break;
				case (0x06):													/* Tone slide up */
							S3M_ToneUp(chancnt);
							break;
				case (0x07):													/* Note portamento */
							S3M_NotePort(chancnt);
							break;
				case (0x08):													/* Note Vibrato */
							S3M_Vibrato(chancnt);
							break;
				case (0x11):													/* Retrigger Sample */
							S3M_Retrig(chancnt);
							break;
				case (0x0B):													/* Slide with vibrato */
							S3M_VolSlide(chancnt);
							S3M_Vibrato(chancnt);
							break;
				case (0x0C):													/* Slide with portamento */
							S3M_VolSlide(chancnt);
							S3M_NotePort(chancnt);
							break;
				case (0x13):													/* Cut or delay sound */
							switch(channels[chancnt].info >> 4)
								{
								case (0x0C):
											S3M_NoteCut(chancnt);
											break;
								case (0x0D):
											S3M_NDelay(chancnt);
											break;
								}
							break;
				}

			M2_SetVol(chancnt, channels[chancnt].vol);
			M2_SetFrq(chancnt, channels[chancnt].pitch);

			if (dirtyperiod & 0x01)
				channels[chancnt].pitch = channels[chancnt].tpitch;
			else
				channels[chancnt].tpitch =channels[chancnt].pitch;

			dirtyperiod  = 0;
			}
		tick++;
		}
}

/*********************************************************************/

void Play_InnerLoop(void)								/* This is where we sit the other part of the time... how exciting! */
{
	AudioTime tmrStart=0;												/* Track the init'd time */
	Item tmrCue;														/* This is the cue for waking up */
	Item tmrClock;														/* This is the audio clock to sync to */

	if ((tmrClock = CreateAudioClock(NULL)) <= 0)						/* Create a new audio clock for the music */
		return;															/* We're screwed, and I mean totally */
	if ((tmrCue = CreateCue(NULL)) <= 0)								/* Create a new audio cue for the music */
		return;															/* We're screwed, and I mean totally */

	if (SetAudioClockRate(tmrClock, (float32)6000.0) < 0)						/* Set the new clock rate */
		printf("Unable to set new Audio Clock Rate (This is BAD).\n");

	do		{
		while (PlayDone != FALSE)							/* This is here for those handy waits */
			{
			tick = speed;
			ReadAudioClock(tmrClock, &tmrStart);										/* Reset and start timing from here */
			SleepUntilAudioTime(tmrClock, tmrCue, (tmrStart + (MAX_TEMPO - tempo)));	/* Wait for something to happen */
			}

		ReadAudioClock(tmrClock, &tmrStart);			/* Reset and start timing from here */

		DoNextTick();									/* Play 1 tick of the module ((1/speed) tempo count) */

		SleepUntilAudioTime(tmrClock, tmrCue, (tmrStart + (MAX_TEMPO - (tempo*globaltempo))));	/* Wait the remainder of the tick */
		}
	while(PlayDone != TRUE);

	DeleteAudioClock(tmrClock);							/* What can I say?  I try to be clean! */
	DeleteCue(tmrCue);

	Play_InnerClean();									/* Clean up all the loose variables */
}

/*********************************************************************/
/*
 *			These routines are the bulk of what you'll call when using
 *			the MP codebase.  Almost all of the routines above this
 *			are used for internal processing...
 */
/*********************************************************************/

void Play_Start(void)									/* Dude, no music is lame, let's hear something! */
{
	int16 x;

	tempo = head.InitTem;
	speed = head.InitSpd;
	globalvolume = ((float32)head.GlobVol/64.0);
	tick  = speed;
	PatRow = 0;
	PatOrd = 0;
	PatNum = ordlist[PatOrd];

	for (x=0; x <= numchannels; x++)					/* Initialize the channel panning information */
		M2_SetPan(x, channelpan[x]);					/* Sorta Left */

	PlayDone = FALSE;
}


void Play_Pause(void)									/* Toggle replay */
{
	int32 x;

	if (PlayDone != 99)									/* Let's pause the replay */
		{
		PlayDone = 99;
		for (x=0; x <= numchannels; x++)
			M2_StopVoice(x);
		}
	else
		PlayDone = FALSE;
}

void Play_Stop(void)									/* Stop the music! */
{
	if (LookupItem(ThisMusicTask) == NULL)
		return;

	PlayDone = TRUE;

	while (LookupItem(ThisMusicTask) != NULL);
	MemCheck("Play Removed");							/* Scavenges memory, then displays amount used */
}


void Play_InnerClean(void)
{
	int16 x;

	if (PlayDone != TRUE)
		PlayDone = TRUE;

	for(x=0 ; x < head.PatNum; x++)
		FreeMem(patterns[x], (sizeof(uint8) * 12288));

	M2_Remove();

}

/*********************************************************************/
/*
 *			These routines are here just to sort of give you an idea
 *			of how you can effect the playback of the song in real-time
 *			Other implimentations could easily be made...
 */
/*********************************************************************/


void Play_ChangeTempo(float32 multiplier)							/* Changes the tempo (global) */
{
	if (multiplier >= 0 && multiplier <= 2)							/* Make sure it's in an acceptable range */
		globaltempo = multiplier;
}

void Play_ChangePitch(float32 multiplier)							/* Changes the pitch (global) */
{
	if (multiplier >= 0 && multiplier <= 2)							/* Make sure it's in an acceptable range */
		globalpitch = multiplier;
}

void Play_ChangeVolume(float32 multiplier)							/* Changes the volume (global) */
{
	int32 x;

	if (multiplier >= 0 && multiplier <= 2)							/* Make sure it's in an acceptable range */
		{
		globalvolume = multiplier;

		for (x=0; x <= numchannels; x++)							/* Initialize the channel panning information */
			M2_SetPan(x, channelpan[x]);										/* Sorta Left */
		}
}

void Play_FastForward(void)										/* Just like a tape deck, only cooler */
{
	tick = speed;

	if (PatRow < 63)											/* Watch for the end of pattern */
		PatRow++;
	else
		{
		PatRow = 0;
		PatOrd++;

		while(ordlist[PatOrd] > MAX_PATTERNS)
			{
			if (PatOrd > head.OrdNum || (ordlist[PatOrd] == 0xFF))
				{
				PatOrd = 0;
				PlayDone = TRUE;												/* Let the app know it's all over */
				break;
				}
			PatOrd++;
			}

		PatNum = ordlist[PatOrd];
		}
}

void Play_Rewind(void)
{
	tick = speed;

	if (PatRow > 0)											/* Watch for the end of pattern */
		PatRow-=2;
	else
		{
		PatRow = 63;

		if (PatOrd > 0)
			PatOrd--;

		while(ordlist[PatOrd] > MAX_PATTERNS)
			{
			if (PatOrd <= 0 || (ordlist[PatOrd] == 0xFF))
				{
				PatOrd = 0;
				PlayDone = TRUE;												/* Let the app know it's all over */
				break;
				}
			PatOrd--;
			}

		PatNum = ordlist[PatOrd];
		}
}

/*********************************************************************/
/*
 *			These routines are here just to sort of give you an idea
 *			of how to synchronize to the music... The best method
 *			would be to impliment your variables as global and put
 *			them in the M2_Play/Stop routines...
 */
/*********************************************************************/

int16 Play_GetFlash(void)								/* This is NOT the best way to do it... */
{
	if (PatRow == 0 || PatRow == 8 || PatRow == 16 || PatRow == 24 || PatRow == 32 || PatRow == 40 || PatRow == 48 || PatRow == 56)
		return PatRow;
	else
		return -1;
}

int16 Play_GetBigFlash(void)								/* This is NOT the best way to do it... */
{
	if (PatRow == 0)
		return PatNum;
	else
		return -1;
}



void Play_GetVoice(int16 voice, int16 *pitch, int16 *volume)	/* Just a very basic way of showing sychronization */
{
	if (((channels[voice].new != 0) || (channels[voice].ninst != 0)) ||		/* Something is being played */
		(instrs[channels[voice].instr].loop != 0))							/* Does it loop? */
		{
		*volume = channels[voice].vol;					/* And here's the values */
		*pitch = channels[voice].pitch;
		}
	else
		{
		*volume = 0;									/* And here's nothing */
		*pitch = 0;
		}
}

void Play_GetInst(int16 inst, int16 *pitch, int16 *volume)	/* Another very basic way of showing sychronization */
{
	int32 x;
	
	for (x=0; x < MAX_CHANNELS; x++)
		{
		if (((channels[x].new != 0) || (channels[x].ninst != 0)) &&				/* Something is being played */
			(channels[x].instr == inst))										/* and it's what we want */
			{
			*volume = channels[x].vol;					/* Your mission was a glorious success */
			*pitch = channels[x].pitch;
			return;
			}
		}

	*volume = 0;										/* You have failed */
	*pitch = 0;
}


/*********************************************************************/

void Play_PrintInfo(void)								/* Print out info about the current song */
{
	printf("\n\n##### MEGAPLAY v%s ##############################################\n\n", MEGA_PLAY_VERSION);
	printf("Name [%s]\n", head.Sname);
	printf("Type [%c%c%c%c (%d) V%d.%.2d]\n", head.SCRM[0], head.SCRM[1], head.SCRM[2], head.SCRM[3], head.Type, ((head.Version >> 8) & 0x0F), (head.Version & 0xFF));
	printf("Info [FFI = %d  Vol = %d]\n", head.FileForm, head.GlobVol);
	printf("Time [Tempo = %d  Speed = %d]\n", head.InitTem, head.InitSpd);
	printf("Stat [Length = %d  Inst = %d  Patterns = %d]\n", head.OrdNum, head.InsNum, head.PatNum);
	printf("\n###################################################################\n\n");
}

void Play_PrintSamples(void)							/* Print out the list of samples, and some related info */
{
	int32 x;

	printf("\n\n##### MEGAPLAY v%s ##############################################\n\n", MEGA_PLAY_VERSION);

	printf("NUM..DESCRIPTION..................FG....C2.VOL.LENGTH.LOOPST.LOOPND\n");
	for(x=0; x < MAX_INSTRUMENTS; x++)
		{
		if (instrs[x].length > 0)
			printf("%.2d [%-28s] %2d %5d %3d %6d %6d %6d\n", x, instrs[x].name, instrs[x].loop, instrs[x].c2spd, instrs[x].volume, instrs[x].length, instrs[x].loopstart, instrs[x].loopend);
		else
			if (strlen(instrs[x].name) > 0)
				printf("%.2d [%-28s]\n", x, instrs[x].name);
		}

	printf("\n###################################################################\n\n");
}

/*********************************************************************/
