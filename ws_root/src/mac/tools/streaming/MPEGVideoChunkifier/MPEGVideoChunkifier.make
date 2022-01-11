#####################################
##
##	@(#) MPEGVideoChunkifier.make 96/04/09 1.2
##
#####################################
#
#   File:       'MPEGVideoChunkifier.make'
#   Target:     'MPEGVideoChunkifier'
#   Created:    02-15-1995
#	Written by:	Philippe Cassereau
#
#	Shahriar Vaghar		11-09-95
#		- Added "{Libraries}"Toollibs.o to CLibs for cursor control.
#
#####################################
# If you wish to build this tool into it's own directory, then
# comment out the next line, instead of the following line.

Destination		= "{3DOMPWTools}"
#Destination		= :

APPL			= MPEGVideoChunkifier
SRCDIR			= ':'
OBJECTDIR		= ':Objects:'

#Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0
 
#
#	C compiler options
#
SymOptions		= -sym full
MemoryModel		= -model far
COptions 		= -mbg full -d NO_64_BIT_SCALARS -d MPW_C -d MACINTOSH -d STANDALONE=1 -i "{CINCLUDES}" -i "{3DOIncludes}" -i {SRCDIR}
AOptions		= -case on -case obj {SymOptions}



#	Include files for the Mac interface of the chunkifier application
#
CIncs			=	{SRCDIR}prototype.h

#
CLibs			=	"{CLibraries}"StdClib.o				¶
					"{CLibraries}"CPlusLib.o			¶
					"{Libraries}"MacRuntime.o			¶
					"{Libraries}"IntEnv.o 				¶
					"{Libraries}"Interface.o			¶
					"{Libraries}"Toollibs.o

#####################################
#	Source and object files for the Mac interface of the chunkifier application
#####################################
OBJECTS			=	{OBJECTDIR}MPEGVideoChunkifier.c.o	¶
					{OBJECTDIR}MPEGStreamInfo.c.o	¶
					{OBJECTDIR}MPEGUtils.c.o

#####################################
#	Default build rules
#####################################
{OBJECTDIR}		Ä	{SRCDIR}	

.c.o			Ä	.c {SRCDIR}{APPL}.make {CIncs}
	if ({DEBUGFLAG})
	 	SCpp {COptions} {SymOptions} -o {TargDir}{Default}.c.o {Default}.c
	else
	 	SCpp {COptions} -o {TargDir}{Default}.c.o {Default}.c
	end


#####################################
#	Build the Mac application
#####################################

{APPL}					Ä "{Destination}"{APPL}
"{Destination}"{APPL}	ÄÄ {OBJECTS} {SRCDIR}{APPL}.make
	if ({DEBUGFLAG})
		Link  {SymOptions} -d -c 'MPS ' -t MPST -o {Targ} {OBJECTS} {CLibs}
	else
		Link  -d -c 'MPS ' -t MPST -o {Targ} {OBJECTS} {CLibs}
	end

"{Destination}"{APPL}	ÄÄ {SRCDIR}{APPL}.r
	Rez -append -o {Targ} {SRCDIR}{APPL}.r

