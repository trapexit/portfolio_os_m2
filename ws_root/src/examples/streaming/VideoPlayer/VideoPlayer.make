#####################################
##
##	@(#) VideoPlayer.make 96/08/30 1.31
##
#####################################
#
#   File:       VideoPlayer.make
#
#	Contains:	make file for building VideoPlayer   (plays MPEG video)
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f VideoPlayer.make  ³³ "{worksheet}" > temp.makeout; temp.makeout ·· "{worksheet}"; delete -i temp.makeout
#
#####################################

#####################################
#		Symbol definitions
#####################################
App				=	VideoPlayer
DebugFlag		=	0
ObjectDir		=	:Objects:
AppDir          =   {3DORemote}Examples:Streaming:{App}:
VectorDir		=	{3DOVectors}{M2LibRelease}:

CC				=	dcc
LINK			=	link3do
				
MakeFileName	=	{App}.make

#####################################
#	Default compiler options for Diab Data
#####################################
CBaseOptions =	-c						# MANDATORY only run the c compiler, don't exec the linker ¶
				-Xstring-align=1		# RECOMMENDED n-byte align strings ¶
				-Ximport				# RECOMMENDED include headers only once ¶
				-Xstrict-ansi			# RECOMMENDED unless you are building code to be released, then its MANDATORY ;-) ¶
				-Xunsigned-char			# RECOMMENDED treat plain char as unsigned char ¶
				-Xforce-prototypes		# RECOMMENDED warn if a function is called without a previous prototype declaration ¶
				-Xlint					# RECOMMENDED warn about questionable or non-portable code ¶
				-Xtrace-table=0			# Don't generate traceback tables (like macsbug names) ¶
				-DSTARTUP_WORKAROUND			# defined ==> avoid the startup flicker ¶
				# -DFIELD_TO_DISPLAY_FIRST=0	# defined ==> in 480-line mode, the source is interlaced, this field first

COptimizeOptions	= -XO				# => optimizations on heavy, -Xtest-at-both, -Xinline=40, ... ¶
					-Xunroll=1			# unroll small loops n times ¶
					-Xtest-at-bottom	# use 1 loop test at the bottom of a loop ¶
					-Xinline=5			# inline fcns with < n parse nodes ¶
					#-Xsize-opt			# OPTIONAL: optimize for space over time when there's a choice

CSymOption			= -g -DDEBUG		# Compiler options for symbolic debugging

LOptions			= -D -r

Link3DOOptions		= -Htime=now -Hsubsys=1 -Htype=5 -Hstack=4096 -Hname={App}

#####################################
#		Object files
# LIBS and OBJECTS are files to link in.
# LIBDEPENDS are library files worth having "make" dependencies on.
# OBJECTDEPENDS are files to create "make" dependency rules for.
#####################################
LIBDEPENDS		=	"{3DOLibs}{M2LibRelease}:libsubscriber.a"	¶
					"{3DOLibs}{M2LibRelease}:libds.a"			¶
					"{3DOLibs}{M2LibRelease}:libdsutils.a"		¶
					"{3DOLibs}{M2LibRelease}:libeventbroker.a"	¶
					"{3DOLibs}{M2LibRelease}:libmusic.a"		¶
					"{3DOLibs}{M2LibRelease}:libc.a"

LIBS			=	-L"{3dolibs}{M2LibRelease}"	# lib search path	¶
					-lsubscriber									¶
					-lds											¶
					-ldsutils										¶
					-leventbroker									¶
					-lmusic											¶
					-lc

DLLS			=   "{VectorDir}filesystem"			¶
					"{VectorDir}kernel"				¶
					"{VectorDir}audio"				¶
					"{VectorDir}mpegaudiodecoder"	¶
                    "{VectorDir}graphics"

OBJECTS			=	"{ObjectDir}{App}.c.o"					¶
					"{ObjectDir}PlayVideoStream.c.o"		¶
					"{ObjectDir}JoyPad.c.o"

