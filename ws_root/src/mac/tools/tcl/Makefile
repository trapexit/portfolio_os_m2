#
# This Makefile is for use when distributing Tcl to the outside world.
# It is normally set up by running the "config" script.  Before modifying
# this file by hand, you should read through the "config" script to see
# what it does.
#
# Some changes you may wish to make here:
#
# 1. To compile for non-UNIX systems (so that only the non-UNIX-specific
# commands are available), change the OBJS line below so it doesn't
# include ${UNIXOBJS}.  Also, add the switch "-DTCL_GENERIC_ONLY" to
# CFLAGS.  Lastly, you'll have to provide your own replacement for the
# "panic" procedure (see panic.c for what the current one does). This
# is not necessary for Macintosh, since these routines are ported.
#
# 2. ANSI-C procedure prototypes are turned on by default if supported
# by the compiler.  To turn them off, add "-DNO_PROTOTYPE" to CFLAGS
# below.
#
# 3. If you've put the Tcl script library in a non-standard place, change
# the definition of TCL_LIBRARY to correspond to its location on your
# system.
#

tcl = :
tclSrc = :
tclObj = :obj:


#TCL_LIBRARY =	/usr/local/lib/tcl
TCL_LIBRARY =	:tcl

CC =		C
CFLAGS =	-w -mbg full -b3 -i {tclSrc} -d TCL_LIBRARY="¶"{TCL_LIBRARY}¶""

GENERIC_OBJS =					¶
	"{tclObj}"regexp.c.o		¶
	"{tclObj}"tclAssem.c.o		¶
	"{tclObj}"tclBasic.c.o		¶
	"{tclObj}"tclCkalloc.c.o	¶
	"{tclObj}"tclCmdAH.c.o		¶
	"{tclObj}"tclCmdIL.c.o		¶
	"{tclObj}"tclCmdMZ.c.o		¶
	"{tclObj}"tclExpr.c.o		¶
	"{tclObj}"tclGet.c.o		¶
	"{tclObj}"tclHash.c.o		¶
	"{tclObj}"tclHistory.c.o	¶
	"{tclObj}"tclParse.c.o		¶
	"{tclObj}"tclProc.c.o		¶
	"{tclObj}"tclUtil.c.o		¶
	"{tclObj}"tclVar.c.o

UNIXOBJS =						¶
	"{tclObj}"stat.c.o			¶
	"{tclObj}"Xmath.c.o			¶
	"{tclObj}"Xfmath.c.o		¶
	"{tclObj}"Xstring.c.o		¶
	"{tclObj}"tclEnv.c.o		¶
	"{tclObj}"tclGlob.c.o		¶
	"{tclObj}"tclUnixAZ.c.o		¶
	"{tclObj}"tclUnixStr.c.o	¶
	"{tclObj}"tclUnixUtil.c.o 

MACOBJS =						¶
	"{tclObj}"tclMacUtil.c.o	¶
	"{tclObj}"macPanic.c.o

COMPAT_OBJS =

OBJS = {GENERIC_OBJS} {MACOBJS} {UNIXOBJS} {COMPAT_OBJS}

LIBS =							¶
	{Libraries}RunTime.o		¶
	{Libraries}Interface.o		¶
	{Libraries}ToolLibs.o		¶
	{CLibraries}StdCLib.o

All Ä "{tclObj}"libtcl.o

Clean Ä
	Echo "# `Date -t` ----- Removing all built objects..."
	Delete -y -i {OBJS} {UNIXOBJS} {MACOBJS} "{tclObj}"libtcl.o 
	
"{tcl}"libtcl.o Ä "{tclObj}"libtcl.o
	Echo "# `Date -t` ----- Copy "{tclObj}"libtcl.o to "{tcl}"libtcl.o..."
	Duplicate -y "{tclObj}"libtcl.o "{tcl}"

"{tcl}"tcl.h Ä "{tclSrc}"tcl.h
	Echo "# `Date -t` ----- Copy "{tclSrc}"tcl.h to "{tcl}"tcl.h..."
	Duplicate -y ""{tclSrc}"tcl.h" "{tcl}"

"{tclObj}"libtcl.o Ä {OBJS}
	Echo "# `Date -t` ----- Building "{tclObj}"libtcl.o..."
	Lib -o "{tclObj}"libtcl.o {OBJS}

"{tcl}"tclTool Ä "{tclObj}"tclTest.c.o "{tclObj}"libtcl.o
	Echo "# `Date -t` ----- Linking "{tcl}"tclTool..."
	Link -c "MPS " -t "MPST"	¶
		-sym off -d				¶
		-o "{tcl}"tclTool		¶
		"{tclObj}"tclTest.c.o	¶
		"{tclObj}"libtcl.o		¶
		{LIBS}					¶
		|| ( Echo "#####   ### ERROR ###   #####"; exit )

"{tcl}"tclShell Ä "{tclObj}"tclTest.c.o "{tclObj}"libtcl.o
	Echo "# `Date -t` ----- Linking "{tcl}"tclShell..."
	Link -c "Stcl" -t "APPL"	¶
		-sym off -d				¶
		-o "{tcl}"tclShell		¶
		"{tclObj}"tclTest.c.o	¶
		"{tclObj}"libtcl.o		¶
		"{Libraries}"SIOW.o		¶
		{LIBS}					¶
		|| ( Echo "#####   ### ERROR ###   #####"; exit )
	Echo "# `Date -t` ----- Resource Compiling "{tcl}"tclShell..."
	Rez -a "{RIncludes}"SIOW.r -o "{tcl}"tclShell


#
# Set up object and source directory dependency
#
"{tclObj}" Ä "{tclSrc}"


#
# This command is what is used to compile all C source code.
#
.c.o	Ä	.c
	Echo "# `Date -t` ----- Compiling {default}.c..."
	C "{depDir}{default}.c"				¶
		{CFlags} {CONTEXTFLAGS}			¶
		-o "{targDir}{default}.c.o"		¶
		|| ( Echo "#####   ### ERROR ###   #####"; exit )

#
# This command is what is used to compile all Asm source code.
#
.a.o	Ä	.a
	Echo "# `Date -t` ----- Assembling {default}.a..."
	Asm "{depDir}{default}.a"			¶
		-o "{targDir}{default}.a.o"		¶
		|| ( Echo "#####   ### ERROR ###   #####"; exit )

