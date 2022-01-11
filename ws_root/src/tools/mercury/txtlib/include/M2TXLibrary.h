/*
	File:		M2TXLibrary.h

	Contains:	M2 Texture Library 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<6+>	  8/4/95	TMA		Add M2TX_Free call.
		 <6>	 7/15/95	TMA		Update prototypes to match functions.
		 <4>	 7/11/95	TMA		Removed M2TX_GetWrapper, M2TX_GetTexel, M2TX_SetTexel. Added
									M2TXPIP_SetIndexOffset, M2TXPIP_GetIndexOFfset,
									M2TXPIP_GetNumColors, M2TXPIP_SetNumColors. M2TX_Init no returns
									an error code.
		 <3>	 5/30/95	TMA		Added M2TXFilter function prototypes. Made C++-friendly.
		 <2>	 4/13/95	TMA		Order of parameters in M2TXColor_Create and M2TXColor_Decode
									were mixed up.
		 <3>	 1/20/95	TMA		Update headers for new error-handling

	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

M2Err M2TX_Copy(M2TX *to, M2TX *from);
M2Err M2TX_GetHeader(M2TX *tex, M2TXHeader **header);
M2Err M2TX_SetHeader(M2TX *tex, M2TXHeader *header);
M2Err M2TX_GetPIP(M2TX *tex, M2TXPIP **pip);
M2Err M2TX_SetPIP(M2TX *tex, M2TXPIP *pip);   
M2Err M2TX_GetDCI(M2TX *tex, M2TXDCI **dci);
M2Err M2TX_SetDCI(M2TX *tex, M2TXDCI *dci);   
M2Err M2TX_SetDefaultXWrap(M2TX *tex, uint8 wrapMode);
M2Err M2TX_SetDefaultYWrap(M2TX *tex, uint8 wrapMode);


/* PIP Calls */

M2Err M2TXPIP_GetSize(M2TXPIP *pip, uint32 *size);
M2Err M2TXPIP_GetColor(M2TXPIP *pip, uint8 index, M2TXColor *color);
M2Err M2TXPIP_GetSortIndex(M2TXPIP *pip, uint8 index, uint8 *value);
M2Err M2TXPIP_SetColor(M2TXPIP *pip, uint8 index, M2TXColor color);
M2Err M2TXPIP_SetSortIndex(M2TXPIP *pip, uint8 index, uint8 value);
M2Err M2TXPIP_MakeTranslation(M2TXPIP *oldPIP, M2TXPIP *newPIP, uint8 cDepth);
M2Err M2TXPIP_SetNumColors(M2TXPIP *pip, int16 value);
M2Err M2TXPIP_GetNumColors(M2TXPIP *pip, int16 *value);
M2Err M2TXPIP_GetIndexOffset(M2TXPIP *pip, uint32 *value);
M2Err M2TXPIP_SetIndexOffset(M2TXPIP *pip, uint32 value);


/* Color functions */

M2TXColor M2TXColor_Create(uint8 ssb, uint8 alpha, uint8 red, uint8 green, uint8 blue);
void M2TXColor_Decode(M2TXColor color, uint8 *ssb, uint8 *alpha, uint8 *red, uint8 *green, uint8 *blue);
uint8 M2TXColor_GetSSB(M2TXColor color);
uint8 M2TXColor_GetAlpha(M2TXColor color);
uint8 M2TXColor_GetRed(M2TXColor color);
uint8 M2TXColor_GetGreen(M2TXColor color);
uint8 M2TXColor_GetBlue(M2TXColor color);

/* Raw calls */
M2Err M2TXRaw_GetColor(M2TXRaw *raw, uint32 index, M2TXColor *color);
M2Err M2TXRaw_Free(M2TXRaw *raw);
M2Err M2TXRaw_Alloc(M2TXRaw *raw, uint32 xSize, uint32 ySize);
M2Err M2TXRaw_Init(M2TXRaw *raw, uint32 xSize, uint32 ySize, 
					bool hasColor, bool hasAlpha, bool hasSSB);
M2Err M2TXRaw_FindPIP(M2TXRaw *raw, M2TXPIP *newPIP, uint8 *cDepth,
					  bool matchColor, bool matchAlpha, bool matchSSB);
M2Err M2TXRaw_LODCreate(M2TXRaw *rawIn, uint32 sample, uint16 channels, M2TXRaw *rawOut);

/* Index calls */
M2Err M2TXIndex_Free(M2TXIndex *raw);
M2Err M2TXIndex_Alloc(M2TXIndex *raw, uint32 xSize, uint32 ySize);
M2Err M2TXIndex_Init(M2TXIndex *index, uint32 xSize, uint32 ySize,
					bool hasColor, bool hasAlpha, bool hasSSB);
M2Err M2TXIndex_ReorderToPIP(M2TXIndex *data, M2TXPIP *pip, uint8 cDepth); 
M2Err M2TXIndex_LODCreate(M2TXIndex *indexIn, uint32 sample, uint16 channels, M2TXIndex *indexOut);

/* Filtering calls */
void M2TXFilter_Reset();
double M2TXFilter_SetWidth(uint32 filter, double width);
double M2TXFilter_GetWidth(uint32 filter);

/* Convenience Calls */
M2Err M2TX_Free(M2TX *tex, bool freeLODs, bool freeTAB, bool freeDAB,
		bool freeLRs);
M2Err M2TX_Init(M2TX *tex);
bool  M2TX_IsLegal(uint8 cDepth, uint8  aDepth, uint8 ssbDepth, bool isLiteral);
M2Err M2TX_Extract(M2TX *tex, uint8 numLOD, uint8 coarseLOD, uint16 xUL, uint16 yUL,
					 uint16 xSize, uint16 ySize, M2TX **newTex);
void M2TX_Print(M2TX *tex);

#ifdef __cplusplus
}
#endif
