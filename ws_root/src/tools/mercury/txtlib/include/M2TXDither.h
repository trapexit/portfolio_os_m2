/*
	File:		M2TXDither.h

	Contains:	M2 Texture Library, Literal Dithering functions 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

M2Err M2TXRaw_DitherDown(M2TXRaw *raw, bool doColor, uint8 endDepth,
			 bool floyd);

#ifdef __cplusplus
}
#endif
