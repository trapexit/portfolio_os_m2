/*
	File:		M2TXcompress.c

	Contains:	M2 Texture Compression API calls 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<6+>	12/17/95	TMA		Fixed the DCI bug that caused the generation of incorrect values
									for DCI's that contained a transparent format
		 <6>	 7/15/95	TMA		Update autodocs and remove use of obsolete fields in M2TX.
		 <4>	 7/11/95	TMA		#include file changes
		 <3>	  6/9/95	TMA		Fixed alpha bug, lowest on bit is lost.
		 <2>	 5/16/95	TMA		Autodocs added. Alpha output bug from RawToUncompressed removed.
		 <6>	 3/31/95	TMA		Fixed literal output bug in M2TXIndex_ToUncompr.
		 <5>	 3/26/95	TMA		Functions now deal with the case where the input texture and the
									requested decode texture have different channels.  Functions now
									only allocate exactly the memory they need instead.
		 <4>	 2/19/95	TMA		Bug fixes and new compression options added
		 <3>	 1/20/95	TMA		Update error handling.  Change Uncompr_ToRaw/Index parameter
									list.
		<1+>	 1/16/95	TMA		Fixed compression so it works in all modes

	To Do:
*/

#include <stdlib.h>
#include "M2TXTypes.h"
#include "M2TXcompress.h"
#include "M2TXbestcompress.h"
#include "M2TXHeader.h"
#include "M2TXDCI.h"
#include "M2TXLibrary.h"
#include "M2TXEncode.h"
#include <stdio.h>
#include "qmem.h"
#include "qGlobal.h"

#define QUICKGET 1
#define QUICKPUT 1


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_ComprToM2TXRaw
|||	Take a level of detail from a compressed texture and decompress the data 
|||	in to an M2TXRaw image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ComprToM2TXRaw(M2TX *tex, M2TXPIP *pip, uint8 lod,
|||	                              M2TXRaw *raw)
|||	
|||	  Description
|||	
|||	    This function takes an M2TX texture pointed to by tex, a pip, and a 
|||	    level of detail index.  It decompresses the data and, using the pip,
|||	    creates an M2TXRaw image pointed to by raw.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The PIP used for decoding the M2TX texture.
|||	    lod
|||	        The level of detail to extract into raw.
|||	    raw
|||	        The pointer to the M2TXRaw image created.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_UncomprToM2TXRaw()
**/

M2Err M2TX_ComprToM2TXRaw(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw)
{
  MemQB_Vars;

	bool  matchColor, matchAlpha, matchSSB;
	bool hasColor[4];
	bool isTrans[4]; 
	bool hasAlpha[4], hasSSB[4];
	bool isLiteral[4];
	uint8 cDepth[4], aDepth[4],  ssbDepth[4];
	M2TXColor colorConst[4], color;
	uint16 xSize, ySize, minX, minY;
	M2TXRaw myRaw;
	uint32 pixels, i, size;
	uint8 ssb, red, green, blue, alpha;
	uint8 cssb[4], cred[4], cgreen[4], cblue[4], calpha[4];
	uint8 shift[4], aShift[4], index, numLOD;
	uint8 sb, al;
	uint8 txFormat, control, count, loop, runLength, j, k;
	bool isString;
	M2Err err;
	M2TXTex texel;
	M2TXHeader *header;
	M2TXDCI *dci;
	
	err = M2TX_GetHeader(tex,&header);
	err = M2TX_GetDCI(tex,&dci);
	err = M2TXHeader_GetLODPtr(header, lod, &size, &texel);
	err = M2TXHeader_GetMinXSize(header, &minX);
	err = M2TXHeader_GetMinYSize(header, &minY);
	err = M2TXHeader_GetNumLOD(header, &numLOD);
 	err = M2TXHeader_GetFHasColor(header, &matchColor);
 	err = M2TXHeader_GetFHasAlpha(header, &matchAlpha);
 	err = M2TXHeader_GetFHasSSB(header, &matchSSB);
	xSize = minX << (numLOD-lod-1);
	ySize = minY << (numLOD-lod-1);

	err = M2TXRaw_Init(&myRaw, xSize, ySize, matchColor, matchAlpha, matchSSB); 
	if (err != M2E_NoErr)
		return(err);
	pixels = xSize * ySize;	

	for (i=0; i<4; i++)
	{
		err = M2TXDCI_GetFIsTrans(dci, i, &(isTrans[i]));		
		err = M2TXDCI_GetFIsLiteral(dci, i, &(isLiteral[i]));
		err = M2TXDCI_GetFHasColor(dci, i, &(hasColor[i]));
		err = M2TXDCI_GetFHasAlpha(dci, i, &(hasAlpha[i]));
		err = M2TXDCI_GetFHasSSB(dci, i, &(hasSSB[i]));
		if (!hasColor[i])
			cDepth[i] = 0;
		else
		{
			err = M2TXDCI_GetCDepth(dci, i, &(cDepth[i]));
		}
	 	if (!hasAlpha[i])
			aDepth[i] = 0;
		else
		{
			err = M2TXDCI_GetADepth(dci, i, &(aDepth[i]));
			raw->HasAlpha = TRUE;
		}
		err = M2TXDCI_GetColorConst(dci, i, &(colorConst[i]));
		if (hasSSB[i]) 
		{
			ssbDepth[i] = 1;
		}
		else
		 	ssbDepth[i] = 0;

		if (!(isLiteral[i]))
		{	
			if (pip == NULL)
				return(M2E_BadPtr);		/* No PIP to decode the data with */
		}

		M2TXColor_Decode(colorConst[i], &(cssb[i]),&(calpha[i]),&(cred[i]),&(cgreen[i]),
						 &(cblue[i]));
		shift[i] = 8 - cDepth[i];
		aShift[i] = 8 - aDepth[i];
	}

#ifdef QUICKGET
	if (((uint32)texel)%4)
	  fprintf(stderr,"Whoops, texel in UtR not quad-byte=%x\n", texel);
	BeginMemQGetBits((uint32 *)texel);
#else
	BeginMemGetBits(texel);
#endif

	for (i=0; i<pixels;)
	{
#ifdef QUICKGET
	  MemQGetBits(8);
	  control = QBResult;
#else
	  control = qMemGetBits(8);
#endif
		txFormat = (control & 0xC0) >>6;
		if (isTrans[txFormat])
		{
			count=(control&0x3F)+1;
			color = pip->PIPData[cblue[txFormat]];
#ifdef QUICKGET
			QuickDecode(color, ssb, alpha, red, green, blue);
#else
			M2TXColor_Decode(color, &ssb,&alpha, &red, &green, &blue);
#endif
			alpha = calpha[txFormat];
			ssb = cssb[txFormat];
			isString = FALSE;
		}
		else
		{
			count=(control&0x1F)+1;
			isString = (control&0x20)>>5;
		}
		if (isString)
			loop = count;
		else
			loop = 1;
		for (j=0; j<loop; j++)
		{
			if(!(isTrans[txFormat]))
			{
				if (hasSSB[txFormat])
				  {
#ifdef QUICKGET
				    MemQGetBits(ssbDepth[txFormat]);
				    ssb = QBResult;
#else
				    ssb = qMemGetBits(ssbDepth[txFormat]);
#endif
				  }
				else
					ssb = cssb[txFormat];
				if (!(hasAlpha[txFormat]) || (aDepth[txFormat] == 0))
					alpha = calpha[txFormat];
				else		
				  {
#ifdef QUICKGET
				    MemQGetBits(aDepth[txFormat]);
				    alpha = oneExtend(aDepth[txFormat], QBResult);
#else
				    alpha = oneExtend(aDepth[txFormat], qMemGetBits(aDepth[txFormat]));
#endif
				  }
				if (!(hasColor[txFormat]) || (cDepth[txFormat] == 0))
				{
					red = cred[txFormat]; 
					green = cgreen[txFormat]; 
					blue = cblue[txFormat];
				}
				else
				{
					if (isLiteral[txFormat])
					{
#ifdef QUICKGET
					  MemQGetBits(cDepth[txFormat]);
					  red = oneExtend(cDepth[txFormat], QBResult);
					  MemQGetBits(cDepth[txFormat]);
					  green = oneExtend(cDepth[txFormat], QBResult);
					  MemQGetBits(cDepth[txFormat]);
					  blue = oneExtend(cDepth[txFormat], QBResult);
#else
					  red = oneExtend(cDepth[txFormat], qMemGetBits(cDepth[txFormat]));
					  green = oneExtend(cDepth[txFormat], qMemGetBits(cDepth[txFormat]));
					  blue = oneExtend(cDepth[txFormat], qMemGetBits(cDepth[txFormat]));
#endif
					}
					else
					{
#ifdef QUICKGET
					  MemQGetBits(cDepth[txFormat]);
					  index = QBResult;
#else
					  index = qMemGetBits(cDepth[txFormat]);
#endif
					  color = pip->PIPData[index];
#ifdef QUICKGET
					  QuickDecode(color, sb, al, red, green, blue);
#else
					  M2TXColor_Decode(color,&sb,&al,&red,&green,&blue);		
#endif
						if (!matchAlpha && (!(hasAlpha[txFormat])))
							alpha = al;
						if (!matchSSB && (!(hasSSB[txFormat])))
							ssb = sb;
					}
				}
				if (isString)
					runLength = 1;
				else
					runLength = count;
			}
			else
				runLength = count;
			for (k=0; k<runLength; k++)
			{
				if (matchColor)
				{
					myRaw.Red[i] = red; myRaw.Green[i] = green; myRaw.Blue[i] = blue;
				}
				if (matchAlpha)
					myRaw.Alpha[i] = alpha;
				if (matchSSB)
					myRaw.SSB[i] = ssb;
				i++;
			}
		}
	}
#ifdef QUICKGET
	EndMemQGetBits();
#else
	EndMemGetBits();
#endif
	*raw = myRaw;
	return(M2E_NoErr);
}

