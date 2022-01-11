/*
	File:		M2Err.h

	Contains:	Definitions for error codes

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <2>	 3/26/95	TMA		Type OSErr removed for compatibility.
		 <1>	 1/20/95	TMA		first checked in

	To Do:
*/

#ifndef _H_M2Err
#define _H_M2Err

/* M2 Error Codes */
typedef int16 		M2Err;

#define M2_NoErr   	((M2Err)0) 			/* No error has occured, for backwards compatibility */
#define M2E_NoErr  	((M2Err)0) 			/* No error has occured, for backwards compatibility */
#define M2E_NoMem  	((M2Err)1)			/* Memory allocation failed */
#define M2E_NoFile 	((M2Err)2)			/* File doesn't exist */
#define M2E_BadFile	((M2Err)3)			/* File being read in does not match expected format */
#define M2E_Limit 	((M2Err)4)			/* Reached a limitation set in the program */
#define M2E_NotImpl ((M2Err)5)			/* Tried to use something not implemented yet */
#define M2E_Range  	((M2Err)6)			/* Parameter was out of range */
#define M2E_BadPtr 	((M2Err)7)			/* Passed a NULL or bad ptr when you shouldn't have */

/* M2 Error Severity Levels */

#define M2E_Info	((M2Err)0)			/* Something happened that you should be aware of */
#define M2E_Warning ((M2Err)1)			/* Something maybe wrong, not severe, check it out */
#define M2E_Severe 	((M2Err)2)			/* Wrong results have occurred, should probably abort */

#endif
