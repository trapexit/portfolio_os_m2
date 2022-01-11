/*
	File:		M2TXattr.c

	Contains:	M2 Texture Library, texture blend attributes 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <6>	  9/6/95	TMA		Fixed infinite loop in M2TX_GetAttr
		<4+>	  8/4/95	TMA		Removed unused variables.
		 <4>	  8/4/95	TMA		Added M2TXTA, M2TXLR, and M2TXDB Set, Get, and Remove functions.
		 <2>	 7/11/95	TMA		Fix warning messages about type mismatches.
	To Do:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "M2TXTypes.h"
#include "M2TXHeader.h"
#include "M2TXLibrary.h"
#include "qstream.h"
#include "qmem.h"
#include "M2TXattr.h"

typedef int32   Err;
#include "clt.h"
#include "clttxdblend.h"


static void putMemLong(uint32 val)
{
	uint8 buf;
	
	buf = val >> 24;      /* UTF specifies Motorala or MSB 1st */
	qMemPutBytes((BYTE *)&buf,  1);
	buf = (val & 0x00FF0000) >> 16;
	qMemPutBytes((BYTE *)&buf,  1);
	buf = (val & 0x0000FF00) >> 8;
	qMemPutBytes((BYTE *)&buf,  1);
	buf = val & 0x000000FF;
	qMemPutBytes((BYTE *)&buf,  1);
}

/* Remove and existing attribute(s) */
M2Err M2TXTA_RemoveAttr(M2TXTA *tab, uint32 attr)
{
  uint32 *ptr;
  uint32 *lastPtr, *endPtr;
  uint32 newSize;
  uint32 *temp;
  uint32 *clt;
  uint32 size;

  clt = tab->Tx_Attr;
  size = tab->Size-4;   /* Reserved byte doesn't count */

  if (clt == NULL)
    return (M2E_BadPtr);
  newSize = size;
  lastPtr = ptr = clt;
  endPtr = ptr+(size>>2);
  while (ptr < endPtr)
    {
      if (*ptr != attr)
	{
	  if (lastPtr != ptr)      /* Shift the contents up by one */
	    {
	      *lastPtr = *ptr;
	      *(lastPtr+1) = *(ptr+1);
	    }
	  lastPtr += 2;
	}
      else
	newSize -= 8;              /* We just removed an attribute/value pair */
      ptr += 2;
    }
  if ((size) != newSize)
    {
      if (newSize == 0)
	{
	  qMemReleasePtr(clt);
	  tab->Tx_Attr = NULL;
	}
      else
	{
	  tab->Size = newSize+4;
	  temp = (uint32 *)qMemResizePtr(clt, newSize);
	  if (temp == NULL)
	    return(M2E_NoErr);    /* It's not an error it couldn't resize */
	  tab->Tx_Attr = temp;
	}
    }
  return(M2E_NoErr);
}

/* If it exists, change it's value.  If not, add it */
M2Err M2TXTA_SetAttr(M2TXTA *tab, uint32 attr, uint32 value)
{
  
  uint32 *ptr;
  uint32 *endPtr;
  uint32 newSize;
  uint32 *clt;
  uint32 size;

  clt = tab->Tx_Attr;
  size = tab->Size-4;   /* Reserved byte doesn't count */
  
  if (clt == NULL)
    { 
      clt = (uint32 *)qMemNewPtr(8);
      if (clt == NULL)
	return(M2E_NoMem);
       tab->Size = 12;
      tab->Tx_Attr = clt;
      ptr = clt;
      *ptr = attr;
      *(ptr+1) = value;
        return(M2E_NoErr);
    }

  newSize = size;
  ptr = clt;
  endPtr = ptr+(size>>2);

  while (ptr < endPtr)    /* Get the new size if necessary */
    {
      if (*ptr == attr)
	break;
      ptr += 2;
    }
  if (ptr < endPtr)
    {
      while (ptr < endPtr)    /* Get the new size if necessary */
	{
	  if ((*ptr) == attr)
	    {
	      *(ptr+1) = value;
	    }
	  ptr += 2;
	}
    }
  else
    {
      newSize += 8;              /* We need to add an attribute/value pair */
      ptr = (uint32 *)qMemResizePtr(clt, newSize);
      if (ptr == NULL)
	return(M2E_NoMem);
      tab->Tx_Attr = ptr;
      endPtr = ptr+(size>>2);  /* Just to be safe */
      *endPtr = attr;
      *(endPtr+1) = value;
      tab->Size = newSize+4;
    }
  return(M2E_NoErr);
}

