#####################################
##
##      @(#) EZFlixChunkifier.68K.make 96/03/15 1.9
##
#####################################
#
#	File:		EZFlixChunkifier.68K.make
#
#	Contains:	xxx put contents here xxx
#
#	Written by:	Donn Denman and Greg Wallace
#
#	To Do:
#
#   To regenerate the .c.o -> .h file dependencies, get write access to this
#   make file and execute the following MPW code:
#	make Depends -f EZFlixChunkifier.68K.make ³³ "{worksheet}" > temp.makeout; temp.makeout ³³ "{worksheet}"; delete -i temp.makeout;
#
######################################
#
#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination = "{3DOMPWTools}"
#Destination    = :

ObjectDir			=	":Objects:"

APPL            	= 	EZFlixChunkifier
CC					=	SC
LibsDir				=	:Libs:
IncludesDir			=	:Includes:
EZFlixEncoderLib	=	EZFlixEncoder.Lib
MakeFileName		=	{APPL}.68K.make

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

DEBUGOPTIONS 	= -sym full

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COptions = -d EZFLIXCHUNKIFIER -d makeformac  -d NO_64_BIT_SCALARS	¶
-mc68020 -mc68881 -r -i :EZFlixEncoder: -i "{CINCLUDES}" -i "{3DOINCLUDES}" -i "{IncludesDir}"

MathLibs = "{Libraries}"MathLib.o

LIBS			= 	"{Libraries}"MacRuntime.o 	¶
					"{Libraries}"Interface.o 	¶
					"{CLibraries}"StdClib.o 	¶
					"{Libraries}"Stubs.o 		¶
					"{Libraries}"IntEnv.o 		¶
					"{Libraries}"ToolLibs.o 	¶
					{MathLibs}					¶
					"{LibsDir}"{EZFlixEncoderLib}

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			=									¶
			{ObjectDir}EZFlixChunkifier.c.o

OBJECTDEPENDS	=									¶
			{ObjectDir}EZFlixChunkifier.c.depends


#####################################
#	Target build rules
#####################################
{Destination}{APPL} Ä {MakeFileName} {OBJECTS} "{LibsDir}"{EZFlixEncoderLib} {APPL}.r
	if ({DEBUGFLAG})
		Link -d -c 'MPS ' -t MPST	¶
			{DEBUGOPTIONS}			¶
			{OBJECTS}				¶
			{LIBS}					¶
			-o {Targ}
	else
		Link -d -c 'MPS ' -t MPST	¶
			{OBJECTS}				¶
			{LIBS}					¶
			-o {Targ}
	end
    Rez  -o {Targ} {APPL}.r -a -ov

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
	# This is a workaround to make it work with the latest version of Make Tool.
	# Without the next line, find /.../ will break (MakeFileName) isn't resolved.
	set MakeFileName "{MakeFileName}"
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(DonÕt change this line or put anything after this section.¶)°/ "{MakeFileName}"
	Find /¥[Â#]/  "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"
	
#####################################
#	Default build rules
#####################################
All				Ä	{Destination}{APPL}

{ObjectDir}		Ä	:

.c.o			Ä	.c {MakeFileName}
    if ({DEBUGFLAG})
		{CC} {DebugOption} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	else
		{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	end

#####################################
# PowerPC C/C++ Compiler is used to generate the dependencies
#####################################
.c.depends		Ä	.c
	set BaseDir "`directory`"
	set Default {Default}
	ppcc {COptions} {DepDir}{Default}.c -e2 ³ Dev:Null > "{DepDir}c.includefiles"
 	search -i -q -ns "Includes:" "{DepDir}c.includefiles"	¶
		| StreamEdit -e "1,$ Replace /Å{MPW}(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{MPW¶}'¨1; ¶
		Replace /Å{3DOIncludes}(?)¨2/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{3DOIncludes¶}'¨2; ¶
		Replace /Å:includes:(?)¨3/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{IncludesDir¶}'¨3" ¶
		>>  "{MakeFileName}"
	delete -i "{DepDir}c.includefiles"


#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ConditionalMacros.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ctype.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:fcntl.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Files.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:MixedMode.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Files.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:OSUtils.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Memory.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:OSUtils.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Files.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:fcntl.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Aliases.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:AppleTalk.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Aliases.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:fcntl.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:string.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:stdio.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:stddef.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:time.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ErrMgr.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:CursorCtl.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Strings.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Errors.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:FixMath.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Packages.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:QuickDraw.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:QuickdrawText.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:QuickDraw.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Movies.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Events.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Movies.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Menus.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Movies.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Components.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Movies.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ImageCompression.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:QDOffscreen.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ImageCompression.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Windows.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Controls.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Windows.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ImageCompression.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:StandardFile.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Dialogs.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:TextEdit.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Dialogs.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:StandardFile.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:ImageCompression.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Movies.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:GestaltEqu.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:Gestalt.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:GestaltEqu.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:StdLib.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{MPW}Interfaces:CIncludes:fenv.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{IncludesDir}EZFlixStream.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{3DOIncludes}streaming:dsstreamdefs.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{3DOIncludes}kernel:types.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{3DOIncludes}streaming:dsstreamdefs.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{3DOIncludes}misc:mpeg.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{3DOIncludes}streaming:dsstreamdefs.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{IncludesDir}EZFlixStream.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{IncludesDir}EZFlixXPlat.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{IncludesDir}EZFlixStream.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{IncludesDir}EZFlixEncoder.h"
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"{IncludesDir}EZFlixDefines.h"
