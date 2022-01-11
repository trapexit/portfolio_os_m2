#####################################
##
##	@(#) MPEGVideoSubscriber.make 96/06/05 1.1
##
#####################################
#
#   File:       MPEGVideoSubscriber.make
#
#	Contains:	makefile for building the MPEGVideoSubscriber
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f MPEGVideoSubscriber.make ³³ "{worksheet}" > temp.makeout; temp.makeout ·· "{worksheet}"; delete -i temp.makeout
#
#####################################

#####################################
#		Symbol definitions
#####################################
FileName		=	MPEGVideoSubscriber
DebugFlag		=	0
CC              =	dcc

#RAMDISKDIR     =
#RAMDISKDIR     = RAM Disk:

SubscriberDir	=	"::"

ObjectDir		=	{SubscriberDir}Objects:

MakeFileName	=	"{FileName}.make"

#####################################
# Trace switches, to help with real time debugging.
#
# Each MPVD_TRACE_xxx switch causes one module of the subscriber to compile-in
# code that logs high-level events to a trace buffer. It's very helpful for
# debugging. If you turn any of these switches on, you must turn on
# MPVD_TRACE_MAIN, which allocates the trace buffer.
#
#####################################
 
MPEGVideoTraceSwitches  =	-DMPVD_TRACE_MAIN=1 -DMPVD_TRACE_CHANNELS=1  ¶
							-DMPVD_TRACE_SUPPORT=1

#####################################
#	Default compiler options
#####################################
CDependsOptions = -c -Ximport -Xstrict-ansi

COptions		= {gCBaseOptions} ¶
				# {MPEGVideoTraceSwitches}		# Turn this on for trace logging

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}MPEGVideoSubscriber.c.o"		¶
					"{ObjectDir}MPEGVideoChannels.c.o"			¶
					"{ObjectDir}MPEGVideoSupport.c.o"			¶
					"{ObjectDir}MPEGBufferQueues.c.o"			¶
					"{ObjectDir}FMVDriverInterface.c.o"

OBJECTDEPENDS	=	"MPEGVideoSubscriber.c.depends"	¶
					"MPEGVideoChannels.c.depends"	¶
					"MPEGVideoSupport.c.depends"		¶
					"MPEGBufferQueues.c.depends"		¶
					"FMVDriverInterface.c.depends"

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
	if ( {DebugFlag} && ( ( {Default} == MPEGVideoSubscriber) ||	¶
		( {Default} == MPEGVideoChannels ) ) )
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
#	HOW TO USE IT: Get write access to this make file then make "Depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					Ä	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	Ä
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(DonÕt change this line or put anything after this section.¶)°/ "{MakeFileName}"
	Find /¥[Â#]/ "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"
	
#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	:mpegvideochannels.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	:mpegvideosupport.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:threadhelper.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:mpegvideosubscriber.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	:fmvdriverinterface.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:subscribertraceutils.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:subscriberutils.h
"{ObjectDir}"MPEGVideoSubscriber.c.o	Ä	"{3DOIncludes}"streaming:mpegbufferqueues.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	:mpegvideochannels.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	:mpegvideosupport.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:mpegvideosubscriber.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	:fmvdriverinterface.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:subscriberutils.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"MPEGVideoChannels.c.o	Ä	"{3DOIncludes}"streaming:mpegbufferqueues.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	:mpegvideochannels.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	:mpegvideosupport.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:threadhelper.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:mpegvideosubscriber.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	:fmvdriverinterface.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:subscriberutils.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:mpegbufferqueues.h
"{ObjectDir}"MPEGVideoSupport.c.o	Ä	"{3DOIncludes}"streaming:subscribertraceutils.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:threadhelper.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:mpegbufferqueues.h
"{ObjectDir}"MPEGBufferQueues.c.o	Ä	"{3DOIncludes}"streaming:subscribertraceutils.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:datastream.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:msgutils.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:mempool.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:dsstreamdefs.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:dserror.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:datastreamlib.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	"{3DOIncludes}"streaming:threadhelper.h
"{ObjectDir}"FMVDriverInterface.c.o	Ä	:fmvdriverinterface.h
