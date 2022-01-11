######################
# @(#) RenderOrder.make 96/08/20 1.7
######################
#   This Makefile modified from a generic CreateM2Make makefile for use with
#   SSJJ example code.

OBJECTS = ¶
		:objects:renderorder.c.o


FINALDIRECTORY = "{3doremote}"Examples:Graphics:Frame2D:renderorder:

# Choose one of the following:
DEBUG_OPTIONS = -g # For best source-level debugging
# DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5



# variables for compiler tools
defines		=	-D__3DO__ -DOS_3DO=2 -DNUPUPSIM

dccopts		=	 ¶
				-c -Xstring-align=1 -Ximport -Xstrict-ansi -Xunsigned-char ¶
				{DEBUG_OPTIONS} ¶
				-Xforce-prototypes -Xlint=0x10 -Xtrace-table=0

dplusopts		=	 ¶
				{dccopts} ¶
				-Xuse-.init=0 "-W:c++:.cp"

appname		= renderorder


LIBS		= -l3do_scaf -lsubscriber -lds -ldsutils -lEZFlixDecoder -lmusic  ¶
				-lclt -lspmath ¶
				-leventbroker -lc
VECTORDIR	= "{3dointerfaces}"vectors:default:
MODULEDIR	= "{VECTORDIR}"
BOOTDIR	= "{VECTORDIR}"

MODULES		= "{MODULEDIR}"audio "{MODULEDIR}"font ¶
				"{MODULEDIR}"graphics "{MODULEDIR}"gstate ¶
				"{MODULEDIR}"compression ¶
				"{MODULEDIR}"fsutils "{MODULEDIR}"icon "{MODULEDIR}"iff ¶
				"{MODULEDIR}"international "{MODULEDIR}"jstring ¶
				"{MODULEDIR}"script ¶
				"{MODULEDIR}"audiopatch ¶
				"{MODULEDIR}"frame2d ¶
				"{BOOTDIR}"kernel "{BOOTDIR}"filesystem

{appname} ÄÄ {appname}.make {OBJECTS}
	link3do -r -D -Htime=now -Hsubsys=1 -Hname={appname} -Htype=5 -Hstack=32768 ¶
		-o {appname} ¶
		{OBJECTS} ¶
		-L"{3dolibs}{m2librelease}" ¶
		{MODULES}					#  ¶
		{LIBS}						#  ¶
		> {appname}.map			# generate extra map info and redirect from dev:stdout to a map file
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

# .c.o files are dependent on the .c source and the makefile
.c.o		Ä	.c {appname}.make
	dcc {depDir}{default}.c {defines} {dccopts} -o {targDir}{default}.c.o

# .cp.o files are dependent on the .cp source and the makefile
.cp.o		Ä	.cp {appname}.make
	dplus {depDir}{default}.cp {defines} {dplusopts} -o {targDir}{default}.cp.o

# .cpp.o files are dependent on the .cpp source and the makefile
.cpp.o		Ä	.cpp {appname}.make
	dplus {depDir}{default}.cpp {defines} {dplusopts} -o {targDir}{default}.cpp.o
	
	

