/*
	File:		M2TXDCI.c

	Contains:	DCI structure manipulation routines 

	Written by:	Todd Allendorf	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<3+>	 7/15/95	TMA		Autodocs updated.
		 <2>	 5/16/95	TMA		Autodocs added.
		<1+>	 1/16/95	TMA		Update functions to handle custom compression

	To Do:
*/

#include "M2TXTypes.h"
#include "M2TXDCI.h"
#include "M2TXFormat.h"

/* Accessors to DCI */


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetSize
|||	Get the size of the data of the DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetSize(M2TXDCI *dci, uint32 *size)
|||	
|||	  Description
|||	
|||	    Gets the size of the data of a DCI and returns it in size. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    size
|||	        The size of the data in dci.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
**/

M2Err M2TXDCI_GetSize(M2TXDCI *dci, uint32 *size)
{
	if (dci != NULL)
	{
		*size = dci->Size;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetTexFormat
|||	Set the texture format of a DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetTexFormat(M2TXDCI *dci, uint8 format, uint16 texFormat)
|||	
|||	  Description
|||	
|||	    Sets one of the four texture formats in a DCI chunk to the 
|||	    format specified by texFormat. The variable format indicates which [0-3]
|||	    format to set. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    texFormat
|||	        The value to set the texel format to.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetTexFormat()
**/

M2Err M2TXDCI_SetTexFormat(M2TXDCI *dci, uint8 format, uint16 texFormat)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		dci->TexelFormat[format] = texFormat;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetFIsTrans
|||	Set the Transparency of a DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetFIsTrans(M2TXDCI *dci, uint8 format, bool flag)
|||	
|||	  Description
|||	
|||	    Sets the transparency flag in a DCI texture format. Use zero as the flag
|||	    argument to indicate the format is not single color and one that it is a
|||	    single color texel format.  The variable "format" indicates which [0-3]
|||	    format to set. 
|||	
|||	  Caveats
|||	
|||	    A transparent format is a format which can be represented entirely by 
|||	    the control byte and is therefore useful as a transparent "filler" 
|||	    around a non-rectuangular sprites.  Transparent formats also can have 
|||	    runs of sixty-four pixels in length instead of the thirty-two of
|||	    non-transparent formats.  A transparent format can be any color or 
|||	    opacity.  A four color or less texture can be represented entirely by 
|||	    transparent formats to provide additional space savings since runs are
|||	    represented as a single byte instead of two.
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    flag
|||	        The value of IsTrans (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetFIsTrans()
**/

M2Err M2TXDCI_SetFIsTrans(M2TXDCI *dci, uint8 format, bool flag)
{
	if (dci != NULL)
	{
		if (format>3)
			return (-1);
		if (flag)
			dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_IsTrans))
										| (M2CI_IsTrans);
		else
			dci->TexelFormat[format] = dci->TexelFormat[format] & (~M2CI_IsTrans);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetFIsLiteral
|||	Set the value of the IsLiteral field of a DCI chunk.
|||	
|||	  Synopsis 
|||	
|||	    M2Err M2TXDCI_SetFIsLiteral(M2TXDCI *dci, uint8 format, bool flag)
|||	
|||	  Description
|||	
|||	    Sets the value of the IsLiteral field of a DCI chunk to off if the value
|||	    of flag is zero, on if the value of flag is 1. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    flag
|||	        The value of IsLiteral (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetFIsTrans()
**/

M2Err M2TXDCI_SetFIsLiteral(M2TXDCI *dci, uint8 format, bool flag)
{
	if (dci != NULL)
	{
		if (format>3)
			return (-1);
		if (flag)
			dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_IsLiteral)) 
										| (M2CI_IsLiteral);
		else
			dci->TexelFormat[format] = dci->TexelFormat[format] & (~M2CI_IsLiteral);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetFHasColor
|||	Set the HasColor field of a DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetFHasColor(M2TXDCI *dci, uint8 format, bool flag)
|||	
|||	  Description
|||	
|||	    Sets the value of the HasColor field of a DCI texel format to off if the
|||	    value of flag is zero, on if the value of flag is 1. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    flag
|||	        The value of HasColor (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetFHasColor()
**/

M2Err M2TXDCI_SetFHasColor(M2TXDCI *dci, uint8 format, bool flag)
{
	if (dci != NULL)
	{
		if (format>3)
			return (-1);
		if (flag)
			dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_HasColor)) 
										| (M2CI_HasColor);
		else
			dci->TexelFormat[format] = dci->TexelFormat[format] & (~M2CI_HasColor);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetFHasAlpha
|||	Set the HasAlpha field of a DCI chunk. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetFHasAlpha(M2TXDCI *dci, uint8 format, bool flag)
|||	
|||	  Description
|||	
|||	    Sets the value of the HasAlpha field of a DCI texel format to off if the
|||	    value of flag is zero, on if the value of flag is 1. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    flag
|||	        The value of HasAlpha (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetFHasAlpha()
**/
M2Err M2TXDCI_SetFHasAlpha(M2TXDCI *dci, uint8 format, bool flag)
{
	if (dci != NULL)
	{
		if (format>3)
			return (-1);
		if (flag)
			dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_HasAlpha)) 
										| (M2CI_HasAlpha);
		else
			dci->TexelFormat[format] = dci->TexelFormat[format] & (~M2CI_HasAlpha);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetFHasSSB
|||	Set the HasSSB field of a DCI chunk. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetFHasSSB(M2TXDCI *dci, uint8 format, bool flag)
|||	
|||	  Description
|||	
|||	    Sets the value of the HasSSB field of a DCI texel format to off if the 
|||	    value of flag is zero, on if the value of flag is 1. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    flag
|||	        The value of HasSSB (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetFHasSSB()
**/

M2Err M2TXDCI_SetFHasSSB(M2TXDCI *dci, uint8 format, bool flag)
{
	if (dci != NULL)
	{
		if (format>3)
			return (-1);
		if (flag)
			dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_HasSSB)) 
										| (M2CI_HasSSB);
		else
			dci->TexelFormat[format] = dci->TexelFormat[format] & (~M2CI_HasSSB);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetCDepth
|||	Set the CDepth of a DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetCDepth(M2TXDCI *dci, uint8 format, uint8 cDepth)
|||	
|||	  Description
|||	
|||	    Sets the CDepth field of the a texel format in the DCI chunk to the value 
|||	    specified in cDepth.  
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    cDepth
|||	        The value of the texel format's color depth (0-8).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetCDepth()
**/

M2Err M2TXDCI_SetCDepth(M2TXDCI *dci, uint8 format, uint8 cDepth)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if (cDepth>8)
			return (-1);
		dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_ColorDepth))
			 				| (M2CI_ColorDepth & cDepth);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetADepth
|||	Set the ADepth of a DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetADepth(M2TXDCI *dci, uint8 format, uint8 aDepth)
|||	
|||	  Description
|||	
|||	    Sets the ADepth field of a texel format in the DCI chunk to the value
|||	    specified in aDepth.
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    aDepth
|||	        The value of the texel format's alpha depth (0,4,7).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetADepth()
**/

M2Err M2TXDCI_SetADepth(M2TXDCI *dci, uint8 format, uint8 aDepth)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if (aDepth>7)
			return (-1);
		dci->TexelFormat[format] = (dci->TexelFormat[format] & (~M2CI_AlphaDepth))
			 				| (M2CI_AlphaDepth & (aDepth<<M2Shift_AlphaDepth));
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_SetColorConst
|||	Set the value of the color constant of a DCI. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_SetColorConst(M2TXDCI *dci, uint8 format, M2TXColor colorConst)
|||	
|||	  Description
|||	
|||	    Sets the value of the color constant of a texel format in the DCI chunk
|||	    to colorConst, which has to be an M2TXColor structure. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be changed.  Values are 0-3.
|||	    colorConst
|||	        The input color constant.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_GetColorConst()
**/

M2Err M2TXDCI_SetColorConst(M2TXDCI *dci, uint8 num, M2TXColor colorConst)
{
	if (dci != NULL)
	{
		if (num>3)
			return (-1);
		dci->TxExpColorConst[num] = colorConst;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetTexFormat
|||	Retrieve the value of a specified texel format from a DCI chunk.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetTexFormat(M2TXDCI *dci, uint8 format, uint16 *texFormat)
|||	  Description
|||	
|||	    Retrieves the texel format value of a specified texel format in the DCI
|||	    chunk and stores it in texFormat
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    texFormat
|||	        The value of the texel format.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetTexFormat()
**/

M2Err M2TXDCI_GetTexFormat(M2TXDCI *dci, uint8 format, uint16 *texFormat)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		*texFormat = dci->TexelFormat[format];
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetFIsTrans
|||	Retrieve the value of the IsTrans flag of the specified DCI chunk. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetFIsTrans(M2TXDCI *dci, uint8 format, bool *flag)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the IsTrans flag of a texel format in the DCI
|||	    chunk specified by dci and stores it in flag. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    flag
|||	        The value of IsTrans (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetFIsTrans()
**/

M2Err M2TXDCI_GetFIsTrans(M2TXDCI *dci, uint8 format, bool *flag)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if((dci->TexelFormat[format])&M2CI_IsTrans)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetFIsLiteral
|||	Retrieve the value of the IsLiteral field of the specified DCI texel 
|||	  format. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetFIsLiteral(M2TXDCI *dci, uint8 format, bool *flag)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the IsLiteral field of a texel format of the DCI
|||	    chunk specified by dci and stores it in flag. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    flag
|||	        The value of IsLiteral (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetFIsLiteral()
**/

M2Err M2TXDCI_GetFIsLiteral(M2TXDCI *dci, uint8 format, bool *flag)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if((dci->TexelFormat[format])&M2CI_IsLiteral)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetFHasColor
|||	Retrieve the value of the HasColor field of the specified a DCI texel format. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetFHasColor(M2TXDCI *dci, uint8 format, bool *flag)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the HasColor field of a texel format of the DCI
|||	    chunk specified by dci and stores it in flag. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    flag
|||	        The value of HasColor (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetFHasColor()
**/

M2Err M2TXDCI_GetFHasColor(M2TXDCI *dci, uint8 format, bool *flag)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if((dci->TexelFormat[format])&M2CI_HasColor)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetFHasAlpha
|||	Retrieve the value of the HasAlpha field of the specified DCI texel format. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetFHasAlpha(M2TXDCI *dci, uint8 format, bool *flag)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the HasAlpha field of a texel format of the DCI
|||	    chunk specified by dci and stores it in flag. 
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    flag
|||	        The value of HasAlpha (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetFHasAlpha()
**/

M2Err M2TXDCI_GetFHasAlpha(M2TXDCI *dci, uint8 format, bool *flag)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if((dci->TexelFormat[format])&M2CI_HasAlpha)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetFHasSSB
|||	Retrieve the value of the HasSSB field of the specified DCI texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetFHasSSB(M2TXDCI *dci, uint8 format, bool *flag)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the HasSSB field of a texel format of the DCI
|||	    chunk specified by dci and stores it in flag.
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    flag
|||	        The value of HasSSB (TRUE or FALSE).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetFHasSSB()
**/
M2Err M2TXDCI_GetFHasSSB(M2TXDCI *dci, uint8 format, bool *flag)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		if((dci->TexelFormat[format])&M2CI_HasSSB)
			*flag = TRUE;
		else
			*flag = FALSE;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetCDepth
|||	Retrieve the value of the CDepth field of the specified DCI texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetCDepth(M2TXDCI *dci, uint8 format, uint8 *cDepth)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the CDepth field of a DCI texel format specified
|||	    by dci and stores it in cDepth.
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    cDepth
|||	        The value of the texel format's color depth (0-8).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetCDepth()
**/

M2Err M2TXDCI_GetCDepth(M2TXDCI *dci, uint8 format, uint8 *cDepth)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		*cDepth = (dci->TexelFormat[format]) & M2CI_ColorDepth;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetADepth
|||	Retrieve the value of the ADepth field of the specified DCI texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetADepth(M2TXDCI *dci, uint8 format, uint8 *aDepth)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the ADepth field of a texel format of the DCI
|||	    chunk specified by dci and stores it in aDepth.
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    aDepth
|||	        The value of the texel format's alpha depth (0,4,7).
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetADepth()
**/

M2Err M2TXDCI_GetADepth(M2TXDCI *dci, uint8 format, uint8 *aDepth)
{
	if (dci != NULL)
	{
		if (format > 3)
			return(-1);
		*aDepth = ((dci->TexelFormat[format]) & M2CI_AlphaDepth)>>M2Shift_AlphaDepth;
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXDCI_GetColorConst
|||	Retrieve the value of the ColorConst field of the specified DCI texel format. 
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXDCI_GetColorConst(M2TXDCI *dci, uint8 format, M2TXColor *colorConst)
|||	
|||	  Description
|||	
|||	    Retrieves the value of the ColorConst field of the DCI chunk specified 
|||	    by dci and stores it in colorConst.
|||	
|||	  Arguments
|||	    
|||	    dci
|||	        The input data compression information chunk.
|||	    format
|||	        The texel format in the DCI to be queried.  Values are 0-3.
|||	    colorConst
|||	        The texel format's color constant.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDCI.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXDCI_SetColorConst()
**/
M2Err M2TXDCI_GetColorConst(M2TXDCI *dci, uint8 num, M2TXColor *colorConst)
{
	if (dci != NULL)
	{
		if (num>3)
			return (-1);
		*colorConst = dci->TxExpColorConst[num];
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

