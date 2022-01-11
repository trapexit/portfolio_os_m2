# @(#) Makefile 96/12/09 1.51

ifdef STATISTICS
STATDEF = -DSTATISTICS=1
endif

LINK_LIBS = eventbroker clt spmath c
MODULES = graphics gstate

# DELIVER_THESE_FILES = regress regress.mp regress.cube regress.hires32

EXAMPLE_CLASS =	Mercury/Examples/bigcircle

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec


ALL_LINK_LIBS += \
		../../../libs/mercury/libmercury1/${OBJDIR}/libmercury1 \
		../../../libs/mercury/libmercury2/${OBJDIR}/libmercury2 \
		../../../libs/mercury/libmercury3/${OBJDIR}/libmercury3 \
		../../../libs/mercury/libmercury4/${OBJDIR}/libmercury4 \
		../../../libs/mercury/libmercury_utils/${OBJDIR}/libmercury_utils \
		${LINK_LIB_DIR}/libspmath.a ${LINK_LIB_DIR}/libc.a

CINCLUDES += -I../../include -I../helloworld -I${SRC}/includes/mercury  -DGSTATE_PRIVATE ${STATDEF}

#AS = ppcas
AS = ${HOSTOSBIN}/ppcas

SFLAGS = -I../../include -I${SRC}/includes/mercury

LIBSETUP = ../../../libs/mercury/libmercury_setup/${OBJDIR}

OBJS += \
	${LIBSETUP}/M_SetupDynLit.co \
	${LIBSETUP}/M_SetupDynLitFog.co \
	${LIBSETUP}/M_SetupDynLitTex.co \
	${LIBSETUP}/M_SetupDynLitEnv.co \
	${LIBSETUP}/M_SetupDynLitSpecTex.co \
	${LIBSETUP}/M_SetupDynLitSpecEnv.co \
	${LIBSETUP}/M_SetupDynLitFogTex.co \
	${LIBSETUP}/M_SetupDynLitFogEnv.co \
	${LIBSETUP}/M_SetupPreLit.co \
	${LIBSETUP}/M_SetupPreLitFog.co \
	${LIBSETUP}/M_SetupPreLitTex.co \
	${LIBSETUP}/M_SetupPreLitFogTex.co \
	${LIBSETUP}/M_SetupDynLitAA.co \
	${LIBSETUP}/M_SetupDynLitTexAA.co \
	${LIBSETUP}/M_SetupPreLitAA.co \
	${LIBSETUP}/M_SetupPreLitTexAA.co \
	${LIBSETUP}/M_SetupDynLitTrans.co \
	${LIBSETUP}/M_SetupDynLitTransTex.co \
	${LIBSETUP}/M_SetupDynLitTransEnv.co \
	${LIBSETUP}/M_SetupDynLitFogTrans.co \
	${LIBSETUP}/M_SetupPreLitTrans.co \
	${LIBSETUP}/M_SetupPreLitTransTex.co \
	${LIBSETUP}/M_SetupPreLitFogTrans.co

SANITIZE += -UALLOW_EXTRA_LIST -USTATISTICS
