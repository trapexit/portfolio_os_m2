##########################################################################
#   File:       gmrec.make
#
##########################################################################

#####################################
#	Make file symbol definitions
#####################################
#	If the name of the object folder is changed, the makeit script must be updated
ObjectDir			=	:Objects:
CC68K = SC
CCPPC = MRC
M2IncludeDir		=	::txtlib:include: 
GmercIncludeDir		=	:includes: 
M2ObjectDir			=	::libs:Mac:
HGReleaseDir		=	::release:Mac:
M2Lib68K			= 	{M2ObjectDir}M2TXlib.68k.a
M2LibPPC			= 	{M2ObjectDir}M2TXlib.PPC.a
IFFLIB68K       	=   {M2ObjectDir}IFFlib.68K.a
IFFLIBPPC       	=   {M2ObjectDir}IFFlib.PPC.a
StdCLibsPPC			= �
						"{SharedLibraries}"InterfaceLib �
						"{SharedLibraries}"MathLib �
#						"{PPCLibraries}"MathLib.xcoff �
						"{SharedLibraries}"StdCLib �
						"{PPCLibraries}"StdCRuntime.o �
						"{PPCLibraries}"PPCCRuntime.o �

StdCLibs68K			=	�
						"{CLibraries}"StdClib.o �
 						"{Libraries}"MacRuntime.o �
#						"{CLibraries}"CSANELib.o �
#						"{CLibraries}"Math.o �
						"{Libraries}"MathLib.o �
 						"{Libraries}"Interface.o �
 						"{Libraries}"IntEnv.o 
						
# Set up to generate tools for a Mac with a floating-point coprocessor
#MathLibs			=	"{MPW}Libraries:CLibraries:CLib881.o"		�
#						"{MPW}Libraries:CLibraries:Math881.o"		�
#						"{MPW}Libraries:CLibraries:CSANELib881.o"
# Set up to generate tools for a Mac without a floating-point coprocessor
MathLibs			=	"{MPW}Libraries:CLibraries:Math.o"			 �
						"{MPW}Libraries:CLibraries:CSANELib.o"

ALLOCDIR = :alloc:
BSDFDIR = :bsdf:
GEO_INTDIR = :geo_int:
GEODIR = :geo:
PARSERDIR = :parser:
ANIMDIR = :anim:
MAC_DIR = :mac:
PORTDIR = :port:

BSDF68KOBJ = �
	{BSDFDIR}bsdf_anim.o	�
	{BSDFDIR}bsdf_main.o	�
	{BSDFDIR}bsdf_write.o

BSDFPPCOBJ = �
	{BSDFDIR}bsdf_anim.c.o		�
	{BSDFDIR}bsdf_main.c.o		�
	{BSDFDIR}bsdf_write.c.o

GEO_INT68KOBJ = �
	{GEO_INTDIR}geointerface.o		�
	{GEO_INTDIR}geoitpr.o			�
	{GEO_INTDIR}texpage.o

GEO_INTPPCOBJ = �
	{GEO_INTDIR}geointerface.c.o	�
	{GEO_INTDIR}geoitpr.c.o		�
	{GEO_INTDIR}texpage.c.o

ALLOC68KOBJ = �
	{ALLOCDIR}allocator.o

ALLOCPPCOBJ = �
	{ALLOCDIR}allocator.c.o

GLIB68KOBJ = �
	{GEODIR}tmsnake.o	�
	{GEODIR}compsurf.o	�
	{GEODIR}box.o		�
	{GEODIR}vec.o		�
	{GEODIR}geo.o		�
	{GEODIR}tmutils.o

GLIBPPCOBJ = �
	{GEODIR}tmsnake.c.o		�
	{GEODIR}compsurf.c.o	�
	{GEODIR}box.c.o			�
	{GEODIR}vec.c.o			�
	{GEODIR}geo.c.o			�
	{GEODIR}tmutils.c.o

PARSER68KOBJ = �
	{PARSERDIR}enum.o				�
	{PARSERDIR}array.o				�
	{PARSERDIR}class.o				�
	{PARSERDIR}errors.o				�
	{PARSERDIR}file.o				�
	{PARSERDIR}parse.o				�
	{PARSERDIR}parserfunctions.o	�
	{PARSERDIR}sdfclasses.o			�
	{PARSERDIR}sdfsyntax.o			�
	{PARSERDIR}sdfvalue.o			�
	{PARSERDIR}tokens.o				�
	{PARSERDIR}memory.o				�
	{PARSERDIR}builtinclasses.o