OBJECTDEPENDS	=	"{App}.c.depends"			¶
					"PlayVideoStream.c.depends"	¶
					"JoyPad.c.depends"

#####################################
#	Default build rules
#####################################
All				Ä	{App}

{ObjectDir}		Ä	:

.c.o			Ä	.c {MakeFileName}
    if !`exists "{ObjectDir}"`
        newfolder "{ObjectDir}"
    end
	if ( {DebugFlag} )
		echo "	compiling {Default}.c with {CSymOption}"
		{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o {CBaseOptions} {CSymOption}
	else
		echo "	compiling {Default}.c with {COptimizeOptions}"
		{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o  {CBaseOptions} {COptimizeOptions}
	end

.c.depends		Ä	.c
	set BaseDir "`directory`"
	{CC} {CBaseOptions} {DepDir}{Default}.c -E -H > Dev:Null ³ "c.includefiles"
	# Extract the non-3DOIncludes lines and generate Make lines, using ':' instead of absolute pathname
	search -i -q -r -ns "{3DOIncludes}" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{BaseDir}/ ':'" ¶
		>> "{MakeFileName}"
	# Extract the {3DOIncludes}streaming: lines and generate Make lines, using "{3DOIncludes}streaming:" instead of absolute pathnames
	search -i -q -ns "{3DOIncludes}streaming:" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{3DOIncludes}streaming:/ '¶"¶{3DOIncludes¶}¶"streaming:'" ¶
		>> "{MakeFileName}"
	delete -i "c.includefiles"


#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the Äs, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					Ä	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	Ä
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(DonÕt change this line or put anything after this section.¶)°/ "{MakeFileName}"
	Find /¥[Â#]/  "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"

############################################
# This generates the .spt file for debugging
############################################
CreateScriptFile		=	¶
	 "setsourcedir "¶n¶
	 "setsourcedir ¶"`directory`¶""¶n¶
	 "setsourcedir {3DOFolder}Streaming:{3DORelease}:Libs:DataStream:"¶n¶
	 "setsourcedir {3DOFolder}Streaming:{3DORelease}:Libs:DSUtils:"¶n¶
	 "setsourcedir {3DOFolder}Streaming:{3DORelease}:Libs:Subscribers:SAudioSubscriber:"¶n¶
	 "setsourcedir {3DOFolder}Streaming:{3DORelease}:Libs:Subscribers:SubscriberUtilities:"¶n¶
	 "setsourcedir {3DOFolder}Streaming:{3DORelease}:Libs:Subscribers:MPEGVideoSubscriber:"¶n	

#####################################
#	Target build rules
#####################################
{App}			Ä	"{AppDir}"{App}
"{AppDir}"{App}	Ä	{App}.make {OBJECTS} {LIBDEPENDS}
	echo "	Linking {App}"
	{LINK} {Link3DOOptions} -o {Targ}		¶
		{OBJECTS}							¶
		{DLLS}								¶
		{LIBS}								¶
		{LOptions}							¶
		#-m2 > "{AppDir}"{App}.map	# Writing a map crashes!
	SetFile "{AppDir}"{App} -c '3DOD' -t 'PROJ'
	if !`Exists {Targ}.spt`
		echo {CreateScriptFile} > {Targ}.spt
	End
	echo " Built {App}"


#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}"VideoPlayer.c.o	Ä	:joypad.h
"{ObjectDir}"VideoPlayer.c.o	Ä	:playvideostream.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:mpegvideosubscriber.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:saudiosubscriber.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:mpegbufferqueues.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:preparestream.h
"{ObjectDir}"VideoPlayer.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	:playvideostream.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:saudiosubscriber.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:satemplatedefs.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:sacontrolmsgs.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:mpegvideosubscriber.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:mpegbufferqueues.h
"{ObjectDir}"PlayVideoStream.c.o	Ä	"{3DOIncludes}"streaming:preparestream.h
"{ObjectDir}"JoyPad.c.o	Ä	:joypad.h
