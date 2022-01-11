######################
# @(#) cltsurface.make 96/08/20 1.4
######################
######################
#   This Makefile modified from a generic CreateM2Make makefile for use with
#   SSJJ example code.

FINALDIRECTORY = "{3doremote}"Examples:Graphics:CLT:cltsurface:

OBJECTS = ¶
		:objects:clip3.c.o ¶
		:objects:cltsurface.c.o


# Choose one of the following:
DEBUG_OPTIONS = -g # For best source-level debugging
# DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5

# Choose one of the following:
USE_PPCAS = 1 # For faster compilation
# USE_PPCAS = 0



# variables for compiler tools
defines		=	-D__3DO__ -DOS_3DO=2 -DNUPUPSIM

dccopts		=	 ¶
				-c					# Produce a .o object file only (do not invoke Diab Linker)¶
				-Xstring-align=1	# (Overrides the default string alignment of 4)¶
				-Ximport			# Treat all #include as #import, i.e., include only once.¶
				-Xstrict-ansi		# (Self-evident.  Won't allow // comments!)¶
				-Xunsigned-char		# Treat the char type as unsigned.¶
				{DEBUG_OPTIONS} ¶
				-Xforce-prototypes	# Compiler will complain about functions without 'em¶
				-Xlint=0x10			# All warnings except unused variable/function.  Add more bits (from 0xFE) for fewer warnings.¶
				-Xtrace-table=0		# Don't generate traceback tables.  This is like macsbug names for you macheads

dplusopts		=	 ¶
				{dccopts} ¶
				-Xuse-.init=0 "-W:c++:.cp"

appname		= cltsurface

LIBS		= -l3do_scaf -lsubscriber -lds -ldsutils -lEZFlixDecoder -lmusic  ¶
				-lclt -lspmath ¶
				-leventbroker -lc

### SWITCH M2 VERSION {
### CASE 3.0:
VECTORDIR	= "{3dointerfaces}"vectors:default:
MODULEDIR	= "{VECTORDIR}"
BOOTDIR	= "{VECTORDIR}"
### CASE 2.0:
#MODULEDIR	= TheBreedingPit:3DO:3DO_OS:M2_3.0a:Remote:System.m2:Modules:
#BOOTDIR	= TheBreedingPit:3DO:3DO_OS:M2_3.0a:Remote:System.m2:Boot:
### } END SWITCH

MODULES		= "{MODULEDIR}"audio "{MODULEDIR}"font ¶
				"{MODULEDIR}"graphics "{MODULEDIR}"gstate ¶
				"{MODULEDIR}"compression ¶
				"{MODULEDIR}"fsutils "{MODULEDIR}"icon "{MODULEDIR}"iff ¶
				"{MODULEDIR}"international "{MODULEDIR}"jstring ¶
				"{MODULEDIR}"script ¶
				"{MODULEDIR}"audiopatchfile ¶
				"{MODULEDIR}"audiopatch ¶
				"{MODULEDIR}"frame2d ¶
				"{BOOTDIR}"kernel "{BOOTDIR}"filesystem

{appname} ÄÄ {appname}.make {OBJECTS}
	Set Echo 0
	Echo "# ---- Linking {appname}"
	Set Echo 1
	link3do			# NOTE: Type Link3DO and hit enter for full list of options ¶
		-r			# Generate relocations in file to keep file relative ¶
		-D			# Allow duplicate symbols ¶
		-Htime=now -Hsubsys=1 -Hname={appname} -Htype=5 -Hstack=32768 # 3DO Header Information  ¶
		-o {appname} ¶
		{OBJECTS} ¶
		-L"{3dolibs}{m2librelease}" ¶
		{MODULES}					 ¶
		{LIBS}
	Set Echo 0
	Echo "# ---- Setting up {appname} in remote folder"
	Set Echo 1
	setfile {appname} -c '3DOD' -t 'PROJ'				# Set creator and type so icon will appear
    duplicate {appname} -y "{FINALDIRECTORY}"			# copy the goodies to /remote
	#¥¥ Make the .spt script, so that 3DODebug finds the source files.
	#¥¥ 3DODebug executes the script <appname>.spt when you debug <appname>.
	#¥¥ This default assumes all your source files are in the current directory at build time.
	echo "setsourcedir" > "{FINALDIRECTORY}{appname}.spt"					# Clear 3DODebug's current list.
	echo "setsourcedir ¶"`directory`¶"" >> "{FINALDIRECTORY}{appname}.spt"	# Tell 3DODebug to look for source files in the current directory.
	#¥¥ <<If you have more than one source directory, insert more lines here.>>
	echo "setdatadir" >> "{FINALDIRECTORY}{appname}.spt"						# Clear 3DODebug's current list of symbol files.
	echo "setdatadir ¶"{FINALDIRECTORY}¶"" >> "{FINALDIRECTORY}{appname}.spt"		# (Because currently the symbols are in the executable file).

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: Ä :

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o Ä .c cltsurface.make
	Set Echo 0
	Echo "# ---- Compiling {depDir}{default}.c"
	if {USE_PPCAS}
		Set Echo 1
		dcc {depDir}{default}.c {defines} {dccopts} -Xdebug-mode=6 -S -o "{TempFolder}"{default}.s
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
.cp.o	 Ä .cp cltsurface.make
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
.cpp.o Ä .cpp cltsurface.make
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
.s.o Ä .s cltsurface.make
	Set Echo 0
	Echo "# ---- Assembling {depDir}{default}.s"
	Set Echo 1
	ppcas  {defines} {depDir}{default}.s -o {targDir}{default}.s.o
