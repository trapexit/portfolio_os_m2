/* @(#) score_pimaploader.c 96/03/01 1.23 */
/* $Id: score_pimaploader.c,v 1.12 1995/03/14 23:58:54 peabody Exp $ */
/****************************************************************
**
** PIMap loader for score player.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 940413 WJB Split off from score.c
** 940727 WJB Added autodocs.
** 940812 WJB Cleaned up triple bangs in includes.
**            Added usage of SCORE_MIN|MAX_PRIORITY.
** 940921 PLB Added 'r' command for rate shift.
** 940921 PLB pimp_RateShift is now pimp_RateDivide.
** 950314 WJB Made DisableScoreMessages() apply to all score player messages.
** 951024 WJB Added -hook option.
**            Now using LoadScoreTemplate() instead of LoadInsTemplate().
****************************************************************/

#include <audio/audio.h>
#include <audio/musicerror.h>           /* ML_ERR_ */
#include <audio/parse_aiff.h>           /* LoadSample() */
#include <file/fileio.h>
#include <ctype.h>                      /* isspace() */
#include <stdlib.h>                     /* strtol() */
#include <string.h>                     /* strlen() */

#include "score_internal.h"             /* self */

/* -------------------- Macros */

#define	DBUG(x)        /* PRT(x) */
#define	DBUGALLOC(x)   DBUG(x)
#define	DBUGLOAD(x)    DBUG(x)


#define NUL ('\0')
#define EOL ('\r')
#define PIMAP_COMMENT_CHAR  (';')

/* -------------------- Local Functions */

    /* stream support */
static int32 ReadStreamLine( RawFile *str, char *pad, int32 PadSize );
static int32 StripTrailingBlanks( char *s );
static char *ParseWord( char *s, char **newp );


/* -------------------- PIMap Loader */

static bool IsSampleFileName (const char *fileName);

