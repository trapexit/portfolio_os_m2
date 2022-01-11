/*  @(#) linkver.h 96/09/23 1.42 */

/*
	File:		linkver.h

	Written by:	Dawn Perchik

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):
	

	   <15+>	96/06/13	JRM		1.22
	   <14+>	96/06/07	JRM		Bump it.
		<13>	96/04/26	JRM		Auto Update - 96/04/26
	   <12+>	96/04/24	JRM		1.18
	   <10+>	96/03/21	JRM		Check for a null pointer to avoid a crash.  Reword some of the
									messages.  Fix autodocs.
		 <7>	96/02/14	JRM		Bump
		<5+>	96/02/13	JRM		bugfix 12
		 <2>	96/01/11	JRM		Bump version

	To Do:
*/
/* linker/elf3do version -
 		used for Elf3do file format version compatibility
		by loader, dump tools, etc.

   version history:
		1.0 - initial release
		1.01 - new and changed relocation types and support for DLLs
		1.02 - _etext & _edata
				single import per library
				options for import libraries
				efficiency improvements
				incremental linking
				error on undefineds for -r
				fix import names - remove path 
				make import name offsets relative to start of record
				make import names relative to start of strings again
		1.03 - fix typos in this file
				don't warn about entry point if Dll
		1.04 - support for .init/.fini sections
		1.05 - fixed -B/-b options
				fixed branch prediction bit for rel14 relocations
		1.06 - removed BRN fix for rel14 from rel24
		1.07 - enabled dos version (complete support for little-endian)
				fixed bug in dlls when dll is first object
				dll ordering now matches command line
				unused dlls aren't added to .imp3do section
				-B/-b support enabled for relocatable objects
				absolute symbols not added to symtab
		1.08 - fix for Dll bug introduced during merge with Dos sources
				support for Dlls in archive libraries
				workaround for MrCpp comp[iler bug
		1.09 - fix deletes for class arrays to be delete arrays -
					MrCpp generates bad code otherwize
				fix string for progress bar
		1.10 - partial fix for structs & crashing forward refs -
				affects debugger and dumper code only
		1.11 - kevinh - fix calculation of symbol bases in elf_l.cpp
		                affects OS builds only
		1.12 - kevinh - fix export base calculation when base != 0
		                affects OS builds only
		1.13 -	kevinh - the linker was sometime leaving holes in
		                the exp3do sections
				the linker made errors if you had more than
				256 imports
 
		     -	johnMcM	Accelerated for Macintosh, Better errors
				for everyone
        1.14 -	kevinh - Add a -N option to defeat the autoloading
	                of DLLs
        1.15 -	johnMcM - Fix many warnings.  Fix array bounds overwrite.
		1.16 -	johnMcM - Check for a null pointer to avoid a crash.
				Reword some of the messages.  Fix autodocs.
		1.17 -	johnMcM - Implement -C option (compression).
		1.18 -	johnMcM - Implement -d option (strip more).
		1.19 -	johnMcM - Clean up FIXMEs.  Fix _etext and _edata relocations.
		1.20 -	johnMcM/Martin T. - Compression fixes; -e0; -g trumps -d;
		        REL14 crash; .x file parsing.
		1.21 -	johnMcM correct diagnostics for REL14 error.
		1.22 -  MH includes swapping for PC, and fix mapfile crash
		1.23 -	MH fix compression mapfile crash
		1.24 -  Path savvy multiplatform, not really relevant to the Mac but... mmh
*/

#define MAJORREVISION 1
#define MINORREVISION 24
#define RELEASESTAGE beta
#define NONFINALREVISION 24



#define ELF_COMPATIBILITY_VER 1.01
	/*loader compares against this*/

/*
**	Base linker version on LINKER_VER so that
**	we can associate executables with linker version
**	without having to pay the space overhead of having 
**	a comment section
*/

#ifdef rez
/*
** Defines for Macintosh resource compiler
*/
#define RELEASESTAGE beta
#define NONFINALREVISON 0x1
#define SHORTVERSION $$Format("%d.%2d", MAJORREVISION, NONFINALREVISION)
#define LONGVERSION1 $$Format("%d.%2d, © 1995-1996 The 3DO Company", MAJORREVISION,NONFINALREVISION);
								
#define LONGVERSION2 ""
#else
/*
** Defines for "C" and C++
*/
#define LINKER_VER MAJORREVISION##.##NONFINALREVISION
	/*reflects linker source changes*/
#define __STR(x) #x
#define _STR(x) __STR(x)
#define LINK3DO_VER _STR(LINKER_VER)
#define ELF3DO_VER ((unsigned long)(MAJORREVISION * 100 + MINORREVISION))
#define ELF3DO_COMPATIBILITY_VER ((unsigned long)(ELF_COMPATIBILITY_VER*100))
#endif
