#####################################
##
##	Copyright (c) 1993-1996, an unpublished work by The 3DO Company.
##	All rights reserved. This material contains confidential
##	information that is the property of The 3DO Company. Any
##	unauthorized duplication, disclosure or use is prohibited.
##	
##  @(#) MPEGAudioChunkifier.make 96/03/22 1.2
##
#####################################
#
#   File:       MPEGAudioChunkifier.make
#   Target:     MPEGAudioChunkifier
#   Sources:    MPEGAudioChunkifier.c
#
#####################################

APPL			= MPEGAudioChunkifier

OBJECTDIR		= 
OBJECTS			= {OBJECTDIR}{APPL}.c.o

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COptions		= -r -i "{CINCLUDES}" -i "{3DOINCLUDES}" -d NO_64_BIT_SCALARS

DEBUGOPTIONS	= -sym on 

#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination		= "{3DOMPWTools}"
#Destination		= :


# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

LIBS            =   "{CLibraries}StdCLib.o"		¶
                    "{Libraries}Interface.o"	¶
#                    "{Libraries}ToolLibs.o"		¶
                    "{Libraries}MacRuntime.o"	¶
                    "{Libraries}"IntEnv.o		¶
					"{FLIXENCODERLIB}"

#####################################
#   Default build rules
#####################################
.c.o        Ä   .c {APPL}.make
	if ({DEBUGFLAG})
		SC {DEBUGOPTIONS} {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	else
		SC {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	end

{APPL} Ä {APPL}.make {OBJECTS}
	if ({DEBUGFLAG})
		Link -d -c 'MPS ' -t MPST 	¶
			{DEBUGOPTONS} {OBJECTS}	¶
			{LIBS} -o {APPL}
	else
		Link -d -c 'MPS ' -t MPST	¶
			{OBJECTS}				¶
			{LIBS} -o {APPL}
	end
	Rez  -o {APPL} {APPL}.r -a -ov
	Duplicate -y {APPL} {Destination}{APPL}

#####################################
#   Include file dependencies
#####################################

{OBJECTDIR}{APPL}.c.o 		Ä	"{3DOINCLUDES}misc:mpeg.h"
{OBJECTDIR}{APPL}.c.o		Ä	"{3DOINCLUDES}streaming:dsstreamdefs.h"

