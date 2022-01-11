/******************************************************************************
**
**  @(#) rsmsg.c 95/05/26 1.1
**
******************************************************************************/
/**
	Reads a chunk from a cel
******************************************************************************/

#include <Types.h>
#include <Memory.h>
#include <Resources.h>
#include <Quickdraw.h>
#include <QDOffScreen.h>
#include <Controls.h>
#include <Dialogs.h>
#include <OSUtils.h>
#include <Packages.h>
#include <Errors.h>
#include <ToolUtils.h>
#ifdef THINK_C
#include <pascal.h>
#include <SetUpA4.h>
#endif
#include <StandardFile.h>
#include <String.h>

#include "ImageFile.h"
#include "operaHW.h"
#include "rSMG.h"


#define NIL					((unsigned long) 0)
typedef unsigned char *PInt8;
typedef unsigned short *PInt16;




/***************************************************/
static OSErr FileSeek(Int16 v3DORefNum,Int32 amt)
{
	/*-----*/
	OSErr err;
	/*-----*/
	err = SetFPos(v3DORefNum,fsFromMark,(Int32)amt);
	return(err);
}



/************************************************/
static OSErr ReadBlk(Int16 v3DORefNum,void*	to,Int16 cnt)
{
	/*-----*/
	Int32 count = cnt;
	OSErr err;
	/*-----*/
	err = FSRead(v3DORefNum,&count,to);
	return(err);
}



/************************************************/
 OSErr  ReadAChunk(Int16 v3DORefNum,char *chunkType,void **buffer)
{
	OSErr err;
	char id[4];
	Int32 blkSize;
	
	err = SetFPos(v3DORefNum,fsFromStart,0);
	if (err != noErr) return(err);
	while (err == noErr )
	{
		err= ReadBlk(v3DORefNum,id,sizeof(id));
		if (err != noErr) return(err);
		err = ReadBlk(v3DORefNum,&blkSize,sizeof(Int32)) ;
		if (err != noErr) return(err);

		if (strncmp(id,chunkType,(long)4) == 0)
		{
	 		if (*buffer == NIL)		// allocate the buffer
	 			*buffer = NewPtrClear(blkSize);
	 		if (*buffer == NIL) return(-1);		// couldn't create the buffer
	 		err = SetFPos(v3DORefNum,fsFromMark,-(2*sizeof(Int32)));
			err = ReadBlk(v3DORefNum,*buffer,blkSize);
			return(err);
		}
		else	
		{
			err=FileSeek(v3DORefNum,blkSize-(2*sizeof(Int32))) ;
			if (err != noErr) return(err);
		}
	}

}



