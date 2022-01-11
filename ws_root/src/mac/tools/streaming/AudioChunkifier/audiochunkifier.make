#####################################
##
##	@(#) audiochunkifier.make 95/11/22 1.5
##
#####################################
#
#   File:       audiochunkifier.make
#   Target:     audiochunkifier
#   Sources:    audiochunkifier.c
#   Created:    Saturday, April 17, 1993 2:33:20 AM
#
#####################################
#
#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination		= "{3DOMPWTools}"
#Destination	= :

APPL			= AudioChunkifier
OBJECTDIR		= :objects:
OBJECTS 		= {OBJECTDIR}{APPL}.c.o

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

DEBUGOPTIONS 	= -sym on

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COPTIONS		=  -i "{CINCLUDES}" -i "{3DOINCLUDES}" -d NO_64_BIT_SCALARS

LIBS			= 	"{CLibraries}StdCLib.o" �
					"{Libraries}Interface.o" �
					"{Libraries}ToolLibs.o" �
					"{Libraries}MacRuntime.o" �
					"{Libraries}"IntEnv.o

#####################################
#   Default build rules
#####################################
{OBJECTDIR} �   :

.c.o        �   .c {APPL}.make
	if ({DEBUGFLAG})
    	SC {DEBUGOPTIONS} {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	else
    	SC {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	end

{APPL}					� "{Destination}"{APPL}
"{Destination}"{APPL}	� {APPL}.make {OBJECTS} {APPL}.r
	if ({DEBUGFLAG})
		Link -d -c 'MPS ' -t MPST �
			{DEBUGOPTIONS} �
			{OBJECTS} �
			{LIBS}	�
			-o {Targ}
	else
		Link -d -c 'MPS ' -t MPST �
			{OBJECTS} �
			{LIBS}	�
			-o {Targ}
	end
	Rez  -o {Targ} {APPL}.r -a -ov

#####################################
#   Include file dependencies
#####################################
{APPL}.c.o		� "{3DOINCLUDES}Streaming:dsstreamdefs.h"
