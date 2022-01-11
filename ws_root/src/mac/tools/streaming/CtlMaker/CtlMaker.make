
#####################################
##
##	@(#) CtlMaker.make 95/11/22 1.4
##
#####################################
#
#   File:       CtlMaker.make
#   Target:     CtlMaker
#   Sources:    CtlMaker.c
#   Created:    Saturday, July 10, 1993 10:56:34 PM
#
#####################################
#
#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination = "{3DOMPWTools}"
#Destination    = :

APPL		= CtlMaker
OBJECTDIR   = :objects:
OBJECTS		= {OBJECTDIR}{APPL}.c.o

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

DEBUGOPTIONS	= -sym on 

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COPTIONS = -r -i "{CINCLUDES}" -i "{3DOINCLUDES}" -d NO_64_BIT_SCALARS

LIBS			=	"{CLibraries}StdCLib.o" ¶
					"{Libraries}Interface.o" ¶
					"{Libraries}MacRuntime.o" ¶
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
		Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
			{OBJECTS}	¶
			{LIBS}		¶
			-o {Targ}
	else
		Link -d -c 'MPS ' -t MPST ¶
			{OBJECTS}	¶
			{LIBS}		¶
			-o {Targ}
	end

"{Destination}"{APPL} ÄÄ {APPL}.r
    Rez  -o {Targ} {APPL}.r -a -ov
