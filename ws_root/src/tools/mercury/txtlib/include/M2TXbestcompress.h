/*
	File:		M2TXbestcompress.h

	Contains:	M2 Compression determination algorithm headers 

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

M2Err M2TXIndex_FindBestDCIPIP(M2TXIndex *myIndex,  M2TX *tex, M2TXDCI *dci, M2TXPIP *newPIP, 
							   uint8 depth, uint8 aDepth, bool hasSSB);
M2Err M2TXIndex_SmartFormatAssign(M2TXIndex *myIndex, M2TXIndex *format, 
									bool matchColor, bool matchAlpha, bool matchSSB,
									M2TXDCI *dci, M2TXPIP *pip);
M2Err M2TXIndex_FindBestDCI(M2TXIndex *myIndex, M2TX *tex, M2TXDCI *dci, uint8 depth,
							uint8 aDepth, bool hasSSB);

#ifdef __cplusplus
}
#endif
