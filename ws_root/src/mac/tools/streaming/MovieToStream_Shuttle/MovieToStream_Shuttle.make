
#####################################
##
##	@(#) MovieToStream_Shuttle.make 95/06/09 1.2
##
#####################################
#
#   File:       MovieToStream_Shuttle.make
#   Target:     MovieToStream_Shuttle
#   Sources:    MovieToStream_Shuttle.c
#   Created:    Sunday, June 27, 1993 4:41:00 PM
#####################################
#
APPL			= MovieToStream_Shuttle
OBJECTDIR		= :objects:
OBJECTS 		= {OBJECTDIR}{APPL}.c.o
COptions 		= -r -d makeformac -i "{CINCLUDES}" -i "{3DOINCLUDES}"
#DEBUGOPTIONS   = -sym on
DEBUGOPTIONS    =

#  If you wish to build this tool into it's own directory, then
#  you should comment out the next line, instead of the following line.

Destination		= :::
#Destination		= :

#####################################
#   Default build rules
#####################################
{OBJECTDIR} Ä   :

.c.o        Ä   .c {APPL}.make
     C {DEBUGOPTIONS} {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c


{APPL}		Ä "{Destination}"{APPL}
"{Destination}"{APPL} ÄÄ {APPL}.make {OBJECTS}
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}Interface.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {Targ}

"{Destination}"{APPL} ÄÄ {APPL}.r
	Rez  -o {Targ} {APPL}.r -a -ov

#####################################
#   Include file dependencies
#####################################

{APPL}.c.o      Ä   Films.h
{APPL}.c.o      Ä   VQDecoder.h
