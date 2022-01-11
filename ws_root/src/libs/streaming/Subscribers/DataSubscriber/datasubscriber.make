#####################################
#
#	@(#) datasubscriber.make 96/02/29 1.3
#   File:       datasubscriber.make
#
#   Contains:   makefile for building DataSubscriber
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f DataSubscriber.make ³³ "{worksheet}" > temp.makeout; temp.makeout ·· "{worksheet}"; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
FileName		=	DataSubscriber
DebugFlag		=	1
SubscriberDir	=	::
StreamDir		=	:::
CC              =	dcc
 
#RAMDISKDIR     =
#RAMDISKDIR     =	RAM Disk:
 
ObjectDir		=	{SubscriberDir}Objects:

MakeFileName	=	{FileName}.make

#####################################
# Trace switches
#
# The TRACE_DATA_SUBSCRIBER switch causes the subscriber to compile-in
# code that logs high-level events to a trace buffer. DATA_TRACE_LEVEL 
#  determines how much detail is in the log:
#    TRACE_LEVEL_1 gives you minimal trace info, 
#    TRACE_LEVEL_2 gives you more info (includes level 1 trace),
#    TRACE_LEVEL_3 gives you  maximal info (includes levels 1 and 2 trace).
#####################################

DataTraceSwitches  = ¶
					-DTRACE_DATA_SUBSCRIBER=1			# define to trace actions in ¶
					-DDATA_TRACE_LEVEL=3				# set to desired trace level

#####################################
#	Default compiler options
#####################################
CDependsOptions = -c -Ximport -Xstrict-ansi

COptions		=	{gCBaseOptions} ¶
					# {DataTraceSwitches}		# Turn this on for trace logging

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}datasubscriber.c.o"		¶
					"{ObjectDir}datachannels.c.o"	

OBJECTDEPENDS	=	"datasubscriber.c.depends"			¶
					"datasubscriber.c.depends"

#####################################
#	Default build rules
#####################################
All				Ä	{OBJECTS}

{ObjectDir}		Ä	:

{FileName}		Ä	{OBJECTS}

.c.o			Ä	.c {MakeFileName}
    if `Exists -d -q "{RAMDISKDIR}"`
        directory "{RAMDISKDIR}"
    end
#
	if "{gCBaseOptions}{gCOptimizeOptions}{gCSymOption}" == ""
		echo "	Error: {MakeFileName} requires args from ::BuildSubscriberLib"
		exit
	end
	if ( {DebugFlag} )
		echo "	compiling {Default}.c with {gCSymOption}"
		{CC} {COptions} {gCSymOption} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	else
		echo "	compiling {Default}.c with {gCOptimizeOptions}"
		{CC} {COptions} {gCOptimizeOptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	end

.c.depends		Ä	.c
	set BaseDir "`directory`"
	{CC} {CDependsOptions} {DepDir}{Default}.c -E -H > Dev:Null ³ "c.includefiles"
	search -i -q -r -ns "{3DOIncludes}" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{BaseDir}/ ':'" ¶
		>> "{MakeFileName}"
	search -i -q -ns "{3DOIncludes}Streaming:" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{3DOIncludes}Streaming:/ '¶"¶{3DOIncludes¶}¶"Streaming:'" ¶
		>> "{MakeFileName}"
	delete -i "c.includefiles"
  
#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the Äs, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "Depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					Ä	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	Ä
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(DonÕt change this line or put anything after this section.¶)°/ {MakeFileName}
	Find /¥[Â#]/ {MakeFileName}
	Replace Æ¤:° "¶n" {MakeFileName}

SaveNewMakefile			Ä
	Save "{MakeFileName}"
	
#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}".c.o	Ä	:datachannels.h
"{ObjectDir}".c.o	Ä	:datasubscriber.h
"{ObjectDir}".c.o	Ä	:datatracecodes.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:subscribertraceutils.h
"{ObjectDir}".c.o	Ä	"{3DOIncludes}"Streaming:subscriberutils.h