PARSERPPCOBJ = �
	{PARSERDIR}enum.c.o				�
	{PARSERDIR}array.c.o			�
	{PARSERDIR}class.c.o			�
	{PARSERDIR}errors.c.o			�
	{PARSERDIR}file.c.o				�
	{PARSERDIR}parse.c.o			�
	{PARSERDIR}parserfunctions.c.o	�
	{PARSERDIR}sdfclasses.c.o		�
	{PARSERDIR}sdfsyntax.c.o		�
	{PARSERDIR}sdfvalue.c.o			�
	{PARSERDIR}tokens.c.o			�
	{PARSERDIR}memory.c.o			�
	{PARSERDIR}builtinclasses.c.o

ANIM68KOBJ = �
	{ANIMDIR}kf_path.o		�
	{ANIMDIR}kf_quat.o		�
	{ANIMDIR}kf_spline.o

ANIMPPCOBJ = �
	{ANIMDIR}kf_path.c.o		�
	{ANIMDIR}kf_quat.c.o		�
	{ANIMDIR}kf_spline.c.o

MAC_68KOBJ = �
	{MAC_DIR}unixglue.o

MAC_PPCOBJ = �
	{MAC_DIR}unixglue.c.o

PORT68KOBJ = �
	{PORTDIR}writeiff.o

PORTPPCOBJ = �
	{PORTDIR}writeiff.c.o

68KGMERCOBJ = �
	{ObjectDir}bsdf_anim.o		�
	{ObjectDir}bsdf_main.o		�
	{ObjectDir}bsdf_write.o		�
	{ObjectDir}geointerface.o	�
	{ObjectDir}geoitpr.o		�
	{ObjectDir}texpage.o		�
	{ObjectDir}allocator.o		�
	{ObjectDir}tmsnake.o		�
	{ObjectDir}compsurf.o		�
	{ObjectDir}box.o			�
	{ObjectDir}vec.o			�
	{ObjectDir}geo.o			�
	{ObjectDir}tmutils.o		�
	{ObjectDir}enum.o			�
	{ObjectDir}array.o			�
	{ObjectDir}class.o			�
	{ObjectDir}errors.o			�
	{ObjectDir}file.o			�
	{ObjectDir}parse.o			�
	{ObjectDir}parserfunctions.o	�
	{ObjectDir}sdfclasses.o		�
	{ObjectDir}sdfsyntax.o		�
	{ObjectDir}sdfvalue.o		�
	{ObjectDir}tokens.o			�
	{ObjectDir}memory.o			�
	{ObjectDir}builtinclasses.o	�
	{ObjectDir}kf_path.o		�
	{ObjectDir}kf_quat.o		�
	{ObjectDir}kf_spline.o		�
	{ObjectDir}unixglue.o		�
	{ObjectDir}writeiff.o
		
PPCGMERCOBJ = �
	{ObjectDir}bsdf_anim.c.o	�
	{ObjectDir}bsdf_main.c.o	�
	{ObjectDir}bsdf_write.c.o	�
	{ObjectDir}geointerface.c.o	�
	{ObjectDir}geoitpr.c.o		�
	{ObjectDir}texpage.c.o		�
	{ObjectDir}allocator.c.o	�
	{ObjectDir}tmsnake.c.o		�
	{ObjectDir}compsurf.c.o		�
	{ObjectDir}box.c.o			�
	{ObjectDir}vec.c.o			�
	{ObjectDir}geo.c.o			�
	{ObjectDir}tmutils.c.o		�
	{ObjectDir}enum.c.o			�
	{ObjectDir}array.c.o		�
	{ObjectDir}class.c.o		�
	{ObjectDir}errors.c.o		�
	{ObjectDir}file.c.o			�
	{ObjectDir}parse.c.o		�
	{ObjectDir}parserfunctions.c.o	�
	{ObjectDir}sdfclasses.c.o	�
	{ObjectDir}sdfsyntax.c.o	�
	{ObjectDir}sdfvalue.c.o		�
	{ObjectDir}tokens.c.o		�
	{ObjectDir}memory.c.o		�
	{ObjectDir}builtinclasses.c.o	�
	{ObjectDir}kf_path.c.o		�
	{ObjectDir}kf_quat.c.o		�
	{ObjectDir}kf_spline.c.o	�
	{ObjectDir}unixglue.c.o		�
	{ObjectDir}writeiff.c.o


#####################################
#	Default compiler and linker options
#####################################
IncludeFlags	=	-i "{M2IncludeDir}" -i "::ifflib:" �
		-i "{GmercIncludeDir}" �
		-i "{PARSERDIR}" �
		-i "{PORTDIR}" �
		-i "{GEO_INTDIR}" �
		-i "{ALLOCDIR}"
