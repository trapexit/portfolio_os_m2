/***************************************************************
** Call Custom Functions for PForth
** This file can be modified to add custom user functions.
** You could, for example, call X11 from Forth.
**
** Author: Phil Burk
**
** Copyright 3DO and Phil Burk 1994
**
***************************************************************/

#include "AudioFolioInterface.h"
#include "pf_internal.h"
#include "pf_tools.h"

/****************************************************************
** Step 1: Put your own special glue routines here.
****************************************************************/
int32 CTest0( int32 Val )
{
	MSG_NUM_D("CTest0: Val = ", Val);
	return Val+1;
}

void CTest1( int32 Val1, cell Val2 )
{

	MSG("CTest1: Val1 = "); ffDot(Val1);
	MSG_NUM_D(", Val2 = ", Val2);
}

/****************************************************************
** Step 2: Fill out this table of function pointers.
**     Do not change the name of this table! It is used by the
**     PForth kernel.
****************************************************************/
void * CustomFunctionTable[] =
{
	(void *) CTest0,
	(void *) CTest1,
	(void *) pfOpenAudioFolio,
	(void *) pfCloseAudioFolio,
	(void *) pfLoadSample,
	(void *) pfDebugSample,
	(void *) pfSelectSamplePlayer,
	(void *) pfLoadInstrument,
	(void *) pfConnectInstruments,
	(void *) pfAttachSample,
	(void *) pfStartInstrument,
	(void *) pfReleaseInstrument,
	(void *) pfStopInstrument,
	(void *) pfUnloadSample,
	(void *) pfUnloadInstrument,
	(void *) pfGrabKnob,
	(void *) pfReleaseKnob,
	(void *) pfTweakKnob,
	(void *) pfDumpDSPPStats

};

/****************************************************************
** Step 3: Add them to the dictionary.
**     Do not change the name of this routine! It is called by the
**     PForth kernel.
****************************************************************/

int32 CompileCustomFunctions( void )
{
	int i=0;

/* Add them to the dictionary in the same order as above table. */
/* Parameters are: Name in UPPER CASE, Index, Mode, NumParams */
	CreateGlueToC( "CTEST0", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "CTEST1", i++, C_RETURNS_VOID, 2 );
	CreateGlueToC( "OPEN.AUDIO.FOLIO", i++, C_RETURNS_VALUE, 0 );
	CreateGlueToC( "CLOSE.AUDIO.FOLIO", i++, C_RETURNS_VALUE, 0 );
	CreateGlueToC( "LOAD.SAMPLE", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "DEBUG.SAMPLE", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "SELECT.SAMPLE.PLAYER", i++, C_RETURNS_VOID, 3 );
	CreateGlueToC( "LOAD.INSTRUMENT", i++, C_RETURNS_VALUE, 3 );
	CreateGlueToC( "CONNECT.INSTRUMENTS", i++, C_RETURNS_VALUE, 4 );
	CreateGlueToC( "ATTACH.SAMPLE", i++, C_RETURNS_VALUE, 2 );
	CreateGlueToC( "START.INSTRUMENT", i++, C_RETURNS_VALUE, 2 );
	CreateGlueToC( "RELEASE.INSTRUMENT", i++, C_RETURNS_VALUE, 2 );
	CreateGlueToC( "STOP.INSTRUMENT", i++, C_RETURNS_VALUE, 2 );
	CreateGlueToC( "UNLOAD.SAMPLE", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "UNLOAD.INSTRUMENT", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "GRAB.KNOB", i++, C_RETURNS_VALUE, 2 );
	CreateGlueToC( "RELEASE.KNOB", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "TWEAK.KNOB", i++, C_RETURNS_VALUE, 2 );
	CreateGlueToC( "DUMP.DSPP.STATS", i++, C_RETURNS_VOID, 0 );
	
	return 0;
}


/****************************************************************
** Step 4: Recompile and link with your code.
**         Then rebuild the Forth using "pforth -i"
**         Test:   10 Ctest0 ( should print message then '11' )
****************************************************************/
