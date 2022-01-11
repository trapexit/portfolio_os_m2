#  @(#) Makefile 96/09/27 1.45

LINK_LIBS = eventbroker clt spmath c
MODULES = graphics gstate

EXAMPLE_CLASS =	Mercury/Examples/bigcirclempm

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec

include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec


ALL_LINK_LIBS += \
		../../../libs/mercury/libmercury_mpm/${OBJDIR}/libmercury_mpm \
		../../../libs/mercury/libmercury1/${OBJDIR}/libmercury1 \
		../../../libs/mercury/libmercury2/${OBJDIR}/libmercury2 \
		../../../libs/mercury/libmercury3/${OBJDIR}/libmercury3 \
		../../../libs/mercury/libmercury4/${OBJDIR}/libmercury4 \
		../../../libs/mercury/libmercury_utils/${OBJDIR}/libmercury_utils \
		${LINK_LIB_DIR}/libspmath.a ${LINK_LIB_DIR}/libc.a


CINCLUDES += -I../bigcircle -I../helloworld -I${SRC}/includes/mercury  -DGSTATE_PRIVATE

AS = ppcas
SFLAGS = -I../../include -I${SRC}/includes/mercury

BC = ../bigcircle/${OBJDIR}

LIBSETUP = ../../../libs/mercury/libmercury_setup/${OBJDIR}

OBJS += \
	${BC}/gamedata.co \
	${BC}/gamedata2.co \
	${BC}/tex_read.co \
	${LIBSETUP}/M_SetupDynLit.co \
	${LIBSETUP}/M_SetupDynLitFog.co \
	${LIBSETUP}/M_SetupDynLitTex.co \
	${LIBSETUP}/M_SetupDynLitSpecTex.co \
	${LIBSETUP}/M_SetupDynLitFogTex.co \
	${LIBSETUP}/M_SetupPreLit.co \
	${LIBSETUP}/M_SetupPreLitFog.co \
	${LIBSETUP}/M_SetupPreLitTex.co \
	${LIBSETUP}/M_SetupPreLitFogTex.co \
	${LIBSETUP}/M_SetupDynLitTrans.co \
	${LIBSETUP}/M_SetupDynLitTransTex.co \
	${LIBSETUP}/M_SetupDynLitFogTrans.co \
	${LIBSETUP}/M_SetupPreLitTrans.co \
	${LIBSETUP}/M_SetupPreLitTransTex.co \
	${LIBSETUP}/M_SetupPreLitFogTrans.co


