/*
	Written by:	Reggie Seagraves

	Copyright:	© 1994 by The 3DO Company, all rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	95/02/02	akm		cat original v32 class back into this file and change only MTE entry

*/

//NOTE: since XsymReader_v3r2 and XsymReader_v3r3 are practically the same,
//we include XsymReader_v3r2.cp and put #ifdefs around code specific to XsymReader_v3r3
//#ifdef wrappers for this file are in "xsym_r_v3r2.h" itself

#include "xsym_verdefs.h"

#undef __XSYM_VER__ 
#define __XSYM_VER__ __XSYM_V32__
#include "xsym_r_v3r3.cp"