M2Err M2TXTA_GetAttr(M2TXTA *tab, uint32 attr, uint32 *value)
{
  uint32 *ptr;
  uint32 *endPtr;
  uint32 *clt;
  uint32 size;

  clt = tab->Tx_Attr;
  size = tab->Size-4;   /* Reserved byte doesn't count */

  if (clt == NULL)
    return (M2E_BadPtr);
  ptr = clt;
  endPtr = ptr+(size>>2);

  while (ptr < endPtr)    /* Get the new size if necessary */
    {
      if (*ptr == attr)
	{
	  *value = *(ptr+1);
	  return(M2E_NoErr);
	}
      ptr += 2;
    }
  return(M2E_BadPtr);
}

/* Remove and existing attribute(s) */
M2Err M2TXDB_RemoveAttr(M2TXDB *dab, uint32 attr)
{
  uint32 *ptr;
  uint32 *lastPtr, *endPtr;
  uint32 newSize;
  uint32 *temp;
  uint32 *clt;
  uint32 size;

  clt = dab->DBl_Attr;
  size = dab->Size-4;   /* Reserved byte doesn't count */

  if (clt == NULL)
    return (M2E_BadPtr);
  newSize = size;
  lastPtr = ptr = clt;
  endPtr = ptr+(size>>2);
  while (ptr < endPtr)
    {
      if (*ptr != attr)
	{
	  if (lastPtr != ptr)      /* Shift the contents up by one */
	    {
	      *lastPtr = *ptr;
	      *(lastPtr+1) = *(ptr+1);
	    }
	  lastPtr += 2;
	}
      else
	newSize -= 8;              /* We just removed an attribute/value pair */
      ptr += 2;
    }
  if ((size) != newSize)
    {
      if (newSize == 0)
	{
	  qMemReleasePtr(clt);
	  dab->DBl_Attr = NULL;
	}
      else
	{
	  dab->Size = newSize+4;
	  temp = (uint32 *)qMemResizePtr(clt, newSize);
	  if (temp == NULL)
	    return(M2E_NoErr);    /* It's not an error it couldn't resize */
	  dab->DBl_Attr = temp;
	}
    }
  return(M2E_NoErr);
}

/* If it exists, change it's value.  If not, add it */
M2Err M2TXDB_SetAttr(M2TXDB *dab, uint32 attr, uint32 value)
{
  
  uint32 *ptr;
  uint32 *endPtr;
  uint32 newSize;
  uint32 *clt;
  uint32 size;

  clt = dab->DBl_Attr;
  size = dab->Size-4;   /* Reserved byte doesn't count */
  
  if (clt == NULL)
    { 
      clt = (uint32 *)qMemNewPtr(8);
      if (clt == NULL)
	return(M2E_NoMem);
       dab->Size = 12;
      dab->DBl_Attr = clt;
      ptr = clt;
      *ptr = attr;
      *(ptr+1) = value;
        return(M2E_NoErr);
    }

  newSize = size;
  ptr = clt;
  endPtr = ptr+(size>>2);

  while (ptr < endPtr)    /* Get the new size if necessary */
    {
      if (*ptr == attr)
	break;
      ptr += 2;
    }
  if (ptr < endPtr)
    {
      while (ptr < endPtr)    /* Get the new size if necessary */
	{
	  if ((*ptr) == attr)
	    {
	      *(ptr+1) = value;
	    }
	  ptr += 2;
	}
    }
  else
    {
      newSize += 8;              /* We need to add an attribute/value pair */
      ptr = (uint32 *)qMemResizePtr(clt, newSize);
      if (ptr == NULL)
	return(M2E_NoMem);
      dab->DBl_Attr = ptr;
      endPtr = ptr+(size>>2);  /* Just to be safe */
      *endPtr = attr;
      *(endPtr+1) = value;
      dab->Size = newSize+4;
    }
  return(M2E_NoErr);
}

