#####################################
##
##  @(#) SAudioSubscriber.make 96/06/05 1.1
##
#####################################
#
#   File:       SAudioSubscriber.make
#
#   Contains:   makefile for building SAudioSubscriber
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f SAudioSubscriber.make �� "{worksheet}" > temp.makeout; temp.makeout �� "{worksheet}"; delete -i temp.makeout
#
##########################################################################

#####################################
#		Symbol definitions
#####################################
FileName		=	SAudioSubscriber
DebugFlag		=	0
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
# Each SAUDIO_TRACE_XXX switch causes one module of the subscriber to compile-in
# code that logs high-level events to a trace buffer. It's very helpful for
# debugging. If you turn any of these switches on, you must turn on
# SAUDIO_TRACE_MAIN, which allocates the trace buffer.
#
#####################################

AudioTraceSwitches  = -DSAUDIO_TRACE_MAIN=1 -DSAUDIO_TRACE_BUFFERS=1 �
					-DSAUDIO_TRACE_CHANNELS=1 -DSAUDIO_TRACE_TEMPLATES=1  -DSAUDIO_TRACE_SUPPORT=1

#####################################
#	Default compiler options
#####################################
CDependsOptions = -c -Ximport -Xstrict-ansi

COptions		=	{gCBaseOptions} �
					#{AudioTraceSwitches}		# Turn this on for trace logging

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}samain.c.o"						�
					"{ObjectDir}sachannel.c.o"					�
					"{ObjectDir}sasoundspoolerinterface.c.o"	�
					"{ObjectDir}satemplates.c.o"				�
					"{ObjectDir}mpabufferqueue.c.o"				�
					"{ObjectDir}mpadecoderinterface.c.o"		�
					"{ObjectDir}mpegaudiosubscriber.c.o"		�
					"{ObjectDir}sasupport.c.o"

OBJECTDEPENDS	=	"samain.c.depends"					�
					"sachannel.c.depends"				�
					"sasoundspoolerinterface.c.depends"	�
					"satemplates.c.depends"				�
					"sasupport.c.depends"				�
					"mpabufferqueue.c.depends"			�
					"mpadecoderinterface.c.depends"		�
					"mpegaudiosubscriber.c.depends"

#####################################
#	Default build rules
#####################################
All				�	{OBJECTS}

{ObjectDir}		�	:

{FileName}		�	{OBJECTS}

.c.o			�	.c {MakeFileName}
    if `Exists -d -q "{RAMDISKDIR}"`
        directory "{RAMDISKDIR}"
    end
#
	if "{gCBaseOptions}{gCOptimizeOptions}{gCSymOption}" == ""
		echo "	Error: {MakeFileName} requires args from ::BuildSubscriberLib"
		exit
	end
	if ( {DebugFlag} && ( ({Default} == samain) || ({Default} == sachannel) ||	�
		 ({Default} == sasupport) || ({Default} == sasoundspoolerinterface) ||	�
		 ({Default} == satemplates) || ({Default} == mpabufferqueue)		||	�
		 ({Default} == mpadecoderinterface) || ({Default} == mpegaudiosubscriber) ) )
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

"{ObjectDir}"samain.c.o	�	:sachannel.h
"{ObjectDir}"samain.c.o	�	:sasupport.h
"{ObjectDir}"samain.c.o	�	:sasoundspoolerinterface.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:saudiosubscriber.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:sacontrolmsgs.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:subscribertraceutils.h
"{ObjectDir}"samain.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
"{ObjectDir}"sachannel.c.o	�	:sachannel.h
"{ObjectDir}"sachannel.c.o	�	:sasoundspoolerinterface.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:mpegaudiosubscriber.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:saudiosubscriber.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"sachannel.c.o	�	"{3DOIncludes}"Streaming:subscribertraceutils.h
"{ObjectDir}"satemplates.c.o	�	:sachannel.h
"{ObjectDir}"satemplates.c.o	�	:sasupport.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:saudiosubscriber.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"satemplates.c.o	�	"{3DOIncludes}"Streaming:subscribertraceutils.h
"{ObjectDir}"mpabufferqueue.c.o	�	:mpabufferqueue.h
"{ObjectDir}"mpabufferqueue.c.o	�	:mpadecoderinterface.h
"{ObjectDir}"mpabufferqueue.c.o	�	:sachannel.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"mpabufferqueue.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
"{ObjectDir}"mpadecoderinterface.c.o	�	:sachannel.h
"{ObjectDir}"mpadecoderinterface.c.o	�	:mpabufferqueue.h
"{ObjectDir}"mpadecoderinterface.c.o	�	:mpadecoderinterface.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"mpadecoderinterface.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	:mpadecoderinterface.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	:sachannel.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	:sasupport.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	:sasoundspoolerinterface.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:mpegaudiosubscriber.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:subscribertraceutils.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"mpegaudiosubscriber.c.o	�	"{3DOIncludes}"Streaming:saudiosubscriber.h
"{ObjectDir}"sasupport.c.o	�	:sachannel.h
"{ObjectDir}"sasupport.c.o	�	:sasupport.h
"{ObjectDir}"sasupport.c.o	�	:sasoundspoolerinterface.h
"{ObjectDir}"sasupport.c.o	�	:mpadecoderinterface.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:datastreamlib.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:datastream.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:msgutils.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:mempool.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:dsstreamdefs.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:dserror.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:threadhelper.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:saudiosubscriber.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:sacontrolmsgs.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:subscriberutils.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:subscribertraceutils.h
"{ObjectDir}"sasupport.c.o	�	"{3DOIncludes}"Streaming:satemplatedefs.h
