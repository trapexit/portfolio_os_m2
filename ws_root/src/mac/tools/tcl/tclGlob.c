
#ifdef macintosh
#	pragma segment TCLGLOB
#endif

/* 
 * tclGlob.c --
 *
 *	This file provides procedures and commands for file name
 *	manipulation, such as tilde expansion and globbing.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

/* 
 * Copyright 1990 Karl Lehenbauer, Mark Diekhans, 
 *                Peter da Silva and Jordan Henderson.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Karl Lehenbauer, Mark Diekhans,
 * Peter da Silva and Jordan Henderson make no representations
 * about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

/* 
 * Copyright 1992 Tim Endres
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies. Tim Endres makes no representations
 * about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#ifndef NOSCCSID
	static char *sccs_id = "tclGlob.c 1.1 8/27/96";
#endif

#include <stdio.h>
#include <errno.h>

#ifndef macintosh
#	include <pwd.h>
#endif

#if defined(SYSV_3_2_0) || defined(XENIX286)
#  include "../../ossupport/src/stdlib.h"
#  include "../../ossupport/src/string.h"
#else
#  ifndef BSD
#    include <stdlib.h>
#  endif
#  include <string.h>
#endif

#ifdef macintosh
#	include <types.h>
#else
#	include <sys/types.h>
#endif

#if defined(BSD) || defined(HPUX)
#  include <sys/dir.h>
#endif

#ifdef macintosh
#	include <files.h>
#	include <errors.h>
#	include <stat.h>
#	define FALSE	0
#else
#	include <sys/stat.h>
#endif
#include "tcl.h"

#if defined(SYSV_3_2_0) || defined(XENIX286)
#  include "../../ossupport/src/ndir.h"
#endif
#if defined(SYSV_3_2_2) || defined(XENIX386)
#  include <dirent.h>
#endif

#ifdef XENIX
   extern int errno;
   extern struct passwd *getpwnam();
#endif

extern char *getenv();
#include "tclInt.h"

static int		_glob_debug_level = 0;
static int		_glob_show_invisibles = 0;

static int		_glob_show_type = 0;
static int		_glob_show_creator = 0;
static ResType	_glob_type;
static ResType	_glob_creator;


/*
 * The structure below is used to keep track of a globbing result
 * being built up (i.e. a partial list of file names).  The list
 * grows dynamically to be as big as needed.
 */

typedef struct {
    char *result;		/* Pointer to result area. */
    int totalSpace;		/* Total number of characters allocated
				 * for result. */
    int spaceUsed;		/* Number of characters currently in use
				 * to hold the partial result (not including
				 * the terminating NULL). */
    int dynamic;		/* 0 means result is static space, 1 means
				 * it's dynamic. */
} GlobResult;

/*
 *----------------------------------------------------------------------
 *
 * AppendResult --
 *
 *	Given two parts of a file name (directory and element within
 *	directory), concatenate the two together and add them to a
 *	partially-formed result.
 *
 * Results:
 *	There is no return value.  The structure at *resPtr is modified
 *	to hold more information.
 *
 * Side effects:
 *	Storage may be allocated if we run out of space in *resPtr.
 *
 *----------------------------------------------------------------------
 */

/*
 *----------------------------------------------------------------------
 *
 * AppendResult --
 *
 *	Given two parts of a file name (directory and element within
 *	directory), concatenate the two together and append them to
 *	the result building up in interp.
 *
 * Results:
 *	There is no return value.
 *
 * Side effects:
 *	Interp->result gets extended.
 *
 *----------------------------------------------------------------------
 */

