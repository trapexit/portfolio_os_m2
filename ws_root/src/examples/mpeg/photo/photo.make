#####################################
##
##  Copyright (c) 1993-1996, an unpublished work by The 3DO Company.
##  All rights reserved. This material contains confidential
##  information that is the property of The 3DO Company. Any
##  unauthorized duplication, disclosure or use is prohibited.
##  
##  Distributed with Portfolio V32.0
##
#####################################


#####################################
#		Symbol definitions
#####################################
Appl			=	photo
DebugFlag		=	0
AppsDir			=	:Apps & Data:
ObjectDir		=	:Objects:
ExamplesDir		=	::
VectorDir =  {3DOVectors}{M2LibRelease}:


CC				=	dcc
LINK			=	link3do

MakeFileName	=	{Appl}.make

#####################################
#	Default compiler options for Diab Data
#####################################
CBaseOptions =	-I"{3doincludes}" 				# include path.  Be careful of ¶
				-I"{ExamplesDir}"				# the new heirarchical ¶
#				-I{3doincludes}audio:			# the new heirarchical ¶
#				-I{3doincludes}file:			# includes! ¶
#				-I{3doincludes}graphics:		# ¶
#				-I{3doincludes}hardware:		# ¶
#				-I{3doincludes}international:	# ¶
#				-I{3doincludes}streaming:		# ¶
#				-I{3doincludes}kernel:			# ¶
#				-I{3doincludes}misc:			# ¶
				-c								# MANDATORY		only run the c compiler, don't exec the linker ¶
				-WDDTARGET=PPC602				# MANDATORY		Tells the compiler you're building for the 602 (which is 603 safe) ¶
				-Xstring-align=1				# RECOMMENDED	word aligns strings ¶
				-Xdouble-warning -Xuse-float	# RECOMMENDED	produces single precision instructions/constants ¶
				-Ximport						# RECOMMENDED forces headers to be included only once ¶
				-Xstrict-ansi					# RECOMMENDED unless you are building code to be released, then its MANDATORY ;-) ¶
				-Xunsigned-char					# ? forces string constants to be unsigned char? ¶
				-XO -Xunroll=1 -Xtest-at-bottom	# ? Optimizations on, unroll loops (but not too aggressively), test loop conditional at bottom ¶
				-Xextend-args=0					# ? ¶
				-Xforce-prototypes				# RECOMMENDED ¶
				-Xinline=5						# ? 5 levels of inline/macro expansion ¶
				-Xno-libc-inlining -Xno-recognize-lib	# MANDATORY(?) Don't use any compiler built in functions/inlines ¶
				-Xno-bss=2						# MANDATORY(?) ¶
				-Xlint=0x10						# RECOMMENDED lovely commentary on your code quality ¶
				-Xtrace-table=0					# ? Don't generate traceback tables.  This is like macsbug names for you macheads	¶
				-Xsmall-data=0					# ¶
				-Xsmall-const=0					# ¶
				{RelativeBranchSwitch}

CSymOption		= -g -DDEBUG

Fixup3DOOptions	= -time now -subsys 1 -type 5 -stack 4096 -flags 128

#####################################
#		Object files
#####################################
LIBS			=	-L"{3dolibs}{M2LibRelease}"						¶
					-leventbroker	#expands to libeventbroker.a	¶
					-lc				#expands to libc.a

OBJECTS			=	"{ObjectDir}{Appl}.c.o" ¶
					"{ObjectDir}photo-util.c.o"

OBJECTDEPENDS		= "{Appl}.c.depends"

STD_MODULE		=	"{VectorDir}filesystem"	¶
					"{VectorDir}kernel"
					
DLLS         =   {STD_MODULE}			¶
					"{VectorDir}gstate"		¶
                    "{VectorDir}graphics"

#####################################
#	Default build rules
#####################################
All				Ä	{Appl}

{ObjectDir}		Ä	:

.c.o			Ä	.c {MakeFileName}
	if ( {DebugFlag} )
		echo "	compiling {Default}.c with {CSymOption}"
		{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o{CSymOption}{CBaseOptions}
	else
		echo "	compiling {Default}.c"
		{CC} {DepDir}{Default}.c -o {TargDir}{Default}.c.o  {CBaseOptions}
	end

.c.depends		Ä	.c
	set BaseDir "`directory`"
	{CC} {CBaseOptions} {DepDir}{Default}.c -E -H > Dev:Null ³ "c.includefiles"
	search -i -q -r -ns "{3DOIncludes}" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o" Ä'¨1; Replace /{BaseDir}/ ':'" ¶
		>> "{MakeFileName}"
	search -i -q -ns "{3DOIncludes}Misc:" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o" Ä'¨1; Replace /{3DOIncludes}Misc:/ '¶"¶{3DOIncludes¶}¶"Misc:'" ¶
		>> "{MakeFileName}"
	delete -i "c.includefiles"

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies,then
#	removes the first line (.c.o dependency on .c), substitutes a symbolicreference
#	to "{ObjectDir}", puts in a tab before the Äs, and appends the result to thismake
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "depends".
#	This will replace the include file dependencies lines at the end of thismakefile.
#####################################
Depends					Ä	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	Ä
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(Don't change this line or put anything after this section.¶)°/ "{MakeFileName}"
	Find /¥[Â#]/  "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"

#####################################
#	Target build rules
#####################################
{Appl}		Ä	{Appl}.make {OBJECTS}
	echo "Linking {Appl}"
	{LINK} -D -r -o {Appl}												¶
		-Htime=now -Hsubsys=1 -Hname={Appl} -Htype=5 -Hstack=32768		¶
		{OBJECTS}														¶
		{DLLS}														¶
		{LIBS}															¶
		> s{LOptions}.map
	SetFile {Appl} -c '3DOD' -t 'PROJ'
	if `exists "{3DORemote}Examples:mpeg:{Appl}:"`
		move -y {Appl} "{3DORemote}Examples:mpeg:{Appl}:"
	End

#####################################
#	Include file dependencies (Don't change this line or put anything after this section.)
#####################################

