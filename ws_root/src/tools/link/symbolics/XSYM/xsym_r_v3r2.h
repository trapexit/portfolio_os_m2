/*
	File:		xsym_r_v3r3.h
	
	Contents:	Declarations for class to parse Version 3.3 Sym files

	Copyright:	© 1994 by The 3DO Company, all rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change history (most recent first):
	95/02/02	NBT		Append "v32" to class name, to allow this class
						to exist while a sister class for Sym 3.3 exists.
	95/02/02	akm		cat original v32 class back into this file and change only MTE entry
*/

//NOTE: since XsymReader_v32_ and XsymReader_ are practically the same,
//we include xsym_r_v3r2.h and put #ifdefs around code specific to XsymReader_
//#ifdef wrappers for this file are in "xsym_r_v3r2.h" itself

#include "xsym_verdefs.h"

#ifndef __XSYM_R_V3R2_H__
	#undef __XSYM_VER__ 
	#define __XSYM_VER__ __XSYM_V32__
	#include "xsym_r_v3r3.h"
#define __XSYM_R_V3R2_H__
#endif