static void
AppendResult(interp, dir, name, nameLength)
    Tcl_Interp *interp;		/* Interpreter whose result should be
				 * appended to. */
    char *dir;			/* Name of directory, with trailing
				 * slash (unless the whole string is
				 * empty). */
    char *name;			/* Name of file withing directory (NOT
				 * necessarily null-terminated!). */
    int nameLength;		/* Number of characters in name. */
{
    int dirLength, dirFlags, nameFlags;
    char *p, saved;

    /*
     * Next, see if we can put together a valid list element from dir
     * and name by calling Tcl_AppendResult.
     */

    if (*dir == 0) {
		dirFlags = 0;
    	}
	else {
		Tcl_ScanElement(dir, &dirFlags);
    	}
    saved = name[nameLength];
    name[nameLength] = 0;
    Tcl_ScanElement(name, &nameFlags);
    if ((dirFlags == 0) && (nameFlags == 0)) {
		/*if (_glob_debug_level>5)
			Feedback("GLOB: APPEND RESULT name '%.*s' result x%lx '%s' ",
						dir, nameLength, name, interp->result, interp->result);*/
		if (interp->result != NULL && *(interp->result) != '\0') {
			if (_glob_debug_level>5)
				Feedback("GLOB: APPEND RESULT ADD SPACE ");
			Tcl_AppendResult(interp, " ", dir, name, (char *) NULL);
			}
		else {
			Tcl_AppendResult(interp, dir, name, (char *) NULL);
			}
		name[nameLength] = saved;
		return;
		}

    /*
     * This name has weird characters in it, so we have to convert it to
     * a list element.  To do that, we have to merge the characters
     * into a single name.  To do that, malloc a buffer to hold everything.
     */

    dirLength = strlen(dir);
    p = (char *) ckalloc((unsigned) (dirLength + nameLength + 1));
    strcpy(p, dir);
    strcpy(p+dirLength, name);
    name[nameLength] = saved;
    Tcl_AppendElement(interp, p, 0);
    ckfree(p);
}


/*
 *----------------------------------------------------------------------
 *
 * DoGlob --
 *
 *	This recursive procedure forms the heart of the globbing
 *	code.  It performs a depth-first traversal of the tree
 *	given by the path name to be globbed.
 *
 * Results:
 *	The return value is a standard Tcl result indicating whether
 *	an error occurred in globbing.  The result in interp will be
 *	set to hold an error message, if any.  The result pointed
 *	to by resPtr is updated to hold all file names given by
 *	the dir and rem arguments.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


spacencount(str, len)
char	*str;
int		len;
{
int		count;

	if (len == -1)
		len = strlen(str);
	
	for (count=0 ; *str && len-- > 0 ; str++)
		if (*str == ' ')
			count++;
	
	return count;
	}

escape_spaces(name)
char	*name;
{
char	buffer[512], *ptr1, *ptr2;

	for (ptr1=name,ptr2=buffer ; *ptr1 ; ) {
		if (*ptr1 == ' ' && ptr1 > name && *(ptr1-1) != '\\')
			*ptr2++ = '\\';
		*ptr2++ = *ptr1++;
		}
	*ptr2 = '\0';
	for (ptr1=name, ptr2=buffer ; *ptr1++ = *ptr2++ ; )
		;
	}


static int
DoGlob(interp, dir, rem/*, resPtr*/)
Tcl_Interp *interp;		/* Interpreter to use for error
						** reporting (e.g. unmatched brace).
						*/
char *dir;				/* Name of a directory at which to
						** start glob expansion.  This name
						** is fixed: it doesn't contain any
						** globbing chars.  If it's non-empty
						** then it should end with a colon.
						*/
