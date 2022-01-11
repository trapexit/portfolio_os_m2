#####################################
##
##	@(#) weaver.make 95/11/22 1.15
##
#####################################
#
#
#	File:		Weaver.make
#
#	Contains:	make rules to build the Weaver stream data merging tool
#	Note:		The Weaver is set up to be built with either THINK C or with
#				MPW. When built as an MPW tool, the file WeaverMain.c is not
#				used as it contains glue code to enable the input of command
#				line arguments when running under THINK C. The only purpose
#				for using THINK C is for its symbolic debugger. The released
#				version of the tool is always expected to be an MPW tool.
#
#	Written by:		Joe Buczek
#
#####################################
#
#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination		= {3DOMPWTools}
#Destination		= :

APPL			= Weaver
OBJECTDIR		= :objects:
OBJECTS 		= "{OBJECTDIR}{APPL}".c.o "{OBJECTDIR}"WeaveStream.c.o "{OBJECTDIR}"ParseScriptFile.c.o

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

DEBUGOPTIONS   = -sym on

# The following defines the various compile switches
#
# FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY compile switch determines whether or
# not the beginning of the stream data will be forced onto a stream block boundary.
# Setting this switch results in wasting space in the stream file, but may be useful
# if the application does a considerable amount of stream looping, whereby the header
# and marker table data, if present, could cause performance problems. It is
# recommended that this be left OFF.
FORCE_DATA_ON_BLOCK_BOUNDARY_FLAG	= 0 

# USE_BIGGER_BUFFERS compile switch determines whether or not larger I/O buffers are used
# for stream I/O. This SIGNIFICANTLY improves performance, and makes an ANSI compliant
# library call to set the buffer size. It is recommended that this option be left ON.
USE_BIGGER_BUFFERS_FLAG				= 1
 
# CACHE_OPEN_FILES compile switch removes limit on number of input streams.
CACHE_OPEN_FILES_FLAG				= 1

SWITCHOPTIONS	=	-d FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY={FORCE_DATA_ON_BLOCK_BOUNDARY_FLAG} ¶
					-d USE_BIGGER_BUFFERS={USE_BIGGER_BUFFERS_FLAG} ¶
					-d CACHE_OPEN_FILES={CACHE_OPEN_FILES_FLAG}

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COPTIONS		= -mc68020 {SWITCHOPTIONS} -r -i "{CINCLUDES}" -i "{3DOINCLUDES}" -d NO_64_BIT_SCALARS

LIBS            =   "{CLibraries}StdCLib.o"		¶
                    "{Libraries}Interface.o"	¶
                    "{Libraries}ToolLibs.o"		¶
                    "{Libraries}MacRuntime.o"	¶
                    "{Libraries}"IntEnv.o
 

#####################################
#   Default build rules
#####################################
{OBJECTDIR} Ä   :

.c.o        Ä   .c .h {APPL}.make
	if ({DEBUGFLAG})
     	SC {DEBUGOPTIONS} {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	else
     	SC {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	end

{APPL}					Ä "{Destination}"{APPL}
"{Destination}"{APPL}	ÄÄ {APPL}.make {OBJECTS}
	if ({DEBUGFLAG})
		Link -d -c 'MPS ' -t MPST		¶
			{DEBUGOPTIONS} {OBJECTS}	¶
			{LIBS}	-o {Targ}
	else
		Link -d -c 'MPS ' -t MPST		¶
			{OBJECTS} {LIBS}			¶
			-o {Targ}
	end

"{Destination}"{APPL} ÄÄ {APPL}.r
	Rez  -o {Targ} {APPL}.r -a -ov

#####################################
#   Include file dependencies
#####################################
"{OBJECTDIR}{APPL}".c.o		Ä {APPL}.h
"{OBJECTDIR}{APPL}".c.o		Ä weavestream.h ParseScriptFile.h
"{OBJECTDIR}{APPL}".c.o		Ä "{3DOINCLUDES}"streaming:satemplatedefs.h
"{OBJECTDIR}{APPL}".c.o		Ä "{3DOINCLUDES}"streaming:dsstreamdefs.h
"{OBJECTDIR}"WeaveStream.c.o Ä weavestream.h
"{OBJECTDIR}"WeaveStream.c.o Ä "{3DOINCLUDES}"streaming:dsstreamdefs.h
