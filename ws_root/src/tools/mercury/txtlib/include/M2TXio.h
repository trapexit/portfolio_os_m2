/*
	File:		M2TXio.h

	Contains:	Prototypes for M2 Texture io functions	 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<3+>	 7/11/95	TMA		Removed obsolete prototypes for old UTF format.
		 <2>	 5/30/95	TMA		Made C++ friendly.
		 <4>	 3/31/95	TMA		Added  M2TX_WriteTXTR functions.
		 <3>	 3/26/95	TMA		Changed error codes, added memory and Mac FS I/O calls
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

M2Err M2TXRaw_WriteFile(char *fileName, M2TXRaw *tex);
M2Err M2TXRaw_WriteChannels(char *fileName, M2TXRaw *tex, uint16 channels);
M2Err M2TXRaw_ReadChannels(char *fileName, M2TXRaw *tex, uint16 channels);
M2Err M2TXPIP_WriteFile(char *fileName, uint16 entries, M2TXPIP *pip);	
M2Err M2TXIndex_WriteFile(char *fileName, M2TXIndex *tex);  

#ifdef __cplusplus
}
#endif


