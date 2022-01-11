#####################################
##
##	@(#) QTVideoChunkifier.make 96/03/15 1.8
##
#####################################
#
#   File:       QTVideoChunkifier.make
#   Target:     QTVideoChunkifier
#   Sources:    QTVideoChunkifier.c
#   Created:    Sunday, June 27, 1993 4:41:00 PM
#
#####################################

APPL			= QTVideoChunkifier

OBJECTDIR		= :objects:
OBJECTS			= {OBJECTDIR}{APPL}.c.o
FLIXINTERFACES	= ::EZFlixChunkifier:Includes:
FLIXENCODERLIB	= ::EZFlixChunkifier:Libs:EZFlixEncoder.Lib

# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h
COptions		= -r -d makeformac -i "{CINCLUDES}" -i "{3DOINCLUDES}" -i {FLIXINTERFACES} -d NO_64_BIT_SCALARS

DEBUGOPTIONS	= -sym on 

#  If you wish to build this tool into it's own directory, then
#  comment out the next line, instead of the following line.

Destination		= "{3DOMPWTools}"
#Destination		= :


# Set the following flag to 1 to generate symbols for symbolic debugging
DEBUGFLAG		= 0

LIBS            =   "{CLibraries}StdCLib.o"		¶
                    "{Libraries}Interface.o"	¶
                    "{Libraries}ToolLibs.o"		¶
                    "{Libraries}MacRuntime.o"	¶
                    "{Libraries}"IntEnv.o		¶
					"{FLIXENCODERLIB}"

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

{APPL} Ä {APPL}.make {OBJECTS} {FLIXENCODERLIB}
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

{OBJECTDIR}{APPL}.c.o 		Ä	Films.h
{OBJECTDIR}{APPL}.c.o		Ä	VQDecoder.h
{OBJECTDIR}{APPL}.c.o		Ä	"{FLIXINTERFACES}"EZFlixEncoder.h
{OBJECTDIR}{APPL}.c.o		Ä	"{FLIXINTERFACES}"EZFlixStream.h
{OBJECTDIR}{APPL}.c.o		Ä	"{FLIXINTERFACES}"EZFlixDefines.h
