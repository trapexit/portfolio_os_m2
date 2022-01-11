#   This Makefile will build the sources to the Mercury lib

OBJECTS = ¶
	:objects:Matrix_BillboardX.c.o ¶
	:objects:Matrix_BillboardY.c.o ¶
	:objects:Matrix_BillboardZ.c.o ¶
	:objects:Matrix_Construct.c.o ¶
	:objects:Matrix_Copy.c.o ¶
	:objects:Matrix_CopyOrientation.c.o ¶
	:objects:Matrix_CopyTranslation.c.o ¶
	:objects:Matrix_Destruct.c.o ¶
	:objects:Matrix_FullInvert.c.o ¶
	:objects:Matrix_GetBank.c.o ¶
	:objects:Matrix_GetElevation.c.o ¶
	:objects:Matrix_GetHeading.c.o ¶
	:objects:Matrix_GetpTranslation.c.o ¶
	:objects:Matrix_GetTranslation.c.o ¶
	:objects:Matrix_Identity.c.o ¶
	:objects:Matrix_Invert.c.o ¶
	:objects:Matrix_LookAt.c.o ¶
	:objects:Matrix_Move.c.o ¶
	:objects:Matrix_MoveByVector.c.o ¶
	:objects:Matrix_Mult.c.o ¶
	:objects:Matrix_Multiply.c.o ¶
	:objects:Matrix_MultiplyOrientation.c.o ¶
	:objects:Matrix_MultOrientation.c.o ¶
	:objects:Matrix_Normalize.c.o ¶
	:objects:Matrix_Perspective.c.o ¶
	:objects:Matrix_Print.c.o ¶
	:objects:Matrix_Rotate.c.o ¶
	:objects:Matrix_RotateLocal.c.o ¶
	:objects:Matrix_RotateX.c.o ¶
	:objects:Matrix_RotateXLocal.c.o ¶
	:objects:Matrix_RotateY.c.o ¶
	:objects:Matrix_RotateYLocal.c.o ¶
	:objects:Matrix_RotateZ.c.o ¶
	:objects:Matrix_RotateZLocal.c.o ¶
	:objects:Matrix_Scale.c.o ¶
	:objects:Matrix_ScaleByVector.c.o ¶
	:objects:Matrix_ScaleLocal.c.o ¶
	:objects:Matrix_ScaleLocalByVector.c.o ¶
	:objects:Matrix_SetTranslation.c.o ¶
	:objects:Matrix_SetTranslationByVec.c.o ¶
	:objects:Matrix_Translate.c.o ¶
	:objects:Matrix_TranslateByVector.c.o ¶
	:objects:Matrix_Turn.c.o ¶
	:objects:Matrix_TurnLocal.c.o ¶
	:objects:Matrix_TurnX.c.o ¶
	:objects:Matrix_TurnXLocal.c.o ¶
	:objects:Matrix_TurnY.c.o ¶
	:objects:Matrix_TurnYLocal.c.o ¶
	:objects:Matrix_TurnZ.c.o ¶
	:objects:Matrix_TurnZLocal.c.o ¶
	:objects:Matrix_Zero.c.o ¶
	:objects:MatrixData.c.o ¶
	:objects:Vector2D_Set.c.o ¶
	:objects:Vector3D_Add.c.o ¶
	:objects:Vector3D_Average.c.o ¶
	:objects:Vector3D_Compare.c.o ¶
	:objects:Vector3D_CompareFuzzy.c.o ¶
	:objects:Vector3D_Construct.c.o ¶
	:objects:Vector3D_Copy.c.o ¶
	:objects:Vector3D_Cross.c.o ¶
	:objects:Vector3D_Destruct.c.o ¶
	:objects:Vector3D_Dot.c.o ¶
	:objects:Vector3D_Length.c.o ¶
	:objects:Vector3D_Maximum.c.o ¶
	:objects:Vector3D_Minimum.c.o ¶
	:objects:Vector3D_Multiply.c.o ¶
	:objects:Vector3D_MultiplyByMatrix.c.o ¶
	:objects:Vector3D_Negate.c.o ¶
	:objects:Vector3D_Normalize.c.o ¶
	:objects:Vector3D_OrientateByMatrix.c.o ¶
	:objects:Vector3D_Print.c.o ¶
	:objects:Vector3D_Scale.c.o ¶
	:objects:Vector3D_Set.c.o ¶
	:objects:Vector3D_Subtract.c.o ¶
	:objects:Vector3D_Zero.c.o ¶

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

libmercury_utils ÄÄ libmercury_utils.make {OBJECTS}
	delete -i :objects:libmercury_utils.a
	dar cq :objects:libmercury_utils.a {OBJECTS}

#  files in the objects sub-dir are dependent upon sources in the current dir
:objects: Ä :

# .c.o files are dependent only upon the .c source (This is not very complete but good enough for now)
.c.o Ä .c libmercury_utils.make
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

