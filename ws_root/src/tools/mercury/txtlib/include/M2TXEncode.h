/*
	File:		M2TXEncode.h

	Contains:	M2 Texture Format data encoders 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<1+>	 5/30/95	TMA		Made C++-friendly.
		 <1>	 1/16/95	TMA		first checked in

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

M2Err M2TXIndex_Compress(M2TXIndex *myIndex, M2TXIndex *format, bool rle, 
								M2TXDCI *dci, uint32 *size, M2TXTex *newTexel);
M2Err M2TXRaw_Compress(M2TXRaw *myRaw, M2TXIndex *format, bool rle, 
								M2TXDCI *dci, uint32 *size, M2TXTex *newTexel);

#ifdef __cplusplus
}
#endif

