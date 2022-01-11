/*
	File:		M2TXbestcompress.c

	Contains:	M2 Compression determination algorithm 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <6>	12/17/95	TMA		Fixed DCI use of transparent formats bug.
		 <5>	 7/15/95	TMA		Autodocs update and support for NumColors and IndexOff fields in
									M2TXPIP.
		 <3>	 7/11/95	TMA		#include file changes.
		 <2>	 5/16/95	TMA		Autodocs added
		<5+>	 5/16/95	TMA		Autodocs added
		 <5>	 3/26/95	TMA		Changed all occurences of M2TXRaw_Alloc to M2TXRaw_Init to save
									memory.  The routines now only allocate memory for channels that
									exist.
		 <4>	 2/19/95	TMA		Fixed major bugs in memory allocation and improved compression
									ratios.
		 <1>	 1/16/95	TMA		first checked in

	To Do:
*/

#include "M2TXTypes.h"
#include "M2TXLibrary.h"
#include "M2TXHeader.h"
#include "M2TXFormat.h"
#include "M2TXDCI.h"
#include "M2TXbestcompress.h"
#include "M2TXio.h"
#include "M2TXEncode.h"
#include <stdlib.h>
#include <stdio.h>
#include "qmem.h"
#include <string.h>

static void IndexToBits(uint8 *palPtr, uint8 index)
{	/* Put the color in the PalUsage structure */
  uint8 theByte, theBit, theOne;
  uint8 *ptr;
	
  theByte = index / 8;
  theBit = index % 8;
	
  ptr = palPtr + theByte;
  theOne = 1 << theBit;
  *ptr = (*ptr) | theOne;
}

static void IndexRemoveBits(uint8 *palPtr, uint8 index)
{	/* Put the color in the PalUsage structure */
  uint8 theByte, theBit, theOne;
  uint8 *ptr;
	
  theByte = index / 8;
  theBit = index % 8;
	
  ptr = palPtr + theByte;
  theOne = ~(1 << theBit);
  *ptr = (*ptr) & theOne;
}

/* From old run, start new run of proper bitdepth */
static void	StartNewRun(M2TXIndex *myIndex, uint8 *palPtr, uint8 index,
			    uint32 *startOfRun, uint32 *lengthOfRun)
{
  uint8	firstIndex, nextIndex;
  bool 	done = FALSE;
  bool	found;
  uint32  i, endOfRun;
	
	
  /* get first color index */
  endOfRun = *startOfRun + *lengthOfRun;
  firstIndex = myIndex->Index[*startOfRun];
  while(done==FALSE)
    {
      /* advance start point until a different color is reached */
      do
	{
	  *startOfRun += 1;
	  *lengthOfRun -= 1;
	  nextIndex = myIndex->Index[*startOfRun];
	}
      while (nextIndex == firstIndex);
      /* check new run for any more occurrences of first color
	 what about case where at the end of the pixels?
	 */
      found = FALSE;
      for (i=(*startOfRun)+1; i<endOfRun; i++)
	{
	  if ((myIndex->Index[i])== firstIndex)
	    {
	      found = TRUE;
	      break;
	    }		
	}
      if (found == TRUE) /* if more occurences, that new color is now first color, loop */
	firstIndex = nextIndex;
      else 
	done = TRUE;		
    }
  /* if no more occurences, remove that color from palette, add index color to palette */
  IndexRemoveBits(palPtr, firstIndex);
  IndexToBits(palPtr, index);
  /* compute new length */
  *lengthOfRun += 1;
}

static uint32 PalFindPrevious(uint8 *palUsage, uint8 bPU, uint32 lIB, uint8 *palPtr)
{
  uint32 prevPal;   			/* See if palette usage already exists */
  int i;
  uint8 *ptr, *refPtr,j;
	
	
  prevPal = lIB;   /* default to returning itself */
  for (i=0; i<=lIB; i++)
    {
      ptr=palUsage+(i*bPU);
      refPtr = palPtr;
      for (j=0; j<bPU; j++, ptr++, refPtr++)
	if ((*ptr) != (*refPtr))
	  break;
      if (j==bPU)
	{
	  prevPal = i;			/* We found a match */
	  return(prevPal);
	}
    }
  return (lIB);
}



static signed long ComputeModOldSavings(uint8 proposedDepth, M2TXIndex *curFormat, 
					uint8 dciFormatDepth[4], uint32 startOfRun,
					uint32 lengthOfRun, uint32 pixels)
{

  uint32 i, endOfRun, startOfSegment, endOfSegment, lengthOfSegment;
	
  signed long save, overallSave;
  uint8 segFormat, segDepth;
	
  overallSave = 0;
  startOfSegment = startOfRun;
  endOfRun = startOfRun+lengthOfRun - 1;
  do
    {
      segFormat = curFormat->Index[startOfSegment];
      endOfSegment = startOfSegment;
      while (endOfSegment < endOfRun)
	if (segFormat == (curFormat->Index[endOfSegment+1]))
	  endOfSegment++;
	else
	  break;
      lengthOfSegment = endOfSegment-startOfSegment+1;
      segDepth = dciFormatDepth[segFormat];
      if (segDepth > proposedDepth )		
	{
	  save = -16;   /* Account for the control byte to start it and the one to end it */
	  for (i=startOfSegment; i<= endOfSegment; i++)
	    {
	      save += curFormat->Alpha[i];       /* Alpha is used to mark control byte usage */
	      save += segDepth - proposedDepth; 
	    }
		
	  if (lengthOfSegment > 32)        /* Count the internal control bytes */
	    {
	      save -= ((lengthOfSegment-1)>>5);
	      if (proposedDepth == 0)
		save += (((lengthOfSegment-1)>>6)+1)*8;	   /* For each run over 32 you save a control byte */
	    }

	  if ((endOfSegment+1) == pixels) /* We don't need a control byte at the end */
	    save +=8;
	  else    /* We may where one has to start anyway */
	    save += curFormat->Alpha[endOfSegment+1];		
	  if (save < 0)
	    save = 0;
	  overallSave += save;
	}
      startOfSegment = endOfSegment+1;
    } while (endOfSegment < endOfRun);
	
  return (overallSave);
}


