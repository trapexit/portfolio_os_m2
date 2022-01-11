/*  @(#) dumpver.h 96/09/23 1.7 */

/*
 *  @(#) dumpver.h 96/01/17 1.2
 *	Copyright 1995-1996, The 3DO Company
 *
 */

/*
	File:		dumpver.h

	Written by:	Dawn Perchik

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		<8+>	96/06/13	JRM		1.05
		<6+>	96/03/21	JRM		Update to 1.0.4
		<2+>	96/01/17	JRM		Merge

	To Do:
*/


/* dumper/elf3do version -
 		used for Elf3do file format version compatibility
		by loader, dump tools, etc.

   version history:
		1.0 - initial release
		1.01 - new and changed relocation types and support for DLLs
		1.02 - disassembler added
		1.04 - Fixed crash in ElfDumper::dump_3dohdr().
		1.05 - Minimal support for compressed sections (at least headers).
*/

#define MAJORREVISION 1
#define MINORREVISION 0.8

/*
**	Base dumper version on DUMPER_VER so that
**	we can associate executables with dumper version
**	without having to pay the space overhead of having 
**	a comment section
*/

#ifdef rez
/*
** Defines for Macintosh resource compiler
*/
#define RELEASESTAGE release
#define NONFINALREVISON 0
#define SHORTVERSION $$Format("%d.%2d", MAJORREVISION, MINORREVISION)
#define LONGVERSION1 $$Format("%d.%2d, © 1995-1996 The 3DO Company", MAJORREVISION, MINORREVISION)
#define LONGVERSION2 ""
#else
/*
** Defines for "C" and C++
*/
#define DUMPER_VER MAJORREVISION##.##MINORREVISION
	/*reflects dumper source changes*/
#define __STR(x) #x
#define _STR(x) __STR(x)
#define DUMP3DO_VER _STR(DUMPER_VER)
#endif
