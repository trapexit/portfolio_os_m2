#####################################
##
##	@(#) SAudioTool.make 95/11/22 1.4
##
#####################################
#
#   File:       SAudioTool.make
#   Target:     SAudioTool
#   Sources:    SAudioTool.c
#
#####################################
#
#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination		= {3DOMPWTools}
#Destination		= :

APPL			= SAudioTool

OBJECTDIR		= :objects:
OBJECTS 		= {OBJECTDIR}{APPL}.c.o

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COPTIONS		=  -i "{CINCLUDES}" -i "{3DOINCLUDES}" -d NO_64_BIT_SCALARS
# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0
 
DEBUGOPTIONS	= -sym on

LIBS            =   "{CLibraries}StdCLib.o"		¶
                    "{Libraries}Interface.o"	¶
                    "{Libraries}ToolLibs.o"		¶
                    "{Libraries}MacRuntime.o"	¶
                    "{Libraries}"IntEnv.o
 

#####################################
#   Default build rules
#####################################
{OBJECTDIR} Ä   :

.c.o        Ä   .c {APPL}.make
    if ({DEBUGFLAG})
     	SC {DEBUGOPTIONS} {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
    else
     	SC {COPTIONS} -o {TargDir}{Default}.c.o -r {Default}.c
	end

{APPL}		Ä "{Destination}"{APPL}
"{Destination}"{APPL} ÄÄ {APPL}.make {OBJECTS}
    if ({DEBUGFLAG})
		Link -d -c 'MPS ' -t MPST	¶
			{DEBUGOPTIONS}			¶
			{OBJECTS}				¶
			{LIBS} 					¶
			-o {Targ}
    else
		Link -d -c 'MPS ' -t MPST	¶
			{OBJECTS}				¶
			{LIBS} 					¶
			-o {Targ}
	end

"{Destination}"{APPL} ÄÄ {APPL}.r
	Rez  -o {Targ} {APPL}.r -a -ov

#####################################
#   Include file dependencies
#####################################
{APPL}.c.o 	Ä "{3DOINCLUDES}streaming:dsstreamdefs.h"