static uint32 ComputeModSavings(uint8 proposedDepth, M2TXIndex *curFormat, 
				uint8 dciFormatDepth[4], uint32 startOfRun,
				uint32 lengthOfRun, uint32 pixels)
{

  uint32 i, j, endOfRun, startOfSegment, endOfSegment, lengthOfSegment;
  signed long save, maxSavings, saveBefore, saveAfter;
  uint32 overallSave;
  int downCount;
  bool goingDown, negative;
	
  overallSave = 0;
  startOfSegment = startOfRun;
  endOfRun = startOfRun+lengthOfRun - 1;
  do
    {
      endOfSegment = endOfRun;
      goingDown = FALSE;
      maxSavings = save = -16;   /* Account for the control byte to start it and the one to end it */
      for (i=startOfSegment; i<= endOfSegment; i++)
	{
	  saveBefore = save;
	  if (save<0)
	    negative = TRUE;
	  else
	    negative = FALSE;
	  save += curFormat->Alpha[i];       /* Alpha is used to mark control byte usage */
	  save += dciFormatDepth[curFormat->Index[i]] - proposedDepth; 
	  saveAfter = save;
	  if (saveBefore>saveAfter)
	    {
	      if (negative)
		{
		  startOfSegment=i+1;
		  maxSavings = save = -16;   /* Account for the control byte to start it and the one to end it */
		  goingDown = FALSE;
		}
	      else
		{
		  if (goingDown == FALSE)
		    {
		      goingDown = TRUE;
		      downCount = 0;
		    }
		  downCount++;
		  if ((maxSavings - save) > 16)
		    {
		      endOfSegment = i - downCount;
		      save = -16;
		      for (j=startOfSegment; j<=endOfSegment; j++)
			{
			  save += curFormat->Alpha[j];       /* Alpha is used to mark control byte usage */
			  save += dciFormatDepth[curFormat->Index[j]] - proposedDepth; 
			}
		      break;
		    }				
		}
	    }
	  else
	    {
	      goingDown = FALSE;
	      if (save > maxSavings)
		maxSavings = save;
	    }
	}
      lengthOfSegment = endOfSegment-startOfSegment+1;
      if (lengthOfSegment > 32)        /* Count the internal control bytes */
	{
	  save -= ((lengthOfSegment-1)>>5);
	  if (proposedDepth == 0)
	    save += (((lengthOfSegment-1)>>6)+1)*8;	   /* For each run over 32 you save a control byte */
	}
      if ((endOfSegment+1) == pixels) /* We don't need a control byte at the end */
	save +=8;
      else    /* We may where one has to start anyway */
	save += curFormat->Alpha[endOfSegment+1];		

      if (save < 0)
	save = 0;
			
      overallSave += (uint32)save;
      startOfSegment = endOfSegment+1;

    } while (endOfSegment < endOfRun);
	
  return (overallSave);
}

static signed long ComputeSavings(uint8 proposedDepth, uint8 curDepth, uint32 lengthOfRun)
{
  signed long save;
	
  /* For encoding the string; */
  save = lengthOfRun*curDepth - lengthOfRun*proposedDepth;
  save -= 8;  				/* Extra control byte to start the run */
	
  if (proposedDepth == 0)
    if (lengthOfRun>32)	
      save += ((lengthOfRun/64)+1)*8;	   /* For each run over 32 you save a control byte */

  if (save < 0)
    save = 0;
  return (save);
}

static void InitPalUsage(uint8 *palPtr, uint8 bPU, uint8 *oldPalPtr)
{	/* Put the color in the PalUsage structure */
  uint8 *ptr, *oldPtr, i;
	
  ptr = palPtr;
  oldPtr = oldPalPtr;
  for (i=0; i<bPU; ptr++,i++)
    if (oldPalPtr == NULL)
      *ptr = 0;
    else
      {
	*ptr = *oldPtr;
	oldPtr++;
      }
}

static bool ColorInPal(uint8 *palPtr, uint32 index)
{   /* Check if color is in the PalUsage structure */
  uint8 theByte, theBit, theOne;
  uint8 *ptr;
	
  theByte = index / 8;
  theBit = index % 8;
	
  ptr = palPtr + (theByte);
  theOne = 1 << theBit;
  return ((*ptr) & theOne);
}

static void PrintPal(uint8 *palPtr, uint16 numColors)
{   /* Check if color is in the PalUsage structure */
  uint8 index, num;
  int i; 	
	
  num = 0;
  for (i=0; i<numColors; i++)
    if (ColorInPal(palPtr,i))
      {
	num++;
	index = i;
      }	
}

static bool NoColorsInPal(uint8 *palPtr, uint8 bPU)
{   /* Check if color is in the PalUsage structure */
  uint8 *ptr;
  int i;
	
  ptr = palPtr;
  for (i=0; i<bPU; i++)
    {
      if ((*ptr) != 0)
	return(FALSE);
      ptr++;
    }
  return (TRUE);
}

static bool PalIsSubset(uint8 *palPtr, uint8 bPU, uint8 *myPal)
{   /* Check if all colors in myPal, exist in palPtr */
  uint8 *ptr, *myPtr, tempVal;
	
  int i;
	
  ptr = palPtr;
  myPtr = myPal;
  for (i=0; i<bPU; i++)
    {
      tempVal = *myPtr;
      if (((*ptr) & (*myPtr)) != tempVal)
	return(FALSE);
      myPtr++;
      ptr++;
    }
  return (TRUE);
}

static bool PalIsSuperset(uint8 *palPtr, uint8 bPU, uint8 *myPal)
{   /* Check if all colors in palPtr exist in myPal */
  uint8 *ptr, *myPtr, tempVal;
	
  int i;
	
  ptr = palPtr;
  myPtr = myPal;
  for (i=0; i<bPU; i++)
    {
      tempVal = *palPtr;
      if (((*ptr) & (*myPtr)) != tempVal)
	return(FALSE);
      ptr++; 
      myPtr++;
    }
  return (TRUE);
}

static void SkimBuffer(uint8 bPU, uint32 cBS, uint32 skimSize, 
		       uint32 *lIB, uint8 **palUsage, uint8 **palPtr, 
		       uint32 **savings)
{
  uint32 	j,k,l, tempEnd, tempBound, *tempIndex;
  uint32  *tempSave;
  uint8 *tempPtr, *tempPal, *oldPalPtr;
	
  tempSave = (uint32 *)calloc(cBS, sizeof(uint32));  	
  tempIndex = (uint32 *)calloc(skimSize, sizeof(uint32));
  tempPal = (uint8 *)calloc(cBS, bPU*sizeof(uint8));  	
  tempEnd = 1;
  *tempIndex = 0;
  tempSave[0] = (*savings)[0];
  for (j=1; j<cBS; j++)
    {
      for(k=0; k<tempEnd; k++)
	{
	  if ((*savings)[j]>tempSave[k])      /* Move the others down to fit */
	    {	
	      if (tempEnd >= skimSize)
		tempBound = skimSize-1;
	      else
		tempBound = tempEnd;
	      for (l=tempBound; l>k; l--)
		{
		  tempSave[l] = tempSave[l-1];
		  tempIndex[l] = tempIndex[l-1];
		}
	      tempSave[k] = (*savings)[j];
	      tempIndex[k] = j;
	      tempEnd++;
	      if (tempEnd > skimSize)
		tempEnd = skimSize;
	      break;
	    }
	}
      if (k>=tempEnd)						/* Attach thing to the end */
	{	
	  if (tempEnd >= skimSize)
	    tempBound = skimSize-1;
	  else
	    tempBound = tempEnd;
	  if (k<skimSize)
	    {
	      tempSave[k] = (*savings)[j];
	      tempIndex[k] = j;
	      tempEnd++;
	      if (tempEnd > skimSize)
		tempEnd = skimSize;
	    }
	}
    }
  for (j=0; j<tempEnd; j++)  
    {	 /* Copy those top skimSize palettes to the new palUsage(tempPal) */
      tempPtr = tempPal + j*bPU;
      oldPalPtr = *palUsage + tempIndex[j]*bPU;
      for (k=0; k<bPU; k++)
	{
	  *tempPtr = *oldPalPtr;
	  tempPtr++; oldPalPtr++;
	}
    }
					
  free(tempIndex);
  for (j=0; j<bPU; j++)
    *(tempPtr+j)= *((*palPtr)+j);		/* Copy PalPtr to end of skimmed list */						
  free(*palUsage);
  *palUsage = tempPal;
  free(*savings);
  *savings = tempSave;
  *palPtr = tempPtr;
  *lIB = tempEnd;

}

