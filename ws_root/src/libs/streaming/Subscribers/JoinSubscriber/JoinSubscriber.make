#####################################
##
##  @(#) JoinSubscriber.make 95/11/22 1.10
##
#####################################
#
#   File:       JoinSubscriber.make
#
#	Contains:	makefile for building JoinSubscriber
#
#	To regenerate the .c.o -> .h file dependencies, get write access to this
#	make file and execute the following MPW code:
#	make Depends -f JoinSubscriber.make ³³ "{worksheet}" > temp.makeout; temp.makeout ³³ "{worksheet}"; delete -i temp.makeout
#
#####################################


#####################################
#		Symbol definitions
#####################################
FileName			=	JoinSubscriber
DebugFlag		=	0
CC				=	dcc

StreamDir		=	":::"
SubscriberDir	=	"::"

ObjectDir		=	{SubscriberDir}Objects:

MakeFileName	=	"{FileName}.make"

#####################################
#	Default compiler options
#####################################
CDependsOptions = -I"{3doincludes}" 			# include path.  Be careful of ¶
				-c								# MANDATORY		only run the c compiler, don't exec the linker ¶
				-WDDTARGET=PPC602				# MANDATORY		Tells the compiler you're building for the 602 (which is 603 safe) ¶
				-Ximport						# RECOMMENDED forces headers to be included only once ¶
				-Xstrict-ansi					# RECOMMENDED unless you are building code to be released, then its MANDATORY ;-)

COptions		= {gCBaseOptions} {gCBaseOptions1}

#####################################
#	Object files
#	Be sure to keep these two definitions in synch!
#####################################

OBJECTS			=	"{ObjectDir}JoinSubscriber.c.o"

OBJECTDEPENDS	=	"JoinSubscriber.c.depends"

#####################################
#	Default build rules
#####################################
All				Ä	{OBJECTS}

{ObjectDir}		Ä	:

{FileName}		Ä	{OBJECTS}

# Target dependancy to rebuild when makefile or build script changes
.c.o			Ä	.c {MakeFileName}
    if `Exists -d -q "{RAMDISKDIR}"`
        directory "{RAMDISKDIR}"
    end
# 
	if ( {DebugFlag} && ( {Default} == JoinSubscriber) )
		echo "	compiling {Default}.c with {gCSymOption}"
		{CC} {gCSymOption} {COptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	else
		echo "	compiling {Default}.c"
		{CC} {COptions} {gCOptimizeOptions} {gCInLineOption} -o {TargDir}{Default}.c.o {DepDir}{Default}.c
	end
	
.c.depends		Ä	.c
	set BaseDir "`directory`"
	{CC} {CDependsOptions} {DepDir}{Default}.c -E -H > Dev:Null ³ "c.includefiles"
	search -i -q -r -ns "{3DOIncludes}" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{BaseDir}/ ':'" ¶
		>> "{MakeFileName}"
	search -i -q -ns "{3DOIncludes}Streaming:" "c.includefiles" ¶
		| StreamEdit -e "1,$ Replace /¥(?)¨1/ '¶"¶{ObjectDir¶}¶""{Default}.c.o"	Ä	'¨1; Replace /{3DOIncludes}Streaming:/ '¶"¶{3DOIncludes¶}¶"Streaming:'" ¶
		>> "{MakeFileName}"
	delete -i "c.includefiles"
 
#####################################
#	Dependency re-building rules
#	The .c.depends rule asks the compiler to generate source file dependencies, then
#	removes the first line (.c.o dependency on .c), substitutes a symbolic reference
#	to "{ObjectDir}", puts in a tab before the Äs, and appends the result to this make
#	file. The following rules setup and sequence the work.
#
#	HOW TO USE IT: Get write access to this make file then make "Depends".
#	This will replace the include file dependencies lines at the end of this makefile.
#####################################
Depends					Ä	DeleteOldDependencies {ObjectDepends} SaveNewMakefile

DeleteOldDependencies	Ä
	Open "{MakeFileName}"
	Find ¥ "{MakeFileName}"
	Find /¥#¶tInclude file dependencies ¶(DonÕt change this line or put anything after this section.¶)°/ "{MakeFileName}"
	Find /¥[Â#]/ "{MakeFileName}"
	Replace Æ¤:° "¶n" "{MakeFileName}"

SaveNewMakefile			Ä
	Save "{MakeFileName}"

#####################################
#	Include file dependencies (DonÕt change this line or put anything after this section.)
#####################################

