/*
	File:		M2TXHeader.c

	Contains:	M2 Texture routines API 

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <6>	  8/7/95	TMA		Add functions to Set/Get new Header flags for determining output
									of TAB, DAB, DCI, LR chunks.
		 <5>	 7/15/95	TMA		Autodocs updated.
		 <3>	 7/11/95	TMA		#include file changes.
		 <2>	 5/16/95	TMA		Autodocs added.
		<1+>	 1/16/95	TMA		Add more error checking

	To Do:
*/

#include "M2TXTypes.h"
#include "M2Err.h"
#include "M2TXHeader.h"
#include "stdlib.h"
#include "stdio.h"
#include "qmem.h"

/* Accessors to the Header */

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFIsCompressed
|||	Set the IsCompressed flag of the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFIsCompressed(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the IsCompressed flag
|||	    to the value in flag. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the IsCompressed flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFIsCompressed()
|||	
**/

M2Err M2TXHeader_SetFIsCompressed(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{		
		if (flag)
			header->Flags = (header->Flags&(~M2HC_IsCompressed)) | (M2HC_IsCompressed);
		else
			header->Flags = header->Flags & (~M2HC_IsCompressed);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasPIP
|||	Set the HasPIP flag of the Header.
|||	 
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasPIP(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasPIP flag
|||	    to the value in flag. The HasPIP flag indicates whether the M2TX texture
|||	    contains a PIP chunk or not.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasPIP flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetHasPIP()
|||	
**/

M2Err M2TXHeader_SetFHasPIP(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->Flags = (header->Flags&(~M2HC_HasPIP)) | (M2HC_HasPIP);
		else
			header->Flags = header->Flags & (~M2HC_HasPIP);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasNoDCI
|||	Set the HasNoDCI flag of the Header.
|||	 
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasNoDCI(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasNoDCI flag
|||	    to the value in flag. The HasNoDCI flag indicates whether the M2TX texture
|||	    should write out the DCI chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoDCI flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetHasNoDCI()
|||	
**/

M2Err M2TXHeader_SetFHasNoDCI(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->Flags = (header->Flags&(~M2HC_HasNoDCI)) | (M2HC_HasNoDCI);
		else
			header->Flags = header->Flags & (~M2HC_HasNoDCI);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasNoTAB
|||	Set the HasNoTAB flag of the Header.
|||	 
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasNoTAB(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasNoTAB flag
|||	    to the value in flag. The HasNoTAB flag indicates whether the M2TX texture
|||	    should write out the TAB chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoTAB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetHasNoTAB()
|||	
**/

M2Err M2TXHeader_SetFHasNoTAB(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->Flags = (header->Flags&(~M2HC_HasNoTAB)) | (M2HC_HasNoTAB);
		else
			header->Flags = header->Flags & (~M2HC_HasNoTAB);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasNoDAB
|||	Set the HasNoDAB flag of the Header.
|||	 
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasNoDAB(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasNoDAB flag
|||	    to the value in flag. The HasNoDAB flag indicates whether the M2TX texture
|||	    should write out the DAB chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoDAB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetHasNoDAB()
|||	
**/

M2Err M2TXHeader_SetFHasNoDAB(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->Flags = (header->Flags&(~M2HC_HasNoDAB)) | (M2HC_HasNoDAB);
		else
			header->Flags = header->Flags & (~M2HC_HasNoDAB);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasNoLR
|||	Set the HasNoLR flag of the Header.
|||	 
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasNoLR(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasNoLR flag
|||	    to the value in flag. The HasNoLR flag indicates whether the M2TX texture
|||	    should write out the Load Rectangles chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoLR flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetHasNoLR()
|||	
**/

M2Err M2TXHeader_SetFHasNoLR(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->Flags = (header->Flags&(~M2HC_HasNoLR)) | (M2HC_HasNoLR);
		else
			header->Flags = header->Flags & (~M2HC_HasNoLR);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasColorConst
|||	Set the HasColorConst flag
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasColorConst(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasColorConst flag
|||	    to the value in flag. The HasColorConst flag indicates whether the M2TX
|||	    texture contains two color constants selected with the ssb bit in 
|||	    certain texture modes.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasColorConst flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFHasColorConst()
|||	
**/
M2Err M2TXHeader_SetFHasColorConst(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->Flags = (header->Flags & (~M2HC_HasColorConst)) | (M2HC_HasColorConst);	
		else
			header->Flags = header->Flags & (~M2HC_HasColorConst);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetVersion
|||	Sets the Version field in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetVersion(M2TXHeader *header, uint32 version)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the Version field to
|||	    to the value in version. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    version
|||	        The version number of the texture format
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetVersion()
|||	
**/
M2Err M2TXHeader_SetVersion(M2TXHeader *header, uint32 version)
{
	if (header != NULL)
	{
		header->Version = version;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetMinXSize
|||	Set the MinXSize filed in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetMinXSize(M2TXHeader *header, uint16 size)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the MinXSize field to
|||	    to the of size.  This sets the X dimensions of the coarsest level of
|||	    detail.  Each finer level of detail (LOD) is twice as wide and twice
|||	    as tall as the previous.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    size
|||	        The x dimensions of the coarsest level of detail.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetMinYSize(), M2TXHeader_GetMinXSize()
|||	
**/
M2Err M2TXHeader_SetMinXSize(M2TXHeader *header, uint16 size)
{
	if (header != NULL)
	{
		if (size > 1024)
			return(M2E_Range);
		header->MinXSize = size;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetMinYSize
|||	Set the MinYSize field in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetMinYSize(M2TXHeader *header, uint16 size)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the MinYSize field to
|||	    to the value of size. This sets the Y dimension of the coarsest level of
|||	    detail.  Each finer level of detail (LOD) is twice as wide and twice
|||	    as tall as the previous.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    size
|||	        The y dimensions of the coarsest level of detail.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetMinXSize(), M2TXHeader_GetMinYSize()
|||	
**/
M2Err M2TXHeader_SetMinYSize(M2TXHeader *header, uint16 size)
{
	if (header != NULL)
	{
		if (size > 1024)
			return(M2E_Range);
		header->MinYSize = size;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetCDepth
|||	Set the color depth of the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetCDepth(M2TXHeader *header, uint8 cDepth)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the color depth to the
|||	    value in cDepth. Valid values are from 0 to 8.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    cDepth
|||	        The color depth.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetCDepth()
|||	
**/
M2Err M2TXHeader_SetCDepth(M2TXHeader *header, uint8 cDepth)
{
	if (header != NULL)
	{
		if (cDepth > 8)
			return(M2E_Range);
		header->TexFormat = (header->TexFormat & (~M2HC_ColorDepth))
			 				| (M2HC_ColorDepth & cDepth);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetADepth
|||	Set the alpha depth of the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetADepth(M2TXHeader *header, uint8 aDepth)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the alpha depth to the
|||	    value in aDepth. Valid values are from 0,4, and 7.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    aDepth
|||	        The alpha depth.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetADepth()
|||	
**/
M2Err M2TXHeader_SetADepth(M2TXHeader *header, uint8 aDepth)
{
	if (header != NULL)
	{
		if (aDepth > 7)
			return(M2E_Range);
		header->TexFormat = (header->TexFormat & (~M2HC_AlphaDepth))
			 				| (M2HC_AlphaDepth & (aDepth<<M2Shift_AlphaDepth));
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFlags
|||	Set the Flags field of the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFlags(M2TXHeader *header, uint32 flags)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the header flags
|||	    IsCompressed, HasPIP, and HasColorConst to the value in flags in one
|||	    shot. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flags
|||	        The value of the header flags.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFlags()
|||	
**/
M2Err M2TXHeader_SetFlags(M2TXHeader *header, uint32 flags)
{
	if (header != NULL)
	{
		header->Flags = flags;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFIsLiteral
|||	Set IsLiteral flag of the Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFIsLiteral(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXHeader chunk and sets the IsLiteral flag to
|||	    the value in flag. A literal texture has no palette (and therefore no
|||	    PIP) and uses either 15 bits per pixel or 24 bits per pixel for color 
|||	    information.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the IsLiteral flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFIsLiteral()
|||	
**/
M2Err M2TXHeader_SetFIsLiteral(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->TexFormat = (header->TexFormat & (~M2HC_IsLiteral)) | (M2HC_IsLiteral);
		else
			header->TexFormat = header->TexFormat & (~M2HC_IsLiteral);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasColor
|||	Set HasColor flag of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasColor(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasColor flag to
|||	    the value in flag.   The flag indicates whether the texel contains 
|||	    imbedded color information. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasColor flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFHasColor()
|||	
**/
M2Err M2TXHeader_SetFHasColor(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->TexFormat = (header->TexFormat & (~M2HC_HasColor)) | M2HC_HasColor;
		else
			header->TexFormat = (header->TexFormat) & (~M2HC_HasColor);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasAlpha
|||	Set the HasAlpha flag of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasAlpha(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the HasAlpha flag to the
|||	    value in flag.  The flag indicates whether the texel contains imbedded
|||	    alpha information. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasAlpha flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFHasAlpha()
|||	
**/
M2Err M2TXHeader_SetFHasAlpha(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->TexFormat = ((header->TexFormat) & (~M2HC_HasAlpha)) | M2HC_HasAlpha;
		else
			header->TexFormat = (header->TexFormat) & (~M2HC_HasAlpha);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetFHasSSB
|||	Set HasSSB flag of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetFHasSSB(M2TXHeader *header, bool flag)
|||	
|||	  Description
|||	    This function takes an M2TXHeader chunk and sets the HasSSB flag to the 
|||	    value in flag.  The flag indicates whether the texel contains imbedded
|||	    source select bit (SSB) information. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasSSB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFHasSSB()
|||	
**/
M2Err M2TXHeader_SetFHasSSB(M2TXHeader *header, bool flag)
{
	if (header != NULL)
	{
		if (flag)
			header->TexFormat = (header->TexFormat & (~M2HC_HasSSB)) | (M2HC_HasSSB);
		else
			header->TexFormat = header->TexFormat & (~M2HC_HasSSB);
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetTexFormat
|||	Set the texture format of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetTexFormat(M2TXHeader *header, M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the header's texture
|||	    format to the value in format.  The flags IsLiteral, HasColor, HasAlpha,
|||	    HasSSB and values for CDepth and ADepth are included in the texture
|||	    format.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetTexFormat()
|||	
**/
M2Err M2TXHeader_SetTexFormat(M2TXHeader *header, M2TXFormat format)
{
	if (header != NULL)
	{
		header->TexFormat = format;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
 
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetNumLOD
|||	Set the NumLOD field in the Header chunk
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetNumLOD(M2TXHeader *header, uint8 num)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the NumLOD field to
|||	    the value of num.  This sets the number of levels of detail in the M2TX
|||	    texture.  Valid values are 1 to 11.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    num
|||	        The number of levels of detail
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetNumLOD()
|||	
**/
M2Err M2TXHeader_SetNumLOD(M2TXHeader *header, uint8 num)
{
	if (header != NULL)
	{	
		if (num > MAX_LOD_NUM)
			return(M2E_Range);
		header->NumLOD = num;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetBorder
|||	Set the border field in the Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetBorder(M2TXHeader *header, uint8 width)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the border field to
|||	    value of width.  This field is reserved for future use.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    width
|||	        The value of border in the header chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetBorder()
|||	
**/
M2Err M2TXHeader_SetBorder(M2TXHeader *header, uint8 width)
{
	if (header != NULL)
	{
		if (width > 2)
			return(M2E_Range);
		header->Border = width;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetColorConst
|||	Sets a color constant in a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetColorConst(M2TXHeader *header, uint8 num, 
|||	                                 M2TXColor colorConst)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets the color constant
|||	    indicated by num (value 0 or 1) to the value in colorConst. Color 
|||	    constants are used to get color values in certain texture modes that use
|||	    the source select bit to select between two colors.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    colorConst
|||	        A color containing ssb, alpha, red, green, and blue values.
|||	    num
|||	        The index into the ColorConst array.  Valid values are 0 and 1.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetColorConst()
|||	
**/
M2Err M2TXHeader_SetColorConst(M2TXHeader *header, uint8 num, M2TXColor colorConst)
{
	if (header != NULL)
	{
		if (num>1)
			return (M2E_Range);
		header->ColorConst[num] = colorConst;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFlags
|||	Get the Flags field of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFlags(M2TXHeader *header, uint32 *flags)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the header flags
|||	    IsCompressed, HasPIP, and HasColorConst in one shot. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flags
|||	        The value of the header flags.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetFlags()
|||	
**/
M2Err M2TXHeader_GetFlags(M2TXHeader *header, uint32 *flags)
{
	if (header != NULL)
	{
		*flags = header->Flags;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFIsCompressed
|||	Get the IsCompressed flag of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFIsCompressed(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	    This function takes an M2TXHeader chunk and gets the IsCompressed flag.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the IsCompressed flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetFIsCompressed()
|||	
**/
M2Err M2TXHeader_GetFIsCompressed(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->Flags)&M2HC_IsCompressed)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasPIP
|||	Get the HasPIP flag of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasPIP(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the HasPIP
|||	    flag. The HasPIP flag indicates whether the M2TX texture
|||	    contains a PIP chunk or not.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasPIP flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetHasPIP()
|||	
**/
M2Err M2TXHeader_GetFHasPIP(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->Flags)&M2HC_HasPIP)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasNoDCI
|||	Get the HasNoDCI flag of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasNoDCI(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the HasNoDCI
|||	    flag. The HasNoDCI flag indicates whether the M2TX texture
|||	    should write out the DCI chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoDCI flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetHasNoDCI()
|||	
**/
M2Err M2TXHeader_GetFHasNoDCI(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->Flags)&M2HC_HasNoDCI)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasNoTAB
|||	Get the HasNoTAB flag of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasNoTAB(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the HasNoTAB
|||	    flag. The HasNoTAB flag indicates whether the M2TX texture
|||	    should write out the TAB chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoTAB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetHasNoTAB()
|||	
**/
M2Err M2TXHeader_GetFHasNoTAB(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->Flags)&M2HC_HasNoTAB)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasNoDAB
|||	Get the HasNoDAB flag of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasNoDAB(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the HasNoDAB
|||	    flag. The HasNoDAB flag indicates whether the M2TX texture
|||	    should write out the DAB chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoDAB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetHasNoDAB()
|||	
**/
M2Err M2TXHeader_GetFHasNoDAB(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->Flags)&M2HC_HasNoDAB)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasNoLR
|||	Get the HasNoLR flag of the Header
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasNoLR(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the HasNoLR
|||	    flag. The HasNoLR flag indicates whether the M2TX texture
|||	    should write out the Load Rectangles chunk on output.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasNoLR flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetHasNoLR()
|||	
**/
M2Err M2TXHeader_GetFHasNoLR(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->Flags)&M2HC_HasNoLR)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasColorConst
|||	Get the HasColorConst flag.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasColorConst(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the HasColorConst flag.
|||	    The HasColorConst flag indicates whether the M2TX
|||	    texture contains two color constants selected with the ssb bit in 
|||	    certain texture modes.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasColorConst flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetFHasColorConst()
|||	
**/
M2Err M2TXHeader_GetFHasColorConst(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if((header->Flags)&M2HC_HasColorConst)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetSize
|||	Get the size of the Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetSize(M2TXHeader *header, uint32 *size)
|||	
|||	  Description
|||	
|||	    This function returns the size of the data of the Header chunk.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    size
|||	        The size of the header chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
**/
M2Err M2TXHeader_GetSize(M2TXHeader *header, uint32 *size)
{
	if (header != NULL)
	{
		*size = header->Size;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetVersion
|||	Gets the Version field in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetVersion(M2TXHeader *header, uint32 *version)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the Version field.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    version
|||	        The version number of the texture format
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetVersion()
|||	
**/
M2Err M2TXHeader_GetVersion(M2TXHeader *header, uint32 *version)
{
	if (header != NULL)
	{
		*version = header->Version;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetMinXSize
|||	Get the value of the MinXSize field in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetMinXSize(M2TXHeader *header, uint16 *size)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets size to the value
|||	    the MinXSize field.  This sets the X dimensions of the coarsest level of
|||	    detail.  Each finer level of detail (LOD) is twice as wide and twice
|||	    as tall as the previous.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    size
|||	        The x dimensions of the coarsest level of detail.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetMinYSize(), M2TXHeader_SetMinXSize()
**/
M2Err M2TXHeader_GetMinXSize(M2TXHeader *header, uint16 *size)
{
	if (header != NULL)
	{
		*size = header->MinXSize;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetMinYSize
|||	Get the value of the MinYSize field in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetMinYSize(M2TXHeader *header, uint16 *size)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the 
|||	    MinYSize field. This sets the Y dimension of the coarsest level of
|||	    detail.  Each finer level of detail (LOD) is twice as wide and twice
|||	    as tall as the previous.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    size
|||	        The y dimensions of the coarsest level of detail.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetMinXSize(), M2TXHeader_SetMinYSize()
|||	
**/
M2Err M2TXHeader_GetMinYSize(M2TXHeader *header, uint16 *size)
{
	if (header != NULL)
	{
		*size = header->MinYSize;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetTexFormat
|||	Get the value of the texture format of a Header chunk
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetTexFormat(M2TXHeader *header, uint16 *format)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and get the value of the
|||	    header's texture format.  The flags IsLiteral, HasColor, HasAlpha,
|||	    HasSSB and values for CDepth and ADepth are included in the texture
|||	    format.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetTexFormat()
|||	
**/
M2Err M2TXHeader_GetTexFormat(M2TXHeader *header, uint16 *format)
{
	if (header != NULL)
	{
		*format = header->TexFormat;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFIsLiteral
|||	Get the value of the IsLiteral flag of the Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFIsLiteral(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the
|||	    IsLiteral flag . A literal texture has no palette (and therefore no
|||	    PIP) and uses either 15 bits per pixel or 24 bits per pixel for color 
|||	    information.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the IsLiteral flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetFIsLiteral()
|||	
**/
M2Err M2TXHeader_GetFIsLiteral(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if ((header->TexFormat)&M2HC_IsLiteral)
			*flag = TRUE;
		else
			*flag = FALSE;

	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasColor
|||	Get the value of the HasColor flag of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasColor(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the 
|||	    HasColor flag.   The flag indicates whether the texel contains 
|||	    imbedded color information. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasColor flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetFHasColor()
|||	
**/
M2Err M2TXHeader_GetFHasColor(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if((header->TexFormat)&M2HC_HasColor)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasAlpha
|||	Get the value of the HasAlpha flag of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasAlpha(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the 
|||	    HasAlpha flag.  The flag indicates whether the texel contains imbedded
|||	    alpha information. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasAlpha flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetFHasAlpha()
|||	
**/
M2Err M2TXHeader_GetFHasAlpha(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if((header->TexFormat)&M2HC_HasAlpha)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetFHasSSB
|||	Get the value of the HasSSB flag of a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetFHasSSB(M2TXHeader *header, bool *flag)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the 
|||	    HasSSB flag.  The flag indicates whether the texel contains imbedded
|||	    source select bit (SSB) information. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    flag
|||	        Value of the HasSSB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetFHasSSB()
|||	
**/
M2Err M2TXHeader_GetFHasSSB(M2TXHeader *header, bool *flag)
{
	if (header != NULL)
	{
		if((header->TexFormat)&M2HC_HasSSB)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetCDepth
|||	Get the value of the color depth of the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetCDepth(M2TXHeader *header, uint8 *cDepth)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets color depth.
|||	    Valid values are from 0 to 8.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    cDepth
|||	        The color depth.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetCDepth()
|||	
**/
M2Err M2TXHeader_GetCDepth(M2TXHeader *header, uint8 *cDepth)
{
	if (header != NULL)
	{
		*cDepth = (header->TexFormat) & M2HC_ColorDepth;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetADepth
|||	Get the alpha depth of the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetADepth(M2TXHeader *header, uint8 *aDepth)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the alpha depth.
|||	    Valid values are from 0,4, and 7.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    aDepth
|||	        The alpha depth.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetADepth()
|||	
**/
M2Err M2TXHeader_GetADepth(M2TXHeader *header, uint8 *aDepth)
{
	if (header != NULL)
	{
		*aDepth = ((header->TexFormat) & M2HC_AlphaDepth)>>M2Shift_AlphaDepth;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetNumLOD
|||	Get the value of the NumLOD field in the Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetNumLOD(M2TXHeader *header, uint8 *num)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the
|||	    NumLOD field.  This sets the number of levels of detail in the M2TX
|||	    texture.  Valid values are 1 to 11.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	      num
|||	        The number of levels of detail
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetNumLOD()
|||	
**/
M2Err M2TXHeader_GetNumLOD(M2TXHeader *header, uint8 *num)
{
	if (header != NULL)
	{
		*num = header->NumLOD;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetBorder
|||	Get the value of the border field in the header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetBorder(M2TXHeader *header, uint8 *border)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the border
|||	    field.  This field is reserved for future use
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    width
|||	        The value of border in the header chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetBorder()
|||	
**/
M2Err M2TXHeader_GetBorder(M2TXHeader *header, uint8 *border)
{
	if (header != NULL)
	{
		*border = header->Border;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetColorConst
|||	Sets a color constant in a Header chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetColorConst(M2TXHeader *header, uint8 num,
|||	                                   M2TXColor *colorConst)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and gets the value of the 
|||	    color constant indicated by num (value 0 or 1). Color 
|||	    constants are used to get color values in certain texture modes that use
|||	    the source select bit to select between two colors.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    colorConst
|||	        A color containing ssb, alpha, red, green, and blue values.
|||	    num
|||	        The index into the ColorConst array.  Valid values are 0 and 1.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetColorConst()
|||	
**/
M2Err M2TXHeader_GetColorConst(M2TXHeader *header, uint8 num, M2TXColor *colorConst)
{
	if (header != NULL)
	{
		if (num>1)
			return (M2E_Range);
		*colorConst = header->ColorConst[num];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetLODDim
|||	Get the dimensions of a specific LOD in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetLODDim(M2TXHeader *header, uint8 lod, uint32 *xSize,
|||	                               uint32 *ySize)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXHeader chunk and sets xSize and ySize to the
|||	    the dimensions of the level of detail specified by lod.  This is simply
|||	    a convenience function.
|||	
|||	  Caveats
|||	
|||	    The header fields NumLOD, MinXSize, and MinYSize need to be set before
|||	    calling this function.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetLODLength()
|||	
**/
M2Err M2TXHeader_GetLODDim(M2TXHeader *header, uint8 lod, uint32 *xSize, uint32 *ySize)
{
	uint16 minX, minY;
	uint8 numLOD, border;
	M2Err err;
	
	if (header != NULL)
	{
		err = M2TXHeader_GetMinXSize(header, &minX);
		err = M2TXHeader_GetMinYSize(header, &minY);
		M2TXHeader_GetBorder(header, &border);
		err = M2TXHeader_GetNumLOD(header, &numLOD);
		*xSize = minX << (numLOD-lod-1);
		*ySize = minY << (numLOD-lod-1);
		return(err);
	}	
	else
		return (M2E_BadPtr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_FreeLODPtr
|||	Frees a level of detail in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_FreeLODPtr(M2TXHeader *header, uint8 lod)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TXHeader chunk and frees level of detail 
|||	    LODPtr[lod].  The block of memory allocated to the level of detail is
|||	    returned to the system and the pointer LODPtr[lod] is set to NULL.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    lod
|||	        The index of the level of detail pointer (value 0 thru NumLOD-1)
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_FreeLODPtrs(), M2TXHeader_SetLODPtr(), M2TXHeader_GetLODPtr()
|||	
**/
M2Err M2TXHeader_FreeLODPtr(M2TXHeader *header, uint8 lod)
{
	if (header != NULL)
	{
		if ((header->LODDataPtr[lod])!=NULL)
		{
			qMemReleasePtr(header->LODDataPtr[lod]);
			header->LODDataPtr[lod] = NULL;				/* 1.04 */
		}
	}
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
	
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_FreeLODPtrs
|||	Frees all the level of detail in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_FreeLODPtrs(M2TXHeader *header)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TXHeader chunk and frees all the levels of 
|||	    detail pointers.  The blocks of memory allocated to the levels of detail
|||	    are returned to the system and the pointers are set to NULL.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_FreeLODPtr(), M2TXHeader_SetLODPtr(), M2TXHeader_GetLODPtr
|||	
**/
M2Err M2TXHeader_FreeLODPtrs(M2TXHeader *header)
{
	uint8 numLOD,lod;
	
	if (header != NULL)
	{
		numLOD = header->NumLOD;
		for (lod=0; lod<numLOD; lod++)
			if ((header->LODDataPtr[lod])!=NULL)
			{
				qMemReleasePtr(header->LODDataPtr[lod]);
				header->LODDataPtr[lod] = NULL;				/* 1.04 */
			}
	}
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetLODLength
|||	Get the memory size of a given level of detail.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetLODLength(M2TXHeader *header, uint8 lod, uint32 *length)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TXHeader chunk and returns the memory size of
|||	    of the block of memory pointed to by LODPtr[lod].  
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    lod
|||	        The index of the level of detail pointer (value 0 thru NumLOD-1)
|||	    length
|||	        The memory size in bytes of the level of detail
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetLODPtr(), M2TXHeader_GetLODPtr()
**/
M2Err M2TXHeader_GetLODLength(M2TXHeader *header, uint8 lod, uint32 *length)
{
	if (header != NULL)
	{
		if (lod>=header->NumLOD)
			return (M2E_Range);
		*length = header->LODDataLength[lod];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}	
	
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_GetLODPtr
|||	Gets a specific level of detail pointer in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetLODPtr(M2TXHeader *header, uint8 lod, uint32 *length,
|||	                               M2TXTex *tex)
|||	
|||	  Description
|||	    This functions takes an M2TXHeader chunk and returns the level of detail
|||	    LODPtr[lod] in variable tex.  The size of the level of detail is also
|||	    returned. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    lod
|||	        The index of the level of detail pointer (value 0 thru NumLOD-1)
|||	    length
|||	        The memory size in bytes of the level of detail
|||	    tex
|||	        The level of detail pointer
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_SetLODPtr(), M2TXHeader_GetLODLength()
|||	
**/
M2Err M2TXHeader_GetLODPtr(M2TXHeader *header, uint8 lod, uint32 *length, M2TXTex *tex)
{
	if (header != NULL)
	{
		if (lod>=header->NumLOD)
			return (M2E_Range);
		*tex = header->LODDataPtr[lod];
		*length = header->LODDataLength[lod];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXHeader_SetLODPtr
|||	Sets a specific level of detail pointer in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_SetLODPtr(M2TXHeader *header, uint8 lod, uint32 length,
|||	                               M2TXTex tex)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TXHeader chunk and sets the level of detail
|||	    LODPtr[lod] to the pointer tex.  The size of the level of detail is also
|||	    set. 
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    lod
|||	        The index of the level of detail pointer (value 0 thru NumLOD-1)
|||	    length
|||	        The memory size in bytes of the level of detail
|||	    tex
|||	        The level of detail pointer
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetLODPtr(), M2TXHeader_GetLODLength()
**/
M2Err M2TXHeader_SetLODPtr(M2TXHeader *header, uint8 lod, uint32 length, M2TXTex tex)
{
	if (header != NULL)
	{
	 	header->LODDataPtr[lod] = tex;
	 	header->LODDataLength[lod] = length;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);	
}

/**
|||	AUTODOC -private -class tools -group m2txlib -name M2TXHeader_GetDCIOffset
|||	Get the DCI chunk pointer in the Header.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXHeader_GetDCIOffset(M2TXHeader *header, uint32 *offset)
|||	
|||	  Description
|||	
|||	    This functions takes an M2TXHeader chunk and returns pointer to the DCI
|||	    chunk.
|||	
|||	  Arguments
|||	    
|||	    header
|||	        The input M2TXHeader chunk.
|||	    offset
|||	        The pointer to the DCI chunk.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXHeader.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	    M2TXHeader_GetPIPOffset(), M2TXHeader_Get
|||	
**/
M2Err M2TXHeader_GetDCIOffset(M2TXHeader *header, uint32 *offset)
{
	if (header != NULL)
	{
		*offset = header->DCIOffset;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

M2Err M2TXHeader_GetPIPOffset(M2TXHeader *header, uint32 *offset)
{
	if (header != NULL)
	{
		*offset = header->PIPOffset;
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}

M2Err M2TXHeader_GetLODOffset(M2TXHeader *header, uint8 lod, uint32 *offset)
{
	if (header != NULL)
	{
		if (lod>=header->NumLOD)
			return (M2E_Range);
		*offset = header->LODDataOffset[lod];
	}	
	else
		return (M2E_BadPtr);
	return (M2E_NoErr);
}
