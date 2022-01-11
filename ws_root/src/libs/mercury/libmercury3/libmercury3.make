#   This Makefile will build the sources to the Mercury lib

OBJECTS = ¶
		:objects:M_DrawDynLit.s.o ¶
		:objects:M_DrawDynLitEnv.s.o ¶
		:objects:M_DrawDynLitTex.s.o ¶
		:objects:M_DrawDynLitTrans.s.o ¶
		:objects:M_DrawPreLit.s.o ¶
		:objects:M_DrawPreLitTex.s.o ¶
		:objects:M_DrawPreLitTrans.s.o ¶
		:objects:M_BoundsTest.s.o ¶
		:objects:M_PreLight.s.o ¶
		:objects:M_BuildAAEdgeTable.c.o ¶
		:objects:M_DrawDynLitAA.s.o ¶
		:objects:M_DrawDynLitTexAA.s.o ¶
		:objects:M_DrawLineAA.s.o ¶
		:objects:M_DrawLineTexAA.s.o ¶
		:objects:M_DrawPreLitAA.s.o ¶
		:objects:M_DrawPreLitTexAA.s.o

# Choose one of the following:
# DEBUG_OPTIONS = -g # For best source-level debugging
DEBUG_OPTIONS = -XO -Xunroll=1 -Xtest-at-bottom -Xinline=5

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

libmercury3 ÄÄ libmercury3.make {OBJECTS}
	delete -i :objects:libmercury3.a
	dar cq :objects:libmercury3.a {OBJECTS}

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: Ä :

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o		Ä	.c
	dcc {defines} {dccopts} {depDir}{default}.c -o {targDir}{default}.c.o

.s.o	Ä	.s
	{asm} {asmopts} {depDir}{default}.s -o {targDir}{default}.s.o

