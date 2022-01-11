/*
	File:		qstream.c

	Contains:	This module contains functions which perform basic file i/o on the Mac 

	Written by:	Anthony Tai 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <5>	 9/18/95	TMA		Fixed bug in qFileSize.
		 <4>	  8/4/95	TMA		Changed the qGetFileSize function to be more platform
									independednt.
		 <2>	 7/11/95	TMA		#include files changed.
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/


#include "qGlobal.h"

#include <stdio.h>                
#include <stdlib.h>               
#include "qstream.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif


/**********************************************************************************************
 *	qOpenReadFile
 *
 *	the function is used to open a file
 *
 **********************************************************************************************/

FILE *qOpenReadFile
	(
	char *filename
	)
{
    FILE *theFile;            /* Pointer to the input FILE stream */

    if ((theFile = fopen(filename, "rb")) == (FILE *) NULL)
    {
    /*    fprintf(stderr, "GIFHEAD: Cannot open file %s\n", filename); */
        return(NULL);
    }
    return(theFile);
}

/**********************************************************************************************
 *	qCloseReadFile
 *
 *	the function is used to close a file
 *
 **********************************************************************************************/

void qCloseReadFile
	(
	FILE *theFile
	)
{
	fclose(theFile);
}

/**********************************************************************************************
 *	qGetFileSize
 *
 *	the function is used to get the file size
 *
 **********************************************************************************************/

long qGetFileSize
	(
	FILE *theFile
	)
{
	long pos, epos;

	pos = ftell(theFile);
	fseek(theFile, 0, SEEK_END);
	epos = ftell(theFile);
	fseek(theFile, pos, SEEK_SET);
	return(epos);	
}

/**********************************************************************************************
 *	qReadByteStream
 *
 *	the function is used to get the file buffer
 *
 **********************************************************************************************/

void qReadByteStream
	(
	FILE *theFile,
	BYTE *ptr,
	long size
	)
{
	fread(ptr, sizeof(char), size, theFile);
}

/**********************************************************************************************
 *	qOpenWriteFile
 *
 *	the function is used to open a file for writing
 *
 **********************************************************************************************/

FILE *qOpenWriteFile
	(
	char *filename
	)
{
    FILE *theFile;            /* Pointer to the input FILE stream */

	theFile = fopen(filename, "wb");  
	
    if (theFile == NULL)	 
    {
     /*   fprintf(stderr, "GIFHEAD: Cannot open file %s\n", filename); */
        return(NULL);
    }
    return(theFile);
}

/**********************************************************************************************
 *	qWriteByteStream
 *
 *	the function is used to write the file buffer
 *
 **********************************************************************************************/

void qWriteByteStream
	(
	FILE *theFile,
	BYTE *ptr,
	long size
	)
{
	fwrite(ptr, sizeof(char), size, theFile);
}

/**********************************************************************************************
 *	qCloseWriteFile
 *
 *	the function is used to close a file
 *
 **********************************************************************************************/

void qCloseWriteFile
	(
	FILE *theFile
	)
{
	fclose(theFile);
}

