#####################################
##
##	@(#) EZFlixSubscriber.make 95/12/21 1.5
##
#####################################
#
#   File:       EZFlixSubscriber.make
#
#	Contains:	makefile for building the EZFlixSubscriber
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f EZFlixSubscriber.make �� "{worksheet}" > temp.makeout; temp.makeout �� "{worksheet}"; delete -i temp.makeout
#
#####################################

#####################################
#		Symbol definitions
#####################################
FileName		=	EZFlixSubscriber
DebugFlag		=	0
SubscriberDir	=	::
StreamDir		=	:::
CC				=	dcc
#RAMDISKDIR     =
#RAMDISKDIR     = RAM Disk:

ObjectDir		=	{SubscriberDir}Objects:

MakeFileName	=	{FileName}.make

#####################################
#	Default compiler options
#####################################
CDependsOptions = -c -Ximport -Xstrict-ansi

COptions		=	{gCBaseOptions}

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			=	"{ObjectDir}EZFlixSubscriber.c.o"
					
OBJECTDEPENDS	=	"EZFlixSubscriber.c.depends"

#####################################
#	Default build rules
#####################################
All				�	{OBJECTS}

{ObjectDir}		�	:

{FileName}		�	{OBJECTS}

# Target dependancy to rebuild when makefile or build script changes
.c.o			�	.c {MakeFileName}
    set BaseDir "`directory`"
    if `Exists -d -q "{RAMDISKDIR}"`
        directory "{RAMDISKDIR}"
    end
#
	if "{gCBaseOptions}{gCOptimizeOptions}{gCSymOption}" == ""
		echo "	Error: {MakeFileName} requires args from ::BuildSubscriberLib"
		exit
	end
	if ({DebugFlag})
		echo "	compiling {Default}.c with {gCSymOption}"
		{CC} {COptions} {gCSymOption} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	else
		echo "	compiling {Default}.c with {gCOptimizeOptions}"
		{CC} {COptions} {gCOptimizeOptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	end

.c.depends		�	.c
	set BaseDir "`directory`"
	{CC} {CDependsOptions} {DepDir}{Default}.c -E -H > Dev:Null � "c.includefiles"
	search -i -q -r -ns "{3DOIncludes}" "c.includefiles" �
		| StreamEdit -e "1,$ Replace /�(?)�1/ '�"�{ObjectDir�}�""{Default}.c.o"	�	'�1; Replace /{BaseDir}/ ':'" �
		>> "{MakeFileName}"
	search -i -q -ns "{3DOIncludes}Streaming:" "c.includefiles" �
		| StreamEdit -e "1,$ Replace /�(?)�1/ '�"�{ObjectDir�}�""{Default}.c.o"	�	'�1; Replace /{3DOIncludes}Streaming:/ '�"�{3DOIncludes�}�"Streaming:'" �
		>> "{MakeFileName}"
	delete -i "c.includefiles"
 
#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the �s, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "Depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					�	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	�
	Open "{MakeFileName}"
	Find � "{MakeFileName}"
	Find /�#�tInclude file dependencies �(Don�t change this line or put anything after this section.�)�/ "{MakeFileName}"
	Find /�[�#]/ "{MakeFileName}"
	Replace Ƥ:� "�n" "{MakeFileName}"

SaveNewMakefile			�
	Save "{MakeFileName}"
	
#####################################
#	Include file dependencies (Don�t change this line or put anything after this section.)
#####################################

"{ObjectDir}"EZFlixSubscriber.c.o	�	:EZFlixDecoder.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"EZFlixSubscriber.c.o	�	"{3DOIncludes}"Streaming:ezflixsubscriber.h
