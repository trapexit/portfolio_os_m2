#   This Makefile will build the sources to the Mercury lib

OBJECTS = ¶
		:objects:M_Draw.c.o ¶
		:objects:M_Draw.s.o ¶
		:objects:M_DrawEnd.c.o ¶

# Choose one of the following:
# DEBUG_OPTIONS = -g # For best source-level debugging
DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5

# Choose one of the following:
USE_PPCAS = 1 # For faster compilation
# USE_PPCAS = 0

asm = ppcas

# variables for compiler tools
defines		=	-I::include: -I:::include: -D__3DO__ -DOS_3DO=2 -DNUPUPSIM -DMACINTOSH

dccopts		= ¶
				-c -Xstring-align=1 -Ximport -Xstrict-ansi -Xunsigned-char ¶
				{DEBUG_OPTIONS} ¶
				-Xforce-prototypes -Xlint=0x10 -Xtrace-table=0
				
asmopts		= ¶
				-I ::include: -I :::include:

# Define the target, "libs", and its dependencies.

libmercury1 ÄÄ libmercury1.make {OBJECTS}
	delete -i :objects:libmercury1.a
	dar cq :objects:libmercury1.a {OBJECTS}

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: Ä :

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o Ä .c libmercury1.make
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

.s.o	Ä	.s
	{asm} {asmopts} {depDir}{default}.s -o {targDir}{default}.s.o

