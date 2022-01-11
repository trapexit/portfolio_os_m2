/*
	File:		M2TXLibrary.c

	Contains:	M2 Texture Library 

	Written by:	Todd Allendorf

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<14>	12/17/95	TMA		Updated the M2TX_Print command to print out some more useful
									information. Made DCI default black opaque instead of white
									opaque.
	   <12+>	 9/27/95	TMA		Change includes and texture attributes to match system software.
		<12>	  8/7/95	TMA		Removed compiler warnings.
	   <10+>	  8/4/95	TMA		Fixed compiler warnings.
		<10>	  8/4/95	TMA		Added M2TX_Free function.
		 <9>	 7/15/95	TMA		Update autodocs.
		 <6>	 7/11/95	TMA		M2TX_Init M2TX_Print changed to accomodate new IFF UTF.
									M2TX_SetDefaultXWrap and M2TX_SetDefaultYWrap added.
		 <5>	  6/9/95	TMA		Fix some compiler warnings and sign extend alpha values.
		 <4>	 5/30/95	TMA		Added M2TXFilter functions.
		 <3>	 5/16/95	TMA		Autodocs added. ResizePtr in M2TX_Extract replaced with copy and
									free to get around Mac memory allocation problems.
		 <2>	 4/13/95	TMA		Order of parameters in M2TXColor_Decode and M2TXColor_Create
									were mixed up.
		 <4>	 2/19/95	TMA		Added new legal M2 types, added additional error checking during
									memory allocation
		 <3>	 1/20/95	TMA		Update error handling. Added PIP_Translation, and Raw_PIPCreate
									functions.

	To Do:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "M2TXlib.h"

typedef int32   Err;
#include "clt.h"
#include "clttxdblend.h"

#include "M2TXattr.h"
#include "qstream.h"
#include "qmem.h"
#include "filter.h"
#include "ReSample.h"

/* acessor to the M2TX structure, Get functions return a pointer to the existing 
   structure.
   Set functions COPY the information (including pointers) from the chunk to the
   field in M2TX. 
   */


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_GetHeader
|||	Return a pointer to the Header chunk of an M2TX texture already loaded into
|||	memory.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_GetHeader(M2TX *tex, M2TXHeader **header)
|||	
|||	  Description
|||	
|||	    This function makes header point to the Header chunk of an M2TX data
|||	    structure pointed to by tex.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    header
|||	        The M2TXHeader chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetHeader()     
**/

