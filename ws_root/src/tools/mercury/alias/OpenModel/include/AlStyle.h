/*
//
//	Copyright (C) 1995, Alias|Wavefront
//
// These coded instructions, statements and computer programs contain
// unpublished information proprietary to Alias|Wavefront and
// are protected by the Canadian and US federal copyright law. They
// may not be disclosed to third parties or copied or duplicated, in 
// whole or in part, without prior written consent of 
// Alias|Wavefront
//
// Unpublished-rights reserved under the Copyright Laws of 
// the United States.
//
*/

#ifndef _AlStyle
#define _AlStyle

#ifndef __STDDEF__
#	include <stddef.h>
#	include <stdlib.h>
#endif

#ifndef _statusCodes
#	include <statusCodes.h>
#endif

#ifdef __cplusplus

#ifndef _debug
#	include <AlDebug.h>
#endif

#endif

#ifndef boolean
	typedef int	boolean;
#endif

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

typedef short Screencoord; /* from gl.h */

#endif	/* _AlStyle */
