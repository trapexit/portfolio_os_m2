#####################################
##
##	@(#) DumpAIFF.make 95/06/07 1.2
##
#####################################
#
#   File:       DumpAIFF.make
#   Target:     DumpAIFF
#   Sources:    DumpAIFF.c
#
#####################################
#
APPL			= DumpAIFF
OBJECTDIR   	= :objects:
OBJECTS			= {OBJECTDIR}{APPL}.c.o
#DEBUGOPTIONS	= -sym on
DEBUGOPTIONS	= 

#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination		= :::
#Destination		= :


#####################################
#   Default build rules
#####################################
{OBJECTDIR} Ä   :

.c.o        Ä   .c {APPL}.make
     C {DEBUGOPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c


{APPL}		Ä "{Destination}"{APPL}
"{Destination}"{APPL} ÄÄ {APPL}.make {OBJECTS}
	Link -d -c 'MPS ' -t MPST ¶
		{DEBUGOPTIONS} ¶
		{OBJECTS} ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}Interface.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {Targ}

"{Destination}"{APPL} ÄÄ {APPL}.r
    Rez  -o {Targ} {APPL}.r -a -ov
