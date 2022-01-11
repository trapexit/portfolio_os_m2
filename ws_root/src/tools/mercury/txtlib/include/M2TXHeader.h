/*
	File:		M2TXHeader.h

	Contains:	Prototypes for M2 Texture mapping library, Header Chunk 

	Written by:	Todd Allendorf, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <5>	  8/7/95	TMA		Set/Get for new flags HasNoDCI, HasNoLR, HasNoTAB, HasNoDAB.
		 <3>	 5/30/95	TMA		Made C++-friendly.
		 <2>	 5/17/95	TMA		Update headers for Set/GetTexFormat
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Accessors to the Header */
M2Err M2TXHeader_SetFlags(M2TXHeader *header, uint32 flags);
M2Err M2TXHeader_SetFIsCompressed(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasPIP(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasNoDCI(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasNoTAB(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasNoDAB(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasNoLR(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasColorConst(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetVersion(M2TXHeader *header, uint32 version);
M2Err M2TXHeader_SetMinXSize(M2TXHeader *header, uint16 size);
M2Err M2TXHeader_SetMinYSize(M2TXHeader *header, uint16 size);
M2Err M2TXHeader_SetTexFormat(M2TXHeader *header, M2TXFormat format);
M2Err M2TXHeader_SetCDepth(M2TXHeader *header, uint8 cDepth);
M2Err M2TXHeader_SetADepth(M2TXHeader *header, uint8 aDepth);
M2Err M2TXHeader_SetFIsLiteral(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasColor(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasAlpha(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetFHasSSB(M2TXHeader *header, bool flag);
M2Err M2TXHeader_SetNumLOD(M2TXHeader *header, uint8 num);
M2Err M2TXHeader_SetBorder(M2TXHeader *header, uint8 width);
M2Err M2TXHeader_SetColorConst(M2TXHeader *header, uint8 num, M2TXColor colorConst);
M2Err M2TXHeader_SetLODPtr(M2TXHeader *header, uint8 lod, uint32 length, M2TXTex tex);
M2Err M2TXHeader_GetLODDim(M2TXHeader *header, uint8 lod, uint32 *xSize, uint32 *ySize);
M2Err M2TXHeader_GetLODLength(M2TXHeader *header, uint8 lod, uint32 *length);
M2Err M2TXHeader_FreeLODPtr(M2TXHeader *header, uint8 lod);
M2Err M2TXHeader_FreeLODPtrs(M2TXHeader *header);

M2Err M2TXHeader_GetFlags(M2TXHeader *header, uint32 *flags);
M2Err M2TXHeader_GetFIsCompressed(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasPIP(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasNoDCI(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasNoTAB(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasNoDAB(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasNoLR(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasColorConst(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetSize(M2TXHeader *header, uint32 *size);
M2Err M2TXHeader_GetVersion(M2TXHeader *header, uint32 *version);
M2Err M2TXHeader_GetMinXSize(M2TXHeader *header, uint16 *size);
M2Err M2TXHeader_GetMinYSize(M2TXHeader *header, uint16 *size);
M2Err M2TXHeader_GetTexFormat(M2TXHeader *header, M2TXFormat *format);
M2Err M2TXHeader_GetFIsLiteral(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasColor(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasAlpha(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetFHasSSB(M2TXHeader *header, bool *flag);
M2Err M2TXHeader_GetCDepth(M2TXHeader *header, uint8 *cDepth);
M2Err M2TXHeader_GetADepth(M2TXHeader *header, uint8 *ADepth);
M2Err M2TXHeader_GetNumLOD(M2TXHeader *header, uint8 *num);
M2Err M2TXHeader_GetBorder(M2TXHeader *header, uint8 *border);
M2Err M2TXHeader_GetColorConst(M2TXHeader *header, uint8 num, M2TXColor *colorConst);
M2Err M2TXHeader_GetLODPtr(M2TXHeader *header, uint8 lod, uint32 *length, M2TXTex *tex );
M2Err M2TXHeader_GetDCIOffset(M2TXHeader *header, uint32 *offset);
M2Err M2TXHeader_GetPIPOffset(M2TXHeader *header, uint32 *offset);
M2Err M2TXHeader_GetLODOffset(M2TXHeader *header, uint8 lod, uint32 *offset);

#ifdef __cplusplus
}
#endif