/* Finds optimal mult-texel format by analazing all runs at a given bitdepth and 
 finding the most savings at a particular bitdepth and limited palette.
 FinalSavings- for curDepth=0; returns three values of the three top const colors,
 else just one value Palette- Multi-Use.  
 On input, Palette[0], if non-zero, is the palette which the output palette is a
 superset of. Palette[1], if non-zero, is the palette which the output palette is a 
 subset of.
 On output, if curDepth = 0; each palette is used (to contain a single color)
 else, Palette[0] has the optimal palette
*/


#define FPAL_MAXBUF  300
#define FPAL_SKIM	 50
#define FPAL_RESAVE   20

static M2Err Find_BestPalette(M2TXIndex *myIndex, M2TXIndex *curFormat,
			      uint8 depth, uint8 curDepth, M2TXDCI *dci, 
			      uint32 FinalSavings[3], uint8 *(Palette[3]))
{
  uint32 	lengthOfRun, startOfRun, endOfRun, rleCount;
  uint32 	width, height, pixels, i,j,k;
  uint16  	colorsInUse, colorsAllowed, numColors, bPU;
  uint8 	*palUsage, *palPtr, *oldPalPtr, curIndex, firstIndex, tempIn;
  uint32	*savings, *savePtr, tempSavings;
  uint8		dciFormatDepth[4];
  uint32	cBS, lIB, prevPal, bestPal[3], rePal[FPAL_RESAVE], reSavings[FPAL_RESAVE];
  uint32     	maxBufSize = FPAL_MAXBUF;
  uint32	skimSize   = FPAL_SKIM;		/*  Keep this many, discard the rest */
  bool		findSubset, findSuperset, advancePtr, colorInPal, inRun, flag;
	
	
  width = myIndex->XSize;
  height = myIndex->YSize;
  pixels = width*height;

  for (i=0; i<4; i++)				/* This table will be used to find the current constraining depth */
    {
      M2TXDCI_GetFIsTrans(dci, i, &flag);
      if (flag)
	dciFormatDepth[i] = 0;
      else
	M2TXDCI_GetCDepth(dci, i, &(dciFormatDepth[i]));
    }
	
  numColors = 1 << depth;
  if (pixels <= 0)
    return (M2E_Range);

  /* Initialize totals */
  colorsAllowed = 1 << curDepth;
  cBS  = maxBufSize;
  lIB = 0;
  bPU = numColors/8;
  if (numColors%8)
    bPU++;
		
  findSuperset = !NoColorsInPal(Palette[0],bPU);
  findSubset = !NoColorsInPal(Palette[1],bPU);
  if (curDepth == 0)
    findSubset = FALSE;
		
  /* Colors in Palette Usage data-use of 4 colors will have 4 on bits, 
     use 10 colors means 10 on bits throughout an entry */
  palUsage = (uint8 *)calloc(cBS, bPU*sizeof(uint8));  	 	
  if (palUsage == NULL)
    return(M2E_NoMem);
  savings  = (uint32 *)calloc(cBS, sizeof(uint32)); 
  if (savings == NULL)
    {
      free(palUsage);
      return(M2E_NoMem);
    }	
  colorsInUse = 1;
  lengthOfRun = 1;
  palPtr = palUsage;					/* Start at the beginning of the buffer */
  savePtr = savings;
  *savePtr = 0;
  startOfRun = 0;						/* The first run is the beginning of the file */
  firstIndex = myIndex->Index[0];		/* First index in run, about to fall off edge */
  InitPalUsage(palPtr, bPU, NULL);
  IndexToBits(palPtr, firstIndex);	/* Put first color in the PalUsage structure */
  for(i=1; i<pixels; i++)
    {
      curIndex = myIndex->Index[i];
      colorInPal = ColorInPal(palPtr, curIndex);
      if (colorInPal)
	lengthOfRun++;
      if ((!colorInPal) || (i==(pixels-1)))
	{
	  if ((colorsInUse == colorsAllowed) || (i==(pixels-1)))
	    {
	      advancePtr = TRUE;
	      if (advancePtr && findSuperset)	/* Do we even care about this palette? */
		advancePtr = PalIsSuperset(Palette[0],bPU, palPtr);
	      if (advancePtr && findSubset)
		advancePtr = PalIsSubset(Palette[1],bPU, palPtr);
	      if (advancePtr)
		*savePtr += ComputeModSavings(curDepth, curFormat, dciFormatDepth, startOfRun, lengthOfRun,pixels);
	      lIB++;
	      oldPalPtr = palPtr;
	      palPtr = palUsage + (lIB*bPU);	/* variable data format, must compute bytes */
	      InitPalUsage(palPtr, bPU, oldPalPtr);
	      savePtr = &(savings[lIB]);     /* It's a long so we can get away w/o computing bytes */
	      *savePtr = 0;
	      if (lIB == (cBS-2))
		{  	 /* If at buffer max, discard all but the top skimSize, and start over */
		  if (cBS >= maxBufSize)
		    SkimBuffer(bPU, cBS, skimSize, &lIB, &palUsage, &palPtr, &savings);
		}
	      StartNewRun(myIndex, palPtr, curIndex, &startOfRun, 
			  &lengthOfRun);
	      prevPal = PalFindPrevious(palUsage, bPU, lIB, palPtr);
	      if (prevPal < lIB)  /* If the combination of colors exists, just use it */
		{
		  palPtr = palUsage + (prevPal*bPU);	/* variable data format, must compute bytes */
		  savePtr = &(savings[prevPal]); /* It's a long so we can get away w/o computing bytes */
		  lIB--;
		}
	    }
	  else
	    {
	      colorsInUse++;
	      IndexToBits(palPtr, curIndex);
	      lengthOfRun++;
	    }
	}
    }
  advancePtr = TRUE;
  if (advancePtr && findSuperset)	    /* Do we even care about this palette? */
    advancePtr = PalIsSuperset(Palette[0],bPU, palPtr);
  if (advancePtr && findSubset)
    advancePtr = PalIsSubset(Palette[1],bPU, palPtr);
  if (advancePtr)
    *savePtr += ComputeModSavings(curDepth, curFormat, dciFormatDepth, startOfRun, lengthOfRun,pixels);


  /* 1.06 Get intermediate results */

  for (i=0; i<FPAL_RESAVE; i++)
    {
      reSavings[i]=rePal[i]=0; 	
    }
  for (i=0; i<=lIB; i++)
    {
      for (j=0; j<FPAL_RESAVE; j++)
	{
	  tempSavings = reSavings[j];
	  if ((savings[i]) > tempSavings)
	    {
	      for (k=(FPAL_RESAVE-1); k>j; k--)
		{
		  rePal[k] = rePal[k-1];
		  reSavings[k] = reSavings[k-1];
		}
	      rePal[j] = i;
	      reSavings[j]= savings[i];
	      break;
	    }
	}
    }
  /* Get the TRUE savings for the actual using each Palette */
  for (i=0; i<FPAL_RESAVE; i++)
    {
      rleCount= 0;
      reSavings[i] = 0;
      endOfRun = startOfRun = 0;
      inRun = FALSE;
      palPtr = palUsage + (rePal[i])*bPU;
      for (j=0; j<pixels; j++)
	{
	  if (ColorInPal(palPtr,myIndex->Index[j]))
	    {
	      if (!inRun)
		{
		  endOfRun = startOfRun = j;
		  inRun = TRUE;
		}
	    }
	  else if (inRun)
	    {
	      inRun = FALSE;
	      endOfRun = j-1;
	      lengthOfRun = endOfRun - startOfRun +1;
	      reSavings[i] += ComputeModSavings(curDepth, curFormat, dciFormatDepth, startOfRun, lengthOfRun,pixels);
	      rleCount++;
	    }
	}
      if (inRun)
	{
	  endOfRun = pixels-1;
	  lengthOfRun = endOfRun - startOfRun + 1;
	  reSavings[i] += ComputeModSavings(curDepth, curFormat, dciFormatDepth, startOfRun, lengthOfRun,pixels);  
	}					
    }
		
  FinalSavings[0]=0;
  FinalSavings[1]=0;
  FinalSavings[2]=0;
	
  /*  Copy Final results */
  for (i=0; i<FPAL_RESAVE; i++)
    {
      for (j=0; j<3; j++)
	{
	  tempSavings = FinalSavings[j];
	  if ((reSavings[i]) > tempSavings)
	    {
	      for (k=2; k>j; k--)
		{
		  bestPal[k] = bestPal[k-1];
		  FinalSavings[k] = FinalSavings[k-1];
		}
	      bestPal[j] = rePal[i];
	      FinalSavings[j]= reSavings[i];
	      break;
	    }
	}
    }	
		
  for (i=0; i<3; i++)
    {
      if (bestPal[i] <= lIB)
	for (j=0; j<bPU; j++)
	  {
	    tempIn = *((Palette[i])+j)= *(palUsage+bPU*(bestPal[i])+j);
	    if (tempIn != 0)
	      tempIn = 1;

	  }
    }
  free(savings);
  free(palUsage);
  return (M2E_NoErr);
}

