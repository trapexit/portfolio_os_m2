#####################################
##
##      @(#) EZFlixEncoderLib.xcoff.make 96/03/06 1.2
##
#####################################
#
#	File:		EZFlixEncoderLib.xcoff.make
#
#	Contains:	xxx put contents here xxx
#
#	Written by:	Donn Denman and Greg Wallace
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f EZFlixEncoderLib.xcoff.make ³³ "{worksheet}" > temp.makeout; temp.makeout ³³ "{worksheet}"; delete -i temp.makeout
#
#	To Do:
#
######################################
#
ObjectDir			=	":PPCObjects:"

Library				=	EZFlixEncoderLib
LibsDir				=	::Libs:
IncludesDir			=	::Includes:
LINK				=	PPCLink
SETFILEOPTS			=	-t 'MPST' -c 'MPS '
MakeFileName		=	{Library}.xcoff.make

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

DEBUGOPTIONS 	= -sym full

EchoOn			=	set oldEcho {echo}; set echo 1
EchoOff			=	set echo {oldEcho}

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			=	{ObjectDir}EZFlixEncoder.c.o		¶
					{ObjectDir}EZFlixCompress.cp.o		¶
					{ObjectDir}Quantizers.cp.o

OBJECTDEPENDS	=	EZFlixEncoder.c.depends				¶
					EZFlixCompress.cp.depends			¶
					Quantizers.cp.depends

LIBS			= 	"{PPCLibraries}"InterfaceLib.xcoff	¶
					"{PPCLibraries}"MathLib.xcoff 		¶
					"{PPCLibraries}"PPCCRuntime.o 		¶
					"{PPCLibraries}"StdCLib.xcoff 		¶
					"{PPCLibraries}"CPlusLib.o 			¶
					"{PPCLibraries}"QuickTimeLib.xcoff 	¶
					"{PPCLibraries}"PPCToolLibs.o 		¶
					"{PPCLibraries}"StdCRuntime.o 		¶
					"{LibsDir}"{Library}.xcoff
					
LIBEQUATES		=	-l InterfaceLib.xcoff=InterfaceLib	¶
					-l StdCLib.xcoff=StdCLib			¶
					-l MathLib.xcoff=MathLib			¶
					-l {Library}.xcoff={Library}

CPlusOptions = -d makeformac -i "{CINCLUDES}" -i ::Includes:

LINKOPTS		=	-xm library

#####################################
#	Target build rules
#####################################
{LibsDir}{Library}.xcoff		Ä	{MakeFileName} {OBJECTS}
	if !`exists {LibsDir}`
		NewFolder {LibsDir}
	end
	if ({DEBUGFLAG})
		{EchoOn}
		{LINK} {LINKOPTS}	¶
			{DEBUGOPTIONS}		¶
			{OBJECTS}			¶
			-o {Targ}
		{EchoOff}
	else
		{EchoOn}
		{LINK} {LINKOPTS}	¶
			{OBJECTS}			¶
			-o {Targ}
		{EchoOff}
	end
	setfile {SETFILEOPTS} {Targ}

#####################################
#	Default build rules
#####################################
All				Ä	{LibsDir}{Library}.xcoff

{ObjectDir}		Ä	:

# Target dependancy to rebuild when makefile or build script changes
.c.o			Ä .c {MakeFileName}
	if ({DEBUGFLAG})
		{EchoOn}
		MrC {CPlusOptions} {DEBUGOPTIONS} {COptions} -o {TargDir}{default}.c.o {DepDir}{default}.c
		{EchoOff}
	else
		{EchoOn}
		MrC {CPlusOptions} -o {TargDir}{default}.c.o {DepDir}{default}.c
		{EchoOff}
	end

.cp.o			Ä .cp {MakeFileName}
	if ({DEBUGFLAG})
		{EchoOn}
		MrCpp {CPlusOptions} {DEBUGOPTIONS} {COptions} -o {TargDir}{default}.cp.o {DepDir}{default}.cp
		{EchoOff}
	else
		{EchoOn}
		MrCpp {CPlusOptions} -o {TargDir}{default}.cp.o {DepDir}{default}.cp
		{EchoOff}
	end

.c.depends		Ä	.c
	set BaseDir "`directory`"
	set Default {Default}
	ppcc {CPlusOptions} {DepDir}{Default}.c -e ³ Dev:Null > "{DepDir}c.includefiles"
 	search -i -q -ns "Includes:" "{DepDir}c.includefiles"	¶
		| StreamEdit -e "1,$ Replace /Å{MPW}(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{MPW¶}'¨1; ¶
		Replace /Å{3DOIncludes}(?)¨2/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{3DOIncludes¶}'¨2; ¶
		Replace /Å:includes:(?)¨3/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{IncludesDir¶}'¨3" ¶
		>>  "{MakeFileName}"
	delete -i "{DepDir}c.includefiles"

.cp.depends		Ä	.cp
	set BaseDir "`directory`"
	set Default {Default}
	ppcc {CPlusOptions} {DepDir}{Default}.cp -e2 ³ Dev:Null > "{DepDir}cp.includefiles"
 	search -i -q -ns "Includes:" "{DepDir}cp.includefiles"	¶
		| StreamEdit -e "1,$ Replace /Å{MPW}(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.cp.o"	Ä	¶"¶{MPW¶}'¨1; ¶
		Replace /Å{3DOIncludes}(?)¨2/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{3DOIncludes¶}'¨2; ¶
		Replace /Å:includes:(?)¨3/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	¶"¶{IncludesDir¶}'¨3" ¶
		>>  "{MakeFileName}"
	delete -i "{DepDir}cp.includefiles"


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

"{ObjectDir}"EZFlixEncoder.c.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:ConditionalMacros.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Memory.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:MixedMode.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Memory.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:StandardFile.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Dialogs.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Errors.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Dialogs.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Windows.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Quickdraw.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:QuickdrawText.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Quickdraw.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Windows.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Events.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:OSUtils.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Events.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Windows.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Controls.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Menus.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Controls.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Windows.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Dialogs.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:TextEdit.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Dialogs.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:StandardFile.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Files.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:StandardFile.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:lowmem.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Fonts.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:lowmem.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Resources.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:lowmem.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Math.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:stdio.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:string.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:strings.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:ctype.h"
"{ObjectDir}"EZFlixCompress.c.o	Ä	"{IncludesDir}EZFlixXPlat.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:Components.h"
"{ObjectDir}"EZFlixCompress.c.o	Ä	"{IncludesDir}EZFlixXPlat.h"
"{ObjectDir}"EZFlixCompress.c.o	Ä	"{IncludesDir}EZFlixEncoder.h"
"{ObjectDir}"EZFlixCompress.cp.o	Ä	"{MPW}Interfaces:CIncludes:stdlib.h"
"{ObjectDir}"Quantizers.cp.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"Quantizers.cp.o	Ä	"{MPW}Interfaces:CIncludes:ConditionalMacros.h"
"{ObjectDir}"Quantizers.cp.o	Ä	"{MPW}Interfaces:CIncludes:Types.h"
"{ObjectDir}"Quantizers.cp.o	Ä	"{MPW}Interfaces:CIncludes:stddef.h"
