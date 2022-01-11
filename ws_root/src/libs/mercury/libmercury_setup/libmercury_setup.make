#   This Makefile will build the sources to the Mercury lib

OBJECTS = ¶
		:objects:M_SetupDynLit.c.o  ¶
		:objects:M_SetupDynLitFog.c.o  ¶
		:objects:M_SetupDynLitTex.c.o  ¶
		:objects:M_SetupDynLitFogTex.c.o  ¶
		:objects:M_SetupDynLitSpecTex.c.o  ¶
		:objects:M_SetupDynLitEnv.c.o  ¶
		:objects:M_SetupDynLitFogEnv.c.o  ¶
		:objects:M_SetupDynLitSpecEnv.c.o  ¶
		:objects:M_SetupPreLit.c.o  ¶
		:objects:M_SetupPreLitFog.c.o  ¶
		:objects:M_SetupPreLitTex.c.o  ¶
		:objects:M_SetupPreLitFogTex.c.o  ¶
		:objects:M_SetupDynLitTrans.c.o  ¶
		:objects:M_SetupDynLitFogTrans.c.o  ¶
		:objects:M_SetupDynLitTransTex.c.o  ¶
		:objects:M_SetupDynLitTransEnv.c.o  ¶
		:objects:M_SetupPreLitTrans.c.o  ¶
		:objects:M_SetupPreLitFogTrans.c.o  ¶
		:objects:M_SetupPreLitTransTex.c.o  ¶
		:objects:M_SetupDynLitAA.c.o ¶
		:objects:M_SetupDynLitTexAA.c.o ¶
		:objects:M_SetupPreLitAA.c.o ¶
		:objects:M_SetupPreLitTexAA.c.o 

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

libmercury_setup ÄÄ libmercury_setup.make {OBJECTS}

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: Ä :

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o Ä .c libmercury_setup.make
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