68KCOptions		=	-mc68020 -opt speed -r {IncludeFlags} -model far -d applec -d NO_64_BIT_SCALARS -d macintosh -d EXCLUDE_SIMULATOR -d HWSIMULATION -d GFX_C_Bind -d NEW_MAC_MEMORY
68KLTOptions	=	-model far -t MPST -c 'MPS ' -br on
68KLOptions		=	-model far -t APPL -c '????' -br on
68KLibOptions	=	-mf -sym off
 
PPCCOptions		=    -opt speed {IncludeFlags} -d applec -d NO_64_BIT_SCALARS -d macintosh -d EXCLUDE_SIMULATOR -d HWSIMULATION -d GFX_C_Bind -d NEW_MAC_MEMORY
PPCLOptions		=	 -outputformat xcoff -d -c 'MPS ' -t MPST -main __start -mf
PPCLibOptions	=	 -xm l -sym off
PEFOptions		=   -l InterfaceLib.xcoff=InterfaceLib �
					-l MathLib.xcoff=MathLib �
					-l StdCLib.xcoff=StdCLib �
					-ft APPL -fc '????'

#####################################
#	Default build rules
#####################################
All				�	complete

{ObjectDir}		�	:


#####################################
#	Include file dependencies
#####################################
LWIncludes		= "{CIncludes}ctype.h" "{CIncludes}stdio.h" "{CIncludes}string.h" "{CIncludes}stdlib.h"

{ObjectDir}bsdf_anim.o		�   {BSDFDIR}bsdf_anim.c
{ObjectDir}bsdf_main.o		�   {BSDFDIR}bsdf_main.c
{ObjectDir}bsdf_write.o		�   {BSDFDIR}bsdf_write.c
{ObjectDir}allocator.o		�   {BSDFDIR}allocator.c
{ObjectDir}geointerface.o	�   {GEO_INTDIR}geointerface.c
{ObjectDir}geoitpr.o		�   {GEO_INTDIR}geoitpr.c
{ObjectDir}texpage.o		�   {GEO_INTDIR}texpage.c
{ObjectDir}tmsnake.o		�   {GEODIR}tmsnake.c
{ObjectDir}compsurf.o		�   {GEODIR}compsurf.c
{ObjectDir}box.o			�   {GEODIR}box.c
{ObjectDir}vec.o			�   {GEODIR}vex.c
{ObjectDir}geo.o			�   {GEODIR}geo.c
{ObjectDir}tmutils.o		�   {GEODIR}tmutils.c
{ObjectDir}enum.o			�   {PARSERDIR}enum.c
{ObjectDir}array.o			�	{PARSERDIR}array.c
{ObjectDir}class.o			�	{PARSERDIR}class.c
{ObjectDir}errors.o			�	{PARSERDIR}errors.c
{ObjectDir}file.o			�	{PARSERDIR}file.c
{ObjectDir}parse.o			�	{PARSERDIR}parse.c
{ObjectDir}parserfunctions.o	�	{PARSERDIR}parserfunctions.c
{ObjectDir}sdfclasses.o		�	{PARSERDIR}sdfclasses.c
{ObjectDir}sdfsyntax.o		�	{PARSERDIR}sdfsyntax.c
{ObjectDir}sdfvalue.o		�	{PARSERDIR}sdfvalue.c
{ObjectDir}tokens.o			�	{PARSERDIR}tokens.c
{ObjectDir}memory.o			�	{PARSERDIR}memory.c
{ObjectDir}builtinclasses.o	�	{PARSERDIR}builtinclasses.c
{ObjectDir}kf_path.o		�	{ANIMDIR}kf_path.c
{ObjectDir}kf_quat.o		�	{ANIMDIR}kf_quat.c
{ObjectDir}kf_spline.o		�	{ANIMDIR}kf_spline.c
{ObjectDir}writeiff.o		�	{PORTDIR}writeiff.c
{ObjectDir}bsdf_anim.c.o	�   {BSDFDIR}bsdf_anim.c
{ObjectDir}bsdf_main.c.o	�   {BSDFDIR}bsdf_main.c
{ObjectDir}bsdf_write.c.o	�   {BSDFDIR}bsdf_write.c
{ObjectDir}allocator.o		�   {BSDFDIR}allocator.c
{ObjectDir}geointerface.c.o	�   {GEO_INTDIR}geointerface.c
{ObjectDir}geoitpr.c.o		�   {GEO_INTDIR}geoitpr.c
{ObjectDir}texpage.c.o		�   {GEO_INTDIR}texpage.c
{ObjectDir}tmsnake.c.o		�   {GEODIR}tmsnake.c
{ObjectDir}compsurf.c.o		�   {GEODIR}compsurf.c
{ObjectDir}box.c.o			�   {GEODIR}box.c
{ObjectDir}vec.c.o			�   {GEODIR}vex.c
{ObjectDir}geo.c.o			�   {GEODIR}geo.c
{ObjectDir}tmutils.c.o		�   {GEODIR}tmutils.c
{ObjectDir}enum.c.o			�   {PARSERDIR}enum.c
{ObjectDir}array.c.o		�	{PARSERDIR}array.c
{ObjectDir}class.c.o		�	{PARSERDIR}class.c
{ObjectDir}errors.c.o		�	{PARSERDIR}errors.c
{ObjectDir}file.c.o			�	{PARSERDIR}file.c
{ObjectDir}parse.c.o		�	{PARSERDIR}parse.c
{ObjectDir}parserfunctions.c.o	�	{PARSERDIR}parserfunctions.c
{ObjectDir}sdfclasses.c.o	�	{PARSERDIR}sdfclasses.c
{ObjectDir}sdfsyntax.c.o	�	{PARSERDIR}sdfsyntax.c
{ObjectDir}sdfvalue.c.o		�	{PARSERDIR}sdfvalue.c
{ObjectDir}tokens.c.o		�	{PARSERDIR}tokens.c
{ObjectDir}memory.c.o		�	{PARSERDIR}memory.c
{ObjectDir}builtinclasses.c.o	�	{PARSERDIR}builtinclasses.c
{ObjectDir}kf_path.c.o		�	{ANIMDIR}kf_path.c
{ObjectDir}kf_quat.c.o		�	{ANIMDIR}kf_quat.c
{ObjectDir}kf_spline.c.o	�	{ANIMDIR}kf_spline.c
{ObjectDir}writeiff.c.o		�	{PORTDIR}writeiff.c