M2Err M2TX_GetHeader(M2TX *tex, M2TXHeader **header)
{
	if (tex != NULL)
	{
		*header = &(tex->Header);
		return (M2E_NoErr);
	}
	else
		return(M2E_BadPtr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_SetHeader
|||	Copy the information in header to the Header chunk in the M2TX texture
|||	structure tex.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_SetHeader(M2TX *tex, M2TXHeader *header)
|||	
|||	  Description
|||	
|||	    This function copies the data in header to the Header chunk of an M2TX 
|||	    data structure pointed to by tex. It returns an error code if it fails.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    header
|||	        The input M2TXHeader chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_GetHeader()     
**/

M2Err M2TX_SetHeader(M2TX *tex, M2TXHeader *header)
{
	int i;
	if (tex != NULL)
	{
		tex->Header.Version = header->Version;
		tex->Header.Flags = header->Flags;
		tex->Header.MinXSize = header->MinXSize;
		tex->Header.MinYSize = header->MinYSize;
		tex->Header.TexFormat = header->TexFormat;
		tex->Header.NumLOD = header->NumLOD;
		tex->Header.Border = header->Border;
		for (i=0; i<12; i++)
		{
			tex->Header.LODDataPtr[i] = header->LODDataPtr[i];
			tex->Header.LODDataLength[i] = header->LODDataLength[i];
		}
		for (i=0; i<2; i++)
			tex->Header.ColorConst[i] = header->ColorConst[i];		

		/* These are worthless but we copy them anyway */
		tex->Header.Size = header->Size;
		tex->Header.DCIOffset = header->DCIOffset;
		tex->Header.PIPOffset = header->PIPOffset;
		for (i=0; i<12; i++)
			tex->Header.LODDataOffset[i] = header->LODDataOffset[i];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}  

M2Err M2TX_Copy(M2TX *to, M2TX *from)
{
  M2TX_SetHeader(to, &(from->Header));
  M2TX_SetPIP(to, &(from->PIP));
  M2TX_SetDCI(to, &(from->DCI));
  to->TexAttr.Tx_Attr = from->TexAttr.Tx_Attr;
  to->DBl.DBl_Attr = from->DBl.DBl_Attr;
  to->LoadRects.LRData = from->LoadRects.LRData;
  to->LoadRects.NumLoadRects = from->LoadRects.NumLoadRects;

  to->Page.PgData = from->Page.PgData;
  to->Page.NumTex = from->Page.NumTex;
  to->Page.Version = from->Page.Version;

  to->PCLT.NumTex = from->PCLT.NumTex;
  to->PCLT.PageLoadCLT.size = from->PCLT.PageLoadCLT.size;
  to->PCLT.PageLoadCLT.allocated = from->PCLT.PageLoadCLT.allocated;
  to->PCLT.PageLoadCLT.data = from->PCLT.PageLoadCLT.data;
  to->PCLT.PIPLoadCLT.size = from->PCLT.PIPLoadCLT.size;
  to->PCLT.PIPLoadCLT.allocated = from->PCLT.PIPLoadCLT.allocated;
  to->PCLT.PIPLoadCLT.data = from->PCLT.PIPLoadCLT.data;
  to->PCLT.TexRefCLT = from->PCLT.TexRefCLT;
  to->PCLT.UVScale = from->PCLT.UVScale;
  to->PCLT.NumCompressed = from->PCLT.NumCompressed;
  to->PCLT.PatchOffset = from->PCLT.PatchOffset;
  to->PCLT.Version = from->PCLT.Version;

  return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_GetPIP
|||	Return a pointer to the PIP chunk of an M2TX texture already loaded into 
|||	memory.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_GetPIP(M2TX *tex, M2TXPIP **pip)
|||	
|||	  Description
|||	 
|||	    This function makes pip point to the PIP chunk of a M2TX data structure
|||	    pointed to by tex.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The pointer to the PIP of tex.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetPIP()     
**/

M2Err M2TX_GetPIP(M2TX *tex, M2TXPIP **pip)
{
	if (tex != NULL)
	{
		*pip = &(tex->PIP);
		return (M2E_NoErr);
	}
	else
		return(M2E_BadPtr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_SetPIP
|||	Copy the information in pip to the PIP chunk in the M2TX texture
|||	structure tex.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_SetPIP(M2TX *tex, M2TXPIP *pip)
|||	
|||	  Description
|||	
|||	    This function copies the data in pip to the PIP chunk of an M2TX data
|||	    structure pointed to by tex. It returns an error code if it fails.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    pip
|||	        The input PIP.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetPIP()     
**/

M2Err M2TX_SetPIP(M2TX *tex, M2TXPIP *pip)
{
  int i;
  
  if (tex != NULL)
    {
      for (i=0; i<256; i++)
	{
	  tex->PIP.PIPData[i] = pip->PIPData[i];
	}
      /* These are worthless but we copy them anyway */
      tex->PIP.Size = pip->Size;
      tex->PIP.NumColors = pip->NumColors;
    }	
  else
    return (M2E_BadPtr);
  return (M2E_NoErr);
}    

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_GetDCI
|||	Return a pointer to the DCI chunk of an M2TX texture already loaded into
|||	memory.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_GetDCI(M2TX *tex, M2TXDCI **dci)
|||	
|||	  Description
|||	
|||	    This function makes dci point to the DCI chunk of an M2TX data structure
|||	    pointed to by tex.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    dci
|||	        Pointer to the DCI in tex.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetDCI()     
**/

M2Err M2TX_GetDCI(M2TX *tex, M2TXDCI **dci)
{
	if (tex != NULL)
	{
		*dci = &(tex->DCI);
		return (M2E_NoErr);
	}
	else
		return(M2E_BadPtr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_SetDCI
|||	Copy the information in dci to the DCI chunk in the M2TX texture structure tex.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_SetDCI(M2TX *tex, M2TXDCI *dci)
|||	
|||	  Description
|||	
|||	    This function copies the data in dci to the DCI chunk of an M2TX data
|||	    structure pointed to by tex. 
|||	    It returns an error code if it fails.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    dci
|||	        The input DCI.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_GetDCI()     
**/

M2Err M2TX_SetDCI(M2TX *tex, M2TXDCI *dci)
{
	int i;
	
	if (tex != NULL)
	{
		for (i=0; i<4; i++)
		{
			tex->DCI.TexelFormat[i] = dci->TexelFormat[i];
			tex->DCI.TxExpColorConst[i] = dci->TxExpColorConst[i];			
		}
		/* These are worthless but we copy them anyway */
		tex->DCI.Size = dci->Size;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/* Accessors to PIP */

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_GetSortIndex
|||	Return the translation value of a given entry of the current PIP.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_GetSortIndex(M2TXPIP *pip, uint8 index, uint8 *value)
|||	
|||	  Description
|||	    This function returns an index in the translation table.  The
|||	    translation table is meant to map one PIP to another in case the colors
|||	    get sorted or rearranged.  The index is the original color's index in the 
|||	    PIP.  The new translated value is returned in value.
|||	
|||	  Caveats 
|||	    The tranlation table is something maintained by the library for each 
|||	    PIP, it is not part of the UTF texture format.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP.
|||	      index
|||	          The index in the translation table.
|||	      value
|||	          The translation value.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetSortIndex()
**/

M2Err M2TXPIP_GetSortIndex(M2TXPIP *pip, uint8 index, uint8 *value)
{
	if (pip != NULL)
	{
		*value = pip->SortIndex[index];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_SetSortIndex
|||	Set the PIP translation table entry specified by index.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_SetSortIndex(M2TXPIP *pip, uint8 index, uint8 value)
|||	
|||	  Description
|||	
|||	    This function will set the PIP translation table value for the specified 
|||	    index.  The translation table is meant to map one PIP to another in case
|||	    the colors get sorted or rearranged.  The index is original color's
|||	    index in the PIP.  The new PIP location is set by the variable value.
|||	
|||	  Caveats 
|||	    The tranlation table is something maintained by the library for each 
|||	    PIP, it is not part of the UTF texture format.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP.
|||	      index
|||	          The index in the translation table.
|||	      value
|||	          The translation value.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_GetSortIndex()
**/

M2Err M2TXPIP_SetSortIndex(M2TXPIP *pip, uint8 index, uint8 value)
{
	if (pip != NULL)
	{
		pip->SortIndex[index] = value;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_SetNumColors
|||	Return the number of colors in the current PIP.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_SetNumColors(M2TXPIP *pip, uint16 value)
|||	
|||	  Description
|||	    This function sets the number of colors in the current PIP.  Normally,
|||	    this is equal to 2^(color depth) of the expanded format, however, it
|||	    may sometimes be less than this number.  This is useful for having
|||	    mini-palettes that are to be loaded into a larger palette at a 
|||	    specified Index Offset.
|||	
|||	  Caveats 
|||	    If you are allocating and setting your own PIP, you should set
|||	    the NumColors field to -1 if you wish to use the default number
|||	    of colors (2^(Color Depth)).  This value is initialized to -1
|||	    when M2TX_Init() is called, but if you overwrite that PIP with a 
|||	    M2TX_SetPIP(),  NumColors will be set to whatever is in YOUR
|||	    PIP, which may be garbage if it is not set.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP.
|||	      value
|||	          The number of colors in the current PIP.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXPIP_GetNumColors(), M2TXPIP_GetIndexOffset(), M2TXPIP_SetIndexOffset
**/

M2Err M2TXPIP_SetNumColors(M2TXPIP *pip, int16 value)
{
  if (pip != NULL)
    {
      pip->NumColors = value;
    }	
  else
    return (M2E_BadPtr);
  return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_GetNumColors
|||	Return the number of colors in the current PIP
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_GetNumColors(M2TXPIP *pip, int16 *value)
|||	
|||	  Description
|||	    This function gets the number of colors in the current PIP.  Normally,
|||	    this is equal to 2^(color depth) of the expanded format, however, it
|||	    may sometimes be less than this number.  This is useful for having
|||	    mini-palettes that are to be loaded into a larger palette at a 
|||	    specified Index Offset.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP.
|||	      value
|||	          The number of colors in the current PIP.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXPIP_SetNumColors(), M2TXPIP_GetIndexOffset(), M2TXPIP_SetIndexOffset
**/
M2Err M2TXPIP_GetNumColors(M2TXPIP *pip, int16 *value)
{
  if (pip != NULL)
    {
      *value = pip->NumColors;
    }	
  else
    return (M2E_BadPtr);
  return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_GetIndexOffset
|||	Return the Index Offset of the current PIP
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_GetIndexOffset(M2TXPIP *pip, int16 *value)
|||	
|||	  Description
|||	    This function gets the offset at which the PIP is to be loaded into
|||	    the hardware color table during PIP mapping.  Normally this value is
|||	    zero, but sometimes it may be desirable to insert a smaller palette
|||	    into a larger one that has already been loaded.  
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP.
|||	      value
|||	          The offset at which to load the PIP into the color table.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXPIP_SetNumColors(), M2TXPIP_GetNumColors(), M2TXPIP_SetIndexOffset()
**/

M2Err M2TXPIP_GetIndexOffset(M2TXPIP *pip, uint32 *value)
{
  if (pip != NULL)
    {
      *value = pip->IndexOffset;
    }	
  else
    return (M2E_BadPtr);
  return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_SetIndexOffset
|||	Set the Index Offset of the current PIP
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_SetIndexOffset(M2TXPIP *pip, uint32 value)
|||	
|||	  Description
|||	
|||	    This function sets the offset at which the PIP is to be loaded into
|||	    the hardware color table during PIP mapping.  Normally this value is
|||	    zero, but sometimes it may be desirable to insert a smaller palette
|||	    into a larger one that has already been loaded.  
|||	
|||	  Caveats 
|||	    If you are allocating and setting your own PIP you should set
|||	    the IndexOffset to 0 if you wish to use the default.
|||	    IndexOffset is initialized to 0 when M2TX_Init() is called, but
|||	    if you overwrite that PIP with a M2TX_SetPIP(),  IndexOffset will
|||	    be set to whatever is in YOUR PIP, which may be garbage if it is
|||	    not set.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input M2TX texture.
|||	      value
|||	          The offset at which to load the PIP into the color table.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXPIP_SetNumColors(), M2TXPIP_GetIndexOffset(), M2TXPIP_SetNumColors()
**/

M2Err M2TXPIP_SetIndexOffset(M2TXPIP *pip, uint32 value)
{
	if (pip != NULL)
	{
		pip->IndexOffset = value;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_GetSize
|||	Get the size size of the pip data chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_GetSize(M2TXPIP *pip, uint32 *size)
|||	
|||	  Description
|||	
|||	    This function copies into size the memory footprint of the PIP chunk 
|||	    pointed to by pip.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP.
|||	      size
|||	          The size of the data of the PIP chunk
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_GetSortIndex()
**/

M2Err M2TXPIP_GetSize(M2TXPIP *pip, uint32 *size)
{
	if (pip != NULL)
	{
		*size = pip->Size;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

M2Err M2TXPIP_SetData(M2TXPIP *pip, uint8 num, M2TXColor *data)
{
	int i;
	if (pip != NULL)
	{
		if (num > 256)
			return(M2E_Range);
		for (i=0; i<num; i++)
	 		pip->PIPData[i] = data[i];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);	
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_SetColor
|||	Set the color of a PIP entry specified by index.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_SetColor(M2TXPIP *pip, uint8 index, M2TXColor color)
|||	
|||	  Description
|||	
|||	    This function will copy the color into the PIP entry specified by index.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP structure.
|||	      index
|||	          The PIP entry to be modified.
|||	      color
|||	          The input color.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXPIP_GetColor()
**/

M2Err M2TXPIP_SetColor(M2TXPIP *pip, uint8 index, M2TXColor color)
{
	if (pip != NULL)
	{
		if (index > 255)
			return(M2E_Range);
		pip->PIPData[index] = color;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);	
}

M2Err M2TXPIP_GetData(M2TXPIP *pip, uint8 num, M2TXColor *data)
{
	int i;
	if (pip != NULL)
	{
		if (num > 256)
			return(M2E_Range);
		for (i=0; i<num; i++)
	 		data[i] = pip->PIPData[i];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);	
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_GetColor
|||	Return the color in the PIP at the specified index.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXPIP_GetColor(M2TXPIP *pip, uint8 index, M2TXColor *color)
|||	
|||	  Description
|||	
|||	    This function will copy into color, the color in the Pen Indexed Palette
|||	    (PIP) at the index specified by num.  The color contains SSB, alpha,
|||	     red, green, and blue information.
|||	
|||	  Arguments
|||	    
|||	      pip
|||	          The input PIP structure.
|||	      index
|||	          The PIP entry to be queried.
|||	      color
|||	          The PIP color.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXPIP_SetColor()
**/


M2Err M2TXPIP_GetColor(M2TXPIP *pip, uint8 index, M2TXColor *color)
{
	if (pip != NULL)
	{
		if (index > 255)
			return(M2E_Range);
		*color = pip->PIPData[index];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);	
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXPIP_MakeTranslation
|||	Make a translation table mapping from the old PIP to the new one.
|||	
|||	  Synopsis
|||	    M2Err M2TXPIP_MakeTranslation(M2TXPIP *oldPIP, M2TXPIP *newPIP, uint8 cDepth)
|||	
|||	  Description
|||	
|||	    This function creates a translation table in newPIP that maps the colors
|||	    in oldPIP to those in newPIP.  The translation table is meant to map one
|||	    PIP to another in case the colors get sorted or rearranged.
|||	    If the NumColors field is set in the PIPs, it will be used to determine
|||	    how many colors are in each PIP. If the NumColors field does not contain
|||	    a valid value value (1-256). 2^cDepth colors will be assume for the PIP(s).
|||	
|||	  Caveat 
|||	    The translation table is something maintained by the library for each 
|||	    PIP, it is not part of the UTF texture format.
|||	
|||	  Arguments
|||	    
|||	      oldPIP
|||	          The old PIP to use as a reference to make a translation from.
|||	      newPIP
|||	          The new PIP to use as a reference to make a translation to.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
**/

M2Err M2TXPIP_MakeTranslation(M2TXPIP *oldPIP, M2TXPIP *newPIP, uint8 cDepth) 
{
	uint16 i, j, numColorsOld, numColorsNew;
	int16 nColors;
	M2TXColor curColor, tmpColor;
	M2Err err;
	
	err= M2E_NoErr;
	M2TXPIP_GetNumColors(oldPIP,&nColors);
	if ((nColors>0)&&(nColors<=256))
	  numColorsOld = nColors;
	else
	  numColorsOld = 1 << cDepth;
	M2TXPIP_GetNumColors(newPIP,&nColors);
	if ((nColors>0)&&(nColors<=256))
	  numColorsNew = nColors;
	else
	  numColorsNew = 1 << cDepth;
	for (i=0; i< numColorsOld; i++)
	{
		M2TXPIP_GetColor(oldPIP,i,&curColor);
		for (j=0; j<numColorsNew; j++)
		{
			M2TXPIP_GetColor(newPIP,j,&tmpColor);
			if (tmpColor == curColor)
				break;
		}
		M2TXPIP_SetSortIndex(newPIP, i, j);
		if (j>=numColorsNew)
			err=M2E_Range;    /* Color was not found in new PIP, error */
	}
	return (err);
}

/* Color functions */

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_Create
|||	Return a color with ssb, alpha, red, green, and blue set.
|||	
|||	  Synopsis
|||	
|||	    M2TXColor M2TXColor_Create(uint8 ssb, uint8 alpha, uint8 red, uint8 green, uint8 blue)
|||	
|||	  Description
|||	
|||	    This function returns an M2TXColor that has been initialized to the
|||	    values in ssb, alpha, red, green, and blue.
|||	
|||	  Arguments
|||	    
|||	      red
|||	          The red component of the final color.
|||	      green
|||	          The green component of the final color.
|||	      blue
|||	          The blue component of the final color.
|||	      alpha
|||	          The alpha component of the final color.
|||	      ssb
|||	          The ssb component of the final color.
|||	
|||	  Return Value
|||	
|||	    An M2TXColor properly initialized.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_Decode() 
**/

M2TXColor M2TXColor_Create(uint8 ssb, uint8 alpha, uint8 red, uint8 green,
					 	 	uint8 blue)
{
	M2TXColor color;
	
	color = ssb <<31;
	color += (alpha & 0xFE) << 23;     		
	color += red << 16;
	color += green << 8;
	color += blue;
	return (color);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_Decode
|||	Take an M2TXColor and extracts the color components ssb, alpha, red, green, and blue.
|||	
|||	  Synopsis
|||	
|||	    void M2TXColor_Decode(M2TXColor color, uint8 *ssb, uint8 *alpha, uint8 *red, uint8 *green, uint8 *blue)
|||	
|||	  Description
|||	
|||	    This function takes in an M2TXColor and returns the color components 
|||	    ssb, alpha, red, green, and blue in the respective variables.
|||	
|||	  Arguments
|||	    
|||	      color
|||	          The input color.
|||	      red
|||	          The red component of the input color.
|||	      green
|||	          The green component of the input color.
|||	      blue
|||	          The blue component of the input color.
|||	      alpha
|||	          The alpha component of the input color.
|||	      ssb
|||	          The ssb component of the input color.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_Create() 
**/

void M2TXColor_Decode(M2TXColor color, uint8 *ssb, uint8 *alpha, uint8 *red,
					  uint8 *green, uint8 *blue)
{
	*blue 	= color & 0xFF;
	*green 	= (color >> 8) &0xFF;
	*red 	= (color >> 16) &0xFF;
	*alpha	= ((color >> 24) &0x7F) << 1;	
	if (color & 0x01000000)
	  *alpha = (*alpha)+1;
	*ssb 	= (color >> 31);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_GetSSB
|||	Return the ssb component of the input color.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXColor_GetSSB(M2TXColor color)
|||	
|||	  Description
|||	
|||	    The function takes the M2TXColor color, extracts the ssb color 
|||	    component, and returns it.
|||	
|||	  Arguments
|||	    
|||	      color
|||	          The input M2TXColor.
|||	
|||	  Return Value
|||	
|||	    The SSB value.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_GetAlpha(), M2TXColor_GetRed(), M2TXColor_GetBlue(), 
|||	    M2TXColor_GetGreen() 
**/

uint8 M2TXColor_GetSSB(M2TXColor color)
{
	return(color >> 31);

}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_GetAlpha
|||	Return the alpha component of the input color.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXColor_GetAlpha(M2TXColor color)
|||	
|||	  Description
|||	
|||	    The function takes the M2TXColor color, extracts the alpha color 
|||	    component, and returns it.
|||	
|||	  Arguments
|||	    
|||	      color
|||	          The input M2TXColor.
|||	
|||	  Return Value
|||	
|||	    The alpha value.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_GetSSB(), M2TXColor_GetRed(), M2TXColor_GetBlue(), 
|||	    M2TXColor_GetGreen() 
**/

uint8 M2TXColor_GetAlpha(M2TXColor color)
{
	return((color >> 24) &0x7F) << 1;
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_GetRed
|||	Return the red component of the input color.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXColor_GetRed(M2TXColor color)
|||	
|||	  Description
|||	
|||	    The function takes the M2TXColor color, extracts the red color component, and returns it.
|||	
|||	  Arguments
|||	    
|||	      color
|||	          The input M2TXColor.
|||	
|||	  Return Value
|||	
|||	    The red value.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_GetSSB(), M2TXColor_GetAlpha(), M2TXColor_GetBlue(), 
|||	    M2TXColor_GetGreen() 
**/

uint8 M2TXColor_GetRed(M2TXColor color)
{
	return((color >> 16) &0xFF);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_GetGreen
|||	Return the green component of the input color.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXColor_GetGreen(M2TXColor color)
|||	
|||	  Description
|||	
|||	    The function takes the M2TXColor color, extracts the green color component, and returns it.
|||	
|||	  Arguments
|||	    
|||	      color
|||	          The input M2TXColor.
|||	
|||	  Return Value
|||	
|||	    The green value.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_GetSSB(), M2TXColor_GetRed(), M2TXColor_GetBlue(), 
|||	    M2TXColor_GetAlpha() 
**/

uint8 M2TXColor_GetGreen(M2TXColor color)
{
	return((color >> 8) &0xFF);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXColor_GetBlue
|||	Return the blue component of the input color.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXColor_GetBlue(M2TXColor color)
|||	
|||	  Description
|||	
|||	    The function takes the M2TXColor color, extracts the blue color component, and returns it.
|||	
|||	  Arguments
|||	    
|||	      color
|||	          The input M2TXColor.
|||	
|||	  Return Value
|||	
|||	    The blue value.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_GetSSB(), M2TXColor_GetAlpha(), M2TXColor_GetBlue(), 
|||	    M2TXColor_GetGreen() 
**/

uint8 M2TXColor_GetBlue(M2TXColor color)
{
	return(color&0xFF);
}

/* Convenience Calls  */

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_Init
|||	Initialize all the fields in the M2TX data structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_Init(M2TX *tex)
|||	
|||	  Description
|||	    This function takes in an M2TX data structure and clears all the fields to their default 
|||	    values.  This includes setting all data pointers to NULL.
|||	
|||	  Arguments
|||	    
|||	      tex
|||	          The input M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

M2Err M2TX_Init(M2TX *tex)
{
  M2Err err;
  M2TXHeader *header;
  M2TXPIP *pip;
  M2TXDCI *dci;
  M2TXRect *rect;
  int i, j;
  uint32 WrapperChunkType = 'TXTR';
  uint32 HeaderChunkType = 'M2TX';
  uint32 PIPChunkType = 'M2PI';
  uint32 DCIChunkType = 'M2CI';
  uint32 TexelChunkType = 'M2TD';
  uint32 TexAttrChunkType = 'M2TA';
  uint32 DBlendChunkType = 'M2DB';
  uint32 LoadRectsChunkType = 'M2LR';
  uint32 PageChunkType = 'M2PG';
  uint32 PCLTChunkType = 'PCLT';

  if (tex != NULL)
    {
      tex->Wrapper.Signature = WrapperChunkType;
      tex->Header.Signature = HeaderChunkType;
      tex->PIP.Signature = PIPChunkType;
      tex->DCI.Signature = DCIChunkType;
      tex->TexAttr.Signature = TexAttrChunkType;
      tex->DBl.Signature = DBlendChunkType;
      tex->LoadRects.Signature = LoadRectsChunkType;
      tex->Texel.Signature = TexelChunkType;
      tex->Page.Signature = PageChunkType;
      
      err = M2TX_GetHeader(tex, &header);
      err += M2TXHeader_SetFlags(header, 0);
      err += M2TXHeader_SetVersion(header, 0);
      err += M2TXHeader_SetMinXSize(header, 0);
      err += M2TXHeader_SetMinYSize(header, 0);
      err += M2TXHeader_SetTexFormat(header, 0);
      err += M2TXHeader_SetNumLOD(header, 0);
      err += M2TXHeader_SetBorder(header, 0);
      for (j=0; j<2; j++)
	err += M2TXHeader_SetColorConst(header, j, 0x7F000000);
      
      for (i=0; i<12; i++)
	err += M2TXHeader_SetLODPtr(header, i, 0, NULL);
      
      err += M2TX_GetPIP(tex, &pip);
      for (i=0; i<256; i++)
	{
	  err += M2TXPIP_SetColor(pip, i, 0x7F000000);
	  M2TXPIP_SetSortIndex(pip, i, i);
	}
      M2TXPIP_SetNumColors(pip, -1); 
      
      M2TXPIP_SetIndexOffset(pip, 0);
      err += M2TX_GetDCI(tex, &dci);
      for (i=0; i<4; i++)
	err += M2TXDCI_SetTexFormat(dci, i, 0);
      for (i=0; i<4; i++)
	err += M2TXDCI_SetColorConst(dci, i, 0x7F000000);
      
      tex->TexAttr.Reserved = 0;
      tex->TexAttr.Size = 4;
      tex->TexAttr.Tx_Attr = NULL;
      
      tex->DBl.Size = 4;
      tex->DBl.Reserved = 0;
      tex->DBl.DBl_Attr = NULL;
      
      tex->LoadRects.Size = 8 + sizeof(M2TXRect);
      tex->LoadRects.Reserved = 0;
      tex->LoadRects.NumLoadRects = 1; 
	    /* Every texture gets a default load rect */
      rect = tex->LoadRects.LRData = (M2TXRect *)qMemNewPtr(sizeof(M2TXRect));
      rect->XOff = rect->YOff = 0;
      rect->YWrapMode = rect->XWrapMode = TX_WrapModeClamp;
      rect->FirstLOD = 0;  /* Default to using the first LOD */
      if (err != M2E_NoErr)
	return(M2E_BadPtr);

      tex->Page.Size = 8;
      tex->Page.NumTex = 0;
      tex->Page.PgData = NULL;
      tex->Page.Version = 1;

      tex->PCLT.NumTex = 0;
      tex->PCLT.Version = 1;
      tex->PCLT.TexCLTOff = NULL;
      tex->PCLT.TexRefCLT = NULL;
      tex->PCLT.NumCompressed = 0;
      tex->PCLT.UVScale = NULL;
      tex->PCLT.PatchOffset = NULL;
      tex->PCLT.PageLoadCLT.size = 0;
      tex->PCLT.PIPLoadCLT.size = 0;
    }	
  else
    return (M2E_BadPtr);
  return (M2E_NoErr);	  
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_Free
|||	Free all the specified fields in the M2TX data structure.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_Free(M2TX *tex, bool freeLODS, bool freeTAB, 
|||	                    bool freeDAB, bool freeLRs)
|||	
|||	  Description
|||	    This function takes in an M2TX data structure and frees the 
|||	    specified data fields and sets them to NULL. 
|||	
|||	  Arguments
|||	    
|||	      tex
|||	          The input M2TX texture.
|||	      freeLODs
|||	          Whether to free all the levels of detail pointers
|||	      freeTAB
|||	          Whether to free the texture attributes block
|||	      freeDAB
|||	          Whether to free the destination blend attributes block
|||	      freeLRs
|||	          Whether to free the load rectangles block
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

M2Err M2TX_Free(M2TX *tex, bool freeLODs, bool freeTAB, bool freeDAB,
		bool freeLRs)
{
  M2Err err;
  int i;

  if (tex != NULL)
    {
      if (freeLODs)
	err = M2TXHeader_FreeLODPtrs(&(tex->Header));
      if (freeTAB)
	{
	  if (tex->TexAttr.Tx_Attr != NULL)
	    qMemReleasePtr(tex->TexAttr.Tx_Attr);
	  tex->TexAttr.Size = 4;
	  tex->TexAttr.Tx_Attr = NULL;
	}
      if (freeDAB)
	{
	  if (tex->DBl.DBl_Attr != NULL)
	    qMemReleasePtr(tex->DBl.DBl_Attr);
	  tex->DBl.Size = 4;
	  tex->DBl.DBl_Attr = NULL;
	}
      if (freeLRs)
	{
	  if (tex->LoadRects.LRData != NULL)
	    qMemReleasePtr(tex->LoadRects.LRData);
	  tex->LoadRects.Size = 8;
	  tex->LoadRects.Reserved = 0;
	  tex->LoadRects.NumLoadRects = 0; 
	  tex->LoadRects.LRData = NULL;
	}
      if (freeDAB && freeTAB && freeLODs)
	{
	  if ((tex->Page.PgData != NULL) && (tex->Page.NumTex>0))
	    qMemReleasePtr(tex->Page.PgData);
	  tex->Page.NumTex=0;

	  if (tex->PCLT.NumTex>0)
	    {
	      if (tex->PCLT.UVScale != NULL)
		qMemReleasePtr(tex->PCLT.UVScale);
	      tex->PCLT.UVScale = NULL;
	      if ((tex->PCLT.PageLoadCLT.size>0) && (tex->PCLT.PageLoadCLT.data!=NULL))
		qMemReleasePtr(tex->PCLT.PageLoadCLT.data);
	      tex->PCLT.PageLoadCLT.data = NULL;
	      tex->PCLT.PageLoadCLT.size = 0;
	      if ((tex->PCLT.PIPLoadCLT.size>0) && (tex->PCLT.PIPLoadCLT.data!=NULL))
		qMemReleasePtr(tex->PCLT.PIPLoadCLT.data);	
	      tex->PCLT.PIPLoadCLT.data = NULL;
	      tex->PCLT.PIPLoadCLT.size = 0;
	      if (tex->PCLT.TexRefCLT != NULL)
		{
		  for (i=0; i<tex->PCLT.NumTex; i++)
		    {
		      if ((tex->PCLT.TexRefCLT[i].size>0) && (tex->PCLT.TexRefCLT[i].data!=NULL))
			qMemReleasePtr(tex->PCLT.TexRefCLT[i].data);	
		      tex->PCLT.TexRefCLT[i].data = NULL;
		      tex->PCLT.TexRefCLT[i].size = 0;
		    }
		  qMemReleasePtr(tex->PCLT.TexRefCLT);
		  tex->PCLT.TexRefCLT = NULL;
		}
	      if (tex->PCLT.TexCLTOff != NULL)
		qMemReleasePtr(tex->PCLT.TexCLTOff);
	      tex->PCLT.TexCLTOff = NULL;
	    }
	  tex->PCLT.NumTex=0;
	}
    }	
  else
    return (M2E_BadPtr);
  return (M2E_NoErr);	  
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_IsLegal
|||	Determine if an M2TX texture with the specified attributes is legal or not.
|||	
|||	  Synopsis
|||	
|||	    bool  M2TX_IsLegal(uint8 cDepth, uint8  aDepth, uint8 ssbDepth, bool isLiteral)
|||	
|||	  Description
|||	    This function takes in color depth, alpha depth, ssb depth, and a flag that indicates 
|||	    whether the texture is literal or not.  It then returns a boolean flag as to whether it is 
|||	    a valid UTF texture type.
|||	
|||	  Arguments
|||	    
|||	      cDepth
|||	          The proposed color depth (0-8).
|||	      aDepth
|||	          The proposed alpha depth(0,4,7).
|||	      ssbDepth
|||	          The proposed ssb depth(0,1).
|||	      isLiteral
|||	          Whether the proposed texture is literal or not.
|||	
|||	  Return Value
|||	
|||	    A bool indicating whether the format is legal or not.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXColor_GetSSB(), M2TXColor_GetAlpha(), M2TXColor_GetBlue(), 
|||	    M2TXColor_GetGreen() 
**/

bool  M2TX_IsLegal(uint8 cDepth, uint8  aDepth, uint8 ssbDepth, bool isLiteral)
{
	uint32 format;

	format = cDepth*100 + aDepth*10 + ssbDepth;
	if (isLiteral)
		switch(format)
		{
			case 871:
			case 800:
			case 501:
				return(TRUE);
				break;
			default:
				return(FALSE);	
		}
	else
		switch(format)
		{
			case 871:
			case 840:
			case 800:
			case 741:
			case 701:
			case 600:
			case 501:
			case 471:
			case 440:
			case 400:
			case 341:
			case 301:
			case 240:
			case 200:
			case 141:
			case 101:
			case 100:
			case 71:    /* 1.06 */
			case 40:    /* 1.06 */
			case 1:     /* 1.06 */
			case 0:   	/* 1.06 */
				return(TRUE);
				break;
			default:
				return(FALSE);		
		}
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_Extract
|||	Extract a rectangular subregion across all the levels of detail from the input texture.  Output a new standalone texture.
|||	
|||	  Synopsis
|||	
|||	    M2ErrM2TX_Extract(M2TX *tex, uint8 numLOD, uint8 coarseLOD, uint16 xUL, uint16 yUL,
|||	    uint16 xSize, uint16 ySize, M2TX **newTex);
|||	
|||	  Description
|||	    This function takes in an input M2TX texture, tex.  It extracts a rectangular region 
|||	    specified by the coordinates by the upper left coordinates xUL and yUL. The size is 
|||	    specified by xSize and ySize.  These coordinates are for the coarsest level of detail which
|||	    is specified by coarseLOD.  Regions are extracted from the coarsest level of detail and the
|||	    specified number of finer levels of detail.  The extracted regions now form a new M2TX 
|||	    texture pointed to by newTex.
|||	
|||	  Arguments
|||	    
|||	      tex
|||	          The input M2TX texture.
|||	      numLOD
|||	          The number of levels of detail to extract.
|||	      coarseLOD
|||	          The coarsest level of detail to extract from tex.
|||	      xUL
|||	          The x coordinate of the upper left corner of the rectangle.
|||	      yUL
|||	          The y coordinate of the upper left corner of the rectangle.
|||	      xSize
|||	          The x size of the rectangle (on the coarsest level of detail).
|||	      ySize
|||	          The y size of the rectangle (on the coarsest level of detail).
|||	      newTex
|||	          The extracted M2TX texture.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/


M2Err M2TX_Extract(M2TX *tex, uint8 numLOD, uint8 coarseLOD, uint16 xUL,
				   uint16 yUL, uint16 xSize, uint16 ySize, M2TX **newTex)
{
	M2TX 		*tmpTex;
	M2TXHeader 	*header, *oldHeader;
	M2TXPIP	   	*pip;
	M2TXDCI		*dci;
	M2TXTex		oldLODPtr, newLODPtr, newLODPtr2;
	M2Err		err;
	bool		 hasColor[4], isTrans[4], hasAlpha[4], hasSSB[4], isLiteral[4];
	bool 		isLiteralU, hasColorU, hasAlphaU, hasSSBU, isString;
	bool 		isCompressed, inRect;
	uint8 		ssb, red, green, blue, alpha,  loop, runLength;
	uint8 		shiftU, aShiftU, cDepthU, aDepthU, ssbDepthU;
	uint8  		curLODs, newCount, count, txFormat, control;
	uint8 		cDepth[4], aDepth[4],  ssbDepth[4], shift[4], aShift[4];
	uint32		curXSize, curYSize, length, xPos, yPos, index;
	uint32 		tempPix,tempX,tempY, rectXUL,rectYUL,rectXLR,rectYLR, pixels;
	uint32		i,j,k,l,newMemSize, newBits, runningTotal;	
	long		theLong;
	int 		lod;
	
	/* Check if he wants more LODs than he can have from that starting point */
	if ((xSize > 1024) || (ySize > 1024))
		return(M2E_Range);
	if ((numLOD-1) > coarseLOD)	
		return(M2E_Range);
		
	err = M2TX_GetHeader(tex, &header);
	err += M2TX_GetPIP(tex, &pip);
	err += M2TX_GetDCI(tex, &dci);
	if (err != M2E_NoErr)
		return(M2E_BadPtr);
		
	tmpTex = (M2TX *)qMemNewPtr(sizeof(M2TX));
	if (tmpTex == NULL)
		return(M2E_NoMem);
	M2TX_Init(tmpTex);
	M2TXHeader_GetNumLOD(header, &curLODs);
	/* He wants to start at a LOD that doesn't exist */
	if (coarseLOD >= curLODs)				 
		return(M2E_Range);
	M2TXHeader_GetLODDim(header, coarseLOD, &curXSize, &curYSize);
	if ((curXSize < xSize) || (curYSize < ySize))
		return(M2E_Range);
	runningTotal = 0;
	err = M2TX_SetHeader(tmpTex, header);
	err += M2TX_SetPIP(tmpTex, pip);
	err += M2TX_SetDCI(tmpTex, dci);
	oldHeader = header;
	err += M2TX_GetHeader(tmpTex, &header);
	if (err != M2E_NoErr)
		return(M2E_BadPtr);

	M2TXHeader_SetMinXSize(header, xSize);
	M2TXHeader_SetMinYSize(header, ySize);		
	M2TXHeader_SetNumLOD(header, numLOD);	
	M2TXHeader_GetFIsCompressed(header, &isCompressed);

	if (isCompressed)
	{
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
	
			shift[i] = 8 - cDepth[i];
			aShift[i] = 8 - aDepth[i];
		}
	}
	else
	{
		err = M2TXHeader_GetFIsLiteral(header, &isLiteralU);
		err = M2TXHeader_GetFHasColor(header, &hasColorU);
		err = M2TXHeader_GetFHasAlpha(header, &hasAlphaU);
		err = M2TXHeader_GetFHasSSB(header, &hasSSBU);
		err = M2TXHeader_GetCDepth(header, &cDepthU);
		err = M2TXHeader_GetADepth(header, &aDepthU);
		shiftU = 8 - cDepthU;
		aShiftU = 8 - aDepthU;
		if (hasSSBU) 
			ssbDepthU = 1;
		else
		 	ssbDepthU = 0;		

	}

	for (lod=numLOD-1,i=coarseLOD; lod>=0; i--,lod--)
	{
		rectXUL = xUL; rectYUL = yUL; rectXLR = xUL + xSize -1; rectYLR = yUL + ySize -1;
		err = M2TXHeader_GetLODPtr(oldHeader, i, &length, &oldLODPtr);
		if (err != M2E_NoErr)
			return(err);
		M2TXHeader_GetLODDim(oldHeader, i, &curXSize, &curYSize);
		pixels = curXSize*curYSize;
		newLODPtr = (M2TXTex)qMemNewPtr(length);
		if (newLODPtr == NULL)
		{
			for (i=numLOD-1; i>lod; i--)
				M2TXHeader_FreeLODPtr(header,i);
			return (M2E_NoMem);	
		}
		BeginMemPutBits(newLODPtr, length);
		BeginMemGetBits(oldLODPtr);
		
		if (isCompressed)
		{
			for (j=0; j<pixels;)
			{
				control = qMemGetBits(8);
				txFormat = (control & 0xC0) >>6;
				if (isTrans[txFormat])
				{
					count=(control&0x3F)+1;
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

				/* Compute number of pixels that will intersect clip rect */
				tempPix = j;
				newCount = 0;
				for (k=0; k<count; k++,tempPix++)
				{
					tempX = tempPix % curXSize;
					tempY = tempPix / curXSize;
					if ((tempX >= rectXUL) && (tempX <= rectXLR))
						if ((tempY >= rectYUL) && (tempY<= rectYLR))
							newCount++;				
				}
	
				if (newCount > 0)		
				{ /* If run intersects clip rect, output the control byte */
					if (isTrans[txFormat])
						control = (0x3F & (newCount-1)) | (control & (~(0x3F)));
					else
						control = (0x1F & (newCount-1)) | (control & (~(0x1F)));
					qMemPutBits(control,8);
					runningTotal += newCount;
				}
	
				for (k=0; k<loop; k++)
				{
					xPos = j % curXSize;
					yPos = j / curXSize;
					inRect = FALSE;
					if ((xPos >= rectXUL) && (xPos <= rectXLR))
						if ((yPos >= rectYUL) && (yPos<= rectYLR))
							inRect = TRUE;		/* Is this pixel in the clip rect? */
					if (!isString)
						if (newCount > 0)
							/* If this is a run, we need to output it anyway */
							inRect = TRUE;		

					if(!(isTrans[txFormat]))
					{
						if (hasSSB[txFormat])
						{
							ssb = qMemGetBits(ssbDepth[txFormat]);
							if (inRect)
								qMemPutBits(ssb,ssbDepth[txFormat]);
						}
						if (!(hasAlpha[txFormat]) || (aDepth[txFormat] == 0))
							;
						else		
						{
							alpha = qMemGetBits(aDepth[txFormat]) << aShift[txFormat];
							if (inRect)
								qMemPutBits(alpha>>aShift[txFormat],aDepth[txFormat]);
						}
						if (!(hasColor[txFormat]) || (cDepth[txFormat] == 0))
							;
						else
						{
							if (isLiteral[txFormat])
							{
								red = qMemGetBits(cDepth[txFormat]) << shift[txFormat];
								green = qMemGetBits(cDepth[txFormat]) << shift[txFormat];
								blue = qMemGetBits(cDepth[txFormat]) << shift[txFormat];
								if (inRect)
								{
									qMemPutBits(red>>shift[txFormat],cDepth[txFormat]);
									qMemPutBits(green>>shift[txFormat],cDepth[txFormat]);
									qMemPutBits(blue>>shift[txFormat],cDepth[txFormat]);							
								}
							}
							else
							{
								index = qMemGetBits(cDepth[txFormat]);
								if (inRect)
									qMemPutBits(index,cDepth[txFormat]);
							}
						}
						if (isString)
							runLength = 1;
						else
							runLength = count;
					}
					else
						runLength = count;
					for (l=0; l<runLength; l++)
						j++;
				}
			}
		}
		else
		{
			for (j=0; j<pixels; j++)
			{
				xPos = j % curXSize;
				yPos = j / curXSize;
				inRect = FALSE;
				if ((xPos >= rectXUL) && (xPos <= rectXLR))
					if ((yPos >= rectYUL) && (yPos<= rectYLR))
					/* Is this pixel in the clip rect? */
						inRect = TRUE;				
				if (hasSSBU)
				{
					ssb = qMemGetBits(ssbDepthU);
					if (inRect)
						qMemPutBits(ssb,ssbDepthU);
				}
				if (!hasAlphaU || (aDepthU == 0))
					;
				else		
				{
					alpha = qMemGetBits(aDepthU) << aShiftU;
					if (inRect)
						qMemPutBits(alpha>>aShiftU,aDepthU);
				}
				if (!hasColorU || (cDepthU == 0))
					;
				else
				{
					if (isLiteralU)
					{
						red = qMemGetBits(cDepthU) << shiftU;
						green = qMemGetBits(cDepthU) << shiftU;
						blue = qMemGetBits(cDepthU) << shiftU;
						if (inRect)
						{
							qMemPutBits(red>>shiftU,cDepthU);
							qMemPutBits(green>>shiftU,cDepthU);
							qMemPutBits(blue>>shiftU,cDepthU);							
						}
					}
					else
					{
						index = qMemGetBits(cDepthU);
						if (inRect)
							qMemPutBits(index,cDepthU);
					}
				}	
			}
		}
		EndMemGetBits();

		newMemSize = GetCurrMemPutPos();
		newBits = GetCurrMemPutBitPos();
		theLong = 0;
		if (newBits)
		  {
		    qMemPutBits(theLong, 8-newBits);
		    newMemSize++;
		  }
		err = newMemSize;
		/* padded zero to the end to align properly */
		while (newMemSize % 4)
		{
			qMemPutBits(theLong, 8);
			newMemSize++;
		}
		EndMemPutBytes();

	/* resize the image */
		if (length > newMemSize)
		{
			newLODPtr2 = (M2TXTex)qMemNewPtr(newMemSize);
			if (newLODPtr2 == NULL)
			{
				qMemReleasePtr(newLODPtr);
				for (i=numLOD-1; i>lod; i--)
					M2TXHeader_FreeLODPtr(header,i);
				return (M2E_NoMem);	
			}
			memcpy(newLODPtr2,newLODPtr,newMemSize);
			qMemReleasePtr(newLODPtr);
			newLODPtr = newLODPtr2;
			/* newLODPtr = (M2TXTex)qMemResizePtr(newLODPtr, newMemSize); */
		}
		err = M2TXHeader_SetLODPtr(header, lod, newMemSize, newLODPtr);
		if (err != M2E_NoErr)
			return(err);
		xUL = xUL << 1; yUL = yUL << 1; xSize = xSize << 1; ySize = ySize << 1;
	}
	for (i=numLOD; i<curLODs; i++)
		M2TXHeader_SetLODPtr(header, lod, 0, NULL);
	*newTex = tmpTex;
	return (M2E_NoErr);
}
 
static void M2TXHeader_Print(M2TXHeader *header)
{
	bool flag;
	uint32 size;
	uint16 xsize, ysize;
	uint8 depth,red,green,blue,alpha,ssb,i;
	M2TXColor color;
	M2Err err;
	
	err = M2TXHeader_GetSize(header, &size);
	printf("Header Size=%d\n",size);	
	M2TXHeader_GetFIsCompressed(header, &flag);
	if (flag)
	{
	 	printf("Is Compressed\n");
	}
	M2TXHeader_GetFHasPIP(header, &flag);
	if (flag)
	{
		printf("Has PIP\n");
	}
	M2TXHeader_GetMinXSize(header, &xsize);
	printf("MinXSize=%d ",xsize);
	M2TXHeader_GetMinYSize(header, &ysize);
	printf("MinYSize=%d\n",ysize);
	M2TXHeader_GetFIsLiteral(header, &flag);
	if (flag) printf("Is Literal\n");
	M2TXHeader_GetFHasColor(header, &flag);
	if (flag) 
	{	
		printf("Has Color ");
		M2TXHeader_GetCDepth(header, &depth);
		printf("CDepth=%d \n",depth);
	}
	M2TXHeader_GetFHasAlpha(header, &flag);
	if (flag)
	{
		printf("Has Alpha ");
		M2TXHeader_GetADepth(header, &depth);
		printf("ADepth=%d \n",depth);
	}
	M2TXHeader_GetFHasSSB(header, &flag);
	if (flag) printf("Has SSB\n");
	M2TXHeader_GetNumLOD(header, &depth);
	printf("NumLOD=%d\n",depth);
	size = 0;
	for (i=0; i<depth; i++)
	  size += header->LODDataLength[i];
	printf("Texel Data Size=%d (%6.3gK)\n",size, (float)size/1024.0);
	printf("MaxXSize=%d \t MaxYSize=%d\n",xsize<<(depth-1), ysize<<(depth-1));
	for (i=0; i<depth; i++)
		printf("LODOffset[%d]=%d ",i,header->LODDataOffset[i]);
	printf("\n");
	M2TXHeader_GetFHasColorConst(header, &flag);
	if (flag)
	{
		printf("Has Color Const\n");
		M2TXHeader_GetColorConst(header, 0, &color);
		M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
		printf("ColorConst[%d][0]=(%d,%d,%d,%d,%d) ",i,ssb,alpha,red,green,blue);
		M2TXHeader_GetColorConst(header, 1, &color);
		M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
		printf("ColorConst[%d][1]=(%d,%d,%d,%d,%d) \n",i,ssb,alpha,red,green,blue);
	}
}

static void M2TXDCI_Print(M2TXDCI *dci)
{
	bool flag;
	uint32 size;
	uint8 depth,red,green,blue,alpha,ssb,i;
	M2TXColor color;
	M2Err err;
	
	err = M2TXDCI_GetSize(dci, &size);
	printf("DCI Size=%d\n",size);
	for (i=0; i<4; i++)
	{	
		M2TXDCI_GetFIsTrans(dci, i, &flag);
		if (flag) printf("Is Trans\n");
		M2TXDCI_GetFIsLiteral(dci, i, &flag);
		if (flag) printf("Is Literal\n");
		M2TXDCI_GetFHasColor(dci, i, &flag);
		if (flag) 
		{	
			printf("Has Color ");
			M2TXDCI_GetCDepth(dci, i, &depth);
			printf("CDepth=%d \n",depth);
		}
		M2TXDCI_GetFHasAlpha(dci, i, &flag);
		if (flag)
		{
			printf("Has Alpha ");
			M2TXDCI_GetADepth(dci, i, &depth);
			printf("ADepth=%d \n",depth);
		}
		M2TXDCI_GetFHasSSB(dci, i, &flag);
		if (flag) printf("Has SSB\n");
		M2TXDCI_GetColorConst(dci, i, &color);
		M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
		printf("ColorConst[%d]=(%d,%d,%d,%d,%d)\n",i,ssb,alpha,red,green,blue);
	}
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_Print
|||	Print information about the M2TX texture to standard out.
|||	
|||	  Synopsis
|||	
|||	    void M2TX_Print(M2TX *tex)
|||	
|||	  Description
|||	    This function will print out Header, DCI, Texture Attributes, and PIP information for the specified M2TX texture
|||	    to standard out.
|||	
|||	  Arguments
|||	    
|||	      tex
|||	          The input M2TX texture.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

void M2TX_Print(M2TX *tex)
{
	bool  		flag;
	M2TXHeader 	*header;
	M2TXDCI    	*dci;
	
	printf("File Data Size=%d\n",tex->Wrapper.FileSize);
	M2TX_GetHeader(tex,&header);
	M2TXHeader_Print(header);
	M2TXHeader_GetFHasPIP(header,&flag);
	if (flag)
	  {
		printf("PIP Size = %d\n",tex->PIP.Size);
		printf("PIP Colors = %d\n", tex->PIP.NumColors);
	  }
	M2TXHeader_GetFIsCompressed(header,&flag);
	if (flag)
	{
		M2TX_GetDCI(tex,&dci);	
		M2TXDCI_Print(dci);
	}

	printf("XWrapMode=%d YWrapMode=%d (0=Clamp, 1=Tile)\n",
	       tex->LoadRects.LRData[0].XWrapMode, 
	       tex->LoadRects.LRData[0].YWrapMode); 
	if (tex->LoadRects.NumLoadRects > 1)
	  printf("Number of Load Rects: %d\n", tex->LoadRects.NumLoadRects);

	if (tex->TexAttr.Size > 4)
	  printf("TAB commands of Size=%d\n",tex->TexAttr.Size);
	if (tex->DBl.Size > 4)
	  printf("DAB commands of Size=%d\n",tex->DBl.Size);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_GetColor
|||	Get the color value of the specified pixel from the specified.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_GetColor(M2TXRaw *raw, uint32 index, M2TXColor *color)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXRaw image and returns the color value of the 
|||	    pixel specified by index.
|||	
|||	  Arguments
|||	    
|||	      raw
|||	          The input image.
|||	      index
|||	          The pixel index in raw.
|||	      color
|||	          The color of the pixel at position index in image raw.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

M2Err M2TXRaw_GetColor(M2TXRaw *raw, uint32 index, M2TXColor *color)
{
	uint8 red, green, blue, alpha, ssb;

	if (raw != NULL)
	{
		if (raw->HasColor)
		{
			red = raw->Red[index];
			green = raw->Green[index];
			blue = raw->Blue[index];
		}
		else
		{
			red = green = blue = 0;
		}
		if (raw->HasAlpha)
			alpha = raw->Alpha[index];
		else
			alpha = 0xFF;
		if (raw->HasSSB)
			ssb = raw->SSB[index];
		else
			ssb = 0;
		*color = M2TXColor_Create(ssb, alpha, red, green, blue);
		return (M2E_NoErr);
	}
	return (M2E_BadPtr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_FindPIP
|||	Given an M2TXRaw image, try to find a PIP that contains all the colors in the image, if possible.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_FindPIP(M2TXRaw *raw, M2TXPIP *newPIP, uint8 *cDepth,
|||	    bool matchColor, bool matchAlpha, bool matchSSB);
|||	
|||	  Description
|||	
|||	    This function takes in an M2TXRaw image, then tries to find a palette that contains all the 
|||	    colors in the image.  Depending on the values of the flags matchColor, matchAlpha, and 
|||	    matchSSB; the function will ignore the specified components when creating a PIP.  If
|||	    successful, cDepth will contain the palettes depth (i.e the maximum number of colors
|||	    needed).  Also, the NumColors field of newPIP will be set to the
|||	    actual number of colors (which may be < 2^cDepth) set in the PIP.
|||	    needed).  No color reduction is done by this function.
|||	
|||	  Arguments
|||	    
|||	    raw
|||	        The input image.
|||	    newPIP
|||	        The output PIP that contains all the colors in the image.
|||	    cDepth
|||	        The color depth needed to contain all the entries in the PIP.
|||	    matchColor
|||	        Whether the color data should used in creating the PIP.
|||	    matchAlpha
|||	        Whether the alpha data should used in creating the PIP.
|||	    matchSSB
|||	        Whether the ssb data should used in creating the PIP.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_MakePIP()
**/

M2Err M2TXRaw_FindPIP(M2TXRaw *raw, M2TXPIP *newPIP, uint8 *cDepth,
					  bool matchColor, bool matchAlpha, bool matchSSB)
{
	uint16  j, colors;
	int16   numColors;
	M2TXColor tmpColor, curColor;
	uint32 i, pixels, mask;

	mask = 0;
	if (matchColor)
		mask += 0x00FFFFFF;
	if (matchAlpha)
		mask += 0x7F000000;
	if (matchSSB)
		mask += 0x80000000;

	pixels = (raw->XSize)*(raw->YSize);
	M2TXPIP_GetNumColors(newPIP,&numColors); 
	for (i = 0; i < pixels; i++)
	{
		M2TXRaw_GetColor(raw, i, &curColor);		
		for (j=0; j<numColors; j++)
		{
			M2TXPIP_GetColor(newPIP,j,&tmpColor);
			if ((curColor & mask) == (tmpColor & mask))
				break;
		}
		if (j>= numColors)
		{
			if (j>255)
				return (M2E_Range);
			numColors++;
			if (!matchColor)
			  curColor = curColor & 0xFF000000;
			if (!matchAlpha)
			  curColor = curColor | 0x7F000000;
			if (!matchSSB)
			  curColor = curColor & 0x7FFFFFFF;
			M2TXPIP_SetColor(newPIP, j, curColor);
		}
	}
	colors = 1;						/* Find the Color Depth */
	for (j=1; j<9; j++)
	{
		colors = colors << 1;
		if (numColors <= colors)
		{
			*cDepth = j;
			M2TXPIP_SetNumColors(newPIP,numColors);
			return (M2E_NoErr);
		}
	}
	*cDepth = 9;
	M2TXPIP_SetNumColors(newPIP,-1); 
	return (M2E_Range);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_Alloc
|||	Allocate memory for an M2TXRaw image of the specified dimensions.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_Alloc(M2TXRaw *raw, uint32 xSize, uint32 ySize)
|||	
|||	  Description
|||	
|||	    This functions takes an xSize and ySize and allocates memory for all the channels of an 
|||	    M2TXRaw image which is pointed to by raw.
|||	
|||	  Arguments
|||	    
|||	      raw
|||	          The M2TXRaw raw image allocated by the function
|||	      xSize
|||	          The x size of the M2TRaw image to create.
|||	      ySize
|||	          The y size of the M2TRaw image to create.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_Init(), M2TXRaw_Free()     
**/

M2Err M2TXRaw_Alloc(M2TXRaw *raw, uint32 xSize, uint32 ySize)
{
	uint32 pixels,i;
	
	if (raw != NULL)
	{
		raw->XSize = xSize; raw->YSize = ySize;
		raw->HasAlpha = raw->HasColor = TRUE;
		raw->HasSSB = TRUE;
		pixels = xSize * ySize;	

		raw->SSB = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (raw->SSB == NULL) 
			return(M2E_NoMem);
		
		raw->Alpha = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (raw->Alpha == NULL) 
			return(M2E_NoMem);
 		raw->Red = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (raw->Red == NULL)
			 return(M2E_NoMem);
		raw->Green = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (raw->Green == NULL) 
			return(M2E_NoMem);
		raw->Blue = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (raw->Blue == NULL) 
			return(M2E_NoMem);
		for (i=0; i<pixels; i++)
		{
			raw->SSB[i] = raw->Red[i] = raw->Green[i] = raw->Blue[i] = 0;
			raw->Alpha[i] = 0xFF;
		}
	}	
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_Init
|||	Allocate memory for the channels specified of an M2TXRaw image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_Init(M2TXRaw *raw, uint32 xSize, uint32 ySize, 
|||	    bool hasColor, bool hasAlpha, bool hasSSB)
|||	
|||	  Description
|||	
|||	    This functions takes in an xSize and ySize and allocates memory for the specified channels 
|||	    of an M2TXRaw image.  It sets the flags HasColor, HasAlpha, and HasSSB in the structure
|||	    as well. 
|||	
|||	  Arguments
|||	    
|||	      raw
|||	          The M2TXRaw raw image allocated by the function
|||	      xSize
|||	          The x size of the M2TRaw image to create.
|||	      ySize
|||	          The y size of the M2TRaw image to create.
|||	      hasColor
|||	          Whether to allocate Red, Green, and Blue channels for raw.
|||	      hasAlpha
|||	          Whether to allocate an Alpha channel for raw.
|||	      hasSSB
|||	          Whether to allocate an SSB channel for raw.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_Alloc(), M2TXRaw_Free()     
**/

M2Err M2TXRaw_Init(M2TXRaw *raw, uint32 xSize, uint32 ySize, 
					bool hasColor, bool hasAlpha, bool hasSSB)
{
	uint32 pixels,i;
	
	if (raw != NULL)
	{
		raw->XSize = xSize; raw->YSize = ySize;
		raw->HasAlpha = hasAlpha;
		raw->HasColor = hasColor;
		raw->HasSSB = hasSSB;
		pixels = xSize * ySize;	
       
        if (hasSSB)
		{
			raw->SSB = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (raw->SSB == NULL) 
				return(M2E_NoMem);
			for (i=0; i<pixels; i++)
			{
				raw->SSB[i] = 0;
			}
		}
		else
			raw->SSB = NULL;
			
        if (hasAlpha)
		{			
			raw->Alpha = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (raw->Alpha == NULL) 
				return(M2E_NoMem);
			for (i=0; i<pixels; i++)
			{
				raw->Alpha[i] = 0xFF;
			}
		}
		else
			raw->Alpha = NULL;
			
		if  (hasColor)
		{
	 		raw->Red = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (raw->Red == NULL)
				 return(M2E_NoMem);
			raw->Green = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (raw->Green == NULL) 
				return(M2E_NoMem);
			raw->Blue = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (raw->Blue == NULL) 
				return(M2E_NoMem);
			for (i=0; i<pixels; i++)
			{
				raw->Red[i] = raw->Green[i] = raw->Blue[i] = 0;
			}
		}
		else
			raw->Red = raw->Green = raw->Blue = NULL;
			
	}	
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_Free
|||	Free up the memory allocated to the given M2TXRaw image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_Free(M2TXRaw *raw)
|||	
|||	  Description
|||	
|||	    This function frees up all the chunks allocated to the M2TRaw image pointed to by raw.
|||	
|||	  Arguments
|||	    
|||	      raw
|||	          The input image.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_Alloc(), M2TXRaw_Init()     
**/

M2Err M2TXRaw_Free(M2TXRaw *raw)
{	
	if (raw != NULL)
	{
		if ((raw->Red)!= NULL)
			qMemReleasePtr(raw->Red); 
		if ((raw->Green)!= NULL)
			qMemReleasePtr(raw->Green); 
		if ((raw->Blue)!= NULL)
			qMemReleasePtr(raw->Blue);
		if ((raw->Alpha)!= NULL)
			qMemReleasePtr(raw->Alpha);
		if ((raw->SSB)!= NULL)
			qMemReleasePtr(raw->SSB);
	}
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_ReorderToPIP
|||	Using the sort table in PIP, map the values in the M2TXIndex image to their new PIP values. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_ReorderToPIP(M2TXIndex *index, M2TXPIP *pip, uint8 cDepth)
|||	
|||	  Description
|||	
|||	    This function takes in an M2TXIndex image and a M2TXPIP.  It remaps the image according to 
|||	    the sort table contained in M2TXPIP.  It is assumed that the PIP has been reordered in such
|||	     a way to necessitate this.
|||	
|||	  Arguments
|||	    
|||	    index
|||	        The input image.
|||	    pip
|||	        The input PIP.
|||	    cDepth
|||	        The color depth of the input image.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_Alloc(), M2TXIndex_Init(), M2TXIndex_Free()     
**/

M2Err M2TXIndex_ReorderToPIP(M2TXIndex *data, M2TXPIP *pip, uint8 cDepth) 
{
	uint32 pixels,i;
	uint8  tmp;

	i = cDepth;          /* Just to get rid of a warning message */
	
	if (data != NULL)
	{
		if (data->HasColor)
		{
			pixels = (data->XSize)*(data->YSize);
			for (i = 0; i < pixels; i++)
			{
				tmp = data->Index[i];
				data->Index[i] = pip->SortIndex[tmp];
			}
		}
	}
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);

}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_Alloc
|||	Allocate memory for an M2TXIndex image of the specified dimensions.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_Alloc(M2TXIndex *index, uint32 xSize, uint32 ySize)
|||	
|||	  Description
|||	
|||	    This function takes an xSize and ySize and allocates memory for all the channels of an 
|||	    M2TXIndex image which is pointed to by index.
|||	
|||	  Arguments
|||	    
|||	      index
|||	          The allocated image.
|||	      xSize
|||	          The x size of the M2TIndex image to create.
|||	      ySize
|||	          The y size of the M2TIndex image to create.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_ReorderToPIP(), M2TXIndex_Init(), M2TXIndex_Free()     
**/

M2Err M2TXIndex_Alloc(M2TXIndex *index, uint32 xSize, uint32 ySize)
{
	uint32 pixels,i;
	
	if (index != NULL)
	{
		index->XSize = xSize; index->YSize = ySize;
		pixels = xSize * ySize;	
		index->HasAlpha = index->HasColor = TRUE;
		index->HasSSB = TRUE;

		index->SSB = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (index->SSB == NULL) 
			return(M2E_NoMem);
		
		index->Alpha = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (index->Alpha == NULL) 
			return(M2E_NoMem);
 		index->Index = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
		if (index->Index == NULL)
			 return(M2E_NoMem);
		for (i=0; i<pixels; i++)
		{
			index->SSB[i] = index->Index[i] = 0;
			index->Alpha[i] = 0xFF;
		}

	}	
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_Init
|||	Allocate memory for the channels specified of an M2TXIndex image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_Init(M2TXIndex *index, uint32 xSize, uint32 ySize, 
|||	    bool hasColor, bool hasAlpha, bool hasSSB)
|||	
|||	  Description
|||	
|||	    This function takes in an xSize and ySize and allocates memory for the specified channels 
|||	    of an M2TXIndex image.  It has sets the flags HasColor, HasAlpha, and HasSSB in the 
|||	    structure as well. 
|||	
|||	  Arguments
|||	    
|||	      index
|||	          The allocated image.
|||	      xSize
|||	          The x size of the M2TIndex image to create.
|||	      ySize
|||	          The y size of the M2TIndex image to create.
|||	      hasColor
|||	          Whether to allocate Red, Green, and Blue channels for index.
|||	      hasAlpha
|||	          Whether to allocate an Alpha channel for index.
|||	      hasSSB
|||	          Whether to allocate an SSB channel for index.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_ReorderToPIP(), M2TXIndex_Alloc(), M2TXIndex_Free()     
**/

M2Err M2TXIndex_Init(M2TXIndex *index, uint32 xSize, uint32 ySize,
					bool hasColor, bool hasAlpha, bool hasSSB)
{
	uint32 pixels,i;
	
	if (index != NULL)
	{
		index->XSize = xSize; index->YSize = ySize;
		pixels = xSize * ySize;	

		index->HasAlpha = hasAlpha;
		index->HasColor = hasColor;
		index->HasSSB = hasSSB;

		if (hasSSB)
		{
			index->SSB = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (index->SSB == NULL) 
				return(M2E_NoMem);
			for (i=0; i<pixels; i++)
			{
				index->SSB[i] = 0;
			}
		}
		else
			index->SSB = NULL;
			
		if (hasAlpha)
		{
			index->Alpha = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (index->Alpha == NULL) 
				return(M2E_NoMem);
			for (i=0; i<pixels; i++)
			{
				index->Alpha[i] = 0xFF;
			}
		}
		else
			index->Alpha = NULL;
			
		if (hasColor)
		{
	 		index->Index = (uint8 *)qMemNewPtr(pixels*sizeof(uint8));
			if (index->Index == NULL)
				 return(M2E_NoMem);
			for (i=0; i<pixels; i++)
			{
				index->Index[i] = 0;
			}
		}
		else
			index->Index = NULL;
			

	}	
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_Free
|||	Free up the memory allocated to the given M2TXIndex image.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_Free(M2TXIndex *index)
|||	
|||	  Description
|||	
|||	    This function frees up all the chunks allocated to the M2TIndex image pointed to by index.
|||	
|||	  Arguments
|||	    
|||	      index
|||	          The input image.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_ReorderToPIP(), M2TXIndex_Alloc(), M2TXIndex_Init()     
**/


M2Err M2TXIndex_Free(M2TXIndex *index)
{	
	if (index != NULL)
	{
		if ((index->Index)!= NULL)
			qMemReleasePtr(index->Index); 
		if ((index->Alpha)!= NULL)
			qMemReleasePtr(index->Alpha);
		if ((index->SSB)!= NULL)
			qMemReleasePtr(index->SSB);
	}
	else
		return (M2E_BadPtr);
	return(M2E_NoErr);
}



/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_LODCreate
|||	Resample the specified channels of rawIn using the specified filter and copies the data to the specified channels of rawOut.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_LODCreate(M2TXRaw *rawIn, uint32 sample, uint16 channels, M2TXRaw *rawOut)
|||	
|||	  Description
|||	    This function takes two M2TXRaw images, rawIn and rawOut.  It resamples the channels of 
|||	    rawIn specified by channels using the filter specified by sample.  The data is then copied 
|||	    to the specified channels of rawOut.
|||	
|||	  Caveats
|||	    rawOut must be initialized or allocated before calling M2TXRaw_LODCreate because the function gets the image
|||	    dimensions from this data structure and does no allocation of its own.
|||	  Arguments
|||	    
|||	      rawIn
|||	          The input image.
|||	      rawOut
|||	          The resize and resampled output image.
|||	      sample
|||	          The resample filter type.
|||	      channels
|||	          Which channels to resample.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXIndex_LODCreate()     
**/

M2Err M2TXRaw_LODCreate(M2TXRaw *rawIn, uint32 sample, uint16 channels, M2TXRaw *rawOut)
{
	
	if (channels&M2Channel_Red)
		ReSampleImage((BYTE *)(rawIn->Red),rawIn->XSize,rawIn->YSize, (BYTE *)(rawOut->Red),
						rawOut->XSize, rawOut->YSize, 1, sample);
	if (channels&M2Channel_Green)
		ReSampleImage((BYTE *)(rawIn->Green),rawIn->XSize,rawIn->YSize, (BYTE *)(rawOut->Green),
						rawOut->XSize, rawOut->YSize, 1, sample);
	if (channels&M2Channel_Blue)
		ReSampleImage((BYTE *)(rawIn->Blue),rawIn->XSize,rawIn->YSize, (BYTE *)(rawOut->Blue),
						rawOut->XSize, rawOut->YSize, 1, sample);
	if (channels&M2Channel_Alpha)
		ReSampleImage((BYTE *)(rawIn->Alpha),rawIn->XSize,rawIn->YSize, (BYTE *)(rawOut->Alpha),
						rawOut->XSize, rawOut->YSize, 1, sample);
	if (channels&M2Channel_SSB)
		ReSampleImage((BYTE *)(rawIn->SSB),rawIn->XSize,rawIn->YSize, (BYTE *)(rawOut->SSB),
						rawOut->XSize, rawOut->YSize, 1, sample);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXIndex_LODCreate
|||	Create a level-of-detail of the specified channels with the specified filter.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXIndex_LODCreate(M2TXIndex *indexIn, uint32 sample, uint16 channels, M2TXIndex *indexOut)
|||	
|||	  Description
|||	    This function take two M2TXIndex images, indexIn and indexOut.  It resamples the channels of
|||	    indexIn specified by channels using the filter specified by sample.  The data is then copied
|||	    to the specified channels of indexOut.
|||	
|||	  Caveats
|||	    indexOut must be initialized or allocated before calling M2TXIndex_LODCreate because the function gets the
|||	    image dimensions from this data structure and does no allocation of its own.
|||	
|||	
|||	  Arguments
|||	    
|||	      indexIn
|||	          The input image.
|||	      indexOut
|||	          The resize and resampled output image.
|||	      sample
|||	          The resample filter type.
|||	      channels
|||	          Which channels to resample.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_LODCreate()
**/

M2Err M2TXIndex_LODCreate(M2TXIndex *indexIn, uint32 sample, uint16 channels, M2TXIndex *indexOut)
{
	
	if (channels&M2Channel_Index)
		ReSampleImage((BYTE *)(indexIn->Index),indexIn->XSize,indexIn->YSize, (BYTE *)(indexOut->Index),
						indexOut->XSize, indexOut->YSize, 1, M2SAMPLE_POINT);
	if (channels&M2Channel_Alpha)
		ReSampleImage((BYTE *)(indexIn->Alpha),indexIn->XSize,indexIn->YSize, (BYTE *)(indexOut->Alpha),
						indexOut->XSize, indexOut->YSize, 1, sample);
	if (channels&M2Channel_SSB)
		ReSampleImage((BYTE *)(indexIn->SSB),indexIn->XSize,indexIn->YSize, (BYTE *)(indexOut->SSB),
						indexOut->XSize, indexOut->YSize, 1, sample);
	return(M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFilter_Reset
|||	Reset all the filter widths to their default values.
|||	
|||	  Synopsis
|||	
|||	    void M2TXFilter_Reset()
|||	
|||	  Description
|||	 
|||	    This function resets all the width of all the resampling filters.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, <M2TXTypes.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFilter_GetWidth(), M2TXFilter_SetWidth()     
**/

void M2TXFilter_Reset()
{
  ResetFilterWidth();
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFilter_SetWidth
|||	Set a specific filter's width to a given value.
|||	
|||	  Synopsis
|||	
|||	    double M2TXFilter_Reset(uint32 filter, double width)
|||	
|||	  Description
|||	 
|||	    This function sets the width of a given resampling filter.
|||	
|||	  Arguments
|||	    
|||	    filter
|||	        The resampling filter type.
|||	    width 
|||	        The new normalized filter width.
|||	
|||	  Return Value
|||	
|||	    It returns the old width of the filter.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, <M2TXTypes.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFilter_GetWidth(), M2TXFilter_Reset()     
**/
double M2TXFilter_SetWidth(uint32 filter, double width)
{
  return(SetFilterWidth(filter, width));
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFilter_GetWidth
|||	Return the current width of a given resampling filter.
|||	
|||	  Synopsis
|||	
|||	    double M2TXFilter_GetWidth(uint32 filter)
|||	
|||	  Description
|||	 
|||	    This function gets the width of a given resampling filter.
|||	
|||	  Arguments
|||	    
|||	    filter
|||	        The resampling filter type.
|||	
|||	  Return Value
|||	
|||	    The normalized width of the input resampling filter.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, <M2TXTypes.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFilter_Reset(), M2TXFilter_SetWidth()     
**/
double M2TXFilter_GetWidth(uint32 filter)
{
  return(GetFilterWidth(filter));
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_SetDefaultXWrap
|||	Set the X Wrap Mode for the entire texture.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_SetDefaultXWrap(M2TX *tex, uint8 wrapMode)
|||	
|||	  Description
|||	
|||	    This function sets the wrap mode in the X direction for the entire
|||	    texture.
|||	
|||	  Caveats 
|||	    Wrap mode TX_WrapModeTile can only be used if the MinXSize value
|||	    is a power of two.  If the this is not the case, the function will
|||	    return M2E_Range error and not set XWrapMode.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    wrapMode
|||	        The wrap mode, either TX_WrapModeClamp or TX_WrapModeTile
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetDefaultYWrap()     
**/

M2Err   M2TX_SetDefaultXWrap(M2TX *tex, uint8 wrapMode)
{
  M2Err err;
  M2TXHeader 	*header;
  uint8 numLOD;
  M2TXRect *rect;
  uint16  xSize;

  err = M2TX_GetHeader(tex, &header);
  if (err != M2E_NoErr)
    return(err);
  err = M2TXHeader_GetNumLOD(header, &numLOD);
  if (err != M2E_NoErr)
    return(err);
  rect = tex->LoadRects.LRData;
  if (rect == NULL)
    return (M2E_BadPtr);
  
  if (wrapMode == TX_WrapModeTile)
    {
      M2TXHeader_GetMinXSize(header, &xSize);
      while (xSize > 1)
	{
	  if (0x01 & xSize)
	    {
	      return (M2E_Range);
	    }
	  else
	    {
	      xSize = xSize >> 1;
	    }
	}
    }
  rect->XWrapMode = wrapMode;
  return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TX_SetDefaultYWrap
|||	Set the Y Wrap Mode for the entire texture.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TX_SetDefaultYWrap(M2TX *tex, uint8 wrapMode)
|||	
|||	  Description
|||	
|||	    This function sets the wrap mode in the Y direction for the entire
|||	    texture.
|||	
|||	  Caveats 
|||	    Wrap mode TX_WrapModeTile can only be used if the MinYSize value
|||	    is a power of two.  If the this is not the case, the function will
|||	    return M2E_Range error and not set YWrapMode.
|||	
|||	  Arguments
|||	    
|||	    tex
|||	        The input M2TX texture.
|||	    wrapMode
|||	        The wrap mode, either TX_WrapModeClamp or TX_WrapModeTile
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXLibrary.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetDefaultXWrap()     
**/
M2Err   M2TX_SetDefaultYWrap(M2TX *tex, uint8 wrapMode)
{
  M2Err err;
  M2TXHeader 	*header;
  uint8 numLOD;
  uint16 ySize;
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

  if (wrapMode == TX_WrapModeTile)
    {
      M2TXHeader_GetMinYSize(header, &ySize);
      while (ySize > 1)
	{
	  if (0x01 & ySize)
	    {
	      return (M2E_Range);
	    }
	  else
	    {
	      ySize = ySize >> 1;
	    }
	}
    }
  rect->YWrapMode = wrapMode;
  return (M2E_NoErr);
}
