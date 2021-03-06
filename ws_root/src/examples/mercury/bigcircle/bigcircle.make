#   Copyright (c) 1993-1996, an unpublished work by The 3DO Company.
#   All rights reserved. This material contains confidential
#   information that is the property of The 3DO Company. Any
#   unauthorized duplication, disclosure or use is prohibited.
#   
#   @(#) bigcircle.make 96/11/07 1.19
#   Distributed with Portfolio V33.1

LIBMERCURY = ?
		-l :::lib:libmercury1:objects:libmercury1 ?
		-l :::lib:libmercury2:objects:libmercury2 ?
		-l :::lib:libmercury3:objects:libmercury3 ?
		-l :::lib:libmercury4:objects:libmercury4 ?
		-l :::lib:libmercury_utils:objects:libmercury_utils ?
		-lspmath ?
		-lc
		
LIBSETUP = :::lib:libmercury_setup:objects:

OBJECTS = ?
		:objects:game.c.o ?
		:objects:gamedata.c.o ?
		:objects:gamedata2.c.o ?
		:objects:mainloop.c.o ?
		:objects:icacheflush.s.o ?
		:objects:bsdf_read.c.o ?
		:objects:data.c.o ?
		:objects:filepod.c.o ?
		:objects:graphicsenv.c.o ?
		:objects:scalemodel.c.o ?
		:objects:tex_read.c.o ?
		?
		{LIBSETUP}M_SetupDynLit.c.o ?
		{LIBSETUP}M_SetupDynLitFog.c.o ?
		{LIBSETUP}M_SetupDynLitTex.c.o ?
		{LIBSETUP}M_SetupDynLitSpecTex.c.o ?
		{LIBSETUP}M_SetupDynLitFogTex.c.o ?
		{LIBSETUP}M_SetupDynLitAA.c.o ?
		{LIBSETUP}M_SetupDynLitTexAA.c.o ?
		{LIBSETUP}M_SetupPreLit.c.o ?
		{LIBSETUP}M_SetupPreLitFog.c.o ?
		{LIBSETUP}M_SetupPreLitTex.c.o ?
		{LIBSETUP}M_SetupPreLitFogTex.c.o ?
		{LIBSETUP}M_SetupDynLitTrans.c.o ?
		{LIBSETUP}M_SetupDynLitTransTex.c.o ?
		{LIBSETUP}M_SetupDynLitFogTrans.c.o ?
		{LIBSETUP}M_SetupDynLitEnv.c.o ?
		{LIBSETUP}M_SetupDynLitFogEnv.c.o ?
		{LIBSETUP}M_SetupDynLitTransEnv.c.o ?
		{LIBSETUP}M_SetupDynLitSpecEnv.c.o ?
		{LIBSETUP}M_SetupPreLitTrans.c.o ?
		{LIBSETUP}M_SetupPreLitTransTex.c.o ?
		{LIBSETUP}M_SetupPreLitFogTrans.c.o ?
		{LIBSETUP}M_SetupPreLitAA.c.o ?
		{LIBSETUP}M_SetupPreLitTexAA.c.o


# Choose one of the following:
DEBUG_OPTIONS = -g # For best source-level debugging
# DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5

# Choose one of the following:
USE_PPCAS = 1 # For faster compilation
# USE_PPCAS = 0


# variables for compiler tools
defines		=	-D__3DO__ -DOS_3DO=2 -DNUPUPSIM -DMACINTOSH

dccopts		=	 ?
				-c					# Produce a .o object file only (do not invoke Diab Linker)?
				-Xstring-align=1	# (Overrides the default string alignment of 4)?
				-Ximport			# Treat all #include as #import, i.e., include only once.?
				-Xstrict-ansi		# (Self-evident.  Won't allow // comments!)?
				-Xunsigned-char		# Treat the char type as unsigned.?
				{DEBUG_OPTIONS} ?
				-Xforce-prototypes	# Compiler will complain about functions without 'em?
				-Xlint=0x10			# All warnings except unused variable/function.  Add more bits (from 0xFE) for fewer warnings.?
				-Xtrace-table=0		# Don't generate traceback tables.  This is like macsbug names for you macheads

dplusopts		=	 ?
				{dccopts} ?
				-Xuse-.init=0 "-W:c++:.cp"

appname		= bigcircle

LIBS		= -l3do_scaf -lsubscriber -lds -ldsutils -lEZFlixDecoder -lmusic  ?
				-lclt -lspmath ?
				-leventbroker -lc ?
				{LIBMERCURY}

### SWITCH M2 VERSION {
### CASE 3.0:
VECTORDIR	= "{3dointerfaces}"vectors:default:
MODULEDIR	= "{VECTORDIR}"
BOOTDIR	= "{VECTORDIR}"
### CASE 2.0:
#MODULEDIR	= Moon:test3do:3DO_OS:M2_3.0b:Remote:System.m2:Modules:
#BOOTDIR	= Moon:test3do:3DO_OS:M2_3.0b:Remote:System.m2:Boot:
### } END SWITCH

