#####################################
##
##      @(#) libEZFlixDecoder.a.make 95/12/13 1.3
##
#####################################

#####################################
#
#   File:       libEZFlixDecoder.a.make
#
#	Contains:	make file for building libEZFlixDecoder.a
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f libEZFlixDecoder.a.make �� "{worksheet}" > temp.makeout; temp.makeout �� "{worksheet}"; delete -i temp.makeout
#
#####################################

#####################################
#		Symbol definitions
#####################################
Destination			=	"{3doLibs}{M2LibRelease}:"
Library				=	libEZFlixDecoder
LibraryFileName		=	{Library}.a
DebugFlag			=	0
ObjectDir			=	:Objects:
StreamDir			=	:
#RAMDISKDIR			=
#RAMDISKDIR			= RAM Disk:
CC					=	dcc
ASM					=	das
LIBRARIAN			=	dar						# archive maintainer
LINK				=	link3do
MakeFileName		=	{LibraryFileName}.make

#####################################
#	Default compiler options for Diab Data
#####################################
CBaseOptions =	-I "{3doincludes}" 				# include path.  Be careful of �
				-I"{SubscriberDir}EZFlixSubscriber:" # for EZFlixDecoder.h	�
#				-I{3doincludes}audio:			# the new heirarchical �
#				-I{3doincludes}file:			# includes! �
#				-I{3doincludes}graphics:		# �
#				-I{3doincludes}hardware:		# �
#				-I{3doincludes}international:	# �
#				-I{3doincludes}streaming:		# �
#				-I{3doincludes}kernel:			# �
#				-I{3doincludes}lib3DO:			# �
#				-I{3doincludes}misc:			# �
				-c								# MANDATORY		only run the c compiler, don't exec the linker �
				-WDDTARGET=PPC602				# MANDATORY		Tells the compiler you're building for the 602 (which is 603 safe) �
				-Xstring-align=1				# RECOMMENDED	word aligns strings �
				-Xdouble-warning -Xuse-float	# RECOMMENDED	produces single precision instructions/constants �
				-Ximport						# RECOMMENDED forces headers to be included only once �
				-Xstrict-ansi					# RECOMMENDED unless you are building code to be released, then its MANDATORY ;-) �
				-Xunsigned-char					# ? forces string constants to be unsigned char? �
				-XO -Xunroll=1 -Xtest-at-bottom	# ? Optimizations on, unroll loops (but not too aggressively), test loop conditional at bottom �
				-Xextend-args=0					# ? �
				-Xforce-prototypes				# RECOMMENDED �
				-Xinline=5						# ? 5 levels of inline/macro expansion �
				-Xno-libc-inlining -Xno-recognize-lib	# MANDATORY(?) Don't use any compiler built in functions/inlines �
				-Xno-bss=2						# MANDATORY(?) �
				-Xlint=0x10						# RECOMMENDED lovely commentary on your code quality �
				-Xtrace-table=0					# ? Don't generate traceback tables.  This is like macsbug names for you macheads
#
CSymOption		= -g -DDEBUG

LOptions	= -D -n -r

# Note: This creates library file from scratch. To replace files have
# been updated, use the -rcu option.  Using -rcu option may be slower
# than creating the library from scratch.
LibOptions      = -cq       # Quickly appends files and don't display
                            # any message when a new archive name
                            # is created.

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################
OBJECTS			=	"{ObjectDir}{library}.c.o"				�
					"{ObjectDir}EZFlixDecoderFrame.c.o"		�
					"{ObjectDir}EZFlixDecodeSymbols.c.o"	�
					"{ObjectDir}EZFlixXPlat.c.o"

OBJECTDEPENDS	=	"{library}.c.depends"				�
					"EZFlixDecoderFrame.c.depends"		�
					"EZFlixDecodeSymbols.c.depends"		�
					"EZFlixXPlat.c.depends"

#####################################
#	Default build rules
#####################################
All				�	{Library}.a

{ObjectDir}		�	:

.c.o			�	.c {MakeFileName}
	if `Exists -d -q "{RAMDISKDIR}"`
		directory "{RAMDISKDIR}"
	end
	if ( {DebugFlag} )
		echo "	compiling {Default}.c with {CSymOption}"
		{CC} {CBaseOptions} {CSymOption} -o {TargDir}{Default}.c.o {BaseDir}{DepDir}{Default}.c
	else
		echo "	compiling {Default}.c"
		{CC} {CBaseOptions} -o {TargDir}{Default}.c.o {BaseDir}{DepDir}{Default}.c
	end
#
.c.depends		�	.c
	set BaseDir "`directory`"
	{CC} {CBaseOptions} {DepDir}{Default}.c -E -H > Dev:Null � "c.includefiles"
	search -i -q -r -ns "{3DOIncludes}" "c.includefiles" �
		| StreamEdit -e "1,$ Replace /�(?)�1/ '�"�{ObjectDir�}�""{Default}.c.o"	�	'�1; Replace /{BaseDir}/ ':'" �
		>> "{MakeFileName}"
	search -i -q -ns "{3DOIncludes}Streaming:" "c.includefiles" �
		| StreamEdit -e "1,$ Replace /�(?)�1/ '�"�{ObjectDir�}�""{Default}.c.o"	�	'�1; Replace /{3DOIncludes}Streaming:/ '�"�{3DOIncludes�}�"Streaming:'" �
		>> "{MakeFileName}"
	delete -i "c.includefiles"

#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the �s, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					�	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	�
	Open "{MakeFileName}"
	Find � "{MakeFileName}"
	Find /�#�tInclude file dependencies �(Don�t change this line or put anything after this section.�)�/ "{MakeFileName}"
	Find /�[�#]/  "{MakeFileName}"
	Replace Ƥ:� "�n" "{MakeFileName}"

SaveNewMakefile			�
	Save "{MakeFileName}"

#####################################
#	Target build rules
#####################################
{LibraryFileName}					�	"{Destination}"{LibraryFileName}
"{Destination}"{LibraryFileName}	�	{MakeFileName} {OBJECTS}
	delete -i "{Targ}"
	{LIBRARIAN}	{LibOptions} "{Targ}" {OBJECTS}
	
#####################################
#	Include file dependencies (Don�t change this line or put anything after this section.)
#####################################

