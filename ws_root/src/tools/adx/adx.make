# @(#) adx.make 95/09/22 1.2
# Macintosh makefile for ADX
#
#   Sources:    411.c
#               adx.c
#               ascii.c
#               html.c
#               troff.c


copts = -d macintosh
ppcopts =  -w conformance -strict on {copts} -appleext on -sym on
scopts =  -r {copts} -sym on -i {cincludes}
objects = :Objects:

POWERPCOBJECTS = ¶
		{objects}411.o ¶
		{objects}adx.o ¶
		{objects}ascii.o ¶
		{objects}html.o ¶
		{objects}troff.o ¶
		{objects}strcasecmp.o ¶
		{objects}strncasecmp.o ¶
		{objects}list.o ¶
		{objects}utils.o ¶

68KOBJECTS = ¶
		{objects}411.c.o ¶
		{objects}adx.c.o ¶
		{objects}ascii.c.o ¶
		{objects}html.c.o ¶
		{objects}troff.c.o ¶
		{objects}strcasecmp.c.o ¶
		{objects}strncasecmp.c.o ¶
		{objects}list.c.o ¶
		{objects}utils.c.o ¶

AutoDocs ÄÄ _ObjFolder

_ObjFolder Ä
	Set Exit 0
	NewFolder "Objects"
	Set Exit 1
	Echo "This file is a fake dependency for creating the Objects folder" > _ObjFolder

AutoDocs ÄÄ AutoDocs.make  {POWERPCOBJECTS}
	PPCLink  -sym on  -outputformat xcoff ¶
		{POWERPCOBJECTS} ¶
		"{PPCLibraries}"InterfaceLib.xcoff ¶
		"{PPCLibraries}"MathLib.xcoff ¶
		"{PPCLibraries}"StdCLib.xcoff ¶
		"{PPCLibraries}"StdCRuntime.o ¶
		"{PPCLibraries}"PPCCRuntime.o ¶
		"{PPCLibraries}"PPCToolLibs.o ¶
		-main __start ¶
		-o {objects}AutoDocs.xcoff
	makePEF {objects}AutoDocs.xcoff -o AutoDocs ¶
		-l InterfaceLib.xcoff=InterfaceLib ¶
		-l MathLib.xcoff=MathLib ¶
		-l "StdCLib.xcoff=StdCLib#`evaluate 0x01104003`" ¶
		-ft MPST -fc 'MPS '
	MakeSYM  {objects}AutoDocs.xcoff -o AutoDocs.xSYM
{objects}411.o Ä AutoDocs.make 411.c adx.h
	 PPCC {ppcopts} 411.c -o {targ}
{objects}adx.o Ä AutoDocs.make adx.c adx.h options.h
	 PPCC {ppcopts} adx.c -o {targ}
{objects}ascii.o Ä AutoDocs.make ascii.c adx.h
	 PPCC {ppcopts} ascii.c -o {targ}
{objects}html.o Ä AutoDocs.make html.c adx.h options.h
	 PPCC {ppcopts} html.c -o {targ}
{objects}troff.o Ä AutoDocs.make troff.c adx.h options.h
	 PPCC {ppcopts} troff.c -o {targ}
{objects}strcasecmp.o Ä AutoDocs.make strcasecmp.c
	 PPCC {ppcopts} strcasecmp.c -o {targ}
{objects}strncasecmp.o Ä AutoDocs.make strncasecmp.c
	 PPCC {ppcopts} strncasecmp.c -o {targ}
{objects}list.o Ä AutoDocs.make list.c
	 PPCC {ppcopts} list.c -o {targ}
{objects}nodes.o Ä AutoDocs.make nodes.c
	 PPCC {ppcopts} list.c -o {targ}
{objects}utils.o Ä AutoDocs.make utils.c
	 PPCC {ppcopts} utils.c -o {targ}

AutoDocs ÄÄ AutoDocs.make  {68KOBJECTS}
	Link -t MPST -c 'MPS ' -sym on -mf ¶
		{68KOBJECTS} ¶
		"{Libraries}IntEnv.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}Interface.o" ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}ToolLibs.o" ¶
		-o AutoDocs
{objects}411.c.o Ä AutoDocs.make 411.c adx.h
	 SC {scopts} 411.c -o {targ}
{objects}adx.c.o Ä AutoDocs.make adx.c adx.h options.h
	 SC {scopts} adx.c -o {targ}
{objects}ascii.c.o Ä AutoDocs.make ascii.c adx.h
	 SC {scopts} ascii.c -o {targ}
{objects}html.c.o Ä AutoDocs.make html.c adx.h options.h
	 SC {scopts} html.c -o {targ}
{objects}troff.c.o Ä AutoDocs.make troff.c adx.h options.h
	 SC {scopts} troff.c -o {targ}
{objects}strcasecmp.c.o Ä AutoDocs.make strcasecmp.c
	 SC {scopts} strcasecmp.c -o {targ}
{objects}strncasecmp.c.o Ä AutoDocs.make strncasecmp.c
	 SC {scopts} strncasecmp.c -o {targ}
{objects}list.c.o Ä AutoDocs.make list.c
	 SC {scopts} list.c -o {targ}
{objects}nodes.c.o Ä AutoDocs.make nodes.c
	 SC {scopts} nodes.c -o {targ}
{objects}utils.c.o Ä AutoDocs.make utils.c
	 SC {scopts} utils.c -o {targ}
