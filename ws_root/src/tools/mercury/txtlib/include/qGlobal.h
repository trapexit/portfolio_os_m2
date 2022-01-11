/*
	File:		qGlobal.h

	Contains:	This file contains global prototypes 

	Written by:	Anthont Tai, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <4>	  8/7/95	TMA		Crapintosh bool type conflict
		 <3>	 7/11/95	TMA		Add check to see kernel memory types have already been declared
									to avoid redefinition.
		 <3>	 1/20/95	TMA		Removed OSErr and other error definitions that belong in M2Err.h
		<1+>	 1/16/95	TMA		Add new definitions to allow for platform independence

	To Do:
*/



#ifndef _H_qGlobal
#define _H_qGlobal

#ifndef NULL
#define NULL 0
#endif

typedef char			CHAR;
#ifndef XMD_H
typedef unsigned char	BYTE;
#endif
typedef unsigned short	WORD;
typedef long			LONG;
typedef unsigned long	DWORD;

#ifndef __KERNEL_TYPES_H
typedef unsigned char	uchar;
typedef long			int32;
typedef short			int16;
typedef unsigned long	uint32;
typedef unsigned short	uint16;
typedef unsigned char	uint8;
#if _HAS_BOOL_TYPE
#else
typedef unsigned char	bool;
#endif

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

#endif
#endif
