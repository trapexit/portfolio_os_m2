/*
	File:		M2TXFormat.c

	Contains:	M2 Texture Library, TexFormat manipulation functions 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<3+>	 7/15/95	TMA		Autodocs updated.
		 <2>	 5/16/95	TMA		Autodocs added.
		<1+>	 1/16/95	TMA		Add more error-checking

	To Do:
*/

#include "M2TXTypes.h"
#include "M2TXFormat.h"

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetCDepth
|||	Set the color depth of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetCDepth(M2TXFormat *format, uint8 cDepth)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the color depth to the value in cDepth. Valid values
|||	    are from 0 to 8.
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    cDepth
|||	        The color depth.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetCDepth()
|||	
**/
M2Err M2TXFormat_SetCDepth(M2TXFormat *format, uint8 cDepth)
{
	if (format != NULL)
	{
		if (cDepth > 8)
			return(-1);
		*format = (*format & (~M2HC_ColorDepth)) | (M2HC_ColorDepth & cDepth);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetADepth
|||	Set the alpha depth of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetADepth(M2TXFormat *format, uint8 aDepth)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the alpha depth to the value in aDepth. Valid values
|||	    are from 0,4,7.
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    aDepth
|||	        The alpha depth.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetADepth()
|||	
**/
M2Err M2TXFormat_SetADepth(M2TXFormat *format, uint8 aDepth)
{
	if (format != NULL)
	{
		if (aDepth > 7)
			return(-1);
		*format = (*format & (~M2HC_AlphaDepth)) | (M2HC_AlphaDepth & (aDepth<<M2Shift_AlphaDepth));
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetFIsLiteral
|||	Set IsLiteral flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetFIsLiteral(M2TXFormat *format, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the IsLiteral flag to the value in flag. A literal 
|||	    texture has no palette (and therefore no PIP) and uses either 15 bits
|||	    per pixel or 24 bits per pixel for color information.
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    flag
|||	        Value of the IsLiteral flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetFIsLiteral()
**/
M2Err M2TXFormat_SetFIsLiteral(M2TXFormat *format, bool flag)
{
	if (format != NULL)
	{
		if (flag)
			*format = (*format & (~M2HC_IsLiteral)) | (M2HC_IsLiteral);
		else
			*format = *format & (~M2HC_IsLiteral);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetFHasColor
|||	Set HasColor flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetFHasColor(M2TXFormat *format, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the HasColor flag to the value in flag.   The flag 
|||	    indicates whether the texel contains imbedded color information. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    flag
|||	        Value of the HasColor flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetFHasColor()
|||	
**/
M2Err M2TXFormat_SetFHasColor(M2TXFormat *format, bool flag)
{
	if (format != NULL)
	{
		if (flag)
			*format = (*format & (~M2HC_HasColor)) | M2HC_HasColor;
		else
			*format = (*format) & (~M2HC_HasColor);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetFHasAlpha
|||	Set the HasAlpha flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetFHasAlpha(M2TXFormat *format, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the HasAlpha flag to the value in flag.  The flag 
|||	    indicates whether the texel contains imbedded alpha information. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    flag
|||	        Value of the HasAlpha flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetFHasAlpha()
|||	
**/
M2Err M2TXFormat_SetFHasAlpha(M2TXFormat *format, bool flag)
{
	if (format != NULL)
	{
		if (flag)
			*format = ((*format) & (~M2HC_HasAlpha)) | M2HC_HasAlpha;
		else
			*format = (*format) & (~M2HC_HasAlpha);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetFIsTrans
|||	Set IsTrans flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetFIsTrans(M2TXFormat *format, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the IsTrans flag to the value in flag.  IsTrans is a
|||	    only used in DCI texel formats as it indicates a special type of 
|||	    compression.  A "transparent" texel format is used to indicate a single
|||	    color and consequently can be represented solely by the control byte in
|||	    a compressed texel.  It is very efficient for representing a widely used
|||	    color such as the transparent area around an irregular object. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    flag
|||	        Value of the IsTrans flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetFIsTrans()
|||	
**/
M2Err M2TXFormat_SetFIsTrans(M2TXFormat *format, bool flag)
{
	if (format != NULL)
	{
		if (flag)
			*format = ((*format) & (~M2CI_IsTrans)) | M2CI_IsTrans;
		else
			*format = (*format) & (~M2CI_IsTrans);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_SetFHasSSB
|||	Set HasSSB flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXFormat_SetFHasSSB(M2TXFormat *format, bool flag)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and sets the HasSSB flag to the value in flag.   The flag 
|||	    indicates whether the texel contains imbedded source select bit (SSB)
|||	    information. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	    flag
|||	        Value of the HasSSB flag.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_GetFHasSSB()
**/
M2Err M2TXFormat_SetFHasSSB(M2TXFormat *format, bool flag)
{
	if (format != NULL)
	{
		if (flag)
			*format = (*format & (~M2HC_HasSSB)) | (M2HC_HasSSB);
		else
			*format = *format & (~M2HC_HasSSB);
	}	
	else
		return (-1);
	return (M2E_NoErr);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetFIsLiteral
|||	Get the value of the IsLiteral flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    bool M2TXFormat_GetFIsLiteral(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the IsLiteral flag . A literal texture has no palette
|||	    (and therefore no PIP) and uses either 15 bits per pixel or 24 bits per
|||	    pixel for color information.
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The value of the IsLiteral flag
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_SetFIsLiteral()
|||	
**/
bool M2TXFormat_GetFIsLiteral(M2TXFormat format)
{
	if ((format)&M2HC_IsLiteral)
		return(TRUE);
	else
		return(FALSE);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetFHasColor
|||	Get the value of the HasColor flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    bool M2TXFormat_GetFHasColor(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the HasColor flag.   The flag indicates whether the
|||	    texel contains imbedded color information. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The value of the HasColor flag
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_SetFHasColor()
|||	
**/
bool M2TXFormat_GetFHasColor(M2TXFormat format)
{
  if (format & M2HC_HasColor)
    return(TRUE);
  else
    return(FALSE);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetFHasAlpha
|||	Get the value of the HasAlpha flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    bool M2TXFormat_GetFHasAlpha(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the HasAlpha flag.   The flag indicates whether the
|||	    texel contains imbedded alpha information. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The value of the HasAlpha flag.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_SetFHasAlpha()
|||	
**/
bool M2TXFormat_GetFHasAlpha(M2TXFormat format)
{
	if((format)&M2HC_HasAlpha)
		return(TRUE);
	else
		return(FALSE);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetFHasSSB
|||	Get the value of the HasSSB flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    bool M2TXFormat_GetFHasSSB(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the HasSSB flag.   The flag indicates whether the
|||	    texel contains imbedded source select bit (SSB) information. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The value of the HasSSB flag
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_SetFHasSSB()
|||	
**/
bool M2TXFormat_GetFHasSSB(M2TXFormat format)
{
  if((format)&M2HC_HasSSB)
    return(TRUE);
  else
    return(FALSE);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetFIsTrans
|||	Get the value of the IsTrans flag of a texel format.
|||	
|||	  Synopsis
|||	
|||	    bool M2TXFormat_GetFIsTrans(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the IsTrans flag.  
|||	    IsTrans is a only used in DCI texel formats as it indicates a special
|||	    type of compression.  A "transparent" texel format is used to indicate a
|||	    single color and consequently can be represented solely by the control
|||	    byte in a compressed texel.  It is very efficient for representing a 
|||	    widely used color such as the transparent area around an irregular 
|||	    object. 
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The value of the IsTrans flag.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TX_SetFIsTrans()
|||	
**/
bool M2TXFormat_GetFIsTrans(M2TXFormat format)
{
	if((format)&M2CI_IsTrans)
		return(TRUE);
	else
		return(FALSE);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetCDepth
|||	Get the color depth of a texel format.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXFormat_GetCDepth(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the color depth. Valid values are from 0 to 8.
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The color depth.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_SetCDepth()
|||	
**/
uint8 M2TXFormat_GetCDepth(M2TXFormat format)
{
       return ((format) & M2HC_ColorDepth);
}

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXFormat_GetADepth
|||	Get the alpha depth of a texel format.
|||	
|||	  Synopsis
|||	
|||	    uint8 M2TXFormat_GetADepth(M2TXFormat format)
|||	
|||	  Description
|||	
|||	    This function takes a M2TXFormat texel format (for use in the header or
|||	    the DCI) and gets the alpha depth. Valid values are from 0,4,7.
|||	
|||	  Arguments
|||	    
|||	    format
|||	        The input M2TXFormat texel format.
|||	
|||	  Return Value
|||	
|||	    The alpha depth.
|||	
|||	  Associated Files
|||	
|||	    <M2TXFormat.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXFormat_SetADepth()
|||	
**/
uint8 M2TXFormat_GetADepth(M2TXFormat format)
{
	return(((format) & M2HC_AlphaDepth)>>M2Shift_AlphaDepth);
}


