// /@(#) xsym_verdefs.h 95/06/27 1.3



#ifndef __XSYM_VERDEFS_H__
#define __XSYM_VERDEFS_H__

// versions defs - __XSYM_VER__ checked against
#define __XSYM_V31__ 31
#define __XSYM_V32__ 32
#define __XSYM_V33__ 33
#define __XSYM_V34__ 34

#ifndef __XSYM_VER__
	#define __XSYM_VER__ __XSYM_V33__	//default
#endif

//figure out which header to use
#undef DEF_FILE 
#if __XSYM_VER__ == __XSYM_V31__
	#define DEF_FILE "xsym_defs_v31.h"
#elif __XSYM_VER__ == __XSYM_V32__
	#define DEF_FILE "xsym_defs_v32.h"
#elif __XSYM_VER__ == __XSYM_V33__
	#define DEF_FILE "xsym_defs_v33.h"
#elif __XSYM_VER__ == __XSYM_V34__
	#define DEF_FILE "xsym_defs_v34.h"
#endif
void  gen_offsets();	//for generating automatic XSYM sizes/offsets
#endif /* __XSYM_VERDEFS_H__ */