char *rem;				/* Path to glob-expand. */
/*GlobResult *resPtr;	*/	/* Where to store fully-expanded file names.*/
{
char	c,
		*p,
		*openBrace,
		*closeBrace;
char	mac_name[256];
int		gotSpecial,
		result;

    /*
    ** When this procedure is entered, the name to be globbed may
    ** already have been partly expanded by ancestor invocations of
    ** DoGlob.  The part that's already been expanded is in "dir"
    ** (this may initially be empty), and the part still to expand
    ** is in "rem".  This procedure expands "rem" one level, making
    ** recursive calls to itself if there's still more stuff left
    ** in the remainder.
    */

    /*
    ** When generating information for the next lower call,
    ** use static areas if the name is short, and malloc if the name
    ** is longer.
    */

#ifdef macintosh
	RotateCursor(32);
#endif

	if (_glob_debug_level)
		Feedback("GLOB: dir <%s> rem <%s> ", dir, rem);
	
#define STATIC_SIZE 200

    /*
    ** First, find the end of the next element in rem, checking
    ** along the way for special globbing characters.
    */

    gotSpecial = 0;
    openBrace = closeBrace = NULL;
    for (p = rem ; ; p++) {
		c = *p;
		if ((c == '\0') || (c == ':')) {
			break;
			}
		if ((c == '{') && (openBrace == NULL)) {
			openBrace = p;
			}
		if ((c == '}') && (closeBrace == NULL)) {
			closeBrace = p;
			}
		if ((c == '*') || (c == '[') || (c == '\\') || (c == '?')) {
			gotSpecial = 1;
			}
		}

    /*
    ** If there is an open brace in the argument, then make a recursive
    ** call for each element between the braces.  In this case, the
    ** recursive call to DoGlob uses the same "dir" that we got.
    ** If there are several brace-pairs in a single name, we just handle
    ** one here, and the others will be handled in recursive calls.
    */
    if (openBrace != NULL) {
		int remLength, l1, l2;
		char static1[STATIC_SIZE];
		char *element, *newRem;
	
		if (closeBrace == NULL)
			{
	    	Tcl_ResetResult(interp);
	    	interp->result = "unmatched open-brace in file name";
	    	return TCL_ERROR;
			}
	
		remLength = strlen(rem) + 1;
		if (remLength <= STATIC_SIZE) {
			newRem = static1;
			}
		else {
			newRem = ckalloc((unsigned) remLength);
			}
		l1 = openBrace-rem;
		strncpy(newRem, rem, l1);
		p = openBrace;
		for (p = openBrace; *p != '}'; )
			{
			element = p+1;
			for (p = element; ((*p != '}') && (*p != ',')); p++)
				{
				/* Empty body:  just find end of this element. */
				}
			l2 = p - element;
			strncpy(newRem+l1, element, l2);
			strcpy(newRem+l1+l2, closeBrace+1);
			if (DoGlob(interp, dir, newRem/*, resPtr*/) != TCL_OK)
				{
				return TCL_ERROR;
				}
			}
		
		if (remLength > STATIC_SIZE)
			{
			ckfree((char *)newRem);
			}
		
		return TCL_OK;
		}

    /*
    ** If there were any pattern-matching characters, then scan through
    ** the directory to find all the matching names.
    */
    if (gotSpecial) {
		char		*mac_name_ptr = dir;	/* Use for > 256 chars */
		int			l1, l2;
		int			i, index, vrefnum;
		long		dirid;
		char		*pattern, *newDir;
		char		static1[STATIC_SIZE], static2[STATIC_SIZE];
		char		volname[32];
		CInfoPBRec		cpb;
		ParamBlockRec	pb;
	
		if (*dir)
			strcpy(mac_name, dir);
		else
			strcpy(mac_name, ":");
		
		vrefnum = 0;
		dirid = 0;

		if (mac_name[0] == ':') {
			/* RELATIVE */
			WDPBRec	wpb;
			
			wpb.ioCompletion = 0;
			wpb.ioNamePtr = NULL;
			PBHGetVol(&wpb, FALSE);
			
			vrefnum = wpb.ioWDVRefNum;
			dirid = wpb.ioWDDirID;
			}
		else {
			/* ABSOLUTE */
			for (i = 0 ; mac_name[i] != '\0' ; i++) {
				if (mac_name[i] == ':')
					break;
				}
			if (mac_name[i] == ':' && i < 27) {
				strncpy(&volname[1], mac_name, i + 1);
				volname[0] = i + 1;
				pb.volumeParam.ioCompletion = 0;
				pb.volumeParam.ioNamePtr = volname;
				pb.volumeParam.ioVRefNum = 0;
				pb.volumeParam.ioVolIndex = -1;
				PBGetVInfo(&pb, FALSE);
				if (_glob_debug_level)
					Feedback("GLOB: SCAN: ABSOLUTE=%d Vol <%.*s>=%d ",
								pb.volumeParam.ioResult,
								volname[0], &volname[1], pb.volumeParam.ioVRefNum);
				if (pb.volumeParam.ioResult == noErr) {
					vrefnum = pb.volumeParam.ioVRefNum;
					dirid = fsRtParID;
					}
				}
			}
		
		c2pstr(mac_name);
		if (mac_name[mac_name[0]] == ':')	/*pete*/
			--mac_name[0];					/*pete*/
		
		cpb.hFileInfo.ioCompletion = 0;
		cpb.hFileInfo.ioVRefNum = vrefnum;
		cpb.hFileInfo.ioNamePtr = mac_name;
		cpb.hFileInfo.ioFDirIndex = 0;			/* Use name... */
		cpb.hFileInfo.ioDirID = dirid;
		result = PBGetCatInfo(&cpb, FALSE);
		if (_glob_debug_level/* > 1 || (_glob_debug_level && result)*/)
			Feedback("GLOB: SCAN: RESULT %d getting DIR <%.*s> ",
						result, mac_name[0], &mac_name[1]);
		if (result != noErr) {
			if (result == fnfErr)
				return TCL_OK;
			else
				{
				Tcl_ResetResult(interp);
				Tcl_AppendResult(interp, "couldn't read directory \"",
									( (*dir) ? dir : ":" ), "\" ", (char *) NULL);
				return TCL_ERROR;
				}
			}
		else if ((cpb.hFileInfo.ioFlAttrib & ioDirMask) == 0) {
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp, "directory \"",
								( (*dir) ? dir : ":" ), "\" is a file", (char *) NULL);
			return TCL_ERROR;
			}

		dirid = cpb.hFileInfo.ioDirID;
		if (_glob_debug_level)
			Feedback("GLOB: gotSpecial <%s> vRef %d dirID %ld ",
						(*dir ? dir : ":" ), cpb.hFileInfo.ioVRefNum, dirid);

		l1 = strlen(dir);
		l2 = (p - rem);
		if (l2 < STATIC_SIZE) {
			pattern = static2;
			}
		else {
			pattern = ckalloc((unsigned) (l2+1));
			}
		strncpy(pattern, rem, l2);
		pattern[l2] = '\0';
		
		result = TCL_OK;
		for (index=1 ; ; index++) {
			
#ifdef NEVER_DEFINED
			RotateCursor(32);
#endif
			
			cpb.hFileInfo.ioCompletion = 0;
			cpb.hFileInfo.ioVRefNum = vrefnum;
			cpb.hFileInfo.ioNamePtr = mac_name; mac_name[0] = '\0';
			cpb.hFileInfo.ioFDirIndex = index;
			cpb.hFileInfo.ioDirID = dirid;
			result = PBGetCatInfo(&cpb, FALSE);
			if ( _glob_debug_level > 1 || (_glob_debug_level && result) )
				Feedback("GLOB: INDEX-%03d = %d, <%.*s> [%ld]",
							index, result, mac_name[0], &mac_name[1], dirid);

			if (result == fnfErr) {
				result = TCL_OK;
				break;
				}

			if (! _glob_show_invisibles)
				if ((cpb.hFileInfo.ioFlFndrInfo.fdFlags & fInvisible) != 0) {
					if (_glob_debug_level)
						Feedback("GLOB: SKIP INVISIBLE <%.*s> ", mac_name[0], &mac_name[1]);
					continue;
					}
			
			if (_glob_show_type)
				if (cpb.hFileInfo.ioFlFndrInfo.fdType != _glob_type) {
					if (_glob_debug_level)
						Feedback("GLOB: SKIP !TYPE '%4.4s' <%.*s> ",
									&cpb.hFileInfo.ioFlFndrInfo.fdType, mac_name[0], &mac_name[1]);
					continue;
					}
			
			if (_glob_show_creator)
				if (cpb.hFileInfo.ioFlFndrInfo.fdCreator != _glob_creator) {
					if (_glob_debug_level)
						Feedback("GLOB: SKIP !CREATOR '%4.4s' <%.*s> ",
									&cpb.hFileInfo.ioFlFndrInfo.fdCreator, mac_name[0], &mac_name[1]);
					continue;
					}
			
			p2cstr(mac_name);
			
			/*
			 * Don't match names starting with "." unless the "." is
			 * present in the pattern.
			 */
			if ((*mac_name == '.') && (*pattern != '.')) {
				if (_glob_debug_level)
					Feedback("GLOB: SKIP DOT FILE <%.*s> ", mac_name[0], &mac_name[1]);
				continue;
				}
			if ((*mac_name == '¥') && (*pattern != '¥')) {
				if (_glob_debug_level)
					Feedback("GLOB: SKIP SPOT FILE <%.*s> ", mac_name[0], &mac_name[1]);
				continue;
				}

			if (Tcl_StringMatch(mac_name, pattern))
				{
				int nameLen;
				
				nameLen = strlen (mac_name);
				
				if (*p == '\0')
					{
					AppendResult(interp, dir, mac_name, nameLen);
					}
				else if ((cpb.hFileInfo.ioFlAttrib & ioDirMask) != 0)
					{
					if ((l1 + nameLen + 2) <= STATIC_SIZE)
						{
						newDir = static1;
						}
					else
						{
						newDir = ckalloc((unsigned) (l1 + nameLen + 2));
						}
					sprintf(newDir, "%s%s:", dir, mac_name);
					result = DoGlob(interp, newDir, p+1);
					if (newDir != static1)
						{
						ckfree((char *) newDir);
						}
					
					if (result != TCL_OK)
						break;
					}
				}
			else {
				if (_glob_debug_level)
					Feedback("GLOB: SKIP !MATCH <%.*s> <%s> ",
								mac_name[0], &mac_name[1], pattern);
				}
			}
		
		if (pattern != static2)
			{
			ckfree((char *)pattern);
			}

		return result;
    	}

    /*
    ** This is the simplest case:  just another path element.  Move
    ** it to the dir side and recurse (or just add the name to the
    ** list, if we're at the end of the path).
    */
    if (*p == 0) {
		AppendResult(interp, dir, rem, (int)(p-rem)/*, resPtr*/);
    	}
	else {
		int l1, l2;
		char *newDir;
		char static1[STATIC_SIZE];
	
		l1 = strlen(dir);
		l2 = l1 + (int)(p - rem) + 2;
		if (l2 <= STATIC_SIZE)
			{
			newDir = static1;
			}
		else {
			newDir = ckalloc((unsigned) l2);
			}
		strcpy(newDir, dir);
		strncpy(newDir+l1, rem, (int)(p - rem));
		newDir[l2-2] = ':';
		newDir[l2-1] = 0;
		result = DoGlob(interp, newDir, p+1/*, resPtr*/);
		if (newDir != static1) {
			ckfree((char *) newDir);
			}
		if (result != TCL_OK) {
			return TCL_ERROR;
			}
		}
	
    return TCL_OK;
	}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_TildeSubst --
 *
 *	Given a name starting with a tilde, produce a name where
 *	the tilde and following characters have been replaced by
 *	the home directory location for the named user.
 *
 * Results:
 *	The result is a pointer to a static string containing
 *	the new name.  This name will only persist until the next
 *	call to Tcl_TildeSubst;  save it if you care about it for
 *	the long term.  If there was an error in processing the
 *	tilde, then an error message is left in interp->result
 *	and the return value is NULL.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_TildeSubst(interp, name)
    Tcl_Interp *interp;		/* Interpreter in which to store error
				 * message (if necessary). */
    char *name;			/* File name, which may begin with "~/"
				 * (to indicate current user's home directory)
				 * or "~<user>/" (to indicate any user's
				 * home directory). */
{
#define STATIC_BUF_SIZE 50
    static char staticBuf[STATIC_BUF_SIZE];
    static int curSize = STATIC_BUF_SIZE;
    static char *curBuf = staticBuf;
    char *dir;
    int length;
    int fromPw = 0;
    register char *p;

    if (name[0] != '~') {
	return name;
    }

    /*
     * First, find the directory name corresponding to the tilde entry.
     */

    if ((name[1] == ':') || (name[1] == '\0')) {
		dir = getenv("HOME");
		if (dir == NULL) {
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp, "couldn't find HOME env. variable to expand \"",
								name, "\" ", (char *) NULL);
			return NULL;
			}
		p = name+1;
		}
	else {
		for (p = &name[1]; (*p != 0) && (*p != ':'); p++) {
			/* Null body;  just find end of name. */
			}
		length = p-&name[1];
		if (length >= curSize) {
			length = curSize-1;
			}
		memcpy(curBuf, name+1, length);
		curBuf[length] = '\0';
		dir = ":";
		fromPw = 0;
		}

    /*
     * Grow the buffer if necessary to make enough space for the
     * full file name.
     */

    length = strlen(dir) + strlen(p);
    if (length >= curSize) {
		if (curBuf != staticBuf) {
			ckfree((char *)curBuf);
			}
		curSize = length + 1;
		curBuf = ckalloc((unsigned) curSize);
		}

    /*
     * Finally, concatenate the directory name with the remainder
     * of the path in the buffer.
     */

    strcpy(curBuf, dir);
    strcat(curBuf, p);
    return curBuf;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobCmd --
 *
 *	This procedure is invoked to process the "glob" and globok Tcl
 *      commands.  See the user documentation for details on what they
 *      do.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_GlobCmd(dummy, interp, argc, argv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
   /*  GlobResult globRes; */
    char staticSpace[TCL_RESULT_SIZE];
    int i, result, noComplain = 0;
    int sargc;				/* Number of arguments. */
    char **sargv;			/* Argument strings. */
#pragma unused (dummy)

	sargc = argc; sargv = argv;
	
	_glob_debug_level = 0;
	_glob_show_type = 0;
	_glob_show_creator = 0;
	_glob_show_invisibles = 0;
	while (argv[1][0] == '-') {
		if (strcmp(argv[1], "-nocomplain") == 0) {
			noComplain = 1;
			argc--; argv++;
			}
		else if (argv[1][1] == 'd') {
			_glob_debug_level = 1;
			if (argv[1][2] >= '0' && argv[1][2] <= '9')
				_glob_debug_level = argv[1][2] - '0';
			argc--; argv++;
			}
		else if (argv[1][1] == 'i' && argv[1][2] == '\0') {
			_glob_show_invisibles = 1;
			argc--; argv++;
			}
		else if (argv[1][1] == 't' && argv[1][2] == '\0') {
			_glob_show_type = 1;
			sprintf((char *)&_glob_type, "%4.4s", argv[2]);
			argc -= 2; argv += 2;
			}
		else if (argv[1][1] == 'c' && argv[1][2] == '\0') {
			_glob_show_creator = 1;
			sprintf((char *)&_glob_creator, "%4.4s", argv[2]);
			argc -= 2; argv += 2;
			}
		else
			break;
		}
	
	if (_glob_debug_level) {
		for (i = 0 ; i < sargc ; i++)
			Feedback("GLOB: ARGV[%d] = '%s'", i, sargv[i]);
		}
	
    for (i = 1; i < argc; i++) {
		char *thisName;
	
		/*
		 * Do special checks for names starting at the root and for
		 * names beginning with ~.  Then let DoGlob do the rest.
		 */
	
		thisName = argv[i];
		if (*thisName == '~') {
			thisName = Tcl_TildeSubst(interp, thisName);
			if (thisName == NULL) {
				return TCL_ERROR;
				}
			}
		
		if (*thisName == ':') {
			result = DoGlob(interp, ":", thisName+1);
			}
		else {
			result = DoGlob(interp, "", thisName);
			}
		
		if (result != TCL_OK) {
	    	return result;
			}
    	}
	
    if ((*interp->result == 0) && !noComplain) {
		interp->result = "no files matched glob pattern(s)";
		return TCL_ERROR;
    	}
	
    return TCL_OK;
	}
