//  @(#) predefines.h 95/12/05 1.11
/*
	predefines.h
	
	Copyright:	© 1994 by The 3DO Company, all rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.
*/

#ifndef __predefines__
#define __predefines__

#define USE_OLD_HEADERS	0
#define STRICT_WINDOWS 0

// Dawn's symbolic additions
#if !defined(DUMPER) && !defined(LINKER)
	#define __3DO_DEBUGGER__
#endif
#if !defined(_SUN4) && !defined(_MSDOS)
	//#define DEBUG
	#define _MAC 
	#define LOAD_FOR_MAC	//to work with loader's headers
	//#define _MW_v6_
#else
	//must define when compiling with new gcc versions 
	//#define _NEW_GCC
#endif
#ifndef _MW_v6_
	#define GetWindowKind(aWindowRef) ( *(SInt16 *)	(((UInt8 *) aWindowRef) + sizeof(GrafPort)))
#endif
#define _DDI_BUG
#define _MW_BUG_
#define __NEW_ARMSYM__
#if defined(USE_DUMP_FILE)
	#define INLINE
#else
	#define INLINE inline
#endif

#endif