#####################################
#	Target build rules
#####################################
#####################################
# CLI tools built as MPW Tools
#####################################

.o			�	.c
	echo "Compiling 68K {Default}.c"; �
	{CC68K} {68KCOptions} -o {DepDir}{Default}.o {DepDir}{Default}.c

.c.o			�	.c
	echo "Compiling  PPC {Default}.c"; �
	{CCPPC} {PPCCOptions} -o {DepDir}{Default}.c.o {DepDir}{Default}.c

68KMERCOBJ = �
	{BSDF68KOBJ} �
	{GEO_INT68KOBJ} �
	{ALLOC68KOBJ} �
	{GLIB68KOBJ} �
	{PARSER68KOBJ} �
	{ANIM68KOBJ} �
	{MAC_68KOBJ} �
	{PORT68KOBJ}

PPCMERCOBJ = �
	{BSDFPPCOBJ} �
	{GEO_INTPPCOBJ} �
	{ALLOCPPCOBJ} �
	{GLIBPPCOBJ} �
	{PARSERPPCOBJ} �
	{ANIMPPCOBJ} �
	{MAC_PPCOBJ} �
	{PORTPPCOBJ}


gmerc_68k �� {BSDF68KOBJ} {GEO_INT68KOBJ} �
		{ALLOC68KOBJ} {GLIB68KOBJ} �
		{PARSER68KOBJ} {ANIM68KOBJ} �
		{MAC_68KOBJ} {PORT68KOBJ}
		echo "Linking gmerc 68K"; �
		Link {BSDF68KOBJ} {GEO_INT68KOBJ} �
		{ALLOC68KOBJ} {GLIB68KOBJ} �
		{PARSER68KOBJ} {ANIM68KOBJ} �
		{MAC_68KOBJ} {PORT68KOBJ} -o gmerc �
		{68KLTOptions} "{Libraries}"ToolLibs.o {StdCLibs68K} {M2Lib68K} {IFFLIB68K} ; �
		echo "gmerc 68K complete" ; �

gmerc �� {68KMERCOBJ} {PPCMERCOBJ}
		echo "Linking gmerc FAT";
		PPCLink {PPCMERCOBJ} -o gmerc.xcoff �
		"{PPCLibraries}"PPCToolLibs.o {StdCLibsPPC} {M2LibPPC} {IFFLIBPPC} {PPCLOptions}; 
		makePEF gmerc.xcoff -o gmerc {PEFOptions}; 
		Link {68KMERCOBJ} -o gmerc �
		{68KLTOptions} "{Libraries}"ToolLibs.o {StdCLibs68K} {M2Lib68K} {IFFLIB68K}; 
		echo "Rezzing";
		Rez -d APPNAME=�"gmerc�" -a "{Rincludes}"SIOW.r -o gmerc;
		echo "gmerc FAT complete" ; 
		delete -i gmerc.xcoff;
		duplicate -y gmerc {HGReleaseDir};
		move gmerc :bin:Mac:gmerc


#####################################
# Build all of the MPW tools
#####################################
complete 		�	gmerc

#####################################
# Scrub all old objects and binaries
#####################################
clean			�
	set exit 0; �
	delete -i {68KMERCOBJ} {PPCMERCOBJ}; �
	set exit 1



