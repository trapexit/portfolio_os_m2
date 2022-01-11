/*
	File:		M2TXEncode.c

	Contains:	M2 Texture Format data encoders 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<4+>	 7/15/95	TMA		Autodocs update and qMemPutBits bug fixed.
		 <3>	 7/11/95	TMA		#include file changes.
		 <2>	 5/16/95	TMA		Autodocs added.
		 <1>	 1/16/95	TMA		first checked in

	To Do:
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "M2TXTypes.h"
#include "M2TXEncode.h"
#include "M2TXDCI.h"
#include "M2TXLibrary.h"
#include "qmem.h"

/* Given a Index structure and a format for each pixel in M2TXIndex, produce a coded output */
/* Which is optionally RLE */


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_Compress
|||	Take an M2TXIndex image and compress it with specified options.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_Compress(M2TXIndex *myIndex, M2TXIndex *format,
|||	                             bool rle, M2TXDCI *dci, uint32 *size, 
|||	                             M2TXTex *newTexel)
|||	
|||	  Description
|||	    This function takes an M2TXIndex image and compresses it with a given dci,
|||	    and optionally according to a specific format and with or without RLE
|||	    compression.  If the variable format is non-NULL, the Index field 
|||	    indicates which of the four DCI texel formats are to be used for each
|||	    pixel.  If rle is true, the resulting M2TXTex texel will be run-length
|||	    encoded as well. The resulting texel data size (including padding) is 
|||	    returned in size.
|||	
|||	  Arguments
|||	    
|||	    myIndex
|||	        The input M2TXIndex texture.
|||	    format
|||	        An M2TXIndex image of the same dimensions as myRaw indicating 
|||	        which texel format to use for each.
|||	    rle
|||	        flag indicating whether run-length encode or not.
|||	    dci
|||	        The input data compression information chunk.
|||	    size
|||	        The file size of the resulting M2TXTex image.
|||	    newTexel
|||	        The compressed image stored as an M2TXTex image (level of detail).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXEncode.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_Compress()
**/
M2Err M2TXIndex_Compress(M2TXIndex *myIndex, M2TXIndex *format, bool rle, M2TXDCI *dci,
			 uint32 *size, M2TXTex *newTexel)
{
  uint32 pixels, curPixel, memLimit, memSize, newMemSize, newBits, stringLength;
  uint32 startOfRun, endOfRun, startOfRLE, endOfRLE, rleLength, count, loop;
  uint32 pad, startOfEncode, endOfEncode, i, j;
  uint8 *newLOD, startFormat, nextFormat, control, txFormat;
  uint8 ssbDepth[4], cDepth[4], aDepth[4], shift[4], aShift[4], isString, depth[4];
  uint8 cIndex, alpha, ssb, maxDepth;
  uint8 lastCIndex, lastAlpha, lastSSB;
  bool isTrans[4], isLiteral[4] ,hasColor[4], hasAlpha[4], hasSSB[4];
  bool hasRun, matches, worthwhile;
  M2Err err;
  long memErr, savings;
  
  
  maxDepth = 0;
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
	err = M2TXDCI_GetCDepth(dci, i, &(cDepth[i]));
      if (!hasAlpha[i])
	aDepth[i] = 0;
      else
	err = M2TXDCI_GetADepth(dci, i, &(aDepth[i]));
      if (hasSSB[i]) 
	ssbDepth[i] = 1;
      else
	ssbDepth[i] = 0;		
      if (isTrans[i])
	depth[i] = 0;
      else
	depth[i] = ssbDepth[i] + aDepth[i] + cDepth[i];	
      
      if (depth[i]>maxDepth)
	maxDepth = depth[i];
      
      shift[i] = 8 - cDepth[i];
      aShift[i] = 8 - aDepth[i];
    }
  
  pixels = (myIndex->XSize) * (myIndex->YSize);
  memLimit = (pixels*(maxDepth+0.5) + 128)/8;
  memSize = memLimit;
  *size = memSize;
  *newTexel = NULL;
  newLOD = (uint8 *) qMemNewPtr(memSize);
  BeginMemPutBits(newLOD, memLimit);
  endOfRun = startOfRun = 0;
  startFormat = 0;
  if (format != NULL)
    startFormat = format->Index[0];
  curPixel = 0;
  if (format == NULL)
    endOfRun = pixels-1;
  while (curPixel < pixels)
    {	/* check next pixel to see if still in the same format */
      if (endOfRun != (pixels-1))
	nextFormat = format->Index[endOfRun+1];
      if ((nextFormat != startFormat) || (endOfRun == (pixels-1)))
	{	/* if not write out the run */
	  startOfEncode = startOfRun;
	  endOfEncode = endOfRun;
	  do 
	    {
	      txFormat = startFormat;
	      hasRun = FALSE;
	      if (rle)
		{
		  lastCIndex = cIndex = 0; 
		  lastAlpha = alpha = 0xFF;
		  lastSSB = ssb=0;
		  if ((hasColor[txFormat]) && (myIndex->HasColor))
		    lastCIndex = myIndex->Index[startOfRun];
		  if ((hasAlpha[txFormat]) && (myIndex->HasAlpha))
		    lastAlpha = myIndex->Alpha[startOfRun];
		  if ((hasSSB[txFormat]) && (myIndex->HasSSB))
		    lastSSB = myIndex->SSB[startOfRun];
		  startOfEncode = startOfRun;
		  endOfEncode = endOfRun;
		  /* Advance to the first RLE or find end */
		  for (i=startOfRun+1; i<= endOfRun; i++)	
		    {						
		      if ((hasColor[txFormat]) && (myIndex->HasColor))
			cIndex = myIndex->Index[i];
		      if ((hasAlpha[txFormat]) && (myIndex->HasAlpha))
			alpha = myIndex->Alpha[i];
		      if ((hasSSB[txFormat]) && (myIndex->HasSSB))
			ssb = myIndex->SSB[i];
		      matches = FALSE;
		      if ((cIndex==lastCIndex) && (alpha==lastAlpha) && (ssb==lastSSB))
			matches = TRUE; 
		      if (matches)
			{
			  if (!hasRun)
			    {
			      hasRun = TRUE;
			      startOfRLE = i-1;
			    }
			  endOfRLE = i;
			}
		      if (hasRun)
			if ((!matches)||(i==endOfRun))
			  {	/* Determine if RLE is worth it */
			    rleLength = endOfRLE - startOfRLE+1;
			    worthwhile = FALSE;
			    savings = (rleLength-1)*depth[txFormat];
			    if (startOfRLE == startOfRun)
			      savings += 8;
			    /* We have to use the control byte anyway */
			    if (endOfRLE == endOfRun)
			      savings += 8; 
			    /* better than the two bytes required to insert */
			    if (savings > 16)
			      worthwhile = TRUE;   /* a run into the string */
			    if (worthwhile)
			      {
				if (startOfRLE != startOfRun)  
				  {   /* Get rid string ahead of first run */
				    endOfEncode = startOfRLE-1;
				    hasRun = FALSE;
				    break;
				  }
				else
				  {
				    startOfEncode = startOfRLE;
				    endOfEncode = endOfRLE;
				    break;
				  }
			      }
			    else
			      hasRun = FALSE;
			  }
		      lastCIndex = cIndex;
		      lastAlpha = alpha;
		      lastSSB = ssb;
		    }
		}
	      isString = 1;
	      if (hasRun)	                    
		if (startOfEncode == startOfRLE)			
		  isString = 0;			
	      
	      curPixel = startOfEncode;
	      control = 0;
	      control = ((txFormat << 6) & 0xC0) | (control & (~0xC0));
	      stringLength = endOfEncode - startOfEncode +1;
	      
	      while (stringLength > 0)		/* Encode the string in 32 or 64 texel chunks */
		{
		  count = stringLength;
		  if (isTrans[txFormat])
		    {
		      if (count > 64)
			count = 64;
		      control = ((count-1) & 0x3F) | (control & (~0x3F));
		      isString = 0;
		    }
		  else
		    {
		      if (count > 32)
			count = 32;
		      control = ((count-1) & 0x1F) | (control & (~0x1F));
		      control = ((isString<<5) & 0x20) | ( control & (~0x20));
		    }
		  qMemPutBits(control,8);
		  if (isString)
		    loop = count;
		  else
		    loop = 1;
		  for (j=0; j<loop; j++)
		    {	
		      if(!(isTrans[txFormat]))
			{
			  if (myIndex->HasSSB)
			    ssb = myIndex->SSB[curPixel];
			  if (myIndex->HasAlpha)
			    alpha = myIndex->Alpha[curPixel];
			  if (myIndex->HasColor)
			    cIndex = myIndex->Index[curPixel];
			  if (hasSSB[txFormat])
			    qMemPutBits(ssb, ssbDepth[txFormat]);
			  if (!(hasAlpha[txFormat]) || (aDepth[txFormat] == 0))
			    ;
			  else		
			    qMemPutBits((alpha>>aShift[txFormat]),aDepth[txFormat]);
			  if (!(hasColor[txFormat]) || (cDepth[txFormat] == 0))
			    ;
			  else
			    qMemPutBits(cIndex, cDepth[txFormat]);
			}
		      curPixel++;
		    }
		  stringLength -= count;
		}
	      startOfRun = endOfEncode+1;
	    } while (endOfEncode != endOfRun);
	  startFormat = nextFormat;
	  curPixel = startOfRun;
	}
      endOfRun++;	
    }
  memErr = GetCurrMemPutPos();
  newMemSize = memErr;
  newBits = GetCurrMemPutBitPos();
  pad = 0;
  if (newBits)
    {
      qMemPutBits(pad, 8-newBits);
      newMemSize++;
    }
  /* padded zero to the end to align properly */
  while (newMemSize % 4)
    {
      qMemPutBits(pad, 8);
      newMemSize++;
    }

  EndMemPutBytes();
  
  if (memErr < 0)
    {	/* the compression fail, the compressed data exceed the uncompressed method */
      qMemReleasePtr(newLOD);
      return(-1);
    }
  else
    {
      /* resize ptr if necessary */
      if (memSize > newMemSize)
	{
	  newLOD = (uint8 *)qMemResizePtr(newLOD, newMemSize);
	  memSize = newMemSize;
	}
      *size = memSize;
      *newTexel = newLOD;
      
      /* Resize ptr and assign to LOD */
    }
  return (M2E_NoErr);	
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_Compress
|||	Take an M2TXRaw image and compress it with specified options.
|||	 
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_Compress(M2TXRaw *myRaw, M2TXIndex *format, bool rle, 
|||	                           M2TXDCI *dci, uint32 *size, M2TXTex *newTexel)
|||	
|||	  Description
|||	    This function takes in an M2TXRaw image and compresses it with a given dci,
|||	    and optionally according to a specific format and with or without RLE
|||	    compression.  If the variable format is non-NULL, the Index field 
|||	    indicates which of the four DCI texel formats are to be used for each 
|||	    pixel.  If rle is true, the resulting M2TXTex texel will be run-length
|||	    encoded as well.
|||	    The resulting texel data size (including padding) is returned in size.
|||	
|||	  Arguments
|||	    
|||	    myRaw
|||	        The input M2TXRaw texture.
|||	    format
|||	        An M2TXIndex image of the same dimensions as myRaw indicating 
|||	        which texel format to use for each.
|||	    rle
|||	        flag indicating whether run-length encode or not.
|||	    dci
|||	        The input data compression information chunk.
|||	    size
|||	        The file size of the resulting M2TXTex image.
|||	    newTexel
|||	        The compressed image stored as an M2TXTex image (level of detail).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXEncode.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_Compress()
**/
M2Err M2TXRaw_Compress(M2TXRaw *myRaw, M2TXIndex *format, bool rle, M2TXDCI *dci,
		       uint32 *size, M2TXTex *newTexel)
{
  uint32 pixels, curPixel, memLimit,memSize,newMemSize,newBits,stringLength;
  uint32 startOfRun, endOfRun, startOfRLE, endOfRLE, rleLength, count, loop;
  uint32 pad, startOfEncode, endOfEncode, i, j;
  uint8 *newLOD, startFormat, nextFormat, control, txFormat, isString;
  uint8 ssbDepth[4], cDepth[4], aDepth[4], shift[4], aShift[4], depth[4];
  uint8 alpha, ssb, maxDepth, red, green, blue;
  uint8 lastRed, lastBlue, lastGreen, lastAlpha, lastSSB;
  bool isTrans[4], isLiteral[4] ,hasColor[4], hasAlpha[4], hasSSB[4];
  bool hasRun, matches, worthwhile;
  M2Err err;
  long memErr, savings;
  
  maxDepth=0;
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
	err = M2TXDCI_GetCDepth(dci, i, &(cDepth[i]));
      if (!hasAlpha[i])
	aDepth[i] = 0;
      else
	err = M2TXDCI_GetADepth(dci, i, &(aDepth[i]));
      if (hasSSB[i]) 
	ssbDepth[i] = 1;
      else
	ssbDepth[i] = 0;		
      if (isTrans[i])
	depth[i] = 0;
      else
	depth[i] = ssbDepth[i] + aDepth[i] + 3*cDepth[i];	
      if (depth[i]>maxDepth)
	maxDepth = depth[i];
      shift[i] = 8 - cDepth[i];
      aShift[i] = 8 - aDepth[i];
    }
  
  pixels = (myRaw->XSize) * (myRaw->YSize);
  memLimit = (pixels*(maxDepth+0.5) + 128)/8;
  memSize = memLimit;
  *size = memSize;
  *newTexel = NULL;
  newLOD = (uint8 *) qMemNewPtr(memSize);
  BeginMemPutBits(newLOD, memLimit);
  endOfRun = startOfRun = 0;
  startFormat = 0;
  if (format != NULL)
    startFormat = format->Index[0];
  curPixel = 0;
  if (format == NULL)
    endOfRun = pixels-1;
  while (curPixel < pixels)
    {	/* check next pixel to see if still in the same format */
      if (endOfRun != (pixels-1))
	nextFormat = format->Index[endOfRun+1];
      if ((nextFormat != startFormat) || (endOfRun == (pixels-1)))
	{	/* if not write out the run */
	  startOfEncode = startOfRun;
	  endOfEncode = endOfRun;
	  do 
	    {
	      txFormat = startFormat;
	      hasRun = FALSE;
	      if (rle)
		{
		  lastRed = red = 0; 
		  lastGreen = green = 0; 
		  lastBlue = blue = 0; 
		  lastAlpha = alpha = 0xFF;
		  lastSSB = ssb=0;
		  if ((hasColor[txFormat]) && (myRaw->HasColor))
		    {
		      lastRed = myRaw->Red[startOfRun];
		      lastGreen = myRaw->Green[startOfRun];
		      lastBlue = myRaw->Blue[startOfRun];
		    }
		  if ((hasAlpha[txFormat])&&(myRaw->HasAlpha))
		    lastAlpha = myRaw->Alpha[startOfRun];
		  if ((hasSSB[txFormat])&&(myRaw->HasSSB))
		    lastSSB = myRaw->SSB[startOfRun];
		  startOfEncode = startOfRun;
		  endOfEncode = endOfRun;
		  for (i=startOfRun+1; i<= endOfRun; i++)
		    /* Advance to the first RLE or find end */
		    {											
		      if ((hasColor[txFormat])&& (myRaw->HasColor))
			{
			  red = myRaw->Red[i];
			  green = myRaw->Green[i];
			  blue = myRaw->Blue[i];
			}
		      if ((hasAlpha[txFormat])&&(myRaw->HasAlpha))
			alpha = myRaw->Alpha[i];
		      if ((hasSSB[txFormat])&&(myRaw->HasSSB))
			ssb = myRaw->SSB[i];
		      matches = FALSE;
		      if ((red==lastRed) &&(green==lastGreen) &&(blue==lastBlue) && 
			  (alpha==lastAlpha) && (ssb==lastSSB))
			matches = TRUE; 
		      if (matches)
			{
			  if (!hasRun)
			    {
			      hasRun = TRUE;
			      startOfRLE = i-1;
			    }
			  endOfRLE = i;
			}
		      if (hasRun)
			if ((!matches)||(i==endOfRun))
			  {	/* Determine if RLE is worth it */
			    rleLength = endOfRLE - startOfRLE+1;
			    worthwhile = FALSE;
			    savings = (rleLength-1)*depth[txFormat];
			    if (startOfRLE == startOfRun)
			      savings += 8;
			    /* We have to use the control byte anyway */
			    if (endOfRLE == endOfRun)
			      savings += 8; 
			    if (savings > 16)
			      /* better than the two bytes required to insert */
			      worthwhile = TRUE;		/* a run into the string */
			    if (worthwhile)
			      {
				if (startOfRLE != startOfRun)  
				  {   /* Get rid string ahead of first run */
				    endOfEncode = startOfRLE-1;
				    hasRun = FALSE;
				    break;
				  }
				else
				  {
				    startOfEncode = startOfRLE;
				    endOfEncode = endOfRLE;
				    break;
				  }
			      }
			    else
			      hasRun = FALSE;
			  }
		      lastRed = red;
		      lastGreen = green;
		      lastBlue = blue;
		      lastAlpha = alpha;
		      lastSSB = ssb;
		    }
		}
	      isString = 1;
	      if (hasRun)	                    
		if (startOfEncode == startOfRLE)			
		  isString = 0;			
	      
	      curPixel = startOfEncode;
	      control = 0;
	      control = ((txFormat << 6) & 0xC0) | (control & (~0xC0));
	      stringLength = endOfEncode - startOfEncode +1;

	      while (stringLength > 0)		/* Encode the string in 32 or 64 texel chunks */
		{
		  count = stringLength;
		  if (isTrans[txFormat])
		    {
		      if (count > 64)
			count = 64;
		      control = ((count-1) & 0x3F) | (control & (~0x3F));
		      isString = 0;
		    }
		  else
		    {
		      if (count > 32)
			count = 32;
		      control = ((count-1) & 0x1F) | (control & (~0x1F));
		      control = ((isString<<5) & 0x20) | ( control & (~0x20));
		    }
		  qMemPutBits(control,8);
		  if (isString)
		    loop = count;
		  else
		    loop = 1;
		  for (j=0; j<loop; j++)
		    {	
		      if(!(isTrans[txFormat]))
			{
			  if (myRaw->HasSSB)
			    ssb = myRaw->SSB[curPixel];
			  if (myRaw->HasAlpha)
			    alpha = myRaw->Alpha[curPixel];
			  if (myRaw->HasColor)
			    {
			      red = myRaw->Red[curPixel];
			      green = myRaw->Green[curPixel];
			      blue = myRaw->Blue[curPixel];
			    }
			  if (hasSSB[txFormat])
			    qMemPutBits(ssb, ssbDepth[txFormat]);
			  if (!(hasAlpha[txFormat]) || (aDepth[txFormat] == 0))
			    ;
			  else		
			    qMemPutBits((alpha>>aShift[txFormat]),aDepth[txFormat]);
			  if (!(hasColor[txFormat]) || (cDepth[txFormat] == 0))
			    ;
			  else
			    {
			      qMemPutBits(red, cDepth[txFormat]);
			      qMemPutBits(green, cDepth[txFormat]);
			      qMemPutBits(blue, cDepth[txFormat]);
			    }
			}
		      curPixel++;
		    }
		  stringLength -= count;
		}
	      startOfRun = endOfEncode+1;
	    } while (endOfEncode != endOfRun);
	  startFormat = nextFormat;
	  curPixel = startOfRun;
	}
      endOfRun++;	
    }
  memErr = GetCurrMemPutPos();
  newMemSize = memErr;
  newBits = GetCurrMemPutBitPos();
  pad = 0;
  if (newBits)
    {
      qMemPutBits(pad, 8-newBits);      
      newMemSize++;
    }
  /* padded zero to the end to align properly */
  while (newMemSize % 4)
    {
      qMemPutBits(pad, 8);
      newMemSize++;
    }
  
  EndMemPutBytes();
  
  if (memErr < 0)
    {	/* the compression fail, the compressed data exceed the uncompressed method */
      qMemReleasePtr(newLOD);
      return(-1);
    }
  else
    {
      /* resize ptr if necessary */
      if (memSize > newMemSize)
	{
	  newLOD = (uint8 *)qMemResizePtr(newLOD, newMemSize);
	  memSize = newMemSize;
	}
      *size = memSize;
      *newTexel = newLOD;
      
      /* Resize ptr and assign to LOD */
    }
  return (M2E_NoErr);	
}
