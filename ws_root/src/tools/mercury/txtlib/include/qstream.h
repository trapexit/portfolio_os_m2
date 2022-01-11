/*
	File:		qStream.h

	Contains:	This file contains prototylpe for qStreamA.c 

	Written by:	Anthont Tai, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/


#ifndef _H_qStream
#define _H_qStream

FILE *qOpenReadFile
	(
	char *filename
	);

void qCloseReadFile
	(
	FILE *theFile
	);

long qGetFileSize
	(
	FILE *theFile
	);

void qReadByteStream
	(
	FILE *theFile,
	BYTE *ptr,
	long size
	);

FILE *qOpenWriteFile
	(
	char *filename
	);

void qWriteByteStream
	(
	FILE *theFile,
	BYTE *ptr,
	long size
	);

void qCloseWriteFile
	(
	FILE *theFile
	);


#endif
