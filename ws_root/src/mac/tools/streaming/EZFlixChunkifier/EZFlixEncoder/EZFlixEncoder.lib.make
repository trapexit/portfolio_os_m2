#####################################
##
##      @(#) EZFlixEncoder.lib.make 96/03/06 1.6
##
#####################################
#
#	File:		EXFlixEncoder.Lib.make
#
#	Contains:	Make file for building EZFlix encoder library.
#
#	Written by:	Greg Wallace
#
#   To regenerate the .c.o -> .h file dependencies, get write access to this
#   make file and execute the following MPW code:
#
#####################################
#		Symbol definitions
#####################################
Library			=	EZFlixEncoder

# Set the following flag to 1 to generate symbols for symbolic debugging
DebugFlag		=	1
ObjectDir		=	:Objects:
CC				=	SC
CPLUS			=	SCpp
ASM				=	Asm
LIBRARIAN		=	Lib

EncoderLibs		=	::Libs:
EncoderIncludes	=	::Includes:

EchoOn			=	set oldEcho {echo}; set echo 1
EchoOff			=	set echo {oldEcho}


	#Debugging?  Enable this
DEBUGOPTIONS 	= -sym full

	#Not debugging? Enable this
#DEBUGOPTIONS 	= -sym off

MCOptions		= -mc68020 #Debugging
#MCOptions		= -mc68020 -mc68881


#####################################
#	Default compiler options
#####################################
# NO_64_BIT_SCALARS compile switch will weed out the 64 bit scalar types defined in kernel/types.h

# Don't use math option, so emulation on PowerMac system will work.
COptions 		= {MCOptions} -i "{EncoderIncludes}" -d DEBUG={DebugFlag} -d NO_64_BIT_SCALARS	-d makeformac
CPlusOptions	= {COptions}
SOptions		= 

LOptions		= -mf 

DebugOption		= -sym full

#####################################
#		Object files
#####################################
OBJECTS			=									¶
					"{ObjectDir}EZFlixEncoder.c.o"	¶
					"{ObjectDir}Quantizers.cp.o"	¶
					"{ObjectDir}EZFlixCompress.cp.o"

#####################################
#	Default build rules
#####################################
All				Ä	{Library}.lib

{ObjectDir}		Ä	:

.cp.o			Ä	.cp
    if ({DEBUGFLAG})
		{EchoOn}
		{CPLUS} {DebugOption} {CPlusOptions} -o {TargDir}{Default}.cp.o {DepDir}{Default}.cp
		{EchoOff}
	else
		{EchoOn}
		{CPLUS} {CPlusOptions} -o {TargDir}{Default}.cp.o {DepDir}{Default}.cp
		{EchoOff}
	end

.c.o			Ä	.c
    if ({DEBUGFLAG})
		{EchoOn}
		{CC} {DebugOption} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
		{EchoOff}
	else
		{EchoOn}
		{CC} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
		{EchoOff}
	end

.s.o			Ä	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################
{Library}.lib		Ä	{Library}.lib.make {OBJECTS}
	if !`exists {EncoderLibs}`
		NewFolder {EncoderLibs}
	end
    if ({DEBUGFLAG})
		{EchoOn}
		{LIBRARIAN}	{DebugOption} {LOptions} -o {Library}.lib		¶
				{OBJECTS}
		{EchoOff}
	else
		{EchoOn}
		{LIBRARIAN}	{LOptions} -o {Library}.lib		¶
				{OBJECTS}
		{EchoOff}
	end
	move -y {Library}.lib	{EncoderLibs}{Library}.lib


#####################################
#	Include file dependencies
#####################################

{ObjectDir}EZFlixEncoder.c.o	Ä {Library}.lib.make ¶
									EZFlixEncoder.c ¶
									{EncoderIncludes}EZFlixEncoder.h ¶
									EZFlixCodec.h ¶
									EZFlixCompress.h
{ObjectDir}EZFlixCompress.cp.o	Ä {Library}.lib.make ¶
									EZFlixCompress.cp ¶
									EZFlixCompress.h ¶
									{EncoderIncludes}EZFlixEncoder.h ¶
									EZFlixCodec.h ¶
									Quantizers.h ¶
									preprocArray.c
{ObjectDir}Quantizers.cp.o		Ä {Library}.lib.make ¶
									Quantizers.cp ¶
									Quantizers.h