M2Err M2TXDB_GetAttr(M2TXDB *dab, uint32 attr, uint32 *value)
{
  uint32 *ptr;
  uint32 *endPtr;
  uint32 *clt;
  uint32 size;

  clt = dab->DBl_Attr;
  size = dab->Size-4;   /* Reserved byte doesn't count */

  if (clt == NULL)
    return (M2E_BadPtr);
  ptr = clt;
  endPtr = ptr+(size>>2);

  while (ptr < endPtr)    /* Get the new size if necessary */
    {
      if (*ptr == attr)
	{
	  *value = *(ptr+1);
	  return(M2E_NoErr);
	}
      ptr += 2;
    }
  return(M2E_BadPtr);
}

M2Err M2TXLR_Append(M2TXLR *lR, M2TXRect rect)
{
  M2TXRect *temp;
  uint32 size;

  if (lR->LRData == NULL) 
    { 
      lR->LRData = (M2TXRect *)qMemNewPtr(sizeof(M2TXRect));
      if ((lR->LRData) == NULL)
	return(M2E_NoMem);
      lR->NumLoadRects = 1;
      memcpy(&(lR->LRData[0]), &rect, sizeof(M2TXRect));
      return(M2E_NoErr);
    }
  size = lR->NumLoadRects;
  size++;
  temp = (M2TXRect *)qMemResizePtr(lR->LRData, size*sizeof(M2TXRect));
  if (temp == NULL)
    return(M2E_NoMem);
  lR->NumLoadRects = size;
  memcpy(&(lR->LRData[size-1]), &rect, sizeof(M2TXRect));
  return(M2E_NoErr);
}

M2Err M2TXLR_Set(M2TXLR *lR, uint32 index, M2TXRect rect)
{

  if (lR->LRData == NULL) 
    return(M2E_Range);
  if (lR->NumLoadRects < index)
    return(M2E_Range);
  
  memcpy(&(lR->LRData[index]), &rect, sizeof(M2TXRect));
  return(M2E_NoErr);
}

M2Err M2TXLR_Get(M2TXLR *lR, uint32 index, M2TXRect *rect)
{
  if (lR->LRData == NULL) 
    return(M2E_Range);
  if (lR->NumLoadRects < index)
    return(M2E_Range);
  
  memcpy(rect,&(lR->LRData[index]), sizeof(M2TXRect));
  return(M2E_NoErr);
}

M2Err M2TXLR_Remove(M2TXLR *lR, uint32 index)
{
  uint32 size, i;
  M2TXRect *temp;

  if (lR->LRData == NULL) 
    return(M2E_Range);
  if (lR->NumLoadRects < index)
    return(M2E_Range);
  
  size = lR->NumLoadRects;
  for (i=index+1; i<size; i++)
      memcpy(&(lR->LRData[i-1]), &(lR->LRData[i]), sizeof(M2TXRect));
  lR->NumLoadRects = size-1;
  temp = (M2TXRect *)qMemResizePtr(lR->LRData, size*sizeof(M2TXRect));
  if (temp == NULL)
    return(M2E_NoErr);
  lR->LRData = temp;
  return(M2E_NoErr);  
}

