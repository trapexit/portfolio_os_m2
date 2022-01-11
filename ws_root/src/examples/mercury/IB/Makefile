
LINK_LIBS = clt eventbroker blitter_utils clt spmath c
MODULES = iff graphics gstate blitter frame2d audiopatchfile audiopatch font

#EXAMPLE_CLASS =	Mercury/Examples/anim

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec

#ALL_LINK_LIBS += \
#               ../../lib/libmercury1/${OBJDIR}/libmercury1 \
#               ../../lib/libmercury2/${OBJDIR}/libmercury2 \
#               ../../lib/libmercury3/${OBJDIR}/libmercury3 \
#               ../../lib/libmercury4/${OBJDIR}/libmercury4 \
#               ../../lib/libmercury_utils/${OBJDIR}/libmercury_utils

ALL_LINK_LIBS += \
                ../../../libs/mercury/libmercury1/${OBJDIR}/libmercury1 \
                ../../../libs/mercury/libmercury2/${OBJDIR}/libmercury2 \
                ../../../libs/mercury/libmercury3/${OBJDIR}/libmercury3 \
                ../../../libs/mercury/libmercury4/${OBJDIR}/libmercury4 \
                ../../../libs/mercury/libmercury_utils/${OBJDIR}/libmercury_utils \
		${REFERENCE_ROOT}/libs/${OPTIONS_PATH}/libspmath.a \
		${LINK_LIB_DIR}/libc.a

CINCLUDES += -I../../include -I${SRC}/includes/mercury -DGSTATE_PRIVATE

AS = ppcas
SFLAGS = -m602 -I../../include -I${SRC}/includes/mercury 
OPTIONS_FLAGS += -DVERBOSE

#LIBSETUP = ../../lib/libmercury_setup/${OBJDIR}
LIBSETUP = ../../../libs/mercury/libmercury_setup/${OBJDIR}

OBJS += \
	${LIBSETUP}/M_SetupDynLit.co \
	${LIBSETUP}/M_SetupDynLitFog.co \
	${LIBSETUP}/M_SetupDynLitTex.co \
	${LIBSETUP}/M_SetupDynLitEnv.co \
	${LIBSETUP}/M_SetupDynLitTransEnv.co \
	${LIBSETUP}/M_SetupDynLitSpecEnv.co \
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