static M2Err GetIndex(uint8 ssb, uint8 alpha, uint8 red, uint8 green,
		      uint8 blue, M2TXPIP *pip, uint8 cDepth, uint8 *index,
		      bool matchColor, bool matchAlpha, bool matchSSB)
{
	int i, colors;
	M2TXColor color, colorPIP;
	uint32 mask;

	mask = 0;
	if (matchColor)
		mask += 0x00FFFFFF;
	if (matchAlpha)
		mask += 0x7F000000;
	if (matchSSB)
		mask += 0x80000000;

	colors = 2 << (cDepth-1);
#ifdef QUICKPUT
	color = QuickCreate(ssb, alpha, red, green, blue); 
#else
	color = M2TXColor_Create(ssb,alpha,red,green,blue);
#endif
	for (i=0; i<colors; i++)
	{
		colorPIP = pip->PIPData[i];
		if ((color & mask) == (colorPIP &mask))
		{
			*index = i;
			return(M2E_NoErr);
		}
	}
	return(M2E_Range);
}

/* Here how the precedence goes, if the expansion format is indexed (i.e. !IsLiteral)
 the color, alpha, and ssb come from the PIP values.  If the expansion format has
 alpha, then the alpha comes an imbedded alpha.  If the format has no alpha, the alpha
 value is taken from the constant color.  Similar for the ssb.
 The PIP's alpha and ssb are only used if the expansion format flags HasColor and
 HasAlpha are false.  Otherwise they are either embedded or come from the const color
*/

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_ComprToM2TXIndex
|||	Take a level of detail from a compressed texture and decompress the data
|||	in to an M2TXIndex image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_ComprToM2TXIndex(M2TX *tex, M2TXPIP *pip, uint8 lod, 
|||	                                M2TXIndex *index)
|||	
|||	  Description
|||	
|||	    This function takes an M2TX texture pointed to by tex, a pip, and a 
|||	    level of detail index.  It decompresses the data and, using the pip,
|||	    creates an M2TXIndex image pointed to by index.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The PIP used for decoding the M2TX texture.
|||	    lod
|||	        The level of detail to extract into index.
|||	    index
|||	        The pointer to the M2TXIndex image created.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_UncomprToM2TXIndex()
**/
M2Err M2TX_ComprToM2TXIndex(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXIndex *index)
{
  MemQB_Vars;
	bool  matchColor, matchAlpha, matchSSB;
	bool hasColor[4];
	bool isTrans[4]; 
	uint8 transIndex[4];
	bool hasAlpha[4], hasSSB[4];
	bool isLiteral[4];
	uint8 cDepth[4], aDepth[4],  ssbDepth[4];
	M2TXColor colorConst[4];
	uint16 xSize, ySize, minX, minY;
	M2TXIndex myIndex;
	uint32 pixels, i, size;
	uint8 ssb, alpha;
	uint8 cssb[4], cred[4], cgreen[4], cblue[4], calpha[4];
	uint8 shift[4], aShift[4], cIndex, numLOD;
	uint8 sb, al;
	uint8 txFormat, control, count, loop, runLength, j, k;
	bool isString;
	M2Err err;
	M2TXTex texel;
	M2TXHeader *header;
	M2TXDCI *dci;
	
	err = M2TX_GetHeader(tex,&header);
	err = M2TX_GetDCI(tex,&dci);
	err = M2TXHeader_GetLODPtr(header, lod, &size, &texel);
	err = M2TXHeader_GetMinXSize(header, &minX);
	err = M2TXHeader_GetMinYSize(header, &minY);
	err = M2TXHeader_GetNumLOD(header, &numLOD);
	err = M2TXHeader_GetFHasColor(header, &matchColor);
 	err = M2TXHeader_GetFHasAlpha(header, &matchAlpha);
 	err = M2TXHeader_GetFHasSSB(header, &matchSSB);

	xSize = minX << (numLOD-lod-1);
	ySize = minY << (numLOD-lod-1);
	al = 0xFF;
	sb = 0;
	M2TXIndex_Init(&myIndex, xSize, ySize, matchColor, matchAlpha, matchSSB); 
	pixels = xSize * ySize;	

	for (i=0; i<4; i++)
	{
		err = M2TXDCI_GetFIsTrans(dci, i, &(isTrans[i]));		
		err = M2TXDCI_GetFIsLiteral(dci, i, &(isLiteral[i]));
		err = M2TXDCI_GetFHasColor(dci, i, &(hasColor[i]));
		err = M2TXDCI_GetFHasAlpha(dci, i, &(hasAlpha[i]));
		err = M2TXDCI_GetFHasSSB(dci, i, &(hasSSB[i]));
		if (!hasColor[i])
		{
			cDepth[i] = 0;
		}
		else
		{
			err = M2TXDCI_GetCDepth(dci, i, &(cDepth[i]));
		}
	 	if (!hasAlpha[i])
	 	{
			aDepth[i] = 0;
		}
		else
		{
			err = M2TXDCI_GetADepth(dci, i, &(aDepth[i]));
		}
		err = M2TXDCI_GetColorConst(dci, i, &(colorConst[i]));
		if (hasSSB[i])
		{ 
			ssbDepth[i] = 1;
		}
		else
		{
		 	ssbDepth[i] = 0;		
		}
		if ((isLiteral[i]) || (pip == NULL))
		{	
			return(M2E_BadPtr);	 /* No PIP to decode the data with */
		}

		M2TXColor_Decode(colorConst[i],&(cssb[i]),&(calpha[i]),&(cred[i]),&(cgreen[i]),
						 &(cblue[i]));
		if (isTrans[i])
			transIndex[i] = cblue[i];
		shift[i] = 8 - cDepth[i];
		aShift[i] = 8 - aDepth[i];
	}
	cIndex = 0;

#ifdef QUICKGET
	if (((uint32)texel)%4)
	  fprintf(stderr,"Whoops, texel in UtR not quad-byte=%x\n", texel);
	BeginMemQGetBits((uint32 *)texel);
#else
	BeginMemGetBits(texel);	
#endif
	for (i=0; i<pixels;)
	{
#ifdef QUICKGET
	  MemQGetBits(8);
	  control = QBResult;
#else
	  control = qMemGetBits(8);
#endif
		txFormat = (control & 0xC0) >>6;
		if (isTrans[txFormat])
		{
			count=(control&0x3F)+1;
			cIndex = transIndex[txFormat];
			ssb=cssb[txFormat]; alpha=calpha[txFormat];
			isString = FALSE;
		}
		else
		{
			count=(control&0x1F)+1;
			isString = (control&0x20)>>5;
		}
		if (isString)
			loop = count;
		else
			loop = 1;
		for (j=0; j<loop; j++)
		{
		  if(!(isTrans[txFormat]))
		    {
		      if (hasSSB[txFormat])
			{
#ifdef QUICKGET
			  MemQGetBits(ssbDepth[txFormat]);
			  ssb = QBResult;
#else
			  ssb = qMemGetBits(ssbDepth[txFormat]);
#endif
			}
		      else
			ssb = cssb[txFormat];
		      if ((!(hasAlpha[txFormat])) || (aDepth[txFormat] == 0))
			alpha = calpha[txFormat];
		      else
			{
#ifdef QUICKGET
			  MemQGetBits(aDepth[txFormat]);
			  alpha = oneExtend(aDepth[txFormat], QBResult);
#else
			  alpha = oneExtend(aDepth[txFormat], qMemGetBits(aDepth[txFormat]));
#endif
			}
		      if ((!(hasColor[txFormat])) || (cDepth[txFormat] == 0))
			{
			  cIndex = 0;;
			}
		      else
			{
#ifdef QUICKGET
			  MemQGetBits(cDepth[txFormat]);
			  cIndex = QBResult;
#else
			  cIndex = qMemGetBits(cDepth[txFormat]);
#endif
			  if (!(hasAlpha[txFormat]))
			    alpha = al;
			  if (!(hasSSB[txFormat]))
			    ssb = sb;
			}
		      if (isString)
			runLength = 1;
		      else
			runLength = count;
		    }
		  else
		    runLength = count;
		  for (k=0; k<runLength; k++)
		    {
		      if (matchColor)
			myIndex.Index[i] = cIndex;
		      if (matchAlpha)
			myIndex.Alpha[i] = alpha; 
		      if (matchSSB)
			myIndex.SSB[i] = ssb;
		      i++;
		    }
		}
	}
#ifdef QUICKGET
	EndMemQGetBits();
#else
	EndMemGetBits();
#endif
	*index = myIndex;
	return(M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_Compress
|||	Take a level of detail from an existing texture, compress it, and return
|||	a pointer to it.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_Compress(M2TX *tex, uint8 lod, M2TXDCI *newDCI, 
|||	                        M2TXPIP *newPIP, uint16 cmpType, uint32 *size, 
|||	                        M2TXTex *newTexel)
|||	
|||	  Description
|||	    This function takes an M2TX texture, an index for a level of detail, and a
|||	    compression type.  It then compresses the level of detail and returns a
|||	    pointer to it, the DCI, and the new PIP that was used to compress it.  It
|||	    also returns the size of the compressed level of detail pointed to 
|||	    newTexel.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    newPIP
|||	        The new PIP that is used for compression (if a new PIP is allowed to
|||	        be created).
|||	    lod
|||	        The level of detail to compress.
|||	    newDCI
|||	        The new DCI that is used for compression (if a new DCI is allowed to
|||	        be created).
|||	    cmpType
|||	        The compression type to perform.
|||	    size
|||	        The size of the compressed level of detail.
|||	    newTexel
|||	        The pointer to the newly-created compressed level of detail.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

M2Err M2TX_Compress(M2TX *tex, uint8 lod, M2TXDCI *newDCI, M2TXPIP *newPIP, uint16 cmpType, uint32 *size,
					M2TXTex *newTexel)
{
	M2TX    tempTex;
	M2TXPIP *oldPIP, *usePIP;
	M2TXDCI *oldDCI;
	M2TXRaw myRaw;
	M2TXColor color;
	M2TXIndex myIndex, format, *cmpFormat;
	M2TXHeader	*header;
	uint8 cDepth, aDepth,  ssbDepth, depth, numLOD;
	uint32 xSize, ySize;
	uint16 minX, minY, txFormat,i;
	M2Err err;
	bool isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
	bool texelIsTemp, rle, bestDCI, bestPIP, customPIP, customDCI, lockPIP;
	
	err = M2TX_GetHeader(tex,&header);
	err = M2TX_GetPIP(tex,&oldPIP);
	err = M2TX_GetDCI(tex,&oldDCI);
 	err = M2TXHeader_GetFIsLiteral(header, &isLiteral);
	err = M2TXHeader_GetFIsCompressed(header, &isCompressed);
	err = M2TXHeader_GetMinXSize(header, &minX);
	err = M2TXHeader_GetMinYSize(header, &minY);
	err = M2TXHeader_GetNumLOD(header, &numLOD);
	err = M2TXHeader_GetFHasColor(header, &hasColor);
	err = M2TXHeader_GetFHasAlpha(header, &hasAlpha);
	err = M2TXHeader_GetFHasSSB(header, &hasSSB);
	if (hasColor)
		err = M2TXHeader_GetCDepth(header, &cDepth);
	else
		cDepth = 0;
	if (hasAlpha)
		err = M2TXHeader_GetADepth(header, &aDepth);
	else
		aDepth = 0;	
	if (hasSSB) 
		ssbDepth = 1;
	else
	 	ssbDepth = 0;
	 if (isLiteral)
	 	depth = ssbDepth + aDepth + 3*cDepth;
	 else
	 	depth = ssbDepth + aDepth + cDepth;	
	if (!isLiteral)
		if (oldPIP == NULL)
			return(M2E_BadPtr);		/* No PIP to decode the data with */

	xSize = minX << (numLOD-lod-1);
	ySize = minY << (numLOD-lod-1);
	texelIsTemp = FALSE;
	
	if (isCompressed)		/* Get it into uncompressed form for compression */
	{
		if (isLiteral)
			err = M2TX_ComprToM2TXRaw(tex, oldPIP, lod, &myRaw);
		else
			err = M2TX_ComprToM2TXIndex(tex, oldPIP, lod, &myIndex);			
	}
	else
	{
		if (isLiteral)
			err = M2TX_UncomprToM2TXRaw(tex, oldPIP, lod, &myRaw);
		else
			err = M2TX_UncomprToM2TXIndex(tex, oldPIP, lod, &myIndex);			
 	}
 	if (err != M2E_NoErr)
 		return (err);
 				

	*newTexel = NULL;
	*size = 0;
	cmpFormat = NULL;
	customDCI = bestDCI = bestPIP = customPIP = rle = lockPIP = FALSE;
	if (cmpType&M2CMP_RLE)
		rle = TRUE;
	if ((cmpType&M2CMP_BestDCIPIP) && (!isLiteral))
	{
		bestDCI = TRUE;
		bestPIP = TRUE;
	}
	if (cmpType&M2CMP_CustomPIP)
		customPIP = TRUE;
	if (cmpType&M2CMP_CustomDCI)	
		customDCI = TRUE;
	if (cmpType&M2CMP_Auto)			/* 1.04 */
	{
		rle = TRUE;
		if (!isLiteral)
		{
			bestDCI = TRUE;
			bestPIP = TRUE;
		}
	}
	if (cmpType&M2CMP_LockPIP)
	{
		bestPIP = customPIP = FALSE;
		lockPIP = TRUE;
	}
	if (bestPIP || customPIP)
		usePIP = newPIP;
	else
		usePIP = oldPIP;
	if (((!customDCI) && (!bestDCI)) || isLiteral) 		
	{ 	/* In case the user was sloppy and didn't copy the Format to the DCI, for RLE only 1.06 */
		M2TXHeader_GetTexFormat(header, &txFormat);
		M2TXHeader_GetColorConst(header, 0, &color);
		for (i=0; i<4; i++)
		{
			M2TXDCI_SetTexFormat(newDCI,i,txFormat);
			M2TXDCI_SetColorConst(newDCI, i, color);
		}
	}
	if (isLiteral)
	{
		/* Right now just do RLE */
		err = M2TXRaw_Compress(&myRaw, cmpFormat, rle, newDCI, size, newTexel);
		if (cmpFormat != NULL)
			err = M2TXIndex_Free(&format);							
		err = M2TXRaw_Free(&myRaw);
	}	
	else	
	{
		if (bestDCI && (!customDCI) && (!customPIP))
		{
			if ((!lockPIP))  	
				M2TXIndex_FindBestDCIPIP(&myIndex, tex, newDCI, newPIP, cDepth, aDepth, hasSSB);
			else
				M2TXIndex_FindBestDCI(&myIndex, tex, newDCI, cDepth, aDepth, hasSSB);
		}
		else if (bestPIP || customPIP)
			M2TXIndex_ReorderToPIP(&myIndex, newPIP, cDepth);		
		if (bestDCI || customDCI)
		{
			if (usePIP == NULL)					/* 1.05 */
				return(M2E_BadPtr);						/* 1.05 */
			M2TXIndex_Init(&format, myIndex.XSize, myIndex.YSize, TRUE, TRUE, TRUE);
			err = M2TXIndex_SmartFormatAssign(&myIndex, &format, hasColor, hasAlpha,
											  hasSSB, newDCI, usePIP);
			if (err != M2E_NoErr)
				return (err);
			cmpFormat = &format;
		}
		err = M2TXIndex_Compress(&myIndex, cmpFormat, rle, newDCI, size, newTexel);
		if (cmpFormat != NULL)
			err = M2TXIndex_Free(&format);							
		err = M2TXIndex_Free(&myIndex);							
	} 

	if (texelIsTemp)
		M2TXHeader_FreeLODPtr(&(tempTex.Header),lod);
		
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_UncomprToM2TXRaw
|||	Take a level of detail from an uncompressed texture and copy the data 
|||	into an M2TXRaw image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_UncomprToM2TXRaw(M2TX *tex, M2TXPIP *pip, uint8 lod,
|||	    M2TXRaw *raw)
|||	
|||	  Description
|||	    This function takes an M2TX texture pointed to by tex, a pip, and a 
|||	    level of detail index. It copies the data and, using the pip, creates an
|||	    M2TXRaw image pointed by raw.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The PIP used for decoding the M2TX texture.
|||	    lod
|||	        The level of detail to extract into raw.
|||	    raw
|||	        The pointer to the M2TXRaw image created.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ComprToM2TXRaw()
**/

M2Err M2TX_UncomprToM2TXRaw(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw)
{
  MemQB_Vars;

	bool  hasColorConst, isLiteral, hasColor, hasAlpha, hasSSB;
	uint8 cDepth, aDepth, ssbDepth;
	M2TXTex texel;
	uint16 xSize, ySize, minX, minY;
	M2TXRaw myRaw;
	M2TXColor colorConst[2], color;
	uint32 pixels, i, size;
	uint8 ssb, red, green, blue, alpha, numLOD;
	uint8 cssb1, cred1, cgreen1, cblue1, calpha1;
	uint8 cssb2, cred2, cgreen2, cblue2, calpha2;
	uint8 shift, aShift, index;
	uint8 sb, al;
	M2Err err;
	M2TXHeader *header;

	err = M2TX_GetHeader(tex, &header);
	err = M2TXHeader_GetFIsLiteral(header, &isLiteral);
	err = M2TXHeader_GetFHasColor(header, &hasColor);
	err = M2TXHeader_GetFHasColorConst(header, &hasColorConst);
	err = M2TXHeader_GetFHasAlpha(header, &hasAlpha);
	err = M2TXHeader_GetFHasSSB(header, &hasSSB);
	err = M2TXHeader_GetCDepth(header, &cDepth);
	err = M2TXHeader_GetADepth(header, &aDepth);
	err = M2TXHeader_GetColorConst(header, 0, &(colorConst[0]));
	err = M2TXHeader_GetColorConst(header, 1, &(colorConst[1]));
	err = M2TXHeader_GetNumLOD(header, &numLOD);
	err = M2TXHeader_GetLODPtr(header, lod, &size, &texel);
	err = M2TXHeader_GetMinXSize(header, &minX);
	err = M2TXHeader_GetMinYSize(header, &minY);
	 	
	xSize = minX << (numLOD-lod-1);
	ySize = minY << (numLOD-lod-1);

	err = M2TXRaw_Init(&myRaw, xSize, ySize, hasColor, hasAlpha, hasSSB); 
	if (err != M2E_NoErr) 				
		return(err);
	pixels = xSize * ySize;	
	
	if (hasSSB) 
	{
		ssbDepth = 1;
		raw->HasSSB = TRUE;
	}
	else
	{
	 	ssbDepth = 0;		
		raw->HasSSB = FALSE;
	}
	if (hasAlpha)
	 	raw->HasAlpha = TRUE;
	 else
	 	raw->HasAlpha = FALSE;
	 if (hasColor)
	 	raw->HasColor = TRUE;
	 else
	 	raw->HasColor = FALSE;	
	 
	if (!isLiteral)
	{	
		if (pip == NULL)
			return(M2E_BadPtr);				/* No PIP to decode the data with */
	}

	if (hasColorConst)
	{
		M2TXColor_Decode(colorConst[0], &cssb1,&calpha1,&cred1,&cgreen1,&cblue1);
		M2TXColor_Decode(colorConst[1], &cssb2,&calpha2,&cred2,&cgreen2,&cblue2);		
	}
	else
	{
		cred1 = cred2 = cgreen1 = cgreen2 = cblue1 = cblue2 = calpha1 = calpha2 = 0xFF;
		cssb1 = cssb2 = 0;     				
	}
	shift = 8 - cDepth;
	aShift = 8 - aDepth;  					
#ifdef QUICKGET
	if (((uint32)texel)%4)
	  fprintf(stderr,"Whoops, texel in UtR not quad-byte=%x\n", texel);
	BeginMemQGetBits((uint32 *)texel);
#else
	BeginMemGetBits(texel);
#endif
	for (i=0; i<pixels; i++)
	{
		if (hasSSB)
		  {
#ifdef QUICKGET
		    MemQGetBits(ssbDepth);
		    ssb = QBResult;
#else
		    ssb = qMemGetBits(ssbDepth);
#endif
		  }
		else
			ssb = cssb1;
		if (!hasAlpha || (aDepth == 0))
		{
			if (ssb == 0)
				alpha = calpha1;
			else
				alpha = calpha2;
		}	
		else		
		  {
#ifdef QUICKGET
		    MemQGetBits(aDepth);
		    alpha = oneExtend(aDepth, QBResult);
#else
		    alpha = oneExtend(aDepth, qMemGetBits(aDepth));
#endif
		  }
		if (!hasColor || (cDepth == 0))
		{
			if (ssb == 0)
			{
				red = cred1; green = cgreen1; blue = cblue1;
			}
			else
			{
				red = cred2; green = cgreen2; blue = cblue2;
			}
		}
		else
		{
			if (isLiteral)
			{
#ifdef QUICKGET
			  MemQGetBits(cDepth);
			  red = oneExtend(cDepth, QBResult);
			  MemQGetBits(cDepth);
			  green = oneExtend(cDepth, QBResult);
			  MemQGetBits(cDepth);
			  blue = oneExtend(cDepth, QBResult);
#else
			  red = oneExtend(cDepth, qMemGetBits(cDepth));
			  green = oneExtend(cDepth, qMemGetBits(cDepth));
			  blue = oneExtend(cDepth, qMemGetBits(cDepth));
#endif
			}
			else
			{
#ifdef QUICKGET
			  MemQGetBits(cDepth);
			  index = QBResult;
#else
			  index = qMemGetBits(cDepth);
#endif
			  color = pip->PIPData[index];
#ifdef QUICKGET
			  QuickDecode(color, sb, al, red, green, blue);
#else
			  M2TXColor_Decode(color,&sb,&al,&red,&green,&blue);
#endif
			  if (!hasAlpha)
			    alpha = al;
			  if (!hasSSB)
			    ssb = sb;
			}
		}
		if (hasColor)
		{
			myRaw.Red[i] = red; myRaw.Green[i] = green; myRaw.Blue[i] = blue; 
		}
		if (hasAlpha)
			myRaw.Alpha[i] = alpha; 
		if (hasSSB)
			myRaw.SSB[i] = ssb;
	}
#ifdef QUICKGET
	EndMemQGetBits();
#else	
	EndMemGetBits();
#endif
	*raw = myRaw;
	return(M2E_NoErr);		
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_UncomprToM2TXIndex
|||	Take a level of detail from an uncompressed texture and copy the data
|||	into an M2TXIndex image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_UncomprToM2TXIndex(M2TX *tex, M2TXPIP *pip, uint8 lod, 
|||	                                  M2TXIndex *index)
|||	
|||	  Description
|||	    This function takes an M2TX texture pointed to by tex, a pip, and a 
|||	    level of detail index.  It copies the data and, using the pip, creates
|||	    an M2TXIndex image pointed by index.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The PIP used for decoding the M2TX texture.
|||	    lod
|||	        The level of detail to extract into index.
|||	    index
|||	        The pointer to the M2TXIndex image created.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_ComprToM2TXIndex()
**/

M2Err M2TX_UncomprToM2TXIndex(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXIndex *index)   
{
  MemQB_Vars;

	bool  hasColorConst, isLiteral, hasColor, hasAlpha, hasSSB;
	uint8 cDepth, aDepth, ssbDepth;
	M2TXTex texel;
	uint16 xSize, ySize, minX, minY;
	M2TXIndex myIndex;
	uint32 pixels, i, size;
	M2TXColor colorConst[2];
	uint8 ssb, alpha, numLOD;
	uint8 cssb1, cred1, cgreen1, cblue1, calpha1;
	uint8 cssb2, cred2, cgreen2, cblue2, calpha2;
	uint8 shift, aShift, cIndex;
	uint8 sb, al;
	M2Err err;
	M2TXHeader *header;

	err = M2TX_GetHeader(tex, &header);
	if (err != M2E_NoErr) 				
		return(err);
	err = M2TXHeader_GetFHasColorConst(header, &hasColorConst);
	err = M2TXHeader_GetFIsLiteral(header, &isLiteral);
	err = M2TXHeader_GetFHasColor(header, &hasColor);
	err = M2TXHeader_GetFHasAlpha(header, &hasAlpha);
	err = M2TXHeader_GetFHasSSB(header, &hasSSB);
	err = M2TXHeader_GetCDepth(header, &cDepth);
	err = M2TXHeader_GetADepth(header, &aDepth);
	err = M2TXHeader_GetNumLOD(header, &numLOD);
	err = M2TXHeader_GetColorConst(header, 0, &(colorConst[0]));
	err = M2TXHeader_GetColorConst(header, 1, &(colorConst[1]));
	err = M2TXHeader_GetLODPtr(header, lod, &size, &texel);
	err = M2TXHeader_GetMinXSize(header, &minX);
	err = M2TXHeader_GetMinYSize(header, &minY);
	 	
	xSize = minX << (numLOD-lod-1);
	ySize = minY << (numLOD-lod-1);

	M2TXIndex_Init(&myIndex, xSize, ySize, hasColor, hasAlpha, hasSSB); 
	if (err != M2E_NoErr) 				
		return(err);
	pixels = xSize * ySize;	
	
	if (hasSSB) 
	{
		ssbDepth = 1;
		index->HasSSB = TRUE;
	}
	else
	{
	 	ssbDepth = 0;		
		index->HasSSB = FALSE;
	}
	if (hasAlpha)
	 	index->HasAlpha = TRUE;
	 else
	 	index->HasAlpha = FALSE;
	 if (hasColor)
	 	index->HasColor = TRUE;
	 else
	 	index->HasColor = FALSE;	
	
	if (!isLiteral)
	{	
		if (pip == NULL)
			return(M2E_BadPtr);				/* No PIP to decode the data with */
	}

	if (hasColorConst)
	{
		M2TXColor_Decode(colorConst[0], &cssb1,&calpha1,&cred1,&cgreen1,&cblue1);
		M2TXColor_Decode(colorConst[1], &cssb2,&calpha2,&cred2,&cgreen2,&cblue2);		
	}
	else
	{
		calpha1 = calpha2 = 0xFF;
		cssb1 = cssb2 = 0; 				
	}
	al = 0xFF;
	sb = 0;
	shift = 8 - cDepth;
	aShift = 8 - aDepth;  				

	cIndex = 0;
	
#ifdef QUICKGET
	if (((uint32)texel)%4)
	  fprintf(stderr,"Whoops, texel in UtR not quad-byte=%x\n", texel);
	BeginMemQGetBits((uint32 *)texel);
#else
	BeginMemGetBits(texel);
#endif
	for (i=0; i<pixels; i++)
	{
		if (hasSSB)
		  {
#ifdef QUICKGET
		    MemQGetBits(ssbDepth);
		    ssb = QBResult;
#else
		    ssb = qMemGetBits(ssbDepth);
#endif
		  }
		else
			ssb = cssb1;
		if (!hasAlpha || (aDepth == 0))
		{
			if (ssb == 0)
				alpha = calpha1;
			else
				alpha = calpha2;
		}	
		else
		  {		
#ifdef QUICKGET
		    MemQGetBits(aDepth);
		    alpha = oneExtend(aDepth, QBResult);
#else
		    alpha = oneExtend(aDepth, qMemGetBits(aDepth));
#endif
		  }
		if (!hasColor || (cDepth == 0))
		{
			cIndex = 0;		
		}
		else
		{
#ifdef QUICKGET
		  MemQGetBits(cDepth);
		  cIndex = QBResult;
#else
		  cIndex = qMemGetBits(cDepth);
#endif
			if (!hasAlpha)
				alpha = al;
			if (!hasSSB)
				ssb = sb;
		}
		if (hasColor)
			myIndex.Index[i] = cIndex;
		if (hasAlpha)
			myIndex.Alpha[i] = alpha; 
		if (hasSSB)
			myIndex.SSB[i] = ssb;
	}
	
#ifdef QUICKGET
	EndMemQGetBits();
#else	
	EndMemGetBits();
#endif
	*index = myIndex;
	return(M2E_NoErr);			
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_ToUncompr
|||	Take an M2TXRaw image and create an uncompressed level of detail in the 
|||	given texture.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_ToUncompr(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw)
|||	
|||	  Description
|||	    This function takes an M2TXRaw image pointed to by raw, a pip, and a 
|||	    level of detail lod.  It creates a level of detail, using the pip,
|||	    in the M2TX texture pointed to by tex.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The PIP used for encoding the M2TX texture.
|||	    lod
|||	        The level of detail to create from raw.
|||	    raw
|||	        The pointer to the input M2TXRaw image.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_ToUncompr()
**/

M2Err M2TXRaw_ToUncompr(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXRaw *raw)
{
  MemQB_Vars;

  M2TXHeader *header;
  bool  hasColorConst, isLiteral, hasColor, hasAlpha, hasSSB;
  uint8 cDepth, aDepth, ssbDepth;
  M2TXColor colorConst[2], color;
  uint16 xSize, ySize, minX, minY;
  uint32 pixels, lodSize,i,j,colors;
  uint8 ssb, alpha, red, green, blue;
  uint8 indexDepth, shift, index, numLOD;
  uint8 *lodPtr;
  M2Err err;
	
  err = M2TX_GetHeader(tex, &header);
  err = M2TXHeader_GetFHasColorConst(header, &hasColorConst);
  err = M2TXHeader_GetFIsLiteral(header, &isLiteral);
  err = M2TXHeader_GetFHasColor(header, &hasColor);
  err = M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  err = M2TXHeader_GetFHasSSB(header, &hasSSB);
  err = M2TXHeader_GetCDepth(header, &cDepth);
  err = M2TXHeader_GetADepth(header, &aDepth);
  err = M2TXHeader_GetColorConst(header, 0, &(colorConst[0]));
  err = M2TXHeader_GetColorConst(header, 1, &(colorConst[1]));
  err = M2TXHeader_GetMinXSize(header, &minX);
  err = M2TXHeader_GetMinYSize(header, &minY);
  err = M2TXHeader_GetNumLOD(header, &numLOD);

  xSize = minX << (numLOD-lod-1);
  ySize = minY << (numLOD-lod-1);
  pixels = xSize * ySize;


  if (hasSSB) 
    ssbDepth = 1;
  else
    ssbDepth = 0;
  if (!(raw->HasSSB))
    ssb = 0;
  if (!(raw->HasAlpha))
    alpha = 0xFF;
  if (!(raw->HasColor))
    {
      red = green = blue = 0;
    }
  if (isLiteral)		
    indexDepth = aDepth + (3*cDepth) + ssbDepth;
  else	
    indexDepth = aDepth + cDepth + ssbDepth;
	
  /* Allocate the LOD Ptr */
	
  lodSize = (pixels*indexDepth); /* How big the encoded buffer needs to be */
  if (lodSize%8)
    lodSize = (lodSize>>3) +1;
  else
    lodSize = lodSize>>3;
  while (lodSize%4)
    lodSize++;
  lodPtr = (uint8 *)malloc(lodSize);
	
  if (!isLiteral)
    {	
      if (pip == NULL)
	return(M2E_BadPtr);	/* No PIP to encode the data with */
      colors = pip->NumColors;
    }

  shift = 8 - cDepth;

#ifdef QUICKPUT
  if (((uint32)lodPtr)%4)
    fprintf(stderr,"Whoops, lodPtr in RtU not quad-byte=%x\n", lodPtr);
  BeginMemQPutBits((uint32 *)lodPtr);
#else
  BeginMemPutBits(lodPtr, lodSize);
#endif

  for (i=0; i<pixels; i++)
    {
      if (raw->HasSSB)
	ssb = raw->SSB[i];
      if (hasSSB)
	{
#ifdef QUICKPUT
	  MemQPutBits(ssb,ssbDepth);
#else
	  qMemPutBits(ssb,ssbDepth);
#endif
	  ssb = 0;		/* It's already been taken care of */
	}
      if (raw->HasAlpha)
	{
	  alpha = raw->Alpha[i];
	}
      if (hasAlpha)
	{
#ifdef QUICKPUT
	  MemQPutBits(alpha>>(8-aDepth),aDepth);
#else
	  qMemPutBits(alpha>>(8-aDepth),aDepth);
#endif
	  alpha = 0xFF;		/* It's already been taken care of */
	}
      if (hasColor)
	{	
	  if (raw->HasColor)
	    {
	      red=raw->Red[i];
	      green=raw->Green[i];			
	      blue=raw->Blue[i];
	    }
	  if (isLiteral)
	    {
#ifdef QUICKPUT
	      MemQPutBits(red>>shift,cDepth);
	      MemQPutBits(green>>shift,cDepth);
	      MemQPutBits(blue>>shift,cDepth);
#else
	      qMemPutBits(red>>shift,cDepth);
	      qMemPutBits(green>>shift,cDepth);
	      qMemPutBits(blue>>shift,cDepth);
#endif
	    }
	  else
	    {
#ifdef QUICKPUT

	      color = QuickCreate(ssb, alpha, red, green, blue);
	      for (j=0; j<colors; j++)
		{
		  if (color == pip->PIPData[j])
		    {
		      index = j;
		      break;
		    }
		}
	      if (j>=colors)
		{
		  fprintf(stderr, "Couldn't find color in PIP\n");
		  return(M2E_Range);
		}

#else
	      err = GetIndex(ssb, alpha, red, green, blue, pip, cDepth, &index, TRUE, TRUE, TRUE);
	      if (err != M2E_NoErr)
		return(err);
#endif

#ifdef QUICKPUT
	      MemQPutBits(index,cDepth);
#else
	      qMemPutBits(index,cDepth);
#endif
	    }
	}
    }
#ifdef QUICKPUT
  EndMemQPutBits();
#else
  EndMemPutBits();
#endif
  err = M2TXHeader_SetLODPtr(header, lod, lodSize, lodPtr);
  /* Possibly add a texel call here, but it's kind of redundant since header has all info */
  return (M2E_NoErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_ToUncompr
|||	Take an M2TXIndex image and create an uncompressed level of detail in the
|||	given texture.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_ToUncompr(M2TX *tex, M2TXPIP *pip, uint8 lod, 
|||	                              M2TXIndex *index)
|||	
|||	  Description
|||	    This function takes an M2TXIndex image pointed to by index, a pip, and a
|||	    level of detail lod.  It creates a level of detail, using the pip,
|||	    in the M2TX texture pointed to by tex.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The PIP used for encoding the M2TX texture.
|||	    lod
|||	        The level of detail to create from index.
|||	    index
|||	        The pointer to the input M2TXIndex image.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_ToUncompr()
**/

M2Err M2TXIndex_ToUncompr(M2TX *tex, M2TXPIP *pip, uint8 lod, M2TXIndex *index)
{
  MemQB_Vars;
  
  M2TXHeader *header;
  bool  hasColorConst, isLiteral, hasColor, hasAlpha, hasSSB;
  uint8 cDepth, aDepth, ssbDepth;
  M2TXColor colorConst[2], color;
  uint16 xSize, ySize, minX, minY;
  uint32 pixels, lodSize,i;
  uint8 ssb, alpha, cIndex, red, green, blue;
  uint8 indexDepth, shift, numLOD;
  uint8 *lodPtr;
  M2Err err;
  
  err = M2TX_GetHeader(tex, &header);
  err = M2TXHeader_GetFHasColorConst(header, &hasColorConst);
  err = M2TXHeader_GetFIsLiteral(header, &isLiteral);
  err = M2TXHeader_GetFHasColor(header, &hasColor);
  err = M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  err = M2TXHeader_GetFHasSSB(header, &hasSSB);
  err = M2TXHeader_GetCDepth(header, &cDepth);
  err = M2TXHeader_GetADepth(header, &aDepth);
  err = M2TXHeader_GetColorConst(header, 0, &(colorConst[0]));
  err = M2TXHeader_GetColorConst(header, 1, &(colorConst[1]));
  err = M2TXHeader_GetMinXSize(header, &minX);
  err = M2TXHeader_GetMinYSize(header, &minY);
  err = M2TXHeader_GetNumLOD(header, &numLOD);
  
  xSize = minX << (numLOD-lod-1);
  ySize = minY << (numLOD-lod-1);
  pixels = xSize * ySize;
  
  if (hasSSB) 
    ssbDepth = 1;
	else
	  ssbDepth = 0;
  if (!(index->HasSSB))
    ssb = 0;
  if (!(index->HasAlpha))
    alpha = 0xFF;
  if (!(index->HasColor))
    cIndex = 0;
  
  if (isLiteral)		
    indexDepth = aDepth + (3*cDepth) + ssbDepth;
  else	
    indexDepth = aDepth + cDepth + ssbDepth;
  
  /* Allocate the LOD Ptr */
  
  lodSize = (pixels*indexDepth);        /* How big the encoded buffer needs to be */
  if (lodSize%8)
    lodSize = (lodSize>>3) +1;
  else
    lodSize = lodSize>>3;
  while (lodSize%4)
    lodSize++;
  lodPtr = (uint8 *)malloc(lodSize);
  
  shift = 8 - cDepth;
  
#ifdef QUICKPUT
  if (((uint32)lodPtr)%4)
    fprintf(stderr,"Whoops, lodPtr in ItU not quad-byte=%x\n", lodPtr);
  BeginMemQPutBits((uint32 *)lodPtr);
#else
  BeginMemPutBits(lodPtr, lodSize);
#endif
  for (i=0; i<pixels; i++)
    {
      if (hasColor)
	{
	  cIndex = index->Index[i];
	  /*
	    if (isLiteral)
	    {
	    */
	  M2TXPIP_GetColor(pip, cIndex, &color);
#ifdef QUICKPUT
	  QuickDecode(color, ssb, alpha, red, green, blue);
#else
	  M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
#endif
	  /*
	    }
	    */
	}
      if (index->HasSSB)
	  ssb = index->SSB[i];
      if (hasSSB)
	{
#ifdef QUICKPUT
	  MemQPutBits(ssb,ssbDepth);
#else
	  qMemPutBits(ssb,ssbDepth);
#endif
	}
      if (index->HasAlpha)
	alpha = index->Alpha[i];
      if (hasAlpha)
	{
#ifdef QUICKPUT
	  MemQPutBits(alpha>>(8-aDepth),aDepth);
#else
	  qMemPutBits(alpha>>(8-aDepth),aDepth);
#endif
	}
      if (index->HasColor)
	cIndex = index->Index[i];
      if (hasColor)
	{
	  if (isLiteral)
	    {
#ifdef QUICKPUT
	      MemQPutBits(red,cDepth);
	      MemQPutBits(green,cDepth);
	      MemQPutBits(blue,cDepth);
#else
	      qMemPutBits(red,cDepth);
	      qMemPutBits(green,cDepth);
	      qMemPutBits(blue,cDepth);
#endif
	    }
	  else
	    {
#ifdef QUICKPUT
	      MemQPutBits(cIndex,cDepth);
#else
	      qMemPutBits(cIndex,cDepth);
#endif
	    }
	}
    }

#ifdef QUICKPUT
  EndMemQPutBits();
#else
  EndMemPutBits();
#endif
  err = M2TXHeader_SetLODPtr(header, lod, lodSize, lodPtr);
  /* Possibly add a texel call here, but it's kind of redundant since header has all info */
  return (M2E_NoErr);
}
