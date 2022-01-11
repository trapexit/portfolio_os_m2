/*
	File:		M2TXcompress.h

	Contains:	M2 Texture routines API headers 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<1+>	 5/30/95	TMA		Made C++ friendly.
		 <3>	 1/20/95	TMA		Updated error handling and headers. Deleted obsolete headers.
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Working Conversion Calls */

M2Err M2TX_ComprToM2TXIndex(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXIndex *index);
M2Err M2TX_ComprToM2TXRaw(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw);
M2Err M2TX_UncomprToM2TXRaw(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw);
M2Err M2TX_UncomprToM2TXIndex(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXIndex *index);  
M2Err M2TXRaw_ToUncompr(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw);
M2Err M2TXIndex_ToUncompr(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXIndex *index);

/* Compression functions */
M2Err M2TX_Compress(M2TX *tex, uint8 lod, M2TXDCI *newDCI, M2TXPIP *newPIP, uint16 cmpType, uint32 *size,
					M2TXTex *newTexel);

#ifdef __cplusplus
}
#endif