MODULES		= "{MODULEDIR}"audio "{MODULEDIR}"font ?
				"{MODULEDIR}"graphics "{MODULEDIR}"gstate ?
				"{MODULEDIR}"compression ?
				"{MODULEDIR}"fsutils "{MODULEDIR}"icon "{MODULEDIR}"iff ?
				"{MODULEDIR}"international "{MODULEDIR}"jstring ?
				"{MODULEDIR}"script ?
				"{MODULEDIR}"audiopatchfile ?
				"{MODULEDIR}"audiopatch ?
				"{MODULEDIR}"frame2d ?
				"{BOOTDIR}"kernel "{BOOTDIR}"filesystem

INCLUDEDIR = :::include: -I::helloworld:

{appname} ?? {appname}.make {OBJECTS}
	Set Echo 0
	Echo "# ---- Linking {appname}"
	Set Echo 1
	link3do			# NOTE: Type Link3DO and hit enter for full list of options ?
		-r			# Generate relocations in file to keep file relative ?
		-D			# Allow duplicate symbols ?
		-Htime=now -Hsubsys=1 -Hname={appname} -Htype=5 -Hstack=32768 # 3DO Header Information  ?
		-o {appname} ?
		{OBJECTS} ?
		-L"{3dolibs}{m2librelease}" ?
		{MODULES}					 ?
		{LIBS}
	Set Echo 0
	Echo "# ---- Setting up {appname} in remote folder"
	Set Echo 1
	setfile {appname} -c '3DOD' -t 'PROJ'				# Set creator and type so icon will appear
	duplicate {appname} -y {3doremote}				# copy the goodies to /remote
	Set Echo 0
	Echo "# ---- Creating the debugging script"
	Set Echo 1
	#?? Make the .spt script, so that 3DODebug finds the source files.
	#?? 3DODebug executes the script <appname>.spt when you debug <appname>.
	#?? This default assumes all your source files are in the current directory at build time.
	echo "setsourcedir" > "{3doremote}{appname}.spt"					# Clear 3DODebug's current list.
	echo "setsourcedir `directory`" >> "{3doremote}{appname}.spt"	# Tell 3DODebug to look for source files in the current directory.
	#?? <<If you have more than one source directory, insert more lines here.>>
	echo "setdatadir" >> "{3doremote}{appname}.spt"						# Clear 3DODebug's current list of symbol files.
	echo "setdatadir ?"{3doremote}?"" >> "{3doremote}{appname}.spt"		# (Because currently the symbols are in the executable file).

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: ? : ::helloworld:

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o ? .c bigcircle.make
	Set Echo 0
	Echo "# ---- Compiling {depDir}{default}.c"
	if {USE_PPCAS}
		Set Echo 1
		dcc {depDir}{default}.c {defines} {dccopts} -I{INCLUDEDIR} -Xdebug-mode=6 -S -o "{TempFolder}"{default}.s
		ppcas "{TempFolder}"{default}.s -o {targDir}{default}.c.o
		delete -i "{TempFolder}"{default}.s
		Set Echo 0
	else
		Set Echo 1
		dcc {depDir}{default}.c {defines} {dccopts} -o {targDir}{default}.c.o
		Set Echo 0
	end if
		Set Echo 1

# .cp.o files are dependent only upon the .cp source (This is not very complete but good enough for now)
.cp.o	 ? .cp bigcircle.make
	Set Echo 0
	Echo "# ---- Compiling {depDir}{default}.cp"
	if {USE_PPCAS}
		Set Echo 1
		dplus {depDir}{default}.cp {defines} {dplusopts} -Xdebug-mode=6 -S -o "{TempFolder}"{default}.s
		ppcas "{TempFolder}"{default}.s -o {targDir}{default}.cp.o
		delete -i "{TempFolder}"{default}.s
		Set Echo 0
	else
		Set Echo 1
		dplus {depDir}{default}.cp {defines} {dplusopts} -o {targDir}{default}.cp.o
		Set Echo 0
	end if
		Set Echo 1

# .cpp.o files are dependent only upon the .cpp source (This is not very complete but good enough for now)
.cpp.o ? .cpp bigcircle.make
	Set Echo 0
	Echo "# ---- Compiling {depDir}{default}.cpp"
	if {USE_PPCAS}
		Set Echo 1
		dplus {depDir}{default}.cpp {defines} {dplusopts} -Xdebug-mode=6 -S -o "{TempFolder}"{default}.s
		ppcas "{TempFolder}"{default}.s -o {targDir}{default}.cpp.o
		delete -i "{TempFolder}"{default}.s
		Set Echo 0
	else
		Set Echo 1
		dplus {depDir}{default}.cpp {defines} {dplusopts} -o {targDir}{default}.cpp.o
		Set Echo 0
	end if
		Set Echo 1

# .s.o files are dependent only upon the .s source (This is not very complete but good enough for now)
.s.o ? .s bigcircle.make
	Set Echo 0
	Echo "# ---- Assembling {depDir}{default}.s"
	Set Echo 1
	ppcas  -I {INCLUDEDIR} {depDir}{default}.s -o {targDir}{default}.s.o