/******************************************************************
** Load a PIMap by parsing a text file.
******************************************************************/
 /**
 |||	AUTODOC -public -class libmusic -group Score -name PIMap
 |||	Program-Instrument Map file format.
 |||
 |||	  Description
 |||
 |||	    A PIMap file consists of any number of lines formatted as below. Comments
 |||	    must have a semicolon in the first character of the line. Blank lines are
 |||	    ignored.
 |||
 |||	  Format
 |||
 |||	    <program number> <name> [switches]
 |||
 |||	  Arguments
 |||
 |||	    These arguments are required for each line of a PIMap.
 |||
 |||	    <program number>
 |||	        MIDI program number to associate <name> with. The valid range is 1..128.
 |||	        The highest number allowed is determined from MaxNumPrograms argument of
 |||	        CreateScoreContext() for the ScoreContext to be associated with the
 |||	        PIMap.
 |||
 |||	    <name>
 |||	        Name of a sample, DSP instrument template, or patch template to
 |||	        associate with this program number. There can be multiple samples
 |||	        associated with a program number, but only one instrument or patch.
 |||
 |||	        If the first occurance of a program number has a patch or instrument
 |||	        name, that patch or instrument is associated with the program number.
 |||	        Subsequent samples can be associated with this program number, and are
 |||	        automatically attached to the instrument or patch template.
 |||
 |||	        If the first occurance of a program number has a sample name (file name
 |||	        extension is .aiff, .aifc, or .aif), then a suitable template is
 |||	        automatically chosen to play the sample. Subsequent samples can be
 |||	        attached to this template just as with patches and instrument templates.
 |||
 |||	  Switches
 |||
 |||	    Any number of these optional switches may appear after the required
 |||	    arguments.
 |||
 |||	    -b <MIDI note number>
 |||	        Sets base note of sample. Range is 0..127. Defaults to value from sample
 |||	        file.
 |||
 |||	    -d <cents>
 |||	        Detune value for sample in cents. Range is -100..100. Defaults to value
 |||	        from sample file.
 |||
 |||	    -f
 |||	        Causes automatic sample player template selection to use a fixed-rate
 |||	        template. If not specified, a variable-rate sample player is selected.
 |||
 |||	    -h <MIDI note number>
 |||	        Sets upper note limit for a sample belonging to a multi-sample program
 |||	        number. Range is 0..127. Defaults to value from sample file.
 |||
 |||	    -hook <name>
 |||	        Name of FIFO hook to attach sample to for instruments with multiple
 |||	        FIFOs.
 |||
 |||	    -l <MIDI note number>
 |||	        Sets lower note limit for a sample belonging to a multi-sample program
 |||	        number. Range is 0..127. Defaults to value from sample file.
 |||
 |||	    -m <num voices>
 |||	        Specifies the maximum number of voices to assign to this program number.
 |||	        Range is 1..127. Defaults to 1.
 |||
 |||	    -p <priority>
 |||	        Sets instrument priority for this program number. Range is 0..200.
 |||	        Defaults to 100.
 |||
 |||	    -r <rate divisor>
 |||	        Specifies the instrument execution rate division for this program
 |||	        number. Valid settings are 1, 2, and 8. Defaults to 1. See
 |||	        CreateInstrument() for more information on this.
 |||
 |||	  See Also
 |||
 |||	    LoadPIMap(), CreateInstrument(), LoadSample(), "Playing MIDI Scores" chapter
 |||	    of the Portfolio Programmer's Guide.
 **/
 /**
 |||	AUTODOC -public -class libmusic -group Score -name LoadPIMap
 |||	Loads a Program-Instrument Map (PIMap(@)) from a text file.
 |||
 |||	  Synopsis
 |||
 |||	    Err LoadPIMap (ScoreContext *scon, const char *fileName)
 |||
 |||	  Description
 |||
 |||	    This procedure reads the designated Program-Instrument Map file (PIMap
 |||	    file) and writes appropriate values to the specified score context's
 |||	    PIMap. It also assigns appropriate sampled-sound instruments to samples
 |||	    listed in the PIMap file and imports all instrument templates listed in
 |||	    the PIMap file.
 |||
 |||	    For information about the format of a PIMap(@) file, read the "Playing
 |||	    MIDI Scores" chapter of the Portfolio Programmer's Guide.
 |||
 |||	    Note that if you want to set a score context's PIMap entries directly,
 |||	    you can use SetPIMapEntry().
 |||
 |||	  Arguments
 |||
 |||	    scon
 |||	        Pointer to a ScoreContext data structure.
 |||
 |||	    fileName
 |||	        Pointer to the character string containing the name of the PIMap file.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libmusic.a V20.
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenAudioFolio()
 |||
 |||	  Associated Files
 |||
 |||	    <audio/patchfile.h>, libmusic.a, libspmath.a, System.m2/Modules/audio,
 |||	    System.m2/Modules/audiopatchfile, System.m2/Modules/iff
 |||
 |||	  See Also
 |||
 |||	    SetPIMapEntry(), UnloadPIMap(), PIMap(@)
 **/
