#
#####################################
#
#	 @(#) DATAChunkify.make 96/03/15 1.2
#
#   File:       DataChunkify.make
#   Target:     DataChunkify
#   Sources:    DataChunkify.c, getconfig.c
#   Created:    Thursday, July 21, 1994 6:56:34 AM
#
#####################################
#

Destination		= {3DOMPWTools}			# uncomment this line to compile into {3DOMPWTools}
#Destination		= :						# uncomment this line to compile into current dir

APPL			= DATAChunkify
OBJECTDIR		= :Objects:
OBJECTS 		= "{OBJECTDIR}{APPL}".c.o "{OBJECTDIR}"getconfig.c.o 

CC				= SC
REZ				= Rez
LINK			= Link

# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 1

DEBUGOPTIONS	= -sym full


# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in 
#  kernel/types.h __KERNEL_MEM_H compile switch will prevent inclusion of 3DO 
#  <:kernel:mem.h>, which causes a problem because the Mac headers have a function 
#  "FreeMem()", defined differently than the other, or course 
COPTIONS	=			¶
			-mc68020 -r -i "{CINCLUDES}" -i "{3DOINCLUDES}" -d NO_64_BIT_SCALARS ¶
				 -d __KERNEL_MEM_H

LIBS		=			¶
			"{CLibraries}StdCLib.o"		¶
            "{Libraries}Interface.o"	¶
            "{Libraries}ToolLibs.o"		¶
            "{Libraries}MacRuntime.o"	¶
            "{Libraries}IntEnv.o"
 

#####################################
#   Default build rules
#####################################
{OBJECTDIR} Ä   :

.c.o        Ä   .c .h {APPL}.make
	if ({DEBUGFLAG})
     	{CC} {Default}.c {DEBUGOPTIONS} {COPTIONS} -o {TargDir}{Default}.c.o -r
	else
     	{CC} {Default}.c {COPTIONS} -o {TargDir}{Default}.c.o -r
	end

{APPL}					Ä "{Destination}"{APPL}
"{Destination}"{APPL}	ÄÄ {APPL}.make {OBJECTS}
	if ({DEBUGFLAG})
		{LINK} -d -c 'MPS ' -t MPST		¶
			{DEBUGOPTIONS} {OBJECTS}	¶
			{LIBS}	-o {Targ}
	else
		{LINK} -d -c 'MPS ' -t MPST		¶
			{OBJECTS} {LIBS}			¶
			-o {Targ}
	end

"{Destination}"{APPL} ÄÄ {APPL}.r
	{REZ}  -o {Targ} {APPL}.r -a -ov

#####################################
#   Include file dependencies
#####################################
"{OBJECTDIR}{APPL}".c.o		Ä {APPL}.h
"{OBJECTDIR}{APPL}".c.o		Ä getconfig.h
"{OBJECTDIR}{APPL}".c.o		Ä "{3DOINCLUDES}streaming:dsstreamdefs.h"
"{OBJECTDIR}getconfig.c.o"	Ä getconfig.h
"{OBJECTDIR}getconfig.c.o"	Ä "{3DOINCLUDES}streaming:dsstreamdefs.h"
