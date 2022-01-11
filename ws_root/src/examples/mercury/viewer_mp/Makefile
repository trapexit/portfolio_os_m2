# @(#) Makefile 96/10/30 1.1

LINK_LIBS = eventbroker clt spmath c
MODULES = graphics gstate

EXAMPLE_CLASS =	Mercury/Examples/viewer_mp

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec

include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec

CINCLUDES += -I../../include -I../helloworld -I../viewer -I${SRC}/includes/mercury

LIBSETUP = ../../../libs/mercury/libmercury_setup/${OBJDIR}



ALL_LINK_LIBS += \
		../../../libs/mercury/libmercury1/${OBJDIR}/libmercury1 \
		../../../libs/mercury/libmercury2/${OBJDIR}/libmercury2 \
		../../../libs/mercury/libmercury3/${OBJDIR}/libmercury3 \
		../../../libs/mercury/libmercury4/${OBJDIR}/libmercury4 \
		../../../libs/mercury/libmercury_utils/${OBJDIR}/libmercury_utils \
		${CODEMGR_WS}/ws_root/libs/${OPTIONS_PATH}/libspmath.a \
		${LINK_LIB_DIR}/libc.a

OBJS += \
	../helloworld/${OBJDIR}/bsdf_read.co \
	../helloworld/${OBJDIR}/tex_read.co \
	../helloworld/${OBJDIR}/filepod.co \
	../helloworld/${OBJDIR}/graphicsenv.co \
	../helloworld/${OBJDIR}/scalemodel.co \
	../viewer/${OBJDIR}/controller.co \
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