int32 LoadPIMap ( ScoreContext *scon, char *FileName )
{
	RawFile *str = NULL;
	int32 Result;
	int32 LineNum = 0;
#define PADSIZE 128
	char pad[PADSIZE];

DBUG(("LoadPIMap: Attempt to open %s\n", FileName ));
	Result =  OpenRawFile( &str, FileName, FILEOPEN_READ);
	if (Result < 0)
	{
		ERR(("Couldn't open %s\n", FileName));
		goto cleanup;
	}

	while(1)
	{
			/* get next line */
		{
			int32 Len;      /* scratch */

			do
			{
				Len = ReadStreamLine( str, pad, PADSIZE );
DBUG(("Line = %s\n", pad));
				if (Len == -1) goto cleanup;
				CHECKRESULT(Len,"LoadPIMap: ReadStreamLine");
				LineNum++;
				Len = StripTrailingBlanks( pad );
			} while( (pad[0] == PIMAP_COMMENT_CHAR) || (Len == 0));   /* Comment char = ';'  OR blank line */
		}

			/* parse line */
		{
				/* word parsing state */
			char *p = &pad[0];

				/* extracted args used at bottom */
			int32 ProgramNum;
			const char *SampleName;         /* sample or instrument template name */
			const char *hookName = NULL;
			bool ifVariable = TRUE;
			TagArg SampleTags[5];
			int32 SampleTagIndex = 0;

				/* result */
			Item TemplateItem;

				/* get program number, convert to internal zero based format. */
			ProgramNum = strtol (p, &p, 0) - 1;
			if ((ProgramNum < 0) || (ProgramNum >= scon->scon_PIMapSize))
			{
				ERR(("PIMap line %d: Program Number out of range = %d\n",
					LineNum, ProgramNum+1));
				Result = ML_ERR_OUT_OF_RANGE;
				goto cleanup;
			}

			SampleName = ParseWord(p, &p);   /* Sample or Instrument name follows number */
			PRT(("%d = %s\n", ProgramNum+1, SampleName));

				/* Parse switches from command line. */
			while(p != NULL)
			{
				const char * const Com = ParseWord(p, &p);
				const int32 comLen = strlen (Com);
				const char *NumText;        /* scratch */
				int32 Temp;                 /* scratch */

					/* at end (empty string)? */
				if (comLen == 0) break;
DBUG(("Com = %s\n", Com));

				if (Com[0] != '-')
				{
					ERR(("PIMap line %d: LoadPIMap expects '-' before flags, got %s\n",
						LineNum, Com));
					Result = ML_ERR_BAD_FORMAT;
					goto cleanup;
				}

					/* if it's a single-character switch, use switch statement */
				if (comLen == 2) switch(Com[1])
				{
					case 'f':           /* fixed-rate */
DBUG(("Fixed rate.\n"));
						ifVariable = FALSE;
						break;

					case 'm':           /* max voices */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp < 0) || (Temp > 127))
						{
							ERR(("Max Voices out of range.\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						scon->scon_PIMap[ProgramNum].pimp_MaxVoices = (uint8)Temp;
DBUGALLOC(("Max Voices = %d.\n", Temp));
						break;

					case 'r':           /* instrument execution rate divisor */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp != 1) || (Temp != 2) || (Temp != 8))
						{
							ERR(("Execution Rate Divisor out of range. N = %d\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						scon->scon_PIMap[ProgramNum].pimp_RateDivide = (uint8)Temp;
DBUGALLOC(("Rate Shift = %d.\n", Temp));
						break;

					case 'p':           /* instrument priority */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp < SCORE_MIN_PRIORITY) || (Temp > SCORE_MAX_PRIORITY))
						{
							ERR(("Priority out of range.\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						scon->scon_PIMap[ProgramNum].pimp_Priority = (uint8)Temp;
DBUGALLOC(("Priority = %d.\n", Temp));
						break;

					case 'l':           /* lower note limit for sample */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp < 0) || (Temp > 127))
						{
							ERR(("Low note out of range.\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						SampleTags[SampleTagIndex].ta_Tag = AF_TAG_LOWNOTE;
						SampleTags[SampleTagIndex++].ta_Arg = (TagData)Temp;
DBUGLOAD(("Low note = %d.\n", Temp));
						break;

					case 'b':           /* base note for sample */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp < 0) || (Temp > 127))
						{
							ERR(("Base note out of range.\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						SampleTags[SampleTagIndex].ta_Tag = AF_TAG_BASENOTE;
						SampleTags[SampleTagIndex++].ta_Arg = (TagData)Temp;
DBUGLOAD(("Base note = %d.\n", Temp));
						break;

					case 'h':           /* upper note limit for sample */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp < 0) || (Temp > 127))
						{
							ERR(("High note out of range.\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						SampleTags[SampleTagIndex].ta_Tag = AF_TAG_HIGHNOTE;
						SampleTags[SampleTagIndex++].ta_Arg = (TagData)Temp;
DBUGLOAD(("High note = %d.\n", Temp));
						break;

					case 'd':           /* detune (in cents) for sample */
						NumText = ParseWord(p, &p);
						Temp = atoi( NumText );
						if ((Temp < -100) || (Temp > 100))
						{
							ERR(("Detune out of range.\n", Temp));
							Result = ML_ERR_OUT_OF_RANGE;
							goto cleanup;
						}
						SampleTags[SampleTagIndex].ta_Tag = AF_TAG_DETUNE;
						SampleTags[SampleTagIndex++].ta_Arg = (TagData)Temp;
DBUGLOAD(("Detune = %d.\n", Temp));
						break;

					default:
						ERR(("Bad option in PIMap = %c\n", Com[1] ));
						break;
				}
					/* otherwise, compare strings */
				else if (!strcasecmp (&Com[1], "hook")) {   /* hook name for sample */
					hookName = ParseWord(p,&p);
					DBUGLOAD(("Hook name = '%s'\n", hookName));
				}
				else {
					ERR(("Bad option in PIMap = %s\n", Com ));
				}
			}

/* Determine whether it is a sample or template name */
			if (IsSampleFileName (SampleName))
			{
				Item SampleItem;
				Item Attachment;

/* Load sample and attach it to Sampler Template */
				SampleItem = LoadSample(SampleName);
				if (SampleItem < 0)
				{
					ERR(("LoadPIMap failed to load sample = %s\n", SampleName));
					Result = SampleItem;
					goto cleanup;
				}

/* Set sample info. */
				if(SampleTagIndex > 0)
				{
					SampleTags[SampleTagIndex].ta_Tag = TAG_END;
					Result = SetAudioItemInfo( SampleItem, SampleTags );
					CHECKRESULT(Result,"LoadPIMap: SetAudioItemInfo");
				}

				if( scon->scon_PIMap[ProgramNum].pimp_InsTemplate > 0)
				{
					TemplateItem = scon->scon_PIMap[ProgramNum].pimp_InsTemplate;
					DBUG(("Multisample on %d\n", ProgramNum));
				}
				else
				{
					char InstrumentName[AF_MAX_NAME_SIZE];

					Result = SampleItemToInsName( SampleItem , ifVariable, InstrumentName, AF_MAX_NAME_SIZE );
					if (Result < 0)
					{
						ERR(("No instrument to play that sample.\n"));
						goto cleanup;
					}

DBUGLOAD(("Use instrument: %s for %s\n", InstrumentName, SampleName));
					TemplateItem = LoadInsTemplate (InstrumentName, NULL);
					if (TemplateItem < 0)
					{
						ERR(("LoadPIMap failed for %s\n", InstrumentName));
						Result = TemplateItem;
						goto cleanup;
					}
				}

				Attachment = CreateAttachmentVA (TemplateItem, SampleItem,
					hookName ? AF_TAG_NAME : TAG_NOP, hookName,
					AF_TAG_SET_FLAGS,         AF_ATTF_FATLADYSINGS,
					AF_TAG_AUTO_DELETE_SLAVE, TRUE,
					TAG_END);
				if( Attachment < 0 )
				{
					Result = Attachment;
					goto cleanup;
				}
			}
			else
			{
				TemplateItem = LoadScoreTemplate (SampleName);

				if (TemplateItem < 0)
				{
					ERR(("LoadPIMap failed for instrument %d = %s\n", ProgramNum+1, SampleName));
					Result = TemplateItem;
					goto cleanup;
				}
			}

			scon->scon_PIMap[ProgramNum].pimp_InsTemplate = TemplateItem;
		}
	}

cleanup:
		/* !!! this doesn't actually clean up after itself on failure */
	CloseRawFile(str);
	if(Result < 0)
	{
		ERR(("LoadPIMap line %d = %s\n", LineNum, pad));
	}

	return(Result);
}

static bool IsSampleFileName (const char *fileName)
{
	const char * const suffix = strrchr (fileName, '.');

	return suffix && (
		!strcasecmp (suffix, ".aiff") ||
		!strcasecmp (suffix, ".aifc") ||
		!strcasecmp (suffix, ".aif"));
}

/******************************************************************
** UNload stuff from Load PIMap
******************************************************************/
/**
 |||	AUTODOC -public -class libmusic -group Score -name UnloadPIMap
 |||	Unloads instrument templates loaded previously with PIMap
 |||	              file.
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadPIMap( ScoreContext *scon )
 |||
 |||	  Description
 |||
 |||	    This procedure is the inverse of LoadPIMap; it unloads any instrument
 |||	    templates specified in the score context's PIMap.  Any instruments
 |||	    created using those templates are freed when the templates are unloaded.
 |||
 |||	  Arguments
 |||
 |||	    scon
 |||	        Pointer to a ScoreContext data structure.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libmusic.a V20.
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenAudioFolio()
 |||
 |||	  Associated Files
 |||
 |||	    <audio/patchfile.h>, libmusic.a, System.m2/Modules/audio
 |||
 |||	  See Also
 |||
 |||	    LoadPIMap(), SetPIMapEntry()
 |||
**/
int32 UnloadPIMap ( ScoreContext *scon )
{
	int32 i;
#ifdef REAL_TIME_TRACE
	DumpTraceRecord();
#endif
	for (i=0; i<scon->scon_PIMapSize; i++)
	{
		if (scon->scon_PIMap[i].pimp_InsTemplate)
		{
			UnloadInsTemplate( scon->scon_PIMap[i].pimp_InsTemplate );
			scon->scon_PIMap[i].pimp_InsTemplate = 0;
		}
	}
	return 0;
}


/* -------------------- Stream support functions */

/*********************************************************************/
static int32 ReadStreamLine( RawFile *str, char *pad, int32 PadSize )
{
	int32 Len, i;
	char *s;
	int32   ifInsideComment = FALSE;

	s = pad;
	for(i=0; i<PadSize; )
	{
		Len = ReadRawFile ( str, s, 1);
		if (Len < 0)  return Len;

		if (Len == 0)
		{
			if( i == 0 )
			{
				pad[0] = NUL;
				return -1;
			}
			else
			{
				*s = NUL;
			}
		}

		if (*s == EOL)
		{
			*s = NUL;
		}

		if (*s == NUL) break;

		if (*s == PIMAP_COMMENT_CHAR)
		{
			ifInsideComment = TRUE;
		}

/* Once inside a comment, just eat characters until EOL or EOF */
		if( !ifInsideComment )
		{
			s++;
			i++;
		}
	}
	return (s-pad);
}

/**********************************************************************
** Remove trailing blanks from a string.
**********************************************************************/
static int32 StripTrailingBlanks( char *s )
{
	int32 Len;
	char *p;

	Len = strlen( s );
	p = s+Len-1;

	while( Len>0 )
	{
		if( isspace(*p) )
		{
			*p = NUL;
		}
		else
		{
			break;
		}
		Len--;
		p--;
	}
DBUG(("Strip returns %d\n", Len));
	return Len;
}

/*********************************************************************/
/* Return empty string if no words. */
/* *newp = NULL if no characters remaining. */
static char *ParseWord( char *s, char **newp )
{
	char *w;

	if(s==NULL)
	{
		ERR(("ParseWord: NULL STRING POINTER!\n"));
		return NULL;
	}

/* Skip leading blanks. */
	while(isspace(*s)) s++;
	w = s;  /* result */
/* Scan for next white space. */
	while(1)
	{
		if (*s == NUL)
		{
			*newp = NULL;
			break;
		}
		if (!isspace(*s))
		{
			s++;
		}
		else
		{
			*s = NUL;  /* NUL terminate returned name. */
			*newp = s+1;
			break;
		}
	}
	return w;
}