M2Err   M2TX_CreateTAB(M2TX *tex)
{
  M2Err err;
  M2TXHeader 	*header;
  bool flag, hasCC, hasSSB, isLiteral;
  uint32 color, value;
  
  err = M2TX_GetHeader(tex, &header);

  M2TXHeader_GetFHasSSB(header, &hasSSB);
  M2TXHeader_GetFHasColorConst(header, &hasCC);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  if (hasSSB)
    { 
     M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipSSBSelect, TX_PipSelectTexture);
    }
  else if (isLiteral)
    {
      M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipSSBSelect, TX_PipSelectConst);
     }
  else
    {
	  if ((M2TXTA_GetAttr(&(tex->TexAttr), TXA_PipSSBSelect, &value)) != M2E_NoErr)
	    M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipSSBSelect, TX_PipSelectColorTable);
	  else if (value == TX_PipSelectTexture)
      M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipSSBSelect, TX_PipSelectColorTable);
    }
  M2TXHeader_GetFHasColor(header, &flag);
  if (flag)
    {
      if (isLiteral)
	{
	  M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipColorSelect, TX_PipSelectTexture);
	}
      else
	{
	  if ((M2TXTA_GetAttr(&(tex->TexAttr), TXA_PipColorSelect, &value)) != M2E_NoErr)
	    M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipColorSelect, TX_PipSelectColorTable);
	  else if (value == TX_PipSelectTexture)
	    M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipColorSelect, TX_PipSelectColorTable);
	}
    }
  else
    {
      M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipColorSelect, TX_PipSelectConst);
    }

  M2TXHeader_GetFHasAlpha(header, &flag);
  if (flag)
    {
      M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipAlphaSelect, TX_PipSelectTexture);
    }
  else
    {
      if (isLiteral)
	{
	  M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipAlphaSelect, TX_PipSelectConst);
	}     
      else
	{
	  if ((M2TXTA_GetAttr(&(tex->TexAttr), TXA_PipAlphaSelect, &value)) != M2E_NoErr)
	    M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipAlphaSelect, TX_PipSelectColorTable);
	  else if (value == TX_PipSelectTexture)
	  M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipAlphaSelect, TX_PipSelectColorTable);
	}
    }
  if (hasCC)
    {
      M2TXHeader_GetColorConst(header, 0, &color);
      M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipConstSSB0, color);
      M2TXHeader_GetColorConst(header, 1, &color);
      M2TXTA_SetAttr(&(tex->TexAttr), TXA_PipConstSSB1, color);
    }

  if ((M2TXTA_GetAttr(&(tex->TexAttr), TXA_MinFilter, &value)) != M2E_NoErr)
    M2TXTA_SetAttr(&(tex->TexAttr), TXA_MinFilter,TX_Bilinear);
  if ((M2TXTA_GetAttr(&(tex->TexAttr), TXA_MagFilter, &value)) != M2E_NoErr)
    M2TXTA_SetAttr(&(tex->TexAttr), TXA_MagFilter,TX_Bilinear);
  if ((M2TXTA_GetAttr(&(tex->TexAttr), TXA_InterFilter, &value)) != M2E_NoErr)
    M2TXTA_SetAttr(&(tex->TexAttr), TXA_InterFilter,TX_QuasiTrilinear);

  return (M2E_NoErr);
}


M2Err   M2TX_SetDefaultLoadRect(M2TX *tex)
{
  M2Err err;
  M2TXHeader 	*header;
  uint8 numLOD;
  uint16 minSize;
  M2TXRect *rect;
  
  err = M2TX_GetHeader(tex, &header);
  if (err != M2E_NoErr)
    return(err);
  err = M2TXHeader_GetNumLOD(header, &numLOD);
  if (err != M2E_NoErr)
    return(err);

  rect = tex->LoadRects.LRData;
  if (rect == NULL)
    return (M2E_BadPtr);

  rect->FirstLOD = 0;          /* Set everything but wrap modes */
  if (numLOD >4)
    rect->NLOD = 4;
  else
    rect->NLOD = numLOD;

  rect->XOff = 0;
  rect->YOff = 0;
  M2TXHeader_GetMinXSize(header, &minSize);
  rect->XSize = (int16)minSize;
  M2TXHeader_GetMinYSize(header, &minSize);
  rect->XSize = (int16)minSize;
  
  return(M2E_NoErr);
}


