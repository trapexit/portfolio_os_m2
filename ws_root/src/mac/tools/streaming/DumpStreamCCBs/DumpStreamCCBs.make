#####################################
##
##	@(#) DumpStreamCCBs.make 95/05/26 1.1
##
#####################################
#
#   File:       DumpStreamCCBs.make
#   Target:     DumpStreamCCBs
#   Sources:    DumpStreamCCBs.c
#   Created:    9/14/93
#
#####################################
#
#  If you wish to build this tool into it's own directory, then
#		you should comment out the next two lines, instead
#		of the following two lines.

Destination	= :::
COPYTODESTINATION	= Move -y {PROGRAM} "{Destination}"{PROGRAM}
#Destination		= :
#COPYTODESTINATION	= 

PROGRAM				= DumpStreamCCBs

OBJECTS			= {PROGRAM}.c.o
DEBUGOPTIONS	= 
#DEBUGOPTIONS	= -sym full
COPTIONS		= -r -i "{CINCLUDES}" -i "{3DOINCLUDES}" -i "::Weaver:"

{PROGRAM}		Ä "{Destination}"{PROGRAM}
"{Destination}"{PROGRAM} Ä {PROGRAM}.make {PROGRAM}.r {OBJECTS}
	Rez  -o {PROGRAM} {PROGRAM}.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}Interface.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}"IntEnv.o ¶
		-o {PROGRAM}
	{COPYTODESTINATION}

{PROGRAM}.c.o Ä {PROGRAM}.make {PROGRAM}.c ¶
			PortableSANMDefs.h ¶
			::Weaver:weavechunk.h
	 C {COPTIONS} {DEBUGOPTIONS} {PROGRAM}.c
