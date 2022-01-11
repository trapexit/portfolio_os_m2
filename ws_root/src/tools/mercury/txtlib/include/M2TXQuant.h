/*
	File:		M2TXQuant.h

	Contains:	M2 Texture Library, Color Quantization functions 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<2>	  8/4/95	TMA		Fixed typo in comments.
		 <1>	 5/30/95	TMA		first checked in
	To Do:
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _H_M2TXQuant
#define _H_M2TXQuant
typedef struct tag_colorspace {
  uint32 *wt;
  uint32 *mr;
  uint32 *mg;
  uint32 *mb;
} colorspace;

#endif

M2Err M2TXRaw_MapToPIP(M2TXRaw *raw, M2TXPIP *pip, int newColors, bool floyd);
M2Err M2TXRaw_MakePIP(M2TXRaw *raw, M2TXPIP *pip, int *numColors, bool remap);
M2Err M2TXRaw_MultiMakePIP(M2TXRaw *raw, M2TXPIP *pip, 
			   int *numColors, bool init, bool makePIP);
M2Err M2TXRaw_Hist3d(M2TXRaw *raw, colorspace *cs, uint16 **qadd, 
			    float *m2);

#ifdef __cplusplus
}
#endif

