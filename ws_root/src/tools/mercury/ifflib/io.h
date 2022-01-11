/*
	File:		io.h

	Contains:	 

	Written by:	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<2+>	 9/19/95	TMA		Removed :misc:iff.h

	To Do:
*/

/* @(#) io.h 95/06/22 1.1 */

#ifndef	__IO_H
#define	__IO_H


/*****************************************************************************/

#include "ifflib.h"

/*****************************************************************************/


#define IFFOpen(iff,k,m)  (* iff->iff_IOFuncs->io_Open)(iff,k,m)
#define IFFClose(iff)     (* iff->iff_IOFuncs->io_Close)(iff)
#define IFFRead(iff,b,s)  (* iff->iff_IOFuncs->io_Read)(iff,b,s)
#define IFFWrite(iff,b,s) (* iff->iff_IOFuncs->io_Write)(iff,b,s)
#define IFFSeek(iff,p)    (* iff->iff_IOFuncs->io_Seek)(iff,p)


/*****************************************************************************/


extern IFFIOFuncs fileFuncs;

#ifdef applec
extern IFFIOFuncs fileFuncsMac;
#endif

extern IFFIOFuncs memFuncs;


/*****************************************************************************/


#endif /* __IO_H */


