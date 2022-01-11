#####################################
##
##      @(#) EZFlixChunkifier.xcoff.make 96/04/10 1.5
##
#####################################
#
#	File:		EZFlixChunkifier.xcoff.make
#
#	Contains:	xxx put contents here xxx
#
#	Written by:	Donn Denman and Greg Wallace
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f EZFlixChunkifier.xcoff.make ³³ "{worksheet}" > temp.makeout; temp.makeout ³³ "{worksheet}"; delete -i temp.makeout
#
#	To Do:
#
######################################
#
#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination = "{3DOMPWTools}"
#Destination    = :

EchoOn			=	set oldEcho {echo}; set echo 1
EchoOff			=	set echo {oldEcho}

ObjectDir			=	":PPCObjects:"

APPL            	= 	EZFlixChunkifier
LibsDir				=	:Libs:
IncludesDir			=	:Includes:
EZFlixEncoderLib	=	EZFlixEncoderLib.xcoff
CPLUS       		=	MrCpp
LINK				=	PPCLink
SETFILEOPTS			=	-t 'MPST' -c 'MPS '
MakeFileName		=	{APPL}.xcoff.make

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

DEBUGOPTIONS 	= -sym full


#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS				=	{ObjectDir}EZFlixChunkifier.c.o

OBJECTDEPENDS		=	EZFlixChunkifier.c.depends

LIBS			= 	"{SharedLibraries}"InterfaceLib			¶
					"{SharedLibraries}"MathLib 				¶
					"{PPCLibraries}"PPCCRuntime.o 			¶
					"{SharedLibraries}"StdCLib 				¶
					"{PPCLibraries}"MrCPlusLib.o 			¶
					"{SharedLibraries}"QuickTimeLib 		¶
					"{PPCLibraries}"PPCToolLibs.o 		¶
					"{PPCLibraries}"StdCRuntime.o 		¶
					"{LibsDir}"{EZFlixEncoderLib}
					
LIBRENAMES		=	-librename InterfaceLib.xcoff=InterfaceLib	¶
					-librename QuickTimeLib.xcoff=QuickTimeLib	¶
					-librename StdCLib.xcoff=StdCLib			¶
					-librename MathLib.xcoff=MathLib			¶
					-librename EZFlixEncoderLib.xcoff=EZFlixEncoderLib

CPlusOptions = -d makeformac -i "{CINCLUDES}" -i "{3DOINCLUDES}" -i "{3DOIncludes}streaming:" -i :Includes: -d NO_64_BIT_SCALARS -d EZFLIXCHUNKIFIER

#####################################
#	Target build rules
#####################################
{Destination}{APPL} Ä {MakeFileName} {OBJECTS} "{LibsDir}"{EZFlixEncoderLib} {APPL}.r
	if ({DEBUGFLAG})
		{EchoOn}
		{LINK} -d -c 'MPS ' -t MPST	¶
			{DEBUGOPTIONS}			¶
			{LIBRENAMES}			¶
			{OBJECTS}				¶
			{LIBS}					¶
			-o {Targ}
		{EchoOff}
	else
		{EchoOn}
		{LINK} -d -c 'MPS ' -t MPST	¶
			{LIBRENAMES}			¶
			{OBJECTS}				¶
			{LIBS}					¶
			-o {Targ}
		{EchoOff}
	end
	setfile {SETFILEOPTS} {Targ}
    Rez  -o {Targ} {APPL}.r -a -ov

#####################################
#	Default build rules
#####################################
All				Ä	{Destination}{APPL}

{ObjectDir}		Ä	:

# Target dependancy to rebuild when makefile or build script changes
.c.o			Ä .c {MakeFileName}
	if ({DEBUGFLAG})
		{EchoOn}
		{CPLUS} {CPlusOptions} {DEBUGOPTIONS} {COptions} -o {ObjectDir}{APPL}.c.o {APPL}.c
		{EchoOff}
	else
		{EchoOn}
		{CPLUS} {CPlusOptions} -o {ObjectDir}{APPL}.c.o {APPL}.c
		{EchoOff}
	end

.c.depends		Ä	.c
	set BaseDir "`directory`"
	set Default {Default}
	{CPLUS} {CPlusOptions} {DepDir}{Default}.c -e2 ³ Dev:Null > "{DepDir}c.includefiles"
 	search -i -q -ns "Includes:" "{DepDir}c.includefiles"	¶
		| StreamEdit -e "1,$ Replace /Å{MPW}(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{MPW¶}'¨1; ¶
		Replace /Å{3DOIncludes}(?)¨2/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{3DOIncludes¶}'¨2; ¶
		Replace /Å:includes:(?)¨3/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{IncludesDir¶}'¨3" ¶
		>>  "{MakeFileName}"
	delete -i "{DepDir}c.includefiles"

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
"{ObjectDir}"EZFlixChunkifier.c.o	Ä	"Version.h"
