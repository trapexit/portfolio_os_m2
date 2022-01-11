######################
# @(#) Spooling.make 96/08/20 1.11
######################
#   This Makefile modified from a generic CreateM2Make makefile for use with
#   SSJJ example code.

Objects		=	:objects:playsf.c.o					¶
				:objects:ta_spool.c.o				¶
				:objects:tsp_algorithmic.c.o		¶
				:objects:tsp_rooms.c.o				¶
				:objects:tsp_spoolsoundfile.c.o		¶
				:objects:tsp_switcher.c.o

FINALDIRECTORY = "{3doremote}"Examples:Audio:Spooling:

# Choose one of the following:
DEBUG_OPTIONS = -g # For best source-level debugging
# DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5

app_name = spooling


# variables for compiler tools
defines		=	-D__3DO__ -DOS_3DO=2 -DNUPUPSIM

dccopts		=	 ¶
				-c -Xstring-align=1 -Ximport -Xstrict-ansi -Xunsigned-char ¶
				{DEBUG_OPTIONS} ¶
				-Xforce-prototypes -Xlint=0x10 -Xtrace-table=0

dplusopts		=	 ¶
				{dccopts} ¶
				-Xuse-.init=0 "-W:c++:.cp"

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
				"{MODULEDIR}"audiopatch "{MODULEDIR}"audiopatchfile ¶
				"{MODULEDIR}"frame2d ¶
				"{BOOTDIR}"kernel "{BOOTDIR}"filesystem

{app_name} Ä {OBJECTS} {app_name}.make

.c.o		Ä	.c {app_name}.make
	dcc {depDir}{default}.c {defines} {dccopts} -o {targDir}{default}.c.o
	link3do -r -D -Htime=now -Hsubsys=1 -Hname={default} -Htype=5 -Hstack=32768 ¶
		-o {default} ¶
		:objects:{default}.c.o ¶
		-L"{3dolibs}{m2librelease}" ¶
		{MODULES}					#  ¶
		{LIBS}						#  ¶
		> {default}.map			# generate extra map info and redirect from dev:stdout to a map file
	setfile {default} -c '3DOD' -t 'PROJ'				# Set creator and type so icon will appear
	duplicate {default} -y "{FINALDIRECTORY}"			# copy the goodies to /remote
	#¥¥ Make the .spt script, so that 3DODebug finds the source files.
	#¥¥ 3DODebug executes the script <default>.spt when you debug <default>.
	#¥¥ This default assumes all your source files are in the current directory at build time.
	echo "setsourcedir" > "{FINALDIRECTORY}{default}.spt"					# Clear 3DODebug's current list.
	echo "setsourcedir ¶"`directory`¶"" >> "{FINALDIRECTORY}{default}.spt"	# Tell 3DODebug to look for source files in the current directory.
	#¥¥ <<If you have more than one source directory, insert more lines here.>>
	echo "setdatadir" >> "{FINALDIRECTORY}{default}.spt"						# Clear 3DODebug's current list of symbol files.
	echo "setdatadir ¶"{FINALDIRECTORY}¶"" >> "{FINALDIRECTORY}{default}.spt"		# (Because currently the symbols are in the executable file).

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: Ä :



