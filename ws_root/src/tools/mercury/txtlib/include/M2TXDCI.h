/*
	File:		M2TXDCI.h

	Contains:	Prototypes for M2 Texture mapping library, DCI Chunk 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<1+>	 5/30/95	TMA		Made C++-friendly.
		<1+>	 1/16/95	TMA		Update headers to match new compression types.

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Accessors to DCI */
M2Err M2TXDCI_GetSize(M2TXDCI *dci, uint32 *size);
M2Err M2TXDCI_SetTexFormat(M2TXDCI *dci, uint8 format, uint16 texFormat);
M2Err M2TXDCI_SetFIsTrans(M2TXDCI *dci, uint8 format, bool flag);
M2Err M2TXDCI_SetCDepth(M2TXDCI *dci, uint8 format, uint8 cDepth);
M2Err M2TXDCI_SetADepth(M2TXDCI *dci, uint8 format, uint8 aDepth);
M2Err M2TXDCI_SetFIsLiteral(M2TXDCI *dci, uint8 format, bool flag);
M2Err M2TXDCI_SetFHasColor(M2TXDCI *dci, uint8 format, bool flag);
M2Err M2TXDCI_SetFHasAlpha(M2TXDCI *dci, uint8 format, bool flag);
M2Err M2TXDCI_SetFHasSSB(M2TXDCI *dci, uint8 format, bool flag);
M2Err M2TXDCI_SetColorConst(M2TXDCI *dci, uint8 num, M2TXColor colorConst);

M2Err M2TXDCI_GetTexFormat(M2TXDCI *dci, uint8 format, uint16 *texFormat);
M2Err M2TXDCI_GetFIsTrans(M2TXDCI *dci, uint8 format, bool *flag);
M2Err M2TXDCI_GetFIsLiteral(M2TXDCI *dci, uint8 format, bool *flag);
M2Err M2TXDCI_GetFHasColor(M2TXDCI *dci, uint8 format, bool *flag);
M2Err M2TXDCI_GetFHasAlpha(M2TXDCI *dci, uint8 format, bool *flag);
M2Err M2TXDCI_GetFHasSSB(M2TXDCI *dci, uint8 format, bool *flag);
M2Err M2TXDCI_GetCDepth(M2TXDCI *dci, uint8 format, uint8 *cDepth);
M2Err M2TXDCI_GetADepth(M2TXDCI *dci, uint8 format, uint8 *aDepth);
M2Err M2TXDCI_GetColorConst(M2TXDCI *dci, uint8 num, M2TXColor *colorConst);

#ifdef __cplusplus
}
#endif