/* Try to "intelligently" assign formats to runs to give the absolute best compression */

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_SmartFormatAssign
|||	Create an M2TXIndex image that indicates which format each pixel should be
|||	assigned to maximum compression.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_SmartFormatAssign(M2TXIndex *myIndex, M2TXIndex *format,
|||	    bool matchColor, bool matchAlpha, bool matchSSB,
|||	    M2TXDCI *dci, M2TXPIP *pip);
|||	
|||	  Description
|||	    This function takes an M2TXIndex, a DCI, a PIP, and flags indicating 
|||	    what channels to use.  It outputs a an M2TXIndex file that indicates
|||	    which format each pixel is assigned.  The format assignment is 
|||	    calculated for maximum compression.
|||	
|||	  Arguments
|||	    
|||	    myIndex
|||	        The input M2TXIndex image.
|||	    dci
|||	        The DCI associated with the input image.
|||	    pip
|||	        The PIP associated with the input image.
|||	    format
|||	        The M2TXIndex image which indicates which texel format (0-3) each pixel in myIndex should be assigned to for optimal compression.
|||	    matchColor
|||	        Whether the color data should used in creating the compressed M2TXIndex image.
|||	    matchAlpha
|||	        Whether the alpha data should used in creating the compressed M2TXIndex image.
|||	    matchSSB
|||	        Whether the ssb data should used in creating the compressed M2TXIndex image.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXbestcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

