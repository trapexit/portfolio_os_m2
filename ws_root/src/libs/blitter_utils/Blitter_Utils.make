#
#  @(#) Blitter_Utils.make 96/11/27 1.1
#
#   This Makefile will build the sources to the Blitter_Utils

OBJECTS = ?
		:objects:blend.c.o ?
		:objects:dimensions.c.o ?
		:objects:mask.c.o ?
		:objects:rectangle.c.o ?
		:objects:scroll.c.o ?
		:objects:utf.c.o ?
		:objects:utfattr.c.o

# Choose one of the following:
# DEBUG_OPTIONS = -g # For best source-level debugging
DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5

# Choose one of the following:
USE_PPCAS = 1 # For faster compilation
# USE_PPCAS = 0

asm = ppcas

# variables for compiler tools
defines		=	-I::include: -I:::include: -D__3DO__ -DOS_3DO=2 -DNUPUPSIM -DMACINTOSH

dccopts		= ?
				-c -Xstring-align=1 -Ximport -Xstrict-ansi -Xunsigned-char ?
				{DEBUG_OPTIONS} ?
				-Xforce-prototypes -Xlint=0x10 -Xtrace-table=0
				
asmopts		= ?
				-I ::include: -I :::include:

# Define the target, "libs", and its dependencies.

Blitter_Utils ?? Blitter_Utils.make {OBJECTS}
	delete -i :objects:Blitter_Utils.a
	dar cq :objects:Blitter_Utils.a {OBJECTS}

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: ? :

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o ? .c Blitter_Utils.make
	Set Echo 0
	Echo "# ---- Compiling {depDir}{default}.c"
	if {USE_PPCAS}
		Set Echo 1
		dcc {depDir}{default}.c {defines} {dccopts} -Xdebug-mode=6 -S -o "{TempFolder}"{default}.s
		ppcas "{TempFolder}"{default}.s -o {targDir}{default}.c.o
		delete -i "{TempFolder}"{default}.s
		Set Echo 0
	else
		Set Echo 1
		dcc {depDir}{default}.c {defines} {dccopts} -o {targDir}{default}.c.o
		Set Echo 0
	end if
		Set Echo 1

.s.o	?	.s
	{asm} {asmopts} {depDir}{default}.s -o {targDir}{default}.s.o

