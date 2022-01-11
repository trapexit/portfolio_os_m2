#####################################
##
##	@(#) libDS.a.make 96/04/05 1.27
##
#####################################
#
#   File:       libds.a.make
#
#	Contains:	make file for building libds.a
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f libDS.a.make ³³ "{worksheet}" > temp.makeout; temp.makeout ·· "{worksheet}"; delete -i temp.makeout
#
#####################################

#####################################
#		Symbol definitions
#####################################
Destination		=	{3doLibs}{M2LibRelease}:
Library			=	libds
LibraryFileName	=	{Library}.a
DebugFlag		=	0
ObjectDir		=	:Objects:
StreamDir		=	:
#RAMDISKDIR		=
#RAMDISKDIR		= RAM Disk:
CC				=	dcc
ASM				=	das
LIBRARIAN		=	dar						# archive maintainer
LINK			=	link3do
MakeFileName	=	{LibraryFileName}.make
#####################################
#	Default compiler options
#####################################
CBaseOptions =	-c						# MANDATORY only run the c compiler, don't exec the linker ¶
				-Xstring-align=1		# RECOMMENDED n-byte align strings ¶
				-Ximport				# RECOMMENDED include headers only once ¶
				-Xstrict-ansi			# RECOMMENDED unless you are building code to be released, then it's MANDATORY ;-) ¶
				-Xunsigned-char			# RECOMMENDED treat plain char as unsigned char ¶
				-Xforce-prototypes		# RECOMMENDED warn if a function is called without a previous prototype declaration ¶
				-Xlint					# RECOMMENDED warn about questionable or non-portable code ¶
				-Xtrace-table=0			# Don't generate traceback tables (like macsbug names)

COptimizeOptions	= -XO				# => optimizations on heavy, -Xtest-at-both, -Xinline=40, ... ¶
					-Xunroll=1			# unroll small loops n times ¶
					-Xtest-at-bottom	# use 1 loop test at the bottom of a loop ¶
					-Xinline=5			# inline fcns with < n parse nodes ¶
					#-Xsize-opt			# OPTIONAL: optimize for space over time when there's a choice

CSymOption			= -g -DDEBUG		# Compiler options for symbolic debugging

LOptions			= -D -r

# Note: This creates library file from scratch. To replace files have
# been updated, use the -rcu option.  Using -rcu option may be slower
# than creating the library from scratch.
LibOptions		= -cq		# Quickly appends files and don't display
							# any message when a new archive name
							# is created.

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			= 	"{ObjectDir}DataStream.c.o"			¶
					"{ObjectDir}DataStreamLib.c.o"		¶
					"{ObjectDir}preparestream.c.o"		¶
					"{ObjectDir}DataAcq.c.o"

OBJECTDEPENDS	=	"DataStream.c.depends"		¶
					"DataStreamLib.c.depends"	¶
					"preparestream.c.o"			¶
					"DataAcq.c.depends"

#####################################
#	Default build rules
#####################################
All				Ä	{Library}.a

{ObjectDir}		Ä	:

# Target dependancy to rebuild when makefile or build script changes
.c.o			Ä	.c {MakeFileName}
	if `Exists -d -q "{RAMDISKDIR}"`
		directory "{RAMDISKDIR}"
	end
	if ( {DebugFlag} && ( ({Default} == DataStream) ||		¶
		                  ({Default} == DataStreamLib) ||	¶
	                      ({Default} == DataAcq) ) )
		echo "	compiling {Default}.c with {CSymOption}"
		{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o {CBaseOptions} {CSymOption}
	else
		echo "	compiling {Default}.c with {COptimizeOptions}"
		{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o {CBaseOptions} {COptimizeOptions}
	end

.c.depends		Ä	.c
	set BaseDir "`directory`"
	{CC} {CBaseOptions} {DepDir}{Default}.c -E -H > Dev:Null ³ "{DepDir}c.includefiles"
	search -i -q -r -ns "{3DOIncludes}" "{DepDir}c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{BaseDir}/ ':'" ¶
		>> "{MakeFileName}"
	search -i -q -ns "{3DOIncludes}Streaming:" "{DepDir}c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{3DOIncludes}Streaming:/ '¶"¶{3DOIncludes¶}¶"Streaming:'" ¶
		>> "{MakeFileName}"
	delete -i "{RAMDISKDIR}c.includefiles"

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

#####################################
#	Target build rules
#####################################
{LibraryFileName}					Ä "{Destination}"{LibraryFileName}
"{Destination}"{LibraryFileName}	Ä {MakeFileName} {OBJECTS}
	echo "	Lib-ing {Targ}"
	delete -i {Targ}
	{LIBRARIAN}	{LibOptions} {Targ} {OBJECTS}

#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"DataStream.c.o	Ä	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"DataStreamLib.c.o	Ä	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"DataStreamLib.c.o	Ä	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"DataStreamLib.c.o	Ä	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"DataStreamLib.c.o	Ä	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"DataStreamLib.c.o	Ä	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"DataStreamLib.c.o	Ä	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"DataAcq.c.o	Ä	"{3DOIncludes}"Streaming:dsblockfile.h