M2Err M2TXIndex_SmartFormatAssign(M2TXIndex *myIndex, M2TXIndex *format, 
				  bool matchColor, bool matchAlpha, bool matchSSB,
				  M2TXDCI *dci, M2TXPIP *pip)
{
  uint32 pixels, curPixel, memSize,i,k,l,m;
  uint32 cIndex, startOfRun, endOfRun, newStart, newEnd, testStart, testEnd;
  uint32 runLength;
  long savings, curSize, newSize;
  int j;
  uint32 numColors[4];
  uint8 red, green, blue, ssb, alpha;
  uint8 cssb[4], cred[4], cgreen[4], cblue[4], calpha[4];
  uint8 tssb[4], tred[4], tgreen[4], tblue[4], talpha[4];
  uint8 cDepth[4], aDepth[4], ssbDepth[4], depth[4];
  uint8 fmtsOrder[4];
  bool isTrans[4], hasColor[4], hasAlpha[4], hasSSB[4], matches, inRun;
  M2Err err;
  M2TXColor colorConst[4], color;

  for (i=0; i<4; i++)
    {
      err = M2TXDCI_GetFIsTrans(dci, i, &(isTrans[i]));		
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

      err = M2TXDCI_GetColorConst(dci, i, &(colorConst[i]));
      numColors[i] = 1 << cDepth[i];
      M2TXColor_Decode(colorConst[i],&(cssb[i]),&(calpha[i]),&(cred[i]),
		       &(cgreen[i]),&(cblue[i]));
      if (hasSSB[i]) 
	ssbDepth[i] = 1;
      else
	ssbDepth[i] = 0;
	 		
      if (isTrans[i])
	{
	  depth[i] = 0;
	  M2TXColor_Decode(pip->PIPData[cblue[i]], &(tssb[i]), &(talpha[i]), &(tred[i]),
			   &(tgreen[i]), &(tblue[i]));

	}
      else
	depth[i] = ssbDepth[i] + aDepth[i] + cDepth[i];	
    }
	
	
  k=0;							/* Get the formats into an ascending total bitdepth */
  for (i=0; i<=16; i++)
    for (j=0; j<4; j++)	
      if (depth[j] == i)   /* change this to account for same cdepth but with an alpha */
	{
	  fmtsOrder[k] = j;
	  k++;
	}	

  pixels = (myIndex->XSize) * (myIndex->YSize);
  memSize = pixels;
  /*	format->XSize = myIndex->XSize;    // 1.06
	format->YSize = myIndex->YSize;
	format->Index = (uint8 *) qMemNewPtr(memSize);
	format->Alpha = (uint8 *) qMemNewPtr(memSize);
	format->SSB = (uint8 *) qMemNewPtr(memSize);
	*/
  for (curPixel=0; curPixel<pixels; curPixel++)
    {
      format->Index[curPixel]=fmtsOrder[3];     /* Everybody gets the worst format initially */
      if (curPixel%32)
	format->Alpha[curPixel] = 0;		/* Use Alpha channel to mark of control byte usage */
      else
	format->Alpha[curPixel] = 8;
    }
	
  for (j=2; j>=0; j--)
    {
      startOfRun=0;
      inRun = FALSE;
      endOfRun=0;		
      i = fmtsOrder[j];

      for (curPixel=0; curPixel<pixels; curPixel++)
	{
	  red = green = blue = ssb = 0; alpha = 0xFF;
	  if (matchColor)						/* 1.04 Maybe put a lookup table here */
	    {
	      cIndex = myIndex->Index[curPixel];
	      M2TXPIP_GetColor(pip, cIndex, &color);
	      M2TXColor_Decode(color, &ssb, &alpha, &red, &green, &blue);
	    }
	  if (matchAlpha)
	    alpha = myIndex->Alpha[curPixel];
	  if (matchSSB)
	    ssb = myIndex->SSB[curPixel];

	  if (isTrans[i]) /* Check for the const color first */
	    {
	      /* Does the color match the color const
		 If it does assign it and break */
	      matches = TRUE;
	      if ((tred[i] != red) || (tgreen[i] != green) || (tblue[i] != blue))
		matches = FALSE;
	      if (calpha[i] != alpha)
		matches = FALSE;					
	      if (ssb != cssb[i])
		matches = FALSE;
	    }
	  else
	    {	/* Is there a match, Index, Alpha, and SSB? */
	      matches = TRUE;
	      if (matchColor)
		{
		  if (hasColor[i])  
		    {
		      if (cIndex >= (numColors[i]))
			{
			  matches = FALSE;
			}
		    }
		  else	/* If no color, maybe it matches color const */
		    {
		      if ((cred[i] != red) || (cgreen[i] != green) || (cblue[i] != blue))
			matches = FALSE;
		      if (!matchAlpha)		
			/* Const color alpha must match that of PIP if no override */
			if (calpha[i] != alpha)
			  matches = FALSE;					
		      if (!matchSSB)		
			/* Const color ssb must match that of PIP if no override */
			if (ssb != cssb[i])
			  matches = FALSE;
		    }
		}
	      if (matchAlpha && matches)
		{
		  if (hasAlpha[i])  		  
		    {  /* Check that the alpha can be contained in format */
		      if (aDepth[i] == 4)    
			/* Only 7 and 4 bits for alpha, see if alpha can cram in 4 */
			if (alpha & 0x0F)
			  matches = FALSE;
		    }
		  else						
		    /* If no alpha, maybe it matches color const's alpha */
		    if (calpha[i] != alpha)
		      matches = FALSE;					
		}
	      if (matchSSB && matches)
		if (!(hasSSB[i]))  			
		  /* If no ssb, maybe it matches color const's ssb  */
		  if (ssb != cssb[i])
		    matches = FALSE;
	    }
	  if (matches)					/* Update run information */
	    {	
	      if (inRun)
		endOfRun++;
	      else
		{
		  endOfRun = startOfRun = curPixel;
		  inRun = TRUE;
		}
	    }
	  if((!matches) || (curPixel == (pixels-1)))
	    if(inRun)					/* At end of run */
	      {
		inRun = FALSE;			
		savings = 0;		
		for (m=0; m<4; m++)			/* Check four cases of the run */
		  {
		    testStart = startOfRun;
		    testEnd = endOfRun;
		    switch (m)
		      {
		      case 0:				/* the run itself */
			break;
		      case 2:				/* the run with the start truncated	*/
			do
			  {
			    testStart++;
			  } while ((format->Alpha[testStart] == 0) && (testStart < testEnd));
			break;	
		      case 1:				/* the run with the end truncated */
			do
			  {
			    if (testEnd != 0)
			      testEnd--;
			  } while ((format->SSB[testEnd] == 0) && (testStart < testEnd));
			if(testStart < testEnd)
			  testEnd--;
			break;
		      case 3:				/* the run with the start and end truncated */
		      default:
			do
			  {
			    testStart++;
			  } while ((format->Alpha[testStart] == 0) && (testStart < testEnd));
			do
			  {
			    if (testEnd != 0)
			      testEnd--;
			  } while ((format->SSB[testEnd] == 0) && (testStart < testEnd));
			if(testStart < testEnd)
			  testEnd--;
			break;
		      }
		    if (testStart <= testEnd)
		      {
			runLength = testEnd - testStart+1;
			newSize = runLength*depth[i];
			newSize += 16 + (runLength/32)*8;      /* Account for control bytes */
			if (testEnd == (pixels-1))				/* Don't need to continue run past end */
			  newSize -= 8;
			if (!(runLength%32))					/* In case of break on an even boundary */
			  newSize -= 8;
			curSize = 0;
			for (k=testStart; k<= testEnd; k++)  /* Determine if it's worthwhile to switch format */
			  {
			    curSize += depth[format->Index[k]];
			    curSize += format->Alpha[k];	   /* Alpha is used to mark off control bytes usage */
			  }
			if (k < pixels)		/* Account for case when end of new run would coinincide with end of old run */
			  curSize += format->Alpha[k];
			if ((curSize - newSize) > savings)
			  {
			    savings = curSize - newSize;
			    newStart = testStart;
			    newEnd = testEnd;
			    if (m != 0)
			      newEnd = testEnd;
			  }
			if ((m==0) || (m==2))
			  for (k=testStart,l=0; k<= testEnd; k++,l++)  /* Determine if it's worthwhile to switch format */
			    if (l%32)
			      format->SSB[k] = 0; /* Use SSB channel to mark of control byte usage */
			    else
			      format->SSB[k] = 8;

		      }	
		  }
		if (savings > 0)    /* It's worth it */
		  {
		    for (k=newStart,l=0; k<=newEnd; k++, l++)
		      {
			format->Index[k] = i;
			if (l%32)
			  format->Alpha[k] = 0; /* Use Alpha channel to mark of control byte usage */
			else
			  format->Alpha[k] = 8;
		      }
		    if (k < pixels)		/* We have to start the old run back up again */
		      format->Alpha[k] = 8;
		  }
	      }
	}		
    }
  /* qMemReleasePtr(format->Alpha);  We need this info sometimes */
  /* format->Alpha = NULL; */
  /*	qMemReleasePtr(format->SSB);
 	format->SSB = NULL;
	*/
  return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_FindBestDCIPIP
|||	Given a valid M2TXIndex image, find the best PIP and DCI to get maximum
|||	compression using multi-texel compression.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_FindBestDCIPIP(M2TXIndex *myIndex,  M2TX *tex, 
|||	                                   M2TXDCI *dci, M2TXPIP *newPIP, 
|||	                                   uint8 depth, uint8 aDepth, bool hasSSB)
|||	
|||	  Description
|||	    This function takes in an M2TXIndex image, an output texture, a color
|||	    depth, an alpha depth, and a hasSSB flag to specify the final output 
|||	    texture type.  It goes through a lengthy heuristic to find the best PIP 
|||	    order and DCI (which specifies the multi-texel formats) for compression.
|||	    On exit, newPIP and dci contain these optimal settings.  newPIP 
|||	    will also have a properly set sort table to convert from the input PIP 
|||	    to the newPIP.
|||	
|||	  Arguments
|||	    
|||	    myIndex
|||	        The input M2TXIndex image.
|||	    tex
|||	        The input M2TX texture.
|||	    dci
|||	        The output DCI that yields the best compression.
|||	    newPIP
|||	        The input PIP and the output PIP that yields the best compression.
|||	    depth
|||	        The color depth of the image (there should be 2^depth PIP entries).
|||	    aDepth
|||	        The alpha depth of the input image.
|||	    hasSSB
|||	        Whether the the input image has imbedded SSB information.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXbestcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_FindBestDCI()
**/

M2Err M2TXIndex_FindBestDCIPIP(M2TXIndex *myIndex, M2TX *tex, M2TXDCI *dci, M2TXPIP *newPIP, 
			       uint8 depth, uint8 aDepth, bool hasSSB)
{
  M2Err err;
  bool matchColor, matchAlpha;
  uint8  bPU, aboveIndex, belowIndex, aboveDepth, belowDepth, bestDepth[4], passFormat;
  uint32 colorSavings[3], pixels, ssbDepth, cDepth, passSavings[3];
  uint8 *colorPal[3], *bestPal[4], *passPal[3], passDepth[3], constColor[3], sortIndex;
  uint8 swapIndex1, swapIndex2, tmpPIPData[256];
  int i, j, k, l, numColors, txF, constIndex, constColorIndex;
  uint16 originalTexFormat, texFormat, constColorTexFormat, depthColors, swapIn, swapOut;
  long bestSavings[4];
  uint32 width, height;
  bool inUse[8], hasColorConst;
  M2TXPIP *oldPIP, tmpPIP;
  M2TXHeader *header;
  M2TXIndex curFormat;
  int16 temp16;
  uint32  temp32;
  M2TXColor color, headerColor, swapTmp;			/* 1.05 */
				
  for (i=0; i<depth; i++)
    inUse[i] = FALSE;
 		
  width = myIndex->XSize;
  height = myIndex->YSize;
  pixels = width*height;
  if (pixels <= 0)
    return (M2E_Range);
  numColors = 1 << depth;

  bPU = numColors/8;
  if (numColors % 8)
    bPU++;

  for (i=0; i<4; i++)
    bestPal[i] = (uint8 *)calloc(bPU,sizeof(uint8));
  for (i=0; i<3; i++)
    {
      colorPal[i] = (uint8 *)calloc(bPU,sizeof(uint8));
      passPal[i] = (uint8 *)calloc(bPU,sizeof(uint8));
    }
  cDepth = depth;
  if (cDepth>0)
    matchColor = TRUE;
  else
    matchColor = FALSE;
  if (aDepth>0)
    matchAlpha = TRUE;
  else
    matchAlpha = FALSE;
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXIndex_Init(&curFormat, width, height, TRUE, TRUE, TRUE);   /* 1.07 */	

  /* set up the dci */
  M2TX_GetHeader(tex, &header);
  M2TX_GetPIP(tex, &oldPIP);
  M2TXHeader_GetTexFormat(header,&texFormat);
  M2TXHeader_GetFHasColorConst(header, &hasColorConst);		 
  if (hasColorConst)											/* 1.05 */
    M2TXHeader_GetColorConst(header,0, &headerColor);		/* 1.05 */
  else														/* 1.05 */
    headerColor = 0x7F000000;								/* 1.05 */
  M2TXFormat_SetFIsLiteral(&texFormat, FALSE);
  M2TXFormat_SetADepth(&texFormat, aDepth);
  if (ssbDepth > 0)
    M2TXFormat_SetFHasSSB(&texFormat, TRUE);
  else 
    M2TXFormat_SetFHasSSB(&texFormat, FALSE);
  if (cDepth > 0)
    M2TXFormat_SetFHasColor(&texFormat, TRUE);
  else 
    M2TXFormat_SetFHasColor(&texFormat, FALSE);
  if (aDepth > 0)
    M2TXFormat_SetFHasAlpha(&texFormat, TRUE);
  else 
    M2TXFormat_SetFHasAlpha(&texFormat, FALSE);
	
  M2TXFormat_SetCDepth(&texFormat, depth);
  originalTexFormat = constColorTexFormat = texFormat;						
  M2TXFormat_SetFIsTrans(&constColorTexFormat,TRUE);
  M2TXFormat_SetFHasColor(&constColorTexFormat,FALSE);
  M2TXFormat_SetCDepth(&constColorTexFormat,0);

  /* Initially everyone is set to the same comp format */
	
  for (i=0; i<4; i++)
    {
      M2TXDCI_SetTexFormat(dci, i, texFormat);
      M2TXDCI_SetColorConst(dci,i, headerColor);				/* 1.05 */	
    }
  bestDepth[0]=depth;
  bestSavings[0]=0;
  constColorIndex=0;
  for (i=0; i<numColors; i++)
    {
      newPIP->PIPData[i] = oldPIP->PIPData[i];
      M2TXPIP_SetSortIndex(newPIP,i,i);
    }
  M2TXPIP_GetNumColors(oldPIP, &temp16);
  M2TXPIP_SetNumColors(newPIP, temp16);
  M2TXPIP_GetIndexOffset(oldPIP, &temp32);
  M2TXPIP_SetIndexOffset(newPIP, temp32);

  /* 3 texel formats not spoken for =  3 iterations */
  for (txF=1; txF<4; txF++)
    {		
      err = M2TXIndex_SmartFormatAssign(myIndex, &curFormat, matchColor, matchAlpha, hasSSB, dci, newPIP);
      for (i=0; i<depth; i++) 
	{
	  cDepth = i;
	  if ((!inUse[i]) && (M2TX_IsLegal(cDepth, aDepth, ssbDepth, FALSE)))	
	    {   /* if bitdepth format is already in use or illegal, then skip
		   Get Palette restrictions */
	      if (i==0)
		{
		  for (j=0; j<bPU; j++)
		    {
		      *((passPal[0])+j) = 0;						
		      *((passPal[1])+j) = 0;						
		    }
		  err = Find_BestPalette(myIndex, &curFormat, depth, i, dci, passSavings, passPal);
		  passDepth[0] = passDepth[1] = passDepth[2] = 0;
		  if (constColorIndex!=0)
		    {
		      for (constIndex=0; constIndex<3; constIndex++)
			for (j=0; j<constColorIndex; j++)
			  if (ColorInPal(passPal[constIndex], constColor[j]))
			    {
			      for (k=constIndex; k<2; k++)
				{
				  passSavings[k] = passSavings[k+1];
				  for(l=0; l<bPU; l++)
				    *((passPal[k])+l) = *((passPal[k+1])+l);
				}
			      passSavings[2] = 0;
			    }									
		    }
		}
	      else
		{
		  aboveDepth = depth;    /* 1.06 */
		  belowDepth = 0;
		  for (j=1; j<txF; j++)
		    {
		      if (bestDepth[j]<i)
			{
			  if (bestDepth[j] > belowDepth)
			    {
			      belowDepth = bestDepth[j];
			      belowIndex = j;
			    }
			}
		      else if (bestDepth[j]>i)
			if (bestDepth[j]< aboveDepth)
			  {
			    aboveDepth = bestDepth[j];
			    aboveIndex = j;
			  }
		    }
		  for (j=0; j<bPU; j++)
		    {
		      *((colorPal[0])+j) = 0;						
		      *((colorPal[1])+j) = 0;						
		    }
		  /* if index is lower than restrict palette, set input palette[1] to restrict palette */
		  if (aboveDepth != depth)	/* we want a subset of the existing restricting palette */
		    for (j=0; j<bPU; j++)
		      *((colorPal[1])+j) = *((bestPal[aboveIndex])+j);
		  /* if index is higher than restrict palette, set input palette[0] to restrict palette */
		  if	(belowDepth != 0)				/* we want a superset of the existing restricting palette */
		    for (j=0; j<bPU; j++)
		      *((colorPal[0])+j) = *((bestPal[belowIndex])+j);

		  err = Find_BestPalette(myIndex, &curFormat, depth, i, dci, colorSavings, colorPal);
					
		  /* Maintain top 3 choices for the very first pass */
		  for (j=0; j<3; j++)
		    {
		      if (colorSavings[0] > passSavings[j]) 
			{
			  /* keep track of best savings and best index */
			  /* Check if this is a const color already in use */
			  /* If it is you must check the next color const */
						
			  for (k=2; k>j; k--)
			    {
			      passSavings[k] = passSavings[k-1];
			      passDepth[k] = passDepth[k-1];
			      for(l=0; l<bPU; l++)
				*((passPal[k])+l) = *((passPal[k-1])+l);
			    }
			  passSavings[j] = colorSavings[0];
			  passDepth[j] = i;
			  for(l=0; l<bPU; l++)
			    *((passPal[j])+l) = *((colorPal[0])+l);
			  break;

			}
		    }
		}
	    }	
	}
		
      passFormat = 0;
			
      bestDepth[txF] = passDepth[passFormat];
      bestSavings[txF] = passSavings[passFormat];
      if (bestDepth[txF]!=0)
	inUse[bestDepth[txF]] = TRUE;
					
      for(l=0; l<bPU; l++)
	*((bestPal[txF])+l) = *((passPal[passFormat])+l);
		
      PrintPal(bestPal[txF], numColors);    /* For diagnostics */

      /* Add new format to the DCI */
      if (bestDepth[txF]==0)
	{	/* Constant Colors are handled a little differently */
	  M2TXDCI_SetTexFormat(dci, txF, constColorTexFormat);
	  for (j=0; j<numColors; j++)
	    if (ColorInPal(bestPal[txF], j))
	      {
		/*   M2TXPIP_GetColor(newPIP, j,&color); */   /* 1.06 */
		color = M2TXColor_Create(0, 0, 0, 0, j);  /* 1.2 */
		M2TXDCI_SetColorConst(dci, txF, color);
		constColor[constColorIndex] = j;
		constColorIndex++;
		break;
	      }
	}
      else				
	{
	  cDepth = bestDepth[txF];
	  texFormat = originalTexFormat;              /* 1.06 */
	  M2TXFormat_SetCDepth(&texFormat, cDepth);
	  M2TXDCI_SetTexFormat(dci, txF, texFormat);
	  M2TXDCI_SetColorConst(dci, txF, headerColor);			/* 1.05 */
	}		
					
      if (bestDepth[txF] > 0)    /* We don't do anything for constant colors */
	{
	  depthColors = 1 << bestDepth[txF];
	  swapOut=0;
	  for (i=0; i<256; i++)
	    M2TXPIP_SetSortIndex(&tmpPIP,i,i);
	  for (i=depthColors; i<numColors; i++)
	    {
	      if (ColorInPal(bestPal[txF],i))
		{
		  swapIn=i;
		  while (ColorInPal(bestPal[txF],swapOut))
		    swapOut++;
		  M2TXPIP_SetSortIndex(&tmpPIP,swapOut, swapIn);
		  M2TXPIP_SetSortIndex(&tmpPIP,swapIn, swapOut);
		  swapTmp=newPIP->PIPData[swapIn];
		  newPIP->PIPData[swapIn] = newPIP->PIPData[swapOut];
		  newPIP->PIPData[swapOut] = swapTmp;
		  M2TXPIP_GetSortIndex(newPIP, swapOut, &swapIndex1);
		  M2TXPIP_GetSortIndex(newPIP, swapIn, &swapIndex2);
		  M2TXPIP_SetSortIndex(newPIP,swapOut, swapIndex2);
		  M2TXPIP_SetSortIndex(newPIP,swapIn, swapIndex1);
		  swapOut++;
		}
	    }
	  /* Fix all the palettes in bestPal so restrictions will come out right*/
	  for (i=1; i<=txF; i++)
	    {
	      for (j=0; j<bPU; j++)	
		*((bestPal[i])+j) = 0;
	      for (j=0; j<(1<<bestDepth[i]); j++)
		IndexToBits(bestPal[i],j); 
	    }
	  swapIndex1 = 0;
	  for (i=1; i<=txF; i++)
	    if (bestDepth[i] == 0)
	      {
		M2TXPIP_GetSortIndex(&tmpPIP,constColor[swapIndex1],&sortIndex);
		constColor[swapIndex1] = sortIndex;
		swapIndex1++;
	      }
	  M2TXIndex_ReorderToPIP(myIndex, &tmpPIP, depth);	
	}
    }
  M2TXIndex_Free(&curFormat);

  for (i=0; i<numColors; i++)
    {
      M2TXPIP_GetSortIndex(newPIP,i,&swapIndex1);
      tmpPIPData[swapIndex1] = i;
    }
  for (i=0; i<numColors; i++)
    M2TXPIP_SetSortIndex(newPIP,i, tmpPIPData[i]); 

  return (M2E_NoErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_FindBestDCI
|||	Find the best DCI for a texture given an M2TXIndex image and an M2TX 
|||	texture as constraints.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_FindBestDCI(M2TXIndex *myIndex, M2TX *tex, M2TXDCI *dci,
|||	                                uint8 depth, uint8 aDepth, bool hasSSB)
|||	
|||	  Description
|||	    This function takes in an M2TXIndex image, an output texture, a color 
|||	    depth, an alpha depth, and a hasSSB flag to specify the final output
|||	    texture type.  It goes through a lengthy heuristic to find the best DCI 
|||	    (which specifies the multi-texel formats) for compression.  On exit, the dci
|||	    contains these optimal settings. It is assumed that the PIP for the 
|||	    texture is already locked, so it can't be changed.
|||	
|||	  Arguments
|||	    
|||	    myIndex
|||	        The input M2TXIndex image.
|||	    tex
|||	        The input M2TX texture.
|||	    dci
|||	        The output DCI that yields the best compression.
|||	    depth
|||	        The color depth of the image (there should be 2^depth PIP entries).
|||	    aDepth
|||	        The alpha depth of the input image.
|||	    hasSSB
|||	        Whether the the input image has imbedded SSB information.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXbestcompress.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_FindBestDCIPIP()
**/

M2Err M2TXIndex_FindBestDCI(M2TXIndex *myIndex, M2TX *tex, M2TXDCI *dci, uint8 depth,
			    uint8 aDepth, bool hasSSB)
{
  M2Err err;
  bool matchColor, matchAlpha;
  uint8  bPU, bestDepth[4], passFormat;
  uint32 colorSavings[3], pixels, ssbDepth, cDepth, passSavings[3];
  uint8 *colorPal[3], *bestPal[4], *passPal[3], passDepth[3], constColor[3];
  int i, j, k, l, numColors, txF, constIndex, constColorIndex;
  uint16 originalTexFormat, texFormat, constColorTexFormat;
  long bestSavings[4];
  uint32 width, height;
  bool inUse[8], hasColorConst;
  M2TXPIP *oldPIP;
  M2TXHeader *header;
  M2TXIndex curFormat;
  M2TXColor color, headerColor;			/* 1.05 */
				
  for (i=0; i<depth; i++)
    inUse[i] = FALSE;
		
  width = myIndex->XSize;
  height = myIndex->YSize;
  pixels = width*height;
  if (pixels <= 0)
    return (M2E_Range);
  numColors = 1 << depth;
  M2TXIndex_Init(&curFormat, width, height, TRUE, TRUE, TRUE);	

  bPU = numColors/8;
  if (numColors % 8)
    bPU++;

  for (i=0; i<4; i++)
    bestPal[i] = (uint8 *)calloc(bPU,sizeof(uint8));

  for (i=0; i<3; i++)
    {
      colorPal[i] = (uint8 *)calloc(bPU,sizeof(uint8));
      passPal[i] = (uint8 *)calloc(bPU,sizeof(uint8));
    }
  cDepth = depth;
  if (cDepth>0)
    matchColor = TRUE;
  else
    matchColor = FALSE;
  if (aDepth>0)
    matchAlpha = TRUE;
  else
    matchAlpha = FALSE;
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;

  /* set up the dci */
  M2TX_GetHeader(tex, &header);
  M2TX_GetPIP(tex, &oldPIP);
  M2TXHeader_GetTexFormat(header,&texFormat);
  M2TXHeader_GetFHasColorConst(header, &hasColorConst);		 
  if (hasColorConst)											/* 1.05 */
    M2TXHeader_GetColorConst(header,0, &headerColor);		/* 1.05 */
  else														/* 1.05 */
    headerColor = 0x7F000000;								/* 1.05 */
  M2TXFormat_SetFIsLiteral(&texFormat, FALSE);
  M2TXFormat_SetADepth(&texFormat, aDepth);
  if (ssbDepth > 0)
    M2TXFormat_SetFHasSSB(&texFormat, TRUE);
  else 
    M2TXFormat_SetFHasSSB(&texFormat, FALSE);
  if (cDepth > 0)
    M2TXFormat_SetFHasColor(&texFormat, TRUE);
  else 
    M2TXFormat_SetFHasColor(&texFormat, FALSE);
  if (aDepth > 0)
    M2TXFormat_SetFHasAlpha(&texFormat, TRUE);
  else 
    M2TXFormat_SetFHasAlpha(&texFormat, FALSE);
	
  M2TXFormat_SetCDepth(&texFormat, depth);
  originalTexFormat = constColorTexFormat = texFormat;						
  M2TXFormat_SetFIsTrans(&constColorTexFormat,TRUE);
  M2TXFormat_SetFHasColor(&constColorTexFormat,FALSE);
  M2TXFormat_SetCDepth(&constColorTexFormat,0);

  /* Initially everyone is set to the same comp format */
	
  for (i=0; i<4; i++)
    {
      M2TXDCI_SetTexFormat(dci, i, texFormat);
      M2TXDCI_SetColorConst(dci,i, headerColor);				/* 1.05 */	
    }
  bestDepth[0]=depth;
  bestSavings[0]=0;
  constColorIndex=0;

  /* 3 texel formats not spoken for =  3 iterations */
  for (txF=1; txF<4; txF++)
    {		
      err = M2TXIndex_SmartFormatAssign(myIndex, &curFormat, matchColor, matchAlpha, hasSSB, dci, oldPIP);
      for (i=0; i<depth; i++) 
	{
	  cDepth = i;
	  if ((!inUse[i]) && (M2TX_IsLegal(cDepth, aDepth, ssbDepth, FALSE)))	
	    {   /* if bitdepth format is already in use or illegal, then skip
		   Get Palette restrictions */
	      if (i==0)
		{
		  for (j=0; j<bPU; j++)
		    {
		      *((passPal[0])+j) = 0;						
		      *((passPal[1])+j) = 0;						
		    }
		  err = Find_BestPalette(myIndex, &curFormat, depth, i, dci, passSavings, passPal);
		  passDepth[0] = passDepth[1] = passDepth[2] = 0;
		  if (constColorIndex!=0)
		    {
		      for (constIndex=0; constIndex<3; constIndex++)
			for (j=0; j<constColorIndex; j++)
			  if (ColorInPal(passPal[constIndex], constColor[j]))
			    {
			      for (k=constIndex; k<2; k++)
				{
				  passSavings[k] = passSavings[k+1];
				  for(l=0; l<bPU; l++)
				    *((passPal[k])+l) = *((passPal[k+1])+l);
				}
			      passSavings[2] = 0;
			    }									
		    }
		}
	      else
		{
		  for (j=0; j<bPU; j++)
		    {
		      *((colorPal[0])+j) = 0;						
		      *((colorPal[1])+j) = 0;						
		    }
		  /* Restrict the palette to the existing palette */
		  for (j=0; j<(1<<i); j++)   
		    IndexToBits(colorPal[0],j);

		  err = Find_BestPalette(myIndex, &curFormat, depth, i, dci, colorSavings, colorPal);
					
		  for (j=0; j<3; j++)
		    {
		      if (colorSavings[0] > passSavings[j]) 
			{	/* keep track of best savings and best index */
			  /*  Check if this is a const color already in use */
			  /* If it is you must check the next color const */
						
			  for (k=2; k>j; k--)
			    {
			      passSavings[k] = passSavings[k-1];
			      passDepth[k] = passDepth[k-1];
			      for(l=0; l<bPU; l++)
				*((passPal[k])+l) = *((passPal[k-1])+l);
			    }
			  passSavings[j] = colorSavings[0];
			  passDepth[j] = i;
			  for(l=0; l<bPU; l++)
			    *((passPal[j])+l) = *((colorPal[0])+l);
			  break;

			}
		    }
		}
	    }	
	}
		
      passFormat = 0;
			
      bestDepth[txF] = passDepth[passFormat];
      bestSavings[txF] = passSavings[passFormat];
      if (bestDepth[txF]!=0)
	inUse[bestDepth[txF]] = TRUE;
      for(l=0; l<bPU; l++)
	*((bestPal[txF])+l) = *((passPal[passFormat])+l);
			
      PrintPal(bestPal[txF], numColors);    /* For diagnostics */

      /* Add new format to the DCI */
      if (bestDepth[txF]==0)
	{	/* Constant Colors are handled a little differently */
	  M2TXDCI_SetTexFormat(dci, txF, constColorTexFormat);
	  for (j=0; j<numColors; j++)
	    if (ColorInPal(bestPal[txF], j))
	      {
		/*   M2TXPIP_GetColor(newPIP, j,&color); */   /* 1.06 */
		color = M2TXColor_Create(0, 0, 0, 0, j);       /* 1.2 */
		M2TXDCI_SetColorConst(dci, txF, color);
		constColor[constColorIndex] = j;
		constColorIndex++;
		break;
	      }
	}
      else				
	{
	  cDepth = bestDepth[txF];
	  texFormat = originalTexFormat;              /* 1.06 */
	  M2TXFormat_SetCDepth(&texFormat, cDepth);
	  M2TXDCI_SetTexFormat(dci, txF, texFormat);
	  M2TXDCI_SetColorConst(dci, txF, headerColor);			/* 1.05 */
	}		
    }
  M2TXIndex_Free(&curFormat);				
  return (M2E_NoErr);
}
